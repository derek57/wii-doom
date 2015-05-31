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
//        Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------




#include <stdio.h>
#include <stdlib.h>

#include "deh_main.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_swap.h"
#include "i_system.h"
#include "r_local.h"
#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"


#define MINZ                    (FRACUNIT*4)
#define BASEYCENTER             100
#define MAX_SPRITE_FRAMES       29

// invisibility is rendered translucently
#define TRANSLUCENT_SHADOW      0


typedef struct
{
    int                x1;
    int                x2;
        
    int                column;
    int                topclip;
    int                bottomclip;

} maskdraw_t;



char*                  spritename;

// constant arrays
//  used for psprite clipping and initializing clipping
int                    negonearray[SCREENWIDTH];          // CHANGED FOR HIRES
int                    screenheightarray[SCREENWIDTH];    // CHANGED FOR HIRES
int                    numsprites;
int                    maxframe;
int                    newvissprite;

int*                   mfloorclip;                        // CHANGED FOR HIRES
int*                   mceilingclip;                      // CHANGED FOR HIRES

int64_t                sprtopscreen;                      // WiggleFix

static int             num_vissprite, num_vissprite_alloc, num_vissprite_ptrs;

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//  and range check thing_t sprites patches
spritedef_t*           sprites;

spriteframe_t          sprtemp[MAX_SPRITE_FRAMES];

vissprite_t*           vissprites = NULL, **vissprite_ptrs;          // killough
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


