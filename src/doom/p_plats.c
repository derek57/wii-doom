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
//        Plats (i.e. elevator platforms) code, raising/lowering.
//
//-----------------------------------------------------------------------------


#include <malloc.h>
#include <stdio.h>

#include "doomdef.h"

// State.
#include "doomstat.h"

#ifdef WII
#include "../i_system.h"
#else
#include "i_system.h"
#endif

#include "m_random.h"
#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"

// Data.
#include "sounds.h"

#ifdef WII
#include "../z_zone.h"
#else
#include "z_zone.h"
#endif


platlist_t *activeplats;        // killough 2/14/98: made global again


//
// Move a plat up and down
//
void T_PlatRaise(plat_t* plat)
{
    result_e        res;
        
    switch(plat->status)
    {
      case up:
        res = T_MovePlane(plat->sector,
                          plat->speed,
                          plat->high,
                          plat->crush,0,1);
                                        
        if (plat->type == raiseAndChange
            || plat->type == raiseToNearestAndChange)
        {
            if (!(leveltime&7))
                S_StartSound(&plat->sector->soundorg, sfx_stnmov);
        }
        
                                
        if (res == crushed && (!plat->crush))
        {
            plat->count = plat->wait;
            plat->status = down;
            S_StartSound(&plat->sector->soundorg, sfx_pstart);
        }
        else
        {
            if (res == pastdest)
            {
                plat->count = plat->wait;
                plat->status = waiting;
                S_StartSound(&plat->sector->soundorg, sfx_pstop);

                switch(plat->type)
                {
                  case blazeDWUS:
                  case downWaitUpStay:
                    P_RemoveActivePlat(plat);
                    break;
                    
                  case raiseAndChange:
                  case raiseToNearestAndChange:
                    P_RemoveActivePlat(plat);
                    break;
                    
                  default:
                    break;
                }
            }
        }
        break;
        
      case        down:
        res = T_MovePlane(plat->sector,plat->speed,plat->low,false,0,-1);

        if (res == pastdest)
        {
            plat->count = plat->wait;
            plat->status = waiting;
            S_StartSound(&plat->sector->soundorg,sfx_pstop);

            // remove the plat if its a pure raise type
            if (!d_floors)
            {
                switch(plat->type)
                {
                  case raiseAndChange:
                  case raiseToNearestAndChange:
                    P_RemoveActivePlat(plat);
                  default:
                    break;
                }
            }
        }
        break;
        
      case        waiting:
        if (!--plat->count)
        {
            if (plat->sector->floor_height == plat->low)
                plat->status = up;
            else
                plat->status = down;
            S_StartSound(&plat->sector->soundorg,sfx_pstart);
        }
      case        in_stasis:
        break;
    }
}


