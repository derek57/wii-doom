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


#include <SDL/SDL.h>
#include <stdio.h>

#include "config.h"
#include "d_main.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_wiimain.h"
#include "xmn_main.h"


// MAIN DEVPARM
boolean        devparm = true;
boolean        devparm_net = false;

// SOLO DEVPARM
boolean        devparm_nerve = false;
boolean        devparm_doom = true;
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


int user_main()
{
    drawDirectory();

    return 0;
}

int main(int argc, char **argv)
{
    // start doom

    wii_main();

    if(devparm || devparm_net)
        D_DoomMain ();
    else
        user_main();

    return 0;
}

