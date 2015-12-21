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
//
//-----------------------------------------------------------------------------



#include <fcntl.h>

#ifdef SDL2
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "c_io.h"

#ifdef WII
#include "../wii/config.h"
#else
#include "config.h"
#endif

#include "d_deh.h"

#include "doom/doomdef.h"

#include "doomtype.h"
#include "i_joystick.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_config.h"
#include "m_misc.h"

#include "doom/s_sound.h"

#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"


//#ifdef WII
#define DEFAULT_RAM 32 // MiB
#define MIN_RAM     16 // MiB
/*
#else
#define DEFAULT_RAM 64 // MiB
#define MIN_RAM     32 // MiB
#endif
*/
#define ZENITY_BINARY "/usr/bin/zenity"


typedef struct atexit_listentry_s atexit_listentry_t;

struct atexit_listentry_s
{
    atexit_func_t func;
    dboolean run_on_error;
    atexit_listentry_t *next;
};


byte *zone_mem;

//extern dboolean devparm;

int memory_size;

static dboolean already_quitting;

static atexit_listentry_t *exit_funcs = NULL;

void I_AtExit(atexit_func_t func, dboolean run_on_error)
{
    atexit_listentry_t *entry;

    entry = malloc(sizeof(*entry));

    entry->func = func;
    entry->run_on_error = run_on_error;
    entry->next = exit_funcs;
    exit_funcs = entry;
}

// Tactile feedback function, probably used for the Logitech Cyberman
//
// [nitr8] UNUSED
//
/*
void I_Tactile(int on, int off, int total)
{
}
*/

// Zone memory auto-allocation function that allocates the zone size
// by trying progressively smaller zone sizes until one is found that
// works.

static byte *AutoAllocMemory(int *size, int default_ram, int min_ram)
{
    byte *zonemem;

    // Allocate the zone memory.  This loop tries progressively smaller
    // zone sizes until a size is found that can be allocated.
    // If we used the -mb command line parameter, only the parameter
    // provided is accepted.

    zonemem = NULL;

    while (zonemem == NULL)
    {
        // We need a reasonable minimum amount of RAM to start.

        if (default_ram < min_ram)
        {
            I_Error("Unable to allocate %i MiB of RAM for zone", default_ram);
        }

        // Try to allocate the zone memory.

        *size = default_ram * 1024 * 1024;

        zonemem = malloc(*size);

//        printf("0x%x allocated for zone\n", default_ram);

        // Failed to allocate?  Reduce zone size until we reach a size
        // that is acceptable.

        if (zonemem == NULL)
        {
            default_ram -= 1;
        }
    }

    return zonemem;
}

byte *I_ZoneBase (int *size)
{
    byte *zonemem;
    int min_ram, default_ram;
    static int i = 1;

#ifndef WII
    int p = 0;

    //!
    // @arg <mb>
    //
    // Specify the heap size, in MiB (default 16).
    //

    if(!beta_style)
        p = M_CheckParmWithArgs("-mb", 1);

    if (p > 0)
    {
        default_ram = atoi(myargv[p+1]);
        min_ram = default_ram;
    }
    else
#endif
    {
        default_ram = DEFAULT_RAM;
        min_ram = MIN_RAM;
    }

    zonemem = AutoAllocMemory(size, default_ram * i, min_ram * i);

    // [crispy] if called again, allocate another zone twice as big
    i *= 2;

    memory_size = *size;

    return zonemem;
}

//
// [nitr8] UNUSED
//
/*
void I_PrintBanner(char *msg)
{
    int i;
    int spaces = 35 - (strlen(msg) / 2);

    for (i=0; i<spaces; ++i)
        putchar(' ');

    puts(msg);
}

void I_PrintDivider(void)
{
    int i;

    for (i=0; i<75; ++i)
    {
        putchar('=');
    }

    putchar('\n');
}

void I_PrintStartupBanner(char *gamedescription)
{
    I_PrintDivider();
    I_PrintBanner(gamedescription);
    I_PrintDivider();
    
    C_Output(
    " Wii-DOOM is free software, covered by the GNU General Public"
    " License.  There is NO warranty; not even for MERCHANTABILITY or FITNESS"
    " FOR A PARTICULAR PURPOSE. You are welcome to change and distribute"
    " copies under certain conditions. See the source for more information.");

    I_PrintDivider();
}
*/

