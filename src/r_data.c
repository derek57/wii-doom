// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:
//        Preparation of data for rendering,
//        generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------


#include <stdio.h>

#include "c_io.h"
#include "deh_main.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_data.h"
#include "r_local.h"
#include "r_sky.h"
#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"


//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
// 



//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
typedef struct
{
    short        originx;
    short        originy;
    short        patch;
    short        stepdir;
    short        colormap;
} PACKEDATTR mappatch_t;


//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//
typedef struct
{
    char         name[8];
    int          masked;        
    short        width;
    short        height;
    int          obsolete;
    short        patchcount;
    mappatch_t   patches[1];
} PACKEDATTR maptexture_t;


// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.
typedef struct
{
    // Block origin (allways UL),
    // which has allready accounted
    // for the internal origin of the patch.
    short        originx;        
    short        originy;
    int          patch;
} texpatch_t;


// A maptexturedef_t describes a rectangular texture,
//  which is composed of one or more mappatch_t structures
//  that arrange graphic patches.

typedef struct texture_s texture_t;

struct texture_s
{
    // Keep name for switch changing, etc.
    char         name[8];                
    short        width;
    short        height;

    // Index in textures list

    int          index;

    // Next in hash table chain

    texture_t    *next;
    
    // All the patches[patchcount]
    //  are drawn back to front into the cached texture.
    short        patchcount;
    texpatch_t   patches[1];                
};


enum {
    r, g, b
} rgb_t;


int              firstflat;
int              lastflat;
int              numflats;

int              firstpatch;
int              lastpatch;
int              numpatches;

int              firstspritelump;
int              lastspritelump;
int              numspritelumps;

int              numtextures;
int              *texturewidthmask;
int              *texturecompositesize;

int              flatmemory;
int              texturememory;
int              spritememory;

// for global animation
int              *flattranslation;
int              *texturetranslation;

int              tran_filter_pct = 66;       // filter percent

lighttable_t     *colormaps;

texture_t**      textures;
texture_t**      textures_hashtable;

// needed for texture pegging
fixed_t*         textureheight;                

// needed for pre rendering
fixed_t*         spritewidth;
fixed_t*         spriteheight;
fixed_t*         spriteoffset;
fixed_t*         spritetopoffset;

short**          texturecolumnlump;

unsigned**       texturecolumnofs; // fix Medusa bug
unsigned**	 texturecolumnofs2; // original column offsets for single-patched textures

byte**           texturecomposite;

extern byte*     tranmap;

//
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.
//



//
// R_DrawColumnInCache
// Clip and draw a column
//  from a patch into a cached post.
//
static void R_DrawColumnInCache(const column_t *patch, byte *cache,
                                int originy, int cacheheight, byte *marks)
{
    while (patch->topdelta != 0xff)
    {
        int count = patch->length;
        int position = originy + patch->topdelta;

        if (position < 0)
        {
            count += position;
            position = 0;
        }

        if (position + count > cacheheight)
            count = cacheheight - position;

        if (count > 0)
        {
            memcpy (cache + position, (byte *)patch + 3, count);

            // killough 4/9/98: remember which cells in column have been drawn,
            // so that column can later be converted into a series of posts, to
            // fix the Medusa bug.
            memset (marks + position, 0xff, count);
        }
        patch = (column_t *)((byte *) patch + patch->length + 4);
    }
}


