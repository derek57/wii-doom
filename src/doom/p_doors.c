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
// DESCRIPTION: Door animation code (opening/closing)
//
//-----------------------------------------------------------------------------


#include "c_io.h"

// Data.
#include "dstrings.h"

#include "d_deh.h"
#include "doomdef.h"

// State.
#include "doomstat.h"
#include "hu_stuff.h"

#include "i_system.h"
#include "m_misc.h"
#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"
#include "sounds.h"
#include "v_trans.h"
#include "z_zone.h"


#if 0
//
// Sliding door frame information
//
slidename_t        slideFrameNames[MAXSLIDEDOORS] =
{
    {"GDOORF1","GDOORF2","GDOORF3","GDOORF4",        // front
     "GDOORB1","GDOORB2","GDOORB3","GDOORB4"},       // back
         
    {"\0","\0","\0","\0"}
};
#endif


//
// VERTICAL DOORS
//

//
// T_VerticalDoor
//
void T_VerticalDoor (vldoor_t* door)
{
    result_e        res;
        
    switch(door->direction)
    {
      case 0:
        // WAITING
        if (!--door->topcountdown)
        {
            switch(door->type)
            {
              case doorBlazeRaise:
              case genBlazeRaise:
                door->direction = -1; // time to go back down

                if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                        fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
                    S_StartSectorSound(&door->sector->soundorg, sfx_bdcls);
                break;
                
              case doorNormal:
              case genRaise:
                door->direction = -1; // time to go back down
                S_StartSectorSound(&door->sector->soundorg, sfx_dorcls);
                break;
                
              case doorClose30ThenOpen:
              case genCdO:
                door->direction = 1;
                S_StartSectorSound(&door->sector->soundorg, sfx_doropn);
                break;
                
              case genBlazeCdO:
                door->direction = 1;    // time to go back up
                S_StartSectorSound(&door->sector->soundorg, sfx_bdopn);
                break;

              default:
                break;
            }
        }
        break;
        
      case 2:
        //  INITIAL WAIT
        if (!--door->topcountdown)
        {
            switch(door->type)
            {
              case doorRaiseIn5Mins:
                door->direction = 1;
                door->type = doorNormal;
                S_StartSectorSound(&door->sector->soundorg, sfx_doropn);
                break;
                
              default:
                break;
            }
        }
        break;
        
      case -1:
        // DOWN
        res = T_MovePlane(door->sector,
                          door->speed,
                          door->sector->floorheight,
                          false,1,door->direction);

        // killough 10/98: implement gradual lighting effects
        // [BH] enhanced to apply effects to all doors
        if (door->topheight - door->sector->floorheight)
        {
            fixed_t level = FixedDiv(door->sector->ceilingheight - door->sector->floorheight,
                door->topheight - door->sector->floorheight);

            if (door->lighttag)
                EV_LightTurnOnPartway(door->line, level);
            else if (!P_SectorHasLightSpecial(door->sector)) 
                EV_LightByAdjacentSectors(door->sector, level);
        }

        if (res == pastdest)
        {
            switch(door->type)
            {
              case doorBlazeRaise:
              case doorBlazeClose:
              case genBlazeRaise:
              case genBlazeClose:
                door->sector->ceilingdata = NULL;
                P_RemoveThinker (&door->thinker);  // unlink and free

                if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                        fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
                {
                    // killough 4/15/98: remove double-closing sound of blazing doors
                    if (!d_blazingsound)
                        S_StartSectorSound(&door->sector->soundorg, sfx_bdcls);
                }
                break;
                
              case doorNormal:
              case doorClose:
              case genRaise:
              case genClose:
                door->sector->ceilingdata = NULL;
                P_RemoveThinker (&door->thinker);  // unlink and free
                break;
                
              case doorClose30ThenOpen:
                door->direction = 0;
                door->topcountdown = TICRATE*30;
                break;
                
              case genCdO:
              case genBlazeCdO:
                door->direction = 0;
                door->topcountdown = door->topwait;     // jff 5/8/98 insert delay
                break;

              default:
                break;
            }
        }
        else if (res == crushed)
        {
            switch(door->type)
            {
              case doorBlazeClose:
              case doorClose:                // DO NOT GO BACK UP!
              case genClose:
              case genBlazeClose:
                break;
                
              case doorBlazeRaise:
                door->direction = 1;
                S_StartSectorSound(&door->sector->soundorg, sfx_bdopn);
                break;

              default:
                door->direction = 1;
                S_StartSectorSound(&door->sector->soundorg, sfx_doropn);
                break;
            }
        }
        break;
        
      case 1:
        // UP
        res = T_MovePlane(door->sector,
                          door->speed,
                          door->topheight,
                          false,1,door->direction);
        
        // killough 10/98: implement gradual lighting effects
        // [BH] enhanced to apply effects to all doors
        if (door->topheight - door->sector->floorheight)
        {
            fixed_t level = FixedDiv(door->sector->ceilingheight - door->sector->floorheight,
                door->topheight - door->sector->floorheight);

            if (door->lighttag)
                EV_LightTurnOnPartway(door->line, level);
            else if (!P_SectorHasLightSpecial(door->sector)) 
                EV_LightByAdjacentSectors(door->sector, level);
        }

        if (res == pastdest)
        {
            switch(door->type)
            {
              case doorBlazeRaise:
              case doorNormal:
              case genRaise:
              case genBlazeRaise:
                door->direction = 0; // wait at top
                door->topcountdown = door->topwait;
                break;
                
              case doorClose30ThenOpen:
              case doorBlazeOpen:
              case doorOpen:
              case genBlazeOpen:
              case genOpen:
              case genCdO:
              case genBlazeCdO:
                door->sector->ceilingdata = NULL;
                P_RemoveThinker (&door->thinker);  // unlink and free
                break;
                
              default:
                break;
            }
        }
        break;
    }
}


