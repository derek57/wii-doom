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
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//        Remark: this was the only stuff that, according
//         to John Carmack, might have been useful for
//         Quake.
//


#ifndef __Z_ZONE__
#define __Z_ZONE__


#include <stdio.h>

#include "doomfeatures.h"


#ifdef BOOM_ZONE_HANDLING

// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: z_zone.h,v 1.7 1998/05/08 20:32:12 killough Exp $
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
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//      Remark: this was the only stuff that, according
//       to John Carmack, might have been useful for
//       Quake.
//
// Rewritten by Lee Killough, though, since it was not efficient enough.
//
//---------------------------------------------------------------------

#ifndef __GNUC__
#define __attribute__(x)
#endif

// Remove all definitions before including system definitions

#undef malloc
#undef free
#undef realloc
#undef calloc
//#undef strdup

// Include system definitions so that prototypes become
// active before macro replacements below are in effect.

#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ZONE MEMORY
// PU - purge tags.

enum {PU_FREE, PU_STATIC, PU_SOUND, PU_MUSIC, PU_LEVEL, PU_LEVSPEC, PU_CACHE,
      /* Must always be last -- killough */ PU_MAX};

#define PU_PURGELEVEL PU_CACHE        /* First purgable tag's level */

void *(Z_Malloc)(size_t size, int tag, void **ptr, char *, int);
void (Z_Free)(void *ptr, char *, int);
void (Z_FreeTags)(int lowtag, int hightag, char *, int);
void (Z_ChangeTag)(void *ptr, int tag, char *, int);
void (Z_Init)(void);
void *(Z_Calloc)(size_t n, size_t n2, int tag, void **user, char *, int);
void *(Z_Realloc)(void *p, size_t n, int tag, void **user, char *, int);
//char *(Z_Strdup)(char *s, int tag, void **user, char *, int);
void (Z_CheckHeap)(char *,int);   // killough 3/22/98: add file/line info
void Z_DumpHistory(char *);

#define Z_Free(a)          (Z_Free)     (a,      __FILE__,__LINE__)
#define Z_FreeTags(a,b)    (Z_FreeTags) (a,b,    __FILE__,__LINE__)
#define Z_ChangeTag(a,b)   (Z_ChangeTag)(a,b,    __FILE__,__LINE__)
#define Z_Malloc(a,b,c)    (Z_Malloc)   (a,b,c,  __FILE__,__LINE__)
//#define Z_Strdup(a,b,c)    (Z_Strdup)   (a,b,c,  __FILE__,__LINE__)
#define Z_Calloc(a,b,c,d)  (Z_Calloc)   (a,b,c,d,__FILE__,__LINE__)
#define Z_Realloc(a,b,c,d) (Z_Realloc)  (a,b,c,d,__FILE__,__LINE__)
#define Z_CheckHeap()      (Z_CheckHeap)(__FILE__,__LINE__)

#define malloc(n)          Z_Malloc(n,PU_STATIC,0)
#define free(p)            Z_Free(p)
#define realloc(p,n)       Z_Realloc(p,n,PU_STATIC,0)
#define calloc(n1,n2)      Z_Calloc(n1,n2,PU_STATIC,0)
//#define strdup(s)          Z_Strdup(s,PU_STATIC,0)

// Doom-style printf
//void dprintf(const char *, ...) __attribute__((format(printf,1,2)));

void Z_ZoneHistory(char *);

void Z_ChangeUser(void *ptr, void **user);


//----------------------------------------------------------------------------
//
// $Log: z_zone.h,v $
// Revision 1.7  1998/05/08  20:32:12  killough
// fix __attribute__ redefinition
//
// Revision 1.6  1998/05/03  22:38:11  killough
// Remove unnecessary #include
//
// Revision 1.5  1998/04/27  01:49:42  killough
// Add history of malloc/free and scrambler (INSTRUMENTED only)
//
// Revision 1.4  1998/03/23  03:43:54  killough
// Make Z_CheckHeap() more diagnostic
//
// Revision 1.3  1998/02/02  13:28:06  killough
// Add dprintf
//
// Revision 1.2  1998/01/26  19:28:04  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:06  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------

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

// Include system definitions so that prototypes become
// active before macro replacements below are in effect.

#include "doomtype.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//
// ZONE MEMORY
// PU - purge tags.
//
enum
{
    PU_FREE,       // a free block
    PU_STATIC,     // static entire execution time
    PU_LEVEL,      // static until level exited
    PU_LEVSPEC,    // a special thinker in a level

    PU_CACHE,
    PU_MAX         // Must always be last -- killough
};

#define PU_PURGELEVEL    PU_CACHE    // First purgeable tag's level

void *Z_Malloc(size_t size, int32_t tag, void **user);
void *Z_Calloc(size_t n1, size_t n2, int32_t tag, void **user); 
void *Z_Realloc(void *ptr, size_t size);
void Z_Free(void *ptr);
void Z_FreeTags(int32_t lowtag, int32_t hightag);
void Z_ChangeTag(void *ptr, int32_t tag);
void Z_ChangeUser(void *ptr, void **user);

#elif defined WIIDOOM_ZONE_HANDLING

