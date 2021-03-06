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
//        Weapon sprite animation, weapon objects.
//        Action functions for weapons.
//
//-----------------------------------------------------------------------------


#include "c_io.h"
#include "d_event.h"
#include "d_deh.h"
#include "doomdef.h"

// State.
#include "doomstat.h"

#include "m_random.h"
#include "p_local.h"
#include "p_pspr.h"
#include "p_tick.h"
#include "s_sound.h"

// Data.
#include "sounds.h"

#include "v_trans.h"
#include "wii-doom.h"


#define LOWERSPEED      FRACUNIT * 6
#define RAISESPEED      FRACUNIT * 6
#define WEAPONBOTTOM    128 * FRACUNIT
#define LOOKSLOPE       400



fixed_t                 bulletslope;

dboolean                skippsprinterp = false;
dboolean                beta_plasma_refired;

int                     beta_plasma_counter;
int                     bfglook = 1;


extern void             P_Thrust(player_t *player, angle_t angle, fixed_t move);
extern void             A_EjectCasing(mobj_t *actor);


// [crispy] weapon recoil {thrust, pitch} values
// thrust values from prboom-plus/src/p_pspr.c:73-83
static const int recoil_values[][2] =
{
    // wp_fist
    { 10,   0 },

    // wp_pistol
    { 10,   4 },

    // wp_shotgun
    { 30,  12 },

    // wp_chaingun
    { 10,   4 },

    // wp_missile
    { 100, 16 },

    // wp_plasma
    { 20,   8 },

    // wp_bfg
    { 100, 16 },

    // wp_chainsaw
    { 0,   -2 },

    // wp_supershotgun
    { 80,  16 }
};


// [crispy] add weapon recoil
// adapted from prboom-plus/src/p_pspr.c:484-495 (A_FireSomething())
void A_Recoil(player_t *player)
{
    if (!netgame && d_thrust && !(player->mo->flags & MF_NOCLIP))
        P_Thrust(player, ANG180 + player->mo->angle, 2048 * recoil_values[player->readyweapon][0]);

    if (d_recoil)
        player->recoilpitch = recoil_values[player->readyweapon][1] << FRACBITS;
}

//
// P_SetPsprite
//
void P_SetPsprite(player_t *player, int position, statenum_t stnum) 
{
    pspdef_t *psp = &player->psprites[position];
    state_t  *state;

    do
    {
        if (!stnum)
        {
            // object removed itself
            psp->state = NULL;
            break;        
        }
        
        // killough 7/19/98: Pre-Beta BFG
        if (stnum == S_BFG1 && beta_style)
            // Skip to alternate weapon frame
            stnum = S_OLDBFG1;

        state = &states[stnum];
        state->num = stnum; 
        psp->state = state;

        // could be 0
        psp->tics = state->tics;

        if (psp->state->nextstate == S_PISTOL4 ||
            psp->state->nextstate == S_SGUN6   ||
            psp->state->nextstate == S_DSGUN6  ||
            psp->state->nextstate == S_DSGUN7  ||
            psp->state->nextstate == S_CHAIN2  ||
            psp->state->nextstate == S_CHAIN3)
            A_EjectCasing(player->mo);

        if (state->misc1)
        {
            // coordinate set
            psp->sx = state->misc1 << FRACBITS;
            psp->sy = state->misc2 << FRACBITS;
        }
        
        if (state->dehacked)
            weaponinfo[player->readyweapon].dehacked = true;

        // Call action routine.
        // Modified handling.
        if (state->action)
        {
            state->action(player, psp);

            if (!psp->state)
                break;
        }
        
        stnum = psp->state->nextstate;

    } while (!psp->tics);
    // an initial state of 0 could cycle through
}

//
// P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//
void P_BringUpWeapon(player_t *player)
{
    statenum_t        newstate;
        
    if (player->pendingweapon == wp_nochange)
        player->pendingweapon = player->readyweapon;
                
    if (player->pendingweapon == wp_chainsaw)
        S_StartSound(player->mo, sfx_sawup);
                
    if (player->pendingweapon >= NUMWEAPONS)
        C_Error("P_BringUpWeapon: weaponinfo overrun has occured.");

    newstate = weaponinfo[player->pendingweapon].upstate;

    if (beta_style)
    {
        if (player->pendingweapon == wp_chaingun)
            newstate = S_BETACHAINUP;
        else if (player->pendingweapon == wp_plasma)
            newstate = S_BETAPLASMAUP;
    }

    player->pendingweapon = wp_nochange;
    player->psprites[ps_weapon].sy = WEAPONBOTTOM;

    P_SetPsprite(player, ps_weapon, newstate);
}

