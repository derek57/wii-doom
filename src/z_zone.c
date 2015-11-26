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
//        Zone Memory Allocation. Neat.
//


#include <malloc.h>

#include "c_io.h"
#include "doomfeatures.h"
#include "doomtype.h"
#include "i_system.h"

#include "m_argv.h"
#include "doom/m_menu.h"

#include "v_trans.h"
#include "z_zone.h"


static size_t       free_memory;
static size_t       active_memory;
static size_t       purgable_memory;
int                 memory_size;


#ifdef BOOM_ZONE_HANDLING

// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: z_zone.c,v 1.13 1998/05/12 06:11:55 killough Exp $
//
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      Zone Memory Allocation. Neat.
//
// Neat enough to be rewritten by Lee Killough...
//
// Must not have been real neat :)
//
// Made faster and more general, and added wrappers for all of Doom's
// memory allocation functions, including malloc() and similar functions.
// Added line and file numbers, in case of error. Added performance
// statistics and tunables.
//-----------------------------------------------------------------------------

#include "doom/doomstat.h"

// Uncomment this to see real-time memory allocation
// statistics, to and enable extra debugging features
#define INSTRUMENTED_MEMORY

// Uncomment this to detect source of error
#define INSTRUMENTED_CODE

// Uncomment this to exhaustively run memory checks
// while the game is running (this is EXTREMELY slow).
// Only useful if INSTRUMENTED is also defined.
#define CHECKHEAP

// Uncomment this to perform id checks on zone blocks,
// to detect corrupted and illegally freed blocks
#define ZONEIDCHECK


// Tunables


// Alignment of zone memory (benefit may be negated by HEADER_SIZE, CHUNK_SIZE)
#define CACHE_ALIGN     32

// size of block header (ought to be a value of 32)
#define HEADER_SIZE     64

// Minimum chunk size at which blocks are allocated
#define CHUNK_SIZE      32

// Minimum size a block must be to become part of a split
#define MIN_BLOCK_SPLIT (1024)

// How much RAM to leave aside for other libraries
#define LEAVE_ASIDE     (128 * 1024)

// Minimum RAM machine is assumed to have
#define MIN_RAM         (4 * 1024 * 1024)

// Amount to subtract when retrying failed attempts to allocate initial pool
#define RETRY_AMOUNT    (256 * 1024)

// signature for block header
#define ZONEID          0x931d4a11

// Number of mallocs & frees kept in history buffer (must be a power of 2)
#define ZONE_HISTORY    4

// End Tunables


typedef struct memblock {

#ifdef ZONEIDCHECK
    unsigned        id;
#endif

    struct memblock *next, *prev;

    size_t          size;

    void            **user;

    unsigned char   tag, vm;

#ifdef INSTRUMENTED_MEMORY
    unsigned short  extra;
#endif

#ifdef INSTRUMENTED_CODE
    char            *file;

    int             line;
#endif

} memblock_t;


static memblock_t   *rover;                // roving pointer to memory blocks
static memblock_t   *zone;                 // pointer to first block
static memblock_t   *zonebase;             // pointer to entire zone memory
static memblock_t   *blockbytag[PU_MAX];

// statistics for evaluating performance
static size_t       zonebase_size;         // zone memory allocated size
static size_t       inactive_memory;
static size_t       virtual_memory;


// Print allocation statistics
void Z_DrawStats(void)
{
    char act_mem[50];
    char pur_mem[50];
    char free_mem[50];
    char frag_mem[50];
    char virt_mem[50];
    char tot_mem[50];

    double s;

    unsigned long total_memory;

    if (gamestate != GS_LEVEL)
        return;

    total_memory = free_memory + active_memory +
                                 purgable_memory + inactive_memory +
                                 virtual_memory;

    s = 100.0 / total_memory;

    sprintf(act_mem, "%zu\t%6.01f%%\tstatic\n", active_memory, active_memory * s);
    sprintf(pur_mem, "%zu\t%6.01f%%\tpurgable\n", purgable_memory, purgable_memory * s);
    sprintf(free_mem, "%zu\t%6.01f%%\tfree\n", free_memory, free_memory * s);
    sprintf(frag_mem, "%zu\t%6.01f%%\tfragmentable\n", inactive_memory, inactive_memory * s);
    sprintf(virt_mem, "%zu\t%6.01f%%\tvirtual\n", virtual_memory, virtual_memory * s);
    sprintf(tot_mem, "%lu\t\ttotal\n", total_memory);

    if(leveltime & 16)
        M_WriteText(0, 10, "Memory Heap Info\n");

    if (memory_size > 0)
    {
        M_WriteText(0, 20, act_mem);
        M_WriteText(0, 30, pur_mem);
        M_WriteText(0, 40, free_mem);
        M_WriteText(0, 50, frag_mem);
        M_WriteText(0, 60, virt_mem);
        M_WriteText(0, 70, tot_mem);
    }
}

#ifdef INSTRUMENTED_CODE

// killough 4/26/98: Add history information

enum {

    malloc_history,
    free_history,

    NUM_HISTORY_TYPES

};

static char *file_history[NUM_HISTORY_TYPES][ZONE_HISTORY];
static int  line_history[NUM_HISTORY_TYPES][ZONE_HISTORY];
static int  history_index[NUM_HISTORY_TYPES];
/*
static char *desc[NUM_HISTORY_TYPES] = { "malloc()'s", "free()'s" };

//
// [nitr8] UNUSED
//
void Z_DumpHistory(char *buf)
{
    int i, j;
    char s[1024];

    strcat(buf, "\n");

    for (i = 0; i < NUM_HISTORY_TYPES; i++)
    {
        sprintf(s, "\nLast several %s:\n\n", desc[i]);
        strcat(buf, s);

        for (j = 0; j < ZONE_HISTORY; j++)
        {
            int k = (history_index[i] - j - 1) & (ZONE_HISTORY - 1);

            if (file_history[i][k])
            {
                sprintf(s, "File: %s, Line: %d\n", file_history[i][k], line_history[i][k]);
                strcat(buf, s);
            }
        }
    }
}
*/

