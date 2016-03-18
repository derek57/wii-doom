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

#include "doom/doomdef.h"

#include "doomfeatures.h"
#include "doomkeys.h"

#include "doom/doomstat.h"

#include "doomtype.h"
#include "i_video.h"
#include "m_config.h"
#include "m_misc.h"

#include "doom/r_local.h"
#include "doom/s_sound.h"

#include "v_video.h"


// FOR PSP: THESE ARE RESERVED AS SPECIAL KEYS
int key_strafeleft = KEY_COMMA;
int key_straferight = KEY_PERIOD;

int key_up = KEY_UPARROW;
int key_down = KEY_DOWNARROW; 
int key_left = KEY_LEFTARROW;
int key_right = KEY_RIGHTARROW;
int key_invright = KEY_RIGHTBRACKET;
int key_useartifact = KEY_ENTER;
int key_use = ' ';
int key_fire = KEY_RCTRL;
int key_speed = KEY_RSHIFT; 
int key_flyup = KEY_PGUP; 
int key_flydown = KEY_PGDN;
int key_jump = KEY_DEL;
int key_strafe = KEY_RALT;
int key_aiming = 'x';

// Map control keys:
int key_map_north     = KEY_UPARROW;
int key_map_south     = KEY_DOWNARROW;
int key_map_east      = KEY_RIGHTARROW;
int key_map_west      = KEY_LEFTARROW;
int key_map_zoomin    = '=';
int key_map_zoomout   = '-';
int key_map_toggle    = KEY_TAB;
int key_map_maxzoom   = '0';
int key_map_follow    = 'f';
int key_map_grid      = 'g';
int key_map_mark      = 'm';
int key_map_clearmark = 'c';

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
int key_menu_screenshot_beta = KEY_F11;

int key_console = KEY_TILDE;
int key_spy = KEY_F12;
int key_demo_quit = 'q';
int key_pause = KEY_PAUSE;

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

int mousebjump = 2;
int mousebfire = 0;
int mousebforward = -1;
int mousebstrafe = -1;
int mousebbackward = -1;
int mousebprevweapon = MOUSE_WHEELUP;
int mousebnextweapon = MOUSE_WHEELDOWN;
int mousebstrafeleft = -1;
int mousebstraferight = -1;
int mousebuse = 1;
int mousebaiming = -1;

// Joystick controls
int joybstrafeleft = -1;
int joybstraferight = -1;


// Bind all of the common controls used by Doom and all other games.
extern dboolean aiming_help;

extern int runcount;
extern int png_screenshots;
extern int pixelwidth;
extern int pixelheight;
extern int r_bloodsplats_max;
extern int correct_lost_soul_bounce;
extern int window_pos_x;
extern int window_pos_y;

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


