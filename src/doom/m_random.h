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
  pr_skullfly,                // #1
  pr_damage,                  // #2
  pr_crush,                   // #3
  pr_genlift,                 // #4
  pr_killtics,                // #5
  pr_damagemobj,              // #6
  pr_painchance,              // #7
  pr_lights,                  // #8
  pr_explode,                 // #9
  pr_respawn,                 // #10
  pr_lastlook,                // #11
  pr_spawnthing,              // #12
  pr_spawnpuff,               // #13
  pr_spawnblood,              // #14
  pr_missile,                 // #15
  pr_shadow,                  // #16
  pr_plats,                   // #17
  pr_punch,                   // #18
  pr_punchangle,              // #19
  pr_saw,                     // #20
  pr_plasma,                  // #21
  pr_gunshot,                 // #22
  pr_misfire,                 // #23
  pr_shotgun,                 // #24
  pr_bfg,                     // #25
  pr_slimehurt,               // #26
  pr_dmspawn,                 // #27
  pr_missrange,               // #28
  pr_trywalk,                 // #29
  pr_newchase,                // #30
  pr_newchasedir,             // #31
  pr_see,                     // #32
  pr_facetarget,              // #33
  pr_posattack,               // #34
  pr_sposattack,              // #35
  pr_cposattack,              // #36
  pr_spidrefire,              // #37
  pr_troopattack,             // #38
  pr_sargattack,              // #39
  pr_headattack,              // #40
  pr_bruisattack,             // #41
  pr_tracer,                  // #42
  pr_skelfist,                // #43
  pr_scream,                  // #44
  pr_brainscream,             // #45
  pr_cposrefire,              // #46
  pr_brainexp,                // #47
  pr_spawnfly,                // #48
  pr_misc,                    // #49
  pr_all_in_one,              // #50
  // Start new entries -- add new entries below
  pr_opendoor,                // #51
  pr_targetsearch,            // #52
  pr_friends,                 // #53
  pr_threshold,               // #54
  pr_skiptarget,              // #55
  pr_enemystrafe,             // #56
  pr_avoidcrush,              // #57
  pr_stayonlift,              // #58
  pr_helpfriend,              // #59
  pr_dropoff,                 // #60
  pr_randomjump,              // #61
  pr_defect,                  // #62
  pr_script,                  // #63: FraggleScript
  // End of new entries

  // Start Eternity classes
  pr_minatk1,   // Minotaur attacks
  pr_minatk2,
  pr_minatk3,
  pr_mindist,
  pr_mffire,
  pr_settics,   // SetTics codepointer
  pr_volcano,   // Heretic volcano stuff
  pr_svolcano,  // ditto
  pr_clrattack,
  pr_splash,    // TerrainTypes
  pr_lightning, // lightning flashes
  pr_nextflash,
  pr_cloudpick,
  pr_fogangle,
  pr_fogcount,
  pr_fogfloat,
  pr_floathealth, // floatbobbing seed
  pr_subtics,     // A_SubTics
  pr_centauratk,  // A_CentaurAttack
  pr_dropequip,   // A_DropEquipment
  pr_bishop1,
  pr_steamspawn, // steam spawn codepointer 
  pr_mincharge,  // minotaur inflictor special
  pr_reflect,    // missile reflection
  pr_tglitz,     // teleglitter z coord
  pr_bishop2,
  pr_custombullets, // parameterized pointers
  pr_custommisfire,
  pr_custompunch,
  pr_tglit,      // teleglitter spawn
  pr_spawnfloat, // random spawn float z flag
  pr_mumpunch,   // mummy punches
  pr_mumpunch2,  
  pr_hdrop1,     // heretic item drops
  pr_hdrop2,     
  pr_hdropmom,   
  pr_clinkatk,   // clink scratch
  pr_ghostsneak, // random failure to sight ghost player
  pr_wizatk,     // wizard attack
  pr_lookact,    // make seesound instead of active sound
  pr_sorctele1,  // d'sparil stuff
  pr_sorctele2,  
  pr_sorfx1xpl,  
  pr_soratk1,    
  pr_soratk2,    
  pr_bluespark,  
  pr_podpain,    // pod pain
  pr_makepod,    // pod spawn
  pr_knightat1,  // knight scratch
  pr_knightat2,  // knight projectile choice
  pr_dripblood,  // for A_DripBlood
  pr_beastbite,  // beast bite
  pr_puffy,      // beast ball puff spawn
  pr_sorc1atk,   // sorcerer serpent attack
  pr_monbullets, // BulletAttack ptr
  pr_monmisfire,
  pr_setcounter, // SetCounter ptr
  pr_madmelee,   // Heretic mad fighting after player death
  pr_whirlwind,  // Whirlwind inflictor
  pr_lichmelee,  // Iron Lich attacks
  pr_lichattack, 
  pr_whirlseek,  // Whirlwind seeking
  pr_impcharge,  // Imp charge attack
  pr_impmelee,   // Imp melee attack
  pr_impmelee2,  // Leader imp melee
  pr_impcrash,   // Imp crash
  pr_rndwnewdir, // RandomWalk rngs
  pr_rndwmovect,
  pr_rndwspawn,
  pr_weapsetctr, // WeaponSetCtr
  pr_quake,      // T_QuakeThinker
  pr_quakedmg,   // quake damage
  pr_skullpop,   // Heretic skull flying
  pr_centaurdef, // A_CentaurDefend
  pr_bishop3,
  pr_spawnblur,  // A_SpawnBlur
  pr_chaosbite,  // A_DemonAttack1
  pr_wraithm,    // A_WraithMelee
  pr_wraithd, 
  pr_wraithfx2,
  pr_wraithfx3,
  pr_wraithfx4a,
  pr_wraithfx4b,
  pr_wraithfx4c,
  pr_ettin,
  pr_affritrock, // A_AffritSpawnRock
  pr_smbounce,   // A_SmBounce
  pr_affrits,    // A_AffritSplotch
  pr_icelook,    // A_IceGuyLook
  pr_icelook2,
  pr_icechase,   // A_IceGuyChase
  pr_icechase2, 
  pr_dragonfx,   // A_DragonFX2
  pr_dropmace,   // A_DropMace
  pr_rip,        // ripper missile damage
  pr_casing,     // A_CasingThrust
  pr_genrefire,  // A_GenRefire
  pr_decjump,    // A_Jump
  pr_decjump2,

  NUMPRCLASS                  // MUST be last item in list
} pr_class_t;

// The random number generator's state.
typedef struct {
  unsigned long seed[NUMPRCLASS];      // Each block's random seed
  int rndindex, prndindex;             // For compatibility support
} rng_t;


extern rng_t rng;                      // The rng's state

extern unsigned long rngseed;          // The starting seed (not part of state)

//int P_SignedRandom ();

// Returns a number from 0 to 255,
// from a lookup table.
int M_Random (void);

int M_RandomInt(int lower, int upper);

// As M_Random, but used only by the play simulation.
int P_Random (void);

int P_RandomSMMU(pr_class_t pr_class);

int P_SubRandom(pr_class_t pr_class);

// Fix randoms for demos.
void M_ClearRandom (void);

// haleyjd: function to get a random within a given range
int P_RangeRandom(pr_class_t pr_class, int min, int max);

#define M_RangeRandom(min, max) P_RangeRandom(pr_misc, (min), (max))

// Returns a number from 0 to 255,
#define M_RandomSMMU() P_RandomSMMU(pr_misc)

#endif
