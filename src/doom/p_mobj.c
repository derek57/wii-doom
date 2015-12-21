// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
//
// Copyright(C) 2015 by Brad Harding: - (Liquid Sector Animations)
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
//        Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#ifdef WII
#include "../c_io.h"
#else
#include "c_io.h"
#endif

#include "doomdef.h"

#ifdef WII
#include "../doomfeatures.h"
#else
#include "doomfeatures.h"
#endif

#include "doomstat.h"
#include "hu_stuff.h"

#ifdef WII
#include "../i_system.h"
#else
#include "i_system.h"
#endif

#include "m_random.h"
#include "p_local.h"
#include "p_tick.h"
#include "r_things.h"
#include "s_sound.h"
#include "sounds.h"
#include "st_stuff.h"

#ifdef WII
#include "../v_trans.h"
#include "../z_zone.h"
#else
#include "v_trans.h"
#include "z_zone.h"
#endif


#define STOPSPEED               0x1000
#define FRICTION_FLY            0xeb00
#define WATERFRICTION           0xfb00


static fixed_t floatbobdiffs[64] =
{
     25695,  25695,  25447,  24955,  24222,  23256,  22066,  20663,
     19062,  17277,  15325,  13226,  10999,   8667,   6251,   3775,
      1262,  -1262,  -3775,  -6251,  -8667, -10999, -13226, -15325,
    -17277, -19062, -20663, -22066, -23256, -24222, -24955, -25447,
    -25695, -25695, -25447, -24955, -24222, -23256, -22066, -20663,
    -19062, -17277, -15325, -13226, -11000,  -8667,  -6251,  -3775,
     -1262,   1262,   3775,   6251,   8667,  10999,  13226,  15325,
     17277,  19062,  20663,  22066,  23256,  24222,  24955,  25447
};

void G_PlayerReborn (int player);
//void P_SpawnMapThing (mapthing_t* mthing);

extern dboolean     not_walking;
extern dboolean     in_slime;

extern fixed_t      animatedliquiddiffs[64];
extern fixed_t      attackrange;

int                 itemrespawntime[ITEMQUESIZE];
int                 iquehead;
int                 iquetail;
int                 puffcount;
int                 correct_lost_soul_bounce;
int                 r_blood = r_blood_default;
int                 r_bloodsplats_max = r_bloodsplats_max_default;
int                 r_bloodsplats_total;

mobj_t              *bloodsplats[r_bloodsplats_max_max];

mapthing_t          itemrespawnque[ITEMQUESIZE];

void                (*P_BloodSplatSpawner)(fixed_t, fixed_t, int, int, mobj_t *);


dboolean P_IsVoodooDoll(mobj_t *mobj)
{
    return (mobj->player && mobj->player->mo != mobj);
}

//
// P_SetMobjState
// Returns true if the mobj is still present.
//
dboolean P_SetMobjState(mobj_t *mobj, statenum_t state)
{
    state_t             *st;

    // killough 4/9/98: remember states seen, to detect cycles:
    static statenum_t   seenstate_tab[NUMSTATES];               // fast transition table
    statenum_t          *seenstate = seenstate_tab;             // pointer to table
    static int          recursion;                              // detects recursion
    statenum_t          i = state;                              // initial state
    dboolean            ret = true;                             // return value
    statenum_t          tempstate[NUMSTATES];                   // for use with recursion
    mobj_t              *shadow = mobj->shadow;

    if (recursion++)                                            // if recursion detected,
        memset((seenstate = tempstate), 0, sizeof(tempstate));  // clear state table

    do
    {
        if (state == S_NULL)
        {
            mobj->state = (state_t *) S_NULL;

            // [BH] Remove mobj's shadow if it has one 
            if (shadow)
                P_RemoveMobjShadow(mobj);

            P_RemoveMobj (mobj);
            ret = false;
            break;                                              // killough 4/9/98
        }

        st = &states[state];
        mobj->state = st;
        mobj->tics = st->tics;
        mobj->sprite = st->sprite;
        mobj->frame = st->frame;

        // Modified handling.
        // Call action functions when the state is set
        if (st->action)                
            st->action(mobj);        
        
        seenstate[state] = 1 + st->nextstate;                   // killough 4/9/98

        state = st->nextstate;
    } while (!mobj->tics && !seenstate[state]);                 // killough 4/9/98

    if (!--recursion)
        for (; (state = seenstate[i]); i = state - 1)
            seenstate[i] = 0;                           // killough 4/9/98: erase memory of states

    // [BH] Use same sprite frame for shadow as mobj 
    if (ret && shadow)
    {
        shadow->sprite = mobj->sprite;
        shadow->frame = mobj->frame;
        shadow->angle = mobj->angle;
    }

    return ret;
}

//
// P_ExplodeMissile  
//
void P_ExplodeMissile (mobj_t* mo)
{
    mo->momx = mo->momy = mo->momz = 0;

    if(beta_style && mobjinfo[mo->type].deathstate == S_EXPLODE1)
        P_SetMobjState (mo, S_BETAEXPLODE1);
    else
        P_SetMobjState (mo, mobjinfo[mo->type].deathstate);

    mo->tics = MAX(1, mo->tics - (P_Random() & 3));

    mo->flags &= ~MF_MISSILE;

    // [BH] make explosion translucent, remove shadow 
    if (mo->type == MT_ROCKET)
    {
        mo->colfunc = tlcolfunc;
        if (mo->shadow)
            P_RemoveMobjShadow(mo);
    }

    // [crispy] missile explosions are translucent
    mo->flags |= MF_TRANSLUCENT;

    if (mo->info->deathsound)
        S_StartSound (mo, mo->info->deathsound);
}