void M_BindBaseControls(void)
{
    M_BindVariable("sfx_volume",               &sfxVolume);
    M_BindVariable("music_volume",             &musicVolume);
    M_BindVariable("screensize",               &screenblocks);
    M_BindVariable("walking_speed",            &forwardmove);
    M_BindVariable("turning_speed",            &turnspeed);
    M_BindVariable("strafing_speed",           &sidemove);
    M_BindVariable("freelook_speed",           &mspeed);
    M_BindVariable("use_gamma",                &usegamma);
    M_BindVariable("mouse_look",               &mouselook);
    M_BindVariable("map_grid",                 &drawgrid);
    M_BindVariable("follow_player",            &followplayer);
    M_BindVariable("showstats",                &show_stats);
    M_BindVariable("show_messages",            &showMessages);
    M_BindVariable("map_rotate",               &am_rotate);
    M_BindVariable("detail",                   &detailLevel);
    M_BindVariable("vanilla_weapon_change",    &use_vanilla_weapon_change);
    M_BindVariable("xhair",                    &crosshair);
    M_BindVariable("jump",                     &jumping);
    M_BindVariable("music_engine",             &mus_engine);
    M_BindVariable("recoil",                   &d_recoil);
    M_BindVariable("monsters_respawn",         &start_respawnparm);
    M_BindVariable("fast_monsters",            &start_fastparm);
    M_BindVariable("auto_aim",                 &autoaim);
    M_BindVariable("max_gore",                 &d_maxgore);
    M_BindVariable("extra_hud",                &hud);
    M_BindVariable("switch_chans",             &opl_stereo_correct);
    M_BindVariable("player_thrust",            &d_thrust);
    M_BindVariable("run_count",                &runcount);
    M_BindVariable("footsteps",                &d_footstep);
    M_BindVariable("footclip",                 &d_footclip);
    M_BindVariable("splash",                   &d_splash);
    M_BindVariable("swirl",                    &d_swirl);

#ifdef WII
    M_BindVariable("pr_beta",                  &beta_style_mode);
#endif

    M_BindVariable("translucency",             &d_translucency);
    M_BindVariable("colored_blood",            &d_colblood);
    M_BindVariable("fixed_blood",              &d_colblood2);
    M_BindVariable("mirrored_corpses",         &d_flipcorpses);
    M_BindVariable("show_secrets",             &d_secrets);
    M_BindVariable("uncapped_framerate",       &d_uncappedframerate);
    M_BindVariable("opltype",                  &opl_type);
    M_BindVariable("trails",                   &smoketrails);
    M_BindVariable("chaingun_speed",           &chaingun_tics);
    M_BindVariable("sound_type",               &snd_module);
    M_BindVariable("sound_chans",              &sound_channels);
    M_BindVariable("hom_detector",             &autodetect_hom);
    M_BindVariable("falling_damage",           &d_fallingdamage);
    M_BindVariable("infinite_ammo",            &d_infiniteammo);
    M_BindVariable("no_monsters",              &not_monsters);
    M_BindVariable("screenwipe_type",          &wipe_type);
    M_BindVariable("automap_overlay",          &overlay_trigger);
    M_BindVariable("show_timer",               &timer_info);
    M_BindVariable("replace",                  &replace_missing);
    M_BindVariable("goreamount",               &gore_amount);
    M_BindVariable("random_pitch",             &randompitch);
    M_BindVariable("telefrag",                 &d_telefrag);
    M_BindVariable("doorstuck",                &d_doorstuck);
    M_BindVariable("resurrect_ghosts",         &d_resurrectghosts);
    M_BindVariable("limited_ghosts",           &d_limitedghosts);
    M_BindVariable("block_skulls",             &d_blockskulls);
    M_BindVariable("blazing_sound",            &d_blazingsound);
    M_BindVariable("god",                      &d_god);
    M_BindVariable("floors",                   &d_floors);
    M_BindVariable("model",                    &d_model);
    M_BindVariable("hell",                     &d_666);
    M_BindVariable("masked_anim",              &d_maskedanim);
    M_BindVariable("sound",                    &d_sound);
    M_BindVariable("ouchface",                 &d_ouchface);
    M_BindVariable("authors",                  &show_authors);
    M_BindVariable("png_screenshot",           &png_screenshots);
    M_BindVariable("menu_type",                &background_type);
    M_BindVariable("menu_shadow",              &font_shadow);
    M_BindVariable("shadows",                  &d_shadows);
    M_BindVariable("offsets",                  &d_fixspriteoffsets);
    M_BindVariable("pixel_width",              &pixelwidth);
    M_BindVariable("pixel_height",             &pixelheight);
    M_BindVariable("brightmaps",               &d_brightmaps);
    M_BindVariable("screenwidth",              &screen_width);
    M_BindVariable("screenheight",             &screen_height);
    M_BindVariable("fixmaperrors",             &d_fixmaperrors);
    M_BindVariable("altlighting",              &d_altlighting);
    M_BindVariable("infighting",               &allow_infighting);
    M_BindVariable("lastenemy",                &last_enemy);
    M_BindVariable("floatitems",               &float_items);
    M_BindVariable("animate_dropping",         &animated_drop);
    M_BindVariable("crushing_sound",           &crush_sound);
    M_BindVariable("no_noise",                 &disable_noise);
    M_BindVariable("nudge_corpses",            &corpses_nudge);
    M_BindVariable("slide_corpses",            &corpses_slide);
    M_BindVariable("smearblood_corpses",       &corpses_smearblood);
    M_BindVariable("diskicon",                 &show_diskicon);
    M_BindVariable("samplerate",               &use_libsamplerate);
    M_BindVariable("mouse_walk",               &mousewalk);
    M_BindVariable("generalsound",             &general_sound);
    M_BindVariable("icon_type",                &icontype);
    M_BindVariable("colored_player_corpses",   &randomly_colored_playercorpses);
    M_BindVariable("endoom_screen",            &show_endoom);
    M_BindVariable("low_health",               &lowhealth);
    M_BindVariable("wiggle_fix",               &d_fixwiggle);
    M_BindVariable("mouse_sensitivity",        &mouseSensitivity);
    M_BindVariable("slime_trails",             &remove_slime_trails);
    M_BindVariable("max_bloodsplats",          &r_bloodsplats_max);
    M_BindVariable("center_weapon",            &d_centerweapon);
    M_BindVariable("eject_casings",            &d_ejectcasings);
    M_BindVariable("status_map",               &d_statusmap);
    M_BindVariable("show_maptitle",            &show_title);
    M_BindVariable("rendermode",               &render_mode);
    M_BindVariable("particles",                &d_drawparticles);
    M_BindVariable("bloodparticles",           &bloodsplat_particle);
    M_BindVariable("bulletparticles",          &bulletpuff_particle);
    M_BindVariable("bfgcloud",                 &d_drawbfgcloud);
    M_BindVariable("rockettrails",             &d_drawrockettrails);
    M_BindVariable("rocketexplosions",         &d_drawrocketexplosions);
    M_BindVariable("bfgexplosions",            &d_drawbfgexplosions);
    M_BindVariable("tele_particle",            &teleport_particle);
    M_BindVariable("spawn_flies",              &d_spawnflies);
    M_BindVariable("drip_blood",               &d_dripblood);
    M_BindVariable("vsync",                    &d_vsync);
    M_BindVariable("aimhelp",                  &aiming_help);
    M_BindVariable("particle_sound",           &particle_sounds);
    M_BindVariable("spawn_teleport_glitter",   &d_spawnteleglit);
    M_BindVariable("lost_soul_bounce",         &correct_lost_soul_bounce);
    M_BindVariable("window_position_x",        &window_pos_x);
    M_BindVariable("window_position_y",        &window_pos_y);
    M_BindVariable("map_color_background",     &mapcolor_back);
    M_BindVariable("map_color_grid",           &mapcolor_grid);
    M_BindVariable("map_color_wall",           &mapcolor_wall);
    M_BindVariable("map_color_floorchange",    &mapcolor_fchg);
    M_BindVariable("map_color_ceilingchange",  &mapcolor_cchg);
    M_BindVariable("map_color_ceilingatfloor", &mapcolor_clsd);
    M_BindVariable("map_color_redkey",         &mapcolor_rkey);
    M_BindVariable("map_color_bluekey",        &mapcolor_bkey);
    M_BindVariable("map_color_yellowkey",      &mapcolor_ykey);
    M_BindVariable("map_color_reddoor",        &mapcolor_rdor);
    M_BindVariable("map_color_bluedoor",       &mapcolor_bdor);
    M_BindVariable("map_color_yellowdoor",     &mapcolor_ydor);
    M_BindVariable("map_color_teleport",       &mapcolor_tele);
    M_BindVariable("map_color_secret",         &mapcolor_secr);
    M_BindVariable("map_color_exit",           &mapcolor_exit);
    M_BindVariable("map_color_unseen",         &mapcolor_unsn);
    M_BindVariable("map_color_flat",           &mapcolor_flat);
    M_BindVariable("map_color_sprite",         &mapcolor_sprt);
    M_BindVariable("map_color_item",           &mapcolor_item);
    M_BindVariable("map_color_enemy",          &mapcolor_enemy);
    M_BindVariable("map_color_crosshair",      &mapcolor_hair);
    M_BindVariable("map_color_single",         &mapcolor_sngl);
    M_BindVariable("map_color_player",         &mapcolor_plyr);
    M_BindVariable("map_gridsize",             &map_grid_size);
    M_BindVariable("map_secrets_after",        &map_secret_after);
    M_BindVariable("use_autosave",             &enable_autosave);
    M_BindVariable("draw_splash",              &drawsplash);

#ifdef WII

    M_BindVariable("key_shoot",                &joy_r);
    M_BindVariable("key_open",                 &joy_l);
    M_BindVariable("key_menu",                 &joy_minus);
    M_BindVariable("key_weapon_left",          &joy_left);
    M_BindVariable("key_automap",              &joy_down);
    M_BindVariable("key_weapon_right",         &joy_right);
    M_BindVariable("key_automap_zoom_in",      &joy_zl);
    M_BindVariable("key_automap_zoom_out",     &joy_zr);
    M_BindVariable("key_flyup",                &joy_a);
    M_BindVariable("key_flydown",              &joy_y);
    M_BindVariable("key_jump",                 &joy_b);
    M_BindVariable("key_run",                  &joy_1);
    M_BindVariable("key_console",              &joy_2);
    M_BindVariable("key_screenshots",          &joy_x);

#else

    M_BindVariable("key_shoot",                &key_fire);
    M_BindVariable("key_open",                 &key_use);
    M_BindVariable("key_menu",                 &key_menu_activate);
    M_BindVariable("key_weapon_left",          &key_prevweapon);
    M_BindVariable("key_automap",              &key_map_toggle);
    M_BindVariable("key_weapon_right",         &key_nextweapon);
    M_BindVariable("key_automap_zoom_in",      &key_map_zoomin);
    M_BindVariable("key_automap_zoom_out",     &key_map_zoomout);
    M_BindVariable("key_flyup",                &key_flyup);
    M_BindVariable("key_flydown",              &key_flydown);
    M_BindVariable("key_jump",                 &key_jump);
    M_BindVariable("key_run",                  &key_speed);
    M_BindVariable("key_console",              &key_console);
    M_BindVariable("key_screenshots",          &key_menu_screenshot);
    M_BindVariable("key_strafe_left",          &key_strafeleft);
    M_BindVariable("key_strafe_right",         &key_straferight);

#endif
}

//
// Apply custom patches to the default values depending on the
// platform we are running on.
//
// [nitr8] UNUSED
//
/*
void M_ApplyPlatformDefaults(void)
{
    // no-op. Add your platform-specific patches here.
}
*/

