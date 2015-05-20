//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
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
//    Configuration file interface.
//


#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomdef.h"
#include "doomfeatures.h"
#include "doomkeys.h"
#include "doomtype.h"
#include "i_system.h"
#include "m_misc.h"
#include "z_zone.h"


#define CONFIG_VARIABLE_GENERIC(name, type) \
    { #name, NULL, type, 0, 0, false }

#define CONFIG_VARIABLE_KEY(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_KEY)

#define CONFIG_VARIABLE_INT(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_INT)

#define CONFIG_VARIABLE_INT_HEX(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_INT_HEX)

#define CONFIG_VARIABLE_FLOAT(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_FLOAT)

#define CONFIG_VARIABLE_STRING(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_STRING)

extern boolean devparm;

extern int key_right;
extern int key_left;
extern int key_up;
extern int key_down;
extern int key_strafeleft;
extern int key_invright;
extern int key_useartifact;

//
// DEFAULTS
//

// Location where all configuration data is stored - 
// default.cfg, savegames, etc.

char *configdir;

// Default filenames for configuration files.

static char *default_main_config;

default_t        doom_defaults_list[] =
{
    CONFIG_VARIABLE_INT                (sfx_volume),
    CONFIG_VARIABLE_INT                (music_volume),
    CONFIG_VARIABLE_INT                (screensize),
    CONFIG_VARIABLE_INT                (walking_speed),
    CONFIG_VARIABLE_INT                (turning_speed),
    CONFIG_VARIABLE_INT                (strafing_speed),
    CONFIG_VARIABLE_INT                (freelook_speed),
    CONFIG_VARIABLE_INT                (use_gamma),
    CONFIG_VARIABLE_INT                (mouse_look),
    CONFIG_VARIABLE_INT                (map_grid),
    CONFIG_VARIABLE_INT                (follow_player),
    CONFIG_VARIABLE_INT                (showstats),
    CONFIG_VARIABLE_INT                (show_messages),
    CONFIG_VARIABLE_INT                (map_rotate),
    CONFIG_VARIABLE_INT                (detail),
    CONFIG_VARIABLE_INT                (vanilla_weapon_change),
    CONFIG_VARIABLE_INT                (xhair),
    CONFIG_VARIABLE_INT                (jump),
    CONFIG_VARIABLE_INT                (music_engine),
    CONFIG_VARIABLE_INT                (recoil),
    CONFIG_VARIABLE_INT                (monsters_respawn),
    CONFIG_VARIABLE_INT                (fast_monsters),
    CONFIG_VARIABLE_INT                (auto_aim),
    CONFIG_VARIABLE_INT                (max_gore),
    CONFIG_VARIABLE_INT                (extra_hud),
    CONFIG_VARIABLE_INT                (switch_chans),
    CONFIG_VARIABLE_INT                (player_thrust),
    CONFIG_VARIABLE_INT                (run_count),
    CONFIG_VARIABLE_INT                (footsteps),
    CONFIG_VARIABLE_INT                (footclip),
    CONFIG_VARIABLE_INT                (splash),
    CONFIG_VARIABLE_INT                (swirl),
    CONFIG_VARIABLE_INT                (classic_bfg),
    CONFIG_VARIABLE_INT                (pr_skulls),
    CONFIG_VARIABLE_INT                (key_shoot),
    CONFIG_VARIABLE_INT                (key_open),
    CONFIG_VARIABLE_INT                (key_menu),
    CONFIG_VARIABLE_INT                (key_weapon_left),
    CONFIG_VARIABLE_INT                (key_automap),
    CONFIG_VARIABLE_INT                (key_weapon_right),
    CONFIG_VARIABLE_INT                (key_automap_zoom_in),
    CONFIG_VARIABLE_INT                (key_automap_zoom_out),
    CONFIG_VARIABLE_INT                (key_flyup),
    CONFIG_VARIABLE_INT                (key_flydown),
    CONFIG_VARIABLE_INT                (key_jump),
    CONFIG_VARIABLE_INT                (key_run),
    CONFIG_VARIABLE_INT                (key_console),
    CONFIG_VARIABLE_INT                (key_aiminghelp),
};

default_collection_t doom_defaults =
{
    doom_defaults_list,
    arrlen(doom_defaults_list),
    NULL,
};

// Search a collection for a variable

static default_t *SearchCollection(default_collection_t *collection, char *name)
{
    int i;

    for (i=0; i<collection->numdefaults; ++i) 
    {
        if (!strcmp(name, collection->defaults[i].name))
        {
            return &collection->defaults[i];
        }
    }

    return NULL;
}

static void SaveDefaultCollection(default_collection_t *collection)
{
    default_t *defaults;
    int i, v;
    FILE *f;
        
    f = fopen (collection->filename, "w");
    if (!f)
        return; // can't write the file, but don't complain

    defaults = collection->defaults;
                
    for (i=0 ; i<collection->numdefaults ; i++)
    {
        int chars_written;

        // Ignore unbound variables

        if (!defaults[i].bound)
        {
            continue;
        }

        // Print the name and line up all values at 30 characters

        chars_written = fprintf(f, "%s ", defaults[i].name);

        for (; chars_written < 30; ++chars_written)
            fprintf(f, " ");

        // Print the value

        switch (defaults[i].type) 
        {
            case DEFAULT_KEY:

                // use the untranslated version if we can, to reduce
                // the possibility of screwing up the user's config
                // file
                
                v = * (int *) defaults[i].location;

                if (v == KEY_RSHIFT)
                {
                    // Special case: for shift, force scan code for
                    // right shift, as this is what Vanilla uses.
                    // This overrides the change check below, to fix
                    // configuration files made by old versions that
                    // mistakenly used the scan code for left shift.

                    v = 54;
                }
                else if (defaults[i].untranslated
                      && v == defaults[i].original_translated)
                {
                    // Has not been changed since the last time we
                    // read the config file.

                    v = defaults[i].untranslated;
                }
                fprintf(f, "%i", v);
                break;

            case DEFAULT_INT:
                fprintf(f, "%i", * (int *) defaults[i].location);
                break;

            case DEFAULT_INT_HEX:
                fprintf(f, "0x%x", * (int *) defaults[i].location);
                break;

            case DEFAULT_FLOAT:
                fprintf(f, "%f", * (float *) defaults[i].location);
                break;

            case DEFAULT_STRING:
                fprintf(f,"\"%s\"", * (char **) (defaults[i].location));
                break;
        }

        fprintf(f, "\n");
    }

    fclose (f);
}

// Parses integer values in the configuration file

static int ParseIntParameter(char *strparm)
{
    int parm;

    if (strparm[0] == '0' && strparm[1] == 'x')
        sscanf(strparm+2, "%x", &parm);
    else
        sscanf(strparm, "%i", &parm);

    return parm;
}

static void SetVariable(default_t *def, char *value)
{
    int intparm;

    // parameter found

    switch (def->type)
    {
        case DEFAULT_STRING:
            * (char **) def->location = M_StringDuplicate(value);
            break;

        case DEFAULT_INT:
        case DEFAULT_INT_HEX:
            * (int *) def->location = ParseIntParameter(value);
            break;

        case DEFAULT_KEY:

            // translate scancodes read from config
            // file (save the old value in untranslated)

            intparm = ParseIntParameter(value);
            def->untranslated = intparm;

            * (int *) def->location = intparm;

            break;

        case DEFAULT_FLOAT:
            * (float *) def->location = (float) atof(value);
            break;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

static void LoadDefaultCollection(default_collection_t *collection)
{
    FILE *f;
    default_t *def;
    char defname[80];
    char strparm[100];

    // read the file in, overriding any set defaults
    f = fopen(collection->filename, "r");

    if (f == NULL)
    {
        // File not opened, but don't complain. 
        // It's probably just the first time they ran the game.

        return;
    }

    while (!feof(f))
    {
        if (fscanf(f, "%79s %99[^\n]\n", defname, strparm) != 2)
        {
            // This line doesn't match

            continue;
        }

        // Find the setting in the list

        def = SearchCollection(collection, defname);

        if (def == NULL || !def->bound)
        {
            // Unknown variable?  Unbound variables are also treated
            // as unknown.

            continue;
        }

        // Strip off trailing non-printable characters (\r characters
        // from DOS text files)

        while (strlen(strparm) > 0 && !isprint(strparm[strlen(strparm)-1]))
        {
            strparm[strlen(strparm)-1] = '\0';
        }

        // Surrounded by quotes? If so, remove them.
        if (strlen(strparm) >= 2
         && strparm[0] == '"' && strparm[strlen(strparm) - 1] == '"')
        {
            strparm[strlen(strparm) - 1] = '\0';
            memmove(strparm, strparm + 1, sizeof(strparm) - 1);
        }

        SetVariable(def, strparm);
    }

    fclose (f);
}

// Set the default filenames to use for configuration files.

void M_SetConfigFilenames(char *main_config)
{
    default_main_config = main_config;
}

//
// M_SaveDefaults
//

void M_SaveDefaults (void)
{
    SaveDefaultCollection(&doom_defaults);
}

//
// Save defaults to alternate filenames
//

void M_SaveDefaultsAlternate(char *main)
{
    char *orig_main;

    // Temporarily change the filenames

    orig_main = doom_defaults.filename;

    doom_defaults.filename = main;

    M_SaveDefaults();

    // Restore normal filenames

    doom_defaults.filename = orig_main;
}

//
// M_LoadDefaults
//

void M_LoadDefaults (void)
{
    doom_defaults.filename
        = M_StringJoin(configdir, default_main_config, NULL);

    LoadDefaultCollection(&doom_defaults);
}

// Get a configuration file variable by its name

static default_t *GetDefaultForName(char *name)
{
    default_t *result;

    // Try the main list and the extras

    result = SearchCollection(&doom_defaults, name);

    // Not found? Internal error.

    if (result == NULL)
    {
        I_Error("Unknown configuration variable: '%s'", name);
    }

    return result;
}

//
// Bind a variable to a given configuration file variable, by name.
//

void M_BindVariable(char *name, void *location)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    variable->location = location;
    variable->bound = true;
}

// Set the value of a particular variable; an API function for other
// parts of the program to assign values to config variables by name.

boolean M_SetVariable(char *name, char *value)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound)
    {
        return false;
    }

    SetVariable(variable, value);

    return true;
}