// 
// I_ConsoleStdout
//
// Returns true if stdout is a real console, false if it is a file
//

dboolean I_ConsoleStdout(void)
{
    return isatty(fileno(stdout));
}

//
// I_Quit
//

void I_Quit (void)
{
    atexit_listentry_t *entry;

//    if(devparm)
        fclose (debugfile);
//    fclose (statsfile);

    C_ConDump();

    // Run through all exit functions
 
    entry = exit_funcs; 

    while (entry != NULL)
    {
        entry->func();
        entry = entry->next;
    }

    SDL_Quit();

    exit(0);
}

void I_QuitSerialFail (void)
{
    exit(0);
}

// returns non-zero if zenity is available

#ifndef WII
static int ZenityAvailable(void)
{
    return system(ZENITY_BINARY " --help >/dev/null 2>&1") == 0;
}

// Escape special characters in the given string so that they can be
// safely enclosed in shell quotes.

static char *EscapeShellString(char *string)
{
    char *result;
    char *r, *s;

    // In the worst case, every character might be escaped.
    result = malloc(strlen(string) * 2 + 3);
    r = result;

    // Enclosing quotes.
    *r = '"';
    ++r;

    for (s = string; *s != '\0'; ++s)
    {
        // From the bash manual:
        //
        //  "Enclosing characters in double quotes preserves the literal
        //   value of all characters within the quotes, with the exception
        //   of $, `, \, and, when history expansion is enabled, !."
        //
        // Therefore, escape these characters by prefixing with a backslash.

        if (strchr("$`\\!", *s) != NULL)
        {
            *r = '\\';
            ++r;
        }

        *r = *s;
        ++r;
    }

    // Enclosing quotes.
    *r = '"';
    ++r;
    *r = '\0';

    return result;
}

// Open a native error box with a message using zenity

static int ZenityErrorBox(char *message)
{
    int result;
    char *escaped_message;
    char *errorboxpath;
    static size_t errorboxpath_size;

    if (!ZenityAvailable())
    {
        return 0;
    }

    escaped_message = EscapeShellString(message);

    errorboxpath_size = strlen(ZENITY_BINARY) + strlen(escaped_message) + 19;
    errorboxpath = malloc(errorboxpath_size);
    M_snprintf(errorboxpath, errorboxpath_size, "%s --error --text=%s",
               ZENITY_BINARY, escaped_message);

    result = system(errorboxpath);

    free(errorboxpath);
    free(escaped_message);

    return result;
}
#endif

//
// I_Error
//

void I_Error (char *error, ...)
{
#ifdef WII
    va_list argptr;

    atexit_listentry_t *entry;

    // Message first.
    va_start(argptr, error);

    vfprintf(debugfile, error, argptr);

    fprintf(debugfile, "\n\n");

    va_end(argptr);

    fclose (debugfile);

//    C_ConDump();

    if (already_quitting)
    {
        C_Warning("Warning: recursive call to I_Error detected.");
        printf("\n Warning: recursive call to I_Error detected.\n");

        error_detected = true;

        I_Quit();
    }
    else
    {
        already_quitting = true;
    }

    // Shutdown. Here might be other errors.

    entry = exit_funcs;

    while (entry != NULL)
    {
        if (entry->run_on_error)
        {
            entry->func();
        }

        entry = entry->next;
    }
    error_detected = true;
/*
    S_Shutdown();

    I_ShutdownGraphics();
*/
    I_Quit();
#else
    char msgbuf[512];
    va_list argptr;
    atexit_listentry_t *entry;
    dboolean exit_gui_popup;

    if (already_quitting)
    {
        fprintf(stderr, "\nWarning: recursive call to I_Error detected.\n");
        exit(-1);
    }
    else
    {
        already_quitting = true;
    }

    // Message first.
    va_start(argptr, error);
    //fprintf(stderr, "\nError: ");
    vfprintf(stderr, error, argptr);
    fprintf(stderr, "\n\n");
    va_end(argptr);
    fflush(stderr);

    // Write a copy of the message into buffer.
    va_start(argptr, error);
    memset(msgbuf, 0, sizeof(msgbuf));
    M_vsnprintf(msgbuf, sizeof(msgbuf), error, argptr);
    va_end(argptr);

    // Shutdown. Here might be other errors.

    entry = exit_funcs;

    while (entry != NULL)
    {
        if (entry->run_on_error)
        {
            entry->func();
        }

        entry = entry->next;
    }

    exit_gui_popup = !M_ParmExists("-nogui");

    // Pop up a GUI dialog box to show the error message, if the
    // game was not run from the console (and the user will
    // therefore be unable to otherwise see the message).
    if (exit_gui_popup && !I_ConsoleStdout())
    {
        ZenityErrorBox(msgbuf);
    }

    printf("\n Warning: Game exited safely (possible crash detected).\n");

    // abort();
#ifdef SDL2
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Wii-DOOM", msgbuf, NULL);
#endif
    SDL_Quit();

    exit(-1);
#endif
}

