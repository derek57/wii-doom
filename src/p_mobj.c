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
//        Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "c_io.h"
#include "doomdef.h"
#include "doomstat.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "m_random.h"
#include "p_local.h"
#include "p_tick.h"
#include "s_sound.h"
#include "sounds.h"
#include "st_stuff.h"
#include "v_trans.h"
#include "z_zone.h"


#define STOPSPEED               0x1000
#define FRICTION                0xe800
#define FRICTION_FLY            0xeb00


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
void P_SpawnMapThing (mapthing_t* mthing);

extern int      mouselook;

extern boolean  not_walking;
extern boolean  in_slime;

extern fixed_t  animatedliquiddiffs[128];
extern fixed_t  attackrange;

int             test;
int             itemrespawntime[ITEMQUESIZE];
int             iquehead;
int             iquetail;
int             puffcount;

mapthing_t      itemrespawnque[ITEMQUESIZE];

//
// P_SetMobjState
// Returns true if the mobj is still present.
//
boolean
P_SetMobjState
( mobj_t*           mobj,
  statenum_t        state )
{
    state_t*        st;

    // killough 4/9/98: remember states seen, to detect cycles:
    static statenum_t   seenstate_tab[NUMSTATES];               // fast transition table
    statenum_t          *seenstate = seenstate_tab;             // pointer to table
    static int          recursion;                              // detects recursion
    statenum_t          i = state;                              // initial state
    boolean             ret = true;                             // return value
    statenum_t          tempstate[NUMSTATES];                   // for use with recursion

    if (recursion++)                                            // if recursion detected,
        memset((seenstate = tempstate), 0, sizeof(tempstate));  // clear state table

    do
    {
        if (state == S_NULL)
        {
            mobj->state = (state_t *) S_NULL;
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
        if (st->action.acp1)                
            st->action.acp1(mobj);        
        
        seenstate[state] = 1 + st->nextstate;                   // killough 4/9/98

        state = st->nextstate;
    } while (!mobj->tics && !seenstate[state]);                 // killough 4/9/98
                                
    if (!--recursion)
        for (; (state = seenstate[i]); i = state - 1)
            seenstate[i] = 0;            // killough 4/9/98: erase memory of states

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

    mo->tics -= P_Random()&3;

    if (mo->tics < 1)
        mo->tics = 1;

    mo->flags &= ~MF_MISSILE;
    mo->flags |= MF_TRANSLUCENT;

    if (mo->info->deathsound)
        S_StartSound (mo, mo->info->deathsound);
}


void P_SpawnSmokeTrail(fixed_t x, fixed_t y, fixed_t z, angle_t angle)
{
    mobj_t      *th = P_SpawnMobj(x, y, z + ((P_Random() - P_Random()) << 10), MT_TRAIL);

    th->momz = FRACUNIT / 2;
    th->tics -= (P_Random() & 3);

    th->angle = angle;

    th->flags2 |= (rand() & 1) * MF2_MIRRORED;
}

//
// P_XYMovement  
//
void P_XYMovement (mobj_t* mo) 
{         
    fixed_t        ptryx;
    fixed_t        ptryy;
    fixed_t        xmove;
    fixed_t        ymove;
    player_t*      player;
    int            flags2 = mo->flags2;

    if (!mo->momx && !mo->momy)
    {
        if (mo->flags & MF_SKULLFLY)
        {
            // the skull slammed into something
            mo->flags &= ~MF_SKULLFLY;
            mo->momx = mo->momy = mo->momz = 0;

            P_SetMobjState (mo, mo->info->spawnstate);
        }
        return;
    }
        
    player = mo->player;
                
    if (flags2 & MF2_SMOKETRAIL)
        if (puffcount++ > 1)
            P_SpawnSmokeTrail(mo->x, mo->y, mo->z, mo->angle);

    if (mo->momx > MAXMOVE)
        mo->momx = MAXMOVE;
    else if (mo->momx < -MAXMOVE)
        mo->momx = -MAXMOVE;

    if (mo->momy > MAXMOVE)
        mo->momy = MAXMOVE;
    else if (mo->momy < -MAXMOVE)
        mo->momy = -MAXMOVE;
                
    xmove = mo->momx;
    ymove = mo->momy;
        
    do
    {
        if (xmove > MAXMOVE / 2 || ymove > MAXMOVE / 2
            || xmove < -MAXMOVE / 2 || ymove < -MAXMOVE / 2)
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
                
        if (!P_TryMove (mo, ptryx, ptryy))
        {
            // blocked move
            if (mo->player)
            {        // try to slide along it
                not_walking = true;
                P_SlideMove (mo);
            }
            else if (mo->flags & MF_MISSILE)
            {
                // explode a missile
                if (ceilingline &&
                    ceilingline->backsector &&
                    ceilingline->backsector->ceilingpic == skyflatnum)
                {
                    // Hack to prevent missiles exploding
                    // against the sky.
                    // Does not handle sky floors.
                    P_RemoveMobj (mo);
                    return;
                }
                P_ExplodeMissile (mo);
            }
            else
                mo->momx = mo->momy = 0;
        }
    } while (xmove || ymove);
    
    // slow down
    if (player && player->cheats & CF_NOMOMENTUM)
    {
        // debug option for no sliding at all
        mo->momx = mo->momy = 0;
        return;
    }

    if (mo->flags & (MF_MISSILE | MF_SKULLFLY) )
        return;         // no friction for missiles ever
                
    if (mo->z > mo->floorz && !(mo->flags2 & MF2_FLY)
        && !(mo->flags2 & MF2_ONMOBJ))
    {                           // No friction when falling
        return;
    }

    if (mo->flags & MF_CORPSE)
    {
        // do not stop sliding
        //  if halfway off a step with some momentum
        if (mo->momx > FRACUNIT/4
            || mo->momx < -FRACUNIT/4
            || mo->momy > FRACUNIT/4
            || mo->momy < -FRACUNIT/4)
        {
            if (mo->floorz != mo->subsector->sector->floor_height)
                return;
        }
    }

    if (mo->momx > -STOPSPEED
        && mo->momx < STOPSPEED
        && mo->momy > -STOPSPEED
        && mo->momy < STOPSPEED
        && (!player
            || (player->cmd.forwardmove== 0
                && player->cmd.sidemove == 0 ) ) )
    {
        // if in a walking frame, stop moving
        if ( player&&(unsigned)((player->mo->state - states)- S_PLAY_RUN1) < 4)
            P_SetMobjState (player->mo, S_PLAY);
        
        mo->momx = 0;
        mo->momy = 0;
    }
    else
    {
        if (mo->flags2 & MF2_FLY && !(mo->z <= mo->floorz)
            && !(mo->flags2 & MF2_ONMOBJ))
        {
            mo->momx = FixedMul(mo->momx, FRICTION_FLY);
            mo->momy = FixedMul(mo->momy, FRICTION_FLY);
        }
        else
        {
            mo->momx = FixedMul (mo->momx, FRICTION);
            mo->momy = FixedMul (mo->momy, FRICTION);
        }
    }
}

//
// P_ZMovement
//
void P_ZMovement (mobj_t* mo)
{
    fixed_t        dist;
    fixed_t        delta;
    
    // check for smooth step up
    if (mo->player && mo->z < mo->floorz)
    {
        mo->player->viewheight -= mo->floorz-mo->z;

        mo->player->deltaviewheight
            = (VIEWHEIGHT - mo->player->viewheight)>>3;
    }
    
    // adjust height
    mo->z += mo->momz;
        
    if ( mo->flags & MF_FLOAT
         && mo->target)
    {
        // float down towards target if too close
        if ( !(mo->flags & MF_SKULLFLY)
             && !(mo->flags & MF_INFLOAT) )
        {
            dist = P_AproxDistance (mo->x - mo->target->x,
                                    mo->y - mo->target->y);
            
            delta =(mo->target->z + (mo->height>>1)) - mo->z;

            if (delta<0 && dist < -(delta*3) )
                mo->z -= FLOATSPEED;
            else if (delta>0 && dist < (delta*3) )
                mo->z += FLOATSPEED;                        
        }
        
    }
    
    if (mo->player && mo->flags2 & MF2_FLY && !(mo->z <= mo->floorz)
        && leveltime & 2)
    {
        mo->z += finesine[(FINEANGLES / 20 * leveltime >> 2) & FINEMASK];
    }
    // clip movement
    if (mo->z <= mo->floorz)
    {
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
        
        int correct_lost_soul_bounce = gameversion >= exe_ultimate;

        if (correct_lost_soul_bounce && mo->flags & MF_SKULLFLY)
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
            if (mo->player
                && mo->momz < -GRAVITY*8 && !(mo->flags2 & MF2_FLY))        
            {
                // Squat down.
                // Decrease viewheight for a moment
                // after hitting the ground (hard),
                // and utter appropriate sound.
                mo->player->deltaviewheight = mo->momz>>3;

                if(P_HitFloor(mo) == 0)
                    S_StartSound (mo, sfx_oof);

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

        if (!correct_lost_soul_bounce && mo->flags & MF_SKULLFLY)
            mo->momz = -mo->momz;

        if ( (mo->flags & MF_MISSILE)
             && !(mo->flags & MF_NOCLIP) )
        {
            P_ExplodeMissile (mo);
            return;
        }
    }
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
            mo->momz = -GRAVITY*2;
        else
            mo->momz -= GRAVITY;
    }
        
    if (mo->z + mo->height > mo->ceilingz)
    {
        // hit the ceiling
        if (mo->momz > 0)
            mo->momz = 0;
        {
            mo->z = mo->ceilingz - mo->height;
        }

        if (mo->flags & MF_SKULLFLY)
        {        // the skull slammed into something
            mo->momz = -mo->momz;
        }
        
        if ( (mo->flags & MF_MISSILE)
             && !(mo->flags & MF_NOCLIP) )
        {
            P_ExplodeMissile (mo);
            return;
        }
    }
} 



//
// P_NightmareRespawn
//
void
P_NightmareRespawn (mobj_t* mobj)
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
                      mobj->subsector->sector->floor_height , MT_TFOG); 
    // initiate teleport sound
    S_StartSound (mo, sfx_telept);

    // spawn a teleport fog at the new spot
    ss = R_PointInSubsector (x,y); 

    mo = P_SpawnMobj (x, y, ss->sector->floor_height , MT_TFOG); 

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
    P_RemoveMobj (mobj);
}