#else

//
// [nitr8] UNUSED
//
/*
void Z_DumpHistory(char *buf)
{
}
*/

#endif

static void Z_Close(void)
{
    (free)(zonebase);
    zone = rover = zonebase = NULL;
}

void Z_Init(void)
{
    size_t size = MIN_RAM;

    memory_size = size;

    if (size < MIN_RAM)         // If less than MIN_RAM, assume MIN_RAM anyway
        size = MIN_RAM;

    size -= LEAVE_ASIDE;        // Leave aside some for other libraries

    assert(HEADER_SIZE >= sizeof(memblock_t) && MIN_RAM > LEAVE_ASIDE);

    atexit(Z_Close);            // exit handler

    size = (size + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1);  // round to chunk size

    // Allocate the memory

    while (!(zonebase = (malloc)(zonebase_size = size + HEADER_SIZE + CACHE_ALIGN)))
    {
        if (size < (MIN_RAM - LEAVE_ASIDE < RETRY_AMOUNT ? RETRY_AMOUNT : MIN_RAM - LEAVE_ASIDE))
            I_Error("Z_Init: failed on allocation of %lu bytes", (unsigned long) zonebase_size);
        else
            size -= RETRY_AMOUNT;
    }

    // Align on cache boundary

    zone = (memblock_t *)((char *) zonebase + CACHE_ALIGN - ((unsigned) zonebase & (CACHE_ALIGN - 1)));

    rover = zone;                            // Rover points to base of zone mem
    zone->next = zone->prev = zone;          // Single node
    zone->size = size;                       // All memory in one block
    zone->tag = PU_FREE;                     // A free block
    zone->vm  = 0;

#ifdef ZONEIDCHECK
    zone->id  = 0;
#endif

#ifdef INSTRUMENTED_MEMORY
    free_memory = size;
    inactive_memory = zonebase_size - size;
    active_memory = purgable_memory = 0;
#endif
}

// Z_Malloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.

void *(Z_Malloc)(size_t size, int32_t tag, void **user, char *file, int line)
{
    register memblock_t *block;

    memblock_t          *start;

#ifdef INSTRUMENTED_MEMORY
    size_t              size_orig = size;

#ifdef CHECKHEAP
    Z_CheckHeap();
#endif

#endif

#ifdef INSTRUMENTED_CODE
    file_history[malloc_history][history_index[malloc_history]] = file;
    line_history[malloc_history][history_index[malloc_history]++] = line;
    history_index[malloc_history] &= ZONE_HISTORY - 1;
#endif

#ifdef ZONEIDCHECK
    if (tag >= PU_PURGELEVEL && !user)
        I_Error ("Z_Malloc: an owner is required for purgable blocks\n" "Source: %s:%d", file, line);
#endif

    if (!size)
        return user ? *user = NULL : NULL;           // malloc(0) returns NULL

    size = (size + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1);  // round to chunk size

    block = rover;

    if (block->prev->tag == PU_FREE)
        block = block->prev;

    start = block;

    do
    {
        if (block->tag >= PU_PURGELEVEL)      // Free purgable blocks
        {                                   // replacement is roughly FIFO
            start = block->prev;
            Z_Free((char *) block + HEADER_SIZE);
            block = start = start->next;      // Important: resets start
        }

        if (block->tag == PU_FREE && block->size >= size)   // First-fit
        {
            size_t extra = block->size - size;

            if (extra >= MIN_BLOCK_SPLIT + HEADER_SIZE)
            {
                memblock_t *newb = (memblock_t *)((char *) block +
                                                HEADER_SIZE + size);

                (newb->next = block->next)->prev = newb;
                (newb->prev = block)->next = newb;          // Split up block
                block->size = size;
                newb->size = extra - HEADER_SIZE;
                newb->tag = PU_FREE;
                newb->vm = 0;

#ifdef INSTRUMENTED_MEMORY
                inactive_memory += HEADER_SIZE;
                free_memory -= HEADER_SIZE;
#endif
            }

            rover = block->next;           // set roving pointer for next search

#ifdef INSTRUMENTED_MEMORY
            inactive_memory += block->extra = block->size - size_orig;

            if (tag >= PU_PURGELEVEL)
                purgable_memory += size_orig;
            else
                active_memory += size_orig;

            free_memory -= block->size;
#endif

allocated:

#ifdef INSTRUMENTED_CODE
            block->file = file;
            block->line = line;
#endif

#ifdef ZONEIDCHECK
            block->id = ZONEID;         // signature required in block header
#endif
            block->tag = tag;           // tag
            block->user = user;         // user
            block = (memblock_t *)((char *) block + HEADER_SIZE);

            if (user)                   // if there is a user
                *user = block;          // set user to point to new block

#ifdef INSTRUMENTED_MEMORY
            // scramble memory -- weed out any bugs
            memset(block, gametic & 0xff, size);
#endif
            return block;
        }
    } while ((block = block->next) != start);   // detect cycles as failure

    // We've run out of physical memory, or so we think.
    // Although less efficient, we'll just use ordinary malloc.
    // This will squeeze the remaining juice out of this machine
    // and start cutting into virtual memory if it has it.

    while (!(block = (malloc)(size + HEADER_SIZE)))
    {
        if (!blockbytag[PU_CACHE])
            I_Error ("Z_Malloc: Failure trying to allocate %lu bytes"
                 "\nSource: %s:%d", (unsigned long) size, file, line);

        Z_FreeTags(PU_CACHE, PU_CACHE);
    }

    if ((block->next = blockbytag[tag]))
        block->next->prev = (memblock_t *) &block->next;

    blockbytag[tag] = block;
    block->prev = (memblock_t *) &blockbytag[tag];
    block->vm = 1;

#ifdef INSTRUMENTED_MEMORY
    virtual_memory += block->size = size + HEADER_SIZE;
#endif

    goto allocated;
}

