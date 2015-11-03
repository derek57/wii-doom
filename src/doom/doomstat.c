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
GameMode_t      gamemode = indetermined;
GameMission_t   gamemission = doom;
GameVersion_t   gameversion = exe_final2;

char            *gamedescription;

// Set if homebrew PWAD stuff has been added.
boolean         modifiedgame;
boolean         am_overlay = false;
boolean         nerve_pwad = false;
boolean         master_pwad = false;

boolean         d_recoil = false;
boolean         d_maxgore = false;
boolean         d_thrust = false;
boolean         respawnparm = false;       // checkparm of -respawn
boolean         fastparm = false;          // checkparm of -fast
boolean         d_footstep = false;
boolean         d_footclip = false;
boolean         d_splash = false;
boolean         beta_bfg = false;
boolean         beta_skulls = false;
boolean         beta_plasma = false;
boolean         beta_imp = false;
boolean         d_translucency = false;
boolean         d_chkblood = false;
boolean         d_chkblood2 = false;
boolean         d_uncappedframerate = false;
boolean         d_flipcorpses = false;
boolean         d_secrets = false;
boolean         beta_style = false;
boolean         beta_style_mode = false;
boolean         smoketrails = false;
boolean         sound_info = false;
boolean         autodetect_hom = false;
boolean         d_fallingdamage = false;
boolean         d_infiniteammo = false;
boolean         not_monsters = false;
boolean         overlay_trigger = false;
boolean         replace_missing = false;
boolean         d_telefrag = false;
boolean         d_doorstuck = false;
boolean         d_resurrectghosts = false;
boolean         d_limitedghosts = false;
boolean         d_blockskulls = false;
boolean         d_blazingsound = false;
boolean         d_god = true;
boolean         d_floors = false;
boolean         d_moveblock = false;
boolean         d_model = false;
boolean         d_666 = false;
boolean         d_maskedanim = false;
boolean         d_sound = false;
boolean         d_ouchface = false;
boolean         show_authors = false;
boolean         font_shadow = false;
boolean         d_shadows = false;
boolean         d_fixspriteoffsets = false;
boolean         d_brightmaps = false;
boolean         d_fixmaperrors = false;
boolean         d_altlighting = false;
boolean         allow_infighting = false;
boolean         last_enemy = false;
boolean         float_items = false;
boolean         animated_drop = false;
boolean         crush_sound = false;
boolean         disable_noise = false;
boolean         corpses_nudge = false;
boolean         corpses_slide = false;
boolean         corpses_smearblood = false;
boolean         show_diskicon = true;
/*
boolean         nerve = false;
boolean         chex = false;
*/
boolean         chexdeh = false;
//boolean         hacx = false;
boolean         BTSX = false;
boolean         BTSXE1 = false;
boolean         BTSXE2 = false;
boolean         BTSXE2A = false;
boolean         BTSXE2B = false;
boolean         BTSXE3 = false;
boolean         BTSXE3A = false;
boolean         BTSXE3B = false;

int             d_colblood = 0;
int             d_colblood2 = 0;
int             d_swirl;
int             autoaim = false;
int             background_type = 1;

