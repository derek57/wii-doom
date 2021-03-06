// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
// Copyright(C) 2015 Brad Harding (Shadowed Menu Background)
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


#ifdef SDL2
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "am_map.h"
#include "c_io.h"
#include "d_main.h"
#include "d_deh.h"
#include "doomdef.h"
#include "doomfeatures.h"
#include "doomkeys.h"
#include "doomstat.h"
#include "dstrings.h"
#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_oplmusic.h"
#include "i_sdlmusic.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_tinttab.h"
#include "i_video.h"
#include "m_controls.h"
#include "m_menu.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_local.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "r_local.h"
#include "s_sound.h"

// Data.
#include "sounds.h"

#include "st_stuff.h"
#include "statdump.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_merge.h"
#include "w_wad.h"
#include "wii-doom.h"
#include "z_zone.h"

#ifdef WII
#include <wiiuse/wpad.h>
#endif


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

#define MOUSE_LEFTBUTTON                    1
#define MOUSE_RIGHTBUTTON                   2


static void (*messageRoutine)(int response);


static const char *credits[] =
{
    "",
    "",
    "",
    "*WII-/LINUX-DOOM WITH BOOM (MBF) SUPPORT",
    "",
    "*PROGRAMMING / PORTING",
    "RONALD LASMANOWICZ ('DEREK57'/'NITR8')",
    "",
    "#--------------------------------------",
    "",
    "+DOOM / DOOM 2 - HELL ON EARTH",
    "",
    "+(ID SOFTWARE)",
    "+WWW.IDSOFTWARE.COM",
    "",
    "*PROGRAMMING",
    "JOHN CARMACK",
    "JOHN ROMERO",
    "DAVE TAYLOR",
    "",
    "*ARTISTS",
    "ADRIAN CARMACK",
    "KEVIN CLOUD",
    "",
    "*LEVEL DESIGN",
    "TIM WILLITS",
    "AMERICAN MCGEE",
    "SANDY PETERSEN",
    "SHAWN GREEN",
    "JOHN ROMERO",
    "JOHN ANDERSON",
    "",
    "*BIZ",
    "JAY WILBUR",
    "",
    "*SPECIAL THANKS",
    "GREGOR PUNCHATZ",
    "BERND KREIMEIER",
    "",
    "#--------------------------------------",
    "",
    "+(RAVEN SOFTWARE)",
    "+WWW.RAVENSOFTWARE.COM",
    "",
    "*PROGRAMMING",
    "BEN GOKEY",
    "CHRIS RHINEHEART",
    "",
    "#--------------------------------------",
    "",
    "+(ROGUE ENTERTAINMENT, INC.)",
    "+WWW.ROGUE-ENT.COM",
    "",
    "*PROGRAMMING",
    "JAMES MONROE",
    "PETER MACK",
    "",
    "#--------------------------------------",
    "",
    "+(TEAM ETERNITY)",
    "+WWW.DOOMWORLD.COM/ETERNITY",
    "",
    "*PROGRAMMING",
    "JAMES 'QUASAR' HALEY",
    "STEPHEN 'SOM' MCGRANAHAN",
    "CHARLES GUNYON",
    "DAVID HILL",
    "",
    "#--------------------------------------",
    "",
    "+(PRBOOM)",
    "+PRBOOM.SOURCEFORGE.NET",
    "",
    "*PROGRAMMING",
    "FLORIAN 'PROFF' SCHULZE",
    "COLIN 'CPH' PHIPPS",
    "ROB 'RJY' YOUNG",
    "ANDREY BUDKO",
    "",
    "#--------------------------------------",
    "",
    "+(TEAMTNT)",
    "+WWW.TEAMTNT.COM",
    "GREGG 'GRANNY' ANDERSON",
    "DAVE ARMSTRONG",
    "ANDRE ''HARKLE' ARSENAULT",
    "NICK 'NICK_PDOX' BAKER",
    "J.C. 'SAILOR SCOUT' BENGSTON",
    "BILL 'DSQUID' BESSETTE",
    "DAVE BRACHMAN",
    "MIKE BRISTOL",
    "CHRIS 'BONESBRO' BROWN",
    "DAVID BRUNI",
    "CHRIS BUTEAU",
    "DEREK 'NFSFREAK' CADWELL",
    "THOMAS ELLIOT 'MYSCHA' CANNON",
    "ROSS CARLSON",
    "DARIO 'NUMBER 6' CASALI",
    "MILO CASALI",
    "RICK 'WILDMAN' CLARK",
    "CHRIS COULEUR",
    "CHARLES 'MANGE' COX",
    "BILLY 'PORKYPIG' DANIEL",
    "JESPER 'JEDA' DANIELSEN",
    "PAUL 'MADUIN' DEBRUYNE",
    "JIM 'DERF' DETHLEFSEN",
    "YONATAN DONNER",
    "ANDREW 'SLASHWHO' DOWSWELL",
    "JEREMY DOYLE",
    "JONATHAN 'BIZ' EL-BIZRI",
    "JIM 'H2H' ELSON",
    "BOB 'ODESSA' EVANS",
    "THOMAS 'XENEX' EVANS",
    "PAUL 'MOE' FLESCHUTE",
    "JIM FLYNN",
    "JEFFREY GALINOVSKY",
    "DAVID 'BLITZR4' GEVERT",
    "STEVE 'SHRIKER' GILL",
    "GARY GOSSON",
    "TOM 'TIMON' GRIEVE",
    "ANDREW 'HUNGRY DONNER' GRIFFITHS",
    "STAN 'SKULLBANE' GULA",
    "TY HALDERMAN",
    "DALE 'CADAVER' HARRIS",
    "MARK 'HATTY' HATTON",
    "DAVID 'MENTZER' HILL",
    "DONALD R. 'DON' HOWARD",
    "STEFFEN 'ADDICT' ITTERHEIM",
    "GREGORY 'JAX-N' JACKSON",
    "RICHARD 'STYX' JASPARS",
    "MATTIAS JOHANNSON",
    "DEAN JOHNSON",
    "JAMES 'JJ' JOPLIN",
    "JIM 'THE PROF' KENNEDY",
    "BRIAN 'THE KID' KIDBY",
    "LEE KILLOUGH",
    "BOB KIRCHER",
    "MARK KLEM",
    "SVERRE KVERNMO",
    "ADAM LANDEFELD",
    "BRUCE LEWIS",
    "CHARLES LI",
    "WAYNE 'OPIUM JOE' LOUDON",
    "JIM 'SYMBOL' LOWELL",
    "JUSTIN MADIGAN",
    "ANDRE MAJOREL",
    "KIM 'MUTATOR' MALDE",
    "MIKE 'KRUSTY' MARCOTTE",
    "JOSH MARTEL",
    "ANDREW 'FLATLINE' MARTIN",
    "PAUL 'NOWOTNY' MAURONE",
    "STEVE MCCREA",
    "JOHN 'MISCHIEF' MINADEO",
    "LISA 'PUP' MOORE",
    "TOM 'PARADOX' MUSTAINE",
    "RICH 'WEEDS' NAGEL",
    "STEVE 'FUNKYMONK' NOONAN",
    "DAVID NORDLUND",
    "DRAKE 'NUMENA' O'BRIEN",
    "ROBIN PATENALL",
    "BOBBY 'XCALIBUR' PAVLOCK",
    "MICHAEL 'CODEX' PEARCE",
    "RAND PHARES",
    "STEVEN PHARES",
    "TOMMIE 'FATAL' QUICK",
    "KEITH REID",
    "ROGER RITENOUR",
    "ERIC JAMES 'RICROB' ROBERTS",
    "CASEY ROBINSON",
    "ADAM ROSS",
    "MIKE 'GRIPP' RUETE",
    "JANI 'SIR ROBIN' SAKSA",
    "COLE 'MANCER' SAVAGE",
    "PAUL SCHMITZ",
    "FLORIAN 'PROFF' SCHULZE",
    "RANDY 'SCREAMING IN DIGITAL' SEACAT",
    "DAVID 'TOLWYN' SHAW",
    "JIMMY 'EVIL GENIUS' SIEBEN",
    "L.A. 'EVIL GENIUS' SIEBEN",
    "KEN 'ENIGMA' SIMPSON",
    "EUGENE 'ED' SMOZHEVSKY",
    "MARK 'KRAM LLENS' SNELL",
    "ANTHONY 'SWEDISH FISH' SOTO",
    "HAROLD 'SWAFF' SWAFFIELD",
    "ROBERT 'BOBCAT' TAYLOR",
    "PETER 'KNIGGIT' TOMASELLI",
    "PAUL 'STENGER' TURNBULL",
    "JEROMY 'MANNA' VISSER",
    "JEREMY 'IRON LICH' WAGNER",
    "JOHN 'SINGLE MALT' WAKELIN",
    "DIETMAR 'DIA' WESTERTEICHER",
    "BILL WHITAKER",
    "DAVID 'HAKX' WOOD",
    "",
    "#--------------------------------------",
    "",
    "+(ZDOOM)",
    "+WWW.ZDOOM.ORG",
    "",
    "*PROGRAMMING",
    "RANDY HEIT",
    "",
    "#--------------------------------------",
    "",
    "+(DOOM LEGACY)",
    "+DOOMLEGACY.SOURCEFORGE.NET",
    "",
    "*PROGRAMMING",
    "BORIS PEREIRA",
    "FABRICE 'FAB' DENIS",
    "THIERRY 'HURDLER' VAN ELSUWE",
    "STEPHEN 'SOM' MCGRANAHAN",
    "STEPHANE DIERICKX",
    "ROBERT BAUML",
    "BELL KIN",
    "",
    "#--------------------------------------",
    "",
    "+(CHOCOLATE DOOM)",
    "+WWW.CHOCOLATE-DOOM.ORG",
    "",
    "*PROGRAMMING",
    "SIMON 'FRAGGLE' HOWARD",
    "JAMES 'QUASAR' HALEY",
    "SAMUEL 'KAISER' VILLARREAL",
    "",
    "#--------------------------------------",
    "",
    "+(CRISPY DOOM)",
    "+FABIANGREFFRATH.GITHUB.IO/CRISPY-DOOM",
    "",
    "*PROGRAMMING",
    "FABIAN GREFFRATH",
    "",
    "#--------------------------------------",
    "",
    "+(DOOM RETRO)",
    "+WWW.DOOMRETRO.COM",
    "",
    "*PROGRAMMING",
    "BRAD HARDING",
    "",
    "#--------------------------------------",
    "",
    "*ADDITIONAL THANKS",
    "*(AS THEY CONTRIBUTED THEIR WORK TO THE DOOM RETRO PORT)",
    "",
    "ALEX MAYFIELD",
    "",
    "ALEXANDRE-XAVIER LABONTE-LAMOUREUX",
    "('AXDOOMER')",
    "",
    "ALEXEY LYSIUK ('ALEXEY.LYSIUK')",
    "ALUN BESTOR ('VIGGLES')",
    "ANDREW STINE ('LINGUICA')",
    "'ANOTHERLIFE'/'VGA'",
    "'ARNEOLAVHAL'",
    "'BREWTAL_LEGEND'",
    "'DA WERECAT'",
    "DANI VENTAS",
    "DARREN MASON",
    "'HOODIE'",
    "IAIN MACFARLANE",
    "JEFF DOGGETT",
    "'JEWELLDS'",
    "JON KRAZOV",
    "JONATHAN BERGERON ('LAZYLAZURUS')",
    "JONATHAN DOWLAND ('JMTD')",
    "'KB1'",
    "'L3GEND'",
    "LUKE JONES ('LUKE-NUKEM')",
    "MIKE SWANSON ('CHUNGY')",
    "'NOXAT'",
    "'RYAN-SG'",
    "'SGT DOPEY'",
    "CLAUDE FREEMAN ('CONSIGNO'/'SNEAKERNETS')",
    "'THELONERD'",
    "'VESPERAS'",
    "CHARLES GUNYON",
    "IOAN CHERA",
    "'DR. SEAN' LEONARD",
    "SAM LANTINGA ET AL.",
    "SAM LANTINGA",
    "STEPHANE PETER",
    "RYAN GORDON",
    "CHI HOANG",
    "",
    "*ADDITIONAL SUPPORT",
    "",
    "*SOUND CODE (DOS VERSION)",
    "PAUL RADEK",
    "",
    "#(C)1994-2002 ID SOFTWARE, INC.",
    "#ALL RIGHTS RESERVED. PUBLISHED",
    "#AND DISTRIBUTED BY ACTIVISION,",
    "#INC. AND IT'S AFFILIATES UNDER",
    "#LICENSE. DOOM, DOOM II, THE ID",
    "#SOFTWARE NAME AND THE ID LOGO",
    "#ARE EITHER REGISTERED TRADEMARKS",
    "#OR TRADEMARKS OF ID SOFTWARE, INC.",
    "#IN THE UNITED STATES AND/OR OTHER",
    "#COUNTRIES. ACTIVISION(R) IS A",
    "#REGISTERED TRADEMARK OF ACTIVISION,",
    "#INC. AND IT'S AFFILIATES. ALL OTHER",
    "#TRADEMARKS AND TRADE NAMES ARE",
    "#PROPERTIES OF THEIR RESPECTIVE OWNERS.",
    "",
    "",
    "THANKS FOR PLAYING THIS PORT",
    0

};

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

