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

#include <malloc.h>

#include "c_io.h"
#include "doomfeatures.h"
#include "doomtype.h"
#include "i_system.h"

#include "m_argv.h"
#include "doom/m_menu.h"

#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"

// Number of mallocs & frees kept in history buffer (must be a power of 2)
#define ZONE_HISTORY 4

// Minimum chunk size at which blocks are allocated
#define CHUNK_SIZE      32

// signature for block header
#define ZONEID  0x931d4a11

typedef struct memblock
{
#ifdef ZONEIDCHECK
    unsigned id;
#endif

    struct memblock     *next;
    struct memblock     *prev;
    size_t              size;
    void                **user;
    unsigned char       tag;

#ifdef INSTRUMENTED
    const char          *file;
    int                 line;
#endif

} memblock_t;

enum {malloc_history, free_history, NUM_HISTORY_TYPES};

static const char *file_history[NUM_HISTORY_TYPES][ZONE_HISTORY];
static int line_history[NUM_HISTORY_TYPES][ZONE_HISTORY];
static int history_index[NUM_HISTORY_TYPES];
static const char *const desc[NUM_HISTORY_TYPES] = {"malloc()'s", "free()'s"};

static int       free_memory;
static int       active_memory;
static int       purgable_memory;

// size of block header
// cph - base on sizeof(memblock_t), which can be larger than CHUNK_SIZE on
// 64bit architectures
static const size_t     HEADER_SIZE = (sizeof(memblock_t) + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1);

static memblock_t       *blockbytag[PU_MAX];

#ifdef HEAPDUMP

#ifndef HEAPDUMP_DIR
#define HEAPDUMP_DIR "."
#endif

void Z_DumpMemory(void)
{
    static int dump;

    char buf[PATH_MAX + 1];

    FILE* fp;

    size_t total_cache = 0;
    size_t total_free = 0;
    size_t total_malloc = 0;

    int tag;

    sprintf(buf, "%s/memdump.%d", HEAPDUMP_DIR, dump++);

    fp = fopen(buf, "w");

    for (tag = PU_FREE; tag < PU_MAX; tag++)
    {
        memblock_t *block = blockbytag[tag];

        memblock_t *end_block;

        if (!block)
            continue;

        end_block = block->prev;

        while (1)
        {
            switch (block->tag)
            {
                case PU_FREE: 
                    fprintf(fp, "free %zu\n", block->size);
                    total_free += block->size;
                    break;

                case PU_CACHE:
                    fprintf(fp, "cache %s:%d:%zu\n", block->file, block->line, block->size);
                    total_cache += block->size;
                    break;

                case PU_LEVEL:
                    fprintf(fp, "level %s:%d:%zu\n", block->file, block->line, block->size);
                    total_malloc += block->size;
                    break;

                default:
                    fprintf(fp, "malloc %s:%d:%zu", block->file, block->line, block->size);
                    total_malloc += block->size;

                    if (block->file)
                    {
                        if (strstr(block->file,"w_memcache.c"))
                        {
                            W_PrintLump(fp, (char*)block + HEADER_SIZE);
                        }
                    }
                    fputc('\n', fp);
                    break;
            }

            if (block == end_block)
                break;

            block=block->next;
        }
    }

    fprintf(fp, "malloc %zu, cache %zu, free %zu, total %zu\n",
            total_malloc, total_cache, total_free,  total_malloc + total_cache + total_free);

    fclose(fp);
}
#endif

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
void *(Z_Malloc)(size_t size, int32_t tag, void **user
#ifdef INSTRUMENTED
     , const char *file, int line
#endif
     )
{
    memblock_t  *block = NULL;

#ifdef INSTRUMENTED
    file_history[malloc_history][history_index[malloc_history]] = file;
    line_history[malloc_history][history_index[malloc_history]++] = line;
    history_index[malloc_history] &= ZONE_HISTORY-1;
#endif

#ifdef ZONEIDCHECK
    if (tag >= PU_PURGELEVEL && !user)
        I_Error ("Z_Malloc: An owner is required for purgable blocks"
#ifdef INSTRUMENTED
             "Source: %s:%d", file, line
#endif
       );
#endif

    if (!size)
        return (user ? (*user = NULL) : NULL);          // malloc(0) returns NULL

    size = (size + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1); // round to chunk size

    while (!(block = malloc(size + HEADER_SIZE)))
    {
        if (!blockbytag[PU_CACHE])
            I_Error("Z_Malloc: Failure trying to allocate %lu bytes"
#ifdef INSTRUMENTED
               "\nSource: %s:%d"
#endif
               , (unsigned long)size
#ifdef INSTRUMENTED
               , file, line
#endif
            );

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

#ifdef INSTRUMENTED
    if (tag >= PU_PURGELEVEL)
        purgable_memory += block->size;
    else
        active_memory += block->size;
#endif

#ifdef INSTRUMENTED
    block->file = file;
    block->line = line;
#endif

#ifdef ZONEIDCHECK
    block->id = ZONEID;         // signature required in block header
#endif

    block->tag = tag;                                   // tag
    block->user = user;                                 // user
    block = (memblock_t *)((char *)block + HEADER_SIZE);
    if (user)                                           // if there is a user
        *user = block;                                  // set user to point to new block

    return block;
}

void *(Z_Calloc)(size_t n1, size_t n2, int32_t tag, void **user
#ifdef INSTRUMENTED
                 , const char *file, int line
#endif
     )
{
    return ((n1 *= n2) ? memset((Z_Malloc)(n1, tag, user DA(file, line)), 0, n1) : NULL);
}

void *(Z_Realloc)(void *ptr, size_t size
#ifdef INSTRUMENTED
                  , const char *file, int line
#endif
     )
{
    void        *newp = realloc(ptr, size);

    if (!newp && size) 
//        I_Error("Z_Realloc: Failure trying to reallocate %i bytes", size);
        I_Error("Z_Realloc: Failure trying to reallocate %i bytes", size DA(file, line));
    else
        ptr = newp;

    return ptr;
}

void (Z_Free)(void *ptr
#ifdef INSTRUMENTED
              , const char *file, int line
#endif
     )
{
    memblock_t  *block = (memblock_t *)((char *)ptr - HEADER_SIZE);

#ifdef INSTRUMENTED
    file_history[free_history][history_index[free_history]] = file;
    line_history[free_history][history_index[free_history]++] = line;
    history_index[free_history] &= ZONE_HISTORY-1;
#endif

    if (!ptr)
        return;

#ifdef ZONEIDCHECK
    if (block->id != ZONEID)
        I_Error("Z_Free: freed a pointer without ZONEID"
#ifdef INSTRUMENTED
            "\nSource: %s:%d"
            "\nSource of malloc: %s:%d"
            , file, line, block->file, block->line
#endif
           );
    block->id = 0;              // Nullify id so another free fails
#endif

    if (block->user)                                    // Nullify user if one exists
        *block->user = NULL;

    if (block == block->next)
        blockbytag[block->tag] = NULL;
    else if (blockbytag[block->tag] == block)
        blockbytag[block->tag] = block->next;
    block->prev->next = block->next;
    block->next->prev = block->prev;

#ifdef INSTRUMENTED
    if (block->tag >= PU_PURGELEVEL)
        purgable_memory -= block->size;
    else
        active_memory -= block->size;
#endif

    free(block);
}

void (Z_FreeTags)(int32_t lowtag, int32_t hightag
#ifdef INSTRUMENTED
                  , const char *file, int line
#endif
     )
{
#ifdef HEAPDUMP
    if (devparm && dump_mem)
        Z_DumpMemory();
#endif

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

#ifdef INSTRUMENTED
            (Z_Free)((char *)block + HEADER_SIZE, file, line);
#else
            (Z_Free)((char *)block + HEADER_SIZE);
#endif

            if (block == end_block)
                break;
            block = next;                               // Advance to next block
        }
    }
}

