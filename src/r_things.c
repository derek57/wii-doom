// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
//
// Copyright(C) 2015 by Brad Harding: - (Liquid Sector Animations)
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
//        Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------




#include <stdio.h>
#include <stdlib.h>

#include "c_io.h"
#include "deh_main.h"
#include "doomdef.h"
#include "doomfeatures.h"
#include "doomstat.h"
#include "i_swap.h"
#include "i_system.h"
#include "p_local.h"
#include "r_local.h"
#include "v_trans.h" // [crispy] colored blood sprites
#include "w_wad.h"
#include "z_zone.h"


#define MINZ                    (FRACUNIT*4)
#define BASEYCENTER             100
#define MAX_SPRITE_FRAMES       29


typedef struct
{
    int                x1;
    int                x2;
        
    int                column;
    int                topclip;
    int                bottomclip;

} maskdraw_t;


boolean                clip_this;

char*                  spritename;

// constant arrays
//  used for psprite clipping and initializing clipping
int                    negonearray[SCREENWIDTH];          // [crispy] 32-bit integer math
int                    screenheightarray[SCREENWIDTH];    // [crispy] 32-bit integer math
int                    numsprites;
int                    maxframe;
int                    newvissprite;

int*                   mfloorclip;                        // [crispy] 32-bit integer math
int*                   mceilingclip;                      // [crispy] 32-bit integer math

int64_t                sprtopscreen;                      // [crispy] WiggleFix
int64_t                shift;

static int             num_vissprite[NUMVISSPRITETYPES];
static int             num_vissprite_alloc[NUMVISSPRITETYPES];
static int             num_vissprite_ptrs;

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t sprites patches
spritedef_t*           sprites;

spriteframe_t          sprtemp[MAX_SPRITE_FRAMES];

vissprite_t*           vissprites[NUMVISSPRITETYPES];
vissprite_t**          vissprite_ptrs;          // killough
vissprite_t*           vissprite_p;
vissprite_t            overflowsprite;
vissprite_t            vsprsortedhead;

//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//
fixed_t                pspritescale;
fixed_t                pspriteiscale;
fixed_t                spryscale;

static lighttable_t    **spritelights;         // killough 1/25/98 made static

extern boolean         realframe;
extern boolean         skippsprinterp;


//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
void R_InstallSpriteLump(lumpinfo_t *lump, int lumpnum, unsigned int frame, char rot,
    boolean flipped)
{
    unsigned int        rotation;

    rotation = (rot >= '0' && rot <= '9' ? rot - '0' : (rot >= 'A' ? rot - 'A' + 10 : 17));

    if (frame >= MAX_SPRITE_FRAMES || rotation > 16)
        I_Error("R_InstallSpriteLump: Bad frame characters in lump %s", lump->name);

    if ((int)frame > maxframe)
        maxframe = frame;

    if (rotation == 0)
    {
        int r;

        // the lump should be used for all rotations
        for (r = 14; r >= 0; r -= 2)
        {
            if (sprtemp[frame].lump[r] == -1)
            {
                sprtemp[frame].lump[r] = lumpnum - firstspritelump;
                if (flipped)
                    sprtemp[frame].flip |= (1 << r);
                sprtemp[frame].rotate = false;  // jff 4/24/98 if any subbed, rotless
            }
        }
        return;
    }

    // the lump is only used for one rotation
    rotation = (rotation <= 8 ? (rotation - 1) * 2 : (rotation - 9) * 2 + 1);

    if (sprtemp[frame].lump[rotation] == -1)
    {
        sprtemp[frame].lump[rotation] = lumpnum - firstspritelump;
        if (flipped)
            sprtemp[frame].flip |= (1 << rotation);
        sprtemp[frame].rotate = true;           //jff 4/24/98 only change if rot used
    }
}




//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
// (4 chars exactly) to be used.
//
// Builds the sprite rotation matrixes to account
// for horizontally flipped sprites.
//
// Will report an error if the lumps are inconsistent.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
//
// A sprite that is flippable will have an additional
//  letter/number appended.
//
// The rotation character can be 0 to signify no rotations.
//
// 1/25/98, 1/31/98 killough : Rewritten for performance
//
// Empirically verified to have excellent hash
// properties across standard Doom sprites:
#define R_SpriteNameHash(s) ((unsigned int)((s)[0] - ((s)[1] * 3 - (s)[3] * 2 - (s)[2]) * 2))