//
// EV_DoLockedDoor
// Move a locked door up/down
//

int
EV_DoLockedDoor
( line_t*        line,
  vldoor_e       type,
  mobj_t*        thing )
{
    player_t*    p;
    static char  buffer[1024];
        
    p = thing->player;
        
    if (!p)
        return 0;
                
    switch(line->special)
    {
      case SR_Door_Blue_OpenStay_Fast:
      case S1_Door_Blue_OpenStay_Fast:
        if ( !p )
            return 0;
        if (p->cards[it_bluecard] <= 0 && p->cards[it_blueskull] <= 0)
        {
            // [BH] display player message distinguishing between keycard and skull key
            // [BH] flash needed key on hud
            if (p->cards[it_bluecard] == CARDNOTFOUNDYET)
            {
                if (hud && (!p->neededcardflash || p->neededcard != it_bluecard))
                {
                    p->neededcard = it_bluecard;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_BLUEO, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "keycard");
                HU_PlayerMessage(buffer, true);
            }
            else if (p->cards[it_blueskull] == CARDNOTFOUNDYET)
            {
                if (hud && (!p->neededcardflash || p->neededcard != it_blueskull))
                {
                    p->neededcard = it_blueskull;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_BLUEO, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "skull key");
                HU_PlayerMessage(buffer, true);
            }
            //  [BH] use sfx_noway instead of sfx_oof
            S_StartSound(p->mo, sfx_noway);
            return 0;
        }
        break;
        
      case SR_Door_Red_OpenStay_Fast:
      case S1_Door_Red_OpenStay_Fast:
        if ( !p )
            return 0;
        if (p->cards[it_redcard] <= 0 && p->cards[it_redskull] <= 0)
        {
            if (p->cards[it_redcard] == CARDNOTFOUNDYET)
            {
                if (hud && (!p->neededcardflash || p->neededcard != it_redcard))
                {
                    p->neededcard = it_redcard;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_REDO, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "keycard");
                HU_PlayerMessage(buffer, true);
            }
            else if (p->cards[it_redskull] == CARDNOTFOUNDYET)
            {
                if (hud && (!p->neededcardflash || p->neededcard != it_redskull))
                {
                    p->neededcard = it_redskull;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_REDO, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "skull key");
                HU_PlayerMessage(buffer, true);
            }
            //  [BH] use sfx_noway instead of sfx_oof
            S_StartSound(p->mo, sfx_noway);
            return 0;
        }
        break;
        
      case SR_Door_Yellow_OpenStay_Fast:
      case S1_Door_Yellow_OpenStay_Fast:
        if ( !p )
            return 0;
        if (p->cards[it_yellowcard] <= 0 && p->cards[it_yellowskull] <= 0)
        {
            if (p->cards[it_yellowcard] == CARDNOTFOUNDYET)
            {
                if (hud && (!p->neededcardflash || p->neededcard != it_yellowcard))
                {
                    p->neededcard = it_yellowcard;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_YELLOWO, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "keycard");
                HU_PlayerMessage(buffer, true);
            }
            else if (p->cards[it_yellowskull] == CARDNOTFOUNDYET)
            {
                if (hud && (!p->neededcardflash || p->neededcard != it_yellowskull))
                {
                    p->neededcard = it_yellowskull;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_YELLOWO, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "skull key");
                HU_PlayerMessage(buffer, true);
            }
            //  [BH] use sfx_noway instead of sfx_oof
            S_StartSound(p->mo, sfx_noway);
            return 0;
        }
        break;        
    }

    return EV_DoDoor(line,type);
}


