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
//        Status bar code.
//        Does the face/direction indicator animatin.
//        Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------


#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "d_event.h"
#include "doomtype.h"


// Size of statusbar.
// Now sensitive for scaling.
#define ST_HEIGHT                     32

#define ST_WIDTH                      ORIGWIDTH
#define ST_Y                          (ORIGHEIGHT - ST_HEIGHT)

#define ST_X_COORD                    10 * SCREENSCALE / 2
#define ST_Y_COORD                    311 * SCREENSCALE / 2 + SBARHEIGHT

#define ST_HEALTH_X                   ST_X_COORD
#define ST_HEALTH_Y                   ST_Y_COORD - 8
#define ST_HEALTH_MIN                 20
#define ST_HEALTH_WAIT                8
#define ST_HEALTH_HIGHLIGHT_WAIT      6

#define ST_AMMO_X                     (ST_X_COORD + 100 * SCREENSCALE / 2)
#define ST_AMMO_Y                     ST_HEALTH_Y
#define ST_AMMO_MIN                   20
#define ST_AMMO_WAIT                  8
#define ST_AMMO_HIGHLIGHT_WAIT        6

#define ST_KEYS_X                     (SCREENWIDTH - ST_X_COORD - 128 * SCREENSCALE / 2)
#define ST_KEYS_Y                     ST_HEALTH_Y

#define ST_ARMOR_X                    (SCREENWIDTH - ST_X_COORD)
#define ST_ARMOR_Y                    ST_HEALTH_Y
#define ST_ARMOR_HIGHLIGHT_WAIT       6

#define ST_KEY_WAIT                   8


//
// STATUS BAR
//

// Called by main loop.
boolean ST_Responder (event_t* ev);

// Called by main loop.
void ST_Ticker (void);

// Called by main loop.
void ST_Drawer (boolean fullscreen, boolean refresh);

// Called when the console player is spawned on each level.
void ST_Start (void);

// Called by startup code.
void ST_Init (void);

void ST_DrawStatus(void);

void ST_doRefresh(void);

void ST_DrawSoundInfo(void);

// States for status bar code.
typedef enum
{
    AutomapState,
    FirstPersonState
    
} st_stateenum_t;


// States for the chat code.
typedef enum
{
    StartChatState,
    WaitDestState,
    GetChatState
    
} st_chatstateenum_t;


extern int      healthhighlight;
extern int      ammohighlight;
extern int      armorhighlight;

#endif
