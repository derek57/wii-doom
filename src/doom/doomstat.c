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
//        Put all global tate variables here.
//
//-----------------------------------------------------------------------------

#include <stdio.h>

#include "doomstat.h"


// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t       gamemode = indetermined;
GameMission_t    gamemission = doom;
GameVersion_t    gameversion = exe_final2;

char             *gamedescription;

// Show messages has default, 0 = off, 1 = on
int              showMessages = 1;        

// specifies whether to follow the player around
int              drawgrid = 0;
int              followplayer = 1;
int              show_stats = 0;
int              timer_info = 0;
int              use_vanilla_weapon_change = 1;
int              chaingun_tics = 4;
int              crosshair = 0;

// Set if homebrew PWAD stuff has been added.
dboolean         modifiedgame;
dboolean         start_respawnparm;
dboolean         start_fastparm;
dboolean         nomonsters;
dboolean         hud;
dboolean         randompitch;
dboolean         opl_stereo_correct;
dboolean         display_ticker;
dboolean         memory_usage;

dboolean         am_overlay = false;
dboolean         nerve_pwad = false;
dboolean         master_pwad = false;

dboolean         remove_slime_trails = false;
dboolean         d_fixwiggle = false;
dboolean         lowhealth = false;
dboolean         d_recoil = false;
dboolean         d_maxgore = false;
dboolean         d_thrust = false;
dboolean         respawnparm = false;
dboolean         fastparm = false;
dboolean         d_footstep = false;
dboolean         d_footclip = false;
dboolean         d_splash = false;
dboolean         d_translucency = false;
dboolean         d_chkblood = false;
dboolean         d_chkblood2 = false;
dboolean         d_flipcorpses = false;
dboolean         d_secrets = false;
dboolean         beta_style = false;
dboolean         beta_style_mode = false;
dboolean         smoketrails = false;
dboolean         sound_info = false;
dboolean         autodetect_hom = false;
dboolean         d_fallingdamage = false;
dboolean         d_infiniteammo = false;
dboolean         not_monsters = false;
dboolean         overlay_trigger = false;
dboolean         replace_missing = false;
dboolean         d_telefrag = true;
dboolean         d_doorstuck = false;
dboolean         d_resurrectghosts = false;
dboolean         d_limitedghosts = true;
dboolean         d_blockskulls = false;
dboolean         d_blazingsound = false;
dboolean         d_god = true;
dboolean         d_floors = false;
dboolean         d_moveblock = false;
dboolean         d_model = false;
dboolean         d_666 = false;
dboolean         d_maskedanim = false;
dboolean         d_sound = false;
dboolean         d_ouchface = false;
dboolean         show_authors = false;
dboolean         d_shadows = false;
dboolean         d_fixspriteoffsets = false;
dboolean         d_brightmaps = false;
dboolean         d_fixmaperrors = false;
dboolean         d_altlighting = false;
dboolean         allow_infighting = false;
dboolean         last_enemy = false;
dboolean         float_items = false;
dboolean         animated_drop = false;
dboolean         crush_sound = false;
dboolean         disable_noise = false;
dboolean         corpses_nudge = false;
dboolean         corpses_slide = false;
dboolean         corpses_smearblood = false;
dboolean         show_diskicon = true;
dboolean         randomly_colored_playercorpses = false;
dboolean         mousewalk = false;
dboolean         am_rotate = false;
dboolean         jumping = false;
dboolean         general_sound = true;
dboolean         d_centerweapon = false;
dboolean         d_ejectcasings = false;
dboolean         d_statusmap = true;
dboolean         show_title = true;
dboolean         dump_mem = false;
dboolean         dump_con = false;
dboolean         dump_stat = false;
dboolean         printdir = false;
dboolean         d_drawparticles = false;
dboolean         d_drawbfgcloud = false;
dboolean         d_drawrockettrails = false;
dboolean         d_drawrocketexplosions = false;
dboolean         d_drawbfgexplosions = false;
dboolean         d_spawnflies = false;
dboolean         d_dripblood = false;
dboolean         d_vsync = false;
dboolean         particle_sounds = false;
dboolean         map_secret_after = false;
dboolean         enable_autosave = false;
/*
dboolean         nerve = false;
dboolean         chex = false;
*/
dboolean         chexdeh = false;
//dboolean         hacx = false;
dboolean         BTSX = false;
dboolean         BTSXE1 = false;
dboolean         BTSXE2 = false;
dboolean         BTSXE2A = false;
dboolean         BTSXE2B = false;
dboolean         BTSXE3 = false;
dboolean         BTSXE3A = false;
dboolean         BTSXE3B = false;

fixed_t          forwardmove = 29;
fixed_t          sidemove = 24; 

int              turnspeed = 7;

// Blocky mode, has default, 0 = high, 1 = normal
int              detailLevel = 0;

// temp for screenblocks (0-9)
int              screenSize;
int              screenblocks = 9;

// Gamma correction level to use
int              usegamma = 10;

int              d_colblood = 0;
int              d_colblood2 = 0;
int              d_swirl;
int              autoaim = true;
int              background_type = 0;
int              icontype = 0;
int              wipe_type = 2;
int              mouselook;
int              mspeed = 2;
int              mus_engine = 1;
int              snd_module = 0;
int              snd_chans = 1;
int              sound_channels = 8;
int              opl_type = 0;
int              use_libsamplerate = 0;
int              gore_amount = 1;
int              display_fps = 0;
int              font_shadow = 0;
#ifdef WII
int              show_endoom = 0;
#else
int              show_endoom = 1;
#endif
int              render_mode = 2;
int              bloodsplat_particle = 0;
int              bulletpuff_particle = 0;
int              teleport_particle = 0;
int              d_uncappedframerate = 0;
int              d_spawnteleglit = 0;

// defaulted values
int              mouseSensitivity = 5;


//jff 1/7/98 default automap colors added
int              mapcolor_back = 247;  // map background
int              mapcolor_grid = 104;  // grid lines color
int              mapcolor_wall = 23;   // normal 1s wall color
int              mapcolor_fchg = 55;   // line at floor height change color
int              mapcolor_cchg = 215;  // line at ceiling height change color
int              mapcolor_clsd = 208;  // line at sector with floor=ceiling color
int              mapcolor_rkey = 175;  // red key color
int              mapcolor_bkey = 204;  // blue key color
int              mapcolor_ykey = 231;  // yellow key color
int              mapcolor_rdor = 175;  // red door color  (diff from keys to allow option)
int              mapcolor_bdor = 204;  // blue door color (of enabling one but not other)
int              mapcolor_ydor = 231;  // yellow door color
int              mapcolor_tele = 119;  // teleporter line color
int              mapcolor_secr = 252;  // secret sector boundary color
int              mapcolor_exit = 0;    // jff 4/23/98 add exit line color
int              mapcolor_unsn = 104;  // computer map unseen line color
int              mapcolor_flat = 88;   // line with no floor/ceiling changes
int              mapcolor_sprt = 112;  // general sprite color
int              mapcolor_item = 231;  // item sprite color
int              mapcolor_enemy = 177; // enemy sprite color
int              mapcolor_hair = 208;  // crosshair color
int              mapcolor_sngl = 208;  // single player arrow color
int              mapcolor_plyr = 112;  // color for player arrow
int              map_grid_size = 128;

