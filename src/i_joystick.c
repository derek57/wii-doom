/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *   Joystick handling for Linux
 *
 *-----------------------------------------------------------------------------
 */


#ifdef WII
#include <math.h>
#include <SDL/SDL.h>
#include <stdlib.h>

#include "d_event.h"

#include "doom/d_main.h"
#include "doom/doomdef.h"

#include "doomtype.h"
#include "i_joystick.h"

#include <wiiuse/wpad.h>


int joyleft;
int joyright;
int joyup;
int joydown;


void I_UpdateJoystick(void)
{
    WPADData *data = WPAD_Data(0);

    ir_t ir;

    int btn_a, btn_b, btn_c, btn_z;
    int btn_1, btn_2, btn_l, btn_r, btn_d, btn_u, btn_p, btn_m, btn_h;
    int btn_x, btn_y, btn_plus, btn_minus;

    event_t ev;
  
    PAD_ScanPads();

    WPAD_ScanPads();

    WPAD_IR(0, &ir);

    btn_a     = 0;
    btn_b     = 0;
    btn_c     = 0;
    btn_z     = 0;
    btn_1     = 0;
    btn_2     = 0;
    btn_l     = 0;
    btn_d     = 0;
    btn_r     = 0;
    btn_u     = 0;
    btn_p     = 0;
    btn_m     = 0;
    btn_h     = 0;
    btn_x     = 0;
    btn_y     = 0;
    btn_plus  = 0;
    btn_minus = 0;

    // Classic Controller
    if (data->exp.type == WPAD_EXP_CLASSIC)
    {  
        Sint16 axis_x, axis_y;

        int nun_x = data->exp.classic.ljs.pos.x;
        int nun_y = data->exp.classic.ljs.pos.y;

        int center = data->exp.classic.ljs.center.x;

        int min = data->exp.classic.ljs.min.x;

        int max = data->exp.classic.ljs.max.x;

        // Left
        if (nun_x < center - ((center - min) * 0.1f))
            axis_x = (1.0f * center - nun_x) / (center - min) * -50.0f;
        // Right
        else if (nun_x > center + ((max - center) * 0.1f))
            axis_x = (1.0f * nun_x - center) / (max - center) * 50.0f;
        // No stick X movement
        else
            axis_x = 0;

        center = data->exp.classic.ljs.center.y;

        min = data->exp.classic.ljs.min.y;

        max = data->exp.classic.ljs.max.y;

        // Up
        if (nun_y < center - ((center - min) * 0.1f))
            axis_y = (1.0f * center - nun_y) / (center - min) * -50.0f;
        // Down
        else if (nun_y > center + ((max - center) * 0.1f))
            axis_y = (1.0f * nun_y - center) / (max - center) * 50.0f;
        // No stick Y movement
        else
            axis_y = 0;

        // For some strange reason, the home button is detected
        // as a keypress and mapped to the esc key.
        // I suspect it's SDL-Port at work here but since it's not
        // a dealbreaker, I'm not terribly
        // interested in tracking it down. It does pretty much what
        // I would have it do anyway.

        // Use / Open / Select
        if (data->btns_h & WPAD_CLASSIC_BUTTON_A)
            btn_d = 1;

        // Fire
        if (data->btns_h & WPAD_CLASSIC_BUTTON_FULL_R)
            btn_b = 1;

        // Map
        if (data->btns_h & WPAD_CLASSIC_BUTTON_PLUS)
            btn_1 = 1;

        // Run
        if (data->btns_h & WPAD_CLASSIC_BUTTON_FULL_L)
            btn_a = 1;

        // Automap follow
        if (data->btns_h & WPAD_CLASSIC_BUTTON_MINUS)
            btn_h = 1;

        // No idea ....
        if (data->btns_h & WPAD_CLASSIC_BUTTON_B)
            btn_2 = 1;

        // Left Weapon Cycle / Pan Map
        if (data->btns_h & WPAD_CLASSIC_BUTTON_LEFT)
            btn_l = 1;

        // Pan map
        if (data->btns_h & WPAD_CLASSIC_BUTTON_DOWN)
            btn_c = 1;

        // Right Weapon Cycle / Pan Map
        if (data->btns_h & WPAD_CLASSIC_BUTTON_RIGHT)
            btn_u = 1;

        if (data->btns_h & WPAD_CLASSIC_BUTTON_UP)
            btn_r = 1;

        // Map zoom in
        if (data->btns_h & WPAD_CLASSIC_BUTTON_ZR)
            btn_p = 1;

        // Map Zoom Out
        if (data->btns_h & WPAD_CLASSIC_BUTTON_ZL)
            btn_m = 1;

        // Escape
        if (data->btns_h & WPAD_CLASSIC_BUTTON_HOME)
            btn_z = 1;

        if (data->btns_h & WPAD_CLASSIC_BUTTON_X)
            btn_x = 1;

        if (data->btns_h & WPAD_CLASSIC_BUTTON_Y)
            btn_y = 1;

        if (data->btns_h & WPAD_BUTTON_1)
            btn_plus = 1;

        if (data->btns_h & WPAD_BUTTON_2)
            btn_minus = 1;

        ev.type = ev_joystick;

        ev.data1 = ((btn_d)     <<  0) |
                   ((btn_b)     <<  1) |
                   ((btn_1)     <<  2) |
                   ((btn_a)     <<  3) |
                   ((btn_h)     <<  4) |
                   ((btn_2)     <<  5) |
                   ((btn_l)     <<  6) |
                   ((btn_c)     <<  7) |
                   ((btn_u)     <<  8) |
                   ((btn_r)     <<  9) |
                   ((btn_p)     << 10) |
                   ((btn_m)     << 11) |
                   ((btn_z)     << 12) |
                   ((btn_x)     << 13) |
                   ((btn_y)     << 14) |
                   ((btn_plus)  << 15) |
                   ((btn_minus) << 16);

        ev.data2 = axis_x; 
        ev.data3 = axis_y;

        // For turning
        nun_x = data->exp.classic.rjs.pos.x;
        nun_y = data->exp.classic.rjs.pos.y;

        center = data->exp.classic.rjs.center.x;

        min = data->exp.classic.rjs.min.x;

        max = data->exp.classic.rjs.max.x;

        // Left
        if (nun_x < center - ((center - min) * 0.1f))
            axis_x = (1.0f * center - nun_x) / (center - min) * -130.0f;
        // Right
        else if (nun_x > center + ((max - center) * 0.1f))
            axis_x = (1.0f * nun_x - center) / (max - center) * 130.0f;
        // No stick X movement
        else
            axis_x = 0;

        center = data->exp.classic.rjs.center.y;

        min = data->exp.classic.rjs.min.y;

        max = data->exp.classic.rjs.max.y;

        // Up
        if (nun_y < center - ((center - min) * 0.1f))
            axis_y = (1.0f * center - nun_y) / (center - min) * -130.0f;
        // Down
        else if (nun_y > center + ((max - center) * 0.1f))
            axis_y = (1.0f * nun_y - center) / (max - center) * 130.0f;
        // No stick Y movement
        else
            axis_y = 0;
    
        ev.data4 = axis_x;
        ev.data5 = axis_y;
    }

    // End Classic Controller
    D_PostEvent(&ev);
}
#endif

