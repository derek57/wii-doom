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

#include <ctype.h>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <mpeg/smpeg.h>
#include <jpeglib.h>
#include <ogc/libversion.h>
#include <png.h>
#include "c_io.h"
#include "deh_str.h"
#include "d_event.h"
#include "doomstat.h"
#include "g_game.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_tinttab.h"
#include "i_video.h"
#include "m_menu.h"
#include "m_misc.h"
#include "p_local.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <zlib.h>
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include <wiiuse/wpad.h>

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

#define SPACEWIDTH              3
#define DIVIDER                 "~~~"
#define ITALICS                 '~'

#define CARETWAIT               10

boolean         consoleactive = false;
int             consoleheight = 0;
int             consoledirection = -1;
static int      consolewait = 0;

patch_t         *unknownchar;
patch_t         *consolefont[CONSOLEFONTSIZE];
patch_t         *lsquote;
patch_t         *ldquote;
patch_t         *degree;
patch_t         *multiply;
patch_t     *lb;
patch_t     *mid;
patch_t     *rb;

char            consoleinput[255] = "";
int             consolestrings = 0;

patch_t         *caret;
patch_t         *route;
patch_t         *space;
int             caretpos = 0;
static boolean  showcaret = true;
static int      caretwait = 0;
int             selectstart = 0;
int             selectend = 0;

char            consolecheat[255] = "";
char            consolecheatparm[3] = "";
static int      outputhistory = -1;

static int      notabs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

extern boolean  translucency;
extern byte     *tinttab75;
extern int      fps;
boolean         alwaysrun;

void G_ToggleAlwaysRun(void);

char *upper =
{
    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0 !\"#$%&\"()*+,_>?)!@#$%^&*(:"
    ":<+>?\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0{\\}^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ"
};

byte            *c_tempscreen;
byte            *c_blurredscreen;

byte            inputcolor = 4;
byte            whitecolor = 80;
byte            bluecolor = 200;
byte            redcolor = 40;
byte            graycolor = 100;
byte            greencolor = 120;
byte            yellowcolor = 160;

byte            consolebrandingcolor = 100;
byte            consolescrollbartrackcolor = 100;
byte            consolescrollbarfacecolor = 88;
byte            consoletintcolor = 5;

byte            consolecolors[STRINGTYPES];


void C_Printf(stringtype_t type, char *string, ...)
{
    va_list     argptr;
    char        buffer[1024];

    va_start(argptr, string);
    memset(buffer, 0, sizeof(buffer));
    M_vsnprintf(buffer, sizeof(buffer) - 1, string, argptr);
    va_end(argptr);

    console = realloc(console, (consolestrings + 1) * sizeof(*console));
    console[consolestrings].string = strdup(buffer);
    console[consolestrings].type = type;
    memset(console[consolestrings].tabs, 0, sizeof(console[consolestrings].tabs));
    ++consolestrings;
    outputhistory = -1;
}

void C_AddConsoleDivider(void)
{
    if (!consolestrings || strcasecmp(console[consolestrings - 1].string, DIVIDER))
        C_Printf(CR_RED, " {||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||}\n");
}
/*
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
*/
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
                screens[0][y - offset + x] = consolescrollbartrackcolor;

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

    unknownchar = W_CacheLumpName("DRFON000", PU_STATIC);
    for (i = 0; i < CONSOLEFONTSIZE; i++)
    {
        M_snprintf(buffer, 9, "DRFON%03d", j++);
        consolefont[i] = W_CacheLumpName(buffer, PU_STATIC);
    }

    lb = W_CacheLumpName("STCFN123", PU_STATIC);
    mid = W_CacheLumpName("STCFN124", PU_STATIC);
    rb = W_CacheLumpName("STCFN125", PU_STATIC);
    lsquote = W_CacheLumpName("DRFON145", PU_STATIC);
    ldquote = W_CacheLumpName("DRFON147", PU_STATIC);
    degree = W_CacheLumpName("DRFON176", PU_STATIC);
    multiply = W_CacheLumpName("DRFON215", PU_STATIC);

    space = consolefont[' ' - CONSOLEFONTSTART];
    route = consolefont['$' - CONSOLEFONTSTART];
    caret = consolefont['_' - CONSOLEFONTSTART];

    consolecolors[yellow] = yellowcolor; // yellow = 160
    consolecolors[red] = redcolor;       // red = 40
    consolecolors[gray] = graycolor;     // gray = 100
    consolecolors[blue] = bluecolor;     // blue = 200
    consolecolors[white] = whitecolor;   // white = 80
    consolecolors[green] = greencolor;   // green = 120

    c_tempscreen = Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    c_blurredscreen = Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
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

