// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005 Simon Howard
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:
//        Gamma correction LUT stuff.
//        Functions to draw patches (by post) directly to screen.
//        Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------


#include <math.h>
#include <stdio.h>
#include <string.h>

#include "c_io.h"
#include "deh_str.h"
#include "doomdef.h"
#include "doomtype.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_tinttab.h"
#include "i_video.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "r_main.h"
#include "v_misc.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"


// TODO: There are separate RANGECHECK defines for different games, but this
// is common code. Fix this.
#define RANGECHECK


// villsa [STRIFE] Blending table used for Strife
byte *xlatab = NULL;

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


// Each screen is [SCREENWIDTH * SCREENHEIGHT];
byte                         *screens[5];

// haleyjd 08/28/10: clipping callback function for patches.
// This is needed for Chocolate Strife, which clips patches to the screen.
static  vpatchclipfunc_t     patchclip_callback = NULL;

byte                         *tranmap = NULL;

byte                         *dp_translation = NULL;

int                          dirtybox[4]; 

int                          italicize[15] = { 0, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, -1, -1, -1 };

boolean                      dp_translucent = false;

extern int                   screenblocks;

//
// V_MarkRect 
// 
void V_MarkRect(int x, int y, int width, int height) 
{ 
    // If we are temporarily using an alternate screen, do not 
    // affect the update box.

    M_AddToBox (dirtybox, x, y); 
    M_AddToBox (dirtybox, x + width-1, y + height-1); 
} 
 

void V_DrawHorizLine(int x, int y, int w, int c)
{
    uint8_t *buf;
    int x1;

    buf = screens[0] + SCREENWIDTH * y + x;

    for (x1 = 0; x1 < w; ++x1)
    {
        *buf++ = c;
    }
}

//
// V_CopyRect 
// 
void
V_CopyRect
( int        srcx,
  int        srcy,
  int        srcscrn,
  int        width,
  int        height,
  int        destx,
  int        desty,
  int        destscrn ) 
{ 
    byte *src;
    byte *dest; 
 
    srcx <<= hires;
    srcy <<= hires;
    width <<= hires;
    height <<= hires;
    destx <<= hires;
    desty <<= hires;

#ifdef RANGECHECK 
    if (srcx < 0
     || srcx + width > SCREENWIDTH
     || srcy < 0
     || srcy + height > SCREENHEIGHT 
     || destx < 0
     || destx /* + width */ > SCREENWIDTH
     || desty < 0
     || desty /* + height */ > SCREENHEIGHT)
    {
        C_Printf(CR_RED, " Bad V_CopyRect: Patch (%d,%d)-(%d,%d) / Dest.: (%d,%d) exceeds LFB\n"
                , srcx, srcy, srcx + width, srcy + height, destx, desty);
    }
#endif 

    // [crispy] prevent framebuffer overflow
    if (destx + width > SCREENWIDTH)
        width = SCREENWIDTH - destx;
    if (desty + height > SCREENHEIGHT)
        height = SCREENHEIGHT - desty;

    V_MarkRect(destx, desty, width, height); 

    src = screens[srcscrn] + SCREENWIDTH * srcy + srcx;
    dest = screens[destscrn] + SCREENWIDTH * desty + destx;

    for ( ; height>0 ; height--) 
    { 
        memcpy(dest, src, width); 
        src += SCREENWIDTH; 
        dest += SCREENWIDTH; 
    } 
} 
 
//
// V_DrawPatch
// Masks a column based masked pic to the screen. 
//

