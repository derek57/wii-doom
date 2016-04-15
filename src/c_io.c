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

#ifdef HAVE_STDLIB_H
#undef HAVE_STDLIB_H
#endif

#include <ogc/libversion.h>
#include <png.h>
#endif

#include <time.h>

#include "c_cmds.h"
#include "c_io.h"
#include "config.h"
#include "d_deh.h"
#include "d_event.h"
#include "doom/d_main.h"
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

#include "doom/m_random.h"
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
#include "wii-doom.h"
#include "z_zone.h"

#ifdef WII
#include <wiiuse/wpad.h>
#endif

#if !defined(MAX_PATH)
#define MAX_PATH                260
#endif

#define CONSOLESPEED            (CONSOLEHEIGHT / 12)
#define CONSOLEFONTSTART        ' '
#define CONSOLEFONTEND          '~'
#define CONSOLEFONTSIZE         (CONSOLEFONTEND - CONSOLEFONTSTART + 1)

#define CONSOLETEXTX            10
#define CONSOLETEXTY            8
#define CONSOLETEXTMAXLENGTH    1024 
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

#define CARETBLINKTIME          530

#define NOBACKGROUNDCOLOR       -1
#define NOQUOTE                 0
#define LDQUOTE                 1
#define RDQUOTE                 2


static int      timestampx;
static int      zerowidth;
static int      caretwait;
static int      outputhistory = -1;
static int      consolewait;
static int      notabs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

#ifndef WII
static int      autocomplete = -1;
static int      inputhistory = -1;
static int      undolevels = 0;
#endif

static int      caretblinktime;
static int      caretpos = 0;
static int      selectend = 0;
static int      selectstart = 0;

static char     consoleinput[255] = "";

#ifndef WII
static char     currentinput[255] = "";
static char     autocompletetext[255] = "";
#endif

static dboolean showcaret = true;
static dboolean forceblurredraw = false;

static patch_t  *unknownchar;
static patch_t  *consolefont[CONSOLEFONTSIZE];
static patch_t  *ldquote;
static patch_t  *rdquote;
static patch_t  *degree;
static patch_t  *multiply;
static patch_t  *caret;
static patch_t  *route;
static patch_t  *warning; 


dboolean        consoleactive = false;
dboolean        alwaysrun;

// actually it's colored red
byte            dividercolor = 0;

byte            inputcolor = 4;
byte            whitecolor = 80;
byte            bluecolor = 200;
byte            redcolor = 30;
byte            graycolor = 100;
byte            greencolor = 120;
byte            yellowcolor = 160;
byte            c_tempscreen[SCREENWIDTH * SCREENHEIGHT];
byte            c_blurscreen[SCREENWIDTH * SCREENHEIGHT];
byte            consolebrandingcolor = 100;
byte            consolescrollbartrackcolor = 100;
byte            consolescrollbarfacecolor = 88;
byte            consoletintcolor = 5;
byte            consolecolors[STRINGTYPES];

char            consolecheat[255] = "";
char            consolecheatparm[3] = "";

char            *upper =
{
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0 !\"#$%&\"()*+,_>?)!@#$%^&*(:"
    ":<+>?\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0{\\}^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ"
};

int             consoleheight = 0;
int             consoledirection = -1;
int             consolestrings = 0;
int             consoleedgecolor1 = 105;
int             consoleedgecolor2 = 100;
int             consoleselectedinputcolor = 4;
int             consoleselectedinputbackgroundcolor = 100;
int             consoleinputcolor = 4;
int             consolecaretcolor = 4;
int             spacewidth;
int             numconsolecmds;


extern dboolean wipe;

extern int      fps;


void C_DebugOutput(char *string)
{
/*
#if defined(_MSC_VER) && defined (_DEBUG)
    OutputDebugString(M_StringJoin(string, "\n", NULL));
#endif
*/
    if (devparm)
    {
        printf("%s%s;%sDEBUG: %s%sm", COLORIZE_CMD, BACKGROUND_RED, TEXT_YELLOW, COLORIZE_CMD, COLORIZE_NORMAL);
        printf("%s\n", string);
    }
}

void C_Print(stringtype_t typestring, char *string, ...)
{
    va_list     argptr;
    char        buffer[CONSOLETEXTMAXLENGTH] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, CONSOLETEXTMAXLENGTH - 1, string, argptr);
    va_end(argptr);

    console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
    M_StringCopy(console[consolestrings].string, buffer, CONSOLETEXTMAXLENGTH); 
    console[consolestrings].type = typestring;
    memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
    console[consolestrings].timestamp[0] = '\0'; 
    ++consolestrings;
    outputhistory = -1;
    C_DebugOutput(buffer);
}

void C_Input(char *string, ...)
{
    va_list     argptr;
    char        buffer[CONSOLETEXTMAXLENGTH] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, CONSOLETEXTMAXLENGTH - 1, string, argptr);
    va_end(argptr);

    console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
    M_StringCopy(console[consolestrings].string, buffer, CONSOLETEXTMAXLENGTH);
    console[consolestrings].type = graystring;
    memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
    console[consolestrings].timestamp[0] = '\0';
    ++consolestrings;
    outputhistory = -1;
    C_DebugOutput(buffer);
}

void C_Output(char *string, ...)
{
    va_list     argptr;
    char        buffer[CONSOLETEXTMAXLENGTH] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, CONSOLETEXTMAXLENGTH - 1, string, argptr);
    va_end(argptr);

    console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
    M_StringCopy(console[consolestrings].string, buffer, CONSOLETEXTMAXLENGTH); 
    console[consolestrings].type = graystring;
    memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
    console[consolestrings].timestamp[0] = '\0'; 
    ++consolestrings;
    outputhistory = -1;
    C_DebugOutput(buffer);
}

