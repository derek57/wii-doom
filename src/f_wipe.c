// Emacs style mode select     -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//        Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "f_wipe.h"
#include "i_video.h"
#include "m_fixed.h"
#include "m_random.h"
#include "v_video.h"
#include "z_zone.h"


// [RH] Fire Wipe
#define FIREWIDTH     64
#define FIREHEIGHT    64


//
//        SCREEN WIPE PACKAGE
//

enum
{
    wipe_None,            // don't bother
    wipe_Melt,            // weird screen melt
    wipe_Burn,            // fade in shape of fire
    wipe_Fade,            // crossfade from old to new
    wipe_CrossXForm,      // crossfade from old to new
    wipe_NUMWIPES
};


int                    wipe_type = 1;

static unsigned int    Col2RGB8[65][256];

// [RH] Crossfade
static int             fade;
static int             density;
static int             burntime;
static int             CurrentWipeType;
static int             *y;

static byte            RGB32k[32][32][32];
static byte            *wipe_scr_start;
static byte            *wipe_scr_end;
static byte            *wipe_scr;
static byte            *burnarray;


// Melt -------------------------------------------------------------

void wipe_shittyColMajorXform (short *array)
{
    int x, y;
    int width = SCREENWIDTH / 2;

    short *dest = (short *)Z_Malloc(width * SCREENHEIGHT * 2, PU_STATIC, 0);

    for(y = 0; y < SCREENHEIGHT; y++)
        for(x = 0; x < width; x++)
            dest[x * SCREENHEIGHT + y] = array[y * width + x];

    memcpy(array, dest, SCREENWIDTH * SCREENHEIGHT * 2);

    Z_Free(dest);
}

int wipe_initMelt (int ticks)
{
    int i, r;
    
    // copy start screen to main screen
    V_DrawBlock (0, 0, 0, SCREENWIDTH, SCREENHEIGHT, (byte *)wipe_scr_start);
    
    // makes this wipe faster (in theory)
    // to have stuff in column-major format
    wipe_shittyColMajorXform ((short*)wipe_scr_start);
    wipe_shittyColMajorXform ((short*)wipe_scr_end);
    
    // setup initial column positions
    // (y<0 => not ready to scroll yet)
    y = (int *) Z_Malloc(SCREENWIDTH * sizeof(int), PU_STATIC, 0);
    y[0] = -(M_Random() & 0xf);

    for (i = 1; i < SCREENWIDTH; i++)
    {
        r = (M_Random() % 3) - 1;
        y[i] = y[i - 1] + r;

        if (y[i] > 0)
            y[i] = 0;
        else if (y[i] == -16)
            y[i] = -15;
    }
    return 0;
}

