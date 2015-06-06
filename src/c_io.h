/*
========================================================================

                               DOOM RETRO
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright (C) 2013-2015 Brad Harding.

  DOOM RETRO is a fork of CHOCOLATE DOOM by Simon Howard.
  For a complete list of credits, see the accompanying AUTHORS file.

  This file is part of DOOM RETRO.

  DOOM RETRO is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM RETRO is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM RETRO. If not, see <http://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM RETRO is in no way affiliated with nor endorsed by
  id Software LLC.

========================================================================
*/

#if !defined(__C_CONSOLE__)
#define __C_CONSOLE__

#include "doomstat.h"
#include "doomtype.h"
#include "d_event.h"

#define CONSOLEHEIGHT   ((SCREENHEIGHT - SBARHEIGHT) / 2)

#define CONSOLETOP      0

#define DIVIDERSTRING   "==========================================================================="

typedef enum
{
    yellow,
    red,
    gray,
    blue,
    white,
    green,
    dark,
    STRINGTYPES
} stringtype_t;


#define CR_GOLD yellow
#define CR_RED red
#define CR_GRAY gray
#define CR_BLUE blue
#define CR_WHITE white
#define CR_GREEN green
#define CR_DARK dark

typedef struct
{
    char                *string;
    stringtype_t        type;
    int                 tabs[8];
} console_t;

console_t       *console;

extern boolean  consoleactive;
extern int      consoleheight;
extern int      consoledirection;

extern char     consolecheat[255];
extern char     consolecheatparm[3];
extern char     consolecmdparm[255];

#if defined(WIN32)
extern boolean  showmemory;
#endif

void C_Printf(stringtype_t type, char *string, ...);
void C_Input(char *string, ...);
void C_Output(char *string, ...);
void C_TabbedOutput(int tabs[8], char *string, ...);
void C_Warning(char *string, ...);
void C_PlayerMessage(char *string, ...);
void C_AddConsoleDivider(void);
void C_Init(void);
void C_HideConsole(void);
void C_HideConsoleFast(void);
void C_Drawer(void);
boolean C_Responder(event_t *ev);
void C_PrintCompileDate(void);
void C_PrintSDLVersions(void);

#endif
