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
//  AutoMap module.
//
//-----------------------------------------------------------------------------


#ifndef __AMMAP_H__
#define __AMMAP_H__


#include "d_event.h"
#include "m_cheat.h"
#include "m_fixed.h"


// Used by ST StatusBar stuff.
#define AM_MSGHEADER     (('a' << 24) + ('m' << 16))
#define AM_MSGENTERED    (AM_MSGHEADER | ('e' << 8))
#define AM_MSGEXITED     (AM_MSGHEADER | ('x' << 8))

#define AM_PANDOWNKEY    0xaf
#define AM_PANUPKEY      KEY_UPARROW
#define AM_PANRIGHTKEY   KEY_RIGHTARROW
#define AM_PANLEFTKEY    KEY_LEFTARROW
#define AM_ZOOMINKEY     '/'
#define AM_ZOOMOUTKEY    ' '
#define AM_ENDKEY        KEY_TAB

// For use if I do walls with outsides/insides
#define REDS             (256 - 5 * 16)
#define REDRANGE         16
#define BLUES            (256 - 4 * 16 + 8)
#define BLUERANGE        8
#define GREENS           (7 * 16)
#define GREENRANGE       16
#define GRAYS            (6 * 16)
#define GRAYSRANGE       16
#define BROWNS           (4 * 16)
#define BROWNRANGE       16
#define YELLOWS          (256 - 32 + 7)
#define YELLOWRANGE      1
#define BLACK            0
#define WHITE            (256 - 47)

// Automap colors
#define BACKGROUND       BLACK
#define YOURCOLORS       WHITE
#define YOURRANGE        0
#define WALLCOLORS       REDS
#define WALLRANGE        REDRANGE
#define TSWALLCOLORS     GRAYS
#define TSWALLRANGE      GRAYSRANGE
#define FDWALLCOLORS     BROWNS
#define FDWALLRANGE      BROWNRANGE
#define CDWALLCOLORS     YELLOWS
#define CDWALLRANGE      YELLOWRANGE
#define THINGCOLORS      GREENS
#define THINGRANGE       GREENRANGE
#define SECRETWALLCOLORS WALLCOLORS
#define SECRETWALLRANGE  WALLRANGE
#define GRIDCOLORS       (GRAYS + GRAYSRANGE / 2)
#define GRIDRANGE        0
#define XHAIRCOLORS      GRAYS


typedef struct
{
    fixed_t x;
    fixed_t y;

} mpoint_t;

typedef struct
{
   fixed_t x, y;
   fixed_t w, h;

   char label[16];
   int widths[16];

} markpoint_t;


// Called by main loop.
void AM_Ticker(void);

// Called by main loop,
// called instead of view drawer if automap active.
void AM_Drawer(void);

// Called to force the automap to quit
// if the level is completed while it is up.
void AM_Stop(void);

void AM_Start(void);
void AM_DrawWorldTimer(void);
void AM_Toggle(void);

// Called by main loop.
dboolean AM_Responder(event_t *ev);


extern cheatseq_t  cheat_amap;

extern int         lightlev;

extern markpoint_t *markpoints;

#endif

