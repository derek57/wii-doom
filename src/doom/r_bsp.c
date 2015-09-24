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
//        BSP traversal, handling of LineSegs for rendering.
//
//-----------------------------------------------------------------------------


#include "doomdef.h"

// State.
#include "doomstat.h"

#ifdef WII
#include "../i_system.h"
#include "../m_bbox.h"
#else
#include "i_system.h"
#include "m_bbox.h"
#endif

#include "r_local.h"
#include "r_main.h"
#include "r_plane.h"
#include "r_state.h"
#include "r_things.h"


#define MAXSEGS     (SCREENWIDTH/2+1) // [crispy] RAISE TO THE VALUE FOUND IN MBF


// 1/11/98: Lee Killough
//
// This fixes many strange venetian blinds crashes, which occurred when a scan
// line had too many "posts" of alternating non-transparent and transparent
// regions. Using a doubly-linked list to represent the posts is one way to
// do it, but it has increased overhead and poor spatial locality, which hurts
// cache performance on modern machines. Since the maximum number of posts
// theoretically possible is a function of screen width, a static limit is
// okay in this case. It used to be 32, which was way too small.
//
// This limit was frequently mistaken for the visplane limit in some Doom
// editing FAQs, where visplanes were said to "double" if a pillar or other
// object split the view's space into two pieces horizontally. That did not
// have anything to do with visplanes, but it had everything to do with these
// clip posts.

//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
typedef struct
{
    int first;
    int last;
    
} cliprange_t;


// newend is one past the last valid seg
cliprange_t*        newend;
cliprange_t         solidsegs[MAXSEGS];

seg_t*              curline;

side_t*             sidedef;

line_t*             linedef;

sector_t*           frontsector;
sector_t*           backsector;

drawseg_t*          drawsegs = NULL;                // CHANGED FOR HIRES
drawseg_t*          ds_p;

int                 numdrawsegs = 0;                // ADDED FOR HIRES

int                 checkcoord[12][4] =
{
    {3,0,2,1},
    {3,0,2,0},
    {3,1,2,0},
    {0},
    {2,0,2,1},
    {0,0,0,0},
    {3,1,3,0},
    {0},
    {2,0,3,1},
    {2,1,3,1},
    {2,1,3,0}
};


unsigned int    maxdrawsegs;

//
// R_ClearDrawSegs
//
void R_ClearDrawSegs (void)
{
    ds_p = drawsegs;
}



//
// R_ClipSolidWallSegment
// Does handle solid walls,
//  e.g. single sided LineDefs (middle texture)
//  that entirely block the view.
// 
static void R_ClipSolidWallSegment(int first, int last)
{
    cliprange_t *next;
    cliprange_t *start = solidsegs;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    while (start->last < first - 1)
        ++start;

    if (first < start->first)
    {
        if (last < start->first - 1)
        {
            // Post is entirely visible (above start), so insert a new clippost.
            R_StoreWallRange(first, last);

            // 1/11/98 killough: performance tuning using fast memmove
            memmove(start + 1, start, (++newend - start) * sizeof(*start));
            start->first = first;
            start->last = last;
            return;
        }

        // There is a fragment above *start.
        R_StoreWallRange(first, start->first - 1);

        // Now adjust the clip size.
        start->first = first;
    }

    // Bottom contained in start?
    if (last <= start->last)
        return;

    next = start;
    while (last >= (next + 1)->first - 1)
    {
        // There is a fragment between two posts.
        R_StoreWallRange(next->last + 1, (next + 1)->first - 1);
        ++next;

        if (last <= next->last)
        {
            // Bottom is contained in next. Adjust the clip size.
            start->last = next->last;
            goto crunch;
        }
    }

    // There is a fragment after *next.
    R_StoreWallRange(next->last + 1, last);

    // Adjust the clip size.
    start->last = last;

    // Remove start + 1 to next from the clip list,
    // because start now covers their area.

crunch:

    if (next == start)
        return;                 // Post just extended past the bottom of one post.

    while (next++ != newend)
        *(++start) = *next;     // Remove a post.

    newend = start;
}



