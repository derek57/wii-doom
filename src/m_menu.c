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
//        DOOM selection menu, options, episode etc.
//        Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------


#include <SDL/SDL.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "am_map.h"
#include "c_io.h"
#include "d_main.h"
#include "deh_str.h"
#include "doomdef.h"
#include "doomkeys.h"
#include "doomstat.h"
#include "dstrings.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_controls.h"
#include "m_menu.h"
#include "m_misc.h"
#include "p_local.h"
#include "p_saveg.h"
#include "r_local.h"
#include "s_sound.h"

// Data.
#include "sounds.h"

#include "st_stuff.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include <wiiuse/wpad.h>

#define FIRSTKEY_MAX                        0
#define SKULLXOFF                           -32
#define LINEHEIGHT                          16
#define CURSORXOFF_SMALL                    -20
#define LINEHEIGHT_SMALL                    10
#define CLASSIC_CONTROLLER_A                0x1
#define CLASSIC_CONTROLLER_R                0x2
#define CLASSIC_CONTROLLER_PLUS             0x4
#define CLASSIC_CONTROLLER_L                0x8
#define CLASSIC_CONTROLLER_MINUS            0x10
#define CLASSIC_CONTROLLER_B                0x20
#define CLASSIC_CONTROLLER_LEFT             0x40
#define CLASSIC_CONTROLLER_DOWN             0x80
#define CLASSIC_CONTROLLER_RIGHT            0x100
#define CLASSIC_CONTROLLER_UP               0x200
#define CLASSIC_CONTROLLER_ZR               0x400
#define CLASSIC_CONTROLLER_ZL               0x800
#define CLASSIC_CONTROLLER_HOME             0x1000
#define CLASSIC_CONTROLLER_X                0x2000
#define CLASSIC_CONTROLLER_Y                0x4000
#define CONTROLLER_1                        0x8000
#define CONTROLLER_2                        0x10000



void    (*messageRoutine)  (int response);

char *maptext[] = {
    " ",
    "E1M1: HANGAR",
    "E1M2: NUCLEAR PLANT",
    "E1M3: TOXIN REFINERY",
    "E1M4: COMMAND CONTROL",
    "E1M5: PHOBOS LAB",
    "E1M6: CENTRAL PROCESSING",
    "E1M7: COMPUTER STATION",
    "E1M8: PHOBOS ANOMALY",
    "E1M9: MILITARY BASE",
    "E2M1: DEIMOS ANOMALY",
    "E2M2: CONTAINMENT AREA",
    "E2M3: REFINERY",
    "E2M4: DEIMOS LAB",
    "E2M5: COMMAND CENTER",
    "E2M6: HALLS OF THE DAMNED",
    "E2M7: SPAWNING VATS",
    "E2M8: TOWER OF BABEL",
    "E2M9: FORTRESS OF MYSTERY",
    "E3M1: HELL KEEP",
    "E3M2: SLOUGH OF DESPAIR",
    "E3M3: PANDEMONIUM",
    "E3M4: HOUSE OF PAIN",
    "E3M5: UNHOLY CATHEDRAL",
    "E3M6: MT. EREBUS",
    "E3M7: LIMBO",
    "E3M8: DIE",
    "E3M9: WARRENS",
    "E4M1: HELL BENEATH",
    "E4M2: PREFECT HATRED",
    "E4M3: SEVER THE WICKED",
    "E4M4: UNRULY EVIL",
    "E4M5: THEY WILL REPENT",
    "E4M6: AGAINST THEE WICKEDLY",
    "E4M7: AND HELL FOLLOWED",
    "E4M8: UNTO THE CRUEL",
    "E4M9: FEAR",
    "LEVEL 1: ENTRYWAY",
    "LEVEL 2: UNDERHALLS",
    "LEVEL 3: THE GAUNTLET",
    "LEVEL 4: THE FOCUS",
    "LEVEL 5: THE WASTE TUNNELS",
    "LEVEL 6: THE CRUSHER",
    "LEVEL 7: DEAD SIMPLE",
    "LEVEL 8: TRICKS AND TRAPS",
    "LEVEL 9: THE PIT",
    "LEVEL 10: REFUELING BASE",
    "LEVEL 11: 'O' OF DESTRUCTION!",
    "LEVEL 12: THE FACTORY",
    "LEVEL 13: DOWNTOWN",
    "LEVEL 14: THE INMOST DENS",
    "LEVEL 15: INDUSTRIAL ZONE",
    "LEVEL 16: SUBURBS",
    "LEVEL 17: TENEMENTS",
    "LEVEL 18: THE COURTYARD",
    "LEVEL 19: THE CITADEL",
    "LEVEL 20: GOTCHA!",
    "LEVEL 21: NIRVANA",
    "LEVEL 22: THE CATACOMBS",
    "LEVEL 23: BARRELS O' FUN",
    "LEVEL 24: THE CHASM",
    "LEVEL 25: BLOODFALLS",
    "LEVEL 26: THE ABANDONED MINES",
    "LEVEL 27: MONSTER CONDO",
    "LEVEL 28: THE SPIRIT WORLD",
    "LEVEL 29: THE LIVING END",
    "LEVEL 30: ICON OF SIN",
    "LEVEL 31: WOLFENSTEIN",
    "LEVEL 32: GROSSE",
    "LEVEL 1: SYSTEM CONTROL",
    "LEVEL 2: HUMAN BBG",
    "LEVEL 3: POWER CONTROL",
    "LEVEL 4: WORMHOLE",
    "LEVEL 5: HANGER",
    "LEVEL 6: OPEN SEASON",
    "LEVEL 7: PRISON",
    "LEVEL 8: METAL",
    "LEVEL 9: STRONGHOLD",
    "LEVEL 10: REDEMPTION",
    "LEVEL 11: STORAGE FACILITY",
    "LEVEL 12: CRATER",
    "LEVEL 13: NUKAGE PROCESSING",
    "LEVEL 14: STEEL WORKS",
    "LEVEL 15: DEAD ZONE",
    "LEVEL 16: DEEPEST REACHES",
    "LEVEL 17: PROCESSING AREA",
    "LEVEL 18: MILL",
    "LEVEL 19: SHIPPING / RESPAWNING",
    "LEVEL 20: CENTRAL PROCESSING",
    "LEVEL 21: ADMINISTRATION CENTER",
    "LEVEL 22: HABITAT",
    "LEVEL 23: LUNAR MINING PROJECT",
    "LEVEL 24: QUARRY",
    "LEVEL 25: BARON'S DEN",
    "LEVEL 26: BALLISTYX",
    "LEVEL 27: MOUNT PAIN",
    "LEVEL 28: HECK",
    "LEVEL 29: RIVER STYX",
    "LEVEL 30: LAST CALL",
    "LEVEL 31: PHARAOH",
    "LEVEL 32: CARIBBEAN",
    "LEVEL 1: CONGO",
    "LEVEL 2: WELL OF SOULS",
    "LEVEL 3: AZTEC",
    "LEVEL 4: CAGED",
    "LEVEL 5: GHOST TOWN",
    "LEVEL 6: BARON'S LAIR",
    "LEVEL 7: CAUGHTYARD",
    "LEVEL 8: REALM",
    "LEVEL 9: ABATTOIRE",
    "LEVEL 10: ONSLAUGHT",
    "LEVEL 11: HUNTED",
    "LEVEL 12: SPEED",
    "LEVEL 13: THE CRYPT",
    "LEVEL 14: GENESIS",
    "LEVEL 15: THE TWILIGHT",
    "LEVEL 16: THE OMEN",
    "LEVEL 17: COMPOUND",
    "LEVEL 18: NEUROSPHERE",
    "LEVEL 19: NME",
    "LEVEL 20: THE DEATH DOMAIN",
    "LEVEL 21: SLAYER",
    "LEVEL 22: IMPOSSIBLE MISSION",
    "LEVEL 23: TOMBSTONE",
    "LEVEL 24: THE FINAL FRONTIER",
    "LEVEL 25: THE TEMPLE OF DARKNESS",
    "LEVEL 26: BUNKER",
    "LEVEL 27: ANTI-CHRIST",
    "LEVEL 28: THE SEWERS",
    "LEVEL 29: ODYSSEY OF NOISES",
    "LEVEL 30: THE GATEWAY TO HELL",
    "LEVEL 31: CYBERDEN",
    "LEVEL 32: GO 2 IT",
    "E1M1: LANDING ZONE",
    "E1M2: STORAGE FACILITY",
    "E1M3: LABORATORY",
    "E1M4: ARBORETUM",
    "E1M5: CAVERNS OF BAZOIK",
    "E1M1: SPACEPORT",
    "E1M2: CINEMA",
    "E1M3: CHEX MUSEUM",
    "E1M4: CITY STREETS",
    "E1M5: SEWER SYSTEM",
    "MAP01: GENEMP CORP.",
    "MAP02: TUNNEL TOWN",
    "MAP03: LAVA ANNEX",
    "MAP04: ALCATRAZ",
    "MAP05: CYBER CIRCUS",
    "MAP06: DIGI-OTA",
    "MAP07: THE GREAT WALL",
    "MAP08: GARDEN OF DELIGHT",
    "MAP09: HIDDEN FORTRESS",
    "MAP10: ANARCHIST DREAM",
    "MAP11: NOTUS US!",
    "MAP12: GOTHIK GAUNTLET",
    "MAP13: THE SEWERS",
    "MAP14: 'TRODE WARS",
    "MAP15: TWILIGHT OF ENKS",
    "MAP16: PROTEAN CYBEX",
    "MAP17: RIVER OF BLOOD",
    "MAP18: BIZARRO",
    "MAP19: THE WAR ROOMS",
    "MAP20: INTRUDER ALERT!",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "MAP31: DESSICANT ROOM"
};

char *songtext[] = {
    " ",
    "01",
    "02",
    "03",
    "04",
    "05",
    "06",
    "07",
    "08",
    "09",
    "10",
    "11",
    "12",
    "13",
    " ",
    "14",
    "15",
    "16",
    "17",
    " ",
    "18",
    "19",
    " ",
    " ",
    " ",
    " ",
    "20",
    " ",
    " ",
    "21",
    "22",
    "23"
};

char *songtextreg[] = {
    " ",
    "01",
    "02",
    "03",
    "04",
    "05",
    "06",
    "07",
    "08",
    "09",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
    "17",
    "18",
    "19",
    "20",
    "21",
    "22",
    "23"
};

char *songtextbeta[] = {
    " ",
    "01",
    "02",
    "03",
    "04",
    "05",
    "06",
    "07",
    "08",
    "09",
    " ",
    " ",
    "10",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "11",
    "12",
    " ",
    " ",
    " ",
    " ",
    " ",
    "10",
    "11",
    " ",
    "12"
};

char *songtextchex[] = {
    " ",
    "01",
    "02",
    "03",
    "04",
    "05",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "06",
    "07",
    " ",
    "08"
};

char *songtext2hacx[] = {
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "01",
    "02",
    "03",
    "04",
    "05",
    "06",
    "07",
    "08",
    "09",
    "10",
    "11",
    "12",
    "13",
    " ",
    "14",
    "15",
    "16",
    "17",
    " ",
    "18",
    "19",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "20",
    " ",
    "21",
    "22",
    "23"
};

char *songtext2fd[] = {
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "01",
    "02",
    "03",
    "04",
    " ",
    "05",
    " ",
    "06",
    " ",
    "07",
    " ",
    " ",
    " ",
    " ",
    " ",
    "08",
    "09",
    "10",
    "11",
    " ",
    " ",
    " ",
    " ",
    "12",
    " ",
    " ",
    " ",
    " ",
    "13",
    "14",
    "15",
    "16",
    " ",
    "17",
    "18"
};

char *songtext2[] = {
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "01",
    "02",
    "03",
    "04",
    "05",
    "06",
    "07",
    "08",
    "09",
    "10",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "11",
    " ",
    "12",
    " ",
    " ",
    "13",
    " ",
    "14",
    " ",
    " ",
    "15",
    " ",
    "16",
    "17",
    "18",
    "19",
    "20",
    "21"
};

char *songtexttnt[] = {
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "01",
    "02",
    "03",
    "04",
    "05",
    "06",
    "07",
    "08",
    " ",
    "09",
    "10",
    "11",
    " ",
    "12",
    " ",
    "13",
    " ",
    " ",
    "14",
    "15",
    "16",
    "17",
    "18",
    "19",
    "20",
    " ",
    " ",
    " ",
    " ",
    " ",
    "21",
    " ",
    " ",
    "22"
};

char *songtextplut[] = {
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "01",
    "02",
    "03",
    "04",
    "05",
    "06",
    "07",
    "08",
    "09",
    "10",
    "11",
    "12",
    "13",
    "14",
    "15",
    "16",
    "17",
    " ",
    "18",
    "19",
    "20",
    "21",
    "22",
    "23",
    "24",
    " ",
    " ",
    " ",
    " ",
    "25",
    " ",
    " ",
    " ",
    "26",
    "27"
};

char gammamsg[5][26] =
{
    GAMMALVL0,
    GAMMALVL1,
    GAMMALVL2,
    GAMMALVL3,
    GAMMALVL4
};

char *stupidtable[] =
{
    "A","B","C","D","E",
    "F","G","H","I","J",
    "K","L","M","N","O",
    "P","Q","R","S","T",
    "U","V","W","X","Y",
    "Z"
};

char *Key2String (int ch)
{
// S.A.: return "[" or "]" or "\"" doesn't work
// because there are no lumps for these chars,
// therefore we have to go with "RIGHT BRACKET"
// and similar for much punctuation.  Probably
// won't work with international keyboards and
// dead keys, either.
//
    switch (ch)
    {
        case CLASSIC_CONTROLLER_UP:       return "UP ARROW";
        case CLASSIC_CONTROLLER_DOWN:     return "DOWN ARROW";
        case CLASSIC_CONTROLLER_LEFT:     return "LEFT ARROW";
        case CLASSIC_CONTROLLER_RIGHT:    return "RIGHT ARROW";
        case CLASSIC_CONTROLLER_MINUS:    return "MINUS";
        case CLASSIC_CONTROLLER_PLUS:     return "PLUS";
        case CLASSIC_CONTROLLER_HOME:     return "HOME";
        case CLASSIC_CONTROLLER_A:        return "A";
        case CLASSIC_CONTROLLER_B:        return "B";
        case CLASSIC_CONTROLLER_X:        return "X";
        case CLASSIC_CONTROLLER_Y:        return "Y";
        case CLASSIC_CONTROLLER_ZL:       return "ZL";
        case CLASSIC_CONTROLLER_ZR:       return "ZR";
        case CLASSIC_CONTROLLER_L:        return "LEFT TRIGGER";
        case CLASSIC_CONTROLLER_R:        return "RIGHT TRIGGER";
        case CONTROLLER_1:                return "1";
        case CONTROLLER_2:                return "2";
    }

    // Handle letter keys
    // S.A.: could also be done with toupper
    if (ch >= 'a' && ch <= 'z')
        return stupidtable[(ch - 'a')];

    return "?";                // Everything else
}

char                       savegamestrings[10][SAVESTRINGSIZE];
char                       endstring[160];
char                       detailNames[2][9] = {"M_GDHIGH","M_GDLOW"};
char                       msgNames[2][9]    = {"M_MSGOFF","M_MSGON"};
char                       fpsDisplay[100];
char                       map_coordinates_textbuffer[50];
char                       massacre_textbuffer[20];

// old save description before edit
char                       saveOldString[SAVESTRINGSIZE];  

// ...and here is the message string!
char                       *messageString;

// graphic name of skulls
// warning: initializer-string for array of chars is too long
char                       *skullName[2]      = {"M_SKULL1","M_SKULL2"};
char                       *skullNameSmall[2] = {"M_SKULL3","M_SKULL4"};

// defaulted values
int                        mouseSensitivity = 5;

// Show messages has default, 0 = off, 1 = on
int                        showMessages = 1;        

// Blocky mode, has default, 0 = high, 1 = normal
int                        detailLevel = 0;
int                        screenblocks = 9;

// temp for screenblocks (0-9)
int                        screenSize;

 // 1 = message to be printed
int                        messageToPrint;

// message x & y
int                        messx;
int                        messy;
int                        messageLastMenuActive;

// we are going to be entering a savegame string
int                        saveStringEnter;              
int                        saveSlot;             // which slot to save in
int                        saveCharIndex;        // which char we're editing

int                        map = 1;
int                        musnum = 1;
int                        cheeting;
int                        coordinates_info = 0;
int                        timer_info = 0;
int                        version_info = 0;
int                        fps = 0;              // calculating the frames per second
int                        key_controls_start_in_cfg_at_pos = 30; // ACTUALLY IT'S +2
int                        key_controls_end_in_cfg_at_pos = 42;   // ACTUALLY IT'S +2
int                        crosshair = 0;
int                        show_stats = 0;
int                        tracknum = 1;
int                        opl = 1;
int                        epi = 1;
int                        repi = 1;
int                        rmap = 1;
int                        rskill = 0;
int                        warped = 0;
int                        faketracknum = 1;
int                        mus_engine = 1;
int                        mp_skill = 4;
int                        warpepi = 2;
int                        warplev = 2;
int                        turnspeed = 7;

//
// MENU TYPEDEFS
//

short                      itemOn;                  // menu item skull is on
short                      skullAnimCounter;        // skull animation counter
short                      whichSkull;              // which skull to draw

// timed message = no input from user
boolean                    messageNeedsInput;

boolean                    inhelpscreens;
boolean                    menuactive;
boolean                    forced = false;
boolean                    fake = false;
boolean                    mus_cheat_used = false;
boolean                    got_invisibility = false;
boolean                    got_radiation_suit = false;
boolean                    got_berserk = false;
boolean                    got_invulnerability = false;
boolean                    got_map = false;
boolean                    got_light_amp = false;
boolean                    got_all = false;
boolean                    aiming_help;
boolean                    hud;
boolean                    swap_sound_chans;
boolean                    skillflag = true;
boolean                    nomonstersflag;
boolean                    fastflag;
boolean                    respawnflag = true;
boolean                    warpflag = true;
boolean                    multiplayerflag = true;
boolean                    deathmatchflag = true;
boolean                    altdeathflag;
boolean                    locallanflag;
boolean                    searchflag;
boolean                    queryflag;
boolean                    dedicatedflag = true;
boolean                    privateserverflag;

fixed_t                    forwardmove = 29;
fixed_t                    sidemove = 21; 

// current menudef
menu_t*                    currentMenu;                          

static boolean             askforkey = false;

static int                 FirstKey = 0;           // SPECIAL MENU FUNCTIONS (ITEMCOUNT)
static int                 keyaskedfor;

extern int                 cheating;
extern int                 mspeed;
extern int                 left;
extern int                 right;
extern int                 mouselook;
extern int                 dots_enabled;
extern int                 fps_enabled;
extern int                 dont_show;
extern int                 display_fps;
extern int                 allocated_ram_size;

extern default_t           doom_defaults_list[];   // KEY BINDINGS

