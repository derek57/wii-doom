/*
========================================================================

                               DOOM RETRO
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright (C) 2013-2015 Brad Harding.

  DOOM RETRO is a fork of CHOCOLATE DOOM by Simon Howard.
  For a complete list of credits, see the accompanying AUTHORS file.

  This file is part of DOOM RETRO.

  DOOM RETRO is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM RETRO is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM RETRO. If not, see <http://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM RETRO is in no way affiliated with nor endorsed by
  id Software LLC.

========================================================================
*/


#ifdef WII
#include <ctype.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <mpeg/smpeg.h>
#include <jpeglib.h>
#include <ogc/libversion.h>
#include <png.h>
#endif

#include <time.h>
#include "c_io.h"
#include "d_deh.h"
#include "d_event.h"
#include "doomfeatures.h"
#include "doomkeys.h"

#include "doom/doomstat.h"
#include "doom/g_game.h"

#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_tinttab.h"
#include "i_video.h"

#include "doom/m_menu.h"

#include "m_misc.h"

#include "doom/p_local.h"

#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#endif

#ifdef WII
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <zlib.h>
#endif

#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#ifdef WII
#include <wiiuse/wpad.h>
#endif

#if !defined(MAX_PATH)
#define MAX_PATH        260
#endif

#define CONSOLESPEED            (CONSOLEHEIGHT / 12)
#define CONSOLEFONTSTART        ' '
#define CONSOLEFONTEND          '~'
#define CONSOLEFONTSIZE         (CONSOLEFONTEND - CONSOLEFONTSTART + 1)

#define CONSOLETEXTX            10
#define CONSOLETEXTY            8
#define CONSOLELINES            11
#define CONSOLELINEHEIGHT       14

#define CONSOLEINPUTPIXELWIDTH  500

#define CONSOLESCROLLBARWIDTH   3
#define CONSOLESCROLLBARHEIGHT  ((CONSOLELINES - 1) * CONSOLELINEHEIGHT - 4)
#define CONSOLESCROLLBARX       (SCREENWIDTH - CONSOLETEXTX - CONSOLESCROLLBARWIDTH)
#define CONSOLESCROLLBARY       (CONSOLETEXTY + 1)

#define CONSOLEDIVIDERWIDTH     (SCREENWIDTH - CONSOLETEXTX * 3 - CONSOLESCROLLBARWIDTH)

#define DIVIDER                 "~~~"
#define ITALICS                 '~'

#define CARETWAIT               10

#define NOBACKGROUNDCOLOR       -1


dboolean        consoleactive = false;
dboolean        alwaysrun;
dboolean        forceblurredraw = false;

patch_t         *unknownchar;
patch_t         *consolefont[CONSOLEFONTSIZE];
patch_t         *lsquote;
patch_t         *ldquote;
patch_t         *degree;
patch_t         *multiply;
patch_t         *caret;
patch_t         *route;
patch_t         *space;

byte            c_tempscreen[SCREENWIDTH * SCREENHEIGHT];
byte            c_blurscreen[SCREENWIDTH * SCREENHEIGHT];
byte            inputcolor = 4;
byte            whitecolor = 80;
byte            bluecolor = 200;
byte            redcolor = 40;
byte            graycolor = 100;
byte            greencolor = 120;
byte            yellowcolor = 160;
byte            dividercolor = 0;   // actually it's colored red
byte            consolebrandingcolor = 100;
byte            consolescrollbartrackcolor = 100;
byte            consolescrollbarfacecolor = 88;
byte            consoletintcolor = 5;
byte            consolecolors[STRINGTYPES];

char            consoleinput[255] = "";
char            consolecheat[255] = "";
char            consolecheatparm[3] = "";

char            *upper =
{
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0 !\"#$%&\"()*+,_>?)!@#$%^&*(:"
    ":<+>?\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0{\\}^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ"
};

int             consoleanimindex = 0;
int             consoleheight = 0;
int             consoledirection = -1;
int             consolestrings = 0;
int             consoleedgecolor1 = 105;
int             consoleedgecolor2 = 100;
int             caretpos = 0;
int             selectstart = 0;
int             selectend = 0;
int             timestampx;
int             zerowidth;
int             spacewidth;

static int      caretwait;
static int      outputhistory = -1;
static int      consolewait;
static int      notabs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

static int      consoleanimdown[] =
{
     14,  28,  42,  56,  70,  84,  98, 112, 126, 140, 146,
    150, 153, 156, 159, 161, 163, 165, 166, 167, 168
};

static int      consoleanimup[] =
{
    154, 140, 126, 112,  98,  84,  70,  56,  42,  28,  22,
     18,  15,  12,   9,   7,   5,   3,   2,   1,   0
};

static dboolean  showcaret = true;

extern dboolean  translucency;
extern dboolean  wipe;