void R_InitSpriteDefs(char **namelist)
{
    size_t              numentries = lastspritelump - firstspritelump + 1;
    unsigned int        i;

    struct {
        int     index;
        int     next;
    } *hash;

    if (!numentries || !*namelist)
        return;

    // count the number of sprite names
    for (i = 0; namelist[i]; i++);

    numsprites = (signed int)i;

    sprites = Z_Malloc(numsprites * sizeof(*sprites), PU_STATIC, NULL);

    // Create hash table based on just the first four letters of each sprite
    // killough 1/31/98
    hash = malloc(sizeof(*hash) * numentries);  // allocate hash table

    for (i = 0; i < numentries; i++)            // initialize hash table as empty
        hash[i].index = -1;

    for (i = 0; i < numentries; i++)            // Prepend each sprite to hash chain
    {                                           // prepend so that later ones win
        int     j = R_SpriteNameHash(lumpinfo[i + firstspritelump]->name) % numentries;

        hash[i].next = hash[j].index;
        hash[j].index = i;
    }

    // scan all the lump names for each of the names,
    //  noting the highest frame letter.
    for (i = 0; i < (unsigned int)numsprites; i++)
    {
        const char      *spritename = namelist[i];
        int             j = hash[R_SpriteNameHash(spritename) % numentries].index;

        if (j >= 0)
        {
            int k;

            memset(sprtemp, -1, sizeof(sprtemp));
            for (k = 0; k < MAX_SPRITE_FRAMES; k++)
                sprtemp[k].flip = 0;
            maxframe = -1;
            do
            {
                lumpinfo_t      *lump = lumpinfo[j + firstspritelump];

                // Fast portable comparison -- killough
                // (using int pointer cast is nonportable):
                if (!((lump->name[0] ^ spritename[0]) | (lump->name[1] ^ spritename[1])
                    | (lump->name[2] ^ spritename[2]) | (lump->name[3] ^ spritename[3])))
                {
                    R_InstallSpriteLump(lump, j + firstspritelump, lump->name[4] - 'A', 
                        lump->name[5], false);
                    if (lump->name[6])
                        R_InstallSpriteLump(lump, j + firstspritelump, lump->name[6] - 'A',
                           lump->name[7], true);
                }
            } while ((j = hash[j].next) >= 0);

            // check the frames that were found for completeness
            if ((sprites[i].numframes = ++maxframe))  // killough 1/31/98
            {
                int     frame;
                int     rot;

                for (frame = 0; frame < maxframe; frame++)
                    switch ((int)sprtemp[frame].rotate)
                    {
                        case -1:
                            // no rotations were found for that frame at all
                            break;

                        case 0:
                            // only the first rotation is needed
                            for (rot = 1; rot < 16; rot++)
                                sprtemp[frame].lump[rot] = sprtemp[frame].lump[0];

                            // If the frame is flipped, they all should be
                            if (sprtemp[frame].flip & 1)
                                sprtemp[frame].flip = 0xFFFF;
                            break;

                        case 1:
                            // must have all 8 frames
                            for (rot = 0; rot < 8; rot++)
                            {
                                if (sprtemp[frame].lump[rot * 2 + 1] == -1)
                                {
                                    sprtemp[frame].lump[rot * 2 + 1] = sprtemp[frame].lump[rot * 2];
                                    if (sprtemp[frame].flip & (1 << (rot * 2)))
                                        sprtemp[frame].flip |= 1 << (rot * 2 + 1);
                                }
                                if (sprtemp[frame].lump[rot * 2] == -1)
                                {
                                    sprtemp[frame].lump[rot * 2] = sprtemp[frame].lump[rot * 2 + 1];
                                    if (sprtemp[frame].flip & (1 << (rot * 2 + 1)))
                                        sprtemp[frame].flip |= 1 << (rot * 2);
                                }
                            }
                            for (rot = 0; rot < 16; rot++)
                                if (sprtemp[frame].lump[rot] == -1)
                                    I_Error("R_InitSprites: Frame %c of sprite %.8s frame %c is "
                                        "missing rotations", frame + 'A', namelist[i]);
                            break;
                    }

                for (frame = 0; frame < maxframe; frame++)
                    if (sprtemp[frame].rotate == -1)
                    {
                        memset(&sprtemp[frame].lump, 0, sizeof(sprtemp[0].lump));
                        sprtemp[frame].flip = 0;
                        sprtemp[frame].rotate = 0;
                    }

                // allocate space for the frames present and copy sprtemp to it
                sprites[i].spriteframes = Z_Malloc(maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
                memcpy(sprites[i].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
            }
        }
    }
    free(hash);             // free hash table
}




//
// GAME FUNCTIONS
//

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites (char** namelist)
{
    int                i;
        
    for (i=0 ; i<SCREENWIDTH ; i++)
    {
        negonearray[i] = -1;
    }
        
    R_InitSpriteDefs (namelist);
}



//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites (void)
{
    int i;

    for (i = 0; i < NUMVISSPRITETYPES; ++i)
        num_vissprite[i] = 0;
}


//
// R_NewVisSprite
//
// FIXME: DISABLED CODE WORKS FOR THE WII (ENABLED CODE IS USED FOR CONSOLE REPORTS)
//
vissprite_t* R_NewVisSprite (visspritetype_t type)
{
/*
    if (num_vissprite >= num_vissprite_alloc)           // killough
    {
        num_vissprite_alloc = (num_vissprite_alloc ? num_vissprite_alloc * 2 : 128);
        vissprites = realloc(vissprites, num_vissprite_alloc * sizeof(*vissprites));
    }
    return (vissprites + num_vissprite++);
*/
    if (num_vissprite[type] >= num_vissprite_alloc[type])           // killough
    {
        static int max;
        int numvissprites_old = num_vissprite_alloc[type];

        // [crispy] cap MAXVISSPRITES limit at 4096
        if (!max && num_vissprite_alloc[type] == 32 * 128)
        {
            C_Printf(CR_GOLD, " R_NewVisSprite: MAXVISSPRITES limit capped at %d.\n", num_vissprite_alloc);
            max++;
        }

        if (max)
            return &overflowsprite;

        num_vissprite_alloc[type] = (num_vissprite_alloc[type] ? num_vissprite_alloc[type] * 2 : 128);
        vissprites[type] = realloc(vissprites[type], num_vissprite_alloc[type] * sizeof(*vissprites[type]));
        memset(vissprites[type] + numvissprites_old, 0, (num_vissprite_alloc[type] - numvissprites_old) * sizeof(*vissprites[type]));

        if (numvissprites_old)
            C_Printf(CR_GOLD, " R_NewVisSprite: Hit MAXVISSPRITES limit at %d, raised to %d.\n", numvissprites_old, num_vissprite_alloc[type]);
    }
    return (vissprites[type] + num_vissprite[type]++);
}



//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//

void R_DrawMaskedColumn (column_t* column, signed int baseclip)
{
    int64_t            topscreen;                         // [crispy] WiggleFix
    int64_t            bottomscreen;                      // [crispy] WiggleFix
    fixed_t            basetexturemid;
        
    basetexturemid = dc_texturemid;
    dc_texheight = 0;                                     // [crispy] Tutti-Frutti fix
        
    for ( ; column->topdelta != 0xff ; ) 
    {
        // calculate unclipped screen coordinates
        //  for post
        topscreen = sprtopscreen + spryscale*column->topdelta;
        bottomscreen = topscreen + spryscale*column->length;

        dc_yl = (int)((topscreen+FRACUNIT-1)>>FRACBITS);  // [crispy] WiggleFix
        dc_yh = (int)((bottomscreen-1)>>FRACBITS);        // [crispy] WiggleFix
                
        if (dc_yh >= mfloorclip[dc_x])
            dc_yh = mfloorclip[dc_x]-1;
        if (dc_yl <= mceilingclip[dc_x])
            dc_yl = mceilingclip[dc_x]+1;

        if (dc_yh >= baseclip && baseclip != -1)
            dc_yh = baseclip;

        if (dc_yl <= dc_yh)
        {
            dc_source = (byte *)column + 3;
            dc_texturemid = basetexturemid - (column->topdelta<<FRACBITS);
            // dc_source = (byte *)column + 3 - column->topdelta;

            // Drawn by either R_DrawColumn
            //  or (SHADOW) R_DrawFuzzColumn.
            colfunc ();        
        }
        column = (column_t *)(  (byte *)column + column->length + 4);
    }
        
    dc_texturemid = basetexturemid;
}



//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
void
R_DrawVisSprite
( vissprite_t*         vis,
  int                  x1,
  int                  x2 )
{
    column_t*          column;
    int                texturecolumn;
    fixed_t            frac;
    patch_t*           patch;
    fixed_t            baseclip;
        
    patch = W_CacheLumpNum (vis->patch+firstspritelump, PU_CACHE);

    dc_colormap = vis->colormap;

    if (!dc_colormap)
    {
        // NULL colormap = shadow draw
        colfunc = fuzzcolfunc;
    }
    else if (vis->mobjflags & MF_TRANSLATION)
    {
        colfunc = transcolfunc;
        dc_translation = translationtables - 256 +
            ( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) );
    }
    // [crispy] color-translated sprites (i.e. blood)
    else if (vis->translation)
    {
        colfunc = transcolfunc;
        dc_translation = vis->translation;
    }
        
    // [crispy] translucent sprites
    if (d_translucency && dc_colormap &&
        ((vis->mobjflags & MF_TRANSLUCENT) ||
        ((vis->mobjflags & MF_SHADOW) && d_translucency)))
    {
        colfunc = tlcolfunc;
    }

    if(d_shadows && vis->mobjflags2 & MF2_SHADOW)
        colfunc = vis->colfunc;

    dc_iscale = abs(vis->xiscale)>>(detailshift && !hires);                // CHANGED FOR HIRES
    dc_texturemid = vis->texturemid;
    frac = vis->startfrac;
    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(dc_texturemid,spryscale);
        
    // check to see if weapon is a vissprite
    if (vis->psprite)
    {
        dc_texturemid += FixedMul(((centery - viewheight / 2) << FRACBITS),
                                  vis->xiscale);
        sprtopscreen += (viewheight / 2 - centery) << FRACBITS;
    }

    if (vis->footclip)
        baseclip = ((int)sprtopscreen + FixedMul(SHORT(patch->height) << FRACBITS, spryscale)
            - FixedMul(vis->footclip, spryscale)) >> FRACBITS;
    else
        baseclip = -1;

    for (dc_x=vis->x1 ; dc_x<=vis->x2 ; dc_x++, frac += vis->xiscale)
    {
	static boolean error = 0;
        texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
        if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
	{
	    // [crispy] make non-fatal
	    if (!error)
	    {
                C_Printf (CR_GOLD, " R_DrawSpriteRange: bad texturecolumn\n");
                error++;
            }
	    continue;
	}
#endif
        column = (column_t *) ((byte *)patch +
                               LONG(patch->columnofs[texturecolumn]));
        R_DrawMaskedColumn (column, baseclip);
    }

    colfunc = basecolfunc;
}


int i;

//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
void R_ProjectSprite(mobj_t *thing)
{
    fixed_t            gzt;
    
    fixed_t            tx;
    
    int                x1;
    int                x2;

    spritedef_t*       sprdef;
    spriteframe_t*     sprframe;
    int                lump;
    
    unsigned           rot;
    boolean            flip;
    
    int                index;

    vissprite_t*       vis;
    
    angle_t            ang;
    fixed_t            iscale;
    
    fixed_t            interpx;
    fixed_t            interpy;
    fixed_t            interpz;
    fixed_t            interpangle;

    int                heightsec;

    sector_t           *sector = thing->subsector->sector;

    // [AM] Interpolate between current and last position,
    // if prudent.
    if (d_uncappedframerate &&
        // Don't interpolate if the mobj did something 
        // that would necessitate turning it off for a tic.
        thing->interp == true &&
        // Don't interpolate during a paused state.
        !paused && !menuactive)
    {
        interpx = thing->oldx + FixedMul(thing->x - thing->oldx, fractionaltic);
        interpy = thing->oldy + FixedMul(thing->y - thing->oldy, fractionaltic);
        interpz = thing->oldz + FixedMul(thing->z - thing->oldz, fractionaltic);
        interpangle = R_InterpolateAngle(thing->oldangle, thing->angle, fractionaltic);
    }
    else
    {
        interpx = thing->x;
        interpy = thing->y;
        interpz = thing->z;
        interpangle = thing->angle;
    }

    fixed_t            tr_x = interpx - viewx;
    fixed_t            tr_y = interpy - viewy;
    
    fixed_t            gxt = FixedMul(tr_x,viewcos);
    fixed_t            gyt = -FixedMul(tr_y,viewsin);
    fixed_t            tz = gxt-gyt;

    fixed_t            xscale = FixedDiv(projection, tz);

    // thing is behind view plane?
    if (tz < MINZ)
        return;
    
    gxt = -FixedMul(tr_x,viewsin); 
    gyt = FixedMul(tr_y,viewcos); 
    tx = -(gyt+gxt); 

    // too far off the side?
    if (abs(tx)>(tz<<2))
        return;
    
    // decide which patch to use for sprite relative to player
#ifdef RANGECHECK
    if ((unsigned int) thing->sprite >= (unsigned int) numsprites)
        I_Error ("R_ProjectSprite: invalid sprite number %i ",
                 thing->sprite);
#endif
    sprdef = &sprites[thing->sprite];
#ifdef RANGECHECK
    if ( (thing->frame&FF_FRAMEMASK) >= sprdef->numframes )
        I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ",
                 thing->sprite, thing->frame);
#endif
    sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
        // choose a different rotation based on player view
        ang = R_PointToAngle (interpx, interpy);
        if (sprframe->lump[0] == sprframe->lump[1])
            rot = (ang - interpangle + (angle_t)(ANG45 / 2) * 9) >> 28;
        else
            rot = (ang - interpangle + (angle_t)(ANG45 / 2) * 9 - (angle_t)(ANG180 / 16)) >> 28;
        lump = sprframe->lump[rot];
        flip = ((boolean)(sprframe->flip & (1 << rot)) || (thing->flags2 & MF2_MIRRORED));
    }
    else
    {
        // use single rotation for all views
        lump = sprframe->lump[0];
        flip = ((boolean)(sprframe->flip & 1) || (thing->flags2 & MF2_MIRRORED));
    }
    
    // calculate edges of the shape
    tx -= spriteoffset[lump];        
    x1 = (centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS;

    // off the right side?
    if (x1 > viewwidth)
        return;
    
    tx +=  spritewidth[lump];
    x2 = ((centerxfrac + FixedMul (tx,xscale) ) >>FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;
    
    gzt = interpz + spritetopoffset[lump];

    if (interpz > viewz + FixedDiv(viewheight << FRACBITS, xscale)
        || gzt < viewz - FixedDiv((viewheight << FRACBITS) - viewheight, xscale))
        return;

    // killough 3/27/98: exclude things totally separated
    // from the viewer, by either water or fake ceilings
    // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
    heightsec = sector->heightsec;

    if (heightsec != -1)   // only clip things which are in special sectors
    {
        int     phs = viewplayer->mo->subsector->sector->heightsec;

        if (phs != -1 && viewz < sectors[phs].floor_height ?
            thing->z >= sectors[heightsec].floor_height :
            gzt < sectors[heightsec].floor_height)
            return;
        if (phs != -1 && viewz > sectors[phs].ceiling_height ?
            gzt < sectors[heightsec].ceiling_height &&
            viewz >= sectors[heightsec].ceiling_height :
            thing->z >= sectors[heightsec].ceiling_height)
            return;
    }

    // store information in a vissprite
    vis = R_NewVisSprite (VST_THING);

    // killough 3/27/98: save sector for special clipping later
    vis->heightsec = heightsec;

    // [crispy] no color translation
    vis->translation = NULL;

    vis->mobjflags = thing->flags;
    vis->mobjflags2 = thing->flags2;
    vis->psprite = false;

    vis->scale = xscale<<(detailshift && !hires);                // CHANGED FOR HIRES
    vis->gx = interpx;
    vis->gy = interpy;
    vis->gz = interpz;
    vis->gzt = gzt;
    vis->colfunc = thing->colfunc;

    // foot clipping
    if ((thing->flags2 & MF2_FEETARECLIPPED) && interpz <= sector->interpfloorheight + FRACUNIT
        && heightsec == -1 && d_footclip)
    {
        int clipheight;

        if ((P_GetThingFloorType(thing) > 68  && P_GetThingFloorType(thing) < 73)  ||
                (P_GetThingFloorType(thing) > 50  && P_GetThingFloorType(thing) < 54)  ||
                (P_GetThingFloorType(thing) > 135 && P_GetThingFloorType(thing) < 144) ||
                (P_GetThingFloorType(thing) > 72  && P_GetThingFloorType(thing) < 77)  ||
                (P_GetThingFloorType(thing) > 88  && P_GetThingFloorType(thing) < 92)  ||
                (P_GetThingFloorType(thing) > 143 && P_GetThingFloorType(thing) < 148))
            clipheight = 10;
        else
            clipheight = 0;

        fixed_t clipfeet = MIN((spriteheight[lump] >> FRACBITS) / 4,
                               clipheight) << FRACBITS;

        vis->texturemid = gzt - viewz - clipfeet;
#ifdef ANIMATED_FLOOR_LIQUIDS
        if ((thing->flags2 & MF2_NOLIQUIDBOB) && sector->animate)
            clipfeet += sector->animate;
#endif
        vis->footclip = clipfeet;
    }
    else
    {
        vis->footclip = 0;

        vis->texturemid = gzt - viewz;
    }

    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;        
    iscale = FixedDiv (FRACUNIT, xscale);

    // [crispy] flip death sprites and corpses randomly
    if (!netgame && ((thing->type != MT_CYBORG &&
        thing->flags & MF_CORPSE && thing->health & 1) || thing->flags2 & MF2_MIRRORED))
    {
        flip = !!d_flipcorpses;
    }

    if (flip)
    {
        vis->startfrac = spritewidth[lump]-1;
        vis->xiscale = -iscale;
    }
    else
    {
        vis->startfrac = 0;
        vis->xiscale = iscale;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale*(vis->x1-x1);
    vis->patch = lump;
    
    // get light level
    // [crispy] do not invalidate colormap if invisibility is rendered translucently
    if (thing->flags & MF_SHADOW && !d_translucency)
    {
        // shadow draw
        vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
        // fixed map
        vis->colormap = fixedcolormap;
    }
    else if (thing->frame & FF_FULLBRIGHT)
    {
        // full bright
        vis->colormap = fixedcolormap ? fixedcolormap : colormaps;
    }
    else
    {
        // diminished light
        index = xscale>>(LIGHTSCALESHIFT-detailshift+hires);        // CHANGED FOR HIRES

        if (index >= MAXLIGHTSCALE) 
            index = MAXLIGHTSCALE-1;

        vis->colormap = spritelights[index];
    }        

    // [crispy] colored blood
    if (d_colblood && d_chkblood && thing->target &&
       (thing->type == MT_BLOOD ||
        thing->type == MT_GORE))
    {
        // [crispy] Thorn Things in Hacx bleed green blood
        if (gamemission == pack_hacx)
        {
            if (thing->target->type == MT_BABY)
            {
                vis->translation = crx[CRX_GREEN];
            }
        }
        else
        {
            // [crispy] Barons of Hell and Hell Knights bleed green blood
            if (thing->target->type == MT_BRUISER ||
                thing->target->type == MT_BETABRUISER ||
                thing->target->type == MT_KNIGHT)
            {
                vis->translation = crx[CRX_GREEN];
            }
            // [crispy] Cacodemons bleed blue blood
            else if (thing->target->type == MT_HEAD ||
                     thing->target->type == MT_BETAHEAD)
            {
                vis->translation = crx[CRX_BLUE];
            }
        }
    }
}


static void R_DrawMaskedShadowColumn(column_t *column)
{
    byte        topdelta;

    while ((topdelta = column->topdelta) != 0xFF)
    {
        int     length = column->length;

        // calculate unclipped screen coordinates for post
        int64_t topscreen = sprtopscreen + spryscale * topdelta;

        dc_yl = MAX((int)(((topscreen >> FRACBITS) + 1) / 10 + shift), mceilingclip[dc_x] + 1);
        dc_yh = MIN((int)(((topscreen + spryscale * length) >> FRACBITS) / 10 + shift),
            mfloorclip[dc_x] - 1);

        if (dc_yl <= dc_yh && dc_yh < viewheight)
            colfunc();
        column = (column_t *)((byte *)column + length + 4);
    }
}

void R_DrawShadowVisSprite(vissprite_t *vis)
{
    fixed_t     frac = vis->startfrac;
    fixed_t     xiscale = vis->xiscale;
    fixed_t     x2 = vis->x2;
    patch_t     *patch = W_CacheLumpNum(vis->patch + firstspritelump, PU_CACHE);

    colfunc = vis->colfunc;

    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(vis->texturemid, spryscale);
    shift = (sprtopscreen * 9 / 10) >> FRACBITS;

    for (dc_x = vis->x1; dc_x <= x2; dc_x++, frac += xiscale)
        R_DrawMaskedShadowColumn((column_t *)((byte *)patch
            + LONG(patch->columnofs[frac >> FRACBITS])));

    colfunc = basecolfunc;
}

//
// R_DrawShadowSprite
//
static void R_DrawShadowSprite(vissprite_t *spr)
{
    drawseg_t   *ds;
    int         clipbot[SCREENWIDTH];
    int         cliptop[SCREENWIDTH];
    int         x;

    for (x = spr->x1; x <= spr->x2; x++)
        clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (ds = ds_p; ds-- > drawsegs;)
    {
        int     r1;
        int     r2;

        // determine if the drawseg obscures the sprite
        if (ds->x1 > spr->x2 || ds->x2 < spr->x1 || (!ds->silhouette && !ds->maskedtexturecol))
            continue;       // does not cover sprite

        if (MAX(ds->scale1, ds->scale2) < spr->scale
            || (MIN(ds->scale1, ds->scale2) < spr->scale
            && !R_PointOnSegSide(spr->gx, spr->gy, ds->curline)))
            continue;       // seg is behind sprite

        r1 = MAX(ds->x1, spr->x1);
        r2 = MIN(ds->x2, spr->x2);

        // clip this piece of the sprite
        // killough 3/27/98: optimized and made much shorter
        if ((ds->silhouette & SIL_BOTTOM) && spr->gz < ds->bsilheight)  // bottom sil
            for (x = r1; x <= r2; x++)
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];

        if ((ds->silhouette & SIL_TOP) && spr->gzt > ds->tsilheight)    // top sil
            for (x = r1; x <= r2; x++)
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1; x <= spr->x2; x++)
    {
        if (clipbot[x] == -2)
            clipbot[x] = viewheight;

        if (cliptop[x] == -2)
            cliptop[x] = -1;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawShadowVisSprite(spr);
}

void R_ProjectShadow(mobj_t *thing)
{
    fixed_t             tx;

    fixed_t             xscale;

    int                 x1;
    int                 x2;

    spritedef_t         *sprdef;
    spriteframe_t       *sprframe;
    int                 lump;

    boolean             flip;

    vissprite_t         *vis;

    fixed_t             fz = thing->subsector->sector->interpfloorheight
                             + thing->shadow->info->shadowoffset;

    // transform the origin point
    fixed_t             tr_x;
    fixed_t             tr_y;

    fixed_t             interpx;
    fixed_t             interpy;
    fixed_t             interpz;
    fixed_t             interpangle;

    // [AM] Interpolate between current and last position,
    // if prudent.
    if (d_uncappedframerate &&
        // Don't interpolate if the mobj did something 
        // that would necessitate turning it off for a tic.
        thing->interp == true &&
        // Don't interpolate during a paused state.
        !paused && !menuactive)
    {
        interpx = thing->oldx + FixedMul(thing->x - thing->oldx, fractionaltic);
        interpy = thing->oldy + FixedMul(thing->y - thing->oldy, fractionaltic);
        interpz = thing->oldz + FixedMul(thing->z - thing->oldz, fractionaltic);
        interpangle = R_InterpolateAngle(thing->oldangle, thing->angle, fractionaltic);
    }
    else
    {
        interpx = thing->x;
        interpy = thing->y;
        interpz = thing->z;
        interpangle = thing->angle;
    }

    // transform the origin point
    tr_x = interpx - viewx;
    tr_y = interpy - viewy;
        
    fixed_t             gxt = FixedMul(tr_x, viewcos);
    fixed_t             gyt = -FixedMul(tr_y, viewsin);
    fixed_t             gzt;

    fixed_t             tz = gxt - gyt;

    // thing is behind view plane?
    if (tz < MINZ)
        return;

    xscale = FixedDiv(projection, tz);

    if (xscale < FRACUNIT / 2)
        return;

    gxt = -FixedMul(tr_x, viewsin);
    gyt = FixedMul(tr_y, viewcos);
    tx = -(gyt + gxt);

    // too far off the side?
    if (abs(tx) > (tz << 2))
        return;

    // decide which patch to use for sprite relative to player
    sprdef = &sprites[thing->sprite];
    sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
        // choose a different rotation based on player view
        angle_t rot;
        angle_t ang = R_PointToAngle(interpx, interpy);

        if (sprframe->lump[0] == sprframe->lump[1])
            rot = (ang - interpangle + (angle_t)(ANG45 / 2) * 9) >> 28;
        else
            rot = (ang - interpangle + (angle_t)(ANG45 / 2) * 9 - (angle_t)(ANG180 / 16)) >> 28;
        lump = sprframe->lump[rot];
        flip = ((boolean)(sprframe->flip & (1 << rot)) || (thing->flags2 & MF2_MIRRORED));
    }
    else
    {
        // use single rotation for all views
        lump = sprframe->lump[0];
        flip = ((boolean)(sprframe->flip & 1) || (thing->flags2 & MF2_MIRRORED));
    }

    // calculate edges of the shape
    tx -= (flip ? spritewidth[lump] - spriteoffset[lump] : spriteoffset[lump]);
    x1 = (centerxfrac + FRACUNIT / 2 + FixedMul(tx, xscale)) >> FRACBITS;

    // off the right side?
    if (x1 > viewwidth)
        return;

    x2 = ((centerxfrac + FRACUNIT / 2 + FixedMul(tx + spritewidth[lump], xscale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    gzt = interpz + spritetopoffset[lump];

    if (interpz > viewz + FixedDiv(viewheight << FRACBITS, xscale)
        || gzt < viewz - FixedDiv((viewheight << FRACBITS) - viewheight, xscale))
        return;

    // store information in a vissprite
    vis = R_NewVisSprite(VST_SHADOW);

    vis->heightsec = -1;
    vis->mobjflags = 0;
    vis->mobjflags2 = 0;
    vis->type = MT_SHADOW;
    vis->scale = xscale;
    vis->gx = interpx;
    vis->gy = interpy;
    vis->gz = interpz;
    vis->gzt = gzt;
    vis->colfunc = thing->colfunc;
    vis->texturemid = fz - viewz;

    vis->x1 = MAX(0, x1);
    vis->x2 = MIN(x2, viewwidth - 1);

    if (flip)
    {
        vis->startfrac = spritewidth[lump] - 1;
        vis->xiscale = -FixedDiv(FRACUNIT, xscale);
    }
    else
    {
        vis->startfrac = 0;
        vis->xiscale = FixedDiv(FRACUNIT, xscale);
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);
    vis->patch = lump;
}


//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites (sector_t* sec)
{
    mobj_t*                thing;
    int                    lightnum;
    short                  floorpic = sec->floorpic;

    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == validcount)
        return;                

    // Well, now it will be done.
    sec->validcount = validcount;
        
    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT)+extralight*LIGHTBRIGHT;

    if (lightnum < 0)                
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
    else
        spritelights = scalelight[lightnum];

    // Handle all things in sector.
    if (fixedcolormap || isliquid[floorpic] || floorpic == skyflatnum || !d_shadows)
        for (thing = sec->thinglist; thing; thing = thing->snext)
        {
            if (thing->type != MT_SHADOW)
                thing->projectfunc((mobj_t *)thing);
        }
    else
        for (thing = sec->thinglist; thing; thing = thing->snext)
            thing->projectfunc((mobj_t *)thing);
}


//
// R_DrawPSprite
//
void R_DrawPSprite (pspdef_t* psp, psprnum_t psprnum)
{
    fixed_t            tx;
    int                x1;
    int                x2;
    spritedef_t*       sprdef;
    spriteframe_t*     sprframe;
    int                lump;
    boolean            flip;
    vissprite_t*       vis;
    vissprite_t        avis;
    player_t*          player = &players[consoleplayer];
    
    // decide which patch to use
#ifdef RANGECHECK
    if ( (unsigned)psp->state->sprite >= (unsigned int) numsprites)
        I_Error ("R_DrawPSprite: invalid sprite number %i ",
                 psp->state->sprite);
#endif
    sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ( (psp->state->frame & FF_FRAMEMASK)  >= sprdef->numframes)
        I_Error ("R_DrawPSprite: invalid sprite frame %i : %i ",
                 psp->state->sprite, psp->state->frame);
#endif
    sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

    lump = sprframe->lump[0];
    flip = (boolean)(sprframe->flip & 1);
    
    // calculate edges of the shape
    tx = psp->sx-160*FRACUNIT;
        
    tx -= spriteoffset[lump];        
    x1 = (centerxfrac + FixedMul (tx,pspritescale) ) >>FRACBITS;

    // off the right side
    if (x1 > viewwidth)
        return;                

    tx +=  spritewidth[lump];
    x2 = ((centerxfrac + FixedMul (tx, pspritescale) ) >>FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;
    
    // store information in a vissprite
    vis = &avis;

    // [crispy] no color translation
    vis->translation = NULL;

    vis->mobjflags = 0;
    vis->mobjflags2 = 0;
    vis->psprite = true;

    vis->texturemid = (BASEYCENTER << FRACBITS) - (psp->sy - spritetopoffset[lump]);

    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;        
    vis->footclip = 0;

    vis->scale = pspritescale<<(detailshift && !hires);                // CHANGED FOR HIRES
    
    if (flip)
    {
        vis->xiscale = -pspriteiscale;
        vis->startfrac = spritewidth[lump]-1;
    }
    else
    {
        vis->xiscale = pspriteiscale;
        vis->startfrac = 0;
    }
    
    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale*(vis->x1-x1);

    vis->patch = lump;

    // [crispy] do not invalidate colormap if invisibility is rendered translucently
    if ((viewplayer->powers[pw_invisibility] > 4*32
        || viewplayer->powers[pw_invisibility] & 8)
        && !d_translucency && !beta_style)
    {
        // shadow draw
        vis->colormap = NULL;
    }
    else if (fixedcolormap)
    {
        // fixed color
        vis->colormap = fixedcolormap;
    }
    else if (psp->state->frame & FF_FULLBRIGHT)
    {
        // full bright
        vis->colormap = colormaps;
    }
    else
    {
        // local light
        vis->colormap = spritelights[MAXLIGHTSCALE-1];
    }
        
    // [crispy] invisibility is rendered translucently
    if ((viewplayer->powers[pw_invisibility] > 4*32 ||
        viewplayer->powers[pw_invisibility] & 8) &&
        d_translucency)
    {
        vis->mobjflags |= MF_TRANSLUCENT;
    }

    // [crispy] translucent gun flash sprites
    if (psprnum == ps_flash && (!beta_style ||
                               ( beta_style && player->readyweapon != wp_chaingun)))
        vis->mobjflags |= MF_TRANSLUCENT;

    //e6y: interpolation for weapon bobbing
    if (d_uncappedframerate)
    {
        typedef struct interpolate_s
        {
            int x1;
            int x1_prev;
            int texturemid;
            int texturemid_prev;
            int lump;
        } psp_interpolate_t;

        static psp_interpolate_t        psp_inter;

        if (realframe)
        {
            psp_inter.x1 = psp_inter.x1_prev;
            psp_inter.texturemid = psp_inter.texturemid_prev;
        }

        psp_inter.x1_prev = vis->x1;
        psp_inter.texturemid_prev = vis->texturemid;

        if (lump == psp_inter.lump && !skippsprinterp)
        {
            int deltax = vis->x2 - vis->x1;

            vis->x1 = psp_inter.x1 + FixedMul(fractionaltic, vis->x1 - psp_inter.x1);
            vis->x2 = vis->x1 + deltax;
            vis->texturemid = psp_inter.texturemid
                + FixedMul(fractionaltic, vis->texturemid - psp_inter.texturemid);
        }
        else
        {
            skippsprinterp = false;
            psp_inter.x1 = vis->x1;
            psp_inter.texturemid = vis->texturemid;
            psp_inter.lump = lump;
        }
    }

    R_DrawVisSprite (vis, vis->x1, vis->x2);
}



//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites (void)
{
    int                i;
    int                lightnum;
    pspdef_t*          psp;
    
    // get light level
    lightnum =
        (viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT) 
        +extralight*LIGHTBRIGHT;

    if (lightnum < 0)                
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
    else
        spritelights = scalelight[lightnum];
    
    // clip to screen bounds
    mfloorclip = screenheightarray;
    mceilingclip = negonearray;
    
    // add all active psprites
    for (i=0, psp=viewplayer->psprites;
         i<NUMPSPRITES;
         i++,psp++)
    {
        if (psp->state)
            R_DrawPSprite (psp, i); // [crispy] pass gun or flash sprite
    }
}




//
// R_SortVisSprites
//
// Rewritten by Lee Killough to avoid using unnecessary
// linked lists, and to use faster sorting algorithm.
//
#define bcopyp(d, s, n) memcpy(d, s, (n) * sizeof(void *))

// killough 9/2/98: merge sort
static void msort(vissprite_t **s, vissprite_t **t, int n)
{
    if (n >= 16)
    {
        int             n1 = n / 2;
        int             n2 = n - n1;
        vissprite_t     **s1 = s;
        vissprite_t     **s2 = s + n1;
        vissprite_t     **d = t;

        msort(s1, t, n1);
        msort(s2, t, n2);

        while ((*s1)->scale > (*s2)->scale ? (*d++ = *s1++, --n1) : (*d++ = *s2++, --n2));

        if (n2)
            bcopyp(d, s2, n2);
        else
            bcopyp(d, s1, n1);

        bcopyp(s, t, n);
    }
    else
    {
        int     i;

        for (i = 1; i < n; i++)
        {
            vissprite_t *temp = s[i];

            if (s[i - 1]->scale < temp->scale)
            {
                int     j = i;

                while ((s[j] = s[j - 1])->scale < temp->scale && --j);
                s[j] = temp;
            }
        }
    }
}

void R_SortVisSprites (void)
{
    if (num_vissprite[VST_THING])
    {
        int     i;

        // If we need to allocate more pointers for the vissprites,
        // allocate as many as were allocated for sprites -- killough
        // killough 9/22/98: allocate twice as many
        if (num_vissprite_ptrs < num_vissprite[VST_THING] * 2)
        {
            free(vissprite_ptrs);
            vissprite_ptrs =
                    (vissprite_t **)malloc((num_vissprite_ptrs = num_vissprite_alloc[VST_THING] * 2)
                            * sizeof(*vissprite_ptrs));
        }

        for (i = num_vissprite[VST_THING]; --i >= 0;)
        {
            vissprite_t     *spr = vissprites[VST_THING] + i;

            spr->drawn = false;
            vissprite_ptrs[i] = spr;
        }

        // killough 9/22/98: replace qsort with merge sort, since the keys
        // are roughly in order to begin with, due to BSP rendering.
        msort(vissprite_ptrs, vissprite_ptrs + num_vissprite[VST_THING], num_vissprite[VST_THING]);
    }
}



//
// R_DrawSprite
//
void R_DrawSprite (vissprite_t* spr)
{
    drawseg_t*         ds;

    int                clipbot[SCREENWIDTH]; // [crispy] 32-bit integer math
    int                cliptop[SCREENWIDTH]; // [crispy] 32-bit integer math
    int                x;
    int                r1;
    int                r2;
    fixed_t            scale;
    fixed_t            lowscale;
    int                silhouette;
                
    for (x = spr->x1 ; x<=spr->x2 ; x++)
        clipbot[x] = cliptop[x] = -2;
    
    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
    {
        // determine if the drawseg obscures the sprite
        if (ds->x1 > spr->x2
            || ds->x2 < spr->x1
            || (!ds->silhouette
                && !ds->maskedtexturecol) )
        {
            // does not cover sprite
            continue;
        }
                        
        r1 = ds->x1 < spr->x1 ? spr->x1 : ds->x1;
        r2 = ds->x2 > spr->x2 ? spr->x2 : ds->x2;

        if (ds->scale1 > ds->scale2)
        {
            lowscale = ds->scale2;
            scale = ds->scale1;
        }
        else
        {
            lowscale = ds->scale1;
            scale = ds->scale2;
        }
                
        if (scale < spr->scale
            || ( lowscale < spr->scale
                 && !R_PointOnSegSide (spr->gx, spr->gy, ds->curline) ) )
        {
            // masked mid texture?
            if (ds->maskedtexturecol)        
                R_RenderMaskedSegRange (ds, r1, r2);
            // seg is behind sprite
            continue;                        
        }

        
        // clip this piece of the sprite
        silhouette = ds->silhouette;
        
        if (spr->gz >= ds->bsilheight)
            silhouette &= ~SIL_BOTTOM;

        if (spr->gzt <= ds->tsilheight)
            silhouette &= ~SIL_TOP;
                        
        if (silhouette == 1)
        {
            // bottom sil
            for (x=r1 ; x<=r2 ; x++)
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
        }
        else if (silhouette == 2)
        {
            // top sil
            for (x=r1 ; x<=r2 ; x++)
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
        }
        else if (silhouette == 3)
        {
            // both
            for (x=r1 ; x<=r2 ; x++)
            {
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
            }
        }
                
    }
    
        // killough 3/27/98:
        // Clip the sprite against deep water and/or fake ceilings.
        // killough 4/9/98: optimize by adding mh
        // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
        // killough 11/98: fix disappearing sprites
        if (spr->heightsec != -1)  // only things in specially marked sectors
        {
            fixed_t h, mh;
            int phs = viewplayer->mo->subsector->sector->heightsec;
            if ((mh = sectors[spr->heightsec].floor_height) > spr->gz &&
                (h = centeryfrac - FixedMul(mh -= viewz, spr->scale)) >= 0 &&
                (h >>= FRACBITS) < viewheight)
            {
                if (mh <= 0 || (phs != -1 && viewz > sectors[phs].floor_height))
                {                          // clip bottom
                    for (x = spr->x1; x <= spr->x2; x++)
                        if (clipbot[x] == -2 || h < clipbot[x])
                            clipbot[x] = h;
                }
                else                        // clip top
                    if (phs != -1 && viewz <= sectors[phs].floor_height) // killough 11/98
                        for (x = spr->x1; x <= spr->x2; x++)
                            if (cliptop[x] == -2 || h > cliptop[x])
                                cliptop[x] = h;
            }

            if ((mh = sectors[spr->heightsec].ceiling_height) < spr->gzt &&
                (h = centeryfrac - FixedMul(mh - viewz, spr->scale)) >= 0 &&
                (h >>= FRACBITS) < viewheight)
            {
                if (phs != -1 && viewz >= sectors[phs].ceiling_height)
                {                         // clip bottom
                    for (x = spr->x1; x <= spr->x2; x++)
                        if (clipbot[x] == -2 || h < clipbot[x])
                            clipbot[x] = h;
                }
                else                       // clip top
                    for (x = spr->x1; x <= spr->x2; x++)
                        if (cliptop[x] == -2 || h > cliptop[x])
                            cliptop[x] = h;
            }
        }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1 ; x<=spr->x2 ; x++)
    {
        if (clipbot[x] == -2)                
            clipbot[x] = viewheight;

        if (cliptop[x] == -2)
            cliptop[x] = -1;
    }
                
    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite (spr, spr->x1, spr->x2);
}




//
// R_DrawMasked
//
void R_DrawMasked (void)
{
    int                i;

    drawseg_t*         ds;
        
    R_SortVisSprites ();

    // draw all shadows
    for (i = num_vissprite[VST_SHADOW]; --i >= 0;)
        R_DrawShadowSprite(&vissprites[VST_SHADOW][i]);

    // draw all other vissprites, back to front
    for (i = num_vissprite[VST_THING]; --i >= 0;)
    {
        vissprite_t     *spr = vissprite_ptrs[i];

        if (!spr->drawn)
            R_DrawSprite(spr);
    }
    
    // render any remaining masked mid textures
    for (ds=ds_p-1 ; ds >= drawsegs ; ds--)
        if (ds->maskedtexturecol)
            R_RenderMaskedSegRange (ds, ds->x1, ds->x2);
    
    // draw the psprites on top of everything
    //  but does not draw on side views
    if (!viewangleoffset)                
        R_DrawPlayerSprites ();
}