//
// P_XYMovement  
//
void P_XYMovement (mobj_t* mo) 
{         
    fixed_t        xmove;
    fixed_t        ymove;
    player_t*      player;
    dboolean        corpse = ((mo->flags & MF_CORPSE) && mo->type != MT_BARREL);

    if (!(mo->momx | mo->momy))
    {
        if (mo->flags & MF_SKULLFLY)
        {
            // the skull slammed into something
            mo->flags &= ~MF_SKULLFLY;
            mo->momz = 0;
            P_SetMobjState (mo, mo->info->spawnstate);
        }
        return;
    }
        
    player = mo->player;

    // [BH] give smoke trails to rockets 
    if (mo->flags2 & MF2_SMOKETRAIL)
        if (puffcount++ > 1)
            P_SpawnSmokeTrail(mo->x, mo->y, mo->z, mo->angle);

    mo->momx = BETWEEN(-MAXMOVE, mo->momx, MAXMOVE);
    mo->momy = BETWEEN(-MAXMOVE, mo->momy, MAXMOVE);
                
    xmove = mo->momx;
    ymove = mo->momy;
        
    do
    {
        fixed_t ptryx, ptryy;

        // killough 8/9/98: fix bug in original DOOM source:
        // Large negative displacements were never considered.
        // This explains the tendency for Mancubus fireballs
        // to pass through walls.
        if (xmove > MAXMOVE / 2 || ymove > MAXMOVE / 2 ||
                (!d_moveblock && (xmove < -MAXMOVE/2 || ymove < -MAXMOVE/2)))
        {
            ptryx = mo->x + xmove/2;
            ptryy = mo->y + ymove/2;
            xmove >>= 1;
            ymove >>= 1;
        }
        else
        {
            ptryx = mo->x + xmove;
            ptryy = mo->y + ymove;
            xmove = ymove = 0;
        }

        // killough 3/15/98: Allow objects to drop off
        if (!P_TryMove(mo, ptryx, ptryy, true))
        {
            // blocked move
            // killough 8/11/98: bouncing off walls
            // killough 10/98:
            // Add ability for objects other than players to bounce on ice
            if (!(mo->flags & MF_MISSILE) && !player && blockline && mo->z <= mo->floorz
                && P_GetFriction(mo, NULL) > ORIG_FRICTION)
            {
                if (blockline)
                {
                    fixed_t     r = ((blockline->dx >> FRACBITS) * mo->momx
                        + (blockline->dy >> FRACBITS) * mo->momy)
                        / ((blockline->dx >> FRACBITS) * (blockline->dx >> FRACBITS)
                        + (blockline->dy >> FRACBITS) * (blockline->dy >> FRACBITS));
                    fixed_t     x = FixedMul(r, blockline->dx);
                    fixed_t     y = FixedMul(r, blockline->dy);

                    // reflect momentum away from wall
                    mo->momx = x * 2 - mo->momx;
                    mo->momy = y * 2 - mo->momy;

                    // if under gravity, slow down in
                    // direction perpendicular to wall.
                    if (!(mo->flags & MF_NOGRAVITY))
                    {
                        mo->momx = (mo->momx + x) / 2;
                        mo->momy = (mo->momy + y) / 2;
                    }
                }
                else
                    mo->momx = mo->momy = 0;
            }
            else if (player)
            {        // try to slide along it
                not_walking = true;
                P_SlideMove (mo);
            }
            else if (mo->flags & MF_MISSILE)
            {
                // explode a missile
                if (ceilingline
                    && ceilingline->backsector
                    && ceilingline->backsector->ceilingpic == skyflatnum
                    && mo->z > ceilingline->backsector->ceilingheight)
                {
                    // Hack to prevent missiles exploding
                    // against the sky.
                    // Does not handle sky floors.
                    if (mo->type == MT_BFG)
                        // [BH] still play sound when firing BFG into sky 
                        S_StartSound(mo, mo->info->deathsound);
                    else if (mo->type == MT_ROCKET && mo->shadow)
                        // [BH] remove shadow from rockets 
                        P_RemoveMobjShadow(mo);
                    P_RemoveMobj(mo);
                    return;
                }
                P_ExplodeMissile (mo);
            }
            else
                mo->momx = mo->momy = 0;
        }
    } while (xmove | ymove);
    
    if (mo->flags & (MF_MISSILE | MF_SKULLFLY))
        return;         // no friction for missiles or lost souls ever
                
    if (mo->z > mo->floorz && !(mo->flags2 & MF2_FLY) && !(mo->flags2 & MF2_ONMOBJ))
        return;         // no friction when airborne

    // [BH] spawn random blood splats on floor as corpses slide 
    if (corpse && !(mo->flags & MF_NOBLOOD) && corpses_slide && (mo->momx || mo->momy) &&
                    corpses_smearblood && mo->bloodsplats && r_bloodsplats_max && !mo->nudge)
    {
        int     radius = (spritewidth[sprites[mo->sprite].spriteframes[0].lump[0]] >> FRACBITS) >> 1;
        int     i;
        int     max = MIN((ABS(mo->momx) + ABS(mo->momy)) >> (FRACBITS - 2), 8);
        int     x = mo->x;
        int     y = mo->y;
        int     blood;
        int     floorz = mo->floorz;

        if(d_colblood && d_chkblood)
            blood = mobjinfo[mo->blood].blood;
        else
            blood = REDBLOOD;

        for (i = 0; i < max; i++)
        {
            int fx, fy;

            if (!mo->bloodsplats)
                break;

            fx = x + (M_RandomInt(-radius, radius) << FRACBITS);
            fy = y + (M_RandomInt(-radius, radius) << FRACBITS);

            if (floorz == R_PointInSubsector(x, y)->sector->floorheight)
                P_BloodSplatSpawner(fx, fy, blood, floorz, mo);
        }
    }

    if ((corpse || (mo->flags2 & MF2_FALLING))
        && (mo->momx > FRACUNIT / 4 || mo->momx < -FRACUNIT / 4
            || mo->momy > FRACUNIT / 4 || mo->momy < -FRACUNIT / 4)
        && mo->floorz != mo->subsector->sector->floorheight)
        return;         // do not stop sliding if halfway off a step with some momentum

    if (mo->momx > -STOPSPEED && mo->momx < STOPSPEED
        && mo->momy > -STOPSPEED && mo->momy < STOPSPEED
        && (!player || (!player->cmd.forwardmov && !player->cmd.sidemov) || P_IsVoodooDoll(mo)))
    {
        // if in a walking frame, stop moving
        if (player && !P_IsVoodooDoll(mo)
            && (unsigned int)((player->mo->state - states) - S_PLAY_RUN1) < 4)
            P_SetMobjState(player->mo, S_PLAY);

        mo->momx = mo->momy = 0;

        // killough 10/98: kill any bobbing momentum too (except in voodoo dolls)
        if (player && player->mo == mo)
            player->momx = player->momy = 0;
    }
    else if ((mo->flags2 & MF2_FEETARECLIPPED) && corpse && !player)
    {
        // [BH] reduce friction for corpses in water 
        mo->momx = FixedMul(mo->momx, WATERFRICTION);
        mo->momy = FixedMul(mo->momy, WATERFRICTION);
    }
    else
    {
        // phares 3/17/98
        //
        // Friction will have been adjusted by friction thinkers for
        // icy or muddy floors. Otherwise it was never touched and
        // remained set at ORIG_FRICTION
        //
        // killough 8/28/98: removed inefficient thinker algorithm,
        // instead using touching_sectorlist in P_GetFriction() to
        // determine friction (and thus only when it is needed).
        //
        // killough 10/98: changed to work with new bobbing method.
        // Reducing player momentum is no longer needed to reduce
        // bobbing, so ice works much better now.
        fixed_t friction = P_GetFriction(mo, NULL);

        mo->momx = FixedMul(mo->momx, friction);
        mo->momy = FixedMul(mo->momy, friction);

        // killough 10/98: Always decrease player bobbing by ORIG_FRICTION.
        // This prevents problems with bobbing on ice, where it was not being
        // reduced fast enough, leading to all sorts of kludges being developed.
        if (player && player->mo == mo)     //  Not voodoo dolls
        {
            player->momx = FixedMul(player->momx, ORIG_FRICTION);
            player->momy = FixedMul(player->momy, ORIG_FRICTION);
        }
        else if ((mo->flags2 & MF2_FLY) && !(mo->z <= mo->floorz) && !(mo->flags2 & MF2_ONMOBJ))
        {
            mo->momx = FixedMul(mo->momx, FRICTION_FLY);
            mo->momy = FixedMul(mo->momy, FRICTION_FLY);
        }
    }
}

