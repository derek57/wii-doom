// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
//
// Copyright(C) 2015 by Brad Harding: - (Liquid Sector Animations)
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
//        Implements special effects:
//        Texture animation, height or lighting changes
//         according to adjacent sectors, respective
//         utility functions, etc.
//        Line Tag handling. Line and Sector triggers.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>

#ifdef WII
#include "../c_io.h"
#else
#include "c_io.h"
#endif

#include "d_englsh.h"

#ifdef WII
#include "../deh_main.h"
#else
#include "deh_main.h"
#endif

#include "doomdef.h"

#ifdef WII
#include "../doomfeatures.h"
#else
#include "doomfeatures.h"
#endif

#include "doomstat.h"
#include "g_game.h"

#ifdef WII
#include "../i_system.h"
#include "../m_argv.h"
#include "../m_misc.h"
#else
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"
#endif

#include "m_random.h"
#include "p_local.h"
#include "r_local.h"

// State.
#include "r_state.h"

#include "s_sound.h"

// Data.
#include "sounds.h"

#ifdef WII
#include "../v_trans.h"
#include "../w_wad.h"
#include "../z_zone.h"
#else
#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"
#endif


#define DONUT_FLOORHEIGHT_DEFAULT    0x00000000
#define DONUT_FLOORPIC_DEFAULT       0x16

#define MAXANIMS                     32
#define MAXLINEANIMS                 64 * 256    // CHANGED FOR HIRES
#define MAX_ADJOINING_SECTORS        20
#define ANIMSPEED                    8


//
// Animating textures and planes
// There is another anim_t used in wi_stuff, unrelated.
//
typedef struct
{
    boolean     istexture;
    int         picnum;
    int         basepic;
    int         numpics;
    int         speed;
    
} anim_t;

//
//      source animation definition
//
typedef struct
{
    int         istexture;        // if false, it is a flat
    char        endname[9];
    char        startname[9];
    int         speed;
} animdef_t;

static struct
{
    char        *pwad;
    char        *texture;
} exception[] = {
    { "BTSX_E1.WAD",  "SHNPRT02" },
    { "BTSX_E2B.WAD", "SHNPRT08" },
    { "BTSX_E2B.WAD", "SLIME09"  },
    { "DOOM2.WAD",    "RROCK05"  },
    { "DOOM2.WAD",    "RROCK06"  },
    { "DOOM2.WAD",    "RROCK07"  },
    { "DOOM2.WAD",    "RROCK08"  },
    { "DOOM2.WAD",    "SLIME09"  },
    { "DOOM2.WAD",    "SLIME10"  },
    { "DOOM2.WAD",    "SLIME11"  },
    { "DOOM2.WAD",    "SLIME12"  },
    { "MOHU2.WAD",    "DIFL_01"  },
    { "PLUTONIA.WAD", "RROCK05"  },
    { "PLUTONIA.WAD", "RROCK06"  },
    { "PLUTONIA.WAD", "RROCK07"  },
    { "PLUTONIA.WAD", "RROCK08"  },
    { "PLUTONIA.WAD", "SLIME09"  },
    { "PLUTONIA.WAD", "SLIME10"  },
    { "PLUTONIA.WAD", "SLIME11"  },
    { "PLUTONIA.WAD", "SLIME12"  },
    { "RC-DC.WAD",    "BWORM00A" },
    { "RC-DC.WAD",    "CFAN00A"  },
    { "RC-DC.WAD",    "CFAN01A"  },
    { "RC-DC.WAD",    "CFAN00D"  },
    { "RC-DC.WAD",    "CFAN01D"  },
    { "REQUIEM.WAD",  "SLIME05"  },
    { "REQUIEM.WAD",  "SLIME08"  },
    { "SID.WAD",      "FWATER1"  },
    { "TNT.WAD",      "RROCK05"  },
    { "TNT.WAD",      "RROCK06"  },
    { "TNT.WAD",      "RROCK07"  },
    { "TNT.WAD",      "RROCK08"  },
    { "TNT.WAD",      "SLIME09"  },
    { "TNT.WAD",      "SLIME10"  },
    { "TNT.WAD",      "SLIME11"  },
    { "TNT.WAD",      "SLIME12"  },
    { "UACULTRA.WAD", "RROCK05"  },
    { "VALIANT.WAD",  "E3SAW_A1" },
    { "VALIANT.WAD",  "E3SAW_A2" },
    { "VALIANT.WAD",  "E3SAW_A3" },
    { "VALIANT.WAD",  "E3SAW_A4" },
    { "",             ""         }
};


fixed_t animatedliquiddiff;

fixed_t animatedliquiddiffs[64] =
{
     6422,  6422,  6360,  6238,  6054,  5814,  5516,  5164,
     4764,  4318,  3830,  3306,  2748,  2166,  1562,   942,
      314,  -314,  -942, -1562, -2166, -2748, -3306, -3830,
    -4318, -4764, -5164, -5516, -5814, -6054, -6238, -6360,
    -6422, -6422, -6360, -6238, -6054, -5814, -5516, -5164,
    -4764, -4318, -3830, -3306, -2748, -2166, -1562,  -942,
     -314,   314,   942,  1562,  2166,  2748,  3306,  3830,
     4318,  4764,  5164,  5516,  5814,  6054,  6238,  6360
};

static anim_t   *anims;         // new structure w/o limits -- killough
static size_t   maxanims;

short           numlinespecials;

line_t*         linespeciallist[MAXLINEANIMS];

anim_t*         lastanim;

boolean         in_slime;
boolean         levelTimer;
boolean         *isliquid;

int             levelTimeCount;

extern boolean  noclip_on;

extern int      snd_module;


//
// P_InitPicAnims
//

// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to
//  be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in
//  the WAD file.
//
animdef_t                animdefs[] =
{
    {false,       "NUKAGE3",      "NUKAGE1",      ANIMSPEED},
    {false,       "FWATER4",      "FWATER1",      ANIMSPEED},
    {false,       "SWATER4",      "SWATER1",      ANIMSPEED},
    {false,       "LAVA4",        "LAVA1",        ANIMSPEED},
    {false,       "BLOOD3",       "BLOOD1",       ANIMSPEED},

    // DOOM II flat animations.
    {false,       "SLIME04",      "SLIME01",      ANIMSPEED},
    {false,       "SLIME08",      "SLIME05",      ANIMSPEED},
    {false,       "SLIME12",      "SLIME09",      ANIMSPEED},
    {false,       "RROCK08",      "RROCK05",      ANIMSPEED},                

    {true,        "BLODGR4",      "BLODGR1",      ANIMSPEED},
    {true,        "SLADRIP3",     "SLADRIP1",     ANIMSPEED},

    {true,        "BLODRIP4",     "BLODRIP1",     ANIMSPEED},
    {true,        "FIREWALL",     "FIREWALA",     ANIMSPEED},
    {true,        "GSTFONT3",     "GSTFONT1",     ANIMSPEED},
    {true,        "FIRELAVA",     "FIRELAV3",     ANIMSPEED},
    {true,        "FIREMAG3",     "FIREMAG1",     ANIMSPEED},
    {true,        "FIREBLU2",     "FIREBLU1",     ANIMSPEED},
    {true,        "ROCKRED3",     "ROCKRED1",     ANIMSPEED},

    {true,        "BFALL4",       "BFALL1",       ANIMSPEED},
    {true,        "SFALL4",       "SFALL1",       ANIMSPEED},
    {true,        "WFALL4",       "WFALL1",       ANIMSPEED},
    {true,        "DBRAIN4",      "DBRAIN1",      ANIMSPEED},
        
    {-1,          "",             "",                     0}
};

/*
int *TerrainTypes;
struct
{
    char *name;
    int type;
} TerrainTypeDefs[] =
{
    { "NUKAGE1", FLOOR_SLUDGE },
    { "FWATER1", FLOOR_WATER },
    { "SWATER1", FLOOR_WATER },
    { "LAVA1", FLOOR_LAVA },
    { "BLOOD1", FLOOR_LAVA },
    { "SLIME01", FLOOR_SLUDGE },
    { "SLIME05", FLOOR_SLUDGE },
    { "SLIME09", FLOOR_SLUDGE },
    { "END", -1 }
};
*/

//
//      Animating line specials
//
void P_InitPicAnims (void)
{
    int  i;
    int  size = (numflats + 1) * sizeof(boolean);

    isliquid = Z_Malloc(size, PU_STATIC, 0);
    memset(isliquid, false, size);
    
    //  Init animation
    lastanim = anims;
    for (i = 0; animdefs[i].endname[0]; i++)
    {
        char    *startname = animdefs[i].startname;
        char    *endname = animdefs[i].endname;

        // 1/11/98 killough -- removed limit by array-doubling
        if (lastanim >= anims + maxanims)
        {
            size_t      newmax = (maxanims ? maxanims * 2 : MAXANIMS);
#ifdef BOOM_ZONE_HANDLING
            anims = Z_Realloc(anims, newmax * sizeof(*anims), PU_LEVEL, NULL);
#else
            anims = Z_Realloc(anims, newmax * sizeof(*anims));
#endif
            lastanim = anims + maxanims;
            maxanims = newmax;
        }

        if (animdefs[i].istexture)
        {
            // different episode?
            if (R_CheckTextureNumForName(startname) == -1)
                continue;

            lastanim->picnum = R_TextureNumForName(endname);
            lastanim->basepic = R_TextureNumForName(startname);

            lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;
        }
        else
        {
            int j;

            if (W_CheckNumForName(startname) == -1)
                continue;

            lastanim->picnum = R_FlatNumForName(endname);
            lastanim->basepic = R_FlatNumForName(startname);

            lastanim->numpics = lastanim->picnum - lastanim->basepic + 1;

            for (j = 0; j < lastanim->numpics; j++)
                isliquid[lastanim->basepic + j] = true;
        }

        lastanim->istexture = animdefs[i].istexture;

        lastanim->speed = animdefs[i].speed;
        lastanim++;
    }

    i = 0;

    while (exception[i].pwad[0])
    {
        int lump = R_CheckFlatNumForName(exception[i].texture);
 
        if (lump >= 0 && !strcasecmp(M_ExtractFilename(lumpinfo[firstflat + lump]->wad_file->path),
                exception[i].pwad))
            isliquid[lump] = false;

        ++i;
    }
}



//
// UTILITIES
//



//
// getSide()
// Will return a side_t*
//  given the number of the current sector,
//  the line number, and the side (0/1) that you want.
//
side_t*
getSide
( int                currentSector,
  int                line,
  int                side )
{
    return &sides[ (sectors[currentSector].lines[line])->sidenum[side] ];
}


//
// getSector()
// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.
//
sector_t*
getSector
( int                currentSector,
  int                line,
  int                side )
{
    return sides[ (sectors[currentSector].lines[line])->sidenum[side] ].sector;
}


//
// twoSided()
// Given the sector number and the line number,
//  it will tell you whether the line is two-sided or not.
//
int
twoSided
( int        sector,
  int        line )
{
    //jff 1/26/98 return what is actually needed, whether the line
    //has two sidedefs, rather than whether the 2S flag is set

    return d_model ?
        (sectors[sector].lines[line])->flags & ML_TWOSIDED :
        (sectors[sector].lines[line])->sidenum[1] != NO_INDEX;
}