//
// P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//
dboolean P_CheckAmmo(player_t *player)
{
    ammotype_t     ammo = weaponinfo[player->readyweapon].ammo;
    int            count;
    statenum_t     newstate;

    // Minimal amount for one shot varies.
    if (player->readyweapon == wp_bfg)
        count = bfgcells;
    // Double barrel.
    else if (player->readyweapon == wp_supershotgun)
        count = 2;
    // Regular.
    else
        count = 1;

    // Some do not need ammunition anyway.
    // Return if current ammunition sufficient.
    if (ammo == am_noammo || player->ammo[ammo] >= count)
        return true;
                
    // Out of ammo, pick a weapon to change to.
    // Preferences are set here.
    do
    {
        if (player->weaponowned[wp_plasma]
            && player->ammo[am_cell]
            && (gamemode != shareware))
        {
            player->pendingweapon = wp_plasma;
        }
        else if (player->weaponowned[wp_supershotgun] 
                 && player->ammo[am_shell] > 2
                 && (gamemode == commercial))
        {
            player->pendingweapon = wp_supershotgun;
        }
        else if (player->weaponowned[wp_chaingun]
                 && player->ammo[am_clip])
        {
            player->pendingweapon = wp_chaingun;
        }
        else if (player->weaponowned[wp_shotgun]
                 && player->ammo[am_shell])
        {
            player->pendingweapon = wp_shotgun;
        }
        else if (player->ammo[am_clip])
        {
            player->pendingweapon = wp_pistol;
        }
        else if (player->weaponowned[wp_chainsaw])
        {
            player->pendingweapon = wp_chainsaw;
        }
        else if (player->weaponowned[wp_missile]
                 && player->ammo[am_misl])
        {
            player->pendingweapon = wp_missile;
        }
        else if (player->weaponowned[wp_bfg] && (player->ammo[am_cell] >= bfgcells
            || bfgcells != BFGCELLS) && (gamemode != shareware))
        {
            player->pendingweapon = wp_bfg;
        }
        else
        {
            // If everything fails.
            player->pendingweapon = wp_fist;
        }
        
    } while (player->pendingweapon == wp_nochange);

    newstate = weaponinfo[player->readyweapon].downstate;

    // Now set appropriate weapon overlay.
    if (beta_style)
    {
        if (player->readyweapon == wp_chaingun)
            newstate = S_BETACHAINDOWN;
        else if (player->readyweapon == wp_plasma)
            newstate = S_BETAPLASMADOWN;
    }

    P_SetPsprite(player, ps_weapon, newstate);

    return false;        
}

//
// P_FireWeapon.
//
void P_FireWeapon(player_t *player)
{
    statenum_t     newstate;
    weapontype_t   readyweapon = player->readyweapon;

    if (!P_CheckAmmo(player))
        return;
        
    P_SetMobjState(player->mo, S_PLAY_ATK1);

    newstate = weaponinfo[player->readyweapon].atkstate;

    if (beta_style)
    {
        if (player->readyweapon == wp_chaingun)
            newstate = S_BETACHAIN1;
        else if (player->readyweapon == wp_plasma)
            newstate = S_BETAPLASMA1;
    }

    P_SetPsprite(player, ps_weapon, newstate);

    // [BH] no noise alert if not punching a monster
    if (readyweapon == wp_fist && !linetarget && disable_noise)
        return;

    P_NoiseAlert(player->mo, player->mo);

    // [crispy] center the weapon sprite horizontally
    if (d_centerweapon)
    {
        // [crispy] do not override state's misc1 if set
        if (!player->psprites[ps_weapon].state->misc1)
        {
            player->psprites[ps_weapon].sx = FRACUNIT;
        }
    }
}

