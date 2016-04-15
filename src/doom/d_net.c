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
#include "w_checksum.h"
#include "w_wad.h"
#include "wii-doom.h"


ticcmd_t *netcmds;


extern dboolean longtics;
extern dboolean lowres_turn;


// Called when a player leaves the game
static void PlayerQuitGame(player_t *player)
{
    static char exitmsg[80];
    unsigned int player_num = player - players;

#ifdef DEBUG_NET
    printf("PlayerQuitGame\n");
#endif

    // Do this the same way as Vanilla Doom does, to allow dehacked
    // replacements of this message
    M_StringCopy(exitmsg, "Player 1 left the game", sizeof(exitmsg));

    exitmsg[7] += player_num;

    playeringame[player_num] = false;
    players[consoleplayer].message = exitmsg;
/*
    // TODO: check if it is sensible to do this:
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

#ifdef DEBUG_NET
    printf("RunTic\n");
#endif

    // Check for player quits.
    for (i = 0; i < MAXPLAYERS; ++i)
    {
        if (/*!demoplayback &&*/ playeringame[i] && !ingame[i])
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

#ifdef DEBUG_NET
    printf("LoadGameSettings\n");
#endif

    deathmatch = settings->deathmatch;
    startepisode = settings->episode;
    startmap = settings->map;
    startskill = settings->skill;
    startloadgame = settings->loadgame;
//    lowres_turn = settings->lowres_turn;
    nomonsters = settings->nomonsters;
    fastparm = settings->fast_monsters;
    respawnparm = settings->respawn_monsters;
    timelimit = settings->timelimit;
    consoleplayer = settings->consoleplayer;
/*
    if (lowres_turn)
    {
        C_Warning("        NOTE: Turning resolution is reduced; this is probably");
        C_Warning("        because there is a client recording a Vanilla demo.");
    }
*/
    for (i = 0; i < MAXPLAYERS; ++i)
    {
        playeringame[i] = i < settings->num_players;
    }
}

// Save the game settings from global variables to the specified
// game settings structure.
static void SaveGameSettings(net_gamesettings_t *settings)
{
#ifdef DEBUG_NET
    printf("SaveGameSettings\n");
#endif

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

#ifndef WII
    settings->lowres_turn = M_CheckParm("-record") > 0
                         && M_CheckParm("-longtics") == 0;
#else
/*
    if (longtics)
        settings->lowres_turn = true;
    else
*/
        settings->lowres_turn = false;
#endif
}

/*
typedef struct deh_context_s deh_context_t;
typedef struct deh_section_s deh_section_t;
typedef void (*deh_section_init_t)(void);
typedef void *(*deh_section_start_t)(deh_context_t *context, char *line);
typedef void (*deh_section_end_t)(deh_context_t *context, void *tag);
typedef void (*deh_line_parser_t)(deh_context_t *context, char *line, void *tag);
typedef void (*deh_sha1_hash_t)(sha1_context_t *context);

struct deh_section_s
{
    char *name;

    // Called on startup to initialize code
    deh_section_init_t init;
    
    // This is called when a new section is started.  The pointer
    // returned is used as a tag for the following calls.
    deh_section_start_t start;

    // This is called for each line in the section
    deh_line_parser_t line_parser;

    // This is called at the end of the section for any cleanup
    deh_section_end_t end;

    // Called when generating an SHA1 sum of the dehacked state
    deh_sha1_hash_t sha1_hash;
};

// deh_ammo.c:
deh_section_t deh_section_ammo;

// deh_cheat.c:
deh_section_t deh_section_cheat;

// deh_frame.c:
deh_section_t deh_section_frame;

// deh_misc.c:
deh_section_t deh_section_misc;

// deh_ptr.c:
deh_section_t deh_section_pointer;

// deh_sound.c
deh_section_t deh_section_sound;

// deh_text.c:
deh_section_t deh_section_text;

// deh_thing.c:
deh_section_t deh_section_thing;

// deh_weapon.c:
deh_section_t deh_section_weapon;

// deh_bexstr.c:
deh_section_t deh_section_bexstr;

//
// List of section types:
//
deh_section_t *deh_section_types[] =
{
    &deh_section_ammo,
    &deh_section_cheat,
    &deh_section_frame,
    &deh_section_misc,
    &deh_section_pointer,
    &deh_section_sound,
    &deh_section_text,
    &deh_section_thing,
    &deh_section_weapon,
    &deh_section_bexstr,
    NULL
};

static void DEH_Checksum(sha1_digest_t digest)
{
    sha1_context_t sha1_context;
    unsigned int i;

    SHA1_Init(&sha1_context);

    for (i = 0; deh_section_types[i] != NULL; ++i)
    {
        if (deh_section_types[i]->sha1_hash != NULL)
        {
            deh_section_types[i]->sha1_hash(&sha1_context);
        }
    }

    SHA1_Final(digest, &sha1_context);
}
*/

