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
//        Do all the WAD I/O, get map description,
//        set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------



#include <math.h>

#include "c_io.h"
#include "deh_main.h"
#include "doomdef.h"
#include "doomstat.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_bbox.h"
#include "p_local.h"
#include "s_sound.h"
#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"


// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS        10

void        P_SpawnMapThing (mapthing_t*        mthing);


//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int                numvertexes;
int                numsegs;
int                numsectors;
int                numsubsectors;
int                numnodes;
int                numlines;
int                numsides;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int                bmapwidth;
int                bmapheight;        // size in mapblocks

vertex_t*          vertexes;
seg_t*             segs;
sector_t*          sectors;
subsector_t*       subsectors;
node_t*            nodes;
line_t*            lines;
side_t*            sides;

// for thing chains
mobj_t**           blocklinks;                

mapthing_t         deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t*        deathmatch_p;
mapthing_t         playerstarts[MAXPLAYERS];


// origin of block map
fixed_t            bmaporgx;
fixed_t            bmaporgy;

static int         totallines;

// offsets in blockmap are from here
int64_t*           blockmaplump; // BLOCKMAP limit
int64_t*           blockmap;     // BLOCKMAP limit (int for larger maps)

boolean            createblockmap = false;

// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
byte*              rejectmatrix;


extern boolean     mus_cheat_used;
extern boolean     finale_music;

extern int         numsplats;

//
// P_LoadVertexes
//
void P_LoadVertexes (int lump)
{
    byte*          data;
    int            i;
    mapvertex_t*   ml;
    vertex_t*      li;

    // Determine number of lumps:
    //  total lump length / vertex record length.
    numvertexes = W_LumpLength (lump) / sizeof(mapvertex_t);

    // Allocate zone memory for buffer.
    vertexes = Z_Malloc (numvertexes*sizeof(vertex_t),PU_LEVEL,0);        

    // Load data into cache.
    data = W_CacheLumpNum (lump, PU_STATIC);
        
    ml = (mapvertex_t *)data;
    li = vertexes;

    // Copy and convert vertex coordinates,
    // internal representation as fixed.
    for (i=0 ; i<numvertexes ; i++, li++, ml++)
    {
        li->x = SHORT(ml->x)<<FRACBITS;
        li->y = SHORT(ml->y)<<FRACBITS;
    }

    // Free buffer memory.
    W_ReleaseLumpNum(lump);
}

//
// GetSectorAtNullAddress
//
sector_t* GetSectorAtNullAddress(void)
{
    static boolean null_sector_is_initialized = false;
    static sector_t null_sector;

    if (!null_sector_is_initialized)
    {
        memset(&null_sector, 0, sizeof(null_sector));
        I_GetMemoryValue(0, &null_sector.floor_height, 4);
        I_GetMemoryValue(4, &null_sector.ceiling_height, 4);
        null_sector_is_initialized = true;
    }

    return &null_sector;
}