void (Z_Free)(void *p, char *file, int line)
{
#ifdef INSTRUMENTED_MEMORY

#ifdef CHECKHEAP
    Z_CheckHeap();
#endif

#endif

#ifdef INSTRUMENTED_CODE
    file_history[free_history][history_index[free_history]] = file;
    line_history[free_history][history_index[free_history]++] = line;
    history_index[free_history] &= ZONE_HISTORY - 1;
#endif

    if (p)
    {
        memblock_t *other, *block = (memblock_t *)((char *) p - HEADER_SIZE);

#ifdef ZONEIDCHECK
        if (block->id != ZONEID)
            I_Error("Z_Free: freed a pointer without ZONEID\n" "Source: %s:%d"

#ifdef INSTRUMENTED_CODE
                "\nSource of malloc: %s:%d"
                , file, line, block->file, block->line
#else
                , file, line
#endif
                );

        block->id = 0;              // Nullify id so another free fails
#endif

#ifdef INSTRUMENTED_MEMORY
        // scramble memory -- weed out any bugs
        memset(p, gametic & 0xff, block->size - block->extra);
#endif

        if (block->user)            // Nullify user if one exists
            *block->user = NULL;

        if (block->vm)
        {
            if ((*(memblock_t **) block->prev = block->next))
                block->next->prev = block->prev;

#ifdef INSTRUMENTED_MEMORY
            virtual_memory -= block->size;
#endif
            (free)(block);
        }
        else
        {

#ifdef INSTRUMENTED_MEMORY
            free_memory += block->size;
            inactive_memory -= block->extra;

            if (block->tag >= PU_PURGELEVEL)
                purgable_memory -= block->size - block->extra;
            else
                active_memory -= block->size - block->extra;
#endif

            block->tag = PU_FREE;       // Mark block freed

            if (block != zone)
            {
                other = block->prev;        // Possibly merge with previous block

                if (other->tag == PU_FREE)
                {
                    if (rover == block)  // Move back rover if it points at block
                        rover = other;

                    (other->next = block->next)->prev = other;
                    other->size += block->size + HEADER_SIZE;
                    block = other;

#ifdef INSTRUMENTED_MEMORY
                    inactive_memory -= HEADER_SIZE;
                    free_memory += HEADER_SIZE;
#endif
                }
            }

            other = block->next;        // Possibly merge with next block

            if (other->tag == PU_FREE && other != zone)
            {
                if (rover == other) // Move back rover if it points at next block
                    rover = block;

                (block->next = other->next)->prev = block;
                block->size += other->size + HEADER_SIZE;

#ifdef INSTRUMENTED_MEMORY
                inactive_memory -= HEADER_SIZE;
                free_memory += HEADER_SIZE;
#endif
            }
        }
    }
}

void (Z_FreeTags)(int32_t lowtag, int32_t hightag, char *file, int line)
{
    memblock_t *block = zone;

    if (lowtag <= PU_FREE)
        lowtag = PU_FREE + 1;

    do               // Scan through list, searching for tags in range
    {
        if (block->tag >= lowtag && block->tag <= hightag)
        {
            memblock_t *prev = block->prev;
            (Z_Free)((char *) block + HEADER_SIZE, file, line);
            block = prev->next;
        }
    } while ((block = block->next) != zone);

    if (hightag > PU_CACHE)
        hightag = PU_CACHE;

    for ( ; lowtag <= hightag; lowtag++)
    {
        for (block = blockbytag[lowtag], blockbytag[lowtag] = NULL; block; )
        {
            memblock_t *next = block->next;

#ifdef ZONEIDCHECK
            if (block->id != ZONEID)
                I_Error("Z_Free: freed a pointer without ZONEID\n" "Source: %s:%d"

#ifdef INSTRUMENTED_CODE
                    "\nSource of malloc: %s:%d"
                    , file, line, block->file, block->line
#else
                    , file, line
#endif
                    );

            block->id = 0;              // Nullify id so another free fails
#endif

#ifdef INSTRUMENTED_MEMORY
            virtual_memory -= block->size;
#endif

            if (block->user)            // Nullify user if one exists
                *block->user = NULL;

            (free)(block);              // Free the block

            block = next;               // Advance to next block
        }
    }
}

void (Z_ChangeTag)(void *ptr, int32_t tag, char *file, int line)
{
    memblock_t *block = (memblock_t *)((char *) ptr - HEADER_SIZE);

#ifdef INSTRUMENTED_MEMORY

#ifdef CHECKHEAP
    Z_CheckHeap();
#endif

#endif

#ifdef ZONEIDCHECK
    if (block->id != ZONEID)
        I_Error ("Z_ChangeTag: freed a pointer without ZONEID" "\nSource: %s:%d"

#ifdef INSTRUMENTED_CODE
             "\nSource of malloc: %s:%d"
             , file, line, block->file, block->line
#else
             , file, line
#endif

             );

    if (tag >= PU_PURGELEVEL && !block->user)
        I_Error ("Z_ChangeTag: an owner is required for purgable blocks\n" "Source: %s:%d"

#ifdef INSTRUMENTED_CODE
             "\nSource of malloc: %s:%d"
             , file, line, block->file, block->line
#else
             , file, line
#endif

             );

#endif // ZONEIDCHECK

    if (block->vm)
    {
        if ((*(memblock_t **) block->prev = block->next))
            block->next->prev = block->prev;

        if ((block->next = blockbytag[tag]))
            block->next->prev = (memblock_t *) &block->next;

        block->prev = (memblock_t *) &blockbytag[tag];
        blockbytag[tag] = block;
    }
    else
    {

#ifdef INSTRUMENTED_MEMORY
        if (block->tag < PU_PURGELEVEL && tag >= PU_PURGELEVEL)
        {
            active_memory -= block->size - block->extra;
            purgable_memory += block->size - block->extra;
        }
        else if (block->tag >= PU_PURGELEVEL && tag < PU_PURGELEVEL)
        {
            active_memory += block->size - block->extra;
            purgable_memory -= block->size - block->extra;
        }
#endif

    }
    block->tag = tag;
}

//
// [nitr8] UNUSED
//
/*
void Z_ChangeUser(void *ptr, void **user)
{
    memblock_t* block;

    block = (memblock_t *) ((byte *)ptr - sizeof(memblock_t));

#ifdef ZONEIDCHECK
    if (block->id != ZONEID)
    {
        I_Error("Z_ChangeUser: Tried to change user for invalid block!");
    }
#endif

    block->user = user;
    *user = ptr;
}
*/

