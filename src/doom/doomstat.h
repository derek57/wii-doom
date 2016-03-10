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


#ifndef __D_STATE__
#define __D_STATE__

// We need globally shared data structures,
//  for defining the global state variables.
#include "doomdata.h"

#include "../d_loop.h"

// We need the playr data structure as well.
#include "d_player.h"

#include "../net_defs.h"


// ------------------------
// Command line parameters.
//
extern  dboolean         nomonsters;      // checkparm of -nomonsters
extern  dboolean         respawnparm;     // checkparm of -respawn
extern  dboolean         fastparm;        // checkparm of -fast

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


// -----------------------------------------------------
// Game Mode - identify IWAD as shareware, retail etc.
//
extern  GameMode_t       gamemode;
extern  GameMission_t    gamemission;
extern  GameVersion_t    gameversion;
extern  char             *gamedescription;

// If true, we're using one of the mangled BFG edition IWADs.
extern  dboolean         bfgedition;
extern  dboolean         nerve_pwad;
extern  dboolean         master_pwad;

// Convenience macro.
// 'gamemission' can be equal to pack_chex or pack_hacx, but these are
// just modified versions of doom and doom2, and should be interpreted
// as the same most of the time.

#define logical_gamemission                             \
    (gamemission == pack_chex ? doom :                  \
     gamemission == pack_hacx ? doom2 : gamemission)

// Set if homebrew PWAD stuff has been added.
extern  dboolean         modifiedgame;


// -------------------------------------------
// Selected skill type, map etc.
//

// Defaults for menu, methinks.
extern  skill_t          startskill;
extern  int              startepisode;
extern  int              startmap;

// Savegame slot to load on startup.  This is the value provided to
// the -loadgame option.  If this has not been provided, this is -1.

extern  int              startloadgame;

extern  dboolean         autostart;

// Selected by user. 
extern  skill_t          gameskill;
extern  int              gameepisode;
extern  int              gamemap;

// If non-zero, exit the level after this number of minutes
extern  int              timelimit;

// Nightmare mode flag, single player.
extern  dboolean         respawnmonsters;

// Netgame? Only true if >1 player.
extern  dboolean         netgame;

// 0=Cooperative; 1=Deathmatch; 2=Altdeath
extern  dboolean         deathmatch;

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


// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//  status bar explicitely.
extern  dboolean         statusbaractive;

extern  dboolean         automapactive;        // In AutoMap mode?
extern  dboolean         menuactive;        // Menu overlayed?
extern  dboolean         paused;                // Game Pause?


extern  dboolean         viewactive;

extern  dboolean         nodrawers;


// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
extern  int              viewangleoffset;

// Player taking events, and displaying.
extern  int              consoleplayer;        
extern  int              displayplayer;


// -------------------------------------
// Scores, rating.
// Statistics on a given map, for intermission.
//
extern  int              totalkills;
extern  int              totalitems;
extern  int              totalsecret;

// Timer, for scores.
extern  int              levelstarttic;        // gametic at level start
extern  int              leveltime;        // tics in game play for par



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




//?
extern  gamestate_t      gamestate;






//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//  according to user inputs. Partly load from
//  WAD, partly set at startup time.



// Bookkeeping on players - state.
extern  player_t         players[MAXPLAYERS];

// Alive? Disconnected?
extern  dboolean         playeringame[MAXPLAYERS];


// Player spawn spots for deathmatch.
#define MAX_DM_STARTS    10
extern  mapthing_t       deathmatchstarts[MAX_DM_STARTS];
extern  mapthing_t*      deathmatch_p;

// Player spawn spots.
extern  mapthing_t       playerstarts[MAXPLAYERS];

// Intermission stats.
// Parameters for world map / intermission.
extern  wbstartstruct_t  wminfo;        







//-----------------------------------------
// Internal parameters, used for engine.
//

// File handling stuff.
extern  char *           savegamedir;
extern  char             basedefault[1024];