//
// P_LoadSegs
//
void P_LoadSegs (int lump)
{
    byte*          data;
    int            i;
    mapseg_t*      ml;
    seg_t*         li;
    line_t*        ldef;
    int            linedef;
    int            side;
    int            sidenum;
        
    numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
    segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);        
    memset (segs, 0, numsegs*sizeof(seg_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
        
    ml = (mapseg_t *)data;
    li = segs;
    for (i=0 ; i<numsegs ; i++, li++, ml++)
    {
        li->v1 = &vertexes[SHORT(ml->v1)];
        li->v2 = &vertexes[SHORT(ml->v2)];

        li->angle = (SHORT(ml->angle))<<16;
        li->offset = (SHORT(ml->offset))<<16;
        linedef = SHORT(ml->linedef);
        ldef = &lines[linedef];
        li->linedef = ldef;
        side = SHORT(ml->side);
        li->sidedef = &sides[ldef->sidenum[side]];
        li->frontsector = sides[ldef->sidenum[side]].sector;

        //e6y: fix wrong side index
        if (side != 0 && side != 1)
        {
            C_Printf(CR_GOLD, " P_LoadSegs: seg %d contains wrong side index %d. Replaced with 1.\n", i, side);
        }

        // cph 2006/09/30 - our frontsector can be the second side of the
        // linedef, so must check for NO_INDEX in case we are incorrectly
        // referencing the back of a 1S line
        if (ldef->sidenum[side] == -1)
            C_Printf(CR_GOLD, " P_LoadSegs: front of seg %i has no sidedef\n", i);

        if (ldef-> flags & ML_TWOSIDED)
        {
            sidenum = ldef->sidenum[side ^ 1];

            // If the sidenum is out of range, this may be a "glass hack"
            // impassible window.  Point at side #0 (this may not be
            // the correct Vanilla behavior; however, it seems to work for
            // OTTAWAU.WAD, which is the one place I've seen this trick
            // used).

            if (sidenum < 0 || sidenum >= numsides)
            {
                li->backsector = GetSectorAtNullAddress();
            }
            else
            {
                li->backsector = sides[sidenum].sector;
            }
        }
        else
        {
            li->backsector = 0;
        }

        // e6y
        // check and fix wrong references to non-existent vertexes
        // see e1m9 @ NIVELES.WAD
        // http://www.doomworld.com/idgames/index.php?id=12647
/*
        if (ml->v1 >= numvertexes)
            C_Printf(" P_LoadSegs: compatibility loss - seg %d references a non-existent vertex %d\n"
                    , i, ml->v1);
        else if(ml->v2 >= numvertexes)
            C_Printf(" P_LoadSegs: compatibility loss - seg %d references a non-existent vertex %d\n"
                    , i, ml->v2);
*/
    }
    W_ReleaseLumpNum(lump);
}

//
// P_LoadSubsectors
//
void P_LoadSubsectors (int lump)
{
    byte*           data;
    int             i;
    mapsubsector_t* ms;
    subsector_t*    ss;
        
    numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
    subsectors = Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);        
    data = W_CacheLumpNum (lump,PU_STATIC);
        
    if(!data || !numsubsectors)
        C_Printf(CR_RED, " P_LoadSubsectors: No subsectors in map!");

    ms = (mapsubsector_t *)data;
    memset (subsectors,0, numsubsectors*sizeof(subsector_t));
    ss = subsectors;
    
    for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
    {
        ss->numlines = SHORT(ms->numsegs);
        ss->firstline = SHORT(ms->firstseg);
    }
        
    W_ReleaseLumpNum(lump);
}



//
// P_LoadSectors
//
void P_LoadSectors (int lump)
{
    byte*           data;
    int             i;
    mapsector_t*    ms;
    sector_t*       ss;
        
    numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
    sectors = Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);        
    memset (sectors, 0, numsectors*sizeof(sector_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
        
    ms = (mapsector_t *)data;
    ss = sectors;
    for (i=0 ; i<numsectors ; i++, ss++, ms++)
    {
        ss->floor_height = SHORT(ms->floor_height)<<FRACBITS;
        ss->ceiling_height = SHORT(ms->ceiling_height)<<FRACBITS;
        ss->floorpic = R_FlatNumForName(ms->floorpic);
        ss->ceilingpic = R_FlatNumForName(ms->ceilingpic);
        ss->lightlevel = SHORT(ms->lightlevel);
        ss->special = SHORT(ms->special);
        ss->tag = SHORT(ms->tag);
        ss->thinglist = NULL;

        // WiggleFix: [kb] for R_FixWiggle()
        ss->cachedheight = 0;

        // Sector interpolation.  Even if we're
        // not running uncapped, the renderer still
        // uses this data.
        ss->oldfloorheight = ss->floor_height;
        ss->interpfloorheight = ss->floor_height;
        ss->oldceilingheight = ss->ceiling_height;
        ss->interpceilingheight = ss->ceiling_height;
        ss->oldgametic = 0;
    }
        
    W_ReleaseLumpNum(lump);
}


//
// P_LoadNodes
//
void P_LoadNodes (int lump)
{
    byte*           data;
    int             i;
    int             j;
    int             k;
    mapnode_t*      mn;
    node_t*         no;
        
    numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
    nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);        
    data = W_CacheLumpNum (lump,PU_STATIC);
        
    mn = (mapnode_t *)data;
    no = nodes;
    
    for (i=0 ; i<numnodes ; i++, no++, mn++)
    {
        no->x = SHORT(mn->x)<<FRACBITS;
        no->y = SHORT(mn->y)<<FRACBITS;
        no->dx = SHORT(mn->dx)<<FRACBITS;
        no->dy = SHORT(mn->dy)<<FRACBITS;
        for (j=0 ; j<2 ; j++)
        {
            no->children[j] = SHORT(mn->children[j]);
            for (k=0 ; k<4 ; k++)
                no->bbox[j][k] = SHORT(mn->bbox[j][k])<<FRACBITS;
        }
    }
        
    W_ReleaseLumpNum(lump);
}