//
// P_DropWeapon
// Player died, so put the weapon away.
//
void P_DropWeapon(player_t *player)
{
    statenum_t     newstate = weaponinfo[player->readyweapon].downstate;

    if (beta_style)
    {
        if (player->readyweapon == wp_chaingun)
            newstate = S_BETACHAINDOWN;
        else if (player->readyweapon == wp_plasma)
            newstate = S_BETAPLASMADOWN;
    }

    P_SetPsprite(player, ps_weapon, newstate);
}

//
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//
void A_WeaponReady(player_t *player, pspdef_t *psp)
{            
    // get out of attack state
    if (player->mo->state == &states[S_PLAY_ATK1]
        || player->mo->state == &states[S_PLAY_ATK2])
    {
        P_SetMobjState(player->mo, S_PLAY);
    }
    
    if (player->readyweapon == wp_chainsaw
        && psp->state == &states[S_SAW])
    {
        S_StartSound(player->mo, sfx_sawidl);

    }
    
    if (beta_style && psp->state == &states[S_SAWB])
        psp->state = &states[S_SAW];

    // check for change
    //  if player is dead, put the weapon away
    if (player->pendingweapon != wp_nochange || !player->health)
    {
        statenum_t     newstate = weaponinfo[player->readyweapon].downstate;

        // change weapon
        //  (pending weapon should allready be validated)
        if (beta_style)
        {
            if (player->readyweapon == wp_chaingun)
                newstate = S_BETACHAINDOWN;
            else if (player->readyweapon == wp_plasma)
                newstate = S_BETAPLASMADOWN;
        }

        P_SetPsprite(player, ps_weapon, newstate);
        return;        
    }
    
    // check for fire
    //  the missile launcher and bfg do not auto fire
    if (player->cmd.buttons & BT_ATTACK)
    {
        if (!player->attackdown
            || (player->readyweapon != wp_missile
                && player->readyweapon != wp_bfg))
        {
            player->attackdown = true;
            P_FireWeapon(player);
            return;
        }
    }
    else
        player->attackdown = false;

    if (player->mo->momx || player->mo->momy || player->mo->momz)
    {
        // bob the weapon based on movement speed
        int     angle = (128 * leveltime) & FINEMASK;
        int     bob = player->bob;

        // [BH] smooth out weapon bob by zeroing out really small bobs
        if (bob < FRACUNIT / 2)
            bob = 0;

        psp->sx = FixedMul(bob, finecosine[angle]);
        psp->sy = WEAPONTOP + FixedMul(bob, finesine[angle & (FINEANGLES / 2 - 1)]);
    }
}

//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//
void A_ReFire(player_t *player, pspdef_t *psp)
{    
    // check for fire
    //  (if a weaponchange is pending, let it go through instead)
    if ((player->cmd.buttons & BT_ATTACK) 
         && player->pendingweapon == wp_nochange
         && player->health)
    {
        if (beta_style && player->readyweapon == wp_plasma)
                beta_plasma_refired = true;

        player->refire++;
        P_FireWeapon(player);
        beta_plasma_refired = false;
    }
    else
    {
        player->refire = 0;
        P_CheckAmmo(player);
    }
}

void A_CheckReload(player_t *player, pspdef_t *psp)
{
    P_CheckAmmo(player);
#if 0
    if (player->ammo[am_shell] < 2)
        P_SetPsprite(player, ps_weapon, S_DSNR1);
#endif
}

//
// A_Lower
// Lowers current weapon,
//  and changes weapon at bottom.
//
void A_Lower(player_t *player, pspdef_t *psp)
{        
    if (beta_style && player->playerstate == PST_DEAD)
    {
        psp->sy = WEAPONTOP;
        return;
    }

    psp->sy += LOWERSPEED;

    // Is already down.
    if (use_vanilla_weapon_change /*|| demoplayback || demorecording*/
#ifndef WII
       || (player->readyweapon == wp_shotgun && player->weaponowned[wp_supershotgun])
       || (player->readyweapon == wp_supershotgun && player->weaponowned[wp_shotgun])
#endif
       )
    {
        if (psp->sy < WEAPONBOTTOM)
            return;
    }

    // Player is dead.
    if (player->playerstate == PST_DEAD)
    {
        psp->sy = WEAPONBOTTOM;

        // don't bring weapon back up
        return;                
    }
    
    // The old weapon has been lowered off the screen,
    // so change the weapon and start raising it
    if (!player->health)
    {
        // Player is dead, so keep the weapon off screen.
        P_SetPsprite(player, ps_weapon, S_NULL);
        return;        
    }
        
    player->readyweapon = player->pendingweapon; 

    P_BringUpWeapon(player);
}

