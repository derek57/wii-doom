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
//        Player related stuff.
//        Bobbing POV/weapon, movement.
//        Pending weapon.
//
//-----------------------------------------------------------------------------



#include <stdlib.h>

#include "d_event.h"
#include "doomdef.h"
#include "doomstat.h"
#include "p_local.h"


#define ANG5                    (ANG90/18)

// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP         32

// 16 pixels of bob
#define MAXBOB                  0x100000        

//
// Movement.
//

boolean           onground;

extern int        prio;

//
// P_Thrust
// Moves the given origin along a given angle.
//
void
P_Thrust
( player_t*      player,
  angle_t        angle,
  fixed_t        move ) 
{
    angle >>= ANGLETOFINESHIFT;
    
    player->mo->momx += FixedMul(move,finecosine[angle]); 
    player->mo->momy += FixedMul(move,finesine[angle]);
}




//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight (player_t* player) 
{
    int            angle;
    fixed_t        bob;
    
    // Regular movement bobbing
    // (needs to be calculated for gun swing
    // even if not on ground)
    // OPTIMIZE: tablify angle
    // Note: a LUT allows for effects
    //  like a ramp with low health.
    player->bob =
        FixedMul (player->mo->momx, player->mo->momx)
        + FixedMul (player->mo->momy,player->mo->momy);
    
    player->bob >>= 2;

    if (player->bob>MAXBOB)
        player->bob = MAXBOB;

    if ((player->cheats & CF_NOMOMENTUM) || !onground)
    {
        player->viewz = player->mo->z + VIEWHEIGHT;

        if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
            player->viewz = player->mo->ceilingz-4*FRACUNIT;

        player->viewz = player->mo->z + player->viewheight;
        return;
    }
                
    angle = (FINEANGLES/20*leveltime)&FINEMASK;
    bob = FixedMul ( player->bob/2, finesine[angle]);

    
    // move viewheight
    if (player->playerstate == PST_LIVE)
    {
        player->viewheight += player->deltaviewheight;

        if (player->viewheight > VIEWHEIGHT)
        {
            player->viewheight = VIEWHEIGHT;
            player->deltaviewheight = 0;
        }

        if (player->viewheight < VIEWHEIGHT/2)
        {
            player->viewheight = VIEWHEIGHT/2;
            if (player->deltaviewheight <= 0)
                player->deltaviewheight = 1;
        }
        
        if (player->deltaviewheight)        
        {
            player->deltaviewheight += FRACUNIT/4;
            if (!player->deltaviewheight)
                player->deltaviewheight = 1;
        }
    }
    player->viewz = player->mo->z + player->viewheight + bob;

    if (player->mo->flags2 & MF2_FEETARECLIPPED && d_footclip
        && player->playerstate != PST_DEAD
        && player->mo->z <= player->mo->floorz)
    {
        player->viewz -= FOOTCLIPSIZE;
    }
    if (player->viewz > player->mo->ceilingz - 4 * FRACUNIT)
    {
        player->viewz = player->mo->ceilingz - 4 * FRACUNIT;
    }
    if (player->viewz < player->mo->floorz + 4 * FRACUNIT)
    {
        player->viewz = player->mo->floorz + 4 * FRACUNIT;
    }
}