int
EV_DoDoor
( line_t*        line,
  vldoor_e       type )
{
    int          secnum, rtn, i;
    sector_t*    sec;
    vldoor_t*    door;
        
    secnum = -1;
    rtn = 0;
    
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];
        if (P_SectorActive(ceiling_special, sec))
            continue;

        // new door thinker
        rtn = 1;
        door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
        P_AddThinker (&door->thinker);
        sec->ceilingdata = door;

        door->thinker.function = T_VerticalDoor;
        door->sector = sec;
        door->type = type;
        door->topwait = VDOORWAIT;
        door->speed = VDOORSPEED;
        door->line = line;      // jff 1/31/98 remember line that triggered us
        door->lighttag = 0;
                
        for (i = 0; i < door->sector->linecount; i++)
            door->sector->lines[i]->flags &= ~ML_SECRET;

        switch(type)
        {
          case doorBlazeClose:
            door->topheight = P_FindLowestCeilingSurrounding(sec);
            door->topheight -= 4*FRACUNIT;
            door->direction = -1;
            door->speed = VDOORSPEED * 4;

            if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                    fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
                S_StartSectorSound(&door->sector->soundorg, sfx_bdcls);
            break;
            
          case doorClose:
            door->topheight = P_FindLowestCeilingSurrounding(sec);
            door->topheight -= 4*FRACUNIT;
            door->direction = -1;
            S_StartSectorSound(&door->sector->soundorg, sfx_dorcls);
            break;
            
          case doorClose30ThenOpen:
            door->topheight = sec->ceilingheight;
            door->direction = -1;
            S_StartSectorSound(&door->sector->soundorg, sfx_dorcls);
            break;
            
          case doorBlazeRaise:
          case doorBlazeOpen:
            door->direction = 1;
            door->topheight = P_FindLowestCeilingSurrounding(sec);
            door->topheight -= 4*FRACUNIT;
            door->speed = VDOORSPEED * 4;
            if (door->topheight != sec->ceilingheight)
            {
                if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                        fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
                    S_StartSectorSound(&door->sector->soundorg, sfx_bdopn);
            }
            break;
            
          case doorNormal:
          case doorOpen:
            door->direction = 1;
            door->topheight = P_FindLowestCeilingSurrounding(sec);
            door->topheight -= 4*FRACUNIT;
            if (door->topheight != sec->ceilingheight)
                S_StartSectorSound(&door->sector->soundorg, sfx_doropn);
            break;
            
          default:
            break;
        }
                
    }
    return rtn;
}