void
V_DrawPatch
( int        x,
  int        y,
  int        scrn,
  patch_t*    patch ) 
{ 
    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w, f, tmpy;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    // haleyjd 08/28/10: Strife needs silent error checking here.
    if(patchclip_callback)
    {
        if(!patchclip_callback(patch, x, y))
            return;
    }

#ifdef RANGECHECK
    if (x < 0
     || x + SHORT(patch->width) > ORIGWIDTH
     || y < 0
     || y + SHORT(patch->height) > ORIGHEIGHT )
    {
        C_Printf(CR_RED, " Bad V_DrawPatch: Patch (%d,%d) exceeds LFB\n", x, y);
    }
#endif

    V_MarkRect(x, y, SHORT(patch->width), SHORT(patch->height));

    col = 0;
    desttop = screens[scrn] + (y << hires) * SCREENWIDTH + x;

    w = SHORT(patch->width);

    // [crispy] prevent framebuffer overflow
    while (x < 0)
    {
        x++;
        col++;
        desttop++;
    }

    // [crispy] quadruple for-loop for each dp_translation and dp_translucent case
    // to avoid checking these variables for each pixel and instead check once per patch
    // (1) normal, opaque patch
    if (!dp_translation && !dp_translucent)
    {
        // [crispy] prevent framebuffer overflow
        for ( ; col<w && x < ORIGWIDTH; x++, col++, desttop++)
        {
            column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

            // step through the posts in a column
            while (column->topdelta != 0xff)
            {
                for (f = 0; f <= hires; f++)
                {
                    source = (byte *)column + 3;
                    dest = desttop + column->topdelta*(SCREENWIDTH << hires) + (x * hires) + f;
                    count = column->length;
                    tmpy = y + column->topdelta;
 
                    // [crispy] prevent framebuffer overflow
                    while (tmpy < 0)
                    {
                        count--;
                        source++;
                        dest += (SCREENWIDTH << hires);
                        tmpy++;
                    }

                    while (tmpy + count > ORIGHEIGHT)
                    {
                        count--;
                    }

                    while (count-- > 0)
                    {
                        if (hires)
                        {
                            *dest = *source;
                            dest += SCREENWIDTH;
                        }
                        *dest = *source++;
                        dest += SCREENWIDTH;
                    }
                }
                column = (column_t *)((byte *)column + column->length + 4);
            }
        }
    }
    // (2) color-translated, opaque patch
    else if (dp_translation && !dp_translucent)
    {
        // [crispy] prevent framebuffer overflow
        for ( ; col<w && x < ORIGWIDTH; x++, col++, desttop++)
        {
            column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

            // step through the posts in a column
            while (column->topdelta != 0xff)
            {
                for (f = 0; f <= hires; f++)
                {
                    source = (byte *)column + 3;
                    dest = desttop + column->topdelta*(SCREENWIDTH << hires) + (x * hires) + f;
                    count = column->length;
                    tmpy = y + column->topdelta;

                    // [crispy] prevent framebuffer overflow
                    while (tmpy < 0)
                    {
                        count--;
                        source++;
                        dest += (SCREENWIDTH << hires);
                        tmpy++;
                    }

                    while (tmpy + count > ORIGHEIGHT)
                    {
                        count--;
                    }

                    while (count-- > 0)
                    {
                        if (hires)
                        {
                            *dest = dp_translation[*source];
                            dest += SCREENWIDTH;
                        }
                        *dest = dp_translation[*source++];
                        dest += SCREENWIDTH;
                    }
                }
                column = (column_t *)((byte *)column + column->length + 4);
            }
        }
    }
    // (3) normal, translucent patch
    else if (!dp_translation && dp_translucent)
    {
        // [crispy] prevent framebuffer overflow
        for ( ; col<w && x < ORIGWIDTH; x++, col++, desttop++)
        {
            column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

            // step through the posts in a column
            while (column->topdelta != 0xff)
            {
                for (f = 0; f <= hires; f++)
                {
                    source = (byte *)column + 3;
                    dest = desttop + column->topdelta*(SCREENWIDTH << hires) + (x * hires) + f;
                    count = column->length;
                    tmpy = y + column->topdelta;

                    // [crispy] prevent framebuffer overflow
                    while (tmpy < 0)
                    {
                        count--;
                        source++;
                        dest += (SCREENWIDTH << hires);
                        tmpy++;
                    }

                    while (tmpy + count > ORIGHEIGHT)
                    {
                        count--;
                    }
 
                    while (count-- > 0)
                    {
                        if (hires)
                        {
                            *dest = tranmap[(*dest<<8)+*source];
                            dest += SCREENWIDTH;
                        }
                        *dest = tranmap[(*dest<<8)+*source++];
                        dest += SCREENWIDTH;
                    }
                }
                column = (column_t *)((byte *)column + column->length + 4);
            }
        }
    }
    // (4) color-translated, translucent patch
    else if (dp_translation && dp_translucent)
    {
        // [crispy] prevent framebuffer overflow
        for ( ; col<w && x < ORIGWIDTH; x++, col++, desttop++)
        {
            column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

            // step through the posts in a column
            while (column->topdelta != 0xff)
            {
                for (f = 0; f <= hires; f++)
                {
                    source = (byte *)column + 3;
                    dest = desttop + column->topdelta*(SCREENWIDTH << hires) + (x * hires) + f;
                    count = column->length;
                    tmpy = y + column->topdelta;

                    // [crispy] prevent framebuffer overflow
                    while (tmpy < 0)
                    {
                        count--;
                        source++;
                        dest += (SCREENWIDTH << hires);
                        tmpy++;
                    }

                    while (tmpy + count > ORIGHEIGHT)
                    {
                        count--;
                    }
 
                    while (count-- > 0)
                    {
                        if (hires)
                        {
                            *dest = tranmap[(*dest<<8)+dp_translation[*source]];
                            dest += SCREENWIDTH;
                        }
                        *dest = tranmap[(*dest<<8)+dp_translation[*source++]];
                        dest += SCREENWIDTH;
                    }
                }
                column = (column_t *)((byte *)column + column->length + 4);
            }
        }
    }
}