char gammamsg[31][40] =
{
    GAMMALVL0,
    GAMMALVL1,
    GAMMALVL2,
    GAMMALVL3,
    GAMMALVL4,
    GAMMALVL5,
    GAMMALVL6,
    GAMMALVL7,
    GAMMALVL8,
    GAMMALVL9,

    GAMMALVL10,
    GAMMALVL11,
    GAMMALVL12,
    GAMMALVL13,
    GAMMALVL14,
    GAMMALVL15,
    GAMMALVL16,
    GAMMALVL17,
    GAMMALVL18,
    GAMMALVL19,

    GAMMALVL20,
    GAMMALVL21,
    GAMMALVL22,
    GAMMALVL23,
    GAMMALVL24,
    GAMMALVL25,
    GAMMALVL26,
    GAMMALVL27,
    GAMMALVL28,
    GAMMALVL29,

    GAMMALVL30
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

char *stupidtable2[] =
{
    "1","2","3","4","5",
    "6","7","8","9","0"
};

char *Key2String(int ch)
{
    // S.A.: return "[" or "]" or "\"" doesn't work
    // because there are no lumps for these chars,
    // therefore we have to go with "RIGHT BRACKET"
    // and similar for much punctuation.  Probably
    // won't work with international keyboards and
    // dead keys, either.
    switch (ch)
    {
#ifdef WII
        case CLASSIC_CONTROLLER_UP:
            return "UP ARROW";

        case CLASSIC_CONTROLLER_DOWN:
            return "DOWN ARROW";

        case CLASSIC_CONTROLLER_LEFT:
            return "LEFT ARROW";

        case CLASSIC_CONTROLLER_RIGHT:
            return "RIGHT ARROW";

        case CLASSIC_CONTROLLER_MINUS:
            return "MINUS";

        case CLASSIC_CONTROLLER_PLUS:
            return "PLUS";

        case CLASSIC_CONTROLLER_HOME:
            return "HOME";

        case CLASSIC_CONTROLLER_A:
            return "A";

        case CLASSIC_CONTROLLER_B:
            return "B";

        case CLASSIC_CONTROLLER_X:
            return "X";

        case CLASSIC_CONTROLLER_Y:
            return "Y";

        case CLASSIC_CONTROLLER_ZL:
            return "ZL";

        case CLASSIC_CONTROLLER_ZR:
            return "ZR";

        case CLASSIC_CONTROLLER_L:
            return "LEFT TRIGGER";

        case CLASSIC_CONTROLLER_R:
            return "RIGHT TRIGGER";

        case CONTROLLER_1:
            return "1";

        case CONTROLLER_2:
            return "2";
#else
        case KEYP_MULTIPLY:
            return "*";

        case KEY_PRTSCR:
            return "PRINT SCREEN";

        case KEY_SCRLCK:
            return "SCREENLOCK";

        case KEY_NUMLOCK:
            return "NUMLOCK";

        case KEY_CAPSLOCK:
            return "CAPSLOCK";

        case KEY_LEFTBRACKET:
            return "LEFT BRACKET";

        case KEY_RIGHTBRACKET:
            return "RIGHT BRACKET";

#ifndef SDL2
        case KEY_BACKQUOTE:
            return "BACK QUOTE";
#endif

        case KEY_QUOTE:
            return "'";

        case KEY_QUOTEDBL:
            return "DOUBLE QUOTE";

        case KEY_SEMICOLON:
            return ";";

        case KEY_MINUS:
            return "-";

        case KEYP_PLUS:
            return "+";

        case KEY_PERIOD:
            return ".";

        case KEY_COMMA:
            return ",";

        case '/':
            return "/";

        case KEY_BACKSLASH:
            return "BACKSLASH";

        case KEY_TAB:
            return "TAB";

        case KEY_EQUALS:
            return "=";

        case KEY_ESCAPE:
            return "ESCAPE";

        case KEY_RIGHTARROW:
            return "RIGHT ARROW";

        case KEY_LEFTARROW:
            return "LEFT ARROW";

        case KEY_DOWNARROW:
            return "DOWN ARROW";

        case KEY_UPARROW:
            return "UP ARROW";

        case KEY_ENTER:
            return "ENTER";

        case KEY_PGUP:
            return "PAGE UP";

        case KEY_PGDN:
            return "PAGE DOWN";

        case KEY_INS:
            return "INSERT";

        case KEY_HOME:
            return "HOME";

        case KEY_END:
            return "END";

        case KEY_DEL:
            return "DELETE";

        case KEY_F12:
            return "F12";

        case KEY_TILDE:
            return "TILDE";

        case ' ':
            return "SPACE";

        case KEY_RSHIFT:
            return "SHIFT";

        case KEY_RALT:
            return "ALT";

        case KEY_RCTRL:
            return "CTRL";

        case '1':
            return "1";

        case '2':
            return "2";

        case '3':
            return "3";

        case '4':
            return "4";

        case '5':
            return "5";

        case '6':
            return "6";

        case '7':
            return "7";

        case '8':
            return "8";

        case '9':
            return "9";

        case '0':
            return "0";

        case KEY_F1:
            return "F1";

        case KEY_F2:
            return "F2";

        case KEY_F3:
            return "F3";

        case KEY_F4:
            return "F4";

        case KEY_F5:
            return "F5";

        case KEY_F6:
            return "F6";

        case KEY_F7:
            return "F7";

        case KEY_F8:
            return "F8";

        case KEY_F9:
            return "F9";

        case KEY_F10:
            return "F10";

        case KEY_F11:
            return "F11";
/*
        // FIXME: NOT YET WORKING (MOUSE BINDINGS)
        case 0x01:
            return "MOUSE BTN LEFT";

        case 0x02:
            return "MOUSE BTN RIGHT";

        case 0x04:
            return "MOUSE BTN MIDDLE";
*/
#endif
    }

    // Handle letter keys
    // S.A.: could also be done with toupper
    if (ch >= 'a' && ch <= 'z')
        return stupidtable[(ch - 'a')];

#ifndef WII
    else if (ch >= '1' && ch <= '0')
        return stupidtable2[(ch - '1')];
    else
        // Everything else
        return "?";
#endif

    // Everything else
    return "?";
}

char                       savegamestrings[10][SAVESTRINGSIZE];
char                       endstring[160];
char                       coordinates_ang_textbuffer[20];
char                       coordinates_x_textbuffer[20];
char                       coordinates_y_textbuffer[20];
char                       coordinates_z_textbuffer[20];
char                       massacre_textbuffer[30];
char                       flight_counter[10];
char                       bloodsplats_buffer[10];
char                       backgroundcolor_buffer[10];
char                       stillbob_buffer[10];
char                       movebob_buffer[10];
char                       string_back[5];
char                       string_grid[5];
char                       string_wall[5];
char                       string_fchg[5];
char                       string_cchg[5];
char                       string_clsd[5];
char                       string_rkey[5];
char                       string_bkey[5];
char                       string_ykey[5];
char                       string_rdor[5];
char                       string_bdor[5];
char                       string_ydor[5];
char                       string_tele[5];
char                       string_secr[5];
char                       string_exit[5];
char                       string_unsn[5];
char                       string_flat[5];
char                       string_sprt[5];
char                       string_item[5];
char                       string_enemy[5];
char                       string_hair[5];
char                       string_sngl[5];
char                       string_plyr[5];
char                       string_mapgridsize[5];
char                       tempstring[80];

// old save description before edit
char                       saveOldString[SAVESTRINGSIZE];  

// ...and here is the message string!
char                       *messageString;

// graphic name of skulls
// warning: initializer-string for array of chars is too long
char                       *skullNameSmall[2] = {
    "M_SKULL3",
    "M_SKULL4"
};

// 1 = message to be printed
int                        messageToPrint;

// message x & y
int                        messx;
int                        messy;
int                        messageLastMenuActive;

// we are going to be entering a savegame string
int                        saveStringEnter;              

// which slot to save in
int                        saveSlot;

// which char we're editing
int                        saveCharIndex;

int                        map = 1;
int                        cheeting;
int                        coordinates_info = 0;
int                        version_info = 0;

#ifdef WII
int                        key_controls_start_in_cfg_at_pos = 181;
int                        key_controls_end_in_cfg_at_pos = 195;
#else
int                        key_controls_start_in_cfg_at_pos = 180;
int                        key_controls_end_in_cfg_at_pos = 196;
#endif

int                        tracknum = 1;
int                        epi = 1;
int                        repi = 1;
int                        rmap = 1;
int                        rskill = 0;
int                        warped = 0;
int                        faketracknum = 1;
int                        mp_skill = 4;
int                        warpepi = 2;
int                        warplev = 2;
int                        height;
int                        expansion = 0;
int                        oldscreenblocks;
int                        oldscreenSize;
int                        substdumpwait;
int                        condumpwait;
int                        memdumpwait;
int                        statdumpwait;
int                        restartsongwait;
int                        printdirwait;

// -1 = no quicksave slot picked!
int                        quickSaveSlot;

float                      r_gamma = 0.75;

//
// MENU TYPEDEFS
//

// menu item skull is on
short                      itemOn;

// skull animation counter
short                      skullAnimCounter;

// which skull to draw
short                      whichSkull;

// timed message = no input from user
dboolean                   messageNeedsInput;
dboolean                   inhelpscreens;
dboolean                   menuactive;
dboolean                   fake = false;
dboolean                   mus_cheat_used = false;
dboolean                   got_invisibility = false;
dboolean                   got_radiation_suit = false;
dboolean                   got_berserk = false;
dboolean                   got_invulnerability = false;
dboolean                   got_map = false;
dboolean                   got_light_amp = false;
dboolean                   got_all = false;
dboolean                   noclip_on;
dboolean                   massacre_cheat_used;
dboolean                   blurred = false;
dboolean                   long_tics = false;
dboolean                   restart_song = false;
dboolean                   dump_subst = false;

// current menudef
menu_t                     *currentMenu;                          

byte                       *firescreen;
byte                       *tempscreen1;
byte                       *blurscreen1;

static char                xc_textbuffer[10];
static char                yc_textbuffer[10];
static char                xfac_textbuffer[10];
static char                yfac_textbuffer[10];
static char                pitch_textbuffer[10];
static char                offset_textbuffer[10];
static char                width_textbuffer[10];
static char                height_textbuffer[10];

static dboolean            askforkey = false;
static dboolean            opldev;
static dboolean            draw_ended;

// SPECIAL MENU FUNCTIONS (ITEMCOUNT)
static int                 FirstKey = 0;

static int                 keyaskedfor;
static int                 firewidth = 64;
static int                 fireheight = 79;
static int                 firepitch = 64;
static int                 fireoffset = 6;
static int                 firexfac = 2;
static int                 fireyfac = 2;
static int                 firexc = 0;
static int                 fireyc = 39;
static int                 credits_start_time;
static int                 mobj_tics;

static state_t             *mobj_state;


extern char                *d_lowpixelsize;

extern int                 cheat_musnum;
extern int                 cheating;
extern int                 dont_show;
extern int                 pagetic;

// KEY BINDINGS
extern default_t           doom_defaults_list[];

// in heads-up code
extern dboolean            chat_on;

extern dboolean            dots_enabled;
extern dboolean            message_dontfuckwithme;
extern dboolean            BorderNeedRefresh;
extern dboolean            sendpause;
extern dboolean            secret_1;
extern dboolean            secret_2;
extern dboolean            done;
extern dboolean            skippsprinterp;
extern dboolean            longtics;
extern dboolean            mus_cheated;
extern dboolean            wipe;
extern dboolean            splashscreen;
extern dboolean            must_recreate_blockmap;

extern short               songlist[148];

//extern char                *demoname;

extern void                A_PainDie(mobj_t *actor);


//
// PROTOTYPES
//
static void M_NewGame(int choice);
static void M_Episode(int choice);
static void M_ChooseSkill(int choice);
static void M_LoadGame(int choice);
static void M_SaveGame(int choice);
static void M_Options(int choice);
static void M_EndGame(int choice);
static void M_ReadThis(int choice);
static void M_ReadThis2(int choice);
static void M_Credits(int choice);
static void M_ChangeMessages(int choice);
static void M_ChangeSensitivity(int choice);
static void M_MouseWalk(int choice);
static void M_WalkingSpeed(int choice);
static void M_TurningSpeed(int choice);
static void M_StrafingSpeed(int choice);
static void M_GeneralSound(int choice);
static void M_SfxVol(int choice);
static void M_MusicVol(int choice);
static void M_ChangeDetail(int choice);
static void M_Translucency(int choice);
static void M_ColoredBloodA(int choice);
static void M_ColoredBloodB(int choice);
static void M_BloodsplatsAmount(int choice);
static void M_WipeType(int choice);
static void M_UncappedFramerate(int choice);
static void M_Screenshots(int choice);
static void M_Background(int choice);
static void M_BackgroundColor(int choice);
static void M_FontShadow(int choice);
static void M_DiskIcon(int choice);
static void M_FixWiggle(int choice);
static void M_RemoveSlimeTrails(int choice);
static void M_IconType(int choice);
static void M_SizeDisplay(int choice);
static void M_Sound(int choice);
static void M_FinishReadThis(int choice);
static void M_LoadSelect(int choice);
static void M_SaveSelect(int choice);
static void M_ReadSaveStrings(void);
static void M_DrawMainMenu(void);
static void M_DrawReadThis1(void);
static void M_DrawReadThis2(void);
static void M_DrawCredits(void);
static void M_DrawNewGame(void);
static void M_DrawEpisode(void);
static void M_DrawOptions(void);
static void M_DrawSound(void);
static void M_DrawLoad(void);
static void M_DrawSave(void);
static void M_DrawSaveLoadBorder(int x, int y);
static void M_SetupNextMenu(menu_t *menudef);
static void M_DrawThermoSmall(int x, int y, int thermWidth, int thermDot);
static void M_StartMessage(char *string, void *routine, dboolean input);
static void M_MusicType(int choice);
static void M_SoundType(int choice);
static void M_SoundOutput(int choice);
static void M_SoundPitch(int choice);
static void M_RandomMusic(int choice);
static void M_RestartSong(int choice);
static void M_OPLDev(int choice);
static void M_SoundChannels(int choice);
static void M_GameFiles(int choice);
static void M_ChangeGamma(int choice);
static void M_Freelook(int choice);
static void M_FreelookSpeed(int choice);
static void M_KeyBindingsClearControls(int ch);
static void M_KeyBindingsClearAll(int choice);
static void M_KeyBindingsReset(int choice);
static void M_KeyBindingsSetKey(int choice);
static void M_KeyBindings(int choice);
static void M_FPS(int choice);
static void M_DisplayTicker(int choice);
static void M_Coordinates(int choice);
static void M_Timer(int choice);
static void M_Authors(int choice);
static void M_StatusMap(int choice);
static void M_MapName(int choice);
static void M_MapSecretAfter(int choice);
static void M_Version(int choice);
static void M_SoundInfo(int choice);
static void M_HUD(int choice);
static void M_Autoload(int choice);
static void M_Autosave(int choice);
static void M_DrawSplash(int choice);
static void M_Stillbob(int choice);
static void M_Movebob(int choice);
static void M_Flip(int choice);
static void M_MapColor_Back(int choice);
static void M_MapColor_Grid(int choice);
static void M_MapColor_Wall(int choice);
static void M_MapColor_FCHG(int choice);
static void M_MapColor_CCHG(int choice);
static void M_MapColor_CLSD(int choice);
static void M_MapColor_RKey(int choice);
static void M_MapColor_BKey(int choice);
static void M_MapColor_YKey(int choice);
static void M_MapColor_RDoor(int choice);
static void M_MapColor_BDoor(int choice);
static void M_MapColor_YDoor(int choice);
static void M_MapColor_Teleport(int choice);
static void M_MapColor_Secret(int choice);
static void M_MapColor_Exit(int choice);
static void M_MapColor_Unseen(int choice);
static void M_MapColor_Flat(int choice);
static void M_MapColor_Sprite(int choice);
static void M_MapColor_Item(int choice);
static void M_MapColor_Enemy(int choice);
static void M_MapColor_Crosshair(int choice);
static void M_MapColor_Player(int choice);
static void M_MapColor_Multiplayer(int choice);
static void M_MapColor_Standard_DOSDoom(int choice);
static void M_MapColor_Standard_PRBoom(int choice);
static void M_MapGrid(int choice);
static void M_MapGridSize(int choice);
static void M_WeaponChange(int choice);
static void M_AimingHelp(int choice);
static void M_MapRotation(int choice);
static void M_FollowMode(int choice);
static void M_Statistics(int choice);
static void M_Crosshair(int choice);
static void M_Jumping(int choice);
static void M_WeaponRecoil(int choice);
static void M_PlayerThrust(int choice);
static void M_Particles(int choice);
static void M_BloodType(int choice);
static void M_BulletType(int choice);
static void M_TeleportType(int choice);
static void M_BFGCloud(int choice);
static void M_BFGExplosions(int choice);
static void M_RocketParticles(int choice);
static void M_RocketExplosions(int choice);
static void M_SpawnFlies(int choice);
static void M_DripBlood(int choice);
static void M_ParticleSounds(int choice);
static void M_TeleportGlitter(int choice);
static void M_RespawnMonsters(int choice);
static void M_FastMonsters(int choice);
static void M_Autoaim(int choice);
static void M_MaxGore(int choice);
static void M_Footstep(int choice);
static void M_Footclip(int choice);
static void M_Splash(int choice);
static void M_Swirl(int choice);
static void M_Corpses(int choice);
static void M_Secrets(int choice);
static void M_Trails(int choice);
static void M_ChaingunTics(int choice);
static void M_FallingDamage(int choice);
static void M_InfiniteAmmo(int choice);
static void M_GoreAmount(int choice);
static void M_Shadows(int choice);
static void M_Offsets(int choice);
static void M_Telefrag(int choice);
static void M_Doorstuck(int choice);
static void M_ResurrectGhosts(int choice);
static void M_LimitedGhosts(int choice);
static void M_BlockSkulls(int choice);
static void M_BlazingDoors(int choice);
static void M_GodAbsolute(int choice);
static void M_Floor(int choice);
static void M_Clipping(int choice);
static void M_Model(int choice);
static void M_BossDeath(int choice);
static void M_Bounce(int choice);
static void M_Masked(int choice);
static void M_Quirk(int choice);
static void M_Ouch(int choice);
static void M_Textures(int choice);
static void M_FixMapErrors(int choice);
static void M_AltLighting(int choice);
static void M_Infighting(int choice);
static void M_LastEnemy(int choice);
static void M_Float(int choice);
static void M_Animate(int choice);
static void M_CrushSound(int choice);
static void M_NoNoise(int choice);
static void M_NudgeCorpses(int choice);
static void M_Slide(int choice);
static void M_Smearblood(int choice);
static void M_ColoredCorpses(int choice);
static void M_LowHealth(int choice);
static void M_CenterWeapon(int choice);
static void M_EjectCasings(int choice);
static void M_EndoomScreen(int choice);
static void M_NoMonsters(int choice);
static void M_AutomapOverlay(int choice);
static void M_Screen(int choice);
static void M_HOMDetector(int choice);
static void M_MemoryUsage(int choice);
static void M_ConDump(int choice);
static void M_MemDump(int choice);
static void M_StatDump(int choice);
static void M_PrintDir(int choice);
static void M_ReplaceMissing(int choice);
static void M_Precache(int choice);
static void M_Controls(int choice);
static void M_System(int choice);
static void M_Automap(int choice);
static void M_Game(int choice);
static void M_Game2(int choice);
static void M_Game3(int choice);
static void M_Game4(int choice);
static void M_Game5(int choice);
static void M_Game6(int choice);
static void M_Game7(int choice);
static void M_Game8(int choice);
static void M_OptSet(int choice);
static void M_DefSet(int choice);
static void M_Expansion(int choice);
static void M_Debug(int choice);
static void M_Test(int choice);
static void M_DrawFilesMenu(void);
static void M_DrawScreen(void);
static void M_DrawKeyBindings(void);
static void M_DrawControls(void);
static void M_DrawSystem(void);
static void M_DrawAutomap(void);
static void M_DrawGame1(void);
static void M_DrawGame2(void);
static void M_DrawGame3(void);
static void M_DrawGame4(void);
static void M_DrawGame5(void);
static void M_DrawGame6(void);
static void M_DrawGame7(void);
static void M_DrawGame8(void);
static void M_DrawDebug(void);
static void M_DrawTest(void);
static void M_XC(int choice);
static void M_YC(int choice);
static void M_XFac(int choice);
static void M_YFac(int choice);
static void M_Pitch(int choice);
static void M_Offset(int choice);
static void M_Width(int choice);
static void M_Height(int choice);
static void M_WidthHeightPitch(int choice);
/*
static void M_Record(int choice);
static void M_RMap(int choice);
static void M_RSkill(int choice);
static void M_RecordLong(int choice);
static void M_StartRecord(int choice);
static void M_DrawRecord(void);
*/

#ifdef SDL2
static void M_RenderMode(int choice);
static void M_VSync(int choice);
#endif

#ifndef WII
static void M_DumpSubstituteConfig(int choice);
static void M_Samplerate(int choice);
#else
static void M_God(int choice);
static void M_Noclip(int choice);
static void M_Weapons(int choice);
static void M_WeaponsA(int choice);
static void M_WeaponsB(int choice);
static void M_WeaponsC(int choice);
static void M_WeaponsD(int choice);
static void M_WeaponsE(int choice);
static void M_WeaponsF(int choice);
static void M_WeaponsG(int choice);
static void M_WeaponsH(int choice);
static void M_Keys(int choice);
static void M_KeysA(int choice);
static void M_KeysB(int choice);
static void M_KeysC(int choice);
static void M_KeysD(int choice);
static void M_KeysE(int choice);
static void M_KeysF(int choice);
static void M_KeysG(int choice);
static void M_Items(int choice);
static void M_ItemsA(int choice);
static void M_ItemsB(int choice);
static void M_ItemsC(int choice);
static void M_ItemsD(int choice);
static void M_ItemsE(int choice);
static void M_ItemsF(int choice);
static void M_ItemsG(int choice);
static void M_ItemsH(int choice);
static void M_ItemsI(int choice);
static void M_ItemsJ(int choice);
static void M_ItemsK(int choice);
static void M_Topo(int choice);
static void M_Rift(int choice);
static void M_RiftNow(int choice);
static void M_Spin(int choice);
static void M_Armor(int choice);
static void M_ArmorA(int choice);
static void M_ArmorB(int choice);
static void M_ArmorC(int choice);
static void M_Beta(int choice);
static void M_Cheats(int choice);
static void M_Massacre(int choice);
static void M_DrawCheats(void);
static void M_DrawItems(void);
static void M_DrawArmor(void);
static void M_DrawWeapons(void);
static void M_DrawKeys(void);
#endif


//
// DOOM MENU
//
enum
{
    newgame = 0,
    options,
    gamefiles,
    credit,
    readthis,
    quitdoom,
    main_end
} main_e;

static menuitem_t MainGameMenu[]=
{
    {1, "New Game"  , M_NewGame  , 'n'},
    {1, "Options"   , M_Options  , 'o'},
    {1, "Game Files", M_GameFiles, 'f'},
    {1, "Credits"   , M_Credits  , 'c'},
    {1, "Read This!", M_ReadThis , 'r'},
    {1, "Quit Game" , M_QuitDOOM , 'q'}
};

static menu_t  MainDef =
{
    main_end,
    NULL,
    MainGameMenu,
    M_DrawMainMenu,
    122, 74,
    0
};

enum
{
    demo_map_1 = 0,
    demo_map_2,
    demo_map_3,
    demo_quit,
    beta_main_end
} beta_main_e;

static menuitem_t BetaMainGameMenu[]=
{
    {1, "Demo Map 1", M_NewGame , '1'},
    {1, "Demo Map 2", M_NewGame , '2'},
    {1, "Demo Map 3", M_NewGame , '3'},
    {1, "Quit Game" , M_QuitDOOM, 'q'}
};

static menu_t  BetaMainDef =
{
    beta_main_end,
    NULL,
    BetaMainGameMenu,
    M_DrawMainMenu,
    122, 74,
    0
};

enum
{
    loadgame,
    savegame,
    endgame,

#ifdef WII
    cheats,
#endif

//    demos,
    files_end
} files_e;

static menuitem_t FilesMenu[]=
{
    {1, "Load Game"  , M_LoadGame, 'l'},
    {1, "Save Game"  , M_SaveGame, 's'},
    {1, "End Game"   , M_EndGame , 'e'}

#ifdef WII
    ,
    {1, "Cheats"     , M_Cheats  , 'c'}
#endif

/*    ,
    {1, "Record Demo", M_Record  , 'r'}*/
};

static menu_t  FilesDef =
{
    files_end,
    &MainDef,
    FilesMenu,
    M_DrawFilesMenu,
    122, 65,
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

static menuitem_t EpisodeMenu[]=
{
    {1, "Knee-Deep in the Dead", M_Episode, 'k'},
    {1, "The Shores of Hell"   , M_Episode, 't'},
    {1, "Inferno"              , M_Episode, 'i'},
    {1, "Thy Flesh Consumed"   , M_Episode, 't'}
};

menu_t  EpiDef =
{
    // # of menu items
    ep_end,

    // previous menu
    &MainDef,

    // menuitem_t ->
    EpisodeMenu,

    // drawing routine ->
    M_DrawEpisode,

    // x, y
    95, 73,

    // lastOn
    ep1
};

//
// EXPANSION SELECT
//
enum
{
    ex1,
    ex2,
    ex_end
} expansions_e;

static menuitem_t ExpansionMenu[]=
{
    {1, "Hell On Earth"         , M_Expansion, 'k'},
    {1, "No Rest For The Living", M_Expansion, 't'}
};

static menu_t  ExpDef =
{
    ex_end,
    &MainDef,
    ExpansionMenu,
    M_DrawEpisode,
    95, 73,
    ex1
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

static menuitem_t NewGameMenu[]=
{
    {1, "I'm too young to die.", M_ChooseSkill, 'i'},
    {1, "Hey, not too rough."  , M_ChooseSkill, 'h'},
    {1, ""                     , M_ChooseSkill, 'h'},
    {1, "Ultra-Violence."      , M_ChooseSkill, 'u'},
    {1, "Nightmare!"           , M_ChooseSkill, 'n'}
};

static menu_t  NewDef =
{
    newg_end,
    &EpiDef,
    NewGameMenu,
    M_DrawNewGame,
    88, 73,

    // FIXME: REMEMBER LAST SKILL A GAME WAS STARTED???
    hurtme
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
    empty1,
    optimal_settings,
    reset_settings,
    empty2,
    dbg,
    tst,
    opt_end
} options_e;

static menuitem_t OptionsMenu[]=
{
    {1 , "", M_Screen  , 's' },
    {1 , "", M_Controls, 'c' },
    {1 , "", M_Sound   , 'v' },
    {1 , "", M_System  , 'y' },
    {1 , "", M_Game    , 'g' },
    {-1, "", 0         , '\0'},
    {1 , "", M_OptSet  , 'o' },
    {1 , "", M_DefSet  , 'r' },
    {-1, "", 0         , '\0'},
    {1 , "", M_Debug   , 'd' },
    {1 , "", M_Test    , 't' }
};

static menu_t  OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    M_DrawOptions,
    100, 47,
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

static menuitem_t ReadMenu1[] =
{
    {1, "", M_ReadThis2, 0}
};

static menu_t  ReadDef1 =
{
    read1_end,
    &MainDef,
    ReadMenu1,
    M_DrawReadThis1,
    280, 185,
    0
};

enum
{
    rdthsempty2,
    read2_end
} read_e2;

static menuitem_t ReadMenu2[]=
{
    {1, "", M_FinishReadThis, 0}
};

static menu_t  ReadDef2 =
{
    read2_end,
    &ReadDef1,
    ReadMenu2,
    M_DrawReadThis2,
    330, 175,
    0
};

enum
{
    credits_end
} credits_e;

static menuitem_t CreditsMenu[] =
{
};

static menu_t  CreditsDef =
{
    credits_end,
    &MainDef,
    CreditsMenu,
    M_DrawCredits,
    0, 2,
    0
};

#ifdef WII

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

static menuitem_t CheatsMenu[]=
{
    {2 , "GOD MODE"         , M_God     , 'g' },
    {2 , "NOCLIP"           , M_Noclip  , 'n' },
    {1 , ""                 , M_Weapons , 'w' }, 
    {1 , ""                 , M_Keys    , 'k' },
    {1 , ""                 , M_Armor   , 'a' },
    {1 , ""                 , M_Items   , 'i' },
    {2 , "AUTOMAP REVEAL:"  , M_Topo    , 't' },
    {2 , "KILL ALL ENEMIES" , M_Massacre, 'm' },
    {2 , "WARP TO MAP:"     , M_Rift    , 'r' },
    {-1, ""                 , 0         , '\0'},
    {-1, ""                 , 0         , '\0'},
    {2 , "EXECUTE WARPING"  , M_RiftNow , 'e' },
    {-1, ""                 , 0         , '\0'},
    {2 , "PLAY MUSIC TITLE:", M_Spin    , 's' }
};

menu_t  CheatsDef =
{
    cheats_end,
    &FilesDef,
    CheatsMenu,
    M_DrawCheats,
    75, 28,
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

static menuitem_t ItemsMenu[]=
{
    {2 , ""                 , M_ItemsA, '1' },
    {-1, ""                 , 0       , '\0'},
    {2 , ""                 , M_ItemsC, '2' },
    {2 , ""                 , M_ItemsD, '3' },
    {2 , ""                 , M_ItemsE, '4' },
    {2 , "FULL HEALTH (100)", M_ItemsH, '5' },
    {2 , ""                 , M_ItemsI, '6' },
    {2 , ""                 , M_ItemsB, '7' },
    {2 , ""                 , M_ItemsF, '8' },
    {2 , ""                 , M_ItemsG, '9' },
    {2 , ""                 , M_ItemsJ, '0' },
    {2 , ""                 , M_ItemsK, 'f' }
};

static menu_t  ItemsDef =
{
    items_end,
    &CheatsDef,
    ItemsMenu,
    M_DrawItems,
    80, 52,
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

static menuitem_t KeysMenu[]=
{
    {2 , ""                , M_KeysA, '1' },
    {-1, ""                , 0      , '\0'},
    {2 , "BLUE KEYCARD"    , M_KeysB, '2' },
    {2 , "YELLOW KEYCARD"  , M_KeysC, '3' },
    {2 , "RED KEYCARD"     , M_KeysD, '4' },
    {2 , "BLUE SKULL KEY"  , M_KeysE, '5' },
    {2 , "YELLOW SKULL KEY", M_KeysF, '6' },
    {2 , "RED SKULL KEY"   , M_KeysG, '7' }
};

static menu_t  KeysDef =
{
    keys_end,
    &CheatsDef,
    KeysMenu,
    M_DrawKeys,
    80, 57,
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

static menuitem_t WeaponsMenu[]=
{
    {2 , "", M_WeaponsA, '1' },
    {-1, "", 0         , '\0'},
    {2 , "", M_WeaponsG, '2' },
    {2 , "", M_WeaponsB, '3' },
    {2 , "", M_WeaponsC, '4' },
    {2 , "", M_WeaponsD, '5' },
    {2 , "", M_WeaponsE, '6' },
    {2 , "", M_WeaponsF, '7' },
    {2 , "", M_WeaponsH, '8' }
};

static menu_t  WeaponsDef =
{
    weapons_end,
    &CheatsDef,
    WeaponsMenu,
    M_DrawWeapons,
    80, 57,
    0
};

enum
{
    armor1,
    armor_empty,
    armor2,
    armor3,
    armor_end
} armor_e;

static menuitem_t ArmorMenu[]=
{
    {2 , "", M_ArmorA, '1' },
    {-1, "", 0       , '\0'},
    {2 , "", M_ArmorB, '2' },
    {2 , "", M_ArmorC, '3' }
};

static menu_t  ArmorDef =
{
    armor_end,
    &CheatsDef,
    ArmorMenu,
    M_DrawArmor,
    80, 57,
    0
};

#endif

enum
{
    screen_gamma,
    screen_size,
    screen_detail,
    screen_translucency,
    screen_wipe,
    screen_framerate,
    screen_shots,
    screen_background,
    screen_color,
    screen_shadow,
    screen_icon,
    screen_type,

#ifdef SDL2
    screen_render,
    screen_vsync,
#endif

    screen_end
} screen_e;

static menuitem_t ScreenMenu[]=
{
    {2, "Brightness"              , M_ChangeGamma      , 'g'},
    {2, "Screen Size"             , M_SizeDisplay      , 's'},
    {2, "Quality"                 , M_ChangeDetail     , 'q'},
    {2, "Translucency"            , M_Translucency     , 'l'},
    {2, "Wipe Type"               , M_WipeType         , 'w'},
    {2, "Framerate"               , M_UncappedFramerate, 'u'},
    {2, "Screenshot Format"       , M_Screenshots      , 'x'},
    {2, "Menu Background"         , M_Background       , 'b'},
    {2, ""                        , M_BackgroundColor  , 'c'},
    {2, "Menu Font Style"         , M_FontShadow       , 'f'},
    {2, "Show Loading Indicator"  , M_DiskIcon         , 'd'},
    {2, ""                        , M_IconType         , 'i'}

#ifdef SDL2
    ,
    {2, "Render Mode"             , M_RenderMode       , 'r'},
    {2, "Vertical Synchronization", M_VSync            , 'v'}
#endif

};

static menu_t  ScreenDef =
{
    screen_end,
    &OptionsDef,
    ScreenMenu,
    M_DrawScreen,
    60, 25,
    0
};

enum
{
    mousesens,
    turnsens,
    strafesens,
    mousespeed,
    controls_freelook,
    mousesensibility,
    mousewalking,
    controls_keybindings,
    controls_end
} controls_e;

static menuitem_t ControlsMenu[]=
{
    {2, "Walking"      , M_WalkingSpeed     , 'w'},
    {2, "Turning"      , M_TurningSpeed     , 't'},
    {2, "Strafing"     , M_StrafingSpeed    , 's'},
    {2, ""             , M_FreelookSpeed    , 'f'},
    {2, "Freelook Mode", M_Freelook         , 'l'},
    {2, ""             , M_ChangeSensitivity, 'm'},
    {2, ""             , M_MouseWalk        , 'n'},
    {1, ""             , M_KeyBindings      , 'b'}
};

static menu_t  ControlsDef =
{
    controls_end,
    &OptionsDef,
    ControlsMenu,
    M_DrawControls,
    20, 60,
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

#ifndef WII
    keybindings_strafeleft,
    keybindings_straferight,
#endif

    keybindings_clearall,
    keybindings_reset,
    keybindings_end
} keybindings_e;

static menuitem_t KeyBindingsMenu[]=
{
    {5, "FIRE"            , M_KeyBindingsSetKey  , 0 },
    {5, "USE / OPEN"      , M_KeyBindingsSetKey  , 1 },
    {5, "MAIN MENU"       , M_KeyBindingsSetKey  , 2 },
    {5, "WEAPON LEFT"     , M_KeyBindingsSetKey  , 3 },
    {5, "SHOW AUTOMAP"    , M_KeyBindingsSetKey  , 4 },
    {5, "WEAPON RIGHT"    , M_KeyBindingsSetKey  , 5 },
    {5, "AUTOMAP ZOOM IN" , M_KeyBindingsSetKey  , 6 },
    {5, "AUTOMAP ZOOM OUT", M_KeyBindingsSetKey  , 7 },
    {5, "FLY UP"          , M_KeyBindingsSetKey  , 8 },
    {5, "FLY DOWN"        , M_KeyBindingsSetKey  , 9 },
    {5, "JUMP"            , M_KeyBindingsSetKey  ,'j'},
    {5, "RUN"             , M_KeyBindingsSetKey  ,'e'},

#ifdef WII
    {5, "CONSOLE"         , M_KeyBindingsSetKey  ,'c'},
#else
    {5, ""                , M_KeyBindingsSetKey  ,'c'},
#endif

    {5, "SCREENSHOTS"     , M_KeyBindingsSetKey  ,'s'},

#ifndef WII
    {5, "STRAFE LEFT"     , M_KeyBindingsSetKey  ,'l'},
    {5, "STRAFE RIGHT"    , M_KeyBindingsSetKey  ,'r'},
#endif

    {5, ""                , M_KeyBindingsClearAll,'x'},
    {5, ""                , M_KeyBindingsReset   ,'d'}
};

static menu_t  KeyBindingsDef =
{
    keybindings_end,
    &ControlsDef,
    KeyBindingsMenu,
    M_DrawKeyBindings,
    45, 18,
    0
};

enum
{
    system_fps,
    system_ticker,
    system_hom,
    system_replace,
    system_precache,
    system_end
} system_e;

static menuitem_t SystemMenu[]=
{
    {2, "FPS Counter"             , M_FPS           , 'f'},
    {2, "Display Ticker"          , M_DisplayTicker , 't'},
    {2, "Hall of Mirrors Detector", M_HOMDetector   , 'h'},
    {2, "Replace Missing Textures", M_ReplaceMissing, 'r'},
    {2, "Precache Level at Start" , M_Precache      , 'p'}
};

static menu_t  SystemDef =
{
    system_end,
    &OptionsDef,
    SystemMenu,
    M_DrawSystem,
    57, 85,
    0
};

enum
{
    game_mapgrid,
    game_mapgridsize,
    game_maprotation,
    game_followmode,
    game_overlay,
    game_statusmap,
    game_statistics,
    game_timer,
    game_authors,
    game_maptitle,
    game_secrets,
    game_mapcols,
    game_respawn,
    game_game2,
    game_end
} game_e;

static menuitem_t GameMenu[]=
{
    {2, "AUTOMAP GRID"                    , M_MapGrid        , 'g'},
    {2, "AUTOMAP GRID SIZE"               , M_MapGridSize    , 'c'},
    {2, "AUTOMAP ROTATION"                , M_MapRotation    , 'r'},
    {2, "AUTOMAP FOLLOW MODE"             , M_FollowMode     , 'f'},
    {2, "AUTOMAP OVERLAY"                 , M_AutomapOverlay , 'o'},
    {2, "AUTOMAP STATUS BAR"              , M_StatusMap      , 'm'},
    {2, "AUTOMAP STATISTICS"              , M_Statistics     , 's'},
    {2, "AUTOMAP TIMER"                   , M_Timer          , 't'},
    {2, "AUTOMAP AUTHORS"                 , M_Authors        , 'a'},
    {2, "AUTOMAP MAP TITLE"               , M_MapName        , 'n'},
    {2, "ONLY SHOW SECRETS AFTER ENTERING", M_MapSecretAfter , 'h'},
    {1, ""                                , M_Automap        , 'x'},
    {2, "RESPAWN MONSTERS"                , M_RespawnMonsters, 'i'},
    {1, ""                                , M_Game2          , '2'}
};

static menu_t  GameDef =
{
    game_end,
    &OptionsDef,
    GameMenu,
    M_DrawGame1,
    23, 22,
    0
};

enum
{
    game2_monsters,
    game2_fast,
    game2_footstep,
    game2_footclip,
    game2_splash,
    game2_swirl,
    game2_endoom,
    game2_corpses,
    game2_secrets,
    game2_messages,
    game2_tics,
    game2_falling,
    game2_ammo,
    game2_game3,
    game2_end
} game2_e;

static menuitem_t GameMenu2[]=
{
    {2, "NO MONSTERS"                 , M_NoMonsters    , 'm'},
    {2, "FAST MONSTERS"               , M_FastMonsters  , 'e'},
    {2, "PLAYER FOOTSTEPS"            , M_Footstep      , 's'},
    {2, "HERETIC FOOTCLIPS"           , M_Footclip      , 'c'},
    {2, "HERETIC LIQUID SPLASH"       , M_Splash        , 'l'},
    {2, "SWIRLING WATER HACK"         , M_Swirl         , 'w'},

#ifdef WII
    {2, ""                            , M_EndoomScreen  , 'q'},
#else
    {2, "Show Endoom Screen on quit"  , M_EndoomScreen  , 'q'},
#endif

    {2, "RANDOMLY FLIP CORPSES & GUNS", M_Corpses       , 'd'},
    {2, "SHOW REVEALED SECRETS"       , M_Secrets       , 'z'},
    {2, "MESSAGES"                    , M_ChangeMessages, 'm'},
    {2, ""                            , M_ChaingunTics  , 'g'},
    {2, "FALLING DAMAGE"              , M_FallingDamage , 'f'},
    {2, "INFINITE AMMO"               , M_InfiniteAmmo  , 'i'},
    {1, ""                            , M_Game3         , 'n'}
};

static menu_t  GameDef2 =
{
    game2_end,
    &GameDef,
    GameMenu2,
    M_DrawGame2,
    23, 22,
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
    game3_bloodsplats,
    game3_shadows,
    game3_telefrag,
    game3_stuck,
    game3_ressurection,
    game3_limitation,
    game3_game4,
    game3_end
} game3_e;

static menuitem_t GameMenu3[]=
{
    {2, "CROSSHAIR"                           , M_Crosshair        , 'c'},
    {2, "AUTOAIM"                             , M_Autoaim          , 'a'},
    {2, "JUMPING"                             , M_Jumping          , 'j'},
    {2, "MORE BLOOD & GORE"                   , M_MaxGore          , 'o'},
    {2, ""                                    , M_GoreAmount       , 'g'},
    {2, "Enable Colored Blood"                , M_ColoredBloodA    , '1'},
    {2, "Fix Monster Blood"                   , M_ColoredBloodB    , '2'},
    {2, ""                                    , M_BloodsplatsAmount, 'b'},
    {2, "Shadows for Monsters and Items"      , M_Shadows          , 's'},
    {2, "Monsters can Telefrag on MAP30"      , M_Telefrag         , 't'},
    {2, "Monsters stuck on doortracks"        , M_Doorstuck        , 'd'},
    {2, "ARCH-VILE can resurrect ghosts"      , M_ResurrectGhosts  , 'r'},
    {2, "Pain-Elementals have lost soul limit", M_LimitedGhosts    , 'l'},
    {1, ""                                    , M_Game4            , 'n'}
};

static menu_t  GameDef3 =
{
    game3_end,
    &GameDef2,
    GameMenu3,
    M_DrawGame3,
    23, 22,
    0
};

enum
{
    game4_block,
    game4_doors,
    game4_god,
    game4_floor,
    game4_clipping,
    game4_model,
    game4_bossdeath,
    game4_bounce,
    game4_masked,
    game4_sound,
    game4_ouch,
    game4_textures,
    game4_errors,
    game4_game5,
    game4_end
} game4_e;

static menuitem_t GameMenu4[]=
{
    {2, "Lost Souls get stuck behind walls"   , M_BlockSkulls , 'w'},
    {2, "Blazing Doors play double sound"     , M_BlazingDoors, 'd'},
    {2, "God mode isn't absolute"             , M_GodAbsolute , 'i'},
    {2, "Use Doom's floor motion behavior"    , M_Floor       , 'f'},
    {2, "Use Doom's movement clipping code"   , M_Clipping    , 'c'},
    {2, "Use Doom's linedef trigger model"    , M_Model       , 'm'},
    {2, "Emulate pre-Ultimate Boss Death"     , M_BossDeath   , 'd'},
    {2, "Lost souls don't bounce off flats"   , M_Bounce      , 'b'},
    {2, "Two-S. middle textures don't animate", M_Masked      , 'a'},
    {2, "Retain quirks in Doom's sound code"  , M_Quirk       , 's'},
    {2, "Use Doom's buggy ouch face code"     , M_Ouch        , 'o'},
    {2, "Partially Fullbright Textures"       , M_Textures    , 't'},
    {2, "Fix Map Errors"                      , M_FixMapErrors, 'e'},
    {1, ""                                    , M_Game5       , 'n'}
};

static menu_t  GameDef4 =
{
    game4_end,
    &GameDef3,
    GameMenu4,
    M_DrawGame4,
    23, 22,
    0
};

enum
{
    game5_lighting,
    game5_infighting,
    game5_enemy,
    game5_float,
    game5_animate,
    game5_sound,
    game5_noise,
    game5_nudge,
    game5_slide,
    game5_smearblood,
    game5_coloredcorpses,
    game5_health,
    game5_offsets,
    game5_game6,
    game5_end
} game5_e;

static menuitem_t GameMenu5[]=
{
    {2, "Alt. Lighting for Player Sprites"     , M_AltLighting   , 'l'},
    {2, "Allow Monsters Infighting"            , M_Infighting    , 'i'},
    {2, "Monsters Remember last enemy"         , M_LastEnemy     , 'e'},
    {2, "Allow floating items"                 , M_Float         , 'f'},
    {2, "Animate items dropped by monsters"    , M_Animate       , 'a'},
    {2, "Play sound crushing things to gibs"   , M_CrushSound    , 's'},
    {2, "Don't alert enemies when firing fist" , M_NoNoise       , 'n'},
    {2, "Nudge corpses when walking over"      , M_NudgeCorpses  , 'c'},
    {2, "Corpses slide caused by explosions"   , M_Slide         , 'x'},
    {2, ""                                     , M_Smearblood    , 'b'},
    {2, "Randomly colored player corpses"      , M_ColoredCorpses, 'r'},
    {2, "Player walks slower if health < 15%"  , M_LowHealth     , 'h'},
    {2, "Fix Sprite Offsets"                   , M_Offsets       , 'o'},
    {1, ""                                     , M_Game6         , '6'}
};

static menu_t  GameDef5 =
{
    game5_end,
    &GameDef4,
    GameMenu5,
    M_DrawGame5,
    23, 22,
    0
};

enum
{
    game6_centerweapon,
    game6_casings,
    game6_particles,
    game6_bloodparticles,
    game6_bulletparticles,
    game6_teleportparticles,
    game6_bfgcloud,
    game6_bfgexplosions,
    game6_rocketparticles,
    game6_rocketexplosions,
    game6_spawnflies,
    game6_dripblood,
    game6_particlesounds,
    game6_game7,
    game6_end
} game6_e;

static menuitem_t GameMenu6[]=
{
    {2, "Center Weapon when firing", M_CenterWeapon    , 'w'},
    {2, ""                         , M_EjectCasings    , 'e'},
    {2, "Draw Particles"           , M_Particles       , 'a'},
    {2, ""                         , M_BloodType       , 'b'},
    {2, ""                         , M_BulletType      , 'c'},
    {2, ""                         , M_TeleportType    , 'w'},
    {2, ""                         , M_BFGCloud        , 'x'},
    {2, ""                         , M_BFGExplosions   , 'y'},
    {2, ""                         , M_RocketParticles , 'r'},
    {2, ""                         , M_RocketExplosions, 's'},
    {2, ""                         , M_SpawnFlies      , 'f'},
    {2, ""                         , M_DripBlood       , 'd'},
    {2, ""                         , M_ParticleSounds  , 'p'},
    {1, ""                         , M_Game7           , '7'}
};

static menu_t  GameDef6 =
{
    game6_end,
    &GameDef5,
    GameMenu6,
    M_DrawGame6,
    23, 22,
    0
};

enum
{
    game7_smoketrails,
    game7_wiggle,
    game7_slimetrails,
    game7_aimbot,
    game7_thrust,
    game7_teleportglitter,
    game7_weapon,
    game7_recoil,
    game7_hud,
    game7_autoload,
    game7_autosave,
    game7_still,
    game7_move,
    game7_game8,
    game7_end
} game7_e;

static menuitem_t GameMenu7[]=
{
    {2, "ROCKET SMOKE-TRAILS"              , M_Trails           , 'r'},
    {2, "Fix Wiggle Effect"                , M_FixWiggle        , 'w'},
    {2, "Remove Slime Trails"              , M_RemoveSlimeTrails, 's'},
    {2, ""                                 , M_AimingHelp       , 'a'},
    {2, "PLAYER THRUST"                    , M_PlayerThrust     , 'p'},
    {2, "Teleport Landings glitter type"   , M_TeleportGlitter  , 't'},
    {2, ""                                 , M_WeaponChange     , 'g'},
    {2, "WEAPON RECOIL"                    , M_WeaponRecoil     , 'c'},
    {2, "FULLSCREEN HUD"                   , M_HUD              , 'h'},
    {2, "AUTOLOAD FEATURE"                 , M_Autoload         , 'l'},
    {2, "AUTOSAVE FEATURE"                 , M_Autosave         , 'f'},
    {2, "Bob the players view (%)"         , M_Stillbob         , 'b'},
    {2, "Bob the players gun (%)"          , M_Movebob          , 'm'},
    {1, ""                                 , M_Game8            , '8'}
};

static menu_t  GameDef7 =
{
    game7_end,
    &GameDef6,
    GameMenu7,
    M_DrawGame7,
    23, 22,
    0
};

enum
{
    game8_splash,
    game8_flip,
#ifdef WII
    game8_prbeta,
#endif

    game8_end
} game8_e;

static menuitem_t GameMenu8[]=
{
    {2, "Draw Splash Screen at game bootup", M_DrawSplash       , 's'},
    {2, "Flip Levels horizontally"         , M_Flip             , 'n'}
#ifdef WII
    ,
    {2, "PRE-RELEASE BETA MODE"            , M_Beta             , 'b'}
#endif

};

static menu_t  GameDef8 =
{
    game8_end,
    &GameDef7,
    GameMenu8,
    M_DrawGame8,
    23, 22,
    0
};

enum
{
    amap_color_back,
    amap_color_grid,
    amap_color_wall,
    amap_color_fchg,
    amap_color_cchg,
    amap_color_clsd,
    amap_color_rkey,
    amap_color_bkey,
    amap_color_ykey,
    amap_color_rdor,
    amap_color_bdor,
    amap_color_ydor,
    amap_color_tele,
    amap_color_secr,
    amap_color_exit,
    amap_color_unsn,
    amap_color_flat,
    amap_color_sprt,
    amap_color_item,
    amap_color_enemy,
    amap_color_hair,
    amap_color_sngl,
    amap_color_plyr,
    amap_color_reset_dos,
    amap_color_reset_prboom,
    amap_end
} amap_e;

static menuitem_t AutomapMenu[]=
{
    {2, "map Background"                      , M_MapColor_Back            , '\0'},
    {2, "Grid lines"                          , M_MapColor_Grid            , '\0'},
    {2, "normal 1-sided wall"                 , M_MapColor_Wall            , '\0'},
    {2, "line at floor height change"         , M_MapColor_FCHG            , '\0'},
    {2, "line at Ceiling height change"       , M_MapColor_CCHG            , '\0'},
    {2, "line at Sector with floor = ceiling" , M_MapColor_CLSD            , '\0'},
    {2, "Red Key"                             , M_MapColor_RKey            , '\0'},
    {2, "Blue Key"                            , M_MapColor_BKey            , '\0'},
    {2, "Yellow Key"                          , M_MapColor_YKey            , '\0'},
    {2, "Red Door"                            , M_MapColor_RDoor           , '\0'},
    {2, "Blue Door"                           , M_MapColor_BDoor           , '\0'},
    {2, "Yellow Door"                         , M_MapColor_YDoor           , '\0'},
    {2, "Teleporter Line"                     , M_MapColor_Teleport        , '\0'},
    {2, "Secret sector boundary"              , M_MapColor_Secret          , '\0'},
    {2, "Exit line"                           , M_MapColor_Exit            , '\0'},
    {2, "computer map unseen line"            , M_MapColor_Unseen          , '\0'},
    {2, "line with no floor / ceiling changes", M_MapColor_Flat            , '\0'},
    {2, "general sprite"                      , M_MapColor_Sprite          , '\0'},
    {2, "item sprite"                         , M_MapColor_Item            , '\0'},
    {2, "enemy sprite"                        , M_MapColor_Enemy           , '\0'},
    {2, "crosshair"                           , M_MapColor_Crosshair       , '\0'},
    {2, "single player arrow"                 , M_MapColor_Player          , '\0'},
    {2, "player arrows in multiplayer"        , M_MapColor_Multiplayer     , '\0'},
    {2, ""                                    , M_MapColor_Standard_DOSDoom, '\0'},
    {2, ""                                    , M_MapColor_Standard_PRBoom , '\0'}
};

static menu_t  AutomapDef =
{
    amap_end,
    &GameDef,
    AutomapMenu,
    M_DrawAutomap,
    22, 18,
    0
};

enum
{
    debug_coordinates,
    debug_version,
    debug_sound,
    debug_opl,
    debug_memusage,
    debug_restart,
    debug_condump,

#ifdef HEAPDUMP
    debug_memdump,
#endif

    debug_statdump,
    debug_printdir,
    debug_end
} debug_e;

static menuitem_t DebugMenu[]=
{
    {2, "Show Coordinates"               , M_Coordinates, 'c'},
    {2, "Show Version"                   , M_Version    , 'v'},
    {2, "Show Sound Info"                , M_SoundInfo  , 'i'},
    {2, "Show OPL Developer Info"        , M_OPLDev     , 'o'},
    {2, "Show Memory Usage"              , M_MemoryUsage, 'u'},
    {2, "Restart Current MAP-Music Track", M_RestartSong, 'r'},
    {2, "Dump current Console Output"    , M_ConDump    , 'd'},

#ifdef HEAPDUMP
    {2, "Dump Memory"                    , M_MemDump    , 'm'},
#endif

    {2, "Dump Level Statistics"          , M_StatDump   , 's'},
    {2, "Print WAD contents to textfile" , M_PrintDir   , 'p'}
};

static menu_t  DebugDef =
{
    debug_end,
    &OptionsDef,
    DebugMenu,
    M_DrawDebug,
    30, 55,
    0
};

enum
{
    tst_pitch,
    tst_width,
    tst_height,
    tst_offset,
    tst_widthheightpitch,
    tst_xfac,
    tst_yfac,
    tst_xc,
    tst_yc,
    tst_end
} tst_e;

static menuitem_t TestMenu[]=
{
    {2, "Pitch"       , M_Pitch           , 'p'},
    {2, "Width"       , M_Width           , 'w'},
    {2, "Height"      , M_Height          , 'h'},
    {2, "Offset"      , M_Offset          , 'o'},
    {2, "W. + H. + P.", M_WidthHeightPitch, '+'},
    {2, "X-Factor"    , M_XFac            , 'x'},
    {2, "Y-Factor"    , M_YFac            , 'y'},
    {2, "X-COORD"     , M_XC              , 'h'},
    {2, "Y-COORD"     , M_YC              , 'v'}
};

static menu_t  TestDef =
{
    tst_end,
    &OptionsDef,
    TestMenu,
    M_DrawTest,
    30, 5,
    0
};

//
// SOUND VOLUME MENU
//
enum
{
    snd,
    sfx_vol,
    music_vol,
    mus_type,
    sfx_type,
    channels,
    output,
    sndpitch,
    rndm,

#ifndef WII
    dsc,
    lsr,
#endif

    sound_end
} sound_e;

static menuitem_t SoundMenu[]=
{
    {2, "General Sound and Music"   , M_GeneralSound        , 'g'},
    {2, ""                          , M_SfxVol              , 'v'},
    {2, ""                          , M_MusicVol            , 'm'},
    {2, "Music Type"                , M_MusicType           , 't'},
    {2, "Sound Type"                , M_SoundType           , 's'},
    {2, "Number of Sound Channels"  , M_SoundChannels       , 'c'},
    {2, "Switch Left / Right Output", M_SoundOutput         , 'o'},
    {2, "v1.1 Random Sound Pitch"   , M_SoundPitch          , 'p'},
    {2, "Random Music Playback"     , M_RandomMusic         , 'r'}

#ifndef WII
    ,
    {2, "Dump Substitute Config"    , M_DumpSubstituteConfig, 'd'},
    {2, "Libsamplerate"             , M_Samplerate          , 'l'}
#endif

};

static menu_t  SoundDef =
{
    sound_end,
    &OptionsDef,
    SoundMenu,
    M_DrawSound,
    45, 40,
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
    load7,
    load8,
    load9,
    load0,
    load_end
} load_e;

static menuitem_t LoadMenu[]=
{
    {1, "", M_LoadSelect, '1'},
    {1, "", M_LoadSelect, '2'},
    {1, "", M_LoadSelect, '3'},
    {1, "", M_LoadSelect, '4'},
    {1, "", M_LoadSelect, '5'},
    {1, "", M_LoadSelect, '6'},
    {1, "", M_LoadSelect, '7'},
    {1, "", M_LoadSelect, '8'},
    {1, "", M_LoadSelect, '9'},
    {1, "", M_LoadSelect, '0'}
};

static menu_t  LoadDef =
{
    load_end,
    &FilesDef,
    LoadMenu,
    M_DrawLoad,
    75, 34,
    0
};

//
// SAVE GAME MENU
//
static menuitem_t SaveMenu[]=
{
    {1, "", M_SaveSelect, '1'},
    {1, "", M_SaveSelect, '2'},
    {1, "", M_SaveSelect, '3'},
    {1, "", M_SaveSelect, '4'},
    {1, "", M_SaveSelect, '5'},
    {1, "", M_SaveSelect, '6'},
    {1, "", M_SaveSelect, '7'},
    {1, "", M_SaveSelect, '8'},
    {1, "", M_SaveSelect, '9'},
    {1, "", M_SaveSelect, '0'}
};

static menu_t  SaveDef =
{
    load_end,
    &FilesDef,
    SaveMenu,
    M_DrawSave,
    75, 34,
    0
};
/*
enum
{
    record_map,
    record_empty1,
    record_empty2,
    record_skill,
    record_empty3,
    record_empty4,
    record_longtics,
    record_start,
    record_end
} record_e;

static menuitem_t RecordMenu[]=
{
    {2 , "Record Map:"              , M_RMap       , 'm' },
    {-1, ""                         , 0            , '\0'},
    {-1, ""                         , 0            , '\0'},
    {2 , "Choose Skill Level:"      , M_RSkill     , 's' },
    {-1, ""                         , 0            , '\0'},
    {-1, ""                         , 0            , '\0'},
    {2 , "Longtics (DOOM.EXE v1.91)", M_RecordLong , 'l' },
    {2 , "Start Recording"          , M_StartRecord, 'r' }
};

static menu_t  RecordDef =
{
    record_end,
    &FilesDef,
    RecordMenu,
    M_DrawRecord,
    55, 60,
    0
};
*/

static void BlurMenuScreen(byte *tempscreen, byte *blurscreen, int x1, int y1, int x2, int y2, int i)
{
    int x, y;

    memcpy(tempscreen, blurscreen, SCREENWIDTH * SCREENHEIGHT);

    for (y = y1; y < y2; y += SCREENWIDTH)
        for (x = y + x1; x < y + x2; ++x)
            blurscreen[x] = tinttab50[tempscreen[x] + (tempscreen[x + i] << 8)];
}

static void BlurScreen(byte *screen, byte *tempscreen, byte *blurscreen)
{
    int i;

    for (i = 0; i < height; ++i)
        blurscreen[i] = grays[screen[i]];

    BlurMenuScreen(tempscreen, blurscreen, 0, 0, SCREENWIDTH - 1, height, 1);
    BlurMenuScreen(tempscreen, blurscreen, 1, 0, SCREENWIDTH, height, -1);
    BlurMenuScreen(tempscreen, blurscreen, 0, 0, SCREENWIDTH - 1, height - SCREENWIDTH,
        SCREENWIDTH + 1);
    BlurMenuScreen(tempscreen, blurscreen, 1, SCREENWIDTH, SCREENWIDTH, height, -(SCREENWIDTH + 1));
    BlurMenuScreen(tempscreen, blurscreen, 0, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH);
    BlurMenuScreen(tempscreen, blurscreen, 0, SCREENWIDTH, SCREENWIDTH, height, -SCREENWIDTH);
    BlurMenuScreen(tempscreen, blurscreen, 1, 0, SCREENWIDTH, height - SCREENWIDTH, SCREENWIDTH - 1);
    BlurMenuScreen(tempscreen, blurscreen, 0, SCREENWIDTH, SCREENWIDTH - 1, height,
        -(SCREENWIDTH - 1));
}

//
// M_DarkBackground
//  darken and blur background while menu is displayed
//
void M_DarkBackground(void)
{
    if (background_type == 2)
    {
        int i;

        if (usergame)
        {
            if (screenSize < 8)
                height = (SCREENHEIGHT - SBARHEIGHT) * SCREENWIDTH;
            else
                height = SCREENHEIGHT * SCREENWIDTH;
        }
        else
            height = SCREENHEIGHT * SCREENWIDTH;

        if (!blurred)
        {
            BlurScreen(screens[0], tempscreen1, blurscreen1);

            blurred = true;
        }

        for (i = 0; i < height; ++i)
            screens[0][i] = tinttab50[blurscreen1[i]];

        if (detailLevel)
            V_LowGraphicDetail(height);
    }
}

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

static void M_ReadSaveStrings(void)
{
    int    i;
    char   name[256];

    for (i = 0; i < load_end; i++)
    {
        FILE   *handle;

        M_StringCopy(name, P_SaveGameFile(i), sizeof(name));

        handle = fopen(name, "rb");

        if (handle == NULL)
        {
            M_StringCopy(&savegamestrings[i][0], s_EMPTYSTRING, SAVESTRINGSIZE);
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
static void M_DrawLoad(void)
{
    int     i;

    V_DrawPatchWithShadow(72, 8, 0, W_CacheLumpName("M_T_LGME", PU_CACHE), false);

    for (i = 0; i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x + 5, LoadDef.y + LINEHEIGHT_SMALL * i);

        if (!strncmp(savegamestrings[i], s_EMPTYSTRING, strlen(s_EMPTYSTRING)))
            dp_translation = crx[CRX_DARK];

        M_WriteText(LoadDef.x, LoadDef.y - 1 + LINEHEIGHT_SMALL * i, savegamestrings[i]);

    }

    if (!whichSkull)
    {
        char *string = "* INDICATES A SAVEGAME THAT WAS";
        char *string2 = "CREATED USING AN OPTIONAL PWAD!";
        int x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;
        int x2 = ORIGINALWIDTH / 2 - M_StringWidth(string2) / 2;

        dp_translation = crx[CRX_GOLD];
        M_WriteText(x, LoadDef.y + 108, string);

        dp_translation = crx[CRX_GOLD];
        M_WriteText(x2, LoadDef.y + 118, string2);
    }
}

//
// Draw border for the savegame description
//
static void M_DrawSaveLoadBorder(int x, int y)
{
    int     i;
        
    V_DrawPatchWithShadow(x - 8, y + 7, 0, W_CacheLumpName("M_LSLEFT", PU_CACHE), false);
        
    for (i = 0; i < 24; i++)
    {
        V_DrawPatchWithShadow(x, y + 7, 0, W_CacheLumpName("M_LSCNTR", PU_CACHE), false);
        x += 8;
    }

    V_DrawPatchWithShadow(x, y + 7, 0, W_CacheLumpName("M_LSRGHT", PU_CACHE), false);
}

//
// User wants to load this game
//
static void M_LoadSelect(int choice)
{
    char    name[256];
        
    M_StringCopy(name, P_SaveGameFile(choice), sizeof(name));

    G_LoadGame(name);
    M_ClearMenus();
}

//
// Selected from DOOM menu
//
static void M_LoadGame(int choice)
{
/*
    // [crispy] forbid New Game and (Quick) Load while recording a demo
    if (demorecording)
    {
        return;
    }
*/
    if (netgame)
    {
        M_StartMessage(s_LOADNET, NULL, false);
        return;
    }

    M_SetupNextMenu(&LoadDef);
    M_ReadSaveStrings();
}

//
// M_SaveGame & Cie.
//
static void M_DrawSave(void)
{
    int     i;
        
    V_DrawPatchWithShadow(72, 8, 0, W_CacheLumpName("M_T_SGME", PU_CACHE), false);

    for (i = 0; i < load_end; i++)
    {
        M_DrawSaveLoadBorder(LoadDef.x + 5, LoadDef.y + LINEHEIGHT_SMALL * i);
        M_WriteText(LoadDef.x, LoadDef.y - 1 + LINEHEIGHT_SMALL * i, savegamestrings[i]);
    }
        
    if (saveStringEnter)
    {
        i = M_StringWidth(savegamestrings[saveSlot]);

        dp_translation = crx[CRX_GRAY];
        M_WriteText(LoadDef.x + i, LoadDef.y - 1 + LINEHEIGHT_SMALL * saveSlot, "_");
    }

    if (!whichSkull)
    {
        char *string = "* INDICATES A SAVEGAME THAT WAS";
        char *string2 = "CREATED USING AN OPTIONAL PWAD!";
        int x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;
        int x2 = ORIGINALWIDTH / 2 - M_StringWidth(string2) / 2;

        dp_translation = crx[CRX_GOLD];
        M_WriteText(x, SaveDef.y + 108, string);

        dp_translation = crx[CRX_GOLD];
        M_WriteText(x2, SaveDef.y + 118, string2);
    }
}

//
// M_Responder calls this when user is finished
//
static void M_DoSave(int slot)
{
    if (players[consoleplayer].playerstate == PST_DEAD)
    {
        M_ClearMenus();
        M_StartMessage("YOU CANNOT SAVE A GAME - YOU'RE DEAD", NULL, true);
        return;
    }

    G_SaveGame(slot, savegamestrings[slot], "");
    M_ClearMenus();

    // PICK QUICKSAVE SLOT YET?
    if (quickSaveSlot == -2)
        quickSaveSlot = slot;
}

//
// User wants to save. Start string input for M_Responder
//
static void M_SaveSelect(int choice)
{
    time_t theTime = time(NULL);
    struct tm *aTime = localtime(&theTime);

    int day = aTime->tm_mday;
    int month = aTime->tm_mon + 1;
    int year = aTime->tm_year + 1900;
    int hour = aTime->tm_hour;
    int min = aTime->tm_min;

    // we are going to be intercepting all chars
    saveStringEnter = 1;
    
    saveSlot = choice;

    if (gamemode == shareware || gamemode == registered || gamemode == retail)
    {
        if (modifiedgame)
            sprintf(savegamestrings[choice], "e%dm%d %d/%d/%d %2.2d:%2.2d *",
                    gameepisode, gamemap, year, month, day, hour, min);
        else
            sprintf(savegamestrings[choice], "e%dm%d %d/%d/%d %2.2d:%2.2d",
                    gameepisode, gamemap, year, month, day, hour, min);
    }
    else
    {
        if (modifiedgame)
            sprintf(savegamestrings[choice], "map%2.2d %d/%d/%d %2.2d:%2.2d *",
                    gamemap, year, month, day, hour, min);
        else
            sprintf(savegamestrings[choice], "map%2.2d %d/%d/%d %2.2d:%2.2d",
                    gamemap, year, month, day, hour, min);
    }

    M_StringCopy(saveOldString, savegamestrings[choice], sizeof(saveOldString));

    if (!strcmp(savegamestrings[choice], s_EMPTYSTRING))
        savegamestrings[choice][0] = 0;

    saveCharIndex = strlen(savegamestrings[choice]);
}

//
// Selected from DOOM menu
//
static void M_SaveGame(int choice)
{
    if (splashscreen)
        return;

    if (!usergame)
    {
        M_StartMessage(s_SAVEDEAD, NULL, true);
        return;
    }
        
    if (gamestate != GS_LEVEL)
        return;
        
    M_SetupNextMenu(&SaveDef);
    M_ReadSaveStrings();
}

//
// M_QuickSave
//

#ifndef WII
static void M_QuickSaveResponse(int key)
{
    if (key == key_menu_confirm)
    {
        M_DoSave(quickSaveSlot);
        S_StartSound(NULL, sfx_swtchx);
    }
}

static void M_QuickSave(void)
{
    if (splashscreen)
        return;

    if (!usergame)
    {
        S_StartSound(NULL, sfx_oof);
        return;
    }

    if (gamestate != GS_LEVEL)
        return;
        
    if (quickSaveSlot < 0)
    {
        M_StartControlPanel();
        M_ReadSaveStrings();
        M_SetupNextMenu(&SaveDef);

        // means to pick a slot now
        quickSaveSlot = -2;
        return;
    }

    M_snprintf(tempstring, 80, s_QSPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickSaveResponse, true);
}

//
// M_QuickLoad
//
static void M_QuickLoadResponse(int key)
{
    if (key == key_menu_confirm)
    {
        M_LoadSelect(quickSaveSlot);
        S_StartSound(NULL, sfx_swtchx);
    }
}

static void M_QuickLoad(void)
{
    if (splashscreen)
        return;

    if (netgame)
    {
        M_StartMessage(s_QLOADNET, NULL, false);
        return;
    }
        
    if (quickSaveSlot < 0)
    {
        M_StartMessage(s_QSAVESPOT, NULL, false);
        return;
    }

    M_snprintf(tempstring, 80, s_QLPROMPT, savegamestrings[quickSaveSlot]);
    M_StartMessage(tempstring, M_QuickLoadResponse, true);
}

#else

static void M_Beta(int choice)
{
    switch (choice)
    {
        case 0:
            if (beta_style_mode && fsize != 28422764 && fsize != 19321722 && fsize != 12361532)
                beta_style_mode = false;

            break;

        case 1:
            if (!beta_style_mode && fsize != 28422764 && fsize != 19321722 && fsize != 12361532)
                beta_style_mode = true;

            break;
    }
}

static void DetectState(void)
{
    if (!netgame /*&& !demoplayback*/ && gamestate == GS_LEVEL
        && gameskill != sk_nightmare &&
        players[consoleplayer].playerstate == PST_DEAD)
    {
        M_StartMessage("CHEATING NOT ALLOWED - YOU'RE DEAD",
        NULL, true);
    }
/*
    else if (!netgame && demoplayback && gamestate == GS_LEVEL
        && players[consoleplayer].playerstate == PST_LIVE)
    {
        M_StartMessage("CHEATING NOT ALLOWED IN DEMO MODE",
        NULL, true);
    }
    else if (!netgame && demoplayback && gamestate == GS_LEVEL
        && players[consoleplayer].playerstate == PST_DEAD)
    {
        M_StartMessage("CHEATING NOT ALLOWED IN DEMO MODE",
        NULL, true);
    }
    else if (!netgame && demoplayback && gamestate != GS_LEVEL)
    {
        M_StartMessage("CHEATING NOT ALLOWED IN DEMO MODE",
        NULL, true);
    }
*/
    else if (!netgame /*&& !demoplayback*/ && gamestate != GS_LEVEL)
    {
        M_StartMessage("CHEATING NOT ALLOWED IN DEMO MODE",
        NULL, true);
    }
    else if (netgame)
    {
        M_StartMessage("CHEATING NOT ALLOWED FOR NET GAME",
        NULL, true);
    }

    if (gameskill == sk_nightmare)
    {
        M_StartMessage("CHEATING DISABLED - NIGHTMARE SKILL",
        NULL, true);
    }
}

static void M_Cheats(int choice)
{
    DetectState();

    M_SetupNextMenu(&CheatsDef);
}
#endif

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
static void M_DrawReadThis1(void)
{
    char *lumpname = "CREDIT";
    int skullx = 330;
    int skully = 175;

    inhelpscreens = true;
    
    // Different versions of Doom 1.9 work differently
    switch (gameversion)
    {
        case exe_doom_1_666:
        case exe_doom_1_7:
        case exe_doom_1_8:
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
            C_Warning("Unhandled game version");

            break;
    }

    V_DrawPatch(0, 0, 0, W_CacheLumpName(lumpname, PU_CACHE));

    if (fsize == 12361532)
        skullx = 325;

    ReadDef1.x = skullx;
    ReadDef1.y = skully;
}

//
// Read This Menus - optional second page.
//
static void M_DrawReadThis2(void)
{
    inhelpscreens = true;

    // We only ever draw the second page if this is 
    // gameversion == exe_doom_1_9 and gamemode == registered

    V_DrawPatch(0, 0, 0, W_CacheLumpName("HELP1", PU_CACHE));
}

static void M_DrawCredits(void)
{
    int i, x, y;

    inhelpscreens = true;

    // draw the credits
    for (i = 0, y = SCREENHEIGHT - ((gametic - credits_start_time) / 2); credits[i] && y < SCREENHEIGHT; y += 10, i++)
    {
        int j;
        int stringoffset = 0;
        int colorize_to = -32;

        if (y <= -8)
            continue;

        if (credits[i][0] == '+')
            colorize_to = 32;
        else if (credits[i][0] == '*')
            colorize_to = 96;
        else if (credits[i][0] == '#')
            colorize_to = 160;

        if (colorize_to > 0)
            stringoffset = 1;

        for (j = 0; credits[i][j + stringoffset] && !draw_ended; j++)
        {
            x = (SCREENWIDTH - strlen(credits[i]) * 8 - stringoffset * 8) / 2 + (j + stringoffset) * 8;

            if (credits[i][0] == 'T' && credits[i][1] == 'H' && credits[i][2] == 'A' &&
                credits[i][3] == 'N' && credits[i][4] == 'K' && credits[i][5] == 'S' && y < 200)
            {
                if (y <= 0)
                {
                    draw_ended = true;
                    break;
                }

                y = 200;
            }

            R_DrawChar(x, y, credits[i][j + stringoffset] + colorize_to);
        }
    }

    if (draw_ended)
    {
        int  k;
        char *thanks = "THANKS FOR PLAYING THIS PORT";

        for (k = 0; k < strlen(thanks); k++)
        {
            x = (SCREENWIDTH - strlen(thanks) * 8) / 2 + k * 8;

            R_DrawChar(x, 200, thanks[k] - 32);
        }
    }

    if (y < 0)
        credits_start_time = gametic;

    S_ChangeMusic(mus_credit, true, false);
}

//
// Change Sfx & Music volumes
//
static void M_DrawSound(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(65, 15, 0, W_CacheLumpName("M_T_XSET", PU_CACHE), false);
    else
        V_DrawPatchWithShadow(65, 15, 0, W_CacheLumpName("M_SNDSET", PU_CACHE), false);

    M_DrawThermoSmall(SoundDef.x + 95, SoundDef.y + LINEHEIGHT_SMALL * (sfx_vol + 1),
                 16, sfxVolume);

    M_DrawThermoSmall(SoundDef.x + 95, SoundDef.y + LINEHEIGHT_SMALL * (music_vol + 1),
                 16, musicVolume);

    if (itemOn == 1 && general_sound)
        dp_translation = crx[CRX_GOLD];
    else if (!general_sound)
        dp_translation = crx[CRX_DARK];

    M_WriteText(SoundDef.x, SoundDef.y + 8, "SOUND VOLUME");

    if (itemOn == 2 && general_sound)
        dp_translation = crx[CRX_GOLD];
    else if (!general_sound)
        dp_translation = crx[CRX_DARK];

    M_WriteText(SoundDef.x, SoundDef.y + 18, "MUSIC VOLUME");

    if (general_sound)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 220, SoundDef.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SoundDef.x + 212, SoundDef.y - 2, "OFF");
    }

    if (mus_engine == 1)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 204, SoundDef.y + 28, "OPL2");
    }
    else if (mus_engine == 2)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 204, SoundDef.y + 28, "OPL3");
    }
    else if (mus_engine == 3)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 212, SoundDef.y + 28, "OGG");
    }
    else if (mus_engine == 4)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 183, SoundDef.y + 28, "TIMIDITY");
    }

    if (snd_module)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 159, SoundDef.y + 38, "PC-SPEAKER");
    }
    else
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 213, SoundDef.y + 38, "SDL");
    }

    if (sound_channels == 8)
    {
        dp_translation = crx[CRX_BLUE];
        M_WriteText(SoundDef.x + 228, SoundDef.y + 48, "8");
    }
    else if (sound_channels == 16)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 223, SoundDef.y + 48, "16");
    }
    else if (sound_channels == 32)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(SoundDef.x + 220, SoundDef.y + 48, "32");
    }
    else if (sound_channels == 64)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(SoundDef.x + 221, SoundDef.y + 48, "64");
    }

    if (opl_stereo_correct)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 220, SoundDef.y + 58, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SoundDef.x + 212, SoundDef.y + 58, "OFF");
    }

    if (randompitch)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 220, SoundDef.y + 68, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SoundDef.x + 212, SoundDef.y + 68, "OFF");
    }

    if (s_randommusic)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 220, SoundDef.y + 78, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SoundDef.x + 212, SoundDef.y + 78, "OFF");
    }

    if (dump_subst)
    {
        if (substdumpwait < 5)
        {
            dp_translation = crx[CRX_GREEN];
            M_WriteText(SoundDef.x + 204, SoundDef.y + 88, "DONE");
        }
        else
        {
            substdumpwait = 0;
            dump_subst = false;
        }
    }

