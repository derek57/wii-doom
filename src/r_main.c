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
//        Rendering main loop and setup functions,
//        utility functions (BSP, geometry, trigonometry).
//        See tables.c, too.
//
//-----------------------------------------------------------------------------



#include <math.h>
#include <stdlib.h>


#include "c_io.h"
#include "d_loop.h"
#include "d_player.h"
#include "doomdef.h"
#include "doomstat.h"
#include "m_bbox.h"
#include "m_menu.h"
#include "r_local.h"
#include "r_sky.h"
#include "v_trans.h"
#include "v_video.h"


// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW        2048        
#define DISTMAP            2


int                        setblocks;
int                        setdetail;
int                        viewangleoffset;
int                        centerx;
int                        centery;
int                        sscount;
int                        linecount;
int                        loopcount;

// increment every time a check is made
int                        validcount = 1;                

// just for profiling purposes
int                        framecount;        

// 0 = high, 1 = low
int                        detailshift;        

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X. 
int                        viewangletox[FINEANGLES/2];

// bumped light from gun blasts
int                        extralight;                        
int                        viewpitch;

boolean                    BorderNeedRefresh;
boolean                    setsizeneeded;

fixed_t                    centerxfrac;
fixed_t                    centeryfrac;
fixed_t                    projection;
fixed_t                    viewx;
fixed_t                    viewy;
fixed_t                    viewz;
fixed_t                    viewcos;
fixed_t                    viewsin;

// [AM] Fractional part of the current tic, in the half-open
//      range of [0.0, 1.0).  Used for interpolation.
fixed_t                    fractionaltic;

angle_t                    viewangle;

// precalculated math tables
angle_t                    clipangle;

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t                    xtoviewangle[SCREENWIDTH+1];

lighttable_t*              fixedcolormap;
lighttable_t*              scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t*              scalelightfixed[MAXLIGHTSCALE];
lighttable_t*              zlight[LIGHTLEVELS][MAXLIGHTZ];

player_t*                  viewplayer;

extern lighttable_t**      walllights;

void (*colfunc) (void);
void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);
void (*transcolfunc) (void);
void (*tlcolfunc) (void);
void (*spanfunc) (void);


//
// R_AddPointToBox
// Expand a given bbox
// so that it encloses a given point.
//
void
R_AddPointToBox
( int                x,
  int                y,
  fixed_t*           box )
{
    if (x< box[BOXLEFT])
        box[BOXLEFT] = x;
    if (x> box[BOXRIGHT])
        box[BOXRIGHT] = x;
    if (y< box[BOXBOTTOM])
        box[BOXBOTTOM] = y;
    if (y> box[BOXTOP])
        box[BOXTOP] = y;
}


//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
int
R_PointOnSide
( fixed_t        x,
  fixed_t        y,
  node_t*        node )
{
    fixed_t      dx;
    fixed_t      dy;
    fixed_t      left;
    fixed_t      right;
        
    if (!node->dx)
    {
        if (x <= node->x)
            return node->dy > 0;
        
        return node->dy < 0;
    }
    if (!node->dy)
    {
        if (y <= node->y)
            return node->dx < 0;
        
        return node->dx > 0;
    }
        
    dx = (x - node->x);
    dy = (y - node->y);
        
    // Try to quickly decide by looking at sign bits.
    if ( (node->dy ^ node->dx ^ dx ^ dy)&0x80000000 )
    {
        if  ( (node->dy ^ dx) & 0x80000000 )
        {
            // (left is negative)
            return 1;
        }
        return 0;
    }

    left = FixedMul ( node->dy>>FRACBITS , dx );
    right = FixedMul ( dy , node->dx>>FRACBITS );
        
    if (right < left)
    {
        // front side
        return 0;
    }
    // back side
    return 1;                        
}


