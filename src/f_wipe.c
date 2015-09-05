// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
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
//        Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------


#include <string.h>

#include "doomdef.h"
#include "doomtype.h"
#include "f_wipe.h"
#include "i_video.h"
#include "m_fixed.h"
#include "m_random.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"


// [RH] Fire Wipe
#define FIREWIDTH     64
#define FIREHEIGHT    64


//
//                       SCREEN WIPE PACKAGE
//


// when zero, stop the wipe
static   boolean  go = 0;

static   int      density;
static   int      burntime;
static   int      *y;

static   byte     *burnarray;
static   byte     *wipe_scr_start;
static   byte     *wipe_scr_end;
static   byte     *wipe_scr;
static   byte     RGB32k[32 * 32 * 32];

unsigned int      Col2RGB8[65][256];

extern   byte     *transtables;


void wipe_shittyColMajorXform(short *array, int width, int height)
{
    int           x;
    int           y;

    short         *dest;

    dest = (short *) Z_Malloc(width * height * 2, PU_STATIC, 0);

    for(y = 0; y < height; y++)
    {
        for(x = 0; x < width; x++)
        {
            dest[x * height + y] = array[y * width + x];
        }
    }

    memcpy(array, dest, width * height * 2);

    Z_Free(dest);
}

// Burn -------------------------------------------------------------

int wipe_initBurn (int width, int height, int ticks)
{
    burnarray = Z_Malloc(FIREWIDTH * (FIREHEIGHT + 4), PU_STATIC, 0);

    memset(burnarray, 0, FIREWIDTH * (FIREHEIGHT + 4));

    density = 4;

    burntime = 0;

    return 0;
}

int wipe_CalcBurn (byte *burnarray, int width, int height, int density)
{
    // This is a modified version of the fire that was once used
    // on the player setup menu.
    static int    voop;

    int           a;
    int           b;

    byte          *from;

    // generator
    from = &burnarray[width * height];

    b = voop;

    voop += density / 3;

    for (a = 0; a < density / 8; a++)
    {
        unsigned int offs = (a + b) & (width - 1);
        unsigned int v = M_Random();

        v = MIN(from[offs] + 4 + (v & 15) + (v >> 3) + (M_Random() & 31), 255u);

        from[offs] = from[width * 2 + ((offs + width * 3 / 2) & (width - 1))] = v;
    }

    density = MIN(density + 10, width * 7);

    from = burnarray;

    for (b = 0; b <= height; b += 2)
    {
        byte *pixel = from;

        // special case: first pixel on line
        byte *p = pixel + (width << 1);

        unsigned int top = *p + *(p + width - 1) + *(p + 1);
        unsigned int bottom = *(pixel + (width << 2));
        unsigned int c1 = (top + bottom) >> 2;

        if (c1 > 1) 
        {
            c1--;
        }

        *pixel = c1;
        *(pixel + width) = (c1 + bottom) >> 1;
        pixel++;

        // main line loop
        for (a = 1; a < width - 1; a++)
        {
            // sum top pixels
            p = pixel + (width << 1);

            top = *p + *(p - 1) + *(p + 1);

            // bottom pixel
            bottom = *(pixel + (width << 2));

            // combine pixels
            c1 = (top + bottom) >> 2;

            if (c1 > 1)
            {
                c1--;
            }

            // store pixels
            *pixel = c1;
            *(pixel + width) = (c1 + bottom) >> 1;        // interpolate

            // next pixel
            pixel++;
        }

        // special case: last pixel on line
        p = pixel + (width << 1);

        top = *p + *(p - 1) + *(p - width + 1);

        bottom = *(pixel + (width << 2));

        c1 = (top + bottom) >> 2;

        if (c1 > 1)
        {
            c1--;
        }

        *pixel = c1;
        *(pixel + width) = (c1 + bottom) >> 1;

        // next line
        from += width << 1;
    }

    // Check for done-ness. (Every pixel with level 126 or higher counts as done.)
    for (a = width * height, from = burnarray; a != 0; --a, ++from)
    {
        if (*from < 126)
        {
            return density;
        }
    }
    return -1;
}