//
// R_ClipPassWallSegment
// Clips the given range of columns,
//  but does not includes it in the clip list.
// Does handle windows,
//  e.g. LineDefs with upper and lower texture.
//
void
R_ClipPassWallSegment
(   int           first,
    int           last )
{
    cliprange_t*  start;

    // Find the first range that touches the range
    //  (adjacent pixels are touching).
    start = solidsegs;
    while (start->last < first-1)
        start++;

    if (first < start->first)
    {
        if (last < start->first-1)
        {
            // Post is entirely visible (above start).
            R_StoreWallRange (first, last);
            return;
        }
                
        // There is a fragment above *start.
        R_StoreWallRange (first, start->first - 1);
    }

    // Bottom contained in start?
    if (last <= start->last)
        return;                        
                
    while (last >= (start+1)->first-1)
    {
        // There is a fragment between two posts.
        R_StoreWallRange (start->last + 1, (start+1)->first - 1);
        start++;
        
        if (last <= start->last)
            return;
    }
        
    // There is a fragment after *next.
    R_StoreWallRange (start->last + 1, last);
}



//
// R_ClearClipSegs
//
void R_ClearClipSegs (void)
{
    solidsegs[0].first = -0x7fffffff;
    solidsegs[0].last = -1;
    solidsegs[1].first = viewwidth;
    solidsegs[1].last = 0x7fffffff;
    newend = solidsegs+2;
}

// [AM] Interpolate the passed sector, if prudent.
void R_MaybeInterpolateSector(sector_t* sector)
{
    if (d_uncappedframerate &&
        // Only if we moved the sector last tic.
        sector->oldgametic == gametic - 1)
    {
        // Interpolate between current and last floor/ceiling position.
        if (sector->floor_height != sector->oldfloorheight)
            sector->interpfloorheight = sector->oldfloorheight +
                    FixedMul(sector->floor_height -
                            sector->oldfloorheight, fractionaltic);
        else
            sector->interpfloorheight = sector->floor_height;
        if (sector->ceiling_height != sector->oldceilingheight)
            sector->interpceilingheight = sector->oldceilingheight +
                    FixedMul(sector->ceiling_height -
                            sector->oldceilingheight, fractionaltic);
        else
            sector->interpceilingheight = sector->ceiling_height;
    }
    else
    {
        sector->interpfloorheight = sector->floor_height;
        sector->interpceilingheight = sector->ceiling_height;
    }
}

