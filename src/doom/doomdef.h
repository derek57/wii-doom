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
//  Internally used data structures for virtually everything,
//   lots of other stuff.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMDEF__
#define __DOOMDEF__


#include <stdio.h>
#include <string.h>

#include "../doomtype.h"
#include "../i_timer.h"


//
// Global parameters/defines.
//

// If rangecheck is undefined,
// most parameter validation debugging code will not be compiled
//
// TODO: There are separate RANGECHECK defines for different games, but this
// is common code. Fix this.
#define RANGECHECK

// DOOM version
#define DOOM_VERSION             109

// Version code for cph's longtics hack ("v1.91")
#define DOOM_191_VERSION         111

// The maximum number of players, multiplayer/networking.
#define MAXPLAYERS               4

#define MAXPATH                  0x108

#define SavePathScreenshotsUSB   "usb:/apps/wiidoom/screenshots"
#define SavePathRoot1USB         "usb:/apps/wiidoom/savegames"
#define SavePathRoot2USB         "usb:/apps/wiidoom/savegames/doom.wad"
#define SavePathRoot3USB         "usb:/apps/wiidoom/savegames/doom2.wad"
#define SavePathRoot4USB         "usb:/apps/wiidoom/savegames/tnt.wad"
#define SavePathRoot5USB         "usb:/apps/wiidoom/savegames/plutonia.wad"
#define SavePathRoot6USB         "usb:/apps/wiidoom/savegames/chex.wad"
#define SavePathRoot7USB         "usb:/apps/wiidoom/savegames/hacx.wad"
#define SavePathRoot8USB         "usb:/apps/wiidoom/savegames/freedoom.wad"
#define SavePathBeta14USB        "usb:/apps/wiidoom/savegames/doom.wad/DM_B_14"
#define SavePathBeta15USB        "usb:/apps/wiidoom/savegames/doom.wad/DM_B_15"
#define SavePathBeta16USB        "usb:/apps/wiidoom/savegames/doom.wad/DM_B_16"
#define SavePathShare10USB       "usb:/apps/wiidoom/savegames/doom.wad/DM_S_10"
#define SavePathShare11USB       "usb:/apps/wiidoom/savegames/doom.wad/DM_S_11"
#define SavePathShare12USB       "usb:/apps/wiidoom/savegames/doom.wad/DM_S_12"
#define SavePathShare125USB      "usb:/apps/wiidoom/savegames/doom.wad/DM_S_125"
#define SavePathShare1666USB     "usb:/apps/wiidoom/savegames/doom.wad/DM_S_16"
#define SavePathShare18USB       "usb:/apps/wiidoom/savegames/doom.wad/DM_S_18"
#define SavePathReg11USB         "usb:/apps/wiidoom/savegames/doom.wad/DM_R_11"
#define SavePathReg12USB         "usb:/apps/wiidoom/savegames/doom.wad/DM_R_12"
#define SavePathReg16USB         "usb:/apps/wiidoom/savegames/doom.wad/DM_R_16"
#define SavePathReg18USB         "usb:/apps/wiidoom/savegames/doom.wad/DM_R_18"
#define SavePathReg19USB         "usb:/apps/wiidoom/savegames/doom.wad/DM_R_19U"
#define SavePathRegBFGXBOX360USB "usb:/apps/wiidoom/savegames/doom.wad/DM_B_XB"
#define SavePathRegBFGPCUSB      "usb:/apps/wiidoom/savegames/doom.wad/DM_B_PC"
#define SavePathRegXBOXUSB       "usb:/apps/wiidoom/savegames/doom.wad/DM_XBOX"
#define SavePath2RegXBOXUSB      "usb:/apps/wiidoom/savegames/doom.wad/DM2_XBOX"
#define SavePath2Reg1666USB      "usb:/apps/wiidoom/savegames/doom2.wad/DM2_R_16"
#define SavePath2Reg1666GUSB     "usb:/apps/wiidoom/savegames/doom2.wad/D2_R_16G"
#define SavePath2Reg17USB        "usb:/apps/wiidoom/savegames/doom2.wad/DM2_R_17"
#define SavePath2Reg18FUSB       "usb:/apps/wiidoom/savegames/doom2.wad/DM2_R_18"
#define SavePath2Reg19USB        "usb:/apps/wiidoom/savegames/doom2.wad/DM2_R_19"
#define SavePath2RegBFGPSNUSB    "usb:/apps/wiidoom/savegames/doom2.wad/DM2_B_PS"
#define SavePath2RegBFGPCUSB     "usb:/apps/wiidoom/savegames/doom2.wad/DM2_B_PC"
#define SavePathTNT191USB        "usb:/apps/wiidoom/savegames/tnt.wad/FD1_1_19"
#define SavePathTNT192USB        "usb:/apps/wiidoom/savegames/tnt.wad/FD1_2_19"
#define SavePathPLUT191USB       "usb:/apps/wiidoom/savegames/plutonia.wad/FD2_1_19"
#define SavePathPLUT192USB       "usb:/apps/wiidoom/savegames/plutonia.wad/FD2_2_19"
#define SavePathChexUSB          "usb:/apps/wiidoom/savegames/chex.wad"
/*
#define SavePathHacxShare10USB   "usb:/apps/wiidoom/savegames/hacx.wad/HX_SW_10"
#define SavePathHacxReg10USB     "usb:/apps/wiidoom/savegames/hacx.wad/HX_R_10"
#define SavePathHacxReg11USB     "usb:/apps/wiidoom/savegames/hacx.wad/HX_R_11"
*/
#define SavePathHacxReg12USB     "usb:/apps/wiidoom/savegames/hacx.wad/HX_R_12"
/*
#define SavePathFreedoom064USB   "usb:/apps/wiidoom/savegames/freedoom.wad/FRDM064"
#define SavePathFreedoom07RC1USB "usb:/apps/wiidoom/savegames/freedoom.wad/FRDM07R1"
#define SavePathFreedoom07USB    "usb:/apps/wiidoom/savegames/freedoom.wad/FRDM07"
#define SavePathFreedoom08B1USB  "usb:/apps/wiidoom/savegames/freedoom.wad/FRDM08B1"
#define SavePathFreedoom08USB    "usb:/apps/wiidoom/savegames/freedoom.wad/FRDM08"
#define SavePathFreedoom08P1USB  "usb:/apps/wiidoom/savegames/freedoom.wad/FRDM08P1"
*/
#define SavePathFreedoom08P2USB  "usb:/apps/wiidoom/savegames/freedoom.wad/FRDM08P2"
#define SavePathRootIWADUSB      "usb:/apps/wiidoom/IWAD"
#define SavePathRootPWADUSB      "usb:/apps/wiidoom/PWAD"
#define SavePathRootD1MusicUSB   "usb:/apps/wiidoom/doom1-music"
#define SavePathRootD2MusicUSB   "usb:/apps/wiidoom/doom2-music"
#define SavePathRootTNTMusicUSB  "usb:/apps/wiidoom/tnt-music"
#define SavePathRootChexMusicUSB "usb:/apps/wiidoom/chex-music"
#define SavePathRootHacxMusicUSB "usb:/apps/wiidoom/hacx-music"
#define SavePathScreenshotsSD    "sd:/apps/wiidoom/screenshots"
#define SavePathRoot1SD          "sd:/apps/wiidoom/savegames"
#define SavePathRoot2SD          "sd:/apps/wiidoom/savegames/doom.wad"
#define SavePathRoot3SD          "sd:/apps/wiidoom/savegames/doom2.wad"
#define SavePathRoot4SD          "sd:/apps/wiidoom/savegames/tnt.wad"
#define SavePathRoot5SD          "sd:/apps/wiidoom/savegames/plutonia.wad"
#define SavePathRoot6SD          "sd:/apps/wiidoom/savegames/chex.wad"
#define SavePathRoot7SD          "sd:/apps/wiidoom/savegames/hacx.wad"
#define SavePathRoot8SD          "sd:/apps/wiidoom/savegames/freedoom.wad"
#define SavePathBeta14SD         "sd:/apps/wiidoom/savegames/doom.wad/DM_B_14"
#define SavePathBeta15SD         "sd:/apps/wiidoom/savegames/doom.wad/DM_B_15"
#define SavePathBeta16SD         "sd:/apps/wiidoom/savegames/doom.wad/DM_B_16"
#define SavePathShare10SD        "sd:/apps/wiidoom/savegames/doom.wad/DM_S_10"
#define SavePathShare11SD        "sd:/apps/wiidoom/savegames/doom.wad/DM_S_11"
#define SavePathShare12SD        "sd:/apps/wiidoom/savegames/doom.wad/DM_S_12"
#define SavePathShare125SD       "sd:/apps/wiidoom/savegames/doom.wad/DM_S_125"
#define SavePathShare1666SD      "sd:/apps/wiidoom/savegames/doom.wad/DM_S_16"
#define SavePathShare18SD        "sd:/apps/wiidoom/savegames/doom.wad/DM_S_18"
#define SavePathReg11SD          "sd:/apps/wiidoom/savegames/doom.wad/DM_R_11"
#define SavePathReg12SD          "sd:/apps/wiidoom/savegames/doom.wad/DM_R_12"
#define SavePathReg16SD          "sd:/apps/wiidoom/savegames/doom.wad/DM_R_16"
#define SavePathReg18SD          "sd:/apps/wiidoom/savegames/doom.wad/DM_R_18"
#define SavePathReg19SD          "sd:/apps/wiidoom/savegames/doom.wad/DM_R_19U"
#define SavePathRegBFGXBOX360SD  "sd:/apps/wiidoom/savegames/doom.wad/DM_B_XB"
#define SavePathRegBFGPCSD       "sd:/apps/wiidoom/savegames/doom.wad/DM_B_PC"
#define SavePathRegXBOXSD        "sd:/apps/wiidoom/savegames/doom.wad/DM_XBOX"
#define SavePath2RegXBOXSD       "sd:/apps/wiidoom/savegames/doom.wad/DM2_XBOX"
#define SavePath2Reg1666SD       "sd:/apps/wiidoom/savegames/doom2.wad/DM2_R_16"
#define SavePath2Reg1666GSD      "sd:/apps/wiidoom/savegames/doom2.wad/D2_R_16G"
#define SavePath2Reg17SD         "sd:/apps/wiidoom/savegames/doom2.wad/DM2_R_17"
#define SavePath2Reg18FSD        "sd:/apps/wiidoom/savegames/doom2.wad/DM2_R_18"
#define SavePath2Reg19SD         "sd:/apps/wiidoom/savegames/doom2.wad/DM2_R_19"
#define SavePath2RegBFGPSNSD     "sd:/apps/wiidoom/savegames/doom2.wad/DM2_B_PS"
#define SavePath2RegBFGPCSD      "sd:/apps/wiidoom/savegames/doom2.wad/DM2_B_PC"
#define SavePathTNT191SD         "sd:/apps/wiidoom/savegames/tnt.wad/FD1_1_19"
#define SavePathTNT192SD         "sd:/apps/wiidoom/savegames/tnt.wad/FD1_2_19"
#define SavePathPLUT191SD        "sd:/apps/wiidoom/savegames/plutonia.wad/FD2_1_19"
#define SavePathPLUT192SD        "sd:/apps/wiidoom/savegames/plutonia.wad/FD2_2_19"
#define SavePathChexSD           "sd:/apps/wiidoom/savegames/chex.wad"
/*
#define SavePathHacxShare10SD    "sd:/apps/wiidoom/savegames/hacx.wad/HX_SW_10"
#define SavePathHacxReg10SD      "sd:/apps/wiidoom/savegames/hacx.wad/HX_R_10"
#define SavePathHacxReg11SD      "sd:/apps/wiidoom/savegames/hacx.wad/HX_R_11"
*/
#define SavePathHacxReg12SD      "sd:/apps/wiidoom/savegames/hacx.wad/HX_R_12"
/*
#define SavePathFreedoom064SD    "sd:/apps/wiidoom/savegames/freedoom.wad/FRDM064"
#define SavePathFreedoom07RC1SD  "sd:/apps/wiidoom/savegames/freedoom.wad/FRDM07R1"
#define SavePathFreedoom07SD     "sd:/apps/wiidoom/savegames/freedoom.wad/FRDM07"
#define SavePathFreedoom08B1SD   "sd:/apps/wiidoom/savegames/freedoom.wad/FRDM08B1"
#define SavePathFreedoom08SD     "sd:/apps/wiidoom/savegames/freedoom.wad/FRDM08"
#define SavePathFreedoom08P1SD   "sd:/apps/wiidoom/savegames/freedoom.wad/FRDM08P1"
*/
#define SavePathFreedoom08P2SD   "sd:/apps/wiidoom/savegames/freedoom.wad/FRDM08P2"
#define SavePathRootIWADSD       "sd:/apps/wiidoom/IWAD"
#define SavePathRootPWADSD       "sd:/apps/wiidoom/PWAD"
#define SavePathRootD1MusicSD    "sd:/apps/wiidoom/doom1-music"
#define SavePathRootD2MusicSD    "sd:/apps/wiidoom/doom2-music"
#define SavePathRootTNTMusicSD   "sd:/apps/wiidoom/tnt-music"
#define SavePathRootChexMusicSD  "sd:/apps/wiidoom/chex-music"
#define SavePathRootHacxMusicSD  "sd:/apps/wiidoom/hacx-music"

