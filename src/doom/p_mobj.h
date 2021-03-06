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
//        Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------


#ifndef __P_MOBJ__
#define __P_MOBJ__


// We need the thinker_t stuff.
#include "d_think.h"

// We need the WAD data structure for Map things,
// from the THINGS lump.
#include "doomdata.h"

// States are tied to finite states are
//  tied to animation frames.
// Needs precompiled tables/data structures.
#include "info.h"

// Basics.
#include "../m_fixed.h"
#include "../tables.h"


// killough 11/98:
// For torque simulation:
#define OVERDRIVE               6
#define MAXGEAR                 (OVERDRIVE + 16)

// killough 11/98:
// Whether an object is "sentient" or not. Used for environmental influences.
#define sentient(mobj)          ((mobj)->health > 0 && (mobj)->info->seestate)

#define REDBLOOD                184
#define GREENBLOOD              123
#define BLUEBLOOD               204
#define FUZZYBLOOD              -1
#define CORPSEBLOODSPLATS       256

#define NUMMOBJCOUNTERS         8

// haleyjd: thing can't trigger particle events
#define MIF_NOPTCLEVTS          0x00000200


//
// NOTES: mobj_t
//
// mobj_ts are used to tell the refresh where to draw an image,
// tell the world simulation when objects are contacted,
// and tell the sound driver how to position a sound.
//
// The refresh uses the next and prev links to follow
// lists of things in sectors as they are being drawn.
// The sprite, frame, and angle elements determine which patch_t
// is used to draw the sprite if it is visible.
// The sprite and frame values are allmost allways set
// from state_t structures.
// The statescr.exe utility generates the states.h and states.c
// files that contain the sprite/frame numbers from the
// statescr.txt source file.
// The xyz origin point represents a point at the bottom middle
// of the sprite (between the feet of a biped).
// This is the default origin position for patch_ts grabbed
// with lumpy.exe.
// A walking creature will have its z equal to the floor
// it is standing on.
//
// The sound code uses the x,y, and subsector fields
// to do stereo positioning of any sound effited by the mobj_t.
//
// The play simulation uses the blocklinks, x,y,z, radius, height
// to determine when mobj_ts are touching each other,
// touching lines in the map, or hit by trace lines (gunshots,
// lines of sight, etc).
// The mobj_t->flags element has various bit flags
// used by the simulation.
//
// Every mobj_t is linked into a single sector
// based on its origin coordinates.
// The subsector_t is found with R_PointInSubsector(x,y),
// and the sector_t can be found with subsector->sector.
// The sector links are only used by the rendering code,
// the play simulation does not care about them at all.
//
// Any mobj_t that needs to be acted upon by something else
// in the play world (block movement, be shot, etc) will also
// need to be linked into the blockmap.
// If the thing has the MF_NOBLOCK flag set, it will not use
// the block links. It can still interact with other things,
// but only as the instigator (missiles will run into other
// things, but nothing can run into a missile).
// Each block in the grid is 128*128 units, and knows about
// every line_t that it contains a piece of, and every
// interactable mobj_t that has its origin contained.  
//
// A valid mobj_t is a mobj_t that has the proper subsector_t
// filled in for its xy coordinates and is linked into the
// sector from which the subsector was made, or has the
// MF_NOSECTOR flag set (the subsector_t needs to be valid
// even if MF_NOSECTOR is set), and is linked into a blockmap
// block or has the MF_NOBLOCKMAP flag set.
// Links should only be modified by the P_[Un]SetThingPosition()
// functions.
// Do not change the MF_NO? flags while a thing is valid.
//
// Any questions?
//

