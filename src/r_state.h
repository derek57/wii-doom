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
//        Refresh/render internal state variables (global).
//
//-----------------------------------------------------------------------------


#ifndef __R_STATE__
#define __R_STATE__


// Need data structure definitions.
#include "d_player.h"
#include "r_data.h"


//
// Refresh internal data structures,
//  for rendering.
//

extern player_t*           viewplayer;

extern angle_t             viewangle;
extern angle_t             xtoviewangle[SCREENWIDTH+1];
extern angle_t             rw_normalangle;

// ?
extern angle_t             clipangle;

// needed for texture pegging
extern fixed_t*            textureheight;

// needed for pre rendering (fracs)
extern fixed_t*            spritewidth;

extern fixed_t*            spriteoffset;
extern fixed_t*            spritetopoffset;
extern fixed_t             rw_distance;

// POV data.
extern fixed_t             viewx;
extern fixed_t             viewy;
extern fixed_t             viewz;

extern lighttable_t*       colormaps;

extern spritedef_t*        sprites;
extern vertex_t*           vertexes;
extern seg_t*              segs;
extern sector_t*           sectors;
extern subsector_t*        subsectors;
extern node_t*             nodes;
extern line_t*             lines;
extern side_t*             sides;
extern visplane_t*         floorplane;
extern visplane_t*         ceilingplane;

extern int                 viewwidth;
extern int                 scaledviewwidth;
extern int                 viewheight;
extern int                 scaledviewheight;   // ADDED FOR HIRES
extern int                 firstflat;

// Sprite....
extern int                 firstspritelump;
extern int                 lastspritelump;
extern int                 numspritelumps;

// Lookup tables for map data.
extern int                 numsprites;
extern int                 numvertexes;
extern int                 numsegs;
extern int                 numsectors;
extern int                 numsubsectors;
extern int                 numnodes;
extern int                 numlines;
extern int                 numsides;
extern int                 viewangletox[FINEANGLES/2];


// angle to line origin
extern int                 rw_angle1;

// Segs count?
extern int                 sscount;

// for global animation
extern int*                flattranslation;        
extern int*                texturetranslation;        


#endif
