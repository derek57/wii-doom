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
//        Enemy thinking, AI.
//        Action Pointer Functions
//        that are associated with states/frames. 
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "c_io.h"
#include "doomdef.h"

// State.
#include "doomstat.h"

#include "g_game.h"
#include "i_system.h"
#include "m_random.h"
#include "p_local.h"
#include "p_tick.h"
#include "r_state.h"
#include "s_sound.h"

// Data.
#include "sounds.h"


#define        FATSPREAD            (ANG90/8)
#define        SKULLSPEED           (20*FRACUNIT)
#define        SPLAT_PER_COUNTER    1


typedef enum
{
    DI_EAST,
    DI_NORTHEAST,
    DI_NORTH,
    DI_NORTHWEST,
    DI_WEST,
    DI_SOUTHWEST,
    DI_SOUTH,
    DI_SOUTHEAST,
    DI_NODIR,
    NUMDIRS
    
} dirtype_t;


//
// P_NewChaseDir related LUT.
//
dirtype_t opposite[] =
{
  DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
  DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

dirtype_t diags[] =
{
    DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};


fixed_t           xspeed[8] = {FRACUNIT,47000,0,-47000,-FRACUNIT,-47000,0,47000};
fixed_t           yspeed[8] = {0,47000,FRACUNIT,47000,0,-47000,-FRACUNIT,-47000};
fixed_t           viletryx;
fixed_t           viletryy;

// 1/11/98 killough: Limit removed on special lines crossed
mobj_t*           soundtarget;
mobj_t*           corpsehit;
mobj_t*           vileobj;
mobj_t*           *braintargets;

mobjtype_t        chunk_type;

int               old_t;
int               old_u;
int               TRACEANGLE = 0xc000000;
int               numbraintargets;
int               braintargeton = 0;
int               numsplats;

static int        maxbraintargets;     // remove braintargets limit

extern int        numspechit;
extern int        snd_module;

boolean           on_ground;

extern boolean    not_walking;

extern line_t     **spechit;

//
// ENEMY THINKING
// Enemies are allways spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//


//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//


void
P_RecursiveSound
( sector_t*     sec,
  int           soundblocks )
{
    int         i;
    line_t*     check;
    sector_t*   other;
        
    // wake up all monsters in this sector
    if (sec->validcount == validcount
        && sec->soundtraversed <= soundblocks+1)
    {
        return;                // already flooded
    }
    
    sec->validcount = validcount;
    sec->soundtraversed = soundblocks+1;
    sec->soundtarget = soundtarget;
        
    for (i=0 ;i<sec->linecount ; i++)
    {
        check = sec->lines[i];
        if (! (check->flags & ML_TWOSIDED) )
            continue;
        
        P_LineOpening (check);

        if (openrange <= 0)
            continue;        // closed door
        
        if ( sides[ check->sidenum[0] ].sector == sec)
            other = sides[ check->sidenum[1] ] .sector;
        else
            other = sides[ check->sidenum[0] ].sector;
        
        if (check->flags & ML_SOUNDBLOCK)
        {
            if (!soundblocks)
                P_RecursiveSound (other, 1);
        }
        else
            P_RecursiveSound (other, soundblocks);
    }
}



//
// P_NoiseAlert
// If a monster yells at a player,
// it will alert other monsters to the player.
//
void
P_NoiseAlert
( mobj_t*        target,
  mobj_t*        emmiter )
{
    soundtarget = target;
    validcount++;
    P_RecursiveSound (emmiter->subsector->sector, 0);
}




//
// P_CheckMeleeRange
//
boolean P_CheckMeleeRange (mobj_t*        actor)
{
    mobj_t*        pl;
    fixed_t        dist;
        
    if (!actor->target)
        return false;
                
    pl = actor->target;
    dist = P_AproxDistance (pl->x-actor->x, pl->y-actor->y);

    if (dist >= MELEERANGE-20*FRACUNIT+pl->info->radius)
        return false;
        
    if (! P_CheckSight (actor, actor->target) )
        return false;
                                                        
    return true;                
}

//
// P_CheckMissileRange
//
boolean P_CheckMissileRange (mobj_t* actor)
{
    fixed_t        dist;
        
    if (! P_CheckSight (actor, actor->target) )
        return false;
        
    if ( actor->flags & MF_JUSTHIT )
    {
        // the target just hit the enemy,
        // so fight back!
        actor->flags &= ~MF_JUSTHIT;
        return true;
    }
        
    if (actor->reactiontime)
        return false;        // do not attack yet
                
    // OPTIMIZE: get this from a global checksight
    dist = P_AproxDistance ( actor->x-actor->target->x,
                             actor->y-actor->target->y) - 64*FRACUNIT;
    
    if (!actor->info->meleestate)
        dist -= 128*FRACUNIT;        // no melee attack, so fire more

    dist >>= 16;

    if (actor->type == MT_VILE)
    {
        if (dist > 14*64)        
            return false;        // too far away
    }
        

    if (actor->type == MT_UNDEAD)
    {
        if (dist < 196)        
            return false;        // close for fist attack
        dist >>= 1;
    }
        

    if (actor->type == MT_CYBORG
        || actor->type == MT_SPIDER
        || actor->type == MT_SKULL
        || actor->type == MT_BETASKULL)
    {
        dist >>= 1;
    }
    
    if (dist > 200)
        dist = 200;
                
    if (actor->type == MT_CYBORG && dist > 160)
        dist = 160;
                
    if (P_Random () < dist)
        return false;
                
    return true;
}


//
// P_Move
// Move in the current direction,
// returns false if the move is blocked.
//

boolean P_Move (mobj_t* actor)
{
    fixed_t        tryx;
    fixed_t        tryy;
    
    // warning: 'catch', 'throw', and 'try'
    // are all C++ reserved words
    boolean        try_ok;
    boolean        good;
                
    if (actor->movedir == DI_NODIR)
        return false;
                
    if ((unsigned)actor->movedir >= 8)
        I_Error ("Weird actor->movedir!");
                
    tryx = actor->x + actor->info->speed*xspeed[actor->movedir];
    tryy = actor->y + actor->info->speed*yspeed[actor->movedir];

    try_ok = P_TryMove (actor, tryx, tryy);

    if (!try_ok)
    {
        // open any specials
        if (actor->flags & MF_FLOAT && floatok)
        {
            // must adjust height
            if (actor->z < tmfloorz)
                actor->z += FLOATSPEED;
            else
                actor->z -= FLOATSPEED;

            actor->flags |= MF_INFLOAT;
            return true;
        }
                
        if (!numspechit)
            return false;
                        
        actor->movedir = DI_NODIR;

        // if the special is not a door that can be opened, return false
        //
        // killough 8/9/98: this is what caused monsters to get stuck in
        // doortracks, because it thought that the monster freed itself
        // by opening a door, even if it was moving towards the doortrack,
        // and not the door itself.
        //
        // killough 9/9/98: If a line blocking the monster is activated,
        // return true 90% of the time. If a line blocking the monster is
        // not activated, but some other line is, return false 90% of the
        // time. A bit of randomness is needed to ensure it's free from
        // lockups, but for most cases, it returns the correct result.
        //
        // Do NOT simply return false 1/4th of the time (causes monsters to
        // back out when they shouldn't, and creates secondary stickiness).
        for (good = false; numspechit--;)
            if (P_UseSpecialLine(actor, spechit[numspechit], 0))
                good |= (spechit[numspechit] == blockline ? 1 : 2);

        return (good && ((P_Random() >= 230) ^ (good & 1)));
    }
    else
    {
        actor->flags &= ~MF_INFLOAT;
    }
        
        
    if (! (actor->flags & MF_FLOAT) )        
    {
        if (actor->z > actor->floorz)
        {
            if(d_splash)
                P_HitFloor(actor);
        }
        actor->z = actor->floorz;
    }
    return true; 
}


//
// TryWalk
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//
boolean P_TryWalk (mobj_t* actor)
{        
    if (!P_Move (actor))
    {
        return false;
    }

    actor->movecount = P_Random()&15;
    return true;
}




void P_NewChaseDir (mobj_t*        actor)
{
    fixed_t        deltax;
    fixed_t        deltay;
    
    dirtype_t      d[3];
    
    int            tdir;
    dirtype_t      olddir;
    
    dirtype_t      turnaround;

    if (!actor->target)
        I_Error ("P_NewChaseDir: called with no target");
                
    olddir = actor->movedir;
    turnaround=opposite[olddir];

    deltax = actor->target->x - actor->x;
    deltay = actor->target->y - actor->y;

    if (deltax>10*FRACUNIT)
        d[1]= DI_EAST;
    else if (deltax<-10*FRACUNIT)
        d[1]= DI_WEST;
    else
        d[1]=DI_NODIR;

    if (deltay<-10*FRACUNIT)
        d[2]= DI_SOUTH;
    else if (deltay>10*FRACUNIT)
        d[2]= DI_NORTH;
    else
        d[2]=DI_NODIR;

    // try direct route
    if (d[1] != DI_NODIR
        && d[2] != DI_NODIR)
    {
        actor->movedir = diags[((deltay<0)<<1)+(deltax>0)];
        if (actor->movedir != (int) turnaround && P_TryWalk(actor))
            return;
    }

    // try other directions
    if (P_Random() > 200
        ||  abs(deltay)>abs(deltax))
    {
        tdir=d[1];
        d[1]=d[2];
        d[2]=tdir;
    }

    if (d[1]==turnaround)
        d[1]=DI_NODIR;
    if (d[2]==turnaround)
        d[2]=DI_NODIR;
        
    if (d[1]!=DI_NODIR)
    {
        actor->movedir = d[1];
        if (P_TryWalk(actor))
        {
            // either moved forward or attacked
            return;
        }
    }

    if (d[2]!=DI_NODIR)
    {
        actor->movedir =d[2];

        if (P_TryWalk(actor))
            return;
    }

    // there is no direct path to the player,
    // so pick another direction.
    if (olddir!=DI_NODIR)
    {
        actor->movedir =olddir;

        if (P_TryWalk(actor))
            return;
    }

    // randomly determine direction of search
    if (P_Random()&1)         
    {
        for ( tdir=DI_EAST;
              tdir<=DI_SOUTHEAST;
              tdir++ )
        {
            if (tdir != (int) turnaround)
            {
                actor->movedir =tdir;
                
                if ( P_TryWalk(actor) )
                    return;
            }
        }
    }
    else
    {
        for ( tdir=DI_SOUTHEAST;
              tdir != (DI_EAST-1);
              tdir-- )
        {
            if (tdir != (int) turnaround)
            {
                actor->movedir = tdir;
                
                if ( P_TryWalk(actor) )
                    return;
            }
        }
    }

    if (turnaround !=  DI_NODIR)
    {
        actor->movedir =turnaround;
        if ( P_TryWalk(actor) )
            return;
    }

    actor->movedir = DI_NODIR;        // can not move
}



//
// P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//
boolean
P_LookForPlayers
( mobj_t*        actor,
  boolean        allaround )
{
    int          c;
    int          stop;
    player_t*    player;
    angle_t      an;
    fixed_t      dist;

    c = 0;
    stop = (actor->lastlook-1)&3;
        
    for ( ; ; actor->lastlook = (actor->lastlook+1)&3 )
    {
        if (!playeringame[actor->lastlook])
            continue;
                        
        if (c++ == 2
            || actor->lastlook == stop)
        {
            // done looking
            return false;        
        }
        
        player = &players[actor->lastlook];

        if (player->health <= 0)
            continue;                // dead

        if (!P_CheckSight (actor, player->mo))
            continue;                // out of sight
                        
        if (!allaround)
        {
            an = R_PointToAngle2 (actor->x,
                                  actor->y, 
                                  player->mo->x,
                                  player->mo->y)
                - actor->angle;
            
            if (an > ANG90 && an < ANG270)
            {
                dist = P_AproxDistance (player->mo->x - actor->x,
                                        player->mo->y - actor->y);
                // if real close, react anyway
                if (dist > MELEERANGE)
                    continue;        // behind back
            }
        }
                
        actor->target = player->mo;
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------
//
// PROC A_MoreBlood
//
//----------------------------------------------------------------------------

void A_MoreBlood(mobj_t * actor)
{
    if(d_maxgore && !(actor->flags & MF_NOBLOOD))
    {
        int i, t;

        mobj_t *mo;
        mobjtype_t chunk_type;

        numsplats++;

        if (d_chkblood && d_colblood)
        {
            if (actor->type == MT_HEAD ||
                    actor->type == MT_BETAHEAD)
                chunk_type = MT_CHUNK_BLUE;
            else if(actor->type == MT_BRUISER ||
                    actor->type == MT_BETABRUISER ||
                    actor->type == MT_KNIGHT)
                chunk_type = MT_CHUNK_GREEN;
            else
                chunk_type = MT_CHUNK;
        }
        else
            chunk_type = MT_CHUNK;
        
        if (d_chkblood2 && d_colblood2)
        {
            if (actor->type == MT_SKULL ||
                   actor->type == MT_BETASKULL)
            {
                actor->flags |= MF_NOBLOOD;
                goto skip;
            }
        }

        // WARNING: don't go lower than SPLAT_PER_COUNTER !!!
        for(i = SPLAT_PER_COUNTER; i <= numsplats / SPLAT_PER_COUNTER && numsplats % i == 0; ++i)
        {
            mo = P_SpawnMobj(actor->x,
                             actor->y,
                             actor->z, chunk_type);

            // added for colored blood and gore!
            mo->target = actor;

            mo->type = chunk_type;

            if (d_colblood2 && d_chkblood2)
            {
                // Spectres bleed spectre blood
                if(mo->target->type == MT_SHADOWS)
                    mo->flags |= MF_SHADOW;
            }

            t = P_Random() % 6;

            if(t == 0 || t == 1 || t == 2 || t == 3 || t == 4 || t == 5)
                P_SetMobjState(mo, mobjinfo[mo->type].spawnstate + t);

            t = P_Random();

            mo->momx = (t - P_Random()) << 11;

            t = P_Random();

            mo->momy = (t - P_Random()) << 11;
            mo->momz = (P_Random() << 11) / 2;
            break;
        }
        skip: ;
    }
}

void A_Pain (mobj_t* actor)
{
    if (actor->info->painsound)
        S_StartSound (actor, actor->info->painsound);

    A_MoreBlood(actor);
}

void A_Fall (mobj_t *actor)
{
    // actor is on ground, it can be walked over
    actor->flags &= ~MF_SOLID;

    // So change this if corpse objects
    // are meant to be obstacles.

    if(d_maxgore && !(actor->flags & MF_NOBLOOD))
    {
        int i, t;
        mobj_t *mo;

        if((actor->type == MT_SKULL ||
               actor->type == MT_BETASKULL) && d_colblood2 && d_chkblood2)
            goto skip;

        for(i = 0; i < 8; i++)
        {
            // spray blood in a random direction
            mo = P_SpawnMobj(actor->x,
                             actor->y,
                             actor->z + actor->info->height/2, MT_GORE);

            // added for colored blood and gore!
            mo->target = actor;

            // Spectres bleed spectre blood
            if (d_colblood2 && d_chkblood2) 
            {
                if(actor->type == MT_SHADOWS)
                    mo->flags |= MF_SHADOW;
            }

            t = P_Random() % 3;
            if(t > 0)
                P_SetMobjState(mo, S_SPRAY_00 + t);

            t = P_Random();
            mo->momx = (t - P_Random ()) << 11;
            t = P_Random();
            mo->momy = (t - P_Random ()) << 11;
            mo->momz = P_Random() << 11;

            A_MoreBlood(actor);
        }
        skip: ;
    }
}


//
// A_KeenDie
// DOOM II special, map 32.
// Uses special tag 666.
//
void A_KeenDie (mobj_t* mo)
{
    thinker_t*  th;
    mobj_t*     mo2;
    line_t      junk;

    A_Fall (mo);
    
    // scan the remaining thinkers
    // to see if all Keens are dead
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
        if (th->function.acp1 != (actionf_p1)P_MobjThinker)
            continue;

        mo2 = (mobj_t *)th;
        if (mo2 != mo
            && mo2->type == mo->type
            && mo2->health > 0)
        {
            // other Keen not dead
            return;                
        }
    }

    junk.tag = 666;
    EV_DoDoor(&junk,open);
}


//
// ACTION ROUTINES
//

//
// A_Look
// Stay in state until a player is sighted.
//
void A_Look (mobj_t* actor)
{
    mobj_t*        targ;
        
    actor->threshold = 0;        // any shot will wake up
    targ = actor->subsector->sector->soundtarget;

    if(beta_style && actor->flags & MF_COUNTKILL &&
            actor->state->frame == 1 && actor->state->tics > 0)
        actor->state->tics = 0;

    if (targ
        && (targ->flags & MF_SHOOTABLE) )
    {
        P_SetTarget(&actor->target, targ);

        if ( actor->flags & MF_AMBUSH )
        {
            if (P_CheckSight (actor, actor->target))
                goto seeyou;
        }
        else
            goto seeyou;
    }
        
        
    if (!P_LookForPlayers (actor, false) )
        return;
                
    // go into chase state
  seeyou:
    if (actor->info->seesound)
    {
        int                sound;
                
        switch (actor->info->seesound)
        {
          case sfx_posit1:
          case sfx_posit2:
          case sfx_posit3:
            sound = sfx_posit1+P_Random()%3;
            break;

          case sfx_bgsit1:
          case sfx_bgsit2:
            sound = sfx_bgsit1+P_Random()%2;
            break;

          default:
            sound = actor->info->seesound;
            break;
        }

        if (actor->type==MT_SPIDER
            || actor->type == MT_CYBORG)
        {
            // full volume
            S_StartSound (NULL, sound);
        }
        else
            S_StartSound (actor, sound);
    }

    P_SetMobjState (actor, actor->info->seestate);
}


//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
void A_Chase (mobj_t*        actor)
{
    int                delta;

    if (actor->reactiontime)
        actor->reactiontime--;
                                

    // modify target threshold
    if  (actor->threshold)
    {
        if (!actor->target
            || actor->target->health <= 0)
        {
            actor->threshold = 0;
        }
        else
            actor->threshold--;
    }
    
    // turn towards movement direction if not there yet
    if (actor->movedir < 8)
    {
        actor->angle &= (7<<29);
        delta = actor->angle - (actor->movedir << 29);
        
        if (delta > 0)
            actor->angle -= ANG90/2;
        else if (delta < 0)
            actor->angle += ANG90/2;
    }

    if (!actor->target
        || !(actor->target->flags&MF_SHOOTABLE))
    {
        // look for a new target
        if (P_LookForPlayers(actor,true))
            return;         // got a new target
        
        P_SetMobjState (actor, actor->info->spawnstate);
        return;
    }
    
    // do not attack twice in a row
    if (actor->flags & MF_JUSTATTACKED)
    {
        actor->flags &= ~MF_JUSTATTACKED;
        if (gameskill != sk_nightmare && !fastparm)
            P_NewChaseDir (actor);
        return;
    }
    
    // check for melee attack
    if (actor->info->meleestate
        && P_CheckMeleeRange (actor))
    {
        if (actor->info->attacksound)
            S_StartSound (actor, actor->info->attacksound);

        P_SetMobjState (actor, actor->info->meleestate);
        return;
    }
    
    // check for missile attack
    if (actor->info->missilestate)
    {
        if (gameskill < sk_nightmare
            && !fastparm && actor->movecount)
        {
            goto nomissile;
        }
        
        if (!P_CheckMissileRange (actor))
            goto nomissile;
        
        P_SetMobjState (actor, actor->info->missilestate);
        actor->flags |= MF_JUSTATTACKED;
        return;
    }

    // ?
  nomissile:
    // possibly choose another target

    if (netgame
        && !actor->threshold
        && !P_CheckSight (actor, actor->target) )
    {
        if (P_LookForPlayers(actor,true))
            return;        // got a new target
    }

    // chase towards player
    if (--actor->movecount<0
        || !P_Move (actor))
    {
        P_NewChaseDir (actor);
    }
    
    // make active sound
    if (actor->info->activesound
        && P_Random () < 3)
    {
        S_StartSound (actor, actor->info->activesound);
    }
}


//
// A_FaceTarget
//
void A_FaceTarget (mobj_t* actor)
{        
    if (!actor->target)
        return;
    
    actor->flags &= ~MF_AMBUSH;
        
    actor->angle = R_PointToAngle2 (actor->x,
                                    actor->y,
                                    actor->target->x,
                                    actor->target->y);
    
    if (actor->target->flags & MF_SHADOW)
        actor->angle += (P_Random()-P_Random())<<21;
}


//
// A_PosAttack
//
void A_PosAttack (mobj_t* actor)
{
    int         angle;
    int         damage;
    int         slope;
        
    if (!actor->target)
        return;
                
    A_FaceTarget (actor);
    angle = actor->angle;
    slope = P_AimLineAttack (actor, angle, MISSILERANGE);

    S_StartSound (actor, sfx_pistol);
    angle += (P_Random()-P_Random())<<20;
    damage = ((P_Random()%5)+1)*3;
    P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
}

void A_SPosAttack (mobj_t* actor)
{
    int                i;
    int                angle;
    int                bangle;
    int                damage;
    int                slope;
        
    if (!actor->target)
        return;

    S_StartSound (actor, sfx_shotgn);
    A_FaceTarget (actor);
    bangle = actor->angle;
    slope = P_AimLineAttack (actor, bangle, MISSILERANGE);

    for (i=0 ; i<3 ; i++)
    {
        angle = bangle + ((P_Random()-P_Random())<<20);
        damage = ((P_Random()%5)+1)*3;
        P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
    }
}

void A_CPosAttack (mobj_t* actor)
{
    int                angle;
    int                bangle;
    int                damage;
    int                slope;
        
    if (!actor->target)
        return;

    S_StartSound (actor, sfx_shotgn);
    A_FaceTarget (actor);
    bangle = actor->angle;
    slope = P_AimLineAttack (actor, bangle, MISSILERANGE);

    angle = bangle + ((P_Random()-P_Random())<<20);
    damage = ((P_Random()%5)+1)*3;
    P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
}

void A_CPosRefire (mobj_t* actor)
{        
    // keep firing unless target got out of sight
    A_FaceTarget (actor);

    if (P_Random () < 40)
        return;

    if (!actor->target
        || actor->target->health <= 0
        || !P_CheckSight (actor, actor->target) )
    {
        P_SetMobjState (actor, actor->info->seestate);
    }
}


void A_SpidRefire (mobj_t* actor)
{        
    // keep firing unless target got out of sight
    A_FaceTarget (actor);

    if (P_Random () < 10)
        return;

    if (!actor->target
        || actor->target->health <= 0
        || !P_CheckSight (actor, actor->target) )
    {
        P_SetMobjState (actor, actor->info->seestate);
    }
}

void A_BspiAttack (mobj_t *actor)
{        
    if (!actor->target)
        return;
                
    A_FaceTarget (actor);

    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_ARACHPLAZ);
}


//
// A_TroopAttack
//
void A_TroopAttack (mobj_t* actor)
{
    int                damage;
        
    if (!actor->target)
        return;
                
    A_FaceTarget (actor);
    if (P_CheckMeleeRange (actor))
    {
        S_StartSound (actor, sfx_claw);
        damage = (P_Random()%8+1)*3;
        P_DamageMobj (actor->target, actor, actor, damage);
        return;
    }

    // launch a missile
    if(beta_imp)
        P_SpawnMissile (actor, actor->target, MT_TROOPSHOT2);
    else
        P_SpawnMissile (actor, actor->target, MT_TROOPSHOT);
}


void A_SargAttack (mobj_t* actor)
{
    int                damage;

    if (!actor->target)
        return;
                
    A_FaceTarget (actor);
    if (P_CheckMeleeRange (actor))
    {
        damage = ((P_Random()%10)+1)*4;
        P_DamageMobj (actor->target, actor, actor, damage);
    }
}

void A_HeadAttack (mobj_t* actor)
{
    int                damage;
        
    if (!actor->target)
        return;
                
    A_FaceTarget (actor);
    if (P_CheckMeleeRange (actor))
    {
        damage = (P_Random()%6+1)*10;
        P_DamageMobj (actor->target, actor, actor, damage);
        return;
    }
    
    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_HEADSHOT);
}