//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//

void
V_DrawPatchFlipped
( int        x,
  int        y,
  int        scrn,
  patch_t*    patch ) 
{
    int count;
    int col; 
    column_t *column; 
    byte *desttop;
    byte *dest;
    byte *source; 
    byte sourcetrans;
    int w, f, tmpy; 

    y -= SHORT(patch->topoffset); 
    x -= SHORT(patch->leftoffset); 

    // haleyjd 08/28/10: Strife needs silent error checking here.
    if(patchclip_callback)
    {
        if(!patchclip_callback(patch, x, y))
            return;
    }

#ifdef RANGECHECK
    if (x < 0
     || x + SHORT(patch->width) > ORIGWIDTH
     || y < 0
     || y + SHORT(patch->height) > ORIGHEIGHT )
    {
        C_Printf(CR_RED, " Bad V_DrawPatchFlipped: Patch (%d,%d)-(%d,%d) exceeds LFB\n"
                , x, y, x + SHORT(patch->width), y + SHORT(patch->height));
    }
#endif

    V_MarkRect (x, y, SHORT(patch->width), SHORT(patch->height));

    col = 0;
    desttop = screens[scrn]+ (y << hires) * SCREENWIDTH + x;

    w = SHORT(patch->width);

    // [crispy] prevent framebuffer overflow
    while (x < 0)
    {
        x++;
        col++;
        desttop++;
    }

    // [crispy] prevent framebuffer overflow
    for ( ; col<w && x < ORIGWIDTH; x++, col++, desttop++)
    {
        column = (column_t *)((byte *)patch + LONG(patch->columnofs[w-1-col]));

        // step through the posts in a column
        while (column->topdelta != 0xff )
        {
            for (f = 0; f <= hires; f++)
            {
                source = (byte *)column + 3;
                dest = desttop + column->topdelta*(SCREENWIDTH << hires) +
                       (x * hires) + f;
                count = column->length;
                tmpy = y + column->topdelta;

                // [crispy] prevent framebuffer overflow
                while (tmpy < 0)
                {
                    count--;
                    source++;
                    dest += (SCREENWIDTH << hires);
                    tmpy++;
                }

                while (tmpy + count > ORIGHEIGHT)
                {
                    count--;
                }

                    if (dp_translation)
                        sourcetrans = dp_translation[*source++];
                    else
                        sourcetrans = *source++;


                while (count-- > 0)
                {
                    if (hires)
                    {
                        *dest = *source;
                        *dest = sourcetrans;
                        dest += SCREENWIDTH;
                    }
                    *dest = *source++;
                    *dest = sourcetrans;
                    dest += SCREENWIDTH;
                }
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}


//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//

void
V_DrawBlock
( int        x,
  int        y,
  int        scrn,
  int        width,
  int        height,
  byte*        src ) 
{ 
    byte *dest; 
 
#ifdef RANGECHECK 
    if (x < 0
     || x + width >SCREENWIDTH
     || y < 0
     || y + height > SCREENHEIGHT)
    {
        C_Printf(CR_RED, " Bad V_DrawBlock: Patch (%d,%d)-(%d,%d) exceeds LFB\n"
                , x, y, x + width, y + height);
    }
#endif 
 
    V_MarkRect (x, y, width, height); 
 
    dest = screens[scrn] + (y << hires) * SCREENWIDTH + x;

    while (height--) 
    { 
        memcpy (dest, src, width); 
        src += width; 
        dest += SCREENWIDTH; 
    } 
} 

//
// V_Init
// 
void V_Init (void) 
{ 
    // no-op!
    // There used to be separate screens that could be drawn to; these are
    // now handled in the upper layers.

    int         i;
    byte        *base = Z_Malloc(SCREENWIDTH * SCREENHEIGHT * 4, PU_STATIC, NULL);

    for (i = 0; i < 4; i++)
        screens[i] = base + i * SCREENWIDTH * SCREENHEIGHT;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"

void V_DrawConsoleChar(int x, int y, patch_t *patch, byte color, boolean italics, int translucency,
    boolean inverted)
{
    int         col = 0;
    byte        *desttop = screens[0] + y * SCREENWIDTH + x;
    int         w = SHORT(patch->width);

    for (; col < w; col++, desttop++)
    {
        column_t        *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            byte        *source = (byte *)column + 3;
            byte        *dest = desttop + column->topdelta * SCREENWIDTH;
            int         count = column->length;

            while (count--)
            {
                if (y + column->topdelta + column->length - count > CONSOLETOP)
                {
                    if ((*source && !inverted) || (!*source && inverted))
                    {
                        if (italics)
                            *(dest + italicize[column->topdelta + column->length - count]) = color;
                        else
                            *dest = (translucency == 1 ? tinttab25[(color << 8) + *dest] :
                            (translucency == 2 ? tinttab25[(*dest << 8) + color] : color));
                    }
                }
                *(source++);
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

void V_DrawStatusPatch(int x, int y, patch_t *patch, byte *tinttab)
{
    int         col = 0;
    byte        *desttop;
    int         w;

    if (!tinttab)
        return;

    desttop = screens[0] + y * SCREENWIDTH + x;
    w = SHORT(patch->width);

    for (; col < w; col++, desttop++)
    {
        column_t        *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            byte        *source = (byte *)column + 3;
            byte        *dest = desttop + column->topdelta * SCREENWIDTH;
            int         count = column->length;

            while (count--)
            {
                *dest = *source++;
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

void V_DrawYellowStatusPatch(int x, int y, patch_t *patch, byte *tinttab)
{
    int         col = 0;
    byte        *desttop;
    int         w;

    if (!tinttab)
        return;

    desttop = screens[0] + y * SCREENWIDTH + x;
    w = SHORT(patch->width);

    for (; col < w; col++, desttop++)
    {
        column_t        *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            byte        *source = (byte *)column + 3;
            byte        *dest = desttop + column->topdelta * SCREENWIDTH;
            int         count = column->length;

            while (count--)
            {
                *dest = redtoyellow[*source++];
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

void V_DrawTranslucentStatusPatch(int x, int y, patch_t *patch, byte *tinttab)
{
    int         col = 0;
    byte        *desttop = screens[0] + y * SCREENWIDTH + x;
    int         w = SHORT(patch->width);

    for (; col < w; col++, desttop++)
    {
        column_t        *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            byte        *source = (byte *)column + 3;
            byte        *dest = desttop + column->topdelta * SCREENWIDTH;
            int         count = column->length;

            while (count--)
            {
                *dest = tinttab[(*source++ << 8) + *dest];
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

void V_DrawTranslucentStatusNumberPatch(int x, int y, patch_t *patch, byte *tinttab)
{
    int         col = 0;
    byte        *desttop = screens[0] + y * SCREENWIDTH + x;
    int         w = SHORT(patch->width);

    for (; col < w; col++, desttop++)
    {
        column_t        *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            byte        *source = (byte *)column + 3;
            byte        *dest = desttop + column->topdelta * SCREENWIDTH;
            int         count = column->length;

            while (count--)
            {
                byte    dot = *source++;

                if (dot == 109 && tinttab)
                    *dest = tinttab33[*dest];
                else
                    *dest = tinttab[(dot << 8) + *dest];
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

void V_DrawTranslucentYellowStatusPatch(int x, int y, patch_t *patch, byte *tinttab)
{
    int         col = 0;
    byte        *desttop = screens[0] + y * SCREENWIDTH + x;
    int         w = SHORT(patch->width);

    for (; col < w; col++, desttop++)
    {
        column_t        *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            byte        *source = (byte *)column + 3;
            byte        *dest = desttop + column->topdelta * SCREENWIDTH;
            int         count = column->length;

            while (count--)
            {
                *dest = tinttab75[(redtoyellow[*source++] << 8) + *dest];
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

boolean V_EmptyPatch(patch_t *patch)
{
    int col = 0;
    int w = SHORT(patch->width);

    for (; col < w; col++)
    {
        column_t        *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            if (column->length)
                return false;

            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
    
    return true;
}

//
// V_ColorBlock
//
// Draws a block of solid color.
//
void V_ColorBlock(int x, int y, int scrn, int width, int height, byte color)
{
    byte *dest;

#ifdef RANGECHECK
    if (x < 0
     || x + width > SCREENWIDTH
     || y < 0
     || y + height > SCREENHEIGHT)
    {
        C_Printf(CR_RED, " V_ColorBlock: block exceeds buffer boundaries.\n");
    }
#endif

    dest = screens[scrn] + y * SCREENWIDTH + x;
   
    while(height--)
    {
        memset(dest, color, width);
        dest += SCREENWIDTH;
    }

    R_SetViewSize (screenblocks, detailLevel);
}

//
// V_GetBlock
// Gets a linear block of pixels from the view buffer.
//
void V_GetBlock (int x, int y, int scrn, int width, int height, byte *dest)
{
    byte *src;

#ifdef RANGECHECK 
    if (x < 0
     || x + width > SCREENWIDTH
     || y < 0
     || y + height > SCREENHEIGHT)
    {
        C_Printf(CR_RED, " Bad V_GetBlock");
    }
#endif

    src = screens[scrn] + y * SCREENWIDTH + x;

    while(height--)
    {
        memcpy (dest, src, width);
        src += SCREENWIDTH;
        dest += width;
    }
}