void C_Warning(char *string, ...)
{
    va_list     argptr;
    char        buffer[CONSOLETEXTMAXLENGTH] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, CONSOLETEXTMAXLENGTH - 1, string, argptr);
    va_end(argptr);

    if (consolestrings && !M_StringCompare(console[consolestrings - 1].string, buffer))
    {
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
        M_StringCopy(console[consolestrings].string, buffer, CONSOLETEXTMAXLENGTH); 
        console[consolestrings].type = yellowstring;
        memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
        console[consolestrings].timestamp[0] = '\0'; 
        ++consolestrings;
        outputhistory = -1;
        C_DebugOutput(buffer);
    }
}

void C_PlayerMessage(char *string, ...)
{
    va_list     argptr;
    char        buffer[CONSOLETEXTMAXLENGTH] = "";
    dboolean    prevplayermessage = (consolestrings && console[consolestrings - 1].type == greencolor);
    time_t      rawtime; 

    va_start(argptr, string);
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);

    time(&rawtime);

    if (prevplayermessage && M_StringCompare(console[consolestrings - 1].string, buffer)) 
    {
        M_snprintf(console[consolestrings - 1].string, CONSOLETEXTMAXLENGTH, "%s (2)", buffer); 

        strftime(console[consolestrings - 1].timestamp, 9, "%H:%M:%S", localtime(&rawtime));
    }
    else if (prevplayermessage && M_StringStartsWith(console[consolestrings - 1].string, buffer)) 
    {
        char    *count = strrchr(console[consolestrings - 1].string, '(') + 1;

        count[strlen(count) - 1] = '\0';
        M_snprintf(console[consolestrings - 1].string, CONSOLETEXTMAXLENGTH, "%s (%i)", buffer, atoi(count) + 1); 

        strftime(console[consolestrings - 1].timestamp, 9, "%H:%M:%S", localtime(&rawtime)); 
    }
    else
    {
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
        M_StringCopy(console[consolestrings].string, buffer, CONSOLETEXTMAXLENGTH); 
        console[consolestrings].type = greenstring;
        memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));

        strftime(console[consolestrings].timestamp, 9, "%H:%M:%S", localtime(&rawtime)); 

        ++consolestrings;
    }

    outputhistory = -1;
    C_DebugOutput(buffer);
}

#ifndef WII
static void C_AddToUndoHistory(void)
{
    undohistory = Z_Realloc(undohistory, (undolevels + 1) * sizeof(*undohistory));
    undohistory[undolevels].input = strdup(consoleinput);
    undohistory[undolevels].caretpos = caretpos;
    undohistory[undolevels].selectstart = selectstart;
    undohistory[undolevels].selectend = selectend;
    ++undolevels;
}
#endif

void C_AddConsoleDivider(void)
{
    if (!consolestrings || !M_StringCompare(console[consolestrings - 1].string, DIVIDER))
        C_Print(dividerstring, DIVIDER);
}

static void C_DrawDivider(int y)
{
    int i;

    y *= SCREENWIDTH;

    if (y >= CONSOLETOP * SCREENWIDTH)
        for (i = y + CONSOLETEXTX; i < y + CONSOLETEXTX + CONSOLEDIVIDERWIDTH; ++i)
            screens[0][i] = redcolor;

    if ((y += SCREENWIDTH) >= CONSOLETOP * SCREENWIDTH)
        for (i = y + CONSOLETEXTX; i < y + CONSOLETEXTX + CONSOLEDIVIDERWIDTH; ++i)
            screens[0][i] = redcolor;
}