void A_CyberAttack (mobj_t* actor)
{        
    mobj_t      *mo;

    if (!actor->target)
        return;
                
    A_FaceTarget (actor);
    mo = P_SpawnMissile (actor, actor->target, MT_ROCKET);

    if (smoketrails)
        mo->flags2 |= MF2_SMOKETRAIL;
}


void A_BruisAttack (mobj_t* actor)
{
    int                damage;
        
    if (!actor->target)
        return;
                
    if (P_CheckMeleeRange (actor))
    {
        S_StartSound (actor, sfx_claw);
        damage = (P_Random()%8+1)*10;
        P_DamageMobj (actor->target, actor, actor, damage);
        return;
    }
    
    // launch a missile
    P_SpawnMissile (actor, actor->target, MT_BRUISERSHOT);
}


//
// A_SkelMissile
//
void A_SkelMissile (mobj_t* actor)
{        
    mobj_t*        mo;
        
    if (!actor->target)
        return;
                
    A_FaceTarget (actor);
    actor->z += 16*FRACUNIT;        // so missile spawns higher
    mo = P_SpawnMissile (actor, actor->target, MT_TRACER);
    actor->z -= 16*FRACUNIT;        // back to normal

    mo->x += mo->momx;
    mo->y += mo->momy;
    P_SetTarget(&mo->tracer, actor->target);
}


