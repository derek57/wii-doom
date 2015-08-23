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
#include "i_tinttab.h"
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
#include "v_trans.h"
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
/*
char                       detailNames[2][9] = {"M_GDHIGH","M_GDLOW"};
char                       msgNames[2][9]    = {"M_MSGOFF","M_MSGON"};
*/
char                       map_coordinates_textbuffer[50];
char                       massacre_textbuffer[30];

// old save description before edit
char                       saveOldString[SAVESTRINGSIZE];  

// ...and here is the message string!
char                       *messageString;

// graphic name of skulls
// warning: initializer-string for array of chars is too long
//char                       *skullName[2]      = {"M_SKULL1","M_SKULL2"};
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
int                        key_controls_start_in_cfg_at_pos = 69;
int                        key_controls_end_in_cfg_at_pos = 83;
int                        crosshair = 0;
int                        show_stats = 0;
int                        tracknum = 1;
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
int                        chaingun_tics = 4;
int                        snd_module = 0;
int                        snd_chans = 1;
int                        sound_channels = 8;
int                        gore_amount = 4;
int                        height;

//
// MENU TYPEDEFS
//

short                      itemOn;                  // menu item skull is on
short                      skullAnimCounter;        // skull animation counter
short                      whichSkull;              // which skull to draw

// timed message = no input from user
boolean                    messageNeedsInput;
boolean                    map_flag = false;
boolean                    inhelpscreens;
boolean                    menuactive;
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
boolean                    noclip_on;
boolean                    dedicatedflag = true;
boolean                    privateserverflag;
boolean                    massacre_cheat_used;
boolean                    randompitch;
boolean                    memory_usage;
boolean                    blurred = false;

fixed_t                    forwardmove = 29;
fixed_t                    sidemove = 21; 

// current menudef
menu_t                     *currentMenu;                          

byte                       *tempscreen;
byte                       *blurredscreen;

static boolean             askforkey = false;

static int                 FirstKey = 0;           // SPECIAL MENU FUNCTIONS (ITEMCOUNT)
static int                 keyaskedfor;

extern int                 opl_type;
extern int                 cheating;
extern int                 mspeed;
extern int                 left;
extern int                 right;
extern int                 mouselook;
extern int                 dots_enabled;
extern int                 dont_show;
extern int                 display_fps;
extern int                 wipe_type;
extern int                 correct_lost_soul_bounce;
extern int                 png_screenshots;

extern default_t           doom_defaults_list[];   // KEY BINDINGS

extern patch_t*            hu_font[HU_FONTSIZE];

extern boolean             overlay_trigger;
extern boolean             message_dontfuckwithme;
extern boolean             chat_on;                // in heads-up code
extern boolean             BorderNeedRefresh;
extern boolean             sendpause;
extern boolean             secret_1;
extern boolean             secret_2;
extern boolean             done;

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
void M_Translucency(int choice);
void M_ColoredBloodA(int choice);
void M_ColoredBloodB(int choice);
void M_WipeType(int choice);
void M_UncappedFramerate(int choice);
void M_Screenshots(int choice);
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
void M_DrawThermoSmall(int x,int y,int thermWidth,int thermDot);
void M_DrawEmptyCell(menu_t *menu,int item);
void M_DrawSelCell(menu_t *menu,int item);
void M_WriteText(int x, int y, char *string);
void M_StartMessage(char *string,void *routine,boolean input);
void M_StopMessage(void);
void M_ClearMenus (void);

void M_MusicType(int choice);
void M_SoundType(int choice);
void M_SoundOutput(int choice);
void M_SoundPitch(int choice);
void M_RestartSong(int choice);
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
void M_Authors(int choice);
void M_Version(int choice);
void M_SoundInfo(int choice);
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
void M_Splash(int choice);
void M_Swirl(int choice);
void M_Beta(int choice);
void M_Corpses(int choice);
void M_Secrets(int choice);
void M_Trails(int choice);
void M_ChaingunTics(int choice);
void M_FallingDamage(int choice);
void M_InfiniteAmmo(int choice);
void M_GoreAmount(int choice);
void M_Telefrag(int choice);
void M_Doorstuck(int choice);
void M_ResurrectGhosts(int choice);
void M_LimitedGhosts(int choice);
void M_BlockSkulls(int choice);
void M_BlazingDoors(int choice);
void M_GodAbsolute(int choice);
void M_Floor(int choice);
void M_Clipping(int choice);
void M_Model(int choice);
void M_BossDeath(int choice);
void M_Bounce(int choice);
void M_Masked(int choice);
void M_Quirk(int choice);
void M_Ouch(int choice);
void M_NoMonsters(int choice);
void M_AutomapOverlay(int choice);

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
void M_ItemsK(int choice);
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
void M_FPSCounter(int choice);
void M_HOMDetector(int choice);
void M_MemoryUsage(int choice);
void M_ReplaceMissing(int choice);
void M_Controls(int choice);
void M_System(int choice);
void M_Sound(int choice);
void M_Game(int choice);
void M_Game2(int choice);
void M_Game3(int choice);
void M_Game4(int choice);
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
void M_DrawGame3(void);
void M_DrawGame4(void);
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
    {1,"New Game",M_NewGame,'n'},
    {1,"Options",M_Options,'o'},
    {1,"Game Files",M_GameFiles,'f'},
    // Another hickup with Special edition.
    {1,"Read This!",M_ReadThis,'r'},
    {1,"Quit Game",M_QuitDOOM,'q'}
};

menu_t  MainDef =
{
    main_end,
    NULL,
    MainGameMenu,
    M_DrawMainMenu,
    122,74,
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
    {1,"Load Game",M_LoadGame,'l'},
    {1,"Save Game",M_SaveGame,'s'},
    {1,"End Game",M_EndGame,'e'},
    {1,"Cheats",M_Cheats,'c'},
    {1,"Record Demo",M_Record,'r'}
};

menu_t  FilesDef =
{
    files_end,
    &MainDef,
    FilesMenu,
    M_DrawFilesMenu,
    122,65,
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
    {1,"Knee-Deep in the Dead", M_Episode,'k'},
    {1,"The Shores of Hell", M_Episode,'t'},
    {1,"Inferno", M_Episode,'i'},
    {1,"Thy Flesh Consumed", M_Episode,'t'}
};

menu_t  EpiDef =
{
    ep_end,                // # of menu items
    &MainDef,              // previous menu
    EpisodeMenu,           // menuitem_t ->
    M_DrawEpisode,         // drawing routine ->
    95,73,                 // x,y
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
    {1,"I'm too young to die.", M_ChooseSkill, 'i'},
    {1,"Hey, not too rough.", M_ChooseSkill, 'h'},
    {1,"",  M_ChooseSkill, 'h'},
    {1,"Ultra-Violence.", M_ChooseSkill, 'u'},
    {1,"Nightmare!",        M_ChooseSkill, 'n'}
};

menu_t  NewDef =
{
    newg_end,           // # of menu items
    &EpiDef,            // previous menu
    NewGameMenu,        // menuitem_t ->
    M_DrawNewGame,      // drawing routine ->
    88,73,              // x,y
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
    {1,"Screen Settings", M_Screen,'s'},
    {1,"Control Settings", M_Controls,'c'},
    {1,"Sound Settings", M_Sound,'v'},
    {1,"System Settings", M_System,'y'},
    {1,"Game Settings", M_Game,'g'},
    {1,"Debug Settings", M_Debug,'d'}
};

menu_t  OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    M_DrawOptions,
    100,67,
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
    {2,"GOD MODE",M_God,'g'},
    {2,"NOCLIP",M_Noclip,'n'},
    {2,"",M_Weapons,'w'}, 
    {2,"",M_Keys,'k'},
    {2,"",M_Armor,'a'},
    {1,"",M_Items,'i'},
    {2,"AUTOMAP REVEAL:",M_Topo,'t'},
    {2,"KILL ALL ENEMIES",M_Massacre,'m'},
    {2,"WARP TO MAP:",M_Rift,'r'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {2,"EXECUTE WARPING",M_RiftNow,'e'},
    {-1,"",0,'\0'},
    {2,"PLAY MUSIC TITLE:",M_Spin,'s'}
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
    itemsf,
    items_end
} items_e;

menuitem_t ItemsMenu[]=
{
    {2,"",M_ItemsA,'1'},
    {-1,"",0,'\0'},
    {2,"",M_ItemsC,'2'},
    {2,"",M_ItemsD,'3'},
    {2,"",M_ItemsE,'4'},
    {2,"FULL HEALTH (100)",M_ItemsH,'5'},
    {2,"FULL HEALTH (200)",M_ItemsI,'6'},
    {2,"",M_ItemsB,'7'},
    {2,"",M_ItemsF,'8'},
    {2,"",M_ItemsG,'9'},
    {2,"",M_ItemsJ,'0'},
    {2,"",M_ItemsK,'f'}
};

menu_t  ItemsDef =
{
    items_end,
    &CheatsDef,
    ItemsMenu,
    M_DrawItems,
    80,52,
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
    {2,"BLUE KEYCARD",M_KeysB,'2'},
    {2,"YELLOW KEYCARD",M_KeysC,'3'},
    {2,"RED KEYCARD",M_KeysD,'4'},
    {2,"BLUE SKULL KEY",M_KeysE,'5'},
    {2,"YELLOW SKULL KEY",M_KeysF,'6'},
    {2,"RED SKULL KEY",M_KeysG,'7'}
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
    scrnsize,
    screen_detail,
    screen_translucency,
    screen_wipe,
    screen_framerate,
    screen_shots,
    screen_end
} screen_e;

menuitem_t ScreenMenu[]=
{
    {2,"Brightness",M_Brightness,'g'},
    {2,"Screen Size",M_SizeDisplay,'s'},
    {2,"Detail",M_ChangeDetail,'d'},
    {2,"Translucency",M_Translucency,'t'},
    {2,"Wipe Type",M_WipeType,'w'},
    {2,"Uncapped Framerate",M_UncappedFramerate,'f'},
    {2,"Screenshot Format",M_Screenshots,'x'},
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
    turnsens,
    strafesens,
    mousespeed,
    controls_freelook,
    controls_keybindings,
    controls_end
} controls_e;

menuitem_t ControlsMenu[]=
{
    {2,"Walking",M_WalkingSpeed,'m'},
    {2,"Turning",M_TurningSpeed,'t'},
    {2,"Strafing",M_StrafingSpeed,'s'},
    {2,"Freelook",M_FreelookSpeed,'f'},
    {2,"Freelook Mode",M_Freelook,'l'},
    {1,"Key Bindings",M_KeyBindings,'b'}
};

menu_t  ControlsDef =
{
    controls_end,
    &OptionsDef,
    ControlsMenu,
    M_DrawControls,
    20,60,
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
    keybindings_flyup,
    keybindings_flydown,
    keybindings_jump,
    keybindings_run,
    keybindings_console,
    keybindings_screenshots,
    keybindings_clearall,
    keybindings_reset,
    keybindings_end
} keybindings_e;

menuitem_t KeyBindingsMenu[]=
{
    {5,"FIRE",M_KeyBindingsSetKey,0},
    {5,"USE / OPEN",M_KeyBindingsSetKey,1},
    {5,"MAIN MENU",M_KeyBindingsSetKey,2},
    {5,"WEAPON LEFT",M_KeyBindingsSetKey,3},
    {5,"SHOW AUTOMAP",M_KeyBindingsSetKey,4},
    {5,"WEAPON RIGHT",M_KeyBindingsSetKey,5},
    {5,"AUTOMAP ZOOM IN",M_KeyBindingsSetKey,6},
    {5,"AUTOMAP ZOOM OUT",M_KeyBindingsSetKey,7},
    {5,"FLY UP",M_KeyBindingsSetKey,8},
    {5,"FLY DOWN",M_KeyBindingsSetKey,9},
    {5,"JUMP",M_KeyBindingsSetKey,'j'},
    {5,"RUN",M_KeyBindingsSetKey,'r'},
    {5,"CONSOLE",M_KeyBindingsSetKey,'c'},
    {5,"SCREENSHOTS",M_KeyBindingsSetKey,'s'},
    {5,"",M_KeyBindingsClearAll,'x'},
    {5,"",M_KeyBindingsReset,'d'}
};

