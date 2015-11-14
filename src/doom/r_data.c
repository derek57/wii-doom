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

#ifdef WII
#include "../c_io.h"
#include "../d_deh.h"
#else
#include "c_io.h"
#include "d_deh.h"
#endif

#include "doomstat.h"

#ifdef WII
#include "../i_scale.h"
#include "../i_swap.h"
#include "../i_system.h"
#include "../m_misc.h"
#else
#include "i_scale.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_misc.h"
#endif

#include "p_local.h"
#include "p_tick.h"
#include "r_sky.h"

#ifdef WII
#include "../v_patch.h"
#include "../v_trans.h"
#include "../w_wad.h"
#include "../z_zone.h"
#else
#include "v_patch.h"
#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"
#endif

//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

// killough 4/17/98: make firstcolormaplump, lastcolormaplump external
int             firstcolormaplump;
int             lastcolormaplump;

int             firstflat;
int             lastflat;
int             numflats;

int             firstpatch;
int             lastpatch;
int             numpatches;

int             firstspritelump;
int             lastspritelump;
int             numspritelumps;

int             numtextures;
texture_t       **textures;
texture_t       **textures_hashtable;

int             *texturewidthmask;

// needed for texture pegging
fixed_t         *textureheight;
byte            **texturefullbright;
int             *texturecompositesize;
short           **texturecolumnlump;
unsigned int    **texturecolumnofs;
unsigned int    **texturecolumnofs2; // [crispy] original column offsets for single-patched textures
byte            **texturecomposite;
int             tran_filter_pct = 66;       // filter percent

// for global animation
int             *flattranslation;
int             *texturetranslation;

// needed for prerendering
fixed_t         *spritewidth;
fixed_t         *spriteheight;
fixed_t         *spriteoffset;
fixed_t         *spritetopoffset;

fixed_t         *newspriteoffset;
fixed_t         *newspritetopoffset;

extern char     *iwadfile;
//dboolean        r_fixspriteoffsets = r_fixspriteoffsets_default;

enum {
    r, g, b
} rgb_t;