#ifndef WII
    if (use_libsamplerate == 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SoundDef.x + 212, SoundDef.y + 98, "OFF");
    }
    else if (use_libsamplerate == 1)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(SoundDef.x + 192, SoundDef.y + 98, "LINEAR");
    }
    else if (use_libsamplerate == 2)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(SoundDef.x + 117, SoundDef.y + 98, "ZERO_ORDER_HOLD");
    }
    else if (use_libsamplerate == 3)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(SoundDef.x + 182, SoundDef.y + 98, "FASTEST");
    }
    else if (use_libsamplerate == 4)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SoundDef.x + 130, SoundDef.y + 98, "MEDIUM_QUALITY");
    }
    else if (use_libsamplerate == 5)
    {
        dp_translation = crx[CRX_BLUE];
        M_WriteText(SoundDef.x + 145, SoundDef.y + 98, "BEST_QUALITY");
    }
#endif

    if (!whichSkull)
    {
        int x;
        char *string = "";
        dp_translation = crx[CRX_GOLD];

        if (itemOn == 10 || (fsize != 28422764 && fsize != 19321722 && fsize != 12361532 && itemOn > 2 && itemOn < 6))
            string = "YOU MUST QUIT AND RESTART TO TAKE EFFECT.";
        else if (fsize == 19321722 && itemOn == 4)
            string = "PC-SPEAKER OPTION NOT AVAILABLE FOR HACX";
        else if (itemOn == 9)
            string = "DUMPS A CONFIG FILE FOR USE WITH OGG-MUSIC.";

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;
        M_WriteText(x, GameDef2.y + 138, string);
    }
}

static void M_Sound(int choice)
{
    M_SetupNextMenu(&SoundDef);
}

static void M_GeneralSound(int choice)
{
    switch (choice)
    {
        case 0:
            if (general_sound)
            {
                general_sound = false;
                S_SetSfxVolume(0 * 8);
                S_SetMusicVolume(0 * 8);
            }

            break;

        case 1:
            if (!general_sound)
            {
                general_sound = true;
                S_SetSfxVolume(sfxVolume * 8);
                S_SetMusicVolume(musicVolume * 8);
            }

            break;
    }
}

static void M_SfxVol(int choice)
{
    if (general_sound)
    {
        switch (choice)
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
}

static void M_MusicVol(int choice)
{
    if (general_sound)
    {
        switch (choice)
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
}

static void M_SoundType(int choice)
{
    switch (choice)
    {
        case 0:
            if (snd_module && fsize != 19321722)
                snd_module = false;

            break;

        case 1:
            if (!snd_module && fsize != 19321722)
                snd_module = true;

            break;
    }
}

static void M_MusicType(int choice)
{
    switch (choice)
    {
        case 0:
            if (mus_engine > 1)
            {
                if (mus_engine == 4)
                    snd_musicdevice = SNDDEVICE_GUS;
                else if (mus_engine == 3)
                    snd_musicdevice = SNDDEVICE_GENMIDI;
                else if (mus_engine < 3)
                {
                    if (mus_engine == 2)
                        opl_type = true;
                    else
                        opl_type = false;

                    snd_musicdevice = SNDDEVICE_SB;
                }

                mus_engine--;
            }

            break;

        case 1:
            if (mus_engine < 4)
            {
                if (mus_engine == 4)
                    snd_musicdevice = SNDDEVICE_GUS;
                else if (mus_engine == 3)
                    snd_musicdevice = SNDDEVICE_GENMIDI;
                else if (mus_engine < 3)
                {
                    if (mus_engine == 2)
                        opl_type = true;
                    else
                        opl_type = false;

                    snd_musicdevice = SNDDEVICE_SB;
                }

                mus_engine++;
            }

            break;
    }
}

static void M_SoundOutput(int choice)
{
    switch (choice)
    {
        case 0:
            if (opl_stereo_correct)
                opl_stereo_correct = false;

            break;

        case 1:
            if (!opl_stereo_correct)
                opl_stereo_correct = true;

            break;
    }
}

static void M_SoundPitch(int choice)
{
    switch (choice)
    {
        case 0:
            if (randompitch)
                randompitch = false;

            break;

        case 1:
            if (!randompitch)
                randompitch = true;

            break;
    }
}

static void M_RandomMusic(int choice)
{
    switch (choice)
    {
        case 0:
            if (s_randommusic)
                s_randommusic = false;

            break;

        case 1:
            if (!s_randommusic)
                s_randommusic = true;

            break;
    }
}

#ifndef WII
static void M_DumpSubstituteConfig(int choice)
{
    dump_subst = true;
    DumpSubstituteConfig("oggmusic.cfg");
}

static void M_Samplerate(int choice)
{
    switch (choice)
    {
        case 0:
            if (use_libsamplerate)
                use_libsamplerate--;

            break;

        case 1:
            if (use_libsamplerate < 5)
                use_libsamplerate++;

            break;
    }
}
#endif

static void M_RestartSong(int choice)
{
    restart_song = true;
    S_StopMusic();

    // FIXME: Add case for other game missions here (CHEX, HACX, FINAL DOOM)
    if (gamemode != commercial)
        S_ChangeMusic(gamemap, true, true);
    else
        S_ChangeMusic(gamemap + 32, true, true);
}

static void M_OPLDev(int choice)
{
    switch (choice)
    {
        case 0:
            if (opldev)
                opldev = false;

            break;

        case 1:
            if (!opldev)
                opldev = true;

            break;
    }
}

static void M_SoundChannels(int choice)
{
    switch (choice)
    {
        case 0:
            if (snd_chans > 1)
                snd_chans--;

            if (snd_chans == 1)
                sound_channels = 8;
            else if (snd_chans == 2)
                sound_channels = 16;
            else if (snd_chans == 3)
                sound_channels = 32;
            else if (snd_chans == 4)
                sound_channels = 64;

            break;

        case 1:
            if (snd_chans < 4)
                snd_chans++;

            if (snd_chans == 1)
                sound_channels = 8;
            else if (snd_chans == 2)
                sound_channels = 16;
            else if (snd_chans == 3)
                sound_channels = 32;
            else if (snd_chans == 4)
                sound_channels = 64;

            break;
    }
}

//
// M_DrawMainMenu
//
static void M_DrawMainMenu(void)
{
    if (beta_style || !font_shadow)
        V_DrawPatch(94, 2, 0, W_CacheLumpName("M_DOOM", PU_CACHE));
    else
        V_DrawPatchWithShadow(94, 2, 0, W_CacheLumpName("M_DOOM", PU_CACHE), false);
}

//
// M_NewGame
//
static void M_DrawNewGame(void)
{
    V_DrawPatchWithShadow(96, 14, 0, W_CacheLumpName("M_NEWG", PU_CACHE), false);
    M_WriteText(NewDef.x, NewDef.y - 22, "CHOOSE SKILL LEVEL:");
}

static void M_NewGame(int choice)
{
    // [crispy] forbid New Game and (Quick) Load while recording a demo
/*
    if (demorecording)
    {
        return;
    }
*/
    if (netgame /*&& !demoplayback*/)
    {
        M_StartMessage(s_NEWGAME, NULL, false);
        return;
    }

    // Chex Quest disabled the episode select screen, as did Doom II.
    if (!beta_style)
    {
        if (fsize == 12361532)
            M_SetupNextMenu(&NewDef);
        else
            M_SetupNextMenu(gamemode == commercial ? (nerve_pwad ? &ExpDef : &NewDef) : &EpiDef);
    }
    else
    {
        if (itemOn == 0)
            G_InitNew (startskill, 1, 2);
        else if (itemOn == 1)
            G_InitNew (startskill, 3, 5);
        else if (itemOn == 2)
            G_InitNew (startskill, 2, 2);
    }
}

//
// M_Episode
//
static void M_DrawEpisode(void)
{
    V_DrawPatchWithShadow(75, 38, 0, W_CacheLumpName("M_EPISOD", PU_CACHE), false);
}

static void M_VerifyNightmare(int ch)
{
#ifdef WII
    if (ch != key_menu_forward)
#else
    if (ch != key_menu_confirm)
#endif
        return;

    G_DeferedInitNew(nightmare, epi + 1, 1);
    M_ClearMenus();
}

static void M_ChooseSkill(int choice)
{
    if (choice == nightmare)
    {
        M_StartMessage(s_NIGHTMARE, M_VerifyNightmare, true);
        return;
    }

    if (fsize == 12361532)
        G_DeferedInitNew(choice, epi, 1);
    else
        G_DeferedInitNew(choice, epi + 1, 1);

    if (gameepisode == 1)
        secret_1 = false;
    else
        secret_2 = false;

    M_ClearMenus();
}

static void M_Episode(int choice)
{
    if ((gamemode == shareware) && choice)
    {
        M_StartMessage(s_SWSTRING, NULL, true);
        M_SetupNextMenu(&ReadDef1);
        return;
    }

    // Yet another hack...
    if ((gamemode == registered) && (choice > 2))
    {
        C_Error("M_Episode: 4th episode requires Ultimate DOOM");
        choice = 0;
    }
         
    epi = choice;
    M_SetupNextMenu(&NewDef);
}

//
// M_Options
//
static void M_DrawOptions(void)
{
    V_DrawPatchWithShadow(108, 15, 0, W_CacheLumpName("M_OPTTTL",
                                               PU_CACHE), false);

    dp_translation = crx[CRX_GRAY];

    if (itemOn == 0)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(OptionsDef.x, OptionsDef.y - 2, "SCREEN SETTINGS");

    dp_translation = crx[CRX_GRAY];

    if (itemOn == 1)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(OptionsDef.x, OptionsDef.y + 8, "CONTROL SETTINGS");

    dp_translation = crx[CRX_GRAY];

    if (itemOn == 2)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(OptionsDef.x, OptionsDef.y + 18, "SOUND SETTINGS");

    dp_translation = crx[CRX_GRAY];

    if (itemOn == 3)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(OptionsDef.x, OptionsDef.y + 28, "SYSTEM SETTINGS");

    dp_translation = crx[CRX_GRAY];

    if (itemOn == 4)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(OptionsDef.x, OptionsDef.y + 38, "GAME SETTINGS");

    dp_translation = crx[CRX_BLUE];

    if (itemOn == 6)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(OptionsDef.x, OptionsDef.y + 58, "OPTIMAL SETTINGS");

    dp_translation = crx[CRX_BLUE];

    if (itemOn == 7)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(OptionsDef.x, OptionsDef.y + 68, "RESET DEFAULTS");

    if (devparm)
    {
        dp_translation = crx[CRX_GRAY];

        if (itemOn == 9)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(OptionsDef.x, OptionsDef.y + 88, "DEBUG SETTINGS");

        dp_translation = crx[CRX_GRAY];

        if (itemOn == 10)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(OptionsDef.x, OptionsDef.y + 98, "TESTINGS");
    }

    if (!whichSkull)
    {
        int x;
        int x2;
        char *string = "";
        char *string2 = "";

        dp_translation = crx[CRX_GOLD];

        if (itemOn == 6 || itemOn == 7)
        {
            string = "(KEY BINDINGS & AUTOMAP COLORS UNAFFECTED)";
            string2 = "YOU MUST RESTART THE GAME TO TAKE EFFECT.";
        }

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;
        x2 = ORIGINALWIDTH / 2 - M_StringWidth(string2) / 2;

        M_WriteText(x, OptionsDef.y + 106, string);

        dp_translation = crx[CRX_GOLD];

        M_WriteText(x2, OptionsDef.y + 113, string2);
    }
}

#ifdef WII
static void M_DrawItems(void)
{
    V_DrawPatchWithShadow(123, 10, 0, W_CacheLumpName("M_T_ITMS",
                                               PU_CACHE), false);

    dp_translation = crx[CRX_GOLD];
    M_WriteText(80, 50, "GIVE THEM ALL AT ONCE");

    if (fsize == 10396254 || fsize == 10399316 || fsize == 4207819 ||
        fsize == 4274218 || fsize == 4225504)
        M_WriteText(80, 110, "FULL HEALTH (199)");
    else
        M_WriteText(80, 110, "FULL HEALTH (200)");

    if (fsize != 12361532 && fsize != 19321722)
    {
        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 70, "RADIATION SHIELDING SUIT");

        if (itemOn == 3)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 80, "COMPUTER AREA MAP");

        if (itemOn == 4)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 90, "LIGHT AMPLIFICATION VISOR");

        if (itemOn == 7)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 120, "PARTIAL INVISIBILITY");

        if (itemOn == 8)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 130, "INVULNERABILITY!");

        if (itemOn == 9)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 140, "BERSERK!");

        if (itemOn == 10)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 150, "BACKPACK");

        if (itemOn == 11)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 160, "FLIGHT");
    }

    if (fsize == 19321722)
    {
        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 70, "VULCAN RUBBER BOOTS");

        if (itemOn == 3)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 80, "SI ARRAY MAPPING");

        if (itemOn == 4)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 90, "INFRARED VISOR");

        if (itemOn == 7)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 120, "ENK BLINDNESS");

        if (itemOn == 8)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 130, "FORCE FIELD");

        if (itemOn == 9)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 140, "007 MICROTEL");

        if (itemOn == 10)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 150, "BACKPACK");

        if (itemOn == 11)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 160, "FLIGHT");
    }

    if (fsize == 12361532)
    {
        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 70, "SLIME-PROOF SUIT");

        if (itemOn == 3)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 80, "COMPUTER AREA MAP");

        if (itemOn == 4)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 90, "ULTRA GOGGLES");

        if (itemOn == 5)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 100, "BACKPACK");

        if (itemOn == 6)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 110, "FLIGHT");
    }
}

static void M_DrawArmor(void)
{
    V_DrawPatchWithShadow(115, 15, 0, W_CacheLumpName("M_T_ARMR",
                                               PU_CACHE), false);

    dp_translation = crx[CRX_GOLD];
    M_WriteText(80, 55, "GIVE THEM BOTH AT ONCE");

    if (fsize != 12361532 && fsize != 19321722)
    {
        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 75, "GREEN ARMOR");

        if (itemOn == 3)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 85, "BLUE ARMOR");
    }

    if (fsize == 12361532)
    {
        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 75, "CHEX(R) ARMOR");

        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 85, "SUPER CHEX(R) ARMOR");
    }

    if (fsize == 19321722)
    {
        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 75, "KEVLAR VEST");

        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 85, "SUPER KEVLAR VEST");
    }
}

static void M_DrawWeapons(void)
{
    V_DrawPatchWithShadow(103, 15, 0, W_CacheLumpName("M_T_WPNS",
                                               PU_CACHE), false);

    dp_translation = crx[CRX_GOLD];
    M_WriteText(80, 55, "GIVE THEM ALL AT ONCE");

    if (fsize != 19321722 && fsize != 12361532)
    {
        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 75, "CHAINSAW");

        if (itemOn == 3)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 85, "SHOTGUN");

        if (itemOn == 4)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 95, "CHAINGUN");

        if (itemOn == 5)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 105, "ROCKET LAUNCHER");

        if (fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
            fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
            fsize != 4225460 && fsize != 4234124 && fsize != 4196020)
        {
            if (itemOn == 6)
                dp_translation = crx[CRX_GOLD];

            M_WriteText(80, 115, "PLASMA CANNON");

            if (itemOn == 7)
                dp_translation = crx[CRX_GOLD];

            M_WriteText(80, 125, "BFG 9000");
        }

        if (fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
            fsize == 14607420 || fsize == 14604584 || fsize == 18195736 ||
            fsize == 14683458 || fsize == 18654796 || fsize == 18240172 ||
            fsize == 17420824 || fsize == 14677988 || fsize == 14691821 ||
            fsize == 28422764)
        {
            if (itemOn == 8)
                dp_translation = crx[CRX_GOLD];

            M_WriteText(80, 135, "SUPER SHOTGUN");
        }
    }

    if (fsize == 19321722)
    {
        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 75, "HOIG REZNATOR");

        if (itemOn == 3)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 85, "TAZER");

        if (itemOn == 4)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 95, "UZI");

        if (itemOn == 5)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 105, "PHOTON 'ZOOKA");

        if (itemOn == 6)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 115, "STICK");

        if (itemOn == 7)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 125, "NUKER");

        if (itemOn == 8)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 135, "CRYOGUN");
    }

    if (fsize == 12361532)
    {
        if (itemOn == 2)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 75, "SUPER BOOTSPORK");

        if (itemOn == 3)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 85, "LARGE ZORCHER");

        if (itemOn == 4)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 95, "RAPID ZORCHER");

        if (itemOn == 5)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 105, "ZORCH PROPULSOR");

        if (itemOn == 6)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 115, "PHASING ZORCHER");

        if (itemOn == 7)
            dp_translation = crx[CRX_GOLD];

        M_WriteText(80, 125, "LARGE AREA ZORCHING DEVICE");
    }
}

static void M_DrawKeys(void)
{
    V_DrawPatchWithShadow(125, 15, 0, W_CacheLumpName("M_T_KEYS",
                                               PU_CACHE), false);

    dp_translation = crx[CRX_GOLD];
    M_WriteText(80, 55, "GIVE ALL KEYS FOR THIS MAP");
}
#endif

