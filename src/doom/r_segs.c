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
//        All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#ifdef WII
#include "../c_io.h"
#else
#include "c_io.h"
#endif

#include "doomdef.h"

#ifdef WII
#include "../doomfeatures.h"
#else
#include "doomfeatures.h"
#endif

#include "doomstat.h"

#ifdef WII
#include "../i_system.h"
#else
#include "i_system.h"
#endif

#include "r_local.h"
#include "r_sky.h"

#ifdef WII
#include "../v_trans.h"
#include "../z_zone.h"
#else
#include "v_trans.h"
#include "z_zone.h"
#endif


#define HEIGHTBITS     12
#define HEIGHTUNIT     (1<<HEIGHTBITS)


// OPTIMIZE: closed two sided lines as single sided

// True if any of the segs textures might be visible.
boolean                segtextured;        

// False if the back side is the same plane.
boolean                markfloor;        
boolean                markceiling;
boolean                maskedtexture;

int                    toptexture;
int                    bottomtexture;
int                    midtexture;
int                    worldtop;
int                    worldbottom;
int                    worldhigh;
int                    worldlow;

// regular wall
int                    rw_x;
int                    rw_stopx;

// angle to line origin
int                    rw_angle1;        

int*                   maskedtexturecol; // [crispy] 32-bit integer math

int64_t                pixhigh;          // [crispy] WiggleFix
int64_t                pixlow;           // [crispy] WiggleFix
int64_t                topfrac;          // [crispy] WiggleFix
int64_t                bottomfrac;       // [crispy] WiggleFix

static int             max_rwscale = 64 * FRACUNIT;
static int             heightbits = 12;
static int             heightunit = (1 << 12);
static int             invhgtbits = 4;

static fixed_t         toptexheight;
static fixed_t         midtexheight;
static fixed_t         bottomtexheight;

static byte            *toptexfullbright;
static byte            *midtexfullbright;
static byte            *bottomtexfullbright;

angle_t                rw_normalangle;
angle_t                rw_centerangle;

fixed_t                rw_offset;
fixed_t                rw_distance;
fixed_t                rw_scale;
fixed_t                rw_scalestep;
fixed_t                rw_midtexturemid;
fixed_t                rw_toptexturemid;
fixed_t                rw_bottomtexturemid;
fixed_t                pixhighstep;
fixed_t                pixlowstep;
fixed_t                topstep;
fixed_t                bottomstep;

lighttable_t**         walllights;


// [crispy] WiggleFix: add this code block near the top of r_segs.c
//
// R_FixWiggle()
// Dynamic wall/texture rescaler, AKA "WiggleHack II"
//  by Kurt "kb1" Baumgardner ("kb") and Andrey "Entryway" Budko ("e6y")
//
//  [kb] When the rendered view is positioned, such that the viewer is
//   looking almost parallel down a wall, the result of the scale
//   calculation in R_ScaleFromGlobalAngle becomes very large. And, the
//   taller the wall, the larger that value becomes. If these large
//   values were used as-is, subsequent calculations would overflow,
//   causing full-screen HOM, and possible program crashes.
//
//  Therefore, vanilla Doom clamps this scale calculation, preventing it
//   from becoming larger than 0x400000 (64*FRACUNIT). This number was
//   chosen carefully, to allow reasonably-tight angles, with reasonably
//   tall sectors to be rendered, within the limits of the fixed-point
//   math system being used. When the scale gets clamped, Doom cannot
//   properly render the wall, causing an undesirable wall-bending
//   effect that I call "floor wiggle". Not a crash, but still ugly.
//
//  Modern source ports offer higher video resolutions, which worsens
//   the issue. And, Doom is simply not adjusted for the taller walls
//   found in many PWADs.
//
//  This code attempts to correct these issues, by dynamically
//   adjusting the fixed-point math, and the maximum scale clamp,
//   on a wall-by-wall basis. This has 2 effects:
//
//  1. Floor wiggle is greatly reduced and/or eliminated.
//  2. Overflow is no longer possible, even in levels with maximum
//     height sectors (65535 is the theoretical height, though Doom
//     cannot handle sectors > 32767 units in height.
//
//  The code is not perfect across all situations. Some floor wiggle can
//   still be seen, and some texture strips may be slightly misaligned in
//   extreme cases. These effects cannot be corrected further, without
//   increasing the precision of various renderer variables, and,
//   possibly, creating a noticable performance penalty.
//

