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


#include "c_io.h"
#include "doomfeatures.h"
#include "doomstat.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_tinttab.h"
#include "p_local.h"
#include "p_partcl.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"


#define MAX_SPRITE_FRAMES       29
#define MINZ                    (FRACUNIT * 4)
#define BASEYCENTER             (ORIGINALHEIGHT / 2)
#define R_SpriteNameHash(s)     ((unsigned int)((s)[0] - ((s)[1] * 3 - (s)[3] * 2 - (s)[2]) * 2))


//
// Sprite rotation 0 is facing the viewer,
//  rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//  which increases counter clockwise (protractor).
// There was a lot of stuff grabbed wrong, so I changed it...
//

static vissprite_t              *vissprites;
static vissprite_t              **vissprite_ptrs;
static vissprite_t              bloodvissprites[NUMVISSPRITES];
static vissprite_t              shadowvissprites[NUMVISSPRITES];

// killough 1/25/98 made static
static lighttable_t             **spritelights;

static spriteframe_t            sprtemp[MAX_SPRITE_FRAMES];

static int                      maxframe;
static int                      num_bloodvissprite;
static int                      num_shadowvissprite;

static unsigned int             num_vissprite;
static unsigned int             num_vissprite_alloc;

static dboolean                 bflash;


fixed_t                         pspriteyscale;
fixed_t                         pspriteiscale;
fixed_t                         pspritescale;
fixed_t                         spryscale;

particle_t                      *Particles;

spritedef_t                     *sprites;

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up and range check thing_t sprites patches
int                             numsprites;

// constant arrays
//  used for psprite clipping and initializing clipping
int                             negonearray[SCREENWIDTH];
int                             screenheightarray[SCREENWIDTH];

// haleyjd: global particle system state
int                             numParticles;
int                             activeParticles;
int                             inactiveParticles;

int                             *mfloorclip;
int                             *mceilingclip;
int                             fuzzpos;

int64_t                         sprtopscreen;
int64_t                         shift;


extern fixed_t                  animatedliquiddiff;

extern dboolean                 skippsprinterp;
extern dboolean                 realframe;


//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
static void R_InstallSpriteLump(lumpinfo_t *lump, int lumpnum, unsigned int frame, char rot,
    dboolean flipped)
{
    unsigned int        rotation = (rot >= '0' && rot <= '9' ? rot - '0' : (rot >= 'A' ?
        rot - 'A' + 10 : 17));

    if (frame >= MAX_SPRITE_FRAMES || rotation > 16)
        I_Error("R_InstallSpriteLump: Bad frame characters in lump %s", lump->name);

    if ((int)frame > maxframe)
        maxframe = frame;

    if (!rotation)
    {
        int r;

        // the lump should be used for all rotations
        for (r = 14; r >= 0; r -= 2)
        {
            if (sprtemp[frame].lump[r] == -1)
            {
                sprtemp[frame].lump[r] = lumpnum - firstspritelump;

                if (flipped)
                    sprtemp[frame].flip |= (1 << r);

                // jff 4/24/98 if any subbed, rotless
                sprtemp[frame].rotate = false;
            }
        }

        return;
    }

    // the lump is only used for one rotation
    rotation = (rotation <= 8 ? (rotation - 1) * 2 : (rotation - 9) * 2 + 1);

    if (sprtemp[frame].lump[rotation] == -1)
    {
        sprtemp[frame].lump[rotation] = lumpnum - firstspritelump;

        if (flipped)
            sprtemp[frame].flip |= (1 << rotation);

        // jff 4/24/98 only change if rot used
        sprtemp[frame].rotate = true;
    }
}

