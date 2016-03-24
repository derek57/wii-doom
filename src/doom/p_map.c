// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard, Andrey Budko
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
//        Movement, collision handling.
//        Shooting and aiming.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "c_io.h"
#include "d_deh.h"
#include "doomdef.h"
#include "doomfeatures.h"

// State.
#include "doomstat.h"

#include "i_system.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_local.h"
#include "p_spec.h"
#include "r_state.h"
#include "s_sound.h"

// Data.
#include "sounds.h"

#include "v_trans.h"
#include "z_zone.h"


// jff 3/21/98 Set if line absorbs use by player
//allow multiple push/switch triggers to be used on one push
#define ML_PASSUSE   512


static mobj_t        *slidemo;
static mobj_t        *tmthing;
static mobj_t        *usething;

// slopes to top and bottom of target
static fixed_t       topslope;
static fixed_t       bottomslope;

static fixed_t       bestslidefrac;
static fixed_t       tmxmove;
static fixed_t       tmymove;
static fixed_t       tmx;
static fixed_t       tmy;
static fixed_t       tmz;

static line_t        *bestslideline;

// Pain Elemental position for Lost Soul checks
// phares
static int           pe_x;
static int           pe_y;

// Lost Soul position for Lost Soul checks
// phares
static int           ls_x;
static int           ls_y;

// killough 8/1/98: whether to allow unsticking
static int           tmunstuck;

static int           spechit_max;

// killough 8/9/98: whether to telefrag at exit
static dboolean      telefrag;

static dboolean      crushchange;
static dboolean      nofit;
static dboolean      isliquidsector;

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
dboolean             floatok;

// killough 11/98: if "felldown" true, object was pushed down ledge
dboolean             felldown;

// Height if not aiming up or down
// ???: use slope for monsters?
fixed_t              shootz;        

fixed_t              attackrange;
fixed_t              aimslope;
fixed_t              tmbbox[4];
fixed_t              tmfloorz;
fixed_t              tmceilingz;
fixed_t              tmdropoffz;
fixed_t              shootdirx;
fixed_t              shootdiry;
fixed_t              shootdirz;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls
line_t               *ceilingline;

// killough 8/11/98: blocking linedef
line_t               *blockline;

// killough 8/1/98: Highest touched floor
line_t               *floorline;

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid

// 1/11/98 killough: removed limit on special lines crossed
line_t               **spechit;

// [BH] angle of blood and puffs for automap
angle_t              shootangle;

// who got hit (or NULL)
mobj_t               *linetarget;

mobj_t               *shootthing;
mobj_t               *onmobj;
mobj_t               *bombsource;
mobj_t               *bombspot;

int                  bombdamage;
int                  la_damage;
int                  numspechit;

dboolean             infight;


extern dboolean      hit_enemy;


void (*P_BloodSplatSpawner)(fixed_t, fixed_t, int, int, mobj_t *);


//
// TELEPORT MOVE
// 

//
// PIT_StompThing
//

dboolean PIT_StompThing(mobj_t *thing)
{
    fixed_t     blockdist;
                
    // phares 9/10/98: moved this self-check to start of routine
    // don't clip against self
    if (thing == tmthing)
        return true;
    
    if (!(thing->flags & MF_SHOOTABLE))
        return true;
                
    blockdist = thing->radius + tmthing->radius;
    
    if (ABS(thing->x - tmx) >= blockdist || ABS(thing->y - tmy) >= blockdist)
        // didn't hit it
        return true;
    
    // monsters don't stomp things except on boss level
    // killough 8/9/98: make consistent across all levels
    if (!telefrag)
        return false;
                
    if (tmthing->flags2 & MF2_PASSMOBJ)
    {
        if (tmz > thing->z + thing->height)
            // overhead
            return true;

        if (tmz + tmthing->height < thing->z)
            // underneath
            return true;
    }

    P_DamageMobj(thing, tmthing, tmthing, 10000);
    return true;
}

//
// killough 8/28/98:
//
// P_GetFriction()
//
// Returns the friction associated with a particular mobj.
int P_GetFriction(const mobj_t *mo, int *frictionfactor)
{
    int                 friction = ORIG_FRICTION;
    int                 movefactor = ORIG_FRICTION_FACTOR;
    const msecnode_t    *m;
    const sector_t      *sec;

    // Assign the friction value to objects on the floor, non-floating,
    // and clipped. Normally the object's friction value is kept at
    // ORIG_FRICTION and this thinker changes it for icy or muddy floors.
    //
    // When the object is straddling sectors with the same
    // floorheight that have different frictions, use the lowest
    // friction value (muddy has precedence over icy).
    if (!(mo->flags & (MF_NOCLIP | MF_NOGRAVITY)))
        for (m = mo->touching_sectorlist; m; m = m->m_tnext)
            if (((sec = m->m_sector)->special & FRICTION_MASK) && (sec->friction < friction
                || friction == ORIG_FRICTION) && (mo->z <= sec->floorheight
                || (sec->heightsec != -1 && mo->z <= sectors[sec->heightsec].floorheight)))
            {
                friction = sec->friction;
                movefactor = sec->movefactor;
            }

    if (frictionfactor)
        *frictionfactor = movefactor;

    return friction;
}

// phares 3/19/98
// P_GetMoveFactor() returns the value by which the x,y
// movements are multiplied to add to player movement.
//
// killough 8/28/98: rewritten
int P_GetMoveFactor(const mobj_t *mo, int *frictionp)
{
    int movefactor;
    int friction = P_GetFriction(mo, &movefactor);

    // If the floor is icy or muddy, it's harder to get moving. This is where
    // the different friction factors are applied to 'trying to move'. In
    // p_mobj.c, the friction factors are applied as you coast and slow down.
    if (friction < ORIG_FRICTION)
    {
        // phares 3/11/98: you start off slowly, then increase as
        // you get better footing
        int     momentum = P_AproxDistance(mo->momx, mo->momy);

        if (momentum > (MORE_FRICTION_MOMENTUM << 2))
            movefactor <<= 3;
        else if (momentum > (MORE_FRICTION_MOMENTUM << 1))
            movefactor <<= 2;
        else if (momentum > MORE_FRICTION_MOMENTUM)
            movefactor <<= 1;
    }

    if (frictionp)
        *frictionp = friction;

    return movefactor;
}

//
// P_TeleportMove
//
dboolean P_TeleportMove(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, dboolean boss)
{
    int         xl;
    int         xh;
    int         yl;
    int         yh;
    int         bx;
    int         by;
    sector_t    *newsec;
    fixed_t     radius = thing->radius;
    
    // killough 8/9/98: make telefragging more consistent, preserve compatibility
    telefrag = (thing->player || (!d_telefrag ? boss : (gamemap == 30)));

    // kill anything occupying the position
    tmthing = thing;
        
    tmx = x;
    tmy = y;
    tmz = z;
        
    tmbbox[BOXTOP] = y + radius;
    tmbbox[BOXBOTTOM] = y - radius;
    tmbbox[BOXRIGHT] = x + radius;
    tmbbox[BOXLEFT] = x - radius;

    newsec = R_PointInSubsector(x, y)->sector;
    ceilingline = NULL;
    
    // The base floor/ceiling is from the subsector
    // that contains the point.
    // Any contacted lines the step closer together
    // will adjust them.
    tmfloorz = tmdropoffz = newsec->floorheight;
    tmceilingz = newsec->ceilingheight;
                        
    ++validcount;
    numspechit = 0;
    
    // stomp on any things contacted
    xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;

    for (bx = xl; bx <= xh; ++bx)
        for (by = yl; by <= yh; ++by)
            if (!P_BlockThingsIterator(bx, by, PIT_StompThing))
                return false;
    
    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition(thing);

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;        

    // killough 11/98
    thing->dropoffz = tmdropoffz;

    thing->x = x;
    thing->y = y;
    
    // [AM] Don't interpolate mobjs that pass through teleporters
    thing->interp = false;
 
    P_SetThingPosition(thing);
        
    // [BH] check if new sector is liquid and clip/unclip feet as necessary
    if (!(thing->flags2 & MF2_NOFOOTCLIP) && isliquid[newsec->floorpic])
        thing->flags2 |= MF2_FEETARECLIPPED;
    else
        thing->flags2 &= ~MF2_FEETARECLIPPED;

    // [BH] update shadow position as well
    if (thing->shadow)
    {
        P_UnsetThingPosition(thing->shadow);

        thing->shadow->x = thing->x;
        thing->shadow->y = thing->y;

        P_SetThingPosition(thing->shadow);
    }

    return true;
}

//
// MOVEMENT ITERATOR FUNCTIONS
//

// PIT_CrossLine
// Checks to see if a PE->LS trajectory line crosses a blocking
// line. Returns false if it does.
//
// tmbbox holds the bounding box of the trajectory. If that box
// does not touch the bounding box of the line in question,
// then the trajectory is not blocked. If the PE is on one side
// of the line and the LS is on the other side, then the
// trajectory is blocked.
//
// Currently this assumes an infinite line, which is not quite
// correct. A more correct solution would be to check for an
// intersection of the trajectory and the line, but that takes
// longer and probably really isn't worth the effort.
//
//
// killough 11/98: reformatted
static dboolean PIT_CrossLine(line_t *ld)
{
    return (!((ld->flags ^ ML_TWOSIDED) & (ML_TWOSIDED | ML_BLOCKING | ML_BLOCKMONSTERS))
        || tmbbox[BOXLEFT]   > ld->bbox[BOXRIGHT]
        || tmbbox[BOXRIGHT]  < ld->bbox[BOXLEFT]
        || tmbbox[BOXTOP]    < ld->bbox[BOXBOTTOM]
        || tmbbox[BOXBOTTOM] > ld->bbox[BOXTOP]
        || P_PointOnLineSide(pe_x, pe_y, ld) == P_PointOnLineSide(ls_x, ls_y, ld));
}

