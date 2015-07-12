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
#include "doomtype.h"
#include "i_system.h"
#include "m_menu.h"
#include "v_trans.h"
#include "z_zone.h"


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


memzone_t*             mainzone;

/*
static const size_t    HEADER_SIZE = (sizeof(memblock_t) + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1);

static memblock_t      *blockbytag[PU_NUM_TAGS];
*/
static int             free_memory = 0;
static int             active_memory = 0;
static int             purgable_memory = 0;

extern int             memory_size;


void Z_DrawStats(void)            // Print allocation statistics
{
    char act_mem[50];
    char pur_mem[50];
    char free_mem[50];
    char tot_mem[50];

    if (gamestate != GS_LEVEL)
        return;

    if (memory_size > 0)
    {
        unsigned long total_memory = free_memory + memory_size + active_memory + purgable_memory;
        double s = 100.0 / total_memory;

        sprintf(act_mem, "%d\t%6.01f%%\tstatic\n", active_memory, active_memory * s);
        sprintf(pur_mem, "%d\t%6.01f%%\tpurgable\n", purgable_memory, purgable_memory * s);
        sprintf(free_mem, "%d\t%6.01f%%\tfree\n", (free_memory + memory_size),
               (free_memory + memory_size) * s);
        sprintf(tot_mem, "%lu\t\ttotal\n", total_memory);
    }
    else
    {
        unsigned long total_memory = active_memory + purgable_memory;
        double s = 100.0 / total_memory;

        sprintf(act_mem, "%d\t%6.01f%%\tstatic\n", active_memory, active_memory * s);
        sprintf(pur_mem, "%d\t%6.01f%%\tpurgable\n", purgable_memory, purgable_memory * s);
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
}


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

void*
Z_Malloc
( int                  size,
  int                  tag,
  void*                user )
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
            // scanned all the way around the list
//            I_Error ("Z_Malloc: failed on allocation of %i bytes", size);

            // [nitr8]: i highly doubt that this is going to work for the Wii port
            //          as RAM is very limited for that console (let's find out)

            // [crispy] allocate another zone twice as big
            Z_Init();
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
void
Z_FreeTags
( int                lowtag,
  int                hightag )
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
            Z_Free ( (byte *)block+sizeof(memblock_t));
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




//
// Z_ChangeTag
//
void Z_ChangeTag2(void *ptr, int tag, char *file, int line)
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



//
// Z_FreeMemory
//
int Z_FreeMemory (void)
{
/*
    memblock_t*                block;
    int                        free;
        
    free = 0;
    
    for (block = mainzone->blocklist.next ;
         block != &mainzone->blocklist;
         block = block->next)
    {
        if (block->tag == PU_FREE || block->tag >= PU_PURGELEVEL)
            free += block->size;
    }

    return free;
*/
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

void* Z_MallocAlign (int reqsize, int tag, void **user, int alignbits)
{
    memblock_t* newblock;

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

    void* basedata = (byte*)newblock + sizeof(memblock_t);

    if (user)
        *user = basedata;

    return basedata;
}