extern patch_t*            hu_font[HU_FONTSIZE];

extern boolean             message_dontfuckwithme;
extern boolean             chat_on;                // in heads-up code
extern boolean             BorderNeedRefresh;
extern boolean             sendpause;
extern boolean             secret_1;
extern boolean             secret_2;

extern short               songlist[148];


//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
void M_LoadGame(int choice);
void M_SaveGame(int choice);
void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_QuitDOOM(int choice);

void M_ChangeMessages(int choice);
void M_WalkingSpeed(int choice);
void M_TurningSpeed(int choice);
void M_StrafingSpeed(int choice);
void M_SfxVol(int choice);
void M_MusicVol(int choice);
void M_ChangeDetail(int choice);
void M_SizeDisplay(int choice);
void M_StartGame(int choice);
void M_Sound(int choice);

void M_FinishReadThis(int choice);
void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);

void M_DrawMainMenu(void);
void M_DrawReadThis1(void);
void M_DrawReadThis2(void);
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
void M_DrawSound(void);
void M_DrawLoad(void);
void M_DrawSave(void);

void M_DrawSaveLoadBorder(int x,int y);
void M_SetupNextMenu(menu_t *menudef);
void M_DrawThermo(int x,int y,int thermWidth,int thermDot);
void M_DrawEmptyCell(menu_t *menu,int item);
void M_DrawSelCell(menu_t *menu,int item);
void M_WriteText(int x, int y, char *string);
void M_StartMessage(char *string,void *routine,boolean input);
void M_StopMessage(void);
void M_ClearMenus (void);

void M_MusicType(int choice);
void M_SoundChannels(int choice);
void M_GameFiles(int choice);
void M_Brightness(int choice);
void M_Freelook(int choice);
void M_FreelookSpeed(int choice);

void M_KeyBindingsClearControls (int ch);
void M_KeyBindingsClearAll (int choice);
void M_KeyBindingsReset (int choice);
void M_KeyBindingsSetKey(int choice);
void M_KeyBindings(int choice);
void M_FPS(int choice);
void M_DisplayTicker(int choice);
void M_Coordinates(int choice);
void M_Timer(int choice);
void M_Version(int choice);
void M_HUD(int choice);
void M_MapGrid(int choice);
void M_WeaponChange(int choice);
void M_AimingHelp(int choice);
void M_MapRotation(int choice);
void M_FollowMode(int choice);
void M_Statistics(int choice);
void M_Crosshair(int choice);
void M_Jumping(int choice);
void M_WeaponRecoil(int choice);
void M_PlayerThrust(int choice);
void M_RespawnMonsters(int choice);
void M_FastMonsters(int choice);
void M_Autoaim(int choice);
void M_MaxGore(int choice);
void M_Footstep(int choice);
void M_Footclip(int choice);

void M_God(int choice);
void M_Noclip(int choice);
void M_Weapons(int choice);
void M_WeaponsA(int choice);
void M_WeaponsB(int choice);
void M_WeaponsC(int choice);
void M_WeaponsD(int choice);
void M_WeaponsE(int choice);
void M_WeaponsF(int choice);
void M_WeaponsG(int choice);
void M_WeaponsH(int choice);
void M_Keys(int choice);
void M_KeysA(int choice);
void M_KeysB(int choice);
void M_KeysC(int choice);
void M_KeysD(int choice);
void M_KeysE(int choice);
void M_KeysF(int choice);
void M_KeysG(int choice);
void M_Items(int choice);
void M_ItemsA(int choice);
void M_ItemsB(int choice);
void M_ItemsC(int choice);
void M_ItemsD(int choice);
void M_ItemsE(int choice);
void M_ItemsF(int choice);
void M_ItemsG(int choice);
void M_ItemsH(int choice);
void M_ItemsI(int choice);
void M_ItemsJ(int choice);
void M_Topo(int choice);
void M_Rift(int choice);
void M_RiftNow(int choice);
void M_Spin(int choice);

void M_Armor(int choice);
void M_ArmorA(int choice);
void M_ArmorB(int choice);
void M_ArmorC(int choice);
void M_Weapons(int choice);
void M_Items(int choice);
void M_Massacre(int choice);
void M_Screen(int choice);
void M_FPSCounter(int display_fps);
void M_Controls(int choice);
void M_System(int choice);
void M_Sound(int choice);
void M_Game(int choice);
void M_Game2(int choice);
void M_Debug(int choice);
void M_Cheats(int choice);
void M_Record(int choice);
void M_RMap(int choice);
void M_RSkill(int choice);
void M_StartRecord(int choice);
void M_DrawFilesMenu(void);
void M_DrawItems(void);
void M_DrawArmor(void);
void M_DrawWeapons(void);
void M_DrawKeys(void);
void M_DrawScreen(void);
void M_DrawKeyBindings(void);
void M_DrawControls(void);
void M_DrawSystem(void);
void M_DrawGame(void);
void M_DrawGame2(void);
void M_DrawGameDev(void);
void M_DrawDebug(void);
void M_DrawSound(void);
void M_DrawCheats(void);
void M_DrawRecord(void);

int  M_StringWidth(char *string);
int  M_StringHeight(char *string);


//
// DOOM MENU
//
enum
{
    newgame = 0,
    options,
    gamefiles,
    readthis,
    quitdoom,
    main_end
} main_e;

menuitem_t MainGameMenu[]=
{
    {1,"M_NGAME",M_NewGame,'n'},
    {1,"M_OPTION",M_Options,'o'},
    {1,"M_GFILES",M_GameFiles,'f'},
    // Another hickup with Special edition.
    {1,"M_RDTHIS",M_ReadThis,'r'},
    {1,"M_QUITG",M_QuitDOOM,'q'}
};

menu_t  MainDef =
{
    main_end,
    NULL,
    MainGameMenu,
    M_DrawMainMenu,
    97,64,
    0
};

enum
{
    loadgame,
    savegame,
    endgame,
    cheats,
    demos,
    files_end
} files_e;

menuitem_t FilesMenu[]=
{
    {1,"M_LOADG",M_LoadGame,'l'},
    {1,"M_SAVEG",M_SaveGame,'s'},
    {1,"M_ENDGAM",M_EndGame,'e'},
    {1,"M_CHEATS",M_Cheats,'c'},
    {1,"M_RECDMO",M_Record,'r'}
};

menu_t  FilesDef =
{
    files_end,
    &MainDef,
    FilesMenu,
    M_DrawFilesMenu,
    97,45,
    0
};

//
// EPISODE SELECT
//
enum
{
    ep1,
    ep2,
    ep3,
    ep4,
    ep_end
} episodes_e;

menuitem_t EpisodeMenu[]=
{
    {1,"M_EPI1", M_Episode,'k'},
    {1,"M_EPI2", M_Episode,'t'},
    {1,"M_EPI3", M_Episode,'i'},
    {1,"M_EPI4", M_Episode,'t'}
};

menu_t  EpiDef =
{
    ep_end,                // # of menu items
    &MainDef,              // previous menu
    EpisodeMenu,           // menuitem_t ->
    M_DrawEpisode,         // drawing routine ->
    48,63,                 // x,y
    ep1                    // lastOn
};

//
// NEW GAME
//
enum
{
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
} newgame_e;

menuitem_t NewGameMenu[]=
{
    {1,"M_JKILL", M_ChooseSkill, 'i'},
    {1,"M_ROUGH", M_ChooseSkill, 'h'},
    {1,"M_HURT",  M_ChooseSkill, 'h'},
    {1,"M_ULTRA", M_ChooseSkill, 'u'},
    {1,"",        M_ChooseSkill, 'n'}  // HACK: ALL ELSE THAN SHAREWARE 1.0 & 1.1
};

menu_t  NewDef =
{
    newg_end,           // # of menu items
    &EpiDef,            // previous menu
    NewGameMenu,        // menuitem_t ->
    M_DrawNewGame,      // drawing routine ->
    48,63,              // x,y
    hurtme              // lastOn
};



//
// OPTIONS MENU
//
enum
{
    screen,
    controls,
    sound,
    sys,
    game,
    debug,
    opt_end
} options_e;

menuitem_t OptionsMenu[]=
{
    {1,"M_SCRSET", M_Screen,'s'},
    {1,"M_CTLSET", M_Controls,'c'},
    {1,"M_SNDSET", M_Sound,'v'},
    {1,"M_SYSSET", M_System,'y'},
    {1,"M_GMESET", M_Game,'g'},
    {1,"M_DBGSET", M_Debug,'d'}
};

menu_t  OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    M_DrawOptions,
    60,57,
    0
};

//
// Read This! MENU 1 & 2
//
enum
{
    rdthsempty1,
    read1_end
} read_e;

menuitem_t ReadMenu1[] =
{
    {1,"",M_ReadThis2,0}
};

menu_t  ReadDef1 =
{
    read1_end,
    &MainDef,
    ReadMenu1,
    M_DrawReadThis1,
    280,185,
    0
};

enum
{
    rdthsempty2,
    read2_end
} read_e2;

menuitem_t ReadMenu2[]=
{
    {1,"",M_FinishReadThis,0}
};

menu_t  ReadDef2 =
{
    read2_end,
    &ReadDef1,
    ReadMenu2,
    M_DrawReadThis2,
    330,175,
    0
};

//
// CHEATS MENU
//
enum
{
    cheats_god,
    cheats_noclip,
    cheats_weapons,
    cheats_keys,
    cheats_armor,
    cheats_items,
    cheats_topo,
    cheats_massacre,
    cheats_rift,
    cheats_empty2,
    cheats_empty3,
    cheats_riftnow,
    cheats_empty4,
    cheats_spin,
    cheats_end
} cheats_e;