//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
sector_t*
getNextSector
( line_t*        line,
  sector_t*      sec )
{
    //jff 1/26/98 check unneeded since line->backsector already
    //returns NULL if the line is not two sided, and does so from
    //the actual two-sidedness of the line, rather than its 2S flag

    if (d_model)
    {
        if (!(line->flags & ML_TWOSIDED))
            return NULL;
    }
                
    if (line->frontsector == sec)
    {
        if (d_model || line->backsector != sec)
            return line->backsector; //jff 5/3/98 don't retn sec unless compatibility
        else                         // fixes an intra-sector line breaking functions
            return NULL;             // like floor->highest floor
    }

    return line->frontsector;
}



//
// P_FindLowestFloorSurrounding()
// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t        P_FindLowestFloorSurrounding(sector_t* sec)
{
    int                    i;
    line_t*                check;
    sector_t*              other;
    fixed_t                floor = sec->floor_height;
        
    for (i=0 ;i < sec->linecount ; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;
        
        if (other->floor_height < floor)
            floor = other->floor_height;
    }
    return floor;
}



//
// P_FindHighestFloorSurrounding()
// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t        P_FindHighestFloorSurrounding(sector_t *sec)
{
    int                    i;
    line_t*                check;
    sector_t*              other;
    fixed_t                floor = -500*FRACUNIT;
        
    //jff 1/26/98 Fix initial value for floor to not act differently
    //in sections of wad that are below -500 units
    if (!d_model)                  /* jff 3/12/98 avoid ovf */
        floor = -32000 * FRACUNIT; // in height calculations

    for (i=0 ;i < sec->linecount ; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);
        
        if (!other)
            continue;
        
        if (other->floor_height > floor)
            floor = other->floor_height;
    }
    return floor;
}



//
// P_FindNextHighestFloor
// FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS
// Note: this should be doable w/o a fixed array.

// Thanks to entryway for the Vanilla overflow emulation.

// 20 adjoining sectors max!
fixed_t
P_FindNextHighestFloor
( sector_t* sec,
  int       currentheight )
{
    int             i;
    int             h;
    int             min;
    line_t*         check;
    sector_t*       other;
    fixed_t         height = currentheight;
    static fixed_t* heightlist = NULL;
    static int      heightlist_size = 0;

    // [crispy] remove MAX_ADJOINING_SECTORS Vanilla limit
    // from prboom-plus/src/p_spec.c:404-411
    if (sec->linecount > heightlist_size)
    {
        do
        {
            heightlist_size = heightlist_size ? 2 * heightlist_size : MAX_ADJOINING_SECTORS;
        } while (sec->linecount > heightlist_size);
#ifdef BOOM_ZONE_HANDLING
        heightlist = Z_Realloc(heightlist, heightlist_size * sizeof(*heightlist), PU_LEVEL, NULL);
#else
        heightlist = Z_Realloc(heightlist, heightlist_size * sizeof(*heightlist));
#endif
    }

    for (i=0, h=0; i < sec->linecount; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;
        
        if (other->floor_height > height)
        {
            C_Printf(CR_GOLD, " P_FindNextHighestFloor: Overflow of heightlist[%d] array is detected.\n", MAX_ADJOINING_SECTORS);

            // Emulation of memory (stack) overflow
            if (h == MAX_ADJOINING_SECTORS + 1)
                height = other->floor_height;
            else if (h <= MAX_ADJOINING_SECTORS + 1)
                C_Printf(CR_GOLD, " Heightlist index %d: successfully emulated.\n", h);
            else if (h == MAX_ADJOINING_SECTORS + 2)
                // Fatal overflow: game crashes at 22 textures
                C_Printf(CR_GOLD, " P_FindNextHighestFloor: Sector with more than 22 adjoining sectors. Vanilla will crash here");
            else if (h <= MAX_ADJOINING_SECTORS + 6)
                C_Printf(CR_RED, " Heightlist index %d: cannot be emulated - unpredictable behaviour.\n", h);
            else
                C_Printf(CR_RED, " Heightlist index %d: cannot be emulated - crash with high probability.\n", h);

            heightlist[h++] = other->floor_height;
        }
    }
    
    // Find lowest height in list
    if (!h)
    {
        return currentheight;
    }
        
    min = heightlist[0];
    
    // Range checking? 
    for (i = 1; i < h; i++)
    {
        if (heightlist[i] < min)
        {
            min = heightlist[i];
        }
    }

    return min;
}

//
// FIND LOWEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t
P_FindLowestCeilingSurrounding(sector_t* sec)
{
    int                    i;
    line_t*                check;
    sector_t*              other;
    fixed_t                height = INT_MAX;
        
    /* jff 3/12/98 avoid ovf in height calculations */
    if (!d_model)
        height = 32000 * FRACUNIT;

    for (i=0 ;i < sec->linecount ; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        if (other->ceiling_height < height)
            height = other->ceiling_height;
    }
    return height;
}


//
// FIND HIGHEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t        P_FindHighestCeilingSurrounding(sector_t* sec)
{
    int            i;
    line_t*        check;
    sector_t*      other;
    fixed_t        height = 0;
        
    /* jff 1/26/98 Fix initial value for floor to not act differently
     * in sections of wad that are below 0 units
     * jff 3/12/98 avoid ovf in height calculations */
    if (!d_model)
        height = -32000 * FRACUNIT;

    for (i=0 ;i < sec->linecount ; i++)
    {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if (!other)
            continue;

        if (other->ceiling_height > height)
            height = other->ceiling_height;
    }
    return height;
}



// Find the next sector with the same tag as a linedef.
// Rewritten by Lee Killough to use chained hashing to improve speed
int P_FindSectorFromLineTag(const line_t *line, int start)
{
    start = (start >= 0 ? sectors[start].nexttag :
        sectors[(unsigned int)line->tag % (unsigned int)numsectors].firsttag);
    while (start >= 0 && sectors[start].tag != line->tag)
        start = sectors[start].nexttag;
    return start;
}


