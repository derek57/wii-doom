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

#include <stdlib.h>

#include "c_io.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_tinttab.h"
#include "m_random.h"
#include "r_local.h"
#include "st_stuff.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#define TRANSPARENT_COLOR       0x1C

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about coordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//

patch_t         *draw_chars;

int             viewwidth;
int             scaledviewwidth;
int             scaledviewheight;                        // ADDED FOR HIRES
int             viewheight;
int             viewheight2;
int             viewwindowx;
int             viewwindowy;
int             fuzztable[SCREENWIDTH * SCREENHEIGHT];

extern int      r_screensize;

// Backing buffer containing the bezel drawn around the screen and 
// surrounding background.
//static byte     *background_buffer = NULL;

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
byte            translations[3][256];

byte redtoblue[] =
{
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
     96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    200, 200, 201, 201, 202, 202, 203, 203, 204, 204, 205, 205, 206, 206, 207, 207,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

byte redtogreen[] =
{
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
     96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    118, 118, 119, 119, 120, 120, 121, 121, 122, 122, 123, 123, 124, 124, 125, 125,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};
/*
byte redtoyellow[] =
{
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43, 164, 164, 165, 165,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
     96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    230, 230, 231, 231, 160, 160, 161, 161, 162, 162, 163, 163, 164, 164, 165, 165,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};
*/
byte megasphere[] =
{
      0,   1,   2,   3,   4,   5,   6,   7,   8, 142,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
     64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
     80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
     96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127,
    128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
    144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 142,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
    176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
    192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
    208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
    224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

//
// R_DrawColumn
// Source is the top of the column to scale.
//
lighttable_t    *dc_colormap;
int             dc_x;
int             dc_yl;
int             dc_yh;
fixed_t         dc_iscale;
fixed_t         dc_texturemid;
fixed_t         dc_texheight;
fixed_t         dc_texturefrac;
dboolean         dc_topsparkle;
dboolean         dc_bottomsparkle;
byte            *dc_blood;
byte            *dc_colormask;
int             dc_baseclip;

// first pixel in a column (possibly virtual)
byte            *dc_source;

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//

void R_DrawColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0
        || dc_yh >= SCREENHEIGHT)
        I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    while (--count)
    {
        *dest = colormap[source[frac >> FRACBITS]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[source[frac >> FRACBITS]];
}

void R_DrawShadowColumn(void)
{
    int32_t     count = dc_yh - dc_yl + 1;
    byte        *dest = R_ADDRESS(0, dc_x, dc_yl);

    if (--count)
    {
        *dest = tinttab25[*dest];
        dest += SCREENWIDTH;
    }
    while (--count > 0)
    {
        *dest = tinttab40[*dest];
        dest += SCREENWIDTH;
    }
    *dest = tinttab25[*dest];
}

void R_DrawSpectreShadowColumn(void)
{
    int32_t     count = dc_yh - dc_yl + 1;
    byte        *dest = R_ADDRESS(0, dc_x, dc_yl);

    if (--count)
    {
        if (!(rand() % 4) && !consoleactive)
            *dest = tinttab25[*dest];
        dest += SCREENWIDTH;
    }
    while (--count > 0)
    {
        *dest = tinttab25[*dest];
        dest += SCREENWIDTH;
    }
    if (!(rand() % 4) && !consoleactive)
        *dest = tinttab25[*dest];
}

void R_DrawSolidShadowColumn(void)
{
    int32_t     count = dc_yh - dc_yl + 1;
    byte        *dest = R_ADDRESS(0, dc_x, dc_yl);

    while (--count > 0)
    {
        *dest = 0;
        dest += SCREENWIDTH;
    }
    *dest = 0;
}

void R_DrawBloodSplatColumn(void)
{
    int32_t     count = dc_yh - dc_yl + 1;
    byte        *dest = R_ADDRESS(0, dc_x, dc_yl);
    byte        *blood = dc_blood;

    while (--count > 0)
    {
        *dest = *(*dest + blood);
        dest += SCREENWIDTH;
    }
    *dest = *(*dest + blood);
}

void R_DrawSolidBloodSplatColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    const fixed_t       blood = *dc_blood;

    while (--count > 0)
    {
        *dest = blood;
        dest += SCREENWIDTH;
    }
    *dest = blood;
}

void R_DrawWallColumn(void)
{
    int32_t     count = dc_yh - dc_yl + 1;

    if (count <= 0)
        return;
    else
    {
        byte                    *dest = R_ADDRESS(0, dc_x, dc_yl);
        byte                    *top = dest;
        const fixed_t           fracstep = dc_iscale;
        fixed_t                 frac = dc_texturemid + (dc_yl - centery) * fracstep;
        const byte              *source = dc_source;
        const lighttable_t      *colormap = dc_colormap;
        const fixed_t           texheight = dc_texheight;
        fixed_t                 heightmask = texheight - 1;

        // [SL] Properly tile textures whose heights are not a power-of-2,
        // avoiding a tutti-frutti effect. From Eternity Engine.
        if (texheight & heightmask)
        {
            heightmask++;
            heightmask <<= FRACBITS;

            if (frac < 0)
                while ((frac += heightmask) <  0);
            else
                while (frac >= heightmask)
                    frac -= heightmask;

            while (count--)
            {
                *dest = colormap[source[frac >> FRACBITS]];
                dest += SCREENWIDTH;
                if ((frac += fracstep) >= heightmask)
                    frac -= heightmask;
            }
        }
        else
        {
            // texture height is a power-of-2
            // do some loop unrolling
            while (count >= 8)
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                count -= 8;
            }

            if (count & 1)
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
            }

            if (count & 2)
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
            }

            if (count & 4)
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
            }
        }

        if (dc_bottomsparkle && !(((frac - fracstep) >> FRACBITS) & 2))
            *(dest - SCREENWIDTH) = *(dest - SCREENWIDTH * 2);

        if (dc_topsparkle)
            *top = *(top + SCREENWIDTH);
    }
}

