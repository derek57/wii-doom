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


#include "c_io.h"
#include "d_deh.h"
#include "doomstat.h"
#include "i_scale.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_misc.h"
#include "p_local.h"
#include "p_tick.h"
#include "r_patch.h"
#include "r_sky.h"
#include "v_patch.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "wii-doom.h"
#include "z_zone.h"


// Generate shading ramps for lighting
#define PALETTEB_SHADE      (0)
#define PALETTEF_SHADE      (1 << PALETTEB_SHADE)

// Apply blend color specified in V_SetBlend()
#define PALETTEB_BLEND      (1)
#define PALETTEF_BLEND      (1 << PALETTEB_SHADE)

// Default palette when none is specified (Do not set directly!)
#define PALETTEB_DEFAULT    (30)
#define PALETTEF_DEFAULT    (1 << PALETTEB_DEFAULT)

#define MAKERGB(r, g, b)    (((r) << 16) | ((g) << 8) | (b))
#define APART(c)            (((c) >> 24) & 0xff)
#define RPART(c)            (((c) >> 16) & 0xff)
#define GPART(c)            (((c) >> 8) & 0xff)
#define BPART(c)            ((c) & 0xff)


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

static byte greenonly3[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
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
    { "SW2MARB",  greenonly1     }, { "SW2MET2",  greenonly1     }, { "SW2METAL", greenonly3     }, 
    { "SW2MOD1",  notgrayorbrown }, { "SW2PANEL", redonly        }, { "SW2ROCK",  redonly        },
    { "SW2SLAD",  redonly        }, { "SW2STARG", greenonly1     }, { "SW2STON1", greenonly1     },
    { "SW2STON2", greenonly1     }, { "SW2STON6", redonly        }, { "SW2STONE", greenonly1     },
    { "SW2STRTN", greenonly1     }, { "SW2TEK",   greenonly1     }, { "SW2VINE",  greenonly1     },
    { "SW2WDMET", redonly        }, { "SW2WOOD",  redonly        }, { "SW2ZIM",   redonly        },
    { "WOOD4",    redonly        }, { "WOODGARG", redonly        }, { "WOODSKUL", redonly        },
    { "ZELDOOR",  redonly        }, { "",         0              }
};


//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

// for global animation
int             *flattranslation;
int             *texturetranslation;

// filter percent
//int             tran_filter_pct = 66;

// killough 4/17/98: make firstcolormaplump, lastcolormaplump external
int             firstcolormaplump;
int             lastcolormaplump;
int             firstflat;
int             lastflat;
int             numflats;
int             firstspritelump;
int             lastspritelump;
int             numspritelumps;
int             numtextures;

texture_t       **textures;

// needed for texture pegging
fixed_t         *textureheight;

// needed for prerendering
fixed_t         *spritewidth;
fixed_t         *spriteheight;
fixed_t         *spriteoffset;
fixed_t         *spritetopoffset;
fixed_t         *newspriteoffset;
fixed_t         *newspritetopoffset;

byte            **texturefullbright;
byte            newgamma[256];
byte            grays[256];


extern char     *iwadfile;


// R_GetTextureColumn 
//
byte *R_GetTextureColumn(rpatch_t *texpatch, int col) 
{
    while (col < 0)
        col += texpatch->width;

    col &= texpatch->widthmask;

    return texpatch->columns[col].pixels; 
}