//
// P_ZMovement
//
void P_ZMovement (mobj_t* mo)
{
    // check for smooth step up
    // killough 5/12/98: exclude voodoo dolls
    if (mo->player && mo->player->mo == mo && mo->z < mo->floorz)
    {
        mo->player->viewheight -= mo->floorz - mo->z;

        mo->player->deltaviewheight = (VIEWHEIGHT - mo->player->viewheight) >> 3;
    }
    
    // adjust height
    mo->z += mo->momz;
        
    // float down towards target if too close
    if (!((mo->flags ^ MF_FLOAT) & (MF_FLOAT | MF_SKULLFLY | MF_INFLOAT))
        && mo->target)  // killough 11/98: simplify
    {
        fixed_t     delta = (mo->target->z + (mo->height >> 1) - mo->z) * 3;
 
        if (P_AproxDistance(mo->x - mo->target->x, mo->y - mo->target->y) < ABS(delta))
            mo->z += (delta < 0 ? -FLOATSPEED : FLOATSPEED);
    }
    
    if (mo->player && (mo->flags2 & MF2_FLY) && !(mo->z <= mo->floorz)
        && (leveltime & 2))
    {
        mo->z += finesine[(FINEANGLES / 20 * leveltime >> 2) & FINEMASK];
    }

    // clip movement
    if (mo->z <= mo->floorz)
    {
        // [BH] remove blood the moment it hits the ground
        //  and spawn a blood splat in its place
        if (mo->flags2 & MF2_BLOOD && d_maxgore)
        {
            P_RemoveMobj(mo);
            if (r_bloodsplats_max)
                P_BloodSplatSpawner(mo->x, mo->y, mo->blood, mo->floorz, NULL);
            return;
        }

        // hit the floor

        // Note (id):
        //  somebody left this after the setting momz to 0,
        //  kinda useless there.
        //
        // cph - This was the a bug in the linuxdoom-1.10 source which
        //  caused it not to sync Doom 2 v1.9 demos. Someone
        //  added the above comment and moved up the following code. So
        //  demos would desync in close lost soul fights.
        // Note that this only applies to original Doom 1 or Doom2 demos - not
        //  Final Doom and Ultimate Doom.  So we test demo_compatibility *and*
        //  gamemission. (Note we assume that Doom1 is always Ult Doom, which
        //  seems to hold for most published demos.)
        //  
        //  fraggle - cph got the logic here slightly wrong.  There are three
        //  versions of Doom 1.9:
        //
        //  * The version used in registered doom 1.9 + doom2 - no bounce
        //  * The version used in ultimate doom - has bounce
        //  * The version used in final doom - has bounce
        //
        // So we need to check that this is either retail or commercial
        // (but not doom2)
        
//        int correct_lost_soul_bounce = gameversion >= exe_ultimate;

        if (correct_lost_soul_bounce && (mo->flags & MF_SKULLFLY))
        {
            // the skull slammed into something
            mo->momz = -mo->momz;
        }
        
        if (mo->momz < 0)
        {
            if (jumping)
            {
                if (mo->player)
                    mo->player->jumpTics = 7;       // delay any jumping for a short time
            }

            if (mo->player && mo->momz < -GRAVITY * 8 && !(mo->flags2 & MF2_FLY))        
            {
                // Squat down.
                // Decrease viewheight for a moment
                // after hitting the ground (hard),
                // and utter appropriate sound.
                mo->player->deltaviewheight = mo->momz >> 3;

                if(P_HitFloor(mo) == 0)
                    S_StartSound (mo, sfx_oof);

                // FIXME: write an exception here for monsters / player falling
                // from high heights onto the ground (spawn blood & play sound)

                if (mouselook && !demorecording && !demoplayback)
                {
                    mo->player->centering = false;
                }
                else
                {
                    mo->player->centering = true;
                }
            }
            mo->momz = 0;
        }
        mo->z = mo->floorz;

        if (mo->z - mo->momz > mo->floorz)
        {                       // Spawn splashes, etc.
            if(d_splash)
                P_HitFloor(mo);
        }

        // cph 2001/05/26 -
        // See lost soul bouncing comment above. We need this here for bug
        // compatibility with original Doom2 v1.9 - if a soul is charging and
        // hit by a raising floor this incorrectly reverses its Y momentum.
        //

        if (!correct_lost_soul_bounce && (mo->flags & MF_SKULLFLY))
            mo->momz = -mo->momz;

        if (!((mo->flags ^ MF_MISSILE) & (MF_MISSILE | MF_NOCLIP)))
        {
            P_ExplodeMissile (mo);
            return;
        }
    }
    else if(beta_style && mo->type == MT_BLOOD)
        mo->momz = -(GRAVITY >> 3) * 10;
    else if (mo->flags2 & MF2_LOGRAV)
    {
        if (mo->momz == 0)
            mo->momz = -(GRAVITY >> 3) * 2;
        else
            mo->momz -= GRAVITY >> 3;
    }
    else if (! (mo->flags & MF_NOGRAVITY) )
    {
        if (mo->momz == 0)
            mo->momz = -GRAVITY;
        mo->momz -= GRAVITY;
    }
        
    if (mo->z + mo->height > mo->ceilingz)
    {
        if (mo->flags & MF_SKULLFLY)
        {        // the skull slammed into something
            mo->momz = -mo->momz;
        }
        
        // hit the ceiling
        if (mo->momz > 0)
            mo->momz = 0;

        mo->z = mo->ceilingz - mo->height;

        if (!((mo->flags ^ MF_MISSILE) & (MF_MISSILE | MF_NOCLIP)))
        {
            P_ExplodeMissile (mo);
            return;
        }
    }
} 



//
// P_NightmareRespawn
//
void P_NightmareRespawn(mobj_t *mobj)
{
    fixed_t                x;
    fixed_t                y;
    fixed_t                z; 
    subsector_t*           ss; 
    mobj_t*                mo;
    mapthing_t*            mthing;
                
    x = mobj->spawnpoint.x << FRACBITS; 
    y = mobj->spawnpoint.y << FRACBITS; 

    // somthing is occupying it's position?
    if (!P_CheckPosition (mobj, x, y) ) 
        return;        // no respwan

    // spawn a teleport fog at old spot
    // because of removal of the body?
    mo = P_SpawnMobj (mobj->x,
                      mobj->y,
                      mobj->subsector->sector->floorheight , MT_TFOG); 
    // initiate teleport sound
    S_StartSound (mo, sfx_telept);

    // spawn a teleport fog at the new spot
    ss = R_PointInSubsector (x,y); 

    mo = P_SpawnMobj (x, y, ss->sector->floorheight , MT_TFOG); 

    S_StartSound (mo, sfx_telept);

    // spawn the new monster
    mthing = &mobj->spawnpoint;
        
    // spawn it
    if (mobj->info->flags & MF_SPAWNCEILING)
        z = ONCEILINGZ;
    else
        z = ONFLOORZ;

    // inherit attributes from deceased one
    mo = P_SpawnMobj (x,y,z, mobj->type);
    mo->spawnpoint = mobj->spawnpoint;        
    mo->angle = ANG45 * (mthing->angle/45);

    if (mthing->options & MTF_AMBUSH)
        mo->flags |= MF_AMBUSH;

    mo->reactiontime = 18;
        
    // remove the old monster,
    if (mobj->shadow)
        P_RemoveMobjShadow(mobj);

    P_RemoveMobj (mobj);
}

static void PlayerLandedOnThing(mobj_t *mo)
{
    mo->player->deltaviewheight = mo->momz >> 3;
    if (mo->momz < -23 * FRACUNIT)
        P_NoiseAlert(mo, mo);
}