//
// P_MobjThinker
//
void P_MobjThinker (mobj_t* mobj)
{
    int      flags2;
    mobj_t   *onmo;
    player_t *player = mobj->player;
    sector_t *sector = mobj->subsector->sector;

    // Handle interpolation unless we're an active player.
    if (!(mobj->player != NULL && mobj == mobj->player->mo))
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

    // momentum movement
    if (mobj->momx
        || mobj->momy
        || (mobj->flags&MF_SKULLFLY) )
    {
        P_XYMovement (mobj);

        // FIXME: decent NOP/NULL/Nil function pointer please.
        if (mobj->thinker.function.acv == (actionf_v) (-1))
            return;                // mobj was removed
    }

    if (!isliquid[sector->floorpic])
        mobj->flags2 &= ~MF2_FEETARECLIPPED;
    flags2 = mobj->flags2;

    if ((flags2 & MF2_FEETARECLIPPED) && !player &&
            mobj->z <= sector->floor_height && !mobj->momz && d_swirl)
        mobj->z += animatedliquiddiffs[leveltime & 127];
    else if (flags2 & MF2_FLOATBOB)
        mobj->z += floatbobdiffs[(mobj->floatbob + leveltime) & 63];
    else if ( (mobj->z != mobj->floorz) || mobj->momz )
    {                           // Handle Z momentum and gravity
        if (mobj->flags2 & MF2_PASSMOBJ)
        {
            if (!(onmo = P_CheckOnmobj(mobj)))
            {
                P_ZMovement(mobj);
            }
            else
            {
                if (mobj->player && mobj->momz < 0)
                {
                    mobj->flags2 |= MF2_ONMOBJ;
                    mobj->momz = 0;
                }
/*
                if (mobj->player && (onmo->player || onmo->type == MT_POD))
                {
                    mobj->momx = onmo->momx;
                    mobj->momy = onmo->momy;
                    if (onmo->z < onmo->floorz)
                    {
                        mobj->z += onmo->floorz - onmo->z;
                        if (onmo->player)
                        {
                            onmo->player->viewheight -=
                                onmo->floorz - onmo->z;
                            onmo->player->deltaviewheight =
                                (VIEWHEIGHT - onmo->player->viewheight) >> 3;
                        }
                        onmo->z = onmo->floorz;
                    }
                }
*/
            }
        }
        else
        {
            P_ZMovement(mobj);
        }
        if (mobj->thinker.function.acv == (actionf_v) (-1))
        {                       // mobj was removed
            return;
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
                return;                // freed itself
    }
    else
    {
        // check for nightmare respawn
        if (! (mobj->flags & MF_COUNTKILL) )
            return;

        if (!respawnmonsters)
            return;

        mobj->movecount++;

        if (mobj->movecount < 12*TICRATE)
            return;

        if ( leveltime&31 )
            return;

        if (P_Random () > 4)
            return;

        P_NightmareRespawn (mobj);
    }

}


//
// P_SpawnMobj
//
mobj_t*
P_SpawnMobj
( fixed_t        x,
  fixed_t        y,
  fixed_t        z,
  mobjtype_t     type )
{
    mobj_t*      mobj;
    state_t*     st;
    mobjinfo_t*  info;
        
    mobj = Z_Malloc (sizeof(*mobj), PU_LEVEL, NULL);
    memset (mobj, 0, sizeof (*mobj));
    info = &mobjinfo[type];
        
    mobj->type = type;
    mobj->info = info;
    mobj->x = x;
    mobj->y = y;
    mobj->radius = info->radius;
    mobj->height = info->height;
    mobj->flags = info->flags;
    mobj->flags2 = info->flags2;
    mobj->health = info->spawnhealth;

    if (gameskill != sk_nightmare)
        mobj->reactiontime = info->reactiontime;
    
    mobj->lastlook = P_Random () % MAXPLAYERS;
    // do not set the state with P_SetMobjState,
    // because action routines can not be called yet
    st = &states[info->spawnstate];

    mobj->state = st;
    mobj->tics = st->tics;
    mobj->sprite = st->sprite;
    mobj->frame = st->frame;

    // set subsector and/or block links
    P_SetThingPosition (mobj);
        
    mobj->floorz = mobj->subsector->sector->floor_height;
    mobj->ceilingz = mobj->subsector->sector->ceiling_height;

    mobj->floatbob = P_Random();

    if (z == ONFLOORZ)
        mobj->z = mobj->floorz;
    else if (z == ONCEILINGZ)
        mobj->z = mobj->ceilingz - mobj->info->height;
    else 
        mobj->z = z;

    // Do not interpolate on spawn.
    mobj->interp = false;

    // Just in case interpolation is attempted...
    mobj->oldx = mobj->x;
    mobj->oldy = mobj->y;
    mobj->oldz = mobj->z;
    mobj->oldangle = mobj->angle;

    mobj->thinker.function.acp1 = (actionf_p1)P_MobjThinker;
        
    P_AddThinker (&mobj->thinker);

    if (!(mobj->flags2 & MF2_NOFOOTCLIP) && isliquid[mobj->subsector->sector->floorpic])
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
            iquetail = (iquetail+1)&(ITEMQUESIZE-1);
    }
        
    // unlink from sector and block lists
    P_UnsetThingPosition (mobj);
    
    // stop any playing sound
    S_StopSound (mobj);
    
    P_SetTarget(&mobj->target, NULL);
    P_SetTarget(&mobj->tracer, NULL);

    // free block
    P_RemoveThinker ((thinker_t*)mobj);
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
        return;        // 

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
    mo = P_SpawnMobj (x, y, ss->sector->floor_height , MT_IFOG); 

    if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4207819 &&
            fsize != 4274218 && fsize != 4225504 && fsize != 4225460)
        S_StartSound (mo, sfx_itmbk);

    // find which type to spawn
    for (i=0 ; i< NUMMOBJTYPES ; i++)
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
    player_t*              p;
    fixed_t                x;
    fixed_t                y;
    fixed_t                z;

    mobj_t*                mobj;

    int                    i;

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

    x                 = mthing->x << FRACBITS;
    y                 = mthing->y << FRACBITS;
    z                = ONFLOORZ;
    mobj        = P_SpawnMobj (x,y,z, MT_PLAYER);

    // set color translations for player sprites
    if (mthing->type > 1)                
        mobj->flags |= (mthing->type-1)<<MF_TRANSSHIFT;
                
    mobj->angle        = ANG45 * (mthing->angle/45);
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

    // setup gun psprite
    P_SetupPsprites (p);
    
    // give all cards in death match mode

    if (deathmatch)
        for (i=0 ; i<NUMCARDS ; i++)
            p->cards[i] = true;

    if (mthing->type-1 == consoleplayer)
    {
        // wake up the status bar
        ST_Start ();
        // wake up the heads up text
        HU_Start ();                
    }
}