int wipe_doBurn (int width, int height, int ticks)
{
    boolean       done;

    burntime += ticks;

    // Make the fire burn
    done = false;

    while (!done && ticks--)
    {
        density = wipe_CalcBurn(burnarray, FIREWIDTH, FIREHEIGHT, density);

        done = (density < 0);
    }

    // Draw the screen
    fixed_t       xstep;
    fixed_t       ystep;

    fixed_t       firex;
    fixed_t       firey;

    int           x;
    int           y;

    byte          *to;
    byte          *fromold;
    byte          *fromnew;

    xstep = (FIREWIDTH * FRACUNIT) / height;
    ystep = (FIREHEIGHT * FRACUNIT) / height;

    to = I_VideoBuffer;

    fromold = (byte *) wipe_scr_start;
    fromnew = (byte *) wipe_scr_end;

    for (y = 0, firey = 0; y < height; y++, firey += ystep)
    {
        for (x = 0, firex = 0; x < width; x++, firex += xstep)
        {
            int fglevel;

            fglevel = burnarray[(firex >> FRACBITS) + (firey >> FRACBITS) * FIREWIDTH] / 2;

            if (fglevel >= 63)
            {
                to[x] = fromnew[x];
            }
            else if (fglevel == 0)
            {
                to[x] = fromold[x];

                done = false;
            }
            else
            {
                int bglevel = 64 - fglevel;

                unsigned int *fg2rgb = Col2RGB8[fglevel];
                unsigned int *bg2rgb = Col2RGB8[bglevel];

                unsigned int fg = fg2rgb[fromnew[x]];
                unsigned int bg = bg2rgb[fromold[x]];

                fg = (fg + bg) | 0x1f07c1f;

                to[x] = RGB32k[fg & (fg >> 15)];

                done = false;
            }
        }
        fromold += width;
        fromnew += width;

        to += width;
    }
    return done || (burntime > 40);
}

int wipe_exitBurn (int width, int height, int ticks)
{
    Z_Free(burnarray);
    Z_Free(wipe_scr_start);
    Z_Free(wipe_scr_end);

    // we DEFINITELY need to do this
    ST_doRefresh();

    return 0;
}

int wipe_initFade(int width, int height, int ticks)
{
    memcpy(wipe_scr, wipe_scr_start, width * height);

    return 0;
}

int wipe_doFade(int width, int height, int ticks)
{
    boolean       changed;

    byte*         w;
    byte*         e;
    byte          newval;

    changed = true;

    ticks >>= hires;

    while (ticks--)
    {        
        w = wipe_scr;
        e = wipe_scr_end;        
        
        while (w != wipe_scr + width * height)
        {
            if (*w != *e)
            {
                if ((newval = transtables[(*e << 8) + *w + ((2 - 1) << 16)]) == *w)
                {
                    if ((newval = transtables[(*e << 8) + *w + ((1 - 1) << 16)]) == *w)
                    {
                        if ((newval = transtables[(*w << 8) + *e + ((2 - 1) << 16)]) == *w)
                        {
                            newval = *e;
                        }
                    }
                }
                *w = newval;

                changed = false;
            }
            w++;
            e++;
        }
    }
    return changed;
}

int wipe_exitFade(int width, int height, int ticks)
{
    Z_Free(wipe_scr_start);
    Z_Free(wipe_scr_end);

    return 0;
}


