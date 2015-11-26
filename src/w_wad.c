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
//        Handles WAD file header, directory, lump I/O.
//


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_io.h"

#ifdef WII
#include "../wii/config.h"
#else
#include "config.h"
#endif

#include "doom/doomdef.h"

#include "doomfeatures.h"
#include "doomtype.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_video.h"
#include "m_misc.h"
#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"


typedef struct
{
    // Should be "IWAD" or "PWAD".
    char                identification[4];                
    int                 numlumps;
    int                 infotableofs;
} PACKEDATTR wadinfo_t;


typedef struct
{
    int                 filepos;
    int                 size;
    char                name[8];
} PACKEDATTR filelump_t;

//
// GLOBALS
//

// Location of each lump on disk.

disk_indicator_e  disk_indicator = disk_off;

int               fsizecq = 0;

unsigned int      numlumps = 0;

lumpinfo_t        **lumpinfo;                

// Hash table for fast lookups

static lumpindex_t *lumphash;

extern int diskicon_readbytes;

// Hash function used for lump names.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

unsigned int W_LumpNameHash(const char *s)
{
    // This is the djb2 string hash function, modded to work on strings
    // that have a maximum length of 8.

    unsigned int result = 5381;
    unsigned int i;

    for (i=0; i < 8 && s[i] != '\0'; ++i)
    {
        result = ((result << 5) ^ result ) ^ toupper(s[i]);
    }

    return result;
}

//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.

wad_file_t *W_AddFile (char *filename, dboolean automatic)
{
    wadinfo_t header;
    lumpindex_t i;
    wad_file_t *wad_file;
    int startlump;
    filelump_t *fileinfo;
    filelump_t *filerover;
    lumpinfo_t *filelumps;
    int numfilelumps;

    // open the file and add to directory

    wad_file = W_OpenFile(filename);

    if (wad_file == NULL)
    {
        printf (" couldn't open %s\n", filename);
        C_Warning(" couldn't open %s", filename);
        return NULL;
    }

//    M_StringCopy(wad_file->path, filename, sizeof(wad_file->path));

    // [crispy] save the file name
    wad_file->path = M_BaseName(filename);

    if (!M_StringCompare(filename+strlen(filename)-3 , "wad" ) )
    {
        // single lump file

        // fraggle: Swap the filepos and size here.  The WAD directory
        // parsing code expects a little-endian directory, so will swap
        // them back.  Effectively we're constructing a "fake WAD directory"
        // here, as it would appear on disk.

        fileinfo = Z_Malloc(sizeof(filelump_t), PU_STATIC, 0);
        fileinfo->filepos = LONG(0);
        fileinfo->size = LONG(wad_file->length);

        // Name the lump after the base of the filename (without the
        // extension).

        M_ExtractFileBase (filename, fileinfo->name);
        numfilelumps = 1;
    }
    else
    {
        int length;

        // WAD file
        W_Read(wad_file, 0, &header, sizeof(header));

        if (strncmp(header.identification,"IWAD",4))
        {
            // Homebrew levels?
            if (strncmp(header.identification,"PWAD",4))
            {
                I_Error ("Wad file %s doesn't have IWAD "
                         "or PWAD id\n", filename);
            }
            
            // ???modifiedgame = true;                
        }

        wad_file->type = (!strncmp(header.identification, "IWAD", 4) ? IWAD : PWAD);

        header.numlumps = LONG(header.numlumps);
        header.infotableofs = LONG(header.infotableofs);
        length = header.numlumps*sizeof(filelump_t);
        fileinfo = Z_Malloc(length, PU_STATIC, 0);

        W_Read(wad_file, header.infotableofs, fileinfo, length);
        numfilelumps = header.numlumps;
    }

    // Increase size of numlumps array to accomodate the new file.
    filelumps = calloc(numfilelumps, sizeof(lumpinfo_t));
    if (filelumps == NULL)
    {
        I_Error("Failed to allocate array for lumps from new file.");
    }

    startlump = numlumps;
    numlumps += numfilelumps;
#ifdef BOOM_ZONE_HANDLING
    lumpinfo = Z_Realloc(lumpinfo, numlumps * sizeof(lumpinfo_t *), PU_STATIC, NULL);
#else
    lumpinfo = Z_Realloc(lumpinfo, numlumps * sizeof(lumpinfo_t *));
#endif
    if (lumpinfo == NULL)
    {
        I_Error("Failed to increase lumpinfo[] array size.");
    }

    filerover = fileinfo;

    for (i=startlump; i<numlumps; ++i)
    {
        lumpinfo_t *lump_p = &filelumps[i - startlump];
        lump_p->wad_file = wad_file;
        lump_p->position = LONG(filerover->filepos);
        lump_p->size = LONG(filerover->size);
        lump_p->cache = NULL;
        strncpy(lump_p->name, filerover->name, 8);
        lumpinfo[i] = lump_p;

        ++filerover;
    }

    Z_Free(fileinfo);

    if (lumphash != NULL)
    {
        Z_Free(lumphash);
        lumphash = NULL;
    }
/*
    C_Output(" %s %s lumps from %.4s file %s",
            (automatic ? "Automatically added" : "Added"),
            commify(numlumps - startlump),
            header.identification, uppercase(filename));
*/
    if (!strncmp(header.identification, "IWAD", 4) || !strncmp(header.identification, "PWAD", 4))
        C_Print(graystring,
            " %s %s lump%s from %.4s file %s.", (automatic ? "Automatically added" : "Added"),
                    commify(numlumps - startlump), (numlumps - startlump == 1 ? "" : "s"),
                            header.identification, uppercase(filename));

    return wad_file;
}