//
// P_LoadThings
//
void P_LoadThings (int lump)
{
    byte            *data;
    int             i;
    mapthing_t      *mt;
    mapthing_t      spawnthing;
    int             numthings;
    boolean         spawn;

    data = W_CacheLumpNum (lump,PU_STATIC);
    numthings = W_LumpLength (lump) / sizeof(mapthing_t);
        
    mt = (mapthing_t *)data;
    for (i=0 ; i<numthings ; i++, mt++)
    {
        spawn = true;

        // Do not spawn cool, new monsters if !commercial
        if (gamemode != commercial)
        {
            switch (SHORT(mt->type))
            {
              case 68:        // Arachnotron
              case 64:        // Archvile
              case 88:        // Boss Brain
              case 89:        // Boss Shooter
              case 69:        // Hell Knight
              case 67:        // Mancubus
              case 71:        // Pain Elemental
              case 65:        // Former Human Commando
              case 66:        // Revenant
              case 84:        // Wolf SS
                spawn = false;
                break;
            }
        }
        if (spawn == false)
            break;

        // Do spawn all other stuff. 
        spawnthing.x = SHORT(mt->x);
        spawnthing.y = SHORT(mt->y);
        spawnthing.angle = SHORT(mt->angle);
        spawnthing.type = SHORT(mt->type);
        spawnthing.options = SHORT(mt->options);
        
        P_SpawnMapThing(&spawnthing);
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs (int lump)
{
    byte*           data;
    int             i;
    maplinedef_t*   mld;
    line_t*         ld;
    vertex_t*       v1;
    vertex_t*       v2;
        
    numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
    lines = Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);        
    memset (lines, 0, numlines*sizeof(line_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
        
    mld = (maplinedef_t *)data;
    ld = lines;
    for (i=0 ; i<numlines ; i++, mld++, ld++)
    {
        ld->flags = SHORT(mld->flags);
        ld->special = SHORT(mld->special);
        ld->tag = SHORT(mld->tag);
        v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
        v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
        ld->dx = v2->x - v1->x;
        ld->dy = v2->y - v1->y;
        
        if (!ld->dx)
            ld->slopetype = ST_VERTICAL;
        else if (!ld->dy)
            ld->slopetype = ST_HORIZONTAL;
        else
        {
            if (FixedDiv (ld->dy , ld->dx) > 0)
                ld->slopetype = ST_POSITIVE;
            else
                ld->slopetype = ST_NEGATIVE;
        }
                
        if (v1->x < v2->x)
        {
            ld->bbox[BOXLEFT] = v1->x;
            ld->bbox[BOXRIGHT] = v2->x;
        }
        else
        {
            ld->bbox[BOXLEFT] = v2->x;
            ld->bbox[BOXRIGHT] = v1->x;
        }

        if (v1->y < v2->y)
        {
            ld->bbox[BOXBOTTOM] = v1->y;
            ld->bbox[BOXTOP] = v2->y;
        }
        else
        {
            ld->bbox[BOXBOTTOM] = v2->y;
            ld->bbox[BOXTOP] = v1->y;
        }

        ld->sidenum[0] = SHORT(mld->sidenum[0]);
        ld->sidenum[1] = SHORT(mld->sidenum[1]);

        // cph 2006/09/30 - fix sidedef errors right away.
        // cph 2002/07/20 - these errors are fatal if not fixed, so apply them
        // in compatibility mode - a desync is better than a crash! */
        int j;
        
        for (j=0; j < 2; j++)
        {
            if (ld->sidenum[j] != -1 && ld->sidenum[j] >= numsides)
            {
                ld->sidenum[j] = -1;
                C_Printf(CR_GOLD, " P_LoadLineDefs: linedef %d has out-of-range sidedef number\n", i);
            }
        }

        // killough 11/98: fix common wad errors (missing sidedefs):

        if (ld->sidenum[0] != -1)
            ld->frontsector = sides[ld->sidenum[0]].sector;
        else if (ld->sidenum[0] == -1)
        {
            // Substitute dummy sidedef for missing right side
            ld->sidenum[0] = 0;

            // cph - print a warning about the bug
            C_Printf(CR_GOLD, " P_LoadLineDefs: linedef %d missing first sidedef\n", i);
        }
        else
            ld->frontsector = 0;

        if (ld->sidenum[1] != -1)
            ld->backsector = sides[ld->sidenum[1]].sector;
        else if ((ld->sidenum[1] == -1) && (ld->flags & ML_TWOSIDED))
        {
            // e6y
            // ML_TWOSIDED flag shouldn't be cleared for compatibility purposes
            // see CLNJ-506.LMP at http://doomedsda.us/wad1005.html

            // Clear 2s flag for missing left side
            ld->flags &= ~ML_TWOSIDED;
            // cph - print a warning about the bug
            C_Printf(CR_GOLD, " P_LoadLineDefs: linedef %d has two-sided flag set, but no second sidedef\n", i);
        }
        else
            ld->backsector = 0;
    }

    W_ReleaseLumpNum(lump);
}


//
// P_LoadSideDefs
//
void P_LoadSideDefs (int lump)
{
    byte*           data;
    int             i;
    mapsidedef_t*   msd;
    side_t*         sd;
        
    numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
    sides = Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);        
    memset (sides, 0, numsides*sizeof(side_t));
    data = W_CacheLumpNum (lump,PU_STATIC);
        
    msd = (mapsidedef_t *)data;
    sd = sides;
    for (i=0 ; i<numsides ; i++, msd++, sd++)
    {
        sd->textureoffset = SHORT(msd->textureoffset)<<FRACBITS;
        sd->rowoffset = SHORT(msd->rowoffset)<<FRACBITS;
        sd->toptexture = R_TextureNumForName(msd->toptexture);
        sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
        sd->midtexture = R_TextureNumForName(msd->midtexture);
        sd->sector = &sectors[SHORT(msd->sector)];
    }

    W_ReleaseLumpNum(lump);
}


// taken from mbfsrc/P_SETUP.C:547-707, slightly adapted
static void P_CreateBlockMap(void)
{
    register int i;

    fixed_t minx = INT_MAX, miny = INT_MAX, maxx = INT_MIN, maxy = INT_MIN;

    // First find limits of map

    for (i=0; i<numvertexes; i++)
    {
        if (vertexes[i].x >> FRACBITS < minx)
            minx = vertexes[i].x >> FRACBITS;
        else if (vertexes[i].x >> FRACBITS > maxx)
            maxx = vertexes[i].x >> FRACBITS;
        if (vertexes[i].y >> FRACBITS < miny)
            miny = vertexes[i].y >> FRACBITS;
        else if (vertexes[i].y >> FRACBITS > maxy)
            maxy = vertexes[i].y >> FRACBITS;
    }

    // Save blockmap parameters

    bmaporgx = minx << FRACBITS;
    bmaporgy = miny << FRACBITS;

    bmapwidth  = ((maxx-minx) >> MAPBTOFRAC) + 1;
    bmapheight = ((maxy-miny) >> MAPBTOFRAC) + 1;

    // Compute blockmap, which is stored as a 2d array of variable-sized lists.
    //
    // Pseudocode:
    //
    // For each linedef:
    //
    //   Map the starting and ending vertices to blocks.
    //
    //   Starting in the starting vertex's block, do:
    //
    //     Add linedef to current block's list, dynamically resizing it.
    //
    //     If current block is the same as the ending vertex's block, exit loop.
    //
    //     Move to an adjacent block by moving towards the ending block in
    //     either the x or y direction, to the block which contains the linedef.

    typedef struct
    {
        int n, nalloc, *list;
    } bmap_t;  // blocklist structure

    unsigned tot = bmapwidth * bmapheight;            // size of blockmap

    bmap_t *bmap = calloc(sizeof *bmap, tot);         // array of blocklists

    int x, y, adx, ady, dx, dy, diff, b, bend;

    for (i=0; i < numlines; i++)
    {
        x = (lines[i].v1->x >> FRACBITS) - minx;
        y = (lines[i].v1->y >> FRACBITS) - miny;

        // x-y deltas
        adx = lines[i].dx >> FRACBITS, dx = adx < 0 ? -1 : 1;
        ady = lines[i].dy >> FRACBITS, dy = ady < 0 ? -1 : 1;

        // difference in preferring to move across y (>0) instead of x (<0)
        diff = !adx ? 1 : !ady ? -1 :
                (((x >> MAPBTOFRAC) << MAPBTOFRAC) +
                (dx > 0 ? MAPBLOCKUNITS-1 : 0) - x) * (ady = abs(ady)) * dx -
                (((y >> MAPBTOFRAC) << MAPBTOFRAC) +
                (dy > 0 ? MAPBLOCKUNITS-1 : 0) - y) * (adx = abs(adx)) * dy;

        // starting block, and pointer to its blocklist structure
        b = (y >> MAPBTOFRAC)*bmapwidth + (x >> MAPBTOFRAC);

        // ending block
        bend = (((lines[i].v2->y >> FRACBITS) - miny) >> MAPBTOFRAC) *
                bmapwidth + (((lines[i].v2->x >> FRACBITS) - minx) >> MAPBTOFRAC);

        // delta for pointer when moving across y
        dy *= bmapwidth;

        // deltas for diff inside the loop
        adx <<= MAPBTOFRAC;
        ady <<= MAPBTOFRAC;

        // Now we simply iterate block-by-block until we reach the end block.
        while ((unsigned) b < tot)    // failsafe -- should ALWAYS be true
        {
            // Increase size of allocated list if necessary
            if (bmap[b].n >= bmap[b].nalloc)
                bmap[b].list = realloc(bmap[b].list,
                                      (bmap[b].nalloc = bmap[b].nalloc ?
                                       bmap[b].nalloc*2 : 8)*sizeof*bmap->list);

            // Add linedef to end of list
            bmap[b].list[bmap[b].n++] = i;

            // If we have reached the last block, exit
            if (b == bend)
                break;

            // Move in either the x or y direction to the next block
            if (diff < 0)
                diff += ady, b += dx;
            else
                diff -= adx, b += dy;
        }
    }

    // Compute the total size of the blockmap.
    //
    // Compression of empty blocks is performed by reserving two offset words
    // at tot and tot+1.
    //
    // 4 words, unused if this routine is called, are reserved at the start.

    int count = tot + 6;  // we need at least 1 word per block, plus reserved's

    for (i = 0; i < tot; i++)
        if (bmap[i].n)
            count += bmap[i].n + 2; // 1 header word + 1 trailer word + blocklist

    // Allocate blockmap lump with computed count
    blockmaplump = Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, 0);

    // Now compress the blockmap.
    int ndx = tot += 4;         // Advance index to start of linedef lists
    bmap_t *bp = bmap;          // Start of uncompressed blockmap

    blockmaplump[ndx++] = 0;    // Store an empty blockmap list at start
    blockmaplump[ndx++] = -1;   // (Used for compression)

    for (i = 4; i < tot; i++, bp++)
        if (bp->n)                                       // Non-empty blocklist
        {
            blockmaplump[blockmaplump[i] = ndx++] = 0;   // Store index & header
            do
                blockmaplump[ndx++] = bp->list[--bp->n]; // Copy linedef list
            while (bp->n);
            blockmaplump[ndx++] = -1;                    // Store trailer
            free(bp->list);                              // Free linedef list
        }
        else            // Empty blocklist: point to reserved empty blocklist
            blockmaplump[i] = tot;

        free(bmap);    // Free uncompressed blockmap

    // copied over from P_LoadBlockMap()
    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = Z_Malloc(count, PU_LEVEL, 0);
    memset(blocklinks, 0, count);
    blockmap = blockmaplump+4;
}

//
// P_LoadBlockMap
//
void P_LoadBlockMap (int lump)
{
    int   i;
    int   count;
    int   lumplen;
    short *wadblockmaplump;

    // (re-)create BLOCKMAP if necessary
    if (lump >= numlumps ||
        (lumplen = W_LumpLength(lump)) < 8 ||
        (count = lumplen / 2) >= 0x10000)
    {
        createblockmap = true;
        C_Printf(CR_GOLD, " P_LoadBlockMap: (Re-)creating BLOCKMAP.\n");
        return;
    }

    // remove BLOCKMAP limit
    // adapted from boom202s/P_SETUP.C:1025-1076
    wadblockmaplump = Z_Malloc(lumplen, PU_LEVEL, NULL);
    W_ReadLump(lump, wadblockmaplump);
    blockmaplump = Z_Malloc(sizeof(*blockmaplump) * count, PU_LEVEL, NULL);
    blockmap = blockmaplump + 4;

    blockmaplump[0] = SHORT(wadblockmaplump[0]);
    blockmaplump[1] = SHORT(wadblockmaplump[1]);
    blockmaplump[2] = (int64_t)(SHORT(wadblockmaplump[2])) & 0xffff;
    blockmaplump[3] = (int64_t)(SHORT(wadblockmaplump[3])) & 0xffff;

    // Swap all short integers to native byte ordering.
  
    for (i=4; i<count; i++)
    {
        short t = SHORT(wadblockmaplump[i]);
        blockmaplump[i] = (t == -1) ? -1l : (int64_t) t & 0xffff;
    }

    Z_Free(wadblockmaplump);
                
    // Read the header

    bmaporgx = blockmaplump[0]<<FRACBITS;
    bmaporgy = blockmaplump[1]<<FRACBITS;
    bmapwidth = blockmaplump[2];
    bmapheight = blockmaplump[3];
        
    // Clear out mobj chains

    count = sizeof(*blocklinks) * bmapwidth * bmapheight;
    blocklinks = Z_Malloc(count, PU_LEVEL, 0);
    memset(blocklinks, 0, count);
}



//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines (void)
{
    line_t**        linebuffer;
    int             i;
    int             j;
    line_t*         li;
    sector_t*       sector;
    subsector_t*    ss;
    seg_t*          seg;
    fixed_t         bbox[4];
    int             block;
        
    // look up sector number for each subsector
    ss = subsectors;
    for (i=0 ; i<numsubsectors ; i++, ss++)
    {
        seg = &segs[ss->firstline];
        ss->sector = seg->sidedef->sector;
    }

    // count number of lines in each sector
    li = lines;
    totallines = 0;
    for (i=0 ; i<numlines ; i++, li++)
    {
        totallines++;
        li->frontsector->linecount++;

        if (li->backsector && li->backsector != li->frontsector)
        {
            li->backsector->linecount++;
            totallines++;
        }
    }

    // build line tables for each sector        
    linebuffer = Z_Malloc (totallines*sizeof(line_t *), PU_LEVEL, 0);

    for (i=0; i<numsectors; ++i)
    {
        // Assign the line buffer for this sector

        sectors[i].lines = linebuffer;
        linebuffer += sectors[i].linecount;

        // Reset linecount to zero so in the next stage we can count
        // lines into the list.

        sectors[i].linecount = 0;
    }

    // Assign lines to sectors

    for (i=0; i<numlines; ++i)
    { 
        li = &lines[i];

        if (li->frontsector != NULL)
        {
            sector = li->frontsector;

            sector->lines[sector->linecount] = li;
            ++sector->linecount;
        }

        if (li->backsector != NULL && li->frontsector != li->backsector)
        {
            sector = li->backsector;

            sector->lines[sector->linecount] = li;
            ++sector->linecount;
        }
    }
    
    // Generate bounding boxes for sectors
        
    sector = sectors;
    for (i=0 ; i<numsectors ; i++, sector++)
    {
        M_ClearBox (bbox);

        for (j=0 ; j<sector->linecount; j++)
        {
            li = sector->lines[j];

            M_AddToBox (bbox, li->v1->x, li->v1->y);
            M_AddToBox (bbox, li->v2->x, li->v2->y);
        }

        // set the degenmobj_t to the middle of the bounding box
        sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
        sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;
                
        // adjust bounding box to map blocks
        block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block >= bmapheight ? bmapheight-1 : block;
        sector->blockbox[BOXTOP]=block;

        block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXBOTTOM]=block;

        block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block >= bmapwidth ? bmapwidth-1 : block;
        sector->blockbox[BOXRIGHT]=block;

        block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
        block = block < 0 ? 0 : block;
        sector->blockbox[BOXLEFT]=block;
    }
        
}