// killough 8/1/98: used to test intersection between thing and line
// assuming NO movement occurs -- used to avoid sticky situations.
static int untouched(line_t *ld)
{
    fixed_t     x, y;
    fixed_t     tmbbox[4];
    fixed_t     tmradius = tmthing->radius;

    return ((tmbbox[BOXRIGHT] = (x = tmthing->x) + tmradius) <= ld->bbox[BOXLEFT]
        || (tmbbox[BOXLEFT] = x - tmradius) >= ld->bbox[BOXRIGHT]
        || (tmbbox[BOXTOP] = (y = tmthing->y) + tmradius) <= ld->bbox[BOXBOTTOM]
        || (tmbbox[BOXBOTTOM] = y - tmradius) >= ld->bbox[BOXTOP]
        || P_BoxOnLineSide(tmbbox, ld) != -1);
}

//
// PIT_CheckLine
// Adjusts tmfloorz and tmceilingz as lines are contacted
//
static dboolean PIT_CheckLine(line_t *ld)
{
    if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
        || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
        || tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
        || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
        // didn't hit it
        return true;

    if (P_BoxOnLineSide(tmbbox, ld) != -1)
        // didn't hit it
        return true;
                
    // A line has been hit
    
    // The moving thing's destination position will cross the given line.
    // If this should not be allowed, return false.
    // If the line is special, keep track of it
    // to process later if the move is proven ok.
    // NOTE: specials are NOT sorted by order,
    // so two special lines that are only 8 pixels apart
    // could be crossed in either order.
    
    // killough 7/24/98: allow player to move out of 1s wall, to prevent sticking

    // one sided line
    if (!ld->backsector)
    {
        blockline = ld;

        return (tmunstuck && !untouched(ld)
            && FixedMul(tmx - tmthing->x, ld->dy) > FixedMul(tmy - tmthing->y, ld->dx));
    }

    if (!(tmthing->flags & (MF_MISSILE | MF_BOUNCES)))
    {
        // explicitly blocking everything
        if (ld->flags & ML_BLOCKING)
            // killough 8/1/98: allow escape
            return (tmunstuck && !untouched(ld));

        // [BH] monster-blockers don't affect corpses
        if (!tmthing->player && !(tmthing->flags & MF_CORPSE) && (ld->flags & ML_BLOCKMONSTERS))
            // block monsters only
            return false;
    }

    // set openrange, opentop, openbottom
    // these define a 'window' from one sector to another across this line
    P_LineOpening(ld);        
        
    // adjust floor / ceiling heights
    if (opentop < tmceilingz)
    {
        tmceilingz = opentop;
        ceilingline = ld;
        blockline = ld;
    }

    if (openbottom > tmfloorz)
    {
        tmfloorz = openbottom;        

        // killough 8/1/98: remember floor linedef
        floorline = ld;

        blockline = ld;
    }

    if (lowfloor < tmdropoffz)
        tmdropoffz = lowfloor;
                
    // if contacted a special line, add it to the list
    if (ld->special)
    {
        // 1/11/98 killough: remove limit on lines hit, by array doubling
        if (numspechit >= spechit_max)
        {
            spechit_max = (spechit_max ? spechit_max * 2 : 8);
            spechit = Z_Realloc(spechit, sizeof(*spechit) * spechit_max);

            if (spechit != 0)
                C_Warning("PIT_CheckLine: Hit MaxSpecHit limit at %d, raised to %u",
                        spechit_max / 2, spechit_max);
        }

        spechit[numspechit++] = ld;
    }

    return true;
}

//
// PIT_CheckThing
//
dboolean PIT_CheckThing(mobj_t *thing)
{
    fixed_t     blockdist;
    int         damage;
    dboolean    unblocking = false;
    int         flags = thing->flags;
    int         tmflags = tmthing->flags;
    fixed_t     dist = P_AproxDistance(thing->x - tmthing->x, thing->y - tmthing->y);

    // [BH] apply small amount of momentum to a corpse when a monster walks over it 
    if (corpses_nudge && (flags & MF_CORPSE) && (tmflags & MF_SHOOTABLE) && !thing->nudge
        && dist < 16 * FRACUNIT && thing->z == tmthing->z)
    {
        thing->nudge = TICRATE;

        if (thing->flags2 & MF2_FEETARECLIPPED)
        {
            thing->momx = M_RandomInt(-1, 1) * FRACUNIT;
            thing->momy = M_RandomInt(-1, 1) * FRACUNIT;
        }
        else
        {
            thing->momx = M_RandomInt(-1, 1) * FRACUNIT / 2;
            thing->momy = M_RandomInt(-1, 1) * FRACUNIT / 2;
        }
    }

    if (!(flags & (MF_SOLID | MF_SPECIAL | MF_SHOOTABLE)))
        return true;
    
    // [BH] specify standard radius of 20 for pickups here as thing->radius
    // has been changed to allow better clipping
    blockdist = ((flags & MF_SPECIAL) ? 20 * FRACUNIT : thing->radius) + tmthing->radius;

    if (ABS(thing->x - tmx) >= blockdist || ABS(thing->y - tmy) >= blockdist)
        // didn't hit it
        return true;

    // don't clip against self
    if (thing == tmthing)
        return true;
    
    // [BH] check if things are stuck and allow move if it makes them further apart
    if (tmx == tmthing->x && tmy == tmthing->y)
        unblocking = true;
    else if (P_AproxDistance(thing->x - tmx, thing->y - tmy) > dist)
        unblocking = (tmthing->z < thing->z + thing->height
            && tmthing->z + tmthing->height > thing->z);

    // check if a mobj passed over/under another object
    if (tmthing->flags2 & MF2_PASSMOBJ)
    {
        if (tmthing->z >= thing->z + thing->height)
            // over thing
            return true;
        else if (tmthing->z + tmthing->height <= thing->z)
            // under thing
            return true;
    }

    // check for skulls slamming into things
    if ((tmflags & MF_SKULLFLY) && (flags & MF_SOLID))
    {
        damage = ((P_Random() % 8) + 1) * tmthing->info->damage;
        
        P_DamageMobj(thing, tmthing, tmthing, damage);
        
        tmthing->flags &= ~MF_SKULLFLY;
        tmthing->momx = tmthing->momy = tmthing->momz = 0;
        
        P_SetMobjState(tmthing, tmthing->info->spawnstate);
        
        // stop moving
        return false;
    }

    // missiles can hit other things
    if ((tmflags & MF_MISSILE) || ((tmflags & MF_BOUNCES) && !(tmflags & MF_SOLID)))
    {
        int clipheight = (thing->projectilepassheight ?
                thing->projectilepassheight : thing->height);

        // see if it went over / under
        if (tmthing->z > thing->z + clipheight)
            // overhead
            return true;

        if (tmthing->z + tmthing->height < thing->z)
            // underneath
            return true;

        if (tmthing->target
            && (tmthing->target->type == thing->type
                || (tmthing->target->type == MT_KNIGHT && thing->type == MT_BRUISER && !beta_style)
                || (tmthing->target->type == MT_KNIGHT && thing->type == MT_BETABRUISER && beta_style)
                || (tmthing->target->type == MT_BRUISER && thing->type == MT_KNIGHT && !beta_style)
                || (tmthing->target->type == MT_BETABRUISER && thing->type == MT_KNIGHT && beta_style)))
        {
            // Don't hit same species as originator.
            if (thing == tmthing->target)
                return true;
            else if (thing->type != MT_PLAYER && !infight && !species_infighting)
                // Explode, but do no damage.
                // Let players missile other players.
                return false;
        }
        
        if (!(flags & MF_SHOOTABLE))
            // didn't do any damage
            return !(flags & MF_SOLID);
        
        // damage / explode
        if (tmthing->type != MT_BULLET && tmthing->type != MT_SHELL && tmthing->type != MT_ROUND)
        {
            damage = ((P_Random() % 8) + 1) * tmthing->info->damage;
            P_DamageMobj(thing, tmthing, tmthing->target, damage);
        }

        if (thing->type != MT_BARREL)
            if (tmthing->type == MT_ROCKET)
                tmthing->nudge++;

        // don't traverse any more
        return false;                                
    }
    
    // check for special pickup
    if (flags & MF_SPECIAL)
    {
        dboolean solid = ((flags & MF_SOLID) != 0);

        if (tmflags & MF_PICKUP)
            // can remove thing
            P_TouchSpecialThing(thing, tmthing);

        return !solid;
    }

    // [BH] don't hit if either thing is a corpse, which may still be solid if
    // they are still going through their death sequence.
    if (!(thing->flags2 & MF2_RESURRECTING) && ((flags & MF_CORPSE) || (tmflags & MF_CORPSE)))
        return true;

    // RjY
    // an attempt to handle blocking hanging bodies
    // A solid hanging body will allow sufficiently small things underneath it.

    // solid and hanging
    if (!((~flags) & (MF_SOLID | MF_SPAWNCEILING))
        // invert everything, then both bits should be clear
        // head height <= base
        && tmthing->z + tmthing->height <= thing->z)
        // top of thing trying to move under the body <= bottom of body
    {
        // pretend ceiling height is at body's base
        tmceilingz = thing->z;

        return true;
    }

    // killough 3/16/98: Allow non-solid moving objects to move through solid
    // ones, by allowing the moving thing (tmthing) to move if it's non-solid,
    // despite another solid thing being in the way.
    // killough 4/11/98: Treat no-clipping things as not blocking
    return (!((flags & MF_SOLID) && !(flags & MF_NOCLIP) && (tmflags & MF_SOLID)) || unblocking);
}

