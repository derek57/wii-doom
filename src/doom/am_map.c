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
//
// DESCRIPTION:  the automap code
//
//-----------------------------------------------------------------------------


#include <math.h>
#include <stdio.h>

#include "am_map.h"
#include "c_io.h"
#include "d_deh.h"
#include "doomkeys.h"
#include "doomdef.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "hu_stuff.h"
#include "i_system.h"
#include "m_controls.h"
#include "../i_video.h"

#ifndef WII
#include "m_cheat.h"
#endif

#include "m_menu.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_state.h"
#include "st_stuff.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

#ifdef WII
#include <wiiuse/wpad.h>
#endif


// drawing stuff

#define AM_NUMMARKPOINTS      10

// scale on entry
#define INITSCALEMTOF         (.2 * FRACUNIT)

// how much the automap moves window per tic in frame-buffer coordinates
// moves 140 pixels in 1 second
#define F_PANINC              4

// translates between frame-buffer and map distances
#define FTOM(x)               FixedMul(((x) << FRACBITS), scale_ftom)
#define MTOF(x)               (FixedMul((x), scale_mtof) >> FRACBITS)

// translates between frame-buffer and map coordinates
#define CXMTOF(x)             (f_x + MTOF((x) - m_x))
#define CYMTOF(y)             (f_y + (f_h - MTOF((y) - m_y)))

// the following is crap
#define LINE_NEVERSEE         ML_DONTDRAW

#define DOOUTCODE(oc, mx, my) \
    (oc) = 0;                 \
                              \
    if ((my) < 0)             \
        (oc) |= TOP;          \
    else if ((my) >= f_h)     \
        (oc) |= BOTTOM;       \
                              \
    if ((mx) < 0)             \
        (oc) |= LEFT;         \
    else if ((mx) >= f_w)     \
        (oc) |= RIGHT;

#define PUTDOT(xx, yy, cc)    fb[(yy) * f_w + (xx)] = (cc)

#define MAPBITS               12
#define FRACTOMAPBITS         (FRACBITS - MAPBITS)

// player radius for movement checking
// e6y
#define PLAYERRADIUS          (16 * (1 << MAPBITS))


typedef struct
{
    int x, y;

} fpoint_t;

typedef struct
{
    fpoint_t a, b;

} fline_t;

typedef struct
{
    mpoint_t a, b;

} mline_t;

//
// [nitr8] UNUSED
//
/*
typedef struct
{
    fixed_t slp, islp;

} islope_t;
*/


//
// The vector graphics for the automap.
//  A line drawing of the player pointing right,
//   starting from the middle.
//
#define R ((8 * PLAYERRADIUS) / 7)
mline_t player_arrow[] = 
{
    // -----
    { { -R + R / 8, 0 }, { R, 0 } },
    // ----->
    { { R, 0 }, { R - R / 2, R / 4 } },
    { { R, 0 }, { R - R / 2, -R / 4 } },
    // >---->
    { { -R + R / 8, 0 }, { -R - R / 8, R / 4 } },
    { { -R + R / 8, 0 }, { -R - R / 8, -R / 4 } },
    // >>--->
    { { -R + 3 * R / 8, 0 }, { -R + R / 8, R / 4 } },
    { { -R + 3 * R / 8, 0 }, { -R + R / 8, -R / 4 } }
};
#undef R

#define R ((8 * PLAYERRADIUS) / 7)
mline_t cheat_player_arrow[] = 
{
    // -----
    { { -R + R / 8, 0 }, { R, 0 } },
    // ----->
    { { R, 0 }, { R - R / 2, R / 6 } },
    { { R, 0 }, { R - R / 2, -R / 6 } },
    // >----->
    { { -R + R / 8, 0 }, { -R - R / 8, R / 6 } },
    { { -R + R / 8, 0 }, { -R - R / 8, -R / 6 } },
    // >>----->
    { { -R + 3 * R / 8, 0 }, { -R + R / 8, R / 6 } },
    { { -R + 3 * R / 8, 0 }, { -R + R / 8, -R / 6 } },
    // >>-d--->
    { { -R / 2, 0 }, { -R / 2, -R / 6 } },
    { { -R / 2, -R / 6 }, { -R / 2 + R / 6, -R / 6 } },
    { { -R / 2 + R / 6, -R / 6 }, { -R / 2 + R / 6, R / 4 } },
    // >>-dd-->
    { { -R / 6, 0 }, { -R / 6, -R / 6 } },
    { { -R / 6, -R / 6 }, { 0, -R / 6 } },
    { { 0, -R / 6 }, { 0, R / 4 } },
    // >>-ddt->
    { { R / 6, R / 4 }, { R / 6, -R / 7 } },
    { { R / 6, -R / 7 }, { R / 6 + R / 32, -R / 7 - R / 32 } },
    { { R / 6 + R / 32, -R / 7 - R / 32 }, { R / 6 + R / 10, -R / 7 } }
};
#undef R

#define R (FRACUNIT)
mline_t triangle_guy[] =
{
    { { (fixed_t)(-.867 * R), (fixed_t)(-.5 * R) }, { (fixed_t)(.867 * R), (fixed_t)(-.5 * R) } },
    { { (fixed_t)(.867 * R), (fixed_t)(-.5 * R) }, { (fixed_t)(0), (fixed_t)(R) } },
    { { (fixed_t)(0), (fixed_t)(R) }, { (fixed_t)(-.867 * R), (fixed_t)(-.5 * R) } }
};
#undef R

#define R (FRACUNIT)
mline_t thintriangle_guy[] =
{
    { { (fixed_t)(-.5 * R), (fixed_t)(-.7 * R) }, { (fixed_t)(R), (fixed_t)(0) } },
    { { (fixed_t)(R), (fixed_t)(0) }, { (fixed_t)(-.5 * R), (fixed_t)(.7 * R) } },
    { { (fixed_t)(-.5 * R), (fixed_t)(.7 * R) }, { (fixed_t)(-.5 * R), (fixed_t)(-.7 * R) } }
};
#undef R

// jff 1/5/98 new symbol for keys on automap
#define R (FRACUNIT)
mline_t cross_mark[] =
{
    { { -R, 0 }, { R, 0} },
    { { 0, -R }, { 0, R } }
};
#undef R

#define NUMCROSSMARKLINES (sizeof(cross_mark) / sizeof(mline_t))


cheatseq_t cheat_amap = CHEAT("iddt", 0);
cheatseq_t cheat_amap_beta = CHEAT("eek", 0);


int                cheating = 0;
int                lightlev;


// pseudo-frame buffer
static byte        *fb;

// next point to be assigned
static int         markpointnum = 0;

// kluge until AM_LevelInit() is called
static int         leveljuststarted = 1;

static int         finit_width = SCREENWIDTH;

// CHANGED FOR HIRES
static int         finit_height = SCREENHEIGHT;

// location of window on screen
static int         f_x;
static int         f_y;

// size of window on screen
static int         f_w;
static int         f_h;

// used for funky strobing effect
static int         amclock;

// how far the window pans each tic (map coords)
static mpoint_t    m_paninc;

// LL x,y where the window is on the map (map coords)
static fixed_t     m_x, m_y;

// UR x,y where the window is on the map (map coords)
static fixed_t     m_x2, m_y2;

