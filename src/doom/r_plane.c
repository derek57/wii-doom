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
//        Here is a core component: drawing the floors and ceilings,
//        while maintaining a per column clipping list only.
//        Moreover, the sky areas have to be determined.
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

#include "r_defs.h"
#include "p_spec.h"
#include "r_local.h"
#include "r_sky.h"

#ifdef WII
#include "../v_trans.h"
#include "../w_wad.h"
#include "../z_zone.h"
#else
#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"
#endif


// Here comes the obnoxious "visplane".
#define MAXVISPLANES       128*8             // CHANGED FOR HIRES

// ?
#define MAXOPENINGS        SCREENWIDTH*64*4  // CHANGED FOR HIRES


planefunction_t            floorfunc;
planefunction_t            ceilingfunc;

//
// opening
//

visplane_t*                visplanes = NULL;   // CHANGED FOR HIRES
visplane_t*                lastvisplane;
visplane_t*                floorplane;
visplane_t*                ceilingplane;

// texture mapping
lighttable_t**             planezlight;

fixed_t                    planeheight;
fixed_t                    yslope[SCREENHEIGHT];
fixed_t                    distscale[SCREENWIDTH];
fixed_t                    basexscale;
fixed_t                    baseyscale;
fixed_t                    cachedheight[SCREENHEIGHT];
fixed_t                    cacheddistance[SCREENHEIGHT];
fixed_t                    cachedxstep[SCREENHEIGHT];
fixed_t                    cachedystep[SCREENHEIGHT];

static fixed_t             xoffs, yoffs;                   // killough 2/28/98: flat offsets

static int                 numvisplanes;             // ADDED FOR HIRES

size_t                     maxopenings;

int                        *openings;                // dropoff overflow
int*                       lastopening;              // [crispy] 32-bit integer math

// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1
int                        floorclip[SCREENWIDTH];   // [crispy] 32-bit integer math
int                        ceilingclip[SCREENWIDTH]; // [crispy] 32-bit integer math

// spanstart holds the start of a plane span
// initialized to 0 at start
int                        spanstart[SCREENHEIGHT];
int                        spanstop[SCREENHEIGHT];

extern int                 mouselook;

extern fixed_t             animatedliquiddiff;


//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes (void)
{
  // Doh!
}


//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  basexscale
//  baseyscale
//  viewx
//  viewy
//
// BASIC PRIMITIVE
//
void
R_MapPlane
( int                y,
  int                x1,
  int                x2 )
{
    fixed_t     distance;
    int         dx, dy;

    if (y == centery)
        return;

#ifdef RANGECHECK
    if (x2 < x1
     || x1 < 0
     || x2 >= viewwidth
     || y > viewheight)
    {
        I_Error ("R_MapPlane: %i, %i at %i",x1,x2,y);
    }
#endif

    distance = FixedMul(planeheight, yslope[y]);

    dx = x1 - centerx;
    dy = abs(centery - y);
    ds_xstep = FixedMul(viewsin, planeheight) / dy;
    ds_ystep = FixedMul(viewcos, planeheight) / dy;

    ds_xfrac = viewx + xoffs + FixedMul(viewcos, distance) + dx * ds_xstep;
    ds_yfrac = -viewy + yoffs - FixedMul(viewsin, distance) + dx * ds_ystep;
/*
    ds_xfrac = viewx + FixedMul(finecosine[angle], length);
    ds_yfrac = -viewy - FixedMul(finesine[angle], length);
*/
    ds_colormap = (fixedcolormap ? fixedcolormap :
        planezlight[BETWEEN(0, distance >> LIGHTZSHIFT, MAXLIGHTZ - 1)]);
        
    ds_y = y;
    ds_x1 = x1;
    ds_x2 = x2;

    // high or low detail
    spanfunc ();        
}


//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes (void)
{
    int            i;
    angle_t        angle;
    
    // opening / clipping determination
    for (i=0 ; i<viewwidth ; i++)
    {
        floorclip[i] = viewheight;
        ceilingclip[i] = -1;
    }

    lastvisplane = visplanes;
    lastopening = openings;
    
    // texture calculation
    memset (cachedheight, 0, sizeof(cachedheight));

    // left to right mapping
    angle = (viewangle-ANG90)>>ANGLETOFINESHIFT;
        
    // scale will be unit scale at SCREENWIDTH/2 distance
    basexscale = FixedDiv (finecosine[angle],centerxfrac);
    baseyscale = -FixedDiv (finesine[angle],centerxfrac);
}