//
// killough 10/98
//
// Remove slime trails.
//
// Slime trails are inherent to Doom's coordinate system -- i.e. there is
// nothing that a node builder can do to prevent slime trails ALL of the time,
// because it's a product of the integer coodinate system, and just because
// two lines pass through exact integer coordinates, doesn't necessarily mean
// that they will intersect at integer coordinates. Thus we must allow for
// fractional coordinates if we are to be able to split segs with node lines,
// as a node builder must do when creating a BSP tree.
//
// A wad file does not allow fractional coordinates, so node builders are out
// of luck except that they can try to limit the number of splits (they might
// also be able to detect the degree of roundoff error and try to avoid splits
// with a high degree of roundoff error). But we can use fractional coordinates
// here, inside the engine. It's like the difference between square inches and
// square miles, in terms of granularity.
//
// For each vertex of every seg, check to see whether it's also a vertex of
// the linedef associated with the seg (i.e, it's an endpoint). If it's not
// an endpoint, and it wasn't already moved, move the vertex towards the
// linedef by projecting it using the law of cosines. Formula:
//
//    dx²  x0 + dy²  x1 + dx dy (y0 - y1)  dy²  y0 + dx²  y1 + dx dy (x0 - x1)
//   {-----------------------------------, -----------------------------------}
//
//                dx²  + dy²                           dx²  + dy²
//
// (x0,y0) is the vertex being moved, and (x1,y1)-(x1+dx,y1+dy) is the
// reference linedef.
//
// Segs corresponding to orthogonal linedefs (exactly vertical or horizontal
// linedefs), which comprise at least half of all linedefs in most wads, don't
// need to be considered, because they almost never contribute to slime trails
// (because then any roundoff error is parallel to the linedef, which doesn't
// cause slime). Skipping simple orthogonal lines lets the code finish quicker.
//
// Please note: This section of code is not interchangable with TeamTNT's
// code which attempts to fix the same problem.
//
// Firelines (TM) is a Rezistered Trademark of MBF Productions
//
// Mostly taken from Lee Killough's implementation in mbfsrc/P_SETUP.C:849-924,
// with the exception that not the actual vertex coordinates are modified,
// but pseudovertexes which are dummies that are *only* used in rendering,
// i.e. r_bsp.c:R_AddLine()
//
static void P_RemoveSlimeTrails(void)
{
    int i;

    for (i = 0; i < numsegs; i++)
    {
        const line_t *l = segs[i].linedef;
        vertex_t *v = segs[i].v1;

        // ignore exactly vertical or horizontal linedefs
        if (l->dx && l->dy)
        {
            do
            {
                // vertex wasn't already moved
                if (!v->moved)
                {
                    v->moved = true;
                    // ignore endpoints of linedefs
                    if (v != l->v1 && v != l->v2)
                    {
                        // move the vertex towards the linedef
                        // by projecting it using the law of cosines
                        int64_t dx2 = (l->dx >> FRACBITS) * (l->dx >> FRACBITS);
                        int64_t dy2 = (l->dy >> FRACBITS) * (l->dy >> FRACBITS);
                        int64_t dxy = (l->dx >> FRACBITS) * (l->dy >> FRACBITS);
                        int64_t s = dx2 + dy2;
                        int x0 = v->x, y0 = v->y, x1 = l->v1->x, y1 = l->v1->y;

                        // MBF actually overrides v->x and v->y here
                        v->px = (fixed_t)((dx2 * x0 + dy2 * x1 + dxy * (y0 - y1)) / s);
                        v->py = (fixed_t)((dy2 * y0 + dx2 * y1 + dxy * (x0 - x1)) / s);
                    }
                }
            // if v doesn't point to the second vertex of the seg already, point it there
            } while ((v != segs[i].v2) && (v = segs[i].v2));
        }
    }
}