void *(Z_Realloc)(void *ptr, size_t n, int tag, void **user, char *file, int line)
{
    void *p = (Z_Malloc)(n, tag, user, file, line);

    if (ptr)
    {
        memblock_t *block = (memblock_t *)((char *) ptr - HEADER_SIZE);
        memcpy(p, ptr, n <= block->size ? n : block->size);
        (Z_Free)(ptr, file, line);

        if (user) // in case Z_Free nullified same user
            *user = p;
    }
    return p;
}

void *(Z_Calloc)(size_t n1, size_t n2, int tag, void **user, char *file, int line)
{
  return (n1 *= n2) ? memset((Z_Malloc)(n1, tag, user, file, line), 0, n1) : NULL;
}

void (Z_CheckHeap)(char *file, int line)
{
    memblock_t *block = zone;   // Start at base of zone mem

    do                          // Consistency check (last node treated special)
    {
        if ((block->next != zone &&
                (memblock_t *)((char *) block + HEADER_SIZE + block->size) != block->next) ||
                block->next->prev != block || block->prev->next != block)
            I_Error("Z_CheckHeap: Block size does not touch the next block\n" "Source: %s:%d"

#ifdef INSTRUMENTED_CODE
              "\nSource of offending block: %s:%d"
              , file, line, block->file, block->line
#else
              , file, line
#endif

              );

    } while ((block = block->next) != zone);
}
/*
void* Z_MallocAlign (int reqsize, int32_t tag, void **user, int alignbits)
{
    memblock_t* newblock;

    void* basedata;

    // with the memalloc header
    int         memalloc_size;
   
    // choose safe interpretation
    if (tag == PU_FREE)
    {
        tag = PU_LEVEL;
    }

    // alloc rounded up to next 4 byte alignment
    reqsize = (reqsize + 3) & ~3;

    // account for size of block header
    memalloc_size = reqsize + sizeof(memblock_t);

    newblock = malloc(memalloc_size);

    if( newblock == NULL )
    {
        I_Error ("Z_Malloc: malloc failed on allocation of %i bytes\n");
    }

    zonebase->size += memalloc_size;

#ifdef ZONEIDCHECK
    newblock->id = ZONEID;
#endif

    newblock->user = user;
    newblock->size = memalloc_size;

    basedata = (byte*)newblock + sizeof(memblock_t);

    if (user)
        *user = basedata;

    return basedata;
}
*/
//-----------------------------------------------------------------------------
//
// $Log: z_zone.c,v $
// Revision 1.13  1998/05/12  06:11:55  killough
// Improve memory-related error messages
//
// Revision 1.12  1998/05/03  22:37:45  killough
// beautification
//
// Revision 1.11  1998/04/27  01:49:39  killough
// Add history of malloc/free and scrambler (INSTRUMENTED only)
//
// Revision 1.10  1998/03/28  18:10:33  killough
// Add memory scrambler for debugging
//
// Revision 1.9  1998/03/23  03:43:56  killough
// Make Z_CheckHeap() more diagnostic
//
// Revision 1.8  1998/03/02  11:40:02  killough
// Put #ifdef CHECKHEAP around slow heap checks (debug)
//
// Revision 1.7  1998/02/02  13:27:45  killough
// Additional debug info turned on with #defines
//
// Revision 1.6  1998/01/26  19:25:15  phares
// First rev with no ^Ms
//
// Revision 1.4  1998/01/26  06:12:30  killough
// Fix memory usage problems and improve debug stat display
//
// Revision 1.3  1998/01/22  05:57:20  killough
// Allow use of virtual memory when physical memory runs out
//
// ???
//
//-----------------------------------------------------------------------------


#elif defined DOOMRETRO_ZONE_HANDLING

/*
========================================================================

                               DOOM Retro
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright (C) 2013-2015 Brad Harding.

  DOOM Retro is a fork of Chocolate DOOM by Simon Howard.
  For a complete list of credits, see the accompanying AUTHORS file.

  This file is part of DOOM Retro.

  DOOM Retro is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM Retro is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM Retro. If not, see <http://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM Retro is in no way affiliated with nor endorsed by
  id Software LLC.

========================================================================
*/

#include "i_system.h"
#include "z_zone.h"

// Minimum chunk size at which blocks are allocated
#define CHUNK_SIZE      32

typedef struct memblock
{
    struct memblock     *next;
    struct memblock     *prev;
    size_t              size;
    void                **user;
    unsigned char       tag;
} memblock_t;

// size of block header
// cph - base on sizeof(memblock_t), which can be larger than CHUNK_SIZE on
// 64bit architectures
static const size_t     HEADER_SIZE = (sizeof(memblock_t) + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1);

static memblock_t       *blockbytag[PU_MAX];

//
// Z_Malloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.
//
// cph - the algorithm here was a very simple first-fit round-robin
//  one - just keep looping around, freeing everything we can until
//  we get a large enough space
//
// This has been changed now; we still do the round-robin first-fit,
// but we only free the blocks we actually end up using; we don't
// free all the stuff we just pass on the way.
//
void *Z_Malloc(size_t size, int tag, void **user)
{
    memblock_t  *block = NULL;

    if (!size)
        return (user ? (*user = NULL) : NULL);          // malloc(0) returns NULL

    size = (size + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1); // round to chunk size

    while (!(block = malloc(size + HEADER_SIZE)))
    {
        if (!blockbytag[PU_CACHE])
            I_Error("Z_Malloc: Failure trying to allocate %lu bytes", (unsigned long)size);
        Z_FreeTags(PU_CACHE, PU_CACHE);
    }

    if (!blockbytag[tag])
    {
        blockbytag[tag] = block;
        block->next = block->prev = block;
    }
    else
    {
        blockbytag[tag]->prev->next = block;
        block->prev = blockbytag[tag]->prev;
        block->next = blockbytag[tag];
        blockbytag[tag]->prev = block;
    }

    block->size = size;

    block->tag = tag;                                   // tag
    block->user = user;                                 // user
    block = (memblock_t *)((char *)block + HEADER_SIZE);
    if (user)                                           // if there is a user
        *user = block;                                  // set user to point to new block

    return block;
}

