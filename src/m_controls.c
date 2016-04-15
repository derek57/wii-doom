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
#include "wii-doom.h"


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
int key_menu_screenshot = 0;
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
int joybprevweapon = -1;

#ifndef WII
int joybmenu = -1;
#endif

// Multiplayer chat keys:
int key_multi_msg = 't';
int key_multi_msgplayer[8];
int key_message_refresh = KEY_ENTER;

// Bind all of the common controls used by Doom and all other games.

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
    M_BindIntVariable("sfx_volume",                   &sfxVolume);
    M_BindIntVariable("music_volume",                 &musicVolume);
    M_BindIntVariable("screensize",                   &screenblocks);
    M_BindIntVariable("walking_speed",                &forwardmove);
    M_BindIntVariable("turning_speed",                &turnspeed);
    M_BindIntVariable("strafing_speed",               &sidemove);
    M_BindIntVariable("freelook_speed",               &mspeed);
    M_BindIntVariable("use_gamma",                    &usegamma);
    M_BindIntVariable("mouse_look",                   &mouselook);
    M_BindBooleanVariable("map_grid",                 &drawgrid);
    M_BindBooleanVariable("follow_player",            &followplayer);
    M_BindBooleanVariable("showstats",                &show_stats);
    M_BindBooleanVariable("show_messages",            &showMessages);
    M_BindBooleanVariable("map_rotate",               &am_rotate);
    M_BindBooleanVariable("detail",                   &detailLevel);
    M_BindBooleanVariable("vanilla_weapon_change",    &use_vanilla_weapon_change);
    M_BindBooleanVariable("xhair",                    &crosshair);
    M_BindBooleanVariable("jump",                     &jumping);
    M_BindIntVariable("music_engine",                 &mus_engine);
    M_BindBooleanVariable("recoil",                   &d_recoil);
    M_BindBooleanVariable("monsters_respawn",         &start_respawnparm);
    M_BindBooleanVariable("fast_monsters",            &start_fastparm);
    M_BindBooleanVariable("auto_aim",                 &autoaim);
    M_BindBooleanVariable("max_gore",                 &d_maxgore);
    M_BindBooleanVariable("extra_hud",                &hud);
    M_BindBooleanVariable("switch_chans",             &opl_stereo_correct);
    M_BindBooleanVariable("player_thrust",            &d_thrust);
    M_BindIntVariable("run_count",                    &runcount);
    M_BindBooleanVariable("footsteps",                &d_footstep);
    M_BindBooleanVariable("footclip",                 &d_footclip);
    M_BindBooleanVariable("splash",                   &d_splash);
    M_BindBooleanVariable("swirl",                    &d_swirl);

#ifdef WII
    M_BindBooleanVariable("pr_beta",                  &beta_style_mode);