menu_t  KeyBindingsDef =
{
    keybindings_end,
    &ControlsDef,
    KeyBindingsMenu,
    M_DrawKeyBindings,
    45,22,
    0
};

enum
{
    system_fps,
    system_ticker,
    system_hom,
    system_replace,
    system_end
} system_e;

menuitem_t SystemMenu[]=
{
    {2,"FPS Counter",M_FPS,'f'},
    {2,"Display Ticker",M_DisplayTicker,'t'},
    {2,"Hall of Mirrors Detector",M_HOMDetector,'h'},
    {2,"Replace Missing Textures",M_ReplaceMissing,'r'}
};

menu_t  SystemDef =
{
    system_end,
    &OptionsDef,
    SystemMenu,
    M_DrawSystem,
    57,85,
    0
};

enum
{
    game_mapgrid,
    game_maprotation,
    game_followmode,
    game_statistics,
    game_overlay,
    game_timer,
    game_authors,
    game_messages,
    game_weapon,
    game_recoil,
    game_thrust,
    game_respawn,
    game_fast,
    game_aiming,
    game_game2,
    game_end
} game_e;

menuitem_t GameMenu[]=
{
    {2,"AUTOMAP GRID",M_MapGrid,'g'},
    {2,"AUTOMAP ROTATION",M_MapRotation,'r'},
    {2,"AUTOMAP FOLLOW MODE",M_FollowMode,'f'},
    {2,"AUTOMAP STATISTICS",M_Statistics,'s'},
    {2,"AUTOMAP OVERLAY",M_AutomapOverlay,'n'},
    {2,"AUTOMAP TIMER",M_Timer,'t'},
    {2,"AUTOMAP AUTHORS",M_Authors,'a'},
    {2,"MESSAGES",M_ChangeMessages,'m'},
    {2,"WEAPON CHANGE",M_WeaponChange,'w'},
    {2,"WEAPON RECOIL",M_WeaponRecoil,'c'},
    {2,"PLAYER THRUST",M_PlayerThrust,'p'},
    {2,"RESPAWN MONSTERS",M_RespawnMonsters,'i'},
    {2,"FAST MONSTERS",M_FastMonsters,'d'},
    {2,"",NULL,'0'},
    {2,"",M_Game2,'2'}
};

menu_t  GameDef =
{
    game_end,
    &OptionsDef,
    GameMenu,
    M_DrawGame,
    75,22,
    0
};

enum
{
    game2_monsters,
    game2_hud,
    game2_footstep,
    game2_footclip,
    game2_splash,
    game2_swirl,
    game2_prbeta,
    game2_corpses,
    game2_secrets,
    game2_trails,
    game2_tics,
    game2_falling,
    game2_ammo,
    game2_game3,
    game2_end
} game2_e;

menuitem_t GameMenu2[]=
{
    {2,"NO MONSTERS",M_NoMonsters,'m'},
    {2,"FULLSCREEN HUD",M_HUD,'h'},
    {2,"PLAYER FOOTSTEPS",M_Footstep,'s'},
    {2,"HERETIC FOOTCLIPS",M_Footclip,'c'},
    {2,"HERETIC LIQUID SPLASH",M_Splash,'l'},
    {2,"SWIRLING WATER HACK",M_Swirl,'w'},
    {2,"PRE-RELEASE BETA MODE",M_Beta,'b'},
    {2,"RANDOMLY FLIP CORPSES & GUNS",M_Corpses,'d'},
    {2,"SHOW REVEALED SECRETS",M_Secrets,'z'},
    {2,"ROCKET TRAILS",M_Trails,'r'},
    {2,"CHAINGUN SPEED",M_ChaingunTics,'g'},
    {2,"FALLING DAMAGE",M_FallingDamage,'f'},
    {2,"INFINITE AMMO",M_InfiniteAmmo,'i'},
    {2,"",M_Game3,'n'}
};

menu_t  GameDef2 =
{
    game2_end,
    &GameDef,
    GameMenu2,
    M_DrawGame2,
    40,22,
    0
};

enum
{
    game3_crosshair,
    game3_autoaim,
    game3_jumping,
    game3_gore,
    game3_amount,
    game3_blooda,
    game3_bloodb,
    game3_telefrag,
    game3_stuck,
    game3_ressurection,
    game3_limitation,
    game3_block,
    game3_doors,
    game3_game4,
    game3_end
} game3_e;

menuitem_t GameMenu3[]=
{
    {2,"CROSSHAIR",M_Crosshair,'x'},
    {2,"AUTOAIM",M_Autoaim,'a'},
    {2,"JUMPING",M_Jumping,'j'},
    {2,"MORE BLOOD & GORE",M_MaxGore,'o'},
    {2,"Gore Amount",M_GoreAmount,'g'},
    {2,"Enable Colored Blood",M_ColoredBloodA,'b'},
    {2,"Fix Monster Blood",M_ColoredBloodB,'2'},
    {2,"Monsters can Telefrag on MAP30",M_Telefrag,'t'},
    {2,"Monsters stuck on doortracks",M_Doorstuck,'s'},
    {2,"ARCH-VILE can resurrect ghosts",M_ResurrectGhosts,'r'},
    {2,"Pain Elementals have lost soul limit",M_LimitedGhosts,'l'},
    {2,"Lost Souls get stuck behind walls",M_BlockSkulls,'w'},
    {2,"Blazing Doors play double sound",M_BlazingDoors,'d'},
    {2,"",M_Game4,'n'}
};

menu_t  GameDef3 =
{
    game3_end,
    &GameDef2,
    GameMenu3,
    M_DrawGame3,
    23,22,
    0
};

enum
{
    game4_god,
    game4_floor,
    game4_clipping,
    game4_model,
    game4_bossdeath,
    game4_bounce,
    game4_masked,
    game4_sound,
    game4_ouch,
    game4_end
} game4_e;

menuitem_t GameMenu4[]=
{
    {2,"God mode isn't absolute",M_GodAbsolute,'i'},
    {2,"Use Doom's floor motion behavior",M_Floor,'f'},
    {2,"Use Doom's movement clipping code",M_Clipping,'c'},
    {2,"Use Doom's linedef trigger model",M_Model,'m'},
    {2,"Emulate pre-Ultimate Boss Death",M_BossDeath,'d'},
    {2,"Lost souls don't bounce off flats",M_Bounce,'b'},
    {2,"Two-S. middle textures don't animate",M_Masked,'a'},
    {2,"Retain quirks in Doom's sound code",M_Quirk,'s'},
    {2,"Use Doom's buggy ouch face code",M_Ouch,'o'}
};

menu_t  GameDef4 =
{
    game4_end,
    &GameDef3,
    GameMenu4,
    M_DrawGame4,
    23,22,
    0
};

enum
{
    debug_coordinates,
    debug_version,
    debug_sound,
    debug_mem,
    debug_restart,
    debug_end
} debug_e;

menuitem_t DebugMenu[]=
{
    {2,"Show Coordinates",M_Coordinates,'c'},
    {2,"Show Version",M_Version,'v'},
    {2,"Show Sound Info",M_SoundInfo,'s'},
    {2,"Show Memory Usage",M_MemoryUsage,'m'},
    {2,"Restart Current MAP-Music Track",M_RestartSong,'r'}
};

menu_t  DebugDef =
{
    debug_end,
    &OptionsDef,
    DebugMenu,
    M_DrawDebug,
    67,75,
    0
};

//
// SOUND VOLUME MENU
//
enum
{
    sfx_vol,
    music_vol,
    mus_type,
    sfx_type,
    channels,
    output,
    pitch,
    sound_end
} sound_e;

menuitem_t SoundMenu[]=
{
    {2,"Sound Volume",M_SfxVol,'v'},
    {2,"Music Volume",M_MusicVol,'m'},
    {2,"Music Type",M_MusicType,'t'},
    {2,"Sound Type",M_SoundType,'s'},
    {2,"Number of Sound Channels",M_SoundChannels,'c'},
    {2,"Switch Left / Right Output",M_SoundOutput,'o'},
    {2,"v1.1 Random Sound Pitch",M_SoundPitch,'p'}
};

menu_t  SoundDef =
{
    sound_end,
    &OptionsDef,
    SoundMenu,
    M_DrawSound,
    45,70,
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
    75,56,
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
    75,56,
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
    {2,"Record Map:",M_RMap,'m'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {2,"Choose Skill Level:",M_RSkill,'s'},
    {-1,"",0,'\0'},
    {-1,"",0,'\0'},
    {2,"Start Recording",M_StartRecord,'r'}
};

menu_t  RecordDef =
{
    record_end,
    &FilesDef,
    RecordMenu,
    M_DrawRecord,
    105,60,
    0
};

static void blurscreen(int x1, int y1, int x2, int y2, int i)
{
    int x, y;

    memcpy(tempscreen, blurredscreen, SCREENWIDTH * SCREENHEIGHT);

    for (y = y1; y < y2; y += SCREENWIDTH)
        for (x = y + x1; x < y + x2; ++x)
            blurredscreen[x] = tinttab50[tempscreen[x] + (tempscreen[x + i] << 8)];
}

//
// M_DarkBackground
//  darken and blur background while menu is displayed
//
void M_DarkBackground(void)
{
    int i;

    height = SCREENHEIGHT * SCREENWIDTH;

    if (!blurred)
    {
        for (i = 0; i < height; ++i)
            blurredscreen[i] = grays[screens[0][i]];

        blurscreen(0, 0, SCREENWIDTH - 1, height, 1);
        blurscreen(1, 0, SCREENWIDTH, height, -1);
        blurscreen(0, 0, SCREENWIDTH - 1, height - SCREENWIDTH, SCREENWIDTH + 1);
        blurscreen(1, SCREENWIDTH, SCREENWIDTH, height, -(SCREENWIDTH + 1));
        blurscreen(0, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH);
        blurscreen(0, SCREENWIDTH, SCREENWIDTH, height, -SCREENWIDTH);
        blurscreen(1, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH - 1);
        blurscreen(0, SCREENWIDTH, SCREENWIDTH - 1, height, -(SCREENWIDTH - 1));

        blurred = true;
    }

    for (i = 0; i < height; ++i)
        screens[0][i] = tinttab50[blurredscreen[i]];

    if (detailLevel)
        V_LowGraphicDetail(SCREENHEIGHT);
}

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
        M_StringCopy(name, P_SaveGameFile(i), sizeof(name));

        handle = fopen(name, "rb");
        if (handle == NULL)
        {
            M_StringCopy(&savegamestrings[i][0], EMPTYSTRING, sizeof(savegamestrings));
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

    M_DarkBackground();

    V_DrawPatch(72, 28, 0,
                      W_CacheLumpName(DEH_String("M_T_LGME"), PU_CACHE));

    for (i = 0;i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x+5,LoadDef.y+LINEHEIGHT_SMALL*i);

        if (!strncmp(savegamestrings[i], EMPTYSTRING, strlen(EMPTYSTRING)))
            dp_translation = crx[CRX_DARK];

        M_WriteText(LoadDef.x,LoadDef.y-1+LINEHEIGHT_SMALL*i,savegamestrings[i]);

        V_ClearDPTranslation();
    }

    dp_translation = crx[CRX_GOLD];
    M_WriteText(62, 148, "* INDICATES A SAVEGAME THAT WAS");
    M_WriteText(62, 158, "CREATED USING AN OPTIONAL PWAD!");
    V_ClearDPTranslation();
}