//
// P_CheckLineSide
//
// This routine checks for Lost Souls trying to be spawned
// across 1-sided lines, impassible lines, or "monsters can't
// cross" lines. Draw an imaginary line between the PE
// and the new Lost Soul spawn spot. If that line crosses
// a 'blocking' line, then disallow the spawn. Only search
// lines in the blocks of the blockmap where the bounding box
// of the trajectory line resides. Then check bounding box
// of the trajectory vs. the bounding box of each blocking
// line to see if the trajectory and the blocking line cross.
// Then check the PE and LS to see if they're on different
// sides of the blocking line. If so, return true, otherwise
// false.
//
dboolean P_CheckLineSide(mobj_t *actor, fixed_t x, fixed_t y)
{
    int bx;
    int by;
    int xl;
    int xh;
    int yl;
    int yh;

    pe_x = actor->x;
    pe_y = actor->y;
    ls_x = x;
    ls_y = y;

    // here is the bounding box of the trajectory
    tmbbox[BOXLEFT] = MIN(pe_x, x);
    tmbbox[BOXRIGHT] = MAX(pe_x, x);
    tmbbox[BOXTOP] = MAX(pe_y, y);
    tmbbox[BOXBOTTOM] = MIN(pe_y, y);

    // determine which blocks to look in for blocking lines
    xl = (tmbbox[BOXLEFT] - bmaporgx) >> MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx) >> MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy) >> MAPBLOCKSHIFT;

    // prevents checking same line twice
    validcount++;

    for (bx = xl; bx <= xh; bx++)
        for (by = yl; by <= yh; by++)
            if (!P_BlockLinesIterator(bx, by, PIT_CrossLine))
                return true;

    return false;
}

//
// PIT_CheckOnmobjZ
//
dboolean PIT_CheckOnmobjZ(mobj_t *thing)
{
    fixed_t     blockdist;

    if (!(thing->flags & MF_SOLID))
        return true;

    // [RH] Corpses and specials don't block moves
    if (thing->flags & (MF_CORPSE | MF_SPECIAL))
        return true;

    // Don't clip against self
    if (thing == tmthing)
        return true;

    // over / under thing
    if (tmthing->z > thing->z + thing->height)
        return true;
    else if (tmthing->z + tmthing->height <= thing->z)
        return true;

    blockdist = thing->radius + tmthing->radius;

    if (ABS(thing->x - tmx) >= blockdist || ABS(thing->y - tmy) >= blockdist)
        // Didn't hit thing
        return true;

    onmobj = thing;

    return false;
}

//
// MOVEMENT CLIPPING
//

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
// 
// in:
//  a mobj_t (can be valid or invalid)
//  a position to be checked
//   (doesn't need to be related to the mobj_t->x, y)
//
// during:
//  special things are touched if MF_PICKUP
//  early out on solid lines?
//
// out:
//  newsubsec
//  floorz
//  ceilingz
//  tmdropoffz
//   the lowest point contacted
//   (monsters won't move to a dropoff)
//  speciallines[]
//  numspeciallines
//
dboolean P_CheckPosition(mobj_t *thing, fixed_t x, fixed_t y)
{
    int         xl;
    int         xh;
    int         yl;
    int         yh;
    int         bx;
    int         by;
    subsector_t *newsubsec;
    fixed_t     radius = ((thing->flags & MF_SPECIAL) ? MIN(20 * FRACUNIT, thing->radius) :
                    thing->radius);

    tmthing = thing;
        
    tmx = x;
    tmy = y;
        
    tmbbox[BOXTOP] = y + radius;
    tmbbox[BOXBOTTOM] = y - radius;
    tmbbox[BOXRIGHT] = x + radius;
    tmbbox[BOXLEFT] = x - radius;

    newsubsec = R_PointInSubsector(x, y);

    // killough 8/1/98
    floorline = blockline = ceilingline = NULL;
    
    // Whether object can get out of a sticky situation:

    // only players
    tmunstuck = (thing->player &&
                // not voodoo dolls
                thing->player->mo == thing);

    // The base floor / ceiling is from the subsector
    // that contains the point.
    // Any contacted lines the step closer together
    // will adjust them.
    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    ++validcount;
    numspechit = 0;

    if (tmthing->flags & MF_NOCLIP)
        return true;
    
    // Check things first, possibly picking things up.
    // The bounding box is extended by MAXRADIUS
    // because mobj_ts are grouped into mapblocks
    // based on their origin point, and can overlap
    // into adjacent blocks by up to MAXRADIUS units.
    xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;

    for (bx = xl; bx <= xh; ++bx)
        for (by = yl; by <= yh; ++by)
            if (!P_BlockThingsIterator(bx, by, PIT_CheckThing))
                return false;
    
    // check lines
    xl = (tmbbox[BOXLEFT] - bmaporgx) >> MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx) >> MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy) >> MAPBLOCKSHIFT;

    for (bx = xl; bx <= xh; ++bx)
        for (by = yl; by <= yh; ++by)
            if (!P_BlockLinesIterator(bx, by, PIT_CheckLine))
                return false;

    return true;
}

//
// P_FakeZMovement
//
void P_FakeZMovement(mobj_t *mo)
{
    // adjust height
    mo->z += mo->momz;

    if ((mo->flags & MF_FLOAT) && mo->target)
    {
        // float down towards target if too close
        if (!(mo->flags & MF_SKULLFLY) && !(mo->flags & MF_INFLOAT))
        {
            fixed_t     delta = (mo->target->z + (mo->height >> 1) - mo->z) * 3;

            if (P_AproxDistance(mo->x - mo->target->x, mo->y - mo->target->y) < ABS(delta))
                mo->z += (delta < 0 ? -FLOATSPEED : FLOATSPEED);
        }
    }

    // clip movement
    if (mo->z <= mo->floorz)
    {
        // hit the floor
        if (mo->flags & MF_SKULLFLY)
            // the skull slammed into something
            mo->momz = -mo->momz;

        if (mo->momz < 0)
            mo->momz = 0;

        mo->z = mo->floorz;
    }
    else if (!(mo->flags & MF_NOGRAVITY))
    {
        if (!mo->momz)
            mo->momz = -GRAVITY;

        mo->momz -= GRAVITY;
    }

    if (mo->z + mo->height > mo->ceilingz)
    {
        // hit the ceiling
        if (mo->momz > 0)
            mo->momz = 0;

        if (mo->flags & MF_SKULLFLY)
            // the skull slammed into something
            mo->momz = -mo->momz;

        mo->z = mo->ceilingz - mo->height;
    }
}

//
// P_CheckOnmobj
// Checks if the new Z position is legal
//
mobj_t *P_CheckOnmobj(mobj_t *thing)
{
    int         xl;
    int         xh;
    int         yl;
    int         yh;
    int         bx;
    int         by;
    subsector_t *newsubsec;
    fixed_t     x = thing->x;
    fixed_t     y = thing->y;

    // save the old mobj before the fake zmovement
    mobj_t      oldmo = *thing;

    fixed_t     radius;

    tmthing = thing;

    P_FakeZMovement(tmthing);

    tmx = x;
    tmy = y;

    radius = tmthing->radius;
    tmbbox[BOXTOP] = y + radius;
    tmbbox[BOXBOTTOM] = y - radius;
    tmbbox[BOXRIGHT] = x + radius;
    tmbbox[BOXLEFT] = x - radius;

    newsubsec = R_PointInSubsector(x, y);
    ceilingline = NULL;

    // the base floor / ceiling is from the subsector that contains the
    // point. Any contacted lines the step closer together will adjust them
    tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
    tmceilingz = newsubsec->sector->ceilingheight;

    ++validcount;
    numspechit = 0;

    if (tmthing->flags & MF_NOCLIP)
        return NULL;

    // check things first, possibly picking things up
    // the bounding box is extended by MAXRADIUS because mobj_ts are grouped
    // into mapblocks based on their origin point, and can overlap into adjacent
    // blocks by up to MAXRADIUS units
    xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS) >> MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS) >> MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS) >> MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS) >> MAPBLOCKSHIFT;

    for (bx = xl; bx <= xh; ++bx)
        for (by = yl; by <= yh; ++by)
            if (!P_BlockThingsIterator(bx, by, PIT_CheckOnmobjZ))
            {
                *tmthing = oldmo;
                return onmobj;
            }

    *tmthing = oldmo;

    return NULL;
}