// width/height of window on map (map coords)
static fixed_t     m_w;
static fixed_t     m_h;

// based on level size
static fixed_t     min_x;
static fixed_t     min_y; 
static fixed_t     max_x;
static fixed_t     max_y;

// max_x-min_x
static fixed_t     max_w;

// max_y-min_y
static fixed_t     max_h;

// based on player size
static fixed_t     min_w;
static fixed_t     min_h;

// used to tell when to stop zooming out
static fixed_t     min_scale_mtof;

// used to tell when to stop zooming in
static fixed_t     max_scale_mtof;

// old stuff for recovery later
static fixed_t     old_m_w, old_m_h;
static fixed_t     old_m_x, old_m_y;

// used by MTOF to scale from map-to-frame-buffer coords
static fixed_t     scale_mtof = (fixed_t)INITSCALEMTOF;

// used by FTOM to scale from frame-buffer-to-map coords (=1/scale_mtof)
static fixed_t     scale_ftom;

// old location used by the Follower routine
static mpoint_t    f_oldloc;

// the player represented by an arrow
static player_t    *plr;

// numbers used for marking by the automap
static patch_t     *marknums[10];

static dboolean    stopped = true;
static dboolean    movement;


// killough 2/22/98: Remove limit on automap marks,
// and make variables external for use in savegames.
// where the points are
markpoint_t        *markpoints = NULL;

// how far the window zooms in each tic (map coords)
fixed_t            mtof_zoommul;

// how far the window zooms in each tic (fb coords)
fixed_t            ftom_zoommul;

fixed_t            am_viewx;
fixed_t            am_viewy;

angle_t            am_viewangle;

dboolean           dont_move_backwards = false;
dboolean           automapactive = false;

// killough 2/22/98
int                markpointnum_max = 0;


extern int         oldscreenblocks;
extern int         oldscreenSize;


// Calculates the slope and slope according to the x-axis of a line
// segment in map coordinates (with the upright y-axis n' all) so
// that it can be used with the brain-dead drawing stuff.
//
// [nitr8] UNUSED
/*
void AM_getIslope(mline_t *ml, islope_t *is)
{
    int dx = ml->b.x - ml->a.x;
    int dy = ml->a.y - ml->b.y;

    if (!dy)
        is->islp = (dx < 0 ? -INT_MAX : INT_MAX);
    else
        is->islp = FixedDiv(dx, dy);

    if (!dx)
        is->slp = (dy < 0 ? -INT_MAX : INT_MAX);

    else is->slp = FixedDiv(dy, dx);
}
*/

//
// AM_activateNewScale()
//
// Changes the map scale after zooming or translating
//
// Passed nothing, returns nothing
//
static void AM_activateNewScale(void)
{
    m_x += m_w / 2;
    m_y += m_h / 2;
    m_w = FTOM(f_w);
    m_h = FTOM(f_h);
    m_x -= m_w / 2;
    m_y -= m_h / 2;
    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;
}

//
// AM_saveScaleAndLoc()
//
// Saves the current center and zoom
// Affects the variables that remember old scale and loc
//
// Passed nothing, returns nothing
//
static void AM_saveScaleAndLoc(void)
{
    old_m_x = m_x;
    old_m_y = m_y;
    old_m_w = m_w;
    old_m_h = m_h;
}

