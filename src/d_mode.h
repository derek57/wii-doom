//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
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
// DESCRIPTION:
//   Functions and definitions relating to the game type and operational
//   mode.
//


#ifndef __D_MODE__
#define __D_MODE__


#include "doomtype.h"


// The "mission" controls what game we are playing.
typedef enum
{
    // Doom 1
    doom,

    // Doom 2
    doom2,

    // Final Doom: TNT: Evilution
    pack_tnt,

    // Final Doom: The Plutonia Experiment
    pack_plut,

    // Chex Quest (modded doom)
    pack_chex,

    // Hacx (modded doom2)
    pack_hacx,

    // Doom 2: No Rest For The Living
    pack_nerve,

    // Master Levels for Doom 2
    pack_master,

    none

} GameMission_t;

// The "mode" allows more accurate specification of the game mode we are
// in: eg. shareware vs. registered.  So doom1.wad and doom.wad are the
// same mission, but a different mode.
typedef enum
{
    // Doom/Heretic shareware
    shareware,

    // Doom/Heretic registered
    registered,

    // Doom II/Hexen
    commercial,

    // Ultimate Doom
    retail,

    // Unknown.
    indetermined

} GameMode_t;

// What version are we emulating?
typedef enum
{
    // Doom 1.2: shareware and registered
    exe_doom_1_2,

    // Doom 1.666: for shareware, registered and commercial
    exe_doom_1_666,

    // Doom 1.7/1.7a: "
    exe_doom_1_7,

    // Doom 1.8: "
    exe_doom_1_8,

    // Doom 1.9: "
    exe_doom_1_9,

    // Hacx
    exe_hacx,

    // Ultimate Doom (retail)
    exe_ultimate,

    // Final Doom
    exe_final,

    // Final Doom (alternate exe)
    exe_final2,

    // Chex Quest executable (based on Final Doom)
    exe_chex

} GameVersion_t;

// Skill level.
typedef enum
{
    // the "-skill 0" hack
    sk_noitems = -1,

    sk_baby = 0,
    sk_easy,
    sk_medium,
    sk_hard,
    sk_nightmare

} skill_t;


int D_GetNumEpisodes(GameMission_t mission, GameMode_t mode);

char *D_GameMissionString(GameMission_t mission);

dboolean D_ValidGameMode(GameMission_t mission, GameMode_t mode);
dboolean D_ValidGameVersion(GameMission_t mission, GameVersion_t version);
dboolean D_IsEpisodeMap(GameMission_t mission);
dboolean D_ValidEpisodeMap(GameMission_t mission, GameMode_t mode,
                          int episode, int map);

#endif /* #ifndef __D_MODE__ */

