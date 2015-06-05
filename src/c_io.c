/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 * Console I/O
 *
 * Basic routines: outputting text to the console, main console functions:
 *                 drawer, responder, ticker, init
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */


#include <ft2build.h>
#include <freetype/freetype.h>
#include <mpeg/smpeg.h>
#include <jpeglib.h>
#include <ogc/libversion.h>
#include <png.h>
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include "c_io.h"
#include "d_event.h"
#include "d_main.h"
#include "deh_str.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "i_video.h"
#include "i_swap.h"
#include "m_menu.h"
#include "m_misc.h"
#include "s_sound.h"
#include "sounds.h"
#include "v_misc.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include <wiiuse/wpad.h>


#define CONSOLEFONTSTART                ' '
#define CONSOLEFONTEND                  '~'
#define CONSOLEFONTSIZE                 (CONSOLEFONTEND - CONSOLEFONTSTART + 1)
#define CONSOLETEXTX                    10

#define ITALICS                         '~'

#define MESSAGES                        384

#define MAX_MYCHARSPERLINE              100

// keep the last 32 typed commands
#define HISTORY                         32

#define PACKAGE_NAMEANDVERSIONSTRING    "Wii-DOOM"


static char        *inputprompt = " $" ;

// left-most point you see of the command line
static char        *input_point;

static byte        *backdrop;

// the messages (what you see in the console window)
static char        messages[MESSAGES][LINELENGTH];

static char        inputtext[INPUTLENGTH];

// position in the history (last line in window)
static int         message_pos = 0;

// the last message
static int         message_last = 0;

static int         backdrop_lumpnum = -1;

// for scrolling command line
static int         pgup_down = 0;
static int         pgdn_down = 0;

static patch_t     *lb;
static patch_t     *mid;
static patch_t     *rb;
static patch_t     *lsquote;
static patch_t     *ldquote;
static patch_t     *degree;
static patch_t     *multiply;
static patch_t     *unknownchar;
static patch_t     *c_font[CONSOLEFONTSIZE];

char               *lumpname;

// the height of the console
int                c_height = 100;        // 50 FOR STRIFE (100 FOR DOOM)

// pixels/tic it moves
int                c_speed = 10;

int                current_target = 0;
int                current_height = 0;
int                console_enabled = true;
int                textcolor;
int                consolestrings = 0;

boolean            c_showprompt;

byte               redcol = 40;
byte               whitecol = 80;
byte               graycol = 100;
byte               greencol = 120;
byte               yellowcol = 160;
byte               bluecol = 200;
byte               consolecolors[STRINGTYPES];

extern boolean     redrawsbar;

extern char        *shiftxform;


typedef struct
{
    char                *string;
    crx_t               type;
    int                 tabs[8];
} console_t;

console_t       *console;


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
        PACKAGE_NAMEANDVERSIONSTRING, days[dayofweek(day, month + 1, year)], months[month], day,
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
/*
    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        SDL_TTF_MAJOR_VERSION, SDL_TTF_MINOR_VERSION, SDL_TTF_PATCHLEVEL,
        "libSDL_ttf.a"
        );

    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        SDL_GFXPRIMITIVES_MAJOR, SDL_GFXPRIMITIVES_MINOR, SDL_GFXPRIMITIVES_MICRO,
        "libSDL_gfx.a"
        );

    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        SDL_IMAGE_MAJOR_VERSION, SDL_IMAGE_MINOR_VERSION, SDL_IMAGE_PATCHLEVEL,
        "libSDL_image.a"
        );
*/
    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        SMPEG_MAJOR_VERSION, SMPEG_MINOR_VERSION, SMPEG_PATCHLEVEL,
        "libsmpeg.a"
        );