menuitem_t CheatsMenu[]=
{
    {2,"",M_God,'g'},
    {2,"",M_Noclip,'n'},
    {2,"",M_Weapons,'w'}, 
    {2,"",M_Keys,'k'},
    {2,"",M_Armor,'a'},
    {1,"",M_Items,'i'},
    {2,"",M_Topo,'t'},
    {2,"",M_Massacre,'m'},
    {2,"",M_Rift,'r'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {2,"",M_RiftNow,'e'},
    {-1,"",0,'\0'},
    {2,"",M_Spin,'s'}
};

menu_t  CheatsDef =
{
    cheats_end,
    &FilesDef,
    CheatsMenu,
    M_DrawCheats,
    75,28,
    0
};

enum
{
    items1,
    items_empty,
    items2,
    items3,
    items4,
    items5,
    items6,
    items7,
    items8,
    items9,
    items0,
    items_end
} items_e;

menuitem_t ItemsMenu[]=
{
    {2,"",M_ItemsA,'1'},
    {-1,"",0,'\0'},
    {2,"",M_ItemsC,'2'},
    {2,"",M_ItemsD,'3'},
    {2,"",M_ItemsE,'4'},
    {2,"",M_ItemsH,'5'},
    {2,"",M_ItemsI,'6'},
    {2,"",M_ItemsB,'7'},
    {2,"",M_ItemsF,'8'},
    {2,"",M_ItemsG,'9'},
    {2,"",M_ItemsJ,'0'}
};

menu_t  ItemsDef =
{
    items_end,
    &CheatsDef,
    ItemsMenu,
    M_DrawItems,
    80,57,
    0
};

enum
{
    keys1,
    keys_empty,
    keys2,
    keys3,
    keys4,
    keys5,
    keys6,
    keys7,
    keys_end
} keys_e;

menuitem_t KeysMenu[]=
{
    {2,"",M_KeysA,'1'},
    {-1,"",0,'\0'},
    {2,"",M_KeysB,'2'},
    {2,"",M_KeysC,'3'},
    {2,"",M_KeysD,'4'},
    {2,"",M_KeysE,'5'},
    {2,"",M_KeysF,'6'},
    {2,"",M_KeysG,'7'}
};

menu_t  KeysDef =
{
    keys_end,
    &CheatsDef,
    KeysMenu,
    M_DrawKeys,
    80,57,
    0
};

enum
{
    weapons1,
    weapons_empty,
    weapons2,
    weapons3,
    weapons4,
    weapons5,
    weapons6,
    weapons7,
    weapons8,
    weapons_end
} weapons_e;

menuitem_t WeaponsMenu[]=
{
    {2,"",M_WeaponsA,'1'},
    {-1,"",0,'\0'},
    {2,"",M_WeaponsG,'2'},
    {2,"",M_WeaponsB,'3'},
    {2,"",M_WeaponsC,'4'},
    {2,"",M_WeaponsD,'5'},
    {2,"",M_WeaponsE,'6'},
    {2,"",M_WeaponsF,'7'},
    {2,"",M_WeaponsH,'8'}
};

menu_t  WeaponsDef =
{
    weapons_end,
    &CheatsDef,
    WeaponsMenu,
    M_DrawWeapons,
    80,57,
    0
};

enum
{
    Armor1,
    Armor_empty,
    Armor2,
    Armor3,
    armor_end
} armor_e;

menuitem_t ArmorMenu[]=
{
    {2,"",M_ArmorA,'1'},
    {-1,"",0,'\0'},
    {2,"",M_ArmorB,'2'},
    {2,"",M_ArmorC,'3'}
};

menu_t  ArmorDef =
{
    armor_end,
    &CheatsDef,
    ArmorMenu,
    M_DrawArmor,
    80,57,
    0
};

enum
{
    gamma,
    screen_empty1,
    scrnsize,
    screen_empty2,
    screen_detail,
    screen_end
} screen_e;

menuitem_t ScreenMenu[]=
{
    {2,"M_BRGTNS",M_Brightness,'b'},
    {-1,"",0,'\0'},
    {2,"M_SCRNSZ",M_SizeDisplay,'s'},
    {-1,"",0,'\0'},
    {2,"M_DETAIL",M_ChangeDetail,'d'}
};

menu_t  ScreenDef =
{
    screen_end,
    &OptionsDef,
    ScreenMenu,
    M_DrawScreen,
    60,55,
    0
};

enum
{
    mousesens,
    controls_empty1,
    turnsens,
    controls_empty2,
    strafesens,
    controls_empty3,
    controls_freelook,
    mousespeed,
    controls_empty4,
    controls_keybindings,
    controls_end
} controls_e;

menuitem_t ControlsMenu[]=
{
    {2,"M_WSPEED",M_WalkingSpeed,'m'},
    {-1,"",0,'\0'},
    {2,"M_TSPEED",M_TurningSpeed,'t'},
    {-1,"",0,'\0'},
    {2,"M_SSPEED",M_StrafingSpeed,'s'},
    {-1,"",0,'\0'},
    {2,"M_FRLOOK",M_Freelook,'l'},
    {2,"M_FLKSPD",M_FreelookSpeed,'f'},
    {-1,"",0,'\0'},
    {1,"M_KBNDGS",M_KeyBindings,'b'}
};

menu_t  ControlsDef =
{
    controls_end,
    &OptionsDef,
    ControlsMenu,
    M_DrawControls,
    50,5,
    0
};

enum
{
    keybindings_up,
    keybindings_down,
    keybindings_left,
    keybindings_right,
    keybindings_select,
    keybindings_lefttrigger,
    keybindings_righttrigger,
    keybindings_fire,
    keybindings_jump,
    keybindings_run,
    keybindings_console,
    keybindings_aiminghelp,
    keybindings_clearall,
    keybindings_reset,
    keybindings_end
} keybindings_e;

menuitem_t KeyBindingsMenu[]=
{
    {5,"",M_KeyBindingsSetKey,0},
    {5,"",M_KeyBindingsSetKey,1},
    {5,"",M_KeyBindingsSetKey,2},
    {5,"",M_KeyBindingsSetKey,3},
    {5,"",M_KeyBindingsSetKey,4},
    {5,"",M_KeyBindingsSetKey,5},
    {5,"",M_KeyBindingsSetKey,6},
    {5,"",M_KeyBindingsSetKey,7},
    {5,"",M_KeyBindingsSetKey,8},
    {5,"",M_KeyBindingsSetKey,9},
    {5,"",M_KeyBindingsSetKey,10},
    {5,"",M_KeyBindingsSetKey,11},
    {5,"",M_KeyBindingsClearAll,'c'},
    {5,"",M_KeyBindingsReset,'r'}
};

menu_t  KeyBindingsDef =
{
    keybindings_end,
    &ControlsDef,
    KeyBindingsMenu,
    M_DrawKeyBindings,
    45,32,
    0
};

enum
{
    system_fps,
    system_ticker,
    system_end
} system_e;

menuitem_t SystemMenu[]=
{
    {2,"M_FPSCNT",M_FPS,'f'},
    {2,"M_DPLTCK",M_DisplayTicker,'t'}
};

menu_t  SystemDef =
{
    system_end,
    &OptionsDef,
    SystemMenu,
    M_DrawSystem,
    50,85,
    0
};

enum
{
    game_mapgrid,
    game_maprotation,
    game_followmode,
    game_statistics,
    game_hud,
    game_messages,
    game_crosshair,
    game_jumping,
    game_weapon,
    game_recoil,
    game_thrust,
    game_respawn,
    game_fast,
    game_empty,
    game_game2,
    game_end
} game_e;

menuitem_t GameMenu[]=
{
    {2,"",M_MapGrid,'g'},
    {2,"",M_MapRotation,'r'},
    {2,"",M_FollowMode,'f'},
    {2,"",M_Statistics,'s'},
    {2,"",M_HUD,'h'},
    {2,"",M_ChangeMessages,'m'},
    {2,"",M_Crosshair,'x'},
    {2,"",M_Jumping,'j'},
    {2,"",M_WeaponChange,'w'},
    {2,"",M_WeaponRecoil,'c'},
    {2,"",M_PlayerThrust,'p'},
    {2,"",M_RespawnMonsters,'t'},
    {2,"",M_FastMonsters,'d'},
    {-1,"",0,'\0'},
    {2,"",M_Game2,'n'}
};

menu_t  GameDef =
{
    game_end,
    &OptionsDef,
    GameMenu,
    M_DrawGame,
    85,22,
    0
};

enum
{
    game2_autoaim,
    game2_gore,
    game2_footstep,
    game2_footclip,
    game2_end
} game2_e;

menuitem_t GameMenu2[]=
{
    {2,"",M_Autoaim,'a'},
    {2,"",M_MaxGore,'o'},
    {2,"",M_Footstep,'f'},
    {2,"",M_Footclip,'c'},
};

menu_t  GameDef2 =
{
    game2_end,
    &GameDef,
    GameMenu2,
    M_DrawGame2,
    85,22,
    0
};

enum
{
    gamedev_mapgrid,
    gamedev_maprotation,
    gamedev_followmode,
    gamedev_statistics,
    gamedev_aiminghelp,
    gamedev_messages,
    gamedev_crosshair,
    gamedev_jumping,
    gamedev_weapon,
    gamedev_recoil,
    gamedev_thrust,
    gamedev_respawn,
    gamedev_fast,
    gamedev_empty,
    gamedev_game2,
    gamedev_end
} gamedev_e;

menuitem_t GameMenuDev[]=
{
    {2,"",M_MapGrid,'g'},
    {2,"",M_MapRotation,'r'},
    {2,"",M_FollowMode,'f'},
    {2,"",M_Statistics,'s'},
    {2,"",M_ChangeMessages,'m'},
    {2,"",M_AimingHelp,'h'},
    {2,"",M_Crosshair,'x'},
    {2,"",M_Jumping,'j'},
    {2,"",M_WeaponChange,'w'},
    {2,"",M_WeaponRecoil,'c'},
    {2,"",M_PlayerThrust,'p'},
    {2,"",M_RespawnMonsters,'t'},
    {2,"",M_FastMonsters,'d'},
    {-1,"",0,'\0'},
    {2,"",M_Game2,'n'}
};

menu_t  GameDefDev =
{
    gamedev_end,
    &OptionsDef,
    GameMenuDev,
    M_DrawGameDev,
    85,22,
    0
};

enum
{
    debug_coordinates,
    debug_timer,
    debug_version,
    debug_end
} debug_e;

menuitem_t DebugMenu[]=
{
    {2,"M_COORDS",M_Coordinates,'c'},
    {2,"M_TIMER",M_Timer,'t'},
    {2,"M_VRSN",M_Version,'v'}
};

menu_t  DebugDef =
{
    debug_end,
    &OptionsDef,
    DebugMenu,
    M_DrawDebug,
    45,75,
    0
};

//
// SOUND VOLUME MENU
//
enum
{
    sfx_vol,
    sfx_empty1,
    music_vol,
    sfx_empty2,
    type,
    channels,
    sound_end
} sound_e;

menuitem_t SoundMenu[]=
{
    {2,"M_SFXVOL",M_SfxVol,'s'},
    {-1,"",0,'\0'},
    {2,"M_MUSVOL",M_MusicVol,'m'},
    {-1,"",0,'\0'},
    {2,"M_MUSTYP",M_MusicType,'t'},
    {2,"M_SNDCHN",M_SoundChannels,'c'}
};

menu_t  SoundDef =
{
    sound_end,
    &OptionsDef,
    SoundMenu,
    M_DrawSound,
    70,55,
    0
};

//
// LOAD GAME MENU
//
enum
{
    load1,
    load2,
    load3,
    load4,
    load5,
    load6,
    load_end
} load_e;

menuitem_t LoadMenu[]=
{
    {1,"", M_LoadSelect,'1'},
    {1,"", M_LoadSelect,'2'},
    {1,"", M_LoadSelect,'3'},
    {1,"", M_LoadSelect,'4'},
    {1,"", M_LoadSelect,'5'},
    {1,"", M_LoadSelect,'6'}
};

menu_t  LoadDef =
{
    load_end,
    &FilesDef,
    LoadMenu,
    M_DrawLoad,
    80,54,
    0
};

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[]=
{
    {1,"", M_SaveSelect,'1'},
    {1,"", M_SaveSelect,'2'},
    {1,"", M_SaveSelect,'3'},
    {1,"", M_SaveSelect,'4'},
    {1,"", M_SaveSelect,'5'},
    {1,"", M_SaveSelect,'6'}
};

menu_t  SaveDef =
{
    load_end,
    &FilesDef,
    SaveMenu,
    M_DrawSave,
    80,54,
    0
};

enum
{
    record_map,
    record_empty1,
    record_empty2,
    record_skill,
    record_empty3,
    record_empty4,
    record_start,
    record_end
} record_e;

menuitem_t RecordMenu[]=
{
    {2,"M_RECMAP",M_RMap,'m'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {2,"M_SKILL",M_RSkill,'s'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {2,"M_STREC",M_StartRecord,'r'}
};

menu_t  RecordDef =
{
    record_end,
    &FilesDef,
    RecordMenu,
    M_DrawRecord,
    55,50,
    0
};

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
void M_ReadSaveStrings(void)
{
    FILE   *handle;
    int     i;
    char    name[256];

    for (i = 0;i < load_end;i++)
    {
        strcpy(name, P_SaveGameFile(i));

        handle = fopen(name, "rb");
        if (handle == NULL)
        {
            strcpy(&savegamestrings[i][0], EMPTYSTRING);
            LoadMenu[i].status = 0;
            continue;
        }
        fread(&savegamestrings[i], 1, SAVESTRINGSIZE, handle);
        fclose(handle);
        LoadMenu[i].status = 1;
    }
}


//
// M_LoadGame & Cie.
//
void M_DrawLoad(void)
{
    int             i;

    V_DrawPatchDirect(72, 28,
                      W_CacheLumpName(DEH_String("M_T_LGME"), PU_CACHE));

    for (i = 0;i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
        M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }

    M_WriteText(62, 148, "* INDICATES A SAVEGAME THAT WAS");
    M_WriteText(62, 158, "CREATED USING AN OPTIONAL PWAD!");
}



//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(int x,int y)
{
    int             i;
        
    V_DrawPatchDirect(x - 8, y + 7,
                      W_CacheLumpName(DEH_String("M_LSLEFT"), PU_CACHE));
        
    for (i = 0;i < 24;i++)
    {
        V_DrawPatchDirect(x, y + 7,
                          W_CacheLumpName(DEH_String("M_LSCNTR"), PU_CACHE));
        x += 8;
    }

    V_DrawPatchDirect(x, y + 7,
                      W_CacheLumpName(DEH_String("M_LSRGHT"), PU_CACHE));
}



//
// User wants to load this game
//
void M_LoadSelect(int choice)
{
    char    name[256];
        
    strcpy(name, P_SaveGameFile(choice));

    G_LoadGame (name);
    M_ClearMenus ();
}

//
// Selected from DOOM menu
//
void M_LoadGame (int choice)
{
    if (netgame)
    {
        M_StartMessage(DEH_String(LOADNET),NULL,false);
        return;
    }
    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings();
}


//
//  M_SaveGame & Cie.
//
void M_DrawSave(void)
{
    int             i;
        
    V_DrawPatchDirect(72, 28, W_CacheLumpName(DEH_String("M_T_SGME"), PU_CACHE));
    for (i = 0;i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
        M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }
        
    if (saveStringEnter)
    {
        i = M_StringWidth(savegamestrings[saveSlot]);
        M_WriteText(LoadDef.x + i,LoadDef.y+LINEHEIGHT*saveSlot,"_");
    }

    M_WriteText(62, 148, "* INDICATES A SAVEGAME THAT WAS");
    M_WriteText(62, 158, "CREATED USING AN OPTIONAL PWAD!");
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot)
{
    G_SaveGame (slot,savegamestrings[slot]);
    M_ClearMenus ();
}

//
// User wants to save. Start string input for M_Responder
//
void M_SaveSelect(int choice)
{
    // we are going to be intercepting all chars
    saveStringEnter = 1;
    
    saveSlot = choice;

    time_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);

    int day = aTime->tm_mday;
    int month = aTime->tm_mon + 1;
    int year = aTime->tm_year + 1900;
    int hour = aTime->tm_hour;
    int min = aTime->tm_min;

    if(gamemode == shareware || gamemode == registered || gamemode == retail)
    {
        if(extra_wad_loaded == 1)
            sprintf(savegamestrings[choice], "e%dm%d %d/%d/%d %2.2d:%2.2d *",
                    gameepisode, gamemap, year, month, day, hour, min);
        else
            sprintf(savegamestrings[choice], "e%dm%d %d/%d/%d %2.2d:%2.2d",
                    gameepisode, gamemap, year, month, day, hour, min);
    }
    else
    {
        if(extra_wad_loaded == 1)
            sprintf(savegamestrings[choice], "map%2.2d %d/%d/%d %2.2d:%2.2d *",
                    gamemap, year, month, day, hour, min);
        else
            sprintf(savegamestrings[choice], "map%2.2d %d/%d/%d %2.2d:%2.2d",
                    gamemap, year, month, day, hour, min);
    }
    strcpy(saveOldString,savegamestrings[choice]);
    if (!strcmp(savegamestrings[choice],EMPTYSTRING))
        savegamestrings[choice][0] = 0;
    saveCharIndex = strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
//
void M_SaveGame (int choice)
{
    if (!usergame)
    {
        M_StartMessage(DEH_String(SAVEDEAD),NULL,true);
        return;
    }
        
    if (gamestate != GS_LEVEL)
        return;
        
    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings();
}

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
    char *lumpname = "CREDIT";
    int skullx = 330, skully = 175;

    inhelpscreens = true;
    
    // Different versions of Doom 1.9 work differently

    switch (gameversion)
    {
        case exe_doom_1_9:
        case exe_hacx:

            if (gamemode == commercial)
            {
                // Doom 2

                lumpname = "HELP";

                skullx = 330;
                skully = 165;
            }
            else
            {
                // Doom 1
                // HELP2 is the first screen shown in Doom 1
                
                lumpname = "HELP2";

                skullx = 280;
                skully = 185;
            }
            break;

        case exe_ultimate:
        case exe_chex:

            // Ultimate Doom always displays "HELP1".

            // Chex Quest version also uses "HELP1", even though it is based
            // on Final Doom.

            lumpname = "HELP1";

            break;

        case exe_final:
        case exe_final2:

            // Final Doom always displays "HELP".

            lumpname = "HELP";

            break;

        default:
            I_Error("Unhandled game version");
            break;
    }

    lumpname = DEH_String(lumpname);
    
    V_DrawPatchDirect (0, 0, W_CacheLumpName(lumpname, PU_CACHE));

    ReadDef1.x = skullx;
    ReadDef1.y = skully;
}



//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
    inhelpscreens = true;

    // We only ever draw the second page if this is 
    // gameversion == exe_doom_1_9 and gamemode == registered

    V_DrawPatchDirect(0, 0, W_CacheLumpName(DEH_String("HELP1"), PU_CACHE));
}


//
// Change Sfx & Music volumes
//
void M_DrawSound(void)
{
    int offset = 0;

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchDirect (65, 15, W_CacheLumpName(DEH_String("M_T_XSET"), PU_CACHE));
    else
    {
        offset = 5;

        V_DrawPatchDirect (65, 15, W_CacheLumpName(DEH_String("M_SNDSET"), PU_CACHE));
    }

    if(fsize == 28422764)
        offset = 3;

    M_DrawThermo(SoundDef.x, SoundDef.y - offset + 2 + LINEHEIGHT * (sfx_vol + 1),
                 16, sfxVolume);

    M_DrawThermo(SoundDef.x, SoundDef.y - offset + 2 + LINEHEIGHT * (music_vol + 1),
                 16, musicVolume);

    if(mus_engine == 1)
        V_DrawPatch (225, 122, W_CacheLumpName(DEH_String("M_MUSOPL"), PU_CACHE));
    else
        V_DrawPatch (225, 122, W_CacheLumpName(DEH_String("M_MUSOGG"), PU_CACHE));

    if(swap_sound_chans)
        V_DrawPatch (225, 135, W_CacheLumpName(DEH_String("M_MSGON"), PU_CACHE));
    else
        V_DrawPatch (225, 135, W_CacheLumpName(DEH_String("M_MSGOFF"), PU_CACHE));

    if(mus_engine < 1)
        mus_engine = 1;
    else if(mus_engine > 2)
        mus_engine = 2;
}

void M_Sound(int choice)
{
    M_SetupNextMenu(&SoundDef);
}

void M_SfxVol(int choice)
{
    switch(choice)
    {
      case 0:
        if (sfxVolume)
            sfxVolume--;
        break;
      case 1:
        if (sfxVolume < 15)
            sfxVolume++;
        break;
    }
    S_SetSfxVolume(sfxVolume * 8);
}

void M_MusicVol(int choice)
{
    switch(choice)
    {
      case 0:
        if (musicVolume)
            musicVolume--;
        break;
      case 1:
        if (musicVolume < 15)
            musicVolume++;
        break;
    }
    S_SetMusicVolume(musicVolume * 8);
}

void M_MusicType(int choice)
{
    switch(choice)
    {
    case 0:
        if(mus_engine > 1)
        {
            snd_musicdevice = SNDDEVICE_SB;
            mus_engine--;
        }
        break;
    case 1:
        if(mus_engine < 2)
        {
            snd_musicdevice = SNDDEVICE_GENMIDI;
            mus_engine++;
        }
        break;
    }
}

void M_SoundChannels(int choice)
{
    switch(choice)
    {
    case 0:
        if(swap_sound_chans == true)
        {
            swap_sound_chans = false;
        }
        break;
    case 1:
        if(swap_sound_chans == false)
        {
            swap_sound_chans = true;
        }
        break;
    }
}

//
// M_DrawMainMenu
//
void M_DrawMainMenu(void)
{
    V_DrawPatchDirect(94, 2,
                      W_CacheLumpName(DEH_String("M_DOOM"), PU_CACHE));
}




//
// M_NewGame
//
void M_DrawNewGame(void)
{
    V_DrawPatchDirect(96, 14, W_CacheLumpName(DEH_String("M_NEWG"), PU_CACHE));
    V_DrawPatchDirect(54, 38, W_CacheLumpName(DEH_String("M_SKILL"), PU_CACHE));

    // HACK: NOT FOR SHARE 1.0 & 1.1 && REG 1.1
    if(fsize != 4207819 && fsize != 4274218 && fsize != 10396254)
        V_DrawPatchDirect(48, 127, W_CacheLumpName(DEH_String("M_NMARE"), PU_CACHE));
}

void M_NewGame(int choice)
{
    if (netgame && !demoplayback)
    {
        M_StartMessage(DEH_String(NEWGAME),NULL,false);
        return;
    }

    // Chex Quest disabled the episode select screen, as did Doom II.

    if (gamemode == commercial || gameversion == exe_chex)
        M_SetupNextMenu(&NewDef);
    else
        M_SetupNextMenu(&EpiDef);
}


//
//      M_Episode
//
int     epi;

void M_DrawEpisode(void)
{
    V_DrawPatchDirect(54, 38, W_CacheLumpName(DEH_String("M_EPISOD"), PU_CACHE));
}

void M_VerifyNightmare(int ch)
{
    if (ch != key_menu_forward)
        return;

    G_DeferedInitNew(nightmare,epi+1,1);
    M_ClearMenus ();
}

void M_ChooseSkill(int choice)
{
    if (choice == nightmare)
    {
        M_StartMessage(DEH_String(NIGHTMARE),M_VerifyNightmare,true);
        return;
    }

    if(fsize == 12361532)
        G_DeferedInitNew(choice,epi,1);
    else
        G_DeferedInitNew(choice,epi+1,1);

    if(gameepisode == 1)
        secret_1 = false;
    else
        secret_2 = false;

    M_ClearMenus ();
}

void M_Episode(int choice)
{
    if ( (gamemode == shareware)
         && choice)
    {
        M_StartMessage(DEH_String(SWSTRING),NULL,true);
        M_SetupNextMenu(&ReadDef1);
        return;
    }

    // Yet another hack...
    if ( (gamemode == registered)
         && (choice > 2))
    {
      C_Printf("M_Episode: 4th episode requires UltimateDOOM\n");
      choice = 0;
    }
         
    epi = choice;
    M_SetupNextMenu(&NewDef);
}



//
// M_Options
//

void M_DrawOptions(void)
{
    V_DrawPatchDirect(108, 15, W_CacheLumpName(DEH_String("M_OPTTTL"),
                                               PU_CACHE));
}

void M_DrawItems(void)
{
    V_DrawPatchDirect(123, 15, W_CacheLumpName(DEH_String("M_T_ITMS"),
                                               PU_CACHE));

    M_WriteText(80, 55, DEH_String("GIVE THEM ALL AT ONCE"));

    if(fsize != 12361532 && fsize != 19321722)
    {
        M_WriteText(80, 75, DEH_String("RADIATION SHIELDING SUIT"));
        M_WriteText(80, 85, DEH_String("COMPUTER AREA MAP"));
        M_WriteText(80, 95, DEH_String("LIGHT AMPLIFICATION VISOR"));
        M_WriteText(80, 125, DEH_String("PARTIAL INVISIBILITY"));
        M_WriteText(80, 135, DEH_String("INVULNERABILITY!"));
        M_WriteText(80, 145, DEH_String("BERSERK!"));
        M_WriteText(80, 155, DEH_String("BACKPACK"));
    }

    if(fsize == 19321722)
    {
        M_WriteText(80, 75, DEH_String("VULCAN RUBBER BOOTS"));
        M_WriteText(80, 85, DEH_String("SI ARRAY MAPPING"));
        M_WriteText(80, 95, DEH_String("INFRARED VISOR"));
        M_WriteText(80, 125, DEH_String("ENK BLINDNESS"));
        M_WriteText(80, 135, DEH_String("FORCE FIELD"));
        M_WriteText(80, 145, DEH_String("007 MICROTEL"));
    }

    if(fsize == 12361532)
    {
        M_WriteText(80, 75, DEH_String("SLIME-PROOF SUIT"));
        M_WriteText(80, 85, DEH_String("COMPUTER AREA MAP"));
        M_WriteText(80, 95, DEH_String("ULTRA GOGGLES"));
    }

    M_WriteText(80, 105, DEH_String("FULL HEALTH (100)"));
    M_WriteText(80, 115, DEH_String("FULL HEALTH (200)"));
}

void M_DrawArmor(void)
{
    V_DrawPatchDirect(115, 15, W_CacheLumpName(DEH_String("M_T_ARMR"),
                                               PU_CACHE));

    M_WriteText(80, 55, DEH_String("GIVE THEM ALL AT ONCE"));

    if(fsize != 12361532 && fsize != 19321722)
    {
        M_WriteText(80, 75, DEH_String("GREEN ARMOR"));
        M_WriteText(80, 85, DEH_String("BLUE ARMOR"));
    }

    if(fsize == 12361532)
    {
        M_WriteText(80, 75, DEH_String("CHEX(R) ARMOR"));
        M_WriteText(80, 85, DEH_String("SUPER CHEX(R) ARMOR"));
    }

    if(fsize == 19321722)
    {
        M_WriteText(80, 75, DEH_String("KEVLAR VEST"));
        M_WriteText(80, 85, DEH_String("SUPER KEVLAR VEST"));
    }
}

void M_DrawWeapons(void)
{
    V_DrawPatchDirect(103, 15, W_CacheLumpName(DEH_String("M_T_WPNS"),
                                               PU_CACHE));

    M_WriteText(80, 55, DEH_String("GIVE THEM ALL AT ONCE"));

    if(fsize != 19321722 && fsize != 12361532)
    {
        M_WriteText(80, 75, DEH_String("CHAINSAW"));
        M_WriteText(80, 85, DEH_String("SHOTGUN"));
        M_WriteText(80, 95, DEH_String("CHAINGUN"));
        M_WriteText(80, 105, DEH_String("ROCKET LAUNCHER"));

        if(fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460 && fsize != 4234124 && fsize != 4196020)
        {
            M_WriteText(80, 115, DEH_String("PLASMA CANNON"));
            M_WriteText(80, 125, DEH_String("BFG 9000"));
        }

        if(fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                fsize == 14607420 || fsize == 14604584 || fsize == 18195736 ||
                fsize == 14683458 || fsize == 18654796 || fsize == 18240172 ||
                fsize == 17420824 || fsize == 14677988 || fsize == 14691821 ||
                fsize == 28422764)
            M_WriteText(80, 135, DEH_String("SUPER SHOTGUN"));
    }

    if(fsize == 19321722)
    {
        M_WriteText(80, 75, DEH_String("HOIG REZNATOR"));
        M_WriteText(80, 85, DEH_String("TAZER"));
        M_WriteText(80, 95, DEH_String("UZI"));
        M_WriteText(80, 105, DEH_String("PHOTON 'ZOOKA"));
        M_WriteText(80, 115, DEH_String("STICK"));
        M_WriteText(80, 125, DEH_String("NUKER"));
        M_WriteText(80, 135, DEH_String("CRYOGUN"));
    }

    if(fsize == 12361532)
    {
        M_WriteText(80, 75, DEH_String("SUPER BOOTSPORK"));
        M_WriteText(80, 85, DEH_String("LARGE ZORCHER"));
        M_WriteText(80, 95, DEH_String("RAPID ZORCHER"));
        M_WriteText(80, 105, DEH_String("ZORCH PROPULSOR"));
        M_WriteText(80, 115, DEH_String("PHASING ZORCHER"));
        M_WriteText(80, 125, DEH_String("LARGE AREA ZORCHING DEVICE"));
    }
}

void M_DrawKeys(void)
{
    V_DrawPatchDirect(125, 15, W_CacheLumpName(DEH_String("M_T_KEYS"),
                                               PU_CACHE));

    M_WriteText(80, 55, DEH_String("GIVE THEM ALL AT ONCE"));

    M_WriteText(80, 75, DEH_String("BLUE KEYCARD"));
    M_WriteText(80, 85, DEH_String("YELLOW KEYCARD"));
    M_WriteText(80, 95, DEH_String("RED KEYCARD"));
    M_WriteText(80, 105, DEH_String("BLUE SKULLKEY"));
    M_WriteText(80, 115, DEH_String("YELLOW SKULLKEY"));
    M_WriteText(80, 125, DEH_String("RED SKULLKEY"));
}

void M_DrawScreen(void)
{
    int offset = 0;

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchDirect(58, 15, W_CacheLumpName(DEH_String("M_T_SSET"),
                                               PU_CACHE));
    else
        V_DrawPatchDirect(58, 15, W_CacheLumpName(DEH_String("M_SCRSET"),
                                               PU_CACHE));

    if(fsize == 28422764)
        offset = 3;

    M_DrawThermo(OptionsDef.x, OptionsDef.y - offset + LINEHEIGHT * (gamma + 1),
                 5, usegamma);

    V_DrawPatchDirect(OptionsDef.x + 175, OptionsDef.y + LINEHEIGHT + 11.5 *
                      screen_detail, W_CacheLumpName(DEH_String
                      (detailNames[detailLevel]), PU_CACHE));

    M_DrawThermo(OptionsDef.x, OptionsDef.y -offset + LINEHEIGHT * (scrnsize + 1),
                 9, screenSize);
}

void M_DrawGame(void)
{
    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchDirect(70, 0, W_CacheLumpName(DEH_String("M_T_GSET"),
                                               PU_CACHE));
    else
        V_DrawPatchDirect(70, 0, W_CacheLumpName(DEH_String("M_GMESET"),
                                               PU_CACHE));

    M_WriteText(85, 20, DEH_String("MAP GRID"));
    M_WriteText(85, 30, DEH_String("MAP ROTATION"));
    M_WriteText(85, 40, DEH_String("FOLLOW MODE"));
    M_WriteText(85, 50, DEH_String("STATISTICS"));
    M_WriteText(85, 60, DEH_String("EXTRA HUD"));
    M_WriteText(85, 70, DEH_String("MESSAGES"));
    M_WriteText(85, 80, DEH_String("CROSSHAIR"));
    M_WriteText(85, 90, DEH_String("JUMPING"));
    M_WriteText(85, 100, DEH_String("WEAPON CHANGE"));
    M_WriteText(85, 110, DEH_String("WEAPON RECOIL"));
    M_WriteText(85, 120, DEH_String("PLAYER THRUST"));
    M_WriteText(85, 130, DEH_String("RESPAWN MONSTERS"));
    M_WriteText(85, 140, DEH_String("FAST MONSTERS"));
    M_WriteText(85, 160, DEH_String("MORE OPTIONS"));

    if(drawgrid == 1)
        M_WriteText(220, 20, DEH_String("ON"));
    else if(drawgrid == 0)
        M_WriteText(220, 20, DEH_String("OFF"));

    if(am_rotate == true)
        M_WriteText(220, 30, DEH_String("ON"));
    else if(am_rotate == false)
        M_WriteText(220, 30, DEH_String("OFF"));

    if(followplayer == 1)
        M_WriteText(220, 40, DEH_String("ON"));
    else if(followplayer == 0)
        M_WriteText(220, 40, DEH_String("OFF"));

    if(show_stats == 1)
        M_WriteText(220, 50, DEH_String("ON"));
    else if (show_stats == 0)
        M_WriteText(220, 50, DEH_String("OFF"));

    if(hud)
        M_WriteText(220, 60, DEH_String("ON"));
    else
        M_WriteText(220, 60, DEH_String("OFF"));

    if(showMessages)
        M_WriteText(220, 70, DEH_String("ON"));
    else
        M_WriteText(220, 70, DEH_String("OFF"));

    if(crosshair == 1)
        M_WriteText(220, 80, DEH_String("ON"));
    else if (crosshair == 0)
        M_WriteText(220, 80, DEH_String("OFF"));

    if(jumping)
        M_WriteText(220, 90, DEH_String("ON"));
    else
        M_WriteText(220, 90, DEH_String("OFF"));

    if(use_vanilla_weapon_change == 1)
        M_WriteText(220, 100, DEH_String("SLOW"));
    else if(use_vanilla_weapon_change == 0)
        M_WriteText(220, 100, DEH_String("FAST"));

    if(d_recoil)
        M_WriteText(220, 110, DEH_String("ON"));
    else
        M_WriteText(220, 110, DEH_String("OFF"));

    if(d_thrust)
        M_WriteText(220, 120, DEH_String("ON"));
    else
        M_WriteText(220, 120, DEH_String("OFF"));

    if(respawnparm)
        M_WriteText(220, 130, DEH_String("ON"));
    else
        M_WriteText(220, 130, DEH_String("OFF"));

    if(fastparm)
        M_WriteText(220, 140, DEH_String("ON"));
    else
        M_WriteText(220, 140, DEH_String("OFF"));
}

void M_DrawGame2(void)
{
    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchDirect(70, 0, W_CacheLumpName(DEH_String("M_T_GSET"),
                                               PU_CACHE));
    else
        V_DrawPatchDirect(70, 0, W_CacheLumpName(DEH_String("M_GMESET"),
                                               PU_CACHE));

    M_WriteText(85, 20, DEH_String("AUTOAIM"));
    M_WriteText(85, 30, DEH_String("MORE GORE"));
    M_WriteText(85, 40, DEH_String("PLAYER FOOTSTEPS"));
    M_WriteText(85, 50, DEH_String("HERETIC FOOTCLIPS"));

    if(autoaim)
        M_WriteText(220, 20, DEH_String("ON"));
    else
        M_WriteText(220, 20, DEH_String("OFF"));

    if(d_maxgore)
        M_WriteText(220, 30, DEH_String("ON"));
    else
        M_WriteText(220, 30, DEH_String("OFF"));

    if(d_footstep)
        M_WriteText(220, 40, DEH_String("ON"));
    else
        M_WriteText(220, 40, DEH_String("OFF"));

    if(d_footclip)
        M_WriteText(220, 50, DEH_String("ON"));
    else
        M_WriteText(220, 50, DEH_String("OFF"));
}

void M_DrawGameDev(void)
{
    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchDirect(70, 0, W_CacheLumpName(DEH_String("M_T_GSET"),
                                               PU_CACHE));
    else
        V_DrawPatchDirect(70, 0, W_CacheLumpName(DEH_String("M_GMESET"),
                                               PU_CACHE));

    M_WriteText(85, 20, DEH_String("MAP GRID"));
    M_WriteText(85, 30, DEH_String("MAP ROTATION"));
    M_WriteText(85, 40, DEH_String("FOLLOW MODE"));
    M_WriteText(85, 50, DEH_String("STATISTICS"));
    M_WriteText(85, 60, DEH_String("MESSAGES"));
    M_WriteText(85, 70, DEH_String("AIMING HELP"));
    M_WriteText(85, 80, DEH_String("CROSSHAIR"));
    M_WriteText(85, 90, DEH_String("JUMPING"));
    M_WriteText(85, 100, DEH_String("WEAPON CHANGE"));
    M_WriteText(85, 110, DEH_String("WEAPON RECOIL"));
    M_WriteText(85, 120, DEH_String("PLAYER THRUST"));
    M_WriteText(85, 130, DEH_String("RESPAWN MONSTERS"));
    M_WriteText(85, 140, DEH_String("FAST MONSTERS"));
    M_WriteText(85, 160, DEH_String("MORE OPTIONS"));

    if(drawgrid == 1)
        M_WriteText(220, 20, DEH_String("ON"));
    else if(drawgrid == 0)
        M_WriteText(220, 20, DEH_String("OFF"));

    if(am_rotate == true)
        M_WriteText(220, 30, DEH_String("ON"));
    else if(am_rotate == false)
        M_WriteText(220, 30, DEH_String("OFF"));

    if(followplayer == 1)
        M_WriteText(220, 40, DEH_String("ON"));
    else if(followplayer == 0)
        M_WriteText(220, 40, DEH_String("OFF"));

    if(show_stats == 1)
        M_WriteText(220, 50, DEH_String("ON"));
    else if (show_stats == 0)
        M_WriteText(220, 50, DEH_String("OFF"));

    if(showMessages)
        M_WriteText(220, 60, DEH_String("ON"));
    else
        M_WriteText(220, 60, DEH_String("OFF"));

    if(aiming_help)
        M_WriteText(220, 70, DEH_String("ON"));
    else
        M_WriteText(220, 70, DEH_String("OFF"));

    if(crosshair == 1)
        M_WriteText(220, 80, DEH_String("ON"));
    else if (crosshair == 0)
        M_WriteText(220, 80, DEH_String("OFF"));

    if(jumping)
        M_WriteText(220, 90, DEH_String("ON"));
    else
        M_WriteText(220, 90, DEH_String("OFF"));

    if(use_vanilla_weapon_change == 1)
        M_WriteText(220, 100, DEH_String("SLOW"));
    else if(use_vanilla_weapon_change == 0)
        M_WriteText(220, 100, DEH_String("FAST"));

    if(d_recoil)
        M_WriteText(220, 110, DEH_String("ON"));
    else
        M_WriteText(220, 110, DEH_String("OFF"));

    if(d_thrust)
        M_WriteText(220, 120, DEH_String("ON"));
    else
        M_WriteText(220, 120, DEH_String("OFF"));

    if(respawnparm)
        M_WriteText(220, 130, DEH_String("ON"));
    else
        M_WriteText(220, 130, DEH_String("OFF"));

    if(fastparm)
        M_WriteText(220, 140, DEH_String("ON"));
    else
        M_WriteText(220, 140, DEH_String("OFF"));
}

void DetectState(void)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_DEAD)
    {
        M_StartMessage(DEH_String("CHEATING NOT ALLOWED - YOU'RE DEAD"),
        NULL, true);
    }
    else if(!netgame && demoplayback && gamestate == GS_LEVEL
        && players[consoleplayer].playerstate == PST_LIVE)
    {
        M_StartMessage(DEH_String("CHEATING NOT ALLOWED IN DEMO MODE"),
        NULL, true);
    }
    else if(!netgame && demoplayback && gamestate == GS_LEVEL
        && players[consoleplayer].playerstate == PST_DEAD)
    {
        M_StartMessage(DEH_String("CHEATING NOT ALLOWED IN DEMO MODE"),
        NULL, true);
    }
    else if(!netgame && demoplayback && gamestate != GS_LEVEL)
    {
        M_StartMessage(DEH_String("CHEATING NOT ALLOWED IN DEMO MODE"),
        NULL, true);
    }
    else if(!netgame && !demoplayback && gamestate != GS_LEVEL)
    {
        M_StartMessage(DEH_String("CHEATING NOT ALLOWED IN DEMO MODE"),
        NULL, true);
    }
    else if(netgame)
    {
        M_StartMessage(DEH_String("CHEATING NOT ALLOWED FOR NET GAME"),
        NULL, true);
    }

    if(gameskill == sk_nightmare)
    {
        M_StartMessage(DEH_String("CHEATING DISABLED - NIGHTMARE SKILL"),
        NULL, true);
    }
}