//
// P_MobjThinker
//
void P_MobjThinker (mobj_t* mobj)
{
    int      flags2;
    sector_t *sector = mobj->subsector->sector;

    // [AM] Handle interpolation unless we're an active player.
    if (!(mobj->player && mobj == mobj->player->mo))
    {
        // Assume we can interpolate at the beginning
        // of the tic.
        mobj->interp = true;

        // Store starting position for mobj interpolation.
        mobj->oldx = mobj->x;
        mobj->oldy = mobj->y;
        mobj->oldz = mobj->z;
        mobj->oldangle = mobj->angle;
    }

    if (mobj->nudge > 0)
        mobj->nudge--;

    // momentum movement
    if (mobj->momx || mobj->momy || (mobj->flags & MF_SKULLFLY))
    {
        P_XYMovement (mobj);
        // FIXME: decent NOP/NULL/Nil function pointer please.
        if (mobj->thinker.function == P_RemoveThinkerDelayed)   // killough
            return;                // mobj was removed
    }

    // [BH] don't clip sprite if no longer in liquid 
    if (!isliquid[sector->floorpic])
        mobj->flags2 &= ~MF2_FEETARECLIPPED;

    flags2 = mobj->flags2;

    // [BH] bob objects in liquid 
    if ((flags2 & MF2_FEETARECLIPPED) && !(flags2 & MF2_NOLIQUIDBOB)
            && mobj->z <= sector->floorheight && !mobj->momz && d_swirl)
        mobj->z += animatedliquiddiffs[(mobj->floatbob + leveltime) & 63];
    // [BH] otherwise bob certain powerups 
    else if ((flags2 & MF2_FLOATBOB) && float_items)
        mobj->z = BETWEEN(mobj->floorz, mobj->z + floatbobdiffs[(mobj->floatbob + leveltime) & 63],
            mobj->ceilingz);
    else if ((mobj->z != mobj->floorz) || mobj->momz)
    {                           // Handle Z momentum and gravity
        if (flags2 & MF2_PASSMOBJ)
        {
            mobj_t *onmo;

            if (!(onmo = P_CheckOnmobj(mobj)))
            {
                P_ZMovement(mobj);
                mobj->flags2 &= ~MF2_ONMOBJ;
            }
            else
            {
                if (mobj->player)
                {
                    if (mobj->momz < -GRAVITY * 8)
                        PlayerLandedOnThing(mobj);
                    if (onmo->z + onmo->height - mobj->z <= 24 * FRACUNIT)
                    {
                        mobj->player->viewheight -= onmo->z + onmo->height - mobj->z;
                        mobj->player->deltaviewheight = (VIEWHEIGHT - mobj->player->viewheight) >> 3;
                        mobj->z = onmo->z + onmo->height;
                        mobj->flags2 |= MF2_ONMOBJ;
                    }
                    mobj->momz = 0;
                }
            }
        }
        else
            P_ZMovement(mobj);

        if (mobj->thinker.function == P_RemoveThinkerDelayed)   // killough
            return;             // mobj was removed
    }
    else if (!(mobj->momx | mobj->momy) && !sentient(mobj))
    {
        // killough 9/12/98: objects fall off ledges if they are hanging off
        // slightly push off of ledge if hanging more than halfway off
        // [RH] Be more restrictive to avoid pushing monsters/players down steps
        if (!(mobj->flags & MF_NOGRAVITY) && !(flags2 & MF2_FLOATBOB)
                && mobj->z > mobj->dropoffz && (mobj->health <= 0 || ((mobj->flags & MF_COUNTKILL)
                && mobj->z - mobj->dropoffz > 24 * FRACUNIT)))
            P_ApplyTorque(mobj);
        else
        {
            // Reset torque
            mobj->flags2 &= ~MF2_FALLING;
            mobj->gear = 0;
        }
    }
    
    // cycle through states,
    // calling action functions at transitions
    if (mobj->tics != -1)
    {
	mobj->tics--;

        if (beta_style && mobj->state->nextstate == S_ARM1)
            mobj->state->nextstate = S_ARM1A;

	// you can cycle through multiple states in a tic
	if (!mobj->tics)
	    if (!P_SetMobjState (mobj, mobj->state->nextstate) )
		return;		// freed itself
    }
    else
    {
        // check for nightmare respawn
        if (!(mobj->flags & MF_COUNTKILL))
            return;

        if (!respawnmonsters)
            return;

        mobj->movecount++;

        if (mobj->movecount < 12*TICRATE)
            return;

        if (leveltime & 31)
            return;

        if (P_Random() > 4)
            return;

        P_NightmareRespawn(mobj);
    }
}

//
// P_SpawnShadow
//
void P_SpawnShadow(mobj_t *actor)
{
    mobj_t      *mobj = Z_Calloc(1, sizeof(*mobj), PU_LEVEL, NULL); 

    mobj->type = MT_SHADOW;
    mobj->info = &mobjinfo[MT_SHADOW];
    mobj->x = actor->x;
    mobj->y = actor->y;

    mobj->sprite = actor->state->sprite;
    mobj->frame = actor->state->frame;

    mobj->flags2 = MF2_DONOTMAP;

    mobj->colfunc = (actor->type == MT_SHADOWS ? R_DrawSpectreShadowColumn :
        (d_translucency ? R_DrawShadowColumn : R_DrawSolidShadowColumn));
    mobj->projectfunc = R_ProjectShadow;

    P_SetThingPosition(mobj);

    actor->shadow = mobj;
    mobj->shadow = actor;
}

//
// P_SpawnMobj
//
mobj_t *P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type)
{
    mobj_t      *mobj = Z_Calloc(1, sizeof(*mobj), PU_LEVEL, NULL); 
    state_t     *st;
    mobjinfo_t  *info = &mobjinfo[type];
    sector_t    *sector;

    mobj->type = type;
    mobj->info = info;
    mobj->x = x;
    mobj->y = y;
    mobj->radius = info->radius;
    mobj->height = info->height;
    mobj->projectilepassheight = info->projectilepassheight;
    mobj->flags = info->flags;
    mobj->flags2 = info->flags2;
    mobj->name = info->name1;
    mobj->health = info->spawnhealth;

    if (gameskill != sk_nightmare)
        mobj->reactiontime = info->reactiontime;
    mobj->lastlook = P_Random () % MAXPLAYERS;

    // do not set the state with P_SetMobjState, because action routines can not be called yet
    st = &states[info->spawnstate];

    // [BH] initialize certain mobj's animations to random start frame
    // so groups of same mobjs are deliberately out of sync
    if (info->frames > 1)
    {
        int     frames = M_RandomInt(0, info->frames);
        int     i = 0;

        while (i++ < frames && st->nextstate != S_NULL)
            st = &states[st->nextstate];
    }

    mobj->state = st;
    mobj->tics = st->tics;
    mobj->sprite = st->sprite;
    mobj->frame = st->frame;
    mobj->colfunc = info->colfunc;
    mobj->projectfunc = R_ProjectSprite;
    mobj->blood = info->blood;

    // [BH] set random pitch for monster sounds when spawned 
    mobj->pitch = ((mobj->flags & MF_SHOOTABLE) ?
        NORM_PITCH + 16 - M_RandomInt(0, 16) : NORM_PITCH);

    // set subsector and/or block links
    P_SetThingPosition (mobj);

    sector = mobj->subsector->sector;
    mobj->dropoffz =           // killough 11/98: for tracking dropoffs
    mobj->floorz = sector->floorheight;
    mobj->ceilingz = sector->ceilingheight;

    // [BH] initialize bobbing powerups 
    if (float_items)
        mobj->floatbob = P_Random();

    mobj->z = (z == ONFLOORZ ? mobj->floorz :
              (z == ONCEILINGZ ? mobj->ceilingz - mobj->height : z));

    // [AM] Do not interpolate on spawn.
    mobj->interp = false;

    // [AM] Just in case interpolation is attempted...
    mobj->oldx = mobj->x;
    mobj->oldy = mobj->y;
    mobj->oldz = mobj->z;
    mobj->oldangle = mobj->angle;

    mobj->thinker.function = P_MobjThinker;
    P_AddThinker (&mobj->thinker);

    // [BH] spawn the mobj's shadow 
    if ((mobj->flags2 & MF2_SHADOW) && d_shadows)
        P_SpawnShadow(mobj);

    if (!(mobj->flags2 & MF2_NOFOOTCLIP) && isliquid[sector->floorpic] && sector->heightsec == -1)
        mobj->flags2 |= MF2_FEETARECLIPPED;

    return mobj;
}

