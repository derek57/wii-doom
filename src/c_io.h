/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 *  02111-1307, USA.
 *
 *-----------------------------------------------------------------------------
 */


#ifndef __C_IO_H__
#define __C_IO_H__


#include "doomstat.h"
#include "d_event.h"
#include "v_video.h"


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


#define INPUTLENGTH   512
#define LINELENGTH    96
#define consoleactive current_height
#define c_moving      (current_height != current_target)

#define CR_GOLD yellow
#define CR_RED red
#define CR_GRAY gray
#define CR_BLUE blue
#define CR_WHITE white
#define CR_GREEN green
#define CR_DARK dark


void C_InitBackdrop(void);
void C_Init(void);
void C_Ticker(void);
void C_Drawer(void);
void C_Update(void);
void C_Puts(char *s);
void C_Printf(stringtype_t type, char *s, ...);
void C_Seperator(void);
void C_SetConsole(void);
void C_Popup(void);
void C_InstaPopup(void);
void C_PrintCompileDate(void);
void C_PrintSDLVersions(void);

int C_Responder(event_t* ev);

extern int     c_height;      // the height of the console
extern int     c_speed;       // pixels/tic it moves
extern int     current_height;
extern int     current_target;

extern boolean c_showprompt;

#endif