void M_DrawCheats(void)
{
    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch (110, 6, W_CacheLumpName(DEH_String("M_T_CHTS"), PU_CACHE));
    else
        V_DrawPatch (110, 6, W_CacheLumpName(DEH_String("M_CHEATS"), PU_CACHE));

    M_WriteText(72, 26, DEH_String("GOD MODE"));

    if (players[consoleplayer].cheats & CF_GODMODE)
        M_WriteText(215, 26, DEH_String("ON"));
    else
        M_WriteText(215, 26, DEH_String("OFF"));

    M_WriteText(72, 36, DEH_String("NOCLIP"));

    if (players[consoleplayer].cheats & CF_NOCLIP)
        M_WriteText(215, 36, DEH_String("ON"));
    else
        M_WriteText(215, 36, DEH_String("OFF"));

    M_WriteText(72, 46, DEH_String("WEAPONS..."));

    M_WriteText(72, 56, DEH_String("KEYS..."));

    M_WriteText(72, 66, DEH_String("ARMOR..."));

    M_WriteText(72, 76, DEH_String("ITEMS..."));

    M_WriteText(72, 86, DEH_String("AUTOMAP REVEAL:"));

    if(!cheating)
        M_WriteText(215, 86, DEH_String("OFF"));
    else if (cheating && cheeting!=2)
        M_WriteText(197, 86, DEH_String("WALLS"));
    else if (cheating && cheeting==2)          
        M_WriteText(215, 86, DEH_String("ALL"));

    M_WriteText(72, 96, DEH_String("KILL ALL ENEMIES"));
    M_WriteText(72, 106, DEH_String("WARP TO MAP:"));
    M_WriteText(72, 136, DEH_String("EXECUTE WARPING"));

    M_WriteText(72, 156, DEH_String("PLAY MUSIC TITLE:"));

    if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
        fsize == 4207819 || fsize == 4274218 || fsize == 4225504 || 
        fsize == 4225460 || fsize == 4234124 || fsize == 4196020 ||
        fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
        fsize == 11159840 || fsize == 12408292 || fsize == 12361532 ||
        fsize == 12474561 || fsize == 12538385 || fsize == 12487824)
    {
        if(epi == 0)
            epi = 1;

        if(epi == 1 && map == 0)
            map = 1;
        else if(epi == 4 && map == 10)
            map = 9;

        if(fsize == 12538385 && epi == 1 && map == 10)
            M_WriteText(72, 116, "E1M10: SEWERS");

        if(epi == 1 && gameversion != exe_chex && map != 10)
            M_WriteText(72, 116, maptext[map]);
        else if(epi == 2 && gameversion != exe_chex)
            M_WriteText(72, 116, maptext[map+9]);
        else if(epi == 3 && gameversion != exe_chex)
            M_WriteText(72, 116, maptext[map+18]);
        else if(epi == 4 && gameversion != exe_chex)
            M_WriteText(72, 116, maptext[map+27]);
        else if(epi == 1 && gameversion == exe_chex && !is_chex_2)
            M_WriteText(72, 116, maptext[map+132]);
        else if(epi == 1 && gameversion == exe_chex && is_chex_2)
            M_WriteText(72, 116, maptext[map+137]);
    }

    if((fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
            fsize == 14607420 || fsize == 14604584 || fsize == 14677988 ||
            fsize == 14683458 || fsize == 14691821 || fsize == 28422764) &&
            map < 33)
        M_WriteText(72, 116, maptext[map+36]);

    if((fsize == 14677988 || fsize == 14683458 || fsize == 14691821) &&
        map == 33)
        M_WriteText(72, 116, "LEVEL 33: BETRAY");        

    if(fsize == 18195736 || fsize == 18654796)
        M_WriteText(72, 116, maptext[map+68]);

    if(fsize == 18240172 || fsize == 17420824)
        M_WriteText(72, 116, maptext[map+100]);

    if(fsize == 19321722)
        M_WriteText(72, 116, maptext[map+142]);

    if(fsize == 12361532 && !is_chex_2)
        M_WriteText(72, 116, maptext[map+132]);

    if(fsize == 12361532 && is_chex_2)
        M_WriteText(72, 116, maptext[map+137]);

    if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
        fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
        fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
    {
        if(tracknum == 0)
            tracknum = 1;

        if(faketracknum == 0)
            faketracknum = 1;

        M_WriteText(220, 156, songtextbeta[tracknum]);
    }
    else if(fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
            fsize == 11159840 || fsize == 12408292 || fsize == 12361532 ||
            fsize == 12474561 || fsize == 12538385 || fsize == 12487824)
    {
        if(tracknum == 0)
            tracknum = 1;

        if(fsize == 12361532)
            M_WriteText(220, 156, songtextchex[tracknum]);
        else
            M_WriteText(220, 156, songtext[tracknum]);
    }
    else if(fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
            fsize == 14607420 || fsize == 14604584 || fsize == 19321722 ||
            fsize == 28422764 || fsize == 14677988 || fsize == 14683458 ||
            fsize == 14691821)
    {
        if(tracknum == 1)
            tracknum = 33;
        if(fsize == 19321722)
            M_WriteText(220, 156, songtext2hacx[tracknum]);
        else if(fsize == 28422764)
            M_WriteText(220, 156, songtext2fd[tracknum]);
        else
            M_WriteText(220, 156, songtext2[tracknum]);
    }
    else if(fsize == 18195736 || fsize == 18654796)
    {
        if(tracknum == 1)
            tracknum = 33;
        M_WriteText(220, 156, songtexttnt[tracknum]);
    }
    else if(fsize == 18240172 || fsize == 17420824)
    {
        if(tracknum == 1)
            tracknum = 33;
        M_WriteText(220, 156, songtextplut[tracknum]);
    }
    DetectState();
}