static void M_DrawScreen(void)
{
    int offset = 0;

#ifndef SDL2
    offset = 10;
#endif

    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(58, 7, 0, W_CacheLumpName("M_T_SSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(58, 7, 0, W_CacheLumpName("M_SCRSET",
                                               PU_CACHE), false);

    M_DrawThermoSmall(ScreenDef.x + 104, ScreenDef.y + offset + LINEHEIGHT_SMALL * (screen_gamma + 1),
                 11, usegamma / 3);

    if (gamemode == commercial || fsize == 4234124 || fsize == 4196020 ||
        fsize == 12474561 || fsize == 12487824 || fsize == 11159840 ||
        fsize == 12408292 || fsize == 12538385 || fsize == 12361532 ||
        fsize == 7585664)
    {
        if (gamemission == pack_hacx)
            dp_translation = crx[CRX_BLUE];
        else if (gamemission == pack_chex)
            dp_translation = crx[CRX_GREEN];
        else
            dp_translation = crx[CRX_RED];
    }
    else
        dp_translation = crx[CRX_DARK];

    if (!show_diskicon)
        dp_translation = crx[CRX_DARK];

    if (itemOn == 11 && show_diskicon && gamemode == commercial)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(ScreenDef.x, ScreenDef.y + 108 + offset, "Type of Indicator");

    if (detailLevel)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ScreenDef.x + 180, ScreenDef.y + offset + 18, "LOW");
    }
    else
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 177, ScreenDef.y + offset + 18, "HIGH");
    }

    if (d_translucency)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 189, ScreenDef.y + offset + 28, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ScreenDef.x + 181, ScreenDef.y + offset + 28, "OFF");
    }

    if (beta_style)
        M_DrawThermoSmall(ScreenDef.x + 128, ScreenDef.y + offset + LINEHEIGHT_SMALL * (screen_size + 1),
                 8, screenSize);
    else
        M_DrawThermoSmall(ScreenDef.x + 120, ScreenDef.y + offset + LINEHEIGHT_SMALL * (screen_size + 1),
                 9, screenSize);

    if (wipe_type == 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ScreenDef.x + 173, ScreenDef.y + offset + 38, "NONE");
    }
    else if (wipe_type == 1)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 173, ScreenDef.y + offset + 38, "FADE");
    }
    else if (wipe_type == 2)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ScreenDef.x + 172, ScreenDef.y + offset + 38, "MELT");
    }
    else if (wipe_type == 3)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(ScreenDef.x + 173, ScreenDef.y + offset + 38, "BURN");
    }

    if (d_uncappedframerate == 0)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(ScreenDef.x + 97, ScreenDef.y + offset + 48, "CAPPED (35 FPS)");
    }
    else if (d_uncappedframerate == 1)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ScreenDef.x + 141, ScreenDef.y + offset + 48, "UNCAPPED");
    }
    else if (d_uncappedframerate == 2)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 96, ScreenDef.y + offset + 48, "CAPPED (60 FPS)");
    }
    else if (d_uncappedframerate == 3)
    {
        dp_translation = crx[CRX_BLUE];
        M_WriteText(ScreenDef.x + 96, ScreenDef.y + offset + 48, "CAPPED (70 FPS)");
    }

    if (png_screenshots)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 181, ScreenDef.y + offset + 58, "PNG");
    }
    else
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ScreenDef.x + 180, ScreenDef.y + offset + 58, "PCX");
    }

    if (background_type == 0)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(ScreenDef.x + 156, ScreenDef.y + offset + 68, "NORMAL");
    }
    else if (background_type == 1)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ScreenDef.x + 123, ScreenDef.y + offset + 68, "CRISPY-DOOM");
    }
    else if (background_type == 2)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 128, ScreenDef.y + offset + 68, "DOOM RETRO");
    }
    else if (background_type == 3)
    {
        dp_translation = crx[CRX_BLUE];
        M_WriteText(ScreenDef.x + 156, ScreenDef.y + offset + 68, "PRBOOM");
    }
    else if (background_type == 4)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(ScreenDef.x + 165, ScreenDef.y + offset + 68, "ZDOOM");
    }

    if (background_type != 4)
        dp_translation = crx[CRX_DARK];
    else if (background_type == 4 && itemOn == 8)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(ScreenDef.x, ScreenDef.y + offset + 78, "Menu Background Color");

    sprintf(backgroundcolor_buffer, "%d", background_color);
    M_WriteText(ScreenDef.x + 205 - M_StringWidth(backgroundcolor_buffer), ScreenDef.y + offset + 78, backgroundcolor_buffer);

    if (font_shadow == 0)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 156, ScreenDef.y + offset + 88, "NORMAL");
    }
    else if (font_shadow == 1)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 141, ScreenDef.y + offset + 88, "SHADOWED");
    }
    else if (font_shadow == 2)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ScreenDef.x + 149, ScreenDef.y + offset + 88, "COLORED");
    }

    if (show_diskicon)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 189, ScreenDef.y + offset + 98, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ScreenDef.x + 181, ScreenDef.y + offset + 98, "OFF");
    }

    if (icontype)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(ScreenDef.x + 158, ScreenDef.y + offset + 108, "CD-ROM");
    }
    else
    {
        dp_translation = crx[CRX_BLUE];
        M_WriteText(ScreenDef.x + 126, ScreenDef.y + offset + 108, "FLOPPY DISK");
    }

#ifdef SDL2
    if (render_mode)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 161, ScreenDef.y + offset + 118, "LINEAR");
    }
    else
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ScreenDef.x + 150, ScreenDef.y + offset + 118, "NEAREST");
    }

    if (d_vsync)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ScreenDef.x + 189, ScreenDef.y + offset + 128, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ScreenDef.x + 181, ScreenDef.y + offset + 128, "OFF");
    }
#endif

    if (!whichSkull)
    {
        int x;
        char *string = "";

        dp_translation = crx[CRX_GOLD];

        if (itemOn == 1 && am_overlay)
            string = "YOU MUST LEAVE AUTOMAP OVERLAY MODE FIRST!!!";
        else if (itemOn == 3)
            string = "START / LOAD A NEW GAME TO TAKE EFFECT.";
        else if (itemOn == 5 && d_vsync)
            string = "DISABLE VSYNC TO UNLOCK 70 FPS & UNCAPPED.";
        else if (itemOn == 8)
        {
            if (background_type != 4)
                string = "ONLY FOR 'ZDOOM' MENU BACKGROUND";
            else
                string = "DEFAULT VALUE IS 231";
        }
        else if (itemOn == 11 && (gamemode == retail || gamemode == registered || gamemode == shareware))
            string = "THIS IS ONLY CHANGEABLE FOR DOOM 2";

#ifdef SDL2
        else if (itemOn == 12 || itemOn == 13)
            string = "YOU MUST QUIT AND RESTART TO TAKE EFFECT.";
#endif

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;

        M_WriteText(x, ScreenDef.y + 136, string);
    }
}

static void M_DrawAutomap(void)
{
    sprintf(string_back, "%d", mapcolor_back);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 20, 0, 10, 10, mapcolor_back);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y - 2, string_back);

    sprintf(string_grid, "%d", mapcolor_grid);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 32, 0, 10, 10, mapcolor_grid);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 4, string_grid);

    sprintf(string_wall, "%d", mapcolor_wall);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 44, 0, 10, 10, mapcolor_wall);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 10, string_wall);

    sprintf(string_fchg, "%d", mapcolor_fchg);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 56, 0, 10, 10, mapcolor_fchg);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 16, string_fchg);

    sprintf(string_cchg, "%d", mapcolor_cchg);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 68, 0, 10, 10, mapcolor_cchg);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 22, string_cchg);

    sprintf(string_clsd, "%d", mapcolor_clsd);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 80, 0, 10, 10, mapcolor_clsd);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 28, string_clsd);

    sprintf(string_rkey, "%d", mapcolor_rkey);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 92, 0, 10, 10, mapcolor_rkey);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 34, string_rkey);

    sprintf(string_bkey, "%d", mapcolor_bkey);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 104, 0, 10, 10, mapcolor_bkey);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 40, string_bkey);

    sprintf(string_ykey, "%d", mapcolor_ykey);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 116, 0, 10, 10, mapcolor_ykey);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 46, string_ykey);

    sprintf(string_rdor, "%d", mapcolor_rdor);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 128, 0, 10, 10, mapcolor_rdor);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 52, string_rdor);

    sprintf(string_bdor, "%d", mapcolor_bdor);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 140, 0, 10, 10, mapcolor_bdor);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 58, string_bdor);

    sprintf(string_ydor, "%d", mapcolor_ydor);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 152, 0, 10, 10, mapcolor_ydor);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 64, string_ydor);

    sprintf(string_tele, "%d", mapcolor_tele);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 164, 0, 10, 10, mapcolor_tele);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 70, string_tele);

    sprintf(string_secr, "%d", mapcolor_secr);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 176, 0, 10, 10, mapcolor_secr);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 76, string_secr);

    sprintf(string_exit, "%d", mapcolor_exit);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 188, 0, 10, 10, mapcolor_exit);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 82, string_exit);

    sprintf(string_unsn, "%d", mapcolor_unsn);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 200, 0, 10, 10, mapcolor_unsn);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 88, string_unsn);

    sprintf(string_flat, "%d", mapcolor_flat);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 212, 0, 10, 10, mapcolor_flat);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 94, string_flat);

    sprintf(string_sprt, "%d", mapcolor_sprt);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 224, 0, 10, 10, mapcolor_sprt);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 100, string_sprt);

    sprintf(string_item, "%d", mapcolor_item);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 236, 0, 10, 10, mapcolor_item);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 106, string_item);

    sprintf(string_enemy, "%d", mapcolor_enemy);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 248, 0, 10, 10, mapcolor_enemy);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 112, string_enemy);

    sprintf(string_hair, "%d", mapcolor_hair);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 260, 0, 10, 10, mapcolor_hair);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 118, string_hair);

    sprintf(string_sngl, "%d", mapcolor_sngl);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 272, 0, 10, 10, mapcolor_sngl);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 124, string_sngl);

    sprintf(string_plyr, "%d", mapcolor_plyr[consoleplayer]);
    V_FillRect(AutomapDef.x + 550, AutomapDef.y + 284, 0, 10, 10, mapcolor_plyr[consoleplayer]);
    M_WriteText(AutomapDef.x + 273, AutomapDef.y + 130, string_plyr);

    dp_translation = crx[CRX_BLUE];

    if (itemOn == 23)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(AutomapDef.x, AutomapDef.y + 136, "reset colors to dos doom standard");

    dp_translation = crx[CRX_BLUE];

    if (itemOn == 24)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(AutomapDef.x, AutomapDef.y + 142, "reset colors to prboom standard");
}

static void M_DrawGame1(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_T_GSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_GMESET",
                                               PU_CACHE), false);

    if (drawgrid)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y - 2, "OFF");
    }

    sprintf(string_mapgridsize, "%d", map_grid_size);
    M_WriteText(GameDef.x + 282 - M_StringWidth(string_mapgridsize), GameDef.y + 8, string_mapgridsize);

    if (am_rotate)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 18, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 18, "OFF");
    }

    if (followplayer)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 28, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 28, "OFF");
    }

    if (overlay_trigger)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 38, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 38, "OFF");
    }

    if (d_statusmap)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 48, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 48, "OFF");
    }

    if (show_stats)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 58, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 58, "OFF");
    }

    if (timer_info)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 68, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 68, "OFF");
    }

    if (show_authors)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 78, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 78, "OFF");
    }

    if (show_title)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 88, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 88, "OFF");
    }

    if (map_secret_after)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 98, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 98, "OFF");
    }

    dp_translation = crx[CRX_GRAY];

    if (itemOn == 11)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef.x, GameDef.y + 108, "AUTOMAP COLORS...");

    if (respawnparm)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef.x + 266, GameDef.y + 118, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef.x + 258, GameDef.y + 118, "OFF");
    }

    if (!whichSkull)
    {
        int x;
        char *string = "";

        if ((itemOn > 5 && itemOn < 10 && d_statusmap && !modifiedgame) ||
            (itemOn > 5 && itemOn < 9 && d_statusmap && modifiedgame))
            string = "YOU NEED TO DISABLE AUTOMAP STATUS BAR FIRST!";
        else if (itemOn == 12)
            string = "YOU MUST START A NEW GAME TO TAKE EFFECT.";

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;
        dp_translation = crx[CRX_GOLD];
        M_WriteText(x, GameDef.y + 138, string);
    }

    if (itemOn == 13)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(GameDef.x, GameDef.y + 128, "MORE OPTIONS...");
}

static void M_DrawGame2(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_T_GSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_GMESET",
                                               PU_CACHE), false);

    if (not_monsters)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y - 2, "OFF");
    }

    if (fastparm)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 8, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 8, "OFF");
    }

    if (d_footstep)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 18, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 18, "OFF");
    }

    if (d_footclip)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 28, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 28, "OFF");
    }

    if (d_splash)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 38, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 38, "OFF");
    }

    if (d_swirl)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 48, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 48, "OFF");
    }

#ifdef WII
    dp_translation = crx[CRX_DARK];

    M_WriteText(GameDef2.x, GameDef2.y + 58, "Show Endoom Screen on quit");
#endif

    if (show_endoom)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 58, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 58, "OFF");
    }

    if (d_flipcorpses)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 68, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 68, "OFF");
    }

    if (d_secrets)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 78, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 78, "OFF");
    }

    if (showMessages)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 88, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 88, "OFF");
    }

    if (gameskill == sk_nightmare)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 10 && gameskill != sk_nightmare)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef2.x, GameDef2.y + 98, "CHAINGUN FIRE RATE");

    if (chaingun_tics == 1)
    {
        dp_translation = crx[CRX_BLUE];
        M_WriteText(GameDef2.x + 242, GameDef2.y + 98, "ULTRA");
    }
    else if (chaingun_tics == 2)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(GameDef2.x + 216, GameDef2.y + 98, "VERY FAST");
    }
    else if (chaingun_tics == 3)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(GameDef2.x + 235, GameDef2.y + 98, "FASTER");
    }
    else if (chaingun_tics == 4)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 233, GameDef2.y + 98, "NORMAL");
    }

    if (d_fallingdamage)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 108, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 108, "OFF");
    }

    if (d_infiniteammo)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef2.x + 266, GameDef2.y + 118, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef2.x + 258, GameDef2.y + 118, "OFF");
    }

    if (!whichSkull)
    {
        int x;
        char *string = "";
        dp_translation = crx[CRX_GOLD];

        if (itemOn == 0 || itemOn == 1)
            string = "YOU MUST START A NEW GAME TO TAKE EFFECT.";
#ifdef WII
        else if (itemOn == 6)
            string = "THIS OPTION IS NOT AVAILABLE FOR THE WII.";
#endif
        else if (itemOn == 10 && gameskill == sk_nightmare)
            string = "NOT AVAILABLE FOR NIGHTMARE SKILL";

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;

        M_WriteText(x, GameDef2.y + 138, string);
    }

    if (itemOn == 13)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(GameDef2.x, GameDef2.y + 128, "MORE & MORE OPTIONS...");
}

static void M_DrawGame3(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_T_GSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_GMESET",
                                               PU_CACHE), false);

    if (crosshair)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y -2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y -2, "OFF");
    }
    
    if (autoaim)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 8, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 8, "OFF");
    }

    if (jumping)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 18, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 18, "OFF");
    }

    if (d_maxgore)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 28, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 28, "OFF");
    }

    if (!d_maxgore)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 4 && d_maxgore)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef3.x, GameDef3.y + 38, "Gore Amount");

    if (gore_amount == 1)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 257, GameDef3.y + 38, "LOW");
    }
    else if (gore_amount == 2)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(GameDef3.x + 236, GameDef3.y + 38, "MEDIUM");
    }
    else if (gore_amount == 3)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(GameDef3.x + 254, GameDef3.y + 38, "HIGH");
    }
    else if (gore_amount == 4)
    {
        dp_translation = crx[CRX_BLUE];
        M_WriteText(GameDef3.x + 174, GameDef3.y + 38, "RIP'EM TO PIECES");
    }

    if (d_colblood)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 48, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 48, "OFF");
    }

    if (d_colblood2)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 58, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 58, "OFF");
    }

    if (!d_maxgore)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 7 && d_maxgore)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef3.x, GameDef3.y + 68, "Max. Number of Bloodsplats");

    sprintf(bloodsplats_buffer, "%d", r_bloodsplats_max);
    M_WriteText(GameDef3.x + 282 - M_StringWidth(bloodsplats_buffer), GameDef3.y + 68, bloodsplats_buffer);

    if (d_shadows)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 78, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 78, "OFF");
    }

    if (d_telefrag)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 88, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 88, "OFF");
    }

    if (d_doorstuck)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 98, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 98, "OFF");
    }

    if (d_resurrectghosts)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 108, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 108, "OFF");
    }

    if (d_limitedghosts)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef3.x + 266, GameDef3.y + 118, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef3.x + 258, GameDef3.y + 118, "OFF");
    }

    if (!whichSkull)
    {
        int x;
        char *string = "";
        dp_translation = crx[CRX_GOLD];

        if (itemOn == 1 && fsize == 12361532)
            string = "NO EXTRA BLOOD & GORE FOR CHEX QUEST.";

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;

        M_WriteText(x, GameDef3.y + 138, string);
    }

    if (itemOn == 13)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(GameDef3.x, GameDef3.y + 128, "EVEN MORE OPTIONS...");
}

static void M_DrawGame4(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_T_GSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_GMESET",
                                               PU_CACHE), false);

    if (!d_blockskulls)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y - 2, "OFF");
    }

    if (!d_blazingsound)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 8, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 8, "OFF");
    }

    if (d_god)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 18, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 18, "OFF");
    }

    if (d_floors)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 28, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 28, "OFF");
    }

    if (d_moveblock)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 38, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 38, "OFF");
    }

    if (d_model)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 48, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 48, "OFF");
    }

    if (d_666)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 58, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 58, "OFF");
    }

    if (correct_lost_soul_bounce)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 68, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 68, "OFF");
    }

    if (d_maskedanim)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 78, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 78, "OFF");
    }

    if (d_sound)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 88, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 88, "OFF");
    }

    if (d_ouchface)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 98, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 98, "OFF");
    }

    if (d_brightmaps)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 108, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 108, "OFF");
    }

    if (d_fixmaperrors)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef4.x + 266, GameDef4.y + 118, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef4.x + 258, GameDef4.y + 118, "OFF");
    }

    if (!whichSkull)
    {
        int x;
        char *string = "";

        dp_translation = crx[CRX_GOLD];

        if (itemOn == 11)
            string = "YOU MUST QUIT AND RESTART TO TAKE EFFECT.";
        else if (itemOn == 12)
            string = "YOU MAY NEED TO RESTART THE MAP FOR THIS.";

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;

        M_WriteText(x, GameDef3.y + 138, string);
    }

    if (itemOn == 13)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(GameDef4.x, GameDef4.y + 128, "ENDLESS ETERNITY...");
}

static void M_DrawGame5(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_T_GSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_GMESET",
                                               PU_CACHE), false);

    if (d_altlighting)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y - 2, "OFF");
    }

    if (allow_infighting)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 8, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 8, "OFF");
    }

    if (last_enemy)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 18, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 18, "OFF");
    }

    if (float_items)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 28, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 28, "OFF");
    }

    if (animated_drop)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 38, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 38, "OFF");
    }

    if (crush_sound)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 48, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 48, "OFF");
    }

    if (disable_noise)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 58, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 58, "OFF");
    }

    if (corpses_nudge)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 68, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 68, "OFF");
    }

    if (corpses_slide)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 78, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 78, "OFF");
    }

    if (!corpses_slide)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 9 && corpses_slide)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef5.x, GameDef5.y + 88, "Corpses smear blood when sliding");

    if (corpses_smearblood)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 88, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 88, "OFF");
    }

    if (randomly_colored_playercorpses)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 98, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 98, "OFF");
    }

    if (lowhealth)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 108, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 108, "OFF");
    }

    if (d_fixspriteoffsets)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef5.x + 266, GameDef5.y + 118, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef5.x + 258, GameDef5.y + 118, "OFF");
    }

    if (itemOn == 13)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(GameDef5.x, GameDef5.y + 128, "IS THIS EVER GOING TO END...?");

    if (!whichSkull)
    {
        int x;
        char *string = "";

        dp_translation = crx[CRX_GOLD];

        if (itemOn == 12)
            string = "YOU NEED TO START A NEW MAP TO TAKE EFFECT.";

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;

        M_WriteText(x, GameDef5.y + 138, string);
    }
}

static void M_DrawGame6(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_T_GSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_GMESET",
                                               PU_CACHE), false);

    if (d_centerweapon)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y - 2, "OFF");
    }

    if (!d_ejectcasings && fsize == 12361532)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 1)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 8, "Eject Weapon Casings");

    if (d_ejectcasings)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y + 8, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y + 8, "OFF");
    }

    if (d_drawparticles)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y + 18, "ON");
    }
    else
    {
        bloodsplat_particle = 0;
        bulletpuff_particle = 0;
        teleport_particle = 0;
        d_drawbfgcloud = false;
        d_drawbfgexplosions = false;
        d_drawrockettrails = false;
        d_drawrocketexplosions = false;
        d_spawnflies = false;
        d_dripblood = false;
        particle_sounds = false;
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y + 18, "OFF");
    }

    if (!d_drawparticles)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 3 && d_drawparticles)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 28, "BLOOD TYPE");

    if (!d_drawparticles)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 4 && d_drawparticles)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 38, "BULLET TYPE");

    if (!d_drawparticles)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 5 && d_drawparticles)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 48, "TELEPORT TYPE");

    if (!d_drawparticles)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 6 && d_drawparticles)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 58, "BFG CLOUD WITH PARTICLES");

    if (!d_drawparticles)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 7 && d_drawparticles)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 68, "BFG Explosion Particles");

    if (!d_drawparticles)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 8 && d_drawparticles)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 78, "ROCKET PARTICLE-TRAILS");

    if (!d_drawparticles)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 9 && d_drawparticles)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 88, "Rocket Explosion Particles");

    if (!d_drawparticles || gamemode != commercial)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 10 && d_drawparticles && gamemode == commercial)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 98, "SPAWN FLIES FOR DEAD CHAINGUNNERS");

    if (!d_drawparticles)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 11 && d_drawparticles)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 108, "Enemies & hangings bleed randomly");

    if (!d_drawparticles)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 12 && d_drawparticles)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef6.x, GameDef6.y + 118, "Particle sounds For Bullets & Flies");

    if (bloodsplat_particle == 0)
    {
        if (!d_drawparticles)
            dp_translation = crx[CRX_DARK];
        else
            dp_translation = crx[CRX_GREEN];

        M_WriteText(GameDef6.x + 232, GameDef6.y + 28, "SPRITES");
    }
    else if (bloodsplat_particle == 1)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(GameDef6.x + 215, GameDef6.y + 28, "PARTICLES");
    }
    else if (bloodsplat_particle == 2)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(GameDef6.x + 250, GameDef6.y + 28, "BOTH");
    }

    if (bulletpuff_particle == 0)
    {
        if (!d_drawparticles)
            dp_translation = crx[CRX_DARK];
        else
            dp_translation = crx[CRX_GREEN];

        M_WriteText(GameDef6.x + 232, GameDef6.y + 38, "SPRITES");
    }
    else if (bulletpuff_particle == 1)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(GameDef6.x + 215, GameDef6.y + 38, "PARTICLES");
    }
    else if (bulletpuff_particle == 2)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(GameDef6.x + 250, GameDef6.y + 38, "BOTH");
    }

    if (teleport_particle == 0)
    {
        if (!d_drawparticles)
            dp_translation = crx[CRX_DARK];
        else
            dp_translation = crx[CRX_GREEN];

        M_WriteText(GameDef6.x + 232, GameDef6.y + 48, "SPRITES");
    }
    else if (teleport_particle == 1)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(GameDef6.x + 215, GameDef6.y + 48, "PARTICLES");
    }
    else if (teleport_particle == 2)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(GameDef6.x + 250, GameDef6.y + 48, "BOTH");
    }

    if (d_drawbfgcloud)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y + 58, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y + 58, "OFF");
    }

    if (d_drawbfgexplosions)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y + 68, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y + 68, "OFF");
    }

    if (d_drawrockettrails)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y + 78, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y + 78, "OFF");
    }

    if (d_drawrocketexplosions)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y + 88, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y + 88, "OFF");
    }

    if (d_spawnflies)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y + 98, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y + 98, "OFF");
    }

    if (d_dripblood)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y + 108, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y + 108, "OFF");
    }

    if (particle_sounds)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef6.x + 266, GameDef6.y + 118, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef6.x + 258, GameDef6.y + 118, "OFF");
    }

    if (!whichSkull)
    {
        int x;
        char *string = "";

        dp_translation = crx[CRX_GOLD];

        if (itemOn == 1 && fsize == 12361532)
            string = "NO WEAPON CASINGS FOR CHEX";

        if (itemOn > 2 && itemOn < 12 && !d_drawparticles)
            string = "YOU MUST ENABLE 'DRAW PARTICLES' FIRST!";

        if (itemOn == 10 && d_drawparticles && (gamemode == retail || gamemode == registered || gamemode == shareware))
            string = "THIS IS ONLY AVAILABLE FOR DOOM 2";

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;

        M_WriteText(x, GameDef6.y + 138, string);
    }

    if (itemOn == 13)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(GameDef6.x, GameDef6.y + 128, "I DON'T THINK SO...");
}

static void M_DrawGame7(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_T_GSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_GMESET",
                                               PU_CACHE), false);

    if (smoketrails)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 266, GameDef7.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y - 2, "OFF");
    }

    if (d_fixwiggle)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 266, GameDef7.y + 8, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 8, "OFF");
    }

    if (remove_slime_trails)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 266, GameDef7.y + 18, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 18, "OFF");
    }

    if ((players[consoleplayer].cheats & CF_GODMODE) || gameskill == sk_nightmare)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 3 && (!(players[consoleplayer].cheats & CF_GODMODE)))
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef7.x, GameDef7.y + 28, "Aimbot for vulnerable player");

    if (aiming_help)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 266, GameDef7.y + 28, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 28, "OFF");
    }

    if (d_thrust)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 266, GameDef7.y + 38, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 38, "OFF");
    }

    if (d_spawnteleglit == 1)
    {
        dp_translation = crx[CRX_RED];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 48, "RED");
    }
    else if (d_spawnteleglit == 2)
    {
        dp_translation = crx[CRX_BLUE];
        M_WriteText(GameDef7.x + 250, GameDef7.y + 48, "BLUE");
    }
    else if (d_spawnteleglit == 3)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(GameDef7.x + 233, GameDef7.y + 48, "RANDOM");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 48, "OFF");
    }

    if (gameskill == sk_nightmare)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 6 && gameskill != sk_nightmare)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(GameDef7.x, GameDef7.y + 58, "WEAPON CHANGE");

    if (use_vanilla_weapon_change)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 250, GameDef7.y + 58, "SLOW");
    }
    else
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 251, GameDef7.y + 58, "FAST");
    }

    if (d_recoil)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 266, GameDef7.y + 68, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 68, "OFF");
    }

    if (hud)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 266, GameDef7.y + 78, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 78, "OFF");
    }

    if (enable_autoload)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 266, GameDef7.y + 88, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 88, "OFF");
    }

    if (enable_autosave)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef7.x + 266, GameDef7.y + 98, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef7.x + 258, GameDef7.y + 98, "OFF");
    }

    sprintf(stillbob_buffer, "%d", stillbob);
    M_WriteText(GameDef7.x + 282 - M_StringWidth(stillbob_buffer), GameDef7.y + 108, stillbob_buffer);

    sprintf(movebob_buffer, "%d", movebob);
    M_WriteText(GameDef7.x + 282 - M_StringWidth(movebob_buffer), GameDef7.y + 118, movebob_buffer);

    if (!whichSkull)
    {
        int x;
        char *string = "";

        dp_translation = crx[CRX_GOLD];

        if (itemOn == 2)
            string = "START / LOAD A NEW GAME TO TAKE EFFECT.";
        else if (itemOn == 3)
        {
            if (players[consoleplayer].cheats & CF_GODMODE)
                string = "YOU NEED TO DISABLE GOD MODE FIRST!";
            else
                string = "TO USE, HOLD DOWN 'X' AFTER BEING ATTACKED.";

            if (gameskill == sk_nightmare)
                string = "NOT AVAILABLE FOR NIGHTMARE SKILL";
        }
        else if (itemOn == 6 && gameskill == sk_nightmare)
            string = "NOT AVAILABLE FOR NIGHTMARE SKILL";
        else if (itemOn == 9 || itemOn == 10)
            string = "YOU NEED TO PICK A QUICKSAVE SLOT FIRST (F6)";
        else if (itemOn == 11)
            string = "DEFAULT VALUE IS 0";
        else if (itemOn == 12)
            string = "DEFAULT VALUE IS 75";

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;

        M_WriteText(x, GameDef7.y + 138, string);
    }

    if (itemOn == 13)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(GameDef7.x, GameDef7.y + 128, "WHATEVER...");
}

static void M_DrawGame8(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_T_GSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(70, 0, 0, W_CacheLumpName("M_GMESET",
                                               PU_CACHE), false);

    if (drawsplash)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef8.x + 266, GameDef8.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef8.x + 258, GameDef8.y - 2, "OFF");
    }

    if (d_fliplevels)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef8.x + 266, GameDef8.y + 8, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef8.x + 258, GameDef8.y + 8, "OFF");
    }

#ifdef WII
    if (beta_style_mode)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(GameDef8.x + 266, GameDef8.y + 18, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(GameDef8.x + 258, GameDef8.y + 18, "OFF");
    }
#endif

    if (!whichSkull)
    {
        int x;
        char *string = "";

        dp_translation = crx[CRX_GOLD];

        if (itemOn == 1)
            string = "YOU NEED TO START A NEW MAP TO TAKE EFFECT.";

#ifdef WII
        if (itemOn == 2)
        {
            if (fsize != 28422764 && fsize != 19321722 && fsize != 12361532)
                string = "YOU MUST QUIT AND RESTART TO TAKE EFFECT.";
            else
                string = "NO BETA MODE FOR CHEX, HACX & FREEDOOM.";
        }
#endif
        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;

        M_WriteText(x, GameDef8.y + 138, string);
    }
}

#ifdef WII
static void M_DrawCheats(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow (110, 6, 0, W_CacheLumpName("M_T_CHTS", PU_CACHE), false);
    else
        V_DrawPatchWithShadow (110, 6, 0, W_CacheLumpName("M_CHEATS", PU_CACHE), false);

    if (players[consoleplayer].cheats & CF_GODMODE)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(223, 26, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(215, 26, "OFF");
    }

    if (players[consoleplayer].cheats & CF_NOCLIP)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(223, 36, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(215, 36, "OFF");
    }

    if (itemOn == 2)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(75, 46, "WEAPONS...");

    if (itemOn == 3)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(75, 56, "KEYS...");

    if (itemOn == 4)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(75, 66, "ARMOR...");

    if (itemOn == 5)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(75, 76, "ITEMS...");

    if (!cheating)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(215, 86, "OFF");
    }
    else if (cheating && cheeting != 2)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(199, 86, "WALLS");
    }
    else if (cheating && cheeting == 2)          
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(215, 86, "ALL");
    }

    if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
        fsize == 4207819 || fsize == 4274218 || fsize == 4225504 || 
        fsize == 4225460 || fsize == 4234124 || fsize == 4196020 ||
        fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
        fsize == 11159840 || fsize == 12408292 || fsize == 12361532 ||
        fsize == 12474561 || fsize == 12538385 || fsize == 12487824)
    {
        if (epi == 0)
            epi = 1;

        if (epi == 1 && map == 0)
            map = 1;
        else if (epi == 4 && map == 10)
            map = 9;

        if (fsize == 12538385 && epi == 1 && map == 10)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, s_HUSTR_E1M10);
        }

        if (epi == 1 && gameversion != exe_chex && map != 10)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map]);
        }
        else if (epi == 2 && gameversion != exe_chex)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map + 9]);
        }
        else if (epi == 3 && gameversion != exe_chex)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map + 18]);
        }
        else if (epi == 4 && gameversion != exe_chex)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(75, 116, maptext[map + 27]);
        }
    }

    if ((fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
        fsize == 14607420 || fsize == 14604584 || fsize == 14677988 ||
        fsize == 14683458 || fsize == 14691821 || fsize == 28422764) &&
        map < 33)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map + 36]);
    }

    if ((fsize == 14677988 || fsize == 14683458 || fsize == 14691821) &&
        map == 33)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, s_HUSTR_33);        
    }

    if (fsize == 18195736 || fsize == 18654796)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map + 68]);
    }

    if (fsize == 18240175 || fsize == 17420824)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map + 100]);
    }

    if (fsize == 19321722)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(75, 116, maptext[map + 142]);
    }

    if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
        fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
        fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
    {
        if (tracknum == 0)
            tracknum = 1;

        if (faketracknum == 0)
            faketracknum = 1;

        dp_translation = crx[CRX_GRAY];
        M_WriteText(220, 156, songtextbeta[tracknum]);
    }
    else if (fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
        fsize == 11159840 || fsize == 12408292 || fsize == 12361532 ||
        fsize == 12474561 || fsize == 12538385 || fsize == 12487824)
    {
        if (tracknum == 0)
            tracknum = 1;

        if (fsize == 12361532)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtextchex[tracknum]);
        }
        else
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtext[tracknum]);
        }
    }
    else if (fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
        fsize == 14607420 || fsize == 14604584 || fsize == 19321722 ||
        fsize == 28422764 || fsize == 14677988 || fsize == 14683458 ||
        fsize == 14691821)
    {
        if (tracknum == 1)
            tracknum = 33;

        if (fsize == 19321722)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtext2hacx[tracknum]);
        }
        else if (fsize == 28422764)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtext2fd[tracknum]);
        }
        else
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(220, 156, songtext2[tracknum]);
        }
    }
    else if (fsize == 18195736 || fsize == 18654796)
    {
        if (tracknum == 1)
            tracknum = 33;

        dp_translation = crx[CRX_GRAY];
        M_WriteText(220, 156, songtexttnt[tracknum]);
    }
    else if (fsize == 18240172 || fsize == 17420824)
    {
        if (tracknum == 1)
            tracknum = 33;

        dp_translation = crx[CRX_GRAY];
        M_WriteText(220, 156, songtextplut[tracknum]);
    }

    if (!whichSkull)
    {
        int x;
        char *string = "";

        dp_translation = crx[CRX_GOLD];

        if (itemOn == 0 && aiming_help)
            string = "IF ENABLED, AIMBOT WILL BE DISABLED!";

        x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;

        M_WriteText(x, CheatsDef.y + 138, string);
    }
}
#endif