//
// A_Raise
//
void A_Raise(player_t *player, pspdef_t *psp)
{
    statenum_t     newstate;
        
    psp->sy -= RAISESPEED;

    if (use_vanilla_weapon_change /*|| demoplayback || demorecording*/
#ifndef WII
       || (player->readyweapon == wp_shotgun && player->weaponowned[wp_supershotgun])
       || (player->readyweapon == wp_supershotgun && player->weaponowned[wp_shotgun])
#endif
       )
    {
        if (psp->sy > WEAPONTOP)
            return;
    }

    psp->sy = WEAPONTOP;
    
    newstate = weaponinfo[player->readyweapon].readystate;

    // The weapon has been raised all the way,
    //  so change to the ready state.
    if (beta_style)
    {
        if (player->readyweapon == wp_chaingun)
            newstate = S_BETACHAIN;
        else if (player->readyweapon == wp_plasma)
            newstate = S_BETAPLASMA;
    }

    P_SetPsprite(player, ps_weapon, newstate);
}

//
// A_GunFlash
//
void A_GunFlash(player_t *player, pspdef_t *psp) 
{
    statenum_t     newstate;

    P_SetMobjState(player->mo, S_PLAY_ATK2);

    newstate = weaponinfo[player->readyweapon].flashstate;

    if (beta_style && player->readyweapon != wp_missile)
    {
        if (player->readyweapon == wp_chaingun)
            newstate = S_BETACHAINFLASH1;
    }

    P_SetPsprite(player, ps_flash, newstate);
}

//
// WEAPON ATTACKS
//

//
// A_Punch
//
void A_Punch(player_t *player, pspdef_t *psp) 
{
    angle_t        angle;
    int            damage = (P_Random() % 10 + 1) << 1;
    int            slope;

    if (player->powers[pw_strength])        
        damage *= 10;

    angle = player->mo->angle;
    angle += (P_Random() - P_Random()) << 18;
    slope = P_AimLineAttack(player->mo, angle, MELEERANGE);
    P_LineAttack(player->mo, angle, MELEERANGE, slope, damage);

    // turn to face target
    if (linetarget)
    {
        S_StartSound(player->mo, sfx_punch);
        player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y,
                                            linetarget->x, linetarget->y);
    }
}

//
// A_Saw
//
void A_Saw(player_t *player, pspdef_t *psp) 
{
    angle_t        angle = player->mo->angle;
    int            damage = 2 * (P_Random() % 10 + 1);
    int            slope;

    angle += (P_Random() - P_Random()) << 18;
    
    // use meleerange + 1 se the puff doesn't skip the flash
    slope = P_AimLineAttack(player->mo, angle, MELEERANGE + 1);
    P_LineAttack(player->mo, angle, MELEERANGE + 1, slope, damage);

    if (!linetarget)
    {
        S_StartSound(player->mo, sfx_sawful);
        return;
    }

    S_StartSound(player->mo, sfx_sawhit);
        
    // turn to face target
    angle = R_PointToAngle2(player->mo->x, player->mo->y,
                            linetarget->x, linetarget->y);

    if (angle - player->mo->angle > ANG180)
    {
        if ((signed int)(angle - player->mo->angle) < -ANG90 / 20)
            player->mo->angle = angle + ANG90 / 21;
        else
            player->mo->angle -= ANG90 / 20;
    }
    else
    {
        if (angle - player->mo->angle > ANG90 / 20)
            player->mo->angle = angle - ANG90 / 21;
        else
            player->mo->angle += ANG90 / 20;
    }

    player->mo->flags |= MF_JUSTATTACKED;
}

