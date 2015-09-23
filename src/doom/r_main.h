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
//        System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __R_MAIN__
#define __R_MAIN__

#include "d_player.h"
#include "r_data.h"


//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.
/*
#define LIGHTLEVELS             32
#define LIGHTSEGSHIFT           3
#define LIGHTBRIGHT             2
#define MAXLIGHTSCALE           48
#define LIGHTSCALESHIFT         12
#define MAXLIGHTZ               128
#define LIGHTZSHIFT             20
*/
#define LIGHTLEVELS             128
#define LIGHTSEGSHIFT           1
#define LIGHTBRIGHT             2
#define MAXLIGHTSCALE           384
#define LIGHTSCALESHIFT         12
#define MAXLIGHTZ               1024
#define LIGHTZSHIFT             17

#define OLDLIGHTLEVELS          32
#define OLDLIGHTSEGSHIFT        3
#define OLDLIGHTBRIGHT          2
#define OLDMAXLIGHTSCALE        48
#define OLDLIGHTSCALESHIFT      13
#define OLDMAXLIGHTZ            2048
#define OLDLIGHTZSHIFT          16

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.
#define NUMCOLORMAPS      32


// killough 3/20/98: Support dynamic colormaps, e.g. deep water
// killough 4/4/98: support dynamic number of them as well
extern int                numcolormaps;

extern int                viewwindowx;
extern int                viewwindowy;
extern int                centerx;
extern int                centery;
extern int                validcount;
extern int                linecount;
extern int                loopcount;
extern int                extralight;

// Blocky/low detail mode.
//B remove this?
//  0 = high, 1 = low
extern int                detailshift;        

//
// POV related.
//
extern fixed_t            viewcos;
extern fixed_t            viewsin;
extern fixed_t            centerxfrac;
extern fixed_t            centeryfrac;
extern fixed_t            projection;

// [AM] Fractional part of the current tic, in the half-open
//      range of [0.0, 1.0).  Used for interpolation.
extern fixed_t            fractionaltic;

extern lighttable_t*      (*scalelight)[MAXLIGHTSCALE];
extern lighttable_t*      (*psprscalelight)[OLDMAXLIGHTSCALE];
extern lighttable_t*      scalelightfixed[MAXLIGHTSCALE];
extern lighttable_t*      (*zlight)[MAXLIGHTZ];
extern lighttable_t*      fixedcolormap;
extern lighttable_t*      fullcolormap;



//
// Function pointers to switch refresh/drawing functions.
// Used to select shadow mode etc.
//
extern void               (*colfunc) (void);
extern void               (*transcolfunc) (void);
extern void               (*basecolfunc) (void);
extern void               (*fuzzcolfunc) (void);
extern void               (*tlcolfunc) (void);
extern void               (*wallcolfunc)(void);
extern void               (*fbwallcolfunc)(void);

// No shadow effects on floors.
extern void               (*spanfunc) (void);


//
// Utility functions.
int
R_PointOnSide
( fixed_t        x,
  fixed_t        y,
  node_t*        node );

int
R_PointOnSegSide
( fixed_t        x,
  fixed_t        y,
  seg_t*         line );

angle_t
R_PointToAngle
( fixed_t        x,
  fixed_t        y );

angle_t
R_PointToAngle2
( fixed_t        x1,
  fixed_t        y1,
  fixed_t        x2,
  fixed_t        y2 );

fixed_t
R_PointToDist
( fixed_t        x,
  fixed_t        y );


subsector_t*
R_PointInSubsector
( fixed_t        x,
  fixed_t        y );

void
R_AddPointToBox
( int            x,
  int            y,
  fixed_t*       box );

// [AM] Interpolate between two angles.
angle_t R_InterpolateAngle(angle_t oangle, angle_t nangle, fixed_t scale);

//
// REFRESH - the actual rendering functions.
//

// Called by G_Drawer.
void R_RenderPlayerView (player_t *player);

// Called by startup code.
void R_Init (void);

// Called by M_Responder.
void R_SetViewSize (int blocks);

angle_t R_WadToAngle(int wadangle);

#endif
