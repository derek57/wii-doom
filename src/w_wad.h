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
//        WAD I/O functions.
//


#ifndef __W_WAD__
#define __W_WAD__


#include <stdio.h>

#include "doom/doomdef.h"
#include "doomtype.h"
#include "w_file.h"


#define IWAD 1
#define PWAD 2
#define DEH  3


//
// TYPES
//

//
// WADFILE I/O related stuff.
//

typedef struct lumpinfo_s lumpinfo_t;

typedef int               lumpindex_t;

struct lumpinfo_s
{
    char         name[8];
    wad_file_t   *wad_file;
    int          position;
    int          size;
    void         *cache;
    unsigned int locks;

#ifdef TIMEDIAG
    int          locktic;
#endif

    // Used for hash table lookups
    lumpindex_t  next;
};

typedef enum
{
    disk_off,
    disk_on,
    disk_dirty

} disk_indicator_e;


wad_file_t  *W_AddFile(char *filename, dboolean automatic);

void        *W_CacheLumpNum(lumpindex_t lump, int tag);
void        *W_CacheLumpName(char *name, int tag);
void        W_GenerateHashTable(void);
void        W_ReleaseLumpNum(lumpindex_t lump);
void        W_ReleaseLumpName(char *name);
void        W_CheckCorrectIWAD(GameMission_t mission);
void        W_CheckSize(int wad);
void        W_ReadLump(lumpindex_t lump, void *dest);
void        W_Reload(void);
void        W_PrintLump(FILE *fp, void *p);

int         W_GetSecondNumForName(char *name);
int         W_LumpLength(lumpindex_t lump);
int         W_CheckMultipleLumps(char *name);
int         W_WadType(char *filename);

dboolean    HasDehackedLump(const char *pwadname);

lumpindex_t W_CheckNumForName(char *name);
lumpindex_t W_GetNumForName(char *name);
lumpindex_t W_GetNumForName2(char *name);
lumpindex_t W_RangeCheckNumForName(lumpindex_t min, lumpindex_t max, char *name);


extern unsigned int W_LumpNameHash(const char *s);


extern disk_indicator_e disk_indicator;

extern lumpinfo_t       **lumpinfo;

extern unsigned int     numlumps;


#endif