/*
    C_Printf(CR_GOLD, " Using version %i.%i of %s\n",
        JPEG_LIB_VERSION_MAJOR, JPEG_LIB_VERSION_MINOR,
        "libjpeg.a"
        );

    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        PNG_LIBPNG_VER_MAJOR, PNG_LIBPNG_VER_MINOR, PNG_LIBPNG_VER_RELEASE,
        "libpng15.a"
        );

    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
       FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH,
        "libfreetype.a"
        );

    C_Printf(CR_GOLD, " Using version %i.%i.%i.%i of %s\n",
        ZLIB_VER_MAJOR, ZLIB_VER_MINOR, ZLIB_VER_REVISION, ZLIB_VER_SUBREVISION,
        "libz.a"
        );
*/
    C_Printf(CR_GOLD, " Using version %i.%i.%i of %s\n",
        _V_MAJOR_, _V_MINOR_, _V_PATCH_,
        "libogc.a"
        );

    C_Printf(CR_GOLD, " Also using the following libs:\n");
    C_Printf(CR_GOLD, " libvorbisidec.a libwiilight.a, libfat.a, libwiiuse.a, libbte.a, libwiikeyboard.a\n");
}

//
// Main Console functions
//
// ticker, responder, drawer, init etc.
//
void C_InitBackdrop(void)
{  
    patch_t *patch;

    switch(gamemode)
    {
    case retail:
    case registered:
        lumpname = "PFUB1";
        break;

    case shareware:
        lumpname = "WIMAP0";
        break;

    case commercial:
        lumpname = "INTERPIC";
        break;

    default:
        lumpname = "TITLEPIC";
        break;
    }

    // allow for custom console background graphic
    if(W_CheckNumForName("CONSOLE") >= 0)
        lumpname = "CONSOLE";

    backdrop_lumpnum = W_GetNumForName(lumpname);

    if(backdrop)
        Z_Free(backdrop);

    backdrop = Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, 0);

    // draw the console background image to the newly allocated video buffer
    patch = W_CacheLumpName(lumpname, PU_CACHE);

    // fill the video buffer with the newly allocated screen
    V_UseBuffer(backdrop);

    dp_translation = crx[CRX_DARK];
    V_DrawPatch(0, 0, patch);
    V_ClearDPTranslation();

    // restore the backup up video buffer
    V_RestoreBuffer();
}

//
// called every tic
//
void C_Ticker(void)
{
    c_showprompt = true;
  
    if(gamestate != GS_CONSOLE)
    {
        // specific to half-screen version only
        if(current_height != current_target)
            redrawsbar = true;

        // move the console toward its target
        if(abs(current_height - current_target) >= c_speed)
            current_height += current_target < current_height ? -c_speed : c_speed;
        else
            current_height = current_target;
    }
    else
    {
        // console gamestate: no moving consoles!
        current_target = current_height;
    }

    // no scrolling thru messages when fullscreen
    // scroll based on keys down
    message_pos += pgdn_down - pgup_down;
      
    // check we're in the area of valid messages        
    if(message_pos < 0)
        message_pos = 0;

    if(message_pos > message_last)
        message_pos = message_last;
}