//
// Misc. mobj flags
//
typedef enum
{
    // Call P_SpecialThing when touched.
    MF_SPECIAL             = 1,

    // Blocks.
    MF_SOLID               = 2,

    // Can be hit.
    MF_SHOOTABLE           = 4,

    // Don't use the sector links (invisible but touchable).
    MF_NOSECTOR            = 8,

    // Don't use the blocklinks (inert but displayable)
    MF_NOBLOCKMAP          = 16,                    

    // Not to be activated by sound, deaf monster.
    MF_AMBUSH              = 32,

    // Will try to attack right back.
    MF_JUSTHIT             = 64,

    // Will take at least one step before attacking.
    MF_JUSTATTACKED        = 128,

    // On level spawning (initial position),
    //  hang from ceiling instead of stand on floor.
    MF_SPAWNCEILING        = 256,

    // Don't apply gravity (every tic),
    //  that is, object will float, keeping current height
    //  or changing it actively.
    MF_NOGRAVITY           = 512,

    // Movement flags.
    // This allows jumps from high places.
    MF_DROPOFF             = 0x400,

    // For players, will pick up items.
    MF_PICKUP              = 0x800,

    // Player cheat. ???
    MF_NOCLIP              = 0x1000,

    // Player: keep info about sliding along walls.
    MF_SLIDE               = 0x2000,

    // Allow moves to any height, no gravity.
    // For active floaters, e.g. cacodemons, pain elementals.
    MF_FLOAT               = 0x4000,

    // Don't cross lines
    //   ??? or look at heights on teleport.
    MF_TELEPORT            = 0x8000,

    // Don't hit same species, explode on block.
    // Player missiles as well as fireballs of various kinds.
    MF_MISSILE             = 0x10000,        

    // Dropped by a demon, not level spawned.
    // E.g. ammo clips dropped by dying former humans.
    MF_DROPPED             = 0x20000,

    // Use fuzzy draw (shadow demons or spectres),
    //  temporary player invisibility powerup.
    MF_SHADOW              = 0x40000,

    // Flag: don't bleed when shot (use puff),
    //  barrels and shootable furniture shall not bleed.
    MF_NOBLOOD             = 0x80000,

    // Don't stop moving halfway off a step,
    //  that is, have dead bodies slide down all the way.
    MF_CORPSE              = 0x100000,

    // Floating to a height for a move, ???
    //  don't auto float to target's height.
    MF_INFLOAT             = 0x200000,

    // On kill, count this enemy object
    //  towards intermission kill total.
    // Happy gathering.
    MF_COUNTKILL           = 0x400000,
    
    // On picking up, count this item object
    //  towards intermission item total.
    MF_COUNTITEM           = 0x800000,

    // Special handling: skull in flight.
    // Neither a cacodemon nor a missile.
    MF_SKULLFLY            = 0x1000000,

    // Don't spawn this object
    //  in death match mode (e.g. key cards).
    MF_NOTDMATCH           = 0x2000000,

    // Player sprites in multiplayer modes are modified
    //  using an internal color lookup table for re-indexing.
    // If 0x4 0x8 or 0xc,
    //  use a translation table for player colormaps
    MF_TRANSLATION         = 0xc000000,

    // Hmm ???.
    MF_TRANSSHIFT          = 26,

    // killough 11/98: dies when solids touch it
    MF_TOUCHY              = 0x10000000,

    // killough 7/11/98: for beta BFG fireballs
    MF_BOUNCES             = 0x20000000,

    // killough 7/18/98: friendly monsters
    MF_FRIEND              = 0x40000000,

    // [crispy] translucent sprite
    MF_TRANSLUCENT         = 0x80000000

} mobjflag_t;

typedef enum
{
    // Apply additive translucency
    MF2_TRANSLUCENT               = 0x00000001,

    // Apply additive translucency on red only
    MF2_TRANSLUCENT_REDONLY       = 0x00000002,

    // Apply additive translucency on green only
    MF2_TRANSLUCENT_GREENONLY     = 0x00000004,

    // Apply additive translucency on blue only
    MF2_TRANSLUCENT_BLUEONLY      = 0x00000008,

    // Apply 33% alpha translucency
    MF2_TRANSLUCENT_33            = 0x00000010,

    // Apply additive translucency on all red to white
    MF2_TRANSLUCENT_REDWHITEONLY  = 0x00000020,

    // Convert all red to green, then apply 33% alpha translucency
    MF2_TRANSLUCENT_REDTOGREEN_33 = 0x00000040,

    // Convert all red to blue, then apply 33% alpha translucency
    MF2_TRANSLUCENT_REDTOBLUE_33  = 0x00000080,

    // Apply 33% alpha translucency on all blue
    MF2_TRANSLUCENT_BLUE_33       = 0x00000100,

    // Convert all red to green
    MF2_REDTOGREEN                = 0x00000200,

    // Convert all green to red
    MF2_GREENTORED                = 0x00000400,

    // Convert all red to blue
    MF2_REDTOBLUE                 = 0x00000800,

    // Object bobs up and down
    MF2_FLOATBOB                  = 0x00001000,

    // Mirrored horizontally
    MF2_MIRRORED                  = 0x00002000,

    MF2_FALLING                   = 0x00004000,

    // Object is resting on top of another object
    MF2_ONMOBJ                    = 0x00008000,

    // Object is allowed to pass over/under other objects
    MF2_PASSMOBJ                  = 0x00010000,

    // Object is a corpse and being resurrected
    MF2_RESURRECTING              = 0x00020000,

    // Object's feet won't be clipped in liquid
    MF2_NOFOOTCLIP                = 0x00040000,

    // Object won't bob in liquid
    MF2_NOLIQUIDBOB               = 0x00080000,

    // Object's feet are now being clipped
    // (when applied to object's shadow, shadow isn't drawn)
    MF2_FEETARECLIPPED            = 0x00100000,

    // Object has a shadow
    MF2_SHADOW                    = 0x00200000,

    // Object is blood
    MF2_BLOOD                     = 0x00400000,

    // Object's thing triangle is not displayed in automap
    MF2_DONOTMAP                  = 0x00800000,

    // Object has smoke trail
    MF2_SMOKETRAIL                = 0x01000000,

    // Object can be crushed into blood splats by moving sectors
    MF2_CRUSHABLE                 = 0x02000000,

    // alternate gravity setting
    MF2_LOGRAV                    = 0x04000000,

    // fly mode is active
    MF2_FLY                       = 0x08000000,

    // does not teleport
    MF2_NOTELEPORT                = 0x10000000,

    // does not splash
    MF2_NOSPLASH                  = 0x20000000

} mobjflag2_t;

