// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005 Simon Howard
//
// Copyright(C) 2015 by Brad Harding: - (Video Drawing Functions for Console & Status Bar)
//                                    - (Shadowed Main Menu Background Items)
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


#include <libpng15/png.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "c_io.h"
#include "d_deh.h"

#include "doom/doomdef.h"

#include "doomtype.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_tinttab.h"
#include "i_video.h"
#include "m_bbox.h"
#include "m_misc.h"

#include "doom/r_main.h"
#include "doom/r_plane.h"

#include "v_misc.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"


// TODO: There are separate RANGECHECK defines for different games, but this
// is common code. Fix this.
#define RANGECHECK

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


// [crispy] four different rendering functions
// for each possible combination of dp_translation and dp_translucent:
// (1) normal, opaque patch
static const inline byte drawpatchpx00(const byte dest, const byte source)
{
    return source;
}

// (2) color-translated, opaque patch
static const inline byte drawpatchpx01(const byte dest, const byte source)
{
    return dp_translation[source];
}

// (3) normal, translucent patch
static const inline byte drawpatchpx10(const byte dest, const byte source)
{
    return tranmap[(dest << 8) + source];
}

// (4) color-translated, translucent patch
static const inline byte drawpatchpx11(const byte dest, const byte source)
{
    return tranmap[(dest << 8) + dp_translation[source]];
}


char                         *d_lowpixelsize = "2x2";

// The screen buffer that the v_video.c code draws to.

//byte                       *dest_screen = NULL;

// Each screen is [SCREENWIDTH * SCREENHEIGHT];
byte                         *screens[5];

byte                         *tranmap = NULL;
byte                         *dp_translation = NULL;

int                          dirtybox[4]; 
int                          pixelwidth;
int                          pixelheight;
int                          italicize[15] = { 0, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, -1, -1, -1 };

dboolean                     dp_translucent = false;


extern byte                  redtoblue[];
extern byte                  redtogreen[];


//
// V_MarkRect 
// 
// [nitr8] UNUSED
//
/*
void V_MarkRect(int x, int y, int srcscrn, int width, int height, int destscrn) 
{ 
    // If we are temporarily using an alternate screen, do not 
    // affect the update box.

//    if (dest_screen == I_VideoBuffer)
    if (screens[srcscrn] == screens[destscrn])
    {
        M_AddToBox (dirtybox, x, y); 
        M_AddToBox (dirtybox, x + width-1, y + height-1); 
    }
} 
 
void V_DrawHorizLine(int x, int y, int scrn, int w, int c)
{
    uint8_t *buf;
    int x1;

#ifdef RANGECHECK 
    if (x < 0
     || x + w > SCREENWIDTH
     || y < 0
     || y > SCREENHEIGHT)
    {
        C_Error("Bad V_DrawHorizLine: (%d,%d,%d,%d)"
                , x, x+w, y);
    }
#endif 

    // [crispy] prevent framebuffer overflows
    if (x + w > SCREENWIDTH)
        w = SCREENWIDTH - x;

//    buf = I_VideoBuffer + SCREENWIDTH * y + x;
    buf = screens[scrn] + SCREENWIDTH * y + x;

    for (x1 = 0; x1 < w; ++x1)
    {
        *buf++ = c;
    }
}
*/

//
// V_CopyRect 
// 
void
V_CopyRect
( int        srcx,
  int        srcy,
  int        srcscrn,
//  byte*      source,
  int        width,
  int        height,
  int        destx,
  int        desty,
  int        destscrn) 
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
        C_Error("Bad V_CopyRect: Patch (%d,%d)-(%d,%d). Dest.: (%d,%d) exceeds LFB"
                , srcx, srcy, srcx + width, srcy + height, destx, desty);
    }
#endif 

    // [crispy] prevent framebuffer overflow
    if (destx + width > SCREENWIDTH)
        width = SCREENWIDTH - destx;
    if (desty + height > SCREENHEIGHT)
        height = SCREENHEIGHT - desty;