//
// AM_restoreScaleAndLoc()
//
// restores the center and zoom from locally saved values
// Affects global variables for location and scale
//
// Passed nothing, returns nothing
//
static void AM_restoreScaleAndLoc(void)
{
    m_w = old_m_w;
    m_h = old_m_h;

    if (!followplayer)
    {
        m_x = old_m_x;
        m_y = old_m_y;
    }
    else
    {
        // e6y
        m_x = (am_viewx >> FRACTOMAPBITS) - m_w / 2;
        m_y = (am_viewy >> FRACTOMAPBITS) - m_h / 2;
    }

    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;

    // Change the scaling multipliers
    scale_mtof = FixedDiv(f_w << FRACBITS, m_w);
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

static void AM_setMarkParams(int num)
{
    int i;

    markpoints[num].w = 0;
    markpoints[num].h = 0;

    M_snprintf(markpoints[num].label, sizeof(markpoints[num].label), "%d", num);

    for (i = 0; i < (int)strlen(markpoints[num].label); i++)
    {
        markpoints[num].widths[i] = 3;
        markpoints[num].w += markpoints[num].widths[i] + 1;
        markpoints[num].h = MAX(markpoints[num].h, 5);
    }
}

//
// AM_addMark()
//
// Adds a marker at the current location
// Affects global variables for marked points
//
// Passed nothing, returns nothing
//
static void AM_addMark(void)
{
    static char message[32];

    // killough 2/22/98:
    // remove limit on automap marks
    if (markpointnum >= markpointnum_max)
    {
        markpointnum_max = (markpointnum_max ? (markpointnum_max << 1) : 16);
        markpoints = Z_Realloc(markpoints, markpointnum_max * sizeof(*markpoints));
    }

    markpoints[markpointnum].x = m_x + m_w / 2;
    markpoints[markpointnum].y = m_y + m_h / 2;
    AM_setMarkParams(markpointnum);
    M_snprintf(message, sizeof(message), s_AMSTR_MARKEDSPOT, ++markpointnum);
    HU_PlayerMessage(message, false);
}

//
// AM_findMinMaxBoundaries()
//
// Determines bounding box of all vertices,
// sets global variables controlling zoom range.
//
// Passed nothing, returns nothing
//
static void AM_findMinMaxBoundaries(void)
{
    int i;
    fixed_t a;
    fixed_t b;

    min_x = min_y =  INT_MAX;
    max_x = max_y = -INT_MAX;
  
    for (i = 0; i < numvertexes; i++)
    {
        if (vertexes[i].x < min_x)
            min_x = vertexes[i].x;
        else if (vertexes[i].x > max_x)
            max_x = vertexes[i].x;
    
        if (vertexes[i].y < min_y)
            min_y = vertexes[i].y;
        else if (vertexes[i].y > max_y)
            max_y = vertexes[i].y;
    }
  
    // e6y
    max_w = (max_x >>= FRACTOMAPBITS) - (min_x >>= FRACTOMAPBITS);
    max_h = (max_y >>= FRACTOMAPBITS) - (min_y >>= FRACTOMAPBITS);

    // const? never changed?
    min_w = 2 * PLAYERRADIUS;
    min_h = 2 * PLAYERRADIUS;

    a = FixedDiv(f_w << FRACBITS, max_w);
    b = FixedDiv(f_h << FRACBITS, max_h);
  
    min_scale_mtof = a < b ? a : b;
    max_scale_mtof = FixedDiv(f_h << FRACBITS, 2 * PLAYERRADIUS);
}

//
// AM_rotate()
//
// Rotation in 2D.
// Used to rotate player arrow line character.
//
// Passed the coordinates of a point, and an angle
// Returns the coordinates rotated by the angle
//
// CPhipps - made static & enhanced for automap rotation
static void AM_rotate(fixed_t *x, fixed_t *y, angle_t a, fixed_t xorig, fixed_t yorig)
{
    fixed_t tmpx;

    // e6y
    xorig >>= FRACTOMAPBITS;
    yorig >>= FRACTOMAPBITS;

    tmpx = FixedMul(*x - xorig, finecosine[a >> ANGLETOFINESHIFT])
            - FixedMul(*y - yorig, finesine[a >> ANGLETOFINESHIFT]);

    *y = yorig + FixedMul(*x - xorig, finesine[a >> ANGLETOFINESHIFT]) +
                 FixedMul(*y - yorig, finecosine[a >> ANGLETOFINESHIFT]);

    *x = tmpx + xorig;
}

static void AM_rotatePoint_i(mpoint_t *p)
{
    fixed_t pivotx = m_x + m_w / 2;
    fixed_t pivoty = m_y + m_h / 2;

    p->x -= pivotx;
    p->y -= pivoty;

    AM_rotate(&p->x, &p->y, ANG90 - am_viewangle, 0, 0);

    p->x += pivotx;
    p->y += pivoty;
}

/*
static void AM_rotate_f(float *x, float *y, angle_t a)
{
    static angle_t prev_angle = 0;
    static float sinrot = 0.0f;
    static float cosrot = 1.0f;

    float rot, tmpx;
  
    if (a != prev_angle)
    {
        prev_angle = a;
        rot = (float)a / (float)(1u << 31) * (float)M_PI;
        sinrot = (float)sin(rot);
        cosrot = (float)cos(rot);
    }
  
    tmpx = ((*x) * cosrot) - ((*y) * sinrot);
    *y = ((*x) * sinrot) + ((*y) * cosrot);
    *x = tmpx;
}

static void AM_rotatePoint_f(mpoint_t *p)
{
    float x = (float)p->x;
    float y = (float)p->y;
    float pivotx = (float)m_x + (float)m_w / 2.0f;
    float pivoty = (float)m_y + (float)m_h / 2.0f;

    x -= pivotx;
    y -= pivoty;

    AM_rotate_f(&x, &y, ANG90 - am_viewangle);

    x += pivotx;
    y += pivoty;

    p->fx = x;
    p->fy = y;

    AM_rotatePoint_i(p);
}
*/

static void AM_rotatePoint(mpoint_t *p)
{
    AM_rotatePoint_i(p);
}

//
// AM_changeWindowLoc()
//
// Moves the map window by the global variables m_paninc.x, m_paninc.y
//
// Passed nothing, returns nothing
//
static void AM_changeWindowLoc(void)
{
    fixed_t incx, incy;

    if (m_paninc.x || m_paninc.y)
    {
        followplayer = false;
        f_oldloc.x = INT_MAX;
    }

    incx = m_paninc.x;
    incy = m_paninc.y;

    if (automapactive && am_rotate)
    {
        AM_rotate(&incx, &incy, am_viewangle - ANG90, 0, 0);
    }

    m_x += incx;
    m_y += incy;

    if (!(automapactive && am_rotate))
    {
        if (m_x + m_w / 2 > max_x)
            m_x = max_x - m_w / 2;
        else if (m_x + m_w / 2 < min_x)
            m_x = min_x - m_w / 2;
  
        if (m_y + m_h / 2 > max_y)
            m_y = max_y - m_h / 2;
        else if (m_y + m_h / 2 < min_y)
            m_y = min_y - m_h / 2;
    }

    m_x2 = m_x + m_w;
    m_y2 = m_y + m_h;
}

//
// AM_initVariables()
//
// Initialize the variables for the automap
//
// Affects the automap global variables
// Status bar is notified that the automap has been entered
// Passed nothing, returns nothing
//
static void AM_initVariables(int scrn)
{
    static event_t st_notify =
    {
        ev_keyup,
        AM_MSGENTERED,
        0,
        0
    };

    automapactive = true;
    fb = screens[scrn];

    f_oldloc.x = INT_MAX;
    amclock = 0;
    lightlev = 0;

    m_paninc.x = m_paninc.y = 0;
    ftom_zoommul = FRACUNIT;
    mtof_zoommul = FRACUNIT;

    m_w = FTOM(f_w);
    m_h = FTOM(f_h);

    // find player to center on initially
    if (playeringame[consoleplayer])
    {
        plr = &players[consoleplayer];
    }
    else
    {
        int pnum;

        plr = &players[0];

        for (pnum = 0; pnum < MAXPLAYERS; pnum++)
        {
            if (playeringame[pnum])
            {
                plr = &players[pnum];
                break;
            }
        }
    }

    // e6y
    m_x = (plr->mo->x >> FRACTOMAPBITS) - m_w / 2;
    m_y = (plr->mo->y >> FRACTOMAPBITS) - m_h / 2;

    m_x = plr->mo->x - m_w / 2;
    m_y = plr->mo->y - m_h / 2;
    AM_changeWindowLoc();

    // for saving & restoring
    old_m_x = m_x;
    old_m_y = m_y;
    old_m_w = m_w;
    old_m_h = m_h;

    // inform the status bar of the change
    ST_Responder(&st_notify);
}

//
// AM_loadPics()
//
static void AM_loadPics(void)
{
    int i;
    char namebuf[9];
  
    for (i = 0; i < 10; i++)
    {
        M_snprintf(namebuf, 9, "AMMNUM%d", i);
        marknums[i] = W_CacheLumpName(namebuf, PU_STATIC);
    }
}

//
// AM_unloadPics()
//
static void AM_unloadPics(void)
{
    int i;
    char namebuf[9];
  
    for (i = 0; i < 10; i++)
    {
        M_snprintf(namebuf, 9, "AMMNUM%d", i);
        W_ReleaseLumpName(namebuf);
    }
}

//
// AM_clearMarks()
//
// Sets the number of marks to 0, thereby clearing them from the display
//
// Affects the global variable markpointnum
// Passed nothing, returns nothing
//
static void AM_clearMarks(void)
{
    markpointnum = 0;
}

//
// AM_LevelInit()
//
// Initialize the automap at the start of a new level
// should be called at the start of every level
//
// Passed nothing, returns nothing
// Affects automap's global variables
//
static void AM_LevelInit(void)
{
    leveljuststarted = 0;

    f_x = f_y = 0;
    f_w = finit_width;
    f_h = finit_height;

    AM_clearMarks();

    AM_findMinMaxBoundaries();
    scale_mtof = FixedDiv(min_scale_mtof, (int)(0.7 * FRACUNIT));

    if (scale_mtof > max_scale_mtof)
        scale_mtof = min_scale_mtof;

    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
}

//
// AM_Stop()
//
// Cease automap operations, unload patches, notify status bar
//
// Passed nothing, returns nothing
//
void AM_Stop(void)
{
    static event_t st_notify =
    {
        0,
        ev_keyup,
        AM_MSGEXITED,
        0
    };

    AM_unloadPics();
    automapactive = false;
    ST_Responder(&st_notify);
    stopped = true;
    dont_move_backwards = false;
}

//
// AM_Start()
//
// Start up automap operations,
//  if a new level, or game start, (re)initialize level variables
//  init map variables
//  load mark patches
//
// Passed nothing, returns nothing
//
void AM_Start(void)
{
    // FOR PSP (CONDITION):
    // DON'T AUTO-ACTIVATE THE AUTOMAP...
    // FIXME: I DON'T LIKE THIS... :-/
#ifdef WII
    if (players[consoleplayer].pendingweapon != wp_chainsaw)
#endif
    {
        static int lastlevel = -1;
        static int lastepisode = -1;

        if (!stopped)
            AM_Stop();

        stopped = false;

        if (lastlevel != gamemap || lastepisode != gameepisode)
        {
            AM_LevelInit();
            lastlevel = gamemap;
            lastepisode = gameepisode;
        }

        AM_initVariables(0);
        AM_loadPics();
    }
}

//
// AM_minOutWindowScale()
//
// set the window scale to the maximum size
//
static void AM_minOutWindowScale(void)
{
    scale_mtof = min_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
    AM_activateNewScale();
}

//
// AM_maxOutWindowScale(void)
//
// set the window scale to the minimum size
//
static void AM_maxOutWindowScale(void)
{
    scale_mtof = max_scale_mtof;
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);
    AM_activateNewScale();
}

