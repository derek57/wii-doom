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

#include "c_io.h"
#include "d_event.h"
#include "doomdef.h"
#include "doomstat.h"
#include "p_inter.h"
#include "p_local.h"
#include "s_sound.h"
#include "sounds.h"
#include "wii-doom.h"


#define ANG5      (ANG90 / 18)

// 16 pixels of bob
#define MAXBOB    0x100000        


//
// Movement.
//

dboolean          onground;

extern int        prio;

//
// P_Thrust
// Moves the given origin along a given angle.
//
void P_Thrust(player_t *player, angle_t angle, fixed_t move) 
{
    angle >>= ANGLETOFINESHIFT;
    
    player->mo->momx += FixedMul(move, finecosine[angle]);
    player->mo->momy += FixedMul(move, finesine[angle]);
}

// P_Bob
// Same as P_Thrust, but only affects bobbing.
//
// killough 10/98: We apply thrust separately between the real physical player
// and the part which affects bobbing. This way, bobbing only comes from player
// motion, nothing external, avoiding many problems, e.g. bobbing should not
// occur on conveyors, unless the player walks on one, and bobbing should be
// reduced at a regular rate, even on ice (where the player coasts).
//
void P_Bob(player_t *player, angle_t angle, fixed_t move)
{
    player->momx += FixedMul(move, finecosine[angle >>= ANGLETOFINESHIFT]);
    player->momy += FixedMul(move, finesine[angle]);
}

//
// P_CalcHeight
// Calculate the walking / running height adjustment
//
void P_CalcHeight(player_t *player) 
{
    mobj_t      *mo = player->mo;

    if (!onground)
        player->viewz = MIN(mo->z + VIEWHEIGHT, mo->ceilingz - 4 * FRACUNIT);
    else if (player->playerstate == PST_LIVE)
    {
        int     angle = (FINEANGLES / 20 * leveltime) & FINEMASK;
        fixed_t bob = ((FixedMul(player->momx, player->momx)
            + FixedMul(player->momy, player->momy)) >> 2);

        // Regular movement bobbing
        // (needs to be calculated for gun swing
        // even if not on ground)
        player->bob = (bob ? MAX(MIN(bob, MAXBOB) * movebob / 100, MAXBOB * stillbob / 400) :
            MAXBOB * stillbob / 400);
        
        bob = FixedMul(player->bob / 2, finesine[angle]);

        // move viewheight
        player->viewheight += player->deltaviewheight;

        if (player->viewheight > VIEWHEIGHT)
        {
            player->viewheight = VIEWHEIGHT;
            player->deltaviewheight = 0;
        }

        if (player->viewheight < VIEWHEIGHT / 2)
        {
            player->viewheight = VIEWHEIGHT / 2;

            if (player->deltaviewheight <= 0)
                player->deltaviewheight = 1;
        }

        if (player->deltaviewheight)
        {
            player->deltaviewheight += FRACUNIT / 4;

            if (!player->deltaviewheight)
                player->deltaviewheight = 1;
        }

        player->viewz = mo->z + player->viewheight + bob;
    }
    else
        player->viewz = mo->z + player->viewheight;

    if ((mo->flags2 & MF2_FEETARECLIPPED) && d_footclip)
    {
        dboolean                    liquid = true;
        const struct msecnode_s     *seclist;

        for (seclist = mo->touching_sectorlist; seclist; seclist = seclist->m_tnext)
            if (!isliquid[seclist->m_sector->floorpic] || seclist->m_sector->heightsec != -1)
            {
                liquid = false;
                break;
            }

        if (liquid)
            player->viewz -= FOOTCLIPSIZE;
    }

    player->viewz = BETWEEN(mo->floorz + 4 * FRACUNIT, player->viewz, mo->ceilingz - 4 * FRACUNIT);
}