//
// Do Platforms
//  "amount" is only used for SOME platforms.
//
int
EV_DoPlat
( line_t*        line,
  plattype_e     type,
  int            amount )
{
    plat_t*      plat;
    int          secnum;
    int          rtn;
    sector_t*    sec;
        
    secnum = -1;
    rtn = 0;

    
    //        Activate all <type> plats that are in_stasis
    switch(type)
    {
      case perpetualRaise:
        P_ActivateInStasis(line->tag);
        break;
        
      default:
        break;
    }
        
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];

        if (sec->specialdata)
            continue;
        
        // Find lowest & highest floors around sector
        rtn = 1;
        plat = Z_Malloc( sizeof(*plat), PU_LEVSPEC, 0);
        P_AddThinker(&plat->thinker);
                
        plat->type = type;
        plat->sector = sec;
        plat->sector->specialdata = plat;
        plat->thinker.function.acp1 = (actionf_p1) T_PlatRaise;
        plat->crush = false;
        plat->tag = line->tag;
        
        switch(type)
        {
          case raiseToNearestAndChange:
            plat->speed = PLATSPEED/2;
            sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
            plat->high = P_FindNextHighestFloor(sec,sec->floor_height);
            plat->wait = 0;
            plat->status = up;
            // NO MORE DAMAGE, IF APPLICABLE
            sec->special = 0;                

            S_StartSound(&sec->soundorg,sfx_stnmov);
            break;
            
          case raiseAndChange:
            plat->speed = PLATSPEED/2;
            sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
            plat->high = sec->floor_height + amount*FRACUNIT;
            plat->wait = 0;
            plat->status = up;

            S_StartSound(&sec->soundorg,sfx_stnmov);
            break;
            
          case downWaitUpStay:
            if(beta_style)
                plat->speed = PLATSPEED;
            else
                plat->speed = PLATSPEED * 4;
            plat->low = P_FindLowestFloorSurrounding(sec);

            if (plat->low > sec->floor_height)
                plat->low = sec->floor_height;

            plat->high = sec->floor_height;
            plat->wait = TICRATE*PLATWAIT;
            plat->status = down;
            S_StartSound(&sec->soundorg,sfx_pstart);
            break;
            
          case blazeDWUS:
            plat->speed = PLATSPEED * 8;
            plat->low = P_FindLowestFloorSurrounding(sec);

            if (plat->low > sec->floor_height)
                plat->low = sec->floor_height;

            plat->high = sec->floor_height;
            plat->wait = TICRATE*PLATWAIT;
            plat->status = down;
            S_StartSound(&sec->soundorg,sfx_pstart);
            break;
            
          case perpetualRaise:
            plat->speed = PLATSPEED;
            plat->low = P_FindLowestFloorSurrounding(sec);

            if (plat->low > sec->floor_height)
                plat->low = sec->floor_height;

            plat->high = P_FindHighestFloorSurrounding(sec);

            if (plat->high < sec->floor_height)
                plat->high = sec->floor_height;

            plat->wait = TICRATE*PLATWAIT;
            plat->status = P_Random()&1;

            S_StartSound(&sec->soundorg,sfx_pstart);
            break;
        }
        P_AddActivePlat(plat);
    }
    return rtn;
}



// The following were all rewritten by Lee Killough
// to use the new structure which places no limits
// on active plats. It also avoids spending as much
// time searching for active plats. Previously a
// fixed-size array was used, with NULL indicating
// empty entries, while now a doubly-linked list
// is used.

//
// P_ActivateInStasis()
// Activate a plat that has been put in stasis
// (stopped perpetual floor, instant floor/ceil toggle)
//
void P_ActivateInStasis(int tag)
{
    platlist_t  *platlist;

    for (platlist = activeplats; platlist; platlist = platlist->next)   // search the active plats
    {
        plat_t  *plat = platlist->plat;                         // for one in stasis with right tag

        if (plat->tag == tag && plat->status == in_stasis)
        {
            plat->status = plat->oldstatus;
            plat->thinker.function.acp1 = (actionf_p1) T_PlatRaise;
        }
    }
}

//
// EV_StopPlat()
// Handler for "stop perpetual floor" linedef type
//
boolean EV_StopPlat(line_t *line)
{
    platlist_t  *platlist;

    for (platlist = activeplats; platlist; platlist = platlist->next)   // search the active plats
    {
        plat_t  *plat = platlist->plat;                         // for one with the tag not in stasis

        if (plat->status != in_stasis && plat->tag == line->tag)
        {
            plat->oldstatus = plat->status;                     // put it in stasis
            plat->status = in_stasis;
            plat->thinker.function.acp1 = NULL;
        }
    }
    return true;
}

//
// P_AddActivePlat()
// Add a plat to the head of the active plat list
//
void P_AddActivePlat(plat_t *plat)
{
    platlist_t  *list = malloc(sizeof(*list));

    list->plat = plat;
    plat->list = list;
    if ((list->next = activeplats))
        list->next->prev = &list->next;
    list->prev = &activeplats;
    activeplats = list;
}

//
// P_RemoveActivePlat()
// Remove a plat from the active plat list
//
void P_RemoveActivePlat(plat_t *plat)
{
    platlist_t  *list = plat->list;

    plat->sector->specialdata = NULL;
    P_RemoveThinker(&plat->thinker);
    if ((*list->prev = list->next))
        list->next->prev = list->prev;
    free(list);
}

//
// P_RemoveAllActivePlats()
// Remove all plats from the active plat list
//
void P_RemoveAllActivePlats(void)
{
    while (activeplats)
    {
        platlist_t      *next = activeplats->next;

        free(activeplats);
        activeplats = next;
    }
}