// [crispy] remove MAXVISPLANES Vanilla limit
static void R_RaiseVisplanes (visplane_t** vp)
{
    if (lastvisplane - visplanes == numvisplanes)
    {
        int numvisplanes_old = numvisplanes;
        visplane_t* visplanes_old = visplanes;

        numvisplanes = numvisplanes ? 2 * numvisplanes : MAXVISPLANES;
#ifdef BOOM_ZONE_HANDLING
        visplanes = Z_Realloc(visplanes, numvisplanes * sizeof(*visplanes), PU_CACHE, NULL);
#else
        visplanes = Z_Realloc(visplanes, numvisplanes * sizeof(*visplanes));
#endif
        memset(visplanes + numvisplanes_old, 0, (numvisplanes - numvisplanes_old) * sizeof(*visplanes));

        lastvisplane = visplanes + numvisplanes_old;
        floorplane = visplanes + (floorplane - visplanes_old);
        ceilingplane = visplanes + (ceilingplane - visplanes_old);

        if (numvisplanes_old)
            C_Printf(CR_GOLD, " R_FindPlane: Hit MAXVISPLANES limit at %d, raised to %d.\n", numvisplanes_old, numvisplanes);

        // keep the pointer passed as argument in relation to the visplanes pointer
        if (vp)
            *vp = visplanes + (*vp - visplanes_old);
    }
}


//
// R_FindPlane
//
visplane_t*
R_FindPlane
( fixed_t        height,
  int            picnum,
  int            lightlevel,
  fixed_t        xoffs,
  fixed_t        yoffs )
{
    visplane_t*  check;
        
    if (picnum == skyflatnum || (picnum & PL_SKYFLAT))          // killough 10/98
        height = lightlevel = 0;                // killough 7/19/98: most skies map together
        
    for (check=visplanes; check<lastvisplane; check++)
    {
        if (height == check->height
            && picnum == check->picnum
            && lightlevel == check->lightlevel
            && xoffs == check->xoffs
            && yoffs == check->yoffs)
        {
            break;
        }
    }
    
                        
    if (check < lastvisplane)
        return check;

    R_RaiseVisplanes(&check);

    lastvisplane++;

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = SCREENWIDTH;
    check->maxx = -1;
    check->xoffs = xoffs;                                      // killough 2/28/98: Save offsets
    check->yoffs = yoffs;
    
    memset (check->top,0xff,sizeof(check->top));
                
    return check;
}


//
// R_CheckPlane
//
visplane_t*
R_CheckPlane
( visplane_t*        pl,
  int                start,
  int                stop )
{
    int              intrl;
    int              intrh;
    int              unionl;
    int              unionh;
    int              x;
        
    if (start < pl->minx)
    {
        intrl = pl->minx;
        unionl = start;
    }
    else
    {
        unionl = pl->minx;
        intrl = start;
    }
        
    if (stop > pl->maxx)
    {
        intrh = pl->maxx;
        unionh = stop;
    }
    else
    {
        unionh = pl->maxx;
        intrh = stop;
    }

    for (x=intrl ; x<= intrh ; x++)
        if (pl->top[x] != 0xffffffffu) // [crispy] hires / 32-bit integer math
            break;

    if (x > intrh)
    {
        pl->minx = unionl;
        pl->maxx = unionh;

        // use the same one
        return pl;                
    }
        
    // make a new visplane

    R_RaiseVisplanes(&pl);                                        // ADDED FOR HIRES

    lastvisplane->height = pl->height;
    lastvisplane->picnum = pl->picnum;
    lastvisplane->lightlevel = pl->lightlevel;
    lastvisplane->xoffs = pl->xoffs;      // killough 2/28/98
    lastvisplane->yoffs = pl->yoffs;
    
    pl = lastvisplane++;
    pl->minx = start;
    pl->maxx = stop;

    memset (pl->top,0xff,sizeof(pl->top));
                
    return pl;
}


//
// R_MakeSpans
//
void
R_MakeSpans
( int                x,
  unsigned int       t1,  // [crispy] hires / 32-bit integer math
  unsigned int       b1,  // [crispy] hires / 32-bit integer math
  unsigned int       t2,  // [crispy] hires / 32-bit integer math
  unsigned int       b2 ) // [crispy] hires / 32-bit integer math
{
    while (t1 < t2 && t1<=b1)
    {
        R_MapPlane (t1,spanstart[t1],x-1);
        t1++;
    }
    while (b1 > b2 && b1>=t1)
    {
        R_MapPlane (b1,spanstart[b1],x-1);
        b1--;
    }
        
    while (t2 < t1 && t2<=b2)
    {
        spanstart[t2] = x;
        t2++;
    }
    while (b2 > b1 && b2>=t2)
    {
        spanstart[b2] = x;
        b2--;
    }
}



#define AMP          2
#define AMP2         2
#define SPEED        40

// swirl factors determine the number of waves per flat width
// 1 cycle per 64 units

#define SWIRLFACTOR  (8192/64)

// 1 cycle per 32 units (2 in 64)
#define SWIRLFACTOR2 (8192/32)

static char *normalflat;
static char distortedflat[4096];