#ifndef __GNUC__
#define __attribute__(x)
#endif

// Include system definitions so that prototypes become
// active before macro replacements below are in effect.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// ZONE MEMORY
// PU - purge tags.

enum {PU_FREE, PU_STATIC, PU_SOUND, PU_MUSIC, PU_LEVEL, PU_LEVSPEC, PU_CACHE,
      /* Must always be last -- killough */ PU_MAX};

#define PU_PURGELEVEL PU_CACHE        /* First purgable tag's level */

#ifdef INSTRUMENTED
#define DA(x,y) ,x,y
#define DAC(x,y) x,y
#else
#define DA(x,y) 
#define DAC(x,y)
#endif

void *(Z_Malloc)(size_t size, int tag, void **ptr DA(const char *, int));
void (Z_Free)(void *ptr DA(const char *, int));
void (Z_FreeTags)(int lowtag, int hightag DA(const char *, int));
void (Z_ChangeTag)(void *ptr, int tag DA(const char *, int));
void (Z_Init)(void);
void Z_Close(void);
void *(Z_Calloc)(size_t n, size_t n2, int tag, void **user DA(const char *, int));
void *(Z_Realloc)(void *p, size_t n, int tag, void **user DA(const char *, int));
char *(Z_Strdup)(const char *s, int tag, void **user DA(const char *, int));
void (Z_CheckHeap)(DAC(const char *,int));   // killough 3/22/98: add file/line info
void Z_DumpHistory(char *);

#ifdef INSTRUMENTED
/* cph - save space if not debugging, don't require file 
 * and line to memory calls */
#define Z_Free(a)          (Z_Free)     (a,      __FILE__,__LINE__)
#define Z_FreeTags(a,b)    (Z_FreeTags) (a,b,    __FILE__,__LINE__)
#define Z_ChangeTag(a,b)   (Z_ChangeTag)(a,b,    __FILE__,__LINE__)
#define Z_Malloc(a,b,c)    (Z_Malloc)   (a,b,c,  __FILE__,__LINE__)
#define Z_Strdup(a,b,c)    (Z_Strdup)   (a,b,c,  __FILE__,__LINE__)
#define Z_Calloc(a,b,c,d)  (Z_Calloc)   (a,b,c,d,__FILE__,__LINE__)
#define Z_Realloc(a,b,c,d) (Z_Realloc)  (a,b,c,d,__FILE__,__LINE__)
#define Z_CheckHeap()      (Z_CheckHeap)(__FILE__,__LINE__)
#endif

/* cphipps 2001/11/18 -
 * If we're using memory mapped file access to WADs, we won't need to maintain
 * our own heap. So we *could* let "normal" malloc users use the libc malloc
 * directly, for efficiency. Except we do need a wrapper to handle out of memory
 * errors... damn, ok, we'll leave it for now.
 */
#ifndef HAVE_LIBDMALLOC
// Remove all definitions before including system definitions

#undef malloc
#undef free
#undef realloc
#undef calloc
#undef strdup

#define malloc(n)          Z_Malloc(n,PU_STATIC,0)
#define free(p)            Z_Free(p)
#define realloc(p,n)       Z_Realloc(p,n,PU_STATIC,0)
#define calloc(n1,n2)      Z_Calloc(n1,n2,PU_STATIC,0)
#define strdup(s)          Z_Strdup(s,PU_STATIC,0)

#else

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

#endif

void Z_ZoneHistory(char *);

#else // CHOCOLATE_ZONE_HANDLING

//
// This is used to get the local FILE:LINE info from CPP
// prior to really call the function in question.
//
/*
#define Z_ChangeTag(p,t)                                       \
    Z_ChangeTag2((p), (t), __FILE__, __LINE__)
*/

//
// ZONE MEMORY
// PU - purge tags.

enum
{
    PU_STATIC = 1,                  // static entire execution time
    PU_SOUND,                       // static while playing
    PU_MUSIC,                       // static while playing
    PU_FREE,                        // a free block
    PU_LEVEL,                       // static until level exited
    PU_LEVSPEC,                     // a special thinker in a level
    
    // Tags >= PU_PURGELEVEL are purgable whenever needed.

    PU_PURGELEVEL,
    PU_CACHE,

    // Total number of different tag types

    PU_NUM_TAGS
};
        

void         *Z_Calloc(size_t n1, size_t n2, int32_t tag, void **user); 
void         *Z_Realloc(void *ptr, size_t size);
void         *Z_Malloc (int size, int tag, void *ptr);
void         Z_Init (void);
void         Z_Free (void *ptr);
void         Z_FreeTags (int lowtag, int hightag);
void         Z_DumpHeap (int lowtag, int hightag);
void         Z_FileDumpHeap (FILE *f);
void         Z_CheckHeap (void);
//void         Z_ChangeTag2 (void *ptr, int tag, char *file, int line);
void         Z_ChangeUser(void *ptr, void **user);

int          Z_FreeMemory (void);

#endif

void         Z_DrawStats(void);            // Print allocation statistics
void         *Z_MallocAlign (int reqsize, int tag, void **user, int alignbits);

#endif

