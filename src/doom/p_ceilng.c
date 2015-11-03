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
// DESCRIPTION:  Ceiling aninmation (lowering, crushing, raising)
//
//-----------------------------------------------------------------------------



#include "doomdef.h"

// State.
#include "doomstat.h"

#include "malloc.h"
#include "p_local.h"
#include "r_state.h"

// Data.
#include "sounds.h"

#include "s_sound.h"

#ifdef WII
#include "../z_zone.h"
#else
#include "z_zone.h"
#endif


//
// CEILINGS
//


// the list of ceilings moving currently, including crushers
ceilinglist_t   *activeceilings;


extern boolean canmodify;


//
// T_MoveCeiling
//
void T_MoveCeiling (ceiling_t* ceiling)
{
    result_e        res;
        
    switch(ceiling->direction)
    {
      case 0:
        // IN STASIS
        break;
      case 1:
        // UP
        res = T_MovePlane(ceiling->sector,
                          ceiling->speed,
                          ceiling->topheight,
                          false,1,ceiling->direction);
        
        if (!(leveltime & 7) && ceiling->sector->ceilingheight != ceiling->topheight)
        {
            switch(ceiling->type)
            {
              case silentCrushAndRaise:
              case genSilentCrusher:
                break;

              default:
                S_StartSectorSound(&ceiling->sector->soundorg, sfx_stnmov);
                break;
            }
        }
        
        if (res == pastdest)
        {
            switch(ceiling->type)
            {
              case raiseToHighest:
              case genCeiling:
                P_RemoveActiveCeiling(ceiling);
                break;
                
              // movers with texture change, change the texture then get removed
              case genCeilingChgT:
              case genCeilingChg0:
                ceiling->sector->special = ceiling->newspecial;

              case genCeilingChg:
                ceiling->sector->ceilingpic = ceiling->texture;
                P_RemoveActiveCeiling(ceiling);
                break;

              case silentCrushAndRaise:
                S_StartSectorSound(&ceiling->sector->soundorg, sfx_pstop);
              case genSilentCrusher:
              case genCrusher:
              case fastCrushAndRaise:
              case crushAndRaise:
                ceiling->direction = -1;
                break;
                
              default:
                break;
            }
            
        }
        break;
        
      case -1:
        // DOWN
        res = T_MovePlane(ceiling->sector,
                          ceiling->speed,
                          ceiling->bottomheight,
                          ceiling->crush,1,ceiling->direction);
        
        if (!(leveltime & 7) && ceiling->sector->ceilingheight != ceiling->bottomheight)
        {
            switch(ceiling->type)
            {
              case silentCrushAndRaise: 
              case genSilentCrusher:
                break;

              default:
                S_StartSectorSound(&ceiling->sector->soundorg, sfx_stnmov);
                break;
            }
        }
        
        if (res == pastdest)
        {
            switch(ceiling->type)
            {
              // 02/09/98 jff change slow crushers' speed back to normal
              // start back up
              case genSilentCrusher:
              case genCrusher:
                if (ceiling->oldspeed < CEILSPEED * 3)
                    ceiling->speed = ceiling->oldspeed;
                ceiling->direction = 1; // jff 2/22/98 make it go back up!
                break;

              case silentCrushAndRaise:
                S_StartSectorSound(&ceiling->sector->soundorg, sfx_pstop);
              case crushAndRaise:
                ceiling->speed = CEILSPEED;
              case fastCrushAndRaise:
                ceiling->direction = 1;
                break;

              // in the case of ceiling mover/changer, change the texture
              // then remove the active ceiling
              case genCeilingChgT:
              case genCeilingChg0:
                ceiling->sector->special = ceiling->newspecial;

              case genCeilingChg:
                ceiling->sector->ceilingpic = ceiling->texture;
                P_RemoveActiveCeiling(ceiling);
                break;

              case lowerAndCrush:
              case lowerToFloor:
              case lowerToLowest:
              case lowerToMaxFloor:
              case genCeiling:
                P_RemoveActiveCeiling(ceiling);
                break;

              default:
                break;
            }
        }
        else // ( res != pastdest )
        {
            if (res == crushed)
            {
                switch(ceiling->type)
                {
                  // jff 02/08/98 slow down slow crushers on obstacle
                  case genCrusher:
                  case genSilentCrusher:
                    if (ceiling->oldspeed < CEILSPEED * 3)
                        ceiling->speed = CEILSPEED / 8;
                    break;

                  case silentCrushAndRaise:
                  case crushAndRaise:
                  case lowerAndCrush:
                    ceiling->speed = CEILSPEED / 8;
                    break;

                  default:
                    break;
                }
            }
        }
        break;
    }
}