//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(int x,int y)
{
    int             i;
        
    V_DrawPatch(x - 8, y + 7, 0,
                      W_CacheLumpName(DEH_String("M_LSLEFT"), PU_CACHE));
        
    for (i = 0;i < 24;i++)
    {
        V_DrawPatch(x, y + 7, 0,
                          W_CacheLumpName(DEH_String("M_LSCNTR"), PU_CACHE));
        x += 8;
    }

    V_DrawPatch(x, y + 7, 0,
                      W_CacheLumpName(DEH_String("M_LSRGHT"), PU_CACHE));
}



//
// User wants to load this game
//
void M_LoadSelect(int choice)
{
    char    name[256];
        
    M_StringCopy(name, P_SaveGameFile(choice), sizeof(name));

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
        
    M_DarkBackground();

    V_DrawPatch(72, 28, 0, W_CacheLumpName(DEH_String("M_T_SGME"), PU_CACHE));
    for (i = 0;i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x+5,LoadDef.y+LINEHEIGHT_SMALL*i);
        M_WriteText(LoadDef.x,LoadDef.y-1+LINEHEIGHT_SMALL*i,savegamestrings[i]);
    }
        
    if (saveStringEnter)
    {
        i = M_StringWidth(savegamestrings[saveSlot]);

        dp_translation = crx[CRX_GRAY];
        M_WriteText(LoadDef.x + i,LoadDef.y-1+LINEHEIGHT_SMALL*saveSlot,"_");
        V_ClearDPTranslation();
    }

    dp_translation = crx[CRX_GOLD];
    M_WriteText(62, 148, "* INDICATES A SAVEGAME THAT WAS");
    M_WriteText(62, 158, "CREATED USING AN OPTIONAL PWAD!");
    V_ClearDPTranslation();
}

//
// M_Responder calls this when user is finished
//
void M_DoSave(int slot)
{
    G_SaveGame(slot, savegamestrings[slot], "");
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
    M_StringCopy(saveOldString,savegamestrings[choice], sizeof(saveOldString));
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
    
    V_DrawPatch (0, 0, 0, W_CacheLumpName(lumpname, PU_CACHE));

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

    V_DrawPatch(0, 0, 0, W_CacheLumpName(DEH_String("HELP1"), PU_CACHE));
}


//
// Change Sfx & Music volumes
//
void M_DrawSound(void)
{
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch (65, 15, 0, W_CacheLumpName(DEH_String("M_T_XSET"), PU_CACHE));
    else
        V_DrawPatch (65, 15, 0, W_CacheLumpName(DEH_String("M_SNDSET"), PU_CACHE));

    M_DrawThermoSmall(SoundDef.x + 95, SoundDef.y + LINEHEIGHT_SMALL * (sfx_vol + 1),
                 16, sfxVolume);

    M_DrawThermoSmall(SoundDef.x + 95, SoundDef.y + LINEHEIGHT_SMALL * (music_vol + 1),
                 16, musicVolume);

    if(mus_engine == 1)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 204, SoundDef.y + 18, "OPL2");
        V_ClearDPTranslation();
    }
    else if(mus_engine == 2)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 204, SoundDef.y + 18, "OPL3");
        V_ClearDPTranslation();
    }
    else if(mus_engine == 3)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 212, SoundDef.y + 18, "OGG");
        V_ClearDPTranslation();
    }
    else if(mus_engine == 4)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 183, SoundDef.y + 18, "TIMIDITY");
        V_ClearDPTranslation();
    }

    if(snd_module)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 159, SoundDef.y + 28, "PC-SPEAKER");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 213, SoundDef.y + 28, "SDL");
        V_ClearDPTranslation();
    }

    if(sound_channels == 8)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 228, SoundDef.y + 38, "8");
        V_ClearDPTranslation();
    }
    else if(sound_channels == 16)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(SoundDef.x + 223, SoundDef.y + 38, "16");
        V_ClearDPTranslation();
    }

    if(swap_sound_chans)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 220, SoundDef.y + 48, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SoundDef.x + 212, SoundDef.y + 48, "OFF");
        V_ClearDPTranslation();
    }

    if(randompitch)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 220, SoundDef.y + 58, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SoundDef.x + 212, SoundDef.y + 58, "OFF");
        V_ClearDPTranslation();
    }
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

void M_SoundType(int choice)
{
    switch(choice)
    {
      case 0:
        if (snd_module && fsize != 19321722)
            snd_module = 0;
        break;
      case 1:
        if (!snd_module && fsize != 19321722)
            snd_module = 1;
        break;
    }
}

void M_MusicType(int choice)
{
    switch(choice)
    {
    case 0:
        if(mus_engine > 1)
        {
            if(mus_engine == 4)
                snd_musicdevice = SNDDEVICE_GUS;
            else if(mus_engine == 3)
                snd_musicdevice = SNDDEVICE_GENMIDI;
            else if(mus_engine < 3)
            {
                if(mus_engine == 2)
                    opl_type = 1;
                else
                    opl_type = 0;

                snd_musicdevice = SNDDEVICE_SB;
            }
            mus_engine--;
        }
        break;
    case 1:
        if(mus_engine < 4)
        {
            if(mus_engine == 4)
                snd_musicdevice = SNDDEVICE_GUS;
            else if(mus_engine == 3)
                snd_musicdevice = SNDDEVICE_GENMIDI;
            else if(mus_engine < 3)
            {
                if(mus_engine == 2)
                    opl_type = 1;
                else
                    opl_type = 0;

                snd_musicdevice = SNDDEVICE_SB;
            }
            mus_engine++;
        }
        break;
    }
}

void M_SoundOutput(int choice)
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

void M_SoundPitch(int choice)
{
    switch(choice)
    {
    case 0:
        if(randompitch == true)
        {
            randompitch = false;
        }
        break;
    case 1:
        if(randompitch == false)
        {
            randompitch = true;
        }
        break;
    }
}

void M_RestartSong(int choice)
{
    S_StopMusic();
    S_ChangeMusic(gamemap, true);
}

void M_SoundChannels(int choice)
{
    switch(choice)
    {
    case 0:
        if(snd_chans > 1)
            snd_chans--;
        if(snd_chans == 1)
            sound_channels = 8;
        else if(snd_chans == 2)
            sound_channels = 16;
        break;
    case 1:
        if(snd_chans < 2)
            snd_chans++;
        if(snd_chans == 1)
            sound_channels = 8;
        else if(snd_chans == 2)
            sound_channels = 16;
        break;
    }
}

//
// M_DrawMainMenu
//
void M_DrawMainMenu(void)
{
    M_DarkBackground();

    V_DrawPatch(94, 2, 0,
                      W_CacheLumpName(DEH_String("M_DOOM"), PU_CACHE));
}




//
// M_NewGame
//
void M_DrawNewGame(void)
{
    M_DarkBackground();

    V_DrawPatch(96, 14, 0, W_CacheLumpName(DEH_String("M_NEWG"), PU_CACHE));
    M_WriteText(NewDef.x, NewDef.y - 22, "CHOOSE SKILL LEVEL:");
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
    M_DarkBackground();

    V_DrawPatch(75, 38, 0, W_CacheLumpName(DEH_String("M_EPISOD"), PU_CACHE));
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
      C_Printf(CR_RED, " M_Episode: 4th episode requires Ultimate DOOM\n");
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
    M_DarkBackground();

    V_DrawPatch(108, 15, 0, W_CacheLumpName(DEH_String("M_OPTTTL"),
                                               PU_CACHE));
}

void M_DrawItems(void)
{
    M_DarkBackground();

    V_DrawPatch(123, 10, 0, W_CacheLumpName(DEH_String("M_T_ITMS"),
                                               PU_CACHE));

    dp_translation = crx[CRX_GOLD];
    M_WriteText(80, 50, DEH_String("GIVE THEM ALL AT ONCE"));
    V_ClearDPTranslation();

    if(fsize != 12361532 && fsize != 19321722)
    {
        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 70, DEH_String("RADIATION SHIELDING SUIT"));
        V_ClearDPTranslation();

        if(itemOn == 3)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 80, DEH_String("COMPUTER AREA MAP"));
        V_ClearDPTranslation();

        if(itemOn == 4)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 90, DEH_String("LIGHT AMPLIFICATION VISOR"));
        V_ClearDPTranslation();

        if(itemOn == 7)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 120, DEH_String("PARTIAL INVISIBILITY"));
        V_ClearDPTranslation();

        if(itemOn == 8)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 130, DEH_String("INVULNERABILITY!"));
        V_ClearDPTranslation();

        if(itemOn == 9)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 140, DEH_String("BERSERK!"));
        V_ClearDPTranslation();

        if(itemOn == 10)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 150, DEH_String("BACKPACK"));
        V_ClearDPTranslation();

        if(itemOn == 11)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 160, DEH_String("FLIGHT"));
        V_ClearDPTranslation();
    }

    if(fsize == 19321722)
    {
        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 70, DEH_String("VULCAN RUBBER BOOTS"));
        V_ClearDPTranslation();

        if(itemOn == 3)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 80, DEH_String("SI ARRAY MAPPING"));
        V_ClearDPTranslation();

        if(itemOn == 4)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 90, DEH_String("INFRARED VISOR"));
        V_ClearDPTranslation();

        if(itemOn == 7)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 120, DEH_String("ENK BLINDNESS"));
        V_ClearDPTranslation();

        if(itemOn == 8)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 130, DEH_String("FORCE FIELD"));
        V_ClearDPTranslation();

        if(itemOn == 9)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 140, DEH_String("007 MICROTEL"));
        V_ClearDPTranslation();

        if(itemOn == 10)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 150, DEH_String("BACKPACK"));
        V_ClearDPTranslation();

        if(itemOn == 11)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 160, DEH_String("FLIGHT"));
        V_ClearDPTranslation();
    }

    if(fsize == 12361532)
    {
        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 70, DEH_String("SLIME-PROOF SUIT"));
        V_ClearDPTranslation();

        if(itemOn == 3)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 80, DEH_String("COMPUTER AREA MAP"));
        V_ClearDPTranslation();

        if(itemOn == 4)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 90, DEH_String("ULTRA GOGGLES"));
        V_ClearDPTranslation();

        if(itemOn == 5)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 100, DEH_String("BACKPACK"));
        V_ClearDPTranslation();

        if(itemOn == 6)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 110, DEH_String("FLIGHT"));
        V_ClearDPTranslation();
    }
}

void M_DrawArmor(void)
{
    M_DarkBackground();

    V_DrawPatch(115, 15, 0, W_CacheLumpName(DEH_String("M_T_ARMR"),
                                               PU_CACHE));

    dp_translation = crx[CRX_GOLD];
    M_WriteText(80, 55, DEH_String("GIVE THEM BOTH AT ONCE"));
    V_ClearDPTranslation();

    if(fsize != 12361532 && fsize != 19321722)
    {
        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 75, DEH_String("GREEN ARMOR"));
        V_ClearDPTranslation();

        if(itemOn == 3)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 85, DEH_String("BLUE ARMOR"));
        V_ClearDPTranslation();
    }

    if(fsize == 12361532)
    {
        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 75, DEH_String("CHEX(R) ARMOR"));
        V_ClearDPTranslation();

        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 85, DEH_String("SUPER CHEX(R) ARMOR"));
        V_ClearDPTranslation();
    }

    if(fsize == 19321722)
    {
        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 75, DEH_String("KEVLAR VEST"));
        V_ClearDPTranslation();

        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 85, DEH_String("SUPER KEVLAR VEST"));
        V_ClearDPTranslation();
    }
}

