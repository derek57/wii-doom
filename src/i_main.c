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
//        Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------


#ifdef WII
#include <ogcsys.h>
#endif

#ifdef SDL2
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include <signal.h>
#include <stdio.h>

#include "config.h"

#include "doom/d_main.h"

#include "doomfeatures.h"
#include "doomtype.h"

#ifdef WII
#include "../wii/gui.h"
#endif

#include "i_system.h"

#ifdef WII
#include "../wii/i_wiimain.h"
#endif

#include "m_argv.h"

#ifdef WII
#include "../wii/xmn_main.h"
#include <wiiuse/wpad.h>
#endif

#include "z_zone.h"


//
// e6y: exeptions handling
//
typedef enum
{
    EXEPTION_NONE,
    EXEPTION_glFramebufferTexture2DEXT,
    EXEPTION_MAX

} ExeptionsList_t;

typedef struct
{
    const char *error_message;

} ExeptionParam_t;


static ExeptionParam_t    ExeptionsParams[];

static ExeptionsList_t    current_exception_index;


// MAIN DEVPARM
dboolean                  devparm = false;
dboolean                  devparm_net = false;

// SOLO DEVPARM
dboolean                  devparm_nerve = false;
dboolean                  devparm_master = false;
dboolean                  devparm_doom = false;
dboolean                  devparm_doom2 = false;
dboolean                  devparm_freedoom2 = false;
dboolean                  devparm_tnt = false;
dboolean                  devparm_plutonia = false;
dboolean                  devparm_chex = false;
dboolean                  devparm_hacx = false;

// NETWORK DEVPARM
dboolean                  devparm_net_nerve = false;
dboolean                  devparm_net_doom = false;
dboolean                  devparm_net_doom2 = false;
dboolean                  devparm_net_freedoom2 = false;
dboolean                  devparm_net_tnt = false;
dboolean                  devparm_net_plutonia = false;
dboolean                  devparm_net_chex = false;
dboolean                  devparm_net_hacx = false;


int                       exit_by_reset = 0;
int                       return_reset = 2;


#ifdef WII
void reset_call()
{
    exit_by_reset = return_reset;
}

void power_call()
{
    exit_by_reset = 3;
}

void My_Quit(void)
{
    if (*((u32*)0x80001800) && strncmp("STUBHAXX", (char *)0x80001804, 8) == 0)
        return_reset = 1;
    else
        return_reset = 2;

    WPAD_Shutdown();

    if (exit_by_reset == 2)
        SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);

    if (exit_by_reset == 3)
        SYS_ResetSystem(SYS_POWEROFF_STANDBY, 0, 0);

    return;
}
#endif

void I_ExeptionProcess(void)
{
    if (current_exception_index > EXEPTION_NONE && current_exception_index < EXEPTION_MAX)
    {
        I_Error("%s", ExeptionsParams[current_exception_index].error_message);
    }
}

// cleanup handling -- killough:
static void I_SignalHandler(int s)
{
    char buf[2048];

    // Ignore future instances of this signal.
    signal(s, SIG_IGN);

    // e6y
    I_ExeptionProcess();

    strcpy(buf, "Exiting on signal: ");

    I_SigString(buf + strlen(buf), 2000 - strlen(buf), s);

    // If corrupted memory could cause crash, dump memory
    // allocation history, which points out probable causes

#ifdef INSTRUMENTED
    if (s == SIGSEGV || s == SIGILL || s == SIGFPE)
        Z_DumpHistory(buf);
#endif

    I_Error("I_SignalHandler: %s", buf);
}

int main(int argc, char **argv)
{
    // save arguments

#ifndef WII
    myargc = argc;
    myargv = argv;

    M_FindResponseFile();
#endif

#ifndef PRBOOM_DEBUG
#ifndef WII
    if (!M_CheckParm("-devparm"))
#else
    if (devparm)
#endif
    {
        signal(SIGSEGV, I_SignalHandler);
    }

    signal(SIGTERM, I_SignalHandler);
    signal(SIGFPE,  I_SignalHandler);
    signal(SIGILL,  I_SignalHandler);

    /* killough 3/6/98: allow CTRL-BRK during init */
    signal(SIGINT,  I_SignalHandler);

    signal(SIGABRT, I_SignalHandler);
#endif

#ifdef WII
    // Set RESET / POWER button callback
    SYS_SetResetCallback(reset_call);
    SYS_SetPowerCallback(power_call);

    wii_main();

    atexit(My_Quit);

    // start doom
    if (devparm || devparm_net)
#endif
        D_DoomMain();
#ifdef WII
    else
        drawDirectory();
#endif

    return 0;
}