// Hash the sector tags across the sectors and linedefs.
static void P_InitTagLists(void)
{
    int i;

    for (i = numsectors; --i >= 0;)     // Initially make all slots empty.
        sectors[i].firsttag = -1;
    for (i = numsectors; --i >= 0;)     // Proceed from last to first sector
    {                                   // so that lower sectors appear first
        int     j = (unsigned int)sectors[i].tag % (unsigned int)numsectors;    // Hash func

        sectors[i].nexttag = sectors[j].firsttag;     // Prepend sector to chain
        sectors[j].firsttag = i;
    }

    // killough 4/17/98: same thing, only for linedefs
    for (i = numlines; --i >= 0;)       // Initially make all slots empty.
        lines[i].firsttag = -1;
    for (i = numlines; --i >= 0;)       // Proceed from last to first linedef
    {                                   // so that lower linedefs appear first
        int     j = (unsigned int)lines[i].tag % (unsigned int)numlines;        // Hash func

        lines[i].nexttag = lines[j].firsttag;   // Prepend linedef to chain
        lines[j].firsttag = i;
    }
}

//
// Find minimum light from an adjacent sector
//
int
P_FindMinSurroundingLight
( sector_t*        sector,
  int              max )
{
    int            i;
    int            min;
    line_t*        line;
    sector_t*      check;
        
    min = max;
    for (i=0 ; i < sector->linecount ; i++)
    {
        line = sector->lines[i];
        check = getNextSector(line,sector);

        if (!check)
            continue;

        if (check->lightlevel < min)
            min = check->lightlevel;
    }
    return min;
}



//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//