void M_DrawWeapons(void)
{
    M_DarkBackground();

    V_DrawPatch(103, 15, 0, W_CacheLumpName(DEH_String("M_T_WPNS"),
                                               PU_CACHE));

    dp_translation = crx[CRX_GOLD];
    M_WriteText(80, 55, DEH_String("GIVE THEM ALL AT ONCE"));
    V_ClearDPTranslation();

    if(fsize != 19321722 && fsize != 12361532)
    {
        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 75, DEH_String("CHAINSAW"));
        V_ClearDPTranslation();

        if(itemOn == 3)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 85, DEH_String("SHOTGUN"));
        V_ClearDPTranslation();

        if(itemOn == 4)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 95, DEH_String("CHAINGUN"));
        V_ClearDPTranslation();

        if(itemOn == 5)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 105, DEH_String("ROCKET LAUNCHER"));
        V_ClearDPTranslation();

        if(fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460 && fsize != 4234124 && fsize != 4196020)
        {
            if(itemOn == 6)
                dp_translation = crx[CRX_GOLD];
            M_WriteText(80, 115, DEH_String("PLASMA CANNON"));
            V_ClearDPTranslation();

            if(itemOn == 7)
                dp_translation = crx[CRX_GOLD];
            M_WriteText(80, 125, DEH_String("BFG 9000"));
            V_ClearDPTranslation();
        }

        if(fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                fsize == 14607420 || fsize == 14604584 || fsize == 18195736 ||
                fsize == 14683458 || fsize == 18654796 || fsize == 18240172 ||
                fsize == 17420824 || fsize == 14677988 || fsize == 14691821 ||
                fsize == 28422764)
        {
            if(itemOn == 8)
                dp_translation = crx[CRX_GOLD];
            M_WriteText(80, 135, DEH_String("SUPER SHOTGUN"));
            V_ClearDPTranslation();
        }
    }

    if(fsize == 19321722)
    {
        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 75, DEH_String("HOIG REZNATOR"));
        V_ClearDPTranslation();

        if(itemOn == 3)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 85, DEH_String("TAZER"));
        V_ClearDPTranslation();

        if(itemOn == 4)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 95, DEH_String("UZI"));
        V_ClearDPTranslation();

        if(itemOn == 5)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 105, DEH_String("PHOTON 'ZOOKA"));
        V_ClearDPTranslation();

        if(itemOn == 6)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 115, DEH_String("STICK"));
        V_ClearDPTranslation();

        if(itemOn == 7)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 125, DEH_String("NUKER"));
        V_ClearDPTranslation();

        if(itemOn == 8)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 135, DEH_String("CRYOGUN"));
        V_ClearDPTranslation();
    }

    if(fsize == 12361532)
    {
        if(itemOn == 2)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 75, DEH_String("SUPER BOOTSPORK"));
        V_ClearDPTranslation();

        if(itemOn == 3)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 85, DEH_String("LARGE ZORCHER"));
        V_ClearDPTranslation();

        if(itemOn == 4)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 95, DEH_String("RAPID ZORCHER"));
        V_ClearDPTranslation();

        if(itemOn == 5)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 105, DEH_String("ZORCH PROPULSOR"));
        V_ClearDPTranslation();

        if(itemOn == 6)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 115, DEH_String("PHASING ZORCHER"));
        V_ClearDPTranslation();

        if(itemOn == 7)
            dp_translation = crx[CRX_GOLD];
        M_WriteText(80, 125, DEH_String("LARGE AREA ZORCHING DEVICE"));
        V_ClearDPTranslation();
    }
}

void M_DrawKeys(void)
{
    M_DarkBackground();

    V_DrawPatch(125, 15, 0, W_CacheLumpName(DEH_String("M_T_KEYS"),
                                               PU_CACHE));

    dp_translation = crx[CRX_GOLD];
    M_WriteText(80, 55, DEH_String("GIVE ALL KEYS FOR THIS MAP"));
    V_ClearDPTranslation();
}

void M_DrawScreen(void)
{
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch(58, 15, 0, W_CacheLumpName(DEH_String("M_T_SSET"),
                                               PU_CACHE));
    else
        V_DrawPatch(58, 15, 0, W_CacheLumpName(DEH_String("M_SCRSET"),
                                               PU_CACHE));

    M_DrawThermoSmall(ScreenDef.x + 152, ScreenDef.y + LINEHEIGHT_SMALL * (gamma + 1),
                 5, usegamma);
/*
    V_DrawPatch(OptionsDef.x + 175, OptionsDef.y + LINEHEIGHT + 11.5 *
                      screen_detail, W_CacheLumpName(DEH_String
                      (detailNames[detailLevel]), PU_CACHE));
*/
    if(detailLevel > 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ScreenDef.x + 180, ScreenDef.y + 18, "LOW");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 177, ScreenDef.y + 18, "HIGH");
        V_ClearDPTranslation();
    }

    if(d_translucency > 0)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 189, ScreenDef.y + 28, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ScreenDef.x + 181, ScreenDef.y + 28, "OFF");
        V_ClearDPTranslation();
    }

    if(beta_style)
        M_DrawThermoSmall(ScreenDef.x + 128, ScreenDef.y + LINEHEIGHT_SMALL * (scrnsize + 1),
                 8, screenSize);
    else
        M_DrawThermoSmall(ScreenDef.x + 120, ScreenDef.y + LINEHEIGHT_SMALL * (scrnsize + 1),
                 9, screenSize);

    if(wipe_type == 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ScreenDef.x + 173, ScreenDef.y + 38, "NONE");
        V_ClearDPTranslation();
    }
    else if(wipe_type == 1)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 173, ScreenDef.y + 38, "FADE");
        V_ClearDPTranslation();
    }
    else if(wipe_type == 2)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ScreenDef.x + 173, ScreenDef.y + 38, "BURN");
        V_ClearDPTranslation();
    }
    else if(wipe_type == 3)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(ScreenDef.x + 172, ScreenDef.y + 38, "MELT");
        V_ClearDPTranslation();
    }

    if(d_uncappedframerate)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 189, ScreenDef.y + 48, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ScreenDef.x + 181, ScreenDef.y + 48, "OFF");
        V_ClearDPTranslation();
    }

    if(png_screenshots)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 181, ScreenDef.y + 58, "PNG");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ScreenDef.x + 180, ScreenDef.y + 58, "PCX");
        V_ClearDPTranslation();
    }
}

void M_DrawGame(void)
{
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch(70, 0, 0, W_CacheLumpName(DEH_String("M_T_GSET"),
                                               PU_CACHE));
    else
        V_DrawPatch(70, 0, 0, W_CacheLumpName(DEH_String("M_GMESET"),
                                               PU_CACHE));

    if(devparm)
    {
        if(aiming_help)
        {
            dp_translation = crx[CRX_GREEN];
            M_WriteText(GameDef.x + 161, GameDef.y + 128, DEH_String("ON"));
            V_ClearDPTranslation();
        }
        else
        {
            dp_translation = crx[CRX_DARK];
            M_WriteText(GameDef.x + 153, GameDef.y + 128, DEH_String("OFF"));
            V_ClearDPTranslation();
        }

        if(itemOn == 13)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(GameDef.x, GameDef.y + 128, DEH_String("AIMING HELP"));
        V_ClearDPTranslation();
    }

    if(drawgrid == 1)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y - 2, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else if(drawgrid == 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y - 2, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(am_rotate == true)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 8, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else if(am_rotate == false)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 8, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(followplayer == 1)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 18, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else if(followplayer == 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 18, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(show_stats == 1)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 28, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else if (show_stats == 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 28, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(overlay_trigger)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 38, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 38, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(timer_info)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 48, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 48, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(show_authors)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 58, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 58, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(showMessages)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 68, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 68, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(use_vanilla_weapon_change == 1)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 145, GameDef.y + 78, DEH_String("SLOW"));
        V_ClearDPTranslation();
    }
    else if(use_vanilla_weapon_change == 0)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 146, GameDef.y + 78, DEH_String("FAST"));
        V_ClearDPTranslation();
    }

    if(d_recoil)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 88, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 88, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_thrust)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 98, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 98, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(respawnparm)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 108, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 108, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(fastparm)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 161, GameDef.y + 118, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 153, GameDef.y + 118, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(devparm)
    {
        if((itemOn == 11 || itemOn == 12) && whichSkull == 1)
        {
            char *string = "YOU MUST START A NEW GAME TO TAKE EFFECT.";
            int x = 160 - M_StringWidth(string) / 2;
            dp_translation = crx[CRX_GOLD];
            M_WriteText(x, GameDef.y + 138, DEH_String(string));
            V_ClearDPTranslation();
        }
        else
        {
            if(itemOn == 14)
                dp_translation = crx[CRX_GOLD];
            else
                dp_translation = crx[CRX_GRAY];

            M_WriteText(GameDef.x, GameDef.y + 138, DEH_String("MORE OPTIONS"));
            V_ClearDPTranslation();
        }
    }
    else
    {
        if((itemOn == 11 || itemOn == 12) && whichSkull == 1)
        {
            char *string = "YOU MUST START A NEW GAME TO TAKE EFFECT.";
            int x = 160 - M_StringWidth(string) / 2;
            dp_translation = crx[CRX_GOLD];
            M_WriteText(x, GameDef.y + 138, DEH_String(string));
            V_ClearDPTranslation();
        }

        if(itemOn == 13)
            dp_translation = crx[CRX_GOLD];
        else
            dp_translation = crx[CRX_GRAY];

        M_WriteText(GameDef.x, GameDef.y + 128, DEH_String("MORE OPTIONS"));
        V_ClearDPTranslation();
    }
}

void M_DrawGame2(void)
{
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch(70, 0, 0, W_CacheLumpName(DEH_String("M_T_GSET"),
                                               PU_CACHE));
    else
        V_DrawPatch(70, 0, 0, W_CacheLumpName(DEH_String("M_GMESET"),
                                               PU_CACHE));

    if(not_monsters)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y - 2, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y - 2, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(hud)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 8, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 8, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_footstep)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 18, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 18, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_footclip)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 28, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 28, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_splash)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 38, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 38, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_swirl)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 48, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 48, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(beta_style_mode)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 58, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 58, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_flipcorpses)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 68, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 68, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_secrets)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 78, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 78, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(smoketrails)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 88, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 88, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(chaingun_tics == 1)
    {
        dp_translation = crx[CRX_BLUE];
        M_WriteText(GameDef2.x + 192, GameDef2.y + 98, DEH_String("ULTRA"));
        V_ClearDPTranslation();
    }
    else if(chaingun_tics == 2)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(GameDef2.x + 166, GameDef2.y + 98, DEH_String("VERY FAST"));
        V_ClearDPTranslation();
    }
    else if(chaingun_tics == 3)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(GameDef2.x + 185, GameDef2.y + 98, DEH_String("FASTER"));
        V_ClearDPTranslation();
    }
    else if(chaingun_tics == 4)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 183, GameDef2.y + 98, DEH_String("NORMAL"));
        V_ClearDPTranslation();
    }

    if(d_fallingdamage)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 108, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 108, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_infiniteammo)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 118, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 208, GameDef2.y + 118, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(itemOn == 0 && whichSkull == 1)
    {
        char *string = "YOU MUST START A NEW GAME TO TAKE EFFECT.";
        int x = 160 - M_StringWidth(string) / 2;
        dp_translation = crx[CRX_GOLD];
        M_WriteText(x, GameDef2.y + 138, DEH_String(string));
        V_ClearDPTranslation();
    }

    if(itemOn == 6 && whichSkull == 1)
    {
        char *string = "YOU MUST QUIT AND RESTART TO TAKE EFFECT.";
        int x = 160 - M_StringWidth(string) / 2;
        dp_translation = crx[CRX_GOLD];
        M_WriteText(x, GameDef2.y + 138, DEH_String(string));
        V_ClearDPTranslation();
    }

    if(itemOn == 13)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(GameDef2.x, GameDef2.y + 128, DEH_String("MORE & MORE OPTIONS"));
    V_ClearDPTranslation();
}