//
// Handle events (user inputs) in automap mode
//
//
// AM_Responder()
//
// Handle events (user inputs) in automap mode
//
// Passed an input event, returns true if its handled
//
dboolean AM_Responder(event_t *ev)
{
    int rc = false;

#ifdef WII
    WPADData *data = WPAD_Data(0);

    if (automapactive && ev->type == ev_joystick && (data->btns_d || ev->data2 != 0 || ev->data3 != 0))
    {
        rc = true;

        if (ev->data2 > 0)
        {
            if (!(automapactive && followplayer))
                m_paninc.x = FTOM(F_PANINC);
            else
                rc = false;
        }
        else if (ev->data2 < 0)
        {
            if (!(automapactive && followplayer))
                m_paninc.x = -FTOM(F_PANINC);
            else
                rc = false;
        }
        else if (ev->data3 > 0)
        {
            if (!(automapactive && followplayer))
                m_paninc.y = FTOM(F_PANINC);
            else
                rc = false;
        }
        else if (ev->data3 < 0)
        {
            if (!(automapactive && followplayer))
                m_paninc.y = -FTOM(F_PANINC);
            else
                rc = false;
        }
        else
            rc = false;
    }
    else if (ev->type == ev_joystick && (data->btns_u || ev->data2 == 0 || ev->data3 == 0))
    {
        rc = false;

        if (ev->data2 == 0)
        {
            if (!(automapactive && followplayer))
                m_paninc.x = 0;
        }

        if (ev->data3 == 0)
        {
            if (!(automapactive && followplayer))
                m_paninc.y = 0;
        }
    }

#else

    int key;

    static int bigstate = 0;

    if (!automapactive)
    {
        if (ev->type == ev_keydown && ev->data1 == key_map_toggle)
        {
            if (!menuactive)
                AM_Toggle();

            rc = true;
        }
    }
    else if (ev->type == ev_keydown)
    {
        rc = true;
        key = ev->data1;

        // pan right
        if (key == key_map_east)
        {
            if (!followplayer)
                m_paninc.x = FTOM(F_PANINC);
            else
                rc = false;
        }
        // pan left
        else if (key == key_map_west)
        {
            if (!followplayer)
                m_paninc.x = -FTOM(F_PANINC);
            else
                rc = false;
        }
        // pan up
        else if (key == key_map_north)
        {
            if (!followplayer)
                m_paninc.y = FTOM(F_PANINC);
            else
                rc = false;
        }
        // pan down
        else if (key == key_map_south)
        {
            if (!followplayer)
                m_paninc.y = -FTOM(F_PANINC);
            else
                rc = false;
        }
        // zoom out
        else if (key == key_map_zoomout)
        {
            mtof_zoommul = M_ZOOMOUT;
            ftom_zoommul = M_ZOOMIN;
        }
        // zoom in
        else if (key == key_map_zoomin)
        {
            mtof_zoommul = M_ZOOMIN;
            ftom_zoommul = M_ZOOMOUT;
        }
        else if (key == key_map_toggle)
        {
            bigstate = 0;

            if (!menuactive)
                AM_Toggle();
        }
        else if (key == key_map_maxzoom)
        {
            bigstate = !bigstate;

            if (bigstate)
            {
                AM_saveScaleAndLoc();
                AM_minOutWindowScale();
            }
            else
                AM_restoreScaleAndLoc();
        }
        else if (key == key_map_follow)
        {
            followplayer = !followplayer;
            f_oldloc.x = INT_MAX;

            if (followplayer)
                HU_PlayerMessage(s_AMSTR_FOLLOWON, true);
            else
                HU_PlayerMessage(s_AMSTR_FOLLOWOFF, true);
        }
        else if (key == key_map_grid)
        {
            drawgrid = !drawgrid;

            if (drawgrid)
                HU_PlayerMessage(s_AMSTR_GRIDON, true);
            else
                HU_PlayerMessage(s_AMSTR_GRIDOFF, true);
        }
        else if (key == key_map_mark)
        {
            AM_addMark();
        }
        else if (key == key_map_clearmark)
        {
            AM_clearMarks();
            HU_PlayerMessage(s_AMSTR_MARKSCLEARED, true);
        }
        else
        {
            rc = false;
        }

        if (!deathmatch)
        {
            if ((cht_CheckCheat(&cheat_amap, ev->data2) && !beta_style) ||
                (cht_CheckCheat(&cheat_amap_beta, ev->data2) && beta_style))
            {
                rc = false;
                cheating = (cheating + 1) % 3;
            }
        }
    }
    else if (ev->type == ev_keyup)
    {
        rc = false;
        key = ev->data1;

        if (key == key_map_east)
        {
            if (!followplayer)
                m_paninc.x = 0;
        }
        else if (key == key_map_west)
        {
            if (!followplayer)
                m_paninc.x = 0;
        }
        else if (key == key_map_north)
        {
            if (!followplayer)
                m_paninc.y = 0;
        }
        else if (key == key_map_south)
        {
            if (!followplayer)
                m_paninc.y = 0;
        }
        else if (key == key_map_zoomout || key == key_map_zoomin)
        {
            mtof_zoommul = FRACUNIT;
            ftom_zoommul = FRACUNIT;
        }
    }
    else if (ev->type == ev_mousewheel)
    {
        // zoom in
        if (ev->data1 > 0)
        {
            movement = true;
            mtof_zoommul = M_ZOOMIN;
            ftom_zoommul = M_ZOOMOUT;
            bigstate = false;
        }
        // zoom out
        else if (ev->data1 < 0)
        {
            movement = true;
            mtof_zoommul = M_ZOOMOUT;
            ftom_zoommul = M_ZOOMIN;
        }
    }
#endif

    return rc;
}

//
// AM_changeWindowScale()
//
// Zooming
//
static void AM_changeWindowScale(void)
{
    // Change the scaling multipliers
    scale_mtof = FixedMul(scale_mtof, mtof_zoommul);
    scale_ftom = FixedDiv(FRACUNIT, scale_mtof);

    if (scale_mtof < min_scale_mtof)
        AM_minOutWindowScale();
    else if (scale_mtof > max_scale_mtof)
        AM_maxOutWindowScale();
    else
        AM_activateNewScale();
}

