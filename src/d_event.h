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
//
//    
//-----------------------------------------------------------------------------


#ifndef __D_EVENT__
#define __D_EVENT__


#include "doomtype.h"


//
// Event handling.
//

// Input event types.
typedef enum
{
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_joystick,
    ev_quit,
    ev_mousewheel

} evtype_t;

// Event structure.
typedef struct
{
    evtype_t            type;

    // keys / mouse/joystick buttons
    int                 data1;

    // mouse/joystick x move
    int                 data2;

    // mouse/joystick y move
    int                 data3;

    // wii ir x
    int                 data4;

    // wii ir y
    int                 data5;

} event_t;
 
//
// Button/action code definitions.
//
typedef enum
{
    // Press "Fire".
    BT_ATTACK           = 1,
    // Use button, to open doors, activate switches.
    BT_USE              = 2,

    // Flag: game events, not really buttons.
    BT_SPECIAL          = 128,
    BT_SPECIALMASK      = 3,
    
    // Flag, weapon change pending.
    // If true, the next 3 bits hold weapon num.
    BT_CHANGE           = 4,
    // The 3bit weapon mask and shift, convenience.
    BT_WEAPONMASK       = (8 + 16 + 32),
    BT_WEAPONSHIFT      = 3,

    // Pause the game.
    BTS_PAUSE           = 1,
    // Save the game at each console.
    BTS_SAVEGAME        = 2,

    // Savegame slot numbers
    //  occupy the second byte of buttons.    
    BTS_SAVEMASK        = (4 + 8 + 16),
    BTS_SAVESHIFT       = 2
  
} buttoncode_t;


// Called by IO functions when input is detected.
void D_PostEvent(event_t *ev);

// Read an event from the event queue

event_t *D_PopEvent(void);


#endif