int wipe_doMelt (int ticks)
{
    int         i;
    int         j;
    int         dy;
    int         idx;
    int         width = SCREENWIDTH / 2;
    
    short*      s;
    short*      d;

    boolean     done = true;

    while (ticks--)
    {
        for (i = 0; i < width; i++)
        {
            if (y[i] < 0)
            {
                y[i]++;
                done = false;
            }
            else if (y[i] < SCREENHEIGHT)
            {
                dy = (y[i] < 16) ? y[i] + 1 : 8;
                dy = (dy * SCREENHEIGHT) / 200;

                if (y[i] + dy >= SCREENHEIGHT)
                    dy = SCREENHEIGHT - y[i];

                s = &((short *)wipe_scr_end)[i * SCREENHEIGHT + y[i]];
                d = &((short *)screens[0])[y[i] * (SCREENWIDTH / 2) + i];
                idx = 0;

                for (j = dy; j; j--)
                {
                    d[idx] = *(s++);
                    idx += SCREENWIDTH / 2;
                }

                y[i] += dy;
                s = &((short *)wipe_scr_start)[i * SCREENHEIGHT];
                d = &((short *)screens[0])[y[i] * (SCREENWIDTH / 2) + i];
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

int wipe_exitMelt (int ticks)
{
    Z_Free(wipe_scr_start);
    Z_Free(wipe_scr_end);
    Z_Free(y);

    return 0;
}

// Burn -------------------------------------------------------------

int wipe_initBurn (int ticks)
{
    burnarray = Z_Malloc(FIREWIDTH * (FIREHEIGHT + 4), PU_STATIC, 0);
    memset(burnarray, 0, FIREWIDTH * (FIREHEIGHT + 4));
    density = 4;
    burntime = 0;

    return 0;
}

int wipe_doBurn (int ticks)
{
    static int voop;

    boolean done;

    // This is a modified version of the fire from the player
    // setup menu.
    burntime += ticks;
    ticks *= 2;

    // Make the fire burn
    while (ticks--)
    {
        int a;
        int b = voop;

        // generator
        byte *from = burnarray + FIREHEIGHT * FIREWIDTH;

        voop += density / 3;

        for (a = 0; a < density / 8; a++)
        {
            unsigned int offs = (a + b) % FIREWIDTH;
            unsigned int v = M_Random();

            v = from[offs] + 4 + (v & 15) + (v >> 3) + (M_Random() & 31);

            if (v > 255)
                v = 255;

            from[offs] = from[FIREWIDTH * 2 + (offs + FIREWIDTH * 3 / 2) % FIREWIDTH] = v;
        }
        density += 10;

        if (density > FIREWIDTH * 7)
            density = FIREWIDTH * 7;

        from = burnarray;

        for (b = 0; b <= FIREHEIGHT; b += 2)
        {
            byte *pixel = from;

            // special case: first pixel on line
            byte *p = pixel + (FIREWIDTH << 1);

            unsigned int top = *p + *(p + FIREWIDTH - 1) + *(p + 1);
            unsigned int bottom = *(pixel + (FIREWIDTH << 2));
            unsigned int c1 = (top + bottom) >> 2;

            if (c1 > 1)
                c1--;

            *pixel = c1;
            *(pixel + FIREWIDTH) = (c1 + bottom) >> 1;
            pixel++;

            // main line loop
            for (a = 1; a < FIREWIDTH - 1; a++)
            {
                // sum top pixels
                p = pixel + (FIREWIDTH << 1);
                top = *p + *(p - 1) + *(p + 1);

                // bottom pixel
                bottom = *(pixel + (FIREWIDTH << 2));

                // combine pixels
                c1 = (top + bottom) >> 2;

                if (c1 > 1)
                    c1--;

                // store pixels
                *pixel = c1;
                *(pixel + FIREWIDTH) = (c1 + bottom) >> 1;        // interpolate

                // next pixel
                pixel++;
            }

            // special case: last pixel on line
            p = pixel + (FIREWIDTH << 1);
            top = *p + *(p - 1) + *(p - FIREWIDTH + 1);
            bottom = *(pixel + (FIREWIDTH << 2));
            c1 = (top + bottom) >> 2;

            if (c1 > 1)
                c1--;

            *pixel = c1;
            *(pixel + FIREWIDTH) = (c1 + bottom) >> 1;

            // next line
            from += FIREWIDTH << 1;
        }
    }

    // Draw the screen
    fixed_t firex, firey;
    fixed_t xstep = (FIREWIDTH * FRACUNIT) / SCREENWIDTH;
    fixed_t ystep = (FIREHEIGHT * FRACUNIT) / SCREENHEIGHT;

    int x, y;

    byte *to = screens[0];
    byte *fromold = (byte *)wipe_scr_start;
    byte *fromnew = (byte *)wipe_scr_end;

    done = true;

    for (y = 0, firey = 0; y < SCREENHEIGHT; y++, firey += ystep)
    {
        for (x = 0, firex = 0; x < SCREENWIDTH; x++, firex += xstep)
        {
            int fglevel = burnarray[(firex >> FRACBITS) + (firey >> FRACBITS) * FIREWIDTH] / 2;

            if (fglevel >= 63)
                to[x] = fromnew[x];
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
                to[x] = RGB32k[0][0][fg & (fg >> 15)];
                done = false;
            }
        }
        fromold += SCREENWIDTH;
        fromnew += SCREENWIDTH;
        to += SCREENWIDTH;
    }
    return done || (burntime > 40);
}

int wipe_exitBurn (int ticks)
{
    Z_Free(wipe_scr_start);
    Z_Free(wipe_scr_end);
    Z_Free(burnarray);

    return 0;
}

// Crossfade --------------------------------------------------------

int wipe_initFade (int ticks)
{
    fade = 0;

    return 0;
}

int wipe_doFade (int ticks)
{
    fade += ticks;

    if (fade > 64)
    {
        V_DrawBlock (0, 0, 0, SCREENWIDTH, SCREENHEIGHT, (byte *)wipe_scr_end);

        return 1;
    }
    else
    {
        int x, y;

        fixed_t bglevel = 64 - fade;

        unsigned int *fg2rgb = Col2RGB8[fade];
        unsigned int *bg2rgb = Col2RGB8[bglevel];

        byte *fromnew = (byte *)wipe_scr_end;
        byte *fromold = (byte *)wipe_scr_start;

        byte *to = screens[0];

        for (y = 0; y < SCREENHEIGHT; y++)
        {
            for (x = 0; x < SCREENWIDTH; x++)
            {
                unsigned int fg = fg2rgb[fromnew[x]];
                unsigned int bg = bg2rgb[fromold[x]];

                fg = (fg + bg) | 0x1f07c1f;
                to[x] = RGB32k[0][0][fg & (fg >> 15)];
            }
            fromnew += SCREENWIDTH;
            fromold += SCREENWIDTH;
            to += SCREENWIDTH;
        }
    }
    fade++;

    return 0;
}

int wipe_exitFade (int ticks)
{
    Z_Free(wipe_scr_start);
    Z_Free(wipe_scr_end);

    return 0;
}

// ColorXForm--------------------------------------------------------

// haleyjd 08/26/10: [STRIFE] Verified unmodified.
int wipe_initColorXForm(int ticks)
{
    memcpy(wipe_scr, wipe_scr_start, SCREENWIDTH * SCREENHEIGHT);

    return 0;
}

//
// wipe_doColorXForm
//
// haleyjd 08/26/10: [STRIFE]
// * Rogue modified the unused ColorXForm wipe in-place in order to implement 
//   their distinctive crossfade wipe.
//
int wipe_doColorXForm(int ticks)
{
    byte *cur_screen = wipe_scr;
    byte *end_screen = wipe_scr_end;

    int   pix = SCREENWIDTH * SCREENHEIGHT;
    int   i;

    boolean changed = false;

    for(i = pix; i > 0; i--)
    {
        if(*cur_screen != *end_screen)
        {
            changed = true;
            *cur_screen = xlatab[(*cur_screen << 8) + *end_screen];
        }
        ++cur_screen;
        ++end_screen;
    }
    return !changed;
}

// haleyjd 08/26/10: [STRIFE] Verified unmodified.
int wipe_exitColorXForm(int ticks)
{
    Z_Free(wipe_scr_start);
    Z_Free(wipe_scr_end);

    return 0;
}

// General Wipe Functions -------------------------------------------

int wipe_StartScreen (void)
{
    CurrentWipeType = wipe_type;

    if (CurrentWipeType < 0)
        CurrentWipeType = 0;
    else if (CurrentWipeType >= wipe_NUMWIPES)
        CurrentWipeType = wipe_NUMWIPES - 1;

    if (CurrentWipeType)
    {
        wipe_scr_start = Z_Malloc(SCREENWIDTH * SCREENHEIGHT * 4, PU_STATIC, NULL);

        I_ReadScreen(wipe_scr_start);
    }
    return 0;
}

int wipe_EndScreen (void)
{
    if (CurrentWipeType)
    {
        wipe_scr_end = Z_Malloc(SCREENWIDTH * SCREENHEIGHT * 4, PU_STATIC, NULL);

        I_ReadScreen(wipe_scr_end);

        V_DrawBlock (0, 0, 0, SCREENWIDTH, SCREENHEIGHT, wipe_scr_start); // restore start scr.
    }
    return 0;
}

int wipe_ScreenWipe (int ticks)
{
    static boolean    go = 0;        // when zero, stop the wipe

    static int (*wipes[])(int) =
    {
        wipe_initMelt, wipe_doMelt, wipe_exitMelt,
        wipe_initBurn, wipe_doBurn, wipe_exitBurn,
        wipe_initFade, wipe_doFade, wipe_exitFade,
	wipe_initColorXForm, wipe_doColorXForm, wipe_exitColorXForm
    };

    int rc;

    if (CurrentWipeType == wipe_None)
        return true;

    // initial stuff
    if (!go)
    {
        go = 1;

        // haleyjd 20110629 [STRIFE]: We *must* use a temp buffer here.
        if (wipe_type == 4)
            wipe_scr = (byte *) Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, 0); // DEBUG

        (*wipes[(CurrentWipeType - 1) * 3])(ticks);
    }

    // do a piece of wipe-in
    V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);

    rc = (*wipes[(CurrentWipeType - 1) * 3 + 1])(ticks);

    // haleyjd 20110629 [STRIFE]: Copy temp buffer to the real screen.
    if (wipe_type == 4)
        V_DrawBlock(0, 0, 0, SCREENWIDTH, SCREENHEIGHT, wipe_scr);

    // final stuff
    if (rc)
    {
        go = 0;
        (*wipes[(CurrentWipeType - 1) * 3 + 2])(ticks);
    }
    return !go;
}

