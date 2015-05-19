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

#include "deh_str.h"
#include "doomdef.h"
#include "doomtype.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_video.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "v_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"


// TODO: There are separate RANGECHECK defines for different games, but this
// is common code. Fix this.
#define RANGECHECK


static  patch_t*             v_font[V_FONTSIZE];

// The screen buffer that the v_video.c code draws to.
static  byte                 *dest_screen = NULL;

// haleyjd 08/28/10: clipping callback function for patches.
// This is needed for Chocolate Strife, which clips patches to the screen.
static  vpatchclipfunc_t     patchclip_callback = NULL;

// Blending table used for fuzzpatch, etc.
// Only used in Heretic/Hexen
byte    *tinttable = NULL;

// villsa [STRIFE] Blending table used for Strife
byte    *xlatab = NULL;

int     dirtybox[4]; 

int     italicize[15] = { 0, 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, -1, -1, -1 };

//
// V_MarkRect 
// 
void V_MarkRect(int x, int y, int width, int height) 
{ 
    // If we are temporarily using an alternate screen, do not 
    // affect the update box.

    if (dest_screen == I_VideoBuffer)
    {
        M_AddToBox (dirtybox, x, y); 
        M_AddToBox (dirtybox, x + width-1, y + height-1); 
    }
} 
 

//
// V_CopyRect 
// 
void V_CopyRect(int srcx, int srcy, byte *source,
                int width, int height,
                int destx, int desty)
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
     || destx + width > SCREENWIDTH
     || desty < 0
     || desty + height > SCREENHEIGHT)
    {
        I_Error ("Bad V_CopyRect");
    }
#endif 

    V_MarkRect(destx, desty, width, height); 
 
    src = source + SCREENWIDTH * srcy + srcx; 
    dest = dest_screen + SCREENWIDTH * desty + destx; 

    for ( ; height>0 ; height--) 
    { 
        memcpy(dest, src, width); 
        src += SCREENWIDTH; 
        dest += SCREENWIDTH; 
    } 
} 
 
//
// V_SetPatchClipCallback
//
// haleyjd 08/28/10: Added for Strife support.
// By calling this function, you can setup runtime error checking for patch 
// clipping. Strife never caused errors by drawing patches partway off-screen.
// Some versions of vanilla DOOM also behaved differently than the default
// implementation, so this could possibly be extended to those as well for
// accurate emulation.
//
void V_SetPatchClipCallback(vpatchclipfunc_t func)
{
    patchclip_callback = func;
}

//
// V_DrawPatch
// Masks a column based masked pic to the screen. 
//