//
// W_CheckNumForName
// Returns -1 if name not found.
//

lumpindex_t W_CheckNumForName (char* name)
{
    lumpindex_t i;

    // Do we have a hash table yet?

    if (lumphash != NULL)
    {
        int hash;
        
        // We do! Excellent.

        hash = W_LumpNameHash(name) % numlumps;
        
        for (i = lumphash[hash]; i != -1; i = lumpinfo[i]->next)
        {
            if (!strncasecmp(lumpinfo[i]->name, name, 8))
            {
                return i;
            }
        }
    } 
    else
    {
        // We don't have a hash table generate yet. Linear search :-(
        // 
        // scan backwards so patch lump files take precedence

        for (i=numlumps-1; i >= 0; --i)
        {
            if (!strncasecmp(lumpinfo[i]->name, name, 8))
            {
                return i;
            }
        }
    }

    // TFB. Not found.

    return -1;
}




//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
lumpindex_t W_GetNumForName (char* name)
{
    lumpindex_t i;

    i = W_CheckNumForName (name);

    // This is a temporary fix for HACX
    if (gamemission == pack_hacx && name[0] == '$' && name[1] == 'M' && name[2] == 'U'
            && name[3] == 'S' && name[4] == 'I' && name[5] == 'C' && name[6] == '_'
            && name[7] == 'R' && name[8] == 'E' && name[9] == 'A' && name[10] == 'D'
            && name[11] == '_' && name[12] == 'M')
        return 0;

    if (i < 0)
    {
        I_Error ("W_GetNumForName: %s not found!", name);
    }
 
    return i;
}

//
// [nitr8] UNUSED
//
/*
int W_GetSecondNumForName (char* name)
{
    int        i, j;

    i = W_GetNumForName (name);

    for (j = i - 1; j >= 0; j--)
    {
        if (!strncasecmp(lumpinfo[j]->name, name, 8))
        {
            return j;
        }
    }

    return i;
}
*/

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength (lumpindex_t lump)
{
    if (lump >= numlumps)
    {
        I_Error ("W_LumpLength: %i >= numlumps", lump);
    }

    return lumpinfo[lump]->size;
}



