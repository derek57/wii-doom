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


#include <ogcsys.h>
#include <SDL/SDL.h>
#include <stdio.h>

#include "config.h"
#include "d_main.h"
#include "doomtype.h"
#include "gui.h"
#include "i_system.h"
#include "i_wiimain.h"
#include "xmn_main.h"

#include <wiiuse/wpad.h>


// MAIN DEVPARM
boolean        devparm = false;
boolean        devparm_net = false;

// SOLO DEVPARM
boolean        devparm_nerve = false;
boolean        devparm_doom = false;
boolean        devparm_doom2 = false;
boolean        devparm_freedoom2 = false;
boolean        devparm_tnt = false;
boolean        devparm_plutonia = false;
boolean        devparm_chex = false;
boolean        devparm_hacx = false;

// NETWORK DEVPARM
boolean        devparm_net_nerve = false;
boolean        devparm_net_doom = false;
boolean        devparm_net_doom2 = false;
boolean        devparm_net_freedoom2 = false;
boolean        devparm_net_tnt = false;
boolean        devparm_net_plutonia = false;
boolean        devparm_net_chex = false;
boolean        devparm_net_hacx = false;


int exit_by_reset = 0;
int return_reset = 2;


void reset_call()
{
    exit_by_reset = return_reset;
}

void power_call()
{
    exit_by_reset = 3;
}

int user_main()
{
    drawDirectory();

    return 0;
}

void My_Quit(void)
{
    if (*((u32*)0x80001800) && strncmp("STUBHAXX", (char *)0x80001804, 8) == 0)
        return_reset = 1;
    else
        return_reset = 2;

    WPAD_Shutdown();

    if(exit_by_reset == 2)
        SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);

    if(exit_by_reset == 3)
        SYS_ResetSystem(SYS_POWEROFF_STANDBY, 0, 0);

    return;
}

int main(int argc, char **argv)
{
    // Set RESET/POWER button callback
    SYS_SetResetCallback(reset_call); // esto es para que puedas salir al pulsar boton de RESET
    SYS_SetPowerCallback(power_call); // esto para apagar con power

    wii_main();

    atexit (My_Quit);

    // start doom
    if(devparm || devparm_net)
        D_DoomMain ();
    else
        user_main();

    return 0;
}

