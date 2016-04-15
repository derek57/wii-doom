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
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include
//    this header file.
//   In practice, things are a bit messy.
//
//-----------------------------------------------------------------------------


#ifndef __D_STATE__
#define __D_STATE__


// We need globally shared data structures,
//  for defining the global state variables.
#include "doomdata.h"

#include "../d_loop.h"
#include "../d_mode.h"

// We need the playr data structure as well.
#include "d_player.h"

#include "../net_defs.h"


// Convenience macro.
// 'gamemission' can be equal to pack_chex or pack_hacx, but these are
// just modified versions of doom and doom2, and should be interpreted
// as the same most of the time.

#define logical_gamemission   (gamemission == pack_chex ? doom :  \
                              gamemission == pack_hacx ? doom2 : gamemission)

// Player spawn spots for deathmatch.
#define MAX_DM_STARTS         10


// -----------------------------------------------------
// Game Mode - identify IWAD as shareware, retail etc.
//
extern  GameMode_t       gamemode;

extern  GameMission_t    gamemission;

extern  GameVersion_t    gameversion;

// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern  skill_t          startskill;

// Selected by user. 
extern  skill_t          gameskill;

// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern  gamestate_t      wipegamestate;

//?
extern  gamestate_t      gamestate;

//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.

// Bookkeeping on players - state.
extern  player_t         players[MAXPLAYERS];

extern  mapthing_t       deathmatchstarts[MAX_DM_STARTS];
extern  mapthing_t       *deathmatch_p;

// Player spawn spots.
extern  mapthing_t       playerstarts[MAXPLAYERS];

// Intermission stats.
// Parameters for world map / intermission.
extern  wbstartstruct_t  wminfo;        

extern  ticcmd_t         *netcmds;

extern  fixed_t          forwardmove; 
extern  fixed_t          sidemove; 


void A_MoreGibs(mobj_t *actor);

#endif