//
// R_InitTextures
// Initializes the texture list
//  with the textures from the world map.
//
void R_InitTextures(void)
{
    const maptexture_t  *mtexture; 
    texture_t           *texture;

    int                 i, j, k;
    int                 maptex_lump[2] = { -1, -1 };
    const int           *maptex1;
    const int           *maptex2;

    char                name[9];

    // cph - new wad lump handling
    int                 names_lump;

    // cph - const*'s
    const char          *names;
    const char          *name_p;

    int                 *patchlookup;

    int                 nummappatches;
    int                 maxoff, maxoff2;
    int                 numtextures1, numtextures2;
    const int           *directory;

    int                 temp1;
    int                 temp2;
    int                 temp3;

    // Load the patch names from pnames.lmp.
    name[8] = '\0';
    names = W_CacheLumpNum(names_lump = W_GetNumForName("PNAMES"), PU_STATIC);
    nummappatches = LONG(*((const int *)names));
    name_p = names + 4;

    // killough
    patchlookup = malloc(nummappatches * sizeof(*patchlookup));

    for (i = 0, k = 0; i < nummappatches; i++)
    {
        strncpy(name, name_p + i * 8, 8);
        patchlookup[k++] = W_CheckNumForName(name);

        if (patchlookup[k] == -1)
            C_Warning("R_InitTextures: Patch %.8s, index %d does not exist", name, k);
    }

    // cph - release the lump 
    W_ReleaseLumpNum(names_lump);

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    maptex_lump[0] = W_GetNumForName("TEXTURE1");
    maptex1 = W_CacheLumpNum(maptex_lump[0], PU_STATIC);
    numtextures1 = LONG(*maptex1);
    maxoff = W_LumpLength(maptex_lump[0]);
    directory = maptex1 + 1;

    if (W_CheckNumForName("TEXTURE2") != -1)
    {
        maptex_lump[1] = W_GetNumForName("TEXTURE2");
        maptex2 = W_CacheLumpNum(maptex_lump[1], PU_STATIC);
        numtextures2 = LONG(*maptex2);
        maxoff2 = W_LumpLength(maptex_lump[1]);
    }
    else
    {
        maptex2 = NULL;
        numtextures2 = 0;
        maxoff2 = 0;
    }

    numtextures = numtextures1 + numtextures2;

    // killough 4/9/98: make column offsets 32-bit;
    // clean up malloc-ing to use sizeof
    textures = Z_Malloc(numtextures * sizeof(*textures), PU_STATIC, 0);
    textureheight = Z_Malloc(numtextures * sizeof(*textureheight), PU_STATIC, 0);

    // Really complex printing shit...
    // P_???????
    temp1 = W_GetNumForName ("S_START");
    temp2 = W_GetNumForName ("S_END") - 1;
    temp3 = ((temp2-temp1+63)/64) + ((numtextures+63)/64);

    if (fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
           fsize != 10396254 && fsize != 10399316)
    {
        printf("[");

        for (i = 0; i < temp3 - 2; i++)
            printf(" ");

        printf("         ]");

        for (i = 0; i < temp3 - 2; i++)
            printf("\x8");

        printf("\x8\x8\x8\x8\x8\x8\x8\x8\x8\x8");        
    }

    for (i = 0; i < numtextures; ++i, ++directory)
    {
        const mappatch_t        *mpatch;
        texpatch_t              *patch;
        int                     offset;

        if (!(i & 63))
            if (!beta_style)
                printf(".");

        if (i == numtextures1) 
        {
            // Start looking in second texture file.
            maptex1 = maptex2;
            maxoff = maxoff2;
            directory = maptex1 + 1;
        }

        offset = LONG(*directory);

        if (offset > maxoff)
            I_Error("R_InitTextures: bad texture directory");

        mtexture = (const maptexture_t *)((const byte *)maptex1 + offset); 

        texture = textures[i] = Z_Malloc(sizeof(texture_t) + sizeof(texpatch_t)
            * (SHORT(mtexture->patchcount) - 1), PU_STATIC, 0); 

        texture->width = SHORT(mtexture->width);
        texture->height = SHORT(mtexture->height);
        texture->patchcount = SHORT(mtexture->patchcount);

        {
            size_t      j;

            for (j = 0; j < sizeof(texture->name); ++j)
                texture->name[j] = mtexture->name[j];
        }

        mpatch = mtexture->patches;
        patch = texture->patches;

        for (j = 0; j < texture->patchcount; ++j, ++mpatch, ++patch)
        {
            patch->originx = SHORT(mpatch->originx);
            patch->originy = SHORT(mpatch->originy);
            patch->patch = patchlookup[SHORT(mpatch->patch)];

            if (patch->patch == -1)
                C_Warning("R_InitTextures: Missing patch %d in texture %.8s.", SHORT(mpatch->patch),
                    // killough 4/17/98
                    texture->name);
        }

        for (j = 1; j * 2 <= texture->width; j <<= 1);

        texture->widthmask = j - 1;
        textureheight[i] = texture->height << FRACBITS;
    }

    free(patchlookup);

    // cph - release the TEXTUREx lumps
    for (i = 0; i < 2; ++i)
        if (maptex_lump[i] != -1)
            W_ReleaseLumpNum(maptex_lump[i]);

    // Create translation table for global animation.
    // killough 4/9/98: make column offsets 32-bit;
    // clean up malloc-ing to use sizeof
    texturetranslation = Z_Malloc((numtextures + 1) * sizeof(*texturetranslation), PU_STATIC, 0);

    for (i = 0; i < numtextures; ++i) 
        texturetranslation[i] = i;

    // killough 1/31/98: Initialize texture hash table
    for (i = 0; i < numtextures; ++i)
        textures[i]->index = -1;

    while (--i >= 0)
    {
        int     j = W_LumpNameHash(textures[i]->name) % (unsigned int)numtextures;

        // Prepend to chain
        textures[i]->next = textures[j]->index;
        textures[j]->index = i;
    }

    // [BH] Initialize partially fullbright textures.
    texturefullbright = Z_Malloc(numtextures * sizeof(*texturefullbright), PU_STATIC, 0);

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
    flattranslation = Z_Malloc((numflats + 1) * sizeof(*flattranslation), PU_STATIC, NULL);

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
    spritewidth = Z_Malloc(numspritelumps * sizeof(*spritewidth), PU_STATIC, NULL);
    spriteheight = Z_Malloc(numspritelumps * sizeof(*spriteheight), PU_STATIC, NULL);
    spriteoffset = Z_Malloc(numspritelumps * sizeof(*spriteoffset), PU_STATIC, NULL);
    spritetopoffset = Z_Malloc(numspritelumps * sizeof(*spritetopoffset), PU_STATIC, NULL);

    newspriteoffset = Z_Malloc(numspritelumps * sizeof(*newspriteoffset), PU_STATIC, NULL);
    newspritetopoffset = Z_Malloc(numspritelumps * sizeof(*newspritetopoffset), PU_STATIC, NULL);

    for (i = 0; i < numspritelumps; i++)
    {
        patch_t *patch = W_CacheLumpNum(firstspritelump + i, PU_CACHE);

        if (!(i & 63))
            if (!beta_style)
                printf(".");

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

                while (*sproffsets[j].name)
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

    // FREEDOOM
    if (fsize == 28422764)
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
    // CHEX
    else if (fsize == 12361532)
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
    // HACX
    else if (fsize == 19321722)
    {
        mobjinfo[MT_HEAD].flags2 |= MF2_DONOTMAP;
        mobjinfo[MT_BETAHEAD].flags2 |= MF2_DONOTMAP;
        mobjinfo[MT_BETAINV].flags2 &= ~MF2_TRANSLUCENT_33;
        mobjinfo[MT_BETAINS].flags2 &= ~(MF2_TRANSLUCENT_33 | MF2_FLOATBOB | MF2_NOFOOTCLIP);
        mobjinfo[MT_INV].flags2 &= ~MF2_TRANSLUCENT_33;
        mobjinfo[MT_INS].flags2 &= ~(MF2_TRANSLUCENT_33 | MF2_FLOATBOB | MF2_NOFOOTCLIP);
        mobjinfo[MT_MISC14].flags2 &= ~(MF2_FLOATBOB | MF2_NOFOOTCLIP);
        mobjinfo[MT_BFG].flags2 &= ~MF2_TRANSLUCENT; 
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

        // [crispy] loaded from a lump
        printf(":");
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

            // [crispy] a palette has 768 bytes!
            unsigned char playpal[256 * 3];

        } cache;

        // [crispy] open file readable
        FILE *cachefp = fopen(fname = M_StringJoin(configdir, "tranmap.dat", NULL), "r+b");

        tranmap = Z_Malloc(256 * 256, PU_STATIC, NULL);

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

            fread(tranmap, 256, 256, cachefp) != 256)
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

                    bg = playpal + 3 * i;
                    fg = playpal + 3 * j;

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
// R_BestColor
//
// (borrowed from Quake2 source: utils3/qdata/images.c)
//
byte R_BestColor(const unsigned int *palette, const int r, const int g, const int b, const int numcolors)
{
    int        i;
    int        bestdistortion;
    int        bestcolor;

    // let any color go to 0 as a last resort
    bestdistortion = 256 * 256 * 4;
    bestcolor = 0;

    for (i = 0; i < numcolors; i++)
    {
        int dr = r - RPART(palette[i]);
        int dg = g - GPART(palette[i]);
        int db = b - BPART(palette[i]);
        int distortion = dr * dr + dg * dg + db * db;

        if (distortion < bestdistortion)
        {
            if (!distortion)
                // perfect match
                return i;

            bestdistortion = distortion;
            bestcolor = i;
        }
    }

    return bestcolor;
}

// Build the tables necessary for translucency
void R_BuildTransTable(unsigned int *palette)
{
    int        r, g, b;
    int        x, y;

    // create the small RGB table
    for (r = 0; r < 32; r++)
        for (g = 0; g < 32; g++)
            for (b = 0; b < 32; b++)
                RGB32k.RGB[r][g][b] = R_BestColor(palette, (r << 3) | (r >> 2),
                                                           (g << 3) | (g >> 2),
                                                           (b << 3) | (b >> 2), 256);

    for (x = 0; x < 65; x++)
        for (y = 0; y < 256; y++)
            Col2RGB8[x][y] = (((RPART(palette[y]) * x) >> 4) << 20)  |
                              ((GPART(palette[y]) * x) >> 4)         |
                             (((BPART(palette[y]) * x) >> 4) << 10);
}

//
// R_InitColormaps
//
// killough 3/20/98: rewritten to allow dynamic colormaps
// and to remove unnecessary 256-byte alignment
//
// killough 4/4/98: Add support for C_START/C_END markers
//
void R_InitColormaps(void)
{
    dboolean    COLORMAP = (W_CheckMultipleLumps("COLORMAP") > 1);
    int         i, j, k;
    byte        *palsrc, *palette, *playpal;
    wad_file_t  *colormapwad;
    dboolean    keepgray = false;
    char        c[3];

    if (W_CheckNumForName("C_START") >= 0 && W_CheckNumForName("C_END") >= 0)
    {
        firstcolormaplump = W_GetNumForName("C_START");
        lastcolormaplump = W_GetNumForName("C_END");
        numcolormaps = lastcolormaplump - firstcolormaplump;

        colormaps = Z_Malloc(sizeof(*colormaps) * numcolormaps, PU_STATIC, NULL);

        colormaps[0] = W_CacheLumpName("COLORMAP", PU_STATIC);

        for (i = 1; i < numcolormaps; i++)
            colormaps[i] = W_CacheLumpNum(i + firstcolormaplump, PU_STATIC);
    }
    else
    {
        colormaps = Z_Malloc(sizeof(*colormaps), PU_STATIC, NULL);
        colormaps[0] = W_CacheLumpName("COLORMAP", PU_STATIC);
    }

    colormapwad = lumpinfo[W_CheckNumForName("COLORMAP")]->wad_file;
    C_Output("R_InitColormaps: Using the COLORMAP lump in %s file %s.",
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

    // [crispy] Status Bar '0'
    i = W_CheckNumForName("sttnum0");

    keepgray = (i >= 0 /*&& !strcmp(lumpinfo[i]->wad_file->path, M_BaseName(iwadfile))*/);

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

    // COLORMAP predefined to return 0
    if (strncasecmp(name, "COLORMAP", 8))
        if ((i = W_CheckNumForName(name)) != -1)
            i -= firstcolormaplump;

    return i;
}

//
// Palette management stuff
//
void R_GammaAdjustPalette(palette_t *pal)
{
    if (pal->colors && pal->basecolors)
    {
        unsigned i;

        for (i = 0; i < pal->numcolors; i++)
        {
            unsigned color = pal->basecolors[i];

            pal->colors[i] = MAKERGB(newgamma[RPART(color)],
                                     newgamma[GPART(color)],
                                     newgamma[BPART(color)]);
        }
    }
}

dboolean R_InternalCreatePalette(palette_t *palette, char *name, byte *colors,
                            unsigned numcolors, unsigned flags)
{
    unsigned i;

    if (numcolors > 256)
        numcolors = 256;
    else if (numcolors == 0)
        return false;

    strncpy(palette->name.name, name, 8);
    palette->flags = flags;
    palette->usecount = 1;
    palette->maps.colormaps = NULL;
    palette->basecolors = Z_Malloc(numcolors * 2 * sizeof(unsigned), PU_STATIC, NULL);
    palette->colors = palette->basecolors + numcolors;
    palette->numcolors = numcolors;

    if (numcolors == 1)
        palette->shadeshift = 0;
    else if (numcolors <= 2)
        palette->shadeshift = 1;
    else if (numcolors <= 4)
        palette->shadeshift = 2;
    else if (numcolors <= 8)
        palette->shadeshift = 3;
    else if (numcolors <= 16)
        palette->shadeshift = 4;
    else if (numcolors <= 32)
        palette->shadeshift = 5;
    else if (numcolors <= 64)
        palette->shadeshift = 6;
    else if (numcolors <= 128)
        palette->shadeshift = 7;
    else
        palette->shadeshift = 8;

    for (i = 0; i < numcolors; i++, colors += 3)
        palette->basecolors[i] = MAKERGB(colors[0],colors[1],colors[2]);

    R_GammaAdjustPalette (palette);

    return true;
}

palette_t *R_InitPalettes(char *name)
{
    byte *colors;

    if (def_pal.usecount)
        return &def_pal;

    if ((colors = W_CacheLumpName (name, PU_CACHE)))
        if (R_InternalCreatePalette(&def_pal, name, colors, 256,
                                    PALETTEF_SHADE|PALETTEF_BLEND|PALETTEF_DEFAULT))
        {
            return &def_pal;
        }

    return NULL;
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

    // [crispy] prints a mark itself
    //R_InitTranMap();

    R_InitColormaps();
}

//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName(char *name)
{
    int  i;

    if (beta_style && gamemode != shareware && gamemode != commercial)
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
        if (replace_missing)
        {
            i = W_CheckNumForName ("FLOOR0_1");
            C_Error("R_FlatNumForName: %.8s not found (replaced with FLOOR0_1)", name);
        }
        else
        {
            C_Error("R_FlatNumForName: %.8s not found", name);
            return 0;
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
    int i = NO_TEXTURE; 

    if (*name != '-') 
    {
        i = textures[W_LumpNameHash(name) % (unsigned int)numtextures]->index;

        while (i >= 0 && strncasecmp(textures[i]->name, name, 8))
            i = textures[i]->next;
    }

    return i; 
}

//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//  aborts with error message.
//
int R_TextureNumForName(char *name)
{
    int i;

    if (beta_style && gamemode != shareware && gamemode != commercial)
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
        if (replace_missing)
        {
            i = R_CheckTextureNumForName ("BROWN1");
            C_Error("R_TextureNumForName: %.8s not found (replaced with BROWN1)", name);
        }
        else
        {
            C_Error("R_TextureNumForName: %.8s not found", name);
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
    byte        *hitlist = malloc(MAX(numtextures, MAX(numflats, NUMSPRITES))); 
    thinker_t   *th;
    int         i;
    int         j;
    int         k;
/*
    if (demoplayback)
    {
        free(hitlist);
        return;
    }
*/
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
    memset(hitlist, 0, NUMSPRITES);

    for (th = thinkerclasscap[th_mobj].cnext; th != &thinkerclasscap[th_mobj]; th = th->cnext)
        hitlist[((mobj_t *)th)->sprite] = 1;

    for (i = 0; i < NUMSPRITES; i++)
        if (hitlist[i])
            for (j = 0; j < sprites[i].numframes; j++)
            {
                short   *lump = sprites[i].spriteframes[j].lump;

                for (k = 0; k < 8; k++)
                    W_CacheLumpNum(firstspritelump + lump[k], PU_CACHE);
            }

    free(hitlist);
}