//
// EV_DoCeiling
// Move a ceiling up/down and all around!
//
int
EV_DoCeiling
( line_t*       line,
  ceiling_e     type )
{
    int         secnum = -1;
    boolean     rtn = false;
    sector_t*   sec;
    ceiling_t*  ceiling;
        
    // Reactivate in-stasis ceilings...for certain types.
    switch(type)
    {
      case fastCrushAndRaise:
      case silentCrushAndRaise:
      case crushAndRaise:
        rtn = P_ActivateInStasisCeiling(line);
      default:
        break;
    }
        
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];
        if (P_SectorActive(ceiling_special, sec))
            continue;
        
        // new door thinker
        rtn = true;
        ceiling = Z_Malloc (sizeof(*ceiling), PU_LEVSPEC, 0);
        memset(ceiling, 0, sizeof(*ceiling));
        P_AddThinker (&ceiling->thinker);
        sec->ceilingdata = ceiling;
        ceiling->thinker.function = T_MoveCeiling;
        ceiling->sector = sec;
        ceiling->crush = false;
        
        switch(type)
        {
          case fastCrushAndRaise:
            ceiling->crush = true;
            ceiling->topheight = sec->ceilingheight;
            ceiling->bottomheight = sec->floorheight + (8*FRACUNIT);
            ceiling->direction = -1;
            ceiling->speed = CEILSPEED * 2;
            break;

          case silentCrushAndRaise:
          case crushAndRaise:
            ceiling->crush = true;
            ceiling->topheight = sec->ceilingheight;
          case lowerAndCrush:
          case lowerToFloor:
            ceiling->bottomheight = sec->floorheight;
            if (type != lowerToFloor && !(gamemission == doom2 && gamemap == 4 && canmodify))
                ceiling->bottomheight += 8 * FRACUNIT;
            ceiling->direction = -1;
            ceiling->speed = CEILSPEED;
            break;

          case raiseToHighest:
            ceiling->topheight = P_FindHighestCeilingSurrounding(sec);
            ceiling->direction = 1;
            ceiling->speed = CEILSPEED;
            break;

          case lowerToLowest:
            ceiling->bottomheight = P_FindLowestCeilingSurrounding(sec);
            ceiling->direction = -1;
            ceiling->speed = CEILSPEED;
            break;

          case lowerToMaxFloor:
            ceiling->bottomheight = P_FindHighestFloorSurrounding(sec);
            ceiling->direction = -1;
            ceiling->speed = CEILSPEED;
            break;

          default:
            break;
        }
                
        ceiling->tag = sec->tag;
        ceiling->type = type;
        P_AddActiveCeiling(ceiling);
    }
    return rtn;
}


//
// P_AddActiveCeiling
// Add an active ceiling
//
void P_AddActiveCeiling(ceiling_t *ceiling)
{
    ceilinglist_t       *list;

    list = malloc(sizeof(*list));
    list->ceiling = ceiling;
    ceiling->list = list;
    if ((list->next = activeceilings))
        list->next->prev = &list->next;
    list->prev = &activeceilings;
    activeceilings = list;
}

//
// P_RemoveActiveCeiling
// Remove a ceiling's thinker
//
void P_RemoveActiveCeiling(ceiling_t *ceiling)
{
    ceilinglist_t       *list = ceiling->list;

    ceiling->sector->ceilingdata = NULL;
    P_RemoveThinker(&ceiling->thinker);
    if ((*list->prev = list->next))
        list->next->prev = list->prev;
    free(list);
}

//
// P_RemoveAllActiveCeilings()
// Removes all ceilings from the active ceiling list
//
void P_RemoveAllActiveCeilings(void)
{
    while (activeceilings)
    {
        ceilinglist_t   *next = activeceilings->next;

        free(activeceilings);
        activeceilings = next;
    }
}

//
// P_ActivateInStasisCeiling
// Restart a ceiling that's in-stasis
//
boolean P_ActivateInStasisCeiling(line_t *line)
{
    boolean             result = false;
    ceilinglist_t       *list;

    for (list = activeceilings; list; list = list->next)
    {
        ceiling_t       *ceiling = list->ceiling;

        if (ceiling->tag == line->tag && !ceiling->direction)
        {
            ceiling->direction = ceiling->olddirection;
            ceiling->thinker.function = T_MoveCeiling;
            result = true;
        }
    }
    return result;
}

//
// EV_CeilingCrushStop
// Stop a ceiling from crushing!
//
boolean EV_CeilingCrushStop(line_t *line)
{
    boolean             result = false;
    ceilinglist_t       *list;

    for (list = activeceilings; list; list = list->next)
    {
        ceiling_t       *ceiling = list->ceiling;

        if (ceiling->direction && ceiling->tag == line->tag)
        {
            ceiling->olddirection = ceiling->direction;
            ceiling->direction = 0;
            ceiling->thinker.function = NULL;
            result = true;
        }
    }
    return result;
}
