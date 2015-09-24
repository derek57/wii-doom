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


#ifdef WII
#include "../c_io.h"
#else
#include "c_io.h"
#endif

// Data.
#include "dstrings.h"

#ifdef WII
#include "../deh_str.h"
#else
#include "deh_str.h"
#endif

#include "doomdef.h"

// State.
#include "doomstat.h"

#ifdef WII
#include "../i_system.h"
#else
#include "i_system.h"
#endif

#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"
#include "sounds.h"

#ifdef WII
#include "../v_trans.h"
#include "../z_zone.h"
#else
#include "v_trans.h"
#include "z_zone.h"
#endif


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
              case blazeRaise:
                door->direction = -1; // time to go back down

                if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                        fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
                    S_StartSound(&door->sector->soundorg, sfx_bdcls);
                break;
                
              case normal:
                door->direction = -1; // time to go back down
                S_StartSound(&door->sector->soundorg, sfx_dorcls);
                break;
                
              case close30ThenOpen:
                door->direction = 1;
                S_StartSound(&door->sector->soundorg, sfx_doropn);
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
              case raiseIn5Mins:
                door->direction = 1;
                door->type = normal;
                S_StartSound(&door->sector->soundorg, sfx_doropn);
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
                          door->sector->floor_height,
                          false,1,door->direction);
        // [BH] enhanced to apply effects to all doors
        if (door->topheight - door->sector->floor_height)
        {
            fixed_t level = FixedDiv(door->sector->ceiling_height - door->sector->floor_height,
                door->topheight - door->sector->floor_height);

            if (door->lighttag)
                EV_LightTurnOnPartway(door->line, level);
            else
                EV_LightByAdjacentSectors(door->sector, level);
        }

        if (res == pastdest)
        {
            switch(door->type)
            {
              case blazeRaise:
              case blazeClose:
                door->sector->specialdata = NULL;
                P_RemoveThinker (&door->thinker);  // unlink and free

                if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                        fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
                {
                    // killough 4/15/98: remove double-closing sound of blazing doors
                    if (!d_blazingsound)
                        S_StartSound((mobj_t *)&door->sector->soundorg, sfx_bdcls);
                }
                break;
                
              case normal:
              case closed:
                door->sector->specialdata = NULL;
                P_RemoveThinker (&door->thinker);  // unlink and free
                break;
                
              case close30ThenOpen:
                door->direction = 0;
                door->topcountdown = TICRATE*30;
                break;
                
              default:
                break;
            }
        }
        else if (res == crushed)
        {
            switch(door->type)
            {
              case blazeClose:
              case closed:                // DO NOT GO BACK UP!
                break;
                
              default:
                door->direction = 1;
                S_StartSound(&door->sector->soundorg, sfx_doropn);
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
        
        // [BH] enhanced to apply effects to all doors
        if (door->topheight - door->sector->floor_height)
        {
            fixed_t level = FixedDiv(door->sector->ceiling_height - door->sector->floor_height,
                door->topheight - door->sector->floor_height);

            if (door->lighttag)
                EV_LightTurnOnPartway(door->line, level);
            else
                EV_LightByAdjacentSectors(door->sector, level);
        }

        if (res == pastdest)
        {
            switch(door->type)
            {
              case blazeRaise:
              case normal:
                door->direction = 0; // wait at top
                door->topcountdown = door->topwait;
                break;
                
              case close30ThenOpen:
              case blazeOpen:
              case open:
                door->sector->specialdata = NULL;
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
        
    p = thing->player;
        
    if (!p)
        return 0;
                
    switch(line->special)
    {
      case 99:        // Blue Lock
      case 133:
        if ( !p )
            return 0;
        if (p->cards[it_bluecard] <= 0 && p->cards[it_blueskull] <= 0)
        {
            if (p->cards[it_bluecard] == CARDNOTFOUNDYET)
            {
                if (!p->neededcardflash || p->neededcard != it_bluecard)
                {
                    p->neededcard = it_bluecard;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                p->message = DEH_String(PD_BLUEO);
            }
            else if (p->cards[it_blueskull] == CARDNOTFOUNDYET)
            {
                if (!p->neededcardflash || p->neededcard != it_blueskull)
                {
                    p->neededcard = it_blueskull;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                p->message = DEH_String(PD_BLUEO);
            }
            S_StartSound(NULL,sfx_oof);
            return 0;
        }
        break;
        
      case 134: // Red Lock
      case 135:
        if ( !p )
            return 0;
        if (p->cards[it_redcard] <= 0 && p->cards[it_redskull] <= 0)
        {
            if (p->cards[it_redcard] == CARDNOTFOUNDYET)
            {
                if (!p->neededcardflash || p->neededcard != it_redcard)
                {
                    p->neededcard = it_redcard;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                p->message = DEH_String(PD_REDO);
            }
            else if (p->cards[it_redskull] == CARDNOTFOUNDYET)
            {
                if (!p->neededcardflash || p->neededcard != it_redskull)
                {
                    p->neededcard = it_redskull;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                p->message = DEH_String(PD_REDO);
            }
            S_StartSound(NULL,sfx_oof);
            return 0;
        }
        break;
        
      case 136:        // Yellow Lock
      case 137:
        if ( !p )
            return 0;
        if (p->cards[it_yellowcard] <= 0 && p->cards[it_yellowskull] <= 0)
        {
            if (p->cards[it_yellowcard] == CARDNOTFOUNDYET)
            {
                if (!p->neededcardflash || p->neededcard != it_yellowcard)
                {
                    p->neededcard = it_yellowcard;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                p->message = DEH_String(PD_YELLOWO);
            }
            else if (p->cards[it_yellowskull] == CARDNOTFOUNDYET)
            {
                if (!p->neededcardflash || p->neededcard != it_yellowskull)
                {
                    p->neededcard = it_yellowskull;
                    p->neededcardflash = NEEDEDCARDFLASH;
                }
                p->message = DEH_String(PD_YELLOWO);
            }
            S_StartSound(NULL,sfx_oof);
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
    int          secnum,rtn;
    sector_t*    sec;
    vldoor_t*    door;
        
    secnum = -1;
    rtn = 0;
    
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];
        if (sec->specialdata)
            continue;
                
        
        // new door thinker
        rtn = 1;
        door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
        P_AddThinker (&door->thinker);
        sec->specialdata = door;

        door->thinker.function.acp1 = (actionf_p1) T_VerticalDoor;
        door->sector = sec;
        door->type = type;
        door->topwait = VDOORWAIT;
        door->speed = VDOORSPEED;
        door->line = line;      // jff 1/31/98 remember line that triggered us
        door->lighttag = 0;
                
        switch(type)
        {
          case blazeClose:
            door->topheight = P_FindLowestCeilingSurrounding(sec);
            door->topheight -= 4*FRACUNIT;
            door->direction = -1;
            door->speed = VDOORSPEED * 4;

            if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                    fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
                S_StartSound(&door->sector->soundorg, sfx_bdcls);
            break;
            
          case closed:
            door->topheight = P_FindLowestCeilingSurrounding(sec);
            door->topheight -= 4*FRACUNIT;
            door->direction = -1;
            S_StartSound(&door->sector->soundorg, sfx_dorcls);
            break;
            
          case close30ThenOpen:
            door->topheight = sec->ceiling_height;
            door->direction = -1;
            S_StartSound(&door->sector->soundorg, sfx_dorcls);
            break;
            
          case blazeRaise:
          case blazeOpen:
            door->direction = 1;
            door->topheight = P_FindLowestCeilingSurrounding(sec);
            door->topheight -= 4*FRACUNIT;
            door->speed = VDOORSPEED * 4;
            if (door->topheight != sec->ceiling_height)
            {
                if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                        fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
                    S_StartSound(&door->sector->soundorg, sfx_bdopn);
            }
            break;
            
          case normal:
          case open:
            door->direction = 1;
            door->topheight = P_FindLowestCeilingSurrounding(sec);
            door->topheight -= 4*FRACUNIT;
            if (door->topheight != sec->ceiling_height)
                S_StartSound(&door->sector->soundorg, sfx_doropn);
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
    int          side;
        
    side = 0;        // only front sides can be used

    //        Check for locks
    player = thing->player;
                
    switch(line->special)
    {
      case 26: // Blue Lock
      case 32:
        if ( !player )
            return;
        
        if (player->cards[it_bluecard] <= 0 && player->cards[it_blueskull] <= 0)
        {
            if (player->cards[it_bluecard] == CARDNOTFOUNDYET)
            {
                if (!player->neededcardflash || player->neededcard != it_bluecard)
                {
                    player->neededcard = it_bluecard;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                player->message = DEH_String(PD_BLUEK);
            }
            else if (player->cards[it_blueskull] == CARDNOTFOUNDYET)
            {
                if (!player->neededcardflash || player->neededcard != it_blueskull)
                {
                    player->neededcard = it_blueskull;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                player->message = DEH_String(PD_BLUEK);
            }
            S_StartSound(NULL,sfx_oof);
            return;
        }
        break;
        
      case 27: // Yellow Lock
      case 34:
        if ( !player )
            return;
        
        if (player->cards[it_yellowcard] <= 0 && player->cards[it_yellowskull] <= 0)
        {
            if (player->cards[it_yellowcard] == CARDNOTFOUNDYET)
            {
                if (!player->neededcardflash || player->neededcard != it_yellowcard)
                {
                    player->neededcard = it_yellowcard;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                player->message = DEH_String(PD_YELLOWK);
            }
            else if (player->cards[it_yellowskull] == CARDNOTFOUNDYET)
            {
                if (!player->neededcardflash || player->neededcard != it_yellowskull)
                {
                    player->neededcard = it_yellowskull;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                player->message = DEH_String(PD_YELLOWK);
            }
            S_StartSound(NULL,sfx_oof);
            return;
        }
        break;
        
      case 28: // Red Lock
      case 33:
        if ( !player )
            return;
        
        if (player->cards[it_redcard] <= 0 && player->cards[it_redskull] <= 0)
        {
            if (player->cards[it_redcard] == CARDNOTFOUNDYET)
            {
                if (!player->neededcardflash || player->neededcard != it_redcard)
                {
                    player->neededcard = it_redcard;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                player->message = DEH_String(PD_REDK);
            }
            else if (player->cards[it_redskull] == CARDNOTFOUNDYET)
            {
                if (!player->neededcardflash || player->neededcard != it_redskull)
                {
                    player->neededcard = it_redskull;
                    player->neededcardflash = NEEDEDCARDFLASH;
                }
                player->message = DEH_String(PD_REDK);
            }
            S_StartSound(NULL,sfx_oof);
            return;
        }
        break;
    }
        
    // if the wrong side of door is pushed, give oof sound
    if (line->sidenum[1] == NO_INDEX)           // killough
    {
        S_StartSound(player->mo, sfx_oof);    // killough 3/20/98
        return;
    }

    // if the sector has an active thinker, use it

    if (line->sidenum[side^1] == -1)
    {
        // [crispy] do not crash if the wrong side of the door is pushed
        C_Printf(CR_RED, " EV_VerticalDoor: DR special type on 1-sided linedef\n");
        return;
    }

    sec = sides[ line->sidenum[side^1]] .sector;

    if (sec->specialdata)
    {
        door = sec->specialdata;
        switch(line->special)
        {
          case        1: // ONLY FOR "RAISE" DOORS, NOT "OPEN"s
          case        26:
          case        27:
          case        28:
          case        117:
            if (door->direction == -1)
                door->direction = 1;        // go back up
            else
            {
                if (!thing->player)
                    return;                // JDC: bad guys never close doors

                // When is a door not a door?
                // In Vanilla, door->direction is set, even though
                // "specialdata" might not actually point at a door.

                if (door->thinker.function.acp1 == (actionf_p1) T_VerticalDoor)
                {
                    door->direction = -1;        // start going down immediately
                }
                else if (door->thinker.function.acp1 == (actionf_p1) T_PlatRaise)
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

                    C_Printf(CR_GOLD, "EV_VerticalDoor: Tried to close "
                                    "something that wasn't a door.\n");

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
      case 117:        // BLAZING DOOR RAISE
      case 118:        // BLAZING DOOR OPEN

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
                fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
            S_StartSound(&sec->soundorg,sfx_bdopn);
        break;
        
      case 1:        // NORMAL DOOR SOUND
      case 31:
        S_StartSound(&sec->soundorg,sfx_doropn);
        break;
        
      default:        // LOCKED DOOR SOUND
        S_StartSound(&sec->soundorg,sfx_doropn);
        break;
    }
        
    
    // new door thinker
    door = Z_Malloc (sizeof(*door), PU_LEVSPEC, 0);
    P_AddThinker (&door->thinker);
    sec->specialdata = door;
    door->thinker.function.acp1 = (actionf_p1) T_VerticalDoor;
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
      case 1:
      case 26:
      case 27:
      case 28:
        door->type = normal;
        break;
        
      case 31:
      case 32:
      case 33:
      case 34:
        door->type = open;
        line->special = 0;
        break;
        
      case 117:        // blazing door raise
        door->type = blazeRaise;
        door->speed = VDOORSPEED*4;
        break;
      case 118:        // blazing door open
        door->type = blazeOpen;
        line->special = 0;
        door->speed = VDOORSPEED*4;
        break;

      default:
        door->lighttag = 0; // killough 10/98
        break;
    }
    
    // find the top and bottom of the movement range
    door->topheight = P_FindLowestCeilingSurrounding(sec);
    door->topheight -= 4*FRACUNIT;
}


//
// Spawn a door that closes after 30 seconds
//
void P_SpawnDoorCloseIn30 (sector_t* sec)
{
    vldoor_t*        door;
        
    door = Z_Malloc ( sizeof(*door), PU_LEVSPEC, 0);

    P_AddThinker (&door->thinker);

    sec->specialdata = door;
    sec->special = 0;

    door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
    door->sector = sec;
    door->direction = 0;
    door->type = normal;
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
( sector_t*        sec,
  int              secnum )
{
    vldoor_t*      door;
        
    door = Z_Malloc ( sizeof(*door), PU_LEVSPEC, 0);
    
    P_AddThinker (&door->thinker);

    sec->specialdata = door;
    sec->special = 0;

    door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
    door->sector = sec;
    door->direction = 2;
    door->type = raiseIn5Mins;
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