//
// P_MovePlayer
//
void P_MovePlayer (player_t* player)
{
    int        look;

    ticcmd_t*  cmd;
        
    cmd = &player->cmd;
        
    player->mo->angle += (cmd->angleturn<<16);

    // Do not let the player control movement
    //  if not onground.
    onground = (player->mo->z <= player->mo->floorz);
        
    if (cmd->forwardmove && onground)
        P_Thrust (player, player->mo->angle, cmd->forwardmove*2048);
    
    if (cmd->sidemove && onground)
        P_Thrust (player, player->mo->angle-ANG90, cmd->sidemove*2048);

    if ( (cmd->forwardmove || cmd->sidemove) 
         && player->mo->state == &states[S_PLAY] )
    {
        P_SetMobjState (player->mo, S_PLAY_RUN1);
    }

    look = cmd->lookfly & 15;
    if (look > 7)
    {
        look -= 16;
    }
    if (look)
    {
        if (look == -8)
        {
            player->centering = true;
        }
        else
        {
            player->lookdir += 5 * look;
            if (player->lookdir > 90 || player->lookdir < -110)
            {
                player->lookdir -= 5 * look;
            }
        }
    }
    if (player->centering)
    {
        if (player->lookdir > 0)
        {
            player->lookdir -= 8;
        }
        else if (player->lookdir < 0)
        {
            player->lookdir += 8;
        }
        if (abs(player->lookdir) < 8)
        {
            player->lookdir = 0;
            player->centering = false;
        }
    }
}        



//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
void P_DeathThink (player_t* player)
{
    angle_t    angle;
    angle_t   delta;

    P_MovePsprites (player);
        
    // fall to the ground
    if (player->viewheight > 6*FRACUNIT)
        player->viewheight -= FRACUNIT;

    if (player->viewheight < 6*FRACUNIT)
        player->viewheight = 6*FRACUNIT;

    player->deltaviewheight = 0;
    onground = (player->mo->z <= player->mo->floorz);

    if (player->lookdir > 0)
    {
        player->lookdir -= 6;
    }
    else if (player->lookdir < 0)
    {
        player->lookdir += 6;
    }

    if (abs(player->lookdir) < 6)
    {
        player->lookdir = 0;
    }

    P_CalcHeight (player);
        
    if (player->attacker && player->attacker != player->mo)
    {
        angle = R_PointToAngle2 (player->mo->x,
                                 player->mo->y,
                                 player->attacker->x,
                                 player->attacker->y);
        
        delta = angle - player->mo->angle;
        
        if (delta < ANG5 || delta > (unsigned)-ANG5)
        {
            // Looking at killer,
            //  so fade damage flash down.
            player->mo->angle = angle;

            if (player->damagecount)
                player->damagecount--;
        }
        else if (delta < ANG180)
            player->mo->angle += ANG5;
        else
            player->mo->angle -= ANG5;
    }
    else if (player->damagecount)
        player->damagecount--;
        

    if (player->cmd.buttons & BT_USE)
        player->playerstate = PST_REBORN;
}