//
// killough 3/7/98: Hack floor/ceiling heights for deep water etc.
//
// If player's view height is underneath fake floor, lower the
// drawn ceiling to be just under the floor height, and replace
// the drawn floor and ceiling textures, and light level, with
// the control sector's.
//
// Similar for ceiling, only reflected.
//
// killough 4/11/98, 4/13/98: fix bugs, add 'back' parameter
//
sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec, int *floorlightlevel,
    int *ceilinglightlevel, boolean back)
{
    if (floorlightlevel)
        *floorlightlevel = (sec->floorlightsec == -1 ? sec->lightlevel :
            sectors[sec->floorlightsec].lightlevel);

    if (ceilinglightlevel)
        *ceilinglightlevel = (sec->ceilinglightsec == -1 ? sec->lightlevel :
            sectors[sec->ceilinglightsec].lightlevel);

    if (sec->heightsec != -1)
    {
        const sector_t  *s = &sectors[sec->heightsec];
        int             heightsec = viewplayer->mo->subsector->sector->heightsec;
        int             underwater = (heightsec != -1 && viewz <= sectors[heightsec].interpfloorheight);

        // Replace sector being drawn, with a copy to be hacked
        *tempsec = *sec;

        // Replace floor and ceiling height with other sector's heights.
        tempsec->interpfloorheight = s->interpfloorheight;
        tempsec->interpceilingheight = s->interpceilingheight;

        // killough 11/98: prevent sudden light changes from non-water sectors:
        if (underwater && (tempsec->interpfloorheight = sec->interpfloorheight,
            tempsec->interpceilingheight = s->interpfloorheight - 1, !back))
        {
            // head-below-floor hack
            tempsec->floorpic = s->floorpic;
            tempsec->floor_xoffs = s->floor_xoffs;
            tempsec->floor_yoffs = s->floor_yoffs;

            if (underwater)
            {
                if (s->ceilingpic == skyflatnum)
                {
                    tempsec->interpfloorheight = tempsec->interpceilingheight + 1;
                    tempsec->ceilingpic = tempsec->floorpic;
                    tempsec->ceiling_xoffs = tempsec->floor_xoffs;
                    tempsec->ceiling_yoffs = tempsec->floor_yoffs;
                }
                else
                {
                    tempsec->ceilingpic = s->ceilingpic;
                    tempsec->ceiling_xoffs = s->ceiling_xoffs;
                    tempsec->ceiling_yoffs = s->ceiling_yoffs;
                }
            }

            tempsec->lightlevel = s->lightlevel;

            if (floorlightlevel)
                *floorlightlevel = (s->floorlightsec == -1 ? s->lightlevel :
                    sectors[s->floorlightsec].lightlevel);              // killough 3/16/98

            if (ceilinglightlevel)
                *ceilinglightlevel = (s->ceilinglightsec == -1 ? s->lightlevel :
                    sectors[s->ceilinglightsec].lightlevel);            // killough 4/11/98
        }
        else if (heightsec != -1 && viewz >= sectors[heightsec].interpceilingheight
            && sec->interpceilingheight > s->interpceilingheight)
        {
            // Above-ceiling hack
            tempsec->interpceilingheight = s->interpceilingheight;
            tempsec->interpfloorheight = s->interpceilingheight + 1;

            tempsec->floorpic = tempsec->ceilingpic = s->ceilingpic;
            tempsec->floor_xoffs = tempsec->ceiling_xoffs = s->ceiling_xoffs;
            tempsec->floor_yoffs = tempsec->ceiling_yoffs = s->ceiling_yoffs;

            if (s->floorpic != skyflatnum)
            {
                tempsec->interpceilingheight = sec->interpceilingheight;
                tempsec->floorpic = s->floorpic;
                tempsec->floor_xoffs = s->floor_xoffs;
                tempsec->floor_yoffs = s->floor_yoffs;
            }

            tempsec->lightlevel = s->lightlevel;

            if (floorlightlevel)
                *floorlightlevel = (s->floorlightsec == -1 ? s->lightlevel :
                    sectors[s->floorlightsec].lightlevel);              // killough 3/16/98

            if (ceilinglightlevel)
                *ceilinglightlevel = (s->ceilinglightsec == -1 ? s->lightlevel :
                    sectors[s->ceilinglightsec].lightlevel);            // killough 4/11/98
        }
        sec = tempsec;        // Use other sector
    }
    return sec;
}

//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//
void R_AddLine (seg_t*     line)
{
    int                    x1;
    int                    x2;
    angle_t                angle1;
    angle_t                angle2;
    angle_t                span;
    angle_t                tspan;
    static sector_t        tempsec;        // killough 3/8/98: ceiling/water hack
    
    curline = line;

    // OPTIMIZE: quickly reject orthogonal back sides.
    // [crispy] remove slime trails
    angle1 = R_PointToAngleCrispy (line->v1->px, line->v1->py);
    angle2 = R_PointToAngleCrispy (line->v2->px, line->v2->py);
    
    // Clip to view edges.
    // OPTIMIZE: make constant out of 2*clipangle (FIELDOFVIEW).
    span = angle1 - angle2;
    
    // Back side? I.e. backface culling?
    if (span >= ANG180)
        return;                

    // Global angle needed by segcalc.
    rw_angle1 = angle1;
    angle1 -= viewangle;
    angle2 -= viewangle;
        
    tspan = angle1 + clipangle;
    if (tspan > 2*clipangle)
    {
        tspan -= 2*clipangle;

        // Totally off the left edge?
        if (tspan >= span)
            return;
        
        angle1 = clipangle;
    }
    tspan = clipangle - angle2;
    if (tspan > 2*clipangle)
    {
        tspan -= 2*clipangle;

        // Totally off the left edge?
        if (tspan >= span)
            return;        
        angle2 = -clipangle;
    }
    
    // The seg is in the view range,
    // but not necessarily visible.
    angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
    angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;

    // killough 1/31/98: Here is where "slime trails" can SOMETIMES occur:
    x1 = viewangletox[angle1];
    x2 = viewangletox[angle2];

    // Does not cross a pixel?
    if (x1 == x2)
        return;                                
        
    backsector = line->backsector;

    // Single sided line?
    if (!backsector)
        goto clipsolid;                

    // [AM] Interpolate sector movement before
    // running clipping tests.  Frontsector
    // should already be interpolated.
    R_MaybeInterpolateSector(backsector);

    // killough 3/8/98, 4/4/98: hack for invisible ceilings / deep water
    backsector = R_FakeFlat(backsector, &tempsec, NULL, NULL, true);

    // Closed door.
    if (backsector->interpceilingheight <= frontsector->interpfloorheight
        || backsector->interpfloorheight >= frontsector->interpceilingheight)
        goto clipsolid;                

    // Window.
    if (backsector->interpceilingheight != frontsector->interpceilingheight
        || backsector->interpfloorheight != frontsector->interpfloorheight)
        goto clippass;        
                
    // Reject empty lines used for triggers
    //  and special events.
    // Identical floor and ceiling on both sides,
    // identical light levels on both sides,
    // and no middle texture.
    if (backsector->ceilingpic == frontsector->ceilingpic
        && backsector->floorpic == frontsector->floorpic
        && backsector->lightlevel == frontsector->lightlevel
        && curline->sidedef->midtexture == 0

        // killough 3/7/98: Take flats offsets into account:
        && backsector->floor_xoffs == frontsector->floor_xoffs
        && backsector->floor_yoffs == frontsector->floor_yoffs
        && backsector->ceiling_xoffs == frontsector->ceiling_xoffs
        && backsector->ceiling_yoffs == frontsector->ceiling_yoffs

        // killough 4/16/98: consider altered lighting
        && backsector->floorlightsec == frontsector->floorlightsec
        && backsector->ceilinglightsec == frontsector->ceilinglightsec)
    {
        return;
    }
    
                                
  clippass:
    R_ClipPassWallSegment (x1, x2-1);        
    return;
                
  clipsolid:
    R_ClipSolidWallSegment (x1, x2-1);
}


