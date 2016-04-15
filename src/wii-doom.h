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
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include
//    this header file.
//   In practice, things are a bit messy.
//
//-----------------------------------------------------------------------------


#ifndef __WIID_STATE__
#define __WIID_STATE__

#include "doom/doomdef.h"

extern float             libsamplerate_scale;

extern char              *net_player_name;
extern char              *pwadfile;
extern char              *window_title;
extern char              *window_position;

// ------------------------
// Command line parameters.
//

extern  dboolean         png_screenshots;

// checkparm of -nomonsters
extern  dboolean         nomonsters;

// checkparm of -respawn
extern  dboolean         respawnparm;

// checkparm of -fast
extern  dboolean         fastparm;

// MAIN DEVPARM
extern  dboolean         devparm;
extern  dboolean         devparm_net;

// NETWORK DEVPARM
extern  dboolean         devparm_net_nerve;
extern  dboolean         devparm_net_doom;
extern  dboolean         devparm_net_doom2;
extern  dboolean         devparm_net_freedoom2;
extern  dboolean         devparm_net_tnt;
extern  dboolean         devparm_net_plutonia;
extern  dboolean         devparm_net_chex;
extern  dboolean         devparm_net_hacx;

// SOLO DEVPARM
extern  dboolean         devparm_nerve;
extern  dboolean         devparm_master;
extern  dboolean         devparm_doom;
extern  dboolean         devparm_doom2;
extern  dboolean         devparm_freedoom2;
extern  dboolean         devparm_tnt;
extern  dboolean         devparm_plutonia;
extern  dboolean         devparm_chex;
extern  dboolean         devparm_hacx;
extern  dboolean         autostart;

// If true, we're using one of the mangled BFG edition IWADs.
extern  dboolean         bfgedition;
extern  dboolean         nerve_pwad;
extern  dboolean         master_pwad;

// Set if homebrew PWAD stuff has been added.
extern  dboolean         modifiedgame;

// Nightmare mode flag, single player.
extern  dboolean         respawnmonsters;

// Netgame? Only true if >1 player.
extern  dboolean         netgame;

// 0 = Cooperative; 1 = Deathmatch; 2 = Altdeath
extern  dboolean         deathmatch;

// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern  dboolean         statusbaractive;

// In AutoMap mode?
extern  dboolean         automapactive;

// Menu overlayed?
extern  dboolean         menuactive;

// Game Pause?
extern  dboolean         paused;


extern  dboolean         viewactive;

extern  dboolean         nodrawers;

// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern  dboolean         usergame;

//?
extern  dboolean         demoplayback;
extern  dboolean         demorecording;

// Quit after playing a demo from cmdline.
extern  dboolean         singledemo;        

// Alive? Disconnected?
extern  dboolean         playeringame[MAXPLAYERS];

// if true, load all graphics at level load
extern  dboolean         precache;