#ifndef VER_ID
#define VER_ID                   "DVL"
#endif

#ifdef RANGECHECK
#define DOOM_VERSIONTEXT         "Version 2 +R "__DATE__" ("VER_ID")"
#else
#define DOOM_VERSIONTEXT         "Version 2 "__DATE__" ("VER_ID")"
#endif

#ifdef RANGECHECK
#define VERSIONTEXT              "V2 +R ("VER_ID") "__TIME__""
#endif

#define YEAR                     (((__DATE__ [9] - '0')) * 10 + (__DATE__ [10] - '0'))

#define MONTH                    (__DATE__ [2] == 'n' ? 0                                \
                                 : __DATE__ [2] == 'b' ? 1                               \
                                 : __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 2 : 3)   \
                                 : __DATE__ [2] == 'y' ? 4                               \
                                 : __DATE__ [2] == 'n' ? 5                               \
                                 : __DATE__ [2] == 'l' ? 6                               \
                                 : __DATE__ [2] == 'g' ? 7                               \
                                 : __DATE__ [2] == 'p' ? 8                               \
                                 : __DATE__ [2] == 't' ? 9                               \
                                 : __DATE__ [2] == 'v' ? 10 : 11)

#define DAY                      ((__DATE__ [4] == ' ' ? 0 : __DATE__ [4] - '0') * 10    \
                                 + (__DATE__ [5] - '0'))