//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//  if some part of the bbox might be visible.
//
boolean R_CheckBBox (fixed_t*  bspcoord)
{
    int                        boxx;
    int                        boxy;
    int                        boxpos;

    fixed_t                    x1;
    fixed_t                    y1;
    fixed_t                    x2;
    fixed_t                    y2;
    
    angle_t                    angle1;
    angle_t                    angle2;
    angle_t                    span;
    angle_t                    tspan;
    
    cliprange_t*               start;

    int                        sx1;
    int                        sx2;
    
    // Find the corners of the box
    // that define the edges from current viewpoint.
    if (viewx <= bspcoord[BOXLEFT])
        boxx = 0;
    else if (viewx < bspcoord[BOXRIGHT])
        boxx = 1;
    else
        boxx = 2;
                
    if (viewy >= bspcoord[BOXTOP])
        boxy = 0;
    else if (viewy > bspcoord[BOXBOTTOM])
        boxy = 1;
    else
        boxy = 2;
                
    boxpos = (boxy<<2)+boxx;
    if (boxpos == 5)
        return true;
        
    x1 = bspcoord[checkcoord[boxpos][0]];
    y1 = bspcoord[checkcoord[boxpos][1]];
    x2 = bspcoord[checkcoord[boxpos][2]];
    y2 = bspcoord[checkcoord[boxpos][3]];
    
    // check clip list for an open space
    angle1 = R_PointToAngleCrispy (x1, y1) - viewangle;
    angle2 = R_PointToAngleCrispy (x2, y2) - viewangle;
        
    span = angle1 - angle2;

    // Sitting on a line?
    if (span >= ANG180)
        return true;
    
    tspan = angle1 + clipangle;

    if (tspan > 2*clipangle)
    {
        tspan -= 2*clipangle;

        // Totally off the left edge?
        if (tspan >= span)
            return false;        

        angle1 = clipangle;
    }
    tspan = clipangle - angle2;
    if (tspan > 2*clipangle)
    {
        tspan -= 2*clipangle;

        // Totally off the left edge?
        if (tspan >= span)
            return false;
        
        angle2 = -clipangle;
    }


    // Find the first clippost
    //  that touches the source post
    //  (adjacent pixels are touching).
    angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
    angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
    sx1 = viewangletox[angle1];
    sx2 = viewangletox[angle2];

    // Does not cross a pixel.
    if (sx1 == sx2)
        return false;                        
    sx2--;
        
    start = solidsegs;
    while (start->last < sx2)
        start++;
    
    if (sx1 >= start->first
        && sx2 <= start->last)
    {
        // The clippost contains the new span.
        return false;
    }

    return true;
}