void *Z_Realloc(void *ptr, size_t size)
{
    void        *newp = realloc(ptr, size);

    if (!newp)
        I_Error("Z_Realloc: Failure trying to reallocate %i bytes", size);
    else
        ptr = newp;

    return ptr;
}

void Z_Free(void *p)
{
    memblock_t  *block = (memblock_t *)((char *)p - HEADER_SIZE);

    if (!p)
        return;

    if (block->user)                                    // Nullify user if one exists
        *block->user = NULL;

    if (block == block->next)
        blockbytag[block->tag] = NULL;
    else if (blockbytag[block->tag] == block)
        blockbytag[block->tag] = block->next;
    block->prev->next = block->next;
    block->next->prev = block->prev;

    free(block);
}

void Z_FreeTags(int32_t lowtag, int32_t hightag)
{
    if (lowtag <= PU_FREE)
        lowtag = PU_FREE + 1;
    if (hightag > PU_CACHE)
        hightag = PU_CACHE;

    for (; lowtag <= hightag; ++lowtag)
    {
        memblock_t      *block;
        memblock_t      *end_block;

        block = blockbytag[lowtag];
        if (!block)
            continue;
        end_block = block->prev;
        while (1)
        {
            memblock_t  *next = block->next;

            Z_Free((char *)block + HEADER_SIZE);
            if (block == end_block)
                break;
            block = next;                               // Advance to next block
        }
    }
}

void Z_ChangeTag(void *ptr, int32_t tag)
{
    memblock_t  *block = (memblock_t *)((char *)ptr - HEADER_SIZE);

    // proff - added sanity check, this can happen when an empty lump is locked
    if (!ptr)
        return;

    // proff - do nothing if tag doesn't differ
    if (tag == block->tag)
        return;

    if (block == block->next)
        blockbytag[block->tag] = NULL;
    else if (blockbytag[block->tag] == block)
        blockbytag[block->tag] = block->next;
    block->prev->next = block->next;
    block->next->prev = block->prev;

    if (!blockbytag[tag])
    {
        blockbytag[tag] = block;
        block->next = block->prev = block;
    }
    else
    {
        blockbytag[tag]->prev->next = block;
        block->prev = blockbytag[tag]->prev;
        block->next = blockbytag[tag];
        blockbytag[tag]->prev = block;
    }

    block->tag = tag;
}
/*
//
// [nitr8] UNUSED
//
void Z_ChangeUser(void *ptr, void **user)
{
    memblock_t  *block;

    block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));

    block->user = user;
    *user = ptr;
}
*/
void Z_DrawStats(void)            // Print allocation statistics
{
    char act_mem[50];
    char pur_mem[50];
    char free_mem[50];
    char tot_mem[50];

    double s;

    unsigned long total_memory;

    if (gamestate != GS_LEVEL)
        return;

    if (memory_size > 0)
    {
        total_memory = free_memory + memory_size + active_memory + purgable_memory;
        s = 100.0 / total_memory;

        sprintf(act_mem, "%zu\t%6.01f%%\tstatic\n", active_memory, active_memory * s);
        sprintf(pur_mem, "%zu\t%6.01f%%\tpurgable\n", purgable_memory, purgable_memory * s);
        sprintf(free_mem, "%lu\t%6.01f%%\tfree\n", (free_memory + memory_size),
               (free_memory + memory_size) * s);
        sprintf(tot_mem, "%lu\t\ttotal\n", total_memory);
    }
    else
    {
        total_memory = active_memory + purgable_memory;
        s = 100.0 / total_memory;

        sprintf(act_mem, "%zu\t%6.01f%%\tstatic\n", active_memory, active_memory * s);
        sprintf(pur_mem, "%zu\t%6.01f%%\tpurgable\n", purgable_memory, purgable_memory * s);
        sprintf(tot_mem, "%lu\t\ttotal\n", total_memory);
    }

    if(leveltime & 16)
        M_WriteText(0, 10, "Memory Heap Info\n");

    M_WriteText(0, 20, act_mem);
    M_WriteText(0, 30, pur_mem);
    M_WriteText(0, 40, free_mem);
    M_WriteText(0, 50, tot_mem);
}

#else // CHOCOLATE_ZONE_HANDLING


//
// ZONE MEMORY ALLOCATION
//
// There is never any space between memblocks,
//  and there will never be two contiguous free memblocks.
// The rover can be left pointing at a non-empty block.
//
// It is of no value to free a cachable block,
//  because it will get overwritten automatically if needed.
// 


#define MEM_ALIGN     sizeof(void *)
#define ZONEID        0x1d4a11
#define MINFRAGMENT   64


typedef struct memblock_s
{
    int                size; // including the header and possibly tiny fragments
    int                tag;  // PU_FREE if this is free
    int                id;   // should be ZONEID

    void**             user;

    struct memblock_s* next;
    struct memblock_s* prev;

} memblock_t;


typedef struct
{
    // total bytes malloced, including header
    int                size;

    // start / end cap for linked list
    memblock_t         blocklist;
    
    memblock_t*        rover;
    
} memzone_t;


static memzone_t*      mainzone;

#ifndef WII
static dboolean         zero_on_free;
static dboolean         scan_on_free;
#endif

static const size_t    HEADER_SIZE = (sizeof(memblock_t) + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1);

//static memblock_t      *blockbytag[PU_NUM_TAGS];

//static int             free_memory = 0;
//static int             active_memory = 0;
//static int             purgable_memory = 0;

extern int             memory_size;