//
// P_MovePlayer
//
void P_MovePlayer(player_t *player)
{
    int        look;
    int        fly;

    ticcmd_t   *cmd;
        
    cmd = &player->cmd;
        
    player->mo->angle += (cmd->angleturn << FRACBITS);

    // Do not let the player control movement
    //  if not onground.
    onground = (player->mo->z <= player->mo->floorz
                || (player->mo->flags2 & MF2_ONMOBJ));
        
    // killough 10/98
    if (cmd->forwardmov | cmd->sidemov)
    {
        // killough 8/9/98
        if (onground || (player->mo->flags2 & MF2_FLY) || (player->mo->flags & MF_BOUNCES))
        {
            int friction;
            int movefactor = P_GetMoveFactor(player->mo, &friction);
 
            // killough 11/98:
            // On sludge, make bobbing depend on efficiency.
            // On ice, make it depend on effort.
            int bobfactor = (friction < ORIG_FRICTION ? movefactor : ORIG_FRICTION_FACTOR);
 
            if (cmd->forwardmov)
            {
                P_Bob(player, player->mo->angle, cmd->forwardmov * bobfactor);
                P_Thrust(player, player->mo->angle, cmd->forwardmov * movefactor);
            }
 
            if (cmd->sidemov)
            {
                P_Bob(player, player->mo->angle - ANG90, cmd->sidemov * bobfactor);
                P_Thrust(player, player->mo->angle - ANG90, cmd->sidemov * movefactor);
            }
        }

        if (player->mo->state == states + S_PLAY)
            P_SetMobjState(player->mo, S_PLAY_RUN1);
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

        if (ABS(player->lookdir) < 8)
        {
            player->lookdir = 0;
            player->centering = false;
        }
    }

    fly = cmd->lookfly >> 4;

    if (fly > 7)
    {
        fly -= 16;
    }

    if (fly && player->powers[pw_flight])
    {
        if (fly != -8)
        {
            player->flyheight = fly * 2;

            if (!(player->mo->flags2 & MF2_FLY))
            {
                player->mo->flags2 |= MF2_FLY;
                player->mo->flags |= MF_NOGRAVITY;
            }
        }
        else
        {
            player->mo->flags2 &= ~MF2_FLY;
            player->mo->flags &= ~MF_NOGRAVITY;
        }
    }

    if (player->mo->flags2 & MF2_FLY)
    {
        player->mo->momz = player->flyheight * FRACUNIT;

        if (player->flyheight)
        {
            player->flyheight /= 2;
        }
    }
}        

