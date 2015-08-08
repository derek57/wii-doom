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
boolean         d_god = false;
boolean         d_floors = false;
boolean         d_moveblock = false;
boolean         d_model = false;
boolean         d_666 = false;
boolean         d_maskedanim = false;
boolean         d_sound = false;
boolean         d_ouchface = false;
boolean         show_authors = false;

int             d_colblood = 0;
int             d_colblood2 = 0;
int             d_swirl;
int             autoaim = false;