// Get the value of a variable.

int M_GetIntVariable(char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || (variable->type != DEFAULT_INT && variable->type != DEFAULT_INT_HEX))
    {
        return 0;
    }

    return *((int *) variable->location);
}

const char *M_GetStrVariable(char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || variable->type != DEFAULT_STRING)
    {
        return NULL;
    }

    return *((const char **) variable->location);
}

float M_GetFloatVariable(char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || variable->type != DEFAULT_FLOAT)
    {
        return 0;
    }

    return *((float *) variable->location);
}

// Get the path to the default configuration dir to use, if NULL
// is passed to M_SetConfigDir.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

static char *GetDefaultConfigDir(void)
{
    // Configuration settings are stored in ~/.chocolate-doom/,
    // except on Windows, where we behave like Vanilla Doom and
    // save in the current directory.

    char *homedir;
    char *result;

    homedir = getenv("HOME");

    if (homedir != NULL)
    {
        // put all configuration in a config directory off the
        // homedir

        result = M_StringJoin(homedir, DIR_SEPARATOR_S,
                              "." "chocolate-doom", DIR_SEPARATOR_S, NULL);

        return result;
    }
    else
    {
        if(usb)
            return M_StringDuplicate("usb:/apps/wiidoom/");
        else if(sd)
            return M_StringDuplicate("sd:/apps/wiidoom/");
    }
}