static const struct
{
    int clamp;
    int heightbits;
} scale_values[8] = {
    {2048 * FRACUNIT, 12},
    {1024 * FRACUNIT, 12},
    {1024 * FRACUNIT, 11},
    { 512 * FRACUNIT, 11},
    { 512 * FRACUNIT, 10},
    { 256 * FRACUNIT, 10},
    { 256 * FRACUNIT,  9},
    { 128 * FRACUNIT,  9}
};

void R_FixWiggle (sector_t *sector)
{
    static int  lastheight = 0;

    // disallow negative heights, force cache initialization
    int         height = MAX(1, (sector->interpceilingheight - sector->interpfloorheight) >> FRACBITS);

    // disallow negative heights. using 1 forces cache initialization
    if (height < 1)
        height = 1;

    // early out?
    if (height != lastheight)
    {
        lastheight = height;

        // initialize, or handle moving sector
        if (height != sector->cachedheight)
        {
            sector->cachedheight = height;
            sector->scaleindex = 0;
            height >>= 7;

            // calculate adjustment
            while (height >>= 1)
                sector->scaleindex++;
        }

        // fine-tune renderer for this wall
        max_rwscale = scale_values[sector->scaleindex].clamp;
        heightbits = scale_values[sector->scaleindex].heightbits;
        heightunit = (1 << heightbits);
        invhgtbits = FRACBITS - heightbits;
    }
}

//
// R_RenderMaskedSegRange
//
void
R_RenderMaskedSegRange
( drawseg_t*        ds,
  int               x1,
  int               x2 )
{
    unsigned        index;
    column_t*       col;
    int             lightnum;
    int             texnum;
    fixed_t         texheight;
    int64_t         t;

    // Calculate light table.
    // Use different light tables
    //   for horizontal / vertical / diagonal. Diagonal?
    // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
    curline = ds->curline;
    frontsector = curline->frontsector;
    backsector = curline->backsector;

//    texnum = texturetranslation[curline->sidedef->midtexture];

    // cph 2001/11/25 - middle textures did not animate in v1.2
    texnum = curline->sidedef->midtexture;

    if (!d_maskedanim)
        texnum = texturetranslation[texnum];

    texheight = textureheight[texnum];
        
    lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight*LIGHTBRIGHT;

    if (curline->v1->y == curline->v2->y)
        lightnum-=LIGHTBRIGHT;
    else if (curline->v1->x == curline->v2->x)
        lightnum+=LIGHTBRIGHT;

    if (lightnum < 0)                
        walllights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
        walllights = scalelight[LIGHTLEVELS-1];
    else
        walllights = scalelight[lightnum];

    maskedtexturecol = ds->maskedtexturecol;

    rw_scalestep = ds->scalestep;                
    spryscale = ds->scale1 + (x1 - ds->x1)*rw_scalestep;
    mfloorclip = ds->sprbottomclip;
    mceilingclip = ds->sprtopclip;
    
    // find positioning
    if (curline->linedef->flags & ML_DONTPEGBOTTOM)
        dc_texturemid = MAX(frontsector->interpfloorheight, backsector->interpfloorheight)
            + texheight - viewz + curline->sidedef->rowoffset;
    else
        dc_texturemid = MIN(frontsector->interpceilingheight, backsector->interpceilingheight)
            - viewz + curline->sidedef->rowoffset;

    if (fixedcolormap)
        dc_colormap = fixedcolormap;
    
    // draw the columns
    for (dc_x = x1 ; dc_x <= x2 ; dc_x++)
    {
        // calculate lighting
        if (maskedtexturecol[dc_x] != INT_MAX) // [crispy] 32-bit integer math
        {
            if (!fixedcolormap)
            {
                // CHANGED FOR HIRES
                index = spryscale>>(LIGHTSCALESHIFT - (hires) + hires);

                if (index >=  MAXLIGHTSCALE )
                    index = MAXLIGHTSCALE-1;

                dc_colormap = walllights[index];
            }

            // [crispy] apply Killough's int64 sprtopscreen overflow fix
            // from winmbf/Source/r_segs.c:174-191
            //
            // This calculation used to overflow and cause crashes in Doom:
            //
            // sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
            //
            // This code fixes it, by using double-precision intermediate
            // arithmetic and by skipping the drawing of 2s normals whose
            // mapping to screen coordinates is totally out of range:

            t = ((int64_t) centeryfrac << FRACBITS) -
                         (int64_t) dc_texturemid * spryscale;

            if (t + (int64_t)texheight * spryscale < 0 ||
                t > (int64_t) SCREENHEIGHT << FRACBITS*2)
                    continue; // skip if the texture is out of screen's range

            sprtopscreen = (int64_t)(t >> FRACBITS);        // [crispy] WiggleFix

            dc_iscale = 0xffffffffu / (unsigned)spryscale;
            
            // draw the texture
            col = (column_t *)( 
                (byte *)R_GetColumn(texnum,maskedtexturecol[dc_x], false) -3);
                        
            R_DrawMaskedColumn (col, -1);
            maskedtexturecol[dc_x] = INT_MAX; // [crispy] 32-bit integer math
        }
        spryscale += rw_scalestep;
    }
        
}