//
// AM_doFollowPlayer()
//
// Turn on follow mode - the map scrolls opposite to player motion
//
// Passed nothing, returns nothing
//
static void AM_doFollowPlayer(void)
{
    if (f_oldloc.x != am_viewx || f_oldloc.y != am_viewy)
    {
        // e6y
        m_x = FTOM(MTOF(am_viewx >> FRACTOMAPBITS)) - m_w / 2;
        m_y = FTOM(MTOF(am_viewy >> FRACTOMAPBITS)) - m_h / 2;

        m_x2 = m_x + m_w;
        m_y2 = m_y + m_h;
        f_oldloc.x = am_viewx;
        f_oldloc.y = am_viewy;

        //  m_x = FTOM(MTOF(plr->mo->x - m_w / 2));
        //  m_y = FTOM(MTOF(plr->mo->y - m_h / 2));
        //  m_x = plr->mo->x - m_w / 2;
        //  m_y = plr->mo->y - m_h / 2;
    }
}

//
//
//
//
// [nitr8] UNUSED
/*
void AM_updateLightLev(void)
{
    static int nexttic = 0;
   
    // Change light level
    if (amclock > nexttic)
    {
        //static int litelevels[] = { 0, 3, 5, 6, 6, 7, 7, 7 };
        static int litelevels[] = { 0, 4, 7, 10, 12, 14, 15, 15 };
        static int litelevelscnt = 0;

        lightlev = litelevels[litelevelscnt++];

        if (litelevelscnt == arrlen(litelevels))
            litelevelscnt = 0;

        nexttic = amclock + 6 - (amclock % 6);
    }
}
*/

//
// AM_Ticker()
//
// Updates on gametic - enter follow mode, zoom, or change map location
//
// Passed nothing, returns nothing
//
void AM_Ticker(void)
{
    if (!automapactive)
        return;

    amclock++;

    if (followplayer)
        AM_doFollowPlayer();

    // Change the zoom if necessary
    if (ftom_zoommul != FRACUNIT)
        AM_changeWindowScale();

    // Change x,y location
    if ((m_paninc.x || m_paninc.y) && !menuactive && !paused && !consoleactive)
        AM_changeWindowLoc();

    // Update light level
    // AM_updateLightLev();
    if (movement)
    {
        movement = false;
        m_paninc.x = 0;
        m_paninc.y = 0;
        mtof_zoommul = FRACUNIT;
        ftom_zoommul = FRACUNIT;
    }
}

//
// Clear automap frame buffer.
//
static void AM_clearFB(int color)
{
    memset(fb, color, f_w * f_h * sizeof(*fb));
}

//
// Automap clipping of lines.
//
// Based on Cohen-Sutherland clipping algorithm but with a slightly
// faster reject and precalculated slopes.  If the speed is needed,
// use a hash algorithm to handle  the common cases.
//
// Passed the line's coordinates on map and in the frame buffer performs
// clipping on them in the lines frame coordinates.
// Returns true if any part of line was not clipped
//
static dboolean AM_clipMline(mline_t *ml, fline_t *fl)
{
    enum
    {
        LEFT   = 1,
        RIGHT  = 2,
        BOTTOM = 4,
        TOP    = 8
    };
    
    register int outcode1 = 0;
    register int outcode2 = 0;
    register int outside;
    
    // do trivial rejects and outcodes
    if (ml->a.y > m_y2)
        outcode1 = TOP;
    else if (ml->a.y < m_y)
        outcode1 = BOTTOM;

    if (ml->b.y > m_y2)
        outcode2 = TOP;
    else if (ml->b.y < m_y)
        outcode2 = BOTTOM;
    
    if (outcode1 & outcode2)
        // trivially outside
        return false;

    if (ml->a.x < m_x)
        outcode1 |= LEFT;
    else if (ml->a.x > m_x2)
        outcode1 |= RIGHT;
    
    if (ml->b.x < m_x)
        outcode2 |= LEFT;
    else if (ml->b.x > m_x2)
        outcode2 |= RIGHT;
    
    if (outcode1 & outcode2)
        // trivially outside
        return false;

    // transform to frame-buffer coordinates.
    fl->a.x = CXMTOF(ml->a.x);
    fl->a.y = CYMTOF(ml->a.y);
    fl->b.x = CXMTOF(ml->b.x);
    fl->b.y = CYMTOF(ml->b.y);

    DOOUTCODE(outcode1, fl->a.x, fl->a.y);
    DOOUTCODE(outcode2, fl->b.x, fl->b.y);

    if (outcode1 & outcode2)
        return false;

    while (outcode1 | outcode2)
    {
        fpoint_t     tmp;
        int          dx;
        int          dy;
    
        // may be partially inside box
        // find an outside point
        if (outcode1)
            outside = outcode1;
        else
            outside = outcode2;
        
        // clip to each side
        if (outside & TOP)
        {
            dy = fl->a.y - fl->b.y;
            dx = fl->b.x - fl->a.x;
            tmp.x = fl->a.x + (dx * (fl->a.y)) / dy;
            tmp.y = 0;
        }
        else if (outside & BOTTOM)
        {
            dy = fl->a.y - fl->b.y;
            dx = fl->b.x - fl->a.x;
            tmp.x = fl->a.x + (dx * (fl->a.y - f_h)) / dy;
            tmp.y = f_h - 1;
        }
        else if (outside & RIGHT)
        {
            dy = fl->b.y - fl->a.y;
            dx = fl->b.x - fl->a.x;
            tmp.y = fl->a.y + (dy * (f_w - 1 - fl->a.x)) / dx;
            tmp.x = f_w - 1;
        }
        else if (outside & LEFT)
        {
            dy = fl->b.y - fl->a.y;
            dx = fl->b.x - fl->a.x;
            tmp.y = fl->a.y + (dy * (-fl->a.x)) / dx;
            tmp.x = 0;
        }
        else
        {
            tmp.x = 0;
            tmp.y = 0;
        }

        if (outside == outcode1)
        {
            fl->a = tmp;
            DOOUTCODE(outcode1, fl->a.x, fl->a.y);
        }
        else
        {
            fl->b = tmp;
            DOOUTCODE(outcode2, fl->b.x, fl->b.y);
        }
        
        if (outcode1 & outcode2)
            // trivially outside
            return false;
    }

    return true;
}
#undef DOOUTCODE

//
// Classic Bresenham w/ whatever optimizations needed for speed
//
static void AM_drawFline(fline_t *fl, int color)
{
    register int x;
    register int y;
    register int dx;
    register int dy;
    register int sx;
    register int sy;
    register int ax;
    register int ay;
    register int d;
    
    // For debugging only
    if (fl->a.x < 0 || fl->a.x >= f_w ||
        fl->a.y < 0 || fl->a.y >= f_h ||
        fl->b.x < 0 || fl->b.x >= f_w ||
        fl->b.y < 0 || fl->b.y >= f_h)
    {
        static int   fuck = 0;

        C_Error("fuck %d", fuck++);

        return;
    }

    dx = fl->b.x - fl->a.x;
    ax = 2 * (dx < 0 ? -dx : dx);
    sx = dx < 0 ? -1 : 1;

    dy = fl->b.y - fl->a.y;
    ay = 2 * (dy < 0 ? -dy : dy);
    sy = dy < 0 ? -1 : 1;

    x = fl->a.x;
    y = fl->a.y;

    if (ax > ay)
    {
        d = ay - ax / 2;

        while (1)
        {
            PUTDOT(x, y, color);

            if (x == fl->b.x)
                return;

            if (d >= 0)
            {
                y += sy;
                d -= ax;
            }

            x += sx;
            d += ay;
        }
    }
    else
    {
        d = ax - ay / 2;

        while (1)
        {
            PUTDOT(x, y, color);

            if (y == fl->b.y)
                return;

            if (d >= 0)
            {
                x += sx;
                d -= ay;
            }

            y += sy;
            d += ax;
        }
    }
}

