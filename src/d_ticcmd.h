// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
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


#ifndef __D_TICCMD__
#define __D_TICCMD__


#include "doomtype.h"


// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.

typedef struct
{
    // *2048 for move
    signed char        forwardmov;

    // *2048 for move
    signed char        sidemov;

    // <<16 for angle delta
    short              angleturn;

    byte               chatchar;
    byte               buttons;

    // villsa [STRIFE] according to the asm,
    // consistancy is a short, not a byte

    // checks for net game
    byte               consistancy;

    // villsa - Strife specific:
    byte               buttons2;
    int                inventory;
   
    // Heretic/Hexen specific:
    // look or fly up / down / centering
    byte               lookfly;

    // artitype_t to use
    byte               arti;

    int                lookdir;

} ticcmd_t;


#endif