void A_Tracer (mobj_t* actor)
{
    angle_t        exact;
    fixed_t        dist;
    fixed_t        slope;
    mobj_t*        dest;
    mobj_t*        th;
                
    if (gametic & 3)
        return;
    
    // spawn a puff of smoke behind the rocket                
    P_SpawnSmokeTrail(actor->x, actor->y, actor->z, actor->angle);
        
    th = P_SpawnMobj (actor->x-actor->momx,
                      actor->y-actor->momy,
                      actor->z, MT_SMOKE);
    
    th->momz = FRACUNIT;
    th->tics -= P_Random()&3;
    if (th->tics < 1)
        th->tics = 1;
    
    // adjust direction
    dest = actor->tracer;
        
    if (!dest || dest->health <= 0)
        return;
    
    // change angle        
    exact = R_PointToAngle2 (actor->x,
                             actor->y,
                             dest->x,
                             dest->y);

    if (exact != actor->angle)
    {
        if (exact - actor->angle > 0x80000000)
        {
            actor->angle -= TRACEANGLE;
            if (exact - actor->angle < 0x80000000)
                actor->angle = exact;
        }
        else
        {
            actor->angle += TRACEANGLE;
            if (exact - actor->angle > 0x80000000)
                actor->angle = exact;
        }
    }
        
    exact = actor->angle>>ANGLETOFINESHIFT;
    actor->momx = FixedMul (actor->info->speed, finecosine[exact]);
    actor->momy = FixedMul (actor->info->speed, finesine[exact]);
    
    // change slope
    dist = P_AproxDistance (dest->x - actor->x,
                            dest->y - actor->y);
    
    dist = dist / actor->info->speed;

    if (dist < 1)
        dist = 1;
    slope = (dest->z+40*FRACUNIT - actor->z) / dist;

    if (slope < actor->momz)
        actor->momz -= FRACUNIT/8;
    else
        actor->momz += FRACUNIT/8;
}