//
// AM_drawMline()
//
// Clip lines, draw visible parts of lines.
//
// Passed the map coordinates of the line, and the color to draw it
// Color -1 is special and prevents drawing. Color 247 is special and
// is translated to black, allowing Color 0 to represent feature disable
// in the defaults file.
// Returns nothing.
//
static void AM_drawMline(mline_t *ml, int color)
{
    static fline_t fl;

    // draws it on frame buffer using fb coords
    if (AM_clipMline(ml, &fl))
        AM_drawFline(&fl, color);
}

//
// AM_drawGrid()
//
// Draws blockmap aligned grid lines.
//
// Passed the color to draw the grid lines
// Returns nothing
//
static void AM_drawGrid(int color)
{
    fixed_t x, y;
    fixed_t start, end;
    mline_t ml;
    fixed_t minlen, extx, exty;
    fixed_t minx, miny;
    int gridsize = map_grid_size << MAPBITS;

    // [RH] Calculate a minimum for how long the grid lines should be so that
    // they cover the screen at any rotation.
    minlen = M_DoubleToInt(sqrt((float)m_w * (float)m_w + (float)m_h * (float)m_h));
    extx = (minlen - m_w) / 2;
    exty = (minlen - m_h) / 2;

    minx = m_x;
    miny = m_y;

    // Fix vanilla automap grid bug: losing grid lines near the map boundary
    // due to unnecessary addition of MAPBLOCKUNITS to start
    // Proper math is to just subtract the remainder; AM_drawMLine will take care
    // of clipping if an extra line is offscreen.

    // Figure out start of vertical gridlines
    start = minx - extx;

    if ((start - bmaporgx) % gridsize)
      start -= ((start - bmaporgx) % gridsize);

    end = minx + minlen - extx;

    // draw vertical gridlines
    for (x = start; x < end; x += gridsize)
    {
        ml.a.x = x;
        ml.b.x = x;
        ml.a.y = miny - exty;
        ml.b.y = ml.a.y + minlen;

        if (automapactive && am_rotate)
        {
            AM_rotatePoint(&ml.a);
            AM_rotatePoint(&ml.b);
        }

        AM_drawMline(&ml, color);
    }

    // Figure out start of horizontal gridlines
    start = miny - exty;

    if ((start - bmaporgy) % gridsize)
        start -= ((start - bmaporgy) % gridsize);

    end = miny + minlen - exty;

    // draw horizontal gridlines
    for (y = start; y < end; y += gridsize)
    {
        ml.a.x = minx - extx;
        ml.b.x = ml.a.x + minlen;
        ml.a.y = y;
        ml.b.y = y;

        if (automapactive && am_rotate)
        {
            AM_rotatePoint(&ml.a);
            AM_rotatePoint(&ml.b);
        }

        AM_drawMline(&ml, color);
    }
}

//
// AM_DoorColor()
//
// Returns the 'color' or key needed for a door linedef type
//
// Passed the type of linedef, returns:
//   -1 if not a keyed door
//    0 if a red key required
//    1 if a blue key required
//    2 if a yellow key required
//    3 if a multiple keys required
//
// jff 4/3/98 add routine to get color of generalized keyed door
//
static int AM_DoorColor(int type)
{
    if (GenLockedBase <= type && type < GenDoorBase)
    {
        type -= GenLockedBase;
        type = (type & LockedKey) >> LockedKeyShift;

        if (!type || type == 7)
            // any or all keys
            return 3;
        else
            return (type - 1) % 3;
    }

    // closed keyed door
    switch (type)
    {
        case 26:
        case 32:
        case 99:
        case 133:
            // blue key
            return 1;

        case 27:
        case 34:
        case 136:
        case 137:
            // yellow key
            return 2;

        case 28:
        case 33:
        case 134:
        case 135:
            // red key
            return 0;

        default:
            // not a keyed door
            return -1;
    }
}