extern int      fps;


void C_Print(stringtype_t typestring, char *string, ...)
{
    va_list     argptr;
    char        buffer[1024] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);
#ifdef BOOM_ZONE_HANDLING
    console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console), PU_STATIC, NULL);
#else
    console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
#endif
    console[consolestrings].string = strdup(buffer);
    console[consolestrings].type = typestring;
    memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
    console[consolestrings].timestamp = "";
    ++consolestrings;
    outputhistory = -1;
}

void C_Output(char *string, ...)
{
    va_list     argptr;
    char        buffer[1024] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);

#ifdef BOOM_ZONE_HANDLING
    console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console), PU_STATIC, NULL);
#else
    console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
#endif
    console[consolestrings].string = strdup(buffer);
    console[consolestrings].type = graystring;
    memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
    console[consolestrings].timestamp = "";
    ++consolestrings;
    outputhistory = -1;
}

void C_Warning(char *string, ...)
{
    va_list     argptr;
    char        buffer[1024] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);

    if (consolestrings && strcasecmp(console[consolestrings - 1].string, buffer))
    {
#ifdef BOOM_ZONE_HANDLING
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console), PU_STATIC, NULL);
#else
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
#endif
        console[consolestrings].string = strdup(buffer);
        console[consolestrings].type = yellowstring;
        memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
        console[consolestrings].timestamp = "";
        ++consolestrings;
        outputhistory = -1;
    }
}

void C_PlayerMessage(char *string, ...)
{
    va_list     argptr;
    char        buffer[1024] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);

    if (consolestrings && !strcasecmp(console[consolestrings - 1].string, buffer))
    {
        M_snprintf(buffer, sizeof(buffer), "%s (2)", console[consolestrings - 1].string);
        console[consolestrings - 1].string = strdup(buffer);
    }
    else if (consolestrings && M_StringStartsWith(console[consolestrings - 1].string, buffer))
    {
        char    *count = strrchr(console[consolestrings - 1].string, '(') + 1;

        count[strlen(count) - 1] = '\0';

        M_snprintf(buffer, sizeof(buffer), "%s (%i)", buffer, atoi(count) + 1);
        console[consolestrings - 1].string = strdup(buffer);
    }
    else
    {
        time_t          rawtime;
        struct tm       *timeinfo;
#ifdef BOOM_ZONE_HANDLING
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console), PU_CACHE, NULL);
#else
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
#endif
        console[consolestrings].string = strdup(buffer);
        console[consolestrings].type = greenstring;
        memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
        console[consolestrings].timestamp = strdup(buffer);

        ++consolestrings;
    }
    outputhistory = -1;
}

void C_AddConsoleDivider(void)
{
    if (!consolestrings || strcasecmp(console[consolestrings - 1].string, DIVIDER))
        C_Print(dividerstring, DIVIDER);
}

static void C_DrawDivider(int y)
{
    int i;

    y *= SCREENWIDTH;
    if (y >= CONSOLETOP * SCREENWIDTH)
        for (i = y + CONSOLETEXTX; i < y + CONSOLETEXTX + CONSOLEDIVIDERWIDTH; ++i)
            I_VideoBuffer[i] = redcolor;
    if ((y += SCREENWIDTH) >= CONSOLETOP * SCREENWIDTH)
        for (i = y + CONSOLETEXTX; i < y + CONSOLETEXTX + CONSOLEDIVIDERWIDTH; ++i)
            I_VideoBuffer[i] = redcolor;
}