// 
// SetConfigDir:
//
// Sets the location of the configuration directory, where configuration
// files are stored - default.cfg, chocolate-doom.cfg, savegames, etc.
//

void M_SetConfigDir(char *dir)
{
    // Use the directory that was passed, or find the default.

    if (dir != NULL)
    {
        configdir = dir;
    }
    else
    {
        configdir = GetDefaultConfigDir();
    }
    // Make the directory if it doesn't already exist:

    M_MakeDirectory(configdir);
}

//
// Calculate the path to the directory to use to store save games.
// Creates the directory as necessary.
//

char *M_GetSaveGameDir(char *iwadname)
{
    char *savegamedir = NULL;
    char *savegameroot;

    // If not "doing" a configuration directory (Windows), don't "do"
    // a savegame directory, either.

    if (!strcmp(configdir, ""))
    {
        savegamedir = M_StringDuplicate("");
    }
    else
    {
        // ~/.chocolate-doom/savegames/

        savegamedir = malloc(strlen(configdir) + 30);
        sprintf(savegamedir, "%ssavegames%c", configdir,
                             DIR_SEPARATOR);

        M_MakeDirectory(savegamedir);

        // eg. ~/.chocolate-doom/savegames/doom2.wad/

        sprintf(savegamedir + strlen(savegamedir), "%s%c",
                iwadname, DIR_SEPARATOR);

        if(usb)
        {
            savegameroot = SavePathRoot1USB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot2USB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot3USB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot4USB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot5USB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot6USB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot7USB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot8USB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootIWADUSB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootPWADUSB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootD1MusicUSB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootD2MusicUSB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootTNTMusicUSB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootChexMusicUSB;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootHacxMusicUSB;

            M_MakeDirectory(savegameroot);
        }
        else if(sd)
        {
            savegameroot = SavePathRoot1SD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot2SD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot3SD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot4SD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot5SD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot6SD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot7SD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRoot8SD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootIWADSD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootPWADSD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootD1MusicSD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootD2MusicSD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootTNTMusicSD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootChexMusicSD;

            M_MakeDirectory(savegameroot);

            savegameroot = SavePathRootHacxMusicSD;

            M_MakeDirectory(savegameroot);
        }

        if(usb)
        {
            if(fsize == 4261144)
                savegamedir = SavePathBeta14USB;
            else if(fsize == 4271324)
                savegamedir = SavePathBeta15USB;
            else if(fsize == 4211660)
                savegamedir = SavePathBeta16USB;
            else if(fsize == 4207819)
                savegamedir = SavePathShare10USB;
            else if(fsize == 4274218)
                savegamedir = SavePathShare11USB;
            else if(fsize == 4225504)
                savegamedir = SavePathShare12USB;
            else if(fsize == 4225460)
                savegamedir = SavePathShare125USB;
            else if(fsize == 4234124)
                savegamedir = SavePathShare1666USB;
            else if(fsize == 4196020)
                savegamedir = SavePathShare18USB;
            else if(fsize == 10396254)
                savegamedir = SavePathReg11USB;
            else if(fsize == 10399316)
                savegamedir = SavePathReg12USB;
            else if(fsize == 10401760)
                savegamedir = SavePathReg16USB;
            else if(fsize == 11159840)
                savegamedir = SavePathReg18USB;
            else if(fsize == 12408292)
                savegamedir = SavePathReg19USB;
            else if(fsize == 12474561)
                savegamedir = SavePathRegBFGXBOX360USB;
            else if(fsize == 12487824)
                savegamedir = SavePathRegBFGPCUSB;
            else if(fsize == 12538385)
                savegamedir = SavePathRegXBOXUSB;
            else if(fsize == 14943400)
                savegamedir = SavePath2Reg1666USB;
            else if(fsize == 14824716)
                savegamedir = SavePath2Reg1666GUSB;
            else if(fsize == 14612688)
                savegamedir = SavePath2Reg17USB;
            else if(fsize == 14607420)
                savegamedir = SavePath2Reg18FUSB;
            else if(fsize == 14604584)
                savegamedir = SavePath2Reg19USB;
            else if(fsize == 14677988)
                savegamedir = SavePath2RegBFGPSNUSB;
            else if(fsize == 14691821)
                savegamedir = SavePath2RegBFGPCUSB;
            else if(fsize == 14683458)
                savegamedir = SavePath2RegXBOXUSB;
            else if(fsize == 18195736)
                savegamedir = SavePathTNT191USB;
            else if(fsize == 18654796)
                savegamedir = SavePathTNT192USB;
            else if(fsize == 18240172)
                savegamedir = SavePathPLUT191USB;
            else if(fsize == 17420824)
                savegamedir = SavePathPLUT192USB;
            else if(fsize == 12361532)
                savegamedir = SavePathChexUSB;

//            else if(fsize == 9745831)
//                savegamedir = SavePathHacxShare10USB;
//            else if(fsize == 21951805)
//                savegamedir = SavePathHacxReg10USB;
//            else if(fsize == 22102300)
//                savegamedir = SavePathHacxReg11USB;

            else if(fsize == 19321722)
                savegamedir = SavePathHacxReg12USB;

//            else if(fsize == 19801320)
//                savegamedir = SavePathFreedoom064USB;
//            else if(fsize == 27704188)
//                savegamedir = SavePathFreedoom07RC1USB;
//            else if(fsize == 27625596)
//                savegamedir = SavePathFreedoom07USB;
//            else if(fsize == 28144744)
//                savegamedir = SavePathFreedoom08B1USB;
//            else if(fsize == 28592816)
//                savegamedir = SavePathFreedoom08USB;
//            else if(fsize == 19362644)
//                savegamedir = SavePathFreedoom08P1USB;

            else if(fsize == 28422764)
                savegamedir = SavePathFreedoom08P2USB;
        }
        else if(sd)
        {
            if(fsize == 4261144)
                savegamedir = SavePathBeta14SD;
            else if(fsize == 4271324)
                savegamedir = SavePathBeta15SD;
            else if(fsize == 4211660)
                savegamedir = SavePathBeta16SD;
            else if(fsize == 4207819)
                savegamedir = SavePathShare10SD;
            else if(fsize == 4274218)
                savegamedir = SavePathShare11SD;
            else if(fsize == 4225504)
                savegamedir = SavePathShare12SD;
            else if(fsize == 4225460)
                savegamedir = SavePathShare125SD;
            else if(fsize == 4234124)
                savegamedir = SavePathShare1666SD;
            else if(fsize == 4196020)
                savegamedir = SavePathShare18SD;
            else if(fsize == 10396254)
                savegamedir = SavePathReg11SD;
            else if(fsize == 10399316)
                savegamedir = SavePathReg12SD;
            else if(fsize == 10401760)
                savegamedir = SavePathReg16SD;
            else if(fsize == 11159840)
                savegamedir = SavePathReg18SD;
            else if(fsize == 12408292)
                savegamedir = SavePathReg19SD;
            else if(fsize == 12474561)
                savegamedir = SavePathRegBFGXBOX360SD;
            else if(fsize == 12487824)
                savegamedir = SavePathRegBFGPCSD;
            else if(fsize == 12538385)
                savegamedir = SavePathRegXBOXSD;
            else if(fsize == 14943400)
                savegamedir = SavePath2Reg1666SD;
            else if(fsize == 14824716)
                savegamedir = SavePath2Reg1666GSD;
            else if(fsize == 14612688)
                savegamedir = SavePath2Reg17SD;
            else if(fsize == 14607420)
                savegamedir = SavePath2Reg18FSD;
            else if(fsize == 14604584)
                savegamedir = SavePath2Reg19SD;
            else if(fsize == 14677988)
                savegamedir = SavePath2RegBFGPSNSD;
            else if(fsize == 14691821)
                savegamedir = SavePath2RegBFGPCSD;
            else if(fsize == 14683458)
                savegamedir = SavePath2RegXBOXSD;
            else if(fsize == 18195736)
                savegamedir = SavePathTNT191SD;
            else if(fsize == 18654796)
                savegamedir = SavePathTNT192SD;
            else if(fsize == 18240172)
                savegamedir = SavePathPLUT191SD;
            else if(fsize == 17420824)
                savegamedir = SavePathPLUT192SD;
            else if(fsize == 12361532)
                savegamedir = SavePathChexSD;

//            else if(fsize == 9745831)
//                savegamedir = SavePathHacxShare10SD;
//            else if(fsize == 21951805)
//                savegamedir = SavePathHacxReg10SD;
//            else if(fsize == 22102300)
//                savegamedir = SavePathHacxReg11SD;

            else if(fsize == 19321722)
                savegamedir = SavePathHacxReg12SD;

//            else if(fsize == 19801320)
//                savegamedir = SavePathFreedoom064SD;
//            else if(fsize == 27704188)
//                savegamedir = SavePathFreedoom07RC1SD;
//            else if(fsize == 27625596)
//                savegamedir = SavePathFreedoom07SD;
//            else if(fsize == 28144744)
//                savegamedir = SavePathFreedoom08B1SD;
//            else if(fsize == 28592816)
//                savegamedir = SavePathFreedoom08SD;
//            else if(fsize == 19362644)
//                savegamedir = SavePathFreedoom08P1SD;

            else if(fsize == 28422764)
                savegamedir = SavePathFreedoom08P2SD;
        }
        M_MakeDirectory(savegamedir);
    }

    return savegamedir;
}

