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
//
//
//-----------------------------------------------------------------------------


#ifndef __D_PLAYER__
#define __D_PLAYER__


// The player data structure depends on a number
// of other structs: items (internal inventory),
// animation states (closely tied to the sprites
// used to represent them, unfortunately).
#include "d_items.h"

// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "../d_ticcmd.h"

#include "../net_defs.h"

// In addition, the player is just a special
// case of the generic moving object/actor.
#include "p_mobj.h"

#include "p_pspr.h"


typedef enum
{
    NOARMOR             = 0,
    GREENARMOR          = 1,
    BLUEARMOR           = 2
} armortype_t;

//
// Player states.
//
typedef enum
{
    // Playing or camping.
    PST_LIVE,

    // Dead on the ground, view follows killer.
    PST_DEAD,

    // Ready to restart/respawn???
    PST_REBORN                

} playerstate_t;

//
// Player internal flags, for cheats and debug.
//
typedef enum
{
    // No clipping, walk through barriers.
    CF_NOCLIP            = 1,

    // No damage, no health loss.
    CF_GODMODE           = 2,

    CF_NOTARGET         = 4,

    CF_MYPOS            = 8,

    CF_ALLMAP           = 16,

    CF_ALLMAP_THINGS    = 32,

    CF_CHOPPERS         = 64

} cheat_t;

//
// Extended player object info: player_t
//
typedef struct player_s
{
    mobj_t               *mo;
    playerstate_t        playerstate;
    ticcmd_t             cmd;

    // Determine POV,
    //  including viewpoint bobbing during movement.
    // Focal origin above r.z
    fixed_t              viewz;

    // Base height above floor for viewz.
    fixed_t              viewheight;

    // Bob/squat speed.
    fixed_t              deltaviewheight;

    // bounded/scaled total momentum.
    fixed_t              bob;        

    // This is only used between levels,
    // mo->health is used during levels.
    int                  health;        
    int                  armorpoints;

    // Armor type is 0-2.
    armortype_t          armortype;        

    // Power ups. invinc and invis are tic counters.
    int                  powers[NUMPOWERS];
    int                  cards[NUMCARDS];
    dboolean             backpack;
    
    // Frags, kills of other players.
    int                  frags[MAXPLAYERS];

    weapontype_t         readyweapon;
    
    // Is wp_nochange if not changing.
    weapontype_t         pendingweapon;

    int                  weaponowned[NUMWEAPONS];
    int                  ammo[NUMAMMO];
    int                  maxammo[NUMAMMO];

    // True if button down last tic.
    int                  attackdown;
    int                  usedown;

    // Bit flags, for cheats and debug.
    // See cheat_t, above.
    int                  cheats;                

    // Refired shots are less accurate.
    int                  refire;                

     // For intermission stats.
    int                  killcount;
    int                  itemcount;
    int                  secretcount;

    // Hint messages.
    char                 *message;
    char                 *messages[3];
    int                  message_count;
    
    // For screen flashing (red or bright).
    int                  damagecount;
    int                  bonuscount;

    // Who did damage (NULL for floors/ceilings).
    mobj_t               *attacker;
    
    // So gun flashes light up areas.
    int                  extralight;

    // Current PLAYPAL, ???
    //  can be set to REDCOLORMAP for pain, etc.
    int                  fixedcolormap;

    // Player skin colorshift,
    //  0-3 for which color to draw player.
    int                  colormap;        

    // Overlay view sprites (gun, etc).
    pspdef_t             psprites[NUMPSPRITES];

    // True if secret level has been done.
    dboolean             didsecret;        

    // delay the next jump for a moment
    unsigned int         jumpTics;

    // total time the player's been playing
    unsigned int         worldTimer;

    // freelook angle
    int                  lookdir;

    // freelook = 0
    dboolean             centering;

    // changes freelook angle for a short time
    // affected by gun shots
    int                  recoilpitch;

    int                  flyheight;

    // [AM] Previous position of viewz before think.
    //      Used to interpolate between camera positions.
    angle_t              oldviewz;

    // beta mode extras
    int                  item;
    int                  score;
    int                  lifes;
    int                  extra_lifes;
    int                  nextextra;

    int                  neededcard;
    int                  neededcardflash;

    // [RH] Used for falling damage
    fixed_t              oldvelocity[3];

    // killough 10/98: used for realistic bobbing (i.e. not simply overall speed)
    // mo->momx and mo->momy represent true momenta experienced by player.
    // This only represents the thrust that the player applies himself.
    // This avoids anomolies with such things as Boom ice and conveyors.
    fixed_t              momx;
    fixed_t              momy;

    // DOOM Retro???
    weapontype_t         preferredshotgun;
    int                  shotguns;
    weapontype_t         fistorchainsaw;
    dboolean             invulnbeforechoppers;
    dboolean             chainsawbeforechoppers;
    weapontype_t         weaponbeforechoppers;

    // For playerstats cmd
    int                  damageinflicted;
    int                  damagereceived;
    int                  cheated;
    int                  shotshit;
    int                  shotsfired;
    int                  deaths;
    int                  mobjcount[NUMMOBJTYPES];

    int                  skin;

} player_t;

//
// INTERMISSION
// Structure passed e.g. to WI_Start(wb)
//
typedef struct
{
    // whether the player is in game
    dboolean             in;
    
    // Player stats, kills, collected items etc.
    int                  skills;
    int                  sitems;
    int                  ssecret;
    int                  stime; 
    int                  frags[4];

    // current score on entry, modified on return
    int                  score;

    // current bonus on entry, modified on return
    int                  bonus;
  
} wbplayerstruct_t;

typedef struct
{
    // episode # (0-2)
    int                  epsd;

    // if true, splash the secret level
    dboolean             didsecret;
    
    // previous and next levels, origin 0
    int                  last;
    int                  next;        
    
    int                  maxkills;
    int                  maxitems;
    int                  maxsecret;
    int                  maxfrags;

    // the par time
    int                  partime;
    
    // index of this player in game
    int                  pnum;        

    wbplayerstruct_t     plyr[MAXPLAYERS];

} wbstartstruct_t;

#endif