/*
    V_MarkRect(destx, desty, 0, width, height); 

    src = source + SCREENWIDTH * srcy + srcx;
    dest = dest_screen + SCREENWIDTH * desty + destx;
*/
    src = screens[srcscrn] + SCREENWIDTH * srcy + srcx;
    dest = screens[destscrn] + SCREENWIDTH * desty + destx;

    for ( ; height>0 ; height--) 
    { 
        memcpy(dest, src, width * sizeof(*dest)); 
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
  patch_t*   patch ) 
{ 
    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w, f, tmpy;

    // [crispy] four different rendering functions
    const byte (* drawpatchpx) (const byte dest, const byte source) =
        (!dp_translucent ?
        (!dp_translation ? drawpatchpx00 : drawpatchpx01) :
        (!dp_translation ? drawpatchpx10 : drawpatchpx11));

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

#ifdef RANGECHECK
    if (x < 0
     || x + SHORT(patch->width) > ORIGINALWIDTH
     || y < 0
     || y + SHORT(patch->height) > ORIGINALHEIGHT)
    {
        C_Error("Bad V_DrawPatch: Patch (%d,%d) exceeds LFB", x, y);
    }
#endif

//    V_MarkRect(x, y, 0, SHORT(patch->width), SHORT(patch->height));

    col = 0;

//    desttop = dest_screen + (y << hires) * SCREENWIDTH + x;
    desttop = screens[scrn] + (y << hires) * SCREENWIDTH + x;

    w = SHORT(patch->width);

    for ( ; col<w ; x++, col++, desttop++)
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
 
                // [crispy] prevent framebuffer overflow
                tmpy = y + column->topdelta;

                // [crispy] too far left
                if (x < 0)
                {
                    continue;
                }

                // [crispy] too far right / width
                if (x >= ORIGINALWIDTH)
                {
                    break;
                }

                // [crispy] too high
                while (tmpy < 0)
                {
                    count--;
                    source++;
                    dest += (SCREENWIDTH << hires);
                    tmpy++;
                }

                // [crispy] too low / height
                while (tmpy + count > ORIGINALHEIGHT)
                {
                    count--;
                }

                // [crispy] nothing left to draw?
                if (count < 1)
                {
                    continue;
                }

                while (count--)
                {
                    if (hires)
                    {
                        *dest = drawpatchpx(*dest, *source);
                        dest += SCREENWIDTH;
                    }
                    *dest = drawpatchpx(*dest, *source++);
                    dest += SCREENWIDTH;
                }
            }
            column = (column_t *)((byte *)column + column->length + 4);
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
  patch_t*   patch ) 
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

#ifdef RANGECHECK
    if (x < 0
     || x + SHORT(patch->width) > ORIGINALWIDTH
     || y < 0
     || y + SHORT(patch->height) > ORIGINALHEIGHT)
    {
        C_Error("Bad V_DrawPatchFlipped: Patch (%d,%d)-(%d,%d) exceeds LFB"
                , x, y, x + SHORT(patch->width), y + SHORT(patch->height));
    }
#endif

//    V_MarkRect (x, y, 0, SHORT(patch->width), SHORT(patch->height));

    col = 0;
//    desttop = dest_screen + (y << hires) * SCREENWIDTH + x;
    desttop = screens[scrn] + (y << hires) * SCREENWIDTH + x;

    w = SHORT(patch->width);

    for ( ; col<w ; x++, col++, desttop++)
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

                // [crispy] prevent framebuffer overflow
                tmpy = y + column->topdelta;

                // [crispy] too far left
                if (x < 0)
                {
                    continue;
                }

                // [crispy] too far right / width
                if (x >= ORIGINALWIDTH)
                {
                    break;
                }

                // [crispy] too high
                while (tmpy < 0)
                {
                    count--;
                    source++;
                    dest += (SCREENWIDTH << hires);
                    tmpy++;
                }

                // [crispy] too low / height
                while (tmpy + count > ORIGINALHEIGHT)
                {
                    count--;
                }

                // [crispy] nothing left to draw?
                if (count < 1)
                {
                    continue;
                }

                while (count--)
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
  byte*      src ) 
{ 
    byte *dest; 
 
#ifdef RANGECHECK 
    if (x < 0
     || x + width >SCREENWIDTH
     || y < 0
     || y + height > SCREENHEIGHT)
    {
        C_Error("Bad V_DrawBlock: Patch (%d,%d)-(%d,%d) exceeds LFB"
                , x, y, x + width, y + height);
    }
#endif 
/*
    V_MarkRect (x, y, 0, width, height); 
 
    dest = dest_screen + (y << hires) * SCREENWIDTH + x;
*/
    dest = screens[scrn] + (y << hires) * SCREENWIDTH + x;

    while (height--) 
    { 
        memcpy (dest, src, width * sizeof(*dest)); 
        src += width; 
        dest += SCREENWIDTH; 
    } 
} 