#endif

    M_BindBooleanVariable("translucency",             &d_translucency);
    M_BindBooleanVariable("colored_blood",            &d_colblood);
    M_BindBooleanVariable("fixed_blood",              &d_colblood2);
    M_BindBooleanVariable("mirrored_corpses",         &d_flipcorpses);
    M_BindBooleanVariable("show_secrets",             &d_secrets);
    M_BindIntVariable("uncapped_framerate",           &d_uncappedframerate);
    M_BindBooleanVariable("opltype",                  &opl_type);
    M_BindBooleanVariable("trails",                   &smoketrails);
    M_BindIntVariable("chaingun_speed",               &chaingun_tics);
    M_BindBooleanVariable("sound_type",               &snd_module);
    M_BindIntVariable("sound_chans",                  &sound_channels);
    M_BindBooleanVariable("hom_detector",             &autodetect_hom);
    M_BindBooleanVariable("falling_damage",           &d_fallingdamage);
    M_BindBooleanVariable("infinite_ammo",            &d_infiniteammo);
    M_BindBooleanVariable("no_monsters",              &not_monsters);
    M_BindIntVariable("screenwipe_type",              &wipe_type);
    M_BindBooleanVariable("automap_overlay",          &overlay_trigger);
    M_BindBooleanVariable("show_timer",               &timer_info);
    M_BindBooleanVariable("replace",                  &replace_missing);
    M_BindIntVariable("goreamount",                   &gore_amount);
    M_BindBooleanVariable("random_pitch",             &randompitch);
    M_BindBooleanVariable("telefrag",                 &d_telefrag);
    M_BindBooleanVariable("doorstuck",                &d_doorstuck);
    M_BindBooleanVariable("resurrect_ghosts",         &d_resurrectghosts);
    M_BindBooleanVariable("limited_ghosts",           &d_limitedghosts);
    M_BindBooleanVariable("block_skulls",             &d_blockskulls);
    M_BindBooleanVariable("blazing_sound",            &d_blazingsound);
    M_BindBooleanVariable("god",                      &d_god);
    M_BindBooleanVariable("floors",                   &d_floors);
    M_BindBooleanVariable("model",                    &d_model);
    M_BindBooleanVariable("hell",                     &d_666);
    M_BindBooleanVariable("masked_anim",              &d_maskedanim);
    M_BindBooleanVariable("sound",                    &d_sound);
    M_BindBooleanVariable("ouchface",                 &d_ouchface);
    M_BindBooleanVariable("authors",                  &show_authors);
    M_BindBooleanVariable("png_screenshot",           &png_screenshots);
    M_BindIntVariable("menu_type",                    &background_type);
    M_BindIntVariable("menu_shadow",                  &font_shadow);
    M_BindBooleanVariable("shadows",                  &d_shadows);
    M_BindBooleanVariable("offsets",                  &d_fixspriteoffsets);
    M_BindIntVariable("pixel_width",                  &pixelwidth);
    M_BindIntVariable("pixel_height",                 &pixelheight);
    M_BindBooleanVariable("brightmaps",               &d_brightmaps);
    M_BindIntVariable("screenwidth",                  &screen_width);
    M_BindIntVariable("screenheight",                 &screen_height);
    M_BindBooleanVariable("fixmaperrors",             &d_fixmaperrors);
    M_BindBooleanVariable("altlighting",              &d_altlighting);
    M_BindBooleanVariable("infighting",               &allow_infighting);
    M_BindBooleanVariable("lastenemy",                &last_enemy);
    M_BindBooleanVariable("floatitems",               &float_items);
    M_BindBooleanVariable("animate_dropping",         &animated_drop);
    M_BindBooleanVariable("crushing_sound",           &crush_sound);
    M_BindBooleanVariable("no_noise",                 &disable_noise);
    M_BindBooleanVariable("nudge_corpses",            &corpses_nudge);
    M_BindBooleanVariable("slide_corpses",            &corpses_slide);
    M_BindBooleanVariable("smearblood_corpses",       &corpses_smearblood);
    M_BindBooleanVariable("diskicon",                 &show_diskicon);
    M_BindIntVariable("samplerate",                   &use_libsamplerate);
    M_BindBooleanVariable("mouse_walk",               &mousewalk);
    M_BindBooleanVariable("generalsound",             &general_sound);
    M_BindBooleanVariable("icon_type",                &icontype);
    M_BindBooleanVariable("colored_player_corpses",   &randomly_colored_playercorpses);
    M_BindBooleanVariable("endoom_screen",            &show_endoom);
    M_BindBooleanVariable("low_health",               &lowhealth);
    M_BindBooleanVariable("wiggle_fix",               &d_fixwiggle);
    M_BindIntVariable("mouse_sensitivity",            &mouseSensitivity);
    M_BindBooleanVariable("slime_trails",             &remove_slime_trails);
    M_BindIntVariable("max_bloodsplats",              &r_bloodsplats_max);
    M_BindBooleanVariable("center_weapon",            &d_centerweapon);
    M_BindBooleanVariable("eject_casings",            &d_ejectcasings);
    M_BindBooleanVariable("status_map",               &d_statusmap);
    M_BindBooleanVariable("show_maptitle",            &show_title);
    M_BindBooleanVariable("rendermode",               &render_mode);
    M_BindBooleanVariable("particles",                &d_drawparticles);
    M_BindIntVariable("bloodparticles",               &bloodsplat_particle);
    M_BindIntVariable("bulletparticles",              &bulletpuff_particle);
    M_BindBooleanVariable("bfgcloud",                 &d_drawbfgcloud);
    M_BindBooleanVariable("rockettrails",             &d_drawrockettrails);
    M_BindBooleanVariable("rocketexplosions",         &d_drawrocketexplosions);
    M_BindBooleanVariable("bfgexplosions",            &d_drawbfgexplosions);
    M_BindIntVariable("tele_particle",                &teleport_particle);
    M_BindBooleanVariable("spawn_flies",              &d_spawnflies);
    M_BindBooleanVariable("drip_blood",               &d_dripblood);
    M_BindBooleanVariable("vsync",                    &d_vsync);
    M_BindBooleanVariable("aimhelp",                  &aiming_help);
    M_BindBooleanVariable("particle_sound",           &particle_sounds);
    M_BindIntVariable("spawn_teleport_glitter",       &d_spawnteleglit);
    M_BindBooleanVariable("lost_soul_bounce",         &correct_lost_soul_bounce);
    M_BindIntVariable("window_position_x",            &window_pos_x);
    M_BindIntVariable("window_position_y",            &window_pos_y);
    M_BindIntVariable("map_color_background",         &mapcolor_back);
    M_BindIntVariable("map_color_grid",               &mapcolor_grid);
    M_BindIntVariable("map_color_wall",               &mapcolor_wall);
    M_BindIntVariable("map_color_floorchange",        &mapcolor_fchg);
    M_BindIntVariable("map_color_ceilingchange",      &mapcolor_cchg);
    M_BindIntVariable("map_color_ceilingatfloor",     &mapcolor_clsd);
    M_BindIntVariable("map_color_redkey",             &mapcolor_rkey);
    M_BindIntVariable("map_color_bluekey",            &mapcolor_bkey);
    M_BindIntVariable("map_color_yellowkey",          &mapcolor_ykey);
    M_BindIntVariable("map_color_reddoor",            &mapcolor_rdor);
    M_BindIntVariable("map_color_bluedoor",           &mapcolor_bdor);
    M_BindIntVariable("map_color_yellowdoor",         &mapcolor_ydor);
    M_BindIntVariable("map_color_teleport",           &mapcolor_tele);
    M_BindIntVariable("map_color_secret",             &mapcolor_secr);
    M_BindIntVariable("map_color_exit",               &mapcolor_exit);
    M_BindIntVariable("map_color_unseen",             &mapcolor_unsn);
    M_BindIntVariable("map_color_flat",               &mapcolor_flat);
    M_BindIntVariable("map_color_sprite",             &mapcolor_sprt);
    M_BindIntVariable("map_color_item",               &mapcolor_item);
    M_BindIntVariable("map_color_enemy",              &mapcolor_enemy);
    M_BindIntVariable("map_color_crosshair",          &mapcolor_hair);
    M_BindIntVariable("map_color_single",             &mapcolor_sngl);
    M_BindIntVariable("map_color_player",             &mapcolor_plyr[consoleplayer]);
    M_BindIntVariable("map_gridsize",                 &map_grid_size);
    M_BindBooleanVariable("map_secrets_after",        &map_secret_after);
    M_BindBooleanVariable("use_autosave",             &enable_autosave);
    M_BindBooleanVariable("draw_splash",              &drawsplash);
    M_BindIntVariable("menu_back",                    &background_color);
    M_BindBooleanVariable("precache_level",           &precache);
    M_BindIntVariable("still_bob",                    &stillbob);
    M_BindIntVariable("move_bob",                     &movebob);
    M_BindBooleanVariable("flip_levels",              &d_fliplevels);
    M_BindBooleanVariable("use_autoload",             &enable_autoload);
    M_BindBooleanVariable("random_music",             &s_randommusic);

    M_BindFloatVariable("samplerate_scale",           &libsamplerate_scale);

