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
//
//    
//-----------------------------------------------------------------------------


#ifndef __M_RANDOM__
#define __M_RANDOM__


#include "doomfeatures.h"
#include "doomtype.h"


#define M_RangeRandom(min, max) P_RangeRandom(pr_misc, (min), (max))

// Returns a number from 0 to 255,
#define M_RandomSMMU()          P_RandomSMMU(pr_misc)


// killough 1/19/98: rewritten to use to use a better random number generator
// in the new engine, although the old one is available for compatibility.

// killough 2/16/98:
//
// Make every random number generator local to each control-equivalent block.
// Critical for demo sync. Changing the order of this list breaks all previous
// versions' demos. The random number generators are made local to reduce the
// chances of sync problems. In Doom, if a single random number generator call
// was off, it would mess up all random number generators. This reduces the
// chances of it happening by making each RNG local to a control flow block.
//
// Notes to developers: if you want to reduce your demo sync hassles, follow
// this rule: for each call to P_Random you add, add a new class to the enum
// type below for each block of code which calls P_Random. If two calls to
// P_Random are not in "control-equivalent blocks", i.e. there are any cases
// where one is executed, and the other is not, put them in separate classes.
//
// Keep all current entries in this list the same, and in the order
// indicated by the #'s, because they're critical for preserving demo
// sync. Do not remove entries simply because they become unused later.