static struct
{
    char        char1;
    char        char2;
    int         adjust;
} kern[] = {
    { ' ',  '(',  -1 }, { '\\', 'V',  -1 }, { '\"', '+',  -1 }, { '\"', '.',  -1 },
    { '\"', 'a',  -1 }, { '\"', 'c',  -1 }, { '\"', 'd',  -1 }, { '\"', 'e',  -1 },
    { '\"', 'g',  -1 }, { '\"', 'j',  -2 }, { '\"', 'o',  -1 }, { '\"', 'q',  -1 },
    { '\"', 's',  -1 }, { '\'', 'a',  -1 }, { '\'', 'c',  -1 }, { '\'', 'd',  -1 },
    { '\'', 'e',  -1 }, { '\'', 'g',  -1 }, { '\'', 'j',  -2 }, { '\'', 'o',  -1 },
    { '\"', 'q',  -1 }, { '\'', 's',  -1 }, { '.',  '\\', -1 }, { '.',  '7',  -1 },
    { '/',  'o',  -1 }, { ':', '\\',  -1 }, { '_',  'f',  -1 }, { '0',  ',',  -1 },
    { '0',  'j',  -2 }, { '1',  '\"', -1 }, { '1',  '\'', -1 }, { '1',  'j',  -2 },
    { '2',  'j',  -2 }, { '3',  ',',  -1 }, { '3',  'j',  -2 }, { '4',  'j',  -2 },
    { '5',  ',',  -1 }, { '5',  'j',  -2 }, { '6',  ',',  -1 }, { '6',  'j',  -2 },
    { '7',  ',',  -2 }, { '7',  'j',  -2 }, { '8',  ',',  -1 }, { '8',  'j',  -2 },
    { '9',  ',',  -1 }, { '9',  'j',  -2 }, { 'F',  '.',  -1 }, { 'F',  ',',  -1 },
    { 'L',  '\\', -1 }, { 'L',  '\"', -1 }, { 'L',  '\'', -1 }, { 'P',  '.',  -1 },
    { 'P',  ',',  -1 }, { 'T',  '.',  -1 }, { 'T',  ',',  -1 }, { 'V',  '.',  -1 },
    { 'V',  ',',  -1 }, { 'Y',  '.',  -1 }, { 'Y',  ',',  -1 }, { 'a',  '\"', -1 },
    { 'a',  '\'', -1 }, { 'a',  'j',  -2 }, { 'b',  ',',  -1 }, { 'b',  '\"', -1 },
    { 'b',  '\\', -1 }, { 'b',  '\'', -1 }, { 'b',  'j',  -2 }, { 'c',  '\\', -1 },
    { 'c',  ',',  -1 }, { 'c',  '\"', -1 }, { 'c',  '\'', -1 }, { 'c',  'j',  -2 },
    { 'd',  'j',  -2 }, { 'e',  '\\', -1 }, { 'e',  ',',  -1 }, { 'e',  '\"', -1 },
    { 'e',  '\'', -1 }, { 'e',  '_',  -1 }, { 'e',  'j',  -2 }, { 'f',  ' ',  -1 },
    { 'f',  ',',  -2 }, { 'f',  '_',  -1 }, { 'f',  'a',  -1 }, { 'f',  'j',  -2 },
    { 'h',  '\\', -1 }, { 'h',  '\"', -1 }, { 'h',  '\'', -1 }, { 'h',  'j',  -2 },
    { 'i',  'j',  -2 }, { 'k',  'j',  -2 }, { 'l',  'j',  -2 }, { 'm',  '\"', -1 },
    { 'm',  '\\', -1 }, { 'm',  '\'', -1 }, { 'm',  'j',  -2 }, { 'n',  '\\', -1 },
    { 'n',  '\"', -1 }, { 'n',  '\'', -1 }, { 'n',  'j',  -2 }, { 'o',  '\\', -1 },
    { 'o',  ',',  -1 }, { 'o',  '\"', -1 }, { 'o',  '\'', -1 }, { 'o',  'j',  -2 },
    { 'p',  '\\', -1 }, { 'p',  ',',  -1 }, { 'p',  '\"', -1 }, { 'p',  '\'', -1 },
    { 'p',  'j',  -2 }, { 'r',  ' ',  -1 }, { 'r',  '\\', -1 }, { 'r',  '.',  -2 },
    { 'r',  ',',  -2 }, { 'r',  '\"', -1 }, { 'r',  '\'', -1 }, { 'r',  '_',  -1 },
    { 'r',  'a',  -1 }, { 'r',  'j',  -2 }, { 's',  ',',  -1 }, { 's',  'j',  -2 },
    { 't',  'j',  -2 }, { 'u',  'j',  -2 }, { 'v',  ',',  -1 }, { 'v',  'j',  -2 },
    { 'w',  'j',  -2 }, { 'x',  'j',  -2 }, { 'z',  'j',  -2 }, {  0 ,   0 ,   0 }
};

static int C_TextWidth(char *text)
{
    size_t      i;
    size_t      len = strlen(text);
    char        prevletter = '\0';
    int         w = 0;

    for (i = 0; i < len; ++i)
    {
        char    letter = text[i];
        int     c = letter - CONSOLEFONTSTART;
        char    nextletter = text[i + 1];
        int     j = 0;

        if (letter == '\xc2' && nextletter == '\xb0')
        {
            w += SHORT(degree->width);
            ++i;
        }
        else
            w += SHORT(c < 0 || c >= CONSOLEFONTSIZE ? unknownchar->width : consolefont[c]->width);

        while (kern[j].char1)
        {
            if (prevletter == kern[j].char1 && letter == kern[j].char2)
            {
                w += kern[j].adjust;
                break;
            }
            ++j;
        }
        prevletter = letter;
    }
    return w;
}