static byte notgray[256] =
{
    0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static byte notgrayorbrown[256] =
{
    0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static byte redonly[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static byte greenonly1[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static byte greenonly2[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
/*
static byte whiteonly[256] =
{
    0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
*/
static struct
{
    char        texture[9];
    byte        *colormask;
} fullbright[] = {
    { "COMP2",    notgrayorbrown }, { "COMPSTA1", notgray        }, { "COMPSTA2", notgray        },
    { "COMPUTE1", notgrayorbrown }, { "COMPUTE2", notgrayorbrown }, { "COMPUTE3", notgrayorbrown },
    { "EXITSIGN", notgray        }, { "EXITSTON", notgray        }, { "PLANET1",  notgray        },
    { "SILVER2",  notgray        }, { "SILVER3",  notgrayorbrown }, { "SLADSKUL", redonly        },
    { "SW1BRCOM", redonly        }, { "SW1BRIK",  redonly        }, { "SW1BRN1",  redonly        },
    { "SW1COMM",  redonly        }, { "SW1DIRT",  redonly        }, { "SW1MET2",  redonly        },
    { "SW1STARG", redonly        }, { "SW1STON1", redonly        }, { "SW1STON2", redonly        },
    { "SW1STONE", redonly        }, { "SW1STRTN", redonly        }, { "SW2BLUE",  redonly        },
    { "SW2BRCOM", greenonly2     }, { "SW2BRIK",  greenonly1     }, { "SW2BRN1",  greenonly1     },
    { "SW2BRN2",  greenonly1     }, { "SW2BRNGN", notgray        }, { "SW2COMM",  greenonly1     },
    { "SW2COMP",  redonly        }, { "SW2DIRT",  greenonly1     }, { "SW2EXIT",  notgray        },
    { "SW2GRAY",  notgray        }, { "SW2GRAY1", notgray        }, { "SW2GSTON", redonly        },
    { "SW2MARB",  greenonly1     }, { "SW2MET2",  greenonly1     }, { "SW2METAL", greenonly1     },
    { "SW2MOD1",  notgrayorbrown }, { "SW2PANEL", redonly        }, { "SW2ROCK",  redonly        },
    { "SW2SLAD",  redonly        }, { "SW2STARG", greenonly1     }, { "SW2STON1", greenonly1     },
    { "SW2STON2", greenonly1     }, { "SW2STON6", redonly        }, { "SW2STONE", greenonly1     },
    { "SW2STRTN", greenonly1     }, { "SW2TEK",   greenonly1     }, { "SW2VINE",  greenonly1     },
    { "SW2WDMET", redonly        }, { "SW2WOOD",  redonly        }, { "SW2ZIM",   redonly        },
    { "WOOD4",    redonly        }, { "WOODGARG", redonly        }, { "WOODSKUL", redonly        },
    { "ZELDOOR",  redonly        }, { "",         0              }
};

//extern dboolean r_brightmaps;

//
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.
//

//
// R_DrawColumnInCache
// Clip and draw a column
//  from a patch into a cached post.
//
void R_DrawColumnInCache(const column_t *patch, byte *cache, int originy, int cacheheight,
    byte *marks, dboolean oldmethod)
{
    int td;
    int topdelta = -1;
    int lastlength = 0;

    while ((td = patch->topdelta) != 0xFF)
    {
        int     count = patch->length;
        int     position;
        byte    *source = (byte *)patch + 3;

        if (td < topdelta + lastlength - 1)
            topdelta += td;
        else
            topdelta = td;

        position = originy + topdelta;
        lastlength = count;

        if (position < 0)
        {
            count += position;
            if (!oldmethod)
                source -= position;
            position = 0;
        }

        if (position + count > cacheheight)
            count = cacheheight - position;

        if (count > 0)
        {
            memcpy(cache + position, source, count);

            // killough 4/9/98: remember which cells in column have been drawn,
            // so that column can later be converted into a series of posts, to
            // fix the Medusa bug.
            memset(marks + position, 0xFF, count);
        }

        patch = (column_t *)((byte *)patch + lastlength + 4);
    }
}

//
// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
// Rewritten by Lee Killough for performance and to fix Medusa bug
//
static void R_GenerateComposite(int texnum)
{
    byte                *block = Z_Malloc(texturecompositesize[texnum], PU_STATIC,
                            (void **)&texturecomposite[texnum]);
    texture_t           *texture = textures[texnum];

    // Composite the columns together.
    texpatch_t          *patch = texture->patches;
    short               *collump = texturecolumnlump[texnum];
    unsigned int        *colofs = texturecolumnofs[texnum];     // killough 4/9/98: make 32-bit
    int                 i;

    // killough 4/9/98: marks to identify transparent regions in merged textures
    byte                *marks = calloc(texture->width, texture->height);
    byte                *source;

    dboolean             tekwall1 = (texnum == R_CheckTextureNumForName("TEKWALL1"));

    // [crispy] initialize composite background to black (index 0)
    memset(block, 0, texturecompositesize[texnum]);

    for (i = texture->patchcount; --i >= 0; ++patch)
    {
        patch_t         *realpatch = W_CacheLumpNum(patch->patch, PU_CACHE);
        int             x1 = MAX(0, patch->originx);
        int             x2 = MIN(x1 + SHORT(realpatch->width), texture->width);
        const int       *cofs = realpatch->columnofs - x1;

        for (; x1 < x2 ; ++x1)
            // [crispy] generate composites for single-patched textures as well
            //        if (collump[x1] == -1)     // Column has multiple patches?
            // killough 1/25/98, 4/9/98: Fix Medusa bug.
            R_DrawColumnInCache((column_t *)((byte *)realpatch + LONG(cofs[x1])),
                block + colofs[x1], patch->originy, texture->height,
                marks + x1 * texture->height, tekwall1);
    }

    // killough 4/9/98: Next, convert multipatched columns into true columns,
    // to fix Medusa bug while still allowing for transparent regions.
    source = malloc(texture->height);   // temporary column
    for (i = 0; i < texture->width; ++i)
        if (collump[i] == -1)                   // process only multipatched columns
        {
            column_t    *col = (column_t *)(block + colofs[i] - 3);     // cached column
            const byte  *mark = marks + i * texture->height;
            int         j = 0;

            // save column in temporary so we can shuffle it around
            memcpy(source, (byte *)col + 3, texture->height);

            while (1)  // reconstruct the column by scanning transparency marks
            {
                unsigned int    len;                    // killough 12/98

                while (j < texture->height && !mark[j]) // skip transparent cells
                    ++j;

                if (j >= texture->height)               // if at end of column
                {
                    col->topdelta = -1;                 // end-of-column marker
                    break;
                }

                col->topdelta = j;                      // starting offset of post

                // killough 12/98:
                // Use 32-bit len counter, to support tall 1s multipatched textures
                for (len = 0; j < texture->height && mark[j]; ++j)
                    ++len;                              // count opaque cells

                col->length = len; // killough 12/98: intentionally truncate length

                // copy opaque cells from the temporary back into the column
                memcpy((byte *)col + 3, source + col->topdelta, len);
                col = (column_t *)((byte *)col + len + 4); // next post
            }
        }
    free(source);       // free temporary column
    free(marks);        // free transparency marks

    // Now that the texture has been built in column cache,
    // it is purgeable from zone memory.
    Z_ChangeTag(block, PU_CACHE);
}

//
// R_GenerateLookup
//
// Rewritten by Lee Killough for performance and to fix Medusa bug
//
static void R_GenerateLookup(int texnum)
{
    const texture_t     *texture = textures[texnum];

    // Composited texture not created yet.
    short               *collump = texturecolumnlump[texnum];
    unsigned int        *colofs = texturecolumnofs[texnum];
    unsigned int        *colofs2 = texturecolumnofs2[texnum];   // [crispy] original column offsets

    // killough 4/9/98: keep count of posts in addition to patches.
    // Part of fix for Medusa bug for multipatched 2s normals.
    struct
    {
        unsigned int    patches;
        unsigned int    posts;
    } *count = calloc(sizeof(*count), texture->width);

    // killough 12/98: First count the number of patches per column.
    const texpatch_t   *patch = texture->patches;
    int                i = texture->patchcount;

    while (--i >= 0)
    {
        int             pat = patch->patch;
        const patch_t   *realpatch = (patch_t *)W_CacheLumpNum(pat, PU_CACHE);
        int             x1 = MAX(0, (patch++)->originx);
        int             x2 = MIN(x1 + SHORT(realpatch->width), texture->width);
        const int       *cofs = realpatch->columnofs - x1;

        for (; x1 < x2; ++x1)
        {
            count[x1].patches++;
            collump[x1] = pat;
            colofs[x1] = colofs2[x1] = LONG(cofs[x1]) + 3;
        }
    }

    // killough 4/9/98: keep a count of the number of posts in column,
    // to fix Medusa bug while allowing for transparent multipatches.
    //
    // killough 12/98:
    // Post counts are only necessary if column is multipatched,
    // so skip counting posts if column comes from a single patch.
    // This allows arbitrarily tall textures for 1s walls.
    //
    // If texture is >= 256 tall, assume it's 1s, and hence it has
    // only one post per column. This avoids crashes while allowing
    // for arbitrarily tall multipatched 1s textures.
    if (texture->patchcount > 1 && texture->height < 256)
    {
        // killough 12/98: Warn about a common column construction bug
        unsigned int            limit = texture->height * 3 + 3;   // absolute column size limit

        for (i = texture->patchcount, patch = texture->patches; --i >= 0;)
        {
            int                 pat = patch->patch;
            const patch_t       *realpatch = (patch_t *)W_CacheLumpNum(pat, PU_CACHE);
            int                 x1 = MAX(0, (patch++)->originx);
            int                 x2 = MIN(x1 + SHORT(realpatch->width), texture->width);
            const int           *cofs = realpatch->columnofs - x1;

            for (; x1 < x2; ++x1)
                if (count[x1].patches > 1)               // Only multipatched columns
                {
                    const column_t      *col = (column_t *)((byte *)realpatch + LONG(cofs[x1]));
                    const byte          *base = (const byte *)col;

                    // count posts
                    for (; col->topdelta != 0xFF; count[x1].posts++)
                        if ((unsigned int)((byte *)col - base) <= limit)
                            col = (column_t *)((byte *)col + col->length + 4);
                }
        }
    }

    // Now count the number of columns
    //  that are covered by more than one patch.
    // Fill in the lump / offset, so columns
    //  with only a single patch are all done.
    texturecomposite[texnum] = 0;

    {
        int     x = texture->width;
        int     height = texture->height;
        int     csize = 0;                              // killough 10/98

        if (!x)
        {
            C_Printf (CR_GOLD, " R_GenerateLookup: column without a patch (%s)\n",
                    texture->name);
            // [crispy] do not return yet
            /*
            return;
            */
        }

        while (--x >= 0)
        {
            if (count[x].patches > 1)                   // killough 4/9/98
                // [crispy] moved up here, the rest in this loop
                // applies to single-patched textures as well
                collump[x] = -1;                        // mark lump as multipatched

            // killough 1/25/98, 4/9/98:
            //
            // Fix Medusa bug, by adding room for column header
            // and trailer bytes for each post in merged column.
            // For now, just allocate conservatively 4 bytes
            // per post per patch per column, since we don't
            // yet know how many posts the merged column will
            // require, and it's bounded above by this limit.
            colofs[x] = csize + 3;                      // three header bytes in a column

            // killough 12/98: add room for one extra post
            csize += 4 * count[x].posts + 5;            // 1 stop byte plus 4 bytes per post

            csize += height;                            // height bytes of texture data
        }

        texturecompositesize[texnum] = csize;
    }
    free(count);                                        // killough 4/9/98
}

//
// R_GetColumn
//
byte *R_GetColumn(int tex, int col, dboolean opaque)
{
    int lump;

    col &= texturewidthmask[tex];
    lump = texturecolumnlump[tex][col];

    // [crispy] single-patched mid-textures on two-sided walls
    if (lump > 0 && !opaque)
        return ((byte *)W_CacheLumpNum(lump, PU_CACHE) + texturecolumnofs2[tex][col]);

    if (!texturecomposite[tex])
        R_GenerateComposite(tex);

    return (texturecomposite[tex] + texturecolumnofs[tex][col]);
}

static void GenerateTextureHashTable(void)
{
    int         i;

    textures_hashtable = Z_Malloc(sizeof(texture_t *) * numtextures, PU_STATIC, 0);

    memset(textures_hashtable, 0, sizeof(texture_t *) * numtextures);

    // Add all textures to hash table
    for (i = 0; i < numtextures; ++i)
    {
        texture_t       **rover;
        int             key;

        // Store index
        textures[i]->index = i;

        // Vanilla DOOM does a linear search of the textures array
        // and stops at the first entry it finds.  If there are two
        // entries with the same name, the first one in the array
        // wins. The new entry must therefore be added at the end
        // of the hash chain, so that earlier entries win.
        key = W_LumpNameHash(textures[i]->name) % numtextures;

        rover = &textures_hashtable[key];

        while (*rover)
            rover = &(*rover)->next;

        // Hook into hash table
        textures[i]->next = NULL;
        *rover = textures[i];
    }
}

//
// R_DoomTextureHacks
//
void R_DoomTextureHacks(texture_t *t)
{
    if (t->height == 128 && t->patches[0].originy == -8 && t->name[0] == 'S' &&
        t->name[1] == 'K' && t->name[2] == 'Y' && t->name[3] == '1' && t->name[4] == '\0')
        t->patches[0].originy = 0;
    else if (t->height == 128 && t->patches[0].originy == -4 && t->patches[1].originy == -4 &&
        t->name[0] == 'B' && t->name[1] == 'I' && t->name[2] == 'G' && t->name[3] == 'D' &&
        t->name[4] == 'O' && t->name[5] == 'O' && t->name[6] == 'R' && t->name[7] == '7')
        t->patches[0].originy = t->patches[1].originy = 0;
}

//
// R_InitTextures
// Initializes the texture list
//  with the textures from the world map.
//
// [crispy] partly rewritten to merge PNAMES and TEXTURE1/2 lumps
void R_InitTextures(void)
{
    maptexture_t        *mtexture;
    texture_t           *texture;

    int                 i;
    int                 j;
    int                 k;

    int                 *maptex = NULL;

    char                name[9];

    int                 *patchlookup;

    int                 nummappatches;
    int                 maxoff = 0;

    int                 *directory = NULL;

    typedef struct
    {
        int             lumpnum;
        void            *names;
        int             nummappatches;
        short           summappatches;
        char            *name_p;
    } pnameslump_t;

    typedef struct
    {
        int             lumpnum;
        int             *maptex;
        int             maxoff;
        int             numtextures;
        short           sumtextures;
        short           pnamesoffset;
    } texturelump_t;

    pnameslump_t        *pnameslumps = NULL;
    texturelump_t       *texturelumps = NULL;
    texturelump_t       *texturelump;

    int                 maxpnameslumps = 1;     // PNAMES
    int                 maxtexturelumps = 2;    // TEXTURE1, TEXTURE2

    int                 numpnameslumps = 0;
    int                 numtexturelumps = 0;

    int                 temp1;
    int                 temp2;
    int                 temp3;

    // [crispy] allocate memory for the pnameslumps and texturelumps arrays
    pnameslumps = Z_Realloc(pnameslumps, maxpnameslumps * sizeof(*pnameslumps));
    texturelumps = Z_Realloc(texturelumps, maxtexturelumps * sizeof(*texturelumps));

    // [crispy] make sure the first available TEXTURE1/2 lumps
    // are always processed first
    texturelumps[numtexturelumps++].lumpnum = W_GetNumForName("TEXTURE1");
    if ((i = W_CheckNumForName("TEXTURE2")) != -1)
        texturelumps[numtexturelumps++].lumpnum = i;
    else
        texturelumps[numtexturelumps].lumpnum = -1;

    // [crispy] fill the arrays with all available PNAMES lumps
    // and the remaining available TEXTURE1/2 lumps
    nummappatches = 0;
    for (i = numlumps - 1; i >= 0; --i)
    {
        if (!strncasecmp(lumpinfo[i]->name, "PNAMES", 6))
        {
            if (numpnameslumps == maxpnameslumps)
            {
                ++maxpnameslumps;
                pnameslumps = Z_Realloc(pnameslumps, maxpnameslumps * sizeof(*pnameslumps));
            }

            pnameslumps[numpnameslumps].lumpnum = i;
            pnameslumps[numpnameslumps].names =
                W_CacheLumpNum(pnameslumps[numpnameslumps].lumpnum, PU_STATIC);
            pnameslumps[numpnameslumps].nummappatches =
                LONG(*((int *)pnameslumps[numpnameslumps].names));

            // [crispy] accumulated number of patches in the lookup tables
            // excluding the current one
            pnameslumps[numpnameslumps].summappatches = nummappatches;
            pnameslumps[numpnameslumps].name_p = (char *)pnameslumps[numpnameslumps].names + 4;

            // [crispy] calculate total number of patches
            nummappatches += pnameslumps[numpnameslumps].nummappatches;
            ++numpnameslumps;
        }
        else if (!strncasecmp(lumpinfo[i]->name, "TEXTURE", 7))
        {
            // [crispy] support only TEXTURE1/2 lumps, not TEXTURE3 etc.
            if (lumpinfo[i]->name[7] != '1' && lumpinfo[i]->name[7] != '2')
                continue;

            // [crispy] make sure the first available TEXTURE1/2 lumps
            // are not processed again
            if (i == texturelumps[0].lumpnum || i == texturelumps[1].lumpnum)
                continue;

            if (numtexturelumps == maxtexturelumps)
            {
                ++maxtexturelumps;
                texturelumps = Z_Realloc(texturelumps, maxtexturelumps * sizeof(*texturelumps));
            }

            // [crispy] do not proceed any further, yet
            // we first need a complete pnameslumps[] array and need
            // to process texturelumps[0] (and also texturelumps[1]) as well
            texturelumps[numtexturelumps].lumpnum = i;
            ++numtexturelumps;
        }
    }

    // [crispy] fill up the patch lookup table
    name[8] = '\0';
    patchlookup = Z_Malloc(nummappatches * sizeof(*patchlookup), PU_STATIC, NULL);
    for (i = 0, k = 0; i < numpnameslumps; i++)
        for (j = 0; j < pnameslumps[i].nummappatches; j++)
        {
            M_StringCopy(name, pnameslumps[i].name_p + j * 8, sizeof(name));
            patchlookup[k++] = W_CheckNumForName(name);

            if (patchlookup[k] == -1)
                C_Printf(CR_GOLD, " R_InitTextures: Patch %.8s, index %d does not exist", name, k);
        }

    // [crispy] calculate total number of textures
    numtextures = 0;
    for (i = 0; i < numtexturelumps; ++i)
    {
        texturelumps[i].maptex = W_CacheLumpNum(texturelumps[i].lumpnum, PU_STATIC);
        texturelumps[i].maxoff = W_LumpLength(texturelumps[i].lumpnum);
        texturelumps[i].numtextures = LONG(*texturelumps[i].maptex);

        // [crispy] accumulated number of textures in the texture files
        // including the current one
        numtextures += texturelumps[i].numtextures;
        texturelumps[i].sumtextures = numtextures;

        // [crispy] link textures to their own WAD's patch lookup table (if any)
        texturelumps[i].pnamesoffset = 0;
        for (j = 0; j < numpnameslumps; ++j)
            // [crispy] both point to the same WAD file name string?
            if (lumpinfo[texturelumps[i].lumpnum]->wad_file->path ==
                lumpinfo[pnameslumps[j].lumpnum]->wad_file->path)
            {
                texturelumps[i].pnamesoffset = pnameslumps[j].summappatches;
                break;
            }
    }

    // [crispy] release memory allocated for patch lookup tables
    for (i = 0; i < numpnameslumps; ++i)
        W_ReleaseLumpNum(pnameslumps[i].lumpnum);
    free(pnameslumps);

    // [crispy] pointer to (i.e. actually before) the first texture file
    texturelump = texturelumps - 1;     // [crispy] gets immediately increased below

    textures = Z_Malloc(numtextures * sizeof(*textures), PU_STATIC, 0);
    texturecolumnlump = Z_Malloc(numtextures * sizeof(*texturecolumnlump), PU_STATIC, 0);
    texturecolumnofs = Z_Malloc(numtextures * sizeof(*texturecolumnofs), PU_STATIC, 0);
    texturecolumnofs2 = Z_Malloc(numtextures * sizeof(*texturecolumnofs2), PU_STATIC, 0);
    texturecomposite = Z_Malloc(numtextures * sizeof(*texturecomposite), PU_STATIC, 0);
    texturecompositesize = Z_Malloc(numtextures * sizeof(*texturecompositesize), PU_STATIC, 0);
    texturewidthmask = Z_Malloc(numtextures * sizeof(*texturewidthmask), PU_STATIC, 0);
    textureheight = Z_Malloc(numtextures * sizeof(*textureheight), PU_STATIC, 0);
    texturefullbright = Z_Malloc(numtextures * sizeof(*texturefullbright), PU_STATIC, 0);

    // Really complex printing shit...
    temp1 = W_GetNumForName ("S_START");  // P_???????
    temp2 = W_GetNumForName ("S_END") - 1;
    temp3 = ((temp2-temp1+63)/64) + ((numtextures+63)/64);

    if(fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
           fsize != 10396254 && fsize != 10399316)
    {
        printf("[");

//        for (i = 0; i < temp3; i++)
        for (i = 0; i < temp3 - 2; i++)
            printf(" ");
        printf("         ]");
//        for (i = 0; i < temp3; i++)
        for (i = 0; i < temp3 - 2; i++)
            printf("\x8");
        printf("\x8\x8\x8\x8\x8\x8\x8\x8\x8\x8");        
    }

    for (i = 0; i < numtextures; ++i, ++directory)
    {
        mappatch_t      *mpatch;
        texpatch_t      *patch;
        int             offset;

        if (!(i&63))
            printf (".");

        if (!i || i == texturelump->sumtextures)
        {
            // [crispy] start looking in next texture file
            texturelump++;
            maptex = texturelump->maptex;
            maxoff = texturelump->maxoff;
            directory = maptex + 1;
        }

        offset = LONG(*directory);

        if (offset > maxoff)
            I_Error("R_InitTextures: bad texture directory");

        mtexture = (maptexture_t *)((byte *)maptex + offset);

        texture = textures[i] = Z_Malloc(sizeof(texture_t) + sizeof(texpatch_t)
            * (SHORT(mtexture->patchcount) - 1), PU_STATIC, 0);

        texture->width = SHORT(mtexture->width);
        texture->height = SHORT(mtexture->height);
        texture->patchcount = SHORT(mtexture->patchcount);

        memcpy(texture->name, mtexture->name, sizeof(texture->name));
        mpatch = &mtexture->patches[0];
        patch = &texture->patches[0];

        for (j = 0; j < texture->patchcount; ++j, ++mpatch, ++patch)
        {
            short       p;

            patch->originx = SHORT(mpatch->originx);
            patch->originy = SHORT(mpatch->originy);

            // [crispy] apply offset for patches not in the
            // first available patch offset table
            p = SHORT(mpatch->patch) + texturelump->pnamesoffset;

            // [crispy] catch out-of-range patches
            if (p < nummappatches)
                patch->patch = patchlookup[p];
            if (patch->patch == -1 || p >= nummappatches)
            {
                char    texturename[9];

                texturename[8] = '\0';
                memcpy(texturename, texture->name, 8);

                // [crispy] make non-fatal
                C_Printf(CR_GOLD, " R_InitTextures: The %s texture has a missing patch.", texturename);
                patch->patch = 0;
            }
        }
        texturecolumnlump[i] = Z_Malloc(texture->width * sizeof(**texturecolumnlump), PU_STATIC, 0);
        texturecolumnofs[i] = Z_Malloc(texture->width * sizeof(**texturecolumnofs), PU_STATIC, 0);
        texturecolumnofs2[i] = Z_Malloc(texture->width * sizeof(**texturecolumnofs2), PU_STATIC, 0);

        j = 1;
        while (j * 2 <= texture->width)
            j <<= 1;

        texturewidthmask[i] = j - 1;
        textureheight[i] = texture->height << FRACBITS;

        R_DoomTextureHacks(texture);
    }

    Z_Free(patchlookup);

    // [crispy] release memory allocated for texture files
    for (i = 0; i < numtexturelumps; ++i)
        W_ReleaseLumpNum(texturelumps[i].lumpnum);
    free(texturelumps);

    // Precalculate whatever possible.
    for (i = 0; i < numtextures; i++)
        R_GenerateLookup(i);

    // Create translation table for global animation.
    texturetranslation = Z_Malloc((numtextures + 1) * sizeof(*texturetranslation), PU_STATIC, 0);

    for (i = 0; i<numtextures; i++)
        texturetranslation[i] = i;

    GenerateTextureHashTable();

    // [BH] Initialize partially fullbright textures.
    memset(texturefullbright, 0, numtextures * sizeof(*texturefullbright));
    if (d_brightmaps)
    {
        i = 0;
        while (fullbright[i].colormask)
        {
            if (fullbright[i].texture)
            {
                int num = R_CheckTextureNumForName(fullbright[i].texture);

                if (num != -1)
                    texturefullbright[num] = fullbright[i].colormask;
                i++;
            }
        }
    }

}

//
// R_InitFlats
//
void R_InitFlats(void)
{
    int i;

    firstflat = W_GetNumForName("F_START") + 1;
    lastflat = W_GetNumForName("F_END") - 1;
    numflats = lastflat - firstflat + 1;

    // Create translation table for global animation.
    flattranslation = Z_Malloc((numflats + 1) * sizeof(*flattranslation), PU_STATIC, 0);

    for (i = 0; i < numflats; i++)
        flattranslation[i] = i;
}

//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//
void R_InitSpriteLumps(void)
{
    int i;

    firstspritelump = W_GetNumForName("S_START") + 1;
    lastspritelump = W_GetNumForName("S_END") - 1;

    numspritelumps = lastspritelump - firstspritelump + 1;
    spritewidth = Z_Malloc(numspritelumps * sizeof(*spritewidth), PU_STATIC, 0);
    spriteheight = Z_Malloc(numspritelumps * sizeof(*spriteheight), PU_STATIC, 0);
    spriteoffset = Z_Malloc(numspritelumps * sizeof(*spriteoffset), PU_STATIC, 0);
    spritetopoffset = Z_Malloc(numspritelumps * sizeof(*spritetopoffset), PU_STATIC, 0);

    newspriteoffset = Z_Malloc(numspritelumps * sizeof(*newspriteoffset), PU_STATIC, 0);
    newspritetopoffset = Z_Malloc(numspritelumps * sizeof(*newspritetopoffset), PU_STATIC, 0);

    for (i = 0; i < numspritelumps; i++)
    {
        patch_t *patch = W_CacheLumpNum(firstspritelump + i, PU_CACHE);

        if (!(i&63))
            printf (".");

        if (patch)
        {
            spritewidth[i] = SHORT(patch->width) << FRACBITS;
            spriteheight[i] = SHORT(patch->height) << FRACBITS;
            spriteoffset[i] = newspriteoffset[i] = SHORT(patch->leftoffset) << FRACBITS;
            spritetopoffset[i] = newspritetopoffset[i] = SHORT(patch->topoffset) << FRACBITS;

            // [BH] override sprite offsets in WAD with those in sproffsets[] in info.c
            if (d_fixspriteoffsets && fsize != 28422764 && fsize != 19321722)
            {
                int j = 0;

                while (sproffsets[j].name[0])
                {
                    if (i == W_CheckNumForName(sproffsets[j].name) - firstspritelump
                        && spritewidth[i] == (SHORT(sproffsets[j].width) << FRACBITS)
                        && spriteheight[i] == (SHORT(sproffsets[j].height) << FRACBITS))
                    {
                        newspriteoffset[i] = SHORT(sproffsets[j].x) << FRACBITS;
                        newspritetopoffset[i] = SHORT(sproffsets[j].y) << FRACBITS;
                        break;
                    }
                    j++;
                }
            }
        }
    }

    if (fsize == 28422764) // FREEDOOM
    {
        states[S_BAR1].tics = 0;
        mobjinfo[MT_BARREL].spawnstate = S_BAR2;
        mobjinfo[MT_BARREL].frames = 0;
        mobjinfo[MT_BETABARREL].spawnstate = S_BAR2;
        mobjinfo[MT_BETABARREL].frames = 0;
        mobjinfo[MT_HEAD].blood = MT_BLOOD;
        mobjinfo[MT_BETAHEAD].blood = MT_BLOOD;
        mobjinfo[MT_BRUISER].blood = MT_BLOOD;
        mobjinfo[MT_BETABRUISER].blood = MT_BLOOD;
        mobjinfo[MT_KNIGHT].blood = MT_BLOOD;
    }
    else if (fsize == 12361532) // CHEX
    {
        states[S_BETAPOSS_DIE5].tics = 0;
        states[S_POSS_DIE5].tics = 0;
        states[S_POSS_XDIE9].tics = 0;
        states[S_BETASPOS_DIE5].tics = 0;
        states[S_SPOS_DIE5].tics = 0;
        states[S_SPOS_XDIE9].tics = 0;
        states[S_TROO_DIE5].tics = 0;
        states[S_TROO_XDIE8].tics = 0;
        states[S_SARG_DIE6].tics = 0;
        states[S_BOSS_DIE7].tics = 0;
    }
    else if (fsize == 19321722) // HACX
    {
        mobjinfo[MT_HEAD].flags2 |= MF2_DONOTMAP;
        mobjinfo[MT_BETAHEAD].flags2 |= MF2_DONOTMAP;
        mobjinfo[MT_BETAINV].flags2 &= ~MF2_TRANSLUCENT_33;
        mobjinfo[MT_BETAINS].flags2 &= ~(MF2_TRANSLUCENT_33 | MF2_FLOATBOB | MF2_NOFOOTCLIP);
        mobjinfo[MT_INV].flags2 &= ~MF2_TRANSLUCENT_33;
        mobjinfo[MT_INS].flags2 &= ~(MF2_TRANSLUCENT_33 | MF2_FLOATBOB | MF2_NOFOOTCLIP);
        mobjinfo[MT_MISC14].flags2 &= ~(MF2_FLOATBOB | MF2_NOFOOTCLIP);
    }
}

//
// R_InitTranMap
//
// [crispy] Initialize translucency filter map
// based in parts on the implementation from boom202s/R_DATA.C:676-787
//
/*
void R_InitTranMap()
{
    int lump = W_CheckNumForName("TRANMAP");

    // If a tranlucency filter map lump is present, use it
    if (lump != -1)
    {
        // Set a pointer to the translucency filter maps.
        tranmap = W_CacheLumpNum(lump, PU_STATIC);
        printf(":"); // [crispy] loaded from a lump
    }
    else
    {
        // Compose a default transparent filter map based on PLAYPAL.
        unsigned char *playpal = W_CacheLumpName("PLAYPAL", PU_STATIC);
        char *fname = NULL;
        extern char *configdir;

        struct
        {
            unsigned char pct;
            unsigned char playpal[256*3]; // [crispy] a palette has 768 bytes!

        } cache;

        // [crispy] open file readable
        FILE *cachefp = fopen(fname = M_StringJoin(configdir, "tranmap.dat", NULL), "r+b");

        tranmap = Z_Malloc(256*256, PU_STATIC, 0);

        // Use cached translucency filter if it's available

        // [crispy] if file not readable, open writable, continue

        if (!cachefp ? cachefp = fopen(fname,"wb") , 1 : 

            // [crispy] could not read struct cache from file

            fread(&cache, 1, sizeof cache, cachefp) != sizeof cache ||

            // [crispy] filter percents differ

            cache.pct != tran_filter_pct ||

            // [crispy] base palettes differ

            memcmp(cache.playpal, playpal, sizeof cache.playpal) ||

            // [crispy] could not read entire translucency map

            fread(tranmap, 256, 256, cachefp) != 256 )
        {
            byte *fg, *bg, blend[3], *tp = tranmap;
            int i, j, btmp;

            // [crispy] background color
            for (i = 0; i < 256; i++)
            {
                // [crispy] foreground color
                for (j = 0; j < 256; j++)
                {
                    // [crispy] shortcut: identical foreground and background
                    if (i == j)
                    {
                        *tp++ = i;
                        continue;
                    }

                    bg = playpal + 3*i;
                    fg = playpal + 3*j;

                    // [crispy] blended color - emphasize blues
                    // Colour matching in RGB space doesn't work very well with the blues
                    // in Doom's palette. Rather than do any colour conversions, just
                    // emphasize the blues when building the translucency table.
                    btmp = fg[b] < (fg[r] + fg[g]) ? 0 : (fg[b] - (fg[r] + fg[g])) / 2;

                    blend[r] = (tran_filter_pct * fg[r] +
                            (100 - tran_filter_pct) * bg[r]) / (100 + btmp);

                    blend[g] = (tran_filter_pct * fg[g] +
                            (100 - tran_filter_pct) * bg[g]) / (100 + btmp);

                    blend[b] = (tran_filter_pct * fg[b] +
                            (100 - tran_filter_pct) * bg[b]) / 100;

                    *tp++ = FindNearestColor(playpal, blend[r], blend[g], blend[b]);
                }
            }

            // write out the cached translucency map
            if (cachefp)
            {
                // [crispy] set filter percents

                cache.pct = tran_filter_pct;

                // [crispy] set base palette

                memcpy(cache.playpal, playpal, sizeof cache.playpal);

                // [crispy] go to start of file

                fseek(cachefp, 0, SEEK_SET);

                // [crispy] write struct cache

                fwrite(&cache, 1, sizeof cache, cachefp);

                // [crispy] write translucency map

                fwrite(tranmap, 256, 256, cachefp);

                // [crispy] generated and saved

                printf("!");
            }
            else
                // [crispy] generated, but not saved

                printf("?");
        }
        else
            // [crispy] loaded from a file

            printf(".");

        if (cachefp)
            fclose(cachefp);

        free(fname);

        Z_ChangeTag(playpal, PU_CACHE);
    }
}
*/
//
// R_InitColormaps
//
// killough 3/20/98: rewritten to allow dynamic colormaps
// and to remove unnecessary 256-byte alignment
//
// killough 4/4/98: Add support for C_START/C_END markers
//
byte grays[256];

void R_InitColormaps(void)
{
    dboolean keepgray = false;
    dboolean    COLORMAP = (W_CheckMultipleLumps("COLORMAP") > 1);
    int        i, j, k;
    byte       *palsrc, *palette, *playpal;
    char       c[3];
    wad_file_t  *colormapwad;

    if (W_CheckNumForName("C_START") >= 0 && W_CheckNumForName("C_END") >= 0)
    {
        firstcolormaplump = W_GetNumForName("C_START");
        lastcolormaplump = W_GetNumForName("C_END");
        numcolormaps = lastcolormaplump - firstcolormaplump;

        colormaps = Z_Malloc(sizeof(*colormaps) * numcolormaps, PU_STATIC, 0);

        colormaps[0] = W_CacheLumpName("COLORMAP", PU_STATIC);

        for (i = 1; i < numcolormaps; i++)
            colormaps[i] = W_CacheLumpNum(i + firstcolormaplump, PU_STATIC);
    }
    else
    {
        colormaps = Z_Malloc(sizeof(*colormaps), PU_STATIC, 0);
        colormaps[0] = W_CacheLumpName("COLORMAP", PU_STATIC);
    }
    colormapwad = lumpinfo[W_CheckNumForName("COLORMAP")]->wad_file;
    C_Printf(CR_GRAY, " R_InitColormaps: Using the COLORMAP lump in %s file %s.",
        (colormapwad->type == IWAD ? "IWAD" : "PWAD"), uppercase(colormapwad->path));

    // [BH] There's a typo in dcolors.c, the source code of the utility Id
    // Software used to construct the palettes and colormaps for DOOM (see
    // http://www.doomworld.com/idgames/?id=16644). When constructing colormap
    // 32, which is used for the invulnerability powerup, the traditional
    // Y luminance values are used (see http://en.wikipedia.org/wiki/YIQ), but a
    // value of 0.144 is used when it should be 0.114. So I've grabbed the
    // offending code from dcolor.c, corrected it, put it here, and now colormap
    // 32 is manually calculated rather than grabbing it from the colormap lump.
    // The resulting differences are minor.
    palsrc = palette = W_CacheLumpName("PLAYPAL", PU_CACHE);

    for (i = 0; i < 255; i++)
    {
        float       red = *palsrc++ / 256.0f;
        float       green = *palsrc++ / 256.0f;
        float       blue = *palsrc++ / 256.0f;
        float       gray = red * 0.299f + green * 0.587f + blue * 0.114f/*0.144f*/;

        grays[i] = FindNearestColor(palette, (int)(gray * 255.0f),
            (int)(gray * 255.0f), (int)(gray * 255.0f));

        if (!COLORMAP)
        {
            gray = (1.0f - gray) * 255.0f;
            colormaps[0][32 * 256 + i] = FindNearestColor(palette, (int)gray, (int)gray, (int)gray);
        }
    }

    // [crispy] initialize color translation and color strings tables
    playpal = W_CacheLumpName("PLAYPAL", PU_STATIC);

    if (!crstr)
        crstr = malloc(CRXMAX * sizeof(*crstr));

    // [crispy] check for status bar graphics replacements
    i = W_CheckNumForName("sttnum0"); // [crispy] Status Bar '0'
    keepgray = (i >= 0 && !strcmp(lumpinfo[i]->wad_file->path, M_BaseName(iwadfile)));

    for (j = 0; j < CRXMAX; j++)
    {
        for (k = 0; k < 256; k++)
        {
            crx[j][k] = V_Colorize(playpal, j, k, keepgray);
        }

        M_snprintf(c, sizeof(c), "\x1b%c", '0' + j);
        crstr[j] = M_StringDuplicate(c);
    }
    Z_ChangeTag(playpal, PU_CACHE);
}

// killough 4/4/98: get colormap number from name
// killough 4/11/98: changed to return -1 for illegal names
int R_ColormapNumForName(char *name)
{
    register int i = 0;

    if (numcolormaps == 1)
        return -1;

    if (strncasecmp(name, "COLORMAP", 8))     // COLORMAP predefined to return 0
        if ((i = W_CheckNumForName(name)) != -1)
            i -= firstcolormaplump;
    return i;
}

//
// R_InitData
// Locates all the lumps
//  that will be used by all views
// Must be called after W_Init.
//
void R_InitData(void)
{
    R_InitTextures();
    R_InitFlats();
    R_InitSpriteLumps();
//    R_InitTranMap(); // [crispy] prints a mark itself
    R_InitColormaps();
}

//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName(char *name)
{
    int  i;

    if(beta_style && gamemode != shareware && gamemode != commercial)
    {
        if (name[0] == 'F' &&
            name[1] == 'L' &&
            name[2] == 'A' &&
            name[3] == 'T' &&
            name[4] == '2' &&
            name[5] == '2')
            name = "BFLAT22";

        else if (name[0] == 'D' &&
            name[1] == 'E' &&
            name[2] == 'M' &&
            name[3] == '1' &&
            name[4] == '_' &&
            name[5] == '5')
            name = "BDEM1_5";
    }

    i = W_RangeCheckNumForName(firstflat, lastflat, name);

    if (i == -1)
    {
        // [crispy] make non-fatal
        if(replace_missing)
        {
            i = W_CheckNumForName ("FLOOR0_1");
            C_Printf(CR_RED, " R_FlatNumForName: %.8s not found (replaced with FLOOR0_1)", name);
        }
        else
        {
            C_Printf(CR_RED, " R_FlatNumForName: %.8s not found");
            return 0;
//        return skyflatnum;
        }
    }
    return (i - firstflat);
}

//
// R_CheckFlatNumForName
// Retrieval, get a flat number for a flat name. No error.
//
int R_CheckFlatNumForName(char *name)
{
    int i;

    for (i = firstflat; i <= lastflat; i++)
        if (!strncasecmp(lumpinfo[i]->name, name, 8))
            return (i - firstflat);

    return -1;
}

//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
int R_CheckTextureNumForName(char *name)
{
    texture_t   *texture;
    int         key;

    // "NoTexture" marker.
    if (name[0] == '-')
        return 0;

    key = W_LumpNameHash(name) % numtextures;

    texture = textures_hashtable[key];

    while (texture)
    {
        if (!strncasecmp(texture->name, name, 8))
            return texture->index;

        texture = texture->next;
    }

    return -1;
}

//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//  aborts with error message.
//
int R_TextureNumForName(char *name)
{
    int i;

    if(beta_style && gamemode != shareware && gamemode != commercial)
    {
        if (name[0] == 'A' &&
            name[1] == 'A' &&
            name[2] == 'S' &&
            name[3] == 'T' &&
            name[4] == 'I' &&
            name[5] == 'N' &&
            name[6] == 'K' &&
            name[7] == 'Y')
            i = R_CheckTextureNumForName ("BASTINKY");

        else if (name[0] == 'C' &&
            name[1] == 'O' &&
            name[2] == 'M' &&
            name[3] == 'P' &&
            name[4] == 'U' &&
            name[5] == 'T' &&
            name[6] == 'E' &&
            name[7] == '2')
            i = R_CheckTextureNumForName ("BCMPUTE2");

        else if (name[0] == 'B' &&
            name[1] == 'I' &&
            name[2] == 'G' &&
            name[3] == 'D' &&
            name[4] == 'O' &&
            name[5] == 'O' &&
            name[6] == 'R' &&
            name[7] == '2')
            i = R_CheckTextureNumForName ("BDOOR102");

        else if (name[0] == 'M' &&
            name[1] == 'A' &&
            name[2] == 'R' &&
            name[3] == 'B' &&
            name[4] == 'F' &&
            name[5] == 'A' &&
            name[6] == 'C' &&
            name[7] == 'E')
            i = R_CheckTextureNumForName ("BMWALL41");

        else if (name[0] == 'S' &&
            name[1] == 'K' &&
            name[2] == 'Y' &&
            name[3] == '1')
            i = R_CheckTextureNumForName ("BSKY1");

        else if (name[0] == 'S' &&
            name[1] == 'T' &&
            name[2] == 'E' &&
            name[3] == 'P' &&
            name[4] == '4')
            i = R_CheckTextureNumForName ("BSTEP4");

        else if (name[0] == 'S' &&
            name[1] == 'W' &&
            name[2] == '1' &&
            name[3] == 'D' &&
            name[4] == 'I' &&
            name[5] == 'R' &&
            name[6] == 'T')
            i = R_CheckTextureNumForName ("BSW1DIRT");

        else if (name[0] == 'S' &&
            name[1] == 'W' &&
            name[2] == '2' &&
            name[3] == 'D' &&
            name[4] == 'I' &&
            name[5] == 'R' &&
            name[6] == 'T')
            i = R_CheckTextureNumForName ("BSW2DIRT");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '5' &&
            name[5] == '7' &&
            name[6] == '_' &&
            name[7] == '2')
            i = R_CheckTextureNumForName ("BWALL572");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '5' &&
            name[5] == '7' &&
            name[6] == '_' &&
            name[7] == '3')
            i = R_CheckTextureNumForName ("BWALL573");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '5' &&
            name[5] == '7' &&
            name[6] == '_' &&
            name[7] == '4')
            i = R_CheckTextureNumForName ("BWALL574");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '6' &&
            name[5] == '3' &&
            name[6] == '_' &&
            name[7] == '1')
            i = R_CheckTextureNumForName ("BWALL631");

        else if (name[0] == 'W' &&
            name[1] == 'A' &&
            name[2] == 'L' &&
            name[3] == 'L' &&
            name[4] == '6' &&
            name[5] == '3' &&
            name[6] == '_' &&
            name[7] == '2')
            i = R_CheckTextureNumForName ("BWALL632");

        else
            i = R_CheckTextureNumForName (name);
    }
    else
        i = R_CheckTextureNumForName (name);

    if (i == -1)
    {
        // [crispy] make non-fatal
        if(replace_missing)
        {
            i = R_CheckTextureNumForName ("BROWN1");
            C_Printf(CR_RED, " R_TextureNumForName: %.8s not found (replaced with BROWN1)", name);
        }
        else
        {
            C_Printf(CR_RED, " R_TextureNumForName: %.8s not found");
            return 0;
        }
    }
    return i;
}

//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
void R_PrecacheLevel(void)
{
    byte        *hitlist;
    thinker_t   *th;
    int         i;
    int         j;
    int         k;

    hitlist = malloc(MAX(numtextures, MAX(numflats, numsprites)));

    // Precache flats.
    memset(hitlist, 0, numflats);

    for (i = 0; i < numsectors; i++)
    {
        hitlist[sectors[i].floorpic] = 1;
        hitlist[sectors[i].ceilingpic] = 1;
    }

    for (i = 0; i < numflats; i++)
        if (hitlist[i])
            W_CacheLumpNum(firstflat + i, PU_CACHE);

    // Precache textures.
    memset(hitlist, 0, numtextures);

    for (i = 0; i < numsides; i++)
    {
        hitlist[sides[i].toptexture] = 1;
        hitlist[sides[i].midtexture] = 1;
        hitlist[sides[i].bottomtexture] = 1;
    }

    // Sky texture is always present.
    // Note that F_SKY1 is the name used to
    //  indicate a sky floor/ceiling as a flat,
    //  while the sky texture is stored like
    //  a wall texture, with an episode dependent
    //  name.
    hitlist[skytexture] = 1;

    for (i = 0; i < numtextures; i++)
        if (hitlist[i])
        {
            texture_t       *texture = textures[i];

            for (j = 0; j < texture->patchcount; j++)
                W_CacheLumpNum(texture->patches[j].patch, PU_CACHE);
        }

    // Precache sprites.
    memset(hitlist, 0, numsprites);

    for (th = thinkerclasscap[th_mobj].cnext; th != &thinkerclasscap[th_mobj]; th = th->cnext)
        hitlist[((mobj_t *)th)->sprite] = 1;

    for (i = 0; i < numsprites; i++)
        if (hitlist[i])
            for (j = 0; j < sprites[i].numframes; j++)
            {
                short   *lump = sprites[i].spriteframes[j].lump;

                for (k = 0; k < 8; k++)
                    W_CacheLumpNum(firstspritelump + lump[k], PU_CACHE);
            }

    free(hitlist);
}