//
// EV_VerticalDoor : open a door manually, no tag value
//
void
EV_VerticalDoor
( line_t*        line,
  mobj_t*        thing )
{
    player_t*    player;
    sector_t*    sec;
    vldoor_t*    door;
    int          i;
    static char  buffer[1024];
/*
    int          side;
        
    side = 0;        // only front sides can be used
*/
    //        Check for locks
    player = thing->player;
                
    switch(line->special)
    {
      case DR_Door_Blue_OpenWaitClose:
      case D1_Door_Blue_OpenStay:
        if ( !player )
            return;
        
        if (player->cards[it_bluecard] <= 0 && player->cards[it_blueskull] <= 0)
        {
            // [BH] display player message distinguishing between keycard and skull key
            // [BH] flash needed key on hud
            if (player->cards[it_bluecard] == CARDNOTFOUNDYET)
            {
                if (hud && (!player->neededcardflash || player->neededcard != it_bluecard))
                {
                    player->neededcard = it_bluecard;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_BLUEK, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "keycard");
                HU_PlayerMessage(buffer, true);
            }
            else if (player->cards[it_blueskull] == CARDNOTFOUNDYET)
            {
                if (hud && (!player->neededcardflash || player->neededcard != it_blueskull))
                {
                    player->neededcard = it_blueskull;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_BLUEK, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "skull key");
                HU_PlayerMessage(buffer, true);
            }
            //  [BH] use sfx_noway instead of sfx_oof
            S_StartSound(player->mo, sfx_noway);
            return;
        }
        break;
        
      case DR_Door_Yellow_OpenWaitClose:
      case D1_Door_Yellow_OpenStay:
        if ( !player )
            return;
        
        if (player->cards[it_yellowcard] <= 0 && player->cards[it_yellowskull] <= 0)
        {
            if (player->cards[it_yellowcard] == CARDNOTFOUNDYET)
            {
                if (hud && (!player->neededcardflash || player->neededcard != it_yellowcard))
                {
                    player->neededcard = it_yellowcard;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_YELLOWK, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "keycard");
                HU_PlayerMessage(buffer, true);
            }
            else if (player->cards[it_yellowskull] == CARDNOTFOUNDYET)
            {
                if (hud && (!player->neededcardflash || player->neededcard != it_yellowskull))
                {
                    player->neededcard = it_yellowskull;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_YELLOWK, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "skull key");
                HU_PlayerMessage(buffer, true);
            }
            //  [BH] use sfx_noway instead of sfx_oof
            S_StartSound(player->mo, sfx_noway);
            return;
        }
        break;
        
      case DR_Door_Red_OpenWaitClose:
      case D1_Door_Red_OpenStay:
        if ( !player )
            return;
        
        if (player->cards[it_redcard] <= 0 && player->cards[it_redskull] <= 0)
        {
            if (player->cards[it_redcard] == CARDNOTFOUNDYET)
            {
                if (hud && (!player->neededcardflash || player->neededcard != it_redcard))
                {
                    player->neededcard = it_redcard;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                M_snprintf(buffer, sizeof(buffer), s_PD_REDK, playername,
                    (M_StringCompare(playername, playername_default) ? "" : "s"), "keycard");
                HU_PlayerMessage(buffer, true);
            }
            else if (player->cards[it_redskull] == CARDNOTFOUNDYET)
            {
                if (hud && (!player->neededcardflash || player->neededcard != it_redskull))
                {
                    player->neededcard = it_redskull;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
               M_snprintf(buffer, sizeof(buffer), s_PD_REDK, playername,
                   (M_StringCompare(playername, playername_default) ? "" : "s"), "skull key");
               HU_PlayerMessage(buffer, true);
            }
            //  [BH] use sfx_noway instead of sfx_oof
            S_StartSound(player->mo, sfx_noway);
            return;
        }
        break;
    }
        
    // if the wrong side of door is pushed, give oof sound
    if (line->sidenum[1] == NO_INDEX)           // killough
    {
        // [BH] use sfx_noway instead of sfx_oof
        // [crispy] do not crash if the wrong side of the door is pushed
        C_Error("EV_VerticalDoor: DR special type on 1-sided linedef");
        S_StartSound(player->mo, sfx_noway);    // killough 3/20/98
        return;
    }

    sec = sides[line->sidenum[1]].sector;

    if (sec->ceilingdata)
    {
        door = sec->ceilingdata;
        switch(line->special)
        {
          case DR_Door_OpenWaitClose_AlsoMonsters:
          case DR_Door_Blue_OpenWaitClose:
          case DR_Door_Yellow_OpenWaitClose:
          case DR_Door_Red_OpenWaitClose:
          case DR_Door_OpenWaitClose_Fast:
            if (door->direction == -1)
            {
                door->direction = 1;        // go back up

                // [BH] play correct door sound
                if (door->type == doorBlazeRaise)
                    S_StartSectorSound(&door->sector->soundorg, sfx_bdcls);
                else
                    S_StartSectorSound(&door->sector->soundorg, sfx_dorcls);
            }
            else
            {
                if (!thing->player)
                    return;                // JDC: bad guys never close doors

                // When is a door not a door?
                // In Vanilla, door->direction is set, even though
                // "specialdata" might not actually point at a door.

                if (door->thinker.function == T_VerticalDoor)
                {
                    door->direction = -1;        // start going down immediately

                    // [BH] play correct door sound
                    if (door->type == doorBlazeRaise)
                        S_StartSectorSound(&door->sector->soundorg, sfx_bdcls);
                    else
                        S_StartSectorSound(&door->sector->soundorg, sfx_dorcls);
                }
                else if (door->thinker.function == T_PlatRaise)
                {
                    // Erm, this is a plat, not a door.
                    // This notably causes a problem in ep1-0500.lmp where
                    // a plat and a door are cross-referenced; the door
                    // doesn't open on 64-bit.
                    // The direction field in vldoor_t corresponds to the wait
                    // field in plat_t.  Let's set that to -1 instead.

                    plat_t *plat;

                    plat = (plat_t *) door;
                    plat->wait = -1;
                }
                else
                {
                    // This isn't a door OR a plat.  Now we're in trouble.

                    C_Warning("EV_VerticalDoor: Tried to close "
                                    "something that wasn't a door.");

                    // Try closing it anyway. At least it will work on 32-bit
                    // machines.

                    door->direction = -1;
                }
            }
            return;
        }
    }
        
    // for proper sound
    switch(line->special)
    {
      case DR_Door_OpenWaitClose_Fast:
      case D1_Door_OpenStay_Fast:

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
            S_StartSectorSound(&sec->soundorg,sfx_bdopn);
        break;
        
      default:        // LOCKED DOOR SOUND
        S_StartSectorSound(&sec->soundorg,sfx_doropn);
        break;
    }
        
    
    // new door thinker
    door = Z_Calloc(1, sizeof(*door), PU_LEVSPEC, 0); 
    P_AddThinker (&door->thinker);
    sec->ceilingdata = door;
    door->thinker.function = T_VerticalDoor;
    door->sector = sec;
    door->direction = 1;
    door->speed = VDOORSPEED;
    door->topwait = VDOORWAIT;
    door->line = line;          // jff 1/31/98 remember line that triggered us

    // killough 10/98: use gradual lighting changes if nonzero tag given
    // [BH] check if tag is valid
    door->lighttag = (P_FindLineFromLineTag(line, 0) ? line->tag : 0);  // killough 10/98

    switch(line->special)
    {
      case DR_Door_OpenWaitClose_AlsoMonsters:
      case DR_Door_Blue_OpenWaitClose:
      case DR_Door_Yellow_OpenWaitClose:
      case DR_Door_Red_OpenWaitClose:
        door->type = doorNormal;
        break;
        
      case D1_Door_OpenStay:
      case D1_Door_Blue_OpenStay:
      case D1_Door_Red_OpenStay:
      case D1_Door_Yellow_OpenStay:
        door->type = doorOpen;
        line->special = 0;
        break;
        
      case DR_Door_OpenWaitClose_Fast:
        door->type = doorBlazeRaise;
        door->speed = VDOORSPEED*4;
        break;

      case D1_Door_OpenStay_Fast:
        door->type = doorBlazeOpen;
        line->special = 0;
        door->speed = VDOORSPEED*4;
        break;

      default:
        door->lighttag = 0; // killough 10/98
        break;
    }
    
    // find the top and bottom of the movement range
    door->topheight = P_FindLowestCeilingSurrounding(sec) - 4 * FRACUNIT;

    // [BH] door is no longer secret 
    for (i = 0; i < sec->linecount; i++)
        sec->lines[i]->flags &= ~ML_SECRET;
}


//
// Spawn a door that closes after 30 seconds
//
void P_SpawnDoorCloseIn30 (sector_t* sec)
{
    vldoor_t    *door = Z_Calloc(1, sizeof(*door), PU_LEVSPEC, 0); 

    P_AddThinker (&door->thinker);

    sec->ceilingdata = door;
    sec->special = 0;

    door->thinker.function = T_VerticalDoor;
    door->sector = sec;
    door->direction = 0;
    door->type = doorNormal;
    door->speed = VDOORSPEED;
    door->topcountdown = 30 * TICRATE;
    door->line = NULL;          // jff 1/31/98 remember line that triggered us
    door->lighttag = 0;         // killough 10/98: no lighting changes
}

//
// Spawn a door that opens after 5 minutes
//
void
P_SpawnDoorRaiseIn5Mins
( sector_t*        sec)
{
    vldoor_t    *door = Z_Calloc(1, sizeof(*door), PU_LEVSPEC, 0); 
    
    P_AddThinker (&door->thinker);

    sec->ceilingdata = door;
    sec->special = 0;

    door->thinker.function = T_VerticalDoor;
    door->sector = sec;
    door->direction = 2;
    door->type = doorRaiseIn5Mins;
    door->speed = VDOORSPEED;
    door->topheight = P_FindLowestCeilingSurrounding(sec);
    door->topheight -= 4*FRACUNIT;
    door->topwait = VDOORWAIT;
    door->topcountdown = 5 * 60 * TICRATE;
    door->line = NULL; // jff 1/31/98 remember line that triggered us
    door->lighttag = 0;  // killough 10/98: no lighting changes
}



// UNUSED
// Separate into p_slidoor.c?

#if 0                // ABANDONED TO THE MISTS OF TIME!!!
//
// EV_SlidingDoor : slide a door horizontally
// (animate midtexture, then set noblocking line)
//


slideframe_t slideFrames[MAXSLIDEDOORS];

void P_InitSlidingDoorFrames(void)
{
    int                i;
    int                f1;
    int                f2;
    int                f3;
    int                f4;
        
    // DOOM II ONLY...
    if ( gamemode != commercial)
        return;
        
    for (i = 0;i < MAXSLIDEDOORS; i++)
    {
        if (!slideFrameNames[i].frontFrame1[0])
            break;
                        
        f1 = R_TextureNumForName(slideFrameNames[i].frontFrame1);
        f2 = R_TextureNumForName(slideFrameNames[i].frontFrame2);
        f3 = R_TextureNumForName(slideFrameNames[i].frontFrame3);
        f4 = R_TextureNumForName(slideFrameNames[i].frontFrame4);

        slideFrames[i].frontFrames[0] = f1;
        slideFrames[i].frontFrames[1] = f2;
        slideFrames[i].frontFrames[2] = f3;
        slideFrames[i].frontFrames[3] = f4;
                
        f1 = R_TextureNumForName(slideFrameNames[i].backFrame1);
        f2 = R_TextureNumForName(slideFrameNames[i].backFrame2);
        f3 = R_TextureNumForName(slideFrameNames[i].backFrame3);
        f4 = R_TextureNumForName(slideFrameNames[i].backFrame4);

        slideFrames[i].backFrames[0] = f1;
        slideFrames[i].backFrames[1] = f2;
        slideFrames[i].backFrames[2] = f3;
        slideFrames[i].backFrames[3] = f4;
    }
}


//
// Return index into "slideFrames" array
// for which door type to use
//
int P_FindSlidingDoorType(line_t*        line)
{
    int                i;
    int                val;
        
    for (i = 0;i < MAXSLIDEDOORS;i++)
    {
        val = sides[line->sidenum[0]].midtexture;
        if (val == slideFrames[i].frontFrames[0])
            return i;
    }
        
    return -1;
}

void T_SlidingDoor (slidedoor_t*        door)
{
    switch(door->status)
    {
      case sd_opening:
        if (!door->timer--)
        {
            if (++door->frame == SNUMFRAMES)
            {
                // IF DOOR IS DONE OPENING...
                sides[door->line->sidenum[0]].midtexture = 0;
                sides[door->line->sidenum[1]].midtexture = 0;
                door->line->flags &= ML_BLOCKING^0xff;
                                        
                if (door->type == sdt_openOnly)
                {
                    door->frontsector->specialdata = NULL;
                    P_RemoveThinker (&door->thinker);
                    break;
                }
                                        
                door->timer = SDOORWAIT;
                door->status = sd_waiting;
            }
            else
            {
                // IF DOOR NEEDS TO ANIMATE TO NEXT FRAME...
                door->timer = SWAITTICS;
                                        
                sides[door->line->sidenum[0]].midtexture =
                    slideFrames[door->whichDoorIndex].
                    frontFrames[door->frame];
                sides[door->line->sidenum[1]].midtexture =
                    slideFrames[door->whichDoorIndex].
                    backFrames[door->frame];
            }
        }
        break;
                        
      case sd_waiting:
        // IF DOOR IS DONE WAITING...
        if (!door->timer--)
        {
            // CAN DOOR CLOSE?
            if (door->frontsector->thinglist != NULL ||
                door->backsector->thinglist != NULL)
            {
                door->timer = SDOORWAIT;
                break;
            }

            //door->frame = SNUMFRAMES-1;
            door->status = sd_closing;
            door->timer = SWAITTICS;
        }
        break;
                        
      case sd_closing:
        if (!door->timer--)
        {
            if (--door->frame < 0)
            {
                // IF DOOR IS DONE CLOSING...
                door->line->flags |= ML_BLOCKING;
                door->frontsector->specialdata = NULL;
                P_RemoveThinker (&door->thinker);
                break;
            }
            else
            {
                // IF DOOR NEEDS TO ANIMATE TO NEXT FRAME...
                door->timer = SWAITTICS;
                                        
                sides[door->line->sidenum[0]].midtexture =
                    slideFrames[door->whichDoorIndex].
                    frontFrames[door->frame];
                sides[door->line->sidenum[1]].midtexture =
                    slideFrames[door->whichDoorIndex].
                    backFrames[door->frame];
            }
        }
        break;
    }
}



void
EV_SlidingDoor
( line_t*        line,
  mobj_t*        thing )
{
    sector_t*    sec;
    slidedoor_t* door;
        
    // DOOM II ONLY...
    if (gamemode != commercial)
        return;
    
    // Make sure door isn't already being animated
    sec = line->frontsector;
    door = NULL;
    if (sec->specialdata)
    {
        if (!thing->player)
            return;
                        
        door = sec->specialdata;
        if (door->type == sdt_openAndClose)
        {
            if (door->status == sd_waiting)
                door->status = sd_closing;
        }
        else
            return;
    }
    
    // Init sliding door vars
    if (!door)
    {
        door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
        P_AddThinker (&door->thinker);
        sec->specialdata = door;
                
        door->type = sdt_openAndClose;
        door->status = sd_opening;
        door->whichDoorIndex = P_FindSlidingDoorType(line);

        if (door->whichDoorIndex < 0)
            I_Error("EV_SlidingDoor: Can't use texture for sliding door!");
                        
        door->frontsector = sec;
        door->backsector = line->backsector;
        door->thinker.function = T_SlidingDoor;
        door->timer = SWAITTICS;
        door->frame = 0;
        door->line = line;
    }
}
#endif
