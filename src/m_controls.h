//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
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


#ifndef __M_CONTROLS_H__
#define __M_CONTROLS_H__

 
extern int key_up;
extern int key_down;
extern int key_left;
extern int key_right;
extern int key_invright;
extern int key_jump;
extern int key_flyup;
extern int key_flydown;
extern int key_strafeleft;
extern int key_useartifact;
extern int key_use;
extern int key_fire;
extern int key_speed;
extern int key_map_north;
extern int key_map_south;
extern int key_map_east;
extern int key_map_west;
extern int key_map_zoomin;
extern int key_map_zoomout;
extern int key_map_toggle;

// menu keys:

extern int key_menu_activate;
extern int key_menu_up;
extern int key_menu_down;
extern int key_menu_left;
extern int key_menu_right;
extern int key_menu_back;
extern int key_menu_forward;
extern int key_menu_confirm;
extern int key_menu_abort;

extern int joybstrafeleft;
extern int joybstraferight;

void M_BindBaseControls(void);
void M_BindStrifeControls(void);

#endif /* #ifndef __M_CONTROLS_H__ */

