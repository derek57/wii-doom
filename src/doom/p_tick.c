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


#include "c_io.h"
#include "doomstat.h"
#include "p_local.h"
#include "p_tick.h"
#include "z_zone.h"


// killough 8/29/98: we maintain several separate threads, each containing
// a special class of thinkers, to allow more efficient searches.
thinker_t        thinkerclasscap[th_all + 1];
thinker_t        *currentthinker;


//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//

int              leveltime;


//
// P_InitThinkers
//
void P_InitThinkers(void)
{
    int i;

    // killough 8/29/98: initialize threaded lists
    for (i = 0; i < NUMTHCLASS; i++)
        thinkerclasscap[i].cprev = thinkerclasscap[i].cnext = &thinkerclasscap[i];

    thinkercap.prev = thinkercap.next  = &thinkercap;
}

//
// P_AddThinker
// Adds a new thinker at the end of the list.
//
void P_AddThinker(thinker_t *thinker)
{
    thinkercap.prev->next = thinker;
    thinker->next = &thinkercap;
    thinker->prev = thinkercap.prev;
    thinkercap.prev = thinker;

    // killough 11/98: init reference counter to 0
    thinker->references = 0;

    // killough 8/29/98: set sentinel pointers, and then add to appropriate list
    thinker->cnext = thinker->cprev = NULL;
    P_UpdateThinker(thinker);
}

//
// killough 11/98:
//
// Make currentthinker external, so that P_RemoveThinkerDelayed
// can adjust currentthinker when thinkers self-remove.

//
// P_RemoveThinkerDelayed()
//
// Called automatically as part of the thinker loop in P_RunThinkers(),
// on nodes which are pending deletion.
//
// If this thinker has no more pointers referencing it indirectly,
// remove it, and set currentthinker to one node preceding it, so
// that the next step in P_RunThinkers() will get its successor.
//
void P_RemoveThinkerDelayed(thinker_t *thinker)
{
    if (!thinker->references)
    {
        thinker_t   *next = thinker->next;
        thinker_t   *th = thinker->cnext;

        // Remove from main thinker list
        // Note that currentthinker is guaranteed to point to us,
        // and since we're freeing our memory, we had better change that. So
        // point it to thinker->prev, so the iterator will correctly move on to
        // thinker->prev->next = thinker->next
        (next->prev = currentthinker = thinker->prev)->next = next;

        // Remove from current thinker class list
        (th->cprev = thinker->cprev)->cnext = th;

        Z_Free(thinker);
    }
}

//
// killough 8/29/98:
//
// We maintain separate threads of friends and enemies, to permit more
// efficient searches.
//
void P_UpdateThinker(thinker_t *thinker)
{
    thinker_t   *th;

    // find the class the thinker belongs to
    int class = (thinker->function == P_RemoveThinkerDelayed ? th_delete :
        (thinker->function == P_MobjThinker ? th_mobj : th_misc));

    // Remove from current thread, if in one
    if ((th = thinker->cnext))
        (th->cprev = thinker->cprev)->cnext = th;

    // Add to appropriate thread
    th = &thinkerclasscap[class];
    th->cprev->cnext = thinker;
    thinker->cnext = th;
    thinker->cprev = th->cprev;
    th->cprev = thinker;
}

//
// P_RemoveThinker
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
// killough 4/25/98:
//
// Instead of marking the function with -1 value cast to a function pointer,
// set the function to P_RemoveThinkerDelayed(), so that later, it will be
// removed automatically as part of the thinker process.
//
void P_RemoveThinker(thinker_t *thinker)
{
    thinker->function = P_RemoveThinkerDelayed;

    P_UpdateThinker(thinker);
}

//
// P_AllocateThinker
// Allocates memory and adds a new thinker at the end of the list.
//
// [nitr8] UNUSED
//
/*
void P_AllocateThinker(thinker_t *thinker)
{
}
*/

//
// P_RunThinkers
//
void P_RunThinkers(void)
{
//    thinker_t *currentthinker, *nextthinker;

    currentthinker = thinkercap.next;

    while (currentthinker != &thinkercap)
    {
/*
        nextthinker = currentthinker->next;

        if (currentthinker->function == NULL)
        {
            // time to remove it
            currentthinker->next->prev = currentthinker->prev;
            currentthinker->prev->next = currentthinker->next;
            Z_Free(currentthinker);
        }
        else
        {
*/
            if (currentthinker->function)
                currentthinker->function(currentthinker);
/*
        }

        currentthinker = nextthinker;
*/
        currentthinker = currentthinker->next;
    }
}

//
// P_Ticker
//
void P_Ticker(void)
{
    int         i;
    
    // run the tic
    if ((paused || menuactive) && !beta_style)
        return;

    // pause if in menu and at least one tic has been run
    if (!netgame && menuactive && !demoplayback
        && players[consoleplayer].viewz != 1 && !beta_style)
    {
        return;
    }

    // haleyjd: think for particles
    P_ParticleThinker();

    for (i = 0; i < MAXPLAYERS; i++)
        if (playeringame[i])
            P_PlayerThink(&players[i]);

    P_RunThinkers();
    P_UpdateSpecials();
    P_RespawnSpecials();

    P_MapEnd();

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

    // haleyjd: run particle effects
    P_RunEffects();
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
    if (*mop)
        // If there was a target already, decrease its refcount
        (*mop)->thinker.references--;

    if ((*mop = targ))
        // Set new target and if non-NULL, increase its counter
        targ->thinker.references++;
}