extern  dboolean         am_overlay;
extern  dboolean         memory_usage;
extern  dboolean         remove_slime_trails;
extern  dboolean         d_fixwiggle;
extern  dboolean         lowhealth;
extern  dboolean         am_rotate;
extern  dboolean         hud;
extern  dboolean         d_recoil;
extern  dboolean         start_respawnparm;
extern  dboolean         start_fastparm;
extern  dboolean         d_maxgore;
extern  dboolean         d_thrust;
extern  dboolean         d_footstep;
extern  dboolean         d_footclip;
extern  dboolean         d_splash;
extern  dboolean         d_translucency;
extern  dboolean         d_chkblood;
extern  dboolean         d_chkblood2;
extern  dboolean         d_flipcorpses;
extern  dboolean         d_secrets;
extern  dboolean         beta_style;
extern  dboolean         beta_style_mode;
extern  dboolean         smoketrails;
extern  dboolean         sound_info;
extern  dboolean         autodetect_hom;
extern  dboolean         d_fallingdamage;
extern  dboolean         d_infiniteammo;
extern  dboolean         not_monsters;
extern  dboolean         overlay_trigger;
extern  dboolean         replace_missing;
extern  dboolean         d_telefrag;
extern  dboolean         d_doorstuck;
extern  dboolean         d_resurrectghosts;
extern  dboolean         d_limitedghosts;
extern  dboolean         d_blockskulls;
extern  dboolean         d_blazingsound;
extern  dboolean         d_god;
extern  dboolean         d_floors;
extern  dboolean         d_moveblock;
extern  dboolean         d_model;
extern  dboolean         d_666;
extern  dboolean         d_maskedanim;
extern  dboolean         d_sound;
extern  dboolean         d_ouchface;
extern  dboolean         show_authors;
extern  dboolean         d_shadows;
extern  dboolean         d_fixspriteoffsets;
extern  dboolean         d_brightmaps;
extern  dboolean         d_fixmaperrors;
extern  dboolean         d_altlighting;
extern  dboolean         allow_infighting;
extern  dboolean         last_enemy;
extern  dboolean         float_items;
extern  dboolean         animated_drop;
extern  dboolean         crush_sound;
extern  dboolean         disable_noise;
extern  dboolean         corpses_nudge;
extern  dboolean         corpses_slide;
extern  dboolean         corpses_smearblood;
extern  dboolean         show_diskicon;
extern  dboolean         randomly_colored_playercorpses;
extern  dboolean         mousewalk;
extern  dboolean         jumping;
extern  dboolean         opl_stereo_correct;
extern  dboolean         randompitch;
extern  dboolean         general_sound;
extern  dboolean         display_ticker;
extern  dboolean         d_centerweapon;
extern  dboolean         d_ejectcasings;
extern  dboolean         d_statusmap;
extern  dboolean         show_title;
extern  dboolean         dump_mem;
extern  dboolean         dump_con;
extern  dboolean         dump_stat;
extern  dboolean         printdir;
extern  dboolean         d_drawparticles;
extern  dboolean         d_drawbfgcloud;
extern  dboolean         d_drawrockettrails;
extern  dboolean         d_drawrocketexplosions;
extern  dboolean         d_drawbfgexplosions;
extern  dboolean         d_spawnflies;
extern  dboolean         d_dripblood;
extern  dboolean         d_vsync;
extern  dboolean         particle_sounds;
extern  dboolean         map_secret_after;
extern  dboolean         enable_autoload;
extern  dboolean         enable_autosave;
extern  dboolean         drawsplash;
extern  dboolean         chexdeh;
extern  dboolean         BTSX;
extern  dboolean         BTSXE1;
extern  dboolean         BTSXE2;
extern  dboolean         BTSXE2A;
extern  dboolean         BTSXE2B;
extern  dboolean         BTSXE3;
extern  dboolean         BTSXE3A;
extern  dboolean         BTSXE3B;
extern  dboolean         drawgrid;
extern  dboolean         followplayer;
extern  dboolean         show_stats;
extern  dboolean         crosshair;
extern  dboolean         use_vanilla_weapon_change;
extern  dboolean         autoaim;
extern  dboolean         detailLevel;
extern  dboolean         showMessages;
extern  dboolean         render_mode;
extern  dboolean         d_fliplevels;
extern  dboolean         show_endoom;
extern  dboolean         display_fps;
extern  dboolean         d_colblood;
extern  dboolean         d_colblood2;
extern  dboolean         d_swirl;
extern  dboolean         timer_info;
extern  dboolean         icontype;
extern  dboolean         snd_module;
extern  dboolean         opl_type;
extern  dboolean         s_randommusic;
extern  dboolean         aiming_help;
extern  dboolean         correct_lost_soul_bounce;

extern  char             *gamedescription;

//-----------------------------------------
// Internal parameters, used for engine.
//

// File handling stuff.
extern  char             *savegamedir;
extern  char             basedefault[1024];

extern  int              startup_delay;
extern  int              screen_bpp;
extern  int              fullscreen;
extern  int              aspect_ratio_correct;
extern  int              autoadjust_video_settings;
extern  int              gametic;

extern  int              startepisode;
extern  int              startmap;

// Savegame slot to load on startup.  This is the value provided to
// the -loadgame option.  If this has not been provided, this is -1.

extern  int              startloadgame;
extern  int              gameepisode;
extern  int              gamemap;

// If non-zero, exit the level after this number of minutes
extern  int              timelimit;

