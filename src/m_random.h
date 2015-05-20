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


#include "doomtype.h"


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
  NUMPRCLASS                  // MUST be last item in list
} pr_class_t;


// The random number generator's state.
typedef struct {
  unsigned long seed[NUMPRCLASS];      // Each block's random seed
  int rndindex, prndindex;             // For compatibility support
} rng_t;


extern rng_t rng;                      // The rng's state

extern unsigned long rngseed;          // The starting seed (not part of state)


int P_SignedRandom ();

// Returns a number from 0 to 255,
// from a lookup table.
int M_Random (void);

int M_RandomInt(int lower, int upper);

// As M_Random, but used only by the play simulation.
int P_Random (void);

int P_RandomSMMU(pr_class_t pr_class);

// Fix randoms for demos.
void M_ClearRandom (void);


#endif