//
// P_TryMove
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//
dboolean P_TryMove(mobj_t *thing, fixed_t x, fixed_t y, dboolean dropoff)
{
    fixed_t     oldx;
    fixed_t     oldy;
    sector_t    *newsec;

    // killough 11/98
    felldown = false;

    floatok = false;

    if (!P_CheckPosition(thing, x, y))
        // solid wall or thing
        return false;
    
    if (!(thing->flags & MF_NOCLIP))
    {
        // killough 7/26/98: reformatted slightly
        // killough 8/1/98: Possibly allow escape if otherwise stuck

        // doesn't fit
        if (tmceilingz - tmfloorz < thing->height
            // mobj must lower to fit
            || (floatok = true, !(thing->flags & MF_TELEPORT)
                && tmceilingz - thing->z < thing->height
                && !(thing->flags2 & MF2_FLY))
            // too big a step up
            || (!(thing->flags & MF_TELEPORT)
                && tmfloorz - thing->z > 24 * FRACUNIT)
            || ((thing->flags2 & MF2_FLY)
                && ((thing->z + thing->height > tmceilingz)
                    || (thing->z < tmfloorz
                        && tmfloorz - tmdropoffz > 24 * FRACUNIT))))
        {
            return (tmunstuck
                    && !(ceilingline && untouched(ceilingline))
                    && !(floorline && untouched(floorline)));
        }

        // killough 3/15/98: Allow certain objects to drop off
        // killough 7/24/98, 8/1/98: 
        // Prevent monsters from getting stuck hanging off ledges
        // killough 10/98: Allow dropoffs in controlled circumstances
        // killough 11/98: Improve symmetry of clipping on stairs
        if (!(thing->flags & (MF_DROPOFF | MF_FLOAT)))
        {
            if (!dropoff)
            {
                if (thing->floorz - tmfloorz > 24 * FRACUNIT
                    || thing->dropoffz - tmdropoffz > 24 * FRACUNIT)
                    return false;
            }
            else
            {
                // dropoff allowed -- check for whether it fell more than 24
                felldown = (!(thing->flags & MF_NOGRAVITY) && thing->z - tmfloorz > 24 * FRACUNIT);
            }
        }

        // killough 8/13/98
        if ((thing->flags & MF_BOUNCES) &&
            !(thing->flags & (MF_MISSILE | MF_NOGRAVITY)) &&
            !sentient(thing) && tmfloorz - thing->z > 16 * FRACUNIT)
            // too big a step up for bouncers under gravity
            return false;

        // killough 11/98: prevent falling objects from going up too many steps
        if ((thing->flags2 & MF2_FALLING)
            && tmfloorz - thing->z > FixedMul(thing->momx, thing->momx) +
                                     FixedMul(thing->momy, thing->momy))
        {
            return false;
        }
    }
    
    // the move is ok,
    // so link the thing into its new position
    P_UnsetThingPosition(thing);

    oldx = thing->x;
    oldy = thing->y;
    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;

    // killough 11/98: keep track of dropoffs
    thing->dropoffz = tmdropoffz;

    thing->x = x;
    thing->y = y;

    P_SetThingPosition(thing);

    newsec = thing->subsector->sector;

    // [BH] check if new sector is liquid and clip/unclip feet as necessary 
    if (!(thing->flags2 & MF2_NOFOOTCLIP) && isliquid[newsec->floorpic])
        thing->flags2 |= MF2_FEETARECLIPPED;
    else
        thing->flags2 &= ~MF2_FEETARECLIPPED;

    // if any special lines were hit, do the effect
    if (!(thing->flags & (MF_TELEPORT | MF_NOCLIP)))
    {
        while (numspechit--)
        {
            // see if the line was crossed
            line_t      *ld = spechit[numspechit];
            int         oldside = P_PointOnLineSide(oldx, oldy, ld);

            if (oldside != P_PointOnLineSide(thing->x, thing->y, ld) && ld->special)
                P_CrossSpecialLine(ld, oldside, thing);
        }
    }

    // [BH] update shadow position as well 
    if (thing->shadow)
    {
        P_UnsetThingPosition(thing->shadow);

        thing->shadow->x = thing->x;
        thing->shadow->y = thing->y;

        P_SetThingPosition(thing->shadow);
    }

    return true;
}

//
// killough 9/12/98:
//
// Apply "torque" to objects hanging off of ledges, so that they
// fall off. It's not really torque, since Doom has no concept of
// rotation, but it's a convincing effect which avoids anomalies
// such as lifeless objects hanging more than halfway off of ledges,
// and allows objects to roll off of the edges of moving lifts, or
// to slide up and then back down stairs, or to fall into a ditch.
// If more than one linedef is contacted, the effects are cumulative,
// so balancing is possible.
//
static dboolean PIT_ApplyTorque(line_t *ld)
{
    // If thing touches two-sided pivot linedef
    if (ld->backsector
        && tmbbox[BOXRIGHT] > ld->bbox[BOXLEFT]
        && tmbbox[BOXLEFT] < ld->bbox[BOXRIGHT]
        && tmbbox[BOXTOP] > ld->bbox[BOXBOTTOM]
        && tmbbox[BOXBOTTOM] < ld->bbox[BOXTOP]
        && P_BoxOnLineSide(tmbbox, ld) == -1)
    {
        mobj_t  *mo = tmthing;

        // lever arm
        fixed_t dist =
              (ld->dx >> FRACBITS) * (mo->y >> FRACBITS)
            - (ld->dy >> FRACBITS) * (mo->x >> FRACBITS)
            - (ld->dx >> FRACBITS) * (ld->v1->y >> FRACBITS)
            + (ld->dy >> FRACBITS) * (ld->v1->x >> FRACBITS);

        // dropoff direction
        if (dist < 0 ?
            ld->frontsector->floorheight < mo->z && ld->backsector->floorheight >= mo->z :
            ld->backsector->floorheight < mo->z && ld->frontsector->floorheight >= mo->z)
        {
            // At this point, we know that the object straddles a two-sided
            // linedef, and that the object's center of mass is above-ground.
            fixed_t     x = ABS(ld->dx);
            fixed_t     y = ABS(ld->dy);

            if (y > x)
            {
                fixed_t t = x;

                x = y;
                y = t;
            }

            y = finesine[(tantoangle[FixedDiv(y, x) >> DBITS] + ANG90) >> ANGLETOFINESHIFT];

            // Momentum is proportional to distance between the
            // object's center of mass and the pivot linedef.
            //
            // It is scaled by 2^(OVERDRIVE - gear). When gear is
            // increased, the momentum gradually decreases to 0 for
            // the same amount of pseudotorque, so that oscillations
            // are prevented, yet it has a chance to reach equilibrium.
            dist = FixedDiv(FixedMul(dist, (mo->gear < OVERDRIVE ?
                y << -(mo->gear - OVERDRIVE) :
                y >> +(mo->gear - OVERDRIVE))), x);

            // Apply momentum away from the pivot linedef.
            x = FixedMul(ld->dy, dist);
            y = FixedMul(ld->dx, dist);

            // Avoid moving too fast all of a sudden (step into "overdrive")
            dist = FixedMul(x, x) + FixedMul(y, y);

            while (dist > FRACUNIT * 4 && mo->gear < MAXGEAR)
            {
                ++mo->gear;
                x >>= 1;
                y >>= 1;
                dist >>= 1;
            }

            mo->momx -= x;
            mo->momy += y;
        }
    }

    return true;
}

//
// killough 9/12/98
//
// Applies "torque" to objects, based on all contacted linedefs
//
void P_ApplyTorque(mobj_t *mo)
{
    int x = mo->x;
    int y = mo->y;
    int radius = mo->radius;
    int xl = ((tmbbox[BOXLEFT] = x - radius) - bmaporgx) >> MAPBLOCKSHIFT;
    int xh = ((tmbbox[BOXRIGHT] = x + radius) - bmaporgx) >> MAPBLOCKSHIFT;
    int yl = ((tmbbox[BOXBOTTOM] = y - radius) - bmaporgy) >> MAPBLOCKSHIFT;
    int yh = ((tmbbox[BOXTOP] = y + radius) - bmaporgy) >> MAPBLOCKSHIFT;
    int bx;
    int by;

    // Remember the current state, for gear-change
    int flags2 = mo->flags2;

    tmthing = mo;

    // prevents checking same line twice
    validcount++;

    for (bx = xl; bx <= xh; bx++)
        for (by = yl; by <= yh; by++)
            P_BlockLinesIterator(bx, by, PIT_ApplyTorque);

    // If any momentum, mark object as 'falling' using engine-internal flags
    if (mo->momx | mo->momy)
        mo->flags2 |= MF2_FALLING;
    // Clear the engine-internal flag indicating falling object.
    else
        mo->flags2 &= ~MF2_FALLING;

    // If the object has been moving, step up the gear.
    // This helps reach equilibrium and avoid oscillations.
    //
    // DOOM has no concept of potential energy, much less
    // of rotation, so we have to creatively simulate these
    // systems somehow :)

    // If not falling for a while,
    if (!((mo->flags2 | flags2) & MF2_FALLING))
        // Reset it to full strength
        mo->gear = 0;
    // Else if not at max gear,
    else if (mo->gear < MAXGEAR)
        // move up a gear
        mo->gear++;
}

//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//
dboolean P_ThingHeightClip(mobj_t *thing)
{
    dboolean    onfloor = (thing->z == thing->floorz);

    // haleyjd
    fixed_t     oldfloorz = thing->floorz;

    int         flags2 = thing->flags2;
        
    P_CheckPosition(thing, thing->x, thing->y);        

    // what about stranding a monster partially off an edge?
    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;

    // killough 11/98: remember dropoffs
    thing->dropoffz = tmdropoffz;

    if ((flags2 & MF2_FEETARECLIPPED) && d_swirl && !thing->player)
        thing->z = thing->floorz;
    else if (flags2 & MF2_FLOATBOB && float_items)
    {
        if (thing->floorz > oldfloorz || !(thing->flags & MF_NOGRAVITY))
            thing->z = thing->z - oldfloorz + thing->floorz;

        if (thing->z + thing->height > thing->ceilingz)
            thing->z = thing->ceilingz - thing->height;
    }
    else if (onfloor)
    {
        // walking monsters rise and fall with the floor
        thing->z = thing->floorz;

        // killough 11/98: Possibly upset balance of objects hanging off ledges
        if ((flags2 & MF2_FALLING) && thing->gear >= MAXGEAR)
            thing->gear = 0;
    }
    else
    {
        // don't adjust a floating monster unless forced to
        if (thing->z + thing->height > thing->ceilingz)
            thing->z = thing->ceilingz - thing->height;
    }

    return (thing->ceilingz - thing->floorz >= thing->height);
}

//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//

