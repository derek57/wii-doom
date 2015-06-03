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
//        Here is a core component: drawing the floors and ceilings,
//        while maintaining a per column clipping list only.
//        Moreover, the sky areas have to be determined.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "c_io.h"
#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "r_local.h"
#include "r_sky.h"
#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"


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

size_t                     maxopenings;

static int                 numvisplanes;             // ADDED FOR HIRES

int                        *openings;                // dropoff overflow
int*                       lastopening;              // CHANGED FOR HIRES

// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1
int                        floorclip[SCREENWIDTH];   // CHANGED FOR HIRES
int                        ceilingclip[SCREENWIDTH]; // CHANGED FOR HIRES

// spanstart holds the start of a plane span
// initialized to 0 at start
int                        spanstart[SCREENHEIGHT];
int                        spanstop[SCREENHEIGHT];

extern int                 mouselook;

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
    angle_t          angle;
    fixed_t          distance;
    fixed_t          length;
    unsigned         index;
        
#ifdef RANGECHECK
    if (x2 < x1
     || x1 < 0
     || x2 >= viewwidth
     || y > viewheight)
    {
        I_Error ("R_MapPlane: %i, %i at %i",x1,x2,y);
    }
#endif

    if (planeheight != cachedheight[y])
    {
        cachedheight[y] = planeheight;
        distance = cacheddistance[y] = FixedMul (planeheight, yslope[y]);
        ds_xstep = cachedxstep[y] = FixedMul (distance,basexscale);
        ds_ystep = cachedystep[y] = FixedMul (distance,baseyscale);
    }
    else
    {
        distance = cacheddistance[y];
        ds_xstep = cachedxstep[y];
        ds_ystep = cachedystep[y];
    }
        
    length = FixedMul (distance,distscale[x1]);
    angle = (viewangle + xtoviewangle[x1])>>ANGLETOFINESHIFT;
    ds_xfrac = viewx + FixedMul(finecosine[angle], length);
    ds_yfrac = -viewy - FixedMul(finesine[angle], length);

    if (fixedcolormap)
        ds_colormap = fixedcolormap;
    else
    {
        index = distance >> LIGHTZSHIFT;
        
        if (index >= MAXLIGHTZ )
            index = MAXLIGHTZ-1;

        ds_colormap = planezlight[index];
    }
        
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