//
// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
static void R_GenerateComposite(int texnum)
{
    byte *block = Z_Malloc(texturecompositesize[texnum], PU_STATIC,
                           (void **) &texturecomposite[texnum]);

    texture_t *texture = textures[texnum];

    // Composite the columns together.
    texpatch_t *patch = texture->patches;
    short *collump = texturecolumnlump[texnum];

    // make 32-bit
    unsigned *colofs = texturecolumnofs[texnum];
    int i = texture->patchcount;

    // marks to identify transparent regions in merged textures
    byte *marks = calloc(texture->width, texture->height), *source;

    // initialize composite background to black (index 0)
    memset(block, 0, texturecompositesize[texnum]);

    for (; --i >=0; patch++)
    {
        patch_t *realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);
        int x, x1 = patch->originx, x2 = x1 + SHORT(realpatch->width);
        const int *cofs = realpatch->columnofs - x1;

        if (x1 < 0)
            x1 = 0;

        if (x2 > texture->width)
            x2 = texture->width;

        for (x = x1; x < x2 ; x++)
            // Column has multiple patches?
            // generate composites for single-patched textures as well
            // Fix medusa bug.
            R_DrawColumnInCache((column_t*)((byte*) realpatch + LONG(cofs[x])),
                              block + colofs[x], patch->originy,
                              texture->height, marks + x*texture->height);
    }

    // Next, convert multipatched columns into true columns,
    // to fix Medusa bug while still allowing for transparent regions.
    // temporary column
    source = malloc(texture->height);

    for (i=0; i < texture->width; i++)
    {
        // process only multipatched columns
        if (collump[i] == -1)
        {
            // cached column
            column_t *col = (column_t *)(block + colofs[i] - 3);
            const byte *mark = marks + i * texture->height;
            int j = 0;

            // save column in temporary so we can shuffle it around
            memcpy(source, (byte *) col + 3, texture->height);

            // reconstruct the column by scanning transparency marks
            for (;;)
            {
                unsigned len;

                // skip transparent cells
                while (j < texture->height && !mark[j])
                    j++;

                // if at end of column
                if (j >= texture->height)
                {
                    // end-of-column marker
                    col->topdelta = -1;
                    break;
                }
                // starting offset of post
                col->topdelta = j;

                // Use 32-bit len counter, to support tall 1s multipatched textures
                for (len = 0; j < texture->height && mark[j]; j++)
                    // count opaque cells
                    len++;

                // intentionally truncate length
                col->length = len;

                // copy opaque cells from the temporary back into the column
                memcpy((byte *) col + 3, source + col->topdelta, len);

                // next post
                col = (column_t *)((byte *) col + len + 4);
            }
        }
    }
    // free temporary column
    free(source);

    // free transparency marks
    free(marks);

    // Now that the texture has been built in column cache,
    // it is purgable from zone memory.
    Z_ChangeTag(block, PU_CACHE);
}