static void InitConnectData(net_connect_data_t *connect_data)
{
#ifdef DEBUG_NET
    printf("InitConnectData\n");
#endif

    connect_data->max_players = MAXPLAYERS;
    connect_data->drone = false;

    //!
    // @category net
    //
    // Run as the left screen in three screen mode.
    //
#ifndef WII
    if (M_CheckParm("-left") > 0)
    {
        viewangleoffset = ANG90;
        connect_data->drone = true;
    }

    //! 
    // @category net
    //
    // Run as the right screen in three screen mode.
    //
    if (M_CheckParm("-right") > 0)
    {
        viewangleoffset = ANG270;
        connect_data->drone = true;
    }
#endif

    //
    // Connect data
    //

    // Game type fields:
    connect_data->gamemode = gamemode;
    connect_data->gamemission = gamemission;
/*
    // Are we recording a demo? Possibly set lowres turn mode
    connect_data->lowres_turn = M_CheckParm("-record") > 0
                             && M_CheckParm("-longtics") == 0;
*/
    connect_data->lowres_turn = false;

    // Read checksums of our WAD directory and dehacked information
    W_Checksum(connect_data->wad_sha1sum);

    // FIXME: ???
    //DEH_Checksum(connect_data->deh_sha1sum);

    // Are we playing with the Freedoom IWAD?
    connect_data->is_freedoom = W_CheckNumForName("FREEDOOM") >= 0;
}

void D_ConnectNetGame(void)
{
    net_connect_data_t connect_data;

#ifdef DEBUG_NET
    printf("D_ConnectNetGame\n");
#endif

    InitConnectData(&connect_data);
    netgame = D_InitNetGame(&connect_data);

    //!
    // @category net
    //
    // Start the game playing as though in a netgame with a single
    // player.  This can also be used to play back single player netgame
    // demos.
    //
#ifndef WII
    if (M_CheckParm("-solo-net") > 0)
    {
        netgame = true;
    }
#endif
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//
void D_CheckNetGame(void)
{
    net_gamesettings_t settings;

#ifdef DEBUG_NET
    printf("D_CheckNetGame\n");
#endif

    if (netgame)
    {
        autostart = true;
    }

    D_RegisterLoopCallbacks(&doom_loop_interface);

    SaveGameSettings(&settings);
    D_StartNetGame(&settings, NULL);
    LoadGameSettings(&settings);

    C_Network("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i",
               startskill, deathmatch, startmap, startepisode);

    C_Network("player %i of %i (%i nodes)",
               consoleplayer + 1, settings.num_players, settings.num_players);

#ifndef WII
    // Show players here; the server might have specified a time limit
    if (timelimit > 0 && deathmatch)
    {
        // Gross hack to work like Vanilla:

        if (timelimit == 20 && M_CheckParm("-avg"))
        {
            C_Network("Austin Virtual Gaming: Levels will end "
                           "after 20 minutes");
        }
        else
        {
            C_Network("Levels will end after %d minute", timelimit);

            if (timelimit > 1)
                C_Network("s");

            C_Network(".");
        }
    }
#endif
}