//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//  texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//  textures.
// CALLED: CORE LOOPING ROUTINE.
//

void R_RenderSegLoop (void)
{
    angle_t         angle;
    unsigned        index;
    int             yl;
    int             yh;
    int             mid;
    fixed_t         texturecolumn;
    int             top;
    int             bottom;
    boolean         usebrightmaps = (d_brightmaps && !fixedcolormap);

    for ( ; rw_x < rw_stopx ; rw_x++)
    {
        boolean     bottomclipped = false;

        // mark floor / ceiling areas
        yl = (int)((topfrac+heightunit-1)>>heightbits); // [crispy] WiggleFix

        // no space above wall?
        if (yl < ceilingclip[rw_x]+1)
        {
            yl = ceilingclip[rw_x]+1;
            bottomclipped = true;
        }
        
        if (markceiling)
        {
            top = ceilingclip[rw_x]+1;
            bottom = yl-1;

            if (bottom >= floorclip[rw_x])
                bottom = floorclip[rw_x]-1;

            if (top <= bottom)
            {
                ceilingplane->top[rw_x] = top;
                ceilingplane->bottom[rw_x] = bottom;
            }
        }
                
        yh = (int)(bottomfrac>>heightbits); // [crispy] WiggleFix

        if (yh >= floorclip[rw_x])
            yh = floorclip[rw_x]-1;

        if (markfloor)
        {
            top = yh+1;
            bottom = floorclip[rw_x]-1;
            if (top <= ceilingclip[rw_x])
                top = ceilingclip[rw_x]+1;
            if (top <= bottom)
            {
                floorplane->top[rw_x] = top;
                floorplane->bottom[rw_x] = bottom;
            }
        }
        
        // texturecolumn and lighting are independent of wall tiers
        if (segtextured)
        {
            // calculate texture offset
            angle = (rw_centerangle + xtoviewangle[rw_x])>>ANGLETOFINESHIFT;
            texturecolumn = rw_offset-FixedMul(finetangent[angle],rw_distance);
            texturecolumn >>= FRACBITS;
            // calculate lighting

            // CHANGED FOR HIRES
            index = rw_scale>>(LIGHTSCALESHIFT - (hires) + hires);

            if (index >=  MAXLIGHTSCALE )
                index = MAXLIGHTSCALE-1;

            dc_colormap = walllights[index];
            dc_x = rw_x;
            dc_iscale = 0xffffffffu / (unsigned)rw_scale;
        }
        else
        {
            // purely to shut up the compiler

            texturecolumn = 0;
        }
        
        // draw the wall tiers
        if (midtexture)
        {
            // single sided line
            dc_yl = yl;
            dc_yh = yh;

            // [BH] for "sparkle" hack
            dc_topsparkle = false;
            dc_bottomsparkle = (!bottomclipped && dc_yh > dc_yl
                && rw_distance < (512 << FRACBITS));

            dc_texturemid = rw_midtexturemid;
            dc_source = R_GetColumn(midtexture,texturecolumn,true);
//            dc_texheight = textureheight[midtexture]>>FRACBITS; // [crispy] Tutti-Frutti fix
            dc_texheight = midtexheight;

            // [BH] apply brightmap
            dc_colormask = midtexfullbright;

            if (dc_colormask && usebrightmaps)
                fbwallcolfunc();
            else
                wallcolfunc();

            ceilingclip[rw_x] = viewheight;
            floorclip[rw_x] = -1;
        }
        else
        {
            // two sided line
            if (toptexture)
            {
                // top wall
                mid = (int)(pixhigh>>heightbits); // [crispy] WiggleFix
                pixhigh += pixhighstep;

                if (mid >= floorclip[rw_x])
                {
                    mid = floorclip[rw_x] - 1;
                    dc_bottomsparkle = false;
                }
                else
                    dc_bottomsparkle = true;

                if (mid >= yl)
                {
                    dc_yl = yl;
                    dc_yh = mid;

                    // [BH] for "sparkle" hack
                    dc_topsparkle = false;
                    dc_bottomsparkle = (dc_bottomsparkle && dc_yh > dc_yl
                        && rw_distance < (512 << FRACBITS));

                    dc_texturemid = rw_toptexturemid;
                    dc_source = R_GetColumn(toptexture,texturecolumn,true);
//                    dc_texheight = textureheight[toptexture]>>FRACBITS; // [crispy] Tutti-Frutti fix
                    dc_texheight = toptexheight;

                    // [BH] apply brightmap
                    dc_colormask = toptexfullbright;

                    if (dc_colormask && usebrightmaps)
                        fbwallcolfunc();
                    else
                        wallcolfunc();

                    ceilingclip[rw_x] = mid;
                }
                else
                    ceilingclip[rw_x] = yl-1;
            }
            else
            {
                // no top wall
                if (markceiling)
                    ceilingclip[rw_x] = yl-1;
            }
                        
            if (bottomtexture)
            {
                // bottom wall
                mid = (int)((pixlow+heightunit-1)>>heightbits); // [crispy] WiggleFix
                pixlow += pixlowstep;

                // no space above wall?
                if (mid <= ceilingclip[rw_x])
                {
                    mid = ceilingclip[rw_x] + 1;
                    dc_topsparkle = false;
                }
                else
                    dc_topsparkle = true;
                
                if (mid <= yh)
                {
                    dc_yl = mid;
                    dc_yh = yh;

                    // [BH] for "sparkle" hack
                    dc_topsparkle = (dc_topsparkle && dc_yh > dc_yl
                        && rw_distance < (128 << FRACBITS));
                    dc_bottomsparkle = (!bottomclipped && dc_yh > dc_yl
                        && rw_distance < (512 << FRACBITS));

                    dc_texturemid = rw_bottomtexturemid;
                    dc_source = R_GetColumn(bottomtexture,
                                            texturecolumn,true);
//                    dc_texheight = textureheight[bottomtexture]>>FRACBITS; // [crispy] Tutti-Frutti fix
                    dc_texheight = bottomtexheight;

                    // [BH] apply brightmap
                    dc_colormask = bottomtexfullbright;

                    if (dc_colormask && usebrightmaps)
                        fbwallcolfunc();
                    else
                        wallcolfunc();

                    floorclip[rw_x] = mid;
                }
                else
                    floorclip[rw_x] = yh+1;
            }
            else
            {
                // no bottom wall
                if (markfloor)
                    floorclip[rw_x] = yh+1;
            }
                        
            if (maskedtexture)
            {
                // save texturecol
                //  for backdrawing of masked mid texture
                maskedtexturecol[rw_x] = texturecolumn;
            }
        }
                
        rw_scale += rw_scalestep;
        topfrac += topstep;
        bottomfrac += bottomstep;
    }
}

