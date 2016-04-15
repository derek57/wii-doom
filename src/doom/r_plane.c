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


#include <stdlib.h>

#include "c_io.h"
#include "doomstat.h"
#include "i_system.h"
#include "i_timer.h"
#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"
#include "w_wad.h"
#include "wii-doom.h"
#include "z_zone.h"


// must be a power of 2
#define MAXVISPLANES    128

// ?
#define MAXOPENINGS     SCREENWIDTH * 64 * 4

// killough -- hash function for visplanes
// Empirically verified to be fairly uniform:
#define visplane_hash(picnum, lightlevel, height) \
    (((unsigned int)(picnum) * 3 + (unsigned int)(lightlevel) + \
    (unsigned int)(height) * 7) & (MAXVISPLANES - 1))

// Ripple Effect from Eternity Engine (r_ripple.cpp) by Simon Howard
#define AMP             2
#define AMP2            2
#define SPEED           40

// swirl factors determine the number of waves per flat width
// 1 cycle per 64 units
#define SWIRLFACTOR     (8192 / 64)

// 1 cycle per 32 units (2 in 64)
#define SWIRLFACTOR2    (8192 / 32)

static visplane_t       *visplanes = NULL;
static visplane_t       *lastvisplane;

// texture mapping
static lighttable_t     **planezlight;

// killough 2/28/98: flat offsets
static fixed_t          xoffs;
static fixed_t          yoffs;

static fixed_t          planeheight;

static byte             *normalflat;
static byte             distortedflat[4096];

// spanstart holds the start of a plane span
// initialized to 0 at start
static int              spanstart[SCREENHEIGHT];

static int              numvisplanes;

fixed_t                 yslope[SCREENHEIGHT];
fixed_t                 distscale[SCREENWIDTH];

visplane_t              *floorplane;
visplane_t              *ceilingplane;

size_t                  maxopenings;

// Clip values are the solid pixel bounding the range.
//  floorclip starts out SCREENHEIGHT
//  ceilingclip starts out -1

// dropoff overflow
int                     *openings;
int                     *lastopening;
int                     floorclip[SCREENWIDTH];
int                     ceilingclip[SCREENWIDTH];


extern fixed_t          animatedliquiddiff;


//
// R_MapPlane
//
// Uses global vars:
//  planeheight
//  ds_source
//  viewx
//  viewy
//
// BASIC PRIMITIVE
//
static void R_MapPlane(int y, int x1, int x2)
{
    fixed_t     distance;
    int         dx, dy;

    if (y == centery)
        return;

#ifdef RANGECHECK
    if (x2 < x1 || x1 < 0 || x2 >= viewwidth || y > viewheight)
    {
        C_Error("R_MapPlane: %i, %i at %i", x1, x2, y);
        return;
    }
#endif

    distance = FixedMul(planeheight, yslope[y]);

    dx = x1 - centerx;
    dy = ABS(centery - y);
    ds_xstep = FixedMul(viewsin, planeheight) / dy;
    ds_ystep = FixedMul(viewcos, planeheight) / dy;

    ds_xfrac = viewx + xoffs + FixedMul(viewcos, distance) + dx * ds_xstep;
    ds_yfrac = -viewy + yoffs - FixedMul(viewsin, distance) + dx * ds_ystep;

    ds_colormap = (fixedcolormap ? fixedcolormap :
        planezlight[BETWEEN(0, distance >> LIGHTZSHIFT, MAXLIGHTZ - 1)]);

    ds_y = y;
    ds_x1 = x1;
    ds_x2 = x2;

    spanfunc();
}

//
// R_ClearPlanes
// At beginning of frame.
//
void R_ClearPlanes(void)
{
    int i;

    // opening/clipping determination
    for (i = 0; i < viewwidth; i++)
    {
        floorclip[i] = viewheight;
        ceilingclip[i] = -1;
    }

    lastvisplane = visplanes;
    lastopening = openings;
}

// [crispy] remove MAXVISPLANES Vanilla limit
static void R_RaiseVisplanes (visplane_t **vp)
{
    if (lastvisplane - visplanes == numvisplanes)
    {
        int numvisplanes_old = numvisplanes;
        visplane_t* visplanes_old = visplanes;

        numvisplanes = numvisplanes ? 2 * numvisplanes : MAXVISPLANES;
        visplanes = Z_Realloc(visplanes, numvisplanes * sizeof(*visplanes));
        memset(visplanes + numvisplanes_old, 0, (numvisplanes - numvisplanes_old) * sizeof(*visplanes));

        lastvisplane = visplanes + numvisplanes_old;
        floorplane = visplanes + (floorplane - visplanes_old);
        ceilingplane = visplanes + (ceilingplane - visplanes_old);

        if (numvisplanes_old)
            C_Warning("R_FindPlane: Hit MAXVISPLANES limit at %d, raised to %d.", numvisplanes_old, numvisplanes);

        // keep the pointer passed as argument in relation to the visplanes pointer
        if (vp)
            *vp = visplanes + (*vp - visplanes_old);
    }
}