void A_SkelWhoosh (mobj_t*        actor)
{
    if (!actor->target)
        return;
    A_FaceTarget (actor);
    S_StartSound (actor,sfx_skeswg);
}

void A_SkelFist (mobj_t*        actor)
{
    int                damage;

    if (!actor->target)
        return;
                
    A_FaceTarget (actor);
        
    if (P_CheckMeleeRange (actor))
    {
        damage = ((P_Random()%10)+1)*6;
        S_StartSound (actor, sfx_skepch);
        P_DamageMobj (actor->target, actor, actor, damage);
    }
}



//
// PIT_VileCheck
// Detect a corpse that could be raised.
//

boolean PIT_VileCheck (mobj_t*        thing)
{
    int                maxdist;
    boolean        check;
        
    if (!(thing->flags & MF_CORPSE) )
        return true;        // not a monster
    
    if (thing->tics != -1)
        return true;        // not lying still yet
    
    if (thing->info->raisestate == S_NULL)
        return true;        // monster doesn't have a raise state
    
    maxdist = thing->info->radius + mobjinfo[MT_VILE].radius;
        
    if ( abs(thing->x - viletryx) > maxdist
         || abs(thing->y - viletryy) > maxdist )
        return true;                // not actually touching
                
    corpsehit = thing;
    corpsehit->momx = corpsehit->momy = 0;
    corpsehit->height <<= 2;
    check = P_CheckPosition (corpsehit, corpsehit->x, corpsehit->y);
    corpsehit->height >>= 2;

    if (!check)
        return true;                // doesn't fit here
                
    return false;                // got one, so stop checking
}