//
// R_GenerateLookup
//
static void R_GenerateLookup(int texnum)
{
    const texture_t *texture = textures[texnum];
    char texturename[9];

    // Composited texture not created yet.
    short *collump = texturecolumnlump[texnum];

    // make 32-bit
    unsigned *colofs = texturecolumnofs[texnum];

    // original column offsets
    unsigned *colofs2 = texturecolumnofs2[texnum];

    // keep count of posts in addition to patches.
    // Part of fix for medusa bug for multipatched 2s normals.
    struct
    {
        unsigned patches, posts;
    } *count = calloc(sizeof *count, texture->width);

    // First count the number of patches per column.
    const texpatch_t *patch = texture->patches;
    int i = texture->patchcount;

    texturename[8] = '\0';
    memcpy(texturename, texture->name, 8);

    while (--i >= 0)
    {
        int pat = patch->patch;
        const patch_t *realpatch = W_CacheLumpNum(pat, PU_CACHE);

        int x, x1 = patch++->originx, x2 = x1 + SHORT(realpatch->width);
        const int *cofs = realpatch->columnofs - x1;

        if (x2 > texture->width)
            x2 = texture->width;

        if (x1 < 0)
            x1 = 0;

        for (x = x1 ; x<x2 ; x++)
        {
            count[x].patches++;
            collump[x] = pat;
            colofs[x] = colofs2[x] = LONG(cofs[x])+3;
        }
    }

    // killough 4/9/98: keep a count of the number of posts in column,
    // to fix Medusa bug while allowing for transparent multipatches.
    //
    // killough 12/98:
    // Post counts are only necessary if column is multipatched,
    // so skip counting posts if column comes from a single patch.
    // This allows arbitrarily tall textures for 1s walls.
    //
    // If texture is >= 256 tall, assume it's 1s, and hence it has
    // only one post per column. This avoids crashes while allowing
    // for arbitrarily tall multipatched 1s textures.
    if (texture->patchcount > 1 && texture->height < 256)
    {
        // Warn about a common column construction bug
        // absolute column size limit
        unsigned limit = texture->height*3+3;

        // warn only if -devparm used
        int badcol = devparm;

        for (i = texture->patchcount, patch = texture->patches; --i >= 0;)
        {
            int pat = patch->patch;
            const patch_t *realpatch = W_CacheLumpNum(pat, PU_CACHE);
            int x, x1 = patch++->originx, x2 = x1 + SHORT(realpatch->width);
            const int *cofs = realpatch->columnofs - x1;

            if (x2 > texture->width)
                x2 = texture->width;
            if (x1 < 0)
                x1 = 0;

            for (x = x1 ; x<x2 ; x++)
            {
                // Only multipatched columns
                if (count[x].patches > 1)
                {
                    // applies to single-patched textures as well
                    // mark lump as multipatched

                    const column_t *col =
                        (column_t*)((byte*) realpatch+LONG(cofs[x]));
                    const byte *base = (const byte *) col;

                    // count posts
                    for (;col->topdelta != 0xff; count[x].posts++)
                    {
                        if ((unsigned)((byte *) col - base) <= limit)
                            col = (column_t *)((byte *) col + col->length + 4);
                        // warn about column construction bug
                        else
                        {
                            if (badcol)
                            {
                                badcol = 0;
                                C_Printf(CR_GOLD, "\n Warning: Texture %8.8s "
                                        "(height %d) has bad column(s)"
                                        " starting at x = %d.",
                                        texturename, texture->height, x);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    // Now count the number of columns
    //  that are covered by more than one patch.
    // Fill in the lump / offset, so columns
    //  with only a single patch are all done.
    texturecomposite[texnum] = 0;

    int x = texture->width;
    int height = texture->height;
    int csize = 0, err = 0;

    while (--x >= 0)
    {
        if (!count[x].patches)
        {
            if (devparm)
            {
                C_Printf(CR_GOLD, "\n R_GenerateLookup:"
                        " Column %d is without a patch in texture %.8s",
                        x, texturename);
            }
            else
                err = 1;
        }

        if (count[x].patches > 1)
            collump[x] = -1; // mark lump as multipatched

        // Fix Medusa bug, by adding room for column header
        // and trailer bytes for each post in merged column.
        // For now, just allocate conservatively 4 bytes
        // per post per patch per column, since we don't
        // yet know how many posts the merged column will
        // require, and it's bounded above by this limit.
        // mark lump as multipatched

        // three header bytes in a column
        colofs[x] = csize + 3;

        // add room for one extra post
        // 1 stop byte plus 4 bytes per post
        csize += 4*count[x].posts+5;

        // height bytes of texture data
        csize += height;
    }
    texturecompositesize[texnum] = csize;

    // killough 10/98: non-verbose output
    if (err)
    {
        C_Printf(CR_GOLD, "\n R_GenerateLookup: Column without a patch in texture %.8s",
                texturename);
    }
    free(count);
}



//
// R_GetColumn
//
byte*
R_GetColumn
(   int                tex,
    int                col,
    boolean            opaque )
{
    int                lump;
    int                ofs;
    int                ofs2;
        
    col &= texturewidthmask[tex];
    lump = texturecolumnlump[tex][col];
    ofs = texturecolumnofs[tex][col];
    ofs2 = texturecolumnofs2[tex][col];
    
    // single-patched mid-textures on two-sided walls
    if (lump > 0 && !opaque)
	return (byte *)W_CacheLumpNum(lump,PU_CACHE)+ofs2;

    if (!texturecomposite[tex])
        R_GenerateComposite (tex);

    return texturecomposite[tex] + ofs;
}


static void GenerateTextureHashTable(void)
{
    texture_t **rover;
    int       i;
    int       key;

    textures_hashtable 
            = Z_Malloc(sizeof(texture_t *) * numtextures, PU_STATIC, 0);

    memset(textures_hashtable, 0, sizeof(texture_t *) * numtextures);

    // Add all textures to hash table

    for (i=0; i<numtextures; ++i)
    {
        // Store index

        textures[i]->index = i;

        // Vanilla Doom does a linear search of the texures array
        // and stops at the first entry it finds.  If there are two
        // entries with the same name, the first one in the array
        // wins. The new entry must therefore be added at the end
        // of the hash chain, so that earlier entries win.

        key = W_LumpNameHash(textures[i]->name) % numtextures;

        rover = &textures_hashtable[key];

        while (*rover != NULL)
        {
            rover = &(*rover)->next;
        }

        // Hook into hash table

        textures[i]->next = NULL;
        *rover = textures[i];
    }
}


//
// R_InitTextures
// Initializes the texture list
//  with the textures from the world map.
//
void R_InitTextures (void)
{
    maptexture_t*        mtexture;
    texture_t*           texture;
    mappatch_t*          mpatch;
    texpatch_t*          patch;

    int                  i;
    int                  j;

    int*                 maptex;
    int*                 maptex2;
    int*                 maptex1;
    
    char                 name[9];
    char*                names;
    char*                name_p;
    
    int*                 patchlookup;
    
    int                  totalwidth;
    int                  nummappatches;
    int                  offset;
    int                  maxoff;
    int                  maxoff2;
    int                  numtextures1;
    int                  numtextures2;

    int*                 directory;
        
    // Load the patch names from pnames.lmp.
    name[8] = 0;
    names = W_CacheLumpName (DEH_String("PNAMES"), PU_STATIC);
    nummappatches = LONG ( *((int *)names) );
    name_p = names + 4;
    patchlookup = Z_Malloc(nummappatches*sizeof(*patchlookup), PU_STATIC, NULL);

    for (i = 0; i < nummappatches; i++)
    {
        M_StringCopy(name, name_p + i * 8, sizeof(name));
        patchlookup[i] = W_CheckNumForName(name);

        if (patchlookup[i] == -1)
            C_Printf(CR_GOLD, " Warning: patch %.8s, index %d does not exist", name, i);
    }
    W_ReleaseLumpName(DEH_String("PNAMES"));

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    maptex = maptex1 = W_CacheLumpName (DEH_String("TEXTURE1"), PU_STATIC);
    numtextures1 = LONG(*maptex);
    maxoff = W_LumpLength (W_GetNumForName (DEH_String("TEXTURE1")));
    directory = maptex+1;
        
    if (W_CheckNumForName (DEH_String("TEXTURE2")) != -1)
    {
        maptex2 = W_CacheLumpName (DEH_String("TEXTURE2"), PU_STATIC);
        numtextures2 = LONG(*maptex2);
        maxoff2 = W_LumpLength (W_GetNumForName (DEH_String("TEXTURE2")));
    }
    else
    {
        maptex2 = NULL;
        numtextures2 = 0;
        maxoff2 = 0;
    }
    numtextures = numtextures1 + numtextures2;
        
    textures = Z_Malloc (numtextures * sizeof(*textures), PU_STATIC, 0);
    texturecolumnlump = Z_Malloc (numtextures * sizeof(*texturecolumnlump), PU_STATIC, 0);
    texturecolumnofs = Z_Malloc (numtextures * sizeof(*texturecolumnofs), PU_STATIC, 0);
    texturecolumnofs2 = Z_Malloc (numtextures * sizeof(*texturecolumnofs2), PU_STATIC, 0);
    texturecomposite = Z_Malloc (numtextures * sizeof(*texturecomposite), PU_STATIC, 0);
    texturecompositesize = Z_Malloc (numtextures * sizeof(*texturecompositesize), PU_STATIC, 0);
    texturewidthmask = Z_Malloc (numtextures * sizeof(*texturewidthmask), PU_STATIC, 0);
    textureheight = Z_Malloc (numtextures * sizeof(*textureheight), PU_STATIC, 0);

    totalwidth = 0;
    
    printf("[");
    //C_Printf(CR_GRAY, "[");

    for (i=0 ; i<numtextures ; i++, directory++)
    {
        if (!(i&63))
        {
            printf (".");
//            C_Printf (CR_GRAY, ".");
        }

        if (i == numtextures1)
        {
            // Start looking in second texture file.
            maptex = maptex2;
            maxoff = maxoff2;
            directory = maptex+1;
        }
                
        offset = LONG(*directory);

        if (offset > maxoff)
            I_Error ("R_InitTextures: bad texture directory");
        
        mtexture = (maptexture_t *) ( (byte *)maptex + offset);

        texture = textures[i] =
            Z_Malloc (sizeof(texture_t)
                      + sizeof(texpatch_t)*(SHORT(mtexture->patchcount)-1),
                      PU_STATIC, 0);
        
        texture->width = SHORT(mtexture->width);
        texture->height = SHORT(mtexture->height);
        texture->patchcount = SHORT(mtexture->patchcount);
        
        memcpy (texture->name, mtexture->name, sizeof(texture->name));
        mpatch = &mtexture->patches[0];
        patch = &texture->patches[0];

        for (j=0 ; j<texture->patchcount ; j++, mpatch++, patch++)
        {
            patch->originx = SHORT(mpatch->originx);
            patch->originy = SHORT(mpatch->originy);
            patch->patch = patchlookup[SHORT(mpatch->patch)];
            if (patch->patch == -1)
            {
                I_Error ("R_InitTextures: Missing patch in texture %s",
                         texture->name);
            }
        }                
        texturecolumnlump[i] = Z_Malloc (texture->width*sizeof(**texturecolumnlump), PU_STATIC,0);
        texturecolumnofs[i] = Z_Malloc (texture->width*sizeof(**texturecolumnofs), PU_STATIC,0);
        texturecolumnofs2[i] = Z_Malloc (texture->width*sizeof(**texturecolumnofs2), PU_STATIC,0);

        j = 1;
        while (j*2 <= texture->width)
            j<<=1;

        texturewidthmask[i] = j-1;
        textureheight[i] = texture->height<<FRACBITS;
                
        totalwidth += texture->width;
    }

    Z_Free(patchlookup);

    W_ReleaseLumpName(DEH_String("TEXTURE1"));
    if (maptex2)
        W_ReleaseLumpName(DEH_String("TEXTURE2"));
    
    // Precalculate whatever possible.        

    for (i=0 ; i<numtextures ; i++)
        R_GenerateLookup (i);
    
    // Create translation table for global animation.
    texturetranslation = Z_Malloc ((numtextures+1)*sizeof(*texturetranslation), PU_STATIC, 0);
    
    for (i=0 ; i<numtextures ; i++)
        texturetranslation[i] = i;

    GenerateTextureHashTable();
}



//
// R_InitFlats
//
void R_InitFlats (void)
{
    int                i;
        
    firstflat = W_GetNumForName (DEH_String("F_START")) + 1;
    lastflat = W_GetNumForName (DEH_String("F_END")) - 1;
    numflats = lastflat - firstflat + 1;
        
    // Create translation table for global animation.
    flattranslation = Z_Malloc ((numflats+1)*sizeof(*flattranslation), PU_STATIC, 0);
    
    for (i=0 ; i<numflats ; i++)
        flattranslation[i] = i;
}


//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//
void R_InitSpriteLumps (void)
{
    int            i;
    patch_t        *patch;
        
    firstspritelump = W_GetNumForName (DEH_String("S_START")) + 1;
    lastspritelump = W_GetNumForName (DEH_String("S_END")) - 1;
    
    numspritelumps = lastspritelump - firstspritelump + 1;
    spritewidth = Z_Malloc (numspritelumps*sizeof(*spritewidth), PU_STATIC, 0);
    spriteheight = Z_Malloc(numspritelumps * sizeof(*spriteheight), PU_STATIC, 0);
    spriteoffset = Z_Malloc (numspritelumps*sizeof(*spriteoffset), PU_STATIC, 0);
    spritetopoffset = Z_Malloc (numspritelumps*sizeof(*spritetopoffset), PU_STATIC, 0);
        
    for (i=0 ; i< numspritelumps ; i++)
    {
        if (!(i&63))
        {
            printf (".");
//            C_Printf (CR_GRAY, ".");
        }

        patch = W_CacheLumpNum (firstspritelump+i, PU_CACHE);
        spritewidth[i] = SHORT(patch->width)<<FRACBITS;
        spriteheight[i] = SHORT(patch->height) << FRACBITS;
        spriteoffset[i] = SHORT(patch->leftoffset)<<FRACBITS;
        spritetopoffset[i] = SHORT(patch->topoffset)<<FRACBITS;

//        I_Sleep(3);
    }
}


//
// R_InitTranMap
//
// Initialize translucency filter map
//
void R_InitTranMap()
{
    int lump = W_CheckNumForName("TRANMAP");

    // If a tranlucency filter map lump is present, use it
    if (lump != -1)
    {
        // Set a pointer to the translucency filter maps.
        tranmap = W_CacheLumpNum(lump, PU_STATIC);
        printf(":"); // loaded from a lump
    }
    else
    {
        // Compose a default transparent filter map based on PLAYPAL.
        unsigned char *playpal = W_CacheLumpName("PLAYPAL", PU_STATIC);
        char *fname = NULL;
        extern char *configdir;

        struct {
            unsigned char pct;
            unsigned char playpal[256*3]; // a palette has 768 bytes!
        } cache;

        FILE *cachefp = fopen(fname = M_StringJoin(configdir,
                              "tranmap.dat", NULL), "r+b"); // open file readable

        tranmap = Z_Malloc(256*256, PU_STATIC, 0);

        // Use cached translucency filter if it's available
        if (!cachefp ? cachefp = fopen(fname,"wb") , 1 : // if file not readable, open writable, continue
            fread(&cache, 1, sizeof cache, cachefp) != sizeof cache || // could not read struct cache from file
            cache.pct != tran_filter_pct || // filter percents differ
            memcmp(cache.playpal, playpal, sizeof cache.playpal) || // base palettes differ
            fread(tranmap, 256, 256, cachefp) != 256 ) // could not read entire translucency map
        {
        byte *fg, *bg, blend[3], *tp = tranmap;
        int i, j, btmp;
        extern int FindNearestColor(byte *palette, int r, int g, int b);

        // background color
        for (i = 0; i < 256; i++)
        {
            // foreground color
            for (j = 0; j < 256; j++)
            {
                // shortcut: identical foreground and background
                if (i == j)
                {
                    *tp++ = i;
                    continue;
                }

                bg = playpal + 3*i;
                fg = playpal + 3*j;

                // blended color - emphasize blues
                // Colour matching in RGB space doesn't work very well with the blues
                // in Doom's palette. Rather than do any colour conversions, just
                // emphasize the blues when building the translucency table.
                btmp = fg[b] < (fg[r] + fg[g]) ? 0 : (fg[b] - (fg[r] + fg[g])) / 2;
                blend[r] = (tran_filter_pct * fg[r] + (100 - tran_filter_pct) * bg[r]) / (100 + btmp);
                blend[g] = (tran_filter_pct * fg[g] + (100 - tran_filter_pct) * bg[g]) / (100 + btmp);
                blend[b] = (tran_filter_pct * fg[b] + (100 - tran_filter_pct) * bg[b]) / 100;

                *tp++ = FindNearestColor(playpal, blend[r], blend[g], blend[b]);
            }
        }

        // write out the cached translucency map
        if (cachefp)
        {
            cache.pct = tran_filter_pct; // set filter percents
            memcpy(cache.playpal, playpal, sizeof cache.playpal); // set base palette
            fseek(cachefp, 0, SEEK_SET); // go to start of file
            fwrite(&cache, 1, sizeof cache, cachefp); // write struct cache
            fwrite(tranmap, 256, 256, cachefp); // write translucency map
            printf("!"); // generated and saved
        }
        else
            printf("?"); // generated, but not saved
        }
        else
            printf("."); // loaded from a file

        if (cachefp)
            fclose(cachefp);

        free(fname);

        Z_ChangeTag(playpal, PU_CACHE);
    }
}

//
// R_InitColormaps
//
void R_InitColormaps (void)
{
    int        lump;

    // Load in the light tables, 
    //  256 byte align tables.
    lump = W_GetNumForName(DEH_String("COLORMAP"));
    colormaps = W_CacheLumpNum(lump, PU_STATIC);

    // initialize color translation and color strings tables
    {
        byte *playpal = W_CacheLumpName("PLAYPAL", PU_STATIC);

        char c[3];
        int i, j;

        extern byte V_Colorize (byte *playpal, int cr, byte source);

        if (!crstr)
            crstr = malloc(CRXMAX * sizeof(*crstr));

        for (i = 0; i < CRXMAX; i++)
        {
            for (j = 0; j < 256; j++)
            {
                crx[i][j] = V_Colorize(playpal, i, j);
            }

            M_snprintf(c, sizeof(c), "\x1b%c", '0' + i);
            crstr[i] = M_StringDuplicate(c);
        }
        Z_ChangeTag(playpal, PU_CACHE);
    }
}



//
// R_InitData
// Locates all the lumps
//  that will be used by all views
// Must be called after W_Init.
//
void R_InitData (void)
{
    R_InitTextures ();

    printf (".");
//    C_Printf (CR_GRAY, ".");

    R_InitFlats ();

    printf (".");
//    C_Printf (CR_GRAY, ".");

    R_InitSpriteLumps ();

    R_InitTranMap();
    printf (".");

    R_InitColormaps ();
}



//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName (char* name)
{
    int         i;
    char        namet[9];

    if(beta_style && gamemode != shareware && gamemode != commercial)
    {
        if (name[0] == 'F' &&
            name[1] == 'L' &&
            name[2] == 'A' &&
            name[3] == 'T' &&
            name[4] == '2' &&
            name[5] == '2')
            name = "BFLAT22";

        else if (name[0] == 'D' &&
            name[1] == 'E' &&
            name[2] == 'M' &&
            name[3] == '1' &&
            name[4] == '_' &&
            name[5] == '5')
            name = "BDEM1_5";
    }

    i = W_CheckNumForName (name);

    if (i == -1)
    {
        namet[8] = 0;
        memcpy (namet, name,8);
        C_Printf(CR_RED, "R_FlatNumForName: %.8s not found", name);
    }
    return i - firstflat;
}




//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
int R_CheckTextureNumForName (char *name)
{
    texture_t *texture;
    int key;

    // "NoTexture" marker.
    if (name[0] == '-')                
        return 0;
                
//    C_Printf(CR_GRAY, "%s\n", name);

    key = W_LumpNameHash(name) % numtextures;

    texture=textures_hashtable[key]; 
    
    while (texture != NULL)
    {
        if (!strncasecmp (texture->name, name, 8) )
            return texture->index;

        texture = texture->next;
    }
    
    return -1;
}



//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//  aborts with error message.
//
int R_TextureNumForName (char* name)
{
    int                i;

    if(beta_style && gamemode != shareware && gamemode != commercial)
    {
        if (name[0] == 'A' &&
            name[1] == 'A' &&
            name[2] == 'S' &&
            name[3] == 'T' &&
            name[4] == 'I' &&
            name[5] == 'N' &&
            name[6] == 'K' &&
            name[7] == 'Y')
            i = R_CheckTextureNumForName ("BASTINKY");

        else if (name[0] == 'C' &&
            name[1] == 'O' &&
            name[2] == 'M' &&
            name[3] == 'P' &&
            name[4] == 'U' &&
            name[5] == 'T' &&
            name[6] == 'E' &&
            name[7] == '2')
            i = R_CheckTextureNumForName ("BCMPUTE2");

        else if (name[0] == 'B' &&
            name[1] == 'I' &&
            name[2] == 'G' &&
            name[3] == 'D' &&
            name[4] == 'O' &&
            name[5] == 'O' &&
            name[6] == 'R' &&
            name[7] == '2')
            i = R_CheckTextureNumForName ("BDOOR102");

        else if (name[0] == 'M' &&
            name[1] == 'A' &&
            name[2] == 'R' &&
            name[3] == 'B' &&
            name[4] == 'F' &&
            name[5] == 'A' &&
            name[6] == 'C' &&
            name[7] == 'E')
            i = R_CheckTextureNumForName ("BMWALL41");

        else if (name[0] == 'S' &&
            name[1] == 'K' &&
            name[2] == 'Y' &&
            name[3] == '1')
            i = R_CheckTextureNumForName ("BSKY1");

        else if (name[0] == 'S' &&
            name[1] == 'T' &&
            name[2] == 'E' &&
            name[3] == 'P' &&
            name[4] == '4')
            i = R_CheckTextureNumForName ("BSTEP4");

        else if (name[0] == 'S' &&
            name[1] == 'W' &&
            name[2] == '1' &&
            name[3] == 'D' &&
            name[4] == 'I' &&
            name[5] == 'R' &&
            name[6] == 'T')
            i = R_CheckTextureNumForName ("BSW1DIRT");

        else if (name[0] == 'S' &&
            name[1] == 'W' &&
            name[2] == '2' &&
            name[3] == 'D' &&
            name[4] == 'I' &&
            name[5] == 'R' &&
            name[6] == 'T')
            i = R_CheckTextureNumForName ("BSW2DIRT");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '5' &&
            name[5] == '7' &&
            name[6] == '_' &&
            name[5] == '2')
            i = R_CheckTextureNumForName ("BWALL572");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '5' &&
            name[5] == '7' &&
            name[6] == '_' &&
            name[5] == '3')
            i = R_CheckTextureNumForName ("BWALL573");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '5' &&
            name[5] == '7' &&
            name[6] == '_' &&
            name[5] == '4')
            i = R_CheckTextureNumForName ("BWALL574");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '6' &&
            name[5] == '3' &&
            name[6] == '_' &&
            name[5] == '1')
            i = R_CheckTextureNumForName ("BWALL631");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '6' &&
            name[5] == '3' &&
            name[6] == '_' &&
            name[5] == '2')
            i = R_CheckTextureNumForName ("BWALL632");

        else
            i = R_CheckTextureNumForName (name);
    }
    else
        i = R_CheckTextureNumForName (name);

    if (i==-1)
    {
        C_Printf(CR_RED, "R_TextureNumForName: %.8s not found", name);
    }
    return i;
}




//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
void R_PrecacheLevel (void)
{
    char*                flatpresent;
    char*                texturepresent;
    char*                spritepresent;

    int                  i;
    int                  j;
    int                  k;
    int                  lump;
    
    texture_t*           texture;
    thinker_t*           th;
    spriteframe_t*       sf;

    if (demoplayback)
        return;
    
    // Precache flats.
    flatpresent = Z_Malloc(numflats, PU_STATIC, NULL);
    memset (flatpresent,0,numflats);        

    for (i=0 ; i<numsectors ; i++)
    {
        flatpresent[sectors[i].floorpic] = 1;
        flatpresent[sectors[i].ceilingpic] = 1;
    }
        
    flatmemory = 0;

    for (i=0 ; i<numflats ; i++)
    {
        if (flatpresent[i])
        {
            lump = firstflat + i;
            flatmemory += lumpinfo[lump].size;
            W_CacheLumpNum(lump, PU_CACHE);
        }
    }

    Z_Free(flatpresent);
    
    // Precache textures.
    texturepresent = Z_Malloc(numtextures, PU_STATIC, NULL);
    memset (texturepresent,0, numtextures);
        
    for (i=0 ; i<numsides ; i++)
    {
        texturepresent[sides[i].toptexture] = 1;
        texturepresent[sides[i].midtexture] = 1;
        texturepresent[sides[i].bottomtexture] = 1;
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //  indicate a sky floor/ceiling as a flat,
    //  while the sky texture is stored like
    //  a wall texture, with an episode dependend
    //  name.
    texturepresent[skytexture] = 1;
        
    texturememory = 0;
    for (i=0 ; i<numtextures ; i++)
    {
        if (!texturepresent[i])
            continue;

        texture = textures[i];
        
        for (j=0 ; j<texture->patchcount ; j++)
        {
            lump = texture->patches[j].patch;
            texturememory += lumpinfo[lump].size;
            W_CacheLumpNum(lump , PU_CACHE);
        }
    }

    Z_Free(texturepresent);
    
    // Precache sprites.
    spritepresent = Z_Malloc(numsprites, PU_STATIC, NULL);
    memset (spritepresent,0, numsprites);
        
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
        if (th->function.acp1 == (actionf_p1)P_MobjThinker)
            spritepresent[((mobj_t *)th)->sprite] = 1;
    }
        
    spritememory = 0;
    for (i=0 ; i<numsprites ; i++)
    {
        if (!spritepresent[i])
            continue;

        for (j=0 ; j<sprites[i].numframes ; j++)
        {
            sf = &sprites[i].spriteframes[j];
            for (k=0 ; k<8 ; k++)
            {
                lump = firstspritelump + sf->lump[k];
                spritememory += lumpinfo[lump].size;
                W_CacheLumpNum(lump , PU_CACHE);
            }
        }
    }

    Z_Free(spritepresent);
}