void M_DrawRecord(void)
{
    int offset = 0;

    V_DrawPatchDirect(58, 15, W_CacheLumpName(DEH_String("M_T_DREC"),
                                               PU_CACHE));
    if (rmap == 0)
        rmap = 1;

    if (fsize != 14943400 && fsize != 14824716 && fsize != 14612688 &&
        fsize != 14607420 && fsize != 14604584 && fsize != 18195736 &&
        fsize != 18654796 && fsize != 18240172 && fsize != 17420824 &&
        fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
    {
        if(repi == 1)
            V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("M_R_E1M"), PU_CACHE));
        else if(repi == 2)
            V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("M_R_E2M"), PU_CACHE));
        else if(repi == 3)
            V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("M_R_E3M"), PU_CACHE));
        else if(repi == 4)
            V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("M_R_E4M"), PU_CACHE));

        if(rmap == 1)
                V_DrawPatch (98, 68, W_CacheLumpName(DEH_String("M_R_NUM1"),
                PU_CACHE));
        else if(rmap == 2)
                V_DrawPatch (98, 68, W_CacheLumpName(DEH_String("M_R_NUM2"),
                PU_CACHE));
        else if(rmap == 3)
                V_DrawPatch (98, 68, W_CacheLumpName(DEH_String("M_R_NUM3"),
                PU_CACHE));
        else if(rmap == 4)
                V_DrawPatch (98, 68, W_CacheLumpName(DEH_String("M_R_NUM4"), 
                PU_CACHE));
        else if(rmap == 5)
                V_DrawPatch (98, 68, W_CacheLumpName(DEH_String("M_R_NUM5"),
                PU_CACHE));
        else if(rmap == 6)
                V_DrawPatch (98, 68, W_CacheLumpName(DEH_String("M_R_NUM6"),
                PU_CACHE));
        else if(rmap == 7)
                V_DrawPatch (98, 68, W_CacheLumpName(DEH_String("M_R_NUM7"), 
                PU_CACHE));
        else if(rmap == 8)
                V_DrawPatch (98, 68, W_CacheLumpName(DEH_String("M_R_NUM8"),
                PU_CACHE));
        else if(rmap == 9)
                V_DrawPatch (98, 68, W_CacheLumpName(DEH_String("M_R_NUM9"),
                PU_CACHE));
    }
    else if(fsize != 12361532)
    {
        if(rmap < 10)
            V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("M_R_MAP0"),
            PU_CACHE));
        else if(rmap < 20)
            V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("M_R_MAP1"),
            PU_CACHE));
        else if(rmap < 30)
            V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("M_R_MAP2"),
            PU_CACHE));
        else if(rmap < 40)
            V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("M_R_MAP3"),
            PU_CACHE));

        if(fsize != 19321722 && fsize != 28422764)
        {
            if(rmap == 1 || rmap == 11 || rmap == 21 || rmap == 31)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM1"),
                PU_CACHE));
            else if(rmap == 2 || rmap == 12 || rmap == 22 || rmap == 32)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM2"),
                PU_CACHE));
            else if(rmap == 3 || rmap == 13 || rmap == 23)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM3"),
                PU_CACHE));
            else if(rmap == 4 || rmap == 14 || rmap == 24)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM4"),
                PU_CACHE));
            else if(rmap == 5 || rmap == 15 || rmap == 25)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM5"),
                PU_CACHE));
            else if(rmap == 6 || rmap == 16 || rmap == 26)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM6"),
                PU_CACHE));
            else if(rmap == 7 || rmap == 17 || rmap == 27)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM7"),
                PU_CACHE));
            else if(rmap == 8 || rmap == 18 || rmap == 28)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM8"),
                PU_CACHE));
            else if(rmap == 9 || rmap == 19 || rmap == 29)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM9"),
                PU_CACHE));
            else if(rmap == 10 || rmap == 20 || rmap == 30)
                V_DrawPatch (115, 68, W_CacheLumpName(DEH_String("M_R_NUM0"),
                PU_CACHE));
        }
        else
        {
            if(fsize == 28422764)
                offset = 7;
            if(rmap == 1 || rmap == 11 || rmap == 21 || rmap == 31)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM1"), PU_CACHE));
            else if(rmap == 2 || rmap == 12 || rmap == 22 || rmap == 32)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM2"), PU_CACHE));
            else if(rmap == 3 || rmap == 13 || rmap == 23)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM3"), PU_CACHE));
            else if(rmap == 4 || rmap == 14 || rmap == 24)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM4"), PU_CACHE));
            else if(rmap == 5 || rmap == 15 || rmap == 25)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM5"), PU_CACHE));
            else if(rmap == 6 || rmap == 16 || rmap == 26)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM6"), PU_CACHE));
            else if(rmap == 7 || rmap == 17 || rmap == 27)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM7"), PU_CACHE));
            else if(rmap == 8 || rmap == 18 || rmap == 28)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM8"), PU_CACHE));
            else if(rmap == 9 || rmap == 19 || rmap == 29)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM9"), PU_CACHE));
            else if(rmap == 10 || rmap == 20 || rmap == 30)
                V_DrawPatch (115 - offset, 68, W_CacheLumpName
                (DEH_String("WINUM0"), PU_CACHE));
        }
    }

    if(fsize == 12361532)
    {
        if(repi == 1)
        {
            if(rmap == 1)
                V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("WILV00"),
                PU_CACHE));
            else if(rmap == 2)
                V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("WILV01"),
                PU_CACHE));
            else if(rmap == 3)
                V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("WILV02"),
                PU_CACHE));
            else if(rmap == 4)
                V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("WILV03"),
                PU_CACHE));
            else if(rmap == 5)
                V_DrawPatch (55, 68, W_CacheLumpName(DEH_String("WILV04"),
                PU_CACHE));
        }
    }

    if(rskill == 0)
        V_DrawPatch (55, 115, W_CacheLumpName(DEH_String("M_JKILL"), PU_CACHE));
    else if(rskill == 1)
        V_DrawPatch (55, 115, W_CacheLumpName(DEH_String("M_ROUGH"), PU_CACHE));
    else if(rskill == 2)
        V_DrawPatch (55, 115, W_CacheLumpName(DEH_String("M_HURT"), PU_CACHE));
    else if(rskill == 3)
        V_DrawPatch (55, 115, W_CacheLumpName(DEH_String("M_ULTRA"), PU_CACHE));

    // HACK: NOT FOR SHARE 1.0 & 1.1 && REG 1.1
    if(fsize != 4207819 && fsize != 4274218 && fsize != 10396254)
    {
        if(rskill == 4)
            V_DrawPatch (55, 115, W_CacheLumpName(DEH_String("M_NMARE"), PU_CACHE));
    }
}

void M_Options(int choice)
{
    M_SetupNextMenu(&OptionsDef);
}



//
//      Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
    // warning: unused parameter `int choice'
    choice = 0;
    showMessages = 1 - showMessages;
        
    if (!showMessages)
        players[consoleplayer].message = DEH_String(MSGOFF);
    else
        players[consoleplayer].message = DEH_String(MSGON);

    message_dontfuckwithme = true;
}


//
// M_EndGame
//
void M_EndGameResponse(int ch)
{
    if (ch != key_menu_forward)
        return;

    currentMenu->lastOn = itemOn;
    M_ClearMenus ();
    D_StartTitle ();
}

void M_EndGame(int choice)
{
    V_DrawPatchDirect(58, 15, W_CacheLumpName(DEH_String("M_T_EGME"),
                                               PU_CACHE));

    choice = 0;
    if (!usergame)
    {
        S_StartSound(NULL,sfx_oof);
        return;
    }

    if (netgame)
    {
        M_StartMessage(DEH_String(NETEND),NULL,false);
        return;
    }
    M_StartMessage(DEH_String(ENDGAME),M_EndGameResponse,true);
}




//
// M_ReadThis
//
void M_ReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
    // Doom 1.9 had two menus when playing Doom 1
    // All others had only one

    if (gameversion == exe_doom_1_9 && gamemode != commercial)
    {
        choice = 0;
        M_SetupNextMenu(&ReadDef2);
    }
    else
    {
        // Close the menu

        M_FinishReadThis(0);
    }
}

void M_FinishReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&MainDef);
}


//
// M_QuitDOOM
//
int     quitsounds[8] =
{
    sfx_pldeth,
    sfx_dmpain,
    sfx_popain,
    sfx_slop,
    sfx_telept,
    sfx_posit1,
    sfx_posit3,
    sfx_sgtatk
};

int     quitsounds2[8] =
{
    sfx_vilact,
    sfx_boscub,
    sfx_slop,
    sfx_skeswg,
    sfx_kntdth,
    sfx_bspact,
    sfx_sgtatk,
    sfx_getpow
};

int     quitsounds3[7] =
{
    sfx_vilact,
    sfx_boscub,
    sfx_slop,
    sfx_skeswg,
    sfx_kntdth,
    sfx_bspact,
    sfx_sgtatk
};

void M_QuitResponse(int ch)
{
    if (ch != key_menu_forward)
        return;

    if (!netgame)
    {
        if (gamemode == commercial)
        {
            if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                    fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                    fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                    fsize != 4225460)
                S_StartSound(NULL,quitsounds2[(gametic>>2)&7]);
            else
                S_StartSound(NULL,quitsounds3[(gametic>>2)&7]);
        }
        else
            S_StartSound(NULL,quitsounds[(gametic>>2)&7]);
        I_WaitVBL(105);
    }
    I_Quit ();
}


static char *M_SelectEndMessage(void)
{
    char **endmsg;

    if (logical_gamemission == doom)
    {
        // Doom 1

        endmsg = doom1_endmsg;
    }
    else
    {
        // Doom 2
        
        endmsg = doom2_endmsg;
    }

    return endmsg[gametic % NUM_QUITMESSAGES];
}


void M_QuitDOOM(int choice)
{
    sprintf(endstring,
            DEH_String("%s\n\n" DOSY),
            DEH_String(M_SelectEndMessage()));

    M_StartMessage(endstring,M_QuitResponse,true);
}




void M_WalkingSpeed(int choice)
{
    switch(choice)
    {
      case 0:
        if(forwardmove > 19)
            forwardmove--;
        break;
      case 1:
        if(forwardmove < 47)
            forwardmove++;
        break;
    }
}

void M_TurningSpeed(int choice)
{
    switch(choice)
    {
      case 0:
        if(turnspeed > 5)
            turnspeed--;
        break;
      case 1:
        if(turnspeed < 10)
            turnspeed++;
        break;
    }
}

void M_StrafingSpeed(int choice)
{
    switch(choice)
    {
      case 0:
        if(sidemove > 16)
            sidemove--;
        break;
      case 1:
        if (sidemove < 32)
            sidemove++;
        break;
    }
}


void M_ChangeDetail(int choice)
{
    choice = 0;
    detailLevel = 1 - detailLevel;

    R_SetViewSize (screenblocks, detailLevel);

    if (!detailLevel)
        players[consoleplayer].message = DEH_String(DETAILHI);
    else
        players[consoleplayer].message = DEH_String(DETAILLO);
}




void M_SizeDisplay(int choice)
{
    switch(choice)
    {
      case 0:
        if (screenSize > 0)
        {
            screenblocks--;
            screenSize--;
        }
        break;
      case 1:
        if (screenSize < 8)
        {
            screenblocks++;
            screenSize++;
        }
        break;
    }
    R_SetViewSize (screenblocks, detailLevel);
}




//
//      Menu Functions
//
void
M_DrawThermo
( int        x,
  int        y,
  int        thermWidth,
  int        thermDot )
{
    int                xx;
    int                i;

    xx = x;
    V_DrawPatchDirect(xx, y, W_CacheLumpName(DEH_String("M_THERML"), PU_CACHE));
    xx += 8;
    for (i=0;i<thermWidth;i++)
    {
        V_DrawPatchDirect(xx, y, W_CacheLumpName(DEH_String("M_THERMM"), PU_CACHE));
        xx += 8;
    }
    V_DrawPatchDirect(xx, y, W_CacheLumpName(DEH_String("M_THERMR"), PU_CACHE));

    V_DrawPatchDirect((x + 8) + thermDot * 8, y,
                      W_CacheLumpName(DEH_String("M_THERMO"), PU_CACHE));
}



void
M_DrawEmptyCell
( menu_t*        menu,
  int            item )
{
    V_DrawPatchDirect(menu->x - 10, menu->y + item * LINEHEIGHT - 1,
                      W_CacheLumpName(DEH_String("M_CELL1"), PU_CACHE));
}

void
M_DrawSelCell
( menu_t*        menu,
  int            item )
{
    V_DrawPatchDirect(menu->x - 10, menu->y + item * LINEHEIGHT - 1,
                      W_CacheLumpName(DEH_String("M_CELL2"), PU_CACHE));
}


void
M_StartMessage
( char*          string,
  void*          routine,
  boolean        input )
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageString = string;
    messageRoutine = routine;
    messageNeedsInput = input;
    menuactive = true;
    return;
}


void M_StopMessage(void)
{
    menuactive = messageLastMenuActive;
    messageToPrint = 0;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

//
// Find string width from hu_font chars
//
int M_StringWidth(char* string)
{
    size_t          i;
    int             w = 0;
    int             c;
        
    for (i = 0;i < strlen(string);i++)
    {
        c = toupper(string[i]) - HU_FONTSTART;
        if (c < 0 || c >= HU_FONTSIZE)
            w += 4;
        else
            w += SHORT (hu_font[c]->width);
    }
                
    return w;
}



//
//      Find string height from hu_font chars
//
int M_StringHeight(char* string)
{
    size_t          i;
    int             h;
    int             height = SHORT(hu_font[0]->height);
        
    h = height;
    for (i = 0;i < strlen(string);i++)
        if (string[i] == '\n')
            h += height;
                
    return h;
}


//
//      Write a string using the hu_font
//
void
M_WriteText
( int                x,
  int                y,
  char*                string)
{
    int                w;
    char*              ch;
    int                c;
    int                cx;
    int                cy;
                

    ch = string;
    cx = x;
    cy = y;
        
    while(1)
    {
        c = *ch++;
        if (!c)
            break;
        if (c == '\n')
        {
            cx = x;
            cy += 12;
            continue;
        }
                
        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c>= HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }
                
        w = SHORT (hu_font[c]->width);
        if (cx+w > ORIGWIDTH)                // CHANGED FOR HIRES
            break;
        V_DrawPatchDirect(cx, cy, hu_font[c]);
        cx+=w;
    }
}

//
// CONTROL PANEL
//

//
// M_Responder
//
boolean M_Responder (event_t* ev)
{
    int             ch;
    int             key;
    int             i;

    ch = -1; // will be changed to a legit char if we're going to use it here

    // Process joystick input
    // For some reason, polling ev.data for joystick input here in the menu code
    // doesn't work when using the twilight hack to launch wiidoom. At the same
    // time, it works fine if you're using the homebrew channel. I don't know
    // why this is so for the meantime I'm polling the wii remote directly.

    WPADData *data = WPAD_Data(0);

    //Classic Controls
    if(data->exp.type == WPAD_EXP_CLASSIC)
    {
        if (data->btns_d & WPAD_CLASSIC_BUTTON_UP)
        {
            ch = key_menu_up;                                // phares 3/7/98
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_DOWN)
        {
            ch = key_menu_down;                              // phares 3/7/98
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_LEFT)
        {
            ch = key_menu_left;                              // phares 3/7/98
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_RIGHT)
        {
            ch = key_menu_right;                             // phares 3/7/98
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_B)
        {
            ch = key_menu_forward;                           // phares 3/7/98
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_A)
        {
            ch = key_menu_back;                              // phares 3/7/98
        }

        if (data->exp.classic.ljs.pos.y > (data->exp.classic.ljs.center.y + 50))
        {
            ch = key_menu_up;
        }
        else if (data->exp.classic.ljs.pos.y < (data->exp.classic.ljs.center.y - 50))
        {
            ch = key_menu_down;
        }

        if (data->exp.classic.ljs.pos.x > (data->exp.classic.ljs.center.x + 50))
        {
            ch = key_menu_right;
        }
        else if (data->exp.classic.ljs.pos.x < (data->exp.classic.ljs.center.x - 50))
        {
            ch = key_menu_left;
        }
    }

    if (askforkey && data->btns_d)                // KEY BINDINGS
    {
        M_KeyBindingsClearControls(ev->data1);
        *doom_defaults_list[keyaskedfor + 30 + FirstKey].location = ev->data1;
        askforkey = false;
        return true;
    }

    if (askforkey && ev->type == ev_mouse)
    {
        if (ev->data1 & 1)
            return true;
        if (ev->data1 & 2)
            return true;
        if (ev->data1 & 4)
            return true;
        return false;
    }

    key = -1;

    // Save Game string input
    if (saveStringEnter)
    {
        switch(ch)
        {
          case KEY_ESCAPE:
            saveStringEnter = 0;
            strcpy(&savegamestrings[saveSlot][0],saveOldString);
            break;
                                
          case KEY_ENTER:
            saveStringEnter = 0;
            M_DoSave(saveSlot);
            break;

          default:
            // This is complicated.
            // Vanilla has a bug where the shift key is ignored when entering
            // a savegame name. If vanilla_keyboard_mapping is on, we want
            // to emulate this bug by using 'data1'. But if it's turned off,
            // it implies the user doesn't care about Vanilla emulation: just
            // use the correct 'data2'.

            ch = key;

            ch = toupper(ch);

            if (ch != ' '
             && (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE))
            {
                break;
            }

            if (ch >= 32 && ch <= 127 &&
                saveCharIndex < SAVESTRINGSIZE-1 &&
                M_StringWidth(savegamestrings[saveSlot]) <
                (SAVESTRINGSIZE-2)*8)
            {
                savegamestrings[saveSlot][saveCharIndex++] = ch;
                savegamestrings[saveSlot][saveCharIndex] = 0;
            }
            break;
        }
        return true;
    }

    // Take care of any messages that need input
    if (messageToPrint)
    {
        if (messageNeedsInput)
        {
            if (!(ch == key_menu_confirm || ch == key_menu_forward ||
                  ch == key_menu_activate))
            {
                return false;
            }
        }

        menuactive = messageLastMenuActive;
        messageToPrint = 0;
        if (messageRoutine)
            messageRoutine(ch);

        menuactive = false;
        S_StartSound(NULL,sfx_swtchx);
        return true;
    }

    // Pop-up menu?
    if (!menuactive)
    {
        if (ch == key_menu_activate)
        {
            M_StartControlPanel ();
            S_StartSound(NULL,sfx_swtchn);
            return true;
        }
        return false;
    }

    // Keys usable within menu

    if (ch == key_menu_down)
    {
        // Move down to next item

        do
        {
            if (itemOn+1 > currentMenu->numitems-1)
            {
                if (FirstKey == FIRSTKEY_MAX)        // FOR PSP (if too many menu items)
                {
                    itemOn = 0;
                    FirstKey = 0;
                }
                else
                {
                    FirstKey++;
                }
            }
            else itemOn++;

            if(!devparm && currentMenu == &KeyBindingsDef && itemOn == 11)
                itemOn++;
            S_StartSound(NULL,sfx_pstop);
        } while(currentMenu->menuitems[itemOn].status==-1);

        return true;
    }
    else if (ch == key_menu_up)
    {
        // Move back up to previous item

        do
        {
            if (!itemOn)
            {
                if (FirstKey == 0)                // FOR PSP (if too many menu items)
                {
                    itemOn = currentMenu->numitems-1;
                    FirstKey = FIRSTKEY_MAX;
                }
                else
                {
                    FirstKey--;
                }
            }
            else itemOn--;

            if(!devparm && currentMenu == &KeyBindingsDef && itemOn == 11)
                itemOn--;
            S_StartSound(NULL,sfx_pstop);
        } while(currentMenu->menuitems[itemOn].status==-1);

        return true;
    }
    else if (ch == key_menu_left)
    {
        // Slide slider left

        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL,sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(0);
        }
        return true;
    }
    else if (ch == key_menu_right)
    {
        // Slide slider right

        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL,sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(1);
        }
        return true;
    }
    else if (ch == key_menu_forward)
    {
        // Activate menu item

        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status)
        {
            currentMenu->lastOn = itemOn;
            if (currentMenu->menuitems[itemOn].status == 2)
            {
                currentMenu->menuitems[itemOn].routine(1);      // right arrow
                S_StartSound(NULL,sfx_stnmov);
            }
            else
            {
                currentMenu->menuitems[itemOn].routine(itemOn);
                S_StartSound(NULL,sfx_pistol);
            }
        }
        return true;
    }
    else if (ch == key_menu_activate)
    {
        // Deactivate menu

        currentMenu->lastOn = itemOn;
        M_ClearMenus ();
        S_StartSound(NULL,sfx_swtchx);
        return true;
    }
    else if (ch == key_menu_back)
    {
        // Go back to previous menu

        currentMenu->lastOn = itemOn;
        if (currentMenu->prevMenu)
        {
            currentMenu = currentMenu->prevMenu;
            itemOn = currentMenu->lastOn;
            S_StartSound(NULL,sfx_swtchn);
        }
        return true;
    }

    // Keyboard shortcut?
    // Vanilla Doom has a weird behavior where it jumps to the scroll bars
    // when the certain keys are pressed, so emulate this.

    else if (ch != 0)
    {
        for (i = itemOn+1;i < currentMenu->numitems;i++)
        {
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(NULL,sfx_pstop);
                return true;
            }
        }

        for (i = 0;i <= itemOn;i++)
        {
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(NULL,sfx_pstop);
                return true;
            }
        }
    }

    return false;
}