static struct
{
    char        char1;
    char        char2;
    int         adjust;
} kern[] = {
    { ' ',  '(',  -1 }, { ' ',  'T',  -1 }, { '\\', 'V',  -1 }, { '\'', 'a',  -1 },
    { '\'', 'c',  -1 }, { '\'', 'd',  -1 }, { '\'', 'e',  -1 }, { '\'', 'g',  -1 },
    { '\'', 'j',  -2 }, { '\'', 'o',  -1 }, { '\'', 's',  -1 }, { '.',  '\\', -1 },
    { '.',  '4',  -1 }, { '.',  '7',  -1 }, { ',',  '4',  -1 }, { '/',  'o',  -1 },
    { ':', '\\',  -1 }, { '_',  'f',  -1 }, { '0',  ',',  -1 }, { '0',  'j',  -2 },
    { '1',  '\"', -1 }, { '1',  '\'', -1 }, { '1',  'j',  -2 }, { '2',  'j',  -2 },
    { '3',  ',',  -1 }, { '3',  'j',  -2 }, { '4',  'j',  -2 }, { '5',  ',',  -1 },
    { '5',  'j',  -2 }, { '6',  ',',  -1 }, { '6',  'j',  -2 }, { '7',  ',',  -2 },
    { '7',  'j',  -2 }, { '8',  ',',  -1 }, { '8',  'j',  -2 }, { '9',  ',',  -1 },
    { '9',  'j',  -2 }, { 'F',  '.',  -1 }, { 'F',  ',',  -1 }, { 'L',  '\\', -1 },
    { 'L',  '\"', -1 }, { 'L',  '\'', -1 }, { 'P',  '.',  -1 }, { 'P',  ',',  -1 },
    { 'T',  '.',  -1 }, { 'T',  ',',  -1 }, { 'V',  '.',  -1 }, { 'V',  ',',  -1 },
    { 'Y',  '.',  -1 }, { 'Y',  ',',  -1 }, { 'a',  '\"', -1 }, { 'a',  '\'', -1 },
    { 'a',  'j',  -2 }, { 'b',  ',',  -1 }, { 'b',  '\"', -1 }, { 'b',  '\\', -1 },
    { 'b',  '\'', -1 }, { 'b',  'j',  -2 }, { 'c',  '\\', -1 }, { 'c',  ',',  -1 },
    { 'c',  '\"', -1 }, { 'c',  '\'', -1 }, { 'c',  'j',  -2 }, { 'd',  'j',  -2 },
    { 'e',  '\\', -1 }, { 'e',  ',',  -1 }, { 'e',  '\"', -1 }, { 'e',  '\'', -1 },
    { 'e',  '_',  -1 }, { 'e',  'j',  -2 }, { 'f',  ' ',  -1 }, { 'f',  ',',  -2 },
    { 'f',  '_',  -1 }, { 'f',  'a',  -1 }, { 'f',  'j',  -2 }, { 'h',  '\\', -1 },
    { 'h',  '\"', -1 }, { 'h',  '\'', -1 }, { 'h',  'j',  -2 }, { 'i',  'j',  -2 },
    { 'k',  'j',  -2 }, { 'l',  'j',  -2 }, { 'm',  '\"', -1 }, { 'm',  '\\', -1 },
    { 'm',  '\'', -1 }, { 'm',  'j',  -2 }, { 'n',  '\\', -1 }, { 'n',  '\"', -1 },
    { 'n',  '\'', -1 }, { 'n',  'j',  -2 }, { 'o',  '\\', -1 }, { 'o',  ',',  -1 },
    { 'o',  '\"', -1 }, { 'o',  '\'', -1 }, { 'o',  'j',  -2 }, { 'p',  '\\', -1 },
    { 'p',  ',',  -1 }, { 'p',  '\"', -1 }, { 'p',  '\'', -1 }, { 'p',  'j',  -2 },
    { 'r',  ' ',  -1 }, { 'r',  '\\', -1 }, { 'r',  '.',  -2 }, { 'r',  ',',  -2 },
    { 'r',  '\"', -1 }, { 'r',  '\'', -1 }, { 'r',  '_',  -1 }, { 'r',  'a',  -1 },
    { 'r',  'j',  -2 }, { 's',  ',',  -1 }, { 's',  'j',  -2 }, { 't',  'j',  -2 },
    { 'u',  'j',  -2 }, { 'v',  ',',  -1 }, { 'v',  'j',  -2 }, { 'w',  'j',  -2 },
    { 'x',  'j',  -2 }, { 'z',  'j',  -2 }, {  0 ,   0 ,   0 }
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
                screens[0][y - offset + x] = tinttab50[screens[0][y - offset + x]
                    + consolescrollbartrackcolor];

    // Draw scrollbar face
    facestart = (CONSOLESCROLLBARY + CONSOLESCROLLBARHEIGHT * (outputhistory == -1 ?
        MAX(0, consolestrings - CONSOLELINES) : outputhistory) / consolestrings) * SCREENWIDTH;

    faceend = facestart + (CONSOLESCROLLBARHEIGHT - CONSOLESCROLLBARHEIGHT
        * MAX(0, consolestrings - CONSOLELINES) / consolestrings) * SCREENWIDTH;

    for (y = facestart; y < faceend; y += SCREENWIDTH)
        if (y - offset >= 0)
            for (x = CONSOLESCROLLBARX; x < CONSOLESCROLLBARX + CONSOLESCROLLBARWIDTH; ++x)
                screens[0][y - offset + x] = consolescrollbarfacecolor;
}

void C_Init(void)
{
    int         i;
    int         j = CONSOLEFONTSTART;
    char        buffer[9];

#ifndef WII
    while (*consolecmds[numconsolecmds++].name);
#endif

    unknownchar = W_CacheLumpName("DRFON000", PU_STATIC);

    for (i = 0; i < CONSOLEFONTSIZE; i++)
    {
        M_snprintf(buffer, 9, "DRFON%03d", j++);
        consolefont[i] = W_CacheLumpName(buffer, PU_STATIC);
    }

    ldquote = W_CacheLumpName("DRFON147", PU_STATIC);
    rdquote = W_CacheLumpName("DRFON148", PU_STATIC);
    degree = W_CacheLumpName("DRFON176", PU_STATIC);
    multiply = W_CacheLumpName("DRFON215", PU_STATIC);
    warning = W_CacheLumpName("DRFONWRN", PU_STATIC); 

    caret = W_CacheLumpName("CARET", PU_STATIC);
    route = W_CacheLumpName("DRFON036", PU_STATIC);

    caretblinktime = CARETBLINKTIME;
    spacewidth = SHORT(consolefont[' ' - CONSOLEFONTSTART]->width);
    timestampx = SCREENWIDTH - C_TextWidth("00:00:00") - CONSOLETEXTX * 2
        - CONSOLESCROLLBARWIDTH + 1;

    zerowidth = SHORT(consolefont['0' - CONSOLEFONTSTART]->width);

    // yellow = 160
    consolecolors[yellowstring] = yellowcolor;

    // red = 40
    consolecolors[redstring] = redcolor;

    // gray = 100
    consolecolors[graystring] = graycolor;

    // blue = 200
    consolecolors[bluestring] = bluecolor;

    // white = 80
    consolecolors[whitestring] = whitecolor;

    // green = 120
    consolecolors[greenstring] = greencolor;

    // divider = 0 (linked to red color)
    consolecolors[dividerstring] = dividercolor;

    consoletintcolor <<= 8;
    consoleedgecolor1 <<= 8;
    consoleedgecolor2 <<= 8;
    consolescrollbartrackcolor <<= 8;
    dividercolor <<= 8;
}