void M_DrawGame3(void)
{
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch(70, 0, 0, W_CacheLumpName(DEH_String("M_T_GSET"),
                                               PU_CACHE));
    else
        V_DrawPatch(70, 0, 0, W_CacheLumpName(DEH_String("M_GMESET"),
                                               PU_CACHE));

    if(crosshair == 1)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y -2, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else if (crosshair == 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y -2, DEH_String("OFF"));
        V_ClearDPTranslation();
    }
    
    if(autoaim)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 8, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 8, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(jumping)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 18, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 18, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_maxgore)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 28, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 28, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(gore_amount == 1)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 257, GameDef3.y + 38, DEH_String("LOW"));
        V_ClearDPTranslation();
    }
    else if(gore_amount == 2)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(GameDef3.x + 236, GameDef3.y + 38, DEH_String("MEDIUM"));
        V_ClearDPTranslation();
    }
    else if(gore_amount == 3)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(GameDef3.x + 254, GameDef3.y + 38, DEH_String("HIGH"));
        V_ClearDPTranslation();
    }
    else if(gore_amount == 4)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(GameDef3.x + 174, GameDef3.y + 38, DEH_String("RIP'EM TO PIECES"));
        V_ClearDPTranslation();
    }

    if(d_colblood)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 48, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 48, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_colblood2)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 58, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 58, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_telefrag)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 68, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 68, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_doorstuck)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 78, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 78, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_resurrectghosts)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 88, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 88, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_limitedghosts)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 98, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 98, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(!d_blockskulls)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 108, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 108, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(!d_blazingsound)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 118, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 118, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(itemOn == 13)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(GameDef3.x, GameDef3.y + 128, DEH_String("EVEN MORE OPTIONS"));
    V_ClearDPTranslation();
}

void M_DrawGame4(void)
{
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch(70, 0, 0, W_CacheLumpName(DEH_String("M_T_GSET"),
                                               PU_CACHE));
    else
        V_DrawPatch(70, 0, 0, W_CacheLumpName(DEH_String("M_GMESET"),
                                               PU_CACHE));

    if(d_god)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y - 2, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y - 2, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_floors)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 8, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 8, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_moveblock)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 18, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 18, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_model)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 28, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 28, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_666)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 38, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 38, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(correct_lost_soul_bounce)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 48, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 48, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_maskedanim)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 58, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 58, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_sound)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 68, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 68, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(d_ouchface)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 78, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 78, DEH_String("OFF"));
        V_ClearDPTranslation();
    }
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
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch (110, 6, 0, W_CacheLumpName(DEH_String("M_T_CHTS"), PU_CACHE));
    else
        V_DrawPatch (110, 6, 0, W_CacheLumpName(DEH_String("M_CHEATS"), PU_CACHE));

    if (players[consoleplayer].cheats & CF_GODMODE)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(223, 26, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(215, 26, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if (players[consoleplayer].cheats & CF_NOCLIP)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(223, 36, DEH_String("ON"));
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(215, 36, DEH_String("OFF"));
        V_ClearDPTranslation();
    }

    if(itemOn == 2)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];
    M_WriteText(75, 46, DEH_String("WEAPONS..."));
    V_ClearDPTranslation();

    if(itemOn == 3)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];
    M_WriteText(75, 56, DEH_String("KEYS..."));
    V_ClearDPTranslation();

    if(itemOn == 4)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];
    M_WriteText(75, 66, DEH_String("ARMOR..."));
    V_ClearDPTranslation();

    if(itemOn == 5)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];
    M_WriteText(75, 76, DEH_String("ITEMS..."));
    V_ClearDPTranslation();

    if(!cheating)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(215, 86, DEH_String("OFF"));
        V_ClearDPTranslation();
    }
    else if (cheating && cheeting!=2)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(199, 86, DEH_String("WALLS"));
        V_ClearDPTranslation();
    }
    else if (cheating && cheeting==2)          
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(215, 86, DEH_String("ALL"));
        V_ClearDPTranslation();
    }

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
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, "E1M10: SEWERS");
            V_ClearDPTranslation();
        }
        if(epi == 1 && gameversion != exe_chex && map != 10)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map]);
            V_ClearDPTranslation();
        }
        else if(epi == 2 && gameversion != exe_chex)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map+9]);
            V_ClearDPTranslation();
        }
        else if(epi == 3 && gameversion != exe_chex)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map+18]);
            V_ClearDPTranslation();
        }
        else if(epi == 4 && gameversion != exe_chex)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map+27]);
            V_ClearDPTranslation();
        }
        else if(epi == 1 && gameversion == exe_chex && !is_chex_2)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map+132]);
            V_ClearDPTranslation();
        }
        else if(epi == 1 && gameversion == exe_chex && is_chex_2)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map+137]);
            V_ClearDPTranslation();
        }
    }

    if((fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
            fsize == 14607420 || fsize == 14604584 || fsize == 14677988 ||
            fsize == 14683458 || fsize == 14691821 || fsize == 28422764) &&
            map < 33)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map+36]);
        V_ClearDPTranslation();
    }
    if((fsize == 14677988 || fsize == 14683458 || fsize == 14691821) &&
        map == 33)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, "LEVEL 33: BETRAY");        
        V_ClearDPTranslation();
    }
    if(fsize == 18195736 || fsize == 18654796)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map+68]);
        V_ClearDPTranslation();
    }
    if(fsize == 18240175 || fsize == 17420824)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map+100]);
        V_ClearDPTranslation();
    }
    if(fsize == 19321722)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map+142]);
        V_ClearDPTranslation();
    }
    if(fsize == 12361532 && !is_chex_2)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map+132]);
        V_ClearDPTranslation();
    }
    if(fsize == 12361532 && is_chex_2)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map+137]);
        V_ClearDPTranslation();
    }
    if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
        fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
        fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
    {
        if(tracknum == 0)
            tracknum = 1;

        if(faketracknum == 0)
            faketracknum = 1;

        dp_translation = crx[CRX_GRAY];
        M_WriteText(220, 156, songtextbeta[tracknum]);
        V_ClearDPTranslation();
    }
    else if(fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
            fsize == 11159840 || fsize == 12408292 || fsize == 12361532 ||
            fsize == 12474561 || fsize == 12538385 || fsize == 12487824)
    {
        if(tracknum == 0)
            tracknum = 1;

        if(fsize == 12361532)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtextchex[tracknum]);
            V_ClearDPTranslation();
        }
        else
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtext[tracknum]);
            V_ClearDPTranslation();
        }
    }
    else if(fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
            fsize == 14607420 || fsize == 14604584 || fsize == 19321722 ||
            fsize == 28422764 || fsize == 14677988 || fsize == 14683458 ||
            fsize == 14691821)
    {
        if(tracknum == 1)
            tracknum = 33;

        if(fsize == 19321722)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtext2hacx[tracknum]);
            V_ClearDPTranslation();
        }
        else if(fsize == 28422764)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtext2fd[tracknum]);
            V_ClearDPTranslation();
        }
        else
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtext2[tracknum]);
            V_ClearDPTranslation();
        }
    }
    else if(fsize == 18195736 || fsize == 18654796)
    {
        if(tracknum == 1)
            tracknum = 33;

        dp_translation = crx[CRX_GRAY];
        M_WriteText(220, 156, songtexttnt[tracknum]);
        V_ClearDPTranslation();
    }
    else if(fsize == 18240172 || fsize == 17420824)
    {
        if(tracknum == 1)
            tracknum = 33;

        dp_translation = crx[CRX_GRAY];
        M_WriteText(220, 156, songtextplut[tracknum]);
        V_ClearDPTranslation();
    }
    DetectState();
}

void M_DrawRecord(void)
{
    M_DarkBackground();

    char buffer_map[2];

    M_snprintf(buffer_map, sizeof(buffer_map), "%d", rmap);

    int offset = 0;

    V_DrawPatch(58, 15, 0, W_CacheLumpName(DEH_String("M_T_DREC"),
                                               PU_CACHE));
    if (rmap == 0)
        rmap = 1;

    if (fsize != 14943400 && fsize != 14824716 && fsize != 14612688 &&
        fsize != 14607420 && fsize != 14604584 && fsize != 18195736 &&
        fsize != 18654796 && fsize != 18240172 && fsize != 17420824 &&
        fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
    {
        if(repi == 1)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "E1M");
            V_ClearDPTranslation();
        }
        else if(repi == 2)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "E2M");
            V_ClearDPTranslation();
        }
        else if(repi == 3)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "E3M");
            V_ClearDPTranslation();
        }
        else if(repi == 4)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "E4M");
            V_ClearDPTranslation();
        }

        if(repi > 1)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x + 25, RecordDef.y + 8, buffer_map);
            V_ClearDPTranslation();
        }
        else
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x + 22, RecordDef.y + 8, buffer_map);
            V_ClearDPTranslation();
        }
    }
    else if(fsize != 12361532)
    {
        if(rmap > 9 && rmap < 20)
            offset = 3;
        else
            offset = 0;

        if(rmap < 10)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "MAP0");
            V_ClearDPTranslation();
        }
        else if(rmap < 20)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "MAP1");
            V_ClearDPTranslation();
        }
        else if(rmap < 30)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "MAP2");
            V_ClearDPTranslation();
        }
        else if(rmap < 40)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "MAP3");
            V_ClearDPTranslation();
        }

        if(fsize != 19321722 && fsize != 28422764)
        {
            if(rmap == 1 || rmap == 11 || rmap == 21 || rmap == 31)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "1");
                V_ClearDPTranslation();
            }
            else if(rmap == 2 || rmap == 12 || rmap == 22 || rmap == 32)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "2");
                V_ClearDPTranslation();
            }
            else if(rmap == 3 || rmap == 13 || rmap == 23)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "3");
                V_ClearDPTranslation();
            }
            else if(rmap == 4 || rmap == 14 || rmap == 24)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "4");
                V_ClearDPTranslation();
            }
            else if(rmap == 5 || rmap == 15 || rmap == 25)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "5");
                V_ClearDPTranslation();
            }
            else if(rmap == 6 || rmap == 16 || rmap == 26)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "6");
                V_ClearDPTranslation();
            }
            else if(rmap == 7 || rmap == 17 || rmap == 27)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "7");
                V_ClearDPTranslation();
            }
            else if(rmap == 8 || rmap == 18 || rmap == 28)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "8");
                V_ClearDPTranslation();
            }
            else if(rmap == 9 || rmap == 19 || rmap == 29)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "9");
                V_ClearDPTranslation();
            }
            else if(rmap == 10 || rmap == 20 || rmap == 30)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "0");
                V_ClearDPTranslation();
            }
        }
        else
        {
            if(fsize == 28422764)
                offset = 7;
            if(rmap == 1 || rmap == 11 || rmap == 21 || rmap == 31)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM1"), PU_CACHE));
            else if(rmap == 2 || rmap == 12 || rmap == 22 || rmap == 32)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM2"), PU_CACHE));
            else if(rmap == 3 || rmap == 13 || rmap == 23)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM3"), PU_CACHE));
            else if(rmap == 4 || rmap == 14 || rmap == 24)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM4"), PU_CACHE));
            else if(rmap == 5 || rmap == 15 || rmap == 25)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM5"), PU_CACHE));
            else if(rmap == 6 || rmap == 16 || rmap == 26)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM6"), PU_CACHE));
            else if(rmap == 7 || rmap == 17 || rmap == 27)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM7"), PU_CACHE));
            else if(rmap == 8 || rmap == 18 || rmap == 28)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM8"), PU_CACHE));
            else if(rmap == 9 || rmap == 19 || rmap == 29)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM9"), PU_CACHE));
            else if(rmap == 10 || rmap == 20 || rmap == 30)
                V_DrawPatch (115 - offset, 68, 0, W_CacheLumpName
                (DEH_String("WINUM0"), PU_CACHE));
        }
    }

    if(fsize == 12361532)
    {
        if(repi == 1)
        {
            if(rmap == 1)
                V_DrawPatch (55, 68, 0, W_CacheLumpName(DEH_String("WILV00"),
                PU_CACHE));
            else if(rmap == 2)
                V_DrawPatch (55, 68, 0, W_CacheLumpName(DEH_String("WILV01"),
                PU_CACHE));
            else if(rmap == 3)
                V_DrawPatch (55, 68, 0, W_CacheLumpName(DEH_String("WILV02"),
                PU_CACHE));
            else if(rmap == 4)
                V_DrawPatch (55, 68, 0, W_CacheLumpName(DEH_String("WILV03"),
                PU_CACHE));
            else if(rmap == 5)
                V_DrawPatch (55, 68, 0, W_CacheLumpName(DEH_String("WILV04"),
                PU_CACHE));
        }
    }

    if(rskill == 0)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(RecordDef.x, RecordDef.y + 38, "I'm too young to die.");
        V_ClearDPTranslation();
    }
    else if(rskill == 1)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(RecordDef.x, RecordDef.y + 38, "Hey, not too rough.");
        V_ClearDPTranslation();
    }
    else if(rskill == 2)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(RecordDef.x, RecordDef.y + 38, "Hurt me plenty.");
        V_ClearDPTranslation();
    }
    else if(rskill == 3)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(RecordDef.x, RecordDef.y + 38, "Ultra-Violence.");
        V_ClearDPTranslation();
    }
    else if(rskill == 4)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(RecordDef.x, RecordDef.y + 38, "Nightmare!");
        V_ClearDPTranslation();
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
/*
    V_DrawPatch(58, 15, 0, W_CacheLumpName(DEH_String("M_T_EGME"),
                                               PU_CACHE));
*/
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

