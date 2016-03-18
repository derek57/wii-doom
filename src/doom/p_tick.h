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
//        ?
//
//-----------------------------------------------------------------------------


#ifndef __P_TICK__
#define __P_TICK__


// killough 8/29/98: threads of thinkers, for more efficient searches
// cph 2002/01/13: for consistency with the main thinker list, keep objects
// pending deletion on a class list too
typedef enum
{
    th_delete,
    th_mobj,
    th_misc,

    NUMTHCLASS,

    // For P_NextThinker, indicates "any class"
    th_all = NUMTHCLASS

} th_class;


extern thinker_t        thinkerclasscap[];


#define thinkercap      thinkerclasscap[th_all]


// Called by C_Ticker,
// can call G_PlayerExited.
// Carries out all thinking of monsters and players.
void P_Ticker(void);
void P_SetTarget(mobj_t **mop, mobj_t *targ);
void P_UpdateThinker(thinker_t *thinker);
void P_RemoveThinkerDelayed(thinker_t *thinker);

#endif

