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
//        Play functions, animation, global header.
//
//-----------------------------------------------------------------------------


#ifndef __P_LOCAL__
#define __P_LOCAL__

#ifndef __R_LOCAL__
#include "r_local.h"
#endif


#define ANGLE_1             (ANG45/45)

#define FOOTCLIPSIZE        (10 * FRACUNIT)
#define FLOATSPEED          (FRACUNIT*4)
#define MAXHEALTH           100
#define VIEWHEIGHT          (41*FRACUNIT)

// mapblocks are used to check movement
// against lines and things
#define MAPBLOCKUNITS       128
#define MAPBLOCKSIZE        (MAPBLOCKUNITS*FRACUNIT)
#define MAPBLOCKSHIFT       (FRACBITS+7)
#define MAPBMASK            (MAPBLOCKSIZE-1)
#define MAPBTOFRAC          (MAPBLOCKSHIFT-FRACBITS)

// player radius for movement checking
#define PLAYERRADIUS        16*FRACUNIT

// MAXRADIUS is for precalculated sector block boxes
// the spider demon is larger,
// but we do not have any moving sectors nearby
#define MAXRADIUS           32*FRACUNIT

#define GRAVITY             FRACUNIT
#define MAXMOVE             (30*FRACUNIT)

#define USERANGE            (64*FRACUNIT)
#define MELEERANGE          (64*FRACUNIT)
#define MISSILERANGE        (32*64*FRACUNIT)

// follow a player exlusively for 3 seconds
#define BASETHRESHOLD       100

#define EXTRAPOINTS         30000

#define NEEDEDCARDFLASH     8

#define ONFLOORZ            INT_MIN
#define ONCEILINGZ          INT_MAX

// Time interval for item respawning.
#define ITEMQUESIZE         128

#define PT_ADDLINES         1
#define PT_ADDTHINGS        2
#define PT_EARLYOUT         4

#define CARDNOTFOUNDYET     -1
#define CARDNOTINMAP        0
#define BONUSADD            6

#define r_blood_min                             r_blood_none
#define r_blood_default                         r_blood_all
#define r_blood_max                             r_blood_all

#define r_bloodsplats_max_min                   0
#define r_bloodsplats_max_default               32768
#define r_bloodsplats_max_max                   32768

#define r_bloodsplats_total_min                 0
#define r_bloodsplats_total_default             0
#define r_bloodsplats_total_max                 0

#define r_corpses_moreblood_default             true
/*
#define r_corpses_smearblood_default            true
#define r_corpses_slide_default                 true
*/
//
// P_TICK
//

// both the head and tail of the thinker list
extern        thinker_t        thinkercap;        

extern        dboolean          infight;

extern        int              species_infighting;

void P_InitThinkers (void);
void P_AddThinker (thinker_t* thinker);
void P_RemoveThinker (thinker_t* thinker);
void P_MapEnd(void);


//
// P_PSPR
//
void P_SetupPsprites (player_t* curplayer);
void P_MovePsprites (player_t* curplayer);
void P_DropWeapon (player_t* player);


//
// P_USER
//
void P_PlayerThink (player_t* player);
void P_AimingHelp (player_t* player);
dboolean P_UseArtifact(player_t * player, artitype_t arti);

//
// P_MOBJ
//

extern mapthing_t           itemrespawnque[ITEMQUESIZE];
extern int                  itemrespawntime[ITEMQUESIZE];
extern int                  iquehead;
extern int                  iquetail;
extern int                  numflats;


void P_RespawnSpecials (void);
void P_CheckMissileSpawn (mobj_t* th);

mobj_t* P_SpawnMobj (fixed_t x, fixed_t y, fixed_t z, mobjtype_t type);

void    P_RemoveMobj (mobj_t* th);
mobj_t* P_SubstNullMobj (mobj_t* th);
dboolean P_SetMobjState (mobj_t* mobj, statenum_t state);
void    P_MobjThinker (mobj_t* mobj);

//void
mobj_t* P_SpawnPuff (fixed_t x, fixed_t y, fixed_t z, angle_t angle);

void    P_SpawnBlood (fixed_t x, fixed_t y, fixed_t z, angle_t angle, int damage, mobj_t* target);
mobj_t* P_SpawnMissile (mobj_t* source, mobj_t* dest, mobjtype_t type);

mobj_t*
//void
        P_SpawnPlayerMissile (mobj_t* source, mobjtype_t type);

void    P_SpawnPlayer (mapthing_t* mthing);

//
// P_ENEMY
//
void    P_NoiseAlert (mobj_t* target, mobj_t* emmiter);


//
// P_MAPUTL
//
typedef struct
{
    fixed_t        x;
    fixed_t        y;
    fixed_t        dx;
    fixed_t        dy;
    
} divline_t;

typedef struct
{
    fixed_t        frac;                // along trace line
    dboolean        isaline;
    union
    {
        mobj_t*    thing;
        line_t*    line;
    } d;
} intercept_t;


typedef dboolean (*traverser_t) (intercept_t *in);

typedef enum
{
    r_blood_none,
    r_blood_redonly,
    r_blood_all
} r_blood_values_e;

fixed_t P_AproxDistance (fixed_t dx, fixed_t dy);
int     P_PointOnLineSide (fixed_t x, fixed_t y, line_t* line);
void    P_MakeDivline (line_t* li, divline_t* dl);
fixed_t P_InterceptVector (divline_t* v2, divline_t* v1);
int     P_BoxOnLineSide (fixed_t* tmbox, line_t* ld);