//
// A_VileChase
// Check for ressurecting a body
//
void A_VileChase (mobj_t* actor)
{
    int                        xl;
    int                        xh;
    int                        yl;
    int                        yh;
    
    int                        bx;
    int                        by;

    mobjinfo_t*                info;
    mobj_t*                    temp;
        
    if (actor->movedir != DI_NODIR)
    {
        // check for corpses to raise
        viletryx =
            actor->x + actor->info->speed*xspeed[actor->movedir];
        viletryy =
            actor->y + actor->info->speed*yspeed[actor->movedir];

        xl = (viletryx - bmaporgx - MAXRADIUS*2)>>MAPBLOCKSHIFT;
        xh = (viletryx - bmaporgx + MAXRADIUS*2)>>MAPBLOCKSHIFT;
        yl = (viletryy - bmaporgy - MAXRADIUS*2)>>MAPBLOCKSHIFT;
        yh = (viletryy - bmaporgy + MAXRADIUS*2)>>MAPBLOCKSHIFT;
        
        vileobj = actor;
        for (bx=xl ; bx<=xh ; bx++)
        {
            for (by=yl ; by<=yh ; by++)
            {
                // Call PIT_VileCheck to check
                // whether object is a corpse
                // that canbe raised.
                if (!P_BlockThingsIterator(bx,by,PIT_VileCheck))
                {
                    // got one!
                    temp = actor->target;
                    actor->target = corpsehit;
                    A_FaceTarget (actor);
                    actor->target = temp;
                                        
                    P_SetMobjState (actor, S_VILE_HEAL1);
                    S_StartSound (corpsehit, sfx_slop);
                    info = corpsehit->info;
                    
                    P_SetMobjState (corpsehit,info->raisestate);
                    corpsehit->height <<= 2;
                    corpsehit->flags = info->flags;
                    corpsehit->health = info->spawnhealth;
                    P_SetTarget(&corpsehit->target, NULL);

                    // resurrected pools of gore ("ghost monsters") are translucent
                    if (corpsehit->height == 0 && corpsehit->radius == 0)
                        corpsehit->flags |= MF_TRANSLUCENT;

                    return;
                }
            }
        }
    }

    // Return to normal attack.
    A_Chase (actor);
}


//
// A_VileStart
//
void A_VileStart (mobj_t* actor)
{
    S_StartSound (actor, sfx_vilatk);
}


//
// A_Fire
// Keep fire in front of player unless out of sight
//

void A_Fire (mobj_t* actor)
{
    mobj_t*        dest;
    mobj_t*        target;
    unsigned       an;
                
    dest = actor->tracer;
    if (!dest)
        return;

    target = P_SubstNullMobj(actor->target);
                
    // don't move it if the vile lost sight
    if (!P_CheckSight (target, dest) )
        return;

    an = dest->angle >> ANGLETOFINESHIFT;

    P_UnsetThingPosition (actor);
    actor->x = dest->x + FixedMul (24*FRACUNIT, finecosine[an]);
    actor->y = dest->y + FixedMul (24*FRACUNIT, finesine[an]);
    actor->z = dest->z;
    P_SetThingPosition (actor);
}

void A_StartFire (mobj_t* actor)
{
    S_StartSound(actor,sfx_flamst);
    A_Fire(actor);
}

void A_FireCrackle (mobj_t* actor)
{
    S_StartSound(actor,sfx_flame);
    A_Fire(actor);
}



//
// A_VileTarget
// Spawn the hellfire
//
void A_VileTarget (mobj_t*        actor)
{
    mobj_t*        fog;
        
    if (!actor->target)
        return;

    A_FaceTarget (actor);

    fog = P_SpawnMobj (actor->target->x,
                       actor->target->x,
                       actor->target->z, MT_FIRE);
    
    P_SetTarget(&actor->tracer, fog);
    P_SetTarget(&fog->target, actor);
    P_SetTarget(&fog->tracer, actor->target);
    A_Fire (fog);
}




//
// A_VileAttack
//
void A_VileAttack (mobj_t* actor)
{        
    mobj_t*        fire;
    int                an;
        
    if (!actor->target)
        return;
    
    A_FaceTarget (actor);

    if (!P_CheckSight (actor, actor->target) )
        return;

    S_StartSound (actor, sfx_barexp);
    P_DamageMobj (actor->target, actor, actor, 20);
    actor->target->momz = 1000*FRACUNIT/actor->target->info->mass;
        
    an = actor->angle >> ANGLETOFINESHIFT;

    fire = actor->tracer;

    if (!fire)
        return;
                
    // move the fire between the vile and the player
    fire->x = actor->target->x - FixedMul (24*FRACUNIT, finecosine[an]);
    fire->y = actor->target->y - FixedMul (24*FRACUNIT, finesine[an]);        
    P_RadiusAttack (fire, actor, 70 );
}




//
// Mancubus attack,
// firing three missiles (bruisers)
// in three different directions?
// Doesn't look like it. 
//

void A_FatRaise (mobj_t *actor)
{
    A_FaceTarget (actor);
    S_StartSound (actor, sfx_manatk);
}


void A_FatAttack1 (mobj_t* actor)
{
    mobj_t*        mo;
    mobj_t*     target;
    int                an;

    A_FaceTarget (actor);

    // Change direction  to ...
    actor->angle += FATSPREAD;
    target = P_SubstNullMobj(actor->target);
    P_SpawnMissile (actor, target, MT_FATSHOT);

    mo = P_SpawnMissile (actor, target, MT_FATSHOT);
    mo->angle += FATSPREAD;
    an = mo->angle >> ANGLETOFINESHIFT;
    mo->momx = FixedMul (mo->info->speed, finecosine[an]);
    mo->momy = FixedMul (mo->info->speed, finesine[an]);
}