//
// M_StartControlPanel
//
void M_StartControlPanel (void)
{
    // intro might call this repeatedly
    if (menuactive)
        return;
    
    menuactive = 1;
    currentMenu = &MainDef;         // JDC
    itemOn = currentMenu->lastOn;   // JDC
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer (void)
{
    static short        x;
    static short        y;
    unsigned int        i;
    unsigned int        max;
    char                string[80];
    char                *name;
    int                 start;

    inhelpscreens = false;
    
    // DISPLAYS BLINKING "BETA" MESSAGE
    if ((fsize == 4261144 || fsize == 4271324 || fsize == 4211660) &&
            !menuactive && leveltime&16 && gamestate == GS_LEVEL)
        M_WriteText(140, 12, "BETA");

    if(display_fps == 1)
    {
        M_FPSCounter(1);
    }
    else if(display_fps == 0)
    {
        M_FPSCounter(0);
    }

    if(coordinates_info)
    {
            if(gamestate == GS_LEVEL)
            {
            static player_t* player;

            player = &players[consoleplayer];

            sprintf(map_coordinates_textbuffer, "ang=0x%x;x,y=(0x%x,0x%x)",
                    player->mo->angle,
                    player->mo->x,
                    player->mo->y);

            M_WriteText(0, 24, map_coordinates_textbuffer);
            }
    }

    // DISPLAYS THE GAME TIME
    if(timer_info)
        DrawWorldTimer();

    // DISPLAYS BINARY VERSION
    if(version_info)
        M_WriteText(65, 36, DOOM_VERSIONTEXT);

    // Horiz. & Vertically center string and print it.
    if (messageToPrint)
    {
        start = 0;
        y = 100 - M_StringHeight(messageString) / 2;
        while (messageString[start] != '\0')
        {
            int foundnewline = 0;

            for (i = 0; i < strlen(messageString + start); i++)
                if (messageString[start + i] == '\n')
                {
                    memset(string, 0, sizeof(string));
                    M_StringCopy(string, messageString + start, i + 1);
                    foundnewline = 1;
                    start += i + 1;
                    break;
                }
                                
            if (!foundnewline)
            {
                strcpy(string, messageString + start);
                start += strlen(string);
            }

            x = 160 - M_StringWidth(string) / 2;
            M_WriteText(x, y, string);
            y += SHORT(hu_font[0]->height);
        }

        return;
    }

    if (!menuactive)
        return;

    if (currentMenu->routine)
        currentMenu->routine();         // call Draw routine
    
    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;
    max = currentMenu->numitems;
/*
    if(currentMenu == &GameDef && devparm)
        currentMenu->numitems = 9;
*/
    if(currentMenu == &SoundDef && itemOn == 4)
        M_WriteText(48, 155, "You must restart to take effect.");

    // FOR PSP (if too many menu items)
    if (currentMenu->menuitems[itemOn].status == 5)
        max += FirstKey;

    if(extra_wad_loaded == 1 && currentMenu == &SoundDef)
        currentMenu->numitems = 4;

    if(!devparm && currentMenu == &OptionsDef)
        currentMenu->numitems = 5;

    if(!netgame && !devparm && currentMenu == &FilesDef)
        currentMenu->numitems = 4;

    if(netgame && currentMenu == &FilesDef)
        currentMenu->numitems = 3;

    // HACK: ALL ELSE THAN SHAREWARE 1.0 & 1.1 && REG 1.1
    if((fsize == 4207819 || fsize == 4274218 || fsize == 10396254) &&
            currentMenu == &NewDef)
        currentMenu->numitems = 4;

    if((fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
        fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
        fsize == 4225460 || fsize == 4234124 || fsize == 4196020) &&
            currentMenu == &WeaponsDef)
        currentMenu->numitems = 6;

    if(fsize == 12361532 && currentMenu == &ItemsDef)
        currentMenu->numitems = 7;

    if((fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                fsize == 11159840 || fsize == 12408292 || fsize == 12474561 ||
                fsize == 12538385 || fsize == 12487824) &&
                currentMenu == &WeaponsDef)
        currentMenu->numitems = 8;

    for (i=0;i<max;i++)
    {
        name = DEH_String(currentMenu->menuitems[i].name);

        if (name[0])
        {
            V_DrawPatchDirect (x, y, W_CacheLumpName(name, PU_CACHE));
        }

        if (currentMenu == &CheatsDef || currentMenu == &KeyBindingsDef ||
            currentMenu == &ItemsDef || currentMenu == &WeaponsDef ||
            currentMenu == &ArmorDef || currentMenu == &KeysDef || 
            currentMenu == &GameDef || currentMenu == &GameDefDev ||
            currentMenu == &GameDef2)
        {
            y += LINEHEIGHT_SMALL;
            // DRAW SKULL
            V_DrawPatch(x + CURSORXOFF_SMALL, currentMenu->y - 5 +
                        itemOn*LINEHEIGHT_SMALL,
                        W_CacheLumpName(DEH_String(skullNameSmall[whichSkull]),
                                              PU_CACHE));
        }
        else if(currentMenu)
        {
            y += LINEHEIGHT;
            // DRAW SKULL
            V_DrawPatchDirect(x + SKULLXOFF, currentMenu->y - 5 +
                              itemOn*LINEHEIGHT,
                              W_CacheLumpName(DEH_String(skullName[whichSkull]),
                                              PU_CACHE));
        }
    }
}


//
// M_ClearMenus
//
void M_ClearMenus (void)
{
    menuactive = 0;

    paused = false;

    // if (!netgame && usergame && paused)
    //       sendpause = true;
}




//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
}


//
// M_Ticker
//
void M_Ticker (void)
{
    if (--skullAnimCounter <= 0)
    {
        whichSkull ^= 1;
        skullAnimCounter = 8;
    }
}


//
// M_Init
//
void M_Init (void)
{
    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    whichSkull = 0;
    skullAnimCounter = 10;
    screenSize = screenblocks - 3;
    messageToPrint = 0;
    messageString = NULL;
    messageLastMenuActive = menuactive;

    // Here we could catch other version dependencies,
    //  like HELP1/2, and four episodes.

  
    switch ( gamemode )
    {
      case commercial:
        // Commercial has no "read this" entry.
        MainGameMenu[readthis] = MainGameMenu[quitdoom];
        MainDef.numitems--;
        MainDef.y += 8;
        NewDef.prevMenu = &MainDef;
        break;
      case shareware:
        // Episode 2 and 3 are handled,
        //  branching to an ad screen.
      case registered:
        // We need to remove the fourth episode.
        EpiDef.numitems--;
        break;
      case retail:
        // We are fine.
      default:
        break;
    }
}

void M_God(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].cheats ^= CF_GODMODE;
        if (players[consoleplayer].cheats & CF_GODMODE)
        {
            if (players[consoleplayer].mo)
                players[consoleplayer].mo->health = 100;
            players[consoleplayer].health = 100;
            players[consoleplayer].message = DEH_String(STSTR_DQDON);
            }
            else
            {
            players[consoleplayer].message = DEH_String(STSTR_DQDOFF);
            }
    }
    DetectState();
}

void M_Noclip(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].cheats ^= CF_NOCLIP;
            if (players[consoleplayer].cheats & CF_NOCLIP)
            {
            players[consoleplayer].message = DEH_String(STSTR_NCON);
            players[consoleplayer].mo->flags |= MF_NOCLIP;
            }
            else
            {
            players[consoleplayer].message = DEH_String(STSTR_NCOFF);
            players[consoleplayer].mo->flags &= ~MF_NOCLIP;
            }
    }
    DetectState();
}

void M_ArmorA(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].armorpoints = 200;
        players[consoleplayer].armortype = 2;
        players[consoleplayer].message = DEH_String("ALL ARMOR ADDED");
    }
    DetectState();
}

void M_ArmorB(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].armorpoints = 100;
        players[consoleplayer].armortype = 1;

        if(fsize != 12361532 && fsize != 19321722)
            players[consoleplayer].message = DEH_String("GREEN ARMOR ADDED");
        if(fsize == 12361532)
            players[consoleplayer].message = DEH_String("CHEX(R) ARMOR ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("KEVLAR VEST ADDED");
    }
    DetectState();
}

void M_ArmorC(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].armorpoints = 200;
        players[consoleplayer].armortype = 2;

        if(fsize != 12361532 && fsize != 19321722)
            players[consoleplayer].message = DEH_String("BLUE ARMOR ADDED");
        if(fsize == 12361532)
            players[consoleplayer].message = DEH_String("SUPER CHEX(R) ARMOR ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("SUPER KEVLAR VEST ADDED");
    }
    DetectState();
}

void M_WeaponsA(int choice)
{
    int i;

    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if(fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
        {
            for (i=0;i<NUMWEAPONS-4;i++)
                players[consoleplayer].weaponowned[i] = true;
                players[consoleplayer].weaponowned[7] = true;
        }
        else if(fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                fsize == 11159840 || fsize == 12408292 || gameversion == exe_chex)
        {
            for (i=0;i<NUMWEAPONS-1;i++)
                players[consoleplayer].weaponowned[i] = true;
        }
        else
        {
            for (i=0;i<NUMWEAPONS;i++)
                players[consoleplayer].weaponowned[i] = true;
        }
        for (i=0;i<NUMAMMO;i++)
          players[consoleplayer].ammo[i] = players[consoleplayer].maxammo[i];

        players[consoleplayer].message = DEH_String(STSTR_FAADDED);
    }
    DetectState();
}

void M_WeaponsB(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
          players[consoleplayer].weaponowned[2] = true;
          players[consoleplayer].ammo[1] = players[consoleplayer].maxammo[1];

        if(fsize != 19321722 && fsize != 12361532)
            players[consoleplayer].message = DEH_String("SHOTGUN ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("TAZER ADDED");
        if(fsize == 12361532)
            players[consoleplayer].message = DEH_String("LARGE ZORCHER ADDED");
    }
    DetectState();
}

void M_WeaponsC(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
          players[consoleplayer].weaponowned[3] = true;
          players[consoleplayer].ammo[1] = players[consoleplayer].maxammo[1];

        if(fsize != 19321722 && fsize != 12361532)
            players[consoleplayer].message = DEH_String("CHAINGUN ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("UZI ADDED");
        if(fsize == 12361532)
            players[consoleplayer].message = DEH_String("RAPID ZORCHER ADDED");
    }
    DetectState();
}

void M_WeaponsD(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
          players[consoleplayer].weaponowned[4] = true;
          players[consoleplayer].ammo[3] = players[consoleplayer].maxammo[3];

        if(fsize != 19321722 && fsize != 12361532)
            players[consoleplayer].message = DEH_String("ROCKET LAUNCHER ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("PHOTON 'ZOOKA ADDED");
        if(fsize == 12361532)
            players[consoleplayer].message = DEH_String("ZORCH PROPULSOR ADDED");
    }
    DetectState();
}

void M_WeaponsE(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
          players[consoleplayer].weaponowned[5] = true;
          players[consoleplayer].ammo[2] = players[consoleplayer].maxammo[2];

        if(fsize != 19321722 && fsize != 12361532)
            players[consoleplayer].message = DEH_String("PLASMA RIFLE ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("STICK ADDED");
        if(fsize == 12361532)
            players[consoleplayer].message = DEH_String("PHASING ZORCHER ADDED");
    }
    DetectState();
}

void M_WeaponsF(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
          players[consoleplayer].weaponowned[6] = true;
          players[consoleplayer].ammo[2] = players[consoleplayer].maxammo[2];

        if(fsize != 19321722 && fsize != 12361532)
            players[consoleplayer].message = DEH_String("BFG9000 ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("NUKER ADDED");
        if(fsize == 12361532)
            players[consoleplayer].message =
            DEH_String("LARGE AREA ZORCHING DEVICE ADDED");
    }
    DetectState();
}

void M_WeaponsG(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
          players[consoleplayer].weaponowned[7] = true;

        if(fsize != 19321722 && fsize != 12361532)
            players[consoleplayer].message = DEH_String("CHAINSAW ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("HOIG REZNATOR ADDED");
        if(fsize == 12361532)
            players[consoleplayer].message = DEH_String("SUPER BOOTSPORK ADDED");
    }
    DetectState();
}

void M_WeaponsH(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
          players[consoleplayer].weaponowned[8] = true;
          players[consoleplayer].ammo[1] = players[consoleplayer].maxammo[1];

        if(fsize != 19321722)
            players[consoleplayer].message = DEH_String("SUPER SHOTGUN ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("CRYOGUN ADDED");
    }
    DetectState();
}

void M_KeysA(int choice)
{
    int i;

    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        for (i=0;i<NUMCARDS;i++)
          players[consoleplayer].cards[i] = true;
        
        players[consoleplayer].message = DEH_String("ALL KEYS ADDED");
    }
    DetectState();
}

void M_KeysB(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].cards[0] = true;
        
        players[consoleplayer].message = DEH_String("BLUE KEYCARD ADDED");
    }
    DetectState();
}

void M_KeysC(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].cards[1] = true;
        
        players[consoleplayer].message = DEH_String("YELLOW KEYCARD ADDED");
    }
    DetectState();
}

void M_KeysD(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].cards[2] = true;
        
        players[consoleplayer].message = DEH_String("RED KEYCARD ADDED");
    }
    DetectState();
}

void M_KeysE(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].cards[3] = true;
        
        players[consoleplayer].message = DEH_String("BLUE SKULLKEY ADDED");
    }
    DetectState();
}

void M_KeysF(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].cards[4] = true;
        
        players[consoleplayer].message = DEH_String("YELLOW SKULLKEY ADDED");
    }
    DetectState();
}

void M_KeysG(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        players[consoleplayer].cards[5] = true;
        
        players[consoleplayer].message = DEH_String("RED SKULLKEY ADDED");
    }
    DetectState();
}

void M_ItemsA(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
            int i;

            static player_t* player;

            player = &players[consoleplayer];

        if(!got_all)
        {
            players[consoleplayer].powers[0] = INVULNTICS;
            players[consoleplayer].powers[1] = 1;
            players[consoleplayer].powers[2] = INVISTICS;
            players[consoleplayer].mo->flags |= MF_SHADOW;
            players[consoleplayer].powers[3] = IRONTICS;
            players[consoleplayer].powers[4] = 1;
            players[consoleplayer].powers[5] = INFRATICS;

            if (!player->backpack)
            {
                for (i=0 ; i<NUMAMMO ; i++)
                    player->maxammo[i] *= 2;
                player->backpack = true;
            }
            for (i=0 ; i<NUMAMMO ; i++)
                P_GiveAmmo (player, i, 1);
            player->message = DEH_String(GOTBACKPACK);

            got_all = true;
        }
        else
        {
            players[consoleplayer].powers[0] = 0;
            players[consoleplayer].powers[1] = 0;
            players[consoleplayer].powers[2] = 0;
            players[consoleplayer].powers[3] = 0;
            players[consoleplayer].powers[4] = 0;
            players[consoleplayer].powers[5] = 0;

            got_all = false;
        }
        players[consoleplayer].message = DEH_String("ALL ITEMS ADDED");
    }
    DetectState();
}

void M_ItemsB(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if(!got_invisibility)
        {
            players[consoleplayer].powers[2] = INVISTICS;
            players[consoleplayer].mo->flags |= MF_SHADOW;

            got_invisibility = true;
        }
        else
        {
            players[consoleplayer].powers[2] = 0;

            got_invisibility = false;
        }

        if(fsize != 19321722)
            players[consoleplayer].message = DEH_String(GOTINVIS);
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("ENK BLINDNESS ADDED");
    }
    DetectState();
}

void M_ItemsC(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if(!got_radiation_suit)
        {
            players[consoleplayer].powers[3] = IRONTICS;

            got_radiation_suit = true;
        }
        else
        {
            players[consoleplayer].powers[3] = 0;

            got_radiation_suit = false;
        }

        if(fsize != 12361532 && fsize != 19321722)
            players[consoleplayer].message = DEH_String(GOTSUIT);
        if(fsize == 12361532)
            players[consoleplayer].message = DEH_String("SLIME-PROOF SUIT ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("VULCAN RUBBER BOOTS ADDED");
    }
    DetectState();
}

void M_ItemsD(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if(!got_map)
        {
            players[consoleplayer].powers[4] = 1;

            got_map = true;
        }
        else
        {
            players[consoleplayer].powers[4] = 0;

            got_map = false;
        }

        if(fsize != 19321722)
            players[consoleplayer].message = DEH_String(GOTMAP);
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("SI ARRAY MAPPING ADDED");
    }
    DetectState();
}

void M_ItemsE(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if(!got_light_amp)
        {
            players[consoleplayer].powers[5] = INFRATICS;

            got_light_amp = true;
        }
        else
        {
            players[consoleplayer].powers[5] = 0;

            got_light_amp = false;
        }

        if(fsize != 12361532 && fsize != 19321722)
            players[consoleplayer].message = DEH_String(GOTVISOR);
        if(fsize == 12361532)
            players[consoleplayer].message = DEH_String("ULTRA GOOGLES ADDED");
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("INFRARED VISOR ADDED");
    }
    DetectState();
}