//
// P_CrossSpecialLine - TRIGGER
// Called every time a thing origin is about
//  to cross a line with a non 0 special.
//
void P_CrossSpecialLine(line_t *line, int side, mobj_t *thing)
{
    int              ok;

    //        Triggers that other things can activate
    if (!thing->player)
    {
        // Things that should NOT trigger specials...
        switch(thing->type)
        {
          case MT_ROCKET:
          case MT_PLASMA:
          case MT_BFG:
          case MT_TROOPSHOT:
          case MT_HEADSHOT:
          case MT_BRUISERSHOT:
          case MT_PLASMA1:    // killough 8/28/98: exclude beta fireballs
          case MT_PLASMA2:
            return;
            break;
            
          default: break;
        }
                
        ok = 0;
        switch(line->special)
        {
          case 39:        // TELEPORT TRIGGER
          case 97:        // TELEPORT RETRIGGER
          case 125:        // TELEPORT MONSTERONLY TRIGGER
          case 126:        // TELEPORT MONSTERONLY RETRIGGER
          case 4:        // RAISE DOOR
          case 10:        // PLAT DOWN-WAIT-UP-STAY TRIGGER
          case 88:        // PLAT DOWN-WAIT-UP-STAY RETRIGGER
            ok = 1;
            break;
        }
        if (!ok)
            return;
    }

    
    // Note: could use some const's here.
    switch (line->special)
    {
        // TRIGGERS.
        // All from here to RETRIGGERS.
      case 2:
        // Open Door
        EV_DoDoor(line,open);
        line->special = 0;
        break;

      case 3:
        // Close Door
        EV_DoDoor(line,closed);
        line->special = 0;
        break;

      case 4:
        // Raise Door
        EV_DoDoor(line,normal);
        line->special = 0;
        break;
        
      case 5:
        // Raise Floor
        EV_DoFloor(line,raiseFloor);
        line->special = 0;
        break;
        
      case 6:
        // Fast Ceiling Crush & Raise
        EV_DoCeiling(line,fastCrushAndRaise);
        line->special = 0;
        break;
        
      case 8:
        // Build Stairs
        EV_BuildStairs(line,build8);
        line->special = 0;
        break;
        
      case 10:
        // PlatDownWaitUp
        EV_DoPlat(line,downWaitUpStay,0);
        line->special = 0;
        break;
        
      case 12:
        // Light Turn On - brightest near
        EV_LightTurnOn(line,0);
        line->special = 0;
        break;
        
      case 13:
        // Light Turn On 255
        EV_LightTurnOn(line,255);
        line->special = 0;
        break;
        
      case 16:
        // Close Door 30
        EV_DoDoor(line,close30ThenOpen);
        line->special = 0;
        break;
        
      case 17:
        // Start Light Strobing
        EV_StartLightStrobing(line);
        line->special = 0;
        break;
        
      case 19:
        // Lower Floor
        EV_DoFloor(line,lowerFloor);
        line->special = 0;
        break;
        
      case 22:
        // Raise floor to nearest height and change texture
        EV_DoPlat(line,raiseToNearestAndChange,0);
        line->special = 0;
        break;
        
      case 25:
        // Ceiling Crush and Raise
        EV_DoCeiling(line,crushAndRaise);
        line->special = 0;
        break;
        
      case 30:
        // Raise floor to shortest texture height
        //  on either side of lines.
        EV_DoFloor(line,raiseToTexture);
        line->special = 0;
        break;
        
      case 35:
        // Lights Very Dark
        EV_LightTurnOn(line,35);
        line->special = 0;
        break;
        
      case 36:
        // Lower Floor (TURBO)
        EV_DoFloor(line,turboLower);
        line->special = 0;
        break;
        
      case 37:
        // LowerAndChange
        EV_DoFloor(line,lowerAndChange);
        line->special = 0;
        break;
        
      case 38:
        // Lower Floor To Lowest
        EV_DoFloor( line, lowerFloorToLowest );
        line->special = 0;
        break;
        
      case 39:
        // TELEPORT!
        EV_Teleport( line, side, thing );
        line->special = 0;
        break;

      case 40:
        // RaiseCeilingLowerFloor
        EV_DoCeiling( line, raiseToHighest );
        EV_DoFloor( line, lowerFloorToLowest );
        line->special = 0;
        break;
        
      case 44:
        // Ceiling Crush
        EV_DoCeiling( line, lowerAndCrush );
        line->special = 0;
        break;
        
      case 52:
        // EXIT!
        G_ExitLevel ();
        break;
        
      case 53:
        // Perpetual Platform Raise
        EV_DoPlat(line,perpetualRaise,0);
        line->special = 0;
        break;
        
      case 54:
        // Platform Stop
        EV_StopPlat(line);
        line->special = 0;
        break;

      case 56:
        // Raise Floor Crush
        EV_DoFloor(line,raiseFloorCrush);
        line->special = 0;
        break;

      case 57:
        // Ceiling Crush Stop
        EV_CeilingCrushStop(line);
        line->special = 0;
        break;
        
      case 58:
        // Raise Floor 24
        EV_DoFloor(line,raiseFloor24);
        line->special = 0;
        break;

      case 59:
        // Raise Floor 24 And Change
        EV_DoFloor(line,raiseFloor24AndChange);
        line->special = 0;
        break;
        
      case 104:
        // Turn lights off in sector(tag)
        EV_TurnTagLightsOff(line);
        line->special = 0;
        break;
        
      case 108:
        // Blazing Door Raise (faster than TURBO!)
        EV_DoDoor (line,blazeRaise);
        line->special = 0;
        break;
        
      case 109:
        // Blazing Door Open (faster than TURBO!)
        EV_DoDoor (line,blazeOpen);
        line->special = 0;
        break;
        
      case 100:
        // Build Stairs Turbo 16
        EV_BuildStairs(line,turbo16);
        line->special = 0;
        break;
        
      case 110:
        // Blazing Door Close (faster than TURBO!)
        EV_DoDoor (line,blazeClose);
        line->special = 0;
        break;

      case 119:
        // Raise floor to nearest surr. floor
        EV_DoFloor(line,raiseFloorToNearest);
        line->special = 0;
        break;
        
      case 121:
        // Blazing PlatDownWaitUpStay
        EV_DoPlat(line,blazeDWUS,0);
        line->special = 0;
        break;
        
      case 124:
        // Secret EXIT
        G_SecretExitLevel ();
        break;
                
      case 125:
        // TELEPORT MonsterONLY
        if (!thing->player)
        {
            EV_Teleport( line, side, thing );
            line->special = 0;
        }
        break;
        
      case 130:
        // Raise Floor Turbo
        EV_DoFloor(line,raiseFloorTurbo);
        line->special = 0;
        break;
        
      case 141:
        // Silent Ceiling Crush & Raise
        EV_DoCeiling(line,silentCrushAndRaise);
        line->special = 0;
        break;
        
        // RETRIGGERS.  All from here till end.
      case 72:
        // Ceiling Crush
        EV_DoCeiling( line, lowerAndCrush );
        break;

      case 73:
        // Ceiling Crush and Raise
        EV_DoCeiling(line,crushAndRaise);
        break;

      case 74:
        // Ceiling Crush Stop
        EV_CeilingCrushStop(line);
        break;
        
      case 75:
        // Close Door
        EV_DoDoor(line,closed);
        break;
        
      case 76:
        // Close Door 30
        EV_DoDoor(line,close30ThenOpen);
        break;
        
      case 77:
        // Fast Ceiling Crush & Raise
        if(beta_style)
            EV_DoCeiling(line,crushAndRaise);
        else
            EV_DoCeiling(line,fastCrushAndRaise);
        break;
        
      case 79:
        // Lights Very Dark
        EV_LightTurnOn(line,35);
        break;
        
      case 80:
        // Light Turn On - brightest near
        EV_LightTurnOn(line,0);
        break;
        
      case 81:
        // Light Turn On 255
        EV_LightTurnOn(line,255);
        break;
        
      case 82:
        // Lower Floor To Lowest
        EV_DoFloor( line, lowerFloorToLowest );
        break;
        
      case 83:
        // Lower Floor
        EV_DoFloor(line,lowerFloor);
        break;

      case 84:
        // LowerAndChange
        EV_DoFloor(line,lowerAndChange);
        break;

      case 86:
        // Open Door
        EV_DoDoor(line,open);
        break;
        
      case 87:
        // Perpetual Platform Raise
        EV_DoPlat(line,perpetualRaise,0);
        break;
        
      case 88:
        // PlatDownWaitUp
        EV_DoPlat(line,downWaitUpStay,0);
        break;
        
      case 89:
        // Platform Stop
        EV_StopPlat(line);
        break;
        
      case 90:
        // Raise Door
        EV_DoDoor(line,normal);
        break;
        
      case 91:
        // Raise Floor
        EV_DoFloor(line,raiseFloor);
        break;
        
      case 92:
        // Raise Floor 24
        EV_DoFloor(line,raiseFloor24);
        break;
        
      case 93:
        // Raise Floor 24 And Change
        EV_DoFloor(line,raiseFloor24AndChange);
        break;
        
      case 94:
        // Raise Floor Crush
        EV_DoFloor(line,raiseFloorCrush);
        break;
        
      case 95:
        // Raise floor to nearest height
        // and change texture.
        EV_DoPlat(line,raiseToNearestAndChange,0);
        break;
        
      case 96:
        // Raise floor to shortest texture height
        // on either side of lines.
        EV_DoFloor(line,raiseToTexture);
        break;
        
      case 97:
        // TELEPORT!
        EV_Teleport( line, side, thing );
        break;
        
      case 98:
        // Lower Floor (TURBO)
        EV_DoFloor(line,turboLower);
        break;

      case 105:
        // Blazing Door Raise (faster than TURBO!)
        EV_DoDoor (line,blazeRaise);
        break;
        
      case 106:
        // Blazing Door Open (faster than TURBO!)
        EV_DoDoor (line,blazeOpen);
        break;

      case 107:
        // Blazing Door Close (faster than TURBO!)
        EV_DoDoor (line,blazeClose);
        break;

      case 120:
        // Blazing PlatDownWaitUpStay.
        EV_DoPlat(line,blazeDWUS,0);
        break;
        
      case 126:
        // TELEPORT MonsterONLY.
        if (!thing->player)
            EV_Teleport( line, side, thing );
        break;
        
      case 128:
        // Raise To Nearest Floor
        EV_DoFloor(line,raiseFloorToNearest);
        break;
        
      case 129:
        // Raise Floor Turbo
        EV_DoFloor(line,raiseFloorTurbo);
        break;
    }
}



