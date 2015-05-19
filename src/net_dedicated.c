//
// Copyright(C) 2005-2014 Simon Howard
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
//
// Dedicated server code.
// 

#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"

#include "i_system.h"
#include "i_timer.h"

#include "net_defs.h"
#include "net_sdl.h"
#include "net_server.h"

#include "doomfeatures.h"

// 
// People can become confused about how dedicated servers work.  Game
// options are specified to the controlling player who is the first to
// join a game.  Bomb out with an error message if game options are
// specified to a dedicated server.
//

void NET_DedicatedServer(void)
{
#ifdef NET_DEBUG
    printf("NET_DedicatedServer\n");
#endif

    NET_SV_Init();
    NET_SV_AddModule(&net_sdl_module);
    NET_SV_RegisterWithMaster();

    while (true)
    {
        NET_SV_Run();
        I_Sleep(10);
    }
}