void M_Translucency(int choice)
{
    choice = 0;
    d_translucency = !d_translucency;

    // translucent HUD?
    if (screenblocks > TRANSLUCENT_HUD)
        M_SizeDisplay(0);
}

void M_ColoredBloodA(int choice)
{
    if (!d_chkblood)
        return;

    choice = 0;
    d_colblood = 1 - !!d_colblood;
}

void M_ColoredBloodB(int choice)
{
    if (!d_chkblood2)
        return;

    choice = 0;
    d_colblood2 = 1 - !!d_colblood2;
}

void M_WipeType(int choice)
{
    switch(choice)
    {
      case 0:
        if (wipe_type > 0)
            wipe_type--;
        break;
      case 1:
        if (wipe_type < 3)
            wipe_type++;
        break;
    }
}

void M_UncappedFramerate(int choice)
{
    choice = 0;
    d_uncappedframerate = !d_uncappedframerate;
}

void M_Screenshots(int choice)
{
    switch(choice)
    {
      case 0:
        if (png_screenshots)
            png_screenshots = 0;
        break;
      case 1:
        if (png_screenshots == 0)
            png_screenshots = 1;
        break;
    }
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
        if (screenSize == 8 && beta_style)
        {
            screenblocks--;
            screenSize--;
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
    V_DrawPatch(xx, y, 0, W_CacheLumpName(DEH_String("M_THERML"), PU_CACHE));
    xx += 8;
    for (i=0;i<thermWidth;i++)
    {
        V_DrawPatch(xx, y, 0, W_CacheLumpName(DEH_String("M_THERMM"), PU_CACHE));
        xx += 8;
    }
    V_DrawPatch(xx, y, 0, W_CacheLumpName(DEH_String("M_THERMR"), PU_CACHE));

    V_DrawPatch((x + 8) + thermDot * 8, y, 0,
                      W_CacheLumpName(DEH_String("M_THERMO"), PU_CACHE));
}

void
M_DrawThermoSmall
( int        x,
  int        y,
  int        thermWidth,
  int        thermDot )
{
    int         xx;
    int         yy;
    int         i;

    xx = x;
    yy = y + 6; // +6 to y coordinate
    V_DrawPatch(xx + 3, yy - 18, 0, W_CacheLumpName(DEH_String("M_SLIDEL"), PU_CACHE));
    xx += 8;
    for (i=0;i<thermWidth;i++)
    {
        V_DrawPatch(xx, yy - 18, 0, W_CacheLumpName(DEH_String("M_SLIDEM"), PU_CACHE));
        xx += 8;
    }
    V_DrawPatch(xx, yy - 18, 0, W_CacheLumpName(DEH_String("M_SLIDER"), PU_CACHE));

    // +2 to initial y coordinate
    V_DrawPatch((x + 9) + thermDot * 8, y - 12, 0,
                      W_CacheLumpName(DEH_String("M_SLIDEO"), PU_CACHE));
}


void
M_DrawEmptyCell
( menu_t*        menu,
  int            item )
{
    V_DrawPatch(menu->x - 10, menu->y + item * LINEHEIGHT_SMALL - 1, 0,
                      W_CacheLumpName(DEH_String("M_CELL1"), PU_CACHE));
}

void
M_DrawSelCell
( menu_t*        menu,
  int            item )
{
    V_DrawPatch(menu->x - 10, menu->y + item * LINEHEIGHT_SMALL - 1, 0,
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
void M_WriteText(int x, int y, char* string)
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

        if (c == '\x1b')
        {
            c = *ch++;
            dp_translation = crx[(int) (c - '0')];
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
        V_DrawPatch(cx, cy, 0, hu_font[c]);
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
        *doom_defaults_list[keyaskedfor + key_controls_start_in_cfg_at_pos + FirstKey].location = ev->data1;
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
            M_StringCopy(&savegamestrings[saveSlot][0],saveOldString, sizeof(savegamestrings));
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
            else
                itemOn++;

            if(!devparm)
            {
/*
                if (currentMenu == &KeyBindingsDef && itemOn == 11)
                    itemOn++;
                else*/ if (currentMenu == &GameDef && itemOn == 14)
                    itemOn = 0;
            }
            S_StartSound(NULL,sfx_pstop);
        } while((currentMenu->menuitems[itemOn].status==-1 && !devparm) ||
                (currentMenu->menuitems[itemOn].status==-1 && currentMenu != &GameDef && devparm));

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
            else
                itemOn--;

            if(!devparm)
            {
                if((currentMenu == &KeyBindingsDef && itemOn == 11) ||
                        (currentMenu == &GameDef && itemOn == 14))
                    itemOn--;
            }
            S_StartSound(NULL,sfx_pstop);
        } while((currentMenu->menuitems[itemOn].status==-1 && !devparm) ||
                (currentMenu->menuitems[itemOn].status==-1 && currentMenu != &GameDef && devparm));

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
        else if(currentMenu == &GameDef && devparm && itemOn == 13)
        {
            S_StartSound(NULL,sfx_stnmov);
            M_AimingHelp(0);
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
        else if(currentMenu == &GameDef && devparm && itemOn == 13)
        {
            S_StartSound(NULL,sfx_stnmov);
            M_AimingHelp(1);
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
        else if(currentMenu == &GameDef && !devparm && itemOn == 13)
        {
            S_StartSound(NULL,sfx_pistol);
            M_Game2(ch);
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
    blurred = false;
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
    if ((fsize == 4261144 || fsize == 4271324 || fsize == 4211660 || beta_style) &&
            !menuactive && leveltime & 16 && gamestate == GS_LEVEL &&
            consoleheight == 0)
    {
        M_WriteText(140, 12, "BETA");
        BorderNeedRefresh = true;
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

    if(memory_usage && done && consoleheight == 0 && !menuactive)
    {
        Z_DrawStats();           // print memory allocation stats
    }

    // DISPLAYS BINARY VERSION
    if(version_info)
        M_WriteText(65, 36, DOOM_VERSIONTEXT);

    // Horiz. & Vertically center string and print it.
    if (messageToPrint)
    {
        M_DarkBackground();

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
                M_StringCopy(string, messageString + start, sizeof(string));
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

    if (fsize != 28422764 && fsize != 19321722 && fsize != 12361532 &&
        currentMenu == &SoundDef && itemOn > 1 && itemOn < 5 && whichSkull == 1)
    {
        char *message_string = "YOU MUST QUIT AND RESTART TO TAKE EFFECT.";
        int message_offset = 160 - M_StringWidth(message_string) / 2;
        dp_translation = crx[CRX_GOLD];
        M_WriteText(message_offset, 160, DEH_String(message_string));
        V_ClearDPTranslation();
    }
    else if((fsize == 28422764 || fsize == 19321722 || fsize == 12361532) &&
            currentMenu == &GameDef2 && itemOn == 6 && whichSkull == 1)
    {
        char *message_string = "NO BETA MODE FOR CHEX, HACX & FREEDOOM.";
        int message_offset = 160 - M_StringWidth(message_string) / 2;
        dp_translation = crx[CRX_GOLD];
        M_WriteText(message_offset, 160, DEH_String(message_string));
        V_ClearDPTranslation();
    }
    else if(fsize == 12361532 && currentMenu == &GameDef3 && itemOn == 1 && whichSkull == 1)
    {
        char *message_string = "NO EXTRA BLOOD & GORE FOR CHEX QUEST.";
        int message_offset = 160 - M_StringWidth(message_string) / 2;
        dp_translation = crx[CRX_GOLD];
        M_WriteText(message_offset, 160, DEH_String(message_string));
        V_ClearDPTranslation();
    }
    else if(fsize == 19321722 && currentMenu == &SoundDef && itemOn == 3 && whichSkull == 1)
    {
        char *message_string = "NO PC-SPEAKERS AVAILABLE FOR HACX";
        int message_offset = 160 - M_StringWidth(message_string) / 2;
        dp_translation = crx[CRX_GOLD];
        M_WriteText(message_offset, 160, DEH_String(message_string));
        V_ClearDPTranslation();
    }

    // FOR PSP (if too many menu items)
    if (currentMenu->menuitems[itemOn].status == 5)
        max += FirstKey;
/*
    if(extra_wad_loaded == 1 && currentMenu == &SoundDef)
        currentMenu->numitems = 4;
*/
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
        menuitem_t *item = &(currentMenu->menuitems[i]);
        name = DEH_String(item->name);

        if(*name)
        {
            if ((currentMenu == &FilesDef && i == savegame && !usergame) ||
                (currentMenu == &FilesDef && i == endgame && !usergame) ||
                (currentMenu == &FilesDef && i == loadgame && netgame))
                dp_translation = crx[CRX_DARK];
            else if (i == itemOn)
                dp_translation = crx[CRX_GOLD];
            else if (currentMenu == &NewDef)
            {
                if (itemOn == 2)
                    dp_translation = crx[CRX_GOLD];

                if(beta_style)
                    M_WriteText(NewDef.x, NewDef.y + 18, "I JUST WANT TO KILL.");
                else
                    M_WriteText(NewDef.x, NewDef.y + 18, "HURT ME PLENTY.");

    
                V_ClearDPTranslation();
            }
            M_WriteText(x, y - 2, item->name);

            V_ClearDPTranslation();
        }
        y += LINEHEIGHT_SMALL;

        // DRAW SKULL
        if(currentMenu == &KeyBindingsDef && itemOn == 15)
            V_DrawPatch(x + 280 + CURSORXOFF_SMALL, currentMenu->y - 15 +
                    itemOn*LINEHEIGHT_SMALL, 0,
                    W_CacheLumpName(DEH_String(skullNameSmall[whichSkull]),
                                          PU_CACHE));
        else
            V_DrawPatch(x + CURSORXOFF_SMALL, currentMenu->y - 5 +
                    itemOn*LINEHEIGHT_SMALL, 0,
                    W_CacheLumpName(DEH_String(skullNameSmall[whichSkull]),
                                          PU_CACHE));
/*
        name = DEH_String(currentMenu->menuitems[i].name);

        if (name[0])
        {
            V_DrawPatch (x, y, W_CacheLumpName(name, PU_CACHE));
        }

        if (currentMenu == &CheatsDef || currentMenu == &KeyBindingsDef ||
            currentMenu == &ItemsDef || currentMenu == &WeaponsDef ||
            currentMenu == &ArmorDef || currentMenu == &KeysDef || 
            currentMenu == &GameDef || currentMenu == &GameDef2)
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
            V_DrawPatch(x + SKULLXOFF, currentMenu->y - 5 +
                              itemOn*LINEHEIGHT,
                              W_CacheLumpName(DEH_String(skullName[whichSkull]),
                                              PU_CACHE));
        }
*/
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
    tempscreen = Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    blurredscreen = Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);

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
            noclip_on = true;
        }
        else
        {
            players[consoleplayer].message = DEH_String(STSTR_NCOFF);
            players[consoleplayer].mo->flags &= ~MF_NOCLIP;
            noclip_on = false;
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
    player_t *player = &players[consoleplayer];

    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        P_GiveAllCards(player);
        players[consoleplayer].message = DEH_String("ALL KEYS FOR THIS MAP ADDED");
    }
    DetectState();
}

void M_KeysB(int choice)
{
    player_t *player = &players[consoleplayer];

    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        P_GiveCard (player, it_bluecard);
        
        players[consoleplayer].message = DEH_String("BLUE KEYCARD ADDED");
    }
    DetectState();
}

void M_KeysC(int choice)
{
    player_t *player = &players[consoleplayer];

    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        P_GiveCard (player, it_yellowcard);
        
        players[consoleplayer].message = DEH_String("YELLOW KEYCARD ADDED");
    }
    DetectState();
}

void M_KeysD(int choice)
{
    player_t *player = &players[consoleplayer];

    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        P_GiveCard (player, it_redcard);
        
        players[consoleplayer].message = DEH_String("RED KEYCARD ADDED");
    }
    DetectState();
}

void M_KeysE(int choice)
{
    player_t *player = &players[consoleplayer];

    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        P_GiveCard (player, it_blueskull);
        
        players[consoleplayer].message = DEH_String("BLUE SKULLKEY ADDED");
    }
    DetectState();
}