//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
//
void P_HitSlideLine(line_t *ld)
{
    int         side;
    angle_t     lineangle;
    angle_t     moveangle;
    angle_t     deltaangle;
    fixed_t     movelen;
    fixed_t     newlen;

    // is floor icy?
    dboolean    icyfloor;

    // phares:
    // Under icy conditions, if the angle of approach to the wall
    // is more than 45 degrees, then you'll bounce and lose half
    // your momentum. If less than 45 degrees, you'll slide along
    // the wall. 45 is arbitrary and is believable.
    //
    // Check for the special cases of horz or vert walls.

    // killough 10/98: only bounce if hit hard (prevents wobbling)
    icyfloor = (P_AproxDistance(tmxmove, tmymove) > 4 * FRACUNIT && slidemo->z <= slidemo->floorz
        && P_GetFriction(slidemo, NULL) > ORIG_FRICTION);        
        
    if (ld->slopetype == ST_HORIZONTAL)
    {
        if (icyfloor && ABS(tmymove) > ABS(tmxmove))
        {
            // oooff!
            S_StartSound(slidemo, sfx_oof);

            // absorb half the momentum
            tmxmove /= 2;
            tmymove = -tmymove / 2;
        }
        else
            // no more movement in the Y direction
            tmymove = 0;

        return;
    }
    
    if (ld->slopetype == ST_VERTICAL)
    {
        if (icyfloor && ABS(tmxmove) > ABS(tmymove))
        {
            // oooff!
            S_StartSound(slidemo, sfx_oof);

            // absorb half the momentum
            tmxmove = -tmxmove / 2;
            tmymove /= 2;
        }
        else
            // no more movement in the X direction
            tmxmove = 0;

        return;
    }
        
    side = P_PointOnLineSide(slidemo->x, slidemo->y, ld);
        
    lineangle = R_PointToAngle2(0, 0, ld->dx, ld->dy);

    if (side == 1)
        lineangle += ANG180;

    moveangle = R_PointToAngle2(0, 0, tmxmove, tmymove);

    // prevents sudden path reversal due to rounding error
    moveangle += 10;

    deltaangle = moveangle - lineangle;

    if (deltaangle > ANG180)
        deltaangle += ANG180;

    lineangle >>= ANGLETOFINESHIFT;
    deltaangle >>= ANGLETOFINESHIFT;
        
    movelen = P_AproxDistance(tmxmove, tmymove);
    newlen = FixedMul(movelen, finecosine[deltaangle]);

    tmxmove = FixedMul(newlen, finecosine[lineangle]);
    tmymove = FixedMul(newlen, finesine[lineangle]);
}

//
// PTR_SlideTraverse
//
dboolean PTR_SlideTraverse(intercept_t *in)
{
    line_t      *li = in->d.line;

    if (!(li->flags & ML_TWOSIDED))
    {
        if (P_PointOnLineSide(slidemo->x, slidemo->y, li))
            // don't hit the back side
            return true;

        goto isblocking;
    }

    // set openrange, opentop, openbottom
    P_LineOpening(li);

    if (openrange < slidemo->height)
        // doesn't fit
        goto isblocking;

    if (opentop - slidemo->z < slidemo->height)
        // mobj is too high
        goto isblocking;

    if (openbottom - slidemo->z > 24 * FRACUNIT)
        // too big a step up
        goto isblocking;

    // this line doesn't block movement
    return true;

    // the line does block movement,
    // see if it is closer than best so far

isblocking:

    if (in->frac < bestslidefrac)
    {
        bestslidefrac = in->frac;
        bestslideline = li;
    }

    // stop
    return false;
}

//
// P_SlideMove
// The momx/momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgey mess.
//
// killough 11/98: reformatted
void P_SlideMove(mobj_t *mo)
{
    int         hitcount = 3;
    fixed_t     radius = mo->radius;

    // the object that's sliding
    slidemo = mo;

    do
    {
        fixed_t leadx, leady;
        fixed_t trailx, traily;
        int     x, y;

        if (!--hitcount)
            // don't loop forever
            goto stairstep;

        // trace along the three leading corners
        x = mo->x;
        y = mo->y;

        if (mo->momx > 0)
        {
            leadx = x + radius;
            trailx = x - radius;
        }
        else
        {
            leadx = x - radius;
            trailx = x + radius;
        }

        if (mo->momy > 0)
        {
            leady = y + radius;
            traily = y - radius;
        }
        else
        {
            leady = y - radius;
            traily = y + radius;
        }

        bestslidefrac = FRACUNIT + 1;

        P_PathTraverse(leadx, leady, leadx + mo->momx, leady + mo->momy,
            PT_ADDLINES, PTR_SlideTraverse);
        P_PathTraverse(trailx, leady, trailx + mo->momx, leady + mo->momy,
            PT_ADDLINES, PTR_SlideTraverse);
        P_PathTraverse(leadx, traily, leadx + mo->momx, traily + mo->momy,
            PT_ADDLINES, PTR_SlideTraverse);

        // move up to the wall

        if (bestslidefrac == FRACUNIT + 1)
        {
            // the move must have hit the middle, so stairstep

        stairstep:

            // killough 3/15/98: Allow objects to drop off ledges
            // phares 5/4/98: kill momentum if you can't move at all
            if (!P_TryMove(mo, mo->x, mo->y + mo->momy, true))
                P_TryMove(mo, mo->x + mo->momx, mo->y, true);

            break;
        }

        // fudge a bit to make sure it doesn't hit
        if ((bestslidefrac -= 0x800) > 0)
        {
            fixed_t     newx = FixedMul(mo->momx, bestslidefrac);
            fixed_t     newy = FixedMul(mo->momy, bestslidefrac);

            // killough 3/15/98: Allow objects to drop off ledges
            if (!P_TryMove(mo, mo->x + newx, mo->y + newy, true))
                goto stairstep;
        }

        // Now continue along the wall.
        // First calculate remainder.
        bestslidefrac = FRACUNIT - (bestslidefrac + 0x800);

        if (bestslidefrac > FRACUNIT)
            bestslidefrac = FRACUNIT;

        if (bestslidefrac <= 0)
            break;

        tmxmove = FixedMul(mo->momx, bestslidefrac);
        tmymove = FixedMul(mo->momy, bestslidefrac);

        // clip the moves
        P_HitSlideLine(bestslideline);

        mo->momx = tmxmove;
        mo->momy = tmymove;

        // killough 10/98: affect the bobbing the same way (but not voodoo dolls)
        if (mo->player && mo->player->mo == mo)
        {
            if (ABS(mo->player->momx) > ABS(tmxmove))
                mo->player->momx = tmxmove;

            if (ABS(mo->player->momy) > ABS(tmymove))
                mo->player->momy = tmymove;
        }

      // killough 3/15/98: Allow objects to drop off ledges:
    } while (!P_TryMove(mo, mo->x + tmxmove, mo->y + tmymove, true));
}

//
// PTR_AimTraverse
// Sets linetarget and aimslope when a target is aimed at.
//
dboolean PTR_AimTraverse(intercept_t *in)
{
    mobj_t      *th;
    fixed_t     thingtopslope;
    fixed_t     thingbottomslope;
    fixed_t     dist;
                
    if (in->isaline)
    {
        line_t  *li = in->d.line;
        fixed_t slope;
        
        if (!(li->flags & ML_TWOSIDED))
            // stop
            return false;
        
        // Crosses a two sided line.
        // A two sided line will restrict
        // the possible target ranges.
        P_LineOpening(li);
        
        if (openbottom >= opentop)
            // stop
            return false;
        
        dist = FixedMul(attackrange, in->frac);

        if (li->backsector == NULL || li->frontsector->floorheight != li->backsector->floorheight)
        {
            slope = FixedDiv(openbottom - shootz , dist);

            if (slope > bottomslope)
                bottomslope = slope;
        }
                
        if (li->backsector == NULL || li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
            slope = FixedDiv(opentop - shootz , dist);

            if (slope < topslope)
                topslope = slope;
        }
                
        if (topslope <= bottomslope)
            // stop
            return false;
                        
        // shot continues
        return true;
    }
    
    // shoot a thing
    th = in->d.thing;

    if (th == shootthing)
        // can't shoot self
        return true;
    
    if (!(th->flags & MF_SHOOTABLE))
        // corpse or something
        return true;

    // check angles to see if the thing can be aimed at
    dist = FixedMul(attackrange, in->frac);
    thingtopslope = FixedDiv(th->z + th->height - shootz, dist);

    if (thingtopslope < bottomslope)
        // shot over the thing
        return true;

    thingbottomslope = FixedDiv(th->z - shootz, dist);

    if (thingbottomslope > topslope)
        // shot under the thing
        return true;
    
    // this thing can be hit!
    if (thingtopslope > topslope)
        thingtopslope = topslope;
    
    if (thingbottomslope < bottomslope)
        thingbottomslope = bottomslope;

    aimslope = (thingtopslope + thingbottomslope) / 2;
    linetarget = th;

    // don't go any farther
    return false;
}