int
R_PointOnSegSide
( fixed_t        x,
  fixed_t        y,
  seg_t*         line )
{
    fixed_t      lx;
    fixed_t      ly;
    fixed_t      ldx;
    fixed_t      ldy;
    fixed_t      dx;
    fixed_t      dy;
    fixed_t      left;
    fixed_t      right;
        
    lx = line->v1->x;
    ly = line->v1->y;
        
    ldx = line->v2->x - lx;
    ldy = line->v2->y - ly;
        
    if (!ldx)
    {
        if (x <= lx)
            return ldy > 0;
        
        return ldy < 0;
    }
    if (!ldy)
    {
        if (y <= ly)
            return ldx < 0;
        
        return ldx > 0;
    }
        
    dx = (x - lx);
    dy = (y - ly);
        
    // Try to quickly decide by looking at sign bits.
    if ( (ldy ^ ldx ^ dx ^ dy)&0x80000000 )
    {
        if  ( (ldy ^ dx) & 0x80000000 )
        {
            // (left is negative)
            return 1;
        }
        return 0;
    }

    left = FixedMul ( ldy>>FRACBITS , dx );
    right = FixedMul ( dy , ldx>>FRACBITS );
        
    if (right < left)
    {
        // front side
        return 0;
    }
    // back side
    return 1;                        
}


//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table.
angle_t
R_PointToAngle
( fixed_t        x,
  fixed_t        y )
{        
    x -= viewx;
    y -= viewy;
    
    if ( (!x) && (!y) )
        return 0;

    if (x>= 0)
    {
        // x >=0
        if (y>= 0)
        {
            // y>= 0

            if (x>y)
            {
                // octant 0
                return tantoangle[ SlopeDiv(y,x)];
            }
            else
            {
                // octant 1
                return ANG90-1-tantoangle[ SlopeDiv(x,y)];
            }
        }
        else
        {
            // y<0
            y = -y;

            if (x>y)
            {
                // octant 8
                return -tantoangle[SlopeDiv(y,x)];
            }
            else
            {
                // octant 7
                return ANG270+tantoangle[ SlopeDiv(x,y)];
            }
        }
    }
    else
    {
        // x<0
        x = -x;

        if (y>= 0)
        {
            // y>= 0
            if (x>y)
            {
                // octant 3
                return ANG180-1-tantoangle[ SlopeDiv(y,x)];
            }
            else
            {
                // octant 2
                return ANG90+ tantoangle[ SlopeDiv(x,y)];
            }
        }
        else
        {
            // y<0
            y = -y;

            if (x>y)
            {
                // octant 4
                return ANG180+tantoangle[ SlopeDiv(y,x)];
            }
            else
            {
                 // octant 5
                return ANG270-1-tantoangle[ SlopeDiv(x,y)];
            }
        }
    }
    return 0;
}


angle_t
R_PointToAngle2
( fixed_t        x1,
  fixed_t        y1,
  fixed_t        x2,
  fixed_t        y2 )
{        
    viewx = x1;
    viewy = y1;
    
    return R_PointToAngle (x2, y2);
}


fixed_t
R_PointToDist
( fixed_t        x,
  fixed_t        y )
{
    int          angle;
    fixed_t      dx;
    fixed_t      dy;
    fixed_t      temp;
    fixed_t      dist;
    fixed_t      frac;
        
    dx = abs(x - viewx);
    dy = abs(y - viewy);
        
    if (dy>dx)
    {
        temp = dx;
        dx = dy;
        dy = temp;
    }

    // Fix crashes in udm1.wad

    if (dx != 0)
    {
        frac = FixedDiv(dy, dx);
    }
    else
    {
        frac = 0;
    }
        
    angle = (tantoangle[frac>>DBITS]+ANG90) >> ANGLETOFINESHIFT;

    // use as cosine
    dist = FixedDiv (dx, finesine[angle] );        
        
    return dist;
}




//
// R_InitPointToAngle
//
void R_InitPointToAngle (void)
{
    // UNUSED - now getting from tables.c
#if 0
    int        i;
    long        t;
    float        f;
//
// slope (tangent) to angle lookup
//
    for (i=0 ; i<=SLOPERANGE ; i++)
    {
        f = atan( (float)i/SLOPERANGE )/(3.141592657*2);
        t = 0xffffffff*f;
        tantoangle[i] = t;
    }
#endif
}



// [AM] Interpolate between two angles.
angle_t R_InterpolateAngle(angle_t oangle, angle_t nangle, fixed_t scale)
{
    if (nangle == oangle)
        return nangle;
    else if (nangle > oangle)
    {
        if (nangle - oangle < ANG270)
            return oangle + (angle_t)((nangle - oangle) * FIXED2DOUBLE(scale));
        else // Wrapped around
            return oangle - (angle_t)((oangle - nangle) * FIXED2DOUBLE(scale));
    }
    else // nangle < oangle
    {
        if (oangle - nangle < ANG270)
            return oangle - (angle_t)((oangle - nangle) * FIXED2DOUBLE(scale));
        else // Wrapped around
            return oangle + (angle_t)((nangle - oangle) * FIXED2DOUBLE(scale));
    }
}


