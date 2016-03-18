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
//  all external data is defined here
//  most of the data is loaded into different structures at run time
//  some internal structures shared by many modules are here
//
//-----------------------------------------------------------------------------


#ifndef __DOOMDATA__
#define __DOOMDATA__


// The most basic types we use, portability.
#include "../doomtype.h"

// Some global defines, that configure the game.
#include "doomdef.h"


//
// LineDef attributes.
//

#define NO_INDEX                ((unsigned short)(-1))

// Solid, is an obstacle.
#define ML_BLOCKING             1

// Blocks monsters only.
#define ML_BLOCKMONSTERS        2

// Backside will not be present at all
//  if not two sided.
#define ML_TWOSIDED             4

// If a texture is pegged, the texture will have
// the end exposed to air held constant at the
// top or bottom of the texture (stairs or pulled
// down things) and will move with a height change
// of one of the neighbor sectors.
// Unpegged textures allways have the first row of
// the texture at the top pixel of the line for both
// top and bottom textures (use next to windows).

// upper texture unpegged
#define ML_DONTPEGTOP           8

// lower texture unpegged
#define ML_DONTPEGBOTTOM        16        

// In AutoMap: don't map as two sided: IT'S A SECRET!
#define ML_SECRET               32

// Sound rendering: don't let sound cross two of these.
#define ML_SOUNDBLOCK           64

// Don't draw on the automap at all.
#define ML_DONTDRAW             128

// Set if already seen, thus drawn in automap.
#define ML_MAPPED               256

//jff 3/21/98 Set if line absorbs use by player
//allow multiple push/switch triggers to be used on one push
#define ML_PASSUSE              512

// Indicate a leaf.
#define NF_SUBSECTOR            0x80000000


//
// Map level types.
// The following data structures define the persistent format
// used in the lumps of the WAD files.
//

// Lump order in a map WAD: each map needs a couple of lumps
// to provide a complete scene geometry description.
enum
{
    // A separator, name, ExMx or MAPxx
    ML_LABEL,

    // Monsters, items..
    ML_THINGS,

    // LineDefs, from editing
    ML_LINEDEFS,

    // SideDefs, from editing
    ML_SIDEDEFS,

    // Vertices, edited and BSP splits generated
    ML_VERTEXES,

    // LineSegs, from LineDefs split by BSP
    ML_SEGS,

    // SubSectors, list of LineSegs
    ML_SSECTORS,

    // BSP nodes
    ML_NODES,

    // Sectors, from editing
    ML_SECTORS,

    // LUT, sector-sector visibility
    ML_REJECT,

    // LUT, motion clipping, walls/grid element
    ML_BLOCKMAP
};


// A single Vertex.
typedef struct
{
    short              x;
    short              y;

} PACKEDATTR mapvertex_t;


// A SideDef, defining the visual appearance of a wall,
// by setting textures and offsets.
typedef struct
{
    short              textureoffset;
    short              rowoffset;
    char               toptexture[8];
    char               bottomtexture[8];
    char               midtexture[8];

    // Front sector, towards viewer.
    short              sector;

} PACKEDATTR mapsidedef_t;



// A LineDef, as used for editing, and as input
// to the BSP builder.
typedef struct
{
    short              v1;
    short              v2;
    short              flags;
    short              special;
    short              tag;

    // sidenum[1] will be -1 if one sided
    short              sidenum[2];                

} PACKEDATTR maplinedef_t;

typedef struct
{
    unsigned short     numsegs;
    int                firstseg;

} PACKEDATTR mapsubsector_v4_t;

typedef struct
{
    int                v1;
    int                v2;
    unsigned short     angle;
    unsigned short     linedef;
    short              side;
    unsigned short     offset;

} PACKEDATTR mapseg_v4_t;

typedef struct
{
    short              x;
    short              y;
    short              dx;
    short              dy;
    short              bbox[2][4];
    int                children[2];

} PACKEDATTR mapnode_v4_t;

typedef struct
{
    unsigned int       v1;
    unsigned int       v2;
    unsigned short     linedef;
    unsigned char      side;

} PACKEDATTR mapseg_znod_t;

typedef struct
{
    short              x;
    short              y;
    short              dx;
    short              dy;
    short              bbox[2][4];
    int                children[2];

} PACKEDATTR mapnode_znod_t;

typedef struct
{
    unsigned int       numsegs;

} PACKEDATTR mapsubsector_znod_t;

// Sector definition, from editing.
typedef struct
{
    short              floorheight;
    short              ceilingheight;
    char               floorpic[8];
    char               ceilingpic[8];
    short              lightlevel;
    short              special;
    short              tag;

} PACKEDATTR mapsector_t;

// SubSector, as generated by BSP.
typedef struct
{
    short              numsegs;

    // Index of first one, segs are stored sequentially.
    short              firstseg;

} PACKEDATTR mapsubsector_t;

// LineSeg, generated by splitting LineDefs
// using partition lines selected by BSP builder.
typedef struct
{
    short              v1;
    short              v2;
    short              angle;                
    short              linedef;
    short              side;
    short              offset;

} PACKEDATTR mapseg_t;

// BSP node structure.
typedef struct
{
    // Partition line from (x, y) to x + dx, y + dy)
    short              x;
    short              y;
    short              dx;
    short              dy;

    // Bounding box for each child,
    // clip against view frustum.
    short              bbox[2][4];

    // If NF_SUBSECTOR its a subsector,
    // else it's a node of another subtree.
    unsigned short     children[2];

} PACKEDATTR mapnode_t;

// Thing definition, position, orientation and type,
// plus skill/visibility flags and attributes.
typedef struct
{
    short              x;
    short              y;
    short              angle;
    short              type;
    short              options;

} PACKEDATTR mapthing_t;

#endif // __DOOMDATA__