#ifdef WII

    M_BindIntVariable("key_shoot",                    &joy_r);
    M_BindIntVariable("key_open",                     &joy_l);
    M_BindIntVariable("key_menu",                     &joy_minus);
    M_BindIntVariable("key_weapon_left",              &joy_left);
    M_BindIntVariable("key_automap",                  &joy_down);
    M_BindIntVariable("key_weapon_right",             &joy_right);
    M_BindIntVariable("key_automap_zoom_in",          &joy_zl);
    M_BindIntVariable("key_automap_zoom_out",         &joy_zr);
    M_BindIntVariable("key_flyup",                    &joy_a);
    M_BindIntVariable("key_flydown",                  &joy_y);
    M_BindIntVariable("key_jump",                     &joy_b);
    M_BindIntVariable("key_run",                      &joy_1);
    M_BindIntVariable("key_console",                  &joy_2);
    M_BindIntVariable("key_screenshots",              &joy_x);

#else

    M_BindIntVariable("key_shoot",                    &key_fire);
    M_BindIntVariable("key_open",                     &key_use);
    M_BindIntVariable("key_menu",                     &key_menu_activate);
    M_BindIntVariable("key_weapon_left",              &key_prevweapon);
    M_BindIntVariable("key_automap",                  &key_map_toggle);
    M_BindIntVariable("key_weapon_right",             &key_nextweapon);
    M_BindIntVariable("key_automap_zoom_in",          &key_map_zoomin);
    M_BindIntVariable("key_automap_zoom_out",         &key_map_zoomout);
    M_BindIntVariable("key_flyup",                    &key_flyup);
    M_BindIntVariable("key_flydown",                  &key_flydown);
    M_BindIntVariable("key_jump",                     &key_jump);
    M_BindIntVariable("key_run",                      &key_speed);
    M_BindIntVariable("key_console",                  &key_console);
    M_BindIntVariable("key_screenshots",              &key_menu_screenshot);
    M_BindIntVariable("key_strafe_left",              &key_strafeleft);
    M_BindIntVariable("key_strafe_right",             &key_straferight);

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

void M_BindChatControls(unsigned int num_players)
{
    char name[32];  // haleyjd: 20 not large enough - Thank you, come again!
    unsigned int i; // haleyjd: signedness conflict

    M_BindIntVariable("key_multi_msg",     &key_multi_msg);

    for (i=0; i<num_players; ++i)
    {
        M_snprintf(name, sizeof(name), "key_multi_msgplayer%i", i + 1);
        M_BindIntVariable(name, &key_multi_msgplayer[i]);
    }
}