//
// P_ShootSpecialLine - IMPACT SPECIALS
// Called when a thing shoots a special line.
//
void
P_ShootSpecialLine
( mobj_t*        thing,
  line_t*        line )
{
    int          ok;
    
    //        Impacts that other things can activate.
    if (!thing->player)
    {
        ok = 0;
        switch(line->special)
        {
          case 46:
            // OPEN DOOR IMPACT
            ok = 1;
            break;
        }
        if (!ok)
            return;
    }

    switch(line->special)
    {
      case 24:
        // RAISE FLOOR
        EV_DoFloor(line,raiseFloor);
        P_ChangeSwitchTexture(line,0);
        break;
        
      case 46:
        // OPEN DOOR
        EV_DoDoor(line,open);
        P_ChangeSwitchTexture(line,1);
        break;
        
      case 47:
        // RAISE FLOOR NEAR AND CHANGE
        EV_DoPlat(line,raiseToNearestAndChange,0);
        P_ChangeSwitchTexture(line,0);
        break;
    }
}



//
// P_PlayerInSpecialSector
// Called every tic frame
//  that the player origin is in a special sector
//
void P_PlayerInSpecialSector (player_t* player)
{
    extern int       showMessages;

    sector_t*        sector = player->mo->subsector->sector;

    // Falling, not all the way down yet?
    if (player->mo->z != sector->floor_height)
        return;        

    // Has hitten ground.
    switch (sector->special)
    {
      case 5:
        // HELLSLIME DAMAGE
        if (!player->powers[pw_ironfeet])
            if (!(leveltime&0x1f))
            {
                in_slime = true;
                P_DamageMobj (player->mo, NULL, NULL, 10);
            }
        break;
        
      case 7:
        // NUKAGE DAMAGE
        if (!player->powers[pw_ironfeet])
            if (!(leveltime&0x1f))
            {
                in_slime = true;
                P_DamageMobj (player->mo, NULL, NULL, 5);
            }
        break;
        
      case 16:
        // SUPER HELLSLIME DAMAGE
      case 4:
        // STROBE HURT
        if (!player->powers[pw_ironfeet]
            || (P_Random()<5) )
        {
            if (!(leveltime&0x1f))
            {
                in_slime = true;
                P_DamageMobj (player->mo, NULL, NULL, 20);
            }
        }
        break;
                        
      case 9:
        // SECRET SECTOR
	// [crispy] show centered "Secret Revealed!" message
        if (showMessages && d_secrets && !noclip_on)
        {
            player->message = HUSTR_SECRETFOUND;
            if (player == &players[consoleplayer] && !snd_module)
                S_StartSound(NULL, sfx_secret);
        }
        player->secretcount++;
        sector->special = 0;
        break;
                        
      case 11:
        // EXIT SUPER DAMAGE! (for E1M8 finale)
        if (d_god) /* killough 2/21/98: add compatibility switch */
            player->cheats &= ~CF_GODMODE; // on godmode cheat clearing
                                           // does not affect invulnerability
        if (!(leveltime&0x1f))
        {
            in_slime = true;
            P_DamageMobj (player->mo, NULL, NULL, 20);
        }

        if (player->health <= 10)
            G_ExitLevel();
        break;
                        
      default:
        break;
    };
}




//
// P_UpdateSpecials
// Animate planes, scroll walls, etc.
//
void P_UpdateSpecials (void)
{
    anim_t*        anim;
    int            pic;
    int            i;
    line_t*        line;
    mobj_t*        so;

    //        LEVEL TIMER
    if (levelTimer == true)
    {
        levelTimeCount--;
        if (!levelTimeCount)
            G_ExitLevel();
    }
    
    //        ANIMATE FLATS AND TEXTURES GLOBALLY
    for (anim = anims ; anim < lastanim ; anim++)
    {
        for (i=anim->basepic ; i<anim->basepic+anim->numpics ; i++)
        {
            pic = anim->basepic + ( (leveltime/anim->speed + i)%anim->numpics );
            if (anim->istexture)
                texturetranslation[i] = pic;
            else
            {
//                flattranslation[i] = pic;
                  flattranslation[i] = d_swirl ? -1 : pic;
                  // sf: > 65535 : swirly hack 
                  if(anim->speed > 65535 || anim->numpics==1)
                      flattranslation[i] = -1;
            }
        }
    }

    
    //        ANIMATE LINE SPECIALS
    for (i = 0; i < numlinespecials; i++)
    {
        line = linespeciallist[i];
        switch(line->special)
        {
          case 48:
            // EFFECT FIRSTCOL SCROLL +
            sides[line->sidenum[0]].textureoffset += FRACUNIT;
            break;
        }
    }

    animatedliquiddiff += animatedliquiddiffs[leveltime & 63];
    
    //        DO BUTTONS
    for (i = 0; i < MAXBUTTONS; i++)
        if (buttonlist[i].btimer)
        {
            buttonlist[i].btimer--;
            if (!buttonlist[i].btimer)
            {
                switch(buttonlist[i].where)
                {
                  case top:
                    sides[buttonlist[i].line->sidenum[0]].toptexture =
                        buttonlist[i].btexture;
                    break;
                    
                  case middle:
                    sides[buttonlist[i].line->sidenum[0]].midtexture =
                        buttonlist[i].btexture;
                    break;
                    
                  case bottom:
                    sides[buttonlist[i].line->sidenum[0]].bottomtexture =
                        buttonlist[i].btexture;
                    break;
                }
                /* don't take the address of the switch's sound origin,
                 * unless in a compatibility mode. */
                so = (mobj_t *)buttonlist[i].soundorg;

                if (d_sound)
                    /* since the buttonlist array is usually zeroed out,
                     * button popouts generally appear to come from (0,0) */
                    so = (mobj_t *)&buttonlist[i].soundorg;

                S_StartSound(so, sfx_swtchn);
                memset(&buttonlist[i],0,sizeof(button_t));
            }
        }
}