char *R_DistortedFlat(int flatnum)
{
    static int lastflat = -1;
    static int swirltic = -1;
    static int offset[4096];
    int        i;
    int        leveltic = gametic;

    // Already swirled this one?
    if (gametic == swirltic && lastflat == flatnum)
        return distortedflat;

    lastflat = flatnum;
  
    // built this tic?

    if(gametic != swirltic && !consoleactive && !paused)
    {
        int x, y;
      
        for (x = 0; x < 64; ++x)
            for (y = 0; y < 64; ++y)
            {
                int x1, y1;
                int sinvalue, sinvalue2;

                sinvalue  = (y * SWIRLFACTOR  + leveltic * SPEED * 5 + 900) & 8191;
                sinvalue2 = (x * SWIRLFACTOR2 + leveltic * SPEED * 4 + 300) & 8191;
                x1 = x + 128 + ((finesine[sinvalue]  * AMP)  >> FRACBITS) +
                               ((finesine[sinvalue2] * AMP2) >> FRACBITS);

                sinvalue  = (x * SWIRLFACTOR  + leveltic * SPEED * 3 + 700) & 8191;
                sinvalue2 = (y * SWIRLFACTOR2 + leveltic * SPEED * 4 + 1200) & 8191;
                y1 = y + 128 + ((finesine[sinvalue]  * AMP)  >> FRACBITS) +
                               ((finesine[sinvalue2] * AMP2) >> FRACBITS);

                x1 &= 63;
                y1 &= 63;

                offset[(y << 6) + x] = (y1 << 6) + x1;
            }
        swirltic = gametic;
    }

    normalflat = W_CacheLumpNum(firstflat + flatnum, PU_STATIC);

    for(i = 0; i < 4096; i++)
        distortedflat[i] = normalflat[offset[i]];

    return distortedflat;
}

//
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanes (void)
{
    visplane_t*         pl;
    int                 x;
    int                 stop;
    int                 angle;
                                
#ifdef RANGECHECK

    if (ds_p - drawsegs > numdrawsegs)                          // CHANGED FOR HIRES
        I_Error ("R_DrawPlanes: drawsegs overflow (%i)",        // CHANGED FOR HIRES
                 ds_p - drawsegs);                              // CHANGED FOR HIRES
    
    if (lastvisplane - visplanes > numvisplanes)                // CHANGED FOR HIRES
        I_Error ("R_DrawPlanes: visplane overflow (%i)",        // CHANGED FOR HIRES
                 lastvisplane - visplanes);                     // CHANGED FOR HIRES

    if (lastopening - openings > MAXOPENINGS)
        I_Error ("R_DrawPlanes: opening overflow (%i)",
                 lastopening - openings);
#endif

    for (pl = visplanes ; pl < lastvisplane ; pl++)
    {
        if (pl->minx > pl->maxx)
            continue;

        
        // sky flat
        if (pl->picnum == skyflatnum)
        {
            dc_iscale = pspriteiscale>>(detailshift && !hires);                // CHANGED FOR HIRES
	    // [crispy] stretch sky
            if (mouselook > 0)
                dc_iscale = dc_iscale * 128 / 228;
            
            // Sky is allways drawn full bright,
            //  i.e. colormaps[0] is used.
            // Because of this hack, sky is not affected
            //  by INVUL inverse mapping.
            dc_colormap = (fixedcolormap ? fixedcolormap : fullcolormap);
            dc_texturemid = skytexturemid;
            dc_texheight = textureheight[skytexture]>>FRACBITS; // [crispy] Tutti-Frutti fix
            for (x=pl->minx ; x <= pl->maxx ; x++)
            {
                dc_yl = pl->top[x];
                dc_yh = pl->bottom[x];

                if ((unsigned) dc_yl <= dc_yh) // [crispy] 32-bit integer math
                {
                    angle = (viewangle + xtoviewangle[x])>>ANGLETOSKYSHIFT;
                    dc_x = x;
                    dc_source = R_GetColumn(skytexture, angle, false);
                    colfunc ();
                }
            }
            continue;
        }
        else      // regular flat
        {
            int         picnum = pl->picnum;
            boolean     liquid = isliquid[picnum];
            boolean     swirling = (liquid && d_swirl);
            int         lumpnum = firstflat + flattranslation[picnum];

            ds_source = (swirling ?
                    R_DistortedFlat(picnum): W_CacheLumpNum(lumpnum,
                            PU_STATIC));

            xoffs = pl->xoffs;  // killough 2/28/98: Add offsets
            yoffs = pl->yoffs;

            planeheight = abs(pl->height-viewz);
#ifdef ANIMATED_FLOOR_LIQUIDS
            if (liquid && pl->sector && d_swirl && isliquid[pl->sector->floorpic])
                planeheight -= animatedliquiddiff;
#endif
            planezlight = zlight[BETWEEN(0, (pl->lightlevel >> LIGHTSEGSHIFT)
                    + extralight * LIGHTBRIGHT, LIGHTLEVELS - 1)];

            pl->top[pl->maxx+1] = 0xffffffffu; // [crispy] hires / 32-bit integer math
            pl->top[pl->minx-1] = 0xffffffffu; // [crispy] hires / 32-bit integer math

            stop = pl->maxx + 1;

            for (x=pl->minx ; x<= stop ; x++)
            {
                R_MakeSpans(x,pl->top[x-1],
                            pl->bottom[x-1],
                            pl->top[x],
                            pl->bottom[x]);
            }

            if(!swirling)
                W_ReleaseLumpNum(lumpnum);
        }
    }
}