#define DATE_AS_INT              (((YEAR - 2000) * 12 + MONTH) * 31 + DAY)

#define NUMKEYS                  256

#define M_ZOOMIN                 ((int)(1.02 * FRACUNIT))

#define M_ZOOMOUT                ((int)(FRACUNIT / 1.02))

#define AFLAG_JUMP               0x80

#define SCREENSCALE              2

#define ORIGINALWIDTH            320
#define ORIGINALHEIGHT           200

#define SCREENWIDTH              (ORIGINALWIDTH * SCREENSCALE)
#define SCREENHEIGHT             (ORIGINALHEIGHT * SCREENSCALE)

#define ORIGINALSBARHEIGHT       32 
#define SBARHEIGHT               (ORIGINALSBARHEIGHT * SCREENSCALE)

// Index of the special effects (INVUL inverse) map.
#define INVERSECOLORMAP          32

#define MOUSE_WHEELUP            MAX_MOUSE_BUTTONS
#define MOUSE_WHEELDOWN          (MAX_MOUSE_BUTTONS + 1)

// phares 3/20/98:
//
// Player friction is variable, based on controlling
// linedefs. More friction can create mud, sludge,
// magnetized floors, etc. Less friction can create ice.

// mud factor based on momentum
#define MORE_FRICTION_MOMENTUM   15000

