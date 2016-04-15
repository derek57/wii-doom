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
 *  Joystick interface.
 *
 *-----------------------------------------------------------------------------*/


#ifndef __I_JOYSTICK__
#define __I_JOYSTICK__

// Number of "virtual" joystick buttons defined in configuration files.
// This needs to be at least as large as the number of different key
// bindings supported by the higher-level game code (joyb* variables).
#define NUM_VIRTUAL_BUTTONS 10

// If this bit is set in a configuration file axis value, the axis is
// not actually a joystick axis, but instead is a "button axis". This
// means that instead of reading an SDL joystick axis, we read the
// state of two buttons to get the axis value. This is needed for eg.
// the PS3 SIXAXIS controller, where the D-pad buttons register as
// buttons, not as two axes.
#define BUTTON_AXIS 0x10000

// Query whether a given axis value describes a button axis.
#define IS_BUTTON_AXIS(axis) ((axis) >= 0 && ((axis) & BUTTON_AXIS) != 0)

// Get the individual buttons from a button axis value.
#define BUTTON_AXIS_NEG(axis)  ((axis) & 0xff)
#define BUTTON_AXIS_POS(axis)  (((axis) >> 8) & 0xff)

// Create a button axis value from two button values.
#define CREATE_BUTTON_AXIS(neg, pos) (BUTTON_AXIS | (neg) | ((pos) << 8))

// If this bit is set in an axis value, the axis is not actually a
// joystick axis, but is a "hat" axis. This means that we read (one of)
// the hats on the joystick.
#define HAT_AXIS    0x20000

#define IS_HAT_AXIS(axis) ((axis) >= 0 && ((axis) & HAT_AXIS) != 0)

// Get the hat number from a hat axis value.
#define HAT_AXIS_HAT(axis)         ((axis) & 0xff)
// Which axis of the hat? (horizonal or vertical)
#define HAT_AXIS_DIRECTION(axis)   (((axis) >> 8) & 0xff)

#define CREATE_HAT_AXIS(hat, direction) \
    (HAT_AXIS | (hat) | ((direction) << 8))

#define HAT_AXIS_HORIZONTAL 1
#define HAT_AXIS_VERTICAL   2


void I_InitJoystick(void);
void I_PollJoystick(void);
void I_UpdateJoystick(void);


extern int joyleft;
extern int joyright;
extern int joyup;
extern int joydown;
extern int usejoystick;

#endif