void Z_DrawStats(void)            // Print allocation statistics
{
    char act_mem[50];
    char pur_mem[50];
    char free_mem[50];
    char tot_mem[50];

    double s;

    unsigned long total_memory;

    if (gamestate != GS_LEVEL)
        return;

    if (memory_size > 0)
    {
        total_memory = free_memory + memory_size + active_memory + purgable_memory;
        s = 100.0 / total_memory;

        sprintf(act_mem, "%zu\t%6.01f%%\tstatic\n", active_memory, active_memory * s);
        sprintf(pur_mem, "%zu\t%6.01f%%\tpurgable\n", purgable_memory, purgable_memory * s);
        sprintf(free_mem, "%d\t%6.01f%%\tfree\n", (free_memory + memory_size),
               (free_memory + memory_size) * s);
        sprintf(tot_mem, "%lu\t\ttotal\n", total_memory);
    }
    else
    {
        total_memory = active_memory + purgable_memory;
        s = 100.0 / total_memory;

        sprintf(act_mem, "%zu\t%6.01f%%\tstatic\n", active_memory, active_memory * s);
        sprintf(pur_mem, "%zu\t%6.01f%%\tpurgable\n", purgable_memory, purgable_memory * s);
        sprintf(tot_mem, "%lu\t\ttotal\n", total_memory);
    }

    if(leveltime & 16)
        M_WriteText(0, 10, "Memory Heap Info\n");

    M_WriteText(0, 20, act_mem);
    M_WriteText(0, 30, pur_mem);
    M_WriteText(0, 40, free_mem);
    M_WriteText(0, 50, tot_mem);
}

/*
//
// Z_ClearZone
//
void Z_ClearZone (memzone_t* zone)
{
    memblock_t*                block;
        
    // set the entire zone to one free block
    zone->blocklist.next =
        zone->blocklist.prev =
        block = (memblock_t *)( (byte *)zone + sizeof(memzone_t) );
    
    zone->blocklist.user = (void *)zone;
    zone->blocklist.tag = PU_STATIC;
    zone->rover = block;
        
    block->prev = block->next = &zone->blocklist;
    
    // a free block.
    block->tag = PU_FREE;

    block->size = zone->size - sizeof(memzone_t);
}
*/


//
// Z_Init
//
void Z_Init (void)
{
    memblock_t*        block;
    int                size;

    mainzone = (memzone_t *)I_ZoneBase (&size);
    mainzone->size = size;

    // set the entire zone to one free block
    mainzone->blocklist.next =
        mainzone->blocklist.prev =
        block = (memblock_t *)( (byte *)mainzone + sizeof(memzone_t) );

    mainzone->blocklist.user = (void *)mainzone;
    mainzone->blocklist.tag = PU_STATIC;
    mainzone->rover = block;
        
    block->prev = block->next = &mainzone->blocklist;

    // free block
    block->tag = PU_FREE;
    
    block->size = mainzone->size - sizeof(memzone_t);

#ifndef WII
    //!
    // Zone memory debugging flag. If set, memory is zeroed after it is freed
    // to deliberately break any code that attempts to use it after free.
    //
    zero_on_free = M_ParmExists("-zonezero");

    //!
    // Zone memory debugging flag. If set, each time memory is freed, the zone
    // heap is scanned to look for remaining pointers to the freed block.
    //
    scan_on_free = M_ParmExists("-zonescan");
#endif
}


void *Z_Realloc(void *ptr, size_t size)
{
    void        *newp = realloc(ptr, size);

    if (!newp)
        I_Error("Z_Realloc: Failure trying to reallocate %i bytes", size);
    else
        ptr = newp;

    return ptr;
}

// Scan the zone heap for pointers within the specified range, and warn about
// any remaining pointers.

#ifndef WII
static void ScanForBlock(void *start, void *end)
{
    memblock_t *block;
    void **mem;
    int i, len;

    block = mainzone->blocklist.next;

    while (block->next != &mainzone->blocklist)
    {
        int tag = block->tag;

        if (tag == PU_STATIC || tag == PU_LEVEL || tag == PU_LEVSPEC)
        {
            // Scan for pointers on the assumption that pointers are aligned
            // on word boundaries (word size depending on pointer size):
            mem = (void **) ((byte *) block + sizeof(memblock_t));
            len = (block->size - sizeof(memblock_t)) / sizeof(void *);

            for (i = 0; i < len; ++i)
            {
                if (start <= mem[i] && mem[i] <= end)
                {
                    fprintf(stderr,
                            "%p has dangling pointer into freed block "
                            "%p (%p -> %p)\n",
                            mem, start, &mem[i], mem[i]);
                }
            }
        }

        block = block->next;
    }
}
#endif

//
// Z_Free
//
void Z_Free (void* ptr)
{
    memblock_t*                block;
    memblock_t*                other;
        
    block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
        I_Error ("Z_Free: freed a pointer without ZONEID");
                
    if (block->tag != PU_FREE && block->user != NULL)
    {
            // clear the user's mark
            *block->user = 0;
    }

    // mark as free
    block->tag = PU_FREE;
    block->user = NULL;
    block->id = 0;

#ifndef WII
    // If the -zonezero flag is provided, we zero out the block on free
    // to break code that depends on reading freed memory.
    if (zero_on_free)
    {
        memset(ptr, 0, block->size - sizeof(memblock_t));
    }

    if (scan_on_free)
    {
        ScanForBlock(ptr,
                     (byte *) ptr + block->size - sizeof(memblock_t));
    }
#endif

    other = block->prev;

    free_memory += block->size;

    if (block->tag >= PU_PURGELEVEL)
        purgable_memory -= block->size;
    else
        active_memory -= block->size;

    if (other->tag == PU_FREE)
    {
        // merge with previous free block
        other->size += block->size;
        other->next = block->next;
        other->next->prev = other;

        if (block == mainzone->rover)
            mainzone->rover = other;

        block = other;
    }
        
    other = block->next;
    if (other->tag == PU_FREE)
    {
        // merge the next free block onto the end
        block->size += other->size;
        block->next = other->next;
        block->next->prev = block;

        if (other == mainzone->rover)
            mainzone->rover = block;
    }
}



//
// Z_Malloc
// You can pass a NULL user if the tag is < PU_PURGELEVEL.
//