static void C_DrawScrollbar(void)
{
    int x, y;
    int trackstart;
    int trackend;
    int facestart;
    int faceend;
    int offset = (CONSOLEHEIGHT - consoleheight) * SCREENWIDTH;

    // Draw scrollbar track
    trackstart = CONSOLESCROLLBARY * SCREENWIDTH;
    trackend = trackstart + CONSOLESCROLLBARHEIGHT * SCREENWIDTH;
    for (y = trackstart; y < trackend; y += SCREENWIDTH)
        if (y - offset >= 0)
            for (x = CONSOLESCROLLBARX; x < CONSOLESCROLLBARX + CONSOLESCROLLBARWIDTH; ++x)
                I_VideoBuffer[y - offset + x] = consolescrollbartrackcolor;

    // Draw scrollbar face
    facestart = (CONSOLESCROLLBARY + CONSOLESCROLLBARHEIGHT * (outputhistory == -1 ?
        MAX(0, consolestrings - CONSOLELINES) : outputhistory) / consolestrings) * SCREENWIDTH;
    faceend = facestart + (CONSOLESCROLLBARHEIGHT - CONSOLESCROLLBARHEIGHT
        * MAX(0, consolestrings - CONSOLELINES) / consolestrings) * SCREENWIDTH;

    for (y = facestart; y < faceend; y += SCREENWIDTH)
        if (y - offset >= 0)
            for (x = CONSOLESCROLLBARX; x < CONSOLESCROLLBARX + CONSOLESCROLLBARWIDTH; ++x)
                I_VideoBuffer[y - offset + x] = consolescrollbarfacecolor;
}

void C_Init(void)
{
    int         i;
    int         j = CONSOLEFONTSTART;
    char        buffer[9];

    unknownchar = W_CacheLumpName("DRFON000", PU_STATIC);
    for (i = 0; i < CONSOLEFONTSIZE; i++)
    {
        M_snprintf(buffer, 9, "DRFON%03d", j++);
        consolefont[i] = W_CacheLumpName(buffer, PU_STATIC);
    }

    caret = W_CacheLumpName("CARET", PU_STATIC);
    space = W_CacheLumpName("DRFON032", PU_STATIC);
    route = W_CacheLumpName("DRFON036", PU_STATIC);

    lsquote = W_CacheLumpName("DRFON145", PU_STATIC);
    ldquote = W_CacheLumpName("DRFON147", PU_STATIC);
    degree = W_CacheLumpName("DRFON176", PU_STATIC);
    multiply = W_CacheLumpName("DRFON215", PU_STATIC);

    spacewidth = SHORT(caret->width);
    timestampx = SCREENWIDTH - C_TextWidth("00:00:00") - CONSOLETEXTX * 2
        - CONSOLESCROLLBARWIDTH + 1;

    zerowidth = SHORT(consolefont['0' - CONSOLEFONTSTART]->width);

    consolecolors[yellowstring] = yellowcolor;   // yellow = 160
    consolecolors[redstring] = redcolor;         // red = 40
    consolecolors[graystring] = graycolor;       // gray = 100
    consolecolors[bluestring] = bluecolor;       // blue = 200
    consolecolors[whitestring] = whitecolor;     // white = 80
    consolecolors[greenstring] = greencolor;     // green = 120
    consolecolors[dividerstring] = dividercolor; // divider = 0 (linked to red color)

    consoletintcolor <<= 8;
    consoleedgecolor1 <<= 8;
    consoleedgecolor2 <<= 8;
    consolescrollbartrackcolor <<= 8;
    dividercolor <<= 8;
}

void C_HideConsole(void)
{
    consoledirection = -1;
    consoleanimindex = 0;
}

void C_HideConsoleFast(void)
{
    consoleheight = 0;
    consoledirection = -1;
    consoleanimindex = 0;
    consoleactive = false;
}

static void DoBlurScreen(int x1, int y1, int x2, int y2, int i)
{
    int x, y;

    memcpy(c_tempscreen, c_blurscreen, SCREENWIDTH * (CONSOLEHEIGHT + 5));

    for (y = y1; y < y2; y += SCREENWIDTH)
        for (x = y + x1; x < y + x2; ++x)
            c_blurscreen[x] = tinttab50[c_tempscreen[x] + (c_tempscreen[x + i] << 8)];
}