void C_HideConsole(void)
{
    consoledirection = -1;
}

void C_HideConsoleFast(void)
{
    consoleheight = 0;
    consoledirection = -1;
    consoleactive = false;
}

void C_StripQuotes(char *string)
{
    size_t len = strlen(string);

    if (len > 2 && ((string[0] == '\"' && string[len - 1] == '\"')
        || (string[0] == '\'' && string[len - 1] == '\'')))
    {
        len -= 2;
        memmove(string, string + 1, len);
        string[len] = '\0';
    }
}

static void BlurConsoleScreen(int x1, int y1, int x2, int y2, int i)
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
            c_blurscreen[i] = screens[0][i];

        BlurConsoleScreen(0, 0, SCREENWIDTH - 1, height, 1);
        BlurConsoleScreen(1, 0, SCREENWIDTH, height, -1);
        BlurConsoleScreen(0, 0, SCREENWIDTH - 1, height - SCREENWIDTH, SCREENWIDTH + 1);
        BlurConsoleScreen(1, SCREENWIDTH, SCREENWIDTH, height, -(SCREENWIDTH + 1));
        BlurConsoleScreen(0, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH);
        BlurConsoleScreen(0, SCREENWIDTH, SCREENWIDTH, height, -SCREENWIDTH);
        BlurConsoleScreen(1, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH - 1);
        BlurConsoleScreen(0, SCREENWIDTH, SCREENWIDTH - 1, height, -(SCREENWIDTH - 1));
    }

    blurred = (consoleheight == CONSOLEHEIGHT && !wipe);

    for (i = 0; i < height; ++i)
        screens[0][i] = colormaps[0][256 * M_RandomInt(0, 10) + tinttab50[c_blurscreen[i]
            + consoletintcolor]];

    for (i = height - 2; i > 1; i -= 3)
    {
        screens[0][i] = colormaps[0][256 * 6 + screens[0][i]];

        if (((i - 1) % SCREENWIDTH) < SCREENWIDTH - 2)
              screens[0][i + 1] = colormaps[0][256 * 6 + screens[0][i - 1]];
    }

    for (i = height - SCREENWIDTH * 3; i < height - SCREENWIDTH * 2; ++i)
        screens[0][i] = tinttab25[consoleedgecolor1 + screens[0][i]];

    for (i = height - SCREENWIDTH * 2; i < height; ++i)
        screens[0][i] = tinttab25[consoleedgecolor2 + screens[0][i]];

    for (j = 1; j <= 4; ++j)
        for (i = height; i < height + SCREENWIDTH * j; ++i)
            screens[0][i] = colormaps[0][256 * 4 + screens[0][i]];
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

