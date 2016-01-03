/*
========================================================================

                               DOOM Retro
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright (C) 2013-2015 Brad Harding.

  DOOM Retro is a fork of Chocolate DOOM by Simon Howard.
  For a complete list of credits, see the accompanying AUTHORS file.

  This file is part of DOOM Retro.

  DOOM Retro is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM Retro is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM Retro. If not, see <http://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM Retro is in no way affiliated with nor endorsed by
  id Software LLC.

========================================================================
*/

#if !defined(__P_FIX__)
#define __P_FIX__

#define DEFAULT 0x7FFF
#define REMOVE  0

#define E2M2    (gamemission == doom && gameepisode == 2 && gamemap == 2 && canmodify)
#define MAP12   (gamemission == doom2 && gamemap == 12 && canmodify)
/*
#define Player1Start                                   1
#define Player2Start                                   2
#define Player3Start                                   3
#define Player4Start                                   4
#define YellowKeycard                                  6
#define Backpack                                       8
#define ShotgunGuy                                     9
#define PlayerDeathmatchStart                          11
#define TeleportDestination                            14
#define DeadPlayer                                     15
#define CellPack                                       17
#define ShortGreenFirestick                            56
#define Spectre                                        58
#define HangingVictimOneLegged                         61
#define HangingLeg                                     62
#define HangingVictimTwitching                         63
#define ArchVile                                       64
#define HeavyWeaponDude                                65
#define Revenant                                       66
#define Mancubus                                       67
#define Arachnotron                                    68
#define HellKnight                                     69
#define BurningBarrel                                  70
#define PainElemental                                  71
#define MegaSphere                                     83
#define WolfensteinSS                                  84
#define ShortTechnoFloorLamp                           86
#define BossBrain                                      88
#define MonstersSpawner                                89
#define Chaingun                                       2002
#define Chainsaw                                       2005
#define Rocket                                         2010
#define Medikit                                        2012
#define Berserk                                        2023
#define Barrel                                         2035
#define BoxOfRockets                                   2046
#define Cell                                           2047
#define BoxOfBullets                                   2048
#define BoxOfShells                                    2049
#define Imp                                            3001
#define Demon                                          3002
#define Zombieman                                      3004
#define Cacodemon                                      3005
#define LostSoul                                       3006
*/
#define MTF_NETGAME                                    16
/*
#define DR_Door_OpenWaitClose_AlsoMonsters             1
#define W1_Floor_RaiseToLowestCeiling                  5
#define S1_Floor_LowerToLowestFloor                    23
#define DR_Door_Red_OpenWaitClose                      28
#define D1_Door_OpenStay                               31
#define W1_Floor_LowerToLowestFloor_ChangesTexture     37
#define SR_Floor_LowerTo8AboveHighestFloor             70
#define W1_Floor_RaiseToNextHighestFloor               119
#define W1_Teleport_MonstersOnly                       125
#define W1_Floor_RaiseToNextHighestFloor_Fast          130
*/
#define ML_DRAWASWALL                                  1024
#define ML_TRIGGER666                                  2048

// [BH] Line won't be shown as teleporter in automap.
#define ML_TELEPORTTRIGGERED                           4096

//#define NoSpecial                                      0

typedef struct
{
    int         mission;
    int         epsiode;
    int         map;
    int         vertex;
    int         oldx;
    int         oldy;
    int         newx;
    int         newy;
} vertexfix_t;

extern vertexfix_t vertexfix[];

typedef struct
{
    int         mission;
    int         epsiode;
    int         map;
    int         linedef;
    int         side;
    char        *toptexture;
    char        *middletexture;
    char        *bottomtexture;
    short       offset;
    short       rowoffset;
    int         flags;
    int         special;
    int         tag;
} linefix_t;

extern linefix_t linefix[];

typedef struct
{
    int         mission;
    int         epsiode;
    int         map;
    int         sector;
    char        *floorpic;
    char        *ceilingpic;
    int         floorheight;
    int         ceilingheight;
    int         special;
    int         tag;
} sectorfix_t;

extern sectorfix_t sectorfix[];

typedef struct
{
    int         mission;
    int         epsiode;
    int         map;
    int         thing;
    int         type;
    int         oldx;
    int         oldy;
    int         newx;
    int         newy;
    int         angle;
    int         options;
} thingfix_t;

extern thingfix_t thingfix[];

#endif