// Pad the REJECT lump with extra data when the lump is too small,
// to simulate a REJECT buffer overflow in Vanilla Doom.

static void PadRejectArray(byte *array, unsigned int len)
{
    unsigned int i;
    unsigned int byte_num;
    byte *dest;
    unsigned int padvalue;

    // Values to pad the REJECT array with:

    unsigned int rejectpad[4] =
    {
        ((totallines * 4 + 3) & ~3) + 24,     // Size
        0,                                    // Part of z_zone block header
        50,                                   // PU_LEVEL
        0x1d4a11                              // DOOM_CONST_ZONEID
    };

    // Copy values from rejectpad into the destination array.

    dest = array;

    for (i=0; i<len && i<sizeof(rejectpad); ++i)
    {
        byte_num = i % 4;
        *dest = (rejectpad[i / 4] >> (byte_num * 8)) & 0xff;
        ++dest;
    }

    // We only have a limited pad size.  Print a warning if the
    // REJECT lump is too small.

    if (len > sizeof(rejectpad))
    {
        C_Printf(CR_GOLD, " PadRejectArray: REJECT lump too short to pad! (%i > %i)\n",
                        len, (int) sizeof(rejectpad));

        // Pad remaining space with 0 (or 0xff, if specified on command line).

        padvalue = 0xf00;

        memset(array + sizeof(rejectpad), padvalue, len - sizeof(rejectpad));
    }
}