static void C_DrawConsoleText(int x, int y, char *text, int color1, int color2, byte *tinttab,
    int tabs[8], dboolean matchingquotes)
{
    dboolean            italics = false;
    size_t              i;
    int                 tab = -1;
    size_t              len = strlen(text);
    unsigned char       prevletter = '\0';
    int                 prevquote = NOQUOTE; 

    y -= CONSOLEHEIGHT - consoleheight; 

    if (len > 80)
        while (C_TextWidth(text) > SCREENWIDTH - CONSOLETEXTX * 3 - CONSOLESCROLLBARWIDTH + 2)
        {
            text[len - 1] = '.';
            text[len] = '.';
            text[len + 1] = '.';
            text[len + 2] = '\0';
            --len;
        }

    if (color1 == yellowstring)
    {
        V_DrawConsoleChar(x, y, 0, warning, color1, color2, false, tinttab);
        x += SHORT(warning->width) + 2;
    }

    for (i = 0; i < len; ++i)
    {
        unsigned char   letter = text[i];
        int             c = letter - CONSOLEFONTSTART;
        unsigned char   nextletter = text[i + 1];

        if (letter == ITALICS && prevletter != ITALICS)
            x += (!(italics = !italics)); 
        else
        {
            patch_t     *patch = NULL;

            if (letter == ITALICS)
                italics = false;
            else if (letter == '\t') 
                x = (x > tabs[++tab] ? x + spacewidth : tabs[tab]);
            else if (letter == '\xc2' && nextletter == '\xb0')
            {
                patch = degree;
                ++i;
            }
            else if (letter == 215)
                patch = multiply;
            else if (letter == '\"' && matchingquotes) 
            {
                if (prevquote == NOQUOTE || prevquote == RDQUOTE)
                {
                    int j;

                    for (j = i + 1; (size_t)j < len; ++j)
                        if (text[j] == '\"')
                        {
                            patch = ldquote;
                            prevquote = LDQUOTE;
                            break;
                        }
                }
                else if (prevquote == LDQUOTE)
                {
                    patch = rdquote;
                    prevquote = RDQUOTE;
                }
            }
            else
                patch = (c < 0 || c >= CONSOLEFONTSIZE ? unknownchar : consolefont[c]);

            if (!italics)
            {
                int     j = 0;

                while (kern[j].char1)
                {
                    if (prevletter == kern[j].char1 && letter == kern[j].char2)
                    {
                        x += kern[j].adjust;
                        break;
                    }

                    ++j;
                }
            }

            if (patch)
            {
                V_DrawConsoleChar(x, y, 0, patch, color1, color2, italics, tinttab);
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

        V_DrawConsoleChar(x + (text[i] == '1' ? (zerowidth - SHORT(patch->width)) / 2 : 0), y, 0,
            patch, consolebrandingcolor, NOBACKGROUNDCOLOR, false, tinttab25);

        x += (isdigit(text[i]) ? zerowidth : SHORT(patch->width));
    }
}

void C_Drawer(void)
{
    if (consoleheight)
    {
        int             i;
        int             x = CONSOLETEXTX;
        int             start;
        int             end;
        char            *lefttext = Z_Malloc(512, PU_STATIC, NULL);
        char            *middletext = Z_Malloc(512, PU_STATIC, NULL);
        char            *righttext = Z_Malloc(512, PU_STATIC, NULL);

        // adjust console height
        if (consolewait < I_GetTime())
        {
            consoleheight = BETWEEN(0, consoleheight + CONSOLESPEED * consoledirection,
                CONSOLEHEIGHT);

            consolewait = I_GetTime();
        }

        consoleactive = (consoledirection == 1);

        // draw tiled background and bottom edge
        C_DrawBackground(consoleheight);

        // draw branding
        C_DrawConsoleText(SCREENWIDTH - C_TextWidth(PACKAGE_NAME) - CONSOLETEXTX + 1,
            CONSOLEHEIGHT - 10, PACKAGE_NAME, consolebrandingcolor, NOBACKGROUNDCOLOR,
            tinttab25, notabs, false);

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
                    consolecolors[console[i].type], NOBACKGROUNDCOLOR, tinttab66, console[i].tabs,
                    true);

                if (*console[i].timestamp)
                    C_DrawTimeStamp(timestampx, y + (CONSOLELINEHEIGHT / 2), console[i].timestamp);
            }
        }

        V_DrawConsoleChar(x, consoleheight - 10, 0, route,
                inputcolor, NOBACKGROUNDCOLOR, false, NULL);

        // draw input text to left of caret
        for (i = 0; i < MIN(selectstart, caretpos); ++i)
            lefttext[i] = consoleinput[i];

        lefttext[i] = '\0';

        C_DrawConsoleText(SHORT(route->width + caret->width) + x,
            CONSOLEHEIGHT - 10, lefttext, consoleinputcolor, NOBACKGROUNDCOLOR,
            NULL, notabs, false);

        x += C_TextWidth(lefttext);

        // draw any selected text to left of caret
        if (selectstart < caretpos)
        {
            for (i = selectstart; i < selectend; ++i)
                middletext[i - selectstart] = consoleinput[i];

            middletext[i - selectstart] = '\0';

            if (*middletext)
            {
                C_DrawConsoleText(SHORT(route->width + caret->width) + x,
                    CONSOLEHEIGHT - 10, middletext, consoleselectedinputcolor,
                    consoleselectedinputbackgroundcolor, NULL, notabs, false);

                x += C_TextWidth(middletext);
            }
        }

        // draw caret
        if (caretwait < I_GetTimeMS())
        {
            showcaret = !showcaret;
            caretwait = I_GetTimeMS() + caretblinktime;
        }

        if (showcaret)
        {
            if (selectend > caretpos)
                V_DrawConsoleChar(SHORT(route->width + caret->width) + x,
                    consoleheight - 10, 0, caret, consoleselectedinputcolor,
                    consoleselectedinputbackgroundcolor, false, NULL);
            else
                V_DrawConsoleChar(SHORT(route->width + caret->width) + x,
                    consoleheight - 10, 0, caret, consolecaretcolor,
                    NOBACKGROUNDCOLOR, false, NULL);
        }

        x += SHORT(caret->width);

        // draw any selected text to right of caret
        if (selectend > caretpos)
        {
            for (i = selectstart; i < selectend; ++i)
                middletext[i - selectstart] = consoleinput[i];

            middletext[i - selectstart] = '\0';

            if (*middletext)
            {
                C_DrawConsoleText(SHORT(route->width + caret->width) + x,
                    CONSOLEHEIGHT - 10, middletext, consoleselectedinputcolor,
                    consoleselectedinputbackgroundcolor, NULL, notabs, false);

                x += C_TextWidth(middletext);
            }
        }

        // draw input text to right of caret
        if ((unsigned int)caretpos < strlen(consoleinput))
        {
            for (i = selectend; (unsigned int)i < strlen(consoleinput); ++i)
                righttext[i - selectend] = consoleinput[i];

            righttext[i - selectend] = '\0';

            if (*righttext)
                C_DrawConsoleText(SHORT(route->width + caret->width) + x,
                    CONSOLEHEIGHT - 10, righttext, consoleinputcolor,
                    NOBACKGROUNDCOLOR, NULL, notabs, false);
        }

        Z_Free(lefttext);
        Z_Free(middletext);
        Z_Free(righttext);

        // draw the scrollbar
        C_DrawScrollbar();
    }
    else
        consoleactive = false;
}