void* Z_Malloc(int size, int32_t tag, void* user)
{
    int                extra;
    memblock_t*        start;
    memblock_t*        rover;
    memblock_t*        newblock;
    memblock_t*        base;
    void*              result;

    size = (size + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1);
    
    // scan through the block list,
    // looking for the first free block
    // of sufficient size,
    // throwing out any purgable blocks along the way.

    // account for size of block header
    size += sizeof(memblock_t);
/*
    if (memory_size > 0 && ((free_memory + memory_size) < (int)(size + HEADER_SIZE)))
    {
        memblock_t *end_block;
        base = blockbytag[PU_CACHE];

        if (base)
        {
            end_block = base->prev;

            while (1)
            {
                memblock_t *next = base->next;

                (Z_Free)((char *) base + HEADER_SIZE);

                if (((free_memory + memory_size) >= (int)(size + HEADER_SIZE)) || (base == end_block))
                    break;

                // Advance to next block
                base = next;
            }
        }
        base = NULL;
    }
*/
    // if there is a free block behind the rover,
    //  back up over them
    base = mainzone->rover;
    
    if (base->prev->tag == PU_FREE)
        base = base->prev;
        
    rover = base;
    start = base->prev;
        
    do
    {
        if (rover == start)
        {
#ifdef WII
            // scanned all the way around the list
            I_Error ("Z_Malloc: failed on allocation of %i bytes", size);
#else
            // [nitr8]: i highly doubt that this is going to work for the Wii port
            //          as RAM is very limited for that console, so: PC ONLY!!

            // [crispy] allocate another zone twice as big
            Z_Init();
#endif
        }
        
        if (rover->tag != PU_FREE)
        {
            if (rover->tag < PU_PURGELEVEL)
            {
                // hit a block that can't be purged,
                // so move base past it
                base = rover = rover->next;
            }
            else
            {
                // free the rover block (adding the size to base)

                // the rover can be the base block
                base = base->prev;
                Z_Free ((byte *)rover+sizeof(memblock_t));
                base = base->next;
                rover = base->next;
            }
        }
        else
        {
            rover = rover->next;
        }

    } while (base->tag != PU_FREE || base->size < size);

    
    // found a block big enough
    extra = base->size - size;
    
    if (extra >  MINFRAGMENT)
    {
        // there will be a free fragment after the allocated block
        newblock = (memblock_t *) ((byte *)base + size );
        newblock->size = extra;

        newblock->tag = PU_FREE;
        newblock->user = NULL;        
        newblock->prev = base;
        newblock->next = base->next;
        newblock->next->prev = newblock;

        base->next = newblock;
        base->size = size;

        if (tag >= PU_PURGELEVEL)
            purgable_memory += base->size;
        else
            active_memory += base->size;

        free_memory -= base->size;
    }

    if (user == NULL && tag >= PU_PURGELEVEL)
        I_Error ("Z_Malloc: an owner is required for purgable blocks");

    base->user = user;
    base->tag = tag;

    result  = (void *) ((byte *)base + sizeof(memblock_t));

    if (base->user)
    {
        *base->user = result;
    }

    // next allocation will start looking here
    mainzone->rover = base->next;        
        
    base->id = ZONEID;
    
    return result;
}



//
// Z_FreeTags
//
void Z_FreeTags(int32_t lowtag, int32_t hightag)
{
    memblock_t*        block;
    memblock_t*        next;
        
    for (block = mainzone->blocklist.next ;
         block != &mainzone->blocklist ;
         block = next)
    {
        // get link before freeing
        next = block->next;

        // free block?
        if (block->tag == PU_FREE)
            continue;
        
        if (block->tag >= lowtag && block->tag <= hightag)
            Z_Free ( (byte *)block + sizeof(memblock_t));
    }
/*
    if (lowtag <= PU_FREE)
        lowtag = PU_FREE + 1;
    if (hightag > PU_CACHE)
        hightag = PU_CACHE;

    for (; lowtag <= hightag; ++lowtag)
    {
        memblock_t      *block;
        memblock_t      *end_block;

        block = blockbytag[lowtag];
        if (!block)
            continue;
        end_block = block->prev;
        while (1)
        {
            memblock_t  *next = block->next;

            Z_Free((char *)block + HEADER_SIZE);
            if (block == end_block)
                break;
            block = next;                               // Advance to next block
        }
    }
*/
}



//
// Z_DumpHeap
// Note: TFileDumpHeap( stdout ) ?
//
// [nitr8] UNUSED
//
/*
void
Z_DumpHeap
( int                lowtag,
  int                hightag )
{
    memblock_t*        block;
        
    C_Printf (CR_GRAY, " zone size: %i  location: %p\n",
            mainzone->size,mainzone);
    
    C_Printf (CR_GRAY, " tag range: %i to %i\n",
            lowtag, hightag);
        
    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
        if (block->tag >= lowtag && block->tag <= hightag)
            C_Printf (CR_GRAY, " block:%p    size:%7i    user:%p    tag:%3i\n",
                    block, block->size, block->user, block->tag);
                
        if (block->next == &mainzone->blocklist)
        {
            // all blocks have been hit
            break;
        }
        
        if ( (byte *)block + block->size != (byte *)block->next)
            C_Printf (CR_RED, " ERROR: block size does not touch the next block\n");

        if ( block->next->prev != block)
            C_Printf (CR_RED, " ERROR: next block doesn't have proper back link\n");

        if (block->tag == PU_FREE && block->next->tag == PU_FREE)
            C_Printf (CR_RED, " ERROR: two consecutive free blocks\n");
    }
}


//
// Z_FileDumpHeap
//
void Z_FileDumpHeap (FILE* f)
{
    memblock_t*        block;
        
    fprintf (f,"zone size: %i  location: %p\n",mainzone->size,mainzone);
        
    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
        fprintf (f,"block:%p    size:%7i    user:%p    tag:%3i\n",
                 block, block->size, block->user, block->tag);
                
        if (block->next == &mainzone->blocklist)
        {
            // all blocks have been hit
            break;
        }
        
        if ( (byte *)block + block->size != (byte *)block->next)
            fprintf (f,"ERROR: block size does not touch the next block\n");

        if ( block->next->prev != block)
            fprintf (f,"ERROR: next block doesn't have proper back link\n");

        if (block->tag == PU_FREE && block->next->tag == PU_FREE)
            fprintf (f,"ERROR: two consecutive free blocks\n");
    }
}
*/