//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(lumpindex_t lump, void *dest)
{
    int c;
    lumpinfo_t *l;

    if (lump >= numlumps)
    {
        I_Error ("W_ReadLump: %i >= numlumps", lump);
    }

    l = lumpinfo[lump];

    diskicon_readbytes += l->size;
    disk_indicator = disk_on;

    c = W_Read(l->wad_file, l->position, dest, l->size);

    if (c < l->size)
    {
        I_Error ("W_ReadLump: only read %i of %i on lump %i",
                 c, l->size, lump);        
    }
}




//
// W_CacheLumpNum
//
// Load a lump into memory and return a pointer to a buffer containing
// the lump data.
//
// 'tag' is the type of zone memory buffer to allocate for the lump
// (usually PU_STATIC or PU_CACHE).  If the lump is loaded as 
// PU_STATIC, it should be released back using W_ReleaseLumpNum
// when no longer needed (do not use Z_ChangeTag).
//

void *W_CacheLumpNum(lumpindex_t lumpnum, int tag)
{
    byte *result;
    lumpinfo_t *lump;

    if ((unsigned)lumpnum >= numlumps)
    {
        I_Error ("W_CacheLumpNum: %i >= numlumps", lumpnum);
    }

    lump = lumpinfo[lumpnum];

    // Get the pointer to return.  If the lump is in a memory-mapped
    // file, we can just return a pointer to within the memory-mapped
    // region.  If the lump is in an ordinary file, we may already
    // have it cached; otherwise, load it into memory.

    if (lump->wad_file->mapped != NULL)
    {
        // Memory mapped file, return from the mmapped region.

        result = lump->wad_file->mapped + lump->position;
    }
    else if (lump->cache != NULL)
    {
        // Already cached, so just switch the zone tag.

        result = lump->cache;
        Z_ChangeTag(lump->cache, tag);
    }
    else
    {
        // Not yet loaded, so load it now

        lump->cache = Z_Malloc(W_LumpLength(lumpnum), tag, &lump->cache);
        W_ReadLump (lumpnum, lump->cache);
        result = lump->cache;
    }
        
    return result;
}



//
// W_CacheLumpName
//
void *W_CacheLumpName(char *name, int tag)
{
    return W_CacheLumpNum(W_GetNumForName(name), tag);
}

// 
// Release a lump back to the cache, so that it can be reused later 
// without having to read from disk again, or alternatively, discarded
// if we run out of memory.
//
// Back in Vanilla Doom, this was just done using Z_ChangeTag 
// directly, but now that we have WAD mmap, things are a bit more
// complicated ...
//

void W_ReleaseLumpNum(lumpindex_t lumpnum)
{
    lumpinfo_t *lump;

    if ((unsigned)lumpnum >= numlumps)
    {
        I_Error ("W_ReleaseLumpNum: %i >= numlumps", lumpnum);
    }

    lump = lumpinfo[lumpnum];

    if (lump->wad_file->mapped != NULL)
    {
        // Memory-mapped file, so nothing needs to be done here.
    }
    else
    {
        Z_ChangeTag(lump->cache, PU_CACHE);
    }
}

void W_ReleaseLumpName(char *name)
{
    W_ReleaseLumpNum(W_GetNumForName(name));
}

#if 0

//
// W_Profile
//
int                info[2500][10];
int                profilecount;

void W_Profile (void)
{
    int                i;
    memblock_t*        block;
    void*        ptr;
    char        ch;
    FILE*        f;
    int                j;
    char        name[9];
        
        
    for (i=0 ; i<numlumps ; i++)
    {        
        ptr = lumpinfo[i].cache;
        if (!ptr)
        {
            ch = ' ';
            continue;
        }
        else
        {
            block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
            if (block->tag < PU_PURGELEVEL)
                ch = 'S';
            else
                ch = 'P';
        }
        info[i][profilecount] = ch;
    }
    profilecount++;
        
    f = fopen ("waddump.txt","w");
    name[8] = 0;

    for (i=0 ; i<numlumps ; i++)
    {
        memcpy (name,lumpinfo[i].name,8);

        for (j=0 ; j<8 ; j++)
            if (!name[j])
                break;

        for ( ; j<8 ; j++)
            name[j] = ' ';

        fprintf (f,"%s ",name);

        for (j=0 ; j<profilecount ; j++)
            fprintf (f,"    %c",info[i][j]);

        fprintf (f,"\n");
    }
    fclose (f);
}


