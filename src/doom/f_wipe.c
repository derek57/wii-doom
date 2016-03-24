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
#include "doomstat.h"
#include "doomtype.h"
#include "f_wipe.h"
#include "i_video.h"
#include "m_fixed.h"
#include "m_random.h"
#include "r_defs.h"
#include "st_stuff.h"
#include "v_video.h"
#include "z_zone.h"


// [RH] Fire Wipe
#define FIREWIDTH     64
#define FIREHEIGHT    64


//
// SCREEN WIPE PACKAGE
//


// when zero, stop the wipe
static   dboolean     go;

// [RH] Crossfade
static   int          fade;

static   int          density;
static   int          burntime;
static   int          *y;

static   byte         *burnarray;
static   byte         *wipe_scr_start;
static   byte         *wipe_scr_end;
static   byte         *wipe_scr;


extern   byte         *transtables;


static void wipe_shittyColMajorXform(short *array)
{
    int           x;
    int           y;

    short         *dest = (short *)Z_Malloc((SCREENWIDTH / 2) * SCREENHEIGHT * sizeof(*dest), PU_STATIC, NULL);

    for (y = 0; y < SCREENHEIGHT; y++)
    {
        for (x = 0; x < (SCREENWIDTH / 2); x++)
        {
            dest[x * SCREENHEIGHT + y] = array[y * (SCREENWIDTH / 2) + x];
        }
    }

    memcpy(array, dest, (SCREENWIDTH / 2) * SCREENHEIGHT * sizeof(*dest));

    Z_Free(dest);
}

// Burn -------------------------------------------------------------
static int wipe_initBurn(int ticks)
{
    burnarray = Z_Malloc(FIREWIDTH * (FIREHEIGHT + 5), PU_STATIC, NULL);
    memset(burnarray, 0, FIREWIDTH * (FIREHEIGHT + 5));
    density = 4;
    burntime = 0;

    return 0;
}