/*
static void M_DrawRecord(void)
{
    int offset = 0;
    char buffer_map[2];

    M_snprintf(buffer_map, sizeof(buffer_map), "%d", rmap);

    V_DrawPatchWithShadow(58, 15, 0, W_CacheLumpName("M_T_DREC",
                                               PU_CACHE), false);
    if (rmap == 0)
        rmap = 1;

    if (fsize != 14943400 && fsize != 14824716 && fsize != 14612688 &&
        fsize != 14607420 && fsize != 14604584 && fsize != 18195736 &&
        fsize != 18654796 && fsize != 18240172 && fsize != 17420824 &&
        fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
    {
        if (repi == 1)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "E1M");
        }
        else if (repi == 2)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "E2M");
        }
        else if (repi == 3)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "E3M");
        }
        else if (repi == 4)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "E4M");
        }

        if (repi > 1)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x + 25, RecordDef.y + 8, buffer_map);
        }
        else
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x + 22, RecordDef.y + 8, buffer_map);
        }
    }
    else if (fsize != 12361532)
    {
        if (rmap > 9 && rmap < 20)
            offset = 3;
        else
            offset = 0;

        if (rmap < 10)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "MAP0");
        }
        else if (rmap < 20)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "MAP1");
        }
        else if (rmap < 30)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "MAP2");
        }
        else if (rmap < 40)
        {
            dp_translation = crx[CRX_GRAY];
            M_WriteText(RecordDef.x, RecordDef.y + 8, "MAP3");
        }

        if (fsize != 19321722 && fsize != 28422764)
        {
            if (rmap == 1 || rmap == 11 || rmap == 21 || rmap == 31)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "1");
            }
            else if (rmap == 2 || rmap == 12 || rmap == 22 || rmap == 32)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "2");
            }
            else if (rmap == 3 || rmap == 13 || rmap == 23)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "3");
            }
            else if (rmap == 4 || rmap == 14 || rmap == 24)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "4");
            }
            else if (rmap == 5 || rmap == 15 || rmap == 25)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "5");
            }
            else if (rmap == 6 || rmap == 16 || rmap == 26)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "6");
            }
            else if (rmap == 7 || rmap == 17 || rmap == 27)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "7");
            }
            else if (rmap == 8 || rmap == 18 || rmap == 28)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "8");
            }
            else if (rmap == 9 || rmap == 19 || rmap == 29)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "9");
            }
            else if (rmap == 10 || rmap == 20 || rmap == 30)
            {
                dp_translation = crx[CRX_GRAY];
                M_WriteText(RecordDef.x + 33 - offset, RecordDef.y + 8, "0");
            }
        }
        else
        {
            if (fsize == 28422764)
                offset = 7;

            if (rmap == 1 || rmap == 11 || rmap == 21 || rmap == 31)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM1", PU_CACHE), false);
            else if (rmap == 2 || rmap == 12 || rmap == 22 || rmap == 32)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM2", PU_CACHE), false);
            else if (rmap == 3 || rmap == 13 || rmap == 23)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM3", PU_CACHE), false);
            else if (rmap == 4 || rmap == 14 || rmap == 24)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM4", PU_CACHE), false);
            else if (rmap == 5 || rmap == 15 || rmap == 25)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM5", PU_CACHE), false);
            else if (rmap == 6 || rmap == 16 || rmap == 26)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM6", PU_CACHE), false);
            else if (rmap == 7 || rmap == 17 || rmap == 27)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM7", PU_CACHE), false);
            else if (rmap == 8 || rmap == 18 || rmap == 28)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM8", PU_CACHE), false);
            else if (rmap == 9 || rmap == 19 || rmap == 29)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM9", PU_CACHE), false);
            else if (rmap == 10 || rmap == 20 || rmap == 30)
                V_DrawPatchWithShadow (115 - offset, 68, W_CacheLumpName
                ("WINUM0", PU_CACHE), false);
        }
    }

    if (fsize == 12361532)
    {
        if (repi == 1)
        {
            if (rmap == 1)
                V_DrawPatchWithShadow (55, 68, 0, W_CacheLumpName("WILV00",
                PU_CACHE), false);
            else if (rmap == 2)
                V_DrawPatchWithShadow (55, 68, 0, W_CacheLumpName("WILV01",
                PU_CACHE), false);
            else if (rmap == 3)
                V_DrawPatchWithShadow (55, 68, 0, W_CacheLumpName("WILV02",
                PU_CACHE), false);
            else if (rmap == 4)
                V_DrawPatchWithShadow (55, 68, 0, W_CacheLumpName("WILV03",
                PU_CACHE), false);
            else if (rmap == 5)
                V_DrawPatchWithShadow (55, 68, 0, W_CacheLumpName("WILV04",
                PU_CACHE), false);
        }
    }

    if (rskill == 0)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(RecordDef.x, RecordDef.y + 38, "I'm too young to die.");
    }
    else if (rskill == 1)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(RecordDef.x, RecordDef.y + 38, "Hey, not too rough.");
    }
    else if (rskill == 2)
    {
        dp_translation = crx[CRX_GRAY];

        if (beta_style)
            M_WriteText(RecordDef.x, RecordDef.y + 38, "I JUST WANT TO KILL.");
        else
            M_WriteText(RecordDef.x, RecordDef.y + 38, "Hurt me plenty.");

    }
    else if (rskill == 3)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(RecordDef.x, RecordDef.y + 38, "Ultra-Violence.");
    }
    else if (rskill == 4)
    {
        dp_translation = crx[CRX_GRAY];
        M_WriteText(RecordDef.x, RecordDef.y + 38, "Nightmare!");
    }

    if (long_tics)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(RecordDef.x + 190, RecordDef.y + 58, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(RecordDef.x + 182, RecordDef.y + 58, "OFF");
    }
}
*/

static void M_Options(int choice)
{
    M_SetupNextMenu(&OptionsDef);
}

//
// Toggle messages on/off
//
static void M_ChangeMessages(int choice)
{
    blurred = false;

    switch (choice)
    {
        case 0:
            if (showMessages)
                showMessages = false;

            break;

        case 1:
            if (!showMessages)
                showMessages = true;

            break;
    }

    message_dontfuckwithme = true;
}

//
// M_EndGame
//
static void M_EndGameResponse(int ch)
{
#ifdef WII
    if (ch != key_menu_forward)
#else
    if (ch != key_menu_confirm)
#endif
        return;

    // [crispy] killough 5/26/98: make endgame quit if recording or playing back demo
/*
    if (demorecording || singledemo)
        G_CheckDemoStatus();
*/
    currentMenu->lastOn = itemOn;
    M_ClearMenus();
    D_StartTitle();
}

static void M_EndGame(int choice)
{
/*
    V_DrawPatchWithShadow(58, 15, 0, W_CacheLumpName("M_T_EGME",
                                               PU_CACHE), false);
*/
    if (!usergame)
    {
        S_StartSound(NULL, sfx_oof);
        return;
    }

    if (netgame)
    {
        M_StartMessage(s_NETEND, NULL, false);
        return;
    }

    M_StartMessage(s_ENDGAME, M_EndGameResponse, true);
}

//
// M_ReadThis
//
static void M_ReadThis(int choice)
{
    M_SetupNextMenu(&ReadDef1);
}

static void M_ReadThis2(int choice)
{
    // Doom 1.9 had two menus when playing Doom 1
    // All others had only one
    if (gameversion <= exe_doom_1_9 && gamemode != commercial)
    {
        M_SetupNextMenu(&ReadDef2);
    }
    else
    {
        // Close the menu
        M_FinishReadThis(0);
    }
}

static void M_FinishReadThis(int choice)
{
    M_SetupNextMenu(&MainDef);
}

static void M_Credits(int choice)
{
    credits_start_time = gametic;
    draw_ended = false;

    oldscreenblocks = screenblocks;
    oldscreenSize = screenSize;
    screenblocks = 11;
    screenSize = 8;
    R_SetViewSize(screenblocks);
    M_SetupNextMenu(&CreditsDef);
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

static void M_QuitResponse(int ch)
{
#ifdef WII
    if (ch != key_menu_forward)
#else
    if (ch != key_menu_confirm)
#endif
        return;

    if (!netgame)
    {
        if (gamemode == commercial)
        {
            if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
                S_StartSound(NULL, quitsounds2[(gametic >> 2) & 7]);
            else
                S_StartSound(NULL, quitsounds3[(gametic >> 2) & 7]);
        }
        else
            S_StartSound(NULL, quitsounds[(gametic >> 2) & 7]);

        I_WaitVBL(105);
    }

    I_Quit();
}

static char *M_SelectEndMessage(void)
{
    return *endmsg[M_Random() % NUM_QUITMESSAGES + (gamemission != doom) * NUM_QUITMESSAGES];
}

void M_QuitDOOM(int choice)
{
    if (splashscreen)
        return;

    if (!beta_style)
    {
        sprintf(endstring, "%s\n\n" DOSY, M_SelectEndMessage());
        M_StartMessage(endstring, M_QuitResponse, true);
    }
    else
        I_Quit();
}

static void M_ChangeSensitivity(int choice)
{
    switch (choice)
    {
        case 0:
            if (mouseSensitivity)
                mouseSensitivity--;

            break;

        case 1:
            if (mouseSensitivity < 9)
                mouseSensitivity++;

            break;
    }
}

static void M_MouseWalk(int choice)
{
    switch (choice)
    {
        case 0:
            if (mousewalk)
                mousewalk = false;

            break;

        case 1:
            if (!mousewalk)
                mousewalk = true;

            if (mousewalk)
                mouselook = false;

            break;
    }
}

static void M_WalkingSpeed(int choice)
{
    switch (choice)
    {
        case 0:
            if (forwardmove > 25)
                forwardmove--;

            break;

        case 1:
            if (forwardmove < 50)
                forwardmove++;

            break;
    }
}

static void M_TurningSpeed(int choice)
{
    switch (choice)
    {
        case 0:
            if (turnspeed > 5)
                turnspeed--;

            break;

        case 1:
            if (turnspeed < 10)
                turnspeed++;

            break;
    }
}

static void M_StrafingSpeed(int choice)
{
    switch (choice)
    {
        case 0:
            if (sidemove > 24)
                sidemove--;

            break;

        case 1:
            if (sidemove < 40)
                sidemove++;

            break;
    }
}

static void M_ChangeDetail(int choice)
{
    blurred = false;

    switch (choice)
    {
        case 0:
            if (!detailLevel)
                detailLevel = true;

            break;

        case 1:
            if (detailLevel)
                detailLevel = false;

            break;
    }

    d_lowpixelsize = "4x4";
    GetPixelSize();

    if (!detailLevel)
        players[consoleplayer].message = s_DETAILHI;
    else
        players[consoleplayer].message = s_DETAILLO;
}

static void M_Translucency(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_translucency)
                d_translucency = false;

            break;

        case 1:
            if (!d_translucency)
                d_translucency = true;

            break;
    }

    R_InitColumnFunctions();
}

static void M_ColoredBloodA(int choice)
{
    if (!d_chkblood)
        return;

    switch (choice)
    {
        case 0:
            if (d_colblood)
                d_colblood = false;

            break;

        case 1:
            if (!d_colblood)
                d_colblood = true;

            break;
    }
}

static void M_ColoredBloodB(int choice)
{
    if (!d_chkblood2)
        return;

    switch (choice)
    {
        case 0:
            if (d_colblood2)
                d_colblood2 = false;

            break;

        case 1:
            if (!d_colblood2)
                d_colblood2 = true;

            break;
    }
}

static void M_BloodsplatsAmount(int choice)
{
    if (!d_maxgore)
        return;

    switch (choice)
    {
        case 0:
            if (r_bloodsplats_max > 0)
            {
                r_bloodsplats_max--;

                if (r_bloodsplats_max < r_bloodsplats_max_max - 99 && r_bloodsplats_max > r_bloodsplats_max_max - 900)
                    r_bloodsplats_max -= 19;
                else if (r_bloodsplats_max < r_bloodsplats_max_max - 899 && r_bloodsplats_max > r_bloodsplats_max_max - 5000)
                    r_bloodsplats_max -= 99;
                else if (r_bloodsplats_max < r_bloodsplats_max_max - 4999)
                    r_bloodsplats_max -= 499;
            }

            break;

        case 1:
            if (r_bloodsplats_max < r_bloodsplats_max_max)
            {
                r_bloodsplats_max++;

                if (r_bloodsplats_max < r_bloodsplats_max_max - 99 && r_bloodsplats_max > r_bloodsplats_max_max - 900)
                    r_bloodsplats_max += 19;
                else if (r_bloodsplats_max < r_bloodsplats_max_max - 899 && r_bloodsplats_max > r_bloodsplats_max_max - 5000)
                    r_bloodsplats_max += 99;
                else if (r_bloodsplats_max < r_bloodsplats_max_max - 4999)
                    r_bloodsplats_max += 499;
            }

            break;
    }

    if (r_bloodsplats_max < 0)
        r_bloodsplats_max = 0;
    else if (r_bloodsplats_max > r_bloodsplats_max_max)
        r_bloodsplats_max = r_bloodsplats_max_max;
}

static void M_WipeType(int choice)
{
    switch (choice)
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

static void M_UncappedFramerate(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_uncappedframerate > 0)
                d_uncappedframerate = (d_uncappedframerate - 1) % NUM_UNCAPPED;

            if (d_vsync)
            {
                if (d_uncappedframerate == 1)
                    d_uncappedframerate--;
            }

            break;

        case 1:
            if (d_uncappedframerate < 3)
                d_uncappedframerate = (d_uncappedframerate + 1) % NUM_UNCAPPED;

            if (d_vsync)
            {
                if (d_uncappedframerate == 1)
                    d_uncappedframerate++;
                else if (d_uncappedframerate == 3)
                    d_uncappedframerate = 2;
            }

            break;
    }
}

static void M_Screenshots(int choice)
{
    switch (choice)
    {
        case 0:
            if (png_screenshots)
                png_screenshots = false;

            break;

        case 1:
            if (!png_screenshots)
                png_screenshots = true;

            break;
    }
}

static void M_Background(int choice)
{
    switch (choice)
    {
        case 0:
            if (background_type > 0)
                background_type--;

            break;

        case 1:
            if (background_type < 4)
                background_type++;

            break;
    }
}

static void M_BackgroundColor(int choice)
{
    if (background_type == 4)
    {
        switch (choice)
        {
            case 0:
                if (background_color > 0)
                    background_color--;

                break;

            case 1:
                if (background_color < 256)
                    background_color++;

                break;
        }
    }
}

static void M_FontShadow(int choice)
{
    switch (choice)
    {
        case 0:
            if (font_shadow > 0)
                font_shadow--;

            break;

        case 1:
            if (font_shadow < 2)
                font_shadow++;

            break;
    }
}

static void M_DiskIcon(int choice)
{
    switch (choice)
    {
        case 0:
            if (show_diskicon)
                show_diskicon = false;

            break;

        case 1:
            if (!show_diskicon)
                show_diskicon = true;

            break;
    }
}

static void M_FixWiggle(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_fixwiggle)
                d_fixwiggle = false;

            break;

        case 1:
            if (!d_fixwiggle)
                d_fixwiggle = true;

            break;
    }
}

static void M_RemoveSlimeTrails(int choice)
{
    switch (choice)
    {
        case 0:
            if (remove_slime_trails)
                remove_slime_trails = false;

            break;

        case 1:
            if (!remove_slime_trails)
                remove_slime_trails = true;

            break;
    }
}

#ifdef SDL2
static void M_RenderMode(int choice)
{
    switch (choice)
    {
        case 0:
            if (render_mode)
                render_mode = false;

            break;

        case 1:
            if (!render_mode)
                render_mode = true;

            break;
    }
}

static void M_VSync(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_vsync)
                d_vsync = false;

            break;

        case 1:
            if (!d_vsync)
                d_vsync = true;

            if (d_uncappedframerate == 3 || d_uncappedframerate == 1)
                d_uncappedframerate--;

            break;
    }
}
#endif

static void M_IconType(int choice)
{
    if ((gamemode == commercial || fsize == 4234124 || fsize == 4196020 ||
        fsize == 12474561 || fsize == 12487824 || fsize == 11159840 ||
        fsize == 12408292 || fsize == 12538385 || fsize == 12361532 ||
        fsize == 7585664) && show_diskicon)
    {
        switch (choice)
        {
            case 0:
                if (icontype)
                    icontype = false;

                break;

            case 1:
                if (!icontype)
                    icontype = true;

                break;
        }
    }
}

static void M_SizeDisplay(int choice)
{
    if (!am_overlay)
    {
        switch (choice)
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

        R_SetViewSize (screenblocks);

        if (screenSize < 8)
        {
            if (usergame)
                ST_doRefresh();
        }

        blurred = false;
        skippsprinterp = true;
        oldscreenSize = screenSize;
        oldscreenblocks = screenblocks;
    }
}

//
// Menu Functions
//
// [nitr8] UNUSED
//
/*
static void M_DrawThermo(int x, int y, int thermWidth, int thermDot)
{
    int                xx = x;
    int                i;

    V_DrawPatchWithShadow(xx, y, 0, W_CacheLumpName("M_THERML", PU_CACHE), false);
    xx += 8;

    for (i = 0; i < thermWidth; i++)
    {
        V_DrawPatchWithShadow(xx, y, 0, W_CacheLumpName("M_THERMM", PU_CACHE), false);
        xx += 8;
    }

    V_DrawPatchWithShadow(xx, y, 0, W_CacheLumpName("M_THERMR", PU_CACHE), false);

    V_DrawPatchWithShadow((x + 8) + thermDot * 8, y, 0, W_CacheLumpName("M_THERMO",
                      PU_CACHE), false);
}
*/

static void M_DrawThermoSmall(int x, int y, int thermWidth, int thermDot)
{
    int         xx = x;
    int         yy = y + 6;
    int         i;

    // +6 to y coordinate
    V_DrawPatchWithShadow(xx + 3, yy - 18, 0, W_CacheLumpName("M_SLIDEL", PU_CACHE), false);
    xx += 8;

    for (i = 0; i < thermWidth; i++)
    {
        V_DrawPatchWithShadow(xx, yy - 18, 0, W_CacheLumpName("M_SLIDEM", PU_CACHE), false);
        xx += 8;
    }

    V_DrawPatchWithShadow(xx, yy - 18, 0, W_CacheLumpName("M_SLIDER", PU_CACHE), false);

    // +2 to initial y coordinate
    V_DrawPatchWithShadow((x + 9) + thermDot * 8, y - 12, 0, W_CacheLumpName("M_SLIDEO",
                     PU_CACHE), false);
}

//
// [nitr8] UNUSED
//
/*
static void M_DrawEmptyCell(menu_t *menu, int item)
{
    V_DrawPatchWithShadow(menu->x - 10,
                          menu->y + item * LINEHEIGHT_SMALL - 1, 0, W_CacheLumpName("M_CELL1",
                          PU_CACHE), false);
}

static void M_DrawSelCell(menu_t *menu, int item)
{
    V_DrawPatchWithShadow(menu->x - 10,
                          menu->y + item * LINEHEIGHT_SMALL - 1, 0, W_CacheLumpName("M_CELL2",
                          PU_CACHE), false);
}
*/

static void M_StartMessage(char *string, void *routine, dboolean input)
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageString = string;
    messageRoutine = routine;
    messageNeedsInput = input;

    blurred = false;
    menuactive = true;

    // [crispy] entering menus while recording demos pauses the game
/*
    if (demorecording && !paused)
    {
        sendpause = true;
    }
*/
    return;
}

//
// [nitr8] UNUSED
//
/*
static void M_StopMessage(void)
{
    menuactive = messageLastMenuActive;
    messageToPrint = 0;
}
*/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

//
// Find string width from hu_font chars
//
int M_StringWidth(char *string)
{
    size_t          i;
    int             w = 0;
        
    for (i = 0; i < strlen(string); i++)
    {
        int c = toupper(string[i]) - HU_FONTSTART;

        if (c < 0 || c >= HU_FONTSIZE)
            w += 4;
        else
            w += SHORT(hu_font[c]->width);
    }
                
    return w;
}

//
// Find string height from hu_font chars
//
int M_StringHeight(char *string)
{
    size_t          i;
    int             height = SHORT(hu_font[0]->height);
    int             h = height;
        
    for (i = 0; i < strlen(string); i++)
        if (string[i] == '\n')
            h += height;

    return h;
}

//
// Write a string using the hu_font
//
void M_WriteText(int x, int y, char *string)
{
    char  *ch = string;
    int   cx = x;
    int   cy = y;
    int   w;

    while (1)
    {
        int c = *ch++;

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
            dp_translation = crx[(int)(c - '0')];
            continue;
        }
                
        c = toupper(c) - HU_FONTSTART;

        if (c < 0 || c >= HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }

        w = SHORT(hu_font[c]->width);

        if (cx + w > ORIGINALWIDTH)
            break;

        if (dp_translation && font_shadow != 2)
            V_ClearDPTranslation();

        if (font_shadow == 1)
            V_DrawPatchWithShadow(cx, cy, 0, hu_font[c], false);
        else
            V_DrawPatch(cx, cy, 0, hu_font[c]);

        cx += w;
    }

    V_ClearDPTranslation();
}

// These keys evaluate to a "null" key in Vanilla Doom that allows weird
// jumping in the menus. Preserve this behavior for accuracy.

#ifndef WII
static dboolean IsNullKey(int key)
{
    return key == KEY_PAUSE || key == KEY_CAPSLOCK
        || key == KEY_SCRLCK || key == KEY_NUMLOCK;
}
#endif

static void M_ChangeGamma(int choice)
{
    static int  gammawait;

    if (gammawait >= I_GetTime() || gamestate != GS_LEVEL || inhelpscreens)
    {
        switch (choice)
        {
            case 0:
                if (usegamma)
                    usegamma--;

                break;

            case 1:
                if (usegamma < GAMMALEVELS - 1)
                    usegamma++;

                break;
        }

        r_gamma = gammalevels[usegamma];

        S_StartSound(NULL, sfx_stnmov);
    }

    gammawait = I_GetTime() + HU_MSGTIMEOUT;
    players[consoleplayer].message = gammamsg[usegamma];
    I_SetPalette (W_CacheLumpName("PLAYPAL", PU_CACHE));
}

//
// CONTROL PANEL
//

//
// M_Responder
//
dboolean M_Responder(event_t *ev)
{
    int             ch = -1;
    int             key;

#ifndef WII
    static int      mousewait = 0;
#endif

#ifdef WII
    WPADData *data = WPAD_Data(0);

    // Process joystick input
    // For some reason, polling ev.data for joystick input here in the menu code
    // doesn't work when using the twilight hack to launch wiidoom. At the same
    // time, it works fine if you're using the homebrew channel. I don't know
    // why this is so for the meantime I'm polling the wii remote directly.

    // Classic Controls
    if (data->exp.type == WPAD_EXP_CLASSIC)
    {
        if (data->btns_d & WPAD_CLASSIC_BUTTON_UP)
        {
            ch = key_menu_up;
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_DOWN)
        {
            ch = key_menu_down;
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_LEFT)
        {
            ch = key_menu_left;
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_RIGHT)
        {
            ch = key_menu_right;
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_B)
        {
            ch = key_menu_forward;
        }

        if (data->btns_d & WPAD_CLASSIC_BUTTON_A)
        {
            ch = key_menu_back;
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

    // KEY BINDINGS
    if (askforkey && data->btns_d)
#else
    if (askforkey && ev->type == ev_keydown)
#endif
    {
        M_KeyBindingsClearControls(ev->data1);

        *doom_defaults_list[keyaskedfor + key_controls_start_in_cfg_at_pos + FirstKey].location.i =
                ev->data1;

        askforkey = false;
        return true;
    }
/*
    // FIXME: NOT YET WORKING (MOUSE BINDINGS)
    if (askforkey && ev->type == ev_mouse)
    {
        if ((ev->data1 & 1) ||
            (ev->data1 & 2) ||
            (ev->data1 & 4) ||
            (ev->data1 & 8) ||
            (ev->data1 & 16))
        {
            M_KeyBindingsClearControls(ev->data1);

            *doom_defaults_list[keyaskedfor + key_controls_start_in_cfg_at_pos + FirstKey].location.i =
                    ev->data1;

            askforkey = false;
            return true;
        }

        return false;
    }
    else if (askforkey && ev->type == ev_mousewheel)
    {
        if ((ev->data1 & 1) || (ev->data1 & -1))
        {
            M_KeyBindingsClearControls(ev->data1);

            *doom_defaults_list[keyaskedfor + key_controls_start_in_cfg_at_pos + FirstKey].location.i =
                    ev->data1;

            askforkey = false;
            return true;
        }

        return false;
    }
*/

#ifndef WII
    // "close" button pressed on window?
    if (ev->type == ev_quit)
    {
        // First click on close button = bring up quit confirm message.
        // Second click on close button = confirm quit
        if (menuactive && messageToPrint && messageRoutine == M_QuitResponse)
        {
            M_QuitResponse(key_menu_confirm);
        }
        else
        {
            S_StartSound(NULL, sfx_swtchn);
            M_QuitDOOM(0);
        }

        return true;
    }

    // key is the key pressed, ch is the actual character typed
    ch = 0;
#endif

    key = -1;

#ifndef WII
    if (ev->type == ev_keydown)
    {
        key = ev->data1;
        ch = ev->data2;
    }
    else if (ev->type == ev_mouse && mousewait < I_GetTime() && menuactive)
    {
        // activate menu item
        if (ev->data1 & MOUSE_LEFTBUTTON)
        {
            key = KEY_ENTER;
            mousewait = I_GetTime() + 5;
        }
        // previous menu
        else if (ev->data1 & MOUSE_RIGHTBUTTON)
        {
            key = KEY_ESCAPE;
            mousewait = I_GetTime() + 5;
        }
    }
    else if (ev->type == ev_mousewheel)
    {
        if (!messageToPrint)
        {
            // select previous menu item
            if (ev->data1 > 0)
            {
                key = KEY_UPARROW;
                mousewait = I_GetTime() + 3;
            }
            // select next menu item
            else if (ev->data1 < 0)
            {
                key = KEY_DOWNARROW;
                mousewait = I_GetTime() + 3;
            }
        }
    }

    if (key == -1)
        return false;
#endif

    // Save Game string input
    if (saveStringEnter)
    {
#ifdef WII
        switch (ch)
#else
        switch (key)
#endif
        {
            case KEY_ESCAPE:
                saveStringEnter = 0;
                M_StringCopy(&savegamestrings[saveSlot][0], saveOldString, sizeof(savegamestrings));
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
#ifdef WII
                ch = key;
#endif
                ch = toupper(ch);

                if (ch != ' '
                   && (ch - HU_FONTSTART < 0 || ch - HU_FONTSTART >= HU_FONTSIZE))
                {
                    break;
                }

                if (ch >= 32 && ch <= 127 &&
                    saveCharIndex < SAVESTRINGSIZE - 1 &&
                    M_StringWidth(savegamestrings[saveSlot]) <
                    (SAVESTRINGSIZE - 2) * 8)
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
#ifdef WII
            if (!(ch == key_menu_confirm || ch == key_menu_forward ||
                  ch == key_menu_activate))
#else
            if (key != ' ' && key != KEY_ESCAPE
                && key != key_menu_confirm && key != key_menu_abort)
#endif
            {
                return false;
            }
        }

        menuactive = messageLastMenuActive;
        messageToPrint = 0;

        if (messageRoutine)
            messageRoutine(ch);

        menuactive = false;
        S_StartSound(NULL, sfx_swtchx);
        return true;
    }

    if ((devparm && key == key_menu_help) ||
        (key != 0 && key == key_menu_screenshot && !beta_style) ||
        (key != 0 && key == key_menu_screenshot_beta && beta_style))
    {
        G_ScreenShot();
        return true;
    }

#ifndef WII
    // F-Keys
    if (!menuactive && !beta_style)
    {
        // Screen size down
        if (key == key_menu_decscreen)
        {
            if (automapactive || chat_on)
                return false;

            M_SizeDisplay(0);
            S_StartSound(NULL, sfx_stnmov);
            return true;
        }
        // Console
        else if (key == KEY_TILDE)
        {
            if (consoleheight < CONSOLEHEIGHT && consoledirection == -1 && !inhelpscreens && !wipe)
            {
                consoleheight = MAX(1, consoleheight);
                consoledirection = 1;
                return true;
            }

            return false;
        }
        // Screen size up
        else if (key == key_menu_incscreen)
        {
            if (automapactive || chat_on)
                return false;

            M_SizeDisplay(1);
            S_StartSound(NULL, sfx_stnmov);
            return true;
        }
        // Help key
        else if (key == key_menu_help)
        {
            M_StartControlPanel();

            if (gamemode == retail)
                currentMenu = &ReadDef2;
            else
                currentMenu = &ReadDef1;

            itemOn = 0;
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }
        // Save
        else if (key == key_menu_save)
        {
            M_StartControlPanel();
            S_StartSound(NULL, sfx_swtchn);
            M_SaveGame(0);
            return true;
        }
        // Load
        else if (key == key_menu_load)
        {
            // [crispy] forbid New Game and (Quick) Load while recording a demo
/*
            if (demorecording)
            {
                S_StartSound(NULL, sfx_oof);
            }
            else
*/
            {
                M_StartControlPanel();
                S_StartSound(NULL, sfx_swtchn);
                M_LoadGame(0);
                return true;
            }
        }
        // Sound Volume
        else if (key == key_menu_volume)
        {
            M_StartControlPanel();
            currentMenu = &SoundDef;
            itemOn = sfx_vol;
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }
        // Detail toggle
        else if (key == key_menu_detail)
        {
            M_ChangeDetail(0);
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }
        // Quicksave
        else if (key == key_menu_qsave)
        {
            S_StartSound(NULL, sfx_swtchn);
            M_QuickSave();
            return true;
        }
        // End game
        else if (key == key_menu_endgame)
        {
            S_StartSound(NULL, sfx_swtchn);
            M_EndGame(0);
            return true;
        }
        // Toggle messages
        else if (key == key_menu_messages)
        {
            M_ChangeMessages(0);
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }
        // Quickload
        else if (key == key_menu_qload)
        {
            // [crispy] forbid New Game and (Quick) Load while recording a demo
/*
            if (demorecording)
            {
                S_StartSound(NULL, sfx_oof);
            }
            else
*/
            {
                S_StartSound(NULL, sfx_swtchn);
                M_QuickLoad();
                return true;
            }
        }
        // Quit DOOM
        else if (key == key_menu_quit)
        {
            S_StartSound(NULL, sfx_swtchn);
            M_QuitDOOM(0);
            return true;
        }
        // gamma toggle
        else if (key == key_menu_gamma)
        {
            if (usergame)
            {
                if (++usegamma > GAMMALEVELS - 1)
                    usegamma = 0;

                M_ChangeGamma(1);
                return false;
            }
        }
    }
#endif

    // Pop-up menu?
    if (!menuactive)
    {
#ifdef WII
        if (ch == key_menu_activate)
#else
        if (key == key_menu_activate)
#endif
        {
            if (consoleactive && consoleheight != 0)
                return false;

            M_StartControlPanel();
            S_StartSound(NULL, sfx_swtchn);
            return true;
        }

        return false;
    }

    // Keys usable within menu
#ifdef WII
    if (ch == key_menu_down)
#else
    if (key == key_menu_down)
#endif
    {
        // Move down to next item
        do
        {
            if (itemOn + 1 > currentMenu->numitems - 1)
            {
                // FOR PSP (if too many menu items)
                if (FirstKey == FIRSTKEY_MAX)
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
#ifndef WII
            if (currentMenu == &KeyBindingsDef && itemOn == 12)
                itemOn++;
#else
            if (currentMenu == &ControlsDef && itemOn == 5)
                itemOn += 2;
#endif
            if (!devparm)
            {
                if (currentMenu == &GameDef && itemOn == 14)
                    itemOn = 0;
            }

            if (background_type == 3 && currentMenu != &MainDef
                && currentMenu != &EpiDef && currentMenu != &NewDef
                && currentMenu != &ExpDef)
                S_StartSound(NULL, sfx_itemup);
            else
                S_StartSound(NULL, sfx_pstop);

        } while ((currentMenu->menuitems[itemOn].status == -1 && !devparm) ||
                 (currentMenu->menuitems[itemOn].status == -1 && currentMenu != &GameDef && devparm));

        return true;
    }
#ifdef WII
    else if (ch == key_menu_up)
#else
    else if (key == key_menu_up)
#endif
    {
        // Move back up to previous item
        do
        {
            if (!itemOn)
            {
                // FOR PSP (if too many menu items)
                if (FirstKey == 0)
                {
                    itemOn = currentMenu->numitems - 1;
                    FirstKey = FIRSTKEY_MAX;
                }
                else
                {
                    FirstKey--;
                }
            }
            else
                itemOn--;
#ifndef WII
            if (currentMenu == &KeyBindingsDef && itemOn == 12)
                itemOn--;
#else
            if (currentMenu == &ControlsDef && itemOn == 6)
                itemOn -= 2;
#endif
            if (!devparm)
            {
                if (currentMenu == &GameDef && itemOn == 14)
                    itemOn--;
            }

            if (background_type == 3 && currentMenu != &MainDef
                && currentMenu != &EpiDef && currentMenu != &NewDef
                && currentMenu != &ExpDef)
                S_StartSound(NULL, sfx_itemup);
            else
                S_StartSound(NULL, sfx_pstop);

        } while ((currentMenu->menuitems[itemOn].status == -1 && !devparm) ||
                 (currentMenu->menuitems[itemOn].status == -1 && currentMenu != &GameDef && devparm));

        return true;
    }
#ifdef WII
    else if (ch == key_menu_left)
#else
    else if (key == key_menu_left)
#endif
    {
        // Slide slider left
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(0);
        }

        return true;
    }
#ifdef WII
    else if (ch == key_menu_right)
#else
    else if (key == key_menu_right)
#endif
    {
        // Slide slider right
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status == 2)
        {
            S_StartSound(NULL, sfx_stnmov);
            currentMenu->menuitems[itemOn].routine(1);
        }

        return true;
    }
#ifdef WII
    else if (ch == key_menu_forward)
#else
    else if (key == key_menu_forward)
#endif
    {
        // Activate menu item
        if (currentMenu->menuitems[itemOn].routine &&
            currentMenu->menuitems[itemOn].status)
        {
            currentMenu->lastOn = itemOn;
            if (currentMenu->menuitems[itemOn].status == 2)
            {
                // right arrow
                currentMenu->menuitems[itemOn].routine(1);
                S_StartSound(NULL, sfx_stnmov);
            }
            else
            {
                currentMenu->menuitems[itemOn].routine(itemOn);
                S_StartSound(NULL, sfx_pistol);
            }
        }
        else if (currentMenu == &GameDef && !devparm && itemOn == 13)
        {
            S_StartSound(NULL, sfx_pistol);
            M_Game2(ch);
        }

        return true;
    }
#ifdef WII
    else if (ch == key_menu_activate)
#else
    else if (key == key_menu_activate)
#endif
    {
        // Deactivate menu
        currentMenu->lastOn = itemOn;
        M_ClearMenus();
        S_StartSound(NULL, sfx_swtchx);
        return true;
    }
#ifdef WII
    else if (ch == key_menu_back)
#else
    else if (key == key_menu_back)
#endif
    {
        // Go back to previous menu
        currentMenu->lastOn = itemOn;
        if (currentMenu->prevMenu)
        {
            if (nerve_pwad && currentMenu == &NewDef)
                currentMenu->prevMenu = &ExpDef;

            currentMenu = currentMenu->prevMenu;
            itemOn = currentMenu->lastOn;
            S_StartSound(NULL, sfx_swtchn);
        }

        return true;
    }

    // Keyboard shortcut?
    // Vanilla Doom has a weird behavior where it jumps to the scroll bars
    // when the certain keys are pressed, so emulate this.
#ifdef WII
    else if (ch != 0)
#else
    else if (ch != 0 || IsNullKey(key))
#endif
    {
        int             i;

        for (i = itemOn + 1; i < currentMenu->numitems; i++)
        {
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(NULL, sfx_pstop);
                return true;
            }
        }

        for (i = 0; i <= itemOn; i++)
        {
            if (currentMenu->menuitems[i].alphaKey == ch)
            {
                itemOn = i;
                S_StartSound(NULL, sfx_pstop);
                return true;
            }
        }
    }

    return false;
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
    // intro might call this repeatedly
    if (menuactive)
        return;

    if (splashscreen)
    {
        if (pagetic < 95)
            pagetic = MIN(10, pagetic);

        return;
    }

    menuactive = 1;

    if (!beta_style)
        // JDC
        currentMenu = &MainDef;
    else
        currentMenu = &BetaMainDef;

    // JDC
    itemOn = currentMenu->lastOn;

    blurred = false;
}

// Display OPL debug messages - hack for GENMIDI development.
static void M_DrawOPLDev(void)
{
    char debug[1024];
    char *curr, *p;
    int line = 0;

    I_OPL_DevMessages(debug, sizeof(debug));
    curr = debug;

    for (;;)
    {
        p = strchr(curr, '\n');

        if (p != NULL)
        {
            *p = '\0';
        }

        M_WriteText(0, line * 8, curr);
        ++line;

        if (p == NULL)
        {
            break;
        }

        curr = p + 1;
    }
}

// [crispy] crispness menu
static void M_DrawCrispnessBackground(void)
{
    static byte *sdest;

    if (!sdest)
    {
        byte *src = W_CacheLumpName("FLOOR4_6", PU_CACHE);
        byte *dest = Z_Malloc(SCREENWIDTH * SCREENHEIGHT * sizeof(*dest), PU_STATIC, NULL);
        int x, y;

        sdest = dest;

        for (y = 0; y < SCREENHEIGHT; y++)
        {
            for (x = 0; x < SCREENWIDTH; x++)
            {
                *dest++ = src[(y & 63) * 64 + (x & 63)];
            }
        }
    }

    memcpy(screens[0], sdest, SCREENWIDTH * SCREENHEIGHT * sizeof(*screens[0]));
}

