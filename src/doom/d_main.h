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
//        System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __D_MAIN__
#define __D_MAIN__


#include "doomdef.h"
        

#define COLORIZE_CMD       "\033["
#define COLORIZE_NORMAL    "0"

#define TEXT_BLACK         "30m"
#define TEXT_RED           "31m"
#define TEXT_GREEN         "32m"
#define TEXT_YELLOW        "33m"
#define TEXT_BLUE          "34m"
#define TEXT_MAGENTA       "35m"
#define TEXT_CYAN          "36m"
#define TEXT_WHITE         "37m"

#define BACKGROUND_BLACK   "40"
#define BACKGROUND_RED     "41"
#define BACKGROUND_GREEN   "42"
#define BACKGROUND_YELLOW  "43"
#define BACKGROUND_BLUE    "44"
#define BACKGROUND_MAGENTA "45"
#define BACKGROUND_CYAN    "46"
#define BACKGROUND_WHITE   "47"


dboolean D_IsDehFile(char *filename);

//
// BASE LEVEL
//
void D_PageTicker(void);
void D_DoAdvanceDemo(void);
void D_StartTitle(void);
void D_DoomMain(void);
void D_AdvanceDemo(void);
void D_DoomLoop(void);

// Read events from all input devices
void D_ProcessEvents(void); 

void LoadDehFile(char *path);


//
// GLOBAL VARIABLES
//

extern gameaction_t gameaction;

#endif