//
// R_InitTables
//
void R_InitTables (void)
{
    // UNUSED: now getting from tables.c
#if 0
    int                i;
    float        a;
    float        fv;
    int                t;
    
    // viewangle tangent table
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
        a = (i-FINEANGLES/4+0.5)*PI*2/FINEANGLES;
        fv = FRACUNIT*tan (a);
        t = fv;
        finetangent[i] = t;
    }
    
    // finesine table
    for (i=0 ; i<5*FINEANGLES/4 ; i++)
    {
        // OPTIMIZE: mirror...
        a = (i+0.5)*PI*2/FINEANGLES;
        t = FRACUNIT*sin (a);
        finesine[i] = t;
    }
#endif

}



//
// R_InitTextureMapping
//
void R_InitTextureMapping (void)
{
    int                    i;
    int                    x;
    int                    t;
    fixed_t                focallength;
    
    // Use tangent table to generate viewangletox:
    //  viewangletox will give the next greatest x
    //  after the view angle.
    //
    // Calc focallength
    //  so FIELDOFVIEW angles covers SCREENWIDTH.
    focallength = FixedDiv (centerxfrac,
                            finetangent[FINEANGLES/4+FIELDOFVIEW/2] );
        
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
        if (finetangent[i] > FRACUNIT*2)
            t = -1;
        else if (finetangent[i] < -FRACUNIT*2)
            t = viewwidth+1;
        else
        {
            t = FixedMul (finetangent[i], focallength);
            t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

            if (t < -1)
                t = -1;
            else if (t>viewwidth+1)
                t = viewwidth+1;
        }
        viewangletox[i] = t;
    }
    
    // Scan viewangletox[] to generate xtoviewangle[]:
    //  xtoviewangle will give the smallest view angle
    //  that maps to x.        
    for (x=0;x<=viewwidth;x++)
    {
        i = 0;
        while (viewangletox[i]>x)
            i++;
        xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }
    
    // Take out the fencepost cases from viewangletox.
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
        t = FixedMul (finetangent[i], focallength);
        t = centerx - t;
        
        if (viewangletox[i] == -1)
            viewangletox[i] = 0;
        else if (viewangletox[i] == viewwidth+1)
            viewangletox[i]  = viewwidth;
    }
        
    clipangle = xtoviewangle[0];
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//