static void R_RaiseVisplanes (visplane_t** vp)
{
    if (lastvisplane - visplanes == numvisplanes)
    {
        int numvisplanes_old = numvisplanes;
        visplane_t* visplanes_old = visplanes;

        numvisplanes = numvisplanes ? 2 * numvisplanes : MAXVISPLANES;
        visplanes = realloc(visplanes, numvisplanes * sizeof(*visplanes));
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
  int            lightlevel )
{
    visplane_t*  check;
        
    if (picnum == skyflatnum)
    {
        height = 0;                        // all skys map together
        lightlevel = 0;
    }
        
    for (check=visplanes; check<lastvisplane; check++)
    {
        if (height == check->height
            && picnum == check->picnum
            && lightlevel == check->lightlevel)
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
        if (pl->top[x] != 0xffffffffu)                                // CHANGED FOR HIRES
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
  unsigned int       t1,                                                // CHANGED FOR HIRES
  unsigned int       b1,                                                // CHANGED FOR HIRES
  unsigned int       t2,                                                // CHANGED FOR HIRES
  unsigned int       b2 )                                                // CHANGED FOR HIRES
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



#define AMP 2
#define AMP2 2
#define SPEED 40

// swirl factors determine the number of waves per flat width
// 1 cycle per 64 units

#define swirlfactor (8192/64)

// 1 cycle per 32 units (2 in 64)
#define swirlfactor2 (8192/32)

char *normalflat;
char distortedflat[4096];

char *R_DistortedFlat(int flatnum)
{
    static int swirltic = -1;
    static int offset[4096];
    int i;
    int leveltic = I_GetTime();
  
    // built this tic?

    if(gametic != swirltic)
    {
        int x, y;
      
        for(x=0; x<64; x++)
        {
            for(y=0; y<64; y++)
            {
                int x1, y1;
                int sinvalue, sinvalue2;

                sinvalue = (y * swirlfactor + leveltic*SPEED*5 + 900) & 8191;
                sinvalue2 = (x * swirlfactor2 + leveltic*SPEED*4 + 300) & 8191;
                x1 = x + 128 + ((finesine[sinvalue]*AMP) >> FRACBITS) +
                        ((finesine[sinvalue2]*AMP2) >> FRACBITS);

                sinvalue = (x * swirlfactor + leveltic*SPEED*3 + 700) & 8191;
                sinvalue2 = (y * swirlfactor2 + leveltic*SPEED*4 + 1200) & 8191;
                y1 = y + 128 + ((finesine[sinvalue]*AMP) >> FRACBITS) +
                        ((finesine[sinvalue2]*AMP2) >> FRACBITS);

                x1 &= 63; y1 &= 63;

                offset[(y<<6) + x] = (y1<<6) + x1;
            }
        }
        swirltic = gametic;
    }

    normalflat = W_CacheLumpNum(firstflat + flatnum, PU_STATIC);

    for(i=0; i<4096; i++)
        distortedflat[i] = normalflat[offset[i]];

    // free the original
    Z_ChangeTag(normalflat, PU_CACHE);

    return distortedflat;
}

//
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanes (void)
{
    visplane_t*         pl;
    int                 light;
    int                 x;
    int                 stop;
    int                 angle;
    int                 lumpnum;
                                
#ifdef RANGECHECK

    if (ds_p - drawsegs > numdrawsegs)                                // CHANGED FOR HIRES
        I_Error ("R_DrawPlanes: drawsegs overflow (%i)",        // CHANGED FOR HIRES
                 ds_p - drawsegs);                                // CHANGED FOR HIRES
    
    if (lastvisplane - visplanes > numvisplanes)                // CHANGED FOR HIRES
        I_Error ("R_DrawPlanes: visplane overflow (%i)",        // CHANGED FOR HIRES
                 lastvisplane - visplanes);                        // CHANGED FOR HIRES

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

             if (mouselook > 0)
                dc_iscale = dc_iscale * 128 / 228;
            
            // Sky is allways drawn full bright,
            //  i.e. colormaps[0] is used.
            // Because of this hack, sky is not affected
            //  by INVUL inverse mapping.
            dc_colormap = colormaps;
            dc_texturemid = skytexturemid;
            dc_texheight = textureheight[skytexture]>>FRACBITS; // Tutti-Frutti fix
            for (x=pl->minx ; x <= pl->maxx ; x++)
            {
                dc_yl = pl->top[x];
                dc_yh = pl->bottom[x];

                if ((unsigned) dc_yl <= dc_yh)                                // CHANGED FOR HIRES
                {
                    angle = (viewangle + xtoviewangle[x])>>ANGLETOSKYSHIFT;
                    dc_x = x;
                    dc_source = R_GetColumn(skytexture, angle);
                    colfunc ();
                }
            }
            continue;
        }
        else      // regular flat
        {
            int swirling = 0;
      
            lumpnum = firstflat + flattranslation[pl->picnum];
            swirling = flattranslation[pl->picnum] == -1;
            ds_source =  swirling ?
                    R_DistortedFlat(pl->picnum): W_CacheLumpNum(lumpnum,
                            PU_STATIC);

            planeheight = abs(pl->height-viewz);
            light = (pl->lightlevel >> LIGHTSEGSHIFT)+extralight;

            if (light >= LIGHTLEVELS)
                light = LIGHTLEVELS-1;

            if (light < 0)
                light = 0;

            planezlight = zlight[light];

            pl->top[pl->maxx+1] = 0xffffffffu;                        // CHANGED FOR HIRES
            pl->top[pl->minx-1] = 0xffffffffu;                        // CHANGED FOR HIRES

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