extern int *openings;
extern size_t maxopenings;

//
// R_AdjustOpenings
//
// killough 1/6/98, 2/1/98: remove limit on openings
// [SL] 2012-01-21 - Moved into its own function
static void R_AdjustOpenings(int start, int stop)
{
#ifdef WII
    ptrdiff_t pos = lastopening - openings;
#else
    long int pos = lastopening - openings;
#endif
    size_t need = (rw_stopx - start)*4 + pos;

    if (need > maxopenings)
    {
        drawseg_t *ds;
        int *oldopenings = openings;
        int *oldlast = lastopening;

        do
            maxopenings = maxopenings ? maxopenings*2 : 16384;
        while (need > maxopenings);
#ifdef BOOM_ZONE_HANDLING
        openings = Z_Realloc (openings, maxopenings * sizeof(*openings), PU_CACHE, NULL);
#else
        openings = Z_Realloc (openings, maxopenings * sizeof(*openings));
#endif
        lastopening = openings + pos;

        if(oldopenings != 0)
            C_Printf(CR_GOLD, " R_AdjustOpenings: Hit MaxOpenings limit at %d, raised to %u\n",
                    oldopenings, maxopenings);

        // [RH] We also need to adjust the openings pointers that
        //        were already stored in drawsegs.
        for (ds = drawsegs; ds < ds_p; ds++) {
#define ADJUST(p) if (ds->p + ds->x1 >= oldopenings && ds->p + ds->x1 <= oldlast)\
                  ds->p = ds->p - oldopenings + openings;
            ADJUST (maskedtexturecol);
            ADJUST (sprtopclip);
            ADJUST (sprbottomclip);
        }
#undef ADJUST
    }
}