static void P_LoadReject(int lumpnum)
{
    int minlength;
    int lumplen;

    // Calculate the size that the REJECT lump *should* be.

    minlength = (numsectors * numsectors + 7) / 8;

    // If the lump meets the minimum length, it can be loaded directly.
    // Otherwise, we need to allocate a buffer of the correct size
    // and pad it with appropriate data.

    lumplen = W_LumpLength(lumpnum);

    if (lumplen >= minlength)
    {
        rejectmatrix = W_CacheLumpNum(lumpnum, PU_LEVEL);
    }
    else
    {
        rejectmatrix = Z_Malloc(minlength, PU_LEVEL, &rejectmatrix);
        W_ReadLump(lumpnum, rejectmatrix);

        PadRejectArray(rejectmatrix + lumplen, minlength - lumplen);

        C_Printf(CR_GOLD, " P_LoadReject: REJECT too short (%d<%d) - padded\n", minlength, lumplen);
    }
}

//
// P_SetupLevel
//
void
P_SetupLevel
( int                episode,
  int                map,
  int                playermask,
  skill_t            skill)
{
    int              i;
    char             lumpname[9];
    int              lumpnum;
        
    mus_cheat_used = false;
    finale_music = false;

    numsplats = 0;

    totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
    wminfo.partime = 180;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        players[i].killcount = players[i].secretcount 
            = players[i].itemcount = 0;
    }

    // Initial height of PointOfView
    // will be set by player think.
    players[consoleplayer].viewz = 1; 

    // Make sure all sounds are stopped before Z_FreeTags.
    S_Start ();                        

    Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);

    // UNUSED W_Profile ();
    P_InitThinkers ();
           
    // find map name
    if ( gamemode == commercial)
    {
        if (map<10)
            DEH_snprintf(lumpname, 9, "map0%i", map);
        else
            DEH_snprintf(lumpname, 9, "map%i", map);
    }
    else
    {
        if(fsize != 12538385 || (fsize == 12538385 && map < 10))
        {
            lumpname[0] = 'E';
            lumpname[1] = '0' + episode;
            lumpname[2] = 'M';
            lumpname[3] = '0' + map;
            lumpname[4] = 0;
        }
        else
            DEH_snprintf(lumpname, 9, "e1m10");
    }

    if(beta_style && gamemode != shareware && gamemode != commercial)
    {
        if(gameepisode == 1 && gamemap == 2)
            DEH_snprintf(lumpname, 9, "e1m0");

        if(gameepisode == 2 && gamemap == 2)
            DEH_snprintf(lumpname, 9, "e3m0");

        if(gameepisode == 3 && gamemap == 5)
            DEH_snprintf(lumpname, 9, "e2m0");
    }

    lumpnum = W_GetNumForName (lumpname);

    if (nerve_pwad && gamemission != pack_nerve)
    {
        lumpnum = W_GetSecondNumForName (lumpname);
    }

    leveltime = 0;
        
    createblockmap = false;

    // note: most of this ordering is important        
    P_LoadBlockMap (lumpnum+ML_BLOCKMAP);
    P_LoadVertexes (lumpnum+ML_VERTEXES);
    P_LoadSectors (lumpnum+ML_SECTORS);
    P_LoadSideDefs (lumpnum+ML_SIDEDEFS);

    P_LoadLineDefs (lumpnum+ML_LINEDEFS);

    // (re-)create BLOCKMAP if necessary
    if (createblockmap)
        P_CreateBlockMap();

    P_LoadSubsectors (lumpnum+ML_SSECTORS);
    P_LoadNodes (lumpnum+ML_NODES);
    P_LoadSegs (lumpnum+ML_SEGS);

    P_GroupLines ();
    P_LoadReject (lumpnum+ML_REJECT);

    // remove slime trails
    P_RemoveSlimeTrails();

    bodyqueslot = 0;
    deathmatch_p = deathmatchstarts;
    P_LoadThings (lumpnum+ML_THINGS);
    
    for (i=0 ; i<MAXPLAYERS ; i++)
        P_InitCards(&players[i]);

    P_InitAnimatedLiquids();

    // if deathmatch, randomly spawn the active players
    if (deathmatch)
    {
        for (i=0 ; i<MAXPLAYERS ; i++)
            if (playeringame[i])
            {
                players[i].mo = NULL;
                G_DeathMatchSpawnPlayer (i);
            }
                        
    }

    // clear special respawning que
    iquehead = iquetail = 0;                
        
    // set up world state
    P_SpawnSpecials ();
        
    // build subsector connect matrix
    //        UNUSED P_ConnectSubsectors ();

    // preload graphics
    if (precache)
        R_PrecacheLevel ();

    //printf ("free memory: 0x%x\n", Z_FreeMemory());

    HU_NewLevel();
}



//
// P_Init
//
void P_Init (void)
{
    P_InitSwitchList ();
    P_InitPicAnims ();
//    P_InitTerrainTypes();
    R_InitSprites (sprnames);
}



