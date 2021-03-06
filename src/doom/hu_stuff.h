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
// DESCRIPTION:  Head up display
//
//-----------------------------------------------------------------------------


#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__


#include "d_event.h"
#include "v_patch.h"


//
// Globally visible constants.
//

// the first font characters
#define HU_FONTSTART                  '!'

// the last font characters
#define HU_FONTEND                    '_'

#define HU_FONTENDBETA                '}'

// Calculate # of glyphs in font.
#define HU_FONTSIZE                   (HU_FONTEND - HU_FONTSTART + 1)        
#define HU_FONTSIZEBETA               (HU_FONTENDBETA - HU_FONTSTART + 1)        

#define HU_BROADCAST                  5

#define HU_MSGX                       0
#define HU_MSGY                       0

// in characters
#define HU_MSGWIDTH                   64

// in lines
#define HU_MSGHEIGHT                  1

#define HU_MSGTIMEOUT                 (4 * TICRATE)

#define HUD_X                         11 * SCREENSCALE / 2
#define HUD_Y                         311 * SCREENSCALE / 2 + SBARHEIGHT

#define HUD_HEALTH_X                  HUD_X
#define HUD_HEALTH_Y                  HUD_Y
#define HUD_HEALTH_MIN                20
#define HUD_HEALTH_WAIT               250
#define HUD_HEALTH_HIGHLIGHT_WAIT     250

#define HUD_AMMO_X                    (HUD_X + 100 * SCREENSCALE / 2)
#define HUD_AMMO_Y                    HUD_HEALTH_Y
#define HUD_AMMO_MIN                  20
#define HUD_AMMO_WAIT                 250
#define HUD_AMMO_HIGHLIGHT_WAIT       250

#define HUD_KEYS_X                    (SCREENWIDTH - HUD_X - 128 * SCREENSCALE / 2)
#define HUD_KEYS_Y                    HUD_HEALTH_Y

#define HUD_ARMOR_X                   (SCREENWIDTH - HUD_X)
#define HUD_ARMOR_Y                   HUD_HEALTH_Y
#define HUD_ARMOR_HIGHLIGHT_WAIT      250

#define HUD_KEY_WAIT                  250

#define playername_default            "you"


//
// HEADS UP TEXT
//

void HU_Init(void);
void HU_Start(void);
void HU_Ticker(void);
void HU_Drawer(void);
void HU_Erase(void);
void HU_DrawStats(void);
void HU_DrawHUD(void);
void HU_PlayerMessage(char *message, dboolean ingame);
void HU_DemoProgressBar(int scrn);

// HU_NewLevel called when we enter a new level
// determine the level name and display it in
// the console

void HU_NewLevel();

dboolean HU_Responder(event_t *ev);

char HU_dequeueChatChar(void);


extern char     *chat_macros[10];
extern char     *playername;

extern patch_t  *hu_font[HU_FONTSIZE];

#endif