//
// Read Access Violation emulation.
//
// From PrBoom+, by entryway.
//

// C:\>debug
// -d 0:0
//
// DOS 6.22:
// 0000:0000  (57 92 19 00) F4 06 70 00-(16 00)
// DOS 7.1:
// 0000:0000  (9E 0F C9 00) 65 04 70 00-(16 00)
// Win98:
// 0000:0000  (9E 0F C9 00) 65 04 70 00-(16 00)
// DOSBox under XP:
// 0000:0000  (00 00 00 F1) ?? ?? ?? 00-(07 00)

#define DOS_MEM_DUMP_SIZE 10

static const unsigned char mem_dump_dos622[DOS_MEM_DUMP_SIZE] = {
  0x57, 0x92, 0x19, 0x00, 0xF4, 0x06, 0x70, 0x00, 0x16, 0x00};
static const unsigned char mem_dump_win98[DOS_MEM_DUMP_SIZE] = {
  0x9E, 0x0F, 0xC9, 0x00, 0x65, 0x04, 0x70, 0x00, 0x16, 0x00};
static const unsigned char mem_dump_dosbox[DOS_MEM_DUMP_SIZE] = {
  0x00, 0x00, 0x00, 0xF1, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00};

//
// nitr8 [UNUSED]
//
/*
#ifndef WII
static unsigned char mem_dump_custom[DOS_MEM_DUMP_SIZE];
#endif

static const unsigned char *dos_mem_dump = mem_dump_dos622;

dboolean I_GetMemoryValue(unsigned int offset, void *value, int size)
{
#ifndef WII
    static dboolean firsttime = true;

    if (firsttime)
    {
        int p, i, val;

        firsttime = false;
        i = 0;

        //!
        // @category compat
        // @arg <version>
        //
        // Specify DOS version to emulate for NULL pointer dereference
        // emulation.  Supported versions are: dos622, dos71, dosbox.
        // The default is to emulate DOS 7.1 (Windows 98).
        //

        p = M_CheckParmWithArgs("-setmem", 1);

        if (p > 0)
        {
            if (!strcasecmp(myargv[p + 1], "dos622"))
            {
                dos_mem_dump = mem_dump_dos622;
            }
            if (!strcasecmp(myargv[p + 1], "dos71"))
            {
                dos_mem_dump = mem_dump_win98;
            }
            else if (!strcasecmp(myargv[p + 1], "dosbox"))
            {
                dos_mem_dump = mem_dump_dosbox;
            }
            else
            {
                for (i = 0; i < DOS_MEM_DUMP_SIZE; ++i)
                {
                    ++p;

                    if (p >= myargc || myargv[p][0] == '-')
                    {
                        break;
                    }

                    M_StrToInt(myargv[p], &val);
                    mem_dump_custom[i++] = (unsigned char) val;
                }

                dos_mem_dump = mem_dump_custom;
            }
        }
    }
#endif

    switch (size)
    {
    case 1:
        *((unsigned char *) value) = dos_mem_dump[offset];
        return true;
    case 2:
        *((unsigned short *) value) = dos_mem_dump[offset]
                                    | (dos_mem_dump[offset + 1] << 8);
        return true;
    case 4:
        *((unsigned int *) value) = dos_mem_dump[offset]
                                  | (dos_mem_dump[offset + 1] << 8)
                                  | (dos_mem_dump[offset + 2] << 16)
                                  | (dos_mem_dump[offset + 3] << 24);
        return true;
    }

    return false;
}
*/