//
// PTR_ShootTraverse
//
dboolean PTR_ShootTraverse(intercept_t *in)
{
    fixed_t     x;
    fixed_t     y;
    fixed_t     z;
    fixed_t     frac;
    mobj_t      *th;
    fixed_t     slope;
    fixed_t     dist;
    fixed_t     thingtopslope;
    fixed_t     thingbottomslope;

    // new vars
    dboolean    hitplane = false;
    sector_t    *sidesector = NULL;
    fixed_t     hitx;
    fixed_t     hity;
    fixed_t     hitz;

    // haleyjd 05/02: particle puff z dist correction
    int         updown = 2;

    if (in->isaline)
    {
        line_t  *li = in->d.line;

        if (li->special)
            P_ShootSpecialLine(shootthing, li);

        dist = FixedMul(attackrange, in->frac);

        if (!(li->flags & ML_TWOSIDED))
            goto hitline;

        // crosses a two sided line
        P_LineOpening(li);

        // Check if backsector is NULL.  See comment in PTR_AimTraverse.
        if (li->backsector == NULL)
        {
            goto hitline;
        }

        if (li->frontsector->floorheight != li->backsector->floorheight)
        {
            slope = FixedDiv(openbottom - shootz, dist);

            if (slope > aimslope)
                goto hitline;
        }

        if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
        {
            slope = FixedDiv(opentop - shootz, dist);

            if (slope < aimslope)
                goto hitline;
        }

        // shot continues
        return true;

        // hit line

hitline:

        sidesector = (P_PointOnLineSide(shootthing->x, shootthing->y, li)) ?
                li->backsector : li->frontsector;

        hitz = shootz + FixedMul(aimslope, dist);

        if (sidesector != NULL)
        {
            if (!(hitz > sidesector->floorheight && hitz < sidesector->ceilingheight))
            {
                // ceiling / floor has been contacted
                hitplane = true;
            }
        }

        //
        // hit ceiling / floor
        // set position based on intersection
        //
        if (hitplane == true && sidesector)
        {
            fixed_t den;
            fixed_t num;

            // determine where we've hit
            if (hitz <= sidesector->floorheight)
            {
                den = shootdirz;

                if (den == 0)
                {
                    den = FRACUNIT;
                }

                num = shootz - sidesector->floorheight;

                // haleyjd
                updown = 0;
            }
            else
            {
                den = -shootdirz;

                if (den == 0)
                {
                    den = -FRACUNIT;
                }

                num = -shootz + sidesector->ceilingheight;

                // haleyjd
                updown = 1;
            }

            // position on plane
            frac = FixedDiv(FixedDiv(-num, den), attackrange);

            hitx = shootdirx;
            hity = shootdiry;
        }
        else
        {
            // position a bit closer
            frac = in->frac - FixedDiv (4 * FRACUNIT, attackrange);

            hitx = trace.dx;
            hity = trace.dy;
        }

        x = trace.x + FixedMul(hitx, frac);
        y = trace.y + FixedMul(hity, frac);
        z = shootz + FixedMul(aimslope, FixedMul(frac, attackrange));

        if (li->frontsector->ceilingpic == skyflatnum)
        {
            // don't shoot the sky!
            if (z > li->frontsector->ceilingheight)
                return false;

            // it's a sky hack wall
            // added ceiling height check fix
            if (li->backsector && li->backsector->ceilingpic == skyflatnum
                && li->backsector->ceilingheight < z)
            {
                return false;
            }
        }

        if (la_damage > 0)
        {
            // Spawn particles.
            if (bulletpuff_particle == 1 || bulletpuff_particle == 2)
            {
                P_SpawnParticle(NULL, x, y, z, shootangle, updown, false);

                // haleyjd: for demo sync etc we still need to do the above, so
                // here we'll make the puff invisible and draw particles instead
                if (attackrange != MELEERANGE)
                {
                    if (isliquid[players[consoleplayer].mo->subsector->sector->floorpic])
                        P_SmokePuff(32, x, y, z - FOOTCLIPSIZE, shootangle, updown);
                    else
                        P_SmokePuff(32, x, y, z, shootangle, updown);
                }
            }

            if (bulletpuff_particle == 0 || bulletpuff_particle == 2)
            {
                mobj_t *puff;

                // Test against attack range
                // Spawn bullet puffs.
                if (attackrange != 2112 * FRACUNIT)
                    puff = P_SpawnPuff(x, y, z, shootangle);
                else
                    puff = P_SpawnMobj(x, y, z, MT_PUFF);

                // clip to floor or ceiling
                if (puff->z > puff->ceilingz)
                {
                    puff->z = puff->ceilingz;
                    puff->oldz = puff->z;
                }

                if (puff->z < puff->floorz)
                {
                    puff->z = puff->floorz;
                    puff->oldz = puff->z;
                }
            }
        }

        // don't go any farther
        return false;   
    }

    // shoot a thing
    th = in->d.thing;

    if (th == shootthing)
    {
        hit_enemy = false;

        // can't shoot self
        return true;
    }

    if (th->effect_flies_spawned == true)
        th->effect_flies_shot = true;

    if (!(th->flags & MF_SHOOTABLE))
    {
        hit_enemy = false;

        // corpse or something
        return true;
    }

    // check angles to see if the thing can be aimed at
    dist = FixedMul(attackrange, in->frac);
    thingtopslope = FixedDiv(th->z + th->height - shootz, dist);

    if (thingtopslope < aimslope)
        // shot over the thing
        return true;

    thingbottomslope = FixedDiv(th->z - shootz, dist);

    if (thingbottomslope > aimslope)
        // shot under the thing
        return true;

    // hit thing
    // position a bit closer
    frac = in->frac - FixedDiv(10 * FRACUNIT, attackrange);

    x = trace.x + FixedMul(trace.dx, frac);
    y = trace.y + FixedMul(trace.dy, frac);
    z = shootz + FixedMul(aimslope, FixedMul(frac, attackrange));

    hit_enemy = true;

    // Spawn bullet puffs or blood spots,
    // depending on target type.
    if (th->flags & MF_NOBLOOD)
    {
        // Spawn particles.
        if (bulletpuff_particle == 1 || bulletpuff_particle == 2)
        {
            P_SpawnParticle(th, x, y, z, shootangle, 2, false);

            // haleyjd: for demo sync etc we still need to do the above, so
            // here we'll make the puff invisible and draw particles instead
            if (attackrange != MELEERANGE)
                P_SmokePuff(32, x, y, z, shootangle, updown);
        }

        if (bulletpuff_particle == 0 || bulletpuff_particle == 2)
            P_SpawnPuff(x, y, z, shootangle);
    }
    else
    {
        mobjtype_t type = th->type;

        if ((type == MT_SKULL || type == MT_BETASKULL) && d_colblood2 && d_chkblood2)
        {
            // Spawn particles.
            if (bulletpuff_particle == 1 || bulletpuff_particle == 2)
            {
                P_SpawnParticle(th, x, y, z - FRACUNIT * 8, shootangle, 2, false);

                // haleyjd: for demo sync etc we still need to do the above, so
                // here we'll make the puff invisible and draw particles instead
                if (attackrange != MELEERANGE)
                    P_SmokePuff(32, x, y, z, shootangle, updown);
            }

            if (bulletpuff_particle == 0 || bulletpuff_particle == 2)
                P_SpawnPuff(x, y, z - FRACUNIT * 8, shootangle);
        }
        else if (r_blood != r_blood_none)
        {
            if (type != MT_PLAYER)
            {
                if (bloodsplat_particle == 1 || bloodsplat_particle == 2)
                {
                    P_SpawnParticle(th, x, y, z, shootangle, 2, true);

                    // for demo sync, etc, we still need to do the above, so
                    // we'll make the sprites above invisible and draw particles
                    // instead
                    P_BloodSpray(th, 32, x, y, z, shootangle);
                }

                if (bloodsplat_particle == 0 || bloodsplat_particle == 2)
                    P_SpawnBlood(x, y, z, shootangle, la_damage, th);
            }
            else
            {
                player_t *player = &players[0];

                if (!player->powers[pw_invulnerability] && !(player->cheats & CF_GODMODE))
                {
                    if (bloodsplat_particle == 1 || bloodsplat_particle == 2)
                    {
                        P_SpawnParticle(th, x, y, z + FRACUNIT * M_RandomInt(4, 16), shootangle, 2, true);

                        // for demo sync, etc, we still need to do the above, so
                        // we'll make the sprites above invisible and draw particles
                        // instead
                        P_BloodSpray(th, 32, x, y, z, shootangle);
                    }

                    if (bloodsplat_particle == 0 || bloodsplat_particle == 2)
                        P_SpawnBlood(x, y, z + FRACUNIT * M_RandomInt(4, 16), shootangle, la_damage, th);
                }
            }
        }
    }

    if (la_damage)
        P_DamageMobj(th, shootthing, shootthing, la_damage);

    // don't go any farther
    return false;
}

//
// P_AimLineAttack
//
fixed_t P_AimLineAttack(mobj_t *t1, angle_t angle, fixed_t distance)
{
    fixed_t        x2, y2;

    t1 = P_SubstNullMobj(t1);
        
    angle >>= ANGLETOFINESHIFT;
    shootthing = t1;
    
    if (t1->player)
    {
        // for player pitch aiming
        angle_t pitch = ((t1->player->lookdir / 256) << 14);
        pitch >>= ANGLETOFINESHIFT;

        x2 = t1->x + FixedMul(FixedMul(finecosine[pitch], finecosine[angle]), distance);
        y2 = t1->y + FixedMul(FixedMul(finecosine[pitch], finesine[angle]), distance);
    }
    else
    {
        x2 = t1->x + (distance >> FRACBITS) * finecosine[angle];
        y2 = t1->y + (distance >> FRACBITS) * finesine[angle];
    }

    shootz = t1->z + (t1->height >> 1) + 8 * FRACUNIT;

    // can't shoot outside view angles
    topslope = (ORIGINALHEIGHT / 2) * FRACUNIT / (ORIGINALWIDTH / 2);
    bottomslope = -(ORIGINALHEIGHT / 2) * FRACUNIT / (ORIGINALWIDTH / 2);
    
    attackrange = distance;
    linetarget = NULL;
        
    P_PathTraverse(t1->x, t1->y, x2, y2, (PT_ADDLINES | PT_ADDTHINGS), PTR_AimTraverse);
                
    if (linetarget)
        return aimslope;
    // checks for player pitch
    else
    {
        if (t1->player)
            return t1->player->lookdir / 200;
    }

    return 0;
} 

//
// P_LineAttack
// If damage == 0, it is just a test trace
// that will leave linetarget set.
//
void P_LineAttack(mobj_t *t1, angle_t angle, fixed_t distance, fixed_t slope, int damage)
{
    // [crispy] smooth laser spot movement with uncapped framerate
    const fixed_t t1x = (damage == INT_MIN) ? viewx : t1->x;
    const fixed_t t1y = (damage == INT_MIN) ? viewy : t1->y;
    fixed_t       x2, y2;
    int           traverseflags;

    shootangle = angle;
    angle >>= ANGLETOFINESHIFT;
    shootthing = t1;
    la_damage = damage;

    shootz = (damage == INT_MIN) ? viewz : t1->z + (t1->height >> 1) + 8 * FRACUNIT; 
/*
    if ((t1->flags2 & MF2_FEETARECLIPPED) && d_footclip)
        shootz -= FOOTCLIPSIZE;
*/
    attackrange = distance;
    aimslope = slope;
                
    if (t1->player)
    {
        // for player pitch aiming
        angle_t pitch = ((t1->player->lookdir / 256) << 14);
        pitch >>= ANGLETOFINESHIFT;

        shootdirx = FixedMul(FixedMul(finecosine[pitch], finecosine[angle]), distance);
        shootdiry = FixedMul(FixedMul(finecosine[pitch], finesine[angle]), distance);

        x2 = t1x + shootdirx;
        y2 = t1y + shootdiry;
    }
    else
    {
        x2 = t1x + (distance >> FRACBITS) * finecosine[angle];
        y2 = t1y + (distance >> FRACBITS) * finesine[angle];

        shootdirx = x2 - t1x;
        shootdiry = y2 - t1y;
    }

    // for plane hit detection
    shootdirz = aimslope;

    // test lines only if damage is <= 0
    if (damage >= 1)
        traverseflags = (PT_ADDLINES | PT_ADDTHINGS);
    else
        traverseflags = PT_ADDLINES;

    P_PathTraverse(t1x, t1y, x2, y2, traverseflags, PTR_ShootTraverse);
}