//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
// (4 chars exactly) to be used.
//
// Builds the sprite rotation matrices to account
// for horizontally flipped sprites.
//
// Will report an error if the lumps are inconsistent.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
//
// A sprite that is flippable will have an additional
//  letter/number appended.
//
// The rotation character can be 0 to signify no rotations.
//
// 1/25/98, 1/31/98 killough : Rewritten for performance
//
// Empirically verified to have excellent hash
// properties across standard DOOM sprites:
static void R_InitSpriteDefs(const char *const *namelist)
{
    size_t              numentries = lastspritelump - firstspritelump + 1;
    unsigned int        i;

    struct
    {
        int     index;
        int     next;
    } *hash;

    if (!numentries || !*namelist)
        return;

    // count the number of sprite names
    for (i = 0; namelist[i]; ++i);

    numsprites = (signed int)i;

    sprites = Z_Calloc(numsprites, sizeof(*sprites), PU_STATIC, NULL);

    // Create hash table based on just the first four letters of each sprite
    // killough 1/31/98

    // allocate hash table
    hash = malloc(sizeof(*hash) * numentries);

    // initialize hash table as empty
    for (i = 0; i < numentries; i++)
        hash[i].index = -1;

    // Prepend each sprite to hash chain
    for (i = 0; i < numentries; i++)
    {
        // prepend so that later ones win
        int     j = R_SpriteNameHash(lumpinfo[i + firstspritelump]->name) % numentries;

        hash[i].next = hash[j].index;
        hash[j].index = i;
    }

    // scan all the lump names for each of the names,
    //  noting the highest frame letter.
    for (i = 0; i < (unsigned int)numsprites; ++i)
    {
        const char      *spritename = namelist[i];
        int             j = hash[R_SpriteNameHash(spritename) % numentries].index;

        if (j >= 0)
        {
            int k;

            memset(sprtemp, -1, sizeof(sprtemp));

            for (k = 0; k < MAX_SPRITE_FRAMES; ++k)
                sprtemp[k].flip = 0;

            maxframe = -1;

            do
            {
                lumpinfo_t      *lump = lumpinfo[j + firstspritelump];

                // Fast portable comparison -- killough
                // (using int pointer cast is nonportable):
                if (!((lump->name[0] ^ spritename[0]) | (lump->name[1] ^ spritename[1])
                    | (lump->name[2] ^ spritename[2]) | (lump->name[3] ^ spritename[3])))
                {
                    R_InstallSpriteLump(lump, j + firstspritelump, lump->name[4] - 'A',
                        lump->name[5], false);

                    if (lump->name[6])
                        R_InstallSpriteLump(lump, j + firstspritelump, lump->name[6] - 'A',
                           lump->name[7], true);
                }

            } while ((j = hash[j].next) >= 0);

            // check the frames that were found for completeness
            // killough 1/31/98
            if ((sprites[i].numframes = ++maxframe))
            {
                int     frame;
                int     rot;

                for (frame = 0; frame < maxframe; ++frame)
                    switch (sprtemp[frame].rotate)
                    {
                        case -1:
                            // no rotations were found for that frame at all
                            C_Warning("R_InitSprites: No patches found for %s frame %c", namelist[i], frame+'A');
                            break;

                        case 0:
                            // only the first rotation is needed
                            for (rot = 1; rot < 16; ++rot)
                                sprtemp[frame].lump[rot] = sprtemp[frame].lump[0];

                            // If the frame is flipped, they all should be
                            if (sprtemp[frame].flip & 1)
                                sprtemp[frame].flip = 0xFFFF;

                            break;

                        case 1:
                            // must have all 8 frames
                            for (rot = 0; rot < 16; rot += 2)
                            {
                                if (sprtemp[frame].lump[rot + 1] == -1)
                                {
                                    sprtemp[frame].lump[rot + 1] = sprtemp[frame].lump[rot];
                                    if (sprtemp[frame].flip & (1 << rot))
                                        sprtemp[frame].flip |= 1 << (rot + 1);
                                }

                                if (sprtemp[frame].lump[rot] == -1)
                                {
                                    sprtemp[frame].lump[rot] = sprtemp[frame].lump[rot + 1];
                                    if (sprtemp[frame].flip & (1 << (rot + 1)))
                                        sprtemp[frame].flip |= 1 << rot;
                                }
                            }

                            for (rot = 0; rot < 16; ++rot)
                                if (sprtemp[frame].lump[rot] == -1)
                                    I_Error("R_InitSprites: Frame %c of sprite %.8s frame %c is "
                                        "missing rotations", frame + 'A', namelist[i]);

                            break;
                    }

                for (frame = 0; frame < maxframe; ++frame)
                    if (sprtemp[frame].rotate == -1)
                    {
                        memset(&sprtemp[frame].lump, 0, sizeof(sprtemp[0].lump));
                        sprtemp[frame].flip = 0;
                        sprtemp[frame].rotate = 0;
                    }

                // allocate space for the frames present and copy sprtemp to it
                sprites[i].spriteframes = Z_Malloc(maxframe * sizeof(spriteframe_t), PU_STATIC,
                    NULL);
                memcpy(sprites[i].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
            }
        }
    }

    // free hash table
    free(hash);
}

//
// GAME FUNCTIONS
//

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites(char **namelist)
{
    int i;

    for (i = 0; i < SCREENWIDTH; i++)
        negonearray[i] = -1;

    R_InitSpriteDefs((const char *const *)namelist);

    num_vissprite = 0;
    num_vissprite_alloc = 128;
    vissprites = malloc(num_vissprite_alloc * sizeof(vissprite_t));
    vissprite_ptrs = malloc(num_vissprite_alloc * sizeof(vissprite_t *));
}

//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites(void)
{
    if (num_vissprite >= num_vissprite_alloc)
    {
        num_vissprite_alloc += 128;
        vissprites = Z_Realloc(vissprites, num_vissprite_alloc * sizeof(vissprite_t));
        vissprite_ptrs = Z_Realloc(vissprite_ptrs, num_vissprite_alloc * sizeof(vissprite_t *));
    }

    num_vissprite = 0;
    num_bloodvissprite = 0;
    num_shadowvissprite = 0;
}

//
// R_NewVisSprite
//
static vissprite_t *R_NewVisSprite(fixed_t scale)
{
    unsigned int        pos;
    unsigned int        pos2;
    unsigned int        step;
    unsigned int        count;
    vissprite_t         *rc;
    vissprite_t         *vis;

    switch (num_vissprite)
    {
        case 0:
            rc = &vissprites[0];
            vissprite_ptrs[0] = rc;
            num_vissprite = 1;
            return rc;

        case 1:
            vis = &vissprites[0];
            rc = &vissprites[1];

            if (scale > vis->scale)
            {
                vissprite_ptrs[0] = rc;
                vissprite_ptrs[1] = vis;
            }
            else
                vissprite_ptrs[1] = rc;

            num_vissprite = 2;
            return rc;
    }

    pos = (num_vissprite + 1) >> 1;
    step = (pos + 1) >> 1;
    count = (pos << 1);

    do
    {
        fixed_t d1;
        fixed_t d2;

        vis = vissprite_ptrs[pos];
        d1 = INT_MAX;
        d2 = vis->scale;

        if (scale >= d2)
        {
            if (!pos)
                break;

            vis = vissprite_ptrs[pos - 1];
            d1 = vis->scale;

            if (scale <= d1)
                break;
        }

        pos = (scale > d1 ? MAX(0, pos - step) : MIN(pos + step, num_vissprite - 1));
        step = (step + 1) >> 1;
        count >>= 1;

        if (!count)
        {
            pos = num_vissprite;
            break;
        }

    } while (1);

    if (num_vissprite >= num_vissprite_alloc)
    {
        if (pos >= num_vissprite)
            return NULL;

        rc = vissprite_ptrs[num_vissprite - 1];
    }
    else
        rc = &vissprites[num_vissprite++];

    pos2 = num_vissprite - 1;

    do
    {
        vissprite_ptrs[pos2] = vissprite_ptrs[pos2 - 1];

    } while (--pos2 > pos);

    vissprite_ptrs[pos] = rc;

    return rc;
}

//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//  in posts/runs of opaque pixels.
//
static void R_DrawMaskedSpriteColumn(column_t *column)
{
    byte        topdelta;
    int         ceilingclip = mceilingclip[dc_x] + 1;
    int         floorclip = mfloorclip[dc_x] - 1;

    while ((topdelta = column->topdelta) != 0xFF)
    {
        int     length = column->length;

        // calculate unclipped screen coordinates for post
        int64_t topscreen = sprtopscreen + spryscale * topdelta + 1;

        dc_yl = MAX((int)((topscreen + FRACUNIT) >> FRACBITS), ceilingclip);
        dc_yh = MIN((int)((topscreen + spryscale * length) >> FRACBITS), floorclip);

        if (dc_baseclip != -1)
            dc_yh = MIN(dc_baseclip, dc_yh);

        dc_texturefrac = dc_texturemid - (topdelta << FRACBITS)
            + FixedMul((dc_yl - centery) << FRACBITS, dc_iscale);

        if (dc_texturefrac < 0)
        {
            int cnt = (FixedDiv(-dc_texturefrac, dc_iscale) + FRACUNIT - 1) >> FRACBITS;

            dc_yl += cnt;
            dc_texturefrac += cnt * dc_iscale;
        }

        {
            const fixed_t       endfrac = dc_texturefrac + (dc_yh - dc_yl) * dc_iscale;
            const fixed_t       maxfrac = length << FRACBITS;

            if (endfrac >= maxfrac)
                dc_yh -= (FixedDiv(endfrac - maxfrac - 1, dc_iscale) + FRACUNIT - 1) >> FRACBITS;
        }

        if (dc_yl <= dc_yh && dc_yh < viewheight)
        {
            dc_source = (byte *)column + 3;
            colfunc();
        }

        column = (column_t *)((byte *)column + length + 4);
    }
}

static void R_DrawMaskedBloodSplatColumn(column_t *column)
{
    byte        topdelta;
    int         ceilingclip = mceilingclip[dc_x] + 1;
    int         floorclip = mfloorclip[dc_x] - 1;

    while ((topdelta = column->topdelta) != 0xFF)
    {
        int     length = column->length;

        // calculate unclipped screen coordinates for post
        int64_t topscreen = sprtopscreen + spryscale * topdelta;

        dc_yl = MAX((int)(topscreen >> FRACBITS) + 1, ceilingclip);
        dc_yh = MIN((int)((topscreen + spryscale * length) >> FRACBITS), floorclip);

        if (dc_yl <= dc_yh && dc_yh < viewheight)
            colfunc();

        column = (column_t *)((byte *)column + length + 4);
    }
}

static void R_DrawMaskedShadowColumn(column_t *column)
{
    byte        topdelta;
    int         ceilingclip = mceilingclip[dc_x] + 1;
    int         floorclip = mfloorclip[dc_x] - 1;

    while ((topdelta = column->topdelta) != 0xFF)
    {
        int     length = column->length;

        // calculate unclipped screen coordinates for post
        int64_t topscreen = sprtopscreen + spryscale * topdelta;

        dc_yl = MAX((int)(((topscreen >> FRACBITS) + 1) / 10 + shift), ceilingclip);
        dc_yh = MIN((int)(((topscreen + spryscale * length) >> FRACBITS) / 10 + shift),
            floorclip);

        if (dc_yl <= dc_yh && dc_yh < viewheight)
            colfunc();

        column = (column_t *)((byte *)column + length + 4);
    }
}

//
// R_DrawParticle
//
// haleyjd: this function had to be mostly rewritten
//
void R_DrawParticle(vissprite_t *vis)
{
    int x1 = vis->x1;
    int x2 = vis->x2;
    int ox1 = vis->x1;
    int ox2 = vis->x2;
    int xcount;
    int ycount;
    int yl;
    int yh;
    int spacing;

    byte color;
    byte *dest;

    if (x1 < 0)
        x1 = 0;

    if (x2 >= viewwidth)
        x2 = viewwidth - 1;

    yl = (centeryfrac - FixedMul(vis->texturemid, vis->scale) + FRACUNIT - 1) >> FRACBITS;
    yh = yl + (x2 - x1);

    // due to square shape, it is unnecessary to clip the entire particle
    if (yh >= mfloorclip[ox1])
        yh = mfloorclip[ox1] - 1;

    if (yl <= mceilingclip[ox1])
        yl = mceilingclip[ox1] + 1;

    if (yh >= mfloorclip[ox2])
        yh = mfloorclip[ox2] - 1;

    if (yl <= mceilingclip[ox2])
        yl = mceilingclip[ox2] + 1;

    color = vis->colormap[vis->startfrac];
    xcount = x2 - x1 + 1;
    ycount = yh - yl;

    if (ycount < 0)
        return;

    ++ycount;

    spacing = SCREENWIDTH - xcount;
    dest = R_ADDRESS(0, x1, yl);

    // haleyjd 02/08/05: rewritten to remove inner loop invariants
    if (d_translucency)
    {
        // step in y
        do
        {
            int count = xcount;

            // step in x
            do
            {
                *dest = tranmap[(*dest << 8) + color];
                ++dest;

            } while (--count);

            // go to next row
            dest += spacing;

        } while (--ycount);
    }
    // opaque (fast, and looks terrible)
    else
    {
        // step in y
        do
        {
            int count = xcount;

            // step in x
            do
                *dest++ = color;
            while (--count);

            // go to next row
            dest += spacing;

        } while (--ycount);
    }
}

//
// R_DrawVisSprite
//  mfloorclip and mceilingclip should also be set.
//
void R_DrawVisSprite(vissprite_t *vis)
{
    fixed_t     frac = vis->startfrac;
    fixed_t     xiscale = vis->xiscale;
    fixed_t     x2 = vis->x2;
    patch_t     *patch = W_CacheLumpNum(vis->patch + firstspritelump, PU_CACHE);

    if (vis->patch == -1)
    {
        // this vissprite belongs to a particle
        R_DrawParticle(vis);

        return;
    }

    dc_colormap = vis->colormap;
    colfunc = vis->colfunc;

    if (!dc_colormap ||
       (!d_translucency && (vis->mobjflags & MF_TRANSLUCENT) && (vis->mobjflags & MF_COUNTKILL)))
    {
        // NULL colormap = shadow draw
        colfunc = fuzzcolfunc;
    }
    // [crispy] translucent sprites
    else if (d_translucency && (vis->mobjflags & MF_TRANSLUCENT))
    {
        colfunc = tlcolfunc;
    }

    dc_iscale = ABS(vis->xiscale) >> (!hires);
    dc_texturemid = vis->texturemid;

    if (vis->mobjflags & MF_TRANSLATION)
    {
        colfunc = transcolfunc;
        dc_translation = translationtables - 256
            + ((vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT - 8));
    }
    // [crispy] color-translated sprites (i.e. blood)
    else if (vis->translation)
    {
        colfunc = transcolfunc;
        dc_translation = vis->translation;
    }

    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);

    if (viewplayer->fixedcolormap == INVERSECOLORMAP && d_translucency)
    {
        if (colfunc == tlcolfunc)
            colfunc = tl50colfunc;
        else if (colfunc == tlredcolfunc)
            colfunc = tlred33colfunc;
        else if (colfunc == tlgreencolfunc)
            colfunc = tlgreen33colfunc;
        else if (colfunc == tlbluecolfunc)
            colfunc = tlblue33colfunc;
        else if (colfunc == tlredwhitecolfunc1 || colfunc == tlredwhitecolfunc2)
            colfunc = tlredwhite50colfunc;
    }

    // check to see if weapon is a vissprite (required for freelook)
    if (vis->psprite)
    {
        dc_texturemid += FixedMul(((centery - viewheight / 2) << FRACBITS),
                                  vis->xiscale);
        sprtopscreen += (viewheight / 2 - centery) << FRACBITS;
    }

    if (vis->footclip)
        dc_baseclip = ((int)sprtopscreen + FixedMul(SHORT(patch->height) << FRACBITS, spryscale)
            - FixedMul(vis->footclip, spryscale)) >> FRACBITS;
    else
        dc_baseclip = -1;

    fuzzpos = 0;

    for (dc_x = vis->x1; dc_x <= x2; dc_x++, frac += xiscale)
    {
        column_t    *column;
        int         texturecolumn = frac >> FRACBITS;

#ifdef RANGECHECK
        if (texturecolumn < 0 || texturecolumn >= SHORT(patch->width))
        {
            static dboolean error = false;

            // [crispy] make non-fatal
            if (!error)
            {
                C_Warning("R_DrawSpriteRange: bad texturecolumn");
                error = true;
            }

            continue;
        }
#endif

        column = (column_t *)((byte *)patch +
                               LONG(patch->columnofs[texturecolumn]));

        R_DrawMaskedSpriteColumn(column);
    }

    colfunc = basecolfunc;
}

