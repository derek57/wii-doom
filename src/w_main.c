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
//     Common code to parse command line, identifying WAD files to load.
//


#include "config.h"
#include "doomfeatures.h"
#include "d_iwad.h"

#include "doom/d_main.h"
#include "doom/doomstat.h"

#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_main.h"
#include "w_merge.h"
#include "w_wad.h"
#include "wii-doom.h"
#include "z_zone.h"


// Lump names that are unique to particular game types. This lets us check
// the user is not trying to play with the wrong executable, eg.
// chocolate-doom -iwad hexen.wad.
static const struct
{
    GameMission_t mission;
    char *lumpname;
} unique_lumps[] = {
    { doom,    "POSSA1" }
};


extern dboolean     version13;
extern dboolean     dont_show_adding_of_resource_wad;


// Parse the command line, merging WAD files that are sppecified.
// Returns true if at least one file was added.

#ifndef WII
dboolean W_ParseCommandLine(void)
{
    dboolean modifiedgame = false;
    int p = 0;

    //!
    // @arg <files>
    // @vanilla
    //
    // Load the specified PWAD files.
    //
    p = M_CheckParmWithArgs("-file", 1);

    if (p)
    {
        for (p = p + 1; p < myargc && myargv[p][0] != '-'; ++p)
        {
            char *filename = D_TryFindWADByName(myargv[p]);
            pwadfile = uppercase(removeext(M_ExtractFilename(filename)));

            if (W_MergeFile(filename, false))
            {
                modifiedgame = true;

                if (D_IsDehFile(filename))
                    LoadDehFile(filename);

                if (!dont_show_adding_of_resource_wad)
                    printf("         adding %s\n", filename);

                if ((search_string(filename, "nerve.wad") > -1) ||
                    (search_string(filename, "NERVE.WAD") > -1))
                {
                    int i;

                    nerve_pwad = true;
                    gamemission = pack_nerve;

                    // [crispy] rename level name patch lumps out of the way
                    for (i = 0; i < 9; i++)
                    {
                        char lumpname[9];

                        M_snprintf (lumpname, 9, "CWILV%2.2d", i);
                        lumpinfo[W_GetNumForName(lumpname)]->name[0] = 'N';
                    }
                }
            }
        }
    }

    return modifiedgame;
}

#endif

void W_CheckCorrectIWAD(GameMission_t mission)
{
    int i;
    lumpindex_t lumpnum;

    for (i = 0; i < arrlen(unique_lumps); ++i)
    {
        if (mission != unique_lumps[i].mission)
        {
            lumpnum = W_CheckNumForName(unique_lumps[i].lumpname);

            if (lumpnum >= 0)
            {
                I_Error("\nYou are trying to use a %s IWAD file with "
                        "the %s%s binary.\nThis isn't going to work.\n"
                        "You probably want to use the %s%s binary.",
                        D_SuggestGameName(unique_lumps[i].mission,
                                          indetermined),
                        PROGRAM_PREFIX,
                        D_GameMissionString(mission),
                        PROGRAM_PREFIX,
                        D_GameMissionString(unique_lumps[i].mission));
            }
        }
    }
}