extern fixed_t                 opentop;
extern fixed_t                 openbottom;
extern fixed_t                 openrange;
extern fixed_t                 lowfloor;

void    P_LineOpening (line_t* linedef);

dboolean P_BlockLinesIterator (int x, int y, dboolean(*func)(line_t*) );
dboolean P_BlockThingsIterator (int x, int y, dboolean(*func)(mobj_t*) );

extern divline_t               trace;

dboolean P_PathTraverse (fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int flags,
                        dboolean (*trav) (intercept_t *));

void P_UnsetThingPosition (mobj_t* thing);
void P_SetThingPosition (mobj_t* thing);


//
// P_MAP
//

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
extern  dboolean        floatok;
extern  fixed_t        tmfloorz;
extern  fixed_t        tmceilingz;

extern  line_t*        ceilingline;
extern  line_t         *blockline;

dboolean P_CheckPosition (mobj_t *thing, fixed_t x, fixed_t y);
dboolean P_TryMove (mobj_t* thing, fixed_t x, fixed_t y, dboolean dropoff);
dboolean P_TeleportMove(mobj_t *thing, fixed_t x, fixed_t y, fixed_t z, dboolean boss);
dboolean P_CheckSight (mobj_t* t1, mobj_t* t2);
dboolean P_ChangeSector (sector_t* sector, dboolean crunch);
mobj_t *P_CheckOnmobj(mobj_t * thing);

void    P_SlideMove (mobj_t* mo);
void    P_UseLines (player_t* player);

extern  mobj_t*        linetarget;        // who got hit (or NULL)

fixed_t P_AimLineAttack (mobj_t* t1, angle_t angle, fixed_t distance);

void P_LineAttack (mobj_t* t1, angle_t angle, fixed_t distance, fixed_t slope, int damage);

void P_RadiusAttack (mobj_t* spot, mobj_t* source, int damage);

void P_SpawnSmokeTrail(fixed_t x, fixed_t y, fixed_t z, angle_t angle);

//
// P_SETUP
//
extern const byte         *rejectmatrix;  // for fast sight rejection

// [crispy] BLOCKMAP limit
extern int*               blockmaplump;     // offsets in blockmap are from here
extern int*               blockmap;         // int for BLOCKMAP limit removal

extern int                bmapwidth;
extern int                bmapheight;       // in mapblocks
extern fixed_t            bmaporgx;
extern fixed_t            bmaporgy;         // origin of block map
extern mobj_t**           blocklinks;       // for thing chains
extern dboolean            felldown;         // killough 11/98: indicates object pushed off ledge
extern fixed_t            tmbbox[4];


//
// P_INTER
//
extern int                god_health;
extern int                idfa_armor;
extern int                idfa_armor_class;
extern int                idkfa_armor;
extern int                idkfa_armor_class;
extern int                initial_health;
extern int                initial_bullets;
extern int                maxhealth;
extern int                max_armor;
extern int                green_armor_class;
extern int                blue_armor_class;
extern int                max_soul;
extern int                soul_health;
extern int                mega_health;
extern int                bfgcells;
extern int                maxammo[NUMAMMO];
extern int                clipammo[NUMAMMO];
extern int                r_blood;
extern mobj_t             *bloodsplats[r_bloodsplats_max_max];
extern int                r_bloodsplats_total;
extern int                r_bloodsplats_max;
extern dboolean            r_corpses_moreblood;
extern dboolean            r_corpses_smearblood;

void P_SetBloodSplatPosition(mobj_t *splat);
void P_SpawnBloodSplat(fixed_t x, fixed_t y, int blood, int maxheight, mobj_t *target);
void P_NullBloodSplatSpawner(fixed_t x, fixed_t y, int blood, int maxheight, mobj_t *target);
void P_TouchSpecialThing (mobj_t* special, mobj_t* toucher);

void P_DamageMobj (mobj_t* target, mobj_t* inflictor, mobj_t* source, int damage);

dboolean P_GiveAmmo (player_t* player, ammotype_t ammo, int num);

int  P_GetThingFloorType(mobj_t * thing);

void P_InitTerrainTypes(void);

int  P_HitFloor(mobj_t * thing);

void P_InitCards(player_t *player);

void P_GiveAllCards(player_t *player);

void P_AddBonus(player_t *player, int amount);

void P_GiveCard (player_t* player, card_t card);

//extern void A_ReFire (player_t* player, pspdef_t* psp );

void P_FallingDamage (mobj_t *mo);

void P_SpawnMapThing(mapthing_t *mthing, int index);

dboolean Check_Sides(mobj_t* actor, int x, int y);

void P_RemoveMobjShadow(mobj_t *mobj);
void P_SpawnShadow(mobj_t *actor);

void EV_LightTurnOnPartway(line_t *line, fixed_t level);
void EV_LightByAdjacentSectors(sector_t *sector, fixed_t level);

int P_FindLineFromLineTag(const line_t *line, int start);

int P_GetMoveFactor(const mobj_t *mo, int *friction);   // killough 8/28/98
int P_GetFriction(const mobj_t *mo, int *factor);       // killough 8/28/98
void P_ApplyTorque(mobj_t *mo);                         // killough 9/12/98
//void P_SpawnFriction(void);      // phares 3/16/98

void P_CalcHeight (player_t* player);
void P_SetPsprite (player_t* player, int position, statenum_t stnum);

//
// P_SPEC
//
#include "p_spec.h"

#endif        // __P_LOCAL__