void M_ItemsF(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if(!got_invulnerability)
        {
            players[consoleplayer].powers[0] = INVULNTICS;

            got_invulnerability = true;
        }
        else
        {
            players[consoleplayer].powers[0] = 0;

            got_invulnerability = false;
        }

        if(fsize != 19321722)
            players[consoleplayer].message = DEH_String(GOTINVUL);
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("FORCE FIELD ADDED");
    }
    DetectState();
}

void M_ItemsG(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if(!got_berserk)
        {
            players[consoleplayer].powers[1] = 1;

            got_berserk = true;
        }
        else
        {
            players[consoleplayer].powers[1] = 0;

            got_berserk = false;
        }

        if(fsize != 19321722)
            players[consoleplayer].message = DEH_String(GOTBERSERK);
        if(fsize == 19321722)
            players[consoleplayer].message = DEH_String("007 MICROTEL ADDED");
    }
    DetectState();
}

void M_ItemsH(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if (players[consoleplayer].mo)
            players[consoleplayer].mo->health = 100;
        players[consoleplayer].health = 100;
        players[consoleplayer].message = DEH_String("GOT FULL HEALTH (100)");
    }
    DetectState();
}

void M_ItemsI(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if (players[consoleplayer].mo)
            players[consoleplayer].mo->health = 200;
        players[consoleplayer].health = 200;
        players[consoleplayer].message = DEH_String("GOT FULL HEALTH (200)");
    }
    DetectState();
}

void M_ItemsJ(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        int i;

        static player_t* player;

        player = &players[consoleplayer];

        if (!player->backpack)
        {
            for (i=0 ; i<NUMAMMO ; i++)
                player->maxammo[i] *= 2;
            player->backpack = true;
        }
        for (i=0 ; i<NUMAMMO ; i++)
            P_GiveAmmo (player, i, 1);
        player->message = DEH_String(GOTBACKPACK);
    }
    DetectState();
}

void M_Topo(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
            cheating = (cheating+1) % 3;
            cheeting = (cheeting+1) % 3;
    }
    DetectState();
}

boolean map_flag = false;

void M_Rift(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        switch(choice)
        {
        case 0:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020 ||
                fsize == 12361532)
            {
                if(epi >= 1 && map >= 1)
                    map--;
            }
            else if(fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                    fsize == 11159840)
            {
                if(epi >= 1 && map >= 1)
                {
                    map--;
                    if(epi == 3 && map == 0)
                    {
                        epi = 2;
                        map = 9;
                    }
                    else if(epi == 2 && map == 0)
                    {
                        epi = 1;
                        map = 9;
                    }
                }
            }
            else if(fsize == 12408292 || fsize == 12474561 || fsize == 12487824)
            {
                if(epi >= 1 && map >= 1)
                {
                    map--;
                    if(epi == 4 && map == 0)
                    {
                        epi = 3;
                        map = 9;
                    }
                    else if(epi == 3 && map == 0)
                    {
                        epi = 2;
                        map = 9;
                    }
                    else if(epi == 2 && map == 0)
                    {
                        epi = 1;
                        map = 9;
                    }
                }
            }
            else if(fsize == 12538385)
            {
                if(epi >= 1 && map >= 1)
                {
                    map--;
                    if(epi == 4 && map == 0)
                    {
                        epi = 3;
                        map = 9;
                    }
                    else if(epi == 3 && map == 0)
                    {
                        epi = 2;
                        map = 9;
                    }
                    else if(epi == 2 && map == 0)
                    {
                        epi = 1;
                        map = 10;
                        map_flag = true;
                    }
                }
            }
            else if(fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                    fsize == 14607420 || fsize == 14604584 || fsize == 18195736 ||
                    fsize == 18654796 || fsize == 18240172 || fsize == 17420824 ||
                    fsize == 28422764 || fsize == 14677988 || fsize == 14683458 ||
                    fsize == 14691821)
            {
                if(map >= 2)
                    map--;
            }
            else if(fsize == 19321722)
            {
                if(map >= 2)
                    map--;
                if(map == 30)
                    map = 20;
            }
            break;
            case 1:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if(epi <= 1 && map <= 9)
                {
                    map++;
                    if(epi == 1 && map == 10)
                    {
                        epi = 1;
                        map = 9;
                    }
                }
            }
            else if(fsize == 12361532)
            {
                if(epi <= 1 && map <= 5)
                {
                    map++;
                    if(epi == 1 && map == 6)
                    {
                        epi = 1;
                        map = 5;
                    }
                }
            }
            else if(fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                    fsize == 11159840)
            {
                if(epi <= 3 && map <= 9)
                {
                    map++;
                    if(epi == 1 && map == 10)
                    {
                        epi = 2;
                        map = 1;
                    }
                    else if(epi == 2 && map == 10)
                    {
                        epi = 3;
                        map = 1;
                    }
                    else if(epi == 3 && map == 10)
                    {
                        epi = 3;
                        map = 9;
                    }
                }
            }
            else if(fsize == 12408292 || fsize == 12474561 || fsize == 12487824)
            {
                if(epi <= 4 && map <= 9)
                {
                    map++;
                    if(epi == 1 && map == 10)
                    {
                        epi = 2;
                        map = 1;
                    }
                    else if(epi == 2 && map == 10)
                    {
                        epi = 3;
                        map = 1;
                    }
                    else if(epi == 3 && map == 10)
                    {
                        epi = 4;
                        map = 1;
                    }
                    else if(epi == 4 && map == 10)
                    {
                        epi = 4;
                        map = 9;
                    }
                }
            }
            else if(fsize == 12538385)
            {
                if(epi <= 4 && map <= 10)
                {
                    map++;
                    if(epi == 1 && map == 11)
                    {
                        epi = 2;
                        map = 1;
                    }
                    else if(epi == 2 && map == 10)
                    {
                        epi = 3;
                        map = 1;
                    }
                    else if(epi == 3 && map == 10)
                    {
                        epi = 4;
                        map = 1;
                    }
                    else if(epi == 4 && map == 10)
                    {
                        epi = 4;
                        map = 9;
                    }
                }
            }
            else if(fsize == 19321722)
            {
                if(map <= 30)
                    map++;
                if(map == 21)
                    map = 31;
            }
            if(!nerve_pwad)
            {
                if (fsize == 14943400 || fsize == 14612688 || fsize == 14607420 ||
                    fsize == 14604584 || fsize == 18195736 || fsize == 18654796 ||
                    fsize == 18240172 || fsize == 17420824 || fsize == 28422764)
                {
                    if(map <= 31)
                        map++;
                }
                else if(fsize == 14677988 || fsize == 14683458 || fsize == 14691821)
                {
                    if(map <= 32)
                        map++;
                }
                else if(fsize == 14824716)
                {
                    if(map <= 29)
                        map++;
                }
            }
            else
            {
                if(map < 9)
                    map++;
            }
            break;
        }
    }
    DetectState();
}

void M_RiftNow(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        if(forced)
            forced = false;

        warped = 1;
        menuactive = 0;
        G_DeferedInitNew(gameskill, epi, map);
        players[consoleplayer].message = DEH_String(STSTR_CLEV);
    }
    DetectState();
}

void M_Spin(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        switch(choice)
        {
        case 0:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if(tracknum > 1)
                {
                    tracknum--;
                    if(tracknum == 30)
                        tracknum = 29;
                    else if(tracknum == 27)
                        tracknum = 9;
                }
            }
            else if(fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                    fsize == 11159840 || fsize == 12408292 || fsize == 12361532 ||
                    fsize == 12474561 || fsize == 12538385 || fsize == 12487824)
            {
                if(tracknum > 1)
                {
                    tracknum--;
                    if(fsize == 12361532)
                    {
                        if(tracknum == 30)
                            tracknum = 29;
                        else if(tracknum == 27)
                            tracknum = 5;
                    }
                    else
                    {
                        if(tracknum == 28)
                            tracknum = 26;
                        else if(tracknum == 25)
                            tracknum = 21;
                        else if(tracknum == 19)
                            tracknum = 18;
                        else if(tracknum == 14)
                            tracknum = 13;
                    }
                }
            }
            else if(fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                    fsize == 14607420 || fsize == 14604584 || fsize == 19321722 ||
                    fsize == 28422764 || fsize == 14677988 || fsize == 14683458 ||
                    fsize == 14691821)
            {
                if(fsize == 19321722)
                {
                    if(tracknum > 33)
                    {
                        tracknum--;
                        if(tracknum == 64)
                            tracknum = 63;
                        else if(tracknum == 62)
                            tracknum = 53;
                        else if(tracknum == 51)
                            tracknum = 50;
                        else if(tracknum == 46)
                            tracknum = 45;
                    }
                }
                else if(fsize == 28422764)
                {
                    if(tracknum > 33)
                    {
                        tracknum--;
                        if(tracknum == 65)
                            tracknum = 64;
                        else if(tracknum == 60)
                            tracknum = 56;
                        else if(tracknum == 55)
                            tracknum = 51;
                        else if(tracknum == 47)
                            tracknum = 42;
                        else if(tracknum == 41)
                            tracknum = 40;
                        else if(tracknum == 39)
                            tracknum = 38;
                        else if(tracknum == 37)
                            tracknum = 36;
                    }
                }
                else
                {
                    if(tracknum > 33)
                    {
                        tracknum--;
                        if(tracknum == 61)
                            tracknum = 60;
                        else if(tracknum == 59)
                            tracknum = 57;
                        else if(tracknum == 56)
                            tracknum = 55;
                        else if(tracknum == 54)
                            tracknum = 52;
                        else if(tracknum == 51)
                            tracknum = 50;
                        else if(tracknum == 49)
                            tracknum = 42;
                    }
                }
            }
            else if(fsize == 18195736 || fsize == 18654796)
            {
                if(tracknum > 33)
                {
                    tracknum--;
                    if(tracknum == 65)
                        tracknum = 63;
                    else if(tracknum == 62)
                        tracknum = 57;
                    else if(tracknum == 50)
                        tracknum = 48;
                    else if(tracknum == 47)
                        tracknum = 46;
                    else if(tracknum == 45)
                        tracknum = 44;
                    else if(tracknum == 41)
                        tracknum = 40;
                }
            }
            else if(fsize == 18240172 || fsize == 17420824)
            {
                if(tracknum > 33)
                {
                    tracknum--;
                    if(tracknum == 65)
                        tracknum = 62;
                    else if(tracknum == 61)
                        tracknum = 57;
                    else if(tracknum == 50)
                        tracknum = 49;
                }
            }
            break;
            case 1:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if(tracknum < 31)
                {
                    tracknum++;
                    if(tracknum == 10)
                        tracknum = 28;
                    else if(tracknum == 30)
                            tracknum = 31;
                }
            }
            else if(fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                    fsize == 11159840 || fsize == 12408292 || fsize == 12361532 ||
                    fsize == 12474561 || fsize == 12538385 || fsize == 12487824)
            {
                if(tracknum < 31)
                {
                    tracknum++;
                    if(fsize == 12361532)
                    {
                        if(tracknum == 6)
                            tracknum = 28;
                        else if(tracknum == 30)
                            tracknum = 31;
                    }
                    else
                    {
                        if(tracknum == 14)
                            tracknum = 15;
                        else if(tracknum == 19)
                            tracknum = 20;
                        else if(tracknum == 22)
                            tracknum = 26;
                        else if(tracknum == 27)
                            tracknum = 29;
                    }
                }
            }
            else if(fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                    fsize == 14607420 || fsize == 14604584 || fsize == 19321722 ||
                    fsize == 28422764 || fsize == 14677988 || fsize == 14683458 ||
                    fsize == 14691821)
            {
                if(fsize == 19321722)
                {
                    if(tracknum < 67)
                    {
                        tracknum++;
                        if(tracknum == 46)
                            tracknum = 47;
                        else if(tracknum == 51)
                            tracknum = 52;
                        else if(tracknum == 54)
                            tracknum = 63;
                        else if(tracknum == 64)
                            tracknum = 65;
                    }
                }
                else if(fsize == 28422764)
                {
                    if(tracknum < 67)
                    {
                        tracknum++;
                        if(tracknum == 37)
                            tracknum = 38;
                        else if(tracknum == 39)
                            tracknum = 40;
                        else if(tracknum == 41)
                            tracknum = 42;
                        else if(tracknum == 43)
                            tracknum = 48;
                        else if(tracknum == 52)
                            tracknum = 56;
                        else if(tracknum == 57)
                            tracknum = 61;
                        else if(tracknum == 65)
                            tracknum = 66;
                    }
                }
                else
                {
                    if(tracknum < 67)
                    {
                        tracknum++;
                        if(tracknum == 43)
                            tracknum = 50;
                        else if(tracknum == 51)
                            tracknum = 52;
                        else if(tracknum == 53)
                            tracknum = 55;
                        else if(tracknum == 56)
                            tracknum = 57;
                        else if(tracknum == 58)
                            tracknum = 60;
                        else if(tracknum == 61)
                            tracknum = 62;
                    }
                }
            }
            else if(fsize == 18195736 || fsize == 18654796)
            {
                if(tracknum < 66)
                {
                    tracknum++;
                    if(tracknum == 41)
                        tracknum = 42;
                    else if(tracknum == 45)
                        tracknum = 46;
                    else if(tracknum == 47)
                        tracknum = 48;
                    else if(tracknum == 49)
                        tracknum = 51;
                    else if(tracknum == 58)
                        tracknum = 63;
                    else if(tracknum == 64)
                        tracknum = 66;
                }
            }
            else if(fsize == 18240172 || fsize == 17420824)
            {
                if(tracknum < 67)
                {
                    tracknum++;
                    if(tracknum == 50)
                        tracknum = 51;
                    else if(tracknum == 58)
                        tracknum = 62;
                    else if(tracknum == 63)
                        tracknum = 66;
                }
            }
            break;
        }
        players[consoleplayer].message = DEH_String(STSTR_MUS);

        if(mus_engine == 2)
            S_ChangeMusic(tracknum, false);
        else if(mus_engine == 1)
            S_ChangeMusic(tracknum, true);

        mus_cheat_used = true;
    }
    forced = false;
}

void M_KeyBindingsSetKey(int choice)
{
    askforkey = true;
    keyaskedfor = choice;

    if (!netgame && !demoplayback)
    {
        paused = true;
    }
}

// XXX (FOR PSP): NOW THIS IS RATHER IMPORTANT: IF...
// ...THE CONFIG VARIABLES IN THIS SOURCE EVER GET...
// ...SOMEWHAT REARRANGED, THEN IT'S IMPORTANT TO...
// ...CHANGE THE START- & END-POS INTEGERS AS WELL...
// ...TO THEIR NEW CORRESPONDING POSITIONS OR ELSE...
// ...THE KEY BINDINGS MENU WILL BE VERY BUGGY!!!
void M_KeyBindingsClearControls (int ch)
{
    int i;

    for (i = key_controls_start_in_cfg_at_pos; i < key_controls_end_in_cfg_at_pos; i++)
    {
        if (*doom_defaults_list[i].location == ch)
            *doom_defaults_list[i].location = 0;
    }
}

void M_KeyBindingsClearAll (int choice)
{
    *doom_defaults_list[30].location = 0;
    *doom_defaults_list[31].location = 0;
    *doom_defaults_list[32].location = 0;
    *doom_defaults_list[33].location = 0;
    *doom_defaults_list[34].location = 0;
    *doom_defaults_list[35].location = 0;
    *doom_defaults_list[36].location = 0;
    *doom_defaults_list[37].location = 0;
    *doom_defaults_list[38].location = 0;
    *doom_defaults_list[39].location = 0;
    *doom_defaults_list[40].location = 0;
    *doom_defaults_list[41].location = 0;
}

void M_KeyBindingsReset (int choice)
{
    *doom_defaults_list[30].location = CLASSIC_CONTROLLER_R;
    *doom_defaults_list[31].location = CLASSIC_CONTROLLER_L;
    *doom_defaults_list[32].location = CLASSIC_CONTROLLER_MINUS;
    *doom_defaults_list[33].location = CLASSIC_CONTROLLER_LEFT;
    *doom_defaults_list[34].location = CLASSIC_CONTROLLER_DOWN;
    *doom_defaults_list[35].location = CLASSIC_CONTROLLER_RIGHT;
    *doom_defaults_list[36].location = CLASSIC_CONTROLLER_ZL;
    *doom_defaults_list[37].location = CLASSIC_CONTROLLER_ZR;
    *doom_defaults_list[38].location = CLASSIC_CONTROLLER_HOME;
    *doom_defaults_list[39].location = CONTROLLER_1;
    *doom_defaults_list[40].location = CONTROLLER_2;
    *doom_defaults_list[41].location = CLASSIC_CONTROLLER_PLUS;
}

void M_DrawKeyBindings(void)
{
    int i;

    if(fsize != 19321722)
        V_DrawPatch (80, 5, W_CacheLumpName(DEH_String("M_T_BNDS"), PU_CACHE));
    else
        V_DrawPatch (80, 5, W_CacheLumpName(DEH_String("M_KBNDGS"), PU_CACHE));

    M_WriteText(40, 30, DEH_String("FIRE"));
    M_WriteText(40, 40, DEH_String("USE / OPEN"));
    M_WriteText(40, 50, DEH_String("MAIN MENU"));
    M_WriteText(40, 60, DEH_String("WEAPON LEFT"));
    M_WriteText(40, 70, DEH_String("SHOW AUTOMAP"));
    M_WriteText(40, 80, DEH_String("WEAPON RIGHT"));
    M_WriteText(40, 90, DEH_String("AUTOMAP ZOOM IN"));
    M_WriteText(40, 100, DEH_String("AUTOMAP ZOOM OUT"));
    M_WriteText(40, 110, DEH_String("JUMP"));
    M_WriteText(40, 120, DEH_String("RUN"));
    M_WriteText(40, 130, DEH_String("CONSOLE"));

    if(devparm)
        M_WriteText(40, 140, DEH_String("AIMING HELP"));

    M_WriteText(40, 150, DEH_String("CLEAR ALL CONTROLS"));
    M_WriteText(40, 160, DEH_String("RESET TO DEFAULTS"));

    for (i = 0; i < 12; i++)
    {
        if(i < 11 || (i == 11 && devparm))
        {
            if (askforkey && keyaskedfor == i)
                M_WriteText(195, (i*10+30), "???");
            else
                M_WriteText(195, (i*10+30),
                Key2String(*(doom_defaults_list[i+FirstKey+30].location)));
        }
    }
}

void M_KeyBindings(int choice)
{
    M_SetupNextMenu(&KeyBindingsDef);
}

void M_Freelook(int choice)
{
    switch(choice)
    {
    case 0:
        if (mouselook)
            mouselook--;
        break;
    case 1:
        if (mouselook < 2)
            mouselook++;
        break;
    }
}

void M_FreelookSpeed(int choice)
{
    switch(choice)
    {
    case 0:
        if (mspeed)
            mspeed--;
        break;
    case 1:
        if (mspeed < 10)
            mspeed++;
        break;
    }
}

