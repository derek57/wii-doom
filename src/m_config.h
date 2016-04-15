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
//      Configuration file interface.
//    


#ifndef __M_CONFIG__
#define __M_CONFIG__


#include "doom/doomdef.h"
#include "doomtype.h"


void M_LoadDefaults(void);
void M_SaveDefaults(void);
void M_SaveDefaultsAlternate(char *main, char *extra);
void M_SetConfigDir(char *dir);
void M_BindIntVariable(char *name, int *location);
void M_SetConfigFilenames(char *main_config);
void M_BindFloatVariable(char *name, float *variable);
void M_BindStringVariable(char *name, char **variable);
void M_BindBooleanVariable(char *name, dboolean *location);

dboolean M_SetVariable(char *name, char *value);
dboolean LoadDefaultCollection(default_collection_t *collection);

int M_GetIntVariable(char *name);

float M_GetFloatVariable(char *name);

const char *M_GetStrVariable(char *name);

char *M_GetSaveGameDir(char *iwadname);


extern char *configdir;


#endif