//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//
void R_Subsector (int num)
{
    int               count;
    int               floorlightlevel;      // killough 3/16/98: set floor lightlevel
    int               ceilinglightlevel;    // killough 4/11/98
    sector_t          tempsec;              // killough 3/7/98: deep water hack
    seg_t*            line;
    subsector_t*      sub;
        
#ifdef RANGECHECK
    if (num>=numsubsectors)
        I_Error ("R_Subsector: ss %i with numss = %i",
                 num,
                 numsubsectors);
#endif

    sub = &subsectors[num];
    frontsector = sub->sector;
    count = sub->numlines;
    line = &segs[sub->firstline];

    // [AM] Interpolate sector movement.  Usually only needed
    // when you're standing inside the sector.
    R_MaybeInterpolateSector(frontsector);

    // killough 3/8/98, 4/4/98: Deep water / fake ceiling effect
    frontsector = R_FakeFlat(frontsector, &tempsec, &floorlightlevel, &ceilinglightlevel, false);

    floorplane = (frontsector->interpfloorheight < viewz        // killough 3/7/98
        || (frontsector->heightsec != -1
        && sectors[frontsector->heightsec].ceilingpic == skyflatnum) ?
        R_FindPlane(frontsector->interpfloorheight,
            (frontsector->floorpic == skyflatnum                // killough 10/98
                && (frontsector->sky & PL_SKYFLAT) ? frontsector->sky : frontsector->floorpic),
            floorlightlevel,                                    // killough 3/16/98
            frontsector->floor_xoffs,                           // killough 3/7/98
            frontsector->floor_yoffs) : NULL);

    ceilingplane = (frontsector->interpceilingheight > viewz
        || frontsector->ceilingpic == skyflatnum
        || (frontsector->heightsec != -1
        && sectors[frontsector->heightsec].floorpic == skyflatnum) ?
        R_FindPlane(frontsector->interpceilingheight,           // killough 3/8/98
            (frontsector->ceilingpic == skyflatnum              // killough 10/98
            && (frontsector->sky & PL_SKYFLAT) ? frontsector->sky : frontsector->ceilingpic),
            ceilinglightlevel,                                  // killough 4/11/98
            frontsector->ceiling_xoffs,                         // killough 3/7/98
            frontsector->ceiling_yoffs) : NULL);

    // killough 9/18/98: Fix underwater slowdown, by passing real sector
    // instead of fake one. Improve sprite lighting by basing sprite
    // lightlevels on floor & ceiling lightlevels in the surrounding area.
    //
    // 10/98 killough:
    //
    // NOTE: TeamTNT fixed this bug incorrectly, messing up sprite lighting!!!
    // That is part of the 242 effect!!!  If you simply pass sub->sector to
    // the old code you will not get correct lighting for underwater sprites!!!
    // Either you must pass the fake sector and handle validcount here, on the
    // real sector, or you must account for the lighting in some other way,
    // like passing it as an argument.
    if (sub->sector->validcount != validcount)
    {
        sub->sector->validcount = validcount;
        R_AddSprites(sub->sector, (floorlightlevel + ceilinglightlevel) / 2);
    }

    while (count--)
    {
        R_AddLine(line++);
        curline = NULL;
    }
}




//
// RenderBSPNode
// Renders all subsectors below a given node,
//  traversing subtree recursively.
// Just call with BSP root.
void R_RenderBSPNode (int bspnum)
{
    node_t*               bsp;
    int                   side;

    // Found a subsector?
    if (bspnum & NF_SUBSECTOR)
    {
        if (bspnum == -1)                        
            R_Subsector (0);
        else
            R_Subsector (bspnum&(~NF_SUBSECTOR));
        return;
    }
                
    bsp = &nodes[bspnum];
    
    // Decide which side the view point is on.
    side = R_PointOnSide (viewx, viewy, bsp);

    // Recursively divide front space.
    R_RenderBSPNode (bsp->children[side]); 

    // Possibly divide back space.
    if (R_CheckBBox (bsp->bbox[side^1]))        
        R_RenderBSPNode (bsp->children[side^1]);
}