void R_DrawBloodSplatVisSprite(vissprite_t *vis)
{
    fixed_t     frac = vis->startfrac;
    fixed_t     xiscale = vis->xiscale;
    fixed_t     x2 = vis->x2;
    patch_t     *patch = W_CacheLumpNum(vis->patch + firstspritelump, PU_CACHE);

    colfunc = vis->colfunc;

    dc_blood = tinttab75 + (vis->colormap[vis->blood] << 8);

    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(vis->texturemid, spryscale);

    fuzzpos = 0;

    for (dc_x = vis->x1; dc_x <= x2; dc_x++, frac += xiscale)
        R_DrawMaskedBloodSplatColumn((column_t *)((byte *)patch
            + LONG(patch->columnofs[frac >> FRACBITS])));

    colfunc = basecolfunc;
}

void R_DrawShadowVisSprite(vissprite_t *vis)
{
    fixed_t     frac = vis->startfrac;
    fixed_t     xiscale = vis->xiscale;
    fixed_t     x2 = vis->x2;
    patch_t     *patch = W_CacheLumpNum(vis->patch + firstspritelump, PU_CACHE);

    colfunc = vis->colfunc;

    spryscale = vis->scale;
    sprtopscreen = centeryfrac - FixedMul(vis->texturemid, spryscale);
    shift = (sprtopscreen * 9 / 10) >> FRACBITS;

    for (dc_x = vis->x1; dc_x <= x2; dc_x++, frac += xiscale)
        R_DrawMaskedShadowColumn((column_t *)((byte *)patch
            + LONG(patch->columnofs[frac >> FRACBITS])));

    colfunc = basecolfunc;
}