//
// P_RemoveMobj
//
void P_RemoveMobj (mobj_t* mobj)
{
    if ((mobj->flags & MF_SPECIAL)
        && !(mobj->flags & MF_DROPPED)
        && (mobj->type != MT_BETAINS)
        && (mobj->type != MT_BETAINV)
        && (mobj->type != MT_INV)
        && (mobj->type != MT_INS))
    {
        itemrespawnque[iquehead] = mobj->spawnpoint;
        itemrespawntime[iquehead] = leveltime;
        iquehead = (iquehead+1)&(ITEMQUESIZE-1);

        // lose one off the end?
        if (iquehead == iquetail)
        {
            iquetail = (iquetail+1)&(ITEMQUESIZE-1);
        }
    }

    // unlink from sector and block lists
    P_UnsetThingPosition (mobj);
    
    // Delete all nodes on the current sector_list
    if (sector_list)
    {
        P_DelSeclist(sector_list);
        sector_list = NULL;
    }

    // stop any playing sound
    S_StopSound (mobj);
    
    mobj->flags |= (MF_NOSECTOR | MF_NOBLOCKMAP);

    P_SetTarget(&mobj->target, NULL);
    P_SetTarget(&mobj->tracer, NULL);

    if(last_enemy)
        P_SetTarget(&mobj->lastenemy, NULL);

    // free block
    P_RemoveThinker ((thinker_t*)mobj);
}

//
// P_RemoveMobjShadow
// [BH] Remove the shadow of a mobj
//
void P_RemoveMobjShadow(mobj_t *mobj)
{
    // unlink from sector and block lists
    P_UnsetThingPosition(mobj->shadow);

    // Delete all nodes on the current sector_list
    if (sector_list)
    {
        P_DelSeclist(sector_list);
        sector_list = NULL;
    }

    mobj->shadow = NULL;
}

//
// P_FindDoomedNum
// Finds a mobj type with a matching doomednum
// killough 8/24/98: rewrote to use hashing
//
mobjtype_t P_FindDoomedNum(unsigned int type)
{
    static struct
    {
        int     first;
        int     next;
    } *hash;

    mobjtype_t i;

    if (!hash)
    {
        hash = Z_Malloc(sizeof(*hash) * NUMMOBJTYPES, PU_CACHE, (void **)&hash);
        for (i = 0; i < NUMMOBJTYPES; i++)
            hash[i].first = NUMMOBJTYPES;
        for (i = 0; i < NUMMOBJTYPES; i++)
            if (mobjinfo[i].doomednum != -1)
            {
                unsigned int    h = (unsigned int)mobjinfo[i].doomednum % NUMMOBJTYPES;

                hash[i].next = hash[h].first;
                hash[h].first = i;
            }
    }

    i = hash[type % NUMMOBJTYPES].first;
    while ((i < NUMMOBJTYPES) && ((unsigned int)mobjinfo[i].doomednum != type))
        i = hash[i].next;
    return i;
}

//
// P_RespawnSpecials
//
void P_RespawnSpecials (void)
{
    fixed_t                x;
    fixed_t                y;
    fixed_t                z;
    
    subsector_t*           ss; 
    mobj_t*                mo;
    mapthing_t*            mthing;
    
    int                    i;

    // only respawn items in deathmatch

    if (deathmatch != 2)
        return;

    // nothing left to respawn?
    if (iquehead == iquetail)
        return;                

    // wait at least 30 seconds
    if (leveltime - itemrespawntime[iquetail] < 30*TICRATE)
        return;                        

    mthing = &itemrespawnque[iquetail];
        
    x = mthing->x << FRACBITS; 
    y = mthing->y << FRACBITS; 
          
    // spawn a teleport fog at the new spot
    ss = R_PointInSubsector (x,y); 
    mo = P_SpawnMobj (x, y, ss->sector->floorheight , MT_IFOG); 

    if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
            fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
        S_StartSound (mo, sfx_itmbk);

    // find which type to spawn
//    for (i=0 ; i< NUMMOBJTYPES; i++)
    for (i=0 ; i< NUMMOBJTYPES - 1 ; i++)
    {
        if (mthing->type == mobjinfo[i].doomednum)
            break;
    }
    
    // spawn it
    if (mobjinfo[i].flags & MF_SPAWNCEILING)
        z = ONCEILINGZ;
    else
        z = ONFLOORZ;

    mo = P_SpawnMobj (x,y,z, i);
    mo->spawnpoint = *mthing;        
    mo->angle = ANG45 * (mthing->angle/45);

    // pull it from the que
    iquetail = (iquetail+1)&(ITEMQUESIZE-1);
}

//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
//
void P_SpawnPlayer (mapthing_t* mthing)
{
    player_t    *p;
    fixed_t     x, y, z;
    mobj_t      *mobj;

    if (mthing->type == 0)
    {
        return;
    }

    // not playing?
    if (!playeringame[mthing->type-1])
        return;                                        
                
    p = &players[mthing->type-1];

    if (p->playerstate == PST_REBORN)
        G_PlayerReborn (mthing->type-1);

    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;
    z = ONFLOORZ;
    mobj = P_SpawnMobj(x, y, z, MT_PLAYER);

    // set color translations for player sprites
    if (mthing->type > 1)                
        mobj->flags |= (mthing->type-1) << MF_TRANSSHIFT;
                
    mobj->angle = ANG45 * (mthing->angle/45);
    mobj->player = p;
    mobj->health = p->health;

    p->mo = mobj;
    p->playerstate = PST_LIVE;        
    p->refire = 0;
    p->message = NULL;
    p->damagecount = 0;
    p->bonuscount = 0;
    p->extralight = 0;
    p->fixedcolormap = 0;
    p->viewheight = VIEWHEIGHT;
    p->recoilpitch = 0;

    p->viewz = p->mo->z + p->viewheight;
    p->psprites[ps_weapon].sx = 0;
    p->mo->momx = p->mo->momy = 0;
    p->momx = p->momy = 0;

    // setup gun psprite
    P_SetupPsprites (p);
    
    // give all cards in death match mode
    if (deathmatch)
    {
        int i;

        for (i=0 ; i<NUMCARDS ; i++)
            p->cards[i] = true;
    }

    if (mthing->type-1 == consoleplayer)
    {
        // wake up the status bar
        ST_Start ();
        // wake up the heads up text
        HU_Start ();                
    }
}

//
// P_SpawnMoreBlood
// [BH] Spawn blood splats around corpses
//
void P_SpawnMoreBlood(mobj_t *mobj)
{
    if(d_maxgore)
    {
        int radius = ((spritewidth[sprites[mobj->sprite].spriteframes[0].lump[0]] >> FRACBITS) >> 1)
                + 12;
        int i;
        int max = M_RandomInt(50, 100) + radius;
        int x = mobj->x;
        int y = mobj->y;
        int blood = mobjinfo[mobj->blood].blood;
        int floorz = mobj->floorz;

        if (!(mobj->flags & MF_SPAWNCEILING))
        {
            x += M_RandomInt(-radius / 3, radius / 3) << FRACBITS;
            y += M_RandomInt(-radius / 3, radius / 3) << FRACBITS;
        }

        for (i = 0; i < max; i++)
        {
            int     angle;
            int     fx, fy;

            if (!mobj->bloodsplats)
                break;

            angle = M_RandomInt(0, FINEANGLES - 1);
            fx = x + FixedMul(M_RandomInt(0, radius) << FRACBITS, finecosine[angle]);
            fy = y + FixedMul(M_RandomInt(0, radius) << FRACBITS, finesine[angle]);

            P_BloodSplatSpawner(fx, fy, blood, floorz, mobj);
        }
    }
}