extern int                      free_memory;
extern int                      active_memory;
extern int                      purgable_memory;

// Print allocation statistics
void DrawStats(void)
{
    char act_mem[50];
    char pur_mem[50];
    char free_mem[50];
    char tot_mem[50];

    double s;

    unsigned long total_memory;

    if (gamestate != GS_LEVEL)
        return;

    total_memory = (DEFAULT_RAM * 1024 * 1024);

    s = 100.0 / total_memory;

    free_memory = total_memory - active_memory - purgable_memory;

    sprintf(act_mem, "%-5i\t%6.01f%%\tstatic\n", active_memory, active_memory * s);
    sprintf(pur_mem, "%-5i\t%6.01f%%\tpurgable\n", purgable_memory, purgable_memory * s);
    sprintf(free_mem, "%-5i\t%6.01f%%\tfree\n", free_memory, free_memory * s);
    sprintf(tot_mem, "%-5lu\t\ttotal\n", total_memory);

    if (leveltime & 16)
        M_WriteText(0, 10, "Memory Heap Info\n");

    M_WriteText(0, 20, act_mem);
    M_WriteText(0, 30, pur_mem);
    M_WriteText(0, 40, free_mem);
    M_WriteText(0, 60, tot_mem);
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
    static short        x;
    static short        y;
    unsigned int        i;
    unsigned int        max;

    if (menuactive)
    {
        if (background_type == 2)
            M_DarkBackground();
        else if (background_type == 3)
            M_DrawCrispnessBackground();
        else if (background_type == 4)
            V_DimScreen();
    }

    if (!inhelpscreens)
    {
        if (!s_randommusic)
        {
            int mnum;

            if (gamemode == commercial)
            {
                if (gamestate == GS_LEVEL)
                {
                    if (gamemission == pack_nerve)
                    {
                        int nmus[]=
                        {
                            mus_messag,
                            mus_ddtblu,
                            mus_doom,
                            mus_shawn,
                            mus_in_cit,
                            mus_the_da,
                            mus_in_cit,
                            mus_shawn2,
                            mus_ddtbl2
                        };

                        mnum = nmus[gamemap - 1];
                        S_ChangeMusic(mnum, true, true);
                    }
                    else
                    {
                        // FIXME: Add case for other game missions here (CHEX, HACX, FINAL DOOM)
                        if (mus_cheat_used)
                            S_ChangeMusic(tracknum, true, true);
                        else if (mus_cheated && !beta_style)
                            S_ChangeMusic(cheat_musnum, true, true);
                        else if (!mus_cheat_used && !mus_cheated)
                            S_ChangeMusic(gamemap + 32, true, true);
                    }
                }
                else if (gamestate == GS_FINALE && finalestage == F_STAGE_TEXT)
                    S_ChangeMusic(mus_read_m, true, false);
                else if (gamestate == GS_FINALE && finalestage == F_STAGE_CAST)
                    S_ChangeMusic(mus_evil, true, false);
                else if (gamestate == GS_DEMOSCREEN)
                    S_ChangeMusic(mus_dm2ttl, false, false);
                else if (gamestate == GS_INTERMISSION)
                    S_ChangeMusic(mus_dm2int, true, false);
            }
            else
            {
                if (gamestate == GS_LEVEL)
                {
                    int spmus[]=
                    {
                        // Who? -       Where?
                        // Song

                        // American     e4m1
                        mus_e3m4,

                        // Romero       e4m2
                        mus_e3m2,

                        // Shawn        e4m3
                        mus_e3m3,

                        // American     e4m4
                        mus_e1m5,

                        // Tim          e4m5
                        mus_e2m7,

                        // Romero       e4m6
                        mus_e2m4,

                        // J.Anderson   e4m7 CHIRON.WAD
                        mus_e2m6,

                        // Shawn        e4m8
                        mus_e2m5,

                        // Tim          e4m9
                        mus_e1m9
                    };

                    if (gameepisode < 4)
                    {
                        mnum = mus_e1m1 + (gameepisode - 1) * 9 + gamemap - 1;
                    }
                    else
                    {
                        mnum = spmus[gamemap - 1];
                    }

                    if (mus_cheat_used)
                        S_ChangeMusic(tracknum, true, true);
                    else if (mus_cheated && !beta_style)
                        S_ChangeMusic(cheat_musnum, true, true);
                    else if (!mus_cheat_used && !mus_cheated)
                        S_ChangeMusic(mnum, true, true);
                }
                else if (gamestate == GS_FINALE && finalestage == F_STAGE_TEXT)
                    S_ChangeMusic(mus_victor, true, false);
                else if (gamestate == GS_FINALE && finalestage == F_STAGE_CAST)
                    S_StartMusic (mus_bunny);
                else if (gamestate == GS_DEMOSCREEN)
                    S_ChangeMusic(mus_intro, false, false);
                else if (gamestate == GS_INTERMISSION)
                    S_ChangeMusic(mus_inter, true, false);
            }
        }
    }

    inhelpscreens = false;

    if (gamestate == GS_LEVEL && !menuactive)
    {
        player_t *player = &players[consoleplayer];

        if (player->powers[pw_flight] > 0)
        {
            int flight_timer = player->powers[pw_flight] / 35;
            sprintf(flight_counter, "%d", flight_timer);
            M_WriteText(299, 5, flight_counter);
        }
    }

    if (coordinates_info)
    {
        if (gamestate == GS_LEVEL)
        {
            player_t *player = &players[consoleplayer];

            sprintf(coordinates_ang_textbuffer, "ang = 0x%x", player->mo->angle);
            sprintf(coordinates_x_textbuffer, "x = 0x%x", player->mo->x);
            sprintf(coordinates_y_textbuffer, "y = 0x%x", player->mo->y);
            sprintf(coordinates_z_textbuffer, "z = 0x%x", player->mo->z);

            M_WriteText(0, 24, coordinates_ang_textbuffer);
            M_WriteText(0, 34, coordinates_x_textbuffer);
            M_WriteText(0, 44, coordinates_y_textbuffer);
            M_WriteText(0, 54, coordinates_z_textbuffer);
        }
    }

    if (memory_usage && done && consoleheight == 0 && !menuactive)
    {
        // print memory allocation stats
        DrawStats();
    }

    // DISPLAYS BINARY VERSION
    if (version_info)
        M_WriteText(65, 36, DOOM_VERSIONTEXT);

    // Horiz. & Vertically center string and print it.
    if (messageToPrint)
    {
        int  start = 0;
        char string[80];

        y = ORIGINALHEIGHT / 2 - M_StringHeight(messageString) / 2;

        while (messageString[start] != '\0')
        {
            int  foundnewline = 0;

            for (i = 0; i < strlen(messageString + start); i++)
            {
                if (messageString[start + i] == '\n')
                {
                    memset(string, 0, sizeof(string));
                    M_StringCopy(string, messageString + start, i + 1);
                    foundnewline = 1;
                    start += i + 1;
                    break;
                }
            }

            if (!foundnewline)
            {
                M_StringCopy(string, messageString + start, sizeof(string));
                start += strlen(string);
            }

            x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;
            M_WriteText(x, y, string);
            y += SHORT(hu_font[0]->height);
        }

        return;
    }

    if (opldev)
    {
        M_DrawOPLDev();
    }

    if (!menuactive)
        return;

    if (currentMenu->routine)
        // call Draw routine
        currentMenu->routine();
    
    // DRAW MENU
    x = currentMenu->x;
    y = currentMenu->y;

#ifndef SDL2
    if (currentMenu == &ScreenDef)
       y += 10;
#endif

    max = currentMenu->numitems;

    // FOR PSP (if too many menu items)
    if (currentMenu->menuitems[itemOn].status == 5)
        max += FirstKey;

    if (!devparm && currentMenu == &OptionsDef)
        currentMenu->numitems = 8;

    if (!netgame && !devparm && currentMenu == &FilesDef)
#ifdef WII
        currentMenu->numitems = 4;
#else
        currentMenu->numitems = 3;
#endif

    if (netgame && currentMenu == &FilesDef)
#ifdef WII
        currentMenu->numitems = 3;
#else
        currentMenu->numitems = 2;
#endif

    // HACK: ALL ELSE THAN SHAREWARE 1.0 & 1.1 && REG 1.1
    if ((fsize == 4207819 || fsize == 4274218 || fsize == 10396254) &&
        currentMenu == &NewDef)
        currentMenu->numitems = 4;

#ifdef WII
    if ((fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
        fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
        fsize == 4225460 || fsize == 4234124 || fsize == 4196020) &&
        currentMenu == &WeaponsDef)
        currentMenu->numitems = 6;

    if (fsize == 12361532 && currentMenu == &ItemsDef)
        currentMenu->numitems = 7;

    if ((fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
        fsize == 11159840 || fsize == 12408292 || fsize == 12474561 ||
        fsize == 12538385 || fsize == 12487824) &&
        currentMenu == &WeaponsDef)
        currentMenu->numitems = 8;
#endif

    for (i = 0; i < max; i++)
    {
        menuitem_t *item = &(currentMenu->menuitems[i]);
        char       *name = item->name;

        if (*name)
        {
            if ((currentMenu == &FilesDef && i == savegame && (!usergame || gamestate != GS_LEVEL)) ||
                (currentMenu == &FilesDef && i == endgame && (!usergame || netgame)) ||
                (currentMenu == &FilesDef && i == loadgame && (netgame /*|| demorecording*/)) ||
                (currentMenu == &MainDef && i == newgame && (/*demorecording ||*/ (netgame /*&& !demoplayback*/))))
                dp_translation = crx[CRX_DARK];
            else if (i == itemOn)
                dp_translation = crx[CRX_GOLD];
            else if (currentMenu == &NewDef)
            {
                if (itemOn == 2)
                    dp_translation = crx[CRX_GOLD];
/*
                if (beta_style)
                    M_WriteText(NewDef.x, NewDef.y + 18, "I JUST WANT TO KILL.");
                else
*/
                    M_WriteText(NewDef.x, NewDef.y + 18, "HURT ME PLENTY.");
            }

            M_WriteText(x, y - 2, item->name);
        }

        y += LINEHEIGHT_SMALL;

        // DRAW SKULL
#ifdef WII
        if (currentMenu == &KeyBindingsDef && itemOn == 15)
#else
        if (currentMenu == &KeyBindingsDef)
            y -= 1;
        else if (currentMenu == &AutomapDef)
            y -= 4;

        if (currentMenu == &KeyBindingsDef && itemOn == 17)
#endif
        {
            if (!beta_style)
                V_DrawPatchWithShadow(x + 280 + CURSORXOFF_SMALL, currentMenu->y - 15 +
                        itemOn*LINEHEIGHT_SMALL - 1 * itemOn, 0, W_CacheLumpName(skullNameSmall[whichSkull],
                                              PU_CACHE), false);
            else
                V_DrawPatch(x + 280 + CURSORXOFF_SMALL, currentMenu->y - 15 +
                        itemOn*LINEHEIGHT_SMALL, 0, W_CacheLumpName(skullNameSmall[whichSkull],
                                              PU_CACHE));
        }
        else if (currentMenu == &KeyBindingsDef && itemOn != 17)
        {
            if (!beta_style)
                V_DrawPatchWithShadow(x + CURSORXOFF_SMALL, currentMenu->y - 5 +
                        itemOn*LINEHEIGHT_SMALL - 1 * itemOn, 0, W_CacheLumpName(skullNameSmall[whichSkull],
                                              PU_CACHE), false);
            else
                V_DrawPatch(x + CURSORXOFF_SMALL, currentMenu->y - 5 +
                        itemOn*LINEHEIGHT_SMALL, 0, W_CacheLumpName(skullNameSmall[whichSkull],
                                              PU_CACHE));
        }
        else if (currentMenu == &AutomapDef)
        {
            if (!beta_style)
                V_DrawPatchWithShadow(x + CURSORXOFF_SMALL, currentMenu->y - 5 +
                        itemOn*LINEHEIGHT_SMALL - 4 * itemOn, 0, W_CacheLumpName(skullNameSmall[whichSkull],
                                              PU_CACHE), false);
            else
                V_DrawPatch(x + CURSORXOFF_SMALL, currentMenu->y - 5 +
                        itemOn*LINEHEIGHT_SMALL, 0, W_CacheLumpName(skullNameSmall[whichSkull],
                                              PU_CACHE));
        }
        else
        {
            int offset = 0;

#ifndef SDL2
            if (currentMenu == &ScreenDef)
                offset = 10;
#endif

            if (!beta_style)
                V_DrawPatchWithShadow(x + CURSORXOFF_SMALL, currentMenu->y - 5 + offset +
                        itemOn*LINEHEIGHT_SMALL, 0, W_CacheLumpName(skullNameSmall[whichSkull],
                                              PU_CACHE), false);
            else
                V_DrawPatch(x + CURSORXOFF_SMALL, currentMenu->y - 5 + offset +
                        itemOn*LINEHEIGHT_SMALL, 0, W_CacheLumpName(skullNameSmall[whichSkull],
                                              PU_CACHE));
        }
    }
}

//
// M_ClearMenus
//
void M_ClearMenus(void)
{
    menuactive = 0;
    //paused = false;

    if (screenSize < 8)
    {
        if (usergame)
            ST_doRefresh();
    }

    V_ClearDPTranslation();

    // if (!netgame && usergame && paused)
    //       sendpause = true;
}

//
// M_SetupNextMenu
//
static void M_SetupNextMenu(menu_t *menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
}

static void M_PlayerSetupTicker(void)
{
    // Based on code in f_finale.c
    if (--mobj_tics > 0)
        return;

    if (mobj_state->tics == -1 || mobj_state->nextstate == S_NULL)
    {
        mobj_state = &states[mobjinfo[MT_PLAYER].seestate];
    }
    else
    {
        mobj_state = &states[mobj_state->nextstate];
    }

    mobj_tics = mobj_state->tics;
}

//
// M_Ticker
//
void M_Ticker(void)
{
    if (--skullAnimCounter <= 0)
    {
        whichSkull ^= 1;
        skullAnimCounter = 8;

        if (dump_con)
            condumpwait++;
        else if (dump_subst)
            substdumpwait++;
        else if (dump_mem)
            memdumpwait++;
        else if (dump_stat)
            statdumpwait++;
        else if (restart_song)
            restartsongwait++;
        else if (printdir)
            printdirwait++;
    }

    if (currentMenu == &TestDef)
        M_PlayerSetupTicker();

    // advance animation
}

//
// M_Init
//
void M_Init(void)
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
    quickSaveSlot = -1;
    tempscreen1 = Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    blurscreen1 = Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    firescreen = Z_Malloc(firewidth * fireheight, PU_STATIC, NULL);

    // Here we could catch other version dependencies,
    //  like HELP1/2, and four episodes.
    switch (gamemode)
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

    if (gamemode == commercial)
        NewDef.prevMenu = (nerve_pwad ? &ExpDef : &MainDef);
}

#ifdef WII
static void M_God(int choice)
{
    players[consoleplayer].cheats ^= CF_GODMODE;

    if (aiming_help)
        aiming_help = false;

    if (players[consoleplayer].cheats & CF_GODMODE)
    {
        if (players[consoleplayer].mo)
            players[consoleplayer].mo->health = 100;

        players[consoleplayer].health = 100;

        if (beta_style)
            players[consoleplayer].message = STSTR_DQDONBETA;
        else
            players[consoleplayer].message = s_STSTR_DQDON;
    }
    else
    {
        if (beta_style)
            players[consoleplayer].message = STSTR_DQDOFFBETA;
        else
            players[consoleplayer].message = s_STSTR_DQDOFF;
    }
}

static void M_Noclip(int choice)
{
    players[consoleplayer].cheats ^= CF_NOCLIP;

    if (players[consoleplayer].cheats & CF_NOCLIP)
    {
        if (beta_style)
            players[consoleplayer].message = STSTR_NCONBETA;
        else
            players[consoleplayer].message = s_STSTR_NCON;

        players[consoleplayer].mo->flags |= MF_NOCLIP;
        noclip_on = true;
    }
    else
    {
        if (beta_style)
            players[consoleplayer].message = STSTR_NCOFFBETA;
        else
            players[consoleplayer].message = s_STSTR_NCOFF;

        players[consoleplayer].mo->flags &= ~MF_NOCLIP;
        noclip_on = false;
    }
}

static void M_ArmorA(int choice)
{
    players[consoleplayer].armorpoints = 200;
    players[consoleplayer].armortype = 2;
    players[consoleplayer].message = "ALL ARMOR ADDED";
}

static void M_ArmorB(int choice)
{
    players[consoleplayer].armorpoints = 100;
    players[consoleplayer].armortype = 1;

    if (fsize != 12361532 && fsize != 19321722)
        players[consoleplayer].message = "GREEN ARMOR ADDED";

    if (fsize == 12361532)
        players[consoleplayer].message = "CHEX(R) ARMOR ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "KEVLAR VEST ADDED";
}

static void M_ArmorC(int choice)
{
    players[consoleplayer].armorpoints = 200;
    players[consoleplayer].armortype = 2;

    if (fsize != 12361532 && fsize != 19321722)
        players[consoleplayer].message = "BLUE ARMOR ADDED";

    if (fsize == 12361532)
        players[consoleplayer].message = "SUPER CHEX(R) ARMOR ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "SUPER KEVLAR VEST ADDED";
}

static void M_WeaponsA(int choice)
{
    int i;

    if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
        fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
        fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
    {
        for (i = 0; i < NUMWEAPONS - 4; i++)
            players[consoleplayer].weaponowned[i] = true;
            players[consoleplayer].weaponowned[7] = true;
    }
    else if (fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
        fsize == 11159840 || fsize == 12408292 || gameversion == exe_chex)
    {
        for (i = 0; i < NUMWEAPONS - 1; i++)
            players[consoleplayer].weaponowned[i] = true;
    }
    else
    {
        for (i = 0; i < NUMWEAPONS; i++)
            players[consoleplayer].weaponowned[i] = true;
    }

    for (i = 0; i < NUMAMMO; i++)
        players[consoleplayer].ammo[i] = players[consoleplayer].maxammo[i];

    players[consoleplayer].message = s_STSTR_FAADDED;
}

static void M_WeaponsB(int choice)
{
    players[consoleplayer].weaponowned[2] = true;
    players[consoleplayer].ammo[1] = players[consoleplayer].maxammo[1];

    if (fsize != 19321722 && fsize != 12361532)
        players[consoleplayer].message = "SHOTGUN ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "TAZER ADDED";

    if (fsize == 12361532)
        players[consoleplayer].message = "LARGE ZORCHER ADDED";
}

static void M_WeaponsC(int choice)
{
    players[consoleplayer].weaponowned[3] = true;
    players[consoleplayer].ammo[1] = players[consoleplayer].maxammo[1];

    if (fsize != 19321722 && fsize != 12361532)
        players[consoleplayer].message = "CHAINGUN ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "UZI ADDED";

    if (fsize == 12361532)
        players[consoleplayer].message = "RAPID ZORCHER ADDED";
}

static void M_WeaponsD(int choice)
{
    players[consoleplayer].weaponowned[4] = true;
    players[consoleplayer].ammo[3] = players[consoleplayer].maxammo[3];

    if (fsize != 19321722 && fsize != 12361532)
        players[consoleplayer].message = "ROCKET LAUNCHER ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "PHOTON 'ZOOKA ADDED";

    if (fsize == 12361532)
        players[consoleplayer].message = "ZORCH PROPULSOR ADDED";
}

static void M_WeaponsE(int choice)
{
    players[consoleplayer].weaponowned[5] = true;
    players[consoleplayer].ammo[2] = players[consoleplayer].maxammo[2];

    if (fsize != 19321722 && fsize != 12361532)
        players[consoleplayer].message = "PLASMA RIFLE ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "STICK ADDED";

    if (fsize == 12361532)
        players[consoleplayer].message = "PHASING ZORCHER ADDED";
}

static void M_WeaponsF(int choice)
{
    players[consoleplayer].weaponowned[6] = true;
    players[consoleplayer].ammo[2] = players[consoleplayer].maxammo[2];

    if (fsize != 19321722 && fsize != 12361532)
        players[consoleplayer].message = "BFG9000 ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "NUKER ADDED";

    if (fsize == 12361532)
        players[consoleplayer].message =
        "LARGE AREA ZORCHING DEVICE ADDED";
}

static void M_WeaponsG(int choice)
{
    players[consoleplayer].weaponowned[7] = true;

    if (fsize != 19321722 && fsize != 12361532)
        players[consoleplayer].message = "CHAINSAW ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "HOIG REZNATOR ADDED";

    if (fsize == 12361532)
        players[consoleplayer].message = "SUPER BOOTSPORK ADDED";
}

static void M_WeaponsH(int choice)
{
    players[consoleplayer].weaponowned[8] = true;
    players[consoleplayer].ammo[1] = players[consoleplayer].maxammo[1];

    if (fsize != 19321722)
        players[consoleplayer].message = "SUPER SHOTGUN ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "CRYOGUN ADDED";
}

static void M_KeysA(int choice)
{
    player_t *player = &players[consoleplayer];
    P_GiveAllCards(player);
    players[consoleplayer].message = "ALL KEYS FOR THIS MAP ADDED";
}

static void M_KeysB(int choice)
{
    player_t *player = &players[consoleplayer];
    P_GiveCard (player, it_bluecard);
    players[consoleplayer].message = "BLUE KEYCARD ADDED";
}

static void M_KeysC(int choice)
{
    player_t *player = &players[consoleplayer];
    P_GiveCard (player, it_yellowcard);
    players[consoleplayer].message = "YELLOW KEYCARD ADDED";
}

static void M_KeysD(int choice)
{
    player_t *player = &players[consoleplayer];
    P_GiveCard (player, it_redcard);
    players[consoleplayer].message = "RED KEYCARD ADDED";
}

static void M_KeysE(int choice)
{
    player_t *player = &players[consoleplayer];
    P_GiveCard (player, it_blueskull);
    players[consoleplayer].message = "BLUE SKULLKEY ADDED";
}

static void M_KeysF(int choice)
{
    player_t *player = &players[consoleplayer];
    P_GiveCard (player, it_yellowskull);
    players[consoleplayer].message = "YELLOW SKULLKEY ADDED";
}

static void M_KeysG(int choice)
{
    player_t *player = &players[consoleplayer];
    P_GiveCard (player, it_redskull);
    players[consoleplayer].message = "RED SKULLKEY ADDED";
}

static void M_ItemsA(int choice)
{
    player_t *player = &players[consoleplayer];

    if (!got_all)
    {
        int i;

        players[consoleplayer].powers[0] = INVULNTICS;
        players[consoleplayer].powers[1] = 1;
        players[consoleplayer].powers[2] = INVISTICS;
        players[consoleplayer].mo->flags |= MF_SHADOW;
        players[consoleplayer].powers[3] = IRONTICS;
        players[consoleplayer].powers[4] = 1;
        players[consoleplayer].powers[5] = INFRATICS;
        players[consoleplayer].powers[6] = FLIGHTTICS;
        players[consoleplayer].mo->flags2 |= MF2_FLY;
        players[consoleplayer].mo->flags |= MF_NOGRAVITY;
        player->cheats ^= CF_NOTARGET;

        if (!player->backpack)
        {
            for (i = 0; i < NUMAMMO; i++)
                player->maxammo[i] *= 2;

            player->backpack = true;
        }

        for (i = 0; i < NUMAMMO; i++)
            P_GiveAmmo (player, i, 1);

        HU_PlayerMessage(s_GOTBACKPACK, true);

        got_all = true;
    }
    else
    {
        players[consoleplayer].powers[0] = 0;
        players[consoleplayer].powers[1] = 0;
        players[consoleplayer].powers[2] = 0;
        players[consoleplayer].mo->flags &= ~MF_SHADOW;
        players[consoleplayer].powers[3] = 0;
        players[consoleplayer].powers[4] = 0;
        players[consoleplayer].powers[5] = 0;
        players[consoleplayer].powers[6] = 0;
        players[consoleplayer].mo->flags2 &= ~MF2_FLY;
        players[consoleplayer].mo->flags &= ~MF_NOGRAVITY;
        player->cheats &= ~CF_NOTARGET;

        if (beta_style)
            players[consoleplayer].fixedcolormap = 0;

        got_all = false;
    }

    players[consoleplayer].message = "ALL ITEMS ADDED";
}

static void M_ItemsB(int choice)
{
    if (!got_invisibility)
    {
        players[consoleplayer].powers[2] = INVISTICS;
        players[consoleplayer].cheats ^= CF_NOTARGET;
        players[consoleplayer].mo->flags |= MF_SHADOW;
        got_invisibility = true;
    }
    else
    {
        players[consoleplayer].powers[2] = 0;
        players[consoleplayer].cheats &= ~CF_NOTARGET;
        players[consoleplayer].mo->flags &= ~MF_SHADOW;
        got_invisibility = false;

        if (beta_style)
            players[consoleplayer].fixedcolormap = 0;
    }

    if (fsize == 19321722)
        players[consoleplayer].message = "ENK BLINDNESS ADDED";
    else
        players[consoleplayer].message = s_GOTINVIS;
}

static void M_ItemsC(int choice)
{
    if (!got_radiation_suit)
    {
        players[consoleplayer].powers[3] = IRONTICS;
        got_radiation_suit = true;
    }
    else
    {
        players[consoleplayer].powers[3] = 0;
        got_radiation_suit = false;
    }

    if (fsize != 12361532 && fsize != 19321722)
        players[consoleplayer].message = s_GOTSUIT;

    if (fsize == 12361532)
        players[consoleplayer].message = "SLIME-PROOF SUIT ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "VULCAN RUBBER BOOTS ADDED";
}

static void M_ItemsD(int choice)
{
    if (!got_map)
    {
        players[consoleplayer].powers[4] = 1;
        got_map = true;
    }
    else
    {
        players[consoleplayer].powers[4] = 0;
        got_map = false;
    }

    if (fsize != 19321722)
        players[consoleplayer].message = s_GOTMAP;

    if (fsize == 19321722)
        players[consoleplayer].message = "SI ARRAY MAPPING ADDED";
}

static void M_ItemsE(int choice)
{
    if (!got_light_amp)
    {
        players[consoleplayer].powers[5] = INFRATICS;
        got_light_amp = true;
    }
    else
    {
        players[consoleplayer].powers[5] = 0;
        got_light_amp = false;
    }

    if (fsize != 12361532 && fsize != 19321722)
        players[consoleplayer].message = s_GOTVISOR;

    if (fsize == 12361532)
        players[consoleplayer].message = "ULTRA GOOGLES ADDED";

    if (fsize == 19321722)
        players[consoleplayer].message = "INFRARED VISOR ADDED";
}

static void M_ItemsF(int choice)
{
    if (!got_invulnerability)
    {
        players[consoleplayer].powers[0] = INVULNTICS;
        got_invulnerability = true;
    }
    else
    {
        players[consoleplayer].powers[0] = 0;
        got_invulnerability = false;
    }

    if (fsize != 19321722)
        players[consoleplayer].message = s_GOTINVUL;

    if (fsize == 19321722)
        players[consoleplayer].message = "FORCE FIELD ADDED";
}

static void M_ItemsG(int choice)
{
    if (!got_berserk)
    {
        players[consoleplayer].powers[1] = 1;
        got_berserk = true;
    }
    else
    {
        players[consoleplayer].powers[1] = 0;
        got_berserk = false;
    }

    if (fsize != 19321722)
        players[consoleplayer].message = s_GOTBERSERK;

    if (fsize == 19321722)
        players[consoleplayer].message = "007 MICROTEL ADDED";
}

static void M_ItemsH(int choice)
{
    if (players[consoleplayer].mo)
        players[consoleplayer].mo->health = 100;

    players[consoleplayer].health = 100;
    players[consoleplayer].message = "GOT FULL HEALTH (100)";
}

static void M_ItemsI(int choice)
{
    if (fsize == 10396254 || fsize == 10399316 || fsize == 4207819 ||
        fsize == 4274218 || fsize == 4225504)
    {
        if (players[consoleplayer].mo)
            players[consoleplayer].mo->health = 199;

        players[consoleplayer].health = 199;
        players[consoleplayer].message = "GOT FULL HEALTH (199)";
    }
    else
    {
        if (players[consoleplayer].mo)
            players[consoleplayer].mo->health = 200;

        players[consoleplayer].health = 200;
        players[consoleplayer].message = "GOT FULL HEALTH (200)";
    }
}

static void M_ItemsJ(int choice)
{
    int i;

    player_t *player = &players[consoleplayer];

    if (!player->backpack)
    {
        for (i = 0; i < NUMAMMO; i++)
            player->maxammo[i] *= 2;

        player->backpack = true;
    }

    for (i = 0; i < NUMAMMO; i++)
        P_GiveAmmo (player, i, 1);

    HU_PlayerMessage(s_GOTBACKPACK, true);
}

static void M_ItemsK(int choice)
{
    player_t *player = &players[consoleplayer];
    P_UseArtifact(player, arti_fly);
    HU_PlayerMessage("FLIGHT ADDED", true);
}

static void M_Topo(int choice)
{
    cheating = (cheating + 1) % 3;
    cheeting = (cheeting + 1) % 3;
}

static void M_Rift(int choice)
{
    switch (choice)
    {
        case 0:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020 ||
                fsize == 12361532)
            {
                if (epi >= 1 && map >= 1)
                    map--;
            }
            else if (fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                fsize == 11159840)
            {
                if (epi >= 1 && map >= 1)
                {
                    map--;

                    if (epi == 3 && map == 0)
                    {
                        epi = 2;
                        map = 9;
                    }
                    else if (epi == 2 && map == 0)
                    {
                        epi = 1;
                        map = 9;
                    }
                }
            }
            else if (fsize == 12408292 || fsize == 12474561 || fsize == 12487824)
            {
                if (epi >= 1 && map >= 1)
                {
                    map--;

                    if (epi == 4 && map == 0)
                    {
                        epi = 3;
                        map = 9;
                    }
                    else if (epi == 3 && map == 0)
                    {
                        epi = 2;
                        map = 9;
                    }
                    else if (epi == 2 && map == 0)
                    {
                        epi = 1;
                        map = 9;
                    }
                }
            }
            else if (fsize == 12538385)
            {
                if (epi >= 1 && map >= 1)
                {
                    map--;

                    if (epi == 4 && map == 0)
                    {
                        epi = 3;
                        map = 9;
                    }
                    else if (epi == 3 && map == 0)
                    {
                        epi = 2;
                        map = 9;
                    }
                    else if (epi == 2 && map == 0)
                    {
                        epi = 1;
                        map = 10;
                    }
                }
            }
            else if (fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                fsize == 14607420 || fsize == 14604584 || fsize == 18195736 ||
                fsize == 18654796 || fsize == 18240172 || fsize == 17420824 ||
                fsize == 28422764 || fsize == 14677988 || fsize == 14683458 ||
                fsize == 14691821)
            {
                if (map >= 2)
                    map--;
            }
            else if (fsize == 19321722)
            {
                if (map >= 2)
                    map--;

                if (map == 30)
                    map = 20;
            }

            break;

        case 1:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if (epi <= 1 && map <= 9)
                {
                    map++;

                    if (epi == 1 && map == 10)
                    {
                        epi = 1;
                        map = 9;
                    }
                }
            }
            else if (fsize == 12361532)
            {
                if (epi <= 1 && map <= 5)
                {
                    map++;

                    if (epi == 1 && map == 6)
                    {
                        epi = 1;
                        map = 5;
                    }
                }
            }
            else if (fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                fsize == 11159840)
            {
                if (epi <= 3 && map <= 9)
                {
                    map++;

                    if (epi == 1 && map == 10)
                    {
                        epi = 2;
                        map = 1;
                    }
                    else if (epi == 2 && map == 10)
                    {
                        epi = 3;
                        map = 1;
                    }
                    else if (epi == 3 && map == 10)
                    {
                        epi = 3;
                        map = 9;
                    }
                }
            }
            else if (fsize == 12408292 || fsize == 12474561 || fsize == 12487824)
            {
                if (epi <= 4 && map <= 9)
                {
                    map++;

                    if (epi == 1 && map == 10)
                    {
                        epi = 2;
                        map = 1;
                    }
                    else if (epi == 2 && map == 10)
                    {
                        epi = 3;
                        map = 1;
                    }
                    else if (epi == 3 && map == 10)
                    {
                        epi = 4;
                        map = 1;
                    }
                    else if (epi == 4 && map == 10)
                    {
                        epi = 4;
                        map = 9;
                    }
                }
            }
            else if (fsize == 12538385)
            {
                if (epi <= 4 && map <= 10)
                {
                    map++;

                    if (epi == 1 && map == 11)
                    {
                        epi = 2;
                        map = 1;
                    }
                    else if (epi == 2 && map == 10)
                    {
                        epi = 3;
                        map = 1;
                    }
                    else if (epi == 3 && map == 10)
                    {
                        epi = 4;
                        map = 1;
                    }
                    else if (epi == 4 && map == 10)
                    {
                        epi = 4;
                        map = 9;
                    }
                }
            }
            else if (fsize == 19321722)
            {
                if (map <= 30)
                    map++;

                if (map == 21)
                    map = 31;
            }
    
            if (!nerve_pwad)
            {
                if (fsize == 14943400 || fsize == 14612688 || fsize == 14607420 ||
                    fsize == 14604584 || fsize == 18195736 || fsize == 18654796 ||
                    fsize == 18240172 || fsize == 17420824 || fsize == 28422764)
                {
                    if (map <= 31)
                        map++;
                }
                else if (fsize == 14677988 || fsize == 14683458 || fsize == 14691821)
                {
                    if (map <= 32)
                        map++;
                }
                else if (fsize == 14824716)
                {
                    if (map <= 29)
                        map++;
                }
            }

            break;
    }
}

static void M_RiftNow(int choice)
{
    warped = 1;
    menuactive = 0;
    G_DeferedInitNew(gameskill, epi, map);
    players[consoleplayer].message = STSTR_CLEV;
}