//
// R_ProjectSprite
// Generates a vissprite for a thing
//  if it might be visible.
//
void R_ProjectSprite(mobj_t *thing)
{
    fixed_t             tx;

    fixed_t             xscale;

    int                 x1;
    int                 x2;

    spritedef_t         *sprdef;
    spriteframe_t       *sprframe;
    int                 lump;

    dboolean            flip;

    vissprite_t         *vis;

    int                 heightsec;

    int                 flags = thing->flags;
    int                 flags2 = thing->flags2;
    int                 frame = thing->frame;
    int                 type = thing->type;

    // transform the origin point
    fixed_t             tr_x;
    fixed_t             tr_y;

    fixed_t             gxt;
    fixed_t             gyt;
    fixed_t             gzt;

    fixed_t             tz;

    angle_t             rot = 0;

    sector_t            *sector = thing->subsector->sector;

    fixed_t             fx;
    fixed_t             fy;
    fixed_t             fz;
    fixed_t             fangle;

    fixed_t             offset;
    fixed_t             topoffset;

    // [AM] Interpolate between current and last position, if prudent.
    if (d_uncappedframerate
        // Don't interpolate if the mobj did something
        // that would necessitate turning it off for a tic.
        && thing->interp
        // Don't interpolate during a paused state.
        && !paused && !menuactive && !consoleactive)
    {
        fx = thing->oldx + FixedMul(thing->x - thing->oldx, fractionaltic);
        fy = thing->oldy + FixedMul(thing->y - thing->oldy, fractionaltic);
        fz = thing->oldz + FixedMul(thing->z - thing->oldz, fractionaltic);
        fangle = R_InterpolateAngle(thing->oldangle, thing->angle, fractionaltic);
    }
    else
    {
        fx = thing->x;
        fy = thing->y;
        fz = thing->z;
        fangle = thing->angle;
    }

    tr_x = fx - viewx;
    tr_y = fy - viewy;

    gxt = FixedMul(tr_x, viewcos);
    gyt = -FixedMul(tr_y, viewsin);

    tz = gxt - gyt;

    // thing is behind view plane?
    if (tz < MINZ)
        return;

    xscale = FixedDiv(projection, tz);

    gxt = -FixedMul(tr_x, viewsin);
    gyt = FixedMul(tr_y, viewcos);
    tx = -(gyt + gxt);

    // too far off the side?
    if (ABS(tx) > (tz << 2))
        return;

    // decide which patch to use for sprite relative to player

#ifdef RANGECHECK
    if ((unsigned int)thing->sprite >= (unsigned int)numsprites)
        I_Error("R_ProjectSprite: invalid sprite number %i ",
                 thing->sprite);
#endif

    sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
    if ((thing->frame & FF_FRAMEMASK) >= sprdef->numframes)
        I_Error("R_ProjectSprite: invalid sprite frame %i : %i ",
                 thing->sprite, thing->frame);
#endif

    sprframe = &sprdef->spriteframes[frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
        // choose a different rotation based on player view
        angle_t ang = R_PointToAngle(fx, fy);

        if (sprframe->lump[0] == sprframe->lump[1])
            rot = (ang - fangle + (angle_t)(ANG45 / 2) * 9) >> 28;
        else
            rot = (ang - fangle + (angle_t)(ANG45 / 2) * 9 - (angle_t)(ANG180 / 16)) >> 28;

        lump = sprframe->lump[rot];
        flip = (!!(sprframe->flip & (1 << rot)) || (flags2 & MF2_MIRRORED));
    }
    else
    {
        // use single rotation for all views
        lump = sprframe->lump[0];
        flip = (!!(sprframe->flip & 1) || (flags2 & MF2_MIRRORED));
    }

    if (thing->state->dehacked)
    {
        offset = spriteoffset[lump];
        topoffset = spritetopoffset[lump];
    }
    else
    {
        offset = newspriteoffset[lump];
        topoffset = newspritetopoffset[lump];
    }

    // calculate edges of the shape
    tx -= (flip ? spritewidth[lump] - offset : offset);
    x1 = (centerxfrac + FRACUNIT / 2 + FixedMul(tx, xscale)) >> FRACBITS;

    // off the right side?
    if (x1 > viewwidth)
        return;

    x2 = ((centerxfrac + FRACUNIT / 2 + FixedMul(tx + spritewidth[lump], xscale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    gzt = fz + topoffset;

    if (fz > viewz + FixedDiv(viewheight << FRACBITS, xscale)
        || gzt < viewz - FixedDiv((viewheight << FRACBITS) - viewheight, xscale))
        return;

    // killough 3/27/98: exclude things totally separated
    // from the viewer, by either water or fake ceilings
    // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
    heightsec = sector->heightsec;

    // only clip things which are in special sectors
    if (heightsec != -1)
    {
        int     phs = viewplayer->mo->subsector->sector->heightsec;

        if (phs != -1 && viewz < sectors[phs].floorheight ?
            thing->z >= sectors[heightsec].floorheight :
            gzt < sectors[heightsec].floorheight)
            return;

        if (phs != -1 && viewz > sectors[phs].ceilingheight ?
            gzt < sectors[heightsec].ceilingheight &&
            viewz >= sectors[heightsec].ceilingheight :
            thing->z >= sectors[heightsec].ceilingheight)
            return;
    }

    // store information in a vissprite
    if (!(vis = R_NewVisSprite(xscale)))
        return;

    // killough 3/27/98: save sector for special clipping later
    vis->heightsec = heightsec;

    // [crispy] no color translation
    vis->translation = NULL;

    vis->mobjflags = flags;
    vis->mobjflags2 = flags2;
    vis->psprite = false;
    vis->type = type;
    vis->scale = xscale << (!hires);
    vis->gx = fx;
    vis->gy = fy;
    vis->gz = fz;
    vis->gzt = gzt;
    vis->blood = thing->blood;

    if ((flags & MF_SHADOW) && (menuactive || paused || consoleactive))
        vis->colfunc = R_DrawPausedFuzzColumn;
    else
        vis->colfunc = thing->colfunc;

    // foot clipping
    if ((flags2 & MF2_FEETARECLIPPED) && fz <= sector->interpfloorheight + FRACUNIT
        && heightsec == -1 && d_footclip)
    {
        fixed_t clipfeet = MIN((spriteheight[lump] >> FRACBITS) / 4, 10) << FRACBITS;

        vis->texturemid = gzt - viewz - clipfeet;

        if (d_swirl && isliquid[sector->floorpic]) 
            clipfeet += animatedliquiddiff;

        vis->footclip = clipfeet;
    }
    else
    {
        vis->footclip = 0;
        vis->texturemid = gzt - viewz;
    }

    vis->x1 = MAX(0, x1);
    vis->x2 = MIN(x2, viewwidth - 1);

    // [crispy] flip death sprites and corpses randomly
    // except for Cyberdemons and Barrels which are too asymmetrical
    if (((thing->type != MT_CYBORG && thing->type != MT_BARREL &&
        thing->flags & MF_CORPSE) || (thing->info->spawnstate == S_PLAY_DIE7 ||
        thing->info->spawnstate == S_PLAY_XDIE9)) && thing->health & 1)
    {
        flip = !!d_flipcorpses;
    }

    if (flip)
    {
        vis->startfrac = spritewidth[lump] - 1;
        vis->xiscale = -FixedDiv(FRACUNIT, xscale);
    }
    else
    {
        vis->startfrac = 0;
        vis->xiscale = FixedDiv(FRACUNIT, xscale);
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);

    vis->patch = lump;

    // get light level
    // [crispy] do not invalidate colormap if invisibility is rendered translucently
    if ((thing->flags & MF_SHADOW) && !d_translucency)
    {
        // shadow draw
        vis->colormap = NULL;
    }
    else if (fixedcolormap)
        // fixed map
        vis->colormap = fixedcolormap;
    else if ((frame & FF_FULLBRIGHT) && (rot <= 3 || rot >= 7))
        // full bright
        vis->colormap = fullcolormap;
    else
        // diminished light
        vis->colormap = spritelights[BETWEEN(0, xscale >> LIGHTSCALESHIFT, MAXLIGHTSCALE - 1)];

    // [crispy] colored blood
    if (d_colblood && d_chkblood && thing->target &&
       (thing->sprite == SPR_POL5 || thing->sprite == SPR_BLD2 ||
        thing->sprite == SPR_SPRY || thing->sprite == SPR_BLUD))
    {
        // [crispy] Thorn Things in Hacx bleed green blood
        if (gamemission == pack_hacx)
        {
            if (thing->target->type == MT_BABY)
            {
                vis->translation = crx[CRX_GREEN];
            }
        }
        else
        {
            // [crispy] Barons of Hell and Hell Knights bleed green blood
            if (thing->target->type == MT_BRUISER ||
                thing->target->type == MT_BETABRUISER ||
                thing->target->type == MT_KNIGHT)
            {
                vis->translation = crx[CRX_GREEN];
            }
            // [crispy] Cacodemons bleed blue blood
            else if (thing->target->type == MT_HEAD ||
                     thing->target->type == MT_BETAHEAD)
            {
                vis->translation = crx[CRX_BLUE];
            }
        }
    }
}

void R_ProjectBloodSplat(mobj_t *thing)
{
    fixed_t             tx;

    fixed_t             xscale;

    int                 x1;
    int                 x2;

    int                 lump;

    spritedef_t         *sprdef;

    vissprite_t         *vis;

    fixed_t             fx = thing->x;
    fixed_t             fy = thing->y;
    fixed_t             fz;

    fixed_t             width;

    // transform the origin point
    fixed_t             tr_x = fx - viewx;
    fixed_t             tr_y = fy - viewy;

    fixed_t             gxt = FixedMul(tr_x, viewcos);
    fixed_t             gyt = -FixedMul(tr_y, viewsin);

    fixed_t             tz = gxt - gyt;

    // thing is behind view plane?
    if (tz < MINZ)
        return;

    xscale = FixedDiv(projection, tz);

    if (xscale < FRACUNIT / 2)
        return;

    gxt = -FixedMul(tr_x, viewsin);
    gyt = FixedMul(tr_y, viewcos);
    tx = -(gyt + gxt);

    // too far off the side?
    if (ABS(tx) > (tz << 2))
        return;

#ifdef RANGECHECK
    if ((unsigned int)thing->sprite >= (unsigned int)numsprites)
        I_Error("R_ProjectBloodsplat: invalid sprite number %i ",
                 thing->sprite);
#endif

    sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
    if ((thing->frame & FF_FRAMEMASK) >= sprdef->numframes)
        I_Error("R_ProjectBloodsplat: invalid sprite frame %i : %i ",
                 thing->sprite, thing->frame);
#endif

    // decide which patch to use for sprite relative to player
    lump = sprites[SPR_BLD2].spriteframes[thing->frame].lump[0];
    width = spritewidth[lump];

    // calculate edges of the shape
    tx -= (width >> 1);
    x1 = (centerxfrac + FRACUNIT / 2 + FixedMul(tx, xscale)) >> FRACBITS;

    // off the right side?
    if (x1 > viewwidth)
        return;

    x2 = ((centerxfrac + FRACUNIT / 2 + FixedMul(tx + width, xscale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    // store information in a vissprite
    vis = &bloodvissprites[num_bloodvissprite++];

    vis->psprite = false;
    vis->type = MT_BLOODSPLAT;
    vis->scale = xscale;
    vis->gx = fx;
    vis->gy = fy;
    fz = thing->subsector->sector->interpfloorheight;
    vis->gz = fz;
    vis->gzt = fz + 1;
    vis->blood = thing->blood;

    if ((thing->flags & MF_SHADOW) && (menuactive || paused || consoleactive))
        vis->colfunc = R_DrawPausedFuzzColumn;
    else
        vis->colfunc = thing->colfunc;

    vis->texturemid = fz + 1 - viewz;

    vis->x1 = MAX(0, x1);
    vis->x2 = MIN(x2, viewwidth - 1);

    vis->startfrac = 0;
    vis->xiscale = FixedDiv(FRACUNIT, xscale);

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);

    vis->patch = lump;

    // get light level
    if (fixedcolormap)
        // fixed map
        vis->colormap = fixedcolormap;
    else
        // diminished light
        vis->colormap = spritelights[BETWEEN(0, xscale >> LIGHTSCALESHIFT, MAXLIGHTSCALE - 1)];
}

void R_ProjectShadow(mobj_t *thing)
{
    fixed_t             tx;

    fixed_t             xscale;

    int                 x1;
    int                 x2;

    spritedef_t         *sprdef;
    spriteframe_t       *sprframe;
    int                 lump;

    dboolean            flip;

    vissprite_t         *vis;

    fixed_t             fx = thing->x;
    fixed_t             fy = thing->y;
    fixed_t             fz = thing->subsector->sector->interpfloorheight
                             + thing->shadow->info->shadowoffset;

    // transform the origin point
    fixed_t             tr_x = fx - viewx;
    fixed_t             tr_y = fy - viewy;

    fixed_t             gxt = FixedMul(tr_x, viewcos);
    fixed_t             gyt = -FixedMul(tr_y, viewsin);

    fixed_t             tz = gxt - gyt;

    // thing is behind view plane?
    if (tz < MINZ)
        return;

    xscale = FixedDiv(projection, tz);

    if (xscale < FRACUNIT / 2)
        return;

    gxt = -FixedMul(tr_x, viewsin);
    gyt = FixedMul(tr_y, viewcos);
    tx = -(gyt + gxt);

    // too far off the side?
    if (ABS(tx) > (tz << 2))
        return;

    // decide which patch to use for sprite relative to player

#ifdef RANGECHECK
    if ((unsigned int)thing->sprite >= (unsigned int)numsprites)
        I_Error("R_ProjectShadow: invalid sprite number %i ",
                 thing->sprite);
#endif

    sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
    if ((thing->frame & FF_FRAMEMASK) >= sprdef->numframes)
        I_Error("R_ProjectShadow: invalid sprite frame %i : %i ",
                 thing->sprite, thing->frame);
#endif

    sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
        // choose a different rotation based on player view
        angle_t rot;
        angle_t ang = R_PointToAngle(fx, fy);

        if (sprframe->lump[0] == sprframe->lump[1])
            rot = (ang - thing->angle + (angle_t)(ANG45 / 2) * 9) >> 28;
        else
            rot = (ang - thing->angle + (angle_t)(ANG45 / 2) * 9 - (angle_t)(ANG180 / 16)) >> 28;

        lump = sprframe->lump[rot];
        flip = (!!(sprframe->flip & (1 << rot)) || (thing->flags2 & MF2_MIRRORED));
    }
    else
    {
        // use single rotation for all views
        lump = sprframe->lump[0];
        flip = (!!(sprframe->flip & 1) || (thing->flags2 & MF2_MIRRORED));
    }

    // calculate edges of the shape
    tx -= (flip ? spritewidth[lump] - newspriteoffset[lump] : newspriteoffset[lump]);
    x1 = (centerxfrac + FRACUNIT / 2 + FixedMul(tx, xscale)) >> FRACBITS;

    // off the right side?
    if (x1 > viewwidth)
        return;

    x2 = ((centerxfrac + FRACUNIT / 2 + FixedMul(tx + spritewidth[lump], xscale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    // store information in a vissprite
    vis = &shadowvissprites[num_shadowvissprite++];

    vis->mobjflags = 0;
    vis->mobjflags2 = 0;
    vis->type = MT_SHADOW;
    vis->scale = xscale<<(!hires);
    vis->gx = fx;
    vis->gy = fy;
    vis->gz = fz;
    vis->gzt = fz;
    vis->colfunc = thing->colfunc;
    vis->texturemid = fz - viewz;

    vis->x1 = MAX(0, x1);
    vis->x2 = MIN(x2, viewwidth - 1);

    if (flip)
    {
        vis->startfrac = spritewidth[lump] - 1;
        vis->xiscale = -FixedDiv(FRACUNIT, xscale);
    }
    else
    {
        vis->startfrac = 0;
        vis->xiscale = FixedDiv(FRACUNIT, xscale);
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);

    vis->patch = lump;
}

//
// R_ProjectParticle
//
void R_ProjectParticle(particle_t *particle)
{
    int x1;
    int x2;
    int heightsec = -1;
    int globalxscale = (SCREENWIDTH << FRACBITS) / SCREENWIDTH;
    int addscaleshift = (globalxscale >> FRACBITS) - 1;

    vissprite_t  *vis;

    sector_t  *sector = NULL;

    fixed_t gzt;
    fixed_t tx;
    fixed_t xscale;
    fixed_t iscale;

    // transform the origin point
    fixed_t tr_x = particle->x - viewx;
    fixed_t tr_y = particle->y - viewy;

    fixed_t gxt = FixedMul(tr_x, viewcos);
    fixed_t gyt = -FixedMul(tr_y, viewsin);
    fixed_t tz = gxt - gyt;

    // particle is behind view plane?
    if (tz < MINZ)
        return;
   
    xscale = FixedDiv(projection, tz);
   
    gxt = -FixedMul(tr_x, viewsin); 
    gyt = FixedMul(tr_y, viewcos); 

    tx = -(gyt + gxt); 
   
    // too far off the side?
    if (ABS(tx) > (tz << 2))
        return;
   
    // calculate edges of the shape
    x1 = (centerxfrac + FixedMul(tx, xscale)) >> FRACBITS;
   
    // off the right side?
    if (x1 >= viewwidth)
        return;
   
    x2 = ((centerxfrac + FixedMul(tx + particle->size * (FRACUNIT / 4), xscale)) >> FRACBITS);
   
    // off the left side?
    if (x2 < 0)
        return;
   
    gzt = particle->z + 1;
   
    // killough 3/27/98: exclude things totally separated
    // from the viewer, by either water or fake ceilings
    // killough 4/11/98: improve sprite clipping for underwater/fake ceilings   
    {
        // haleyjd 02/20/04: use subsector now stored in particle
        subsector_t *subsector = particle->subsector;
        sector = subsector->sector;
        heightsec = sector->heightsec;

        if (particle->z < sector->floorheight || particle->z > sector->ceilingheight)
            return;
    }
   
    // only clip particles which are in special sectors
    if (heightsec != -1)
    {
        int phs = viewplayer->mo->subsector->sector->heightsec;
      
        if (phs != -1 && 
            viewz < sectors[phs].floorheight ?
                    particle->z >= sectors[heightsec].floorheight :
                    gzt < sectors[heightsec].floorheight)
            return;

        if (phs != -1 && 
            viewz > sectors[phs].ceilingheight ?
                    gzt < sectors[heightsec].ceilingheight &&
                      viewz >= sectors[heightsec].ceilingheight :
                    particle->z >= sectors[heightsec].ceilingheight)
            return;
    }
   
    // store information in a vissprite
    if (!(vis = R_NewVisSprite(xscale)))
        return;

    vis->heightsec = heightsec;
    vis->scale = xscale;
    vis->gx = particle->x;
    vis->gy = particle->y;
    vis->gz = particle->z;
    vis->gzt = gzt;
    vis->texturemid = vis->gzt - viewz;
    vis->x1 = x1 < 0 ? 0 : x1;
    vis->x2 = x2 >= viewwidth ? viewwidth - 1 : x2;
    iscale = FixedDiv(FRACUNIT, xscale);
    vis->startfrac = particle->color;
    vis->xiscale = iscale;
    vis->patch = -1;
    vis->mobjflags = particle->trans;
   
    if (fixedcolormap ==
        fullcolormap + INVERSECOLORMAP * 256 * sizeof(lighttable_t))
    {
        vis->colormap = fixedcolormap;
    } 
    else
    {
        // haleyjd 01/12/02: wow is this code wrong! :)
        //int index = xscale >> (LIGHTSCALESHIFT + hires);
        //if (index >= MAXLIGHTSCALE) 
        //    index = MAXLIGHTSCALE - 1;      
        //vis->colormap = spritelights[index];

        //R_SectorColormap(sector);

        if (d_brightmaps && (particle->styleflags & PS_FULLBRIGHT))
        {
            vis->colormap = fullcolormap;
        }
        else
        {
            lighttable_t **ltable;
            sector_t tmpsec;
            int floorlightlevel, ceilinglightlevel, lightnum, index;

            R_FakeFlat(sector, &tmpsec, &floorlightlevel, &ceilinglightlevel, false);

            lightnum = (floorlightlevel + ceilinglightlevel) / 2;
            lightnum = (lightnum >> LIGHTSEGSHIFT) + extralight;
         
            if (lightnum >= LIGHTLEVELS || fixedcolormap)
                ltable = scalelight[LIGHTLEVELS - 1];      
            else if (lightnum < 0)
                ltable = scalelight[0];
            else
                ltable = scalelight[lightnum];
         
            // SoM: ANYRES
            index = xscale >> (LIGHTSCALESHIFT + addscaleshift);

            if (index >= MAXLIGHTSCALE)
                index = MAXLIGHTSCALE - 1;
         
            vis->colormap = ltable[index];
        }
    }
}

//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
// killough 9/18/98: add lightlevel as parameter, fixing underwater lighting
void R_AddSprites(sector_t *sec, int lightlevel)
{
    mobj_t      *thing;
    short       floorpic = sec->floorpic;

    spritelights = scalelight[BETWEEN(0, (lightlevel >> LIGHTSEGSHIFT) + extralight * LIGHTBRIGHT,
        LIGHTLEVELS - 1)];

    // Handle all things in sector.
    if (fixedcolormap || isliquid[floorpic] || floorpic == skyflatnum || !d_shadows)
        for (thing = sec->thinglist; thing; thing = thing->snext)
        {
            if (thing->type != MT_SHADOW)
                thing->projectfunc(thing);
        }
    else
        for (thing = sec->thinglist; thing; thing = thing->snext)
            thing->projectfunc(thing);

    // haleyjd 02/20/04: Handle all particles in sector.
    if (d_drawparticles)
    {
        particle_t  *ptcl;

        for (ptcl = sec->ptcllist; ptcl; ptcl = (particle_t *)(ptcl->seclinks.next))
            R_ProjectParticle(ptcl);
    }
}

//
// R_DrawPSprite
//
static void R_DrawPSprite(pspdef_t *psp, dboolean invisibility)
{
    fixed_t             tx;
    int                 x1, x2;
    spritenum_t         spr;
    spritedef_t         *sprdef;
    long                frame;
    spriteframe_t       *sprframe;
    int                 lump;
    dboolean            flip;
    vissprite_t         *vis;
    vissprite_t         avis;
    state_t             *state;
    dboolean            dehacked = weaponinfo[viewplayer->readyweapon].dehacked;

    // decide which patch to use
    state = psp->state;
    spr = state->sprite;

#ifdef RANGECHECK
    if ((unsigned)psp->state->sprite >= (unsigned int)numsprites)
        I_Error("R_DrawPSprite: invalid sprite number %i ",
                 psp->state->sprite);
#endif

    sprdef = &sprites[spr];
    frame = state->frame;

#ifdef RANGECHECK
    if ((psp->state->frame & FF_FRAMEMASK) >= sprdef->numframes)
        I_Error("R_DrawPSprite: invalid sprite frame %i : %i ",
                 psp->state->sprite, psp->state->frame);
#endif

    sprframe = &sprdef->spriteframes[frame & FF_FRAMEMASK];

    lump = sprframe->lump[0];
    flip = !!(sprframe->flip & 1);

    // calculate edges of the shape
    tx = psp->sx - (ORIGINALWIDTH / 2) * FRACUNIT - (dehacked ? spriteoffset[lump] :
        newspriteoffset[lump]);
    x1 = (centerxfrac + FixedMul(tx, pspritescale)) >> FRACBITS;

    // off the right side
    if (x1 > viewwidth)
        return;

    tx += spritewidth[lump];
    x2 = ((centerxfrac + FixedMul(tx, pspritescale)) >> FRACBITS) - 1;

    // off the left side
    if (x2 < 0)
        return;

    // store information in a vissprite
    vis = &avis;

    // [crispy] no color translation
    vis->translation = NULL;

    vis->mobjflags = 0;
    vis->mobjflags2 = 0;
    vis->psprite = true;
    vis->texturemid = (BASEYCENTER << FRACBITS) + FRACUNIT / 4 - (psp->sy - spritetopoffset[lump]);
    vis->x1 = MAX(0, x1);
    vis->x2 = MIN(x2, viewwidth - 1);
    vis->scale = pspritescale << (!hires);
    vis->blood = 0;
    vis->footclip = 0;

    if (flip)
    {
        vis->xiscale = -pspriteiscale;
        vis->startfrac = spritewidth[lump] - 1;
    }
    else
    {
        vis->xiscale = pspriteiscale;
        vis->startfrac = 0;
    }

    if (vis->x1 > x1)
        vis->startfrac += vis->xiscale * (vis->x1 - x1);

    vis->patch = lump;

    // [crispy] do not invalidate colormap if invisibility is rendered translucently
    if ((viewplayer->powers[pw_invisibility] > 4 * 32
        || (viewplayer->powers[pw_invisibility] & 8))
        && !d_translucency && !beta_style)
    {
        // shadow draw
        vis->colfunc = fuzzcolfunc;
        vis->colormap = NULL;
    }
    else
    {
        if (spr == SPR_SHT2 && (!frame || frame >= 8) && !dehacked) 
            vis->colfunc = (d_translucency ? R_DrawTranslucentSuperShotgunColumn :
                R_DrawSuperShotgunColumn);
        else
        {
            if (d_translucency)
            {
                void (*colfuncs[])(void) =
                {
                    // n/a
                    NULL,

                    // SPR_SHTG
                    basecolfunc,

                    // SPR_PUNG
                    basecolfunc,

                    // SPR_PISG
                    basecolfunc,

                    // SPR_PISF
                    tlcolfunc,

                    // SPR_SHTF
                    tlcolfunc,

                    // SPR_SHT2
                    tlredwhitecolfunc1,

                    // SPR_CHGG
                    basecolfunc,

                    // SPR_CHGF
                    tlredwhitecolfunc2,

                    // SPR_MISG
                    basecolfunc,

                    // SPR_MISF
                    tlredwhitecolfunc1,

                    // SPR_SAWG
                    basecolfunc,

                    // SPR_PLSG
                    basecolfunc,

                    // SPR_PLSF
                    tlcolfunc,

                    // SPR_BFGG
                    basecolfunc,

                    // SPR_BFGF
                    tlcolfunc
                };

                vis->colfunc = (bflash && spr <= SPR_BFGF && (!dehacked || state->translucent) ?
                    colfuncs[spr] : basecolfunc);
            }
            else
            {
                void (*colfuncs[])(void) =
                {
                    // n/a
                    NULL,

                    // SPR_SHTG
                    basecolfunc,

                    // SPR_PUNG
                    basecolfunc,

                    // SPR_PISG
                    basecolfunc,

                    // SPR_PISF
                    basecolfunc,

                    // SPR_SHTF
                    basecolfunc,

                    // SPR_SHT2
                    basecolfunc,

                    // SPR_CHGG
                    basecolfunc,

                    // SPR_CHGF
                    basecolfunc,

                    // SPR_MISG
                    basecolfunc,

                    // SPR_MISF
                    basecolfunc,

                    // SPR_SAWG
                    basecolfunc,

                    // SPR_PLSG
                    basecolfunc,

                    // SPR_PLSF
                    basecolfunc,

                    // SPR_BFGG
                    basecolfunc,

                    // SPR_BFGF
                    basecolfunc
                };

                vis->colfunc = (bflash && spr <= SPR_BFGF && (!dehacked || state->translucent) ?
                    colfuncs[spr] : basecolfunc);
            }
        }

        if (fixedcolormap)
            // fixed color
            vis->colormap = fixedcolormap;
        else
        {
            if (bflash || (frame & FF_FULLBRIGHT))
                // full bright
                vis->colormap = fullcolormap;
            else if (d_altlighting)
            {
                int lightnum = (viewplayer->mo->subsector->sector->lightlevel >> OLDLIGHTSEGSHIFT)
                    + extralight * OLDLIGHTBRIGHT;

                vis->colormap = psprscalelight[BETWEEN(0, lightnum, OLDLIGHTLEVELS - 1)]
                    [BETWEEN(0, lightnum + 16, OLDMAXLIGHTSCALE - 1)];
            }
            else
            {
                int lightnum = (viewplayer->mo->subsector->sector->lightlevel >> LIGHTSEGSHIFT)
                    + extralight * LIGHTBRIGHT;

                vis->colormap = scalelight[BETWEEN(0, lightnum, LIGHTLEVELS - 1)]
                    [BETWEEN(0, lightnum + 8, MAXLIGHTSCALE - 1)];
            }
        }
    }

    // [crispy] invisibility is rendered translucently
    if ((viewplayer->powers[pw_invisibility] > 4 * 32 ||
        (viewplayer->powers[pw_invisibility] & 8)) &&
        d_translucency)
    {
        vis->mobjflags |= MF_TRANSLUCENT;
    }

    // e6y: interpolation for weapon bobbing
    if (d_uncappedframerate)
    {
        typedef struct interpolate_s
        {
            int x1;
            int x1_prev;
            int texturemid;
            int texturemid_prev;
            int lump;
        } psp_interpolate_t;

        static psp_interpolate_t        psp_inter;

        if (realframe)
        {
            psp_inter.x1 = psp_inter.x1_prev;
            psp_inter.texturemid = psp_inter.texturemid_prev;
        }

        psp_inter.x1_prev = vis->x1;
        psp_inter.texturemid_prev = vis->texturemid;

        if (lump == psp_inter.lump && !skippsprinterp)
        {
            int deltax = vis->x2 - vis->x1;

            vis->x1 = psp_inter.x1 + FixedMul(fractionaltic, vis->x1 - psp_inter.x1);
            vis->x2 = vis->x1 + deltax;
            vis->texturemid = psp_inter.texturemid
                + FixedMul(fractionaltic, vis->texturemid - psp_inter.texturemid);
        }
        else
        {
            skippsprinterp = false;
            psp_inter.x1 = vis->x1;
            psp_inter.texturemid = vis->texturemid;
            psp_inter.lump = lump;
        }
    }

    R_DrawVisSprite(vis);
}

//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites(void)
{
    int         i;
    int         invisibility = viewplayer->powers[pw_invisibility];
    pspdef_t    *psp;

    // clip to screen bounds
    mfloorclip = screenheightarray;
    mceilingclip = negonearray;

    // add all active psprites
    if (invisibility > 128 || (invisibility & 8))
    {
        for (i = 0, psp = viewplayer->psprites; i < NUMPSPRITES; i++, psp++)
            if (psp->state)
                R_DrawPSprite(psp, true);
    }
    else
    {
        bflash = false;

        for (i = 0, psp = viewplayer->psprites; i < NUMPSPRITES; i++, psp++)
            if (psp->state && (psp->state->frame & FF_FULLBRIGHT))
            {
                bflash = true;
                break;
            }

        for (i = 0, psp = viewplayer->psprites; i < NUMPSPRITES; i++, psp++)
            if (psp->state)
                R_DrawPSprite(psp, false);
    }
}

//
// R_DrawBloodSprite
//
static void R_DrawBloodSprite(vissprite_t *spr)
{
    drawseg_t   *ds;
    int         clipbot[SCREENWIDTH];
    int         cliptop[SCREENWIDTH];
    int         x;
    int         x1 = spr->x1;
    int         x2 = spr->x2;

    // [RH] Quickly reject sprites with bad x ranges.
    if (x1 > x2)
        return;

    for (x = x1; x <= x2; x++)
        clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (ds = ds_p; ds-- > drawsegs;)
    {
        int     r1;
        int     r2;

        // determine if the drawseg obscures the sprite
        if (ds->x1 > x2 || ds->x2 < x1 || (!ds->silhouette && !ds->maskedtexturecol))
            // does not cover sprite
            continue;

        if (MAX(ds->scale1, ds->scale2) < spr->scale
            || (MIN(ds->scale1, ds->scale2) < spr->scale
            && !R_PointOnSegSide(spr->gx, spr->gy, ds->curline)))
            // seg is behind sprite
            continue;

        r1 = MAX(ds->x1, x1);
        r2 = MIN(ds->x2, x2);

        // clip this piece of the sprite
        // killough 3/27/98: optimized and made much shorter

        // bottom sil
        if ((ds->silhouette & SIL_BOTTOM) && spr->gz < ds->bsilheight)
            for (x = r1; x <= r2; x++)
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];

        // top sil
        if ((ds->silhouette & SIL_TOP) && spr->gzt > ds->tsilheight)
            for (x = r1; x <= r2; x++)
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = x1; x <= x2; x++)
    {
        if (clipbot[x] == -2)
            clipbot[x] = viewheight;

        if (cliptop[x] == -2)
            cliptop[x] = -1;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;

    if (spr->type == MT_BLOODSPLAT)
        R_DrawBloodSplatVisSprite(spr);
    else
        R_DrawVisSprite(spr);
}

//
// R_DrawShadowSprite
//
static void R_DrawShadowSprite(vissprite_t *spr)
{
    drawseg_t   *ds;
    int         clipbot[SCREENWIDTH];
    int         cliptop[SCREENWIDTH];
    int         x;
    int         x1 = spr->x1;
    int         x2 = spr->x2;

    // [RH] Quickly reject sprites with bad x ranges.
    if (x1 > x2)
        return;

    for (x = x1; x <= x2; x++)
        clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale
    //  is the clip seg.
    for (ds = ds_p; ds-- > drawsegs;)
    {
        int     r1;
        int     r2;

        // determine if the drawseg obscures the sprite
        if (ds->x1 > x2 || ds->x2 < x1 || (!ds->silhouette && !ds->maskedtexturecol))
            // does not cover sprite
            continue;

        if (MAX(ds->scale1, ds->scale2) < spr->scale
            || (MIN(ds->scale1, ds->scale2) < spr->scale
            && !R_PointOnSegSide(spr->gx, spr->gy, ds->curline)))
            // seg is behind sprite
            continue;

        r1 = MAX(ds->x1, x1);
        r2 = MIN(ds->x2, x2);

        // clip this piece of the sprite
        // killough 3/27/98: optimized and made much shorter

        // bottom sil
        if ((ds->silhouette & SIL_BOTTOM) && spr->gz < ds->bsilheight)
            for (x = r1; x <= r2; x++)
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];

        // top sil
        if ((ds->silhouette & SIL_TOP) && spr->gzt > ds->tsilheight)
            for (x = r1; x <= r2; x++)
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = x1; x <= x2; x++)
    {
        if (clipbot[x] == -2)
            clipbot[x] = viewheight;

        if (cliptop[x] == -2)
            cliptop[x] = -1;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawShadowVisSprite(spr);
}

static void R_DrawSprite(vissprite_t *spr)
{
    drawseg_t   *ds;
    int         clipbot[SCREENWIDTH];
    int         cliptop[SCREENWIDTH];
    int         x;
    int         x1 = spr->x1;
    int         x2 = spr->x2;

    if (x1 > x2)
        return;

    for (x = x1; x <= x2; x++)
        clipbot[x] = cliptop[x] = -2;

    // Scan drawsegs from end to start for obscuring segs.
    // The first drawseg that has a greater scale is the clip seg.
    for (ds = ds_p; ds-- > drawsegs;)
    {
        int     r1;
        int     r2;

        // determine if the drawseg obscures the sprite
        if (ds->x1 > x2 || ds->x2 < x1 || (!ds->silhouette && !ds->maskedtexturecol))
            // does not cover sprite
            continue;

        if (MAX(ds->scale1, ds->scale2) < spr->scale
            || (MIN(ds->scale1, ds->scale2) < spr->scale
            && !R_PointOnSegSide(spr->gx, spr->gy, ds->curline)))
        {
            // masked mid texture?
            if (ds->maskedtexturecol)
            {
                r1 = MAX(ds->x1, x1);
                r2 = MIN(ds->x2, x2);
                R_RenderMaskedSegRange(ds, r1, r2);
            }

            // seg is behind sprite
            continue;
        }

        r1 = MAX(ds->x1, x1);
        r2 = MIN(ds->x2, x2);

        // clip this piece of the sprite
        // killough 3/27/98: optimized and made much shorter

        // bottom sil
        if ((ds->silhouette & SIL_BOTTOM) && spr->gz < ds->bsilheight)
            for (x = r1; x <= r2; x++)
                if (clipbot[x] == -2)
                    clipbot[x] = ds->sprbottomclip[x];

        // top sil
        if ((ds->silhouette & SIL_TOP) && spr->gzt > ds->tsilheight)
            for (x = r1; x <= r2; x++)
                if (cliptop[x] == -2)
                    cliptop[x] = ds->sprtopclip[x];
    }

    // killough 3/27/98:
    // Clip the sprite against deep water and/or fake ceilings.
    // killough 4/9/98: optimize by adding mh
    // killough 4/11/98: improve sprite clipping for underwater/fake ceilings
    // killough 11/98: fix disappearing sprites
    if (spr->heightsec != -1)  // only things in specially marked sectors
    {
        fixed_t     h, mh;
        int         phs = viewplayer->mo->subsector->sector->heightsec;

        if ((mh = sectors[spr->heightsec].interpfloorheight) > spr->gz
            && (h = centeryfrac - FixedMul(mh -= viewz, spr->scale)) >= 0
            && (h >>= FRACBITS) < viewheight)
        {
            if (mh <= 0 || (phs != -1 && viewz > sectors[phs].interpfloorheight))
            {
                // clip bottom
                for (x = x1; x <= x2; x++)
                    if (clipbot[x] == -2 || h < clipbot[x])
                        clipbot[x] = h;
            }
            else
                // clip top
                // killough 11/98
                if (phs != -1 && viewz <= sectors[phs].interpfloorheight)
                    for (x = x1; x <= x2; x++)
                        if (cliptop[x] == -2 || h > cliptop[x])
                            cliptop[x] = h;
        }

        if ((mh = sectors[spr->heightsec].ceilingheight) < spr->gzt
            && (h = centeryfrac - FixedMul(mh - viewz, spr->scale)) >= 0
            && (h >>= FRACBITS) < viewheight)
        {
            if (phs != -1 && viewz >= sectors[phs].ceilingheight)
            {
                // clip bottom
                for (x = x1; x <= x2; x++)
                    if (clipbot[x] == -2 || h < clipbot[x])
                        clipbot[x] = h;
            }
            else
                // clip top
                for (x = x1; x <= x2; x++)
                    if (cliptop[x] == -2 || h > cliptop[x])
                        cliptop[x] = h;
        }
    }

    // all clipping has been performed, so draw the sprite

    // check for unclipped columns
    for (x = spr->x1; x <= x2; x++)
    {
        if (clipbot[x] == -2)
            clipbot[x] = viewheight;

        if (cliptop[x] == -2)
            cliptop[x] = -1;
    }

    mfloorclip = clipbot;
    mceilingclip = cliptop;
    R_DrawVisSprite(spr);
}

//
// R_DrawMasked
//
void R_DrawMasked(void)
{
    drawseg_t   *ds;
    int         i;

    if (d_drawparticles)
    {
        int j = activeParticles;

        while (j != -1)
        {
            R_ProjectParticle(Particles + j);

            j = Particles[j].next;
        }
    }

    // draw all blood splats
    for (i = num_bloodvissprite; --i >= 0;)
        R_DrawBloodSprite(&bloodvissprites[i]);

    // draw all shadows
    for (i = num_shadowvissprite; --i >= 0;)
        R_DrawShadowSprite(&shadowvissprites[i]);

    // draw all other vissprites back to front
    for (i = num_vissprite; --i >= 0;)
        R_DrawSprite(vissprite_ptrs[i]);

    // render any remaining masked mid textures
    for (ds = ds_p; ds-- > drawsegs;)
        if (ds->maskedtexturecol)
            R_RenderMaskedSegRange(ds, ds->x1, ds->x2);

    // draw the psprites on top of everything
    R_DrawPlayerSprites();
}

//
// haleyjd 09/30/01
//
// Particle Rendering
// This incorporates itself mostly seamlessly within the
// vissprite system, incurring only minor changes to the functions
// above.

//
// newParticle
//
// Tries to find an inactive particle in the Particles list
// Returns NULL on failure
//
particle_t *newParticle(void)
{
    particle_t *result = NULL;

    if (inactiveParticles != -1)
    {
        result = Particles + inactiveParticles;
        inactiveParticles = result->next;
        result->next = activeParticles;
        activeParticles = result - Particles;
    }

    return result;
}

//
// R_ClearParticles
//
// set up the particle list
//
void R_ClearParticles(void)
{
    int i;
   
    memset(Particles, 0, numParticles * sizeof(particle_t));
    activeParticles = -1;
    inactiveParticles = 0;

    for (i = 0; i < numParticles - 1; i++)
        Particles[i].next = i + 1;

    Particles[i].next = -1;
}

//
// R_InitParticles
//
// Allocate the particle list and initialize it
//
void R_InitParticles(void)
{
    numParticles = 0;

    // assume default
    if (numParticles == 0)
        numParticles = 4000;
    else if (numParticles < 100)
        numParticles = 100;
   
    Particles = (particle_t *)(Z_Malloc(numParticles * sizeof(particle_t), PU_STATIC, NULL));
    R_ClearParticles();
}

