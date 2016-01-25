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
dboolean         nomonsters;     // checkparm of -nomonsters
dboolean         hud;
dboolean         randompitch;
dboolean         swap_sound_chans;
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
dboolean         respawnparm = false;       // checkparm of -respawn
dboolean         fastparm = false;          // checkparm of -fast
dboolean         d_footstep = false;
dboolean         d_footclip = false;
dboolean         d_splash = false;
dboolean         d_translucency = false;
dboolean         d_chkblood = false;
dboolean         d_chkblood2 = false;
dboolean         d_uncappedframerate = false;
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
int              show_endoom = 1;
int              render_mode = 1;

// defaulted values
int              mouseSensitivity = 5;