void M_KeysF(int choice)
{
    player_t *player = &players[consoleplayer];

    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        P_GiveCard (player, it_yellowskull);
        
        players[consoleplayer].message = DEH_String("YELLOW SKULLKEY ADDED");
    }
    DetectState();
}

void M_KeysG(int choice)
{
    player_t *player = &players[consoleplayer];

    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        P_GiveCard (player, it_redskull);
        
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
            players[consoleplayer].powers[6] = FLIGHTTICS;
            player->cheats ^= CF_NOTARGET;

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
            players[consoleplayer].powers[6] = 0;
            player->cheats &= ~CF_NOTARGET;

            if(beta_style)
                players[consoleplayer].fixedcolormap = 0;

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
            players[consoleplayer].cheats ^= CF_NOTARGET;
            players[consoleplayer].powers[2] = INVISTICS;
            players[consoleplayer].mo->flags |= MF_SHADOW;

            got_invisibility = true;
        }
        else
        {
            players[consoleplayer].powers[2] = 0;

            got_invisibility = false;

            if(beta_style)
                players[consoleplayer].fixedcolormap = 0;
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

void M_ItemsK(int choice)
{
    if(!netgame && !demoplayback && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_LIVE)
    {
        static player_t* player;
        player = &players[consoleplayer];

        P_UseArtifact(player, arti_fly);

        player->message = DEH_String("FLIGHT ADDED");
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

        if(mus_engine == 3)
            S_ChangeMusic(tracknum, false);
        else if(mus_engine == 1 || mus_engine == 2)
            S_ChangeMusic(tracknum, true);

        mus_cheat_used = true;
    }
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
    int i;

    for (i = key_controls_start_in_cfg_at_pos; i < key_controls_end_in_cfg_at_pos; i++)
        *doom_defaults_list[i].location = 0;

    *doom_defaults_list[key_controls_start_in_cfg_at_pos + 2].location = CLASSIC_CONTROLLER_MINUS;
}

void M_KeyBindingsReset (int choice)
{
    int i = key_controls_start_in_cfg_at_pos;

    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_R;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_L;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_MINUS;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_LEFT;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_DOWN;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_RIGHT;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_ZL;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_ZR;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_A;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_Y;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_B;
    *doom_defaults_list[i++].location = CONTROLLER_1;
    *doom_defaults_list[i++].location = CONTROLLER_2;
    *doom_defaults_list[i++].location = CLASSIC_CONTROLLER_X;
}

void M_DrawKeyBindings(void)
{
    M_DarkBackground();

    int i;

    if(fsize != 19321722)
        V_DrawPatch (80, 0, 0, W_CacheLumpName(DEH_String("M_T_BNDS"), PU_CACHE));
    else
        V_DrawPatch (80, 0, 0, W_CacheLumpName(DEH_String("M_KBNDGS"), PU_CACHE));

    for (i = 0; i < key_controls_end_in_cfg_at_pos - key_controls_start_in_cfg_at_pos; i++)
    {
        if (askforkey && keyaskedfor == i)
            M_WriteText(195, (i*10+20), "???");
        else
            M_WriteText(195, (i*10+20),
                    Key2String(*(doom_defaults_list[i + FirstKey + key_controls_start_in_cfg_at_pos].location)));
    }

    dp_translation = crx[CRX_GRAY];
    M_WriteText(183, 160, "/");
    V_ClearDPTranslation();

    dp_translation = crx[CRX_BLUE];

    if(itemOn == 14)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(45, 160, "CLEAR ALL CONTROLS");

    if(itemOn == 14)
        V_ClearDPTranslation();

    V_ClearDPTranslation();

    dp_translation = crx[CRX_BLUE];

    if(itemOn == 15)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(195, 160, "RESET DEFAULTS");

    if(itemOn == 15)
        V_ClearDPTranslation();

    V_ClearDPTranslation();
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
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch(48, 15, 0, W_CacheLumpName(DEH_String("M_T_CSET"),
                                               PU_CACHE));
    else
        V_DrawPatch(48, 15, 0, W_CacheLumpName(DEH_String("M_CTLSET"),
                                               PU_CACHE));

    if(mouselook == 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ControlsDef.x + 276, ControlsDef.y + 38, "OFF");
        V_ClearDPTranslation();
    }
    else if(mouselook == 1)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ControlsDef.x + 251, ControlsDef.y + 38, "NORMAL");
        V_ClearDPTranslation();
    }
    else if(mouselook == 2)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ControlsDef.x + 250, ControlsDef.y + 38, "INVERSE");
        V_ClearDPTranslation();
    }
    M_WriteText(ControlsDef.x, ControlsDef.y - 12, "SPEEDS:");

    M_DrawThermoSmall(ControlsDef.x + 55,ControlsDef.y + LINEHEIGHT_SMALL*(mousesens+1),
                 29,forwardmove-19);

    M_DrawThermoSmall(ControlsDef.x + 239,ControlsDef.y + LINEHEIGHT_SMALL*(turnsens+1),
                 6,turnspeed-5);

    M_DrawThermoSmall(ControlsDef.x + 151,ControlsDef.y + LINEHEIGHT_SMALL*(strafesens+1),
                 17,sidemove-16);

    M_DrawThermoSmall(ControlsDef.x + 199,ControlsDef.y + LINEHEIGHT_SMALL*(mousespeed+1),
                 11,mspeed);
}

void M_FPS(int choice)
{
    if(display_fps < 1)
    {
        display_fps++;
        players[consoleplayer].message = DEH_String("FPS COUNTER ON");
    }
    else if(display_fps)
    {
        display_fps--;
        players[consoleplayer].message = DEH_String("FPS COUNTER OFF");
    }
}

void M_HOMDetector(int choice)
{
    if(!autodetect_hom)
    {
        autodetect_hom = true;
        players[consoleplayer].message = DEH_String("HALL OF MIRRORS DETECTOR ENABLED");
    }
    else if(autodetect_hom)
    {
        autodetect_hom = false;
        players[consoleplayer].message = DEH_String("HALL OF MIRRORS DETECTOR DISABLED");
    }
}

void M_MemoryUsage(int choice)
{
    if(!memory_usage)
    {
        memory_usage = true;
        players[consoleplayer].message = DEH_String("SHOW MEMORY USAGE ENABLED");
    }
    else if(memory_usage)
    {
        memory_usage = false;
        players[consoleplayer].message = DEH_String("SHOW MEMORY USAGE DISABLED");
    }
}

void M_ReplaceMissing(int choice)
{
    if(!replace_missing)
    {
        replace_missing = true;
        players[consoleplayer].message = DEH_String("MISSING TEXTURES & FLATS WILL BE REPLACED");
    }
    else if(replace_missing)
    {
        replace_missing = false;
        players[consoleplayer].message = DEH_String("MISSING TEXTURES & FLATS WON'T BE REPLACED");
    }
}

u64 GetTicks(void)
{
    return (u64)SDL_GetTicks();
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

void M_Authors(int choice)
{
    switch(choice)
    {
    case 0:
        if (show_authors)
            show_authors = false;
        break;
    case 1:
        if (!show_authors)
            show_authors = true;
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

void M_SoundInfo(int choice)
{
    switch(choice)
    {
    case 0:
        if (sound_info)
            sound_info = false;
        break;
    case 1:
        if (sound_info == false)
            sound_info = true;
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
    M_DarkBackground();
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
    M_SetupNextMenu(&GameDef);
}

void M_Game2(int choice)
{
    M_SetupNextMenu(&GameDef2);
}

void M_Game3(int choice)
{
    M_SetupNextMenu(&GameDef3);
}

void M_Game4(int choice)
{
    M_SetupNextMenu(&GameDef4);
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
        if (d_maxgore && fsize != 12361532)
            d_maxgore = false;
        players[consoleplayer].message = DEH_String("MORE GORE DISABLED");
        break;
    case 1:
        if (!d_maxgore && fsize != 12361532)
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

void M_Splash(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_splash)
            d_splash = false;
        players[consoleplayer].message = DEH_String("HERETIC LIQUID SPLASH DISABLED");
        break;
    case 1:
        if (!d_splash)
            d_splash = true;
        players[consoleplayer].message = DEH_String("HERETIC LIQUID SPLASH ENABLED");
        break;
    }
}

void M_Swirl(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_swirl)
            d_swirl = 0;
        players[consoleplayer].message = DEH_String("SWIRLING WATER HACK DISABLED");
        break;
    case 1:
        if (!d_swirl)
            d_swirl = 1;
        players[consoleplayer].message = DEH_String("SWIRLING WATER HACK ENABLED");
        break;
    }
}

void M_Beta(int choice)
{
    switch(choice)
    {
    case 0:
        if (beta_style_mode && fsize != 28422764 && fsize != 19321722 && fsize != 12361532)
        {
            beta_style_mode = false;
        }
        players[consoleplayer].message = DEH_String("PRE-RELEASE MODE DISABLED");
        break;
    case 1:
        if (!beta_style_mode && fsize != 28422764 && fsize != 19321722 && fsize != 12361532)
        {
            beta_style_mode = true;
        }
        players[consoleplayer].message = DEH_String("PRE-RELEASE MODE ENABLED");
        break;
    }
}

void M_Corpses(int choice)
{
    choice = 0;
    d_flipcorpses = 1 - !!d_flipcorpses;
}

void M_Secrets(int choice)
{
    choice = 0;
    d_secrets = !d_secrets;
}

void M_Trails(int choice)
{
    choice = 0;
    smoketrails = !smoketrails;
}

void M_ChaingunTics(int choice)
{
    switch(choice)
    {
    case 0:
        if (chaingun_tics > 1)
            chaingun_tics--;
        break;
    case 1:
        if (chaingun_tics < 4)
            chaingun_tics++;
        break;
    }
    players[consoleplayer].message = DEH_String("CHAINGUN SPEED HAS BEEN ADJUSTED");
}

void M_FallingDamage(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_fallingdamage)
            d_fallingdamage = false;
        players[consoleplayer].message = DEH_String("FALLING DAMAGE HAS BEEN DISABLED");
        break;
    case 1:
        if (!d_fallingdamage)
            d_fallingdamage = true;
        players[consoleplayer].message = DEH_String("FALLING DAMAGE HAS BEEN ENABLED");
        break;
    }
}