//
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
void P_SpawnMapThing(mapthing_t *mthing, int index)
{
    int         i;
    int         bit;
    mobj_t      *mobj;
    fixed_t     x, y, z;
    short       type = mthing->type;
    int         flags;

    // count deathmatch start positions
    if (mthing->type == 11)
    {
        if (deathmatch_p < &deathmatchstarts[10])
        {
            memcpy (deathmatch_p, mthing, sizeof(*mthing));
            deathmatch_p++;
        }
        return;
    }

    if (mthing->type <= 0)
    {
        // Thing type 0 is actually "player -1 start".  
        // For some reason, Vanilla Doom accepts/ignores this.

        return;
    }

    // check for players specially
    if (mthing->type <= 4)
    {
        // save spots for respawning in network games
        playerstarts[mthing->type-1] = *mthing;
        if (!deathmatch)
            P_SpawnPlayer (mthing);

        return;
    }

    // check for apropriate skill level
    if (!netgame && (mthing->options & 16) )
        return;
                
    if (gameskill == sk_baby)
        bit = 1;
    else if (gameskill == sk_nightmare)
        bit = 4;
    else
        bit = 1<<(gameskill-1);

    if (!(mthing->options & bit) )
        return;
        
    // find which type to spawn

    // killough 8/23/98: use table for faster lookup
    i = P_FindDoomedNum(type);

    if (i == NUMMOBJTYPES)
    {
        // [BH] make unknown thing type non-fatal and show console warning instead 
        C_Warning("Thing %i at (%i,%i) has an unknown type of %i.",
            index, mthing->x, mthing->y, type);
        return;
    }
                
    // don't spawn keycards and players in deathmatch
    if (deathmatch && (mobjinfo[i].flags & MF_NOTDMATCH))
        return;

    if(beta_style)
    {
        if(i == MT_POSSESSED)
            i = MT_BETAPOSSESSED;

        if(i == MT_SHOTGUY)
            i = MT_BETASHOTGUY;

        if(i == MT_HEAD)
            i = MT_BETAHEAD;

        if(i == MT_MISC2)
            i = MT_DAGGER;

        if(i == MT_MISC3)
            i = MT_SKULLCHEST;

        if(i == MT_MISC7)
            i = MT_BETAYELLOWSKULLKEY;

        if(i == MT_MISC8)
            i = MT_BETAREDSKULLKEY;

        if(i == MT_MISC9)
            i = MT_BETABLUESKULLKEY;

        if(i == MT_MISC20)
            i = MT_BETACELL;

        if(i == MT_MISC22)
            i = MT_BETASHELL;

        if(i == MT_MISC32)
            i = MT_BETACOL1;

        if(i == MT_MISC23)
            i = MT_BETASHELLBOX;

        if(i == MT_MISC47)
            i = MT_BETASTALAGTITE;

        if(i == MT_MISC48)
            i = MT_BETAELEC;

        if(i == MT_MISC71)
            i = MT_BETAGIBS;

        if(i == MT_MISC72)
            i = MT_BETAHEADONASTICK;

        if(i == MT_MISC73)
            i = MT_BETAHEADCANDLES;

        if(i == MT_MISC74)
            i = MT_BETADEADSTICK;

        if(i == MT_MISC75)
            i = MT_BETALIVESTICK;

        if(i == MT_BARREL)
            i = MT_BETABARREL;

        if(i == MT_INS)
            i = MT_BETAINS;

        if(i == MT_INV)
            i = MT_BETAINV;

        if(i == MT_SKULL)
            i = MT_BETASKULL;

        if(i == MT_BRUISER)
            i = MT_BETABRUISER;
    }

    // don't spawn any monsters if -nomonsters
    if (nomonsters && (i == MT_SKULL || i == MT_BETASKULL || (mobjinfo[i].flags & MF_COUNTKILL)))
        return;
    
    // spawn it
    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;
    z = ((mobjinfo[i].flags & MF_SPAWNCEILING) ? ONCEILINGZ : ONFLOORZ);

    mobj = P_SpawnMobj (x,y,z, i);
    mobj->spawnpoint = *mthing;

    if (mthing->options & MTF_AMBUSH)
        mobj->flags |= MF_AMBUSH;

    flags = mobj->flags;

    if (mobj->tics > 0)
        mobj->tics = 1 + (P_Random () % mobj->tics);

    if (mobj->flags & MF_COUNTKILL)
        totalkills++;

    if (flags & MF_COUNTITEM)
        totalitems++;
                
    mobj->angle = ANG45 * (mthing->angle/45);

    if (mobj->shadow)
        mobj->shadow->angle = mobj->angle;

    if ((flags & MF_CORPSE) && d_flipcorpses)
    {
        static int      prev;
        int             r = M_RandomInt(1, 10);

        if (r <= 5 + prev)
        {
            prev--;
            mobj->flags2 |= MF2_MIRRORED;
            if (mobj->shadow)
                mobj->shadow->flags2 |= MF2_MIRRORED;
        }
        else
            prev++;
    }

    // [crispy] Lost Souls bleed Puffs
    if (d_colblood2 && d_chkblood2 && (i == MT_SKULL || i == MT_BETASKULL))
        mobj->flags |= MF_NOBLOOD;
    else if(!d_colblood2 && !d_chkblood2 && (i == MT_SKULL || i == MT_BETASKULL))
        mobj->flags &= ~MF_NOBLOOD;

    // RjY
    // Print a warning when a solid hanging body is used in a sector where
    // the player can walk under it, to help people with map debugging
    if (!((~mobj->flags) & (MF_SOLID | MF_SPAWNCEILING)) // solid and hanging
        // invert everything, then both bits should be clear
        && mobj->floorz + mobjinfo[MT_PLAYER].height <= mobj->z) // head height <= base
        // player under body's head height <= bottom of body
    {
        C_Warning("P_SpawnMapThing: solid hanging body in tall sector at %d,%d (%s)",
                mthing->x, mthing->y, mobj->name);
    }

    // [BH] randomly mirror corpses 
    if (d_flipcorpses && (type == SuperShotgun || (type >= Shotgun && type <= BFG9000))
            && (rand() & 1))
        mobj->flags2 |= MF2_MIRRORED;

    // [BH] Spawn blood splats around corpses 
    if (!(flags & (MF_SHOOTABLE | MF_NOBLOOD)) &&
            mobj->blood && fsize != 12361532 && r_bloodsplats_max)
    {
        mobj->bloodsplats = CORPSEBLOODSPLATS;
        if (d_maxgore)
            P_SpawnMoreBlood(mobj);
    }

    // [crispy] randomly colorize space marine corpse objects
    if (!netgame &&
        (randomly_colored_playercorpses) &&
        (mobj->info->spawnstate == S_PLAY_DIE7 ||
         mobj->info->spawnstate == S_PLAY_XDIE9))
    {
        mobj->flags |= (mobj->lastlook << MF_TRANSSHIFT);
    }
}