int wipe_initMelt(int width, int height, int ticks)
{
    int           i;
    int           r;
    
    // copy start screen to main screen
    memcpy(wipe_scr, wipe_scr_start, width * height);
    
    // makes this wipe faster (in theory)
    // to have stuff in column-major format
    wipe_shittyColMajorXform((short *) wipe_scr_start, width / 2, height);
    wipe_shittyColMajorXform((short *) wipe_scr_end, width / 2, height);
    
    // setup initial column positions
    // (y<0 => not ready to scroll yet)
    y = (int *) Z_Malloc(width * sizeof(int), PU_STATIC, 0);
    y[0] = -(M_Random() % 16);

    for (i = 1; i < width; i++)
    {
        r = (M_Random() % 3) - 1;

        y[i] = y[i - 1] + r;

        if (y[i] > 0)
        {
            y[i] = 0;
        }
        else if (y[i] == -16)
        {
            y[i] = -15;
        }
    }
    return 0;
}

int wipe_doMelt(int width, int height, int ticks)
{
    int           i;
    int           j;
    int           dy;
    int           idx;
    
    short         *s;
    short         *d;

    boolean       done = true;

    width /= 2;

    while (ticks--)
    {
        for (i = 0; i < width; i++)
        {
            if (y[i] < 0)
            {
                y[i]++;

                done = false;
            }
            else if (y[i] < height)
            {
                dy = (y[i] < 16) ? y[i] + 1 : 8;

                if (y[i] + dy >= height)
                {
                    dy = height - y[i];
                }

                s = &((short *) wipe_scr_end)[i * height + y[i]];
                d = &((short *) wipe_scr)[y[i] * width + i];

                idx = 0;

                for (j = dy; j; j--)
                {
                    d[idx] = *(s++);

                    idx += width;
                }

                y[i] += dy;

                s = &((short *) wipe_scr_start)[i * height];
                d = &((short *) wipe_scr)[y[i] * width + i];

                idx = 0;

                for (j = height - y[i]; j; j--)
                {
                    d[idx] = *(s++);

                    idx += width;
                }
                done = false;
            }
        }
    }
    return done;
}

int wipe_exitMelt(int width, int height, int ticks)
{
    Z_Free(y);
    Z_Free(wipe_scr_start);
    Z_Free(wipe_scr_end);

    return 0;
}

int wipe_StartScreen(int x, int y, int width, int height )
{
    wipe_scr_start = Z_Malloc(width * height, PU_STATIC, NULL);

    I_ReadScreen(wipe_scr_start);

//    V_GetBlock (0, 0, width, height, wipe_scr_start);

    return 0;
}

int wipe_EndScreen(int x, int y, int width, int height)
{
    wipe_scr_end = Z_Malloc(width * height, PU_STATIC, NULL);

//    V_GetBlock (0, 0, width, height, wipe_scr_end);
    I_ReadScreen(wipe_scr_end);

    V_DrawBlock(x, y, width, height, wipe_scr_start); // restore start scr.

    return 0;
}

int wipe_ScreenWipe(int wipeno, int x, int y, int width, int height, int ticks)
{
    if (wipeno == wipe_None)
    {
        // we DEFINITELY need to do this
        ST_doRefresh();

        return true;
    }

    int           rc;

    static int    (*wipes[])(int, int, int) =
    {
                    0,           0,             0,
        wipe_initFade, wipe_doFade, wipe_exitFade,
        wipe_initBurn, wipe_doBurn, wipe_exitBurn,
        wipe_initMelt, wipe_doMelt, wipe_exitMelt
    };

    ticks <<= hires; // ADDED FOR HIRES

    // initial stuff
    if (!go)
    {
        go = 1;

        // wipe_scr = (byte *) Z_Malloc(width*height, PU_STATIC, 0); // DEBUG

        wipe_scr = I_VideoBuffer;

        (*wipes[wipeno * 3])(width, height, ticks);
    }

    // do a piece of wipe-in
    V_MarkRect(0, 0, width, height);

    rc = (*wipes[wipeno * 3 + 1])(width, height, ticks);

    //  V_DrawBlock(x, y, width, height, wipe_scr); // DEBUG

    // final stuff
    if (rc)
    {
        go = 0;
        (*wipes[wipeno * 3 + 2])(width, height, ticks);
    }
    return !go;
}