#endif

// Generate a hash table for fast lookups

void W_GenerateHashTable(void)
{
    // Free the old hash table, if there is one

    if (lumphash != NULL)
    {
        Z_Free(lumphash);
    }

    // Generate hash table
    if (numlumps > 0)
    {
        lumpindex_t i;

        lumphash = Z_Malloc(sizeof(lumpindex_t) * numlumps, PU_STATIC, NULL);

        for (i = 0; i < numlumps; ++i)
        {
            lumphash[i] = -1;
        }

        for (i = 0; i < numlumps; ++i)
        {
            unsigned int hash;

            hash = W_LumpNameHash(lumpinfo[i]->name) % numlumps;

            // Hook into the hash table

            lumpinfo[i]->next = lumphash[hash];
            lumphash[hash] = i;
        }
    }

    // All done!
}

void W_CheckSize(int wad)
{
    FILE *fprw = NULL;

    if(wad == 0)
    {
#ifdef WII
        if(usb)
            fprw = fopen("usb:/apps/wiidoom/pspdoom.wad", "r");
        else if(sd)
            fprw = fopen("sd:/apps/wiidoom/pspdoom.wad", "r");
#else
        fprw = fopen("pspdoom.wad", "r");
#endif
        if (fprw == NULL)
            printf(" ");
        else
        {
            fseek(fprw, 0, 2);                // file pointer at the end of file
            fsizerw = ftell(fprw);        // take a position of file pointer un size variable

            if(fsizerw != 1138380)
                print_resource_pwad_error = true;

            fclose(fprw);
        }
    }
    else if(wad == 1)
    {
#ifdef WII
        if(usb)
            fprw = fopen("usb:/apps/wiidoom/pspchex.wad", "r");
        else if(sd)
            fprw = fopen("sd:/apps/wiidoom/pspchex.wad", "r");
#else
        fprw = fopen("pspchex.wad", "r");
#endif

        if (fprw == NULL)
            printf(" ");
        else
        {
            fseek(fprw, 0, 2);                // file pointer at the end of file
            fsizerw = ftell(fprw);        // take a position of file pointer un size variable

            if(fsizerw != 29995)
                print_resource_pwad_error = true;

            fclose(fprw);
        }
    }
    else if(wad == 2)
    {
#ifdef WII
        if(usb)
            fprw = fopen("usb:/apps/wiidoom/psphacx.wad", "r");
        else if(sd)
            fprw = fopen("sd:/apps/wiidoom/psphacx.wad", "r");
#else
        fprw = fopen("psphacx.wad", "r");
#endif

        if (fprw == NULL)
            printf(" ");
        else
        {
            fseek(fprw, 0, 2);                // file pointer at the end of file
            fsizerw = ftell(fprw);        // take a position of file pointer un size variable

            if(fsizerw != 42617)
                print_resource_pwad_error = true;

            fclose(fprw);
        }
    }
    else if(wad == 3)
    {
#ifdef WII
        if(usb)
            fprw = fopen("usb:/apps/wiidoom/pspfreedoom.wad", "r");
        else if(sd)
            fprw = fopen("sd:/apps/wiidoom/pspfreedoom.wad", "r");
#else
        fprw = fopen("pspfreedoom.wad", "r");
#endif

        if (fprw == NULL)
            printf(" ");
        else
        {
            fseek(fprw, 0, 2);            // file pointer at the end of file
            fsizerw = ftell(fprw);        // take a position of file pointer un size variable

            if(fsizerw != 42278)
                print_resource_pwad_error = true;

            fclose(fprw);
        }
    }
    else if(wad == 4)
    {
        if(extra_wad_slot_1_loaded == 1)
            fprw = fopen(extra_wad_1, "r");
        else if(extra_wad_slot_2_loaded == 1)
            fprw = fopen(extra_wad_2, "r");
        else if(extra_wad_slot_3_loaded == 1)
            fprw = fopen(extra_wad_3, "r");

        if (fprw == NULL)
            printf(" ");
        else
        {
            fseek(fprw, 0, 2);            // file pointer at the end of file
            fsizecq = ftell(fprw);        // take a position of file pointer un size variable

            fclose(fprw);
        }
    }
    else if(wad == 5)
    {
#ifdef WII
        if(usb)
            fprw = fopen("usb:/apps/wiidoom/doom1extras.wad", "r");
        else if(sd)
            fprw = fopen("sd:/apps/wiidoom/doom1extras.wad", "r");
#else
        fprw = fopen("doom1extras.wad", "r");
#endif
        if (fprw == NULL)
            printf(" ");
        else
        {
            fseek(fprw, 0, 2);             // file pointer at the end of file
            fsizerw2 = ftell(fprw);        // take a position of file pointer un size variable
            fclose(fprw);
        }
    }
}