// original value
#define ORIG_FRICTION            0xE800

// original value
#define ORIG_FRICTION_FACTOR     2048

//
// Difficulty/skill settings/filters.
//

// Skill flags.
#define MTF_EASY                 1
#define MTF_NORMAL               2
#define MTF_HARD                 4

// Deaf monsters/do not react to sound.
#define MTF_AMBUSH               8

#ifndef PI
#define PI                       3.14159265358979323846
#endif


typedef enum 
{
    DEFAULT_INT,
    DEFAULT_INT_HEX,
    DEFAULT_STRING,
    DEFAULT_FLOAT,
    DEFAULT_KEY,
    DEFAULT_BOOLEAN

} default_type_t;

typedef struct
{
    // Name of the variable
    char           *name;

    // Pointer to the location in memory of the variable
    union {
        int *i;
        char **s;
        float *f;
        dboolean *b;
    } location;

    // Type of the variable
    default_type_t type;

    // If this is a key value, the original integer scancode we read from
    // the config file before translating it to the internal key value.
    // If zero, we didn't read this value from a config file.
    int            untranslated;

    // The value we translated the scancode into when we read the 
    // config file on startup.  If the variable value is different from
    // this, it has been changed and needs to be converted; otherwise,
    // use the 'untranslated' value.
    int            original_translated;

    // If true, this config variable has been bound to a variable
    // and is being used.
    dboolean       bound;

} default_t;