dboolean C_ValidateInput(char *input)
{
    int i = 0;

    while (*consolecmds[i].name)
    {
        char    cmd[256] = "";

        if (consolecmds[i].type == CT_CHEAT)
        {
            if (consolecmds[i].parameters)
            {
                size_t  length = strlen(input);

                if (isdigit(input[length - 2]) && isdigit(input[length - 1]))
                {
                    consolecheatparm[0] = input[length - 2];
                    consolecheatparm[1] = input[length - 1];
                    consolecheatparm[2] = '\0';

                    M_StringCopy(cmd, input, 255);
                    cmd[length - 2] = '\0';

                    if ((M_StringCompare(cmd, consolecmds[i].name)
                        || M_StringCompare(cmd, consolecmds[i].alternate))
                        && length == strlen(cmd) + 2
                        && consolecmds[i].func1(consolecmds[i].name, consolecheatparm, "", ""))
                    {
                        M_StringCopy(consolecheat, cmd, 255);
                        return true;
                    }
                }
            }
            else if ((M_StringCompare(input, consolecmds[i].name)
                || M_StringCompare(input, consolecmds[i].alternate))
                && consolecmds[i].func1(consolecmds[i].name, "", "", ""))
            {
                M_StringCopy(consolecheat, input, 255);
                return true;
            }
        }
        else
        {
            char        parm1[256] = "";
            char        parm2[256] = "";
            char        parm3[256] = "";

            sscanf(input, "%255s %255s %255s %255s", cmd, parm1, parm2, parm3);
            C_StripQuotes(parm1);
            C_StripQuotes(parm2);
            C_StripQuotes(parm3);
            if ((M_StringCompare(cmd, consolecmds[i].name)
                || M_StringCompare(cmd, consolecmds[i].alternate))
                && consolecmds[i].func1(consolecmds[i].name, parm1, parm2, parm3))
            {
                C_Input((input[strlen(input) - 1] == '%' ? "%s%" : "%s"), input);
                consolecmds[i].func2(consolecmds[i].name, parm1, parm2, parm3);
                return true;
            }
        }
        ++i;
    }
    return false;
}

