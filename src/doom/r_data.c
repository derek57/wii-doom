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

#ifdef WII
#include "../c_io.h"
#include "../deh_main.h"
#else
#include "c_io.h"
#include "deh_main.h"
#endif

#include "doomdef.h"
#include "doomstat.h"

#ifdef WII
#include "../i_scale.h"
#include "../i_swap.h"
#include "../i_system.h"
#include "../m_misc.h"
#else
#include "i_scale.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_misc.h"
#endif

#include "p_local.h"
#include "r_data.h"
#include "r_local.h"
#include "r_sky.h"

#ifdef WII
#include "../v_trans.h" // [crispy] tranmap, CRMAX
#include "../w_wad.h"
#include "../z_zone.h"
#else
#include "v_trans.h" // [crispy] tranmap, CRMAX
#include "w_wad.h"
#include "z_zone.h"
#endif


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


static byte notgray[256] =
{
    0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static byte notgrayorbrown[256] =
{
    0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static byte redonly[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static byte greenonly1[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static byte greenonly2[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static struct
{
    char        texture[9];
    byte        *colormask;
} fullbright[] = {
    { "COMP2",    notgrayorbrown }, { "COMPSTA1", notgray        }, { "COMPSTA2", notgray        },
    { "COMPUTE1", notgrayorbrown }, { "COMPUTE2", notgrayorbrown }, { "COMPUTE3", notgrayorbrown },
    { "EXITSIGN", notgray        }, { "EXITSTON", notgray        }, { "PLANET1",  notgray        },
    { "SILVER2",  notgray        }, { "SILVER3",  notgrayorbrown }, { "SLADSKUL", redonly        },
    { "SW1BRCOM", redonly        }, { "SW1BRIK",  redonly        }, { "SW1BRN1",  redonly        },
    { "SW1COMM",  redonly        }, { "SW1DIRT",  redonly        }, { "SW1MET2",  redonly        },
    { "SW1STARG", redonly        }, { "SW1STON1", redonly        }, { "SW1STON2", redonly        },
    { "SW1STONE", redonly        }, { "SW1STRTN", redonly        }, { "SW2BLUE",  redonly        },
    { "SW2BRCOM", greenonly2     }, { "SW2BRIK",  greenonly1     }, { "SW2BRN1",  greenonly1     },
    { "SW2BRN2",  greenonly1     }, { "SW2BRNGN", notgray        }, { "SW2COMM",  greenonly1     },
    { "SW2COMP",  redonly        }, { "SW2DIRT",  greenonly1     }, { "SW2EXIT",  notgray        },
    { "SW2GRAY",  notgray        }, { "SW2GRAY1", notgray        }, { "SW2GSTON", redonly        },
    { "SW2MARB",  greenonly1     }, { "SW2MET2",  greenonly1     }, { "SW2METAL", greenonly1     },
    { "SW2MOD1",  notgrayorbrown }, { "SW2PANEL", redonly        }, { "SW2ROCK",  redonly        },
    { "SW2SLAD",  redonly        }, { "SW2STARG", greenonly1     }, { "SW2STON1", greenonly1     },
    { "SW2STON2", greenonly1     }, { "SW2STON6", redonly        }, { "SW2STONE", greenonly1     },
    { "SW2STRTN", greenonly1     }, { "SW2TEK",   greenonly1     }, { "SW2VINE",  greenonly1     },
    { "SW2WDMET", redonly        }, { "SW2WOOD",  redonly        }, { "SW2ZIM",   redonly        },
    { "WOOD4",    redonly        }, { "WOODGARG", redonly        }, { "WOODSKUL", redonly        },
    { "ZELDOOR",  redonly        }, { "",         0              }
};


// killough 4/17/98: make firstcolormaplump, lastcolormaplump external
int              firstcolormaplump;
int              lastcolormaplump;

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

lighttable_t     **colormaps;

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

unsigned**       texturecolumnofs;  // [crispy] fix Medusa bug
unsigned**	 texturecolumnofs2; // [crispy] original column offsets for single-patched textures

byte**           texturecomposite;
byte**           texturefullbright;

byte             grays[256];

//extern byte*     tranmap;


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



// [crispy] replace R_DrawColumnInCache(), R_GenerateComposite() and R_GenerateLookup()
// with Lee Killough's implementations found in MBF to fix Medusa bug
// taken from mbfsrc/R_DATA.C:136-425

// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_data.c,v 1.23 1998/05/23 08:05:57 killough Exp $
//
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      Preparation of data for rendering,
//      generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------

// R_DrawColumnInCache
// Clip and draw a column
//  from a patch into a cached post.
//
// Rewritten by Lee Killough for performance and to fix Medusa bug
//
void
R_DrawColumnInCache
( column_t*	patch,
  byte*		cache,
  int		originy,
  int		cacheheight,
  byte*		marks )
{
    int		count;
    int		position;
    byte*	source;

    while (patch->topdelta != 0xff)
    {
	source = (byte *)patch + 3;
	count = patch->length;
	position = originy + patch->topdelta;

	if (position < 0)
	{
	    count += position;
	    position = 0;
	}

	if (position + count > cacheheight)
	    count = cacheheight - position;

	if (count > 0)
	{
	    // killough 4/9/98: remember which cells in column have been drawn,
	    // so that column can later be converted into a series of posts, to
	    // fix the Medusa bug.
	    memcpy (cache + position, source, count);
	    memset (marks + position, 0xff, count);
	}
	patch = (column_t *)(  (byte *)patch + patch->length + 4); 
    }
}


//
// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
// Rewritten by Lee Killough for performance and to fix Medusa bug
//
void R_GenerateComposite(int texnum)
{
    byte*		block;
    texture_t*		texture;
    texpatch_t*		patch;	
    patch_t*		realpatch;
    int			x;
    int			x1;
    int			x2;
    int			i;
    column_t*		patchcol;
    short*		collump;
    unsigned*		colofs; // [crispy] fix Medusa bug
    byte*		marks; // [crispy] fix Medusa bug
    byte*		source; // [crispy] fix Medusa bug
	
    texture = textures[texnum];

    block = Z_Malloc (texturecompositesize[texnum],
		      PU_STATIC, 
		      (void **) &texturecomposite[texnum]);	

    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];
    
    // Composite the columns together.
    patch = texture->patches;
		
    // killough 4/9/98: marks to identify transparent regions in merged textures
    marks = calloc(texture->width, texture->height);

    // [crispy] initialize composite background to black (index 0)
    memset(block, 0, texturecompositesize[texnum]);

    for (i=0 , patch = texture->patches;
	 i<texture->patchcount;
	 i++, patch++)
    {
	realpatch = W_CacheLumpNum (patch->patch, PU_CACHE);
	x1 = patch->originx;
	x2 = x1 + SHORT(realpatch->width);

	if (x1<0)
	    x = 0;
	else
	    x = x1;
	
	if (x2 > texture->width)
	    x2 = texture->width;

	for ( ; x<x2 ; x++)
	{
	    // Column does not have multiple patches?
	    // [crispy] generate composites for single-patched textures as well
	    /*
	    if (collump[x] >= 0)
		continue;
	    */
	    
	    patchcol = (column_t *)((byte *)realpatch
				    + LONG(realpatch->columnofs[x-x1]));
	    R_DrawColumnInCache (patchcol,
				 block + colofs[x],
				 // [crispy] single-patched columns are normally not composited
				 // but directly read from the patch lump ignoring their originy
				 collump[x] >= 0 ? 0 : patch->originy,
				 texture->height,
				 marks + x*texture->height);
	}
    }

    // killough 4/9/98: Next, convert multipatched columns into true columns,
    // to fix Medusa bug while still allowing for transparent regions.
    // temporary column
    source = malloc(texture->height); // temporary column
    for (i = 0; i < texture->width; i++)
    {
	if (collump[i] == -1) // process only multipatched columns
	{
	    column_t *col = (column_t *)(block + colofs[i] - 3); // cached column
	    const byte *mark = marks + i * texture->height;
	    int j = 0;

            // save column in temporary so we can shuffle it around
            memcpy(source, (byte *) col + 3, texture->height);

            // reconstruct the column by scanning transparency marks
            for (;;)
            {
                unsigned len; // killough 12/98

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

                // killough 12/98
                // Use 32-bit len counter, to support tall 1s multipatched textures
                for (len = 0; j < texture->height && mark[j]; j++)
                    // count opaque cells
                    len++;

                // killough 12/98
                // intentionally truncate length
                col->length = len;

		// copy opaque cells from the temporary back into the column
		memcpy((byte *) col + 3, source + col->topdelta, len);
		col = (column_t *)((byte *) col + len + 4); // next post
	    }
	}
    }
    free(source); // free temporary column
    free(marks); // free transparency marks

    // Now that the texture has been built in column cache,
    //  it is purgable from zone memory.
    Z_ChangeTag (block, PU_CACHE);
}


//
// R_GenerateLookup
//
// Rewritten by Lee Killough for performance and to fix Medusa bug
//
void R_GenerateLookup(int texnum)
{
    texture_t*		texture;
    byte*		patchcount;	// patchcount[texture->width]
    byte*		postcount; // killough 4/9/98: keep count of posts in addition to patches.
    texpatch_t*		patch;	
    patch_t*		realpatch;
    int			x;
    int			x1;
    int			x2;
    int			i;
    short*		collump;
    unsigned*		colofs; // killough 4/9/98: make 32-bit
    unsigned*		colofs2; // [crispy] original column offsets
    int			csize = 0; // killough 10/98
	
    texture = textures[texnum];

    // Composited texture not created yet.
    texturecomposite[texnum] = 0;
    
    texturecompositesize[texnum] = 0;
    collump = texturecolumnlump[texnum];
    colofs = texturecolumnofs[texnum];
    colofs2 = texturecolumnofs2[texnum]; // [crispy] original column offsets
    
    // Now count the number of columns
    //  that are covered by more than one patch.
    // Fill in the lump / offset, so columns
    //  with only a single patch are all done.
    patchcount = (byte *) Z_Malloc(texture->width, PU_STATIC, (void **) &patchcount);
    postcount = (byte *) Z_Malloc(texture->width, PU_STATIC, (void **) &postcount);
    memset (patchcount, 0, texture->width);
    patch = texture->patches;

    for (i=0 , patch = texture->patches;
	 i<texture->patchcount;
	 i++, patch++)
    {
	realpatch = W_CacheLumpNum (patch->patch, PU_CACHE);
	x1 = patch->originx;
	x2 = x1 + SHORT(realpatch->width);
	
	if (x1 < 0)
	    x = 0;
	else
	    x = x1;

	if (x2 > texture->width)
	    x2 = texture->width;

	for ( ; x<x2 ; x++)
        {
	    patchcount[x]++;
	    collump[x] = patch->patch;
	    colofs[x] = colofs2[x] = LONG(realpatch->columnofs[x-x1])+3; // [crispy] original column offsets
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
	// killough 12/98: Warn about a common column construction bug
	unsigned limit = texture->height * 3 + 3; // absolute column size limit

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

	    for (x = x1 ; x < x2 ; x++)
	    {
		if (patchcount[x] > 1) // Only multipatched columns
		{
		    const column_t *col = (column_t*)((byte*) realpatch + LONG(cofs[x]));
		    const byte *base = (const byte *) col;

		    // count posts
		    for ( ; col->topdelta != 0xff; postcount[x]++)
		    {
			if ((unsigned)((byte *) col - base) <= limit)
			    col = (column_t *)((byte *) col + col->length + 4);
			else
			    break;
                    }
                }
            }
        }
    }

    // Now count the number of columns
    //  that are covered by more than one patch.
    // Fill in the lump / offset, so columns
    //  with only a single patch are all done.

    for (x=0 ; x<texture->width ; x++)
    {
	if (!patchcount[x])
        {
	    C_Printf (CR_GOLD, " R_GenerateLookup: column without a patch (%s)\n",
		    texture->name);
	    // [crispy] do not return yet
	    /*
	    return;
	    */
        }

	// I_Error ("R_GenerateLookup: column without a patch");
	
	if (patchcount[x] > 1)
	{
	    // Use the cached block.
	    // [crispy] moved up here, the rest in this loop
	    // applies to single-patched textures as well
	    collump[x] = -1;	
	}
	    // killough 1/25/98, 4/9/98:
	    //
	    // Fix Medusa bug, by adding room for column header
	    // and trailer bytes for each post in merged column.
	    // For now, just allocate conservatively 4 bytes
	    // per post per patch per column, since we don't
	    // yet know how many posts the merged column will
	    // require, and it's bounded above by this limit.

	    colofs[x] = csize + 3; // three header bytes in a column
	    csize += 4 * postcount[x] + 5; // 1 stop byte plus 4 bytes per post
	    
	    // [crispy] remove limit
	    /*
	    if (texturecompositesize[texnum] > 0x10000-texture->height)
	    {
		I_Error ("R_GenerateLookup: texture %i is >64k",
			 texnum);
	    }
	    */
	csize += texture->height; // height bytes of texture data
    }
    texturecompositesize[texnum] = csize;

    Z_Free(patchcount);
    Z_Free(postcount);
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
    
    // [crispy] single-patched mid-textures on two-sided walls
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
    texturefullbright = Z_Malloc(numtextures * sizeof(*texturefullbright), PU_STATIC, 0);

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
		// [crispy] make non-fatal
                C_Printf (CR_GOLD, " R_InitTextures: Missing patch in texture %s",
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

    // [BH] Initialize partially fullbright textures.
    memset(texturefullbright, 0, numtextures * sizeof(*texturefullbright));
    if (d_brightmaps)
    {
        i = 0;
        while (fullbright[i].colormask)
        {
            if (fullbright[i].texture)
            {
                int num = R_CheckTextureNumForName(fullbright[i].texture);

                if (num != -1)
                    texturefullbright[num] = fullbright[i].colormask;
                i++;
            }
        }
    }
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

        // [BH] override sprite offsets in WAD with those in sproffsets[] in info.c
        if (d_fixspriteoffsets && fsize != 28422764 && fsize != 19321722 && !modifiedgame)
        {
            int j = 0;

            while (sproffsets[j].name[0])
            {
                if (i == W_CheckNumForName(sproffsets[j].name) - firstspritelump
                    && spritewidth[i] == (SHORT(sproffsets[j].width) << FRACBITS)
                    && spriteheight[i] == (SHORT(sproffsets[j].height) << FRACBITS))
                {
                    spriteoffset[i] = SHORT(sproffsets[j].x) << FRACBITS;
                    spritetopoffset[i] = SHORT(sproffsets[j].y) << FRACBITS;
                    break;
                }
                j++;
            }
        }

//        I_Sleep(3);
    }
}


//
// R_InitTranMap
//
// [crispy] Initialize translucency filter map
// based in parts on the implementation from boom202s/R_DATA.C:676-787
//
void R_InitTranMap()
{
    int lump = W_CheckNumForName("TRANMAP");

    // If a tranlucency filter map lump is present, use it
    if (lump != -1)
    {
        // Set a pointer to the translucency filter maps.
        tranmap = W_CacheLumpNum(lump, PU_STATIC);
        printf(":"); // [crispy] loaded from a lump
    }
    else
    {
        // Compose a default transparent filter map based on PLAYPAL.
        unsigned char *playpal = W_CacheLumpName("PLAYPAL", PU_STATIC);
        char *fname = NULL;
        extern char *configdir;

        struct
        {
            unsigned char pct;
            unsigned char playpal[256*3]; // [crispy] a palette has 768 bytes!

        } cache;

        // [crispy] open file readable
        FILE *cachefp = fopen(fname = M_StringJoin(configdir, "tranmap.dat", NULL), "r+b");

        tranmap = Z_Malloc(256*256, PU_STATIC, 0);

        // Use cached translucency filter if it's available

        // [crispy] if file not readable, open writable, continue

        if (!cachefp ? cachefp = fopen(fname,"wb") , 1 : 

            // [crispy] could not read struct cache from file

            fread(&cache, 1, sizeof cache, cachefp) != sizeof cache ||

            // [crispy] filter percents differ

            cache.pct != tran_filter_pct ||

            // [crispy] base palettes differ

            memcmp(cache.playpal, playpal, sizeof cache.playpal) ||

            // [crispy] could not read entire translucency map

            fread(tranmap, 256, 256, cachefp) != 256 )
        {
            byte *fg, *bg, blend[3], *tp = tranmap;
            int i, j, btmp;

            // [crispy] background color
            for (i = 0; i < 256; i++)
            {
                // [crispy] foreground color
                for (j = 0; j < 256; j++)
                {
                    // [crispy] shortcut: identical foreground and background
                    if (i == j)
                    {
                        *tp++ = i;
                        continue;
                    }

                    bg = playpal + 3*i;
                    fg = playpal + 3*j;

                    // [crispy] blended color - emphasize blues
                    // Colour matching in RGB space doesn't work very well with the blues
                    // in Doom's palette. Rather than do any colour conversions, just
                    // emphasize the blues when building the translucency table.
                    btmp = fg[b] < (fg[r] + fg[g]) ? 0 : (fg[b] - (fg[r] + fg[g])) / 2;

                    blend[r] = (tran_filter_pct * fg[r] +
                            (100 - tran_filter_pct) * bg[r]) / (100 + btmp);

                    blend[g] = (tran_filter_pct * fg[g] +
                            (100 - tran_filter_pct) * bg[g]) / (100 + btmp);

                    blend[b] = (tran_filter_pct * fg[b] +
                            (100 - tran_filter_pct) * bg[b]) / 100;

                    *tp++ = FindNearestColor(playpal, blend[r], blend[g], blend[b]);
                }
            }

            // write out the cached translucency map
            if (cachefp)
            {
                // [crispy] set filter percents

                cache.pct = tran_filter_pct;

                // [crispy] set base palette

                memcpy(cache.playpal, playpal, sizeof cache.playpal);

                // [crispy] go to start of file

                fseek(cachefp, 0, SEEK_SET);

                // [crispy] write struct cache

                fwrite(&cache, 1, sizeof cache, cachefp);

                // [crispy] write translucency map

                fwrite(tranmap, 256, 256, cachefp);

                // [crispy] generated and saved

                printf("!");
            }
            else
                // [crispy] generated, but not saved

                printf("?");
        }
        else
            // [crispy] loaded from a file

            printf(".");

        if (cachefp)
            fclose(cachefp);

        free(fname);

        Z_ChangeTag(playpal, PU_CACHE);
    }
}

//
// R_InitColormaps
//
extern byte V_Colorize (byte *playpal, int cr, byte source);

void R_InitColormaps (void)
{
    boolean    COLORMAP = (W_CheckMultipleLumps("COLORMAP") > 1);
    int        i, j, k;
    byte       *palsrc, *palette, *playpal;
    char       c[3];

    if (W_CheckNumForName("C_START") >= 0 && W_CheckNumForName("C_END") >= 0)
    {
        firstcolormaplump = W_GetNumForName("C_START");
        lastcolormaplump = W_GetNumForName("C_END");
        numcolormaps = lastcolormaplump - firstcolormaplump;

        colormaps = Z_Malloc(sizeof(*colormaps) * numcolormaps, PU_STATIC, 0);

        colormaps[0] = W_CacheLumpName("COLORMAP", PU_STATIC);

        for (i = 1; i < numcolormaps; i++)
            colormaps[i] = W_CacheLumpNum(i + firstcolormaplump, PU_STATIC);
    }
    else
    {
        colormaps = Z_Malloc(sizeof(*colormaps), PU_STATIC, 0);
        colormaps[0] = W_CacheLumpName("COLORMAP", PU_STATIC);
    }

    // [BH] There's a typo in dcolors.c, the source code of the utility Id
    // Software used to construct the palettes and colormaps for DOOM (see
    // http://www.doomworld.com/idgames/?id=16644). When constructing colormap
    // 32, which is used for the invulnerability powerup, the traditional
    // Y luminance values are used (see http://en.wikipedia.org/wiki/YIQ), but a
    // value of 0.144 is used when it should be 0.114. So I've grabbed the
    // offending code from dcolor.c, corrected it, put it here, and now colormap
    // 32 is manually calculated rather than grabbing it from the colormap lump.
    // The resulting differences are minor.
    palsrc = palette = W_CacheLumpName("PLAYPAL", PU_CACHE);

    for (i = 0; i < 255; i++)
    {
        float       red = *palsrc++ / 256.0f;
        float       green = *palsrc++ / 256.0f;
        float       blue = *palsrc++ / 256.0f;
        float       gray = red * 0.299f + green * 0.587f + blue * 0.114f/*0.144f*/;

        grays[i] = FindNearestColor(palette, (int)(gray * 255.0f),
            (int)(gray * 255.0f), (int)(gray * 255.0f));

        if (!COLORMAP)
        {
            gray = (1.0f - gray) * 255.0f;
            colormaps[0][32 * 256 + i] = FindNearestColor(palette, (int)gray, (int)gray, (int)gray);
        }
    }

    // [crispy] initialize color translation and color strings tables
    playpal = W_CacheLumpName("PLAYPAL", PU_STATIC);

    if (!crstr)
        crstr = malloc(CRXMAX * sizeof(*crstr));

    for (j = 0; j < CRXMAX; j++)
    {
        for (k = 0; k < 256; k++)
        {
            crx[j][k] = V_Colorize(playpal, j, k);
        }

        M_snprintf(c, sizeof(c), "\x1b%c", '0' + j);
        crstr[j] = M_StringDuplicate(c);
    }
    Z_ChangeTag(playpal, PU_CACHE);
}

// killough 4/4/98: get colormap number from name
// killough 4/11/98: changed to return -1 for illegal names
int R_ColormapNumForName(char *name)
{
    register int i = 0;

    if (numcolormaps == 1)
        return -1;

    if (strncasecmp(name, "COLORMAP", 8))     // COLORMAP predefined to return 0
        if ((i = W_CheckNumForName(name)) != -1)
            i -= firstcolormaplump;
    return i;
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

//    printf (".");
//    C_Printf (CR_GRAY, ".");

    R_InitFlats ();

//    printf (".");
//    C_Printf (CR_GRAY, ".");

    R_InitSpriteLumps ();

    R_InitTranMap(); // [crispy] prints a mark itself
//    printf (".");

    R_InitColormaps ();
}



//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName (char* name)
{
    int         i;
//    char        namet[9];

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
/*
        namet[8] = 0;
        memcpy (namet, name,8);
*/
	// [crispy] make non-fatal
        if(replace_missing)
        {
            i = W_CheckNumForName ("FLOOR0_1");
            C_Printf(CR_RED, " R_FlatNumForName: %.8s not found (replaced with FLOOR0_1)", name);
        }
        else
        {
            C_Printf(CR_RED, " R_FlatNumForName: %.8s not found");
            return 0;
        }
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
	// [crispy] make non-fatal
        if(replace_missing)
        {
            i = R_CheckTextureNumForName ("BROWN1");
            C_Printf(CR_RED, " R_TextureNumForName: %.8s not found (replaced with BROWN1)", name);
        }
        else
        {
            C_Printf(CR_RED, " R_TextureNumForName: %.8s not found");
            return 0;
        }
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
            flatmemory += lumpinfo[lump]->size;
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
            texturememory += lumpinfo[lump]->size;
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
                spritememory += lumpinfo[lump]->size;
                W_CacheLumpNum(lump , PU_CACHE);
            }
        }
    }

    Z_Free(spritepresent);
}

//
// R_CheckFlatNumForName
// Retrieval, get a flat number for a flat name. No error.
//
int R_CheckFlatNumForName(char *name)
{
    int i;

    for (i = firstflat; i <= lastflat; i++)
        if (!strncasecmp(lumpinfo[i]->name, name, 8))
            return (i - firstflat);

    return -1;
}