//
// Donut overrun emulation
//
// Derived from the code from PrBoom+.  Thanks go to Andrey Budko (entryway)
// as usual :-)
//
static void DonutOverrun(fixed_t *s3_floorheight, short *s3_floorpic,
                         line_t *line, sector_t *pillar_sector)
{
    static int first = 1;
    static int tmp_s3_floorheight;
    static int tmp_s3_floorpic;

    if (first)
    {
#ifndef WII
        int p;
#endif
        // This is the first time we have had an overrun.
        first = 0;

        // Default values
        tmp_s3_floorheight = DONUT_FLOORHEIGHT_DEFAULT;
        tmp_s3_floorpic = DONUT_FLOORPIC_DEFAULT;

        //!
        // @category compat
        // @arg <x> <y>
        //
        // Use the specified magic values when emulating behavior caused
        // by memory overruns from improperly constructed donuts.
        // In Vanilla Doom this can differ depending on the operating
        // system.  The default (if this option is not specified) is to
        // emulate the behavior when running under Windows 98.
#ifndef WII
        p = M_CheckParmWithArgs("-donut", 2);

        if (p > 0)
        {
            // Dump of needed memory: (fixed_t)0000:0000 and (short)0000:0008
            //
            // C:\>debug
            // -d 0:0
            //
            // DOS 6.22:
            // 0000:0000    (57 92 19 00) F4 06 70 00-(16 00)
            // DOS 7.1:
            // 0000:0000    (9E 0F C9 00) 65 04 70 00-(16 00)
            // Win98:
            // 0000:0000    (00 00 00 00) 65 04 70 00-(16 00)
            // DOSBox under XP:
            // 0000:0000    (00 00 00 F1) ?? ?? ?? 00-(07 00)

            M_StrToInt(myargv[p + 1], &tmp_s3_floorheight);
            M_StrToInt(myargv[p + 2], &tmp_s3_floorpic);

            if (tmp_s3_floorpic >= numflats)
            {
                fprintf(stderr,
                        "DonutOverrun: The second parameter for \"-donut\" "
                        "switch should be greater than 0 and less than number "
                        "of flats (%d). Using default value (%d) instead. \n",
                        numflats, DONUT_FLOORPIC_DEFAULT);
                tmp_s3_floorpic = DONUT_FLOORPIC_DEFAULT;
            }
        }
#endif
    }

    /*
    fprintf(stderr,
            "Linedef: %d; Sector: %d; "
            "New floor height: %d; New floor pic: %d\n",
            line->iLineID, pillar_sector->iSectorID,
            tmp_s3_floorheight >> 16, tmp_s3_floorpic);
    */

    *s3_floorheight = (fixed_t) tmp_s3_floorheight;
    *s3_floorpic = (short) tmp_s3_floorpic;
}


//
// Special Stuff that can not be categorized
//
int EV_DoDonut(line_t*       line)
{
    sector_t*                s1;
    sector_t*                s2;
    sector_t*                s3;
    int                      secnum;
    int                      rtn;
    int                      i;
    floormove_t*             floor;
    fixed_t                  s3_floorheight;
    short                    s3_floorpic;

    secnum = -1;
    rtn = 0;
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        s1 = &sectors[secnum];

        // ALREADY MOVING?  IF SO, KEEP GOING...
        if (s1->specialdata)
            continue;

        rtn = 1;
        s2 = getNextSector(s1->lines[0],s1);

        // Vanilla Doom does not check if the linedef is one sided.  The
        // game does not crash, but reads invalid memory and causes the
        // sector floor to move "down" to some unknown height.
        // DOSbox prints a warning about an invalid memory access.
        //
        // I'm not sure exactly what invalid memory is being read.  This
        // isn't something that should be done, anyway.
        // Just print a warning and return.

        if (s2 == NULL)
        {
            C_Printf(CR_RED, " EV_DoDonut: linedef had no second sidedef! "
                     " Unexpected behavior may occur in Vanilla Doom. \n");
            break;
        }

        for (i = 0; i < s2->linecount; i++)
        {
            s3 = s2->lines[i]->backsector;

            if (s3 == s1)
                continue;

            if (s3 == NULL)
            {
                // e6y
                // s3 is NULL, so
                // s3->floorheight is an int at 0000:0000
                // s3->floorpic is a short at 0000:0008
                // Trying to emulate

                C_Printf(CR_RED, " EV_DoDonut: WARNING: emulating buffer overrun due to "
                         " NULL back sector. "
                         " Unexpected behavior may occur in Vanilla Doom.\n");

                DonutOverrun(&s3_floorheight, &s3_floorpic, line, s1);
            }
            else
            {
                s3_floorheight = s3->floor_height;
                s3_floorpic = s3->floorpic;
            }

            //        Spawn rising slime
            floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
            P_AddThinker (&floor->thinker);
            s2->specialdata = floor;
            floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
            floor->type = donutRaise;
            floor->crush = false;
            floor->direction = 1;
            floor->sector = s2;
            floor->speed = FLOORSPEED / 2;
            floor->texture = s3_floorpic;
            floor->newspecial = 0;
            floor->floordestheight = s3_floorheight;
            
            //        Spawn lowering donut-hole
            floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
            P_AddThinker (&floor->thinker);
            s1->specialdata = floor;
            floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
            floor->type = lowerFloor;
            floor->crush = false;
            floor->direction = -1;
            floor->sector = s1;
            floor->speed = FLOORSPEED / 2;
            floor->floordestheight = s3_floorheight;
            break;
        }
    }
    return rtn;
}



