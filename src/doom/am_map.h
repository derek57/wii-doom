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

#ifdef WII
#include "../d_event.h"
#include "../m_cheat.h"
#include "../m_fixed.h"
#else
#include "d_event.h"
#include "m_cheat.h"
#include "m_fixed.h"
#endif


// Used by ST StatusBar stuff.
#define AM_MSGHEADER (('a'<<24)+('m'<<16))
#define AM_MSGENTERED (AM_MSGHEADER | ('e'<<8))
#define AM_MSGEXITED (AM_MSGHEADER | ('x'<<8))

#define AM_PANDOWNKEY   0xaf
#define AM_PANUPKEY     KEY_UPARROW
#define AM_PANRIGHTKEY  KEY_RIGHTARROW
#define AM_PANLEFTKEY   KEY_LEFTARROW
#define AM_ZOOMINKEY    '/'
#define AM_ZOOMOUTKEY   ' '
#define AM_ENDKEY       KEY_TAB


typedef struct
{
    fixed_t x,y;
} mpoint_t;


// Called by main loop.
boolean AM_Responder (event_t* ev);

extern cheatseq_t cheat_amap;

// Called by main loop.
void AM_Ticker (void);

// Called by main loop,
// called instead of view drawer if automap active.
void AM_Drawer (void);

// Called to force the automap to quit
// if the level is completed while it is up.
void AM_Stop (void);

void AM_Start (void);

void AM_DrawWorldTimer(void);

void AM_Toggle (void);

#endif