static void M_Spin(int choice)
{
    switch (choice)
    {
        case 0:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if (tracknum > 1)
                {
                    tracknum--;

                    if (tracknum == 30)
                        tracknum = 29;
                    else if (tracknum == 27)
                        tracknum = 9;
                }
            }
            else if (fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                fsize == 11159840 || fsize == 12408292 || fsize == 12361532 ||
                fsize == 12474561 || fsize == 12538385 || fsize == 12487824)
            {
                if (tracknum > 1)
                {
                    tracknum--;

                    if (fsize == 12361532)
                    {
                        if (tracknum == 30)
                            tracknum = 29;
                        else if (tracknum == 27)
                            tracknum = 5;
                    }
                    else
                    {
                        if (tracknum == 28)
                            tracknum = 26;
                        else if (tracknum == 25)
                            tracknum = 21;
                        else if (tracknum == 19)
                            tracknum = 18;
                        else if (tracknum == 14)
                            tracknum = 13;
                    }
                }
            }
            else if (fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                fsize == 14607420 || fsize == 14604584 || fsize == 19321722 ||
                fsize == 28422764 || fsize == 14677988 || fsize == 14683458 ||
                fsize == 14691821)
            {
                if (fsize == 19321722)
                {
                    if (tracknum > 33)
                    {
                        tracknum--;

                        if (tracknum == 64)
                            tracknum = 63;
                        else if (tracknum == 62)
                            tracknum = 53;
                        else if (tracknum == 51)
                            tracknum = 50;
                        else if (tracknum == 46)
                            tracknum = 45;
                    }
                }
                else if (fsize == 28422764)
                {
                    if (tracknum > 33)
                    {
                        tracknum--;

                        if (tracknum == 65)
                            tracknum = 64;
                        else if (tracknum == 60)
                            tracknum = 56;
                        else if (tracknum == 55)
                            tracknum = 51;
                        else if (tracknum == 47)
                            tracknum = 42;
                        else if (tracknum == 41)
                            tracknum = 40;
                        else if (tracknum == 39)
                            tracknum = 38;
                        else if (tracknum == 37)
                            tracknum = 36;
                    }
                }
                else
                {
                    if (tracknum > 33)
                    {
                        tracknum--;

                        if (tracknum == 61)
                            tracknum = 60;
                        else if (tracknum == 59)
                            tracknum = 57;
                        else if (tracknum == 56)
                            tracknum = 55;
                        else if (tracknum == 54)
                            tracknum = 52;
                        else if (tracknum == 51)
                            tracknum = 50;
                        else if (tracknum == 49)
                            tracknum = 42;
                    }
                }
            }
            else if (fsize == 18195736 || fsize == 18654796)
            {
                if (tracknum > 33)
                {
                    tracknum--;

                    if (tracknum == 65)
                        tracknum = 63;
                    else if (tracknum == 62)
                        tracknum = 57;
                    else if (tracknum == 50)
                        tracknum = 48;
                    else if (tracknum == 47)
                        tracknum = 46;
                    else if (tracknum == 45)
                        tracknum = 44;
                    else if (tracknum == 41)
                        tracknum = 40;
                }
            }
            else if (fsize == 18240172 || fsize == 17420824)
            {
                if (tracknum > 33)
                {
                    tracknum--;

                    if (tracknum == 65)
                        tracknum = 62;
                    else if (tracknum == 61)
                        tracknum = 57;
                    else if (tracknum == 50)
                        tracknum = 49;
                }
            }

            break;

        case 1:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if (tracknum < 31)
                {
                    tracknum++;

                    if (tracknum == 10)
                        tracknum = 28;
                    else if (tracknum == 30)
                            tracknum = 31;
                }
            }
            else if (fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                fsize == 11159840 || fsize == 12408292 || fsize == 12361532 ||
                fsize == 12474561 || fsize == 12538385 || fsize == 12487824)
            {
                if (tracknum < 31)
                {
                    tracknum++;

                    if (fsize == 12361532)
                    {
                        if (tracknum == 6)
                            tracknum = 28;
                        else if (tracknum == 30)
                            tracknum = 31;
                    }
                    else
                    {
                        if (tracknum == 14)
                            tracknum = 15;
                        else if (tracknum == 19)
                            tracknum = 20;
                        else if (tracknum == 22)
                            tracknum = 26;
                        else if (tracknum == 27)
                            tracknum = 29;
                    }
                }
            }
            else if (fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                fsize == 14607420 || fsize == 14604584 || fsize == 19321722 ||
                fsize == 28422764 || fsize == 14677988 || fsize == 14683458 ||
                fsize == 14691821)
            {
                if (fsize == 19321722)
                {
                    if (tracknum < 67)
                    {
                        tracknum++;

                        if (tracknum == 46)
                            tracknum = 47;
                        else if (tracknum == 51)
                            tracknum = 52;
                        else if (tracknum == 54)
                            tracknum = 63;
                        else if (tracknum == 64)
                            tracknum = 65;
                    }
                }
                else if (fsize == 28422764)
                {
                    if (tracknum < 67)
                    {
                        tracknum++;

                        if (tracknum == 37)
                            tracknum = 38;
                        else if (tracknum == 39)
                            tracknum = 40;
                        else if (tracknum == 41)
                            tracknum = 42;
                        else if (tracknum == 43)
                            tracknum = 48;
                        else if (tracknum == 52)
                            tracknum = 56;
                        else if (tracknum == 57)
                            tracknum = 61;
                        else if (tracknum == 65)
                            tracknum = 66;
                    }
                }
                else
                {
                    if (tracknum < 67)
                    {
                        tracknum++;

                        if (tracknum == 43)
                            tracknum = 50;
                        else if (tracknum == 51)
                            tracknum = 52;
                        else if (tracknum == 53)
                            tracknum = 55;
                        else if (tracknum == 56)
                            tracknum = 57;
                        else if (tracknum == 58)
                            tracknum = 60;
                        else if (tracknum == 61)
                            tracknum = 62;
                    }
                }
            }
            else if (fsize == 18195736 || fsize == 18654796)
            {
                if (tracknum < 66)
                {
                    tracknum++;

                    if (tracknum == 41)
                        tracknum = 42;
                    else if (tracknum == 45)
                        tracknum = 46;
                    else if (tracknum == 47)
                        tracknum = 48;
                    else if (tracknum == 49)
                        tracknum = 51;
                    else if (tracknum == 58)
                        tracknum = 63;
                    else if (tracknum == 64)
                        tracknum = 66;
                }
            }
            else if (fsize == 18240172 || fsize == 17420824)
            {
                if (tracknum < 67)
                {
                    tracknum++;

                    if (tracknum == 50)
                        tracknum = 51;
                    else if (tracknum == 58)
                        tracknum = 62;
                    else if (tracknum == 63)
                        tracknum = 66;
                }
            }

            break;
    }

    players[consoleplayer].message = s_STSTR_MUS;

    if (mus_engine == 3)
        S_ChangeMusic(tracknum, false, true);
    else if (mus_engine == 1 || mus_engine == 2)
        S_ChangeMusic(tracknum, true, true);

    mus_cheat_used = true;
}
#endif

static void M_KeyBindingsSetKey(int choice)
{
    askforkey = true;
    keyaskedfor = choice;
/*
    if (!netgame && !demoplayback)
    {
        paused = true;
    }
*/
}

// XXX (FOR PSP): NOW THIS IS RATHER IMPORTANT: IF...
// ...THE CONFIG VARIABLES IN THIS SOURCE EVER GET...
// ...SOMEWHAT REARRANGED, THEN IT'S IMPORTANT TO...
// ...CHANGE THE START- & END-POS INTEGERS AS WELL...
// ...TO THEIR NEW CORRESPONDING POSITIONS OR ELSE...
// ...THE KEY BINDINGS MENU WILL BE VERY BUGGY!!!
static void M_KeyBindingsClearControls(int ch)
{
    int i;

    for (i = key_controls_start_in_cfg_at_pos; i < key_controls_end_in_cfg_at_pos; i++)
    {
        if (*doom_defaults_list[i].location.i == ch)
            *doom_defaults_list[i].location.i = 0;
    }
}

static void M_KeyBindingsClearAll(int choice)
{
    int i;

    for (i = key_controls_start_in_cfg_at_pos; i < key_controls_end_in_cfg_at_pos; i++)
        *doom_defaults_list[i].location.i = 0;

#ifdef WII
    *doom_defaults_list[key_controls_start_in_cfg_at_pos + 2].location.i = CLASSIC_CONTROLLER_MINUS;
#else
    *doom_defaults_list[key_controls_start_in_cfg_at_pos + 2].location.i = KEY_ESCAPE;
#endif
}

static void M_KeyBindingsReset(int choice)
{
    int i = key_controls_start_in_cfg_at_pos;

#ifdef WII
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_R;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_L;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_MINUS;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_LEFT;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_DOWN;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_RIGHT;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_ZL;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_ZR;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_A;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_Y;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_B;
    *doom_defaults_list[i++].location.i = CONTROLLER_1;
    *doom_defaults_list[i++].location.i = CONTROLLER_2;
    *doom_defaults_list[i++].location.i = CLASSIC_CONTROLLER_X;
#else
    *doom_defaults_list[i++].location.i = KEY_RCTRL;
    *doom_defaults_list[i++].location.i = ' ';
    *doom_defaults_list[i++].location.i = KEY_ESCAPE;
    *doom_defaults_list[i++].location.i = KEY_LEFTBRACKET;
    *doom_defaults_list[i++].location.i = KEY_TAB;
    *doom_defaults_list[i++].location.i = KEY_RIGHTBRACKET;
    *doom_defaults_list[i++].location.i = KEYP_PLUS;
    *doom_defaults_list[i++].location.i = KEYP_MINUS;
    *doom_defaults_list[i++].location.i = KEY_PGUP;
    *doom_defaults_list[i++].location.i = KEY_PGDN;
    *doom_defaults_list[i++].location.i = KEY_DEL;
    *doom_defaults_list[i++].location.i = KEY_RSHIFT;
    *doom_defaults_list[i++].location.i = KEY_TILDE;
    *doom_defaults_list[i++].location.i = KEY_F12;
    *doom_defaults_list[i++].location.i = KEY_COMMA;
    *doom_defaults_list[i++].location.i = KEY_PERIOD;
#endif
}

static void M_DrawKeyBindings(void)
{
    int i;

    if (fsize != 19321722)
        V_DrawPatchWithShadow (80, 0, 0, W_CacheLumpName("M_T_BNDS", PU_CACHE), false);
    else
        V_DrawPatchWithShadow (80, 0, 0, W_CacheLumpName("M_KBNDGS", PU_CACHE), false);

    for (i = 0; i < key_controls_end_in_cfg_at_pos - key_controls_start_in_cfg_at_pos; i++)
    {
        if (askforkey && keyaskedfor == i)
            M_WriteText(195, (i * 9 + 16), "???");
        else
            M_WriteText(195, (i * 9 + 16),
                    Key2String(*(doom_defaults_list[i + FirstKey +
                            key_controls_start_in_cfg_at_pos].location.i)));
    }

#ifndef WII
    dp_translation = crx[CRX_DARK];
    M_WriteText(45, 124, "CONSOLE");
#endif

    dp_translation = crx[CRX_GRAY];

#ifdef WII
    M_WriteText(183, 140, "/");
#else
    M_WriteText(183, 160, "/");
#endif

    dp_translation = crx[CRX_BLUE];

#ifdef WII
    if (itemOn == 14)
#else
    if (itemOn == 16)
#endif

    dp_translation = crx[CRX_GOLD];

#ifdef WII
    M_WriteText(45, 140, "CLEAR ALL CONTROLS");
#else
    M_WriteText(45, 160, "CLEAR ALL CONTROLS");
#endif

    dp_translation = crx[CRX_BLUE];

#ifdef WII
    if (itemOn == 15)
#else
    if (itemOn == 17)
#endif

    dp_translation = crx[CRX_GOLD];

#ifdef WII
    M_WriteText(195, 140, "RESET DEFAULTS");
#else
    M_WriteText(195, 160, "RESET DEFAULTS");
#endif
}

static void M_KeyBindings(int choice)
{
    M_SetupNextMenu(&KeyBindingsDef);
}

static void M_Freelook(int choice)
{
    if (!mousewalk)
    {
        switch (choice)
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
}

static void M_FreelookSpeed(int choice)
{
    if (mouselook)
    {
        switch (choice)
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
}

static void M_Controls(int choice)
{
    M_SetupNextMenu(&ControlsDef);
}

static void M_DrawControls(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow(48, 15, 0, W_CacheLumpName("M_T_CSET",
                                               PU_CACHE), false);
    else
        V_DrawPatchWithShadow(48, 15, 0, W_CacheLumpName("M_CTLSET",
                                               PU_CACHE), false);
#ifndef WII
    if (itemOn == 5)
        dp_translation = crx[CRX_GOLD];
    else
    {
        if (gamemission == pack_hacx)
            dp_translation = crx[CRX_BLUE];
        else if (gamemission == pack_chex)
            dp_translation = crx[CRX_GREEN];
        else
            dp_translation = crx[CRX_RED];
    }

    M_WriteText(ControlsDef.x, ControlsDef.y + 48, "MOUSE SENSITIVITY");

    M_DrawThermoSmall(ControlsDef.x + 205, ControlsDef.y + LINEHEIGHT_SMALL * (mousesensibility + 1),
                 10, mouseSensitivity);

    if (itemOn == 6)
        dp_translation = crx[CRX_GOLD];
    else
    {
        if (gamemission == pack_hacx)
            dp_translation = crx[CRX_BLUE];
        else if (gamemission == pack_chex)
            dp_translation = crx[CRX_GREEN];
        else
            dp_translation = crx[CRX_RED];
    }

    M_WriteText(ControlsDef.x, ControlsDef.y + 58, "MOUSE WALK (TURNS FREELOOK MODE OFF)");

    if (mousewalk)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ControlsDef.x + 284, ControlsDef.y + 58, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ControlsDef.x + 276, ControlsDef.y + 58, "OFF");
    }    
#endif

    if (mouselook == 0)
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(ControlsDef.x + 276, ControlsDef.y + 38, "OFF");
    }
    else if (mouselook == 1)
    {
        dp_translation = crx[CRX_GOLD];
        M_WriteText(ControlsDef.x + 251, ControlsDef.y + 38, "NORMAL");
    }
    else if (mouselook == 2)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(ControlsDef.x + 250, ControlsDef.y + 38, "INVERSE");
    }

    dp_translation = crx[CRX_GREEN];
    M_WriteText(ControlsDef.x, ControlsDef.y - 12, "SPEEDS:");

    M_DrawThermoSmall(ControlsDef.x + 77, ControlsDef.y + LINEHEIGHT_SMALL * (mousesens + 1),
                 26, forwardmove - 25);

    M_DrawThermoSmall(ControlsDef.x + 237, ControlsDef.y + LINEHEIGHT_SMALL * (turnsens + 1),
                 6, turnspeed - 5);

    M_DrawThermoSmall(ControlsDef.x + 149, ControlsDef.y + LINEHEIGHT_SMALL * (strafesens + 1),
                 17, sidemove - 24);

    M_DrawThermoSmall(ControlsDef.x + 197, ControlsDef.y + LINEHEIGHT_SMALL * (mousespeed + 1),
                 11, mspeed);

    if (itemOn == 7)
        dp_translation = crx[CRX_GOLD];
    else
        dp_translation = crx[CRX_GRAY];

    M_WriteText(ControlsDef.x, ControlsDef.y + 68, "KEY BINDINGS...");

    if (!mouselook)
        dp_translation = crx[CRX_DARK];
    else if (itemOn == 3 && mouselook)
        dp_translation = crx[CRX_GOLD];

    M_WriteText(ControlsDef.x, ControlsDef.y + 28, "FREELOOK");

    if (!whichSkull)
    {
        char *string = "IF THE BARS FOR WALKING, TURNING & STRAFING";
        char *string2 = "ARE AT THEIR HIGHEST LEVEL, IT MEANS THE SAME";
        char *string3 = "AS PLAYING THE GAME WHILE HOLDING DOWN [SHIFT]";
        int x = ORIGINALWIDTH / 2 - M_StringWidth(string) / 2;
        int x2 = ORIGINALWIDTH / 2 - M_StringWidth(string2) / 2;
        int x3 = ORIGINALWIDTH / 2 - M_StringWidth(string3) / 2;

        dp_translation = crx[CRX_GOLD];
        M_WriteText(x, ControlsDef.y + 78, string);

        dp_translation = crx[CRX_GOLD];
        M_WriteText(x2, ControlsDef.y + 88, string2);

        dp_translation = crx[CRX_GOLD];
        M_WriteText(x3, ControlsDef.y + 98, string3);
    }
}

static void M_FPS(int choice)
{
    switch (choice)
    {
        case 0:
            if (display_fps)
                display_fps = false;

            break;

        case 1:
            if (!display_fps)
                display_fps = true;

            break;
    }
}

static void M_HOMDetector(int choice)
{
    switch (choice)
    {
        case 0:
            if (autodetect_hom)
                autodetect_hom = false;

            break;

        case 1:
            if (!autodetect_hom)
                autodetect_hom = true;

            break;
    }
}

static void M_MemoryUsage(int choice)
{
    switch (choice)
    {
        case 0:
            if (memory_usage)
                memory_usage = false;

            break;

        case 1:
            if (!memory_usage)
                memory_usage = true;

            break;
    }
}

static void M_ConDump(int choice)
{
    dump_con = true;
    C_ConDump();
    players[consoleplayer].message = "Console Output has been dumped.";
}

#ifdef HEAPDUMP
static void M_MemDump(int choice)
{
    dump_mem = true;
    Z_DumpMemory();    
    players[consoleplayer].message = "Memory has been dumped.";
}
#endif

static void M_StatDump(int choice)
{
    dump_stat = true;
    StatDump();    
    players[consoleplayer].message = "Level Statistics have been dumped.";
}

static void M_PrintDir(int choice)
{
    printdir = true;
    W_PrintDirectory();
    players[consoleplayer].message = "WAD contents have been dumped to textfile.";
}

static void M_ReplaceMissing(int choice)
{
    switch (choice)
    {
        case 0:
            if (replace_missing)
                replace_missing = false;

            break;

        case 1:
            if (!replace_missing)
                replace_missing = true;

            break;
    }
}

static void M_Precache(int choice)
{
    switch (choice)
    {
        case 0:
            if (precache)
                precache = false;

            break;

        case 1:
            if (!precache)
                precache = true;

            break;
    }
}

static void M_DisplayTicker(int choice)
{
    switch (choice)
    {
        case 0:
            if (display_ticker)
            {
                dots_enabled = false;
                display_ticker = false;
            }

            break;

        case 1:
            if (!display_ticker)
            {
                dots_enabled = true;
                display_ticker = true;
            }

            break;
    }

    I_DisplayFPSDots(display_ticker);
}

static void M_Coordinates(int choice)
{
    switch (choice)
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

static void M_Timer(int choice)
{
    switch (choice)
    {
        case 0:
            if (timer_info)
                timer_info = false;

            break;

        case 1:
            if (!timer_info)
                timer_info = true;

            break;
    }

    HU_Start();
}

static void M_Authors(int choice)
{
    switch (choice)
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

    HU_Start();
}

static void M_StatusMap(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_statusmap)
                d_statusmap = false;

            break;

        case 1:
            if (!d_statusmap)
                d_statusmap = true;

            break;
    }

    HU_Start();
}

static void M_MapName(int choice)
{
    switch (choice)
    {
        case 0:
            if (show_title)
                show_title = false;

            break;

        case 1:
            if (!show_title)
                show_title = true;

            break;
    }

    HU_Start();
}

static void M_MapSecretAfter(int choice)
{
    switch (choice)
    {
        case 0:
            if (map_secret_after)
                map_secret_after = false;

            break;

        case 1:
            if (!map_secret_after)
                map_secret_after = true;

            break;
    }
}

static void M_Version(int choice)
{
    switch (choice)
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

static void M_SoundInfo(int choice)
{
    switch (choice)
    {
        case 0:
            if (sound_info)
                sound_info = false;

            break;

        case 1:
            if (!sound_info)
                sound_info = true;

            break;
    }
}

static void M_System(int choice)
{
    M_SetupNextMenu(&SystemDef);
}

static void M_GameFiles(int choice)
{
    M_SetupNextMenu(&FilesDef);
}

static void M_DrawFilesMenu(void)
{
}

#ifdef WII
static void M_Armor(int choice)
{
    M_SetupNextMenu(&ArmorDef);
}

static void M_Weapons(int choice)
{
    M_SetupNextMenu(&WeaponsDef);
}

static void M_Keys(int choice)
{
    M_SetupNextMenu(&KeysDef);
}

static void M_Items(int choice)
{
    M_SetupNextMenu(&ItemsDef);
}
#endif

static void M_Screen(int choice)
{
    M_SetupNextMenu(&ScreenDef);
}

/*
static void M_Record(int choice)
{
    M_SetupNextMenu(&RecordDef);
}
*/

static void M_Automap(int choice)
{
    M_SetupNextMenu(&AutomapDef);
}

static void M_Game(int choice)
{
    M_SetupNextMenu(&GameDef);
}

static void M_Game2(int choice)
{
    M_SetupNextMenu(&GameDef2);
}

static void M_Game3(int choice)
{
    M_SetupNextMenu(&GameDef3);
}

static void M_Game4(int choice)
{
    M_SetupNextMenu(&GameDef4);
}

static void M_Game5(int choice)
{
    M_SetupNextMenu(&GameDef5);
}

static void M_Game6(int choice)
{
    M_SetupNextMenu(&GameDef6);
}

static void M_Game7(int choice)
{
    M_SetupNextMenu(&GameDef7);
}

static void M_Game8(int choice)
{
    M_SetupNextMenu(&GameDef8);
}

static void M_OptSet(int choice)
{
    musicVolume = 15;
    sfxVolume = 8;
    showMessages = true;
    drawgrid = false;
    followplayer = true;
    show_stats = true;
    timer_info = true;
    use_vanilla_weapon_change = false;
    chaingun_tics = 1;
    crosshair = true;
    d_colblood = true;
    d_colblood2 = true;
    d_swirl = true;
    autoaim = false;
    background_type = 1;
    icontype = false;
    wipe_type = 3;
    mouselook = 1;
    mspeed = 4;
    mus_engine = 2;
    snd_module = false;
    snd_chans = 2;
    sound_channels = 16;
    opl_type = true;
    use_libsamplerate = 0;
    gore_amount = 4;
    display_fps = false;
    font_shadow = 2;
    show_endoom = true;
    forwardmove = 50;
    sidemove = 36; 
    turnspeed = 5;
    detailLevel = false;
    screenblocks = 11;
    screenSize = 8;
    usegamma = 5;
    mouseSensitivity = 4;
    r_bloodsplats_max = 32768;
    correct_lost_soul_bounce = true;
    dots_enabled = false;
    display_ticker = false;
    nerve_pwad = false;
    master_pwad = false;
    d_recoil = true;
    d_maxgore = true;
    d_thrust = true;
    respawnparm = false;
    fastparm = false;
    d_footstep = true;
    d_footclip = true;
    d_splash = true;
    d_translucency = true;
    d_chkblood = true;
    d_chkblood2 = true;
    d_uncappedframerate = 2;
    d_flipcorpses = true;
    d_secrets = true;
    smoketrails = false;
    sound_info = false;
    autodetect_hom = false;
    d_fallingdamage = true;
    d_infiniteammo = false;
    not_monsters = false;
    overlay_trigger = true;
    replace_missing = true;
    d_telefrag = false;
    d_doorstuck = false;
    d_resurrectghosts = false;
    d_limitedghosts = false;
    d_blockskulls = true;
    d_blazingsound = true;
    d_god = true;
    d_floors = false;
    d_moveblock = false;
    d_model = false;
    d_666 = false;
    d_maskedanim = false;
    d_sound = false;
    d_ouchface = false;
    show_authors = true;
    d_shadows = true;
    d_fixspriteoffsets = true;
    d_brightmaps = true;
    d_fixmaperrors = true;
    d_altlighting = true;
    allow_infighting = true;
    last_enemy = true;
    float_items = true;
    animated_drop = true;
    crush_sound = true;
    disable_noise = true;
    corpses_nudge = true;
    corpses_slide = true;
    corpses_smearblood = true;
    show_diskicon = true;
    randomly_colored_playercorpses = true;
    mousewalk = false;
    am_rotate = true;
    jumping = true;
    general_sound = true;
    lowhealth = true;
    d_fixwiggle = true;
    d_centerweapon = true;
    d_ejectcasings = true;
    d_statusmap = false;
    show_title = true;
    render_mode = false;
    d_drawparticles = true;
    d_drawbfgcloud = true;
    d_drawrockettrails = true;
    d_drawrocketexplosions = true;
    d_drawbfgexplosions = true;
    d_spawnflies = false;
    d_dripblood = true;
    d_vsync = true;
    particle_sounds = true;
    bloodsplat_particle = 1;
    bulletpuff_particle = 1;
    teleport_particle = 1;
    aiming_help = false;
    d_spawnteleglit = 3;
    png_screenshots = true;
    opl_stereo_correct = true;
    randompitch = true;
    hud = true;
    remove_slime_trails = true;
    aiming_help = false;
    enable_autoload = true;
    enable_autosave = true;
    stillbob = 0;
    movebob = 75;
    drawsplash = true;
    d_fliplevels = false;
    s_randommusic = false;
}

static void M_DefSet(int choice)
{
    musicVolume = 8;
    sfxVolume = 8;
    showMessages = true;
    drawgrid = false;
    followplayer = true;
    show_stats = false;
    timer_info = false;
    use_vanilla_weapon_change = true;
    chaingun_tics = 4;
    crosshair = false;
    d_colblood = false;
    d_colblood2 = false;
    d_swirl = false;
    autoaim = true;
    background_type = 0;
    icontype = false;
    wipe_type = 2;
    mouselook = 0;
    mspeed = 2;
    mus_engine = 1;
    snd_module = false;
    snd_chans = 1;
    sound_channels = 8;
    opl_type = false;
    use_libsamplerate = 0;
    gore_amount = 1;
    display_fps = false;
    font_shadow = 0;
    show_endoom = true;
    forwardmove = 29;
    sidemove = 24; 
    turnspeed = 7;
    detailLevel = false;
    screenblocks = 10;
    screenSize = 7;
    usegamma = 10;
    mouseSensitivity = 5;
    r_bloodsplats_max = 32768;
    correct_lost_soul_bounce = true;
    dots_enabled = false;
    display_ticker = false;
    nerve_pwad = false;
    master_pwad = false;
    d_recoil = false;
    d_maxgore = false;
    d_thrust = false;
    respawnparm = false;
    fastparm = false;
    d_footstep = false;
    d_footclip = false;
    d_splash = false;
    d_translucency = false;
    d_chkblood = false;
    d_chkblood2 = false;
    d_uncappedframerate = 0;
    d_flipcorpses = false;
    d_secrets = false;
    smoketrails = false;
    sound_info = false;
    autodetect_hom = false;
    d_fallingdamage = false;
    d_infiniteammo = false;
    not_monsters = false;
    overlay_trigger = false;
    replace_missing = false;
    d_telefrag = true;
    d_doorstuck = false;
    d_resurrectghosts = false;
    d_limitedghosts = true;
    d_blockskulls = false;
    d_blazingsound = false;
    d_god = true;
    d_floors = false;
    d_moveblock = false;
    d_model = false;
    d_666 = false;
    d_maskedanim = false;
    d_sound = false;
    d_ouchface = false;
    show_authors = false;
    d_shadows = false;
    d_fixspriteoffsets = false;
    d_brightmaps = false;
    d_fixmaperrors = false;
    d_altlighting = false;
    allow_infighting = false;
    last_enemy = false;
    float_items = false;
    animated_drop = false;
    crush_sound = false;
    disable_noise = false;
    corpses_nudge = false;
    corpses_slide = false;
    corpses_smearblood = false;
    show_diskicon = true;
    randomly_colored_playercorpses = false;
    mousewalk = true;
    am_rotate = false;
    jumping = false;
    general_sound = true;
    lowhealth = false;
    d_fixwiggle = false;
    d_centerweapon = false;
    d_ejectcasings = false;
    d_statusmap = true;
    show_title = true;
    render_mode = true;
    d_drawparticles = false;
    d_drawbfgcloud = false;
    d_drawrockettrails = false;
    d_drawrocketexplosions = false;
    d_drawbfgexplosions = false;
    d_spawnflies = false;
    d_dripblood = false;
    d_vsync = false;
    particle_sounds = false;
    bloodsplat_particle = 0;
    bulletpuff_particle = 0;
    teleport_particle = 0;
    aiming_help = false;
    d_spawnteleglit = 0;
    png_screenshots = false;
    opl_stereo_correct = false;
    randompitch = false;
    hud = false;
    remove_slime_trails = false;
    aiming_help = false;
    enable_autoload = false;
    enable_autosave = false;
    stillbob = 0;
    movebob = 75;
    drawsplash = false;
    d_fliplevels = false;
    s_randommusic = false;
}

static void M_Expansion(int choice)
{
    gamemission = (choice == ex1 ? doom2 : pack_nerve);

    M_SetupNextMenu(&NewDef);
}

/*
static void M_RMap(int choice)
{
    switch (choice)
    {
        case 0:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if (repi >= 1 && rmap >= 1)
                    rmap--;
            }
            else if (fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                fsize == 11159840)
            {
                if (repi >= 1 && rmap >= 1)
                {
                    rmap--;

                    if (repi == 3 && rmap == 0)
                    {
                        repi = 2;
                        rmap = 9;
                    }
                    else if (repi == 2 && rmap == 0)
                    {
                        repi = 1;
                        rmap = 9;
                    }
                }
            }
            else if (fsize == 12408292)
            {
                if (repi >= 1 && rmap >= 1)
                {
                    rmap--;

                    if (repi == 4 && rmap == 0)
                    {
                        repi = 3;
                        rmap = 9;
                    }
                    else if (repi == 3 && rmap == 0)
                    {
                        repi = 2;
                        rmap = 9;
                    }
                    else if (repi == 2 && rmap == 0)
                    {
                        repi = 1;
                        rmap = 9;
                    }
                }
            }
            else if (fsize == 12361532)
            {
                if (repi >= 1 && rmap >= 1)
                {
                    rmap--;
                }
            }
            else if (fsize == 14943400 || fsize == 14824716 || fsize == 14612688 ||
                fsize == 14607420 || fsize == 14604584 || fsize == 18195736 ||
                fsize == 18654796 || fsize == 18240172 || fsize == 17420824 ||
                fsize == 28422764)
            {
                if (rmap >= 2)
                    rmap--;
            }
            else if (fsize == 19321722)
            {
                if (rmap >= 2)
                    rmap--;

                if (rmap == 30)
                    rmap = 20;
            }

            break;

        case 1:
            if (fsize == 4261144 || fsize == 4271324 || fsize == 4211660 ||
                fsize == 4207819 || fsize == 4274218 || fsize == 4225504 ||
                fsize == 4225460 || fsize == 4234124 || fsize == 4196020)
            {
                if (repi <= 1 && rmap <= 9)
                {
                    rmap++;

                    if (repi == 1 && rmap == 10)
                    {
                        repi = 1;
                        rmap = 9;
                    }
                }
            }
            else if (fsize == 10396254 || fsize == 10399316 || fsize == 10401760 ||
                fsize == 11159840)
            {
                if (repi <= 3 && rmap <= 9)
                {
                    rmap++;

                    if (repi == 1 && rmap == 10)
                    {
                        repi = 2;
                        rmap = 1;
                    }
                    else if (repi == 2 && rmap == 10)
                    {
                        repi = 3;
                        rmap = 1;
                    }
                    else if (repi == 3 && rmap == 10)
                    {
                        repi = 3;
                        rmap = 9;
                    }
                }
            }
            else if (fsize == 12408292)
            {
                if (repi <= 4 && rmap <= 9)
                {
                    rmap++;

                    if (repi == 1 && rmap == 10)
                    {
                        repi = 2;
                        rmap = 1;
                    }
                    else if (repi == 2 && rmap == 10)
                    {
                        repi = 3;
                        rmap = 1;
                    }
                    else if (repi == 3 && rmap == 10)
                    {
                        repi = 4;
                        rmap = 1;
                    }
                    else if (repi == 4 && rmap == 10)
                    {
                        repi = 4;
                        rmap = 9;
                    }
                }
            }
            else if (fsize == 12361532)
            {
                if (repi <= 1 && rmap <= 4)
                {
                    rmap++;
                }
            }
            else if (fsize == 14943400 || fsize == 14612688 || fsize == 14607420 ||
                fsize == 14604584 || fsize == 18195736 || fsize == 18654796 ||
                fsize == 18240172 || fsize == 17420824 || fsize == 28422764)
            {
                if (rmap <= 31)
                    rmap++;
            }
            else if (fsize == 14824716)
            {
                if (rmap <= 29)
                    rmap++;
            }
            else if (fsize == 19321722)
            {
                if (rmap <= 30)
                    rmap++;

                if (rmap == 21)
                    rmap = 31;
            }

            break;
        }
}

static void M_RSkill(int choice)
{
    switch (choice)
    {
        case 0:
            if (rskill)
                rskill--;

            break;

        case 1:
            // HACK: NOT FOR SHARE 1.0 & 1.1 && REG 1.1
            if (fsize != 4207819 && fsize != 4274218 && fsize != 10396254)
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

static void M_RecordLong(int choice)
{
    switch (choice)
    {
        case 0:
            if (long_tics)
                long_tics = false;

            break;

        case 1:
            if (!long_tics)
                long_tics = true;

            break;
    }
}

static void M_StartRecord(int choice)
{
    if (!demoplayback)
    {
        M_ClearMenus();
        G_RecordDemo(rskill, repi, rmap);

        // never returns
        D_DoomLoop();
    }
    else if (demoplayback)
    {
        players[consoleplayer].message = "MAP ROTATION DISABLED";
    }

    if (long_tics)
        longtics = true;
    else
        longtics = false;
}
*/

static void M_Crosshair(int choice)
{
    switch (choice)
    {
        case 0:
            if (crosshair)
                crosshair = false;

            break;

        case 1:
            if (!crosshair)
                crosshair = true;

            break;
    }
}

static void M_Jumping(int choice)
{
    switch (choice)
    {
        case 0:
            if (jumping)
                jumping = false;

            break;

        case 1:
            if (!jumping)
                jumping = true;

            break;
    }
}

static void M_WeaponRecoil(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_recoil)
                d_recoil = false;

            break;

        case 1:
            if (!d_recoil)
                d_recoil = true;

            break;
    }
}

static void M_PlayerThrust(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_thrust)
                d_thrust = false;

            break;

        case 1:
            if (!d_thrust)
                d_thrust = true;

            break;
    }
}

static void M_TeleportGlitter(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_spawnteleglit > 0)
                d_spawnteleglit--;

            break;

        case 1:
            if (d_spawnteleglit < 3)
                d_spawnteleglit++;

            break;
    }
}

static void M_Particles(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_drawparticles)
                d_drawparticles = false;

            break;

        case 1:
            if (!d_drawparticles)
                d_drawparticles = true;

            break;
    }
}

static void M_BloodType(int choice)
{
    if (d_drawparticles)
    {
        switch (choice)
        {
            case 0:
                if (bloodsplat_particle > 0)
                    bloodsplat_particle--;

                break;

            case 1:
                if (bloodsplat_particle < 2)
                    bloodsplat_particle++;

                break;
        }
    }
}

static void M_BulletType(int choice)
{
    if (d_drawparticles)
    {
        switch (choice)
        {
            case 0:
                if (bulletpuff_particle > 0)
                    bulletpuff_particle--;

                break;

            case 1:
                if (bulletpuff_particle < 2)
                    bulletpuff_particle++;

                break;
        }
    }
}

static void M_TeleportType(int choice)
{
    if (d_drawparticles)
    {
        switch (choice)
        {
            case 0:
                if (teleport_particle > 0)
                    teleport_particle--;

                break;

            case 1:
                if (teleport_particle < 2)
                    teleport_particle++;

                break;
        }
    }
}

static void M_BFGCloud(int choice)
{
    if (d_drawparticles)
    {
        switch (choice)
        {
            case 0:
                if (d_drawbfgcloud)
                    d_drawbfgcloud = false;

                break;

            case 1:
                if (!d_drawbfgcloud)
                    d_drawbfgcloud = true;

                break;
        }
    }
}

static void M_BFGExplosions(int choice)
{
    if (d_drawparticles)
    {
        switch (choice)
        {
            case 0:
                if (d_drawbfgexplosions)
                    d_drawbfgexplosions = false;

                break;

            case 1:
                if (!d_drawbfgexplosions)
                    d_drawbfgexplosions = true;

                break;
        }
    }
}

static void M_RocketParticles(int choice)
{
    if (d_drawparticles)
    {
        switch (choice)
        {
            case 0:
                if (d_drawrockettrails)
                    d_drawrockettrails = false;

                break;

            case 1:
                if (!d_drawrockettrails)
                    d_drawrockettrails = true;

                break;
        }
    }
}

static void M_RocketExplosions(int choice)
{
    if (d_drawparticles)
    {
        switch (choice)
        {
            case 0:
                if (d_drawrocketexplosions)
                    d_drawrocketexplosions = false;

                break;

            case 1:
                if (!d_drawrocketexplosions)
                    d_drawrocketexplosions = true;

                break;
        }
    }
}

static void M_SpawnFlies(int choice)
{
    if (d_drawparticles && gamemode == commercial)
    {
        switch (choice)
        {
            case 0:
                if (d_spawnflies)
                    d_spawnflies = false;

                break;

            case 1:
                if (!d_spawnflies)
                    d_spawnflies = true;

                break;
        }
    }
}

static void M_DripBlood(int choice)
{
    if (d_drawparticles)
    {
        switch (choice)
        {
            case 0:
                if (d_dripblood)
                    d_dripblood = false;

                break;

            case 1:
                if (!d_dripblood)
                    d_dripblood = true;

                break;
        }
    }
}