static void c_blurscreen(int x1, int y1, int x2, int y2, int i)
{
    int x, y;

    memcpy(c_tempscreen, c_blurredscreen, SCREENWIDTH * (CONSOLEHEIGHT + 5));

    for (y = y1; y < y2; y += SCREENWIDTH)
        for (x = y + x1; x < y + x2; ++x)
            c_blurredscreen[x] = tinttab50[c_tempscreen[x] + (c_tempscreen[x + i] << 8)];
}

static void C_DrawBackground(int height)
{
//    static boolean      blurred = false;
    int                 i;

//    extern boolean      wipe;

    height = (height + 5) * SCREENWIDTH;

//    if (!blurred)
    {
        for (i = 0; i < height; ++i)
            c_blurredscreen[i] = screens[0][i];

        c_blurscreen(0, 0, SCREENWIDTH - 1, height, 1);
        c_blurscreen(1, 0, SCREENWIDTH, height, -1);
        c_blurscreen(0, 0, SCREENWIDTH - 1, height - SCREENWIDTH, SCREENWIDTH + 1);
        c_blurscreen(1, SCREENWIDTH, SCREENWIDTH, height, -(SCREENWIDTH + 1));
        c_blurscreen(0, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH);
        c_blurscreen(0, SCREENWIDTH, SCREENWIDTH, height, -SCREENWIDTH);
        c_blurscreen(1, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH - 1);
        c_blurscreen(0, SCREENWIDTH, SCREENWIDTH - 1, height, -(SCREENWIDTH - 1));
    }

//    blurred = (consoleheight == CONSOLEHEIGHT && !wipe);

    for (i = 0; i < height; ++i)
        screens[0][i] = tinttab50[c_blurredscreen[i] + (consoletintcolor << 8)];

    for (i = height - SCREENWIDTH * 3; i < height - SCREENWIDTH * 2; ++i)
        screens[0][i] = tinttab25[((consolebrandingcolor + 5) << 8) + screens[0][i]];

    for (i = height - SCREENWIDTH * 2; i < height; ++i)
        screens[0][i] = tinttab25[(consolebrandingcolor << 8) + screens[0][i]];
}