void (Z_ChangeTag)(void *ptr, int32_t tag
#ifdef INSTRUMENTED
       , const char *file, int line
#endif
     )
{
    memblock_t  *block = (memblock_t *)((char *)ptr - HEADER_SIZE);

    // proff - added sanity check, this can happen when an empty lump is locked
    if (!ptr)
        return;

    // proff - do nothing if tag doesn't differ
    if (tag == block->tag)
        return;

#ifdef ZONEIDCHECK
    if (block->id != ZONEID)
        I_Error ("Z_ChangeTag: freed a pointer without ZONEID"
#ifdef INSTRUMENTED
             "\nSource: %s:%d"
             "\nSource of malloc: %s:%d"
             , file, line, block->file, block->line
#endif
            );

    if (tag >= PU_PURGELEVEL && !block->user)
        I_Error ("Z_ChangeTag: an owner is required for purgable blocks\n"
#ifdef INSTRUMENTED
             "Source: %s:%d"
             "\nSource of malloc: %s:%d"
             , file, line, block->file, block->line
#endif
            );

#endif // ZONEIDCHECK

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

#ifdef INSTRUMENTED
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
#endif

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

    total_memory = (DEFAULT_RAM * 1024 * 1024);

    s = 100.0 / total_memory;

    free_memory = total_memory - active_memory - purgable_memory;

    sprintf(act_mem, "%-5i\t%6.01f%%\tstatic\n", active_memory, active_memory * s);
    sprintf(pur_mem, "%-5i\t%6.01f%%\tpurgable\n", purgable_memory, purgable_memory * s);
    sprintf(free_mem, "%-5i\t%6.01f%%\tfree\n", free_memory, free_memory * s);
    sprintf(tot_mem, "%-5lu\t\ttotal\n", total_memory);

    if(leveltime & 16)
        M_WriteText(0, 10, "Memory Heap Info\n");

    M_WriteText(0, 20, act_mem);
    M_WriteText(0, 30, pur_mem);
    M_WriteText(0, 40, free_mem);
    M_WriteText(0, 60, tot_mem);
}

//
// [nitr8] UNUSED
//
/*
void *Z_MallocAlign(int reqsize, int32_t tag, void **user, int alignbits)
{
    memblock_t* newblock;

    void*       basedata;

    // with the memalloc header
    int         memalloc_size;
   
    // choose safe interpretation
    if (tag == PU_FREE)
        tag = PU_LEVEL;

    // alloc rounded up to next 4 byte alignment
    reqsize = (reqsize + 3) & ~3;

    // account for size of block header
    memalloc_size = reqsize + sizeof(memblock_t);

    newblock = malloc(memalloc_size);

    if (newblock == NULL)
        I_Error ("Z_Malloc: malloc failed on allocation of %i bytes\n");

    newblock->user = user;
    newblock->size = memalloc_size;

    basedata = (byte*)newblock + sizeof(memblock_t);

    if (user)
        *user = basedata;

    return basedata;
}
*/