//
// GAME SPAWN FUNCTIONS
//

//
// P_SpawnPuff
//
mobj_t* P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z, angle_t angle)
{
    mobj_t      *th = P_SpawnMobj(x, y, z + ((P_Random() - P_Random()) << 10), MT_PUFF);

    th->momz = FRACUNIT;
    th->tics = MAX(1, th->tics - (P_Random() & 3));

    th->angle = angle;

    // don't make punches spark on the wall
    if (attackrange == MELEERANGE)
        P_SetMobjState (th, S_PUFF3);

    return th;
}

//
// P_SpawnSmokeTrail
//
void P_SpawnSmokeTrail(fixed_t x, fixed_t y, fixed_t z, angle_t angle)
{
    mobj_t      *th = P_SpawnMobj(x, y, z + ((P_Random() - P_Random()) << 10), MT_TRAIL);

    th->momz = FRACUNIT / 2;
    th->tics -= (P_Random() & 3);

    th->angle = angle;

    if(d_flipcorpses)
        th->flags2 |= (rand() & 1) * MF2_MIRRORED;
}

//
// P_SpawnBlood
// [BH] spawn much more blood than Vanilla DOOM 
// 
void P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, angle_t angle, int damage, mobj_t *target)
{
    int         i;
    int         minz = target->z;
    int         maxz = minz + spriteheight[sprites[target->sprite].spriteframes[0].lump[0]];
    int         color = ((d_chkblood && d_colblood) ? target->blood : MT_BLOOD);
    mobjinfo_t  *info = &mobjinfo[color];

    angle += ANG180;

    if(target->type != MT_BARREL && target->type != MT_BETABARREL)
    {
        int j;

        if(d_maxgore)
            j = (damage >> 2) + 1;
        else
            j = 1;

        for (i = j; i; i--)
        {
            mobj_t      *th = Z_Calloc(1, sizeof(*th), PU_LEVEL, NULL); 
            state_t     *st;

            if (d_colblood2 && d_chkblood2)
            {
                if(target->type == MT_SKULL || target->type == MT_BETASKULL)
                {
                    th->flags |= MF_NOBLOOD;
                    goto skip;
                }
            }

            th->type = color;
            th->info = info;
            th->x = x;
            th->y = y;
            th->flags = info->flags;

            if(d_flipcorpses)
                th->flags2 = (info->flags2 | (rand() & 1) * MF2_MIRRORED);
            else
                th->flags2 = (info->flags2);

            st = &states[info->spawnstate];

            th->state = st;
            th->tics = MAX(1, st->tics - (P_Random() & 3));
            th->sprite = st->sprite;
            th->frame = st->frame;

            th->colfunc = info->colfunc;
            th->projectfunc = R_ProjectSprite;
            th->blood = info->blood;

            // [crispy] connect blood object with the monster that bleeds it
            th->target = target;

            // [crispy] Spectres bleed spectre blood
            if (d_colblood2 && d_chkblood2)
            {
                if(target->type == MT_SHADOWS)
                    th->flags |= MF_SHADOW;
            }

            P_SetThingPosition(th);

            th->dropoffz = th->floorz = th->subsector->sector->floorheight;
            th->ceilingz = th->subsector->sector->ceilingheight;

            th->z = BETWEEN(minz, z + ((P_Random() - P_Random()) << 10), maxz);

            th->thinker.function = P_MobjThinker;
            P_AddThinker(&th->thinker);

            th->momx = FixedMul(i * FRACUNIT / 4, finecosine[angle >> ANGLETOFINESHIFT]);
            th->momy = FixedMul(i * FRACUNIT / 4, finesine[angle >> ANGLETOFINESHIFT]);
            th->momz = FRACUNIT * (2 + i / 6);

            th->angle = angle;
            angle += ((P_Random() - P_Random()) * 0xb60b60);

            if (damage <= 12 && th->state->nextstate)
                P_SetMobjState(th, th->state->nextstate);
            if (damage < 9 && th->state->nextstate)
                P_SetMobjState(th, th->state->nextstate);
/*
            // more blood and gore!
            if(d_maxgore)                // FIXME: MAYBE USE FOR EVEN MORE BLOOD???
            {
                int t;

                mobj_t *th2 = Z_Malloc(sizeof(*th2), PU_LEVEL, NULL);

                th2->type = color;
                th2 = P_SpawnMobj(x, y, z, MT_SPRAY);

                th2->colfunc = info->colfunc;
                th2->projectfunc = R_ProjectSprite;
                th2->blood = info->blood;

                // added for colored blood and gore!
                th2->target = target;

                th2->z = th->z;

                t = P_Random();
                th2->momx = (t - P_Random ()) << 10;
                t = P_Random();
                th2->momy = (t - P_Random ()) << 10;
                th2->momz = P_Random() << 10;

                // Spectres bleed spectre blood
                if (d_colblood2 && d_chkblood2)
                {
                    if(th2->target->type == MT_SHADOWS)
                        th2->flags |= MF_SHADOW;
                }

                P_SetMobjState(th2, S_BLOOD1 + (P_Random() % 2));
            }
*/
        }
        skip: ;
    }
}


//
// P_SpawnBloodSplat
//
void P_SpawnBloodSplat(fixed_t x, fixed_t y, int blood, int maxheight, mobj_t *target)
{
    subsector_t *subsec = R_PointInSubsector(x, y);
    sector_t    *sec = subsec->sector;
    short       floorpic = sec->floorpic;

    if (!isliquid[floorpic] && sec->floorheight <= maxheight && floorpic != skyflatnum)
    {
        mobj_t  *newsplat = Z_Calloc(1, sizeof(*newsplat), PU_LEVEL, NULL); 

        newsplat->type = MT_BLOODSPLAT;
        newsplat->sprite = SPR_BLD2;
        newsplat->frame = rand() & 7;

        if(d_flipcorpses)
            newsplat->flags2 = (MF2_DONOTMAP | (rand() & 1) * MF2_MIRRORED);
        else
            newsplat->flags2 = (MF2_DONOTMAP);

        if (blood == FUZZYBLOOD)
        {
            newsplat->flags = MF_SHADOW;
/*
            if (d_translucency)                // FIXME: Translucent blood for spectres is
            {                                // rendered gold on map E4M1 of "THE ULTIMATE DOOM".
                newsplat->colfunc = tlcolfunc;
            }
            else
*/
            {
                newsplat->colfunc = fuzzcolfunc;
            }
        }
        else
        {
            newsplat->colfunc = bloodsplatcolfunc;
        }

        newsplat->projectfunc = R_ProjectBloodSplat;
        newsplat->blood = blood;

        newsplat->x = x;
        newsplat->y = y;
        newsplat->subsector = subsec;
        P_SetBloodSplatPosition(newsplat);

        if (r_bloodsplats_total > r_bloodsplats_max)
        {
            mobj_t      *oldsplat = bloodsplats[r_bloodsplats_total % r_bloodsplats_max];

            if (oldsplat)
                P_UnsetThingPosition(oldsplat);
        }

        bloodsplats[r_bloodsplats_total++ % r_bloodsplats_max] = newsplat;

        if (target)
            target->bloodsplats = MAX(0, target->bloodsplats - 1);
    }
}

void P_NullBloodSplatSpawner(fixed_t x, fixed_t y, int blood, int maxheight, mobj_t *target) {}