//
// W_CheckMultipleLumps
// Check if there's more than one of the same lump.
//
int W_CheckMultipleLumps(char *name)
{
    int         i;
    int         count = 0;

    if (fsize == 28422764 || fsize == 19321722)
        return 3;

    for (i = numlumps - 1; i >= 0; --i)
        if (!strncasecmp(lumpinfo[i]->name, name, 8))
            ++count;

    return count;
}

dboolean HasDehackedLump(const char *pwadname)
{
    FILE        *fp = fopen(pwadname, "rb");
    filelump_t  lump;
    wadinfo_t   header;
    const char  *n = lump.name;
    int         result = false;

    if (!fp)
        return false;

    // read IWAD header
    if (fread(&header, 1, sizeof(header), fp) == sizeof(header))
    {
        fseek(fp, LONG(header.infotableofs), SEEK_SET);

        // Determine game mode from levels present
        // Must be a full set for whichever mode is present
        for (header.numlumps = LONG(header.numlumps);
            header.numlumps && fread(&lump, sizeof(lump), 1, fp); header.numlumps--)
        {
            if (*n == 'D' && n[1] == 'E' && n[2] == 'H' && n[3] == 'A' &&
                n[4] == 'C' && n[5] == 'K' && n[6] == 'E' && n[7] == 'D')
            {
                result = true;
                break;
            }
        }
    }

    fclose(fp);

    return result;
}

// Go forwards rather than backwards so we get lump from IWAD and not PWAD
lumpindex_t W_GetNumForName2(char *name)
{
    lumpindex_t i;

    for (i = 0; i < numlumps; i++)
        if (!strncasecmp(lumpinfo[i]->name, name, 8))
            break;

    if (i == numlumps)
        I_Error("W_GetNumForName2: %s not found!", name);

    return i;
}

//
// W_WadType
// Returns IWAD, PWAD or 0.
//
int W_WadType(char *filename)
{
    wadinfo_t   header;
    wad_file_t  *wad_file = W_OpenFile(filename);

    if (!wad_file)
        return 0;

    W_Read(wad_file, 0, &header, sizeof(header));

    W_CloseFile(wad_file);

    if (!strncmp(header.identification, "IWAD", 4))
        return IWAD;
    else if (!strncmp(header.identification, "PWAD", 4))
        return PWAD;
/*
    else if (!strncmp(header.identification, "Patch File for DeHackEd", 23))
        return DEH;
*/
    else
        return 0;
}

//
// W_RangeCheckNumForName
// Linear Search that checks for a lump number ONLY
// inside a range, not all lumps.
//
lumpindex_t W_RangeCheckNumForName(lumpindex_t min, lumpindex_t max, char *name)
{
    lumpindex_t i;

    for (i = min; i <= max; i++)
        if (!strncasecmp(lumpinfo[i]->name, name, 8))
            return i;

    return -1;
}