void A_FatAttack2 (mobj_t* actor)
{
    mobj_t*        mo;
    mobj_t*     target;
    int                an;

    A_FaceTarget (actor);
    // Now here choose opposite deviation.
    actor->angle -= FATSPREAD;
    target = P_SubstNullMobj(actor->target);
    P_SpawnMissile (actor, target, MT_FATSHOT);

    mo = P_SpawnMissile (actor, target, MT_FATSHOT);
    mo->angle -= FATSPREAD*2;
    an = mo->angle >> ANGLETOFINESHIFT;
    mo->momx = FixedMul (mo->info->speed, finecosine[an]);
    mo->momy = FixedMul (mo->info->speed, finesine[an]);
}

void A_FatAttack3 (mobj_t*        actor)
{
    mobj_t*        mo;
    mobj_t*     target;
    int                an;

    A_FaceTarget (actor);

    target = P_SubstNullMobj(actor->target);
    
    mo = P_SpawnMissile (actor, target, MT_FATSHOT);
    mo->angle -= FATSPREAD/2;
    an = mo->angle >> ANGLETOFINESHIFT;
    mo->momx = FixedMul (mo->info->speed, finecosine[an]);
    mo->momy = FixedMul (mo->info->speed, finesine[an]);

    mo = P_SpawnMissile (actor, target, MT_FATSHOT);
    mo->angle += FATSPREAD/2;
    an = mo->angle >> ANGLETOFINESHIFT;
    mo->momx = FixedMul (mo->info->speed, finecosine[an]);
    mo->momy = FixedMul (mo->info->speed, finesine[an]);
}


//
// SkullAttack
// Fly at the player like a missile.
//

void A_SkullAttack (mobj_t* actor)
{
    mobj_t*                dest;
    angle_t                an;
    int                        dist;

    if (!actor->target)
        return;
                
    dest = actor->target;        
    actor->flags |= MF_SKULLFLY;

    S_StartSound (actor, actor->info->attacksound);
    A_FaceTarget (actor);
    an = actor->angle >> ANGLETOFINESHIFT;
    actor->momx = FixedMul (SKULLSPEED, finecosine[an]);
    actor->momy = FixedMul (SKULLSPEED, finesine[an]);
    dist = P_AproxDistance (dest->x - actor->x, dest->y - actor->y);
    dist = dist / SKULLSPEED;
    
    if (dist < 1)
        dist = 1;
    actor->momz = (dest->z+(dest->height>>1) - actor->z) / dist;
}

void A_BetaSkullAttack (mobj_t* actor)
{
    int damage;

    if (!actor->target || actor->target->type == MT_SKULL ||
                          actor->target->type == MT_BETASKULL)
        return;

    S_StartSound(actor, actor->info->attacksound);
    A_FaceTarget(actor);

    damage = (P_RandomSMMU(pr_skullfly)%8+1)*actor->info->damage;

    P_DamageMobj(actor->target, actor, actor, damage);
}

void A_Stop(mobj_t *actor)
{
    actor->momx = actor->momy = actor->momz = 0;
}

//
// A_PainShootSkull
// Spawn a lost soul and launch it at the target
//
void
A_PainShootSkull
( mobj_t*        actor,
  angle_t        angle )
{
    fixed_t        x;
    fixed_t        y;
    fixed_t        z;
    
    mobj_t*        newmobj;
    angle_t        an;
    int            prestep;
    int            count;
    thinker_t*     currentthinker;

    // count total number of skull currently on the level
    count = 0;

    currentthinker = thinkercap.next;
    while (currentthinker != &thinkercap)
    {
        if (   (currentthinker->function.acp1 == (actionf_p1)P_MobjThinker)
            && (((mobj_t *)currentthinker)->type == MT_SKULL ||
                ((mobj_t *)currentthinker)->type == MT_SKULL))
            count++;
        currentthinker = currentthinker->next;
    }

    // if there are allready 20 skulls on the level,
    // don't spit another one
    if (count > 20)
        return;


    // okay, there's playe for another one
    an = angle >> ANGLETOFINESHIFT;
    
    if(beta_skulls)
    {
        prestep =
            4*FRACUNIT
            + 3*(actor->info->radius + mobjinfo[MT_BETASKULL].radius)/2;
    }
    else
    {
        prestep =
            4*FRACUNIT
            + 3*(actor->info->radius + mobjinfo[MT_SKULL].radius)/2;
    }
    x = actor->x + FixedMul (prestep, finecosine[an]);
    y = actor->y + FixedMul (prestep, finesine[an]);
    z = actor->z + 8*FRACUNIT;
                
    if(beta_skulls)
        newmobj = P_SpawnMobj (x , y, z, MT_BETASKULL);
    else
        newmobj = P_SpawnMobj (x , y, z, MT_SKULL);

    // Check for movements.
    if (!P_TryMove (newmobj, newmobj->x, newmobj->y))
    {
        // kill it immediately
        P_DamageMobj (newmobj,actor,actor,10000);        
        return;
    }
                
    // Lost Souls bleed Puffs
    if (d_colblood2 && d_chkblood2)
        newmobj->flags |= MF_NOBLOOD;

    P_SetTarget(&newmobj->target, actor->target);
    A_SkullAttack (newmobj);
}


//
// A_PainAttack
// Spawn a lost soul and launch it at the target
// 
void A_PainAttack (mobj_t* actor)
{
    if (!actor->target)
        return;

    A_FaceTarget (actor);
    A_PainShootSkull (actor, actor->angle);
}


void A_PainDie (mobj_t* actor)
{
    A_Fall (actor);
    A_PainShootSkull (actor, actor->angle+ANG90);
    A_PainShootSkull (actor, actor->angle+ANG180);
    A_PainShootSkull (actor, actor->angle+ANG270);
}


void A_Scream (mobj_t* actor)
{
    int                sound;
        
    switch (actor->info->deathsound)
    {
      case 0:
        return;
                
      case sfx_podth1:
      case sfx_podth2:
      case sfx_podth3:
        sound = sfx_podth1 + P_Random ()%3;
        break;
                
      case sfx_bgdth1:
      case sfx_bgdth2:
        sound = sfx_bgdth1 + P_Random ()%2;
        break;
        
      default:
        sound = actor->info->deathsound;
        break;
    }

    // Check for bosses.
    if (actor->type==MT_SPIDER
        || actor->type == MT_CYBORG)
    {
        // full volume
        S_StartSound (NULL, sound);
    }
    else
        S_StartSound (actor, sound);
}


void A_XScream (mobj_t* actor)
{
    S_StartSound (actor, sfx_slop);        
}



//
// A_Explode
//
void A_Explode (mobj_t* thingy)
{
    P_RadiusAttack(thingy, thingy->target, 128);

    if(d_splash)
        P_HitFloor(thingy);
}

// Check whether the death of the specified monster type is allowed
// to trigger the end of episode special action.
//
// This behavior changed in v1.9, the most notable effect of which
// was to break uac_dead.wad