//
// SPECIAL SPAWNING
//

//
// P_SpawnSpecials
// After the map has been loaded, scan for specials
//  that spawn thinkers
//
// Parses command line parameters.
void P_SpawnSpecials (void)
{
    sector_t*          sector;
    int                i;

    // See if -TIMER was specified.

    if (timelimit > 0 && deathmatch)
    {
        levelTimer = true;
        levelTimeCount = timelimit * 60 * TICRATE;
    }
    else
    {
        levelTimer = false;
    }

    //        Init special SECTORs.
    sector = sectors;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
        if (!sector->special)
            continue;
        
        switch (sector->special)
        {
          case 1:
            // FLICKERING LIGHTS
            P_SpawnLightFlash (sector);
            break;

          case 2:
            // STROBE FAST
            P_SpawnStrobeFlash(sector,FASTDARK,0);
            break;
            
          case 3:
            // STROBE SLOW
            P_SpawnStrobeFlash(sector,SLOWDARK,0);
            break;
            
          case 4:
            // STROBE FAST/DEATH SLIME
            P_SpawnStrobeFlash(sector,FASTDARK,0);
            sector->special = 4;
            break;
            
          case 8:
            // GLOWING LIGHT
            P_SpawnGlowingLight(sector);
            break;
          case 9:
            // SECRET SECTOR
            totalsecret++;
            break;
            
          case 10:
            // DOOR CLOSE IN 30 SECONDS
            P_SpawnDoorCloseIn30 (sector);
            break;
            
          case 12:
            // SYNC STROBE SLOW
            P_SpawnStrobeFlash (sector, SLOWDARK, 1);
            break;

          case 13:
            // SYNC STROBE FAST
            P_SpawnStrobeFlash (sector, FASTDARK, 1);
            break;

          case 14:
            // DOOR RAISE IN 5 MINUTES
            P_SpawnDoorRaiseIn5Mins (sector, i);
            break;
            
          case 17:
            P_SpawnFireFlicker(sector);
            break;
        }
    }

    
    //        Init line EFFECTs
    numlinespecials = 0;
    for (i = 0;i < numlines; i++)
    {
        switch(lines[i].special)
        {
          case 48:
            if (numlinespecials >= MAXLINEANIMS)
            {
                I_Error("Too many scrolling wall linedefs! "
                        "(Vanilla limit is 64)");
            }
            // EFFECT FIRSTCOL SCROLL+
            linespeciallist[numlinespecials] = &lines[i];
            numlinespecials++;
            break;
        }
    }

    
    P_RemoveAllActiveCeilings();
    P_RemoveAllActivePlats();
    
    for (i = 0;i < MAXBUTTONS;i++)
        memset(&buttonlist[i],0,sizeof(button_t));

    P_InitTagLists();

    for (i = 0; i < numlines; i++)
    {
        int sec;
        int s;

        switch (lines[i].special)
        {
            // killough 3/7/98:
            // support for drawn heights coming from different sector
            case 242:
                sec = sides[*lines[i].sidenum].sector - sectors;
                for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
                    sectors[s].heightsec = sec;
                break;

            // killough 3/16/98: Add support for setting
            // floor lighting independently (e.g. lava)
            case 213:
                sec = sides[*lines[i].sidenum].sector - sectors;
                for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
                    sectors[s].floorlightsec = sec;
                break;

            // killough 4/11/98: Add support for setting
            // ceiling lighting independently
            case 261:
                sec = sides[*lines[i].sidenum].sector - sectors;
                for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
                    sectors[s].ceilinglightsec = sec;
                break;

            // killough 10/98:
            //
            // Support for sky textures being transferred from sidedefs.
            // Allows scrolling and other effects (but if scrolling is
            // used, then the same sector tag needs to be used for the
            // sky sector, the sky-transfer linedef, and the scroll-effect
            // linedef). Still requires user to use F_SKY1 for the floor
            // or ceiling texture, to distinguish floor and ceiling sky.
            case 271:
            case 272:
                for (s = -1; (s = P_FindSectorFromLineTag(lines + i, s)) >= 0;)
                    sectors[s].sky = i | PL_SKYFLAT;
                break;
        }
    }

    // UNUSED: no horizonal sliders.
    //        P_InitSlidingDoorFrames();
}

// killough 4/16/98: Same thing, only for linedefs
int P_FindLineFromLineTag(const line_t *line, int start)
{
    start = (start >= 0 ? lines[start].nexttag :
        lines[(unsigned int)line->tag % (unsigned int)numlines].firsttag);
    while (start >= 0 && lines[start].tag != line->tag)
        start = lines[start].nexttag;
    return start;
}

//----------------------------------------------------------------------------
//
// PROC P_InitTerrainTypes
//
//----------------------------------------------------------------------------
/*
void P_InitTerrainTypes(void)
{
    int i;
    int lump;
    int size;

    size = (numflats + 1) * sizeof(int);
    TerrainTypes = Z_Malloc(size, PU_STATIC, 0);
    memset(TerrainTypes, 0, size);
    for (i = 0; TerrainTypeDefs[i].type != -1; i++)
    {
        lump = W_CheckNumForName(TerrainTypeDefs[i].name);
        if (lump != -1)
        {
            TerrainTypes[lump - firstflat] = TerrainTypeDefs[i].type;
        }
    }
}
*/