typedef struct
{
    default_t      *defaults;

    int            numdefaults;

    char           *filename;

} default_collection_t;

typedef struct
{
    // 0 = no cursor here, 1 = ok, 2 = arrows ok
    short          status;
    
    char           *name;
    
    // choice = menu item #.
    // if status = 2,
    //   choice = 0 : leftarrow, 1 : rightarrow
    void           (*routine)(int choice);
    
    // hotkey in menu
    char           alphaKey;                        

} menuitem_t;

typedef struct menu_s
{
    // # of menu items
    short          numitems;

    // previous menu
    struct menu_s  *prevMenu;

    // menu items
    menuitem_t     *menuitems;

    // draw routine
    void           (*routine)();

    // x, y of menu
    short          x;
    short          y;

    // last item user was on in menu
    short          lastOn;

} menu_t;

// The current state of the game: whether we are
// playing, gazing at the intermission screen,
// the game final animation, or a demo. 
typedef enum
{
    GS_LEVEL,
    GS_INTERMISSION,
    GS_FINALE,
    GS_DEMOSCREEN

} gamestate_t;

typedef enum
{
    ga_nothing,
    ga_loadlevel,
    ga_newgame,
    ga_loadgame,
    ga_savegame,
    ga_playdemo,
    ga_completed,
    ga_victory,
    ga_worlddone,
    ga_screenshot,
    ga_autoloadgame, 
    ga_autosavegame

} gameaction_t;

//
// Key cards.
//
typedef enum
{
    it_bluecard,
    it_yellowcard,
    it_redcard,
    it_blueskull,
    it_yellowskull,
    it_redskull,
    
    NUMCARDS
    
} card_t;

// The defined weapons,
//  including a marker indicating
//  user has not changed weapon.
typedef enum
{
    wp_fist,
    wp_pistol,
    wp_shotgun,
    wp_chaingun,
    wp_missile,
    wp_plasma,
    wp_bfg,
    wp_chainsaw,
    wp_supershotgun,

    NUMWEAPONS,
    
    // No pending weapon change.
    wp_nochange

} weapontype_t;

// Ammunition types defined.
typedef enum
{
    // Pistol / chaingun ammo.
    am_clip,

    // Shotgun / double barreled shotgun.
    am_shell,

    // Plasma rifle, BFG.
    am_cell,

    // Missile launcher.
    am_misl,

    NUMAMMO,

    // Unlimited for chainsaw / fist.        
    am_noammo

} ammotype_t;

typedef enum
{
    arti_none,
    arti_fly,

    NUMARTIFACTS

} artitype_t;

// Power up artifacts.
typedef enum
{
    pw_invulnerability,
    pw_strength,
    pw_invisibility,
    pw_ironfeet,
    pw_allmap,
    pw_infrared,
    pw_flight,

    NUMPOWERS
    
} powertype_t;

//
// Power up durations,
//  how many seconds till expiration,
//  assuming TICRATE is 35 ticks/second.
//
typedef enum
{
    INVULNTICS      = (30 * TICRATE),
    INVISTICS       = (60 * TICRATE),
    INFRATICS       = (120 * TICRATE),
    IRONTICS        = (60 * TICRATE),
    FLIGHTTICS      = (60 * TICRATE)
    
} powerduration_t;

typedef enum
{
    DOOMBSP = 0,
    DEEPBSP = 1,
    ZDBSPX  = 2

} mapformat_t;


char               extra_wad_1[256];
char               extra_wad_2[256];
char               extra_wad_3[256];
char               dehacked_file[256];
char               target[MAXPATH];
char               path_tmp[MAXPATH];
char               temp[MAXPATH];

dboolean           extra_wad_slot_1_loaded;
dboolean           extra_wad_slot_2_loaded;
dboolean           extra_wad_slot_3_loaded;
dboolean           dont_move_forwards;
dboolean           error_detected;
dboolean           print_resource_pwad_error;
dboolean           print_resource_pwad2_error;
dboolean           gamekeydown[NUMKEYS]; 

FILE               *debugfile;
FILE               *statsfile;


extern dboolean    dont_move_backwards;
extern dboolean    sd;
extern dboolean    usb;
extern dboolean    is_chex_2;
extern dboolean    extra_wad_loaded;
extern dboolean    load_extra_wad;
extern dboolean    load_dehacked;

extern int         fsize;
extern int         fsizerw;
extern int         fsizerw2;
extern int         fsizecq;
//extern int         show_deh_loading_message;

#endif // __DOOMDEF__