//
// R_FindPlane
//
visplane_t *R_FindPlane(fixed_t height, int picnum, int lightlevel, fixed_t xoffs, fixed_t yoffs)
{
    visplane_t          *check;

    // killough 10/98
    if (picnum == skyflatnum || (picnum & PL_SKYFLAT))
        // killough 7/19/98: most skies map together
        height = lightlevel = 0;

    for (check = visplanes; check < lastvisplane; check++)
        if (height == check->height && picnum == check->picnum && lightlevel == check->lightlevel)
            break;

    if (check < lastvisplane)
        return check;

    R_RaiseVisplanes(&check);

    lastvisplane++;

    check->height = height;
    check->picnum = picnum;
    check->lightlevel = lightlevel;
    check->minx = viewwidth;
    check->maxx = -1;

    // killough 2/28/98: Save offsets
    check->xoffs = xoffs;
    check->yoffs = yoffs;

    // FIXME: The 2nd memset() argument '32767' doesn't fit into an 'unsigned char'.
    memset(check->top, SHRT_MAX, sizeof(check->top));

    return check;
}

//
// R_CheckPlane
//
visplane_t *R_CheckPlane(visplane_t *pl, int start, int stop)
{
    int intrl;
    int intrh;
    int unionl;
    int unionh;
    int x;

    if (start < pl->minx)
    {
        intrl = pl->minx;
        unionl = start;
    }
    else
    {
        unionl = pl->minx;
        intrl = start;
    }

    if (stop > pl->maxx)
    {
        intrh = pl->maxx;
        unionh = stop;
    }
    else
    {
        unionh = pl->maxx;
        intrh = stop;
    }

    for (x = intrl; x <= intrh && pl->top[x] == SHRT_MAX; x++);

    // [crispy] fix HOM if ceilingplane and floorplane are the same
    // visplane (e.g. both skies)
    if (!(pl == floorplane && markceiling && floorplane == ceilingplane) && x > intrh)
    {
        pl->minx = unionl;
        pl->maxx = unionh;
    }
    else
    {
        // make a new visplane
        R_RaiseVisplanes(&pl);

        lastvisplane->height = pl->height;
        lastvisplane->picnum = pl->picnum;
        lastvisplane->lightlevel = pl->lightlevel;
        lastvisplane->sector = pl->sector;

        // killough 2/28/98
        lastvisplane->xoffs = pl->xoffs;
        lastvisplane->yoffs = pl->yoffs;
    
        pl = lastvisplane++;

        // make a new visplane
        pl->minx = start;
        pl->maxx = stop;

        // FIXME: The 2nd memset() argument '32767' doesn't fit into an 'unsigned char'.
        memset(pl->top, SHRT_MAX, sizeof(pl->top));
    }

    return pl;
}

//
// R_MakeSpans
//
static void R_MakeSpans(visplane_t *pl)
{
    int x;

    for (x = pl->minx; x <= pl->maxx + 1; ++x)
    {
        unsigned short  t1 = pl->top[x - 1];
        unsigned short  b1 = pl->bottom[x - 1];
        unsigned short  t2 = pl->top[x];
        unsigned short  b2 = pl->bottom[x];

        for (; t1 < t2 && t1 <= b1; ++t1)
            R_MapPlane(t1, spanstart[t1], x - 1);

        for (; b1 > b2 && b1 >= t1; --b1)
            R_MapPlane(b1, spanstart[b1], x - 1);

        while (t2 < t1 && t2 <= b2)
            spanstart[t2++] = x;

        while (b2 > b1 && b2 >= t2)
            spanstart[b2--] = x;
    }
}

//
// R_DistortedFlat
//
// Generates a distorted flat from a normal one using a two-dimensional
// sine wave pattern.
//
byte *R_DistortedFlat(int flatnum)
{
    static int  lastflat = -1;
    static int  swirltic = -1;
    static int  offset[4096];
    int         i;
    int         leveltic = gametic;

    // Already swirled this one?
    if (leveltic == swirltic && lastflat == flatnum)
        return distortedflat;

    lastflat = flatnum;

    // built this tic?
    if (leveltic != swirltic && ((consoleactive || swirltic == -1) ||
                                (!consoleactive || swirltic == -1)) && !paused)
    {
        int     x, y;

        leveltic *= SPEED; 
        for (x = 0; x < 64; ++x)
            for (y = 0; y < 64; ++y)
            {
                int     x1, y1;
                int     sinvalue, sinvalue2;

                sinvalue = (y * SWIRLFACTOR + leveltic * 5 + 900) & 8191;
                sinvalue2 = (x * SWIRLFACTOR2 + leveltic * 4 + 300) & 8191;
                x1 = x + 128 + ((finesine[sinvalue] * AMP) >> FRACBITS)
                    + ((finesine[sinvalue2] * AMP2) >> FRACBITS);

                sinvalue = (x * SWIRLFACTOR + leveltic * 3 + 700) & 8191;
                sinvalue2 = (y * SWIRLFACTOR2 + leveltic * 4 + 1200) & 8191;
                y1 = y + 128 + ((finesine[sinvalue] * AMP) >> FRACBITS)
                    + ((finesine[sinvalue2] * AMP2) >> FRACBITS);

                offset[(y << 6) + x] = ((y1 & 63) << 6) + (x1 & 63); 
            }

        swirltic = gametic;
    }

    normalflat = W_CacheLumpNum(firstflat + flatnum, PU_LEVEL);

    for (i = 0; i < 4096; ++i)
        distortedflat[i] = normalflat[offset[i]];

    return distortedflat;
}

