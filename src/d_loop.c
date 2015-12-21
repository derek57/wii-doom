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
//     Main loop code.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <string.h>

#include "c_io.h"
#include "doomfeatures.h"
#include "d_event.h"
#include "d_loop.h"
#include "d_ticcmd.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_fixed.h"
#include "v_trans.h"


// The complete set of data for a particular tic.

typedef struct
{
    ticcmd_t cmds[NET_MAXPLAYERS];
    dboolean ingame[NET_MAXPLAYERS];
} ticcmd_set_t;

// Maximum time that we wait in TryRunTics() for netgame data to be
// received before we bail out and render a frame anyway.
// Vanilla Doom used 20 for this value, but we use a smaller value
// instead for better responsiveness of the menu when we're stuck.
#define MAX_NETGAME_STALL_TICS  5

//
// gametic is the tic about to (or currently being) run
// maketic is the tic that hasn't had control made for it yet
// recvtic is the latest tic received from the server.
//
// a gametic cannot be run until ticcmds are received for it
// from all players.
//

// The index of the next tic to be made (with a call to BuildTiccmd).

static int      maketic;

// The number of complete tics received from the server so far.

static int      recvtic;

// Index of the local player.

static int      localplayer;

// Used for original sync code.

static int      skiptics = 0;

// Requested player class "sent" to the server on connect.
// If we are only doing a single player game then this needs to be remembered
// and saved in the game settings.

static int      player_class;

// Current players in the multiplayer game.
// This is distinct from playeringame[] used by the game code, which may
// modify playeringame[] when playing back multiplayer demos.

static dboolean  local_playeringame[NET_MAXPLAYERS];

// Use new client syncronisation code

static dboolean  new_sync = true;

// Callback functions for loop code.

static loop_interface_t *loop_interface = NULL;

static ticcmd_set_t ticdata[BACKUPTICS];

// The number of tics that have been run (using RunTic) so far.

int             gametic;

// Reduce the bandwidth needed by sampling game input less and transmitting
// less.  If ticdup is 2, sample half normal, 3 = one third normal, etc.

int             ticdup;
int             lasttime;

// Amount to offset the timer for game sync.

fixed_t         offsetms;

extern dboolean  privateserverflag;
extern dboolean  multiplayerflag;

// 35 fps clock adjusted by offsetms milliseconds

static int GetAdjustedTime(void)
{
    int time_ms;

    time_ms = I_GetTimeMS();

    if (new_sync)
    {
        // Use the adjustments from net_client.c only if we are
        // using the new sync mode.

        time_ms += (offsetms / FRACUNIT);
    }

    return (time_ms * TICRATE) / 1000;
}

static dboolean BuildNewTic(void)
{
    int      gameticdiv;
    ticcmd_t cmd;

    gameticdiv = gametic/ticdup;

    I_StartTic ();
    loop_interface->ProcessEvents();

    // Always run the menu

    loop_interface->RunMenu();

    if (new_sync)
    {
       // If playing single player, do not allow tics to buffer
       // up very far

       if (maketic - gameticdiv > 2)
           return false;

       // Never go more than ~200ms ahead

       if (maketic - gameticdiv > 8)
           return false;
    }
    else
    {
       if (maketic - gameticdiv >= 5)
           return false;
    }

    //printf ("mk:%i ",maketic);
    memset(&cmd, 0, sizeof(ticcmd_t));
    loop_interface->BuildTiccmd(&cmd, maketic);

    ticdata[maketic % BACKUPTICS].cmds[localplayer] = cmd;
    ticdata[maketic % BACKUPTICS].ingame[localplayer] = true;

    ++maketic;

    return true;
}

//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
void NetUpdate (void)
{
    int nowtime;
    int newtics;
    int i;

    // check time
    nowtime = GetAdjustedTime() / ticdup;
    newtics = nowtime - lasttime;

    lasttime = nowtime;

    if (skiptics <= newtics)
    {
        newtics -= skiptics;
        skiptics = 0;
    }
    else
    {
        skiptics -= newtics;
        newtics = 0;
    }

    // build new ticcmds for console player

    for (i=0 ; i<newtics ; i++)
    {
        if (!BuildNewTic())
        {
            break;
        }
    }
}

//
// Start game loop
//
// Called after the screen is set but before the game starts running.
//

void D_StartGameLoop(void)
{
    lasttime = GetAdjustedTime() / ticdup;
}