//
// P_PlayerThink
//
void P_PlayerThink (player_t* player)
{
    ticcmd_t*           cmd;
    weapontype_t        newweapon;

    // chain saw run forward
    cmd = &player->cmd;
    if (player->mo->flags & MF_JUSTATTACKED)
    {
        cmd->angleturn = 0;
        cmd->forwardmove = 0xc800/512;
        cmd->sidemove = 0;
        player->mo->flags &= ~MF_JUSTATTACKED;
    }

    player->worldTimer++;
        
    if (player->playerstate == PST_DEAD)
    {
        P_DeathThink (player);
        return;
    }
    
    if(jumping)
    {
        if (player->jumpTics)
        {
            player->jumpTics--;
        }
    }

    // Move around.
    // Reactiontime is used to prevent movement
    //  for a bit after a teleport.
    if (player->mo->reactiontime)
        player->mo->reactiontime--;
    else
        P_MovePlayer (player);
    
    P_CalcHeight (player);

    if (player->mo->subsector->sector->special)
        P_PlayerInSpecialSector (player);
    
    if (cmd->arti)
    {                           // Use an artifact
        if(jumping)
        {
                if ((cmd->arti & AFLAG_JUMP) && onground && !player->jumpTics)
                {
                player->mo->momz = 9 * FRACUNIT;
                    player->jumpTics = 18;
                }
        }
    }
    // Check for weapon change.

    // A special event has no other buttons.
    if (cmd->buttons & BT_SPECIAL)
        cmd->buttons = 0;                        
                
    if (cmd->buttons & BT_CHANGE)
    {
        // The actual changing of the weapon is done
        //  when the weapon psprite can do it
        //  (read: not in the middle of an attack).
        newweapon = (cmd->buttons&BT_WEAPONMASK)>>BT_WEAPONSHIFT;
        
        if (newweapon == wp_fist
            && player->weaponowned[wp_chainsaw]
            && !(player->readyweapon == wp_chainsaw
                 && player->powers[pw_strength]))
        {
            newweapon = wp_chainsaw;
        }
        
        if ( (gamemode == commercial)
            && newweapon == wp_shotgun 
            && player->weaponowned[wp_supershotgun]
            && player->readyweapon != wp_supershotgun)
        {
            newweapon = wp_supershotgun;
        }
        

        if (player->weaponowned[newweapon]
            && newweapon != player->readyweapon)
        {
            // Do not go to plasma or BFG in shareware,
            //  even if cheated.
            if ((newweapon != wp_plasma
                 && newweapon != wp_bfg)
                || (gamemode != shareware) )
            {
                player->pendingweapon = newweapon;
            }
        }
    }
    
    // check for use
    if (cmd->buttons & BT_USE)
    {
        if (!player->usedown)
        {
            P_UseLines (player);
            player->usedown = true;
        }
    }
    else
        player->usedown = false;
    
    // cycle psprites
    P_MovePsprites (player);
    
    // Counters, time dependend power ups.

    // Strength counts up to diminish fade.
    if (player->powers[pw_strength])
        player->powers[pw_strength]++;        
                
    if (player->powers[pw_invulnerability])
        player->powers[pw_invulnerability]--;

    if (player->powers[pw_invisibility])
        if (! --player->powers[pw_invisibility] )
            player->mo->flags &= ~MF_SHADOW;
                        
    if (player->powers[pw_infrared])
        player->powers[pw_infrared]--;
                
    if (player->powers[pw_ironfeet])
        player->powers[pw_ironfeet]--;
                
    if (player->damagecount)
        player->damagecount--;
                
    if (player->bonuscount)
        player->bonuscount--;

    
    // Handling colormaps.
    if (player->powers[pw_invulnerability])
    {
        if (player->powers[pw_invulnerability] > 4*32
            || (player->powers[pw_invulnerability]&8) )
            player->fixedcolormap = INVERSECOLORMAP;
        else
            player->fixedcolormap = 0;
    }
    else if (player->powers[pw_infrared])        
    {
        if (player->powers[pw_infrared] > 4*32
            || (player->powers[pw_infrared]&8) )
        {
            // almost full bright
            player->fixedcolormap = 1;
        }
        else
            player->fixedcolormap = 0;
    }
    else
        player->fixedcolormap = 0;

    // recoil pitch from weapons
    if(player->recoilpitch && d_recoil)
    {
        fixed_t recoil = (player->recoilpitch >> 3);

        if(player->recoilpitch - recoil > 0)
            player->recoilpitch -= recoil;
        else
            player->recoilpitch = 0;
    }
    else
        player->recoilpitch = 0;
}

void P_AimingHelp (player_t* player)
{
    angle_t                angle;
    angle_t                delta;

    if (prio == 7 &&
        player->attacker->health > 0 &&
        player->attacker &&
        player->attacker != player->mo &&
        P_CheckSight (player->mo, player->attacker))
    {
        angle = R_PointToAngle2 (player->mo->x,
                                 player->mo->y,
                                 player->attacker->x,
                                 player->attacker->y);

        delta = angle - player->mo->angle;

        // Looking at attacker
        if (delta < ANG5 || delta > (unsigned)-ANG5)
            player->mo->angle = angle;
        else if (delta < ANG180)
            player->mo->angle += ANG5;
        else
            player->mo->angle -= ANG5;
    }

    if (prio == 7 &&
        player->attacker->health < 1 &&
        player->attacker &&
        player->attacker != player->mo)
        prio = 0;
}