void V_DrawPatch(int x, int y, patch_t *patch)
{ 
    int count;
    int col;
    column_t *column;
    byte *desttop;
    byte *dest;
    byte *source;
    int w, f;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    // haleyjd 08/28/10: Strife needs silent error checking here.
    if(patchclip_callback)
    {
        if(!patchclip_callback(patch, x, y))
            return;
    }

#ifdef RANGECHECK    // FIXME: DO WE NEED TO DISABLE THIS ONE FOR THE WII PORT ?!
    if (x < 0
     || x + SHORT(patch->width) > ORIGWIDTH
     || y < 0
     || y + SHORT(patch->height) > ORIGHEIGHT)
    {
        I_Error("Bad V_DrawPatch");
    }
#endif

    V_MarkRect(x, y, SHORT(patch->width), SHORT(patch->height));

    col = 0;
    desttop = dest_screen + (y << hires) * SCREENWIDTH + x;

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
                dest = desttop + column->topdelta*(SCREENWIDTH << hires) +
                       (x * hires) + f;
                count = column->length;

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
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//

void V_DrawPatchFlipped(int x, int y, patch_t *patch)
{
    int count;
    int col; 
    column_t *column; 
    byte *desttop;
    byte *dest;
    byte *source; 
    int w, f; 
 
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
     || y + SHORT(patch->height) > ORIGHEIGHT)
    {
        I_Error("Bad V_DrawPatchFlipped");
    }
#endif

    V_MarkRect (x, y, SHORT(patch->width), SHORT(patch->height));

    col = 0;
    desttop = dest_screen + (y << hires) * SCREENWIDTH + x;

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
// V_DrawPatchDirect
// Draws directly to the screen on the pc. 
//

void V_DrawPatchDirect(int x, int y, patch_t *patch)
{
    V_DrawPatch(x, y, patch); 
} 

//
// V_DrawTLPatch
//
// Masks a column based translucent masked pic to the screen.
//

void V_DrawTLPatch(int x, int y, patch_t * patch)
{
    int count, col;
    column_t *column;
    byte *desttop, *dest, *source;
    int w, f;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    if (x < 0
     || x + SHORT(patch->width) > ORIGWIDTH 
     || y < 0
     || y + SHORT(patch->height) > ORIGHEIGHT)
    {
        I_Error("Bad V_DrawTLPatch");
    }

    col = 0;
    desttop = dest_screen + (y << hires) * SCREENWIDTH + x;

    w = SHORT(patch->width);
    for (; col < w; x++, col++, desttop++)
    {
        column = (column_t *) ((byte *) patch + LONG(patch->columnofs[col]));

        // step through the posts in a column

        while (column->topdelta != 0xff)
        {
            for (f = 0; f <= hires; f++)
            {
                source = (byte *) column + 3;
                dest = desttop + column->topdelta * (SCREENWIDTH << hires) +
                       (x * hires) + f;
                count = column->length;

                while (count--)
                {
                    if (hires)
                    {
                        *dest = tinttable[((*dest) << 8) + *source];
                        dest += SCREENWIDTH;
                    }
                    *dest = tinttable[((*dest) << 8) + *source++];
                    dest += SCREENWIDTH;
                }
            }
            column = (column_t *) ((byte *) column + column->length + 4);
        }
    }
}

//
// V_DrawXlaPatch
//
// villsa [STRIFE] Masks a column based translucent masked pic to the screen.
//

void V_DrawXlaPatch(int x, int y, patch_t * patch)
{
    int count, col;
    column_t *column;
    byte *desttop, *dest, *source;
    int w, f;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    if(patchclip_callback)
    {
        if(!patchclip_callback(patch, x, y))
            return;
    }

    col = 0;
    desttop = dest_screen + (y << hires) * SCREENWIDTH + x;

    w = SHORT(patch->width);
    for(; col < w; x++, col++, desttop++)
    {
        column = (column_t *) ((byte *) patch + LONG(patch->columnofs[col]));

        // step through the posts in a column

        while(column->topdelta != 0xff)
        {
            for (f = 0; f <= hires; f++)
            {
                source = (byte *) column + 3;
                dest = desttop + column->topdelta * (SCREENWIDTH << hires) +
                       (x * hires) + f;
                count = column->length;

                while(count--)
                {
                    if (hires)
                    {
                        *dest = xlatab[*dest + ((*source) << 8)];
                        dest += SCREENWIDTH;
                    }
                    *dest = xlatab[*dest + ((*source) << 8)];
                    source++;
                    dest += SCREENWIDTH;
                }
            }
            column = (column_t *) ((byte *) column + column->length + 4);
        }
    }
}

//
// V_DrawAltTLPatch
//
// Masks a column based translucent masked pic to the screen.
//

void V_DrawAltTLPatch(int x, int y, patch_t * patch)
{
    int count, col;
    column_t *column;
    byte *desttop, *dest, *source;
    int w, f;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    if (x < 0
     || x + SHORT(patch->width) > ORIGWIDTH
     || y < 0
     || y + SHORT(patch->height) > ORIGHEIGHT)
    {
        I_Error("Bad V_DrawAltTLPatch");
    }

    col = 0;
    desttop = dest_screen + (y << hires) * SCREENWIDTH + x;

    w = SHORT(patch->width);
    for (; col < w; x++, col++, desttop++)
    {
        column = (column_t *) ((byte *) patch + LONG(patch->columnofs[col]));

        // step through the posts in a column

        while (column->topdelta != 0xff)
        {
            for (f = 0; f <= hires; f++)
            {
                source = (byte *) column + 3;
                dest = desttop + column->topdelta * (SCREENWIDTH << hires) +
                       (x * hires) + f;
                count = column->length;

                while (count--)
                {
                    if (hires)
                    {
                        *dest = tinttable[((*dest) << 8) + *source];
                        dest += SCREENWIDTH;
                    }
                    *dest = tinttable[((*dest) << 8) + *source++];
                    dest += SCREENWIDTH;
                }
            }
            column = (column_t *) ((byte *) column + column->length + 4);
        }
    }
}

//
// V_DrawShadowedPatch
//
// Masks a column based masked pic to the screen.
//

void V_DrawShadowedPatch(int x, int y, patch_t *patch)
{
    int count, col;
    column_t *column;
    byte *desttop, *dest, *source;
    byte *desttop2, *dest2;
    int w, f;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    if (x < 0
     || x + SHORT(patch->width) > ORIGWIDTH
     || y < 0
     || y + SHORT(patch->height) > ORIGHEIGHT)
    {
        I_Error("Bad V_DrawShadowedPatch");
    }

    col = 0;
    desttop = dest_screen + (y << hires) * SCREENWIDTH + x;
    desttop2 = dest_screen + ((y + 2) << hires) * SCREENWIDTH + x + 2;

    w = SHORT(patch->width);
    for (; col < w; x++, col++, desttop++, desttop2++)
    {
        column = (column_t *) ((byte *) patch + LONG(patch->columnofs[col]));

        // step through the posts in a column

        while (column->topdelta != 0xff)
        {
            for (f = 0; f <= hires; f++)
            {
                source = (byte *) column + 3;
                dest = desttop + column->topdelta * (SCREENWIDTH << hires) +
                       (x * hires) + f;
                dest2 = desttop2 + column->topdelta * (SCREENWIDTH << hires) +
                       (x * hires) + f;
                count = column->length;

                while (count--)
                {
                    if (hires)
                    {
                        *dest2 = tinttable[((*dest2) << 8)];
                        dest2 += SCREENWIDTH;
                        *dest = *source;
                        dest += SCREENWIDTH;
                    }
                    *dest2 = tinttable[((*dest2) << 8)];
                    dest2 += SCREENWIDTH;
                    *dest = *source++;
                    dest += SCREENWIDTH;
                }
            }
            column = (column_t *) ((byte *) column + column->length + 4);
        }
    }
}

//
// Load tint table from TINTTAB lump.
//

void V_LoadTintTable(void)
{
    tinttable = W_CacheLumpName("TINTTAB", PU_STATIC);
}

//
// V_LoadXlaTable
//
// villsa [STRIFE] Load xla table from XLATAB lump.
//

void V_LoadXlaTable(void)
{
    xlatab = W_CacheLumpName("XLATAB", PU_STATIC);
}

//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//

void V_DrawBlock(int x, int y, int width, int height, byte *src) 
{ 
    byte *dest; 
 
#ifdef RANGECHECK 
    if (x < 0
     || x + width >SCREENWIDTH
     || y < 0
     || y + height > SCREENHEIGHT)
    {
        I_Error ("Bad V_DrawBlock");
    }
#endif 
 
    V_MarkRect (x, y, width, height); 
 
    dest = dest_screen + (y << hires) * SCREENWIDTH + x;

    while (height--) 
    { 
        memcpy (dest, src, width); 
        src += width; 
        dest += SCREENWIDTH; 
    } 
} 

void V_DrawScaledBlock(int x, int y, int width, int height, byte *src)
{
    byte *dest;
    int i, j;

#ifdef RANGECHECK
    if (x < 0
     || x + width > ORIGWIDTH
     || y < 0
     || y + height > ORIGHEIGHT)
    {
        I_Error ("Bad V_DrawScaledBlock");
    }
#endif

    V_MarkRect (x, y, width, height);

    dest = dest_screen + (y << hires) * SCREENWIDTH + (x << hires);

    for (i = 0; i < (height << hires); i++)
    {
        for (j = 0; j < (width << hires); j++)
        {
            *(dest + i * SCREENWIDTH + j) = *(src + (i >> hires) * width +
                                            (j >> hires));
        }
    }
}

void V_DrawFilledBox(int x, int y, int w, int h, int c)
{
    uint8_t *buf, *buf1;
    int x1, y1;

    buf = I_VideoBuffer + SCREENWIDTH * y + x;

    for (y1 = 0; y1 < h; ++y1)
    {
        buf1 = buf;

        for (x1 = 0; x1 < w; ++x1)
        {
            *buf1++ = c;
        }

        buf += SCREENWIDTH;
    }
}

void V_DrawHorizLine(int x, int y, int w, int c)
{
    uint8_t *buf;
    int x1;

    buf = I_VideoBuffer + SCREENWIDTH * y + x;

    for (x1 = 0; x1 < w; ++x1)
    {
        *buf++ = c;
    }
}

void V_DrawVertLine(int x, int y, int h, int c)
{
    uint8_t *buf;
    int y1;

    buf = I_VideoBuffer + SCREENWIDTH * y + x;

    for (y1 = 0; y1 < h; ++y1)
    {
        *buf = c;
        buf += SCREENWIDTH;
    }
}

void V_DrawBox(int x, int y, int w, int h, int c)
{
    V_DrawHorizLine(x, y, w, c);
    V_DrawHorizLine(x, y+h-1, w, c);
    V_DrawVertLine(x, y, h, c);
    V_DrawVertLine(x+w-1, y, h, c);
}

//
// Draw a "raw" screen (lump containing raw data to blit directly
// to the screen)
//

void V_CopyScaledBuffer(byte *dest, byte *src, size_t size)
{
    int i, j;

#ifdef RANGECHECK
    if (size < 0
     || size > ORIGWIDTH * ORIGHEIGHT)
    {
        I_Error("Bad V_CopyScaledBuffer");
    }
#endif

    while (size--)
    {
        for (i = 0; i <= hires; i++)
        {
            for (j = 0; j <= hires; j++)
            {
                *(dest + (size << hires) + (hires *
                (int) (size / ORIGWIDTH) + i) * SCREENWIDTH + j) =
                *(src + size);
            }
        }
    }
}
 
void V_DrawRawScreen(byte *raw)
{
    V_CopyScaledBuffer(dest_screen, raw, ORIGWIDTH * ORIGHEIGHT);
}

//
// V_Init
// 
void V_Init (void) 
{ 
    // no-op!
    // There used to be separate screens that could be drawn to; these are
    // now handled in the upper layers.
}

// Set the buffer that the code draws to.

void V_UseBuffer(byte *buffer)
{
    dest_screen = buffer;
}

// Restore screen buffer to the i_video screen buffer.

void V_RestoreBuffer(void)
{
    dest_screen = I_VideoBuffer;
}

// isprint() function (win32 doesnt like it, seems)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

boolean V_IsPrint(char c)
{
    // new colour
    if (c >= 128)
    {
        int colnum = c - 128;

        if(colnum < 0 || colnum >= 10)
            return false;
        else
            return true;
    }

    // hack to make spacebar work
    if(c == ' ')
        return true;
  
    c = toupper(c) - V_FONTSTART;

    if (c >= V_FONTSIZE)
    {
        return false;
    }
  
    return v_font[c] != NULL;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"

void V_DrawConsoleChar(int x,
                       int y,
                       patch_t *patch,
                       byte color,
                       boolean italics
                       /*, int translucency*/)
{
    int         col = 0;
    byte        *desttop = dest_screen + (y << hires) * SCREENWIDTH + x;
    int         w = SHORT(patch->width);

    for (; col < w; col++, desttop++)
    {
        column_t        *column = (column_t *)((byte *)patch +
                                  LONG(patch->columnofs[col]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            byte        *source = (byte *)column + 3;
            byte        *dest = desttop + column->topdelta * SCREENWIDTH;
            int         count = column->length;

            while (count--)
            {
                if (y + column->topdelta + column->length - count > 0)
                {
                    if (*source == 160)
                    {
                        if (italics)
                            *(dest + italicize[column->topdelta +
                                    column->length - count]) = color;
                        else if(!italics)
                            *dest = color;
/*
                        else
                            *dest = (translucency == 1 ?
                                    tinttab25[(color << 8) + *dest] :
                                    (translucency == 2 ?
                                    tinttab25[(*dest << 8) + color] : color));
*/
                    }
                    *(source++);
                }
                dest += SCREENWIDTH;
            }
            column = (column_t *)((byte *)column + column->length + 4);
        }
    }
}