//
// R_DrawPlanes
// At the end of each frame.
//
void R_DrawPlanes(void)
{
    visplane_t      *pl;

#ifdef RANGECHECK

    if (ds_p - drawsegs > maxdrawsegs)
        I_Error("R_DrawPlanes: drawsegs overflow (%i)",
                 ds_p - drawsegs);

    if (lastvisplane - visplanes > numvisplanes)
        I_Error("R_DrawPlanes: visplane overflow (%i)",
                 lastvisplane - visplanes);

    if (lastopening - openings > MAXOPENINGS)
        I_Error("R_DrawPlanes: opening overflow (%i)",
                 lastopening - openings);
#endif

    for (pl = visplanes ; pl < lastvisplane ; pl++)
    {
        int     picnum = pl->picnum;

        if (pl->minx > pl->maxx)
            continue;

        // sky flat
        if (picnum == skyflatnum || (picnum & PL_SKYFLAT))
        {
            int         x;
            int         texture;
            int         offset;
            angle_t     an, flip;
            rpatch_t    *tex_patch; 

            // killough 10/98: allow skies to come from sidedefs.
            // Allows scrolling and/or animated skies, as well as
            // arbitrary multiple skies per level without having
            // to use info lumps.
            an = viewangle;

            if (picnum & PL_SKYFLAT)
            {
                // Sky Linedef
                const line_t    *l = &lines[picnum & ~PL_SKYFLAT];

                // Sky transferred from first sidedef
                const side_t    *s = *l->sidenum + sides;

                // Texture comes from upper texture of reference sidedef
                texture = texturetranslation[s->toptexture];

                // Horizontal offset is turned into an angle offset,
                // to allow sky rotation as well as careful positioning.
                // However, the offset is scaled very small, so that it
                // allows a long-period of sky rotation.
                an += s->textureoffset;

                // Vertical offset allows careful sky positioning.
                dc_texturemid = s->rowoffset - 28 * FRACUNIT;

                // We sometimes flip the picture horizontally.
                //
                // DOOM always flipped the picture, so we make it optional,
                // to make it easier to use the new feature, while to still
                // allow old sky textures to be used.
                flip = (l->special == TransferSkyTextureToTaggedSectors_Flipped ?
                    0u : ~0u);
            }
            // Normal DOOM sky, only one allowed per level
            else
            {
                // Default y-offset
                dc_texturemid = skytexturemid;

                // Default texture
                texture = skytexture;

                // DOOM flips it
                flip = 0;
            }

            dc_iscale = pspriteiscale >> (!hires);

            // [crispy] stretch sky
            if (mouselook > 0)
                dc_iscale = dc_iscale * 108 / 228;

            // Sky is always drawn full bright,
            //  i.e. colormaps[0] is used.
            // Because of this hack, sky is not affected
            //  by INVUL inverse mapping.
            dc_colormap = (fixedcolormap ? fixedcolormap : fullcolormap);

            dc_texheight = textureheight[texture] >> FRACBITS;

            tex_patch = R_CacheTextureCompositePatchNum(texture);

            offset = skycolumnoffset >> FRACBITS;

            for (x = pl->minx; x <= pl->maxx; x++)
            {
                dc_yl = pl->top[x];
                dc_yh = pl->bottom[x];

                if (dc_yl <= dc_yh)
                {
                    dc_x = x;
                    dc_source = R_GetTextureColumn(tex_patch,
                        (((an + xtoviewangle[x]) ^ flip) >> ANGLETOSKYSHIFT) + offset);
                    skycolfunc();
                }
            }

            R_UnlockTextureCompositePatchNum(texture);
        }
        else
        {
            // regular flat
            dboolean    liquid = isliquid[picnum];
            dboolean    swirling = (liquid && d_swirl && !menuactive);
            int         lumpnum = firstflat + flattranslation[picnum];

            ds_source = (swirling ? R_DistortedFlat(picnum) :
                W_CacheLumpNum(lumpnum, PU_STATIC));

            // killough 2/28/98: Add offsets
            xoffs = pl->xoffs;
            yoffs = pl->yoffs;

            planeheight = ABS(pl->height - viewz);

            if (liquid && pl->sector && d_swirl && isliquid[pl->sector->floorpic])
                planeheight -= animatedliquiddiff;

            planezlight = zlight[BETWEEN(0, (pl->lightlevel >> LIGHTSEGSHIFT)
                + extralight * LIGHTBRIGHT, LIGHTLEVELS - 1)];

            pl->top[pl->minx - 1] = pl->top[pl->maxx + 1] = SHRT_MAX;

            R_MakeSpans(pl);

            if (!swirling)
                W_ReleaseLumpNum(lumpnum);
        }
    }
}