//
// P_CheckMissileSpawn
// Moves the missile forward a bit
//  and possibly explodes it right there.
//
void P_CheckMissileSpawn (mobj_t* th)
{
    th->tics = MAX(1, th->tics - (P_Random() & 3));
    
    // move a little forward so an angle can
    // be computed if it immediately explodes
    th->x += (th->momx>>1);
    th->y += (th->momy>>1);
    th->z += (th->momz>>1);

    if (!P_TryMove(th, th->x, th->y, false))
        P_ExplodeMissile (th);
}

// Certain functions assume that a mobj_t pointer is non-NULL,
// causing a crash in some situations where it is NULL.  Vanilla
// Doom did not crash because of the lack of proper memory 
// protection. This function substitutes NULL pointers for
// pointers to a dummy mobj, to avoid a crash.

mobj_t *P_SubstNullMobj(mobj_t *mobj)
{
    if (mobj == NULL)
    {
        static mobj_t dummy_mobj;

        dummy_mobj.x = 0;
        dummy_mobj.y = 0;
        dummy_mobj.z = 0;
        dummy_mobj.flags = 0;

        mobj = &dummy_mobj;
    }

    return mobj;
}

//
// P_SpawnMissile
//
mobj_t *P_SpawnMissile(mobj_t *source, mobj_t *dest, mobjtype_t type)
{
    fixed_t     z;
    mobj_t      *th;
    angle_t     an;
    int         dist;
    int         speed;

    z = source->z + 4 * 8 * FRACUNIT;
    if ((source->flags2 & MF2_FEETARECLIPPED) && d_footclip &&
            source->subsector->sector->heightsec == -1)
        z -= FOOTCLIPSIZE;

    th = P_SpawnMobj(source->x, source->y, z, type);
    
    if (th->info->seesound)
        S_StartSound (th, th->info->seesound);

    P_SetTarget(&th->target, source);   // where it came from
    an = R_PointToAngle2 (source->x, source->y, dest->x, dest->y);

    // fuzzy player
    if (dest->flags & MF_SHADOW)
        an += (P_Random()-P_Random())<<20;

    th->angle = an;
    an >>= ANGLETOFINESHIFT;
    speed = th->info->speed;
    th->momx = FixedMul(speed, finecosine[an]);
    th->momy = FixedMul(speed, finesine[an]);
        
    dist = MAX(1, P_AproxDistance(dest->x - source->x, dest->y - source->y) / speed);

    th->momz = (dest->z - source->z) / dist;
    P_CheckMissileSpawn (th);
        
    return th;
}

//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
mobj_t* P_SpawnPlayerMissile(mobj_t *source, mobjtype_t type)
{
    mobj_t      *th;
    angle_t     an;
    fixed_t     x, y, z, aim;
    fixed_t     slope = 0;                // SHUT UP COMPILER
    
    // see which target is to be aimed at
    an = source->angle;
    if(autoaim) // single player autoaim toggle
    {
        slope = P_AimLineAttack (source, an, 16*64*FRACUNIT);

        if(!linetarget)
        {
            an += 1<<26;
            slope = P_AimLineAttack (source, an, 16*64*FRACUNIT);

            if(!linetarget)
            {
                an -= 2<<26;
                slope = P_AimLineAttack (source, an, 16*64*FRACUNIT);
            }

            if(!linetarget)
            {
                an = source->angle;
                // Removed, for look up/down support.
                //slope = 0; 
            }
        }

        if(linetarget)
            source->target = linetarget;
    }
    else
    {
        P_AimLineAttack(source, an, 16*64*FRACUNIT);

        if(linetarget)
            source->target = linetarget;

        linetarget = NULL;
    }
                
    x = source->x;
    y = source->y;
    z = source->z + 4 * 8 * FRACUNIT +
        ((source->player->lookdir) << FRACBITS) / 173;
        
    if ((source->flags2 & MF2_FEETARECLIPPED) && d_footclip &&
            source->subsector->sector->heightsec == -1)
        z -= FOOTCLIPSIZE;

    th = P_SpawnMobj (x,y,z, type);

    if (th->info->seesound)
        S_StartSound (th, th->info->seesound);

    if(!linetarget)
    {
        fixed_t pitch = (source->player->lookdir / 256);

        slope = (source->player->lookdir << FRACBITS) / 173;

        if(pitch < 0)
            pitch = pitch + FRACUNIT;
        else
            pitch = FRACUNIT - pitch;

        aim = FixedMul(th->info->speed, pitch);
    }
    else
        aim = th->info->speed;

    P_SetTarget(&th->target, source);
    th->angle = an;
    th->momx = FixedMul(aim, finecosine[an>>ANGLETOFINESHIFT]);
    th->momy = FixedMul(aim, finesine[an>>ANGLETOFINESHIFT]);
    th->momz = FixedMul(th->info->speed, slope);

    if (type == MT_ROCKET && smoketrails)
    {
        th->flags2 |= MF2_SMOKETRAIL;
        puffcount = 0;
    }

    P_CheckMissileSpawn (th);

    return th;
}

//
// P_GetThingFloorType
//
int P_GetThingFloorType(mobj_t * thing)
{
    return (thing->subsector->sector->floorpic);
}

//
// P_HitFloor
//
int P_HitFloor(mobj_t * thing)
{
    mobj_t *mo;

    if(thing->z > thing->subsector->sector->floorheight || !d_splash)
    {                           // don't splash if landing on the edge above water/lava/etc....
        return false;
    }

    switch (P_GetThingFloorType(thing))
    {
        case 69:
        case 70:
        case 71:
        case 72:
            P_SpawnMobj(thing->x, thing->y, ONFLOORZ, MT_SPLASHBASE);
            mo = P_SpawnMobj(thing->x, thing->y, ONFLOORZ, MT_SPLASH);
            mo->target = thing;
            mo->momx = (P_Random() - P_Random()) << 8;
            mo->momy = (P_Random() - P_Random()) << 8;
            mo->momz = 2 * FRACUNIT + (P_Random() << 8);

            if(!snd_module)
            {
                if(in_slime)
                    S_StartSound(mo, sfx_burn);
                else
                    S_StartSound(mo, sfx_gloop);
            }

            return true;
        case 73:
        case 74:
        case 75:
        case 76:
        case 89:
        case 90:
        case 91:
        case 144:
        case 145:
        case 146:
        case 147:
            P_SpawnMobj(thing->x, thing->y, ONFLOORZ, MT_LAVASPLASH);
            mo = P_SpawnMobj(thing->x, thing->y, ONFLOORZ, MT_LAVASMOKE);
            mo->momz = FRACUNIT + (P_Random() << 7);

            if(!snd_module)
            {
                if(in_slime)
                    S_StartSound(mo, sfx_burn);
                else
                    S_StartSound(mo, sfx_gloop);
            }

            return true;
        case 51:
        case 52:
        case 53:
        case 136:
        case 137:
        case 138:
        case 139:
        case 140:
        case 141:
        case 142:
        case 143:
            P_SpawnMobj(thing->x, thing->y, ONFLOORZ, MT_SLUDGESPLASH);
            mo = P_SpawnMobj(thing->x, thing->y, ONFLOORZ, MT_SLUDGECHUNK);
            mo->target = thing;
            mo->momx = (P_Random() - P_Random()) << 8;
            mo->momy = (P_Random() - P_Random()) << 8;
            mo->momz = FRACUNIT + (P_Random() << 8);

            if(!snd_module)
            {
                if(in_slime)
                    S_StartSound(mo, sfx_burn);
                else
                    S_StartSound(mo, sfx_gloop);
            }

            return true;
        default:
            return false;
    }
    return false;
}