//
// Z_CheckHeap
//
void Z_CheckHeap (void)
{
    memblock_t*        block;
        
    for (block = mainzone->blocklist.next ; ; block = block->next)
    {
        if (block->next == &mainzone->blocklist)
        {
            // all blocks have been hit
            break;
        }
        
        if ( (byte *)block + block->size != (byte *)block->next)
            I_Error ("Z_CheckHeap: block size does not touch the next block\n");

        if ( block->next->prev != block)
            I_Error ("Z_CheckHeap: next block doesn't have proper back link\n");

        if (block->tag == PU_FREE && block->next->tag == PU_FREE)
            I_Error ("Z_CheckHeap: two consecutive free blocks\n");
    }
}



/*
//
// Z_ChangeTag
//
void Z_ChangeTag2(void *ptr, int32_t tag, char *file, int line)
{
    memblock_t*        block;
        
    block = (memblock_t *) ((byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
        I_Error("%s:%i: Z_ChangeTag: block without a ZONEID!",
                file, line);

    if (tag >= PU_PURGELEVEL && block->user == NULL)
        I_Error("%s:%i: Z_ChangeTag: an owner is required "
                "for purgable blocks", file, line);

    if (block->tag < PU_PURGELEVEL && tag >= PU_PURGELEVEL)
    {
        active_memory -= block->size;
        purgable_memory += block->size;
    }
    else if (block->tag >= PU_PURGELEVEL && tag < PU_PURGELEVEL)
    {
        active_memory += block->size;
        purgable_memory -= block->size;
    }

    block->tag = tag;
}
*/

//
// [nitr8] UNUSED
//
/*
void Z_ChangeUser(void *ptr, void **user)
{
    memblock_t*        block;

    block = (memblock_t *) ((byte *)ptr - sizeof(memblock_t));

    if (block->id != ZONEID)
    {
        I_Error("Z_ChangeUser: Tried to change user for invalid block!");
    }

    block->user = user;
    *user = ptr;
}
*/


//
// Z_FreeMemory
//
// [nitr8] UNUSED
//
/*
int Z_FreeMemory (void)
{
//    memblock_t*                block;
//    int                        free;
        
//    free = 0;
    
//    for (block = mainzone->blocklist.next ;
//         block != &mainzone->blocklist;
//         block = block->next)
//    {
//        if (block->tag == PU_FREE || block->tag >= PU_PURGELEVEL)
//            free += block->size;
//    }

//    return free;

    memblock_t*         block;
    int                 free = 0;

    for (block = mainzone->blocklist.next ;
         block != &mainzone->blocklist;
         block = block->next)
    {
        if (block->user == 0)
        {
            // free memory
            free += block->size;
        }
        else
        {
            if(block->tag >= PU_PURGELEVEL)
            {
                // purgable memory (cache)
                free += block->size;
            }
            else
            {
                // used block
                free = 0;
            }
        }
    }
    return free;
}

void* Z_MallocAlign (int reqsize, int32_t tag, void **user, int alignbits)
{
    memblock_t* newblock;

    void* basedata;

    // with the memalloc header
    int         memalloc_size;
   
    // choose safe interpretation
    if( tag == PU_FREE )
    {
        tag = PU_LEVEL;
    }

    // alloc rounded up to next 4 byte alignment
    reqsize = (reqsize + 3) & ~3;

    // account for size of block header
    memalloc_size = reqsize + sizeof(memblock_t);

    newblock = malloc(memalloc_size);

    if( newblock == NULL )
    {
        I_Error ("Z_Malloc: malloc failed on allocation of %i bytes\n");
    }

    mainzone->size += memalloc_size;

    newblock->id = ZONEID;
    newblock->user = user;
    newblock->size = memalloc_size;

    basedata = (byte*)newblock + sizeof(memblock_t);

    if (user)
        *user = basedata;

    return basedata;
}
*/
#endif // CHOCOLATE_ZONE_HANDLING

//
// Z_ChangeTag
//
void Z_ChangeTag2(void *ptr, int32_t tag, char *file, int line)
{
    memblock_t*        block;
        
    block = (memblock_t *) ((byte *)ptr - sizeof(memblock_t));
#ifdef WII
    if (block->id != ZONEID)
        I_Error("%s:%i: Z_ChangeTag: block without a ZONEID!",
                file, line);
#endif
    if (tag >= PU_PURGELEVEL && block->user == NULL)
        I_Error("%s:%i: Z_ChangeTag: an owner is required "
                "for purgable blocks", file, line);

    if (block->tag < PU_PURGELEVEL && tag >= PU_PURGELEVEL)
    {
        active_memory -= block->size;
        purgable_memory += block->size;
    }
    else if (block->tag >= PU_PURGELEVEL && tag < PU_PURGELEVEL)
    {
        active_memory += block->size;
        purgable_memory -= block->size;
    }

    block->tag = tag;
}

void* Z_MallocAlign (int reqsize, int32_t tag, void **user, int alignbits)
{
    memblock_t* newblock;

    void* basedata;

    // with the memalloc header
    int         memalloc_size;
   
    // choose safe interpretation
    if( tag == PU_FREE )
    {
        tag = PU_LEVEL;
    }

    // alloc rounded up to next 4 byte alignment
    reqsize = (reqsize + 3) & ~3;

    // account for size of block header
    memalloc_size = reqsize + sizeof(memblock_t);

    newblock = malloc(memalloc_size);

    if( newblock == NULL )
    {
        I_Error ("Z_Malloc: malloc failed on allocation of %i bytes\n");
    }

#ifndef DOOMRETRO_ZONE_HANDLING
    mainzone->size += memalloc_size;

    newblock->id = ZONEID;
#endif

    newblock->user = user;
    newblock->size = memalloc_size;

    basedata = (byte*)newblock + sizeof(memblock_t);

    if (user)
        *user = basedata;

    return basedata;
}