static int wipe_CalcBurn(byte *burnarray, int width, int height, int density)
{
    // This is a modified version of the fire that was once used
    // on the player setup menu.
    static int voop;

    int        a, b = voop;
    byte       *from = &burnarray[width * height];

    // generator
    voop += density / 3;

    for (a = 0; a < density / 8; a++)
    {
        unsigned int offs = (a + b) & (width - 1);
        unsigned int v = M_RandomSMMU();
        v = MIN(from[offs] + 4 + (v & 15) + (v >> 3) + (M_RandomSMMU() & 31), 255u);
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
            c1--;

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
                c1--;

            // store pixels
            *pixel = c1;

            // interpolate
            *(pixel + width) = (c1 + bottom) >> 1;

            // next pixel
            pixel++;
        }

        // special case: last pixel on line
        p = pixel + (width << 1);
        top = *p + *(p - 1) + *(p - width + 1);
        bottom = *(pixel + (width << 2));
        c1 = (top + bottom) >> 2;

        if (c1 > 1)
            c1--;

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

static int wipe_doBurn(int ticks)
{
    // Draw the screen
    fixed_t  firex, firey;
    fixed_t  xstep = (FIREWIDTH * FRACUNIT) / SCREENWIDTH;
    fixed_t  ystep = (FIREHEIGHT * FRACUNIT) / SCREENHEIGHT;
    int      x, y;
    byte     *to = screens[0];
    byte     *fromold = (byte *)wipe_scr_start;
    byte     *fromnew = (byte *)wipe_scr_end;
    int      done = 0;

    burntime += ticks;
    ticks *= 2;

    // Make the fire burn
    while (done == 0 && ticks--)
    {
        density = wipe_CalcBurn(burnarray, FIREWIDTH, FIREHEIGHT, density);
        done = (density < 0);
    }

    for (y = 0, firey = 0; y < SCREENHEIGHT; y++, firey += ystep)
    {
        for (x = 0, firex = 0; x < SCREENWIDTH; x++, firex += xstep)
        {
            int fglevel = burnarray[(firex >> FRACBITS) + (firey >> FRACBITS) * FIREWIDTH] / 2;

            if (fglevel >= 63)
            {
                to[x] = fromnew[x];
            }
            else if (fglevel == 0)
            {
                to[x] = fromold[x];
                done = 0;
            }
            else
            {
                int bglevel = 64 - fglevel;
                uint32_t *fg2rgb = Col2RGB8[fglevel];
                uint32_t *bg2rgb = Col2RGB8[bglevel];
                uint32_t fg = fg2rgb[fromnew[x]];
                uint32_t bg = bg2rgb[fromold[x]];

                fg = (fg + bg) | 0x1f07c1f;
                to[x] = RGB32k.RGB[0][0][fg & (fg >> 15)];
                done = 0;
            }
        }

        fromold += SCREENWIDTH;
        fromnew += SCREENWIDTH;
        to += SCREENWIDTH;
    }

    return done || (burntime > 40);
}

static int wipe_exitBurn(int ticks)
{
    Z_Free(burnarray);
    return 0;
}

static int wipe_initFade(int ticks)
{
    fade = 0;
    return 0;
}

static int wipe_doFade(int ticks)
{
    fade += ticks * 2;

    if (fade > 64)
    {
        V_DrawBlock(0, 0, 0, SCREENWIDTH, SCREENHEIGHT, (byte *)wipe_scr_end);
        return 1;
    }
    else
    {
        int x, y;
        fixed_t bglevel = 64 - fade;
        uint32_t *fg2rgb = Col2RGB8[fade];
        uint32_t *bg2rgb = Col2RGB8[bglevel];
        byte *fromnew = (byte *)wipe_scr_end;
        byte *fromold = (byte *)wipe_scr_start;
        byte *to = screens[0];

        for (y = 0; y < SCREENHEIGHT; y++)
        {
            for (x = 0; x < SCREENWIDTH; x++)
            {
                uint32_t fg = fg2rgb[fromnew[x]];
                uint32_t bg = bg2rgb[fromold[x]];
                fg = (fg + bg) | 0x1f07c1f;
                to[x] = RGB32k.RGB[0][0][fg & (fg >> 15)];
            }

            fromnew += SCREENWIDTH;
            fromold += SCREENWIDTH;
            to += SCREENWIDTH;
        }
    }

    return 0;
}

static int wipe_exitFade(int ticks)
{
   return 0;
}

static int wipe_initMelt(int ticks)
{
    int           i;

    // copy start screen to main screen
    memcpy(wipe_scr, wipe_scr_start, SCREENWIDTH * SCREENHEIGHT * sizeof(*wipe_scr));
    
    // makes this wipe faster (in theory)
    // to have stuff in column-major format
    wipe_shittyColMajorXform((short *)wipe_scr_start);
    wipe_shittyColMajorXform((short *)wipe_scr_end);
    
    // setup initial column positions
    // (y<0 => not ready to scroll yet)
    y = (int *)Z_Malloc(SCREENWIDTH * sizeof(int), PU_STATIC, NULL);
    y[0] = -(M_Random() % 16);

    for (i = 1; i < SCREENWIDTH; i++)
    {
        int r = (M_Random() % 3) - 1;

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

static int wipe_doMelt(int ticks)
{
    int           i;
    int           j;
    int           dy;
    int           idx;
    short         *s;
    short         *d;

    dboolean      done = true;

    while (ticks--)
    {
        for (i = 0; i < SCREENWIDTH / 2; i++)
        {
            if (y[i] < 0)
            {
                y[i]++;

                done = false;
            }
            else if (y[i] < SCREENHEIGHT)
            {
                dy = (y[i] < 16) ? y[i] + 1 : 8;
                s = &((short *)wipe_scr_end)[i * SCREENHEIGHT + y[i]];
                d = &((short *)wipe_scr)[y[i] * (SCREENWIDTH / 2) + i];

                if (y[i] + dy >= SCREENHEIGHT)
                {
                    dy = SCREENHEIGHT - y[i];
                }

                idx = 0;

                for (j = dy; j; j--)
                {
                    d[idx] = *(s++);

                    idx += SCREENWIDTH / 2;
                }

                y[i] += dy;

                s = &((short *)wipe_scr_start)[i * SCREENHEIGHT];
                d = &((short *)wipe_scr)[y[i] * (SCREENWIDTH / 2) + i];

                idx = 0;

                for (j = SCREENHEIGHT - y[i]; j; j--)
                {
                    d[idx] = *(s++);

                    idx += SCREENWIDTH / 2;
                }

                done = false;
            }
        }
    }

    return done;
}

static int wipe_exitMelt(int ticks)
{
    Z_Free(y);
    Z_Free(wipe_scr_start);
    Z_Free(wipe_scr_end);
    return 0;
}

int wipe_StartScreen(void)
{
    wipe_scr_start = Z_Malloc(SCREENWIDTH * SCREENHEIGHT * sizeof(*wipe_scr_start), PU_STATIC, NULL);

    I_ReadScreen(0, wipe_scr_start);

    //V_GetBlock (0, 0, 0, SCREENWIDTH, SCREENHEIGHT, wipe_scr_start);
    return 0;
}

int wipe_EndScreen(void)
{
    wipe_scr_end = Z_Malloc(SCREENWIDTH * SCREENHEIGHT * sizeof(*wipe_scr_end), PU_STATIC, NULL);

    I_ReadScreen(0, wipe_scr_end);

    //V_GetBlock (0, 0, 0, SCREENWIDTH, SCREENHEIGHT, wipe_scr_end);

    // restore start scr.
    V_DrawBlock(0, 0, 0, SCREENWIDTH, SCREENHEIGHT, wipe_scr_start);

    return 0;
}

int wipe_ScreenWipe(int wipeno, int ticks, int scrn)
{
    int           rc;

    static int    (*wipes[])(int) =
    {
                    0,           0,             0,
        wipe_initFade, wipe_doFade, wipe_exitFade,
        wipe_initMelt, wipe_doMelt, wipe_exitMelt,
        wipe_initBurn, wipe_doBurn, wipe_exitBurn
    };

    if (wipeno == wipe_None)
    {
        // we DEFINITELY need to do this
        if (screenSize < 8)
        {
            if (usergame)
            {
                if (wipe_type == 3)
                    ST_doRefresh();
            }
        }

        return true;
    }

    ticks <<= hires;

    // initial stuff
    if (!go)
    {
        go = 1;

        // DEBUG
        //wipe_scr = (byte *)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);

        wipe_scr = screens[scrn];

        (*wipes[wipeno * 3])(ticks);
    }

    // do a piece of wipe-in
    //V_MarkRect(0, 0, 1, SCREENWIDTH, SCREENHEIGHT, 0);

    rc = (*wipes[wipeno * 3 + 1])(ticks);

    // DEBUG
    //V_DrawBlock(x, y, 0, SCREENWIDTH, SCREENHEIGHT, wipe_scr);

    // final stuff
    if (rc)
    {
        go = 0;
        (*wipes[wipeno * 3 + 2])(ticks);
    }

    return !go;
}