// Doom does not check the bounds of the ammo array.  As a result,
// it is possible to use an ammo type > 4 that overflows into the
// maxammo array and affects that instead.  Through dehacked, for
// example, it is possible to make a weapon that decreases the max
// number of ammo for another weapon.  Emulate this.
static void DecreaseAmmo(player_t *player, int ammonum, int amount)
{
    if (ammonum < NUMAMMO)
    {
        player->ammo[ammonum] -= amount;
    }
    else
    {
        player->maxammo[ammonum - NUMAMMO] -= amount;
    }
}

//
// A_FireMissile
//
void A_FireMissile(player_t *player, pspdef_t *psp) 
{
    if (!d_infiniteammo)
        DecreaseAmmo(player, weaponinfo[player->readyweapon].ammo, 1);

    P_SpawnPlayerMissile(player->mo, MT_ROCKET);
    A_Recoil(player);
}

//
// A_FireBFG
//
void A_FireBFG(player_t *player, pspdef_t *psp) 
{
    if (!d_infiniteammo)
        DecreaseAmmo(player, weaponinfo[player->readyweapon].ammo, bfgcells);

    P_SpawnPlayerMissile(player->mo, MT_BFG);
    A_Recoil(player);
}

//
// A_FireOldBFG
//
// This function emulates Doom's Pre-Beta BFG
// By Lee Killough 6/6/98, 7/11/98, 7/19/98, 8/20/98
//
// This code may not be used in other mods without appropriate credit given.
// Code leeches will be telefragged.
void A_FireOldBFG(player_t *player, pspdef_t *psp)
{
    int type = MT_PLASMA1;

    // sf: make sure the player is in firing frame, or it looks silly
    P_SetMobjState(player->mo, S_PLAY_ATK2);

    if (!d_infiniteammo)
        player->ammo[weaponinfo[player->readyweapon].ammo]--;

    player->extralight = 2;

    do
    {
        mobj_t *th;
        mobj_t *mo = player->mo;

        angle_t an = mo->angle;
        angle_t an1 = ((P_Random() & 127) - 64) * (ANG90 / 768) + an;
        angle_t an2 = ((P_Random() & 127) - 64) * (ANG90 / 640) + ANG90;

        fixed_t slope;

        if (autoaim)
        {
            // killough 8/2/98: make autoaiming prefer enemies
            slope = P_AimLineAttack(mo, an, 16 * 64 * FRACUNIT);

            if (!linetarget)
                slope = P_AimLineAttack(mo, an += 1 << 26, 16 * 64 * FRACUNIT);

            if (!linetarget)
                slope = P_AimLineAttack(mo, an -= 2 << 26, 16 * 64 * FRACUNIT);

            if (!linetarget)
                // sf: looking up/down
                slope = bfglook == 1 ? player->lookdir * LOOKSLOPE : 0, an = mo->angle;

            an1 += an - mo->angle;

            // sf: despite killough's infinite wisdom.. even
            // he is prone to mistakes. seems negative numbers
            // won't survive a bitshift!
            an2 += slope < 0 ? -tantoangle[-slope >> DBITS] :
                                tantoangle[slope >> DBITS];
        }
        else
        {
            slope = bfglook == 1 ? player->lookdir * LOOKSLOPE : 0;
            an2 += slope < 0 ? -tantoangle[-slope >> DBITS] :
                                tantoangle[slope >> DBITS];
        }

        th = P_SpawnMobj(mo->x, mo->y,
                         mo->z + 62 * FRACUNIT - player->psprites[ps_weapon].sy,
                         type);

        P_SetTarget(&th->target, mo);

        th->angle = an1;

        th->momx = finecosine[an1 >> ANGLETOFINESHIFT] * 25;
        th->momy = finesine[an1 >> ANGLETOFINESHIFT] * 25;
        th->momz = finetangent[an2 >> ANGLETOFINESHIFT] * 25;

        P_CheckMissileSpawn(th);
    }

    // killough: obfuscated!
    while (type != MT_PLASMA2 && (type = MT_PLASMA2));
}