void D_StartNetGame(net_gamesettings_t *settings,
                    netgame_startup_callback_t callback)
{
    int i = 0;

    offsetms = 0;
    recvtic = 0;

    settings->consoleplayer = 0;
    settings->num_players = 1;
    settings->player_classes[0] = player_class;

    //!
    // @category net
    //
    // Use new network client sync code rather than the classic
    // sync code. This is currently disabled by default because it
    // has some bugs.
    //

#ifndef WII
    if (M_CheckParm("-newsync") > 0 && !beta_style)
        settings->new_sync = 1;
    else
#endif
        settings->new_sync = 0;

    //!
    // @category net
    // @arg <n>
    //
    // Send n extra tics in every packet as insurance against dropped
    // packets.
    //

#ifndef WII
    if(!beta_style)
        i = M_CheckParmWithArgs("-extratics", 1);

    if (i > 0)
        settings->extratics = atoi(myargv[i+1]);
    else
#endif
        settings->extratics = 1;

    //!
    // @category net
    // @arg <n>
    //
    // Reduce the resolution of the game by a factor of n, reducing
    // the amount of network bandwidth needed.
    //

#ifndef WII
    if(!beta_style)
        i = M_CheckParmWithArgs("-dup", 1);

    if (i > 0)
        settings->ticdup = atoi(myargv[i+1]);
    else
#endif
        settings->ticdup = 1;

    // Set the local player and playeringame[] values.

    localplayer = settings->consoleplayer;

    for (i = 0; i < NET_MAXPLAYERS; ++i)
    {
        local_playeringame[i] = i < settings->num_players;
    }

    // Copy settings to global variables.

    ticdup = settings->ticdup;
    new_sync = settings->new_sync;
/*
    if (!new_sync)
    {
        C_Network("Syncing netgames like Vanilla Doom.");
    }
*/
}

static int GetLowTic(void)
{
    int lowtic;

    lowtic = maketic;

    return lowtic;
}

// When using ticdup, certain values must be cleared out when running
// the duplicate ticcmds.

static void TicdupSquash(ticcmd_set_t *set)
{
    unsigned int i;

    for (i = 0; i < NET_MAXPLAYERS ; ++i)
    {
        ticcmd_t *cmd = &set->cmds[i];
        cmd->chatchar = 0;
        if (cmd->buttons & BT_SPECIAL)
            cmd->buttons = 0;
    }
}

// When running in single player mode, clear all the ingame[] array
// except the local player.

static void SinglePlayerClear(ticcmd_set_t *set)
{
    unsigned int i;

    for (i = 0; i < NET_MAXPLAYERS; ++i)
    {
        if (i != localplayer)
        {
            set->ingame[i] = false;
        }
    }
}

//
// TryRunTics
//

void TryRunTics (void)
{
    int i;
    int lowtic;
    int availabletics;
    static int oldentertics;

    // get real tics
    int entertic = I_GetTime() / ticdup;
    int counts;

    int realtics = entertic - oldentertics;
    oldentertics = entertic;

    NetUpdate ();

    lowtic = GetLowTic();

    availabletics = lowtic - gametic/ticdup;

    // decide how many tics to run

    if (new_sync)
    {
        counts = availabletics;
    }
    else
    {
        // decide how many tics to run
        if (realtics < availabletics-1)
            counts = realtics+1;
        else if (realtics < availabletics)
            counts = realtics;
        else
            counts = availabletics;

        // [AM] If we've uncapped the framerate and there are no tics
        // to run, return early instead of waiting around.
        if (counts == 0 && d_uncappedframerate && gametic)
            return;

        if (counts < 1)
            counts = 1;
    }

    if (counts < 1)
        counts = 1;

    // wait for new tics if needed
    while (lowtic < gametic/ticdup + counts)
    {
        NetUpdate ();

        lowtic = GetLowTic();

        if (lowtic < gametic/ticdup)
            I_Error ("TryRunTics: lowtic < gametic");

        // Still no tics to run? Sleep until some are available.
        if (lowtic < gametic/ticdup + counts)
        {
            // If we're in a netgame, we might spin forever waiting for
            // new network data to be received. So don't stay in here
            // forever - give the menu a chance to work.

            if (I_GetTime() / ticdup - entertic >= MAX_NETGAME_STALL_TICS)
            {
                return;
            }

            I_Sleep(1);
        }
    }

    // run the count * ticdup dics
    while (counts--)
    {
        ticcmd_set_t *set;

        set = &ticdata[(gametic / ticdup) % BACKUPTICS];

        SinglePlayerClear(set);

        for (i=0 ; i<ticdup ; i++)
        {
            if (gametic/ticdup > lowtic)
                I_Error ("gametic>lowtic");

            memcpy(local_playeringame, set->ingame, sizeof(local_playeringame));

            loop_interface->RunTic(set->cmds, set->ingame);
            gametic++;

            // modify command for duplicated tics

            TicdupSquash(set);
        }

        NetUpdate ();        // check for new console commands
    }
}

void D_RegisterLoopCallbacks(loop_interface_t *i)
{
    loop_interface = i;
}

