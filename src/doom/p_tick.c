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
//        Archiving: SaveGame I/O.
//        Thinker, Ticker.
//
//-----------------------------------------------------------------------------


#ifdef WII
#include "../c_io.h"
#else
#include "c_io.h"
#endif

#include "doomstat.h"
#include "p_local.h"

#ifdef WII
#include "../z_zone.h"
#else
#include "z_zone.h"
#endif


//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//


int              leveltime;

// Both the head and tail of the thinker list.
thinker_t        thinkercap;


//
// P_InitThinkers
//
void P_InitThinkers (void)
{
    thinkercap.prev = thinkercap.next  = &thinkercap;
}



//
// P_AddThinker
// Adds a new thinker at the end of the list.
//
void P_AddThinker (thinker_t* thinker)
{
    thinkercap.prev->next = thinker;
    thinker->next = &thinkercap;
    thinker->prev = thinkercap.prev;
    thinkercap.prev = thinker;

    thinker->references = 0;    // killough 11/98: init reference counter to 0
}



//
// P_RemoveThinker
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
void P_RemoveThinker (thinker_t* thinker)
{
    // FIXME: NOP.
    thinker->function.acv = (actionf_v)(-1);
}



//
// P_AllocateThinker
// Allocates memory and adds a new thinker at the end of the list.
//
void P_AllocateThinker (thinker_t*        thinker)
{
}



//
// P_RunThinkers
//
void P_RunThinkers (void)
{
    thinker_t *currentthinker, *nextthinker;

    currentthinker = thinkercap.next;
    while (currentthinker != &thinkercap)
    {
        nextthinker = currentthinker->next;

        if ( currentthinker->function.acv == (actionf_v)(-1) )
        {
            // time to remove it
            currentthinker->next->prev = currentthinker->prev;
            currentthinker->prev->next = currentthinker->next;
            Z_Free (currentthinker);
        }
        else
        {
            if (currentthinker->function.acp1)
                currentthinker->function.acp1 (currentthinker);
        }
        currentthinker = nextthinker;
    }
}



//
// P_Ticker
//

void P_Ticker (void)
{
    int         i;
    
    // run the tic
    if (paused || menuactive /*|| consoleactive*/)
        return;
                
    // pause if in menu and at least one tic has been run
    if ( !netgame
         && menuactive
         && !demoplayback
         && players[consoleplayer].viewz != 1)
    {
        return;
    }
    
                
    for (i=0 ; i<MAXPLAYERS ; i++)
        if (playeringame[i])
            P_PlayerThink (&players[i]);
                        
    P_RunThinkers ();
    P_UpdateSpecials ();
    P_RespawnSpecials ();

    // [RH] Apply falling damage
    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            P_FallingDamage (players[i].mo);

            players[i].oldvelocity[0] = players[i].mo->momx;
            players[i].oldvelocity[1] = players[i].mo->momy;
            players[i].oldvelocity[2] = players[i].mo->momz;
        }
    }

    // for par times
    leveltime++;        
}

//
// P_SetTarget
//
// This function is used to keep track of pointer references to mobj thinkers.
// In Doom, objects such as lost souls could sometimes be removed despite
// their still being referenced. In Boom, 'target' mobj fields were tested
// during each gametic, and any objects pointed to by them would be prevented
// from being removed. But this was incomplete, and was slow (every mobj was
// checked during every gametic). Now, we keep a count of the number of
// references, and delay removal until the count is 0.
//
void P_SetTarget(mobj_t **mop, mobj_t *targ)
{
    if (*mop)           // If there was a target already, decrease its refcount
        (*mop)->thinker.references--;
    if ((*mop = targ))  // Set new target and if non-NULL, increase its counter
        targ->thinker.references++;
}

