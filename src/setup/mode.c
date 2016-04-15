//
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


#include <stdlib.h>
#include <string.h>

#include "doomtype.h"
#include "config.h"
#include "textscreen.h"
#include "doomtype.h"
#include "d_mode.h"
#include "d_iwad.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_misc.h"
#include "display.h"
#include "multiplayer.h"
#include "mode.h"
#include "wii-doom.h"


#define DEFAULT_MISSION (&mission_configs[0])


typedef struct
{
    char *label;
    GameMission_t mission;
    int mask;
    char *name;
    char *config_file;
    char *extra_config_file;
    char *executable;
} mission_config_t;


GameMission_t gamemission;

static GameSelectCallback game_selected_callback;

// Miscellaneous variables that aren't used in setup.
static char *executable = NULL;
static char *game_title = "Doom";

static const iwad_t **iwads;


// Default mission to fall back on, if no IWADs are found at all:
static mission_config_t mission_configs[] =
{
    {
        "Doom",
        doom,
        IWAD_MASK_DOOM,
        "doom",
        "default.cfg",
        PROGRAM_PREFIX "doom.cfg",
        PROGRAM_PREFIX "doom"
    }
};

// FIXME
static void BindMiscVariables(void)
{
    if (gamemission == doom)
    {
        M_BindBooleanVariable("detail",   &detailLevel);
        M_BindBooleanVariable("show_messages", &showMessages);
    }

    M_BindIntVariable("screensize",   &screenblocks);
}

//
// Initialise all configuration file bindings.
//
void InitBindings(void)
{
    // Keyboard, mouse, joystick controls

    M_BindBaseControls();

    BindDisplayVariables();
    BindMiscVariables();
    BindMultiplayerVariables();
}

// Set the name of the executable program to run the game:
static void SetExecutable(mission_config_t *config)
{
    char *extension;

    free(executable);

    extension = "";

    executable = M_StringJoin(config->executable, extension, NULL);
}

static void SetMission(mission_config_t *config)
{
    iwads = D_FindAllIWADs(config->mask);
    gamemission = config->mission;
    SetExecutable(config);
    game_title = config->label;
    M_SetConfigFilenames(config->config_file);
}

// Check the name of the executable.  If it contains one of the game
// names (eg. chocolate-hexen-setup.exe) then use that game.
static dboolean CheckExecutableName(GameSelectCallback callback)
{
    char *exe_name;
    int i;

    exe_name = M_GetExecutableName();

    for (i = 0; i < arrlen(mission_configs); ++i)
    {
        mission_config_t *config = &mission_configs[i];

        if (strstr(exe_name, config->name) != NULL)
        {
            SetMission(config);
            callback();
            return true;
        }
    }

    return false;
}

static void GameSelected(TXT_UNCAST_ARG(widget), TXT_UNCAST_ARG(config))
{
    TXT_CAST_ARG(mission_config_t, config);

    SetMission(config);
    game_selected_callback();
}

static void OpenGameSelectDialog(GameSelectCallback callback)
{
    mission_config_t *mission = NULL;
    txt_window_t *window;
    int num_games;
    int i;

    window = TXT_NewWindow("Select game");

    TXT_AddWidget(window, TXT_NewLabel("Select a game to configure:\n"));
    num_games = 0;

    // Add a button for each game.
    for (i = 0; i < arrlen(mission_configs); ++i)
    {
        // Do we have any IWADs for this game installed?
        // If so, add a button.
        const iwad_t **iwads = D_FindAllIWADs(mission_configs[i].mask);

        if (iwads[0] != NULL)
        {
            mission = &mission_configs[i];

            TXT_AddWidget(window, TXT_NewButton2(mission_configs[i].label,
                                                 GameSelected,
                                                 &mission_configs[i]));

            ++num_games;
        }

        free(iwads);
    }

    TXT_AddWidget(window, TXT_NewStrut(0, 1));

    // No IWADs found at all?  Fall back to doom, then.
    if (num_games == 0)
    {
        TXT_CloseWindow(window);
        SetMission(DEFAULT_MISSION);
        callback();
        return;
    }

    // Only one game? Use that game, and don't bother with a dialog.
    if (num_games == 1)
    {
        TXT_CloseWindow(window);
        SetMission(mission);
        callback();
        return;
    }

    game_selected_callback = callback;
}

void SetupMission(GameSelectCallback callback)
{
    if (!CheckExecutableName(callback))
    {
        OpenGameSelectDialog(callback);
    }
}

char *GetExecutableName(void)
{
    return executable;
}

char *GetGameTitle(void)
{
    return game_title;
}

const iwad_t **GetIwads(void)
{
    return iwads;
}