//
// USE LINES
//

static dboolean PTR_UseTraverse(intercept_t *in)
{
    int         side = 0;
    line_t      *line = in->d.line;

    if (!line->special)
    {
        P_LineOpening(line);

        if (openrange <= 0)
        {
            S_StartSound(usething, sfx_noway);

            // can't use through a wall
            return false;
        }

        // not a special line, but keep checking
        return true;
    }

    if (P_PointOnLineSide(usething->x, usething->y, line) == 1)
        side = 1;

    P_UseSpecialLine(usething, line, side);

    // can't use for more than one special line in a row
    // [BH] unless its the wrong side
    return (side || (line->flags & ML_PASSUSE));
}

// Returns false if a "oof" sound should be made because of a blocking
// linedef. Makes 2s middles which are impassable, as well as 2s uppers
// and lowers which block the player, cause the sound effect when the
// player tries to activate them. Specials are excluded, although it is
// assumed that all special linedefs within reach have been considered
// and rejected already (see P_UseLines).
//
// by Lee Killough
//
dboolean PTR_NoWayTraverse(intercept_t *in)
{
    line_t      *ld = in->d.line;

    return (ld->special || !((ld->flags & ML_BLOCKING) || (P_LineOpening(ld), (openrange <= 0
        || openbottom > usething->z + 24 * FRACUNIT || opentop < usething->z + usething->height))));
}

//
// P_UseLines
// Looks for special lines in front of the player to activate.
//
void P_UseLines(player_t *player) 
{
    int         angle;
    fixed_t     x1, y1;
    fixed_t     x2, y2;
        
    if (automapactive && !followplayer)
        return;

    usething = player->mo;
                
    angle = player->mo->angle >> ANGLETOFINESHIFT;

    x1 = player->mo->x;
    y1 = player->mo->y;
    x2 = x1 + (USERANGE >> FRACBITS) * finecosine[angle];
    y2 = y1 + (USERANGE >> FRACBITS) * finesine[angle];
        
    // This added test makes the "oof" sound work on 2s lines -- killough:
    if (P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse))
        if (!P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_NoWayTraverse))
            S_StartSound(usething, sfx_noway);
}

//
// RADIUS ATTACK
//

//
// PIT_RadiusAttack
// "bombsource" is the creature
// that caused the explosion at "bombspot".
//
dboolean PIT_RadiusAttack(mobj_t *thing)
{
    fixed_t     dist;

    if (!(thing->flags & (MF_SHOOTABLE | MF_BOUNCES))
        // [BH] allow corpses to react to blast damage 
        && !(thing->flags & MF_CORPSE))
        return true;

    // Boss spider and cyborg
    // take no damage from concussion.
    if (thing->type == MT_CYBORG || thing->type == MT_SPIDER)
        return true;

    dist = MAX(ABS(thing->x - bombspot->x), ABS(thing->y - bombspot->y)) - thing->radius; 

    if (thing->type == MT_BOSSBRAIN)
    {
        // [BH] if killing boss in DOOM II MAP30, use old code that
        //  doesn't use z height in blast radius
        dist = MAX(0, dist >> FRACBITS);

        if (dist >= bombdamage)
            // out of range
            return true;
    }
    else
    {
        fixed_t dz = ABS(thing->z + (thing->height >> 1) - bombspot->z);

        dist = MAX(0, MAX(dist, dz) >> FRACBITS);

        if (dist >= bombdamage)
            // out of range
            return true;

        // [BH] check z height for blast damage
        if ((thing->floorz > bombspot->z && bombspot->ceilingz < thing->z)
            || (thing->ceilingz < bombspot->z && bombspot->floorz > thing->z))
            return true;
    }

    if (P_CheckSight(thing, bombspot))
    {
        // must be in direct path
        P_DamageMobj(thing, bombspot, bombsource, bombdamage - dist);

        // [BH] count number of times player's rockets hit a monster
        if (bombspot->type == MT_ROCKET && thing->type != MT_BARREL && !(thing->flags & MF_CORPSE))
            bombspot->nudge++;
    }

    return true;
}

//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void P_RadiusAttack(mobj_t *spot, mobj_t *source, int damage)
{
    int         x, y;
    fixed_t     dist = (damage + MAXRADIUS) << FRACBITS;
    int         yh = (spot->y + dist - bmaporgy) >> MAPBLOCKSHIFT;
    int         yl = (spot->y - dist - bmaporgy) >> MAPBLOCKSHIFT;
    int         xh = (spot->x + dist - bmaporgx) >> MAPBLOCKSHIFT;
    int         xl = (spot->x - dist - bmaporgx) >> MAPBLOCKSHIFT;

    bombspot = spot;
    bombsource = source;
    bombdamage = damage;

    for (y = yl; y <= yh; ++y)
        for (x = xl; x <= xh; ++x)
            P_BlockThingsIterator(x, y, PIT_RadiusAttack);
}

//
// SECTOR HEIGHT CHANGING
// After modifying a sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//

//
// PIT_ChangeSector
//
void PIT_ChangeSector(mobj_t *thing)
{
    int flags = thing->flags;
    int flags2 = thing->flags2;

    if (isliquidsector && !(flags2 & MF2_NOFOOTCLIP))
        thing->flags2 |= MF2_FEETARECLIPPED;
    else
        thing->flags2 &= ~MF2_FEETARECLIPPED;

    if (P_ThingHeightClip(thing))
        // keep checking
        return;

    if (!beta_style)
    {
        // crunch bodies to giblets
        if (thing->health <= 0 && (flags2 & MF2_CRUSHABLE))
        {
            if (thing->type == MT_PLAYER)
            {
                nofit = true;
                return;
            }

            // [crispy] connect giblet object with the crushed monster
            thing->target = thing;

            if (thing->type != MT_BARREL && thing->type != MT_BETABARREL &&
                thing->type != MT_SKULL && thing->type != MT_BETASKULL &&
                thing->type != MT_PAIN)
                P_SetMobjState(thing, S_GIBS);

                thing->flags &= ~MF_SOLID;
                thing->height = 0;
                thing->radius = 0;

            if (!(flags & MF_SHADOW) && !(flags & MF_NOBLOOD) && d_maxgore
                && (bloodsplat_particle == 0 || bloodsplat_particle == 2))
            {
                int radius = ((spritewidth[sprites[thing->sprite].spriteframes[0].lump[0]]
                             >> FRACBITS) >> 1) + 12;
                int i;
                int max = M_RandomInt(50, 100) + radius;
                int x = thing->x;
                int y = thing->y;
                int blood = mobjinfo[thing->blood].blood;
                int floorz = thing->floorz;

                for (i = 0; i < max; i++)
                {
                    int     angle = M_RandomInt(0, FINEANGLES - 1);
                    int     fx = x + FixedMul(M_RandomInt(0, radius) << FRACBITS, finecosine[angle]);
                    int     fy = y + FixedMul(M_RandomInt(0, radius) << FRACBITS, finesine[angle]);

                    P_BloodSplatSpawner(fx, fy, blood, floorz, NULL);
                }
            }

            if (crush_sound)
                S_StartSound(thing, sfx_slop);

            //P_RemoveMobj(thing);

            // keep checking
            return;
        }

        // crunch dropped items
        if (flags & MF_DROPPED)
        {
            P_RemoveMobj(thing);
        
            // keep checking
            return;
        }

        if (!(flags & MF_SHOOTABLE))
            // assume it is bloody gibs or something
            return;
    }

    nofit = true;

    if (crushchange && !(leveltime & 3))
    {
        P_DamageMobj(thing, NULL, NULL, 10);

        // spray blood in a random direction
        if (!beta_style && thing->type != MT_BARREL && thing->type != MT_BETABARREL)
        {        
            mobjtype_t type = MT_BLOOD;

            if (d_colblood && d_chkblood)
            {
                if (thing->type == MT_HEAD || thing->type == MT_BETAHEAD)
                    type = MT_BLUEBLOOD;
                else if (thing->type == MT_BRUISER || thing->type == MT_BETABRUISER ||
                    thing->type == MT_KNIGHT)
                    type = MT_GREENBLOOD;
            }

            if (bloodsplat_particle == 0 || bloodsplat_particle == 2)
            {
                mobj_t *mo = P_SpawnMobj(thing->x,
                                         thing->y,
                                         thing->z + thing->height / 2, type);
        
                // [crispy] connect blood object with the monster that bleeds it
                mo->target = thing;
                mo->momx = (P_Random() - P_Random()) << 12;
                mo->momy = (P_Random() - P_Random()) << 12;
            }

            if (bloodsplat_particle == 1 || bloodsplat_particle == 2)
            {
                angle_t an;
                an = (M_Random() - 128) << 24;

                P_DrawSplash2(32, thing->x, thing->y, thing->z + thing->height/2, an, 2, thing->info->blood | MBC_BLOODMASK); 

                // for demo sync, etc, we still need to do the above, so
                // we'll make the sprites above invisible and draw particles
                // instead
                P_BloodSpray(thing, 32, thing->x, thing->y, thing->z, an);
            }
        }
    }

    // keep checking (crush other things)        
    return;
}