//
// respond to keyboard input/events
//
void C_DrawBackdrop()
{
    // re-init to the new screen size
    C_InitBackdrop();

    memcpy(I_VideoBuffer, backdrop +
          (800 - (current_height * 4)) *
           320, ((current_height * 4)) * 320);
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

int C_StringWidth(char* string)
{
    size_t          i;
    int             w = 0;
    int             c;
        
    for (i = 0;i < strlen(string);i++)
    {
        c = toupper(string[i]) - CONSOLEFONTSTART;
        if (c < 0 || c >= CONSOLEFONTSIZE)
            w += 4;
        else
            w += SHORT (c_font[c]->width);
    }

    return w;
}

//
// input_point is the leftmost point of the inputtext which
// we see. This function is called every time the inputtext
// changes to decide where input_point should be.
//
static void C_UpdateInputPoint(void)
{
    for(input_point = inputtext; C_StringWidth(input_point) > 320 - 20; input_point++);
}


int C_Responder(event_t* ev)
{
    static int shiftdown;
    char ch;
    boolean didsound = false;

    WPADData *data = WPAD_Data(0);

    //Classic Controls
    if(data->exp.type == WPAD_EXP_CLASSIC)
    {
        if(data->btns_d & WPAD_CLASSIC_BUTTON_UP)
        {
            pgup_down = 1;
            pgdn_down = 0;

            return consoleactive;
        }
        else if(data->btns_d & WPAD_CLASSIC_BUTTON_DOWN)
        {
            pgup_down = 0;
            pgdn_down = 1;

            return consoleactive;
        }
        else if(data->btns_d & WPAD_CLASSIC_BUTTON_LEFT   ||
                data->btns_d & WPAD_CLASSIC_BUTTON_B      ||
                data->btns_d & WPAD_CLASSIC_BUTTON_MINUS  ||
                data->btns_d & WPAD_CLASSIC_BUTTON_HOME   ||
                data->btns_d & WPAD_CLASSIC_BUTTON_PLUS   ||
                data->btns_d & WPAD_CLASSIC_BUTTON_A      ||
                data->btns_d & WPAD_CLASSIC_BUTTON_RIGHT  ||
                data->btns_d & WPAD_CLASSIC_BUTTON_X      ||
                data->btns_d & WPAD_CLASSIC_BUTTON_FULL_L ||
                data->btns_d & WPAD_CLASSIC_BUTTON_Y      ||
                data->btns_d & WPAD_CLASSIC_BUTTON_FULL_R ||
                data->btns_d & WPAD_CLASSIC_BUTTON_ZL     ||
                data->btns_d & WPAD_CLASSIC_BUTTON_ZR)
            return consoleactive;

        if(data->btns_u)
        {
            pgdn_down = 0;
            pgup_down = 0;
        }
    }  

    // only interested in keypresses
    if(!data->btns_d)
        return false;

    // Check for special keypresses and
    // detect activation of console etc.  
    if(consoleactive && console_enabled)
    {
        if(current_target > 0 && !didsound)
        {
            didsound = true;

            S_StartSound(NULL, sfx_dorcls);
        }

        // set console
        current_target = current_target == c_height ? 0 : c_height;

        return consoleactive;
    }

    if(!consoleactive)
        return false;

    // not til its stopped moving
    if(current_target < current_height)
        return false;

    // Normal Text Input:
    // probably just a normal character
    // (shifted)?
    ch = shiftdown ? shiftxform[ev->data1] : ev->data1;

    // only care about valid characters
    // dont allow too many characters on one command line
    // sf 19/6 V_IsPrint  
    if(V_IsPrint(ch) && strlen(inputtext) < INPUTLENGTH - 3)
    {
        sprintf(inputtext, "%s%c", inputtext, ch);
      
        // reset scrolling
        C_UpdateInputPoint();

        return true;
    }  

    // dont care about this event
    return false;
}

//
// initialise the console
//
void C_Init(void)
{
    int         i;
    int         j;
    char        buffer[9];

    // load the heads-up font
    j = CONSOLEFONTSTART + 1;
    for (i = 0; i < CONSOLEFONTSIZE; i++)
    {
        if(j < CONSOLEFONTEND + 1)
        {
            DEH_snprintf(buffer, 9, "DRFON%.3d", j++);
            c_font[i] = W_CacheLumpName(buffer, PU_STATIC);
        }
    }

    lb = W_CacheLumpName("STCFN123", PU_STATIC);
    mid = W_CacheLumpName("STCFN124", PU_STATIC);
    rb = W_CacheLumpName("STCFN125", PU_STATIC);
    lsquote = W_CacheLumpName("DRFON145", PU_STATIC);
    ldquote = W_CacheLumpName("DRFON147", PU_STATIC);
    degree = W_CacheLumpName("DRFON176", PU_STATIC);
    multiply = W_CacheLumpName("DRFON215", PU_STATIC);
    unknownchar = W_CacheLumpName("DRFON000", PU_STATIC);

    consolecolors[yellow] = yellowcol;
    consolecolors[red] = redcol;
    consolecolors[gray] = graycol;
    consolecolors[blue] = bluecol;
    consolecolors[white] = whitecol;
    consolecolors[green] = greencol;

    C_UpdateInputPoint();
}

static void C_DrawText(int x, int y, char *text, byte color, int translucency, int italics)
{
    size_t      i;
    size_t      len = strlen(text);

    char        prevletter = '\0';

    while (C_StringWidth(text) > SCREENWIDTH - CONSOLETEXTX * 3 - 5)
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

        patch_t *charpatch = NULL;

        if((letter > 32 && letter < 127)    ||
           (letter == 0    || letter == 145 || letter == 147 ||
            letter == 215) || letter == ' ')
        {
            if(letter == ' ')
                x += 4;
            else if(letter == '\t')
                x += 32;
            else if(letter == '{')
            {
                charpatch = lb;
                V_DrawPatch(x, y, charpatch);
            }
            else if(letter == '|')
            {
                charpatch = mid;
                V_DrawPatch(x, y, charpatch);
            }
            else if(letter == '}')
            {
                charpatch = rb;
                V_DrawPatch(x, y, charpatch);
            }
            else if(letter == '\xc2' && nextletter == '\xb0')
            {
                charpatch = degree;
                ++i;
            }
            else
                charpatch = (c < 0 || c >= CONSOLEFONTSIZE ? unknownchar : c_font[c - 1]);
        }

        if (isdigit(prevletter) && letter == 'x' && isdigit(nextletter))
            charpatch = multiply;
        else if (prevletter == ' ' || prevletter == '\t' || !i)
        {
            if (letter == '\'')
                charpatch = lsquote;
            else if (letter == '\"')
                charpatch = ldquote;
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

        if (charpatch)
        {
            V_DrawConsoleChar(x, y, charpatch, color, italics, translucency);
            x += SHORT(charpatch->width);
        }
        prevletter = letter;
    }
}

//
// draw the console
//
void C_Drawer(void)
{
    int y;
    int count;
  
    // dont draw if not active
    if(!consoleactive)
        return;

    // draw backdrop
    C_DrawBackdrop();

    // draw text messages
    // offset starting point up by 8 if we are showing input prompt
    y = current_height - ((c_showprompt && message_pos == message_last) ? 8 : 0);

    // start at our position in the message history
    count = message_pos;

    while(1)
    {
        // move up one line on the screen
        // back one line in the history
        y -= 8;

        // end of message history?
        if(--count < 0)
            break;

        // past top of screen?
        if(y < 0)
            break;

        // draw this line
        C_DrawText(0, y, console[count].string, consolecolors[console[count].type], 0, 0);
    }

    // draw branding
    C_DrawText(SCREENWIDTH - C_StringWidth(PACKAGE_NAMEANDVERSIONSTRING) -
        CONSOLETEXTX + 1, current_height - 8, PACKAGE_NAMEANDVERSIONSTRING, 100, 1, 1);

    // Draw input line
    // input line on screen, not scrolled back in history?  
    if(current_height > 8 && c_showprompt && message_pos == message_last)
    {
        char tempstr[LINELENGTH];
        char *underline;

        if(leveltime & 16)
            underline = "_";
        else
            underline = " ";
      
        // if we are scrolled back, dont draw the input line
        if(message_pos == message_last)
            sprintf(tempstr, "%s%s%s", inputprompt, input_point, underline);

        C_DrawText(0, current_height - 8, tempstr, 80, 0, 0);
    }
}

//
// updates the screen without actually waiting for d_display
// useful for functions that get input without using the gameloop
// eg. serial code
//
void C_Update(void)
{
    C_Drawer();

    I_FinishUpdate();
}


//
// I/O Functions
//
// scroll console up
static void C_ScrollUp(void)
{
    if(message_last == message_pos)
        message_pos++;

    message_last++;

    // past the end of the string
    if(message_last >= MESSAGES)
    {
        // cut off the oldest 128 messages
        int i;

        // haleyjd 03/02/02: fixed code that assumed MESSAGES == 256
        for(i = 128; i < MESSAGES; i++)
            strcpy(messages[i - 128], messages[i]);

        // move the message boundary
        message_last -= 128;

        // haleyjd 09/04/02: set message_pos to message_last
        // to avoid problems with console flooding
        message_pos = message_last;
    }
    // new line is empty
    messages[message_last][0] = '\0';
}


// 
// C_AddMessage
//
// haleyjd:
// Add a message to the console.
// Replaced C_AddChar.
//
static void C_AddMessage(char *s)
{
    char *c;
    char *end;
    char linecolor = CR_RED + FC_BASEVALUE;

    // haleyjd 09/04/02: set color to default at beginning
    if(C_StringWidth(messages[message_last]) > SCREENWIDTH - 9 ||
       strlen(messages[message_last]) >= LINELENGTH - 1)
    {
        C_ScrollUp();
    }

    end = messages[message_last] + strlen(messages[message_last]);
    *end++ = linecolor;
    *end = '\0';

    for(c = (char *)s; *c; c++)
    {
        // >= 128 for colours
        if(*c == '\t' || (*c > 31 && *c < 127) || *c >= 128)
        {
            if(*c >= 128)
                linecolor = *c;

            if(C_StringWidth(messages[message_last]) > SCREENWIDTH - 9 ||
                strlen(messages[message_last]) >= LINELENGTH - 1)
            {
                // might possibly over-run, go onto next line
                C_ScrollUp();

                end = messages[message_last] + strlen(messages[message_last]);

                // keep current color on next line
                *end++ = linecolor;
                *end = '\0';
            }
         
            end = messages[message_last] + strlen(messages[message_last]);
            *end++ = *c;
            *end = '\0';
        }

        // alert
        if(*c == '\a')
        {
            // 'tink'!
            S_StartSound(NULL, sfx_radio);
        }

        if(*c == '\n')
        {
            C_ScrollUp();

            end = messages[message_last] + strlen(messages[message_last]);

            // keep current color on next line
            *end++ = linecolor;
            *end = '\0';
        }
    }
}

//
// haleyjd: this function attempts to break up formatted strings 
// into segments no more than a gamemode-dependent number of 
// characters long. It'll succeed as long as the string in question 
// doesn't contain that number of consecutive characters without a 
// space, tab, or line-break, so like, don't print stupidness 
// like that. Its a console, not a hex editor...
//
static void C_AdjustLineBreaks(char *str)
{
    int i;
    int count = 0;
    int firstspace = -1;
    int lastspace = 0;
    int len = strlen(str);

    for(i = 0; i < len; ++i)
    {
        if(str[i] == ' ' || str[i] == '\t')
        {
            if(firstspace == -1)
                firstspace = i;

            lastspace = i;
        }

        if(str[i] == '\n')
            count = lastspace = 0;
        else
            count++;

        if(count == MAX_MYCHARSPERLINE)
        {
            // 03/16/01: must add length since last space to new line
            count = i - (lastspace + 1);

            // replace last space with \n
            // if no last space, we're screwed
            if(lastspace)
            {
                if(lastspace == firstspace)
                    firstspace = 0;
                    str[lastspace] = '\n';
                    lastspace = 0;
            }
        }
    }

    if(firstspace)
    {      
        // temporarily put a \0 in the first space
        char temp = str[firstspace];

        str[firstspace] = '\0';

        // if the first segment of the string doesn't fit on the 
        // current line, move the console up one line in advance

        if(C_StringWidth(str) + C_StringWidth(messages[message_last]) > SCREENWIDTH - 9
            || strlen(str) + strlen(messages[message_last]) >= LINELENGTH - 1)
        {
            C_ScrollUp();
        }

        // restore the string to normal
        str[firstspace] = temp;
    }
}

//
// C_Printf -
// write some text 'printf' style to the console
// the main function for I/O
// cph 2001/07/22 - remove arbitrary limit, use malloc'd buffer instead
//  and make format string parameter const char*
//
// These symbols are not being printed: ä ö ü € ² ³ µ \ % § °
// The game might crash if trying to print this symbol: ~
//
void C_Printf(stringtype_t type, char *s, ...)
{
    va_list args;

//    char *t;

    char buffer[1024];
  
    // haleyjd: sanity check
    if(!s)
        return;
  
    if(fgets(buffer, sizeof(buffer), stdin))
        C_Printf(CR_RED, "C_Printf: Buffer overflow");

    // difficult to remove limit
    va_start(args, s);
    memset(buffer, 0, sizeof(buffer));
//    vasprintf(&t, s, args);

    M_vsnprintf(buffer, sizeof(buffer) - 1, s, args);
    buffer[0] = toupper(buffer[0]);

    va_end(args);

    console = realloc(console, (consolestrings + 1) * sizeof(*console));
    console[consolestrings].string = strdup(buffer);
    console[consolestrings].type = type;
    ++consolestrings;

    // haleyjd
    C_AdjustLineBreaks(buffer);

    C_AddMessage(buffer);
  
//    (free)(buffer);
}


//
// Console activation
//
// put smmu into console mode
//
void C_SetConsole(void)
{
    current_target = 100;
  
    C_Update();
}

//
// make the console go up
//
void C_Popup(void)
{
    current_target = 0;
}

//
// make the console disappear
//
void C_InstaPopup(void)
{
    current_target = current_height = 0;
}

void C_Seperator(void)
{
  C_Printf(CR_RED, " {|||||||||||||||||||||||||||||}\n");
}