void R_DrawFullbrightWallColumn(void)
{
    int32_t     count = dc_yh - dc_yl + 1;

    if (count <= 0)
        return;
    else
    {
        byte                    *dest = R_ADDRESS(0, dc_x, dc_yl);
        byte                    *top = dest;
        const fixed_t           fracstep = dc_iscale;
        fixed_t                 frac = dc_texturemid + (dc_yl - centery) * fracstep;
        const byte              *source = dc_source;
        const byte              *colormask = dc_colormask;
        const lighttable_t      *colormap = dc_colormap;
        const fixed_t           texheight = dc_texheight;
        fixed_t                 heightmask = texheight - 1;
        byte                    dot;

        // [SL] Properly tile textures whose heights are not a power-of-2,
        // avoiding a tutti-frutti effect. From Eternity Engine.
        if (texheight & heightmask)
        {
            heightmask++;
            heightmask <<= FRACBITS;

            if (frac < 0)
                while ((frac += heightmask) <  0);
            else
                while (frac >= heightmask)
                    frac -= heightmask;

            while (count--)
            {
                dot = source[frac >> FRACBITS];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                if ((frac += fracstep) >= heightmask)
                    frac -= heightmask;
            }
        }
        else
        {
            // texture height is a power-of-2
            // do some loop unrolling
            while (count >= 8)
            {
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                count -= 8;
            }

            if (count & 1)
            {
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
            }

            if (count & 2)
            {
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
            }

            if (count & 4)
            {
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
                dot = source[(frac >> FRACBITS) & heightmask];
                *dest = (colormask[dot] ? dot : colormap[dot]);
                dest += SCREENWIDTH;
                frac += fracstep;
            }
        }

        if (dc_bottomsparkle && !(((frac - fracstep) >> FRACBITS) & 2))
            *(dest - SCREENWIDTH) = *(dest - SCREENWIDTH * 2);

        if (dc_topsparkle)
            *top = *(top + SCREENWIDTH);
    }
}

void R_DrawPlayerSpriteColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(1, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;

    while (--count)
    {
        *dest = dc_source[frac >> FRACBITS];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = dc_source[frac >> FRACBITS];
}

void R_DrawSuperShotgunColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        byte    dot = source[frac >> FRACBITS];

        if (dot != 71)
            *dest = colormap[dot];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[source[frac >> FRACBITS]];
}

void R_DrawTranslucentSuperShotgunColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        byte    dot = source[frac >> FRACBITS];

        if (dot != 71)
            *dest = colormap[tinttabredwhite1[(*dest << 8) + dot]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[tinttabredwhite1[(*dest << 8) + source[frac >> FRACBITS]]];
}

void R_DrawSkyColumn(void)
{
    int32_t     count = dc_yh - dc_yl + 1;

    if (count <= 0)
        return;
    else
    {
        byte                    *dest = R_ADDRESS(0, dc_x, dc_yl);
        const fixed_t           fracstep = dc_iscale;
        fixed_t                 frac = dc_texturemid + (dc_yl - centery) * fracstep;
        const byte              *source = dc_source;
        const lighttable_t      *colormap = dc_colormap;
        const fixed_t           texheight = dc_texheight;
        fixed_t                 heightmask = texheight - 1;

        // [SL] Properly tile textures whose heights are not a power-of-2,
        // avoiding a tutti-frutti effect. From Eternity Engine.
        if (texheight & heightmask)
        {
            heightmask++;
            heightmask <<= FRACBITS;

            if (frac < 0)
                while ((frac += heightmask) <  0);
            else
                while (frac >= heightmask)
                    frac -= heightmask;

            while (count--)
            {
                *dest = colormap[source[frac >> FRACBITS]];
                dest += SCREENWIDTH;
                if ((frac += fracstep) >= heightmask)
                    frac -= heightmask;
            }
        }
        else
        {
            // texture height is a power-of-2
            // do some loop unrolling
            while (count >= 8)
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                count -= 8;
            }

            if (count & 1)
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
            }

            if (count & 2)
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
            }

            if (count & 4)
            {
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
                dest += SCREENWIDTH;
                frac += fracstep;
                *dest = colormap[source[(frac >> FRACBITS) & heightmask]];
            }
        }
    }
}

