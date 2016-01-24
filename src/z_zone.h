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


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "doomfeatures.h"
#include "doomtype.h"

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

#ifdef INSTRUMENTED
#define DA(x,y) ,x,y
#define DAC(x,y) x,y
#else
#define DA(x,y) 
#define DAC(x,y)
#endif

void *(Z_Malloc)(size_t size, int32_t tag, void **user DA(const char *, int));
void *Z_MallocAlign(int reqsize, int32_t tag, void **user, int alignbits);
void *(Z_Calloc)(size_t n1, size_t n2, int32_t tag, void **user DA(const char *, int)); 
void *(Z_Realloc)(void *ptr, size_t size, int tag, void **user DA(const char *, int));
void (Z_Free)(void *ptr DA(const char *, int));
void (Z_FreeTags)(int32_t lowtag, int32_t hightag DA(const char *, int));
void (Z_ChangeTag)(void *ptr, int32_t tag DA(const char *, int));
void Z_ChangeUser(void *ptr, void **user);
void Z_DrawStats(void);            // Print allocation statistics
void Z_DumpHistory(char *buf);
void Z_DumpMemory(void);

#ifdef INSTRUMENTED
/* cph - save space if not debugging, don't require file 
 * and line to memory calls */
#define Z_Free(a)          (Z_Free)     (a,      __FILE__,__LINE__)
#define Z_FreeTags(a,b)    (Z_FreeTags) (a,b,    __FILE__,__LINE__)
#define Z_ChangeTag(a,b)   (Z_ChangeTag)(a,b,    __FILE__,__LINE__)
#define Z_Malloc(a,b,c)    (Z_Malloc)   (a,b,c,  __FILE__,__LINE__)
#define Z_Calloc(a,b,c,d)  (Z_Calloc)   (a,b,c,d,__FILE__,__LINE__)
#define Z_Realloc(a,b,c,d) (Z_Realloc)  (a,b,c,d,__FILE__,__LINE__)
#endif