//
// P_ChangeSector
// jff 3/19/98 added to just check monsters on the periphery
// of a moving sector instead of all in bounding box of the
// sector. Both more accurate and faster.
// [BH] renamed from P_CheckSector to P_ChangeSector to replace old one entirely
//
dboolean P_ChangeSector(sector_t *sector, dboolean crunch)
{
    msecnode_t  *n;
    mobj_t      *mobj;
    mobjtype_t  type;

    nofit = false;
    crushchange = crunch;
    isliquidsector = isliquid[sector->floorpic];

    // Mark all things invalid
    for (n = sector->touching_thinglist; n; n = n->m_snext)
        n->visited = false;

    if (isliquidsector)
    {
        do
        {
            // go through list
            for (n = sector->touching_thinglist; n; n = n->m_snext)
            {
                // unprocessed thing found
                if (!n->visited)
                {
                    // mark thing as processed
                    n->visited = true;

                    mobj = n->m_thing;

                    if (mobj)
                    {
                        type = mobj->type;

                        if (type == MT_BLOODSPLAT)
                        {
                            P_UnsetThingPosition(mobj);
                            --r_bloodsplats_total;
                        }
                        else if (type != MT_SHADOW && !(mobj->flags & MF_NOBLOCKMAP))
                            // process it
                            PIT_ChangeSector(mobj);
                    }

                    // exit and start over
                    break;
                }
            }

          // repeat from scratch until all things left are marked valid
        } while (n);
    }
    else
    {
        sector->floor_xoffs = 0;
        sector->floor_yoffs = 0;

        do
        {
            // go through list
            for (n = sector->touching_thinglist; n; n = n->m_snext)
            {
                // unprocessed thing found
                if (!n->visited)
                {
                    // mark thing as processed
                    n->visited = true;

                    mobj = n->m_thing;

                    if (mobj)
                    {
                        type = mobj->type;

                        if (type != MT_BLOODSPLAT && type != MT_SHADOW
                            && !(mobj->flags & MF_NOBLOCKMAP))
                            // process it
                            PIT_ChangeSector(mobj);
                    }

                    // exit and start over
                    break;
                }
            }

          // repeat from scratch until all things left are marked valid
        } while (n);
    }

    return nofit;
}

// phares 3/21/98
//
// Maintain a freelist of msecnode_t's to reduce memory allocs and frees.
void P_FreeSecNodeList(void)
{
    // this is all that's needed to fix the bug
    headsecnode = NULL;
}

// P_GetSecnode() retrieves a node from the freelist. The calling routine
// should make sure it sets all fields properly.
//
// killough 11/98: reformatted
static msecnode_t *P_GetSecnode(void)
{
    msecnode_t  *node;

    return (headsecnode ? node = headsecnode, headsecnode = node->m_snext, node :
        Z_Malloc(sizeof(*node), PU_LEVEL, NULL));
}

// P_PutSecnode() returns a node to the freelist.
static void P_PutSecnode(msecnode_t *node)
{
    node->m_snext = headsecnode;
    headsecnode = node;
}

// phares 3/16/98
//
// P_AddSecnode() searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.
//
// killough 11/98: reformatted
static msecnode_t *P_AddSecnode(sector_t *s, mobj_t *thing, msecnode_t *nextnode)
{
    msecnode_t  *node = nextnode;

    while (node)
    {
        // Already have a node for this sector?
        if (node->m_sector == s)
        {
            // Yes. Setting m_thing says 'keep it'.
            node->m_thing = thing;
            return nextnode;
        }

        node = node->m_tnext;
    }

    // Couldn't find an existing node for this sector. Add one at the head
    // of the list.
    node = P_GetSecnode();

    // killough 4/4/98, 4/7/98: mark new nodes unvisited.
    node->visited = 0;

    // sector
    node->m_sector = s;

    // mobj
    node->m_thing = thing;

    // prev node on Thing thread
    node->m_tprev = NULL;

    // next node on Thing thread
    node->m_tnext = nextnode;

    if (nextnode)
        // set back link on Thing
        nextnode->m_tprev = node;

    // Add new node at head of sector thread starting at s->touching_thinglist

    // prev node on sector thread
    node->m_sprev = NULL;

    // next node on sector thread
    node->m_snext = s->touching_thinglist;

    if (s->touching_thinglist)
        node->m_snext->m_sprev = node;

    s->touching_thinglist = node;

    return node;
}

// P_DelSecnode() deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.
//
// killough 11/98: reformatted
static msecnode_t *P_DelSecnode(msecnode_t *node)
{
    if (node)
    {
        // prev node on thing thread
        msecnode_t      *tp = node->m_tprev;

        // next node on thing thread
        msecnode_t      *tn = node->m_tnext;

        // prev node on sector thread
        msecnode_t      *sp;

        // next node on sector thread
        msecnode_t      *sn;

        // Unlink from the Thing thread. The Thing thread begins at
        // sector_list and not from mobj_t->touching_sectorlist.
        if (tp)
            tp->m_tnext = tn;

        if (tn)
            tn->m_tprev = tp;

        // Unlink from the sector thread. This thread begins at
        // sector_t->touching_thinglist.
        sp = node->m_sprev;
        sn = node->m_snext;

        if (sp)
            sp->m_snext = sn;
        else
            node->m_sector->touching_thinglist = sn;

        if (sn)
            sn->m_sprev = sp;

        // Return this node to the freelist
        P_PutSecnode(node);

        return tn;
    }

    return NULL;
}

// Delete an entire sector list
void P_DelSeclist(msecnode_t *node)
{
    while (node)
        node = P_DelSecnode(node);
}

// phares 3/14/98
//
// PIT_GetSectors
// Locates all the sectors the object is in by looking at the lines that
// cross through it. You have already decided that the object is allowed
// at this location, so don't bother with checking impassable or
// blocking lines.
static dboolean PIT_GetSectors(line_t *ld)
{
    if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
        || tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
        || tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
        || tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
        return true;

    if (P_BoxOnLineSide(tmbbox, ld) != -1)
        return true;

    // This line crosses through the object.

    // Collect the sector(s) from the line and add to the
    // sector_list you're examining. If the Thing ends up being
    // allowed to move to this position, then the sector_list
    // will be attached to the Thing's mobj_t at touching_sectorlist.
    sector_list = P_AddSecnode(ld->frontsector, tmthing, sector_list);

    // Don't assume all lines are 2-sided, since some Things
    // like MT_TFOG are allowed regardless of whether their radius takes
    // them beyond an impassable linedef.

    // killough 3/27/98, 4/4/98:
    // Use sidedefs instead of 2s flag to determine two-sidedness.
    // killough 8/1/98: avoid duplicate if same sector on both sides
    if (ld->backsector && ld->backsector != ld->frontsector)
        sector_list = P_AddSecnode(ld->backsector, tmthing, sector_list);

    return true;
}

// phares 3/14/98
//
// P_CreateSecNodeList alters/creates the sector_list that shows what sectors
// the object resides in.
//
// killough 11/98: reformatted
void P_CreateSecNodeList(mobj_t *thing, fixed_t x, fixed_t y)
{
    int         xl;
    int         xh;
    int         yl;
    int         yh;
    int         bx;
    int         by;
    msecnode_t  *node = sector_list;
    mobj_t      *saved_tmthing = tmthing;
    fixed_t     saved_tmx = tmx;
    fixed_t     saved_tmy = tmy;
    fixed_t     radius = thing->radius;

    // First, clear out the existing m_thing fields. As each node is
    // added or verified as needed, m_thing will be set properly. When
    // finished, delete all nodes where m_thing is still NULL. These
    // represent the sectors the Thing has vacated.
    while (node)
    {
        node->m_thing = NULL;
        node = node->m_tnext;
    }

    tmthing = thing;

    tmx = x;
    tmy = y;

    tmbbox[BOXTOP] = y + radius;
    tmbbox[BOXBOTTOM] = y - radius;
    tmbbox[BOXRIGHT] = x + radius;
    tmbbox[BOXLEFT] = x - radius;

    // used to make sure we only process a line once
    ++validcount;

    xl = (tmbbox[BOXLEFT] - bmaporgx) >> MAPBLOCKSHIFT;
    xh = (tmbbox[BOXRIGHT] - bmaporgx) >> MAPBLOCKSHIFT;
    yl = (tmbbox[BOXBOTTOM] - bmaporgy) >> MAPBLOCKSHIFT;
    yh = (tmbbox[BOXTOP] - bmaporgy) >> MAPBLOCKSHIFT;

    for (bx = xl; bx <= xh; ++bx)
        for (by = yl; by <= yh; ++by)
            P_BlockLinesIterator(bx, by, PIT_GetSectors);

    // Add the sector of the (x, y) point to sector_list.
    sector_list = P_AddSecnode(thing->subsector->sector, thing, sector_list);

    // Now delete any nodes that won't be used. These are the ones where
    // m_thing is still NULL.
    node = sector_list;

    while (node)
    {
        if (!node->m_thing)
        {
            if (node == sector_list)
                sector_list = node->m_tnext;

            node = P_DelSecnode(node);
        }
        else
            node = node->m_tnext;
    }

    // cph -
    // This is the strife we get into for using global variables. tmthing
    //  is being used by several different functions calling
    //  P_BlockThingIterator, including functions that can be called *from*
    //  P_BlockThingIterator. Using a global tmthing is not reentrant.
    //  Fun. We restore its previous value.
    tmthing = saved_tmthing;
    tmx = saved_tmx;
    tmy = saved_tmy;

    if (tmthing)
    {
        tmbbox[BOXTOP] = tmy + tmthing->radius;
        tmbbox[BOXBOTTOM] = tmy - tmthing->radius;
        tmbbox[BOXRIGHT] = tmx + tmthing->radius;
        tmbbox[BOXLEFT] = tmx - tmthing->radius;
    }
}

void P_MapEnd(void)
{
    tmthing = NULL;
}

