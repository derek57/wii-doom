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

#ifdef WII
#include "../c_io.h"
#include "../d_deh.h"
#else
#include "c_io.h"
#include "d_deh.h"
#endif

#include "doomdef.h"

#ifdef WII
#include "../doomfeatures.h"
#else
#include "doomfeatures.h"
#endif

// State.
#include "doomstat.h"

#include "g_game.h"

#ifdef WII
#include "../i_system.h"
#include "../m_bbox.h"
#else
#include "i_system.h"
#include "m_bbox.h"
#endif

#include "m_random.h"
#include "p_local.h"
#include "p_tick.h"
#include "r_state.h"
#include "s_sound.h"

// Data.
#include "sounds.h"

#ifdef WII
#include "../z_zone.h"
#else
#include "z_zone.h"
#endif


#define        FATSPREAD            (ANG90/8)
#define        SKULLSPEED           (20*FRACUNIT)
#define        SPLAT_PER_COUNTER    1
#define        MONS_LOOK_RANGE      (32 * 64 * FRACUNIT)


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

// Codepointer operation types
/*
enum
{
   CPOP_ASSIGN,
   CPOP_ADD,
   CPOP_SUB,
   CPOP_MUL,
   CPOP_DIV,
   CPOP_MOD,
   CPOP_AND,
   CPOP_ANDNOT,
   CPOP_OR,
   CPOP_XOR,
   CPOP_RND,
   CPOP_RNDMOD,
   CPOP_DAMAGE,
   CPOP_SHIFTLEFT,
   CPOP_SHIFTRIGHT,

   // unary operators
   CPOP_ABS,
   CPOP_NEGATE,
   CPOP_NOT,
   CPOP_INVERT,
};

// Codepointer comparison types

enum
{
   CPC_LESS,
   CPC_LESSOREQUAL,
   CPC_GREATER,
   CPC_GREATEROREQUAL,
   CPC_EQUAL,
   CPC_NOTEQUAL,
   CPC_BITWISEAND,
   
   CPC_CNTR_LESS,           // alternate counter versions
   CPC_CNTR_LESSOREQUAL,
   CPC_CNTR_GREATER,
   CPC_CNTR_GREATEROREQUAL,
   CPC_CNTR_EQUAL,
   CPC_CNTR_NOTEQUAL,
   CPC_CNTR_BITWISEAND,

   CPC_NUMIMMEDIATE = CPC_BITWISEAND + 1
};
*/

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

boolean           on_ground;

int               old_t;
int               old_u;
int               TRACEANGLE = 0xc000000;
int               numbraintargets = 0; // [crispy] initialize
int               braintargeton = 0;
int               numsplats;

static int        maxbraintargets;     // [crispy] remove braintargets limit

extern boolean    not_walking;

extern line_t     **spechit;

extern int        numspechit;
extern int        snd_module;
extern int        gore_amount;

extern void A_ReFire
( player_t*        player,
  pspdef_t*        psp );

//
// P_IsOnLift
//
// killough 9/9/98:
//
// Returns true if the object is on a lift. Used for AI,
// since it may indicate the need for crowded conditions,
// or that a monster should stay on the lift for a while
// while it goes up or down.
//
static boolean P_IsOnLift(const mobj_t *actor)
{
    const sector_t      *sec = actor->subsector->sector;
    line_t              line;

    // Short-circuit: it's on a lift which is active.
    if (sec->floordata && ((thinker_t *)sec->floordata)->function == T_PlatRaise)
        return true;

    // Check to see if it's in a sector which can be activated as a lift.
    if ((line.tag = sec->tag))
    {
        int     l;

        for (l = -1; (l = P_FindLineFromLineTag(&line, l)) >= 0;)
            switch (lines[l].special)
            {
                case W1_Lift_LowerWaitRaise:
                case S1_Floor_RaiseBy32_ChangesTexture:
                case S1_Floor_RaiseBy24_ChangesTexture:
                case S1_Floor_RaiseToNextHighestFloor_ChangesTexture:
                case S1_Lift_LowerWaitRaise:
                case W1_Floor_RaiseToNextHighestFloor_ChangesTexture:
                case G1_Floor_RaiseToNextHighestFloor_ChangesTexture:
                case W1_Floor_StartMovingUpAndDown:
                case SR_Lift_LowerWaitRaise:
                case SR_Floor_RaiseBy24_ChangesTexture:
                case SR_Floor_RaiseBy32_ChangesTexture:
                case SR_Floor_RaiseToNextHighestFloor_ChangesTexture:
                case WR_Floor_StartMovingUpAndDown:
                case WR_Lift_LowerWaitRaise:
                case WR_Floor_RaiseToNextHighestFloor_ChangesTexture:
                case WR_Lift_LowerWaitRaise_Fast:
                case W1_Lift_LowerWaitRaise_Fast:
                case S1_Lift_LowerWaitRaise_Fast:
                case SR_Lift_LowerWaitRaise_Fast:
                    return true;
            }
        }

    return false;
}

//
// P_IsUnderDamage
//
// killough 9/9/98:
//
// Returns nonzero if the object is under damage based on
// their current position. Returns 1 if the damage is moderate,
// -1 if it is serious. Used for AI.
//
static int P_IsUnderDamage(mobj_t *actor)
{
    const struct msecnode_s     *seclist;
    int                         dir = 0;

    for (seclist = actor->touching_sectorlist; seclist; seclist = seclist->m_tnext)
    {
        const ceiling_t          *cl;    // Crushing ceiling

        if ((cl = seclist->m_sector->ceilingdata) && cl->thinker.function == T_MoveCeiling)
            dir |= cl->direction;
    }
    return dir;
}

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
        
    if (players[0].cheats & CF_NOTARGET)
        return;

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
    mobj_t      *pl = actor->target;
    fixed_t     dist;

    if (!pl)
        return false;

    dist = P_AproxDistance(pl->x - actor->x, pl->y - actor->y);

    if (dist >= MELEERANGE - 20 * FRACUNIT + pl->info->radius)
        return false;

    // [BH] check difference in height as well
    if (pl->z > actor->z + actor->height || actor->z > pl->z + pl->height)
        return false;

    if (!P_CheckSight(actor, pl))
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
        
    if (actor->flags & MF_JUSTHIT)
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

