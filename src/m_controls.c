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


#include <stdio.h>

#include "doomdef.h"
#include "doomfeatures.h"
#include "doomkeys.h"
#include "doomstat.h"
#include "doomtype.h"
#include "m_config.h"
#include "m_misc.h"
#include "r_local.h"
#include "s_sound.h"
#include "v_video.h"


extern boolean am_rotate;
extern boolean opl;
extern boolean d_recoil;
extern boolean d_maxgore;
extern boolean d_thrust;
extern boolean respawnparm;
extern boolean fastparm;
extern boolean hud;
extern boolean swap_sound_chans;

extern int display_fps;
extern int drawgrid;
extern int followplayer;
extern int showMessages;
extern int mouseSensitivity;

// Bind all of the common controls used by Doom and all other games.

extern int show_stats;
extern int screenblocks;
extern int sfxVolume;
extern int musicVolume;
extern int forwardmove;
extern int sidemove;
extern int turnspeed;
extern int crosshair;
extern int mus_engine;
extern int mouselook;
extern int autoaim;
extern int runcount;

extern int joy_up;
extern int joy_down;
extern int joy_left;
extern int joy_right;
extern int joy_zl;
extern int joy_zr;
extern int joy_l;
extern int joy_r;
extern int joy_plus;
extern int joy_minus;
extern int joy_home;
extern int joy_a;
extern int joy_b;
extern int joy_x;
extern int joy_y;
extern int joy_1;
extern int joy_2;

extern int opl_type;

int key_up = KEY_UPARROW;
int key_down = KEY_DOWNARROW; 
int key_left = KEY_LEFTARROW;
int key_right = KEY_RIGHTARROW;
int key_invright = KEY_RIGHTBRACKET;
int key_strafeleft = KEY_ESCAPE;                // FOR PSP: THIS IS RESERVED AS A SPECIAL KEY
int key_useartifact = KEY_ENTER;
int key_use = KEY_SPACE;
int key_fire = KEY_RCTRL;
int key_speed; 
int mspeed = 2;

// Map control keys:

int key_map_north     = KEY_UPARROW;
int key_map_south     = KEY_DOWNARROW;
int key_map_east      = KEY_RIGHTARROW;
int key_map_west      = KEY_LEFTARROW;
int key_map_zoomin    = '=';
int key_map_zoomout   = '-';
int key_map_toggle    = KEY_DOWNARROW;

// menu keys:

int key_menu_activate  = KEY_ESCAPE;
int key_menu_up        = KEY_UPARROW;
int key_menu_down      = KEY_DOWNARROW;
int key_menu_left      = KEY_LEFTARROW;
int key_menu_right     = KEY_RIGHTARROW;
int key_menu_back      = KEY_BACKSPACE;
int key_menu_forward   = KEY_ENTER;
int key_menu_confirm   = 'y';
int key_menu_abort     = 'n';

// Joystick controls

int joybstrafeleft = -1;
int joybstraferight = -1;

void M_BindBaseControls(void)
{
    M_BindVariable("sfx_volume",             &sfxVolume);
    M_BindVariable("music_volume",           &musicVolume);
    M_BindVariable("screensize",             &screenblocks);
    M_BindVariable("walking_speed",          &forwardmove);
    M_BindVariable("turning_speed",          &turnspeed);
    M_BindVariable("strafing_speed",         &sidemove);
    M_BindVariable("freelook_speed",         &mspeed);
    M_BindVariable("use_gamma",              &usegamma);
    M_BindVariable("mouse_look",             &mouselook);
    M_BindVariable("map_grid",               &drawgrid);
    M_BindVariable("follow_player",          &followplayer);
    M_BindVariable("showstats",              &show_stats);
    M_BindVariable("show_messages",          &showMessages);
    M_BindVariable("map_rotate",             &am_rotate);
    M_BindVariable("detail",                 &detailLevel);
    M_BindVariable("vanilla_weapon_change",  &use_vanilla_weapon_change);
    M_BindVariable("xhair",                  &crosshair);
    M_BindVariable("jump",                   &jumping);
    M_BindVariable("music_engine",           &mus_engine);
    M_BindVariable("recoil",                 &d_recoil);
    M_BindVariable("monsters_respawn",       &respawnparm);
    M_BindVariable("fast_monsters",          &fastparm);
    M_BindVariable("auto_aim",               &autoaim);
    M_BindVariable("max_gore",               &d_maxgore);
    M_BindVariable("extra_hud",              &hud);
    M_BindVariable("switch_chans",           &swap_sound_chans);
    M_BindVariable("player_thrust",          &d_thrust);
    M_BindVariable("run_count",              &runcount);
    M_BindVariable("footsteps",              &d_footstep);
    M_BindVariable("footclip",               &d_footclip);
    M_BindVariable("splash",                 &d_splash);
    M_BindVariable("swirl",                  &d_swirl);
    M_BindVariable("pr_beta",                &beta_style_mode);
    M_BindVariable("translucency",           &d_translucency);
    M_BindVariable("colored_blood",          &d_colblood);
    M_BindVariable("fixed_blood",            &d_colblood2);
    M_BindVariable("mirrored_corpses",       &d_flipcorpses);
    M_BindVariable("show_secrets",           &d_secrets);
//    M_BindVariable("uncapped_framerate",     &d_uncappedframerate);
    M_BindVariable("opltype",                &opl_type);
    M_BindVariable("trails",                 &smoketrails);
    M_BindVariable("key_shoot",              &joy_r);
    M_BindVariable("key_open",               &joy_l);
    M_BindVariable("key_menu",               &joy_minus);
    M_BindVariable("key_weapon_left",        &joy_left);
    M_BindVariable("key_automap",            &joy_down);
    M_BindVariable("key_weapon_right",       &joy_right);
    M_BindVariable("key_automap_zoom_in",    &joy_zl);
    M_BindVariable("key_automap_zoom_out",   &joy_zr);
    M_BindVariable("key_flyup",              &joy_a);
    M_BindVariable("key_flydown",            &joy_y);
    M_BindVariable("key_jump",               &joy_b);
    M_BindVariable("key_run",                &joy_1);
    M_BindVariable("key_console",            &joy_2);
//    M_BindVariable("key_aiminghelp",         &joy_plus);
}

//
// Apply custom patches to the default values depending on the
// platform we are running on.
//

void M_ApplyPlatformDefaults(void)
{
    // no-op. Add your platform-specific patches here.
}