void R_DrawFlippedSkyColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    const fixed_t       fracstep = dc_iscale;
    fixed_t             frac = dc_texturemid + (dc_yl - centery) * fracstep;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;
    fixed_t             i;

    while (--count)
    {
        i = frac >> FRACBITS;
        *dest = colormap[source[i > 127 ? 126 - (i & 127) : i]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    i = frac >> FRACBITS;
    *dest = colormap[source[i > 127 ? 126 - (i & 127) : i]];
}

void R_DrawRedToBlueColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = colormap[redtoblue[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[redtoblue[source[frac >> FRACBITS]]];
}

void R_DrawTranslucentRedToBlue33Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab33[(*dest << 8) + colormap[redtoblue[source[frac >> FRACBITS]]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab33[(*dest << 8) + colormap[redtoblue[source[frac >> FRACBITS]]]];
}

void R_DrawRedToGreenColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = colormap[redtogreen[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[redtogreen[source[frac >> FRACBITS]]];
}

void R_DrawTranslucentRedToGreen33Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab33[(*dest << 8) + colormap[redtogreen[source[frac >> FRACBITS]]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab33[(*dest << 8) + colormap[redtogreen[source[frac >> FRACBITS]]]];
}

void R_DrawTranslucentColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent5Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab5[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab5[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent10Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab10[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab10[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent15Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab15[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab15[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent20Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab20[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab20[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent25Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab25[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab25[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent30Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab30[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab30[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent33Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab33[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab33[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent35Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab35[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab35[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent40Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab40[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab40[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent45Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab45[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab45[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent50Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tranmap[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tranmap[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent55Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab55[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab55[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent60Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab60[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab60[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent65Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab65[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab65[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent70Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab70[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab70[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent75Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab75[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab75[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent80Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab80[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab80[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent85Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab85[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab85[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent90Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab90[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab90[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucent95Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab95[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab95[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawMegaSphereColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttab33[(*dest << 8) + colormap[megasphere[source[frac >> FRACBITS]]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttab33[(*dest << 8) + colormap[megasphere[source[frac >> FRACBITS]]]];
}

void R_DrawSolidMegaSphereColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = colormap[megasphere[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[megasphere[source[frac >> FRACBITS]]];
}

void R_DrawTranslucentRedColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttabred[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttabred[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucentRedWhiteColumn1(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = colormap[tinttabredwhite1[(*dest << 8) + source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[tinttabredwhite1[(*dest << 8) + source[frac >> FRACBITS]]];
}

void R_DrawTranslucentRedWhiteColumn2(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = colormap[tinttabredwhite2[(*dest << 8) + source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[tinttabredwhite2[(*dest << 8) + source[frac >> FRACBITS]]];
}

void R_DrawTranslucentRedWhite50Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = colormap[tinttabredwhite50[(*dest << 8) + source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[tinttabredwhite50[(*dest << 8) + source[frac >> FRACBITS]]];
}

void R_DrawTranslucentGreenColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttabgreen[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttabgreen[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucentBlueColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = tinttabblue[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = tinttabblue[(*dest << 8) + colormap[source[frac >> FRACBITS]]];
}

void R_DrawTranslucentRed33Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = colormap[tinttabred33[(*dest << 8) + source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[tinttabred33[(*dest << 8) + source[frac >> FRACBITS]]];
}

void R_DrawTranslucentGreen33Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = colormap[tinttabgreen33[(*dest << 8) + source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[tinttabgreen33[(*dest << 8) + source[frac >> FRACBITS]]];
}

void R_DrawTranslucentBlue33Column(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;

    while (--count)
    {
        *dest = colormap[tinttabblue33[(*dest << 8) + source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[tinttabblue33[(*dest << 8) + source[frac >> FRACBITS]]];
}

//
// Spectre/Invisibility.
//
extern int      fuzzpos;

int             fuzzrange[3] = { -SCREENWIDTH, 0, SCREENWIDTH };

#define FUZZ(a, b)      fuzzrange[rand() % (b - a + 1) + a]
#define NOFUZZ          251

void R_DrawFuzzColumn(void)
{
    byte        *dest;
    int         count = dc_yh - dc_yl;

    if (count < 0)
        return;

#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
        I_Error ("R_DrawFuzzColumn: %i to %i at %i",
                 dc_yl, dc_yh, dc_x);
    }
#endif

    dest = R_ADDRESS(0, dc_x, dc_yl);

    if (count)
    {
        // top
        if (!dc_yl)
            *dest = fullcolormap[6 * 256 + dest[(fuzztable[fuzzpos++] = FUZZ(1, 2))]];
        else if (!(rand() % 4))
            *dest = fullcolormap[12 * 256 + dest[(fuzztable[fuzzpos++] = FUZZ(0, 2))]];
        dest += SCREENWIDTH;

        while (--count)
        {
            // middle
            *dest = fullcolormap[6 * 256 + dest[(fuzztable[fuzzpos++] = FUZZ(0, 2))]];
            dest += SCREENWIDTH;
        }

        // bottom
        if (dc_yh == viewheight - 1)
            *dest = fullcolormap[5 * 256 + dest[(fuzztable[fuzzpos] = FUZZ(0, 1))]];
        else if (dc_baseclip == -1 && !(rand() % 4))
            *dest = fullcolormap[14 * 256 + dest[(fuzztable[fuzzpos] = FUZZ(0, 1))]];
    }
}

void R_DrawPausedFuzzColumn(void)
{
    byte        *dest;
    int         count = dc_yh - dc_yl;

    if (count < 0)
        return;

    dest = R_ADDRESS(0, dc_x, dc_yl);

    if (count)
    {
        // top
        if (!dc_yl)
        {
            *dest = fullcolormap[6 * 256 + dest[fuzztable[fuzzpos++]]];
            if (fuzzpos == SCREENWIDTH * SCREENHEIGHT)
                fuzzpos = 0;
        }
        dest += SCREENWIDTH;

        while (--count)
        {
            // middle
            *dest = fullcolormap[6 * 256 + dest[fuzztable[fuzzpos++]]];
            if (fuzzpos == SCREENWIDTH * SCREENHEIGHT)
                fuzzpos = 0;
            dest += SCREENWIDTH;
        }

        // bottom
        if (dc_yh == viewheight - 1)
            *dest = fullcolormap[5 * 256 + dest[fuzztable[fuzzpos]]];
    }
}

void R_DrawFuzzColumns(int srcscrn, int destscrn)
{
    int         x, y;
    int         w = viewwindowx + viewwidth;
    int         h = (viewwindowy + viewheight) * SCREENWIDTH;

    for (x = viewwindowx; x < w; x++)
        for (y = viewwindowy * SCREENWIDTH; y < h; y += SCREENWIDTH)
        {
            int         i = x + y;
//            byte        *src = background_buffer + i;
            byte        *src = screens[srcscrn] + i;

            if (*src != NOFUZZ)
            {
//                byte    *dest = I_VideoBuffer + i;
                byte    *dest = screens[destscrn] + i;

                if (!y || *(src - SCREENWIDTH) == NOFUZZ)
                {
                    // top
                    if (!(rand() % 4))
                        *dest = fullcolormap[12 * 256 + dest[(fuzztable[i] = FUZZ(0, 2))]];
                }
                else if (y == h - SCREENWIDTH)
                {
                    // bottom of view
                    *dest = fullcolormap[5 * 256 + dest[(fuzztable[i] = FUZZ(0, 1))]];
                }
                else if (*(src + SCREENWIDTH) == NOFUZZ)
                {
                    // bottom of post
                    if (!(rand() % 4))
                        *dest = fullcolormap[12 * 256 + dest[(fuzztable[i] = FUZZ(0, 2))]];
                }
                else
                {
                    // middle
                    if (*(src - 1) == NOFUZZ || *(src + 1) == NOFUZZ)
                    {
                        if (!(rand() % 4))
                            *dest = fullcolormap[12 * 256 + dest[(fuzztable[i] = FUZZ(0, 2))]];
                    }
                    else
                        *dest = fullcolormap[6 * 256 + dest[(fuzztable[i] = FUZZ(0, 2))]];
                }
            }
        }
}

void R_DrawPausedFuzzColumns(int srcscrn, int destscrn)
{
    int         x, y;
    int         w = viewwindowx + viewwidth;
    int         h = (viewwindowy + viewheight) * SCREENWIDTH;

    for (x = viewwindowx; x < w; x++)
        for (y = viewwindowy * SCREENWIDTH; y < h; y += SCREENWIDTH)
        {
            int         i = x + y;
//            byte        *src = background_buffer + i;
            byte        *src = screens[srcscrn] + i;

            if (*src != NOFUZZ)
            {
//                byte    *dest = I_VideoBuffer + i;
                byte    *dest = screens[destscrn] + i;

                if (!y || *(src - SCREENWIDTH) == NOFUZZ)
                {
                    // top
                    // do nothing
                }
                else if (y == h - SCREENWIDTH)
                {
                    // bottom of view
                    *dest = fullcolormap[5 * 256 + dest[fuzztable[i]]];
                }
                else if (*(src + SCREENWIDTH) == NOFUZZ)
                {
                    // bottom of post
                    // do nothing
                }
                else
                {
                    // middle
                    if (*(src - 1) == NOFUZZ || *(src + 1) == NOFUZZ)
                    {
                        // do nothing
                    }
                    else
                        *dest = fullcolormap[6 * 256 + dest[fuzztable[i]]];
                }
            }
        }
}

//
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//
byte    *dc_translation;
byte    *translationtables;

void R_DrawTranslatedColumn(void)
{
    int32_t             count = dc_yh - dc_yl + 1;
    byte                *dest = R_ADDRESS(0, dc_x, dc_yl);
    fixed_t             frac = dc_texturefrac;
    const fixed_t       fracstep = dc_iscale;
    const byte          *source = dc_source;
    const lighttable_t  *colormap = dc_colormap;
    const byte          *translation = dc_translation;

#ifdef RANGECHECK 
    if ((unsigned)dc_x >= SCREENWIDTH
        || dc_yl < 0
        || dc_yh >= SCREENHEIGHT)
    {
        I_Error ( "R_DrawTranslatedColumn: %i to %i at %i",
                  dc_yl, dc_yh, dc_x);
    }
    
#endif 

    while (--count)
    {
        *dest = colormap[translation[source[frac >> FRACBITS]]];
        dest += SCREENWIDTH;
        frac += fracstep;
    }
    *dest = colormap[translation[source[frac >> FRACBITS]]];
}

//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
void R_InitTranslationTables(void)
{
    int i;

    translationtables = Z_Malloc(256 * 3, PU_STATIC, NULL);

    // translate just the 16 green colors
    for (i = 0; i < 256; i++)
        if (i >= 0x70 && i <= 0x7F)
        {
            // map green ramp to gray, brown, red
            translationtables[i] = 0x60 + (i & 0xF);
            translationtables[i + 256] = 0x40 + (i & 0xF);
            translationtables[i + 512] = 0x20 + (i & 0xF);
        }
        else
            // Keep all other colors as is.
            translationtables[i] = translationtables[i + 256] = translationtables[i + 512] = i;
}

//
// R_DrawSpan
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
int             ds_y;
int             ds_x1;
int             ds_x2;

lighttable_t    *ds_colormap;

fixed_t         ds_xfrac;
fixed_t         ds_yfrac;
fixed_t         ds_xstep;
fixed_t         ds_ystep;

// start of a 64*64 tile image
byte            *ds_source;

//
// Draws the actual span.
//
void R_DrawSpan(void)
{
    unsigned int        count = ds_x2 - ds_x1 + 1;
    byte                *dest = R_ADDRESS(0, ds_x1, ds_y);
    fixed_t             xfrac = ds_xfrac;
    fixed_t             yfrac = ds_yfrac;
    const fixed_t       xstep = ds_xstep;
    const fixed_t       ystep = ds_ystep;
    const byte          *source = ds_source;
    const lighttable_t  *colormap = ds_colormap;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
        || ds_x1<0
        || ds_x2>=SCREENWIDTH
        || (unsigned)ds_y>SCREENHEIGHT)
    {
        I_Error( "R_DrawSpan: %i to %i at %i",
                 ds_x1,ds_x2,ds_y);
    }
//        dscount++;
#endif

    while (count >= 4)
    {
        *dest++ = colormap[source[((xfrac >> 16) & 63) | ((yfrac >> 10) & 4032)]];
        xfrac += xstep;
        yfrac += ystep;
        *dest++ = colormap[source[((xfrac >> 16) & 63) | ((yfrac >> 10) & 4032)]];
        xfrac += xstep;
        yfrac += ystep;
        *dest++ = colormap[source[((xfrac >> 16) & 63) | ((yfrac >> 10) & 4032)]];
        xfrac += xstep;
        yfrac += ystep;
        *dest++ = colormap[source[((xfrac >> 16) & 63) | ((yfrac >> 10) & 4032)]];
        xfrac += xstep;
        yfrac += ystep;
        count -= 4;
    }
    while (count-- > 0)
    {
        *dest++ = colormap[source[((xfrac >> 16) & 63) | ((yfrac >> 10) & 4032)]];
        xfrac += xstep;
        yfrac += ystep;
    }
}

//
// R_InitBuffer
// Creates lookup tables that avoid
//  multiplies and other hassles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void R_InitBuffer(int width, int height)
{
    // Handle resize, e.g. smaller view windows with border and/or status bar.
    viewwindowx = (SCREENWIDTH - width) >> 1;

    // Same with base row offset.
    viewwindowy = (width == SCREENWIDTH ? 0 : (SCREENHEIGHT - SBARHEIGHT - height) >> 1);
}

//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
void R_FillBackScreen(int srcscrn, int destscrn)
{
    byte*        src;
    byte*        dest; 
    int          x;
    int          y; 
    char         *name;

    // DOOM border patch.
    char *name1 = "FLOOR7_2";

    // DOOM II border patch.
    char *name2 = "GRNROCK";

    // If we are running full screen, there is no need to do any of this,
    // and the background buffer can be freed if it was previously in use.

    if (scaledviewwidth == SCREENWIDTH)
    {
/*
        if (background_buffer != NULL)
        {
            Z_Free(background_buffer);
            background_buffer = NULL;
        }
*/
        return;
    }

    // Allocate the background buffer if necessary
/*        
    if (background_buffer == NULL)
    {
        background_buffer = Z_Malloc(SCREENWIDTH * (SCREENHEIGHT - SBARHEIGHT) * sizeof(*background_buffer),
                                     PU_STATIC, NULL);
    }
*/
    if (gamemode == commercial)
        name = name2;
    else
        name = name1;
    
    src = W_CacheLumpName(name, PU_CACHE); 
//    dest = background_buffer;
    dest = screens[destscrn];
         
    for (y=0 ; y<SCREENHEIGHT-SBARHEIGHT ; y++) 
    { 
        for (x=0 ; x<SCREENWIDTH/64 ; x++) 
        { 
            memcpy (dest, src+((y&63)<<6), 64); 
            dest += 64; 
        } 

        if (SCREENWIDTH&63) 
        { 
            memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63); 
            dest += (SCREENWIDTH&63); 
        } 
    } 
     
    // Draw screen and bezel; this is done to a separate screen buffer.

//    V_UseBuffer(background_buffer);
//    V_UseBuffer(screens[srcscrn]);

    // COMPLETELY CHANGED FOR HIRES
    for (x=0 ; x<(scaledviewwidth >> hires) ; x+=8)
        V_DrawPatch((viewwindowx >> hires)+x,
                    (viewwindowy >> hires)-8, 1, W_CacheLumpName("brdr_t",PU_CACHE));

    for (x=0 ; x<(scaledviewwidth >> hires) ; x+=8)
        V_DrawPatch((viewwindowx >> hires)+x, (viewwindowy >> hires)+
                    (scaledviewheight >> hires), 1, W_CacheLumpName("brdr_b",PU_CACHE));

    for (y=0 ; y<(scaledviewheight >> hires) ; y+=8)
        V_DrawPatch((viewwindowx >> hires)-8,
                    (viewwindowy >> hires)+y, 1, W_CacheLumpName("brdr_l",PU_CACHE));

    for (y=0 ; y<(scaledviewheight >> hires); y+=8)
        V_DrawPatch((viewwindowx >> hires)+
                    (scaledviewwidth >> hires),
                    (viewwindowy >> hires)+y, 1, W_CacheLumpName("brdr_r",PU_CACHE));

    // Draw beveled edge. 
    V_DrawPatch((viewwindowx >> hires)-8,
                (viewwindowy >> hires)-8, 1, W_CacheLumpName("brdr_tl",PU_CACHE));
    
    V_DrawPatch((viewwindowx >> hires)+
                (scaledviewwidth >> hires),
                (viewwindowy >> hires)-8, 1, W_CacheLumpName("brdr_tr",PU_CACHE));
    
    V_DrawPatch((viewwindowx >> hires)-8,
                (viewwindowy >> hires)+
                (scaledviewheight >> hires), 1, W_CacheLumpName("brdr_bl",PU_CACHE));
    
    V_DrawPatch((viewwindowx >> hires)+
                (scaledviewwidth >> hires),
                (viewwindowy >> hires)+
                (scaledviewheight >> hires), 1, W_CacheLumpName("brdr_br",PU_CACHE));

//    V_RestoreBuffer();
}

//
// Copy a screen buffer.
//
void R_VideoErase(unsigned int ofs, int count, int srcscrn, int destscrn)
{
    // LFB copy.
    // This might not be a good idea if memcpy
    //  is not optimal, e.g. byte by byte on
    //  a 32bit CPU, as GNU GCC/Linux libc did
    //  at one point.
/*
    if (background_buffer != NULL)
    {
        memcpy(I_VideoBuffer + ofs, background_buffer + ofs, count * sizeof(*I_VideoBuffer));
    }
*/
    memcpy(screens[destscrn] + ofs, screens[srcscrn] + ofs, count);
}

//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void R_DrawViewBorder(void)
{
    int                top;
    int                side;
    int                ofs;
    int                i; 
 
    if (scaledviewwidth == SCREENWIDTH || am_overlay) 
        return; 
  
    top = ((SCREENHEIGHT-SBARHEIGHT)-scaledviewheight)/2; // CHANGED FOR HIRES
    side = (SCREENWIDTH-scaledviewwidth)/2; 
 
    // copy top and one line of left side 
    R_VideoErase (0, top*SCREENWIDTH+side, 1, 0); 
 
    // copy one line of right side and bottom 
    ofs = (scaledviewheight+top)*SCREENWIDTH-side;        // CHANGED FOR HIRES
    R_VideoErase (ofs, top*SCREENWIDTH+side, 1, 0); 
 
    // copy sides using wraparound 
    ofs = top*SCREENWIDTH + SCREENWIDTH-side; 
    side <<= 1;
    
    for (i=1 ; i<scaledviewheight ; i++)                  // CHANGED FOR HIRES
    { 
        R_VideoErase (ofs, side, 1, 0); 
        ofs += SCREENWIDTH; 
    } 

    // ? 
//    V_MarkRect (0, 0, 1, SCREENWIDTH, SCREENHEIGHT - SBARHEIGHT, 0); 
}

/*
================
Draw_Char

Draws one 8*8 graphics character
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void R_DrawChar(int x, int y, int scrn, int num)
{
    byte       *dest;
    byte       *source;

    int        drawline;
    int        row;
    int        col;

    num &= 255;

    // load console characters
    if (!draw_chars)
    {
        byte *pic = NULL;

        int i;
        int c;
        int width = 0;
        int height = 0;

        // TODO: Try loading the char table from a WAD lump
#ifdef WII
        if (usb)
            LoadPCX("usb:/apps/wiidoom/conchars.pcx", &pic, NULL, &width, &height);
        else if (sd)
            LoadPCX("sd:/apps/wiidoom/conchars.pcx", &pic, NULL, &width, &height);
#else
        LoadPCX("conchars.pcx", &pic, NULL, &width, &height);
#endif

        c = width * height;

        draw_chars = Z_Malloc(c, PU_STATIC, NULL);
        draw_chars->pixels[0] = Z_Malloc(c, PU_STATIC, NULL);
        draw_chars->pic = pic;
        draw_chars->width = width;
        draw_chars->height = height;

        for (i = 0; i < c; i++)
        {
            int b = draw_chars->pic[i];
            draw_chars->pixels[0][i] = b;
        }
    }

    // totally off screen
    if (y <= -8)
        return;

    // PGM - status text was missing in sw...
    if ((y + 8) > SCREENHEIGHT)
        return;

#ifdef RANGECHECK
    if (y > SCREENHEIGHT - 8 || x < 0 || x > SCREENWIDTH - 8)
    {
        C_Error("R_DrawChar: (%i, %i)", x, y);
        return;
    }

    if (num < 0 || num > 255)
    {
        C_Error("R_DrawChar: char %i", num);
        return;
    }
#endif

    row = num >> 4;
    col = num & 15;
    source = draw_chars->pixels[0] + (row << 10) + (col << 3);

    // clipped
    if (y < 0)
    {
        drawline = 8 + y;
        source -= 128 * y;
        y = 0;
    }
    else
        drawline = 8;

    dest = screens[scrn] + y * SCREENWIDTH + x;

    while (drawline--)
    {
        if (source[0] != TRANSPARENT_COLOR)
            dest[0] = source[0];

        if (source[1] != TRANSPARENT_COLOR)
            dest[1] = source[1];

        if (source[2] != TRANSPARENT_COLOR)
            dest[2] = source[2];

        if (source[3] != TRANSPARENT_COLOR)
            dest[3] = source[3];

        if (source[4] != TRANSPARENT_COLOR)
            dest[4] = source[4];

        if (source[5] != TRANSPARENT_COLOR)
            dest[5] = source[5];

        if (source[6] != TRANSPARENT_COLOR)
            dest[6] = source[6];

        if (source[7] != TRANSPARENT_COLOR)
            dest[7] = source[7];

        source += 128;
        dest += SCREENWIDTH;
    }
}