//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//
void P_DeathThink(player_t *player)
{
    if (allow_infighting)
        infight = true;

    P_MovePsprites(player);
        
    // fall to the ground
    if (player->viewheight > 6 * FRACUNIT)
        player->viewheight -= FRACUNIT;

    if (player->viewheight < 6 * FRACUNIT)
        player->viewheight = 6 * FRACUNIT;

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

    if (ABS(player->lookdir) < 6)
    {
        player->lookdir = 0;
    }

    P_CalcHeight(player);
        
    if (player->attacker && player->attacker != player->mo)
    {
        angle_t angle = R_PointToAngle2(player->mo->x,
                                        player->mo->y,
                                        player->attacker->x,
                                        player->attacker->y);
        
        angle_t delta = angle - player->mo->angle;
        
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
        
    if (consoleheight)
        return;

    if (player->cmd.buttons & BT_USE)
        player->playerstate = PST_REBORN;
}

void P_ResurrectPlayer(player_t *player)
{
    fixed_t     x, y;
    int         angle;
    mobj_t      *thing;

    // remove player's corpse
    P_RemoveMobj(player->mo);

    // spawn a teleport fog
    x = player->mo->x;
    y = player->mo->y;
    angle = player->mo->angle >> ANGLETOFINESHIFT;
    thing = P_SpawnMobj(x + 20 * finecosine[angle], y + 20 * finesine[angle],
        ONFLOORZ, MT_TFOG);
    thing->angle = player->mo->angle;
    S_StartSound(thing, sfx_telept);

    // telefrag anything in this spot
    P_TeleportMove(thing, thing->x, thing->y, thing->z, true);

    // respawn the player.
    thing = P_SpawnMobj(x, y, ONFLOORZ, MT_PLAYER);
    thing->angle = player->mo->angle;
    thing->player = player;
    thing->health = initial_health;
    thing->reactiontime = 18;
    player->mo = thing;
    player->playerstate = PST_LIVE;
    player->viewheight = VIEWHEIGHT;
    player->health = initial_health;
    infight = false;
    P_SetupPsprites(player);
    P_MapEnd();

    C_HideConsole(); 
}

//
// P_PlayerThink
//
void P_PlayerThink(player_t *player)
{
    ticcmd_t      *cmd;

    // [AM] Assume we can interpolate at the beginning
    //      of the tic.
    player->mo->interp = true;

    // [AM] Store starting position for player interpolation.
    player->mo->oldx = player->mo->x;
    player->mo->oldy = player->mo->y;
    player->mo->oldz = player->mo->z;
    player->mo->oldangle = player->mo->angle;
    player->oldviewz = player->viewz;

    // chain saw run forward
    cmd = &player->cmd;

    if (player->mo->flags & MF_JUSTATTACKED)
    {
        cmd->angleturn = 0;
        cmd->forwardmov = 0xc800 / 512;
        cmd->sidemov = 0;
        player->mo->flags &= ~MF_JUSTATTACKED;
    }

    player->worldTimer++;

    if (player->playerstate == PST_DEAD)
    {
        P_DeathThink(player);
        return;
    }

    if (jumping)
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
        P_MovePlayer(player);
    
    P_CalcHeight(player);

    if (player->mo->subsector->sector->special)
        P_PlayerInSpecialSector(player);
    
    if (cmd->arti)
    {
        // Use an artifact
        if (jumping)
        {
            if ((cmd->arti & AFLAG_JUMP) && onground && !player->jumpTics)
            {
                player->mo->momz = 9 * FRACUNIT;
                player->mo->flags2 &= ~MF2_ONMOBJ;
                player->jumpTics = 18;
                S_StartSound(NULL, sfx_jump);
            }
        }
    }

    // Check for weapon change.

    // A special event has no other buttons.
    if (cmd->buttons & BT_SPECIAL)
        cmd->buttons = 0;                        
                
    if (cmd->buttons & BT_CHANGE)
    {
        weapontype_t newweapon = (cmd->buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT;
        // The actual changing of the weapon is done
        //  when the weapon psprite can do it
        //  (read: not in the middle of an attack).
        if (newweapon == wp_fist && player->weaponowned[wp_chainsaw]
            && !(player->readyweapon == wp_chainsaw && player->powers[pw_strength]))
        {
            newweapon = wp_chainsaw;
        }
        
        if ((gamemode == commercial) && newweapon == wp_shotgun 
            && player->weaponowned[wp_supershotgun] && player->readyweapon != wp_supershotgun)
        {
            newweapon = wp_supershotgun;
        }        

        if (player->weaponowned[newweapon] && newweapon != player->readyweapon)
        {
            // Do not go to plasma or BFG in shareware,
            //  even if cheated.
            if ((newweapon != wp_plasma && newweapon != wp_bfg) || (gamemode != shareware))
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
            P_UseLines(player);
            player->usedown = true;
        }
    }
    else
        player->usedown = false;
    
    // cycle psprites
    P_MovePsprites(player);
    
    // Counters, time dependend power ups.

    // Strength counts up to diminish fade.
    if (player->powers[pw_strength])
        player->powers[pw_strength]++;        
                
    if (player->powers[pw_invulnerability])
        player->powers[pw_invulnerability]--;

    if (player->powers[pw_invisibility])
        if (!--player->powers[pw_invisibility])
            player->mo->flags &= ~MF_SHADOW;

    if (player->powers[pw_infrared])
        player->powers[pw_infrared]--;
                
    if (player->powers[pw_ironfeet])
        player->powers[pw_ironfeet]--;
                
    if (player->powers[pw_flight])
    {
        if (!--player->powers[pw_flight])
        {
            // haleyjd: removed externdriver crap
            if (player->mo->z != player->mo->floorz)
            {
                player->centering = true;
            }

            player->mo->flags2 &= ~MF2_FLY;
            player->mo->flags &= ~MF_NOGRAVITY;
        }
    }

    if (player->damagecount)
        player->damagecount--;
                
    if (player->bonuscount)
        player->bonuscount--;

    // Handling colormaps.
    if (beta_style)
    {
        if (player->powers[pw_invisibility])
        {
            if (player->powers[pw_invisibility] > 4 * 32
                || (player->powers[pw_invisibility] & 8))
                player->fixedcolormap = INVERSECOLORMAP;
            else
                player->fixedcolormap = 0;
        }
    }

    if (player->powers[pw_invulnerability])
    {
        if ((player->powers[pw_invulnerability] > 4 * 32
            || (player->powers[pw_invulnerability] & 8)) && !beta_style)
            player->fixedcolormap = INVERSECOLORMAP;
        else
            player->fixedcolormap = 0;
    }
    else if (player->powers[pw_infrared])
    {
        if ((player->powers[pw_infrared] > 4 * 32
            || (player->powers[pw_infrared] & 8)) && !beta_style)
        {
            // almost full bright
            player->fixedcolormap = 1;
        }
        else
            player->fixedcolormap = 0;
    }
    else if (!beta_style)
        player->fixedcolormap = 0;

    // recoil pitch from weapons
    if (player->recoilpitch && d_recoil)
    {
        fixed_t recoil = (player->recoilpitch >> 3);

        if (player->recoilpitch - recoil > 0)
            player->recoilpitch -= recoil;
        else
            player->recoilpitch = 0;
    }
    else
        player->recoilpitch = 0;
}

void P_AimingHelp(player_t *player)
{
    if (prio == 7 && player->attacker && player->attacker != player->mo)
    {
        if (player->attacker->health > 0 &&
            P_CheckSight(player->mo, player->attacker))
        {
            angle_t angle = R_PointToAngle2(player->mo->x,
                                            player->mo->y,
                                            player->attacker->x,
                                            player->attacker->y);

            angle_t delta = angle - player->mo->angle;

            // Looking at attacker
            if (delta < ANG5 || delta > (unsigned)-ANG5)
                player->mo->angle = angle;
            else if (delta < ANG180)
                player->mo->angle += ANG5;
            else
                player->mo->angle -= ANG5;
        }
        else if (player->attacker->health < 1)
            prio = 0;
    }
}

//----------------------------------------------------------------------------
//
// FUNC P_UseArtifact
//
// Returns true if artifact was used.
//
//----------------------------------------------------------------------------

dboolean P_UseArtifact(player_t *player, artitype_t arti)
{
    switch (arti)
    {
        case arti_fly:
            if (!P_GivePower(player, pw_flight))
            {
                return (false);
            }

            break;

        default:
            return (false);
    }

    return (true);
}

// [RH] (Adapted from Q2)
// P_FallingDamage
//
void P_FallingDamage(mobj_t *mo)
{
    float    delta;

    if (!mo->player)
        // not a player
        return;

    if (mo->flags & MF_NOCLIP)
        return;

    if ((mo->player->oldvelocity[2] < 0)
        && (mo->momz > mo->player->oldvelocity[2])
        && (!(mo->flags2 & MF2_ONMOBJ) || !(mo->z <= mo->floorz)))
    {
        delta = (float)mo->player->oldvelocity[2];
    }
    else
    {
        if (!(mo->flags2 & MF2_ONMOBJ))
            return;

        delta = (float)(mo->momz - mo->player->oldvelocity[2]);
    }

    delta = delta * delta * 2.03904313e-11f;

    if (delta < 1)
        return;

    if (delta < 15)
        return;

    if (delta > 30)
    {
        int damage = (int)((delta - 30) / 2);

        if (damage < 1)
            damage = 1;

        if (d_fallingdamage)
            P_DamageMobj(mo, NULL, NULL, damage);
    }
    else
    {
        return;
    }
}