// -------------------------
// Internal parameters for sound rendering.
// These have been taken from the DOS version,
//  but are not (yet) supported with Linux
//  (e.g. no sound volume adjustment with menu.

// From m_menu.c:
//  Sound FX volume has default, 0 - 15
//  Music volume has default, 0 - 15
// These are multiplied by 8.
extern  int              sfxVolume;
extern  int              musicVolume;

// Current music/sfx card - index useless
//  w/o a reference LUT in a sound module.
// Ideally, this would use indices found
//  in: /usr/include/linux/soundcard.h
extern  int              snd_MusicDevice;
extern  int              snd_SfxDevice;

// Config file? Same disclaimer as above.
extern  int              snd_DesiredMusicDevice;
extern  int              snd_DesiredSfxDevice;

// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
extern  int              viewangleoffset;

// -------------------------------------
// Scores, rating.
// Statistics on a given map, for intermission.
//
extern  int              totalkills;
extern  int              totalitems;
extern  int              totalsecret;

// Timer, for scores.

// gametic at level start
extern  int              levelstarttic;

// tics in game play for par
extern  int              leveltime;

extern  int              mouseSensitivity;

extern  int              bodyqueslot;

// Needed to store the number of the dummy sky flat.
// Used for rendering,
//  as well as tracking projectiles etc.
extern  int              skyflatnum;

// Netgame stuff (buffers and pointers, i.e. indices).
extern  int              rndindex;

extern  int              turnspeed;
extern  int              background_type;
extern  int              chaingun_tics;
extern  int              wipe_type;
extern  int              usegamma;
extern  int              screenSize;
extern  int              screenblocks;
extern  int              mouselook;
extern  int              mspeed;
extern  int              mus_engine;
extern  int              snd_chans;
extern  int              sound_channels;
extern  int              use_libsamplerate;
extern  int              gore_amount;
extern  int              font_shadow;
extern  int              bloodsplat_particle;
extern  int              bulletpuff_particle;
extern  int              teleport_particle;
extern  int              d_uncappedframerate;
extern  int              d_spawnteleglit;
extern  int              map_grid_size;
extern  int              stillbob;
extern  int              movebob;
extern  int              background_color;

// map background
extern  int              mapcolor_back;

// grid lines color
extern  int              mapcolor_grid;

// normal 1s wall color
extern  int              mapcolor_wall;

// line at floor height change color
extern  int              mapcolor_fchg;

// line at ceiling height change color
extern  int              mapcolor_cchg;

// line at sector with floor = ceiling color
extern  int              mapcolor_clsd;

// red key color
extern  int              mapcolor_rkey;

// blue key color
extern  int              mapcolor_bkey;

// yellow key color
extern  int              mapcolor_ykey;

// red door color (diff from keys to allow option)
extern  int              mapcolor_rdor;

// blue door color (of enabling one but not other)
extern  int              mapcolor_bdor;

// yellow door color
extern  int              mapcolor_ydor;

// teleporter line color
extern  int              mapcolor_tele;

// secret sector boundary color
extern  int              mapcolor_secr;

// jff 4/23/98 add exit line color
extern  int              mapcolor_exit;

// computer map unseen line color
extern  int              mapcolor_unsn;

// line with no floor / ceiling changes
extern  int              mapcolor_flat;

// general sprite color
extern  int              mapcolor_sprt;

// item sprite color
extern  int              mapcolor_item;

// enemy sprite color
extern  int              mapcolor_enemy;

// crosshair color
extern  int              mapcolor_hair;

// single player arrow color
extern  int              mapcolor_sngl;

// color for player arrow
extern  int              mapcolor_plyr[4];

// player taking events and displaying 
extern  int              consoleplayer;

// view being displayed 
extern  int              displayplayer;

#ifndef WII
extern  int              joybfire;
extern  int              joybstrafe;
extern  int              joybuse;
extern  int              joybnextweapon;
extern  int              joybspeed;
extern  int              joybjump;
#endif

extern  int              window_pos_x;
extern  int              window_pos_y;
extern  int              screen_width;
extern  int              screen_height;

extern  int              pixelwidth;
extern  int              pixelheight;
extern  int              runcount;
extern  int              r_bloodsplats_max;

#endif