dboolean C_Responder(event_t *ev)
{
#ifdef WII
    WPADData *data = WPAD_Data(0);
#endif

    if (consoleheight < CONSOLEHEIGHT && consoledirection == -1)
        return false;

#ifdef WII
    if (data->exp.type == WPAD_EXP_CLASSIC)
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
        char            ch = (char)ev->data2;
        int             i;

        if (key == KEY_TILDE)
        {
            consoledirection = -1;
            return true;
        }

        switch (key)
        {
            case KEY_BACKSPACE:
                if (selectstart < selectend)
                {
                    // delete selected text
                    C_AddToUndoHistory();
                    for (i = selectend; (unsigned int)i < strlen(consoleinput); ++i)
                        consoleinput[selectstart + i - selectend] = consoleinput[i];
                    consoleinput[selectstart + i - selectend] = '\0';
                    caretpos = selectend = selectstart;
                    caretwait = I_GetTimeMS() + caretblinktime;
                    showcaret = true;
                    autocomplete = -1;
                    inputhistory = -1;
                }
                else if (caretpos > 0)
                {
                    // delete character left of caret
                    C_AddToUndoHistory();
                    for (i = caretpos - 1; (unsigned int)i < strlen(consoleinput); ++i)
                        consoleinput[i] = consoleinput[i + 1];
                    selectend = selectstart = --caretpos;
                    caretwait = I_GetTimeMS() + caretblinktime;
                    showcaret = true;
                    autocomplete = -1;
                    inputhistory = -1;
                }
                break;

            case KEY_DEL:
                if (selectstart < selectend)
                {
                    // delete selected text
                    C_AddToUndoHistory();
                    for (i = selectend; (unsigned int)i < strlen(consoleinput); ++i)
                        consoleinput[selectstart + i - selectend] = consoleinput[i];
                    consoleinput[selectstart + i - selectend] = '\0';
                    caretpos = selectend = selectstart;
                    caretwait = I_GetTimeMS() + caretblinktime;
                    showcaret = true;
                    autocomplete = -1;
                    inputhistory = -1;
                }
                else if ((unsigned int)caretpos < strlen(consoleinput))
                {
                    // delete character right of caret
                    C_AddToUndoHistory();
                    for (i = caretpos; (unsigned int)i < strlen(consoleinput); ++i)
                        consoleinput[i] = consoleinput[i + 1];
                    caretwait = I_GetTimeMS() + caretblinktime;
                    showcaret = true;
                    autocomplete = -1;
                    inputhistory = -1;
                }
                break;

            // confirm input
            case KEY_ENTER:
                if (*consoleinput)
                {
                    if (C_ValidateInput(consoleinput))
                    {
                        // clear input
                        consoleinput[0] = '\0';
                        caretpos = selectstart = selectend = 0;
                        caretwait = I_GetTimeMS() + caretblinktime;
                        showcaret = true;
                        undolevels = 0;
                        autocomplete = -1;
                        inputhistory = -1;
                        outputhistory = -1;
                        forceblurredraw = true;
                    }

                    return !consolecheat[0];
                }
                break;

            // move caret left
            case KEY_LEFTARROW:
                if (caretpos > 0)
                {
/*
                    if (modstate & KMOD_SHIFT)
                    {
                        --caretpos;
                        caretwait = I_GetTimeMS() + caretblinktime;
                        showcaret = true;
                        if (selectstart <= caretpos)
                            selectend = caretpos;
                        else
                            selectstart = caretpos;
                    }
                    else
*/
                    {
                        if (selectstart < selectend)
                            caretpos = selectend = selectstart;
                        else
                            selectstart = selectend = --caretpos;
                        caretwait = I_GetTimeMS() + caretblinktime;
                        showcaret = true;
                    }
                }
                else //if (!(modstate & KMOD_SHIFT))
                    caretpos = selectend = selectstart = 0;
                break;

            // move caret right
            case KEY_RIGHTARROW:
                if ((unsigned int)caretpos < strlen(consoleinput))
                {
/*
                    if (modstate & KMOD_SHIFT)
                    {
                        ++caretpos;
                        caretwait = I_GetTimeMS() + caretblinktime;
                        showcaret = true;
                        if (selectend >= caretpos)
                            selectstart = caretpos;
                        else
                            selectend = caretpos;
                    }
                    else
*/
                    {
                        if (selectstart < selectend)
                            caretpos = selectstart = selectend;
                        else
                            selectstart = selectend = ++caretpos;
                        caretwait = I_GetTimeMS() + caretblinktime;
                        showcaret = true;
                    }
                }
                else //if (!(modstate & KMOD_SHIFT))
                    caretpos = selectend = selectstart = strlen(consoleinput);
                break;

            // move caret to start
            case KEY_HOME:
                if ((outputhistory != -1 || !caretpos) && outputhistory
                    && consolestrings > CONSOLELINES)
                    outputhistory = 0;
                else if (caretpos > 0)
                {
                    caretpos = selectstart = selectend = 0;
                    caretwait = I_GetTimeMS() + caretblinktime;
                    showcaret = true;
                }
                break;

            // move caret to end
            case KEY_END:
                if (outputhistory != -1 && consolestrings > CONSOLELINES)
                    outputhistory = -1;
                else if ((unsigned int)caretpos < strlen(consoleinput))
                {
                    caretpos = selectstart = selectend = strlen(consoleinput);
                    caretwait = I_GetTimeMS() + caretblinktime;
                    showcaret = true;
                }
                break;

            // autocomplete
            case KEY_TAB:
                if (*consoleinput)
                {
                    int direction = /*((modstate & KMOD_SHIFT) ? -1 : 1)*/1;
                    int start = autocomplete;

                    if (autocomplete == -1)
                        M_StringCopy(autocompletetext, consoleinput, sizeof(autocompletetext));

                    while ((direction == -1 && autocomplete > 0)
                        || (direction == 1 && autocomplete < numconsolecmds - 1))
                    {
                        autocomplete += direction;
                        if (M_StringStartsWith(consolecmds[autocomplete].name, autocompletetext)
                            && consolecmds[autocomplete].type != CT_CHEAT
                            && *consolecmds[autocomplete].description)
                        {
                            M_StringCopy(consoleinput, consolecmds[autocomplete].name,
                                sizeof(consoleinput));
                            if (consolecmds[autocomplete].parameters)
                            {
                                int     length = strlen(consoleinput);

                                consoleinput[length] = ' ';
                                consoleinput[length + 1] = '\0';
                            }
                            caretpos = selectstart = selectend = strlen(consoleinput);
                            caretwait = I_GetTimeMS() + caretblinktime;
                            showcaret = true;
                            return true;
                        }
                    }
                    autocomplete = start;
                }
                break;

            // previous input
            case KEY_UPARROW:
                if (inputhistory == -1)
                    M_StringCopy(currentinput, consoleinput, sizeof(currentinput));
                for (i = (inputhistory == -1 ? consolestrings : inputhistory) - 1; i >= 0; --i)
                    if (console[i].type == graystring
                        && !M_StringCompare(consoleinput, console[i].string))
                    {
                        inputhistory = i;
                        M_StringCopy(consoleinput, console[i].string, 255);
                        caretpos = selectstart = selectend = strlen(consoleinput);
                        caretwait = I_GetTimeMS() + caretblinktime;
                        showcaret = true;
                        break;
                    }
                break;

            // next input
            case KEY_DOWNARROW:
                if (inputhistory != -1)
                {
                    for (i = inputhistory + 1; i < consolestrings; ++i)
                        if (console[i].type == graystring
                            && !M_StringCompare(consoleinput, console[i].string))
                        {
                            inputhistory = i;
                            M_StringCopy(consoleinput, console[i].string, 255);
                            break;
                        }
                    if (i == consolestrings)
                    {
                        inputhistory = -1;
                        M_StringCopy(consoleinput, currentinput, sizeof(consoleinput));
                    }
                    caretpos = selectstart = selectend = strlen(consoleinput);
                    caretwait = I_GetTimeMS() + caretblinktime;
                    showcaret = true;
                }
                break;

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
                break;

            default:
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

                        M_snprintf(buffer, sizeof(buffer), "%s%s%s", M_SubString(consoleinput, 0,
                            selectstart), SDL_GetClipboardText(), M_SubString(consoleinput,
                            selectend, strlen(consoleinput) - selectend));
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
*/
                {
/*
                    if ((modstate & KMOD_SHIFT)
                        || (key_alwaysrun != KEY_CAPSLOCK && (modstate & KMOD_CAPS)))
                        ch = shiftxform[ch];
*/
                    if (ch >= ' ' && ch < '~' && ch != '`'
                        && C_TextWidth(consoleinput) + (ch == ' ' ? spacewidth :
                        consolefont[ch - CONSOLEFONTSTART]->width) <= CONSOLEINPUTPIXELWIDTH
                        /*&& !(modstate & KMOD_ALT)*/)
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
                            if (strlen(consoleinput) < 255)
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
        }
    }
    else if (ev->type == ev_keyup)
        return false;
/*
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
                if (++outputhistory + 10 == consolestrings)
                    outputhistory = -1;
            }
        }
    }
*/
#endif

    return true;
}