//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
void R_InstallSpriteLump(lumpinfo_t *lump, int lumpnum, unsigned int frame,
                         unsigned int rotation, boolean flipped)
{
    if (frame >= MAX_SPRITE_FRAMES || rotation > 8)
        I_Error("R_InstallSpriteLump: Bad frame characters in lump %s", lump->name);

    if ((int)frame > maxframe)
        maxframe = frame;

    if (rotation == 0)
    {
        int r;

        // the lump should be used for all rotations
        for (r = 0; r < 8; r++)
        {
            if (sprtemp[frame].lump[r] == -1)
            {
                sprtemp[frame].lump[r] = lumpnum - firstspritelump;
                sprtemp[frame].flip[r] = (byte)flipped;
                sprtemp[frame].rotate = false;
            }
        }
        return;
    }

    // the lump is only used for one rotation
    if (sprtemp[frame].lump[--rotation] == -1)
    {
        sprtemp[frame].lump[rotation] = lumpnum - firstspritelump;
        sprtemp[frame].flip[rotation] = (byte)flipped;
        sprtemp[frame].rotate = true;
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
        int     j = R_SpriteNameHash(lumpinfo[i + firstspritelump].name) % numentries;

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
            memset(sprtemp, -1, sizeof(sprtemp));
            maxframe = -1;
            do
            {
                lumpinfo_t      *lump = &lumpinfo[j + firstspritelump];

                // Fast portable comparison -- killough
                // (using int pointer cast is nonportable):
                if (!((lump->name[0] ^ spritename[0]) |
                      (lump->name[1] ^ spritename[1]) |
                      (lump->name[2] ^ spritename[2]) |
                      (lump->name[3] ^ spritename[3])))
                {
                    R_InstallSpriteLump(lump, j + firstspritelump, lump->name[4] - 'A', 
                        lump->name[5] - '0', false);
                    if (lump->name[6])
                        R_InstallSpriteLump(lump, j + firstspritelump, lump->name[6] - 'A',
                           lump->name[7] - '0', true);
                }
            } while ((j = hash[j].next) >= 0);

            // check the frames that were found for completeness
            if ((sprites[i].numframes = ++maxframe))  // killough 1/31/98
            {
                int     frame;

                for (frame = 0; frame < maxframe; frame++)
                    switch ((int)sprtemp[frame].rotate)
                    {
                        case -1:
                            // no rotations were found for that frame at all
                            break;

                        case 0:
                            // only the first rotation is needed
                            break;

                        case 1:
                            // must have all 8 frames
                        {
                            int rotation;

                            for (rotation = 0; rotation < 8; rotation++)
                                if (sprtemp[frame].lump[rotation] == -1)
                                    I_Error("R_InitSprites: Sprite %.8s frame %c is missing rotations",
                                        namelist[i], frame + 'A');
                            break;
                        }
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
    num_vissprite = 0;          // killough
}


//
// R_NewVisSprite
//
vissprite_t* R_NewVisSprite (void)
{
    if (num_vissprite >= num_vissprite_alloc)           // killough
    {
        num_vissprite_alloc = (num_vissprite_alloc ? num_vissprite_alloc * 2 : 128);
        vissprites = realloc(vissprites, num_vissprite_alloc * sizeof(*vissprites));
    }
    return (vissprites + num_vissprite++);
}



//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//

void R_DrawMaskedColumn (column_t* column, signed int baseclip)
{
    int64_t            topscreen;                         // WiggleFix
    int64_t            bottomscreen;                      // WiggleFix
    fixed_t            basetexturemid;
        
    basetexturemid = dc_texturemid;
    dc_texheight = 0;                                     // Tutti-Frutti fix
        
    for ( ; column->topdelta != 0xff ; ) 
    {
        // calculate unclipped screen coordinates
        //  for post
        topscreen = sprtopscreen + spryscale*column->topdelta;
        bottomscreen = topscreen + spryscale*column->length;

        dc_yl = (int)((topscreen+FRACUNIT-1)>>FRACBITS);  // WiggleFix
        dc_yh = (int)((bottomscreen-1)>>FRACBITS);        // WiggleFix
                
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
    fixed_t            sprbotscreen;        
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
    else if (vis->translation)
    {
	colfunc = transcolfunc;
	dc_translation = vis->translation;
    }
        
    // translucent sprites
    if (d_translucency && dc_colormap &&
        ((vis->mobjflags & MF_TRANSLUCENT) ||
        ((vis->mobjflags & MF_SHADOW) && TRANSLUCENT_SHADOW)))
    {
	colfunc = tlcolfunc;
    }

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

    if (vis->footclip && !vis->psprite && d_footclip)
    {
        sprbotscreen = sprtopscreen + FixedMul(SHORT(patch->height) << FRACBITS, // WII FIX
                                               spryscale);

        baseclip = (sprbotscreen - FixedMul(vis->footclip << FRACBITS,
                                            spryscale)) >> FRACBITS;
    }
    else
    {
        baseclip = -1;
    }

    for (dc_x=vis->x1 ; dc_x<=vis->x2 ; dc_x++, frac += vis->xiscale)
    {
        texturecolumn = frac>>FRACBITS;
#ifdef RANGECHECK
        if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
            I_Error ("R_DrawSpriteRange: bad texturecolumn");
#endif
        column = (column_t *) ((byte *)patch +
                               LONG(patch->columnofs[texturecolumn]));
        R_DrawMaskedColumn (column, baseclip);
    }

    colfunc = basecolfunc;
}



//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
void R_ProjectSprite (mobj_t* thing)
{
    fixed_t            tr_x;
    fixed_t            tr_y;
    
    fixed_t            gxt;
    fixed_t            gyt;
    
    fixed_t            tx;
    fixed_t            tz;

    fixed_t            xscale;
    
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

    // Interpolate between current and last position,
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
        
    gxt = FixedMul(tr_x,viewcos); 
    gyt = -FixedMul(tr_y,viewsin);
    
    tz = gxt-gyt; 

    // thing is behind view plane?
    if (tz < MINZ)
        return;
    
    xscale = FixedDiv(projection, tz);
        
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
	rot = (ang-interpangle+(unsigned)(ANG45/2)*9)>>29;
        lump = sprframe->lump[rot];
        flip = (boolean)sprframe->flip[rot];
    }
    else
    {
        // use single rotation for all views
        lump = sprframe->lump[0];
        flip = (boolean)sprframe->flip[0];
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
    
    // store information in a vissprite
    vis = R_NewVisSprite ();

    // no color translation
    vis->translation = NULL;

    vis->mobjflags = thing->flags;
    vis->psprite = false;

    vis->scale = xscale<<(detailshift && !hires);                // CHANGED FOR HIRES
    vis->gx = interpx;
    vis->gy = interpy;
    vis->gz = interpz;
    vis->gzt = interpz + spritetopoffset[lump];

    // foot clipping
    if (thing->flags2 & MF2_FEETARECLIPPED && d_footclip
        && thing->z <= thing->subsector->sector->floor_height)
    {
        vis->footclip = 10;
    }
    else
        vis->footclip = 0;

    vis->texturemid = vis->gzt - viewz - (vis->footclip << FRACBITS);

    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;        
    iscale = FixedDiv (FRACUNIT, xscale);

    // flip death sprites and corpses randomly
    if (!netgame && thing->type != MT_CYBORG &&
        thing->flags & MF_CORPSE && thing->health & 1)
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
    // do not invalidate colormap if invisibility is rendered translucently
    if (thing->flags & MF_SHADOW && !TRANSLUCENT_SHADOW)
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

    // colored blood
    if (d_colblood && d_chkblood && thing->target &&
       (thing->type == MT_BLOOD ||
        thing->type == MT_GORE ||
        thing->type == MT_CHUNK ||
        thing->type == MT_FLESH))
    {
        // Thorn Things in Hacx bleed green blood
	if (gamemission == pack_hacx)
	{
	    if (thing->target->type == MT_BABY)
	    {
		vis->translation = crx[CRX_GREEN];
	    }
	}
	else
	{
	    // Barons of Hell and Hell Knights bleed green blood
	    if (thing->target->type == MT_BRUISER || thing->target->type == MT_KNIGHT)
	    {
		vis->translation = crx[CRX_GREEN];
	    }
	    // Cacodemons bleed blue blood
	    else if (thing->target->type == MT_HEAD)
	    {
		vis->translation = crx[CRX_BLUE];
	    }
	}
    }
}




//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
void R_AddSprites (sector_t* sec)
{
    mobj_t*                thing;
    int                    lightnum;

    // BSP is traversed by subsector.
    // A sector might have been split into several
    //  subsectors during BSP building.
    // Thus we check whether its already added.
    if (sec->validcount == validcount)
        return;                

    // Well, now it will be done.
    sec->validcount = validcount;
        
    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (lightnum < 0)                
        spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        spritelights = scalelight[LIGHTLEVELS-1];
    else
        spritelights = scalelight[lightnum];

    // Handle all things in sector.
    for (thing = sec->thinglist ; thing ; thing = thing->snext)
        R_ProjectSprite (thing);
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
        I_Error ("R_ProjectSprite: invalid sprite number %i ",
                 psp->state->sprite);
#endif
    sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
    if ( (psp->state->frame & FF_FRAMEMASK)  >= sprdef->numframes)
        I_Error ("R_ProjectSprite: invalid sprite frame %i : %i ",
                 psp->state->sprite, psp->state->frame);
#endif
    sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

    lump = sprframe->lump[0];
    flip = (boolean)sprframe->flip[0];
    
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

    // no color translation
    vis->translation = NULL;

    vis->mobjflags = 0;
    vis->psprite = true;
    vis->texturemid = (BASEYCENTER << FRACBITS) - (psp->sy - spritetopoffset[lump]); // HIRES
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;        

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

    // do not invalidate colormap if invisibility is rendered translucently
    if ((viewplayer->powers[pw_invisibility] > 4*32
        || viewplayer->powers[pw_invisibility] & 8)
	&& !TRANSLUCENT_SHADOW && !beta_style)
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
        
    // invisibility is rendered translucently
    if ((viewplayer->powers[pw_invisibility] > 4*32 ||
        viewplayer->powers[pw_invisibility] & 8) &&
        TRANSLUCENT_SHADOW)
    {
	vis->mobjflags |= MF_TRANSLUCENT;
    }

    // translucent gun flash sprites
    if (psprnum == ps_flash && (!beta_style ||
                               ( beta_style && player->readyweapon != wp_chaingun)))
        vis->mobjflags |= MF_TRANSLUCENT;

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
        +extralight;

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
            R_DrawPSprite (psp, i);
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
    if (num_vissprite)
    {
        int     i;

        // If we need to allocate more pointers for the vissprites,
        // allocate as many as were allocated for sprites -- killough
        // killough 9/22/98: allocate twice as many
        if (num_vissprite_ptrs < num_vissprite * 2)
        {
            free(vissprite_ptrs);
            vissprite_ptrs = (vissprite_t **)malloc((num_vissprite_ptrs = num_vissprite_alloc * 2)
                * sizeof(*vissprite_ptrs));
        }

        for (i = num_vissprite; --i >= 0;)
        {
            vissprite_t     *spr = vissprites + i;

            spr->drawn = false;
            vissprite_ptrs[i] = spr;
        }

        // killough 9/22/98: replace qsort with merge sort, since the keys
        // are roughly in order to begin with, due to BSP rendering.
        msort(vissprite_ptrs, vissprite_ptrs + num_vissprite, num_vissprite);
    }
}



//
// R_DrawSprite
//
void R_DrawSprite (vissprite_t* spr)
{
    drawseg_t*         ds;

    int                clipbot[SCREENWIDTH];                        // CHANGED FOR HIRES
    int                cliptop[SCREENWIDTH];                        // CHANGED FOR HIRES
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

    // draw all other vissprites, back to front
    for (i = num_vissprite; --i >= 0;)
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