void R_InitLightTables (void)
{
    int                i;
    int                j;
    int                level;
    int                startmap;         
    int                scale;
    
    // Calculate the light levels to use
    //  for each level / distance combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
        startmap = ((LIGHTLEVELS-LIGHTBRIGHT-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
        for (j=0 ; j<MAXLIGHTZ ; j++)
        {
            scale = FixedDiv ((ORIGWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT); // HIRES
            scale >>= LIGHTSCALESHIFT;
            level = startmap - scale/DISTMAP;
            
            if (level < 0)
                level = 0;

            if (level >= NUMCOLORMAPS)
                level = NUMCOLORMAPS-1;

            zlight[i][j] = colormaps + level*256;
        }
    }
}



//
// R_SetViewSize
// Do not really change anything here,
//  because it might be in the middle of a refresh.
// The change will take effect next refresh.
//

void
R_SetViewSize
( int                blocks,
  int                detail )
{
    setsizeneeded = true;
    setblocks = blocks;
    setdetail = detail;
}


//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize (void)
{
    fixed_t        cosadj;
    fixed_t        dy;
    int            i;
    int            j;
    int            level;
    int            startmap;

    setsizeneeded = false;

    if (setblocks == 11)
    {
        scaledviewwidth = SCREENWIDTH;
        scaledviewheight = SCREENHEIGHT;                   // CHANGED FOR HIRES
    }
    else
    {
        scaledviewwidth = (setblocks*32)<<hires;           // CHANGED FOR HIRES
        scaledviewheight = ((setblocks*168/10)&~7)<<hires; // CHANGED FOR HIRES
    }
    
    detailshift = setdetail;
    viewwidth = scaledviewwidth>>detailshift;
    viewheight = scaledviewheight>>(detailshift && hires); // ADDED FOR HIRES

    centery = viewheight/2;

    centerx = viewwidth/2;
    centerxfrac = centerx<<FRACBITS;
    centeryfrac = centery<<FRACBITS;
    projection = centerxfrac;

    if (!detailshift)
    {
        colfunc = basecolfunc = R_DrawColumn;
        fuzzcolfunc = R_DrawFuzzColumn;
        transcolfunc = R_DrawTranslatedColumn;
        spanfunc = R_DrawSpan;
        tlcolfunc = R_DrawTLColumn;
    }
    else
    {
        colfunc = basecolfunc = R_DrawColumnLow;
        fuzzcolfunc = R_DrawFuzzColumnLow;
        transcolfunc = R_DrawTranslatedColumnLow;
        spanfunc = R_DrawSpanLow;
        tlcolfunc = R_DrawTLColumnLow;
    }

    R_InitBuffer (scaledviewwidth, scaledviewheight); // CHANGED FOR HIRES
        
    R_InitTextureMapping ();
    
    // psprite scales
    pspritescale = FRACUNIT*viewwidth/ORIGWIDTH;      // CHANGED FOR HIRES
    pspriteiscale = FRACUNIT*ORIGWIDTH/viewwidth;     // CHANGED FOR HIRES
    
    // thing clipping
    for (i=0 ; i<viewwidth ; i++)
        screenheightarray[i] = viewheight;
    
    // planes
    for (i=0 ; i<viewheight ; i++)
    {
        dy = ((i-viewheight/2)<<FRACBITS)+FRACUNIT/2;
        dy = abs(dy);

        // HIRES
        yslope[i] = FixedDiv ( (viewwidth<<(detailshift && !hires))/2*FRACUNIT, dy);
    }
        
    for (i=0 ; i<viewwidth ; i++)
    {
        cosadj = abs(finecosine[xtoviewangle[i]>>ANGLETOFINESHIFT]);
        distscale[i] = FixedDiv (FRACUNIT,cosadj);
    }
    
    // Calculate the light levels to use
    //  for each level / scale combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
        startmap = ((LIGHTLEVELS-LIGHTBRIGHT-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
        for (j=0 ; j<MAXLIGHTSCALE ; j++)
        {
            level = startmap - j*SCREENWIDTH/(viewwidth<<detailshift)/DISTMAP;
            
            if (level < 0)
                level = 0;

            if (level >= NUMCOLORMAPS)
                level = NUMCOLORMAPS-1;

            scalelight[i][j] = colormaps + level*256;
        }
    }
}


void R_InitColumnFunctions(void)
{
    int i;

    colfunc = basecolfunc = R_DrawColumn;

    for (i = 0; i < NUMMOBJTYPES; i++)
    {
        mobjinfo_t      *info = &mobjinfo[i];

        if (info->flags & MF_SHADOW)
            info->colfunc = fuzzcolfunc;
        else
            info->colfunc = basecolfunc;
    }
}


//
// R_Init
//



void R_Init (void)
{
    R_InitData ();
    printf (".");
    //C_Printf (CR_GRAY, ".");

    R_InitPointToAngle ();
    printf (".");
    //C_Printf (CR_GRAY, ".");

    R_InitTables ();
    // viewwidth / viewheight / detailLevel are set by the defaults
    printf (".");
    //C_Printf (CR_GRAY, ".");


    R_SetViewSize (screenblocks, detailLevel);
    R_InitPlanes ();
    printf (".");
    //C_Printf (CR_GRAY, ".");

    R_InitLightTables ();
    printf (".");
    //C_Printf (CR_GRAY, ".");

    R_InitSkyMap ();
    R_InitTranslationTables ();
    printf (".");

    R_InitColumnFunctions();
    //C_Printf (CR_GRAY, ".");
    printf("]");
    //C_Printf (CR_GRAY, "]");
        
    framecount = 0;
}


//
// R_PointInSubsector
//
subsector_t*
R_PointInSubsector
( fixed_t        x,
  fixed_t        y )
{
    node_t*      node;
    int          side;
    int          nodenum;

    // single subsector is a special case
    if (!numnodes)                                
        return subsectors;
                
    nodenum = numnodes-1;

    while (! (nodenum & NF_SUBSECTOR) )
    {
        node = &nodes[nodenum];
        side = R_PointOnSide (x, y, node);
        nodenum = node->children[side];
    }
        
    return &subsectors[nodenum & ~NF_SUBSECTOR];
}


//
// R_SetupPitch
// villsa [STRIFE] new function
// Calculate centery/centeryfrac for player viewpitch
//
void R_SetupPitch(player_t* player)
{
    int i;
    int tempCentery;

    tempCentery = (viewheight / 2) + ((player->recoilpitch / (SCREENWIDTH * 32)) +
                  (player->lookdir << ((hires && !detailshift)) * (screenblocks / 10)));

    if (centery != tempCentery)
    {
        centery = tempCentery;
        centeryfrac = centery << FRACBITS;
        for (i = 0; i < viewheight; i++)
        {
            yslope[i] = FixedDiv((viewwidth << (detailshift && !hires)) / 2 * FRACUNIT,
                                 abs(((i - centery) << FRACBITS) +
                                     FRACUNIT / 2));
        }
    }
}

//
// R_SetupFrame
//
void R_SetupFrame (player_t* player)
{                
    int          i;
    
    R_SetupPitch(player);

    viewplayer = player;

    // [AM] Interpolate the player camera if the feature is enabled.

    // Figure out how far into the current tic we're in as a fixed_t
    if (d_uncappedframerate)
        fractionaltic = I_GetTimeMS() * TICRATE % 1000 * FRACUNIT / 1000;

    if (d_uncappedframerate &&
        // Don't interpolate on the first tic of a level,
        // otherwise oldviewz might be garbage.
        leveltime > 1 &&
        // Don't interpolate if the player did something 
        // that would necessitate turning it off for a tic.
        player->mo->interp == true &&
        // Don't interpolate during a paused state
        !paused && !menuactive)
    {
        // Interpolate player camera from their old position to their current one.
        viewx = player->mo->oldx + FixedMul(player->mo->x -
                player->mo->oldx, fractionaltic);
        viewy = player->mo->oldy + FixedMul(player->mo->y -
                player->mo->oldy, fractionaltic);
        viewz = player->oldviewz + FixedMul(player->viewz -
                player->oldviewz, fractionaltic);
        viewangle = R_InterpolateAngle(player->mo->oldangle,
                player->mo->angle, fractionaltic) + viewangleoffset;
    }
    else
    {
        viewx = player->mo->x;
        viewy = player->mo->y;
        viewz = player->viewz;
        viewangle = player->mo->angle + viewangleoffset;
    }

    extralight = player->extralight;

    viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
    viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];
        
    sscount = 0;
        
    if (player->fixedcolormap)
    {
        fixedcolormap =
            colormaps
            + player->fixedcolormap*256*sizeof(lighttable_t);
        
        walllights = scalelightfixed;

        for (i=0 ; i<MAXLIGHTSCALE ; i++)
            scalelightfixed[i] = fixedcolormap;
    }
    else
        fixedcolormap = 0;
                
    framecount++;
    validcount++;

    if (BorderNeedRefresh)
    {
        if (setblocks < 10)
        {
            R_DrawViewBorder();
        }
        BorderNeedRefresh = false;
    }
}


void R_HOMdrawer()
{
    byte colour = (gametic % 20) < 9 ? 40 : 0;

    V_ColorBlock(0, 0, 0, SCREENWIDTH, SCREENHEIGHT - SBARHEIGHT, colour);
}


//
// R_RenderView
//
void R_RenderPlayerView (player_t* player)
{        
    R_SetupFrame (player);

    // Clear buffers.
    R_ClearClipSegs ();
    R_ClearDrawSegs ();
    R_ClearPlanes ();
    R_ClearSprites ();
    
    if(autodetect_hom && !menuactive)
        R_HOMdrawer();

    // check for new console commands.
    NetUpdate ();

    // The head node is the last node output.
    R_RenderBSPNode (numnodes-1);
    
    // Check for new console commands.
    NetUpdate ();
    
    R_DrawPlanes ();
    
    // Check for new console commands.
    NetUpdate ();
    
    R_DrawMasked ();

    // Check for new console commands.
    NetUpdate ();                                
}

angle_t R_WadToAngle(int wadangle)
{
  // maintain compatibility
  
//  if(demo_version < 302)
    return (wadangle / 45) * ANG45;

  // allows wads to specify angles to
  // the nearest degree, not nearest 45  

//  return wadangle * (ANG45 / 45);
}