//
// P_FindDoomedNum
//
// Finds a mobj type with a matching doomednum
// killough 8/24/98: rewrote to use hashing
//
int P_FindDoomedNum(unsigned int type)
{
    static struct
    {
        int     first;
        int     next;
    } *hash;

    int i;

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
// P_SpawnMapThing
// The fields of the mapthing should
// already be in host byte order.
//
void P_SpawnMapThing (mapthing_t* mthing)
{
    int                    i;
    int                    bit;
    mobj_t*                mobj;
    fixed_t                x;
    fixed_t                y;
    fixed_t                z;
    short                  type = mthing->type;
                
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
        
    if (i==NUMMOBJTYPES)
        I_Error ("P_SpawnMapThing: Unknown type %i at (%i, %i)",
                 mthing->type,
                 mthing->x, mthing->y);
                
    // don't spawn keycards and players in deathmatch

    if (deathmatch && mobjinfo[i].flags & MF_NOTDMATCH)
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
    if (nomonsters && ( i == MT_SKULL ||
                        i == MT_BETASKULL ||
                      ( mobjinfo[i].flags & MF_COUNTKILL)) )
    {
        return;
    }
    
    // spawn it
    x = mthing->x << FRACBITS;
    y = mthing->y << FRACBITS;

    if (mobjinfo[i].flags & MF_SPAWNCEILING)
        z = ONCEILINGZ;
    else
        z = ONFLOORZ;
    
    mobj = P_SpawnMobj (x,y,z, i);
    mobj->spawnpoint = *mthing;

    if (mobj->tics > 0)
        mobj->tics = 1 + (P_Random () % mobj->tics);
    if (mobj->flags & MF_COUNTKILL)
        totalkills++;
    if (mobj->flags & MF_COUNTITEM)
        totalitems++;
                
    mobj->angle = ANG45 * (mthing->angle/45);
    if (mthing->options & MTF_AMBUSH)
        mobj->flags |= MF_AMBUSH;

    // Lost Souls bleed Puffs
    if (d_colblood2 && d_chkblood2 && (i == MT_SKULL || i == MT_BETASKULL))
        mobj->flags |= MF_NOBLOOD;

    // RjY
    // Print a warning when a solid hanging body is used in a sector where
    // the player can walk under it, to help people with map debugging
    if (!((~mobj->flags) & (MF_SOLID | MF_SPAWNCEILING)) // solid and hanging
        // invert everything, then both bits should be clear
        && mobj->floorz + mobjinfo[MT_PLAYER].height <= mobj->z) // head <= base
        // player under body's head height <= bottom of body
    {
        C_Printf(CR_GOLD, " P_SpawnMapThing: solid hanging body in tall sector at %d,%d (type = %d)\n",
                mthing->x, mthing->y, type);
    }
}



//
// GAME SPAWN FUNCTIONS
//

//
// P_SpawnPuff
//

//void
mobj_t*
P_SpawnPuff
( fixed_t        x,
  fixed_t        y,
  fixed_t        z )
{
    mobj_t*      th;
        
    z += ((P_Random()-P_Random())<<10);

    th = P_SpawnMobj (x,y,z, MT_PUFF);
    th->momz = FRACUNIT;
    th->tics -= P_Random()&3;

    if (th->tics < 1)
        th->tics = 1;
        
    // don't make punches spark on the wall
    if (attackrange == MELEERANGE)
        P_SetMobjState (th, S_PUFF3);

    return th;
}


//
// P_SpawnBlood
// 
void
P_SpawnBlood
( fixed_t        x,
  fixed_t        y,
  fixed_t        z,
  int            damage,
  mobj_t*        target )
{
    mobj_t*        th;
        
    z += ((P_Random()-P_Random())<<10);
    th = P_SpawnMobj (x,y,z, MT_BLOOD);
    th->momz = FRACUNIT*2;
    th->tics -= P_Random()&3;
    th->target = target;

    // Spectres bleed spectre blood
    if (d_colblood2 && d_chkblood2)
    {
        if(target->type == MT_SHADOWS)
            th->flags |= MF_SHADOW;
    }

    if (th->tics < 1)
        th->tics = 1;

    if (damage <= 12 && damage >= 9)
        P_SetMobjState (th,S_BLOOD2);
    else if (damage < 9)
        P_SetMobjState (th,S_BLOOD3);

    // more blood and gore!
    if(d_maxgore)
    {
        mobj_t *th2 = P_SpawnMobj(x, y, z, MT_GORE);

        // added for colored blood and gore!
        th2->target = target;

        int t;
        
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
}



//
// P_CheckMissileSpawn
// Moves the missile forward a bit
//  and possibly explodes it right there.
//
void P_CheckMissileSpawn (mobj_t* th)
{
    th->tics -= P_Random()&3;
    if (th->tics < 1)
        th->tics = 1;
    
    // move a little forward so an angle can
    // be computed if it immediately explodes
    th->x += (th->momx>>1);
    th->y += (th->momy>>1);
    th->z += (th->momz>>1);

    if (!P_TryMove (th, th->x, th->y))
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
mobj_t*
P_SpawnMissile
( mobj_t*        source,
  mobj_t*        dest,
  mobjtype_t     type )
{
    mobj_t*      th;
    angle_t      an;
    int          dist;
    fixed_t      z;

    switch (type)
    {
        default:
            z = source->z + 32 * FRACUNIT;
            break;
    }

    if (source->flags2 & MF2_FEETARECLIPPED && d_footclip)
        z -= FOOTCLIPSIZE;

    th = P_SpawnMobj (source->x,
                      source->y,
                      z, type);
    
    if (th->info->seesound)
        S_StartSound (th, th->info->seesound);

    P_SetTarget(&th->target, source);   // where it came from
    an = R_PointToAngle2 (source->x, source->y, dest->x, dest->y);

    // fuzzy player
    if (dest->flags & MF_SHADOW)
        an += (P_Random()-P_Random())<<20;

    th->angle = an;
    an >>= ANGLETOFINESHIFT;
    th->momx = FixedMul (th->info->speed, finecosine[an]);
    th->momy = FixedMul (th->info->speed, finesine[an]);
        
    dist = P_AproxDistance (dest->x - source->x, dest->y - source->y);
    dist = dist / th->info->speed;

    if (dist < 1)
        dist = 1;

    th->momz = (dest->z - source->z) / dist;
    P_CheckMissileSpawn (th);
        
    return th;
}


//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//
//void
mobj_t*
P_SpawnPlayerMissile
( mobj_t*        source,
  mobjtype_t     type )
{
    mobj_t*      th;
    angle_t      an;
    
    fixed_t      aim;
    fixed_t      x;
    fixed_t      y;
    fixed_t      z;
    fixed_t      slope = NULL;                // SHUT UP COMPILER
    
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
        
    if (source->flags2 & MF2_FEETARECLIPPED && d_footclip)
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

    th->momz = FixedMul( th->info->speed, slope);

    if (type == MT_ROCKET && smoketrails && !load_dehacked)
    {
        th->flags2 |= MF2_SMOKETRAIL;
        puffcount = 0;
    }

    P_CheckMissileSpawn (th);

    return th;
}

//---------------------------------------------------------------------------
//
// FUNC P_GetThingFloorType
//
//---------------------------------------------------------------------------

int P_GetThingFloorType(mobj_t * thing)
{
    return (thing->subsector->sector->floorpic);
}

//---------------------------------------------------------------------------
//
// FUNC P_HitFloor
//
//---------------------------------------------------------------------------

int P_HitFloor(mobj_t * thing)
{
    mobj_t *mo;

//    if (thing->floorz != thing->subsector->sector->floor_height)
    if(thing->z > thing->subsector->sector->floor_height)
    {                           // don't splash if landing on the edge above water/lava/etc....
        return (FLOOR_SOLID);
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

            if(in_slime)
                S_StartSound(mo, sfx_burn);
            else
                S_StartSound(mo, sfx_gloop);

            return (FLOOR_WATER);
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

            if(in_slime)
                S_StartSound(mo, sfx_burn);
            else
                S_StartSound(mo, sfx_gloop);

            return (FLOOR_LAVA);
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

            if(in_slime)
                S_StartSound(mo, sfx_burn);
            else
                S_StartSound(mo, sfx_gloop);

            return (FLOOR_SLUDGE);
        default:
            return (FLOOR_SOLID);
    }
    return (FLOOR_SOLID);
}