static void C_DrawBackground(int height)
{
    static dboolean     blurred;
    int                 i, j;

    height = (height + 5) * SCREENWIDTH;

    if (!blurred || forceblurredraw)
    {
        forceblurredraw = false;

        for (i = 0; i < height; ++i)
            c_blurscreen[i] = I_VideoBuffer[i];

        DoBlurScreen(0, 0, SCREENWIDTH - 1, height, 1);
        DoBlurScreen(1, 0, SCREENWIDTH, height, -1);
        DoBlurScreen(0, 0, SCREENWIDTH - 1, height - SCREENWIDTH, SCREENWIDTH + 1);
        DoBlurScreen(1, SCREENWIDTH, SCREENWIDTH, height, -(SCREENWIDTH + 1));
        DoBlurScreen(0, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH);
        DoBlurScreen(0, SCREENWIDTH, SCREENWIDTH, height, -SCREENWIDTH);
        DoBlurScreen(1, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH - 1);
        DoBlurScreen(0, SCREENWIDTH, SCREENWIDTH - 1, height, -(SCREENWIDTH - 1));
    }

    forceblurredraw = true;
    blurred = (consoleheight == CONSOLEHEIGHT && !wipe);

    for (i = 0; i < height; ++i)
        I_VideoBuffer[i] = tinttab50[c_blurscreen[i] + consoletintcolor];

    for (i = height - 2; i > 1; i -= 3)
    {
        I_VideoBuffer[i] = colormaps[0][256 * 6 + I_VideoBuffer[i]];
        if (((i - 1) % SCREENWIDTH) < SCREENWIDTH - 2)
            I_VideoBuffer[i + 1] = colormaps[0][256 * 6 + I_VideoBuffer[i - 1]];
    }

    for (i = height - SCREENWIDTH * 3; i < height - SCREENWIDTH * 2; ++i)
        I_VideoBuffer[i] = tinttab25[consoleedgecolor1 + I_VideoBuffer[i]];

    for (i = height - SCREENWIDTH * 2; i < height; ++i)
        I_VideoBuffer[i] = tinttab25[consoleedgecolor2 + I_VideoBuffer[i]];

    for (j = 1; j <= 4; ++j)
        for (i = height; i < height + SCREENWIDTH * j; ++i)
            I_VideoBuffer[i] = colormaps[0][256 * 4 + I_VideoBuffer[i]];
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

static void C_DrawConsoleText(int x, int y, char *text, int color1, int color2, byte *tinttab,
    int tabs[8])
{
    dboolean            italics = false;
    size_t              i;
    int                 tab = -1;
    size_t              len = strlen(text);
    unsigned char       prevletter = '\0';

    y -= (CONSOLEHEIGHT - consoleheight);

    if (len > 80)
        while (C_TextWidth(text) > SCREENWIDTH - CONSOLETEXTX * 3 - CONSOLESCROLLBARWIDTH + 2)
        {
            text[len - 1] = '.';
            text[len] = '.';
            text[len + 1] = '.';
            text[len + 2] = '\0';
            --len;
        }

    for (i = 0; i < len; ++i)
    {
        unsigned char   letter = text[i];
        int             c = letter - CONSOLEFONTSTART;
        unsigned char   nextletter = text[i + 1];

        if (letter == ITALICS && prevletter != ITALICS)
        {
            italics = !italics;
            if (!italics)
                ++x;
        }
        else
        {
            patch_t     *patch = NULL;

            if (letter == ITALICS)
                italics = false;
            if (letter == '\t')
                x = (x > tabs[++tab] ? x + spacewidth : tabs[tab]);
            else if (letter == '\xc2' && nextletter == '\xb0')
            {
                patch = degree;
                ++i;
            }
            else
                patch = (c < 0 || c >= CONSOLEFONTSIZE ? unknownchar : consolefont[c]);

            if (isdigit(prevletter) && letter == 'x' && isdigit(nextletter))
                patch = multiply;
            else if (prevletter == ' ' || prevletter == '\t' || !i)
            {
                if (letter == '\'')
                    patch = lsquote;
                else if (letter == '\"')
                    patch = ldquote;
            }

            if (!italics)
            {
                int     k = 0;

                while (kern[k].char1)
                {
                    if (prevletter == kern[k].char1 && letter == kern[k].char2)
                    {
                        x += kern[k].adjust;
                        break;
                    }
                    ++k;
                }
            }

            if (patch)
            {
                V_DrawConsoleChar(x, y, patch, color1, color2, italics, tinttab);
                x += SHORT(patch->width);
            }
        }
        prevletter = letter;
    }
}

static void C_DrawTimeStamp(int x, int y, char *text)
{
    size_t      i;
    size_t      len = strlen(text);

    y -= (CONSOLEHEIGHT - consoleheight);

    for (i = 0; i < len; ++i)
    {
        patch_t *patch = consolefont[text[i] - CONSOLEFONTSTART];

        V_DrawConsoleChar(x + (text[i] == '1' ? (zerowidth - SHORT(patch->width)) / 2 : 0), y,
            patch, consolebrandingcolor, NOBACKGROUNDCOLOR, false, tinttab25);
        x += (isdigit(text[i]) ? zerowidth : SHORT(patch->width));
    }
}

void C_Drawer(void)
{
    if (consoleheight)
    {
        int     i;
        int     x = CONSOLETEXTX;
        int     start;
        int     end;

        // adjust console height

        if (consolewait < I_GetTime())
        {
            if (consoledirection == 1)
            {
                if (consoleheight < CONSOLEHEIGHT)
                    consoleheight = consoleanimdown[consoleanimindex++];
            }
            else if (consoleheight > 0)
                consoleheight = consoleanimup[consoleanimindex++];
            consolewait = I_GetTime();
        }

        consoleactive = (consoledirection == 1);

        // draw tiled background and bottom edge
        C_DrawBackground(consoleheight);

        // draw branding
        C_DrawConsoleText(SCREENWIDTH - C_TextWidth("Wii-DOOM") - CONSOLETEXTX + 1,
            CONSOLEHEIGHT - 10, "Wii-DOOM", graycolor, NOBACKGROUNDCOLOR, tinttab25, notabs);

        // draw console text
        if (outputhistory == -1)
        {
            start = MAX(0, consolestrings - CONSOLELINES);
            end = consolestrings;
        }
        else
        {
            start = outputhistory;
            end = outputhistory + CONSOLELINES;
        }
        for (i = start; i < end; ++i)
        {
            int y = CONSOLELINEHEIGHT * (i - start + MAX(0, CONSOLELINES - consolestrings))
                    - CONSOLELINEHEIGHT / 2 + 1;

            if (console[i].type == dividerstring)
                C_DrawDivider(y + 5 - (CONSOLEHEIGHT - consoleheight));
            else
            {
                C_DrawConsoleText(CONSOLETEXTX, y + (CONSOLELINEHEIGHT / 2), console[i].string,
                    consolecolors[console[i].type], NOBACKGROUNDCOLOR, NULL, console[i].tabs);
                if (console[i].timestamp[0])
                    C_DrawTimeStamp(timestampx, y + (CONSOLELINEHEIGHT / 2), console[i].timestamp);
            }
        }

        // draw caret
        if (caretwait < I_GetTime())
        {
            showcaret = !showcaret;
            caretwait = I_GetTime() + CARETWAIT;
        }

        V_DrawConsoleChar(x, consoleheight - 10, space, inputcolor, NOBACKGROUNDCOLOR,
                false, NULL);
        V_DrawConsoleChar(x + SHORT(space->width), consoleheight - 10, route,
                inputcolor, NOBACKGROUNDCOLOR, false, NULL);

        if (showcaret)
            V_DrawConsoleChar(x + SHORT(route->width) + 4, consoleheight - 10,
                    caret, inputcolor, NOBACKGROUNDCOLOR, false, NULL);
//        x += SHORT(caret->width);

        // draw the scrollbar
        C_DrawScrollbar();
    }
    else
        consoleactive = false;
}

dboolean C_Responder(event_t *ev)
{
    if (consoleheight < CONSOLEHEIGHT && consoledirection == -1)
        return false;
#ifdef WII
    WPADData *data = WPAD_Data(0);

    if(data->exp.type == WPAD_EXP_CLASSIC)
    {
        int     key = data->btns_h;

        switch (key)
        {
            // scroll output up
            case WPAD_CLASSIC_BUTTON_UP:
                if (consolestrings > CONSOLELINES)
                    outputhistory = (outputhistory == -1 ? consolestrings - (CONSOLELINES + 1)
                        : MAX(0, outputhistory - 1));
                break;

            // scroll output down
            case WPAD_CLASSIC_BUTTON_DOWN:
                if (outputhistory != -1)
                {
                    ++outputhistory;
                    if (outputhistory + CONSOLELINES == consolestrings)
                        outputhistory = -1;
                }
                break;

            default:
                return false;
        }
    }
#else
    if (ev->type == ev_keydown)
    {
        int             key = ev->data1;
//        char            ch = (char)ev->data2;
//        int             i;
//        SDL_Keymod      modstate = SDL_GetModState();

        switch (key)
        {
            // scroll output up
            case KEY_PGUP:
                if (consolestrings > CONSOLELINES)
                    outputhistory = (outputhistory == -1 ? consolestrings - (CONSOLELINES + 1)
                        : MAX(0, outputhistory - 1));
                break;

            // scroll output down
            case KEY_PGDN:
                if (outputhistory != -1)
                {
                    ++outputhistory;
                    if (outputhistory + CONSOLELINES == consolestrings)
                        outputhistory = -1;
                }
                break;

            // close console
            case KEY_ESCAPE:
            case KEY_TILDE:
                consoledirection = -1;
                consoleanimindex = 0;
                break;

            default:
                return false;
/*
                if (modstate & KMOD_CTRL)
                {
                    // select all text
                    if (ch == 'a')
                    {
                        selectstart = 0;
                        selectend = caretpos = strlen(consoleinput);
                    }

                    // copy selected text to clipboard
                    else if (ch == 'c')
                    {
                        if (selectstart < selectend)
                            SDL_SetClipboardText(M_SubString(consoleinput, selectstart,
                                selectend - selectstart));
                    }

                    // paste text from clipboard
                    else if (ch == 'v')
                    {
                        char    buffer[255];

                        M_snprintf(buffer, sizeof(buffer), "%s%s%s",
                            M_SubString(consoleinput, 0, selectstart), SDL_GetClipboardText(),
                            M_SubString(consoleinput, selectend, strlen(consoleinput)
                                - selectend));
                        if (C_TextWidth(buffer) <= CONSOLEINPUTPIXELWIDTH)
                        {
                            C_AddToUndoHistory();
                            M_StringCopy(consoleinput, buffer, sizeof(consoleinput));
                            selectstart += strlen(SDL_GetClipboardText());
                            selectend = caretpos = selectstart;
                        }
                    }

                    // cut selected text to clipboard
                    else if (ch == 'x')
                    {
                        if (selectstart < selectend)
                        {
                            C_AddToUndoHistory();
                            SDL_SetClipboardText(M_SubString(consoleinput, selectstart,
                                selectend - selectstart));
                            for (i = selectend; (unsigned int)i < strlen(consoleinput); ++i)
                                consoleinput[selectstart + i - selectend] = consoleinput[i];
                            consoleinput[selectstart + i - selectend] = '\0';
                            caretpos = selectend = selectstart;
                            caretwait = I_GetTimeMS() + caretblinktime;
                            showcaret = true;
                        }
                    }

                    // undo
                    else if (ch == 'z')
                        if (undolevels)
                        {
                            --undolevels;
                            M_StringCopy(consoleinput, undohistory[undolevels].input,
                                sizeof(consoleinput));
                            caretpos = undohistory[undolevels].caretpos;
                            selectstart = undohistory[undolevels].selectstart;
                            selectend = undohistory[undolevels].selectend;
                        }
                }
                else
                {
                    if ((modstate & KMOD_SHIFT)
                        || (key_alwaysrun != KEY_CAPSLOCK && (modstate & KMOD_CAPS)))
                        ch = shiftxform[ch];
                    if (ch >= ' ' && ch < '~' && ch != '`'
                        && C_TextWidth(consoleinput) + (ch == ' ' ? spacewidth :
                        consolefont[ch - CONSOLEFONTSTART]->width) <= CONSOLEINPUTPIXELWIDTH
                        && !(modstate & KMOD_ALT))
                    {
                        C_AddToUndoHistory();
                        if (selectstart < selectend)
                        {
                            // replace selected text with a character
                            consoleinput[selectstart] = ch;
                            for (i = selectend; (unsigned int)i < strlen(consoleinput); ++i)
                                consoleinput[selectstart + i - selectend + 1] = consoleinput[i];
                            consoleinput[selectstart + i - selectend + 1] = '\0';
                            caretpos = selectend = selectstart + 1;
                            caretwait = I_GetTimeMS() + caretblinktime;
                            showcaret = true;
                        }
                        else
                        {
                            // insert a character
                            consoleinput[strlen(consoleinput) + 1] = '\0';
                            for (i = strlen(consoleinput); i > caretpos; --i)
                                consoleinput[i] = consoleinput[i - 1];
                            consoleinput[caretpos++] = ch;
                        }
                        selectstart = selectend = caretpos;
                        caretwait = I_GetTimeMS() + caretblinktime;
                        showcaret = true;
                        autocomplete = -1;
                        inputhistory = -1;
                    }
                }
*/
        }
    }
    else if (ev->type == ev_keyup)
        return false;
#ifndef WII
    else if (ev->type == ev_mousewheel)
    {
        // scroll output up
        if (ev->data1 > 0)
        {
            if (consolestrings > 10)
                outputhistory = (outputhistory == -1 ? consolestrings - 11 :
                    MAX(0, outputhistory - 1));
        }

        // scroll output down
        else if (ev->data1 < 0)
        {
            if (outputhistory != -1)
            {
                ++outputhistory;
                if (outputhistory + 10 == consolestrings)
                    outputhistory = -1;
            }
        }
    }
#endif
#endif
    return true;
}

static int dayofweek(int day, int month, int year)
{
    int adjustment = (14 - month) / 12;
    int m = month + 12 * adjustment - 2;
    int y = year - adjustment;

    return ((day + (13 * m - 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7);
}

void C_PrintCompileDate(void)
{
    int                 day, month, year, hour, minute;
    static const char   *days[] =
    {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    static const char   mths[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    static const char   *months[] =
    {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    static char         mth[4] = "";

    sscanf(__DATE__, "%3s %2d %4d", mth, &day, &year);
    sscanf(__TIME__, "%2d:%2d:%*d", &hour, &minute);
    month = (strstr(mths, mth) - mths) / 3;

    C_Output(" This %i-bit %s binary of ~Wii-DOOM~ was built on %s, %s %i, "
        "%i at %i:%02i%s", (sizeof(intptr_t) == 4 ? 32 : 64), "Linux",
        days[dayofweek(day, month + 1, year)], months[month], day, year,
        (hour > 12 ? hour - 12 : hour), minute, (hour < 12 ? "am" : "pm"));
}

void C_PrintSDLVersions(void)
{
#ifdef SDL2
    int revision = SDL_GetRevisionNumber();

    if (revision)
        C_Warning(" Using version %i.%i.%i (Revision %i) of %s",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL, revision,
            "libSDL.a"
            );
    else
#endif
        C_Warning(" Using version %i.%i.%i of %s",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
            "libSDL.a"
            );

    C_Warning(" Using version %i.%i.%i of %s",
        SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL,
        "libSDL_mixer.a"
        );
#ifdef WII
    C_Warning(" Using version %i.%i.%i of %s",
        SMPEG_MAJOR_VERSION, SMPEG_MINOR_VERSION, SMPEG_PATCHLEVEL,
        "libsmpeg.a"
        );

    C_Warning(" Using version %i.%i.%i of %s",
        _V_MAJOR_, _V_MINOR_, _V_PATCH_,
        "libogc.a"
        );

    C_Warning(" Using version %i.%i.%i of %s",
        PNG_LIBPNG_VER_MAJOR, PNG_LIBPNG_VER_MINOR, PNG_LIBPNG_VER_RELEASE,
        "libpng15.a"
        );

    C_Warning(" Using version %i (6b) of %s",
        JPEG_LIB_VERSION,
        "libjpeg.a"
        );

    C_Warning(" Using version %i.%i.%i.%i of %s",
        ZLIB_VER_MAJOR, ZLIB_VER_MINOR, ZLIB_VER_REVISION, ZLIB_VER_SUBREVISION,
        "libz.a"
        );

    C_Warning(" Also using the following libraries:");
    C_Warning(" libvorbisidec.a libwiilight.a, libfat.a, libwiiuse.a, libbte.a,");
    C_Warning(" libwiikeyboard.a, libsupc++.a, libstdc++.a, libm.a");
#endif
}

void C_ConDump(void)
{
    if (consolestrings)
    {
        char *filename;

        FILE *file;

#ifdef WII
        if(usb)
            filename = "usb:/apps/wiidoom/condump.txt";
        else
            filename = "sd:/apps/wiidoom/condump.txt";
#else
        filename = "condump.txt";
#endif

        file = fopen(filename, "wt");

        if (file)
        {
            int i;

            for (i = 1; i < consolestrings - 1; ++i)
            {
                if (console[i].type == dividerstring)
                    fprintf(file, "%s\n", DIVIDERSTRING);
                else
                {
                    unsigned int        inpos;
                    unsigned int        spaces;
                    unsigned int        len = strlen(console[i].string);
                    unsigned int        outpos = 0;
                    int                 tabcount = 0;

                    for (inpos = 0; inpos < len; ++inpos)
                    {
                        char    ch = console[i].string[inpos];

                        if (ch != '\n')
                        {
                            if (ch == '\t')
                            {
                                unsigned int    tabstop = console[i].tabs[tabcount] / 5;

                                if (outpos < tabstop)
                                {
                                    for (spaces = 0; spaces < tabstop - outpos; ++spaces)
                                        fputc(' ', file);
                                    outpos = tabstop;
                                    ++tabcount;
                                }
                                else
                                {
                                    fputc(' ', file);
                                    ++outpos;
                                }
                            }
                            else
                            {
                                fputc(ch, file);
                                ++outpos;
                            }
                        }
                    }

                    if (console[i].timestamp[0])
                    {
                        for (spaces = 0; spaces < 91 - outpos; ++spaces)
                            fputc(' ', file);
                        fputs(console[i].timestamp, file);
                    }

                    fputc('\n', file);
                }
            }
            fclose(file);
        }
    }
}

void C_Error(char *string, ...)
{
    va_list     argptr;
    char        buffer[1024] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);

    if (consolestrings && strcasecmp(console[consolestrings - 1].string, buffer))
    {
#ifdef BOOM_ZONE_HANDLING
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console), PU_STATIC, NULL);
#else
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
#endif
        console[consolestrings].string = strdup(buffer);
        console[consolestrings].type = redstring;
        memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
        console[consolestrings].timestamp = "";
        ++consolestrings;
        outputhistory = -1;
    }
}

void C_Network(char *string, ...)
{
    va_list     argptr;
    char        buffer[1024] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);

    if (consolestrings && strcasecmp(console[consolestrings - 1].string, buffer))
    {
#ifdef BOOM_ZONE_HANDLING
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console), PU_STATIC, NULL);
#else
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
#endif
        console[consolestrings].string = strdup(buffer);
        console[consolestrings].type = bluestring;
        memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
        console[consolestrings].timestamp = "";
        ++consolestrings;
        outputhistory = -1;
    }
}