static boolean CheckBossEnd(mobjtype_t motype)
{
    if (gameversion < exe_ultimate)
    {
        if (gamemap != 8)
        {
            return false;
        }

        // Baron death on later episodes is nothing special.

        if(((motype == MT_BRUISER && !beta_style) ||
            (motype == MT_BETABRUISER && beta_style)) && gameepisode != 1)
        {
            return false;
        }

        return true;
    }
    else
    {
        // New logic that appeared in Ultimate Doom.
        // Looks like the logic was overhauled while adding in the
        // episode 4 support.  Now bosses only trigger on their
        // specific episode.

        switch(gameepisode)
        {
            case 1:
                if(beta_style)
                    return gamemap == 8 && motype == MT_BETABRUISER;
                else
                    return gamemap == 8 && motype == MT_BRUISER;

            case 2:
                return gamemap == 8 && motype == MT_CYBORG;

            case 3:
                return gamemap == 8 && motype == MT_SPIDER;

            case 4:
                return (gamemap == 6 && motype == MT_CYBORG)
                    || (gamemap == 8 && motype == MT_SPIDER);

            default:
                return gamemap == 8;
        }
    }
}

//
// A_BossDeath
// Possibly trigger special effects
// if on first boss level
//
void A_BossDeath (mobj_t* mo)
{
    thinker_t*        th;
    mobj_t*        mo2;
    line_t        junk;
    int                i;
                
    if ( gamemode == commercial)
    {
        if (gamemap != 7)
            return;
                
        if ((mo->type != MT_FATSO)
            && (mo->type != MT_BABY))
            return;
    }
    else
    {
        if (!CheckBossEnd(mo->type))
        {
            return;
        }
    }

    // make sure there is a player alive for victory
    for (i=0 ; i<MAXPLAYERS ; i++)
        if (playeringame[i] && players[i].health > 0)
            break;
    
    if (i==MAXPLAYERS)
        return;        // no one left alive, so do not end game
    
    // scan the remaining thinkers to see
    // if all bosses are dead
    for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
    {
        if (th->function.acp1 != (actionf_p1)P_MobjThinker)
            continue;
        
        mo2 = (mobj_t *)th;
        if (mo2 != mo
            && mo2->type == mo->type
            && mo2->health > 0)
        {
            // other boss not dead
            return;
        }
    }
        
    // victory!
    if ( gamemode == commercial)
    {
        if (gamemap == 7)
        {
            if (mo->type == MT_FATSO)
            {
                junk.tag = 666;
                EV_DoFloor(&junk,lowerFloorToLowest);
                return;
            }
            
            if (mo->type == MT_BABY)
            {
                junk.tag = 667;
                EV_DoFloor(&junk,raiseToTexture);
                return;
            }
        }
    }
    else
    {
        switch(gameepisode)
        {
          case 1:
            junk.tag = 666;
            EV_DoFloor (&junk, lowerFloorToLowest);
            return;
            break;
            
          case 4:
            switch(gamemap)
            {
              case 6:
                junk.tag = 666;
                EV_DoDoor (&junk, blazeOpen);
                return;
                break;
                
              case 8:
                junk.tag = 666;
                EV_DoFloor (&junk, lowerFloorToLowest);
                return;
                break;
            }
        }
    }
        
    G_ExitLevel ();
}


void A_Footstep (mobj_t* mo)
{
    player_t*        player = &players[consoleplayer];

    sector_t*        sector = player->mo->subsector->sector;

    if(d_footstep &&
           !mo->player->powers[pw_flight] &&
           !mo->player->jumpTics &&
           player->mo->z == sector->floor_height)
    {
        int t = P_Random() % 4;

        if(old_t == t)
            t = P_Random() % 4;

        if(!not_walking)
        {
            if (P_GetThingFloorType(mo) < 51 ||
                    (P_GetThingFloorType(mo) > 53  && P_GetThingFloorType(mo) < 69)  ||
                    (P_GetThingFloorType(mo) > 76  && P_GetThingFloorType(mo) < 89)  ||
                    (P_GetThingFloorType(mo) > 91  && P_GetThingFloorType(mo) < 136) ||
                    P_GetThingFloorType(mo) > 147)
            {
                if(!snd_module)
                    S_StartSound (mo, sfx_step0 + t);
            }
            else if (P_GetThingFloorType(mo) > 68  && P_GetThingFloorType(mo) < 73)
            {
                if(!snd_module)
                    S_StartSound (mo, sfx_water);
            }
            else if((P_GetThingFloorType(mo) > 50  && P_GetThingFloorType(mo) < 54)  ||
                    (P_GetThingFloorType(mo) > 135 && P_GetThingFloorType(mo) < 144) ||
                    (P_GetThingFloorType(mo) > 72  && P_GetThingFloorType(mo) < 77)  ||
                    (P_GetThingFloorType(mo) > 88  && P_GetThingFloorType(mo) < 92)  ||
                    (P_GetThingFloorType(mo) > 143 && P_GetThingFloorType(mo) < 148))
            {
                if(!snd_module)
                {
                    if(!(players[consoleplayer].cheats & CF_GODMODE))
                        S_StartSound (mo, sfx_lava);
                    else
                        S_StartSound (mo, sfx_water);
                }
            }
        }
        old_t = t;
    }
}

void A_Hoof (mobj_t* mo)
{
    if(fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
            fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
            fsize != 4196020 && fsize != 4225460 && fsize != 4234124)
        S_StartSound (mo, sfx_hoof);
    A_Chase (mo);
}

void A_Metal (mobj_t* mo)
{
    if(fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
            fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
            fsize != 4196020 && fsize != 4225460 && fsize != 4234124)
        S_StartSound (mo, sfx_metal);
    A_Chase (mo);
}

void A_BabyMetal (mobj_t* mo)
{
    S_StartSound (mo, sfx_bspwlk);
    A_Chase (mo);
}

void A_OpenShotgun2 (player_t* player, pspdef_t* psp)
{
    S_StartSound (player->mo, sfx_dbopn);
}

void A_LoadShotgun2 (player_t* player, pspdef_t* psp )
{
    S_StartSound (player->mo, sfx_dbload);
}


void A_CloseShotgun2 (player_t* player, pspdef_t* psp )
{
    S_StartSound (player->mo, sfx_dbcls);
    A_ReFire(player,psp);
}



void A_BrainAwake (mobj_t* mo)
{
    thinker_t*        thinker;
    mobj_t*        m;
        
    // find all the target spots
    numbraintargets = 0;
    braintargeton = 0;
        
    thinker = thinkercap.next;
    for (thinker = thinkercap.next ;
         thinker != &thinkercap ;
         thinker = thinker->next)
    {
        if (thinker->function.acp1 != (actionf_p1)P_MobjThinker)
            continue;        // not a mobj

        m = (mobj_t *)thinker;

        if (m->type == MT_BOSSTARGET )
        {
            // remove braintargets limit
            if (numbraintargets == maxbraintargets)
            {
                maxbraintargets = maxbraintargets ? 2 * maxbraintargets : 32;
                braintargets = realloc(braintargets, maxbraintargets * sizeof(*braintargets));

                if (maxbraintargets > 32)
                    C_Printf(CR_GOLD, " R_BrainAwake: Raised braintargets limit to %d.\n", maxbraintargets);
            }

            braintargets[numbraintargets] = m;
            numbraintargets++;
        }
    }
        
    S_StartSound (NULL,sfx_bossit);

    // no spawn spots available
    if (numbraintargets == 0)
        numbraintargets = INT_MIN;
}