// [crispy] WiggleFix: move R_ScaleFromGlobalAngle function to r_segs.c,
// above R_StoreWallRange
fixed_t R_ScaleFromGlobalAngle (angle_t visangle)
{
    int              anglea = ANG90 + (visangle - viewangle);
    int              angleb = ANG90 + (visangle - rw_normalangle);
    int              den = FixedMul(rw_distance, finesine[anglea >> ANGLETOFINESHIFT]);
    fixed_t          num = FixedMul(projection, finesine[angleb >> ANGLETOFINESHIFT]);
    fixed_t          scale;

    if (den > (num >> 16))
    {
        scale = FixedDiv(num, den);

        // [kb] When this evaluates True, the scale is clamped,
        //  and there will be some wiggling.
        if (scale > max_rwscale)
            scale = max_rwscale;
        else if (scale < 256)
            scale = 256;
    }
    else
        scale = max_rwscale;

    return scale;
}


//
// R_StoreWallRange
// A wall segment will be drawn
//  between start and stop pixels (inclusive).
//
void
R_StoreWallRange
( int        start,
  int        stop )
{
    fixed_t         h;
    fixed_t         hyp;
    fixed_t         sineval;
    angle_t         distangle, offsetangle;
    int             lightnum;
    int             liquidoffset = 0;

    // [crispy] remove MAXDRAWSEGS Vanilla limit
    if (ds_p == &drawsegs[numdrawsegs])
    {
        int numdrawsegs_old = numdrawsegs;

        numdrawsegs = numdrawsegs ? 2 * numdrawsegs : MAXDRAWSEGS;
#ifdef BOOM_ZONE_HANDLING
        drawsegs = Z_Realloc(drawsegs, numdrawsegs * sizeof(*drawsegs), PU_LEVEL, NULL);
#else
        drawsegs = Z_Realloc(drawsegs, numdrawsegs * sizeof(*drawsegs));
#endif
        memset(drawsegs + numdrawsegs_old, 0,
              (numdrawsegs - numdrawsegs_old) * sizeof(*drawsegs));

        ds_p = drawsegs + numdrawsegs_old;

        if (numdrawsegs_old)
            C_Printf(CR_GOLD, " R_StoreWallRange: Hit MAXDRAWSEGS limit at %d, \
                      raised to %d.\n", numdrawsegs_old, numdrawsegs);
    }

#ifdef RANGECHECK
    if (start >=viewwidth || start > stop)
        I_Error ("Bad R_RenderWallRange: %i to %i", start , stop);
#endif
    
    sidedef = curline->sidedef;
    linedef = curline->linedef;

    // mark the segment as visible for auto map
    linedef->flags |= ML_MAPPED;
    
    // [crispy] (flags & ML_MAPPED) is all we need to know for automap
    if (automapactive && !am_overlay)
        return;

    // calculate rw_distance for scale calculation
    rw_normalangle = curline->angle + ANG90; // [crispy] use re-calculated angle
    offsetangle = abs(rw_normalangle-rw_angle1);
    
    if (offsetangle > ANG90)
        offsetangle = ANG90;

    distangle = ANG90 - offsetangle;
    hyp = R_PointToDist (curline->v1->x, curline->v1->y);
    sineval = finesine[distangle>>ANGLETOFINESHIFT];
    rw_distance = FixedMul (hyp, sineval);
                
        
    ds_p->x1 = rw_x = start;
    ds_p->x2 = stop;
    ds_p->curline = curline;
    rw_stopx = stop+1;
    
    // killough: remove limits on openings
    R_AdjustOpenings(start, stop);

    // [crispy] WiggleFix: add this line, in r_segs.c:R_StoreWallRange,
    // right before calls to R_ScaleFromGlobalAngle:
    R_FixWiggle(frontsector);

    // calculate scale at both ends and step
    ds_p->scale1 = rw_scale = 
        R_ScaleFromGlobalAngle (viewangle + xtoviewangle[start]);
    
    if (stop > start )
    {
        ds_p->scale2 = R_ScaleFromGlobalAngle (viewangle + xtoviewangle[stop]);
        ds_p->scalestep = rw_scalestep = 
            (ds_p->scale2 - rw_scale) / (stop-start);
    }
    else
    {
        // UNUSED: try to fix the stretched line bug
#if 0
        if (rw_distance < FRACUNIT/2)
        {
            fixed_t                trx,try;
            fixed_t                gxt,gyt;

            trx = curline->v1->x - viewx;
            try = curline->v1->y - viewy;
                        
            gxt = FixedMul(trx,viewcos); 
            gyt = -FixedMul(try,viewsin); 
            ds_p->scale1 = FixedDiv(projection, gxt-gyt)<<detailshift;
        }
#endif
        ds_p->scale2 = ds_p->scale1;
    }
    
    // calculate texture boundaries
    //  and decide if floor / ceiling marks are needed
    worldtop = frontsector->interpceilingheight - viewz;
    worldbottom = frontsector->interpfloorheight - viewz;
        
    // [BH] animate liquid sectors
#ifdef ANIMATED_FLOOR_LIQUIDS
    if (frontsector->animate && (frontsector->heightsec == -1
        || viewz > sectors[frontsector->heightsec].interpfloorheight))
        worldbottom += frontsector->animate;
#endif
    midtexture = toptexture = bottomtexture = maskedtexture = 0;
    ds_p->maskedtexturecol = NULL;

    if (!backsector)
    {
        // single sided line
        midtexture = texturetranslation[sidedef->midtexture];
        midtexheight = textureheight[midtexture] >> FRACBITS;
        midtexfullbright = texturefullbright[midtexture];

        // a single sided line is terminal, so it must mark ends
        markfloor = markceiling = true;

        if (linedef->flags & ML_DONTPEGBOTTOM)
            // bottom of texture at bottom
            rw_midtexturemid = frontsector->interpfloorheight + textureheight[sidedef->midtexture]
                - viewz + sidedef->rowoffset;
        else
            // top of texture at top
            rw_midtexturemid = worldtop + sidedef->rowoffset;

        // killough 3/27/98: reduce offset
        h = textureheight[midtexture];

        if (h & (h - FRACUNIT))
            rw_midtexturemid %= h;

        ds_p->silhouette = SIL_BOTH;
        ds_p->sprtopclip = screenheightarray;
        ds_p->sprbottomclip = negonearray;
        ds_p->bsilheight = INT_MAX;
        ds_p->tsilheight = INT_MIN;
    }
    else
    {
        // two sided line
        ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
        ds_p->silhouette = 0;
        
        if (frontsector->interpfloorheight > backsector->interpfloorheight)
        {
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->bsilheight = frontsector->interpfloorheight;
        }
        else if (backsector->interpfloorheight > viewz)
        {
            ds_p->silhouette = SIL_BOTTOM;
            ds_p->bsilheight = INT_MAX;
            // ds_p->sprbottomclip = negonearray;
        }
        
        if (frontsector->interpceilingheight < backsector->interpceilingheight)
        {
            ds_p->silhouette |= SIL_TOP;
            ds_p->tsilheight = frontsector->interpceilingheight;
        }
        else if (backsector->interpceilingheight < viewz)
        {
            ds_p->silhouette |= SIL_TOP;
            ds_p->tsilheight = INT_MIN;
            // ds_p->sprtopclip = screenheightarray;
        }
                
        if (backsector->interpceilingheight <= frontsector->interpfloorheight)
        {
            ds_p->sprbottomclip = negonearray;
            ds_p->bsilheight = INT_MAX;
            ds_p->silhouette |= SIL_BOTTOM;
        }
        
        if (backsector->interpfloorheight >= frontsector->interpceilingheight)
        {
            ds_p->sprtopclip = screenheightarray;
            ds_p->tsilheight = INT_MIN;
            ds_p->silhouette |= SIL_TOP;
        }
        
        worldhigh = backsector->interpceilingheight - viewz;
        worldlow = backsector->interpfloorheight - viewz;
                
        // [BH] animate liquid sectors
#ifdef ANIMATED_FLOOR_LIQUIDS
        if (backsector->animate
            && backsector->interpfloorheight > frontsector->interpfloorheight
            && (backsector->heightsec == -1
            || viewz > sectors[backsector->heightsec].interpfloorheight))
        {
            liquidoffset = backsector->animate;
            worldlow += liquidoffset;
        }
#endif
        // hack to allow height changes in outdoor areas
        if (frontsector->ceilingpic == skyflatnum 
            && backsector->ceilingpic == skyflatnum)
        {
            worldtop = worldhigh;
        }
        
        markfloor = (worldlow != worldbottom
            || backsector->floorpic != frontsector->floorpic
            || backsector->lightlevel != frontsector->lightlevel

            // killough 4/15/98: prevent 2s normals
            // from bleeding through deep water
            || frontsector->heightsec != -1);

        markceiling = (worldhigh != worldtop
            || backsector->ceilingpic != frontsector->ceilingpic
            || backsector->lightlevel != frontsector->lightlevel

            // killough 4/15/98: prevent 2s normals
            // from bleeding through fake ceilings
            || (frontsector->heightsec != -1 && frontsector->ceilingpic != skyflatnum));

        if (backsector->interpceilingheight <= frontsector->interpfloorheight
            || backsector->interpfloorheight >= frontsector->interpceilingheight)
        {
            // closed door
            markceiling = markfloor = true;
        }
        

        if (worldhigh < worldtop)
        {
            // top texture
            toptexture = texturetranslation[sidedef->toptexture];
            toptexheight = textureheight[toptexture] >> FRACBITS;
            toptexfullbright = texturefullbright[toptexture];

            if (linedef->flags & ML_DONTPEGTOP)
                // top of texture at top
                rw_toptexturemid = worldtop;
            else
                // bottom of texture
                rw_toptexturemid = backsector->interpceilingheight + toptexheight - viewz;

            rw_toptexturemid += sidedef->rowoffset;

            // killough 3/27/98: reduce offset
            h = textureheight[toptexture];

            if (h & (h - FRACUNIT))
                rw_toptexturemid %= h;
        }

        if (worldlow > worldbottom)
        {
            // bottom texture
            bottomtexture = texturetranslation[sidedef->bottomtexture];
            bottomtexheight = textureheight[bottomtexture] >> FRACBITS;
            bottomtexfullbright = texturefullbright[bottomtexture];

            if (linedef->flags & ML_DONTPEGBOTTOM)
                // bottom of texture at bottom, top of texture at top
                rw_bottomtexturemid = worldtop;
            else        // top of texture at top
                rw_bottomtexturemid = worldlow - liquidoffset;

            rw_bottomtexturemid += sidedef->rowoffset;

            // killough 3/27/98: reduce offset
            h = textureheight[bottomtexture];

            if (h & (h - FRACUNIT))
                rw_bottomtexturemid %= h;
        }

        // allocate space for masked texture tables
        if (sidedef->midtexture)
        {
            // masked midtexture
            maskedtexture = true;
            ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
            lastopening += rw_stopx - rw_x;
        }
    }
    
    // calculate rw_offset (only needed for textured lines)
    segtextured = midtexture | toptexture | bottomtexture | maskedtexture;

    if (segtextured)
    {
        offsetangle = rw_normalangle-rw_angle1;
        
        if (offsetangle > ANG180)
            offsetangle = -offsetangle;

        if (offsetangle > ANG90)
            offsetangle = ANG90;

        sineval = finesine[offsetangle >>ANGLETOFINESHIFT];
        rw_offset = FixedMul (hyp, sineval);

        if (rw_normalangle-rw_angle1 < ANG180)
            rw_offset = -rw_offset;

        rw_offset += sidedef->textureoffset + curline->offset;
        rw_centerangle = ANG90 + viewangle - rw_normalangle;
        
        // calculate light table
        //  use different light tables
        //  for horizontal / vertical / diagonal
        // OPTIMIZE: get rid of LIGHTSEGSHIFT globally
        if (!fixedcolormap)
        {
            lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)+extralight*LIGHTBRIGHT;

            if (curline->v1->y == curline->v2->y)
                lightnum-=LIGHTBRIGHT;
            else if (curline->v1->x == curline->v2->x)
                lightnum+=LIGHTBRIGHT;

            if (lightnum < 0)                
                walllights = scalelight[0];
            else if (lightnum >= LIGHTLEVELS)
                walllights = scalelight[LIGHTLEVELS-1];
            else
                walllights = scalelight[lightnum];
        }
    }
    
    // if a floor / ceiling plane is on the wrong side
    //  of the view plane, it is definitely invisible
    //  and doesn't need to be marked.
    // killough 3/7/98: add deep water check
    if (frontsector->heightsec == -1)
    {
        if (frontsector->interpfloorheight >= viewz)
            markfloor = false;          // above view plane

        if (frontsector->interpceilingheight <= viewz && frontsector->ceilingpic != skyflatnum)
            markceiling = false;        // below view plane
    }
    
    // calculate incremental stepping values for texture edges
    worldtop >>= invhgtbits;
    worldbottom >>= invhgtbits;
        
    topstep = -FixedMul (rw_scalestep, worldtop);
    topfrac = ((int64_t)centeryfrac>>invhgtbits) -
              (((int64_t)worldtop * rw_scale)>>FRACBITS); // [crispy] WiggleFix

    bottomstep = -FixedMul (rw_scalestep,worldbottom);
    bottomfrac = ((int64_t)centeryfrac>>invhgtbits) -
                 (((int64_t)worldbottom * rw_scale)>>FRACBITS); // [crispy] WiggleFix
        
    if (backsector)
    {        
        worldhigh >>= invhgtbits;
        worldlow >>= invhgtbits;

        if (worldhigh < worldtop)
        {
            pixhigh = ((int64_t)centeryfrac>>invhgtbits) -
                      (((int64_t)worldhigh * rw_scale)>>FRACBITS); // [crispy] WiggleFix
            pixhighstep = -FixedMul (rw_scalestep,worldhigh);
        }
        
        if (worldlow > worldbottom)
        {
            pixlow = ((int64_t)centeryfrac>>invhgtbits) -
                     (((int64_t)worldlow * rw_scale)>>FRACBITS); // [crispy] WiggleFix
            pixlowstep = -FixedMul (rw_scalestep,worldlow);
        }
    }
    
    // render it
    if (markceiling)
        ceilingplane = R_CheckPlane (ceilingplane, rw_x, rw_stopx-1);
    
    if (markfloor)
        floorplane = R_CheckPlane (floorplane, rw_x, rw_stopx-1);

    R_RenderSegLoop ();

    
    // save sprite clipping info
    if ( ((ds_p->silhouette & SIL_TOP) || maskedtexture)
         && !ds_p->sprtopclip)
    {
        memcpy (lastopening, ceilingclip+start,
                sizeof(*lastopening)*(rw_stopx-start)); // [crispy] 32-bit integer math
        ds_p->sprtopclip = lastopening - start;
        lastopening += rw_stopx - start;
    }
    
    if ( ((ds_p->silhouette & SIL_BOTTOM) || maskedtexture)
         && !ds_p->sprbottomclip)
    {
        memcpy (lastopening, floorclip+start,
                sizeof(*lastopening)*(rw_stopx-start)); // [crispy] 32-bit integer math
        ds_p->sprbottomclip = lastopening - start;
        lastopening += rw_stopx - start;        
    }

    if (maskedtexture && !(ds_p->silhouette&SIL_TOP))
    {
        ds_p->silhouette |= SIL_TOP;
        ds_p->tsilheight = INT_MIN;
    }
    if (maskedtexture && !(ds_p->silhouette&SIL_BOTTOM))
    {
        ds_p->silhouette |= SIL_BOTTOM;
        ds_p->bsilheight = INT_MAX;
    }
    ds_p++;
}