static void M_ParticleSounds(int choice)
{
    switch (choice)
    {
        case 0:
            if (particle_sounds)
                particle_sounds = false;

            break;

        case 1:
            if (!particle_sounds)
                particle_sounds = true;

            break;
    }
}

static void M_RespawnMonsters(int choice)
{
    switch (choice)
    {
        case 0:
            if (respawnparm)
            {
                start_respawnparm = false;
                respawnparm = false;
            }

            break;

        case 1:
            if (!respawnparm)
            {
                start_respawnparm = true;
                respawnparm = true;
            }

            break;
    }
}

static void M_FastMonsters(int choice)
{
    switch (choice)
    {
        case 0:
            if (fastparm)
            {
                start_fastparm = false;
                fastparm = false;
            }

            break;

        case 1:
            if (!fastparm)
            {
                start_fastparm = true;
                fastparm = true;
            }

            break;
    }
}

static void M_Autoaim(int choice)
{
    switch (choice)
    {
        case 0:
            if (autoaim)
                autoaim = false;

            break;

        case 1:
            if (!autoaim)
                autoaim = true;

            break;
    }
}

static void M_MaxGore(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_maxgore && fsize != 12361532)
                d_maxgore = false;

            break;

        case 1:
            if (!d_maxgore && fsize != 12361532)
                d_maxgore = true;

            break;
    }
}

static void M_Footstep(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_footstep)
                d_footstep = false;

            break;

        case 1:
            if (!d_footstep)
                d_footstep = true;

            break;
    }
}

static void M_Footclip(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_footclip)
                d_footclip = false;

            break;

        case 1:
            if (!d_footclip)
                d_footclip = true;

            break;
    }
}

static void M_Splash(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_splash)
                d_splash = false;

            break;

        case 1:
            if (!d_splash)
                d_splash = true;

            break;
    }
}

static void M_Swirl(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_swirl)
                d_swirl = false;

            break;

        case 1:
            if (!d_swirl)
                d_swirl = true;

            break;
    }
}

static void M_Corpses(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_flipcorpses)
                d_flipcorpses = false;

            break;

        case 1:
            if (!d_flipcorpses)
                d_flipcorpses = true;

            break;
    }
}

static void M_Secrets(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_secrets)
                d_secrets = false;

            break;

        case 1:
            if (!d_secrets)
                d_secrets = true;

            break;
    }
}

static void M_Trails(int choice)
{
    switch (choice)
    {
        case 0:
            if (smoketrails)
                smoketrails = false;

            break;

        case 1:
            if (!smoketrails)
                smoketrails = true;

            break;
    }
}

static void M_ChaingunTics(int choice)
{
    if (gameskill != sk_nightmare)
    {
        switch (choice)
        {
            case 0:
                if (chaingun_tics < 4)
                    chaingun_tics++;

                break;

            case 1:
                if (chaingun_tics > 1)
                    chaingun_tics--;

                break;
        }
    }
}

static void M_FallingDamage(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_fallingdamage)
                d_fallingdamage = false;

            break;

        case 1:
            if (!d_fallingdamage)
                d_fallingdamage = true;

            break;
    }
}

static void M_InfiniteAmmo(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_infiniteammo)
                d_infiniteammo = false;

            break;

        case 1:
            if (!d_infiniteammo)
                d_infiniteammo = true;

            break;
    }
}

static void M_Shadows(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_shadows)
                d_shadows = false;

            break;

        case 1:
            if (!d_shadows)
                d_shadows = true;

            break;
    }
}

static void M_Offsets(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_fixspriteoffsets)
                d_fixspriteoffsets = false;

            break;

        case 1:
            if (!d_fixspriteoffsets)
                d_fixspriteoffsets = true;

            break;
    }
}

static void M_Telefrag(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_telefrag)
                d_telefrag = false;

            break;

        case 1:
            if (!d_telefrag)
                d_telefrag = true;

            break;
    }
}

static void M_Doorstuck(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_doorstuck)
                d_doorstuck = false;

            break;

        case 1:
            if (!d_doorstuck)
                d_doorstuck = true;

            break;
    }
}

static void M_ResurrectGhosts(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_resurrectghosts)
                d_resurrectghosts = false;

            break;

        case 1:
            if (!d_resurrectghosts)
                d_resurrectghosts = true;

            break;
    }
}

static void M_LimitedGhosts(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_limitedghosts)
                d_limitedghosts = false;

            break;

        case 1:
            if (!d_limitedghosts)
                d_limitedghosts = true;

            break;
    }
}

static void M_BlockSkulls(int choice)
{
    switch (choice)
    {
        case 0:
            if (!d_blockskulls)
                d_blockskulls = true;

            break;

        case 1:
            if (d_blockskulls)
                d_blockskulls = false;

            break;
    }
}

static void M_BlazingDoors(int choice)
{
    switch (choice)
    {
        case 0:
            if (!d_blazingsound)
                d_blazingsound = true;

            break;

        case 1:
            if (d_blazingsound)
                d_blazingsound = false;

            break;
    }
}

static void M_GodAbsolute(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_god)
                d_god = false;

            break;

        case 1:
            if (!d_god)
                d_god = true;

            break;
    }
}

static void M_Floor(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_floors)
                d_floors = false;

            break;

        case 1:
            if (!d_floors)
                d_floors = true;

            break;
    }
}

static void M_Clipping(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_moveblock)
                d_moveblock = false;

            break;

        case 1:
            if (!d_moveblock)
                d_moveblock = true;

            break;
    }
}

static void M_Model(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_model)
                d_model = false;

            break;

        case 1:
            if (!d_model)
                d_model = true;

            break;
    }
}

static void M_BossDeath(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_666)
                d_666 = false;

            break;

        case 1:
            if (!d_666)
                d_666 = true;

            break;
    }
}

static void M_Bounce(int choice)
{
    switch (choice)
    {
        case 0:
            if (correct_lost_soul_bounce)
                correct_lost_soul_bounce = false;

            break;

        case 1:
            if (!correct_lost_soul_bounce)
                correct_lost_soul_bounce = true;

            break;
    }
}

static void M_Masked(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_maskedanim)
                d_maskedanim = false;

            break;

        case 1:
            if (!d_maskedanim)
                d_maskedanim = true;

            break;
    }
}

static void M_Quirk(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_sound)
                d_sound = false;

            break;

        case 1:
            if (!d_sound)
                d_sound = true;

            break;
    }
}

static void M_Ouch(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_ouchface)
                d_ouchface = false;

            break;

        case 1:
            if (!d_ouchface)
                d_ouchface = true;

            break;
    }
}

static void M_Textures(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_brightmaps)
                d_brightmaps = false;

            break;

        case 1:
            if (!d_brightmaps)
                d_brightmaps = true;

            break;
    }
}

static void M_FixMapErrors(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_fixmaperrors)
                d_fixmaperrors = false;

            break;

        case 1:
            if (!d_fixmaperrors)
                d_fixmaperrors = true;

            break;
    }
}

static void M_AltLighting(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_altlighting)
                d_altlighting = false;

            break;

        case 1:
            if (!d_altlighting)
                d_altlighting = true;

            break;
    }
}

static void M_Infighting(int choice)
{
    switch (choice)
    {
        case 0:
            if (allow_infighting)
                allow_infighting = false;

            break;

        case 1:
            if (!allow_infighting)
                allow_infighting = true;

            break;
    }
}

static void M_LastEnemy(int choice)
{
    switch (choice)
    {
        case 0:
            if (last_enemy)
                last_enemy = false;

            break;

        case 1:
            if (!last_enemy)
                last_enemy = true;

            break;
    }
}

static void M_Float(int choice)
{
    switch (choice)
    {
        case 0:
            if (float_items)
                float_items = false;

            break;

        case 1:
            if (!float_items)
                float_items = true;

            break;
    }
}

static void M_Animate(int choice)
{
    switch (choice)
    {
        case 0:
            if (animated_drop)
                animated_drop = false;

            break;

        case 1:
            if (!animated_drop)
                animated_drop = true;

            break;
    }
}

static void M_CrushSound(int choice)
{
    switch (choice)
    {
        case 0:
            if (crush_sound)
                crush_sound = false;

            break;

        case 1:
            if (!crush_sound)
                crush_sound = true;

            break;
    }
}

static void M_NoNoise(int choice)
{
    switch (choice)
    {
        case 0:
            if (disable_noise)
                disable_noise = false;

            break;

        case 1:
            if (!disable_noise)
                disable_noise = true;

            break;
    }
}

static void M_NudgeCorpses(int choice)
{
    switch (choice)
    {
        case 0:
            if (corpses_nudge)
                corpses_nudge = false;

            break;

        case 1:
            if (!corpses_nudge)
                corpses_nudge = true;

            break;
    }
}

static void M_Slide(int choice)
{
    switch (choice)
    {
        case 0:
            if (corpses_slide)
                corpses_slide = false;

            if (corpses_smearblood)
                corpses_smearblood = false;

            break;

        case 1:
            if (!corpses_slide)
                corpses_slide = true;

            break;
    }
}

static void M_Smearblood(int choice)
{
    if (corpses_slide)
    {
        switch (choice)
        {
            case 0:
                if (corpses_smearblood)
                    corpses_smearblood = false;

                break;

            case 1:
                if (!corpses_smearblood)
                    corpses_smearblood = true;

                break;
        }
    }
}

static void M_ColoredCorpses(int choice)
{
    switch (choice)
    {
        case 0:
            if (randomly_colored_playercorpses)
                randomly_colored_playercorpses = false;

            break;

        case 1:
            if (!randomly_colored_playercorpses)
                randomly_colored_playercorpses = true;

            break;
    }
}

static void M_LowHealth(int choice)
{
    switch (choice)
    {
        case 0:
            if (lowhealth)
                lowhealth = false;

            break;

        case 1:
            if (!lowhealth)
                lowhealth = true;

            break;
    }
}

static void M_CenterWeapon(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_centerweapon)
                d_centerweapon = false;

            break;

        case 1:
            if (!d_centerweapon)
                d_centerweapon = true;

            break;
    }
}

static void M_EjectCasings(int choice)
{
    if (fsize != 12361532)
    {
        switch (choice)
        {
            case 0:
                if (d_ejectcasings)
                    d_ejectcasings = false;

                break;

            case 1:
                if (!d_ejectcasings)
                    d_ejectcasings = true;

                break;
        }
    }
}

static void M_EndoomScreen(int choice)
{
    switch (choice)
    {
        case 0:
            if (show_endoom)
                show_endoom = false;

            break;

        case 1:
#ifndef WII
            if (!show_endoom)
                show_endoom = true;
#endif
            break;
    }
}

static void M_GoreAmount(int choice)
{
    if (d_maxgore)
    {
        switch (choice)
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
    }
}

static void M_NoMonsters(int choice)
{
    switch (choice)
    {
        case 0:
            if (not_monsters)
                not_monsters = false;

            break;

        case 1:
            if (!not_monsters)
                not_monsters = true;

            break;
    }
}

static void M_AutomapOverlay(int choice)
{
    switch (choice)
    {
        case 0:
            if (overlay_trigger)
                overlay_trigger = false;

            break;

        case 1:
            if (!overlay_trigger)
                overlay_trigger = true;

            break;
    }

    HU_Start();
}

static void M_Debug(int choice)
{
    M_SetupNextMenu(&DebugDef);
}

static void M_Test(int choice)
{
    M_SetupNextMenu(&TestDef);

    mobj_state = &states[mobjinfo[MT_PLAYER].seestate];
    mobj_tics = mobj_state->tics;
}

static void M_DrawSystem(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow (62, 20, 0, W_CacheLumpName("M_T_YSET", PU_CACHE), false);
    else
        V_DrawPatchWithShadow (62, 20, 0, W_CacheLumpName("M_SYSSET", PU_CACHE), false);

    if (display_fps)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SystemDef.x + 198, SystemDef.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SystemDef.x + 190, SystemDef.y - 2, "OFF");
    }

    if (display_ticker)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SystemDef.x + 198, SystemDef.y + 8, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SystemDef.x + 190, SystemDef.y + 8, "OFF");
    }

    if (autodetect_hom)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SystemDef.x + 198, SystemDef.y + 18, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SystemDef.x + 190, SystemDef.y + 18, "OFF");
    }

    if (replace_missing)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SystemDef.x + 198, SystemDef.y + 28, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SystemDef.x + 190, SystemDef.y + 28, "OFF");
    }

    if (precache)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(SystemDef.x + 198, SystemDef.y + 38, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(SystemDef.x + 190, SystemDef.y + 38, "OFF");
    }
}

static void M_HUD(int choice)
{
    switch (choice)
    {
        case 0:
            if (hud)
                hud = false;

            break;

        case 1:
            if (!hud)
                hud = true;

            break;
    }
}

static void M_Autoload(int choice)
{
    switch (choice)
    {
        case 0:
            if (enable_autoload)
                enable_autoload = false;

            break;

        case 1:
            if (!enable_autoload)
                enable_autoload = true;

            break;
    }
}

static void M_Autosave(int choice)
{
    switch (choice)
    {
        case 0:
            if (enable_autosave)
                enable_autosave = false;

            break;

        case 1:
            if (!enable_autosave)
                enable_autosave = true;

            break;
    }
}

static void M_DrawSplash(int choice)
{
    switch (choice)
    {
        case 0:
            if (drawsplash)
                drawsplash = false;

            break;

        case 1:
            if (!drawsplash)
                drawsplash = true;

            break;
    }
}

static void M_Stillbob(int choice)
{
    switch (choice)
    {
        case 0:
            if (stillbob > 0)
                stillbob--;

            break;

        case 1:
            if (stillbob < 100)
                stillbob++;

            break;
    }
}

static void M_Movebob(int choice)
{
    switch (choice)
    {
        case 0:
            if (movebob > 0)
                movebob--;

            break;

        case 1:
            if (movebob < 100)
                movebob++;

            break;
    }
}

static void M_Flip(int choice)
{
    switch (choice)
    {
        case 0:
            if (d_fliplevels)
                d_fliplevels = false;

            break;

        case 1:
            if (!d_fliplevels)
                d_fliplevels = true;

            break;
    }

    if (usergame)
        must_recreate_blockmap = true;
}

static void M_MapColor_Back(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_back > 0)
                mapcolor_back--;

            break;

        case 1:
            if (mapcolor_back < 255)
                mapcolor_back++;

            break;
    }
}

static void M_MapColor_Grid(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_grid > 0)
                mapcolor_grid--;

            break;

        case 1:
            if (mapcolor_grid < 255)
                mapcolor_grid++;

            break;
    }
}

static void M_MapColor_Wall(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_wall > 0)
                mapcolor_wall--;

            break;

        case 1:
            if (mapcolor_wall < 255)
                mapcolor_wall++;

            break;
    }
}

static void M_MapColor_FCHG(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_fchg > 0)
                mapcolor_fchg--;

            break;

        case 1:
            if (mapcolor_fchg < 255)
                mapcolor_fchg++;

            break;
    }
}

static void M_MapColor_CCHG(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_cchg > 0)
                mapcolor_cchg--;

            break;

        case 1:
            if (mapcolor_cchg < 255)
                mapcolor_cchg++;

            break;
    }
}

static void M_MapColor_CLSD(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_clsd > 0)
                mapcolor_clsd--;

            break;

        case 1:
            if (mapcolor_clsd < 255)
                mapcolor_clsd++;

            break;
    }
}

static void M_MapColor_RKey(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_rkey > 0)
                mapcolor_rkey--;

            break;

        case 1:
            if (mapcolor_rkey < 255)
                mapcolor_rkey++;

            break;
    }
}

static void M_MapColor_BKey(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_bkey > 0)
                mapcolor_bkey--;

            break;

        case 1:
            if (mapcolor_bkey < 255)
                mapcolor_bkey++;

            break;
    }
}

static void M_MapColor_YKey(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_ykey > 0)
                mapcolor_ykey--;

            break;

        case 1:
            if (mapcolor_ykey < 255)
                mapcolor_ykey++;

            break;
    }
}

static void M_MapColor_RDoor(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_rdor > 0)
                mapcolor_rdor--;

            break;

        case 1:
            if (mapcolor_rdor < 255)
                mapcolor_rdor++;

            break;
    }
}

static void M_MapColor_BDoor(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_bdor > 0)
                mapcolor_bdor--;

            break;

        case 1:
            if (mapcolor_bdor < 255)
                mapcolor_bdor++;

            break;
    }
}

static void M_MapColor_YDoor(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_ydor > 0)
                mapcolor_ydor--;

            break;

        case 1:
            if (mapcolor_ydor < 255)
                mapcolor_ydor++;

            break;
    }
}

static void M_MapColor_Teleport(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_tele > 0)
                mapcolor_tele--;

            break;

        case 1:
            if (mapcolor_tele < 255)
                mapcolor_tele++;

            break;
    }
}

static void M_MapColor_Secret(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_secr > 0)
                mapcolor_secr--;

            break;

        case 1:
            if (mapcolor_secr < 255)
                mapcolor_secr++;

            break;
    }
}

static void M_MapColor_Exit(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_exit > 0)
                mapcolor_exit--;

            break;

        case 1:
            if (mapcolor_exit < 255)
                mapcolor_exit++;

            break;
    }
}

static void M_MapColor_Unseen(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_unsn > 0)
                mapcolor_unsn--;

            break;

        case 1:
            if (mapcolor_unsn < 255)
                mapcolor_unsn++;

            break;
    }
}

static void M_MapColor_Flat(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_flat > 0)
                mapcolor_flat--;

            break;

        case 1:
            if (mapcolor_flat < 255)
                mapcolor_flat++;

            break;
    }
}

static void M_MapColor_Sprite(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_sprt > 0)
                mapcolor_sprt--;

            break;

        case 1:
            if (mapcolor_sprt < 255)
                mapcolor_sprt++;

            break;
    }
}

static void M_MapColor_Item(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_item > 0)
                mapcolor_item--;

            break;

        case 1:
            if (mapcolor_item < 255)
                mapcolor_item++;

            break;
    }
}

static void M_MapColor_Enemy(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_enemy > 0)
                mapcolor_enemy--;

            break;

        case 1:
            if (mapcolor_enemy < 255)
                mapcolor_enemy++;

            break;
    }
}

static void M_MapColor_Crosshair(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_hair > 0)
                mapcolor_hair--;

            break;

        case 1:
            if (mapcolor_hair < 255)
                mapcolor_hair++;

            break;
    }
}

static void M_MapColor_Player(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_sngl > 0)
                mapcolor_sngl--;

            break;

        case 1:
            if (mapcolor_sngl < 255)
                mapcolor_sngl++;

            break;
    }
}

static void M_MapColor_Multiplayer(int choice)
{
    switch (choice)
    {
        case 0:
            if (mapcolor_plyr[consoleplayer] > 0)
                mapcolor_plyr[consoleplayer]--;

            break;

        case 1:
            if (mapcolor_plyr[consoleplayer] < 255)
                mapcolor_plyr[consoleplayer]++;

            break;
    }
}

static void M_MapColor_Standard_PRBoom(int choice)
{
    mapcolor_back = 247;
    mapcolor_grid = 104;
    mapcolor_wall = 23;
    mapcolor_fchg = 55;
    mapcolor_cchg = 215;
    mapcolor_clsd = 208;
    mapcolor_rkey = 175;
    mapcolor_bkey = 204;
    mapcolor_ykey = 231;
    mapcolor_rdor = 175;
    mapcolor_bdor = 204;
    mapcolor_ydor = 231;
    mapcolor_tele = 119;
    mapcolor_secr = 252;
    mapcolor_exit = 0;
    mapcolor_unsn = 104;
    mapcolor_flat = 88;
    mapcolor_sprt = 112;
    mapcolor_item = 231;
    mapcolor_enemy = 177;
    mapcolor_hair = 208;
    mapcolor_sngl = 208;
    mapcolor_plyr[consoleplayer] = 112;
}
int lightlev;
static void M_MapColor_Standard_DOSDoom(int choice)
{
    mapcolor_back = BACKGROUND;
    mapcolor_grid = GRIDCOLORS;
    mapcolor_wall = WALLCOLORS+lightlev;
    mapcolor_fchg = FDWALLCOLORS+lightlev;
    mapcolor_cchg = CDWALLCOLORS+lightlev;
    mapcolor_clsd = CDWALLCOLORS+lightlev;
    mapcolor_rkey = THINGCOLORS+lightlev;
    mapcolor_bkey = THINGCOLORS+lightlev;
    mapcolor_ykey = THINGCOLORS+lightlev;
    mapcolor_rdor = CDWALLCOLORS+lightlev;
    mapcolor_bdor = CDWALLCOLORS+lightlev;
    mapcolor_ydor = CDWALLCOLORS+lightlev;
    mapcolor_tele = FDWALLCOLORS+lightlev;
    mapcolor_secr = SECRETWALLCOLORS + lightlev;
    mapcolor_exit = 0;
    mapcolor_unsn = GRAYS+3;
    mapcolor_flat = TSWALLCOLORS+lightlev;
    mapcolor_sprt = THINGCOLORS+lightlev;
    mapcolor_item = THINGCOLORS+lightlev;
    mapcolor_enemy = THINGCOLORS+lightlev;
    mapcolor_hair = XHAIRCOLORS;
    mapcolor_sngl = WHITE;
    mapcolor_plyr[consoleplayer] = WHITE;
}

static void M_MapGrid(int choice)
{
    switch (choice)
    {
        case 0:
            if (drawgrid)
                drawgrid = false;

            break;

        case 1:
            if (!drawgrid)
                drawgrid = true;

            break;
    }
}

static void M_MapGridSize(int choice)
{
    switch (choice)
    {
        case 0:
            if (map_grid_size > 10)
                map_grid_size--;

            break;

        case 1:
            if (map_grid_size < 256)
                map_grid_size++;

            break;
    }
}

static void M_MapRotation(int choice)
{
    switch (choice)
    {
        case 0:
            if (am_rotate)
                am_rotate = false;

            break;

        case 1:
            if (!am_rotate)
                am_rotate = true;

            break;
    }
}

static void M_WeaponChange(int choice)
{
    if (gameskill != sk_nightmare)
    {
        switch (choice)
        {
            case 0:
                if (!use_vanilla_weapon_change)
                    use_vanilla_weapon_change = true;

                break;

            case 1:
                if (use_vanilla_weapon_change)
                    use_vanilla_weapon_change = false;

                break;
        }
    }
}

static void M_AimingHelp(int choice)
{
    if (!(players[consoleplayer].cheats & CF_GODMODE) && gameskill != sk_nightmare)
    {
        switch (choice)
        {
            case 0:
                if (aiming_help)
                    aiming_help = false;

                break;

            case 1:
                if (!aiming_help)
                    aiming_help = true;

                break;
        }
    }
}

static void M_FollowMode(int choice)
{
    switch (choice)
    {
        case 0:
            if (followplayer)
                followplayer = false;

            break;

        case 1:
            if (!followplayer)
                followplayer = true;

            break;
    }
}

static void M_Statistics(int choice)
{
    switch (choice)
    {
        case 0:
            if (show_stats)
                show_stats = false;

            break;

        case 1:
            if (!show_stats)
                show_stats = true;

            break;
    }

    HU_Start();
}

static void M_DrawDebug(void)
{
    if (fsize != 19321722 && fsize != 12361532 && fsize != 28422764)
        V_DrawPatchWithShadow (67, 15, 0, W_CacheLumpName("M_T_DSET", PU_CACHE), false);
    else
        V_DrawPatchWithShadow (67, 15, 0, W_CacheLumpName("M_DBGSET", PU_CACHE), false);

    if (coordinates_info)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(DebugDef.x + 253, DebugDef.y - 2, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(DebugDef.x + 245, DebugDef.y - 2, "OFF");
    }

    if (version_info)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(DebugDef.x + 253, DebugDef.y + 8, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(DebugDef.x + 245, DebugDef.y + 8, "OFF");
    }

    if (sound_info)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(DebugDef.x + 253, DebugDef.y + 18, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(DebugDef.x + 245, DebugDef.y + 18, "OFF");
    }

    if (opldev)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(DebugDef.x + 253, DebugDef.y + 28, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(DebugDef.x + 245, DebugDef.y + 28, "OFF");
    }

    if (memory_usage)
    {
        dp_translation = crx[CRX_GREEN];
        M_WriteText(DebugDef.x + 253, DebugDef.y + 38, "ON");
    }
    else
    {
        dp_translation = crx[CRX_DARK];
        M_WriteText(DebugDef.x + 245, DebugDef.y + 38, "OFF");
    }

    if (restart_song)
    {
        if (restartsongwait < 5)
        {
            dp_translation = crx[CRX_GREEN];
            M_WriteText(DebugDef.x + 245, DebugDef.y + 48, "DONE");
        }
        else
        {
            restartsongwait = 0;
            restart_song = false;
        }
    }

    if (dump_con)
    {
        if (condumpwait < 5)
        {
            dp_translation = crx[CRX_GREEN];
            M_WriteText(DebugDef.x + 245, DebugDef.y + 58, "DONE");
        }
        else
        {
            condumpwait = 0;
            dump_con = false;
        }
    }

    if (dump_mem)
    {
        if (memdumpwait < 5)
        {
            dp_translation = crx[CRX_GREEN];
            M_WriteText(DebugDef.x + 245, DebugDef.y + 68, "DONE");
        }
        else
        {
            memdumpwait = 0;
            dump_mem = false;
        }
    }

    if (dump_stat)
    {
        if (statdumpwait < 5)
        {
            dp_translation = crx[CRX_GREEN];
            M_WriteText(DebugDef.x + 245, DebugDef.y + 78, "DONE");
        }
        else
        {
            statdumpwait = 0;
            dump_stat = false;
        }
    }

    if (printdir)
    {
        if (printdirwait < 5)
        {
            dp_translation = crx[CRX_GREEN];
            M_WriteText(DebugDef.x + 245, DebugDef.y + 88, "DONE");
        }
        else
        {
            printdirwait = 0;
            printdir = false;
        }
    }
}

static void M_XC(int choice)
{
    switch (choice)
    {
        case 0:
            if (firexc > -120)
                firexc--;

            break;

        case 1:
            if (firexc < 120)
                firexc++;

            break;
    }
}

static void M_YC(int choice)
{
    switch (choice)
    {
        case 0:
            if (fireyc > 0)
                fireyc--;

            break;

        case 1:
            if (fireyc < 113)
                fireyc++;

            break;
    }
}

static void M_XFac(int choice)
{
    switch (choice)
    {
        case 0:
            if (firexfac > 1)
                firexfac--;

            break;

        case 1:
            if (firexfac < 4)
                firexfac++;

            break;
    }
}

static void M_YFac(int choice)
{
    switch (choice)
    {
        case 0:
            if (fireyfac > 1)
                fireyfac--;

            break;

        case 1:
            if (fireyfac < 4)
                fireyfac++;

            break;
    }
}

static void M_Pitch(int choice)
{
    switch (choice)
    {
        case 0:
            if (firepitch > 0)
                firepitch--;

            break;

        case 1:
            firepitch++;
            break;
    }
}

static void M_Offset(int choice)
{
    switch (choice)
    {
        case 0:
            if (fireoffset > 0)
                fireoffset--;

            break;

        case 1:
            fireoffset++;
            break;
    }
}

static void M_Width(int choice)
{
    switch (choice)
    {
        case 0:
            if (firewidth > 0)
                firewidth--;

            break;

        case 1:
            firewidth++;
            break;
    }
}

static void M_Height(int choice)
{
    switch (choice)
    {
        case 0:
            if (fireheight > 0)
                fireheight--;

            break;

        case 1:
            fireheight++;
            break;
    }
}

static void M_WidthHeightPitch(int choice)
{
    switch (choice)
    {
        case 0:
            fireheight--;
            firewidth--;
            firepitch--;
            break;

        case 1:
            fireheight++;
            firewidth++;
            firepitch++;
            break;
    }
}

static void M_DrawTest(void)
{
    spriteframe_t *sprframe = &sprites[states[mobjinfo[MT_PLAYER].seestate].sprite].spriteframes[mobj_state->frame & FF_FRAMEMASK];

    // note: width must be divisible by 4
    int width = firewidth;
    int height = fireheight;
    int pitch = firepitch;
    int CleanXfac = firexfac;
    int CleanYfac = fireyfac;
    int x = ORIGINALHEIGHT + firexc - 72;
    int y = TestDef.y - fireyc + LINEHEIGHT * 3 + 64;
    int lump = sprframe->lump[0];
    int a;
    int b;

    byte *from;

    patch_t *patch = W_CacheLumpNum (lump + firstspritelump, PU_CACHE);

    sprintf(xc_textbuffer, "%d", firexc);
    sprintf(yc_textbuffer, "%d", fireyc);
    sprintf(xfac_textbuffer, "%d", firexfac);
    sprintf(yfac_textbuffer, "%d", fireyfac);
    sprintf(pitch_textbuffer, "%d", firepitch);
    sprintf(width_textbuffer, "%d", firewidth);
    sprintf(height_textbuffer, "%d", fireheight);
    sprintf(offset_textbuffer, "%d", fireoffset);

    M_WriteText(TestDef.x + 245, TestDef.y - 2, pitch_textbuffer);
    M_WriteText(TestDef.x + 245, TestDef.y + 8, width_textbuffer);
    M_WriteText(TestDef.x + 245, TestDef.y + 18, height_textbuffer);
    M_WriteText(TestDef.x + 245, TestDef.y + 28, offset_textbuffer);
    M_WriteText(TestDef.x + 245, TestDef.y + 48, xfac_textbuffer);
    M_WriteText(TestDef.x + 245, TestDef.y + 58, yfac_textbuffer);
    M_WriteText(TestDef.x + 245, TestDef.y + 68, xc_textbuffer);
    M_WriteText(TestDef.x + 245, TestDef.y + 78, yc_textbuffer);

    x = (x - ORIGINALWIDTH / 2) * CleanXfac + (SCREENWIDTH >> 1);
    y = (y - ORIGINALHEIGHT / 2) * CleanYfac + (SCREENHEIGHT >> 1);

    // [RH] The following fire code is based on the PTC fire demo
    from = firescreen + (height - 3) * pitch;

    for (a = 0; a < width; a++, from++)
    {
        *from = *(from + (pitch << 1)) = M_RandomSMMU();
    }

    from = firescreen;

    for (b = 0; b < fireheight - 4; b += 2)
    {
        byte *pixel = from;

        // special case: first pixel on line
        byte *p = pixel + (pitch << 1);

        unsigned int top = *p + *(p + width - 1) + *(p + 1);
        unsigned int bottom = *(pixel + (pitch << 2));
        unsigned int c1 = (top + bottom) >> 2;

        if (c1 > 1)
            c1--;

        *pixel = c1;
        *(pixel + pitch) = (c1 + bottom) >> 1;
        pixel++;

        // main line loop
        for (a = 1; a < width - 1; a++)
        {
            // sum top pixels
            p = pixel + (pitch << 1);
            top = *p + *(p - 1) + *(p + 1);

            // bottom pixel
            bottom = *(pixel + (pitch << 2));

            // combine pixels
            c1 = (top + bottom) >> 2;

            if (c1 > 1)
                c1--;

            // store pixels
            *pixel = c1;

            // interpolate
            *(pixel + pitch) = (c1 + bottom) >> 1;

            // next pixel
            pixel++;
        }

        // special case: last pixel on line
        p = pixel + (pitch << 1);
        top = *p + *(p - 1) + *(p - width + 1);
        bottom = *(pixel + (pitch << 2));
        c1 = (top + bottom) >> 2;

        if (c1 > 1)
            c1--;

        *pixel = c1;
        *(pixel + pitch) = (c1 + bottom) >> 1;

        // next line
        from += pitch << 1;
    }

    y--;
    pitch = SCREENWIDTH;

    switch (CleanXfac)
    {
        case 1:
            for (b = 0; b < fireheight - fireoffset; b++)
            {
                byte *to = screens[0] + y * SCREENWIDTH + x;
                from = firescreen + b * firepitch;
                y += CleanYfac;

                for (a = 0; a < firewidth; a++, to++, from++)
                {
                    int c;

                    for (c = CleanYfac; c; c--)
                        *(to + pitch * c) = fireremap[*from];
                }
            }

            break;

        case 2:
            for (b = 0; b < fireheight - fireoffset; b++)
            {
                byte *to = screens[0] + y * SCREENWIDTH + x;
                from = firescreen + b * firepitch;
                y += CleanYfac;

                for (a = 0; a < firewidth; a++, to += 2, from++)
                {
                    int c;

                    for (c = CleanYfac; c; c--)
                    {
                        *(to + pitch * c) = fireremap[*from];
                        *(to + pitch * c + 1) = fireremap[*from];
                    }
                }
            }

            break;

        case 3:
            for (b = 0; b < fireheight - fireoffset; b++)
            {
                byte *to = screens[0] + y * SCREENWIDTH + x;
                from = firescreen + b * firepitch;
                y += CleanYfac;

                for (a = 0; a < firewidth; a++, to += 3, from++)
                {
                    int c;

                    for (c = CleanYfac; c; c--)
                    {
                        *(to + pitch * c) = fireremap[*from];
                        *(to + pitch * c + 1) = fireremap[*from];
                        *(to + pitch * c + 2) = fireremap[*from];
                    }
                }
            }

            break;

        case 4:
        default:
            for (b = 0; b < fireheight - fireoffset; b++)
            {
                byte *to = screens[0] + y * SCREENWIDTH + x;
                from = firescreen + b * firepitch;
                y += CleanYfac;

                for (a = 0; a < firewidth; a++, to += 4, from++)
                {
                    int c;

                    for (c = CleanYfac; c; c--)
                    {
                        *(to + pitch * c) = fireremap[*from];
                        *(to + pitch * c + 1) = fireremap[*from];
                        *(to + pitch * c + 2) = fireremap[*from];
                        *(to + pitch * c + 3) = fireremap[*from];
                    }
                }
            }

            break;
    }

    V_DrawPatch(160 + firexc, TestDef.y - fireyc + LINEHEIGHT * 3 + 100, 0, W_CacheLumpName ("M_PBOX", PU_CACHE));
    V_DrawPatch(162 + firexc, TestDef.y - fireyc + LINEHEIGHT * 3 + 124, 0, patch);
}

#ifdef WII
// jff 2/01/98 kill all monsters
static void M_Massacre(int choice)
{
    thinker_t *thinker = &thinkercap;
    int       killcount = 0;

    massacre_cheat_used = true;

    // jff 02/01/98 'em' cheat - kill all monsters
    // partially taken from Chi's .46 port

    // killough 2/7/98: cleaned up code and changed to use dprintf;
    // fixed lost soul bug (Lost Souls left behind when Pain Elementals are killed)
    while ((thinker=thinker->next) != &thinkercap)
    {
        if (thinker->function == P_MobjThinker &&
            ((((mobj_t *)thinker)->flags & MF_COUNTKILL) ||
            ((mobj_t *)thinker)->type == MT_SKULL ||
            ((mobj_t *)thinker)->type == MT_BETASKULL))
        {
            // killough 3/6/98: kill even if Pain Elemental is dead
            if (((mobj_t *)thinker)->health > 0)
            {
                killcount++;

                P_DamageMobj((mobj_t *)thinker, NULL, NULL, 10000);
            }

            if (((mobj_t *)thinker)->type == MT_PAIN)
            {
                // killough 2/8/98
                A_PainDie((mobj_t *)thinker);

                P_SetMobjState((mobj_t *)thinker, S_PAIN_DIE6);
            }
        }
    }

    // killough 3/22/98: make more intelligent about plural

    // Ty 03/27/98 - string(s) *not* externalized
    sprintf(massacre_textbuffer, "%d MONSTER%s KILLED",
        killcount, killcount == 1 ? "" : "S");

    players[consoleplayer].message = massacre_textbuffer;
    massacre_cheat_used = false;
}
#endif