void A_BrainPain (mobj_t*        mo)
{
    A_MoreBlood(mo);

    S_StartSound (NULL,sfx_bospn);
}


void A_BrainScream (mobj_t*        mo)
{
    int                x;
    int                y;
    int                z;
    mobj_t*        th;
        
    for (x=mo->x - 196*FRACUNIT ; x< mo->x + 320*FRACUNIT ; x+= FRACUNIT*8)
    {
        y = mo->y - 320*FRACUNIT;
        z = 128 + P_Random()*2*FRACUNIT;
        th = P_SpawnMobj (x,y,z, MT_ROCKET);
        th->momz = P_Random()*512;

        P_SetMobjState (th, S_BRAINEXPLODE1);

        th->tics -= P_Random()&7;
        if (th->tics < 1)
            th->tics = 1;
    }
        
    S_StartSound (NULL,sfx_bosdth);
}



void A_BrainExplode (mobj_t* mo)
{
    int                x;
    int                y;
    int                z;
    mobj_t*        th;
        
    x = mo->x + (P_Random () - P_Random ())*2048;
    y = mo->y;
    z = 128 + P_Random()*2*FRACUNIT;
    th = P_SpawnMobj (x,y,z, MT_ROCKET);
    th->momz = P_Random()*512;

    P_SetMobjState (th, S_BRAINEXPLODE1);

    th->tics -= P_Random()&7;
    if (th->tics < 1)
        th->tics = 1;

    th->flags |= MF_TRANSLUCENT;
}


void A_BrainDie (mobj_t*        mo)
{
    G_ExitLevel ();
}

void A_BrainSpit (mobj_t*        mo)
{
    mobj_t*        targ;
    mobj_t*        newmobj;
    
    static int        easy = 0;
        
    easy ^= 1;
    if (gameskill <= sk_easy && (!easy))
        return;
                
    // avoid division by zero by recalculating the number of spawn spots
    if (numbraintargets == 0)
        A_BrainAwake(NULL);

    // still no spawn spots available
    if (numbraintargets == INT_MIN)
        return;

    // shoot a cube at current target
    targ = braintargets[braintargeton];
    braintargeton = (braintargeton+1)%numbraintargets;

    // spawn brain missile
    newmobj = P_SpawnMissile (mo, targ, MT_SPAWNSHOT);
    P_SetTarget(&newmobj->target, targ);
    newmobj->reactiontime =
        ((targ->y - mo->y)/newmobj->momy) / newmobj->state->tics;

    S_StartSound(NULL, sfx_bospit);
}


void A_SpawnFly (mobj_t* mo)
{
    mobj_t*        newmobj;
    mobj_t*        fog;
    mobj_t*        targ;
    int                r;
    mobjtype_t        type;
        
    if (--mo->reactiontime)
        return;        // still flying
        
    targ = P_SubstNullMobj(mo->target);

    // First spawn teleport fog.
    fog = P_SpawnMobj (targ->x, targ->y, targ->z, MT_SPAWNFIRE);
    S_StartSound (fog, sfx_telept);

    // Randomly select monster to spawn.
    r = P_Random ();

    // Probability distribution (kind of :),
    // decreasing likelihood.
    if ( r<50 )
        type = MT_TROOP;
    else if (r<90)
        type = MT_SERGEANT;
    else if (r<120)
        type = MT_SHADOWS;
    else if (r<130)
        type = MT_PAIN;
    else if (r<160)
    {
        if(beta_style)
            type = MT_BETAHEAD;
        else
            type = MT_HEAD;
    }
    else if (r<162)
        type = MT_VILE;
    else if (r<172)
        type = MT_UNDEAD;
    else if (r<192)
        type = MT_BABY;
    else if (r<222)
        type = MT_FATSO;
    else if (r<246)
        type = MT_KNIGHT;
    else
    {
        if(beta_style)
            type = MT_BETABRUISER;
        else
            type = MT_BRUISER;
    }

    newmobj = P_SpawnMobj (targ->x, targ->y, targ->z, type);

    if (P_LookForPlayers (newmobj, true) )
        P_SetMobjState (newmobj, newmobj->info->seestate);
        
    // telefrag anything in this spot
    P_TeleportMove (newmobj, newmobj->x, newmobj->y);

    if ((mo->z <= mo->floorz) && (P_HitFloor(mo) != FLOOR_SOLID) && d_splash)
    {                           // Landed in some sort of liquid
        // remove self (i.e., cube).
        P_RemoveMobj (mo);
    }
}


// travelling cube sound
void A_SpawnSound (mobj_t* mo)        
{
    S_StartSound (mo,sfx_boscub);
    A_SpawnFly(mo);
}




void A_PlayerScream (mobj_t* mo)
{
    // Default death sound.
    int                sound = sfx_pldeth;
        
    if ( (gamemode == commercial)
        &&         (mo->health < -50))
    {
        // IF THE PLAYER DIES
        // LESS THAN -50% WITHOUT GIBBING
        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
            sound = sfx_pdiehi;
    }
    
    S_StartSound (mo, sound);
}

//
// A_MoreGibs
//
// Spawns gibs when organic actors get splattered.
//

void A_MoreGibs(mobj_t* actor)
{
    if(d_maxgore)
    {
        mobj_t* mo;
        angle_t an;
        int t;

        if((actor->type == MT_SKULL ||
               actor->type == MT_BETASKULL) && d_colblood2 && d_chkblood2)
        {
            actor->flags |= MF_NOBLOOD;
            goto skip;
        }

        // max gore - ludicrous gibs
        mo = P_SpawnMobj(actor->x, actor->y, actor->z + (24*FRACUNIT), MT_FLESH);

        // added for colored blood and gore!
        mo->target = actor;

        P_SetMobjState(mo, mo->info->spawnstate + (P_Random() % 19));

        an = (P_Random() << 13) / 255;
        mo->angle = an << ANGLETOFINESHIFT;

        mo->momx = FixedMul(finecosine[an], (P_Random() & 0x0f) << FRACBITS);
        mo->momy = FixedMul(finesine[an], (P_Random() & 0x0f) << FRACBITS);
        mo->momz = (P_Random() & 0x0f) << FRACBITS;

        on_ground = (actor->z <= actor->floorz);

        if(on_ground)
        {
            t = P_Random() % 9;

            if((t == 0 || t == 1 || t == 2 || t == 3 || t == 4 || t == 5 || 
                t == 6 || t == 7 || t == 8 || t == 9) && t != old_u)
            {
                if(!snd_module)
                    S_StartSound(actor, sfx_splsh0 + t);

                old_u = t;
            }
        }

        // even more ludicrous gore
        if(d_maxgore && !(actor->flags & MF_NOBLOOD))
        {
            mobj_t *gore = P_SpawnMobj(actor->x,
                                       actor->y,
                                       actor->z + (32*FRACUNIT), MT_GORE);

            // added for colored blood and gore!
            gore->target = mo;

            gore->angle = mo->angle;

            gore->momx = mo->momx;
            gore->momy = mo->momy;
            gore->momz = mo->momz;
        }
        skip: ;
    }
}