static struct
{
    char        char1;
    char        char2;
    int         adjust;
} kern[] = {
    { '\"', '+',  -1 }, { '\"', '.',  -1 }, { '\"', 'a',  -1 }, { '\"', 'c',  -1 },
    { '\"', 'd',  -1 }, { '\"', 'e',  -1 }, { '\"', 'g',  -1 }, { '\"', 'j',  -2 },
    { '\"', 'o',  -1 }, { '\"', 'q',  -1 }, { '\"', 's',  -1 }, { '\'', 'a',  -1 },
    { '\'', 'c',  -1 }, { '\'', 'd',  -1 }, { '\'', 'e',  -1 }, { '\'', 'g',  -1 },
    { '\'', 'j',  -2 }, { '\'', 'o',  -1 }, { '\"', 'q',  -1 }, { '\'', 's',  -1 },
    { '.',  '\\', -1 }, { '.',  '7',  -1 }, { '/',  'o',  -1 }, { ':', '\\',  -1 },
    { '_',  'f',  -1 }, { '0',  ',',  -1 }, { '0',  'j',  -2 }, { '1',  '\"', -1 },
    { '1',  '\'', -1 }, { '1',  'j',  -2 }, { '2',  'j',  -2 }, { '3',  ',',  -1 },
    { '3',  'j',  -2 }, { '4',  'j',  -2 }, { '5',  ',',  -1 }, { '5',  'j',  -2 },
    { '6',  ',',  -1 }, { '6',  'j',  -2 }, { '7',  ',',  -2 }, { '7',  'j',  -2 },
    { '8',  ',',  -1 }, { '8',  'j',  -2 }, { '9',  ',',  -1 }, { '9',  'j',  -2 },
    { 'F',  '.',  -1 }, { 'F',  ',',  -1 }, { 'L',  '\\', -1 }, { 'L',  '\"', -1 },
    { 'L',  '\'', -1 }, { 'P',  '.',  -1 }, { 'P',  ',',  -1 }, { 'T',  '.',  -1 },
    { 'T',  ',',  -1 }, { 'V',  '.',  -1 }, { 'V',  ',',  -1 }, { 'Y',  '.',  -1 },
    { 'Y',  ',',  -1 }, { 'a',  '\"', -1 }, { 'a',  '\'', -1 }, { 'a',  'j',  -2 },
    { 'b',  ',',  -1 }, { 'b',  '\"', -1 }, { 'b',  '\\', -1 }, { 'b',  '\'', -1 },
    { 'b',  'j',  -2 }, { 'c',  '\\', -1 }, { 'c',  ',',  -1 }, { 'c',  '\"', -1 },
    { 'c',  '\'', -1 }, { 'c',  'j',  -2 }, { 'd',  'j',  -2 }, { 'e',  '\\', -1 },
    { 'e',  ',',  -1 }, { 'e',  '\"', -1 }, { 'e',  '\'', -1 }, { 'e',  '_',  -1 },
    { 'e',  'j',  -2 }, { 'f',  ',',  -2 }, { 'f',  '_',  -1 }, { 'f',  'j',  -2 },
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
    char        prevletter = '\0';
    int         w = 0;

    for (i = 0; i < strlen(text); ++i)
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

static void C_DrawConsoleText(int x, int y, char *text, byte color, int translucency, int tabs[8],
    boolean inverted)
{
    boolean     italics = false;
    size_t      i;
    int         tab = -1;
    size_t      len = strlen(text);
    char        prevletter = '\0';

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
        char    letter = text[i];
        int     c = letter - CONSOLEFONTSTART;
        char    nextletter = text[i + 1];

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
                x = (x > tabs[++tab] ? x + SPACEWIDTH : tabs[tab]);
            else if(letter == '{')
                patch = lb;
            else if(letter == '|')
                patch = mid;
            else if(letter == '}')
                patch = rb;
            else if (letter == '\xc2' && nextletter == '\xb0')
            {
                patch = degree;
                ++i;
            }
            else if (letter != '\n')
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
                V_DrawConsoleChar(x, y - (CONSOLEHEIGHT - consoleheight), patch, color, italics,
                    translucency, inverted);
                x += SHORT(patch->width);
            }
        }
        prevletter = letter;
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
        C_DrawConsoleText(SCREENWIDTH - C_TextWidth("Wii-DOOM") - CONSOLETEXTX - 1,
            CONSOLEHEIGHT - 10, "Wii-DOOM", graycolor, 1, notabs, false);

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
/*
            if (console[i].type == red)
                C_DrawDivider(y + 5 - (CONSOLEHEIGHT - consoleheight));
            else
*/
                C_DrawConsoleText(CONSOLETEXTX, y + (CONSOLELINEHEIGHT / 2), console[i].string,
                    consolecolors[console[i].type], 0, console[i].tabs, false);
        }

        // draw caret
        if (caretwait < I_GetTime())
        {
            showcaret = !showcaret;
            caretwait = I_GetTime() + CARETWAIT;
        }

        V_DrawConsoleChar(x, consoleheight - 10, space, inputcolor,
                false, 0, false);
        V_DrawConsoleChar(x + SHORT(space->width), consoleheight - 10, route,
                inputcolor, false, 0, false);

        if (showcaret)
            V_DrawConsoleChar(x + SHORT(route->width) + 4, consoleheight - 10,
                    caret, inputcolor, false, 0, false);
        x += 3;

        // draw the scrollbar
        C_DrawScrollbar();
    }

    else
        consoleactive = false;
}

boolean C_Responder(event_t *ev)
{
    if (consoleheight < CONSOLEHEIGHT && consoledirection == -1)
        return false;

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
    return true;
}

static int dayofweek(int day, int month, int year)
{
    int adjustment = (14 - month) / 12;
    int m = month + 12 * adjustment - 2;
    int y = year - adjustment;

    return (day + (13 * m - 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7;
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
    static char         mth[4];

    sscanf(__DATE__, "%3s %2d %4d", mth, &day, &year);
    sscanf(__TIME__, "%2d:%2d:%*d", &hour, &minute);
    month = (strstr(mths, mth) - mths) / 3;

    C_Printf(CR_GRAY, " This %i-bit %s binary of %s was built on %s, %s %i, %i at %i:%02i%s\n",
        (sizeof(intptr_t) == 4 ? 32 : 64),
        "Linux",
        "Wii-DOOM", days[dayofweek(day, month + 1, year)], months[month], day,
        year, (hour > 12 ? hour - 12 : hour), minute, (hour < 12 ? "am" : "pm"));
}


void C_PrintSDLVersions(void)
{
    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
        "libSDL.a"
        );

    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_PATCHLEVEL,
        "libSDL_mixer.a"
        );

    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        SMPEG_MAJOR_VERSION, SMPEG_MINOR_VERSION, SMPEG_PATCHLEVEL,
        "libsmpeg.a"
        );

    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        _V_MAJOR_, _V_MINOR_, _V_PATCH_,
        "libogc.a"
        );

    C_Printf(CR_GOLD, " Also using the following libs:\n");
    C_Printf(CR_GOLD, " libvorbisidec.a libwiilight.a, libfat.a, libwiiuse.a, libbte.a, libwiikeyboard.a\n");
}