void M_InfiniteAmmo(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_infiniteammo)
            d_infiniteammo = false;
        players[consoleplayer].message = DEH_String("INFINITE AMMO HAS BEEN DISABLED");
        break;
    case 1:
        if (!d_infiniteammo)
            d_infiniteammo = true;
        players[consoleplayer].message = DEH_String("INFINITE AMMO HAS BEEN ENABLED");
        break;
    }
}

void M_Telefrag(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_telefrag)
            d_telefrag = false;
        players[consoleplayer].message = DEH_String("MONSTERS DON'T TELEFRAG ON MAP30");
        break;
    case 1:
        if (!d_telefrag)
            d_telefrag = true;
        players[consoleplayer].message = DEH_String("MONSTERS DO TELEFRAG ON MAP30");
        break;
    }
}

void M_Doorstuck(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_doorstuck)
            d_doorstuck = false;
        players[consoleplayer].message = DEH_String("MONSTERS WON'T GET STUCK ON DOORTRACKS");
        break;
    case 1:
        if (!d_doorstuck)
            d_doorstuck = true;
        players[consoleplayer].message = DEH_String("MONSTERS WILL GET STUCK ON DOORTRACKS");
        break;
    }
}

void M_ResurrectGhosts(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_resurrectghosts)
            d_resurrectghosts = false;
        players[consoleplayer].message = DEH_String("ARCH-VILE WON'T RESURRECT GHOSTS");
        break;
    case 1:
        if (!d_resurrectghosts)
            d_resurrectghosts = true;
        players[consoleplayer].message = DEH_String("ARCH-VILE WILL RESURRECT GHOSTS");
        break;
    }
}

void M_LimitedGhosts(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_limitedghosts)
            d_limitedghosts = false;
        players[consoleplayer].message = DEH_String("PAIN ELEMENTALS DO SPIT UNLIMITED GHOSTS");
        break;
    case 1:
        if (!d_limitedghosts)
            d_limitedghosts = true;
        players[consoleplayer].message = DEH_String("PAIN ELEMENTALS DON'T SPIT UNLIMITED GHOSTS");
        break;
    }
}

void M_BlockSkulls(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_blockskulls)
            d_blockskulls = false;
        players[consoleplayer].message = DEH_String("LOST SOULS WILL GET STUCK BEHIND WALLS");
        break;
    case 1:
        if (!d_blockskulls)
            d_blockskulls = true;
        players[consoleplayer].message = DEH_String("LOST SOULS WON'T GET STUCK BEHIND WALLS");
        break;
    }
}

void M_BlazingDoors(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_blazingsound)
            d_blazingsound = false;
        players[consoleplayer].message = DEH_String("BLAZING DOORS PLAY DOUBLE CLOSING SOUND");
        break;
    case 1:
        if (!d_blazingsound)
            d_blazingsound = true;
        players[consoleplayer].message = DEH_String("BLAZING DOORS DON'T PLAY DOUBLE CLOSING SOUND");
        break;
    }
}

void M_GodAbsolute(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_god)
            d_god = false;
        players[consoleplayer].message = DEH_String("GOD MODE IS ABSOLUTE");
        break;
    case 1:
        if (!d_god)
            d_god = true;
        players[consoleplayer].message = DEH_String("GOD MODE IS NOT ABSOLUTE");
        break;
    }
}

void M_Floor(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_floors)
            d_floors = false;
        players[consoleplayer].message = DEH_String("Don't use Doom's floor motion behavior");
        break;
    case 1:
        if (!d_floors)
            d_floors = true;
        players[consoleplayer].message = DEH_String("Use Doom's floor motion behavior");
        break;
    }
}

void M_Clipping(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_moveblock)
            d_moveblock = false;
        players[consoleplayer].message = DEH_String("Don't use Doom's movement clipping code");
        break;
    case 1:
        if (!d_moveblock)
            d_moveblock = true;
        players[consoleplayer].message = DEH_String("Use Doom's movement clipping code");
        break;
    }
}

void M_Model(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_model)
            d_model = false;
        players[consoleplayer].message = DEH_String("Don't use Doom's linedef trigger model");
        break;
    case 1:
        if (!d_model)
            d_model = true;
        players[consoleplayer].message = DEH_String("Use Doom's linedef trigger model");
        break;
    }
}

void M_BossDeath(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_666)
            d_666 = false;
        players[consoleplayer].message = DEH_String("Don't emulate pre-Ultimate Boss Death");
        break;
    case 1:
        if (!d_666)
            d_666 = true;
        players[consoleplayer].message = DEH_String("Emulate pre-Ultimate Boss Death");
        break;
    }
}

void M_Bounce(int choice)
{
    switch(choice)
    {
    case 0:
        if (correct_lost_soul_bounce == 1)
            correct_lost_soul_bounce = 0;
        players[consoleplayer].message = DEH_String("Lost souls don't bounce off flats");
        break;
    case 1:
        if (correct_lost_soul_bounce == 0)
            correct_lost_soul_bounce = 1;
        players[consoleplayer].message = DEH_String("Lost souls will bounce off flats");
        break;
    }
}

void M_Masked(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_maskedanim)
            d_maskedanim = false;
        players[consoleplayer].message = DEH_String("2S middle textures won't animate");
        break;
    case 1:
        if (!d_maskedanim)
            d_maskedanim = true;
        players[consoleplayer].message = DEH_String("2S middle textures will be animated");
        break;
    }
}

void M_Quirk(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_sound)
            d_sound = false;
        players[consoleplayer].message = DEH_String("Quirks in the sound code will be retained");
        break;
    case 1:
        if (!d_sound)
            d_sound = true;
        players[consoleplayer].message = DEH_String("Quirks in the sound code won't be retained");
        break;
    }
}

void M_Ouch(int choice)
{
    switch(choice)
    {
    case 0:
        if (d_ouchface)
            d_ouchface = false;
        players[consoleplayer].message = DEH_String("Don't use Doom's buggy ouch face code");
        break;
    case 1:
        if (!d_ouchface)
            d_ouchface = true;
        players[consoleplayer].message = DEH_String("Use Doom's buggy ouch face code");
        break;
    }
}

void M_GoreAmount(int choice)
{
    switch(choice)
    {
    case 0:
        if (gore_amount > 1)
            gore_amount--;
        break;
    case 1:
        if (gore_amount < 4)
            gore_amount++;
        break;
    }
    players[consoleplayer].message = DEH_String("THE AMOUNT OF GORE HAS BEEN ADJUSTED");
}

void M_NoMonsters(int choice)
{
    switch(choice)
    {
    case 0:
        if (not_monsters)
            not_monsters = false;
        players[consoleplayer].message = DEH_String("NO MONSTERS OPTION HAS BEEN DISABLED");
        break;
    case 1:
        if (!not_monsters)
            not_monsters = true;
        players[consoleplayer].message = DEH_String("NO MONSTERS OPTION HAS BEEN ENABLED");
        break;
    }
}

void M_AutomapOverlay(int choice)
{
    switch(choice)
    {
    case 0:
        if (overlay_trigger)
            overlay_trigger = false;
        players[consoleplayer].message = DEH_String("AUTOMAP OVERLAY MODE HAS BEEN DISABLED");
        break;
    case 1:
        if (!overlay_trigger)
            overlay_trigger = true;
        players[consoleplayer].message = DEH_String("AUTOMAP OVERLAY MODE HAS BEEN ENABLED");
        break;
    }
}

void M_Debug(int choice)
{
    M_SetupNextMenu(&DebugDef);
}

void M_DrawSystem(void)
{
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch (62, 20, 0, W_CacheLumpName(DEH_String("M_T_YSET"), PU_CACHE));
    else
        V_DrawPatch (62, 20, 0, W_CacheLumpName(DEH_String("M_SYSSET"), PU_CACHE));

    if(display_fps)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SystemDef.x + 198, SystemDef.y - 2, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SystemDef.x + 190, SystemDef.y - 2, "OFF");
        V_ClearDPTranslation();
    }

    if(display_ticker)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SystemDef.x + 198, SystemDef.y + 8, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SystemDef.x + 190, SystemDef.y + 8, "OFF");
        V_ClearDPTranslation();
    }

    if(autodetect_hom)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SystemDef.x + 198, SystemDef.y + 18, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SystemDef.x + 190, SystemDef.y + 18, "OFF");
        V_ClearDPTranslation();
    }

    if(replace_missing)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SystemDef.x + 198, SystemDef.y + 28, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SystemDef.x + 190, SystemDef.y + 28, "OFF");
        V_ClearDPTranslation();
    }
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
    M_DarkBackground();

    if(fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatch (67, 15, 0, W_CacheLumpName(DEH_String("M_T_DSET"), PU_CACHE));
    else
        V_DrawPatch (67, 15, 0, W_CacheLumpName(DEH_String("M_DBGSET"), PU_CACHE));

    if(coordinates_info)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(DebugDef.x + 177, DebugDef.y - 2, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(DebugDef.x + 169, DebugDef.y - 2, "OFF");
        V_ClearDPTranslation();
    }

    if(version_info)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(DebugDef.x + 177, DebugDef.y + 8, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(DebugDef.x + 169, DebugDef.y + 8, "OFF");
        V_ClearDPTranslation();
    }

    if(sound_info)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(DebugDef.x + 177, DebugDef.y + 18, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(DebugDef.x + 169, DebugDef.y + 18, "OFF");
        V_ClearDPTranslation();
    }

    if(memory_usage)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(DebugDef.x + 177, DebugDef.y + 28, "ON");
        V_ClearDPTranslation();
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(DebugDef.x + 169, DebugDef.y + 28, "OFF");
        V_ClearDPTranslation();
    }
}

// jff 2/01/98 kill all monsters
//static void cheat_massacre()
void M_Massacre(int choice)
{
    massacre_cheat_used = true;
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
            ((mobj_t *) currentthinker)->type == MT_SKULL ||
            ((mobj_t *) currentthinker)->type == MT_BETASKULL))
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

    sprintf(massacre_textbuffer, "%d MONSTER%s KILLED",
        killcount, killcount == 1 ? "" : "S");

    players[consoleplayer].message = DEH_String(massacre_textbuffer);

    massacre_cheat_used = false;
}