void GetPixelSize(void)
{
    int     width = -1;
    int     height = -1;
    char    *left = strtok(strdup(d_lowpixelsize), "x");
    char    *right = strtok(NULL, "x");

    if (!right)
        right = "";

    sscanf(left, "%10i", &width);
    sscanf(right, "%10i", &height);

    if (width > 0 && width <= SCREENWIDTH && height > 0 && height <= SCREENHEIGHT
        && (width >= 2 || height >= 2))
    {
        pixelwidth = width;
        pixelheight = height;
    }
    else
    {
        pixelwidth = 2;
        pixelheight = 2;
        d_lowpixelsize = "2x2";

//        M_SaveCVARs();
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

    GetPixelSize();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"

void V_DrawConsoleChar(int x, int y, int scrn, patch_t *patch, int color1, int color2, dboolean italics, byte *tinttab)
{
    int         col = 0;
//    byte        *desttop = dest_screen + y * SCREENWIDTH + x;
    byte        *desttop = screens[scrn] + y * SCREENWIDTH + x;
    int         w = SHORT(patch->width);

    for (; col < w; col++, desttop++)
    {
        column_t        *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
        byte            topdelta;

        // step through the posts in a column
        while ((topdelta = column->topdelta) != 0xff)
        {
            byte        *source = (byte *)column + 3;
            byte        *dest = desttop + topdelta * SCREENWIDTH;
            byte        length = column->length;
            int         count = length;

            while (count--)
            {
                int     height = topdelta + length - count;

                if (y + height > CONSOLETOP)
                {
                    if (color2 == -1)
                    {
                        if (*source)
                        {
                            if (italics)
                                *(dest + italicize[height]) = (!tinttab ? color1 :
                                    tinttab[(color1 << 8) + *(dest + italicize[height])]);
                            else
                                *dest = (!tinttab ? color1 : tinttab[(color1 << 8) + *dest]);
                        }
                    }
                    else if (*source == 4) // (4 = WHITE)
                        *dest = color1;
                    else if (*dest != color1)
                        *dest = color2;
                }
                ++source;
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + length + 4);
        }
    }
}

void V_DrawHUDPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab)
{
    int         col = 0;
    byte        *desttop;
    int         w;

    if (!tinttab)
        return;

//    desttop = dest_screen + y * SCREENWIDTH + x;
    desttop = screens[scrn] + y * SCREENWIDTH + x;
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

void V_DrawYellowHUDPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab)
{
    int         col = 0;
    byte        *desttop;
    int         w;

    if (!tinttab)
        return;

//    desttop = dest_screen + y * SCREENWIDTH + x;
    desttop = screens[scrn] + y * SCREENWIDTH + x;
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

void V_DrawTranslucentHUDPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab)
{
    int         col = 0;
//    byte        *desttop = dest_screen + y * SCREENWIDTH + x;
    byte        *desttop = screens[scrn] + y * SCREENWIDTH + x;
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

void V_DrawTranslucentHUDNumberPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab)
{
    int         col = 0;
//    byte        *desttop = dest_screen + y * SCREENWIDTH + x;
    byte        *desttop = screens[scrn] + y * SCREENWIDTH + x;
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

                *dest = (dot == 109 ? tinttab33[*dest] : tinttab[(dot << 8) + *dest]);
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

void V_DrawTranslucentYellowHUDPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab)
{
    int         col = 0;
//    byte        *desttop = dest_screen + y * SCREENWIDTH + x;
    byte        *desttop = screens[scrn] + y * SCREENWIDTH + x;
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

dboolean V_EmptyPatch(patch_t *patch)
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
// [nitr8] UNUSED
//
/*
void V_ColorBlock(int x, int y, int scrn, int width, int height, byte color)
{
    byte *dest;

#ifdef RANGECHECK
    if (x < 0
     || x + width > SCREENWIDTH
     || y < 0
     || y + height > SCREENHEIGHT)
    {
        C_Error("V_ColorBlock: block exceeds buffer boundaries.");
    }
#endif

//    dest = dest_screen + y * SCREENWIDTH + x;
    dest = screens[scrn] + y * SCREENWIDTH + x;
   
    while(height--)
    {
        memset(dest, color, width);
        dest += SCREENWIDTH;
    }

    R_SetViewSize (screenblocks);
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
        C_Error("Bad V_GetBlock");
    }
#endif

//    src = dest_screen + y * SCREENWIDTH + x;
    src = screens[scrn] + y * SCREENWIDTH + x;

    while(height--)
    {
        memcpy (dest, src, width);
        src += SCREENWIDTH;
        dest += width;
    }
}
*/

//
// SCREEN SHOTS
//

typedef struct
{
    char                manufacturer;
    char                version;
    char                encoding;
    char                bits_per_pixel;

    unsigned short        xmin;
    unsigned short        ymin;
    unsigned short        xmax;
    unsigned short        ymax;
    
    unsigned short        hres;
    unsigned short        vres;

    unsigned char        palette[48];
    
    char                reserved;
    char                color_planes;
    unsigned short        bytes_per_line;
    unsigned short        palette_type;
    
    char                filler[58];
    unsigned char        data;                // unbounded
} PACKEDATTR pcx_t;


//
// WritePCXfile
//

void WritePCXfile(char *filename, byte *data,
                  int width, int height,
                  byte *palette)
{
    int                i;
    int                length;
    pcx_t*        pcx;
    byte*        pack;
        
    pcx = Z_Malloc (width*height*2+1000, PU_STATIC, NULL);

    pcx->manufacturer = 0x0a;                // PCX id
    pcx->version = 5;                        // 256 color
    pcx->encoding = 1;                        // uncompressed
    pcx->bits_per_pixel = 8;                // 256 color
    pcx->xmin = 0;
    pcx->ymin = 0;
    pcx->xmax = SHORT(width-1);
    pcx->ymax = SHORT(height-1);
    pcx->hres = SHORT(width);
    pcx->vres = SHORT(height);
    memset (pcx->palette,0,sizeof(pcx->palette));
    pcx->color_planes = 1;                // chunky image
    pcx->bytes_per_line = SHORT(width);
    pcx->palette_type = SHORT(2);        // not a grey scale
    memset (pcx->filler,0,sizeof(pcx->filler));

    // pack the image
    pack = &pcx->data;
        
    for (i=0 ; i<width*height ; i++)
    {
        if ( (*data & 0xc0) != 0xc0)
            *pack++ = *data++;
        else
        {
            *pack++ = 0xc1;
            *pack++ = *data++;
        }
    }
    
    // write the palette
    *pack++ = 0x0c;        // palette ID byte
    for (i=0 ; i<768 ; i++)
        *pack++ = *palette++;
    
    // write output file
    length = pack - (byte *)pcx;
    M_WriteFile (filename, pcx, length);

    Z_Free (pcx);
}

void LoadPCX(char *filename, byte **pic, byte **palette, int *width, int *height)
{
    byte    *raw;
    byte    *out;
    byte    *pix;

    pcx_t   *pcx;

    int     x;
    int     y;
    int     len;
    int     dataByte;
    int     runLength;

    *pic = NULL;

    //
    // load the file
    //
    len = M_ReadFile (filename, &raw);

    if (!raw)
    {
        C_Error("Bad PCX file: wrong raw data in file %s", filename);
        return;
    }

    //
    // parse the PCX file
    //
    pcx = (pcx_t *)raw;

    pcx->xmin = SHORT(pcx->xmin);
    pcx->ymin = SHORT(pcx->ymin);
    pcx->xmax = SHORT(pcx->xmax);
    pcx->ymax = SHORT(pcx->ymax);
    pcx->hres = SHORT(pcx->hres);
    pcx->vres = SHORT(pcx->vres);
    pcx->bytes_per_line = SHORT(pcx->bytes_per_line);
    pcx->palette_type = SHORT(pcx->palette_type);

    raw = &pcx->data;

    if (pcx->manufacturer != 0x0a
        || pcx->version != 5
        || pcx->encoding != 1
        || pcx->bits_per_pixel != 8
        || pcx->xmax >= 640
        || pcx->ymax >= 480)
    {
        C_Error("Bad PCX file: wrong format in file %s", filename);
        return;
    }

    out = malloc((pcx->ymax + 1) * (pcx->xmax + 1));

    *pic = out;

    pix = out;

    if (palette)
    {
        *palette = malloc(768);
        memcpy (*palette, (byte *)pcx + len - 768, 768);
    }

    if (width)
        *width = pcx->xmax + 1;

    if (height)
        *height = pcx->ymax + 1;

    for (y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1)
    {
        for (x = 0; x <= pcx->xmax;)
        {
            dataByte = *raw++;

            if ((dataByte & 0xC0) == 0xC0)
            {
                runLength = dataByte & 0x3F;
                dataByte = *raw++;
            }
            else
                runLength = 1;

            while (runLength-- > 0)
                pix[x++] = dataByte;
        }
    }

    if (raw - (byte *)pcx > len)
    {
        C_Error("PCX file %s was malformed", filename);
        free (*pic);
        *pic = NULL;
    }

    Z_Free(pcx);
}

//#ifdef HAVE_LIBPNG
//
// WritePNGfile
//

static void error_fn(png_structp p, png_const_charp s)
{
#ifndef WII
    printf("libpng error: %s\n", s);
#endif
    C_Error("libpng error: %s", s);
}

static void warning_fn(png_structp p, png_const_charp s)
{
#ifndef WII
    printf("libpng warning: %s\n", s);
#endif
    C_Warning("libpng warning: %s", s);
}

void WritePNGfile(char *filename, byte *data,
                  int inwidth, int inheight,
                  byte *palette)
{
    png_structp ppng;
    png_infop pinfo;
    png_colorp pcolor;
    FILE *handle;
    int i;
    int width, height;
    byte *rowbuf;

    // scale up to accommodate aspect ratio correction
    width = inwidth * 5;
    height = inheight * 6;

    handle = fopen(filename, "wb");
    if (!handle)
    {
        return;
    }

    ppng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                   error_fn, warning_fn);
    if (!ppng)
    {
        fclose(handle);

        return;
    }

    pinfo = png_create_info_struct(ppng);
    if (!pinfo)
    {
        png_destroy_write_struct(&ppng, NULL);

        fclose(handle);

        return;
    }

    png_init_io(ppng, handle);

    png_set_IHDR(ppng, pinfo, width, height,
                 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    pcolor = malloc(sizeof(*pcolor) * 256);
    if (!pcolor)
    {
        png_destroy_write_struct(&ppng, &pinfo);
        return;
    }

    for (i = 0; i < 256; i++)
    {
        pcolor[i].red   = *(palette + 3 * i);
        pcolor[i].green = *(palette + 3 * i + 1);
        pcolor[i].blue  = *(palette + 3 * i + 2);
    }

    png_set_PLTE(ppng, pinfo, pcolor, 256);
    free(pcolor);

    png_write_info(ppng, pinfo);

    rowbuf = malloc(width);

    if (rowbuf)
    {
        for (i = 0; i < SCREENHEIGHT; i++)
        {
            int j;

            // expand the row 5x
            for (j = 0; j < SCREENWIDTH; j++)
            {
                memset(rowbuf + j * 5, *(data + i*SCREENWIDTH + j), 5);
            }

            // write the row 6 times
            for (j = 0; j < 6; j++)
            {
                png_write_row(ppng, rowbuf);
            }
        }

        free(rowbuf);
    }

    png_write_end(ppng, pinfo);
    png_destroy_write_struct(&ppng, &pinfo);
    fclose(handle);
}
//#endif

//
// V_ScreenShot
//

    char lbmname[60]; // haleyjd 20110213: BUG FIX - 12 is too small!
void V_ScreenShot(int scrn, char *format)
{
    int i;
    char *ext;
    
    // find a file name to save it to

//#ifdef HAVE_LIBPNG
    extern int png_screenshots;
    if (png_screenshots)
    {
        ext = "png";
    }
    else
//#endif
    {
        ext = "pcx";
    }

    for (i=0; i<=99; i++)
    {
        M_snprintf(lbmname, sizeof(lbmname), format, i, ext);

        if (!M_FileExists(lbmname))
        {
            break;      // file doesn't exist
        }
    }

    if (i == 100)
    {
        I_Error ("V_ScreenShot: Couldn't create a PCX");
    }

//#ifdef HAVE_LIBPNG
    if (png_screenshots)
    {
/*
        WritePNGfile(lbmname, I_VideoBuffer,
                SCREENWIDTH, SCREENHEIGHT,
                W_CacheLumpName("PLAYPAL", PU_CACHE));
*/
        WritePNGfile(lbmname, screens[scrn],
                SCREENWIDTH, SCREENHEIGHT,
                W_CacheLumpName("PLAYPAL", PU_CACHE));
    }
    else
//#endif
    {
        // save the pcx file
/*
        WritePCXfile(lbmname, I_VideoBuffer,
                SCREENWIDTH, SCREENHEIGHT,
                W_CacheLumpName("PLAYPAL", PU_CACHE));
*/
        WritePCXfile(lbmname, screens[scrn],
                SCREENWIDTH, SCREENHEIGHT,
                W_CacheLumpName("PLAYPAL", PU_CACHE));
    }
}

void V_LowGraphicDetail(int scrn, int height)
{
    int x, y;
    int h = pixelheight * SCREENWIDTH;

    for (y = 0; y < height; y += h)
        for (x = 0; x < SCREENWIDTH; x += pixelwidth)
        {
//            byte        *dot = dest_screen + y + x;
            byte        *dot = screens[scrn] + y + x;
            int         xx, yy;

            for (yy = 0; yy < h; yy += SCREENWIDTH)
                for (xx = 0; xx < pixelwidth; xx++)
                    *(dot + yy + xx) = *dot;
        }
}

void V_DrawPatchWithShadow(int x, int y, int scrn, patch_t *patch, dboolean flag)
{
    int         col = 0;
    byte        *desttop;
    int         w = SHORT(patch->width) << FRACBITS;
    fixed_t     DX = (SCREENWIDTH << FRACBITS) / ORIGINALWIDTH;
    fixed_t     DXI = (ORIGINALWIDTH << FRACBITS) / SCREENWIDTH;
    fixed_t     DY = (SCREENHEIGHT << FRACBITS) / ORIGINALHEIGHT;
    fixed_t     DYI = (ORIGINALHEIGHT << FRACBITS) / SCREENHEIGHT;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

//    desttop = dest_screen + ((y * DY) >> FRACBITS) * SCREENWIDTH + ((x * DX) >> FRACBITS);
    desttop = screens[scrn] + ((y * DY) >> FRACBITS) * SCREENWIDTH + ((x * DX) >> FRACBITS);

    for (; col < w; col += DXI, desttop++)
    {
        column_t        *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col >> FRACBITS]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            byte        *source = (byte *)column + 3;
            byte        *dest = desttop + ((column->topdelta * DY) >> FRACBITS) * SCREENWIDTH;
            int         count = (column->length * DY) >> FRACBITS;
            int         srccol = 0;

            while (count--)
            {
                int     height = (((y + column->topdelta + column->length) * DY) >> FRACBITS) - count;

                if (height > 0)
                    *dest = source[srccol >> FRACBITS];

                dest += SCREENWIDTH;

                if (height + 2 > 0)
                {
                    byte        *shadow = dest + SCREENWIDTH + 2;

                    if (!flag || (*shadow != 47 && *shadow != 191))
                        *shadow = (d_translucency ? tinttab50[*shadow] : 0);
                }
                srccol += DYI;
            }

            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

// Set the buffer that the code draws to.
//
// [nitr8] UNUSED
//
/*
void V_UseBuffer(int srcscrn, int destscrn)
{
//    dest_screen = buffer;
    screens[srcscrn] = screens[destscrn];
}

// Restore screen buffer to the i_video screen buffer.

void V_RestoreBuffer(int srcscrn, int destscrn)
{
//    dest_screen = I_VideoBuffer;
    screens[srcscrn] = screens[destscrn];
}

//
// V_DrawBackground tiles a 64x64 patch over the entire screen,
// providing the background for the Help and Setup screens.
//

static void V_TileFlat(byte *src, byte *dest)
{
    int x, y;

//    V_MarkRect (0, 0, 0, SCREENWIDTH, SCREENHEIGHT);

    for (y = 0; y < SCREENHEIGHT; y++)
    {
        for (x = 0; x < SCREENWIDTH / 64; x++)
        {
            memcpy (dest, src + ((y & 63) << 6), 64);

            dest += 64;
        }
    }
}

void V_DrawDistortedBackground(int scrn, char *patchname)
{
    byte *src;

//    V_MarkRect (0, 0, 0, SCREENWIDTH, SCREENHEIGHT);

    src = R_DistortedFlat(R_FlatNumForName(patchname));

    V_TileFlat(src, screens[scrn]);
}
*/

//
// V_FillRect
//

void V_FillRect(int x, int y, int scrn, int width, int height, byte color)
{
//    byte        *dest = I_VideoBuffer + y * SCREENWIDTH + x;
    byte        *dest = screens[scrn] + y * SCREENWIDTH + x;

    while (height--)
    {
        memset(dest, color, width);
        dest += SCREENWIDTH;
    }
}

// [RH] Set an area to a specified color
//
// [nitr8] UNUSED
//
/*
void V_Clear(int left, int top, int right, int bottom, int scrn, int color)
{
    int x, y;

    byte *dest;

    dest = screens[scrn] + top * SCREENWIDTH + left;
    x = right - left;
    for (y = top; y < bottom; y++)
    {
        memset (dest, color, x);
        dest += SCREENWIDTH;
    }
}
*/