typedef enum {
    pr_skullfly,
    pr_damage,
    pr_crush,
    pr_genlift,
    pr_killtics,
    pr_damagemobj,
    pr_painchance,
    pr_lights,
    pr_explode,
    pr_respawn,
    pr_lastlook,
    pr_spawnthing,
    pr_spawnpuff,
    pr_spawnblood,
    pr_missile,
    pr_shadow,
    pr_plats,
    pr_punch,
    pr_punchangle,
    pr_saw,
    pr_plasma,
    pr_gunshot,
    pr_misfire,
    pr_shotgun,
    pr_bfg,
    pr_slimehurt,
    pr_dmspawn,
    pr_missrange,
    pr_trywalk,
    pr_newchase,
    pr_newchasedir,
    pr_see,
    pr_facetarget,
    pr_posattack,
    pr_sposattack,
    pr_cposattack,
    pr_spidrefire,
    pr_troopattack,
    pr_sargattack,
    pr_headattack,
    pr_bruisattack,
    pr_tracer,
    pr_skelfist,
    pr_scream,
    pr_brainscream,
    pr_cposrefire,
    pr_brainexp,
    pr_spawnfly,
    pr_misc,
    pr_all_in_one,

    // Start new entries -- add new entries below
    pr_opendoor,
    pr_targetsearch,
    pr_friends,
    pr_threshold,
    pr_skiptarget,
    pr_enemystrafe,
    pr_avoidcrush,
    pr_stayonlift,
    pr_helpfriend,
    pr_dropoff,
    pr_randomjump,
    pr_defect,

    // FraggleScript
    pr_script,

    // End of new entries

    // Start Eternity classes
    // Minotaur attacks
    pr_minatk1,
    pr_minatk2,
    pr_minatk3,
    pr_mindist,
    pr_mffire,

    // SetTics codepointer
    pr_settics,

    // Heretic volcano stuff
    pr_volcano,

    // ditto
    pr_svolcano,
    pr_clrattack,

    // TerrainTypes
    pr_splash,

    // lightning flashes
    pr_lightning,
    pr_nextflash,
    pr_cloudpick,
    pr_fogangle,
    pr_fogcount,
    pr_fogfloat,

    // floatbobbing seed
    pr_floathealth,

    // A_SubTics
    pr_subtics,

    // A_CentaurAttack
    pr_centauratk,

    // A_DropEquipment
    pr_dropequip,
    pr_bishop1,

    // steam spawn codepointer 
    pr_steamspawn,

    // minotaur inflictor special
    pr_mincharge,

    // missile reflection
    pr_reflect,

    // teleglitter z coord
    pr_tglitz,

    pr_bishop2,

    // parameterized pointers
    pr_custombullets,
    pr_custommisfire,
    pr_custompunch,

    // teleglitter spawn
    pr_tglit,

    // random spawn float z flag
    pr_spawnfloat,

    // mummy punches
    pr_mumpunch,
    pr_mumpunch2,  

    // heretic item drops
    pr_hdrop1,
    pr_hdrop2,     
    pr_hdropmom,   

    // clink scratch
    pr_clinkatk,

    // random failure to sight ghost player
    pr_ghostsneak,

    // wizard attack
    pr_wizatk,

    // make seesound instead of active sound
    pr_lookact,

    // d'sparil stuff
    pr_sorctele1,
    pr_sorctele2,  
    pr_sorfx1xpl,  
    pr_soratk1,    
    pr_soratk2,    
    pr_bluespark,  

    // pod pain
    pr_podpain,

    // pod spawn
    pr_makepod,

    // knight scratch
    pr_knightat1,

    // knight projectile choice
    pr_knightat2,

    // for A_DripBlood
    pr_dripblood,

    // beast bite
    pr_beastbite,

    // beast ball puff spawn
    pr_puffy,

    // sorcerer serpent attack
    pr_sorc1atk,

    // BulletAttack ptr
    pr_monbullets,

    pr_monmisfire,

    // SetCounter ptr
    pr_setcounter,

    // Heretic mad fighting after player death
    pr_madmelee,

    // Whirlwind inflictor
    pr_whirlwind,

    // Iron Lich attacks
    pr_lichmelee,
    pr_lichattack, 

    // Whirlwind seeking
    pr_whirlseek,

    // Imp charge attack
    pr_impcharge,

    // Imp melee attack
    pr_impmelee,

    // Leader imp melee
    pr_impmelee2,

    // Imp crash
    pr_impcrash,

    // RandomWalk rngs
    pr_rndwnewdir,
    pr_rndwmovect,
    pr_rndwspawn,

    // WeaponSetCtr
    pr_weapsetctr,

    // T_QuakeThinker
    pr_quake,

    // quake damage
    pr_quakedmg,

    // Heretic skull flying
    pr_skullpop,

    // A_CentaurDefend
    pr_centaurdef,

    pr_bishop3,

    // A_SpawnBlur
    pr_spawnblur,

    // A_DemonAttack1
    pr_chaosbite,

    // A_WraithMelee
    pr_wraithm,

    pr_wraithd, 
    pr_wraithfx2,
    pr_wraithfx3,
    pr_wraithfx4a,
    pr_wraithfx4b,
    pr_wraithfx4c,
    pr_ettin,

    // A_AffritSpawnRock
    pr_affritrock,

    // A_SmBounce
    pr_smbounce,

    // A_AffritSplotch
    pr_affrits,

    // A_IceGuyLook
    pr_icelook,

    pr_icelook2,

    // A_IceGuyChase
    pr_icechase,
    pr_icechase2, 

    // A_DragonFX2
    pr_dragonfx,

    // A_DropMace
    pr_dropmace,

    // ripper missile damage
    pr_rip,

    // A_CasingThrust
    pr_casing,

    // A_GenRefire
    pr_genrefire,

    // A_Jump
    pr_decjump,
    pr_decjump2,

    // MUST be last item in list
    NUMPRCLASS

} pr_class_t;

// The random number generator's state.

typedef struct {

    // Each block's random seed
    unsigned long seed[NUMPRCLASS];

    // For compatibility support
    int           rndindex;
    int           prndindex;

} rng_t;


// Fix randoms for demos.
void M_ClearRandom(void);

//int P_SignedRandom();

// Returns a number from 0 to 255,
// from a lookup table.
int M_Random(void);

int M_RandomInt(int lower, int upper);

int M_RandomIntNoRepeat(int lower, int upper, int previous);

// As M_Random, but used only by the play simulation.
int P_Random(void);

int P_RandomSMMU(pr_class_t pr_class);

int P_SubRandom(pr_class_t pr_class);

// haleyjd: function to get a random within a given range
int P_RangeRandom(pr_class_t pr_class, int min, int max);


// The rng's state
extern rng_t         rng;

// The starting seed (not part of state)
extern unsigned long rngseed;

#endif