static boolean P_Move(mobj_t *actor, boolean dropoff)   // killough 9/12/98
{
    fixed_t        tryx;
    fixed_t        tryy;
    fixed_t        deltax;
    fixed_t        deltay;
    
    int            movefactor = ORIG_FRICTION_FACTOR;   // killough 10/98
    int            friction = ORIG_FRICTION;
    int            speed;

    // warning: 'catch', 'throw', and 'try'
    // are all C++ reserved words
    boolean        try_ok;
                
    if (actor->movedir == DI_NODIR)
        return false;
                
    if ((unsigned)actor->movedir >= 8)
        I_Error ("Weird actor->movedir!");

    // [RH] Instead of yanking non-floating monsters to the ground,
    // let gravity drop them down, unless they're moving down a step.
    if (!(actor->flags & MF_NOGRAVITY) && actor->z > actor->floorz
        && !(actor->flags2 & MF2_ONMOBJ))
    {
        if (actor->z > actor->floorz + 24 * FRACUNIT)
            return false;
        else
            actor->z = actor->floorz;
    }

    // killough 10/98: make monsters get affected by ice and sludge too:
    movefactor = P_GetMoveFactor(actor, &friction);
    speed = actor->info->speed;

    if (friction < ORIG_FRICTION        // sludge
        && !(speed = ((ORIG_FRICTION_FACTOR - (ORIG_FRICTION_FACTOR - movefactor) / 2)
        * speed) / ORIG_FRICTION_FACTOR))
        speed = 1;                      // always give the monster a little bit of speed

    tryx = actor->x + (deltax = speed * xspeed[actor->movedir]);
    tryy = actor->y + (deltay = speed * yspeed[actor->movedir]);

    // killough 12/98: rearrange, fix potential for stickiness on ice
    if (friction <= ORIG_FRICTION)
        try_ok = P_TryMove(actor, tryx, tryy, dropoff);
    else
    {
        fixed_t x = actor->x;
        fixed_t y = actor->y;
        fixed_t floorz = actor->floorz;
        fixed_t ceilingz = actor->ceilingz;
        fixed_t dropoffz = actor->dropoffz;

        try_ok = P_TryMove(actor, tryx, tryy, dropoff);

        // killough 10/98:
        // Let normal momentum carry them, instead of steptoeing them across ice.
        if (try_ok)
        {
            P_UnsetThingPosition(actor);
            actor->x = x;
            actor->y = y;
            actor->floorz = floorz;
            actor->ceilingz = ceilingz;
            actor->dropoffz = dropoffz;
            P_SetThingPosition(actor);
            movefactor *= FRACUNIT / ORIG_FRICTION_FACTOR / 4;
            actor->momx += FixedMul(deltax, movefactor);
            actor->momy += FixedMul(deltay, movefactor);
        }
    }

    if (!try_ok)
    {
        // open any specials
        int     good;

        if ((actor->flags & MF_FLOAT) && floatok)
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
                good |= spechit[numspechit] == blockline ? 1 : 2;

        if (!good || d_doorstuck)
            return good;

        return good && ((P_Random() >= 230) ^ (good & 1));
    }
    else
        actor->flags &= ~MF_INFLOAT;

    if (!(actor->flags & MF_FLOAT) && !felldown)
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
// P_SmartMove
//
// killough 9/12/98: Same as P_Move, except smarter
//
static boolean P_SmartMove(mobj_t *actor)
{
    mobj_t      *target = actor->target;
    int         on_lift;
    int         under_damage;

    // killough 9/12/98: Stay on a lift if target is on one
    on_lift = (target && target->health > 0
        && target->subsector->sector->tag == actor->subsector->sector->tag && P_IsOnLift(actor));

    under_damage = P_IsUnderDamage(actor);

    if (!P_Move(actor, false))
        return false;

    // killough 9/9/98: avoid crushing ceilings or other damaging areas
    if ((on_lift && P_Random() < 230         // Stay on lift
         && !P_IsOnLift(actor))
        || (!under_damage                    // Get away from damage
            && (under_damage = P_IsUnderDamage(actor))
            && (under_damage < 0 || P_Random() < 200)))
        actor->movedir = DI_NODIR;           // avoid the area (most of the time anyway)

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
//    if (!P_Move (actor, false))
    if (!P_SmartMove(actor))
    {
        return false;
    }

    actor->movecount = P_Random()&15;
    return true;
}




//
// P_DoNewChaseDir
//
// killough 9/8/98:
//
// Most of P_NewChaseDir(), except for what
// determines the new direction to take
//
static void P_DoNewChaseDir(mobj_t *actor, fixed_t deltax, fixed_t deltay)
{
    dirtype_t   xdir, ydir, tdir;
    dirtype_t   olddir = actor->movedir;
    dirtype_t   turnaround = olddir;

    // find reverse direction
    if (turnaround != DI_NODIR)
        turnaround ^= 4;

    xdir = (deltax >  10 * FRACUNIT ? DI_EAST : (deltax < -10 * FRACUNIT ? DI_WEST : DI_NODIR));
    ydir = (deltay < -10 * FRACUNIT ? DI_SOUTH : (deltay >  10 * FRACUNIT ? DI_NORTH : DI_NODIR));

    // try direct route
    if (xdir != DI_NODIR && ydir != DI_NODIR && turnaround !=
        (actor->movedir = deltay < 0 ? deltax > 0 ? DI_SOUTHEAST : DI_SOUTHWEST :
        deltax > 0 ? DI_NORTHEAST : DI_NORTHWEST) && P_TryWalk(actor))
        return;

    // try other directions
    if (P_Random() > 200 || ABS(deltay) > ABS(deltax))
    {
        tdir = xdir;
        xdir = ydir;
        ydir = tdir;
    }

    if ((xdir == turnaround ? xdir = DI_NODIR : xdir) != DI_NODIR
        && (actor->movedir = xdir, P_TryWalk(actor)))
        return;         // either moved forward or attacked

    if ((ydir == turnaround ? ydir = DI_NODIR : ydir) != DI_NODIR
        && (actor->movedir = ydir, P_TryWalk(actor)))
        return;

    // there is no direct path to the player, so pick another direction.
    if (olddir != DI_NODIR && (actor->movedir = olddir, P_TryWalk(actor)))
        return;

    // randomly determine direction of search
    if (P_Random() & 1)
    {
        for (tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++)
            if (tdir != turnaround && (actor->movedir = tdir, P_TryWalk(actor)))
                return;
    }
    else
        for (tdir = DI_SOUTHEAST; tdir != DI_EAST - 1; tdir--)
            if (tdir != turnaround && (actor->movedir = tdir, P_TryWalk(actor)))
                return;

    if ((actor->movedir = turnaround) != DI_NODIR && !P_TryWalk(actor))
        actor->movedir = DI_NODIR;
}

//
// killough 11/98:
//
// Monsters try to move away from tall dropoffs.
//
// In Doom, they were never allowed to hang over dropoffs,
// and would remain stuck if involuntarily forced over one.
// This logic, combined with p_map.c (P_TryMove), allows
// monsters to free themselves without making them tend to
// hang over dropoffs.
//
static fixed_t  dropoff_deltax;
static fixed_t  dropoff_deltay;
static fixed_t  floorz;

static boolean PIT_AvoidDropoff(line_t *line)
{
    if (line->backsector                                // Ignore one-sided linedefs
        && tmbbox[BOXRIGHT]  > line->bbox[BOXLEFT]
        && tmbbox[BOXLEFT]   < line->bbox[BOXRIGHT]
        && tmbbox[BOXTOP]    > line->bbox[BOXBOTTOM]    // Linedef must be contacted
        && tmbbox[BOXBOTTOM] < line->bbox[BOXTOP]
        && P_BoxOnLineSide(tmbbox, line) == -1)
    {
        fixed_t front = line->frontsector->floorheight;
        fixed_t back = line->backsector->floorheight;
        angle_t angle;

        // The monster must contact one of the two floors,
        // and the other must be a tall dropoff (more than 24).
        if (back == floorz && front < floorz - FRACUNIT * 24)
            angle = R_PointToAngle2(0, 0, line->dx, line->dy);          // front side dropoff
        else if (front == floorz && back < floorz - FRACUNIT * 24)
            angle = R_PointToAngle2(line->dx, line->dy, 0, 0);          // back side dropoff
        else
            return true;

        // Move away from dropoff at a standard speed.
        // Multiple contacted linedefs are cumulative (e.g. hanging over corner)
        dropoff_deltax -= finesine[angle >> ANGLETOFINESHIFT] * 32;
        dropoff_deltay += finecosine[angle >> ANGLETOFINESHIFT] * 32;
    }
    return true;
}

//
// Driver for above
//
static fixed_t P_AvoidDropoff(mobj_t *actor)
{
    int yh = ((tmbbox[BOXTOP] = actor->y + actor->radius) - bmaporgy) >> MAPBLOCKSHIFT;
    int yl = ((tmbbox[BOXBOTTOM] = actor->y - actor->radius) - bmaporgy) >> MAPBLOCKSHIFT;
    int xh = ((tmbbox[BOXRIGHT] = actor->x + actor->radius) - bmaporgx) >> MAPBLOCKSHIFT;
    int xl = ((tmbbox[BOXLEFT] = actor->x - actor->radius) - bmaporgx) >> MAPBLOCKSHIFT;
    int bx, by;

    floorz = actor->z;                                          // remember floor height

    dropoff_deltax = dropoff_deltay = 0;

    // check lines
    validcount++;
    for (bx = xl; bx <= xh; bx++)
        for (by = yl; by <= yh; by++)
            P_BlockLinesIterator(bx, by, PIT_AvoidDropoff);     // all contacted lines

    return (dropoff_deltax | dropoff_deltay);                   // Non-zero if movement prescribed
}

//
// P_NewChaseDir
//
// killough 9/8/98: Split into two functions
//
static void P_NewChaseDir(mobj_t *actor)
{
    mobj_t      *target = actor->target;
    fixed_t     deltax = target->x - actor->x;
    fixed_t     deltay = target->y - actor->y;

    if (actor->floorz - actor->dropoffz > FRACUNIT * 24
        && actor->z <= actor->floorz
        && !(actor->flags & (MF_DROPOFF | MF_FLOAT))
        && P_AvoidDropoff(actor))       // Move away from dropoff
    {
        P_DoNewChaseDir(actor, dropoff_deltax, dropoff_deltay);

        // If moving away from dropoff, set movecount to 1 so that
        // small steps are taken to get monster away from dropoff.
        actor->movecount = 1;
        return;
    }

    P_DoNewChaseDir(actor, deltax, deltay);
}



//
// P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//
static boolean P_LookForMonsters(mobj_t *actor)
{
    mobj_t      *mo;
    thinker_t   *think;

    if (!P_CheckSight(players[0].mo, actor))
        return false;           // player can't see monster

    for (think = thinkerclasscap[th_mobj].cnext; think != &thinkerclasscap[th_mobj];
        think = think->cnext)
    {
        mo = (mobj_t *)think;
        if (!(mo->flags & MF_COUNTKILL) || mo == actor || mo->health <= 0)
            continue;           // not a valid monster

        if (P_AproxDistance(actor->x - mo->x, actor->y - mo->y) > MONS_LOOK_RANGE)
            continue;           // out of range

        if (!P_CheckSight(actor, mo))
            continue;           // out of sight

        // Found a target monster
        if(last_enemy)
            P_SetTarget(&actor->lastenemy, actor->target);

        P_SetTarget(&actor->target, mo);
        return true;
    }
    return false;
}

static boolean P_LookForPlayers(mobj_t *actor, boolean allaround)
{
    player_t    *player;
    mobj_t      *mo;
    angle_t     an;
    fixed_t     dist;

    if (infight)
        // player is dead, look for monsters
        return P_LookForMonsters(actor);

    player = &players[0];
    mo = player->mo;

    if (player->cheats & CF_NOTARGET)
        return false;

    if (player->health <= 0 || !P_CheckSight(actor, mo))
    {
        // Use last known enemy if no players sighted -- killough 2/15/98
        if (actor->lastenemy && actor->lastenemy->health > 0 && last_enemy)
        {
            P_SetTarget(&actor->target, actor->lastenemy);
            P_SetTarget(&actor->lastenemy, NULL);
            return true;
        }
        return false;
    }

    dist = P_AproxDistance(mo->x - actor->x, mo->y - actor->y);

    if (!allaround)
    {
        an = R_PointToAngle2(actor->x, actor->y, mo->x, mo->y) - actor->angle;

        if (an > ANG90 && an < ANG270)
        {
            // if real close, react anyway
            if (dist > MELEERANGE)
            {
                // Use last known enemy if no players sighted -- killough 2/15/98
                if (actor->lastenemy && actor->lastenemy->health > 0 && last_enemy)
                {
                    P_SetTarget(&actor->target, actor->lastenemy);
                    P_SetTarget(&actor->lastenemy, NULL);
                    return true;
                }
                return false;
            }
        }
    }

    if (mo->flags & MF_SHADOW)
    {
        // player is invisible
        if (dist > 2 * MELEERANGE && P_AproxDistance(mo->momx, mo->momy) < 5 * FRACUNIT)
            return false;       // player is sneaking - can't detect
        if (P_Random() < 225)
            return false;       // player isn't sneaking, but still didn't detect
    }

    P_SetTarget(&actor->target, mo);
    actor->threshold = 60;
    return true;
}

void A_Pain (mobj_t* actor)
{
    if (actor->info->painsound)
        S_StartSound (actor, actor->info->painsound);
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
        int color = ((d_chkblood && d_colblood) ? actor->blood : MT_BLOOD);
        mobjinfo_t *info = &mobjinfo[color];
        mobj_t *mo = Z_Malloc(sizeof(*mo), PU_LEVEL, NULL);

        if((actor->type == MT_SKULL ||
               actor->type == MT_BETASKULL) && d_colblood2 && d_chkblood2)
            goto skip;

        for(i = 0; i < 8; i++)
        {
            mo->type = color;

            // added for colored blood and gore!
            mo->target = actor;

            // spray blood in a random direction (colored)
            if((actor->type == MT_HEAD || actor->type == MT_BETAHEAD) && d_chkblood && d_colblood)
                mo = P_SpawnMobj(actor->x,
                                 actor->y,
                                 actor->z + actor->info->height/2, MT_BLUESPRAY);
            else if((actor->type == MT_BRUISER || actor->type == MT_BETABRUISER ||
                    actor->type == MT_KNIGHT) && d_chkblood && d_colblood)
                mo = P_SpawnMobj(actor->x,
                                 actor->y,
                                 actor->z + actor->info->height/2, MT_GREENSPRAY);
            else if(actor->type == MT_SHADOWS && d_colblood2 && d_chkblood2)
                mo = P_SpawnMobj(actor->x,
                                 actor->y,
                                 actor->z + actor->info->height/2, MT_FUZZYSPRAY);
            else
                mo = P_SpawnMobj(actor->x,
                                 actor->y,
                                 actor->z + actor->info->height/2, MT_SPRAY);

            mo->colfunc = info->colfunc;

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
    for (th = thinkerclasscap[th_mobj].cnext; th != &thinkerclasscap[th_mobj]; th = th->cnext)
    {
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
    EV_DoDoor(&junk,doorOpen);
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

    if(beta_style && (actor->flags & MF_COUNTKILL) &&
            actor->state->frame == 1 && actor->state->tics > 0)
        actor->state->tics = 0;

    if (targ
        && (targ->flags & MF_SHOOTABLE))
    {
        P_SetTarget(&actor->target, targ);

        if (actor->flags & MF_AMBUSH)
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

    if (actor->shadow)
        actor->shadow->angle = actor->angle;

    if (!actor->target
        || !(actor->target->flags & MF_SHOOTABLE))
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
//        || !P_Move (actor, false))
        || !P_SmartMove (actor))
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

    if (actor->shadow)
        actor->shadow->angle = actor->angle;
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
    actor->frame |= FF_FULLBRIGHT;
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
    actor->frame |= FF_FULLBRIGHT;
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
    actor->frame |= FF_FULLBRIGHT;
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
        
    if (!(thing->flags & MF_CORPSE))
        return true;        // not a monster
    
    if (thing->tics != -1)
        return true;        // not lying still yet
    
    if (thing->info->raisestate == S_NULL)
        return true;        // monster doesn't have a raise state
    
    maxdist = thing->info->radius + mobjinfo[MT_VILE].radius;
        
    if ( ABS(thing->x - viletryx) > maxdist
         || ABS(thing->y - viletryy) > maxdist )
        return true;                // not actually touching
                
    corpsehit = thing;
    corpsehit->momx = corpsehit->momy = 0;

    if (d_resurrectghosts)                                          // phares
    {                                                               //   |
        corpsehit->height <<= 2;                                    //   V
        check = P_CheckPosition(corpsehit, corpsehit->x, corpsehit->y);
        corpsehit->height >>= 2;
    }
    else
    {
        int height, radius;

        height = corpsehit->height; // save temporarily
        radius = corpsehit->radius; // save temporarily
        corpsehit->height = corpsehit->info->height;
        corpsehit->radius = corpsehit->info->radius;
        corpsehit->flags |= MF_SOLID;
        corpsehit->flags2 |= MF2_RESURRECTING;
        check = P_CheckPosition(corpsehit, corpsehit->x, corpsehit->y);
        corpsehit->height = height; // restore
        corpsehit->radius = radius; // restore                      //   ^
        corpsehit->flags &= ~MF_SOLID;                              //   |
        corpsehit->flags2 &= ~MF2_RESURRECTING;
    }                                                               // phares

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

                    if (d_resurrectghosts)                            // phares
                        corpsehit->height <<= 2;                      //   |
                    else                                              //   V
                    {
                        corpsehit->height = info->height; // fix Ghost bug
                        corpsehit->radius = info->radius; // fix Ghost bug
                    }                                                 // phares

                    corpsehit->flags = info->flags;

                    if (corpsehit->shadow)
                        corpsehit->shadow->flags2 &= ~MF2_MIRRORED;

                    corpsehit->health = info->spawnhealth;
                    P_SetTarget(&corpsehit->target, NULL);

                    if(last_enemy)
                        P_SetTarget(&corpsehit->lastenemy, NULL);

                    // resurrected pools of gore ("ghost monsters") are translucent
                    if (corpsehit->height == 0 && corpsehit->radius == 0)
                        corpsehit->flags |= MF_TRANSLUCENT;

                    // killough 8/29/98: add to appropriate thread
                    P_UpdateThinker(&corpsehit->thinker);

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

//    damage = (P_RandomSMMU(pr_skullfly)%8+1)*actor->info->damage;
    damage = (P_Random() % 8 + 1) * actor->info->damage;

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
        if (   (currentthinker->function == P_MobjThinker)
            && (((mobj_t *)currentthinker)->type == MT_SKULL ||
                ((mobj_t *)currentthinker)->type == MT_BETASKULL))
            count++;
        currentthinker = currentthinker->next;
    }

    // if there are allready 20 skulls on the level,
    // don't spit another one
    if (count > 20 && d_limitedghosts)
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

    if (!d_blockskulls)   // killough 10/98: compatibility-optioned
    {
        if (beta_skulls)
            newmobj = P_SpawnMobj (x, y, z, MT_BETASKULL);
        else
            newmobj = P_SpawnMobj (x, y, z, MT_SKULL);
    }
    else                                                            // phares
    {
        // Check whether the Lost Soul is being fired through a 1-sided
        // wall or an impassible line, or a "monsters can't cross" line.
        // If it is, then we don't allow the spawn. This is a bug fix, but
        // it should be considered an enhancement, since it may disturb
        // existing demos, so don't do it in compatibility mode.

        if (Check_Sides(actor,x,y))
            return;

        if (beta_skulls)
            newmobj = P_SpawnMobj (x, y, z, MT_BETASKULL);
        else
            newmobj = P_SpawnMobj (x, y, z, MT_SKULL);

        // Check to see if the new Lost Soul's z value is above the
        // ceiling of its new sector, or below the floor. If so, kill it.

        if ((newmobj->z >
                (newmobj->subsector->sector->ceilingheight - newmobj->height)) ||
                (newmobj->z < newmobj->subsector->sector->floorheight))
        {
            // kill it immediately
            P_DamageMobj(newmobj, actor, actor, 10000);
            return;                                                 //   ^
        }                                                           //   |
    }                                                               // phares

    // Check for movements.
    if (!P_TryMove (newmobj, newmobj->x, newmobj->y, false))
    {
        if (newmobj->shadow)
            P_RemoveMobjShadow(newmobj);

        // kill it immediately
        P_DamageMobj (newmobj,actor,actor,10000);        
        return;
    }
                
    // [crispy] Lost Souls bleed Puffs
    if (d_colblood2 && d_chkblood2)
        newmobj->flags |= MF_NOBLOOD;

    P_SetTarget(&newmobj->target, actor->target);
    A_SkullAttack (newmobj);
/*
    mobj_t      *newmobj;
    angle_t     an = angle >> ANGLETOFINESHIFT;
    int         prestep = 4 * FRACUNIT + 3 * (actor->info->radius + mobjinfo[MT_SKULL].radius) / 2;
    fixed_t     x = actor->x + FixedMul(prestep, finecosine[an]);
    fixed_t     y = actor->y + FixedMul(prestep, finesine[an]);
    fixed_t     z = actor->z + 8 * FRACUNIT;

    if (P_CheckLineSide(actor, x, y))
        return;

    if (!d_blockskulls)   // killough 10/98: compatibility-optioned
    {
        if (beta_skulls)
            newmobj = P_SpawnMobj (x, y, z, MT_BETASKULL);
        else
            newmobj = P_SpawnMobj (x, y, z, MT_SKULL);
    }
    else                                                            // phares
    {
        // Check whether the Lost Soul is being fired through a 1-sided
        // wall or an impassible line, or a "monsters can't cross" line.
        // If it is, then we don't allow the spawn. This is a bug fix, but
        // it should be considered an enhancement, since it may disturb
        // existing demos, so don't do it in compatibility mode.

//        if (Check_Sides(actor,x,y))
//            return;

        if (beta_skulls)
            newmobj = P_SpawnMobj (x, y, z, MT_BETASKULL);
        else
            newmobj = P_SpawnMobj (x, y, z, MT_SKULL);

        // Check to see if the new Lost Soul's z value is above the
        // ceiling of its new sector, or below the floor. If so, kill it.

        if ((newmobj->z >
                (newmobj->subsector->sector->ceilingheight - newmobj->height)) ||
                (newmobj->z < newmobj->subsector->sector->floorheight))
        {
            // kill it immediately
            P_DamageMobj(newmobj, actor, actor, 10000);
            return;                                                 //   ^
        }                                                           //   |
    }
    newmobj->flags &= ~MF_COUNTKILL;

    if (!P_TryMove(newmobj, newmobj->x, newmobj->y, false))
    {
        if (newmobj->shadow)
            P_RemoveMobjShadow(newmobj);
        P_RemoveMobj(newmobj);
    }
    else
    {
        // [crispy] Lost Souls bleed Puffs
        if (d_colblood2 && d_chkblood2)
            newmobj->flags |= MF_NOBLOOD;
        else if (!d_colblood2 && !d_chkblood2)
            newmobj->flags &= ~MF_NOBLOOD;

        P_SetTarget(&newmobj->target, actor->target);
        A_SkullAttack(newmobj);
    }
*/
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
    if (gameversion < exe_ultimate || d_666)
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
    for (th = thinkerclasscap[th_mobj].cnext; th != &thinkerclasscap[th_mobj]; th = th->cnext)
    {
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
        if (gamemap == 7 ||
        // [crispy] Master Levels in PC slot 7
        (gamemission == pack_master && (gamemap == 14 || gamemap == 15 || gamemap == 16)))
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
                EV_DoDoor (&junk, doorBlazeOpen);
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
           player->mo->z == sector->floorheight)
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
        
    for (thinker = thinkerclasscap[th_mobj].cnext; thinker != &thinkerclasscap[th_mobj];
        thinker = thinker->cnext)
    {
        m = (mobj_t *)thinker;

        if (m->type == MT_BOSSTARGET )
        {
            // [crispy] remove braintargets limit
            if (numbraintargets == maxbraintargets)
            {
                maxbraintargets = maxbraintargets ? 2 * maxbraintargets : 32;
#ifdef BOOM_ZONE_HANDLING
                braintargets = Z_Realloc(braintargets, maxbraintargets * sizeof(*braintargets), PU_LEVEL, NULL);
#else
                braintargets = Z_Realloc(braintargets, maxbraintargets * sizeof(*braintargets));
#endif
                if (maxbraintargets > 32)
                    C_Printf(CR_GOLD, " R_BrainAwake: Raised braintargets limit to %d.\n", maxbraintargets);
            }

            braintargets[numbraintargets] = m;
            numbraintargets++;
        }
    }
        
    S_StartSound (NULL,sfx_bossit);

    // [crispy] no spawn spots available
    if (numbraintargets == 0)
        numbraintargets = INT_MIN;
}


void A_BrainPain (mobj_t*        mo)
{
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

    // [crispy] brain explosions are translucent
    if(d_translucency)
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
    
    static int        easy;
        
    easy ^= 1;
    if (gameskill <= sk_easy && (!easy))
        return;
                
    // [crispy] avoid division by zero by recalculating the number of spawn spots
    if (numbraintargets == 0)
        A_BrainAwake(NULL);

    // [crispy] still no spawn spots available
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
    P_TeleportMove (newmobj, newmobj->x, newmobj->y, newmobj->z, true); /* killough 8/9/98 */

    if ((mo->z <= mo->floorz) && P_HitFloor(mo) && d_splash)
    {                           // Landed in some sort of liquid
        if (mo->shadow)
            P_RemoveMobjShadow(mo);

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
        int numchunks = gore_amount;

        if((actor->type == MT_SKULL ||
               actor->type == MT_BETASKULL) && d_colblood2 && d_chkblood2)
        {
            actor->flags |= MF_NOBLOOD;
            goto skip;
        }

        do
        {
            // max gore - ludicrous gibs
            mo = P_SpawnMobj(actor->x, actor->y, actor->z + (24*FRACUNIT), MT_FLESH);

//            printf("Sprite: %d, Frame: %d (MT_FLESH)\n", mo->sprite, mo->frame);

            // added for colored blood and gore!
            mo->target = actor;
            mo->flags2 = MF2_DONOTMAP;

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
/*
            // even more ludicrous gore
            if(d_maxgore && !(actor->flags & MF_NOBLOOD))
            {
                mobj_t *gore;

                if(mo->target->type == MT_HEAD || mo->type == MT_BETAHEAD)
                    gore = P_SpawnMobj(actor->x,
                                       actor->y,
                                       actor->z + (32*FRACUNIT), MT_BLUESPRAY);
                else if(mo->type == MT_BRUISER || mo->type == MT_BETABRUISER || mo->type == MT_KNIGHT)
                    gore = P_SpawnMobj(actor->x,
                                       actor->y,
                                       actor->z + (32*FRACUNIT), MT_GREENSPRAY);
                else
                    gore = P_SpawnMobj(actor->x,
                                       actor->y,
                                       actor->z + (32*FRACUNIT), MT_SPRAY);

//                printf("Sprite: %d, Frame: %d (MT_SPRAY)\n", gore->sprite, gore->frame);

                // added for colored blood and gore!
                gore->target = mo;

                gore->angle = mo->angle;

                gore->momx = mo->momx;
                gore->momy = mo->momy;
                gore->momz = mo->momz;
            }
*/
        }
        while(--numchunks > 0);

        skip: ;
    }
}


// killough 11/98: kill an object
void A_Die(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    P_DamageMobj(actor, NULL, NULL, actor->health);
}

//
// A_Detonate
// killough 8/9/98: same as A_Explode, except that the damage is variable
//
void A_Detonate(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    P_RadiusAttack(actor, actor->target, actor->info->damage);
}

//
// killough 9/98: a mushroom explosion effect, sorta :)
// Original idea: Linguica
//
void A_Mushroom(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    int         i;
    int         j;
    int         n = actor->info->damage;

    // Mushroom parameters are part of code pointer's state
    fixed_t     misc1 = (actor->state->misc1 ? actor->state->misc1 : FRACUNIT * 4);
    fixed_t     misc2 = (actor->state->misc2 ? actor->state->misc2 : FRACUNIT / 2);

    A_Explode(actor);                               // First make normal explosion

    // Now launch mushroom cloud
    for (i = -n; i <= n; i += 8)
        for (j = -n; j <= n; j += 8)
        {
            mobj_t      target = *actor;
            mobj_t      *mo;

            target.x += i << FRACBITS;                          // Aim in many directions from source
            target.y += j << FRACBITS;
            target.z += P_AproxDistance(i, j) * misc1;         // Aim up fairly high
            mo = P_SpawnMissile(actor, &target, MT_FATSHOT);    // Launch fireball
            mo->momx = FixedMul(mo->momx, misc2);
            mo->momy = FixedMul(mo->momy, misc2);               // Slow down a bit
            mo->momz = FixedMul(mo->momz, misc2);
            mo->flags &= ~MF_NOGRAVITY;                         // Make debris fall under gravity
        }
}

//
// killough 11/98
//
// The following were inspired by Len Pitre
//
// A small set of highly-sought-after code pointers
//
void A_Spawn(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    if (actor->state->misc1)
        P_SpawnMobj(actor->x, actor->y, (actor->state->misc2 << FRACBITS) + actor->z,
            actor->state->misc1 - 1);
}

void A_Turn(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    actor->angle += (unsigned int)(((uint64_t)actor->state->misc1 << 32) / 360);
}

void A_Face(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    actor->angle = (unsigned int)(((uint64_t)actor->state->misc1 << 32) / 360);
}

void A_Scratch(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    (actor->target && (A_FaceTarget(actor), P_CheckMeleeRange(actor)) ?
        (actor->state->misc2 ? S_StartSound(actor, actor->state->misc2) : (void)0,
        P_DamageMobj(actor->target, actor, actor, actor->state->misc1)) : (void)0);
}

void A_PlaySound(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    S_StartSound((actor->state->misc2 ? NULL : actor), actor->state->misc1);
}

void A_RandomJump(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    if (psp)
    {
        state_t *state = psp->state;

        if (P_Random() < state->misc2)
            P_SetPsprite(player, psp - &player->psprites[0], state->misc1);
    }
    else
    {
        state_t *state = actor->state;

        if (P_Random() < state->misc2)
            P_SetMobjState(actor, state->misc1);
    }
}

//
// This allows linedef effects to be activated inside deh frames.
//
void A_LineEffect(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    static line_t       junk;
    player_t            newplayer;
    player_t            *oldplayer;

    junk = *lines;
    oldplayer = actor->player;
    actor->player = &newplayer;
    newplayer.health = 100;
    junk.special = (short)actor->state->misc1;
    if (!junk.special)
        return;
    junk.tag = (short)actor->state->misc2;
    if (!P_UseSpecialLine(actor, &junk, 0))
        P_CrossSpecialLine(&junk, 0, actor);
    actor->state->misc1 = junk.special;
    actor->player = oldplayer;
}

void A_SkullPop(mobj_t *actor, player_t *player, pspdef_t *psp)
{
    mobj_t      *mo;

    S_StartSound(actor, sfx_pldeth);

    actor->flags &= ~MF_SOLID;
    mo = P_SpawnMobj(actor->x, actor->y, actor->z + 48 * FRACUNIT, MT_GIBDTH);
    mo->momx = (P_Random() - P_Random()) << 9;
    mo->momy = (P_Random() - P_Random()) << 9;
    mo->momz = FRACUNIT * 2 + (P_Random() << 6);

    // Attach player mobj to bloody skull
    player = actor->player;
    actor->player = NULL;
    mo->player = player;
    mo->health = actor->health;
    mo->angle = actor->angle;
    if (player)
    {
        player->mo = mo;
        player->damagecount = 32;
    }
}
/*
//
// A_CounterJump
//
// Parameterized codepointer for branching based on comparisons
// against a thing's counter values.
//
// args[0] : state number
// args[1] : comparison type
// args[2] : immediate value OR counter number
// args[3] : counter # to use
//
void A_CounterJump(mobj_t *mo)
{
   boolean branch = false;
   int statenum   = S_PISCASE_A_FADE1;
   int checktype  = 0;
   short value    = (short)(40);
   int cnum       = 1;
   int *counter;
   
   // validate state number
//   statenum = E_StateNumForDEHNum(statenum);
   if(statenum == NUMSTATES)
      return;

   if(cnum < 0 || cnum >= NUMMOBJCOUNTERS)
      return; // invalid

   counter = &(mo->counters[cnum]);

   // 08/02/04:
   // support getting check value from a counter
   // if checktype is greater than the last immediate operator,
   // then the comparison value is actually a counter number

   if(checktype >= CPC_NUMIMMEDIATE)
   {
      // turn it into the corresponding immediate operation
      checktype -= CPC_NUMIMMEDIATE;

      if(value < 0 || value >= NUMMOBJCOUNTERS)
         return; // invalid counter number

      value = mo->counters[value];
   }

   switch(checktype)
   {
   case CPC_LESS:
      branch = (*counter < value); break;
   case CPC_LESSOREQUAL:
      branch = (*counter <= value); break;
   case CPC_GREATER:
      branch = (*counter > value); break;
   case CPC_GREATEROREQUAL:
      branch = (*counter >= value); break;
   case CPC_EQUAL:
      branch = (*counter == value); break;
   case CPC_NOTEQUAL:
      branch = (*counter != value); break;
   case CPC_BITWISEAND:
      branch = (*counter & value); break;
   default:
      break;
   }

   if(branch)
      P_SetMobjState(mo, statenum);
}

//
// A_FadeOut
//
// ZDoom-inspired action function, implemented using wiki docs.
//
// args[0] : alpha step
//
void A_FadeOut(mobj_t *mo)
{
//   mo->translucency -= 0.025;
   
//   if(mo->translucency < 0)
//      mo->translucency = 0;
//   else if(mo->translucency > FRACUNIT)
//      mo->translucency = FRACUNIT;
}

//
// A_CounterSwitch
//
// This powerful codepointer can branch to one of N states
// depending on the value of the indicated counter, and it
// remains totally safe at all times. If the entire indicated
// frame set is not valid, no actions will be taken.
//
// args[0] : counter # to use
// args[1] : DeHackEd number of first frame in consecutive set
// args[2] : number of frames in consecutive set
//
void A_CounterSwitch(mobj_t *mo)
{
   int cnum = 1;
   int startstate = S_PISCASE_A;
   int numstates  = 4 - 1;
   int *counter;

   // get counter
   if(cnum < 0 || cnum >= NUMMOBJCOUNTERS)
      return; // invalid

   counter = &(mo->counters[cnum]);

   // verify startstate
//   startstate = E_StateNumForDEHNum(startstate);
   if(startstate == NUMSTATES)
      return;

   // verify last state is < NUMSTATES
   if(startstate + numstates >= NUMSTATES)
      return;

   // verify counter is in range
   if(*counter < 0 || *counter > numstates)
      return;

   // jump!
   P_SetMobjState(mo, startstate + *counter);
}

//
// A_SetCounter
//
// Sets the value of the indicated counter variable for the thing.
// Can perform numerous operations -- this is more like a virtual
// machine than a codepointer ;)
//
// args[0] : counter # to set
// args[1] : value to utilize
// args[2] : operation to perform
//
void A_SetCounter(mobj_t *mo)
{
   int cnum = 1;
   short value = (short)(4);
   int specialop = 11;
   int *counter;

   if(cnum < 0 || cnum >= NUMMOBJCOUNTERS)
      return; // invalid

   counter = &(mo->counters[cnum]);

   switch(specialop)
   {
   case CPOP_ASSIGN:
      *counter = value; break;
   case CPOP_ADD:
      *counter += value; break;
   case CPOP_SUB:
      *counter -= value; break;
   case CPOP_MUL:
      *counter *= value; break;
   case CPOP_DIV:
      if(value) // don't divide by zero
         *counter /= value;
      break;
   case CPOP_MOD:
      if(value > 0) // only allow modulus by positive values
         *counter %= value;
      break;
   case CPOP_AND:
      *counter &= value; break;
   case CPOP_ANDNOT:
      *counter &= ~value; break; // compound and-not operation
   case CPOP_OR:
      *counter |= value; break;
   case CPOP_XOR:
      *counter ^= value; break;
   case CPOP_RND:
      *counter = P_Random(); break;
   case CPOP_RNDMOD:
      if(value > 0)
         *counter = P_Random() % value; break;
   case CPOP_SHIFTLEFT:
      *counter <<= value; break;
   case CPOP_SHIFTRIGHT:
      *counter >>= value; break;
   default:
      break;
   }
}

//
// A_EjectCasing
//
// A pointer meant for spawning bullet casing objects.
// Parameters:
//   args[0] : distance in front in 16th's of a unit
//   args[1] : distance from middle in 16th's of a unit (negative = left)
//   args[2] : z height relative to player's viewpoint in 16th's of a unit
//   args[3] : thingtype to toss
//
void A_EjectCasing(mobj_t *actor)
{
   angle_t     angle = actor->angle;
   fixed_t     x, y, z;
   fixed_t     frontdist;
   int         frontdisti;
   fixed_t     sidedist;
   fixed_t     zheight;
   int         thingtype;
   mobj_t      *mo;

   frontdisti = 512;
   
   frontdist  = frontdisti * FRACUNIT / 16;
   sidedist   = 64 * FRACUNIT / 16;
   zheight    = -112 * FRACUNIT / 16;

   // account for mlook - EXPERIMENTAL
   if(actor->player)
   {
      int pitch = actor->player->lookdir;
            
      z = actor->z + actor->player->viewheight + zheight;
      
      // modify height according to pitch - hack warning.
      z -= (pitch / ANGLE_1) * ((10 * frontdisti / 256) * FRACUNIT / 32);
   }
   else
      z = actor->z + (0) * FRACUNIT / 16;

   x = actor->x + FixedMul(frontdist, finecosine[angle>>ANGLETOFINESHIFT]);
   y = actor->y + FixedMul(frontdist, finesine[angle>>ANGLETOFINESHIFT]);

   // adjust x/y along a vector orthogonal to the source object's angle
   angle = angle - ANG90;

   x += FixedMul(sidedist, finecosine[angle>>ANGLETOFINESHIFT]);
   y += FixedMul(sidedist, finesine[angle>>ANGLETOFINESHIFT]);

   thingtype = MT_BULLET;

   mo = P_SpawnMobj(x, y, z, thingtype);

   mo->angle = sidedist >= 0 ? angle : angle + ANG180;
}

//
// A_CasingThrust
//
// A casing-specific thrust function.
//    args[0] : lateral force in 16ths of a unit
//    args[1] : z force in 16ths of a unit
//
void A_CasingThrust(mobj_t *actor)
{
   fixed_t moml, momz;

   moml = 16 * FRACUNIT / 16;
   momz = 32 * FRACUNIT / 16;
   
   actor->momx = FixedMul(moml, finecosine[actor->angle>>ANGLETOFINESHIFT]);
   actor->momy = FixedMul(moml, finesine[actor->angle>>ANGLETOFINESHIFT]);
   
   // randomize
   actor->momx += P_SubRandom(pr_casing) << 8;
   actor->momy += P_SubRandom(pr_casing) << 8;
   actor->momz = momz + (P_SubRandom(pr_casing) << 8);
}
*/