//
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//
// jff 1/5/98 many changes in this routine
// backward compatibility not needed, so just changes, no ifs
// addition of clauses for:
//    doors opening, keyed door id, secret sectors,
//    teleports, exit lines, key things
// ability to suppress any of added features or lines with no height changes
//
// support for gamma correction in automap abandoned
//
// jff 4/3/98 changed mapcolor_xxxx=0 as control to disable feature
// jff 4/3/98 changed mapcolor_xxxx=-1 to disable drawing line completely
//
static void AM_drawWalls(void)
{
    int            i;

    for (i = 0; i < numlines; i++)
    {
        static mline_t l;

        l.a.x = lines[i].v1->x >> FRACTOMAPBITS;
        l.a.y = lines[i].v1->y >> FRACTOMAPBITS;
        l.b.x = lines[i].v2->x >> FRACTOMAPBITS;
        l.b.y = lines[i].v2->y >> FRACTOMAPBITS;

        if (automapactive && am_rotate)
        {
            AM_rotatePoint(&l.a);
            AM_rotatePoint(&l.b);
        }

        if (cheating || (lines[i].flags & ML_MAPPED))
        {
            if ((lines[i].flags & LINE_NEVERSEE) && !cheating)
                continue;

            {
                // cph - show keyed doors and lines
                int amd;

                if ((mapcolor_bdor || mapcolor_ydor || mapcolor_rdor) &&
                    // non-secret
                    !(lines[i].flags & ML_SECRET) && (amd = AM_DoorColor(lines[i].special)) != -1)
                {
                    // closed keyed door
                    switch (amd)
                    {
                        case 1:
                            // blue key
                            AM_drawMline(&l, mapcolor_bdor ? mapcolor_bdor : mapcolor_cchg);
                            continue;

                        case 2:
                            // yellow key
                            AM_drawMline(&l, mapcolor_ydor ? mapcolor_ydor : mapcolor_cchg);
                            continue;

                        case 0:
                            // red key
                            AM_drawMline(&l, mapcolor_rdor ? mapcolor_rdor : mapcolor_cchg);
                            continue;

                        case 3:
                            // any or all
                            AM_drawMline(&l, mapcolor_clsd ? mapcolor_clsd : mapcolor_cchg);
                            continue;
                    }
                }
            }

            // jff 4/23/98 add exit lines to automap
            if (mapcolor_exit && (lines[i].special == 11  ||
                                  lines[i].special == 52  ||
                                  lines[i].special == 197 ||
                                  lines[i].special == 51  ||
                                  lines[i].special == 124 ||
                                  lines[i].special == 198))
            {
                // exit line
                AM_drawMline(&l, mapcolor_exit);
                continue;
            }

            if (!lines[i].backsector)
            {
                // jff 1/10/98 add new color for 1S secret sector boundary
                // jff 4/3/98 0 is disable
                if (mapcolor_secr &&
                    ((map_secret_after && P_WasSecret(lines[i].frontsector) && !P_IsSecret(lines[i].frontsector)) ||
                    (!map_secret_after && P_WasSecret(lines[i].frontsector))))
                    // line bounding secret sector
                    AM_drawMline(&l, mapcolor_secr);
                // jff 2/16/98 fixed bug
                else
                    // special was cleared
                    AM_drawMline(&l, mapcolor_wall);
            }
            else
            {
                // jff 1/10/98 add color change for all teleporter types
                if (mapcolor_tele && !(lines[i].flags & ML_SECRET) && (lines[i].special == 39 ||
                                                                       lines[i].special == 97 ||
                                                                       lines[i].special == 125 ||
                                                                       lines[i].special == 126))
                {
                    // teleporters
                    AM_drawMline(&l, mapcolor_tele);
                }
                // secret door
                else if (lines[i].flags & ML_SECRET)
                {
                    if (cheating)
                        AM_drawMline(&l, mapcolor_secr);
                    else
                        // wall color
                        AM_drawMline(&l, mapcolor_wall);
                }
                // non-secret closed door
                else if (mapcolor_clsd && !(lines[i].flags & ML_SECRET) &&
                        ((lines[i].backsector->floorheight == lines[i].backsector->ceilingheight) ||
                        (lines[i].frontsector->floorheight == lines[i].frontsector->ceilingheight)))
                {
                    // non-secret closed door
                    AM_drawMline(&l, mapcolor_clsd);
                }
                // jff 1/6/98 show secret sector 2S lines
                // special was cleared after getting it
                else if (mapcolor_secr &&
                        ((map_secret_after && ((P_WasSecret(lines[i].frontsector) && !P_IsSecret(lines[i].frontsector)) ||
                        (P_WasSecret(lines[i].backsector) && !P_IsSecret(lines[i].backsector)))) ||
                        // jff 3/9/98 add logic to not show secret til after entered
                        //  if map_secret_after is true
                        (!map_secret_after && (P_WasSecret(lines[i].frontsector) ||
                        P_WasSecret(lines[i].backsector)))))
                {
                    // line bounding secret sector
                    AM_drawMline(&l, mapcolor_secr);
                }
                // jff 1/6/98 end secret sector line change
                else if (lines[i].backsector->floorheight != lines[i].frontsector->floorheight)
                {
                    // floor level change
                    AM_drawMline(&l, mapcolor_fchg);
                }
                else if (lines[i].backsector->ceilingheight != lines[i].frontsector->ceilingheight)
                {
                    // ceiling level change
                    AM_drawMline(&l, mapcolor_cchg);
                }
                else if (mapcolor_flat && cheating)
                {
                    //2S lines that appear only in IDDT
                    AM_drawMline(&l, mapcolor_flat);
                }
            }
        }
        else if (plr->powers[pw_allmap])
        {
            // invisible flag lines do not show
            if (!(lines[i].flags & ML_DONTDRAW))
            {
                if (mapcolor_flat ||
                    !lines[i].backsector ||
                    lines[i].backsector->floorheight != lines[i].frontsector->floorheight ||
                    lines[i].backsector->ceilingheight != lines[i].frontsector->ceilingheight)
                    AM_drawMline(&l, mapcolor_unsn);
            }
        }
    }
}

//
// AM_drawLineCharacter()
//
// Draws a vector graphic according to numerous parameters
//
// Passed the structure defining the vector graphic shape, the number
// of vectors in it, the scale to draw it at, the angle to draw it at,
// the color to draw it with, and the map coordinates to draw it at.
// Returns nothing
//
static void AM_drawLineCharacter(mline_t *lineguy, int lineguylines, fixed_t scale, angle_t angle, int color, fixed_t x, fixed_t y)
{
    int          i;

    if (automapactive && am_rotate)
        // cph
        angle -= am_viewangle - ANG90;

    for (i = 0; i < lineguylines; i++)
    {
        mline_t      l;

        l.a.x = lineguy[i].a.x;
        l.a.y = lineguy[i].a.y;

        if (scale)
        {
            l.a.x = FixedMul(scale, l.a.x);
            l.a.y = FixedMul(scale, l.a.y);
        }

        if (angle)
            AM_rotate(&l.a.x, &l.a.y, angle, 0, 0);

        l.a.x += x;
        l.a.y += y;

        l.b.x = lineguy[i].b.x;
        l.b.y = lineguy[i].b.y;

        if (scale)
        {
            l.b.x = FixedMul(scale, l.b.x);
            l.b.y = FixedMul(scale, l.b.y);
        }

        if (angle)
            AM_rotate(&l.b.x, &l.b.y, angle, 0, 0);
        
        l.b.x += x;
        l.b.y += y;

        AM_drawMline(&l, color);
    }
}

//
// AM_drawPlayers()
//
// Draws the player arrow in single player,
// or all the player arrows in a netgame.
//
// Passed nothing, returns nothing
//
static void AM_drawPlayers(void)
{
    int                i;
    angle_t            angle;
    mpoint_t           pt;

    if (!netgame)
    {
        pt.x = am_viewx >> FRACTOMAPBITS;
        pt.y = am_viewy >> FRACTOMAPBITS;

        if (automapactive && am_rotate)
        {
            AM_rotatePoint(&pt);
        }

        if (cheating)
            AM_drawLineCharacter(cheat_player_arrow, arrlen(cheat_player_arrow), 0,
                 am_viewangle, mapcolor_sngl, pt.x, pt.y);
        else
            AM_drawLineCharacter(player_arrow, arrlen(player_arrow), 0, am_viewangle,
                 mapcolor_sngl, pt.x, pt.y);

        return;
    }

    for (i = 0; i < MAXPLAYERS; i++)
    {
        player_t *p = &players[i];

        pt.x = p->mo->x >> FRACTOMAPBITS;
        pt.y = p->mo->y >> FRACTOMAPBITS;
        angle = p->mo->angle;

        if ((deathmatch && !singledemo) && p != plr)
            continue;

        if (!playeringame[i])
            continue;

        if (playeringame[i])
        {
            if (automapactive && am_rotate)
                AM_rotatePoint(&pt);
        }

        AM_drawLineCharacter(player_arrow, arrlen(player_arrow), 0, angle,

            // close to black
            p->powers[pw_invisibility] ? 246 : 

            // jff 1/6/98 use default color
            mapcolor_plyr,

            pt.x, pt.y);
    }
}