//
// A_FirePlasma
//
void A_FirePlasma(player_t *player, pspdef_t *psp) 
{
    if (!d_infiniteammo)
        DecreaseAmmo(player, weaponinfo[player->readyweapon].ammo, 1);

    if (beta_style)
    {
        P_SetPsprite(player, ps_flash, S_BETAPLASMAFLASH1);

        if (beta_plasma_refired && (beta_plasma_counter % 2))
        {
            beta_plasma_counter = 1;
            P_SpawnPlayerMissile(player->mo, MT_PLASMA2);
        }
        else
        {
            P_SpawnPlayerMissile(player->mo, MT_PLASMA1);
            beta_plasma_counter = 0;
        }

        if (player->attackdown)
            beta_plasma_counter++;
    }
    else
    {
        P_SetPsprite(player, ps_flash, weaponinfo[player->readyweapon].flashstate + (P_Random() & 1));

        P_SpawnPlayerMissile(player->mo, MT_PLASMA);
    }

    A_Recoil(player);
}

//
// P_BulletSlope
// Sets a slope so a near miss is at aproximately
// the height of the intended target
//
void P_BulletSlope(mobj_t *mo)
{
    // see which target is to be aimed at
    angle_t an = mo->angle;

    if (mo->player && !autoaim)
    {
        bulletslope = (mo->player->lookdir << FRACBITS) / 173;

        if (linetarget)
            mo->target = linetarget;

        return;
    }

    bulletslope = P_AimLineAttack(mo, an, 16 * 64 * FRACUNIT);

    if (!linetarget)
    {
        an += 1 << 26;
        bulletslope = P_AimLineAttack(mo, an, 16 * 64 * FRACUNIT);

        if (!linetarget)
        {
            an -= 2 << 26;
            bulletslope = P_AimLineAttack(mo, an, 16 * 64 * FRACUNIT);
        }

        if (!linetarget)
        {
            bulletslope = (mo->player->lookdir << FRACBITS) / 173;
        }
    }
}

//
// P_GunShot
//
void P_GunShot(mobj_t *mo, dboolean accurate)
{
    angle_t      angle = mo->angle;
    int          damage = 5 * (P_Random() % 3 + 1);

    if (!aiming_help)
    {
        if (!accurate)
            angle += (P_Random() - P_Random()) << 18;
    }

    P_LineAttack(mo, angle, MISSILERANGE, bulletslope, damage);
}

