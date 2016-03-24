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


#if !defined(__R_STATE__)
#define __R_STATE__


// Need data structure definitions.
#include "d_player.h"
#include "r_data.h"


//
// Refresh internal data structures,
//  for rendering.
//

// needed for texture pegging
extern fixed_t          *textureheight;

// needed for pre rendering (fracs)
extern fixed_t          *spritewidth;
extern fixed_t          *spriteheight;

extern fixed_t          *spriteoffset;
extern fixed_t          *spritetopoffset;
extern fixed_t          *newspriteoffset;
extern fixed_t          *newspritetopoffset;

//
// POV data.
//
extern fixed_t          viewx;
extern fixed_t          viewy;
extern fixed_t          viewz;

// ?
extern angle_t          clipangle;

extern angle_t          xtoviewangle[SCREENWIDTH + 1];
extern angle_t          rw_normalangle;
extern angle_t          viewangle;

extern player_t         *viewplayer;

extern visplane_t       *floorplane;
extern visplane_t       *ceilingplane;

//
// Lookup tables for map data.
//
extern spritedef_t      *sprites;

extern vertex_t         *vertexes;

extern seg_t            *segs;

extern sector_t         *sectors;

extern subsector_t      *subsectors;

extern node_t           *nodes;

extern line_t           *lines;

extern side_t           *sides;

extern leaf_t           *leafs;

extern int              numsprites;
extern int              numvertexes;
extern int              numsegs;
extern int              numsectors;
extern int              numsubsectors;
extern int              numnodes;
extern int              numlines;
extern int              numsides;
extern int              numthings;

// [SVE] svillarreal
extern int              numleafs;

// for global animation
extern int              *flattranslation;
extern int              *texturetranslation;

// Sprite....
extern int              firstspritelump;
extern int              lastspritelump;
extern int              numspritelumps;

extern int              viewwidth;
extern int              scaledviewwidth;
extern int              scaledviewheight;
extern int              viewheight;
extern int              firstflat;
extern int              viewangletox[FINEANGLES / 2];

extern byte             **flatfullbright;
extern byte             **texturefullbright;

extern dboolean         boomlinespecials;
extern dboolean         blockmaprecreated;


#endif