void M_Controls(int choice)
{
    M_SetupNextMenu(&ControlsDef);
}

void M_DrawControls(void)
{
    if(mouselook == 0)
    V_DrawPatch(190, 101,
                      W_CacheLumpName(DEH_String("M_MSGOFF"), PU_CACHE));
    else if(mouselook == 1)
    V_DrawPatch(190, 104,
                      W_CacheLumpName(DEH_String("M_NORMAL"), PU_CACHE));
    else if(mouselook == 2)
    V_DrawPatch(190, 104,
                      W_CacheLumpName(DEH_String("M_INVRSE"), PU_CACHE));

    M_DrawThermo(OptionsDef.x-10,OptionsDef.y-50+LINEHEIGHT*(mousesens+1),
                 29,forwardmove-19);

    M_DrawThermo(OptionsDef.x-10,OptionsDef.y-50+LINEHEIGHT*(turnsens+1),
                 6,turnspeed-5);

    M_DrawThermo(OptionsDef.x-10,OptionsDef.y-50+LINEHEIGHT*(strafesens+1),
                 17,sidemove-16);

    M_DrawThermo(OptionsDef.x-5,OptionsDef.y-50+LINEHEIGHT*(mousespeed+1),
                 11,mspeed);
}

void M_FPS(int choice)
{
    if(display_fps < 1)
    {
        display_fps++;
        fps_enabled = 1;
        players[consoleplayer].message = DEH_String("FPS COUNTER ON");
    }
    else if(display_fps)
    {
        display_fps--;
        fps_enabled = 0;
        players[consoleplayer].message = DEH_String("FPS COUNTER OFF");
    }
}

u64 GetTicks(void)
{
    return (u64)SDL_GetTicks();
}

void M_FPSCounter(int display_fps)
{
    int tickfreq = 1000;

    static int fpsframecount = 0;
    static u64 fpsticks;

    fpsframecount++;

    if(GetTicks() >= fpsticks + tickfreq)
    {
        fps = fpsframecount;
        fpsframecount = 0;
        fpsticks = GetTicks();
    }
    sprintf( fpsDisplay, "FPS: %d", fps );

    if(display_fps)
    {
        M_WriteText(0, 30, fpsDisplay);
    }
    BorderNeedRefresh = true;
}

void M_DisplayTicker(int choice)
{
    display_ticker = !display_ticker;
    if (display_ticker)
    {
        dots_enabled = 1;
        players[consoleplayer].message = DEH_String("TICKER ON");
    }
    else
    {
        dots_enabled = 0;
        players[consoleplayer].message = DEH_String("TICKER OFF");
    }
    I_DisplayFPSDots(display_ticker);

    if(usergame)
        ST_doRefresh();
}

void M_Coordinates(int choice)
{
    switch(choice)
    {
    case 0:
        if (coordinates_info)
            coordinates_info--;
        break;
    case 1:
        if (coordinates_info < 1)
            coordinates_info++;
        break;
    }
}

void M_Timer(int choice)
{
    switch(choice)
    {
    case 0:
        if (timer_info)
            timer_info--;
        break;
    case 1:
        if (timer_info < 1)
            timer_info++;
        break;
    }
}

void M_Version(int choice)
{
    switch(choice)
    {
    case 0:
        if (version_info)
            version_info--;
        break;
    case 1:
        if (version_info < 1)
            version_info++;
        break;
    }
}

void M_System(int choice)
{
    M_SetupNextMenu(&SystemDef);
}

void M_GameFiles(int choice)
{
    M_SetupNextMenu(&FilesDef);
}

void M_DrawFilesMenu(void)
{
}

void M_Brightness(int choice)
{
    switch(choice)
    {
    case 0:
        if (usegamma)
            usegamma--;
        break;
    case 1:
        if (usegamma < 4)
            usegamma++;
        break;
    }
    players[consoleplayer].message = DEH_String(gammamsg[usegamma]);
    I_SetPalette (W_CacheLumpName (DEH_String("PLAYPAL"),PU_CACHE));
}

void M_Armor(int choice)
{
    M_SetupNextMenu(&ArmorDef);
}

void M_Weapons(int choice)
{
    M_SetupNextMenu(&WeaponsDef);
}

void M_Keys(int choice)
{
    M_SetupNextMenu(&KeysDef);
}

void M_Items(int choice)
{
    M_SetupNextMenu(&ItemsDef);
}

void M_Screen(int choice)
{
    M_SetupNextMenu(&ScreenDef);
}

void M_Cheats(int choice)
{
    M_SetupNextMenu(&CheatsDef);
}

void M_Record(int choice)
{
    M_SetupNextMenu(&RecordDef);
}

void M_Game(int choice)
{
    if(devparm)
        M_SetupNextMenu(&GameDefDev);
    else
        M_SetupNextMenu(&GameDef);
}

void M_Game2(int choice)
{
    M_SetupNextMenu(&GameDef2);
}

void M_RMap(int choice)
{
        switch(choice)
        {
        case 0:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if(repi >= 1 && rmap >= 1)
                    rmap--;
            }
            else if(fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                    fsize == 11159840)
            {
                if(repi >= 1 && rmap >= 1)
                {
                    rmap--;
                    if(repi == 3 && rmap == 0)
                    {
                        repi = 2;
                        rmap = 9;
                    }
                    else if(repi == 2 && rmap == 0)
                    {
                        repi = 1;
                        rmap = 9;
                    }
                }
            }
            else if(fsize == 12408292)
            {
                if(repi >= 1 && rmap >= 1)
                {
                    rmap--;
                    if(repi == 4 && rmap == 0)
                    {
                        repi = 3;
                        rmap = 9;
                    }
                    else if(repi == 3 && rmap == 0)
                    {
                        repi = 2;
                        rmap = 9;
                    }
                    else if(repi == 2 && rmap == 0)
                    {
                        repi = 1;
                        rmap = 9;
                    }
                }
            }
            else if(fsize == 12361532)
            {
                if(repi >= 1 && rmap >= 1)
                {
                    rmap--;
                }
            }
            else if(fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                    fsize == 14607420 || fsize == 14604584 || fsize == 18195736 ||
                    fsize == 18654796 || fsize == 18240172 || fsize == 17420824 ||
                    fsize == 28422764)
            {
                if(rmap >= 2)
                    rmap--;
            }
            else if(fsize == 19321722)
            {
                if(rmap >= 2)
                    rmap--;
                if(rmap == 30)
                    rmap = 20;
            }
            break;
            case 1:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if(repi <= 1 && rmap <= 9)
                {
                    rmap++;
                    if(repi == 1 && rmap == 10)
                    {
                        repi = 1;
                        rmap = 9;
                    }
                }
            }
            else if(fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                    fsize == 11159840)
            {
                if(repi <= 3 && rmap <= 9)
                {
                    rmap++;
                    if(repi == 1 && rmap == 10)
                    {
                        repi = 2;
                        rmap = 1;
                    }
                    else if(repi == 2 && rmap == 10)
                    {
                        repi = 3;
                        rmap = 1;
                    }
                    else if(repi == 3 && rmap == 10)
                    {
                        repi = 3;
                        rmap = 9;
                    }
                }
            }
            else if(fsize == 12408292)
            {
                if(repi <= 4 && rmap <= 9)
                {
                    rmap++;
                    if(repi == 1 && rmap == 10)
                    {
                        repi = 2;
                        rmap = 1;
                    }
                    else if(repi == 2 && rmap == 10)
                    {
                        repi = 3;
                        rmap = 1;
                    }
                    else if(repi == 3 && rmap == 10)
                    {
                        repi = 4;
                        rmap = 1;
                    }
                    else if(repi == 4 && rmap == 10)
                    {
                        repi = 4;
                        rmap = 9;
                    }
                }
            }
            else if(fsize == 12361532)
            {
                if(repi <= 1 && rmap <= 4)
                {
                    rmap++;
                }
            }
            else if(fsize == 14943400 || fsize == 14612688 || fsize == 14607420 ||
                    fsize == 14604584 || fsize == 18195736 || fsize == 18654796 ||
                    fsize == 18240172 || fsize == 17420824 || fsize == 28422764)
            {
                if(rmap <= 31)
                    rmap++;
            }
            else if(fsize == 14824716)
            {
                if(rmap <= 29)
                    rmap++;
            }
            else if(fsize == 19321722)
            {
                if(rmap <= 30)
                    rmap++;
                if(rmap == 21)
                    rmap = 31;
            }
            break;
        }
}

void M_RSkill(int choice)
{
    switch(choice)
    {
    case 0:
        if (rskill)
            rskill--;
        break;
    case 1:
        // HACK: NOT FOR SHARE 1.0 & 1.1 && REG 1.1
        if(fsize != 4207819 && fsize != 4274218 && fsize != 10396254)
        {
            if (rskill < 4)
                rskill++;
        }
        else
        {
            if (rskill < 3)
                rskill++;
        }
        break;
    }
}

void M_StartRecord(int choice)
{
    if(!demoplayback)
    {
        M_ClearMenus();
        G_RecordDemo(rskill, 1, repi, rmap);
        D_DoomLoop();               // never returns
    }
    else if(demoplayback)
    {
        players[consoleplayer].message = DEH_String("MAP ROTATION DISABLED");
    }
}

void M_Crosshair(int choice)
{
    switch(choice)
    {
    case 0:
        if (crosshair)
            crosshair--;
        break;
    case 1:
        if (crosshair < 1)
            crosshair++;
        break;
    }
}

void M_Jumping(int choice)
{
    switch(choice)
    {
    case 0:
        if (jumping)
            jumping = false;
        break;
    case 1:
        if (jumping == false)
            jumping = true;
        break;
    }
}

void M_WeaponRecoil(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_recoil)
            d_recoil = false;
        players[consoleplayer].message = DEH_String("WEAPON RECOIL OFF");
        break;
    case 1:
        if (d_recoil == false)
            d_recoil = true;
        players[consoleplayer].message = DEH_String("WEAPON RECOIL ON");
        break;
    }
}

void M_PlayerThrust(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_thrust)
            d_thrust = false;
        players[consoleplayer].message = DEH_String("PLAYER THRUST OFF");
        break;
    case 1:
        if (d_thrust == false)
            d_thrust = true;
        players[consoleplayer].message = DEH_String("PLAYER THRUST ON");
        break;
    }
}

void M_RespawnMonsters(int choice)
{
    switch(choice)
    {
    case 0:
        if (respawnparm)
        {
            start_respawnparm = false;
            respawnparm = false;
        }
        players[consoleplayer].message = DEH_String("RESPAWN MONSTERS DISABLED");
        break;
    case 1:
        if (respawnparm == false)
        {
            start_respawnparm = true;
            respawnparm = true;
        }
        players[consoleplayer].message = DEH_String("RESPAWN MONSTERS ENABLED");
        break;
    }
}

void M_FastMonsters(int choice)
{
    switch(choice)
    {
    case 0:
        if (fastparm)
        {
            start_fastparm = false;
            fastparm = false;
        }
        players[consoleplayer].message = DEH_String("FAST MONSTERS DISABLED");
        break;
    case 1:
        if (fastparm == false)
        {
            start_fastparm = true;
            fastparm = true;
        }
        players[consoleplayer].message = DEH_String("FAST MONSTERS ENABLED");
        break;
    }
}

void M_Autoaim(int choice)
{
    switch(choice)
    {
    case 0:
        if (autoaim)
            autoaim = 0;
        players[consoleplayer].message = DEH_String("AUTOAIM DISABLED");
        break;
    case 1:
        if (autoaim == 0)
            autoaim = 1;
        players[consoleplayer].message = DEH_String("AUTOAIM ENABLED");
        break;
    }
}

void M_MaxGore(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_maxgore)
            d_maxgore = false;
        players[consoleplayer].message = DEH_String("MORE GORE DISABLED");
        break;
    case 1:
        if (!d_maxgore)
            d_maxgore = true;
        players[consoleplayer].message = DEH_String("MORE GORE ENABLED");
        break;
    }
}

void M_Footstep(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_footstep)
            d_footstep = false;
        players[consoleplayer].message = DEH_String("PLAYER FOOTSTEPS DISABLED");
        break;
    case 1:
        if (!d_footstep)
            d_footstep = true;
        players[consoleplayer].message = DEH_String("PLAYER FOOTSTEPS ENABLED");
        break;
    }
}

void M_Footclip(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_footclip)
            d_footclip = false;
        players[consoleplayer].message = DEH_String("HERETIC FOOTCLIPS DISABLED");
        break;
    case 1:
        if (!d_footclip)
            d_footclip = true;
        players[consoleplayer].message = DEH_String("HERETIC FOOTCLIPS ENABLED");
        break;
    }
}

void M_Debug(int choice)
{
    M_SetupNextMenu(&DebugDef);
}

void M_DrawSystem(void)
{
    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch (62, 20, W_CacheLumpName(DEH_String("M_T_YSET"), PU_CACHE));
    else
        V_DrawPatch (62, 20, W_CacheLumpName(DEH_String("M_SYSSET"), PU_CACHE));

    if(display_fps)
        V_DrawPatch (244, 85, W_CacheLumpName(DEH_String("M_MSGON"), PU_CACHE));
    else
        V_DrawPatch (244, 85, W_CacheLumpName(DEH_String("M_MSGOFF"), PU_CACHE));

    if(display_ticker)
        V_DrawPatch (244, 101, W_CacheLumpName(DEH_String("M_MSGON"), PU_CACHE));
    else
        V_DrawPatch (244, 101, W_CacheLumpName(DEH_String("M_MSGOFF"), PU_CACHE));
}

void M_HUD(int choice)
{
    switch(choice)
    {
    case 0:
        if (hud)
            hud = false;
        players[consoleplayer].message = DEH_String("EXTRA HUD ENABLED");
        break;
    case 1:
        if (hud == false)
            hud = true;
        players[consoleplayer].message = DEH_String("EXTRA HUD ENABLED");
        break;
    }
}

void M_MapGrid(int choice)
{
    switch(choice)
    {
    case 0:
        if (drawgrid)
            drawgrid--;
        players[consoleplayer].message = DEH_String(AMSTR_GRIDOFF);
        break;
    case 1:
        if (drawgrid < 1)
            drawgrid++;
        players[consoleplayer].message = DEH_String(AMSTR_GRIDON);
        break;
    }
}

void M_MapRotation(int choice)
{
    switch(choice)
    {
    case 0:
        if (am_rotate)
            am_rotate = false;
        players[consoleplayer].message = DEH_String("MAP ROTATION DISABLED");
        break;
    case 1:
        if (am_rotate == false)
            am_rotate = true;
        players[consoleplayer].message = DEH_String("MAP ROTATION ENABLED");
        break;
    }
}

void M_WeaponChange(int choice)
{
    switch(choice)
    {
    case 0:
        if (use_vanilla_weapon_change == 1)
            use_vanilla_weapon_change = 0;
        players[consoleplayer].message =
        DEH_String("ORIGINAL WEAPON CHANGING STYLE DISABLED");
        break;
    case 1:
        if (use_vanilla_weapon_change == 0)
            use_vanilla_weapon_change = 1;
        players[consoleplayer].message =
        DEH_String("ORIGINAL WEAPON CHANGING STYLE ENABLED");
        break;
    }
}

void M_AimingHelp(int choice)
{
    switch(choice)
    {
    case 0:
        if (aiming_help)
            aiming_help = false;
        players[consoleplayer].message = DEH_String("AIMING HELP DISABLED");
        break;
    case 1:
        if (!aiming_help)
            aiming_help = true;
        players[consoleplayer].message = DEH_String("AIMING HELP ENABLED");
        break;
    }
}

void M_FollowMode(int choice)
{
    switch(choice)
    {
    case 0:
        if (followplayer)
            followplayer--;
        players[consoleplayer].message = DEH_String(AMSTR_FOLLOWOFF);
        break;
    case 1:
        if (followplayer < 1)
            followplayer++;
        players[consoleplayer].message = DEH_String(AMSTR_FOLLOWON);
        break;
    }
}

void M_Statistics(int choice)
{
    switch(choice)
    {
    case 0:
        if (show_stats)
            show_stats--;
        players[consoleplayer].message = DEH_String("LEVEL STATISTICS OFF");
        break;
    case 1:
        if (show_stats < 1)
            show_stats++;
        players[consoleplayer].message = DEH_String("LEVEL STATISTICS ON");
        break;
    }
}

void M_DrawDebug(void)
{
    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch (67, 15, W_CacheLumpName(DEH_String("M_T_DSET"), PU_CACHE));
    else
        V_DrawPatch (67, 15, W_CacheLumpName(DEH_String("M_DBGSET"), PU_CACHE));

    if(coordinates_info)
        V_DrawPatch (234, 75, W_CacheLumpName(DEH_String("M_MSGON"), PU_CACHE));
    else
        V_DrawPatch (234, 75, W_CacheLumpName(DEH_String("M_MSGOFF"), PU_CACHE));

    if(timer_info)
        V_DrawPatch (234, 92, W_CacheLumpName(DEH_String("M_MSGON"), PU_CACHE));
    else
        V_DrawPatch (234, 92, W_CacheLumpName(DEH_String("M_MSGOFF"), PU_CACHE));

    if(version_info)
        V_DrawPatch (234, 108, W_CacheLumpName(DEH_String("M_MSGON"), PU_CACHE));
    else
        V_DrawPatch (234, 108, W_CacheLumpName(DEH_String("M_MSGOFF"), PU_CACHE));
}

// jff 2/01/98 kill all monsters
//static void cheat_massacre()
void M_Massacre(int choice)
{
    // jff 02/01/98 'em' cheat - kill all monsters
    // partially taken from Chi's .46 port

    // killough 2/7/98: cleaned up code and changed to use dprintf;
    // fixed lost soul bug (Lost Souls left behind when Pain Elementals are killed)

    int killcount = 0;

    extern void A_PainDie(mobj_t *);

    thinker_t *currentthinker = &thinkercap;

    while ((currentthinker=currentthinker->next) != &thinkercap)
    {
        if (currentthinker->function.acp1 == (actionf_p1) P_MobjThinker &&
           (((mobj_t *) currentthinker)->flags & MF_COUNTKILL ||
            ((mobj_t *) currentthinker)->type == MT_SKULL))
        {
            // killough 3/6/98: kill even if Pain Elemental is dead

            if (((mobj_t *) currentthinker)->health > 0)
            {
                killcount++;

                P_DamageMobj((mobj_t *)currentthinker, NULL, NULL, 10000);
            }
            if (((mobj_t *) currentthinker)->type == MT_PAIN)
            {
                // killough 2/8/98

                A_PainDie((mobj_t *) currentthinker);

                P_SetMobjState ((mobj_t *) currentthinker, S_PAIN_DIE6);
            }
        }
    }
    // killough 3/22/98: make more intelligent about plural

    // Ty 03/27/98 - string(s) *not* externalized

    sprintf(massacre_textbuffer, "%d MONSTER%s KILLED\n",
        killcount, killcount == 1 ? "" : "S");

    players[consoleplayer].message = DEH_String(massacre_textbuffer);
}