// if true, load all graphics at level load
extern  dboolean         precache;


// wipegamestate can be set to -1
//  to force a wipe on the next draw
extern  gamestate_t      wipegamestate;

extern  int              mouseSensitivity;

extern  int              bodyqueslot;



// Needed to store the number of the dummy sky flat.
// Used for rendering,
//  as well as tracking projectiles etc.
extern  int              skyflatnum;



// Netgame stuff (buffers and pointers, i.e. indices).


extern  int              rndindex;

extern  ticcmd_t         *netcmds;

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
extern  dboolean         enable_autosave;
/*
extern  dboolean         nerve;
extern  dboolean         chex;
*/
extern  dboolean         chexdeh;
//extern  dboolean         hacx;
extern  dboolean         BTSX;
extern  dboolean         BTSXE1;
extern  dboolean         BTSXE2;
extern  dboolean         BTSXE2A;
extern  dboolean         BTSXE2B;
extern  dboolean         BTSXE3;
extern  dboolean         BTSXE3A;
extern  dboolean         BTSXE3B;

extern  fixed_t          forwardmove; 
extern  fixed_t          sidemove; 

extern  int              show_endoom;
extern  int              display_fps;
extern  int              turnspeed;
extern  int              d_colblood;
extern  int              d_colblood2;
extern  int              d_swirl;
extern  int              autoaim;
extern  int              background_type;
extern  int              drawgrid;
extern  int              followplayer;
extern  int              show_stats;
extern  int              timer_info;
extern  int              showMessages;
extern  int              use_vanilla_weapon_change;
extern  int              chaingun_tics;
extern  int              crosshair;
extern  int              icontype;
extern  int              wipe_type;
extern  int              usegamma;
extern  int              screenSize;
extern  int              screenblocks;
extern  int              detailLevel;
extern  int              mouselook;
extern  int              mspeed;
extern  int              mus_engine;
extern  int              snd_module;
extern  int              snd_chans;
extern  int              sound_channels;
extern  int              opl_type;
extern  int              use_libsamplerate;
extern  int              gore_amount;
extern  int              font_shadow;
extern  int              render_mode;
extern  int              bloodsplat_particle;
extern  int              bulletpuff_particle;
extern  int              teleport_particle;
extern  int              d_uncappedframerate;
extern  int              d_spawnteleglit;
extern  int              mapcolor_back;  // map background
extern  int              mapcolor_grid;  // grid lines color
extern  int              mapcolor_wall;  // normal 1s wall color
extern  int              mapcolor_fchg;  // line at floor height change color
extern  int              mapcolor_cchg;  // line at ceiling height change color
extern  int              mapcolor_clsd;  // line at sector with floor=ceiling color
extern  int              mapcolor_rkey;  // red key color
extern  int              mapcolor_bkey;  // blue key color
extern  int              mapcolor_ykey;  // yellow key color
extern  int              mapcolor_rdor;  // red door color  (diff from keys to allow option)
extern  int              mapcolor_bdor;  // blue door color (of enabling one but not other)
extern  int              mapcolor_ydor;  // yellow door color
extern  int              mapcolor_tele;  // teleporter line color
extern  int              mapcolor_secr;  // secret sector boundary color
extern  int              mapcolor_exit;  // jff 4/23/98 add exit line color
extern  int              mapcolor_unsn;  // computer map unseen line color
extern  int              mapcolor_flat;  // line with no floor/ceiling changes
extern  int              mapcolor_sprt;  // general sprite color
extern  int              mapcolor_item;  // item sprite color
extern  int              mapcolor_enemy; // enemy sprite color
extern  int              mapcolor_hair;  // crosshair color
extern  int              mapcolor_sngl;  // single player arrow color
extern  int              mapcolor_plyr;  // color for player arrow
extern  int              map_grid_size;

void A_MoreGibs(mobj_t* actor);


#endif
