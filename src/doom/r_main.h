/*
========================================================================

                               DOOM Retro
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright (C) 2013-2015 Brad Harding.

  DOOM Retro is a fork of Chocolate DOOM by Simon Howard.
  For a complete list of credits, see the accompanying AUTHORS file.

  This file is part of DOOM Retro.

  DOOM Retro is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM Retro is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM Retro. If not, see <http://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM Retro is in no way affiliated with nor endorsed by
  id Software LLC.

========================================================================
*/


#if !defined(__R_MAIN__)
#define __R_MAIN__


#include "d_player.h"
#include "r_data.h"


//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.
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
#define NUMCOLORMAPS            32


// haleyjd 20140902: [SVE] interpolation
typedef enum
{
    SEC_INTERPOLATE,
    SEC_NORMAL

} secinterpstate_e;


//
// Function pointers to switch refresh/drawing functions.
// Used to select shadow mode etc.
//
void (*colfunc)(void);
void (*wallcolfunc)(void);
void (*fbwallcolfunc)(void);
void (*transcolfunc)(void);
void (*basecolfunc)(void);
void (*fuzzcolfunc)(void);
void (*tlcolfunc)(void);
void (*tl5colfunc)(void);
void (*tl10colfunc)(void);
void (*tl12colfunc)(void);
void (*tl15colfunc)(void);
void (*tl20colfunc)(void);
void (*tl25colfunc)(void);
void (*tl30colfunc)(void);
void (*tl33colfunc)(void);
void (*tl35colfunc)(void);
void (*tl40colfunc)(void);
void (*tl45colfunc)(void);
void (*tl50colfunc)(void);
void (*tl55colfunc)(void);
void (*tl60colfunc)(void);
void (*tl65colfunc)(void);
void (*tl66colfunc)(void);
void (*tl70colfunc)(void);
void (*tl75colfunc)(void);
void (*tl80colfunc)(void);
void (*tl85colfunc)(void);
void (*tl90colfunc)(void);
void (*tl95colfunc)(void);
void (*tlgreencolfunc)(void);
void (*tlredcolfunc)(void);
void (*tlredwhitecolfunc1)(void);
void (*tlredwhitecolfunc2)(void);
void (*tlredwhite50colfunc)(void);
void (*tlbluecolfunc)(void);
void (*tlgreen33colfunc)(void);
void (*tlred33colfunc)(void);
void (*tlblue33colfunc)(void);
void (*redtobluecolfunc)(void);
void (*tlredtoblue33colfunc)(void);
void (*skycolfunc)(void);
void (*redtogreencolfunc)(void);
void (*tlredtogreen33colfunc)(void);
void (*psprcolfunc)(void);
void (*spanfunc)(void);
void (*bloodsplatcolfunc)(void);
void (*megaspherecolfunc)(void);

//
// Utility functions.

// [AM] Interpolate between two angles.
angle_t R_InterpolateAngle(angle_t oangle, angle_t nangle, fixed_t scale);

angle_t R_PointToAngle(fixed_t x, fixed_t y);
angle_t R_PointToAngle2(fixed_t x1, fixed_t y1, fixed_t x, fixed_t y);
angle_t R_PointToAngleEx(fixed_t x, fixed_t y);
angle_t R_PointToAngleEx2(fixed_t x1, fixed_t y1, fixed_t x, fixed_t y);

fixed_t R_PointToDist(fixed_t x, fixed_t y);

subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);

// Called by G_Drawer.
void R_RenderPlayerView(player_t *player);

// Called by startup code.
void R_Init(void);

// Called by M_Responder.
void R_SetViewSize(int blocks);

void R_ExecuteSetViewSize(void);
void R_InitColumnFunctions(void);
void R_AddSprites(sector_t *sec, int lightlevel);
void R_StoreWallRange(int start, int stop);

int R_PointOnSide(fixed_t x, fixed_t y, const node_t *node);
int R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line);


//
// POV related.
//
extern fixed_t          viewcos;
extern fixed_t          viewsin;

// [AM] Fractional part of the current tic, in the half-open
//      range of [0.0, 1.0). Used for interpolation.
extern fixed_t          fractionaltic;

extern fixed_t          centerxfrac;
extern fixed_t          centeryfrac;
extern fixed_t          projection;
extern fixed_t          projectiony;

// killough 3/20/98: Allow colormaps to be dynamic (e.g. underwater)
extern lighttable_t     *(*scalelight)[MAXLIGHTSCALE];
extern lighttable_t     *(*zlight)[MAXLIGHTZ];
extern lighttable_t     *(*psprscalelight)[OLDMAXLIGHTSCALE];
extern lighttable_t     *fullcolormap;
extern lighttable_t     *fixedcolormap;
extern lighttable_t     **colormaps;

// killough 4/4/98: dynamic number of maps
extern int              numcolormaps;

extern int              viewwindowx;
extern int              viewwindowy;
extern int              centerx;
extern int              centery;
extern int              validcount;
extern int              extralight;


#endif