// Map Object definition.
typedef struct mobj_s
{
    // List: thinker links.
    thinker_t              thinker;

    // Info for drawing: position.
    fixed_t                x;
    fixed_t                y;
    fixed_t                z;

    // More list: links in sector (if needed)
    struct mobj_s          *snext;
    struct mobj_s          **sprev;

    // More drawing info: to determine current sprite.
    // orientation
    angle_t                angle;

    // used to find patch_t and flip value
    spritenum_t            sprite;

    // might be ORed with FF_FULLBRIGHT
    int                    frame;

    // Interaction info, by BLOCKMAP.
    // Links in blocks (if needed).
    struct mobj_s          *bnext;
    struct mobj_s          **bprev;
    
    struct subsector_s     *subsector;

    // The closest interval over all contacted Sectors.
    fixed_t                floorz;
    fixed_t                ceilingz;

    // For movement checking.
    fixed_t                radius;
    fixed_t                height;        

    // Momentums, used to update position.
    fixed_t                momx;
    fixed_t                momy;
    fixed_t                momz;

    mobjtype_t             type;

    // &mobjinfo[mobj->type]
    mobjinfo_t             *info;
    
    // state tic counter
    int                    tics;

    state_t                *state;
    int                    flags;
    int                    flags2;
    int                    health;

    // Movement direction, movement generation (zig-zagging).
    // 0-7
    int                    movedir;

    // when 0, select a new dir
    int                    movecount;

    // Thing being chased/attacked (or NULL),
    // also the originator for missiles.
    struct mobj_s          *target;

    // Reaction time: if non 0, don't attack yet.
    // Used by player to freeze a bit after teleporting.
    int                    reactiontime;   

    // If >0, the target will be chased
    // no matter what (even if shot)
    int                    threshold;

    // Additional info record for player avatars only.
    // Only valid if type == MT_PLAYER
    struct player_s        *player;

    // Player number last looked for.
    int                    lastlook;        

    // For nightmare respawn.
    mapthing_t             spawnpoint;        

    // Thing being chased/attacked for tracers.
    struct mobj_s          *tracer;        
    
    // [AM] If true, ok to interpolate this tic.
    dboolean               interp;

    // [AM] Previous position of mobj before think.
    //      Used to interpolate between positions.
    fixed_t                oldx;
    fixed_t                oldy;
    fixed_t                oldz;
    angle_t                oldangle;

    // For bobbing up and down.
    int                    floatbob;

    // new field: last known enemy -- killough 2/15/98
    struct mobj_s          *lastenemy;

    char                   *name;

    struct mobj_s          *shadow;

    void                   (*colfunc)(void);
    void                   (*projectfunc)();

    // killough 11/98: the lowest floor over all contacted Sectors.
    fixed_t                dropoffz;

    // killough 11/98: used in torque simulation
    short                  gear;

    // a linked list of sectors where this object appears
    // phares 3/14/98
    struct msecnode_s      *touching_sectorlist;

    int                    pitch;

    fixed_t                projectilepassheight;

    int                    blood;

    int                    bloodsplats;

    fixed_t                nudge;

    // casing counters
    int                    casing_counters[NUMMOBJCOUNTERS];
    int                    casing_counter;

    // particle effect flag field
    unsigned int           effects;

    int                    effect_flies_start_timer;
    int                    effect_flies_sound_timer;
    int                    particle_sound_timer;

    // enemy dead but shot again with flies surrounding
    dboolean               effect_flies_can_spawn;
    dboolean               effect_flies_spawned;
    dboolean               effect_flies_shot;

    // [RH] Used for falling damage
    fixed_t                oldvelocity[3];

} mobj_t;

#endif