//
// AM_drawThings()
//
// Draws the things on the automap in double IDDT cheat mode
//
// Passed colors and colorrange, no longer used
// Returns nothing
//
static void AM_drawThings(void)
{
    int       i;

    for (i = 0; i < numsectors; i++)
    {
        // e6y
        // Two-pass method for better usability of automap:
        // The first one will draw all things except enemies
        // The second one is for enemies only
        // Stop after first pass if the current sector has no enemies
        int     pass;
        int     enemies = 0;

        for (pass = 0; pass < 2; pass += (enemies ? 1 : 2))
        {
            mobj_t *t = sectors[i].thinglist;

            while (t)
            {
                // e6y: stop if all enemies from current sector already have been drawn
                if (pass && !enemies)
                    break;

                if (pass == ((t->flags & (MF_SHOOTABLE | MF_CORPSE)) == MF_SHOOTABLE ?
                    (!pass ? enemies++ : enemies--), 0 : 1))
                {
                    t = t->snext;
                    continue;
                }

                if (!(t->flags2 & MF2_DONOTMAP))
                {
                    mpoint_t p;
                    angle_t  angle = t->angle;

                    p.x = t->x >> FRACTOMAPBITS;
                    p.y = t->y >> FRACTOMAPBITS;

                    if (automapactive && am_rotate)
                        AM_rotatePoint(&p);

                    // jff 1/5/98 case over doomednum of thing being drawn
                    if (mapcolor_rkey || mapcolor_ykey || mapcolor_bkey)
                    {
                        switch (t->info->doomednum)
                        {
                            // jff 1/5/98 treat keys special
                            case 38:

                            // jff red key
                            case 13:
                                AM_drawLineCharacter(cross_mark, NUMCROSSMARKLINES,
                                    16 << MAPBITS,
                                    t->angle, mapcolor_rkey != -1 ? mapcolor_rkey : mapcolor_sprt, p.x, p.y);

                                t = t->snext;
                                continue;

                            case 39:

                            // jff yellow key
                            case 6:
                                AM_drawLineCharacter(cross_mark, NUMCROSSMARKLINES,
                                    16 << MAPBITS,
                                    t->angle, mapcolor_ykey != -1 ? mapcolor_ykey : mapcolor_sprt, p.x, p.y);

                                t = t->snext;
                                continue;

                            case 40:

                            // jff blue key
                            case 5:
                                AM_drawLineCharacter(cross_mark, NUMCROSSMARKLINES,
                                    16 << MAPBITS,
                                    t->angle, mapcolor_bkey != -1 ? mapcolor_bkey : mapcolor_sprt, p.x, p.y);

                                t = t->snext;
                                continue;

                            default:
                                break;
                        }
                    }

                    AM_drawLineCharacter(thintriangle_guy, arrlen(thintriangle_guy),

                            // e6y
                            16 << MAPBITS,

                            angle,

                            // cph 2006/07/30 - Show count-as-kills in red.
                            ((t->flags & (MF_COUNTKILL | MF_CORPSE)) == MF_COUNTKILL) ? mapcolor_enemy :

                            // bbm 2/28/03 Show countable items in yellow.
                            (t->flags & MF_COUNTITEM) ? mapcolor_item : mapcolor_sprt, p.x, p.y);

                }

                t = t->snext;
            }
        }
    }
}

//
// AM_drawMarks()
//
// Draw the marked locations on the automap
//
// Passed nothing, returns nothing
//
// killough 2/22/98:
// Rewrote AM_drawMarks(). Removed limit on marks.
//
static void AM_drawMarks(void)
{
    int i;
    char namebuf[16] = "AMMNUM0";

    // killough 2/22/98: remove automap mark limit
    for (i = 0; i < markpointnum; i++)
    {
        if (markpoints[i].x != -1)
        {
            int w;
            int k;
            mpoint_t p;
      
            // - m_x + prev_m_x;
            p.x = markpoints[i].x;

            // - m_y + prev_m_y;
            p.y = markpoints[i].y;

            //      w = SHORT(marknums[i]->width);
            //      h = SHORT(marknums[i]->height);
            // because something's wrong with the wad, i guess

            if (automapactive && am_rotate)
                AM_rotatePoint(&p);

            p.x = CXMTOF(p.x) - markpoints[i].w * SCREENWIDTH / 320 / 2;
            p.y = CYMTOF(p.y) - markpoints[i].h * SCREENHEIGHT / 200 / 2;

            if (p.y < f_y + f_w && p.y >= f_x)
            {
                w = 0;

                for (k = 0; k < (int)strlen(markpoints[i].label); k++)
                {
                    namebuf[6] = markpoints[i].label[k];

                    if (p.x < f_x + f_w &&
                        p.x + markpoints[i].widths[k] * SCREENWIDTH / 320 >= f_x)
                    {
                        V_DrawPatch(p.x * 320 / SCREENWIDTH, p.y * 200 / SCREENHEIGHT,
                            0, W_CacheLumpName(namebuf, PU_CACHE));
                    }

                    w += markpoints[i].widths[k] + 1;
                    p.x += w * SCREENWIDTH / 320;
                }
            }
        }
    }
}

//
// AM_drawCrosshair()
//
// Draw the single point crosshair representing map center
//
// Passed the color to draw the pixel with
// Returns nothing
//
static void AM_drawCrosshair(int color)
{
    // single point for now
    fb[(f_w * (f_h + 1)) / 2] = color;
}

void AM_Toggle(void)
{
    if (gamestate != GS_LEVEL)
        return;

    if (!automapactive)
    {
        AM_Start();

        if (overlay_trigger)
        {
            oldscreenblocks = screenblocks;
            oldscreenSize = screenSize;
            screenblocks = 11;
            screenSize = 8;
            R_SetViewSize(screenblocks);
            am_overlay = true;
        }
        else
            am_overlay = false;
    }
    else
    {
        if (overlay_trigger && am_overlay)
        {
            screenSize = oldscreenSize;
            screenblocks = oldscreenblocks;
            R_SetViewSize(screenblocks);
            am_overlay = false;
        }
        else
            AM_Stop();
    }
}

//
// AM_Drawer()
//
// Draws the entire automap
//
// Passed nothing, returns nothing
//
void AM_Drawer(void)
{
    if (!automapactive)
        return;

    am_viewangle = viewangle;
    am_viewx = viewx;
    am_viewy = viewy;

    if (automapactive && followplayer)
    {
        m_x = (am_viewx >> FRACTOMAPBITS) - m_w / 2;
        m_y = (am_viewy >> FRACTOMAPBITS) - m_h / 2;
    }

    // [crispy] automap-overlay
    if (!am_overlay)
        AM_clearFB(mapcolor_back);

    if (drawgrid)
        AM_drawGrid(mapcolor_grid);

    AM_drawWalls();
    AM_drawPlayers();

    if (cheating == 2)
        AM_drawThings();

    AM_drawCrosshair(mapcolor_hair);

    AM_drawMarks();

    //V_MarkRect(f_x, f_y, f_w, f_h);
}

void AM_DrawWorldTimer(void)
{
    int days;
    int hours;
    int minutes;
    int seconds;
    int worldTimer = players[consoleplayer].worldTimer;

    worldTimer /= 35;
    days = worldTimer / 86400;
    worldTimer -= days * 86400;
    hours = worldTimer / 3600;
    worldTimer -= hours * 3600;
    minutes = worldTimer / 60;
    worldTimer -= minutes * 60;
    seconds = worldTimer;

    if (gamestate == GS_LEVEL && automapactive)
    {
        if (!d_statusmap || am_overlay)
        {
            int x;
            char timeBuffer[15];

            sprintf(timeBuffer, "%.2d:%.2d:%.2d", hours, minutes, seconds);

            x = ORIGINALWIDTH / 2 - M_StringWidth(timeBuffer) / 2;
            M_WriteText(x, 172, timeBuffer);
        }
    }

    if (days)
    {
        char dayBuffer[20];

        if (days == 1)
        {
            sprintf(dayBuffer, "%.2d DAY", days);
        }
        else
        {
            sprintf(dayBuffer, "%.2d DAYS", days);
        }

        M_WriteText(240, 20, dayBuffer);

        if (days >= 5)
        {
            M_WriteText(230, 35, "YOU FREAK!!!");
        }
    }
}

