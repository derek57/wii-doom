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
//        DOOM Network game communication and protocol,
//        all OS independend parts.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <unistd.h>

#include "c_io.h"
#include "d_loop.h"
#include "d_net.h"
#include "d_main.h"
#include "doomdef.h"
#include "doomfeatures.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_menu.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_misc.h"
#include "v_trans.h"
#include "w_wad.h"


ticcmd_t *netcmds;


extern dboolean longtics;
extern dboolean lowres_turn;


// Called when a player leaves the game
static void PlayerQuitGame(player_t *player)
{
    static char exitmsg[80];
    unsigned int player_num = player - players;

    // Do this the same way as Vanilla Doom does, to allow dehacked
    // replacements of this message
    M_StringCopy(exitmsg, "Player 1 left the game", sizeof(exitmsg));

    exitmsg[7] += player_num;

    playeringame[player_num] = false;
    players[consoleplayer].message = exitmsg;

    // TODO: check if it is sensible to do this:
/*
    if (demorecording) 
    {
        G_CheckDemoStatus();
    }
*/
}

static void RunTic(ticcmd_t *cmds, dboolean *ingame)
{
    extern dboolean advancedemo;
    unsigned int i;

    // Check for player quits.
    for (i = 0; i < MAXPLAYERS; ++i)
    {
        if (!demoplayback && playeringame[i] && !ingame[i])
        {
            PlayerQuitGame(&players[i]);
        }
    }

    netcmds = cmds;

    // check that there are players in the game.  if not, we cannot
    // run a tic.
    if (advancedemo)
        D_DoAdvanceDemo();

    G_Ticker();
}

static loop_interface_t doom_loop_interface = {
    D_ProcessEvents,
    G_BuildTiccmd,
    RunTic,
    M_Ticker
};


// Load game settings from the specified structure and
// set global variables.
static void LoadGameSettings(net_gamesettings_t *settings)
{
    unsigned int i;

    deathmatch = settings->deathmatch;
    startepisode = settings->episode;
    startmap = settings->map;
    startskill = settings->skill;
    startloadgame = settings->loadgame;
    lowres_turn = settings->lowres_turn;
    nomonsters = settings->nomonsters;
    fastparm = settings->fast_monsters;
    respawnparm = settings->respawn_monsters;
    timelimit = settings->timelimit;
    consoleplayer = settings->consoleplayer;

    if (lowres_turn)
    {
        C_Warning("        NOTE: Turning resolution is reduced; this is probably");
        C_Warning("        because there is a client recording a Vanilla demo.");
    }

    for (i = 0; i < MAXPLAYERS; ++i)
    {
        playeringame[i] = i < settings->num_players;
    }
}

// Save the game settings from global variables to the specified
// game settings structure.
static void SaveGameSettings(net_gamesettings_t *settings)
{
    // Fill in game settings structure with appropriate parameters
    // for the new game
    settings->deathmatch = deathmatch;
    settings->episode = startepisode;
    settings->map = startmap;
    settings->skill = startskill;
    settings->loadgame = startloadgame;
    settings->gameversion = gameversion;
    settings->nomonsters = nomonsters;
    settings->fast_monsters = fastparm;
    settings->respawn_monsters = respawnparm;
    settings->timelimit = timelimit;
/*
#ifndef WII
    settings->lowres_turn = M_CheckParm("-record") > 0
                         && M_CheckParm("-longtics") == 0;
#else
    if (longtics)
        settings->lowres_turn = true;
    else
*/
        settings->lowres_turn = false;
//#endif
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
void D_CheckNetGame(void)
{
    net_gamesettings_t settings;

    if (netgame)
    {
        autostart = true;
    }

    D_RegisterLoopCallbacks(&doom_loop_interface);

    SaveGameSettings(&settings);
    D_StartNetGame(&settings, NULL);
    LoadGameSettings(&settings);

    C_Output("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i",
               startskill, deathmatch, startmap, startepisode);

    C_Output("player %i of %i (%i nodes)",
               consoleplayer + 1, settings.num_players, settings.num_players);

    // Show players here; the server might have specified a time limit
    if (timelimit > 0 && deathmatch)
    {
        // Gross hack to work like Vanilla:
        C_Network("Levels will end after %d minute", timelimit);

        if (timelimit > 1)
            C_Network("s");

        C_Network(".");
    }
}

