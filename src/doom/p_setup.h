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
//   Setup a game, startup stuff.
//
//-----------------------------------------------------------------------------


#ifndef __P_SETUP__
#define __P_SETUP__




// NOT called by W_Ticker. Fixme.
void
P_SetupLevel
( int                ep,
  int                map);

char *P_GetMapAuthor(int map);
int P_GetMapMusic(int map);
char *P_GetMapName(int map);
int P_GetMapNext(int map);
int P_GetMapPar(int map);
int P_GetMapSecretNext(int map);
int P_GetMapSky1Texture(int map);
int P_GetMapSky1ScrollDelta(int map);
int P_GetMapTitlePatch(int map); 

// Called by startup code.
void P_Init (void);

#endif
