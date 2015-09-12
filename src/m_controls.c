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

#ifdef WII
#include "doomdef.h"
#else
#include "doom/doomdef.h"
#endif

#include "doomfeatures.h"
#include "doomkeys.h"

#ifdef WII
#include "doomstat.h"
#else
#include "doom/doomstat.h"
#endif

#include "doomtype.h"
#include "m_config.h"
#include "m_misc.h"

#ifdef WII
#include "r_local.h"
#include "s_sound.h"
#else
#include "doom/r_local.h"
#include "doom/s_sound.h"
#endif

#include "v_video.h"

/*
extern boolean am_rotate;
extern boolean d_recoil;
extern boolean d_maxgore;
extern boolean d_thrust;
extern boolean respawnparm;
extern boolean fastparm;
*/
extern boolean hud;
extern boolean swap_sound_chans;
extern boolean randompitch;

extern int display_fps;
/*
extern int drawgrid;
extern int followplayer;
*/
extern int showMessages;
//extern int mouseSensitivity;
extern int wipe_type;
extern int timer_info;

// Bind all of the common controls used by Doom and all other games.

extern int show_stats;
extern int screenblocks;
/*
extern int sfxVolume;
extern int musicVolume;
*/
extern int forwardmove;
extern int sidemove;
extern int turnspeed;
extern int crosshair;
extern int mus_engine;
extern int mouselook;
//extern int autoaim;
extern int runcount;
extern int chaingun_tics;
extern int snd_module;
extern int sound_channels;
extern int gore_amount;
extern int png_screenshots;

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
int key_strafeleft = KEY_COMMA;                // FOR PSP: THIS IS RESERVED AS A SPECIAL KEY
int key_straferight = KEY_PERIOD;              // FOR PSP: THIS IS RESERVED AS A SPECIAL KEY
int key_useartifact = KEY_ENTER;
int key_use = ' ';
int key_fire = KEY_RCTRL;
int key_speed = KEY_RSHIFT; 
int key_flyup = KEY_PGUP; 
int key_flydown = KEY_PGDN;
int key_jump = KEY_DEL;
int key_strafe = KEY_RALT;
int mspeed = 2;

// Map control keys:

int key_map_north     = KEY_UPARROW;
int key_map_south     = KEY_DOWNARROW;
int key_map_east      = KEY_RIGHTARROW;
int key_map_west      = KEY_LEFTARROW;
int key_map_zoomin    = '=';
int key_map_zoomout   = '-';
int key_map_toggle    = KEY_TAB;

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

int key_menu_help      = KEY_F1;
int key_menu_save      = KEY_F2;
int key_menu_load      = KEY_F3;
int key_menu_volume    = KEY_F4;
int key_menu_detail    = KEY_F5;
int key_menu_qsave     = KEY_F6;
int key_menu_endgame   = KEY_F7;
int key_menu_messages  = KEY_F8;
int key_menu_qload     = KEY_F9;
int key_menu_quit      = KEY_F10;
int key_menu_gamma     = KEY_F11;

int key_menu_incscreen = KEY_EQUALS;
int key_menu_decscreen = KEY_MINUS;
int key_menu_screenshot = KEY_F12;

int key_console = KEY_TILDE;

// Control whether if a mouse button is double clicked, it acts like 
// "use" has been pressed

int dclick_use = 1;
 
// Weapon selection keys:

int key_weapon1 = '1';
int key_weapon2 = '2';
int key_weapon3 = '3';
int key_weapon4 = '4';
int key_weapon5 = '5';
int key_weapon6 = '6';
int key_weapon7 = '7';
int key_weapon8 = '8';
int key_prevweapon = 0;
int key_nextweapon = 0;

//int mousebjump = -1;
int mousebjump = 2;
int mousebfire = 0;
//int mousebforward = 1;
int mousebforward = -1;
//int mousebstrafe = 2;
int mousebstrafe = -1;
int mousebbackward = -1;
//int mousebprevweapon = -1;
int mousebprevweapon = 3;
//int mousebnextweapon = -1;
int mousebnextweapon = 4;
int mousebstrafeleft = -1;
int mousebstraferight = -1;
int mousebuse = 1;
//int mousebuse = -1;

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
    M_BindVariable("uncapped_framerate",     &d_uncappedframerate);
    M_BindVariable("opltype",                &opl_type);
    M_BindVariable("trails",                 &smoketrails);
    M_BindVariable("chaingun_speed",         &chaingun_tics);
    M_BindVariable("sound_type",             &snd_module);
    M_BindVariable("sound_chans",            &sound_channels);
    M_BindVariable("hom_detector",           &autodetect_hom);
    M_BindVariable("falling_damage",         &d_fallingdamage);
    M_BindVariable("infinite_ammo",          &d_infiniteammo);
    M_BindVariable("no_monsters",            &not_monsters);
    M_BindVariable("screenwipe_type",        &wipe_type);
    M_BindVariable("automap_overlay",        &overlay_trigger);
    M_BindVariable("show_timer",             &timer_info);
    M_BindVariable("replace",                &replace_missing);
    M_BindVariable("goreamount",             &gore_amount);
    M_BindVariable("random_pitch",           &randompitch);
    M_BindVariable("telefrag",               &d_telefrag);
    M_BindVariable("doorstuck",              &d_doorstuck);
    M_BindVariable("resurrect_ghosts",       &d_resurrectghosts);
    M_BindVariable("limited_ghosts",         &d_limitedghosts);
    M_BindVariable("block_skulls",           &d_blockskulls);
    M_BindVariable("blazing_sound",          &d_blazingsound);
    M_BindVariable("god",                    &d_god);
    M_BindVariable("floors",                 &d_floors);
    M_BindVariable("model",                  &d_model);
    M_BindVariable("hell",                   &d_666);
    M_BindVariable("masked_anim",            &d_maskedanim);
    M_BindVariable("sound",                  &d_sound);
    M_BindVariable("ouchface",               &d_ouchface);
    M_BindVariable("authors",                &show_authors);
    M_BindVariable("png_screenshot",         &png_screenshots);
    M_BindVariable("menu_type",              &background_type);
    M_BindVariable("menu_shadow",            &font_shadow);
    M_BindVariable("shadows",                &d_shadows);
    M_BindVariable("offsets",                &d_fixspriteoffsets);
//    M_BindVariable("memory",                 &memory_usage);
#ifdef WII
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
    M_BindVariable("key_screenshots",        &joy_x);
#else
    M_BindVariable("key_shoot",              &key_fire);
    M_BindVariable("key_open",               &key_use);
    M_BindVariable("key_menu",               &key_menu_activate);
    M_BindVariable("key_weapon_left",        &key_prevweapon);
    M_BindVariable("key_automap",            &key_map_toggle);
    M_BindVariable("key_weapon_right",       &key_nextweapon);
    M_BindVariable("key_automap_zoom_in",    &key_map_zoomin);
    M_BindVariable("key_automap_zoom_out",   &key_map_zoomout);
    M_BindVariable("key_flyup",              &key_flyup);
    M_BindVariable("key_flydown",            &key_flydown);
    M_BindVariable("key_jump",               &key_jump);
    M_BindVariable("key_run",                &key_speed);
    M_BindVariable("key_console",            &key_console);
    M_BindVariable("key_screenshots",        &key_menu_screenshot);
    M_BindVariable("key_strafe_left",        &key_strafeleft);
    M_BindVariable("key_strafe_right",       &key_straferight);
#endif
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