//
// A_FirePistol
//
void A_FirePistol(player_t *player, pspdef_t *psp) 
{
    S_StartSound(player->mo, sfx_pistol);

    P_SetMobjState(player->mo, S_PLAY_ATK2);

    if (!d_infiniteammo)
        DecreaseAmmo(player, weaponinfo[player->readyweapon].ammo, 1);

    P_SetPsprite(player, ps_flash, weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope(player->mo);

    P_GunShot(player->mo, !player->refire);

    A_Recoil(player);
}

//
// A_FireShotgun
//
void A_FireShotgun(player_t *player, pspdef_t *psp) 
{
    int            i;
        
    S_StartSound(player->mo, sfx_shotgn);
    P_SetMobjState(player->mo, S_PLAY_ATK2);

    if (!d_infiniteammo)
        DecreaseAmmo(player, weaponinfo[player->readyweapon].ammo, 1);

    P_SetPsprite(player, ps_flash, weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope(player->mo);
        
    for (i = 0; i < 7; i++)
        P_GunShot(player->mo, false);

    A_Recoil(player);
}

//
// A_FireShotgun2
//
void A_FireShotgun2(player_t *player, pspdef_t *psp) 
{
    int            i;
        
    S_StartSound(player->mo, sfx_dshtgn);
    P_SetMobjState(player->mo, S_PLAY_ATK2);

    if (!d_infiniteammo)
        DecreaseAmmo(player, weaponinfo[player->readyweapon].ammo, 2);

    P_SetPsprite(player, ps_flash, weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope(player->mo);
        
    for (i = 0; i < 20; i++)
    {
        int        damage = 5 * (P_Random() % 3 + 1);
        angle_t    angle = player->mo->angle;

        angle += (P_Random() - P_Random()) << ANGLETOFINESHIFT;

        P_LineAttack(player->mo,
                     angle,
                     MISSILERANGE,
                     bulletslope + ((P_Random() - P_Random()) << 5), damage);
    }

    A_Recoil(player);
}

void A_OpenShotgun2(player_t *player, pspdef_t *psp)
{
    S_StartSound(player->mo, sfx_dbopn);
}

void A_LoadShotgun2(player_t *player, pspdef_t *psp)
{
    S_StartSound(player->mo, sfx_dbload);
}

void A_CloseShotgun2(player_t *player, pspdef_t *psp)
{
    S_StartSound(player->mo, sfx_dbcls);
    A_ReFire(player, psp);
}

//
// A_FireCGun
//
void A_FireCGun(player_t *player, pspdef_t *psp) 
{
    if (player->ammo[weaponinfo[player->readyweapon].ammo] || d_sound)
        S_StartSound(player->mo, sfx_pistol);

    if (!player->ammo[weaponinfo[player->readyweapon].ammo])
        return;
                
    P_SetMobjState(player->mo, S_PLAY_ATK2);

    if (!d_infiniteammo)
        DecreaseAmmo(player, weaponinfo[player->readyweapon].ammo, 1);

    if (player->readyweapon == wp_chaingun)
    {
        if (beta_style)
            P_SetPsprite(player, ps_flash, S_BETACHAINFLASH1 + psp->state - &states[S_BETACHAIN1]);
        else
            P_SetPsprite(player, ps_flash, weaponinfo[player->readyweapon].flashstate
                + (unsigned int)((psp->state - &states[S_CHAIN1]) & 1));
    }

    psp->state->tics = chaingun_tics;

    P_BulletSlope(player->mo);
        
    P_GunShot(player->mo, !player->refire);

    A_Recoil(player);
}

//
// ?
//
void A_Light0(player_t *player, pspdef_t *psp)
{
    player->extralight = 0;
}

void A_Light1(player_t *player, pspdef_t *psp)
{
    player->extralight = 1;
}

void A_Light2(player_t *player, pspdef_t *psp)
{
    player->extralight = 2;
}

//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//
void A_BFGSpray(mobj_t *mo) 
{
    int          i;
    int          j;
    int          damage;
        
    // offset angles from its attack angle
    for (i = 0; i < 40; i++)
    {
        angle_t an = mo->angle - ANG90 / 2 + ANG90 / 40 * i;

        // mo->target is the originator (player)
        //  of the missile
        P_AimLineAttack(mo->target, an, 16 * 64 * FRACUNIT);

        if (!linetarget)
            continue;

        P_SpawnMobj(linetarget->x,
                    linetarget->y,
                    linetarget->z + (linetarget->height >> 2),
                    MT_EXTRABFG);
        
        damage = 0;

        for (j = 0; j < 15; j++)
            damage += (P_Random() & 7) + 1;

        P_DamageMobj(linetarget, mo->target, mo->target, damage);
    }
}

//
// A_BFGsound
//
void A_BFGsound(player_t *player, pspdef_t *psp)
{
    if (fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
        fsize != 4207819 && fsize != 4225460 && fsize != 4274218 &&
        fsize != 4225504 && fsize != 4196020 && fsize != 4234124)
        S_StartSound(player->mo, sfx_bfg);
}

//
// P_SetupPsprites
// Called at start of level for each player.
//
void P_SetupPsprites(player_t *player) 
{
    int        i;
        
    // remove all psprites
    for (i = 0; i < NUMPSPRITES; i++)
        player->psprites[i].state = NULL;
                
    // spawn the gun
    player->pendingweapon = player->readyweapon;
    P_BringUpWeapon(player);
    skippsprinterp = true;
}

//
// P_MovePsprites
// Called every tic by player thinking routine.
//
void P_MovePsprites(player_t *player) 
{
    int          i;
    pspdef_t     *psp = &player->psprites[0];

    for (i = 0; i < NUMPSPRITES; i++, psp++)
    {
        state_t *state;

        // a null state means not active
        // careful: parentheses!!
        if ((state = psp->state))
        {
            // drop tic count and possibly change state
            // a -1 tic count never changes
            if (psp->tics != -1)        
            {
                psp->tics--;

                if (!psp->tics)
                    P_SetPsprite(player, i, psp->state->nextstate);
            }
        }
    }
    
    player->psprites[ps_flash].sx = player->psprites[ps_weapon].sx;
    player->psprites[ps_flash].sy = player->psprites[ps_weapon].sy;
}