static int dayofweek(int d, int m, int y)
{
    int adjustment = (14 - m) / 12;

    m += 12 * adjustment - 2;
    y -= adjustment;

    return ((d + (13 * m - 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7);
}

void C_PrintCompileDate(void)
{
    int                 d, mn, y, h, mnt;

    static const char   *dys[] =
    {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };

    static const char   mths[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    static const char   *mnths[] =
    {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };

    static char         mth[4] = "";

    sscanf(__DATE__, "%3s %2d %4d", mth, &d, &y);
    sscanf(__TIME__, "%2d:%2d:%*d", &h, &mnt);
    mn = (strstr(mths, mth) - mths) / 3;

    C_Output("This %i-bit %s binary of ~Wii-DOOM~ was built on %s, %s %i, "
        "%i at %i:%02i%s", (sizeof(intptr_t) == 4 ? 32 : 64),
#ifdef WII
    "Wii",
#else
     "Linux",
#endif
        dys[dayofweek(d, mn + 1, y)], mnths[mn], d, y,
        (h > 12 ? h - 12 : h), mnt, (h < 12 ? "am" : "pm"));
}

void C_PrintSDLVersions(void)
{
    int revision = 0;

#ifdef SDL2
    revision = SDL_GetRevisionNumber();

    C_Warning("Using version %i.%i.%i of %s",
        SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL,
        "libSDL2_mixer.a"
        );
#else
    C_Warning("Using version %i.%i.%i of %s",
        SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL,
        "libSDL_mixer.a"
        );
#endif

    if (revision)
        C_Warning("Using version %i.%i.%i (Revision %i) of %s",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL, revision,
            "libSDL2.a"
            );
    else
        C_Warning("Using version %i.%i.%i of %s",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
            "libSDL.a"
            );

#ifdef WII
    C_Warning("Using version %i.%i.%i of %s",
        SMPEG_MAJOR_VERSION, SMPEG_MINOR_VERSION, SMPEG_PATCHLEVEL,
        "libsmpeg.a"
        );

    C_Warning("Using version %i.%i.%i of %s",
        _V_MAJOR_, _V_MINOR_, _V_PATCH_,
        "libogc.a"
        );

    C_Warning("Using version %i.%i.%i of %s",
        PNG_LIBPNG_VER_MAJOR, PNG_LIBPNG_VER_MINOR, PNG_LIBPNG_VER_RELEASE,
        "libpng15.a"
        );

    C_Warning("Using version %i (6b) of %s",
        JPEG_LIB_VERSION,
        "libjpeg.a"
        );

    C_Warning("Using version %i.%i.%i.%i of %s",
        ZLIB_VER_MAJOR, ZLIB_VER_MINOR, ZLIB_VER_REVISION, ZLIB_VER_SUBREVISION,
        "libz.a"
        );

    C_Warning("Also using the following libraries:");
    C_Warning("libvorbisidec.a libfat.a, libwiiuse.a, libbte.a, libwiikeyboard.a, libstdc++.a, libm.a");
#endif
}

void C_ConDump(void)
{
    if (consolestrings)
    {
        char    *filename;

        FILE    *file;

#ifdef WII
        if (usb)
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
                            else if (ch != '~')
                            {
                                fputc(ch, file);
                                ++outpos;
                            }
                        }
                    }

                    if (*console[i].timestamp)
                    {
                        for (spaces = 0; spaces < 91 - outpos; ++spaces)
                            fputc(' ', file);

                        fputs(console[i].timestamp, file);
                    }

                    fputc('\n', file);
                }
            }

            fclose(file);

            C_Output("Dumped the console to the file %s.", uppercase(filename));
        }
    }
}

void C_Error(char *string, ...)
{
    va_list     argptr;
    char        buffer[CONSOLETEXTMAXLENGTH] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);

    if (consolestrings && !M_StringCompare(console[consolestrings - 1].string, buffer))
    {
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
        M_StringCopy(console[consolestrings].string, buffer, CONSOLETEXTMAXLENGTH); 
        console[consolestrings].type = redstring;
        memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
        console[consolestrings].timestamp[0] = '\0'; 
        ++consolestrings;
        outputhistory = -1;
        C_DebugOutput(buffer);
    }
}

void C_Network(char *string, ...)
{
    va_list     argptr;
    char        buffer[CONSOLETEXTMAXLENGTH] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);

    if (consolestrings && !M_StringCompare(console[consolestrings - 1].string, buffer))
    {
        console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
        M_StringCopy(console[consolestrings].string, buffer, CONSOLETEXTMAXLENGTH); 
        console[consolestrings].type = bluestring;
        memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
        console[consolestrings].timestamp[0] = '\0'; 
        ++consolestrings;
        outputhistory = -1;
        C_DebugOutput(buffer);
    }
}

void C_TabbedOutput(int tabs[8], char *string, ...)
{
    va_list     argptr;
    char        buffer[CONSOLETEXTMAXLENGTH] = "";

    va_start(argptr, string);
    M_vsnprintf(buffer, CONSOLETEXTMAXLENGTH - 1, string, argptr);
    va_end(argptr);

    console = Z_Realloc(console, (consolestrings + 1) * sizeof(*console));
    M_StringCopy(console[consolestrings].string, buffer, CONSOLETEXTMAXLENGTH);
    console[consolestrings].type = graystring;
    memcpy(console[consolestrings].tabs, tabs, sizeof(console[consolestrings].tabs));
    console[consolestrings].timestamp[0] = '\0';
    ++consolestrings;
    outputhistory = -1;
    C_DebugOutput(buffer);
}

