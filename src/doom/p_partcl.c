// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// This module, except for code marked otherwise, is covered by the 
// zdoom source distribution license, which is included in the 
// Eternity source distribution, and is compatible with the terms of
// the GNU General Public License.
//
// Copyright (c) 2016 by Ronald Lasmanowicz:
//
//   - first base taken from the Eternity Engine ("Gamma" Release v3.29 Final)
//        which was from July 2002
//
//   - then continously updated using codebases: - 3.31.10
//                                               - 3.33.02
//                                               - 3.33.33
//                                               - 3.33.50
//                                               - 3.35.90
//                                               - 3.35.92
//                                               - 3.37.00
//                                               - 3.39.20
//
//        (repository at http://eternity.mancubus.net/ee-old/ was a good help
//                as GitHub does only list Eternity's code from 2006 onwards)
//
//   - then updated once again using the latest codebase at GitHub
//   - and slightly modified to get flies for corpses and dripping blood
//             working with this port
//
// See the license file for details.
//
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//
//   Code that ties particle effects to map objects, adapted
//   from zdoom. Thanks to Randy Heit.
//
//----------------------------------------------------------------------------

#include <math.h>
#include "z_zone.h"
#include "d_main.h"
#include "doomstat.h"
#include "doomtype.h"
#include "m_random.h"
#include "p_partcl.h"
#include "p_setup.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"
#include "w_wad.h"
#include "p_mobj.h"
#include "p_spec.h"
#include "v_trans.h"
#include "c_io.h"
#include "p_local.h"
#include "p_tick.h"
#include "s_sound.h"
#include "p_spec.h"

// static integers to hold particle color values
static byte grey1;
static byte grey2;
static byte grey3;
static byte grey4;
static byte red;
static byte green;
static byte blue;
static byte yellow;
static byte black;
static byte red1;
static byte green1;
static byte blue1;
static byte yellow1;
static byte purple;
static byte purple1;
static byte white;
static byte rblue1;
static byte rblue2;
static byte rblue3;
static byte rblue4;
static byte orange;
static byte yorange;
static byte dred;
static byte grey5;
static byte maroon1;
static byte maroon2;
static byte mdred;
static byte dred2;

static struct particleColorList {
   byte *color, r, g, b;
} particleColors[] = {
    { &grey1,    85,  85,  85 },
    { &grey2,   171, 171, 171 },
    { &grey3,    50,  50,  50 },
    { &grey4,   210, 210, 210 },
    { &grey5,   128, 128, 128 },
    { &red,     255,   0,   0 },
    { &green,     0, 200,   0 },
    { &blue,      0,   0, 255 },
    { &yellow,  255, 255,   0 },
    { &black,     0,   0,   0 },
    { &red1,    255, 127, 127 },
    { &green1,  127, 255, 127 },
    { &blue1,   127, 127, 255 },
    { &yellow1, 255, 255, 180 },
    { &purple,  120,   0, 160 },
    { &purple1, 200,  30, 255 },
    { &white,   255, 255, 255 },
    { &rblue1,   81,  81, 255 },
    { &rblue2,    0,   0, 227 },
    { &rblue3,    0,   0, 130 },
    { &rblue4,    0,   0,  80 },
    { &orange,  255, 120,   0 },
    { &yorange, 255, 170,   0 },
    { &dred,     80,   0,   0 },
    { &maroon1, 154,  49,  49 },
    { &maroon2, 125,  24,  24 },
    { &mdred,   165,   0,   0 },
    { &dred2,   115,   0,   0 },
    { NULL                    }
};

//
// Begin Quake 2 particle effects data. This code is taken from Quake 2,
// copyright 1997 id Software, Inc. Available under the GNU General
// Public License.
//

#define BEAMLENGTH       16
#define NUMVERTEXNORMALS 162

typedef float vec3_t[3];

static vec3_t avelocities[NUMVERTEXNORMALS];

static vec3_t bytedirs[NUMVERTEXNORMALS] = {
    { -0.525731f, 0.000000f, 0.850651f },
    { -0.442863f, 0.238856f, 0.864188f },
    { -0.295242f, 0.000000f, 0.955423f },
    { -0.309017f, 0.500000f, 0.809017f },
    { -0.162460f, 0.262866f, 0.951056f },
    {  0.000000f, 0.000000f, 1.000000f },
    {  0.000000f, 0.850651f, 0.525731f },
    { -0.147621f, 0.716567f, 0.681718f },
    {  0.147621f, 0.716567f, 0.681718f },
    {  0.000000f, 0.525731f, 0.850651f },
    {  0.309017f, 0.500000f, 0.809017f },
    {  0.525731f, 0.000000f, 0.850651f },
    {  0.295242f, 0.000000f, 0.955423f },
    {  0.442863f, 0.238856f, 0.864188f },
    {  0.162460f, 0.262866f, 0.951056f },
    { -0.681718f, 0.147621f, 0.716567f },
    { -0.809017f, 0.309017f, 0.500000f },
    { -0.587785f, 0.425325f, 0.688191f },
    { -0.850651f, 0.525731f, 0.000000f },
    { -0.864188f, 0.442863f, 0.238856f },
    { -0.716567f, 0.681718f, 0.147621f },
    { -0.688191f, 0.587785f, 0.425325f },
    { -0.500000f, 0.809017f, 0.309017f },
    { -0.238856f, 0.864188f, 0.442863f },
    { -0.425325f, 0.688191f, 0.587785f },
    { -0.716567f, 0.681718f,-0.147621f },
    { -0.500000f, 0.809017f,-0.309017f },
    { -0.525731f, 0.850651f, 0.000000f },
    {  0.000000f, 0.850651f,-0.525731f },
    { -0.238856f, 0.864188f,-0.442863f },
    {  0.000000f, 0.955423f,-0.295242f },
    { -0.262866f, 0.951056f,-0.162460f },
    {  0.000000f, 1.000000f, 0.000000f },
    {  0.000000f, 0.955423f, 0.295242f },
    { -0.262866f, 0.951056f, 0.162460f },
    {  0.238856f, 0.864188f, 0.442863f },
    {  0.262866f, 0.951056f, 0.162460f },
    {  0.500000f, 0.809017f, 0.309017f },
    {  0.238856f, 0.864188f,-0.442863f },
    {  0.262866f, 0.951056f,-0.162460f },
    {  0.500000f, 0.809017f,-0.309017f },
    {  0.850651f, 0.525731f, 0.000000f },
    {  0.716567f, 0.681718f, 0.147621f },
    {  0.716567f, 0.681718f,-0.147621f },
    {  0.525731f, 0.850651f, 0.000000f },
    {  0.425325f, 0.688191f, 0.587785f },
    {  0.864188f, 0.442863f, 0.238856f },
    {  0.688191f, 0.587785f, 0.425325f },
    {  0.809017f, 0.309017f, 0.500000f },
    {  0.681718f, 0.147621f, 0.716567f },
    {  0.587785f, 0.425325f, 0.688191f },
    {  0.955423f, 0.295242f, 0.000000f },
    {  1.000000f, 0.000000f, 0.000000f },
    {  0.951056f, 0.162460f, 0.262866f },
    {  0.850651f,-0.525731f, 0.000000f },
    {  0.955423f,-0.295242f, 0.000000f },
    {  0.864188f,-0.442863f, 0.238856f },
    {  0.951056f,-0.162460f, 0.262866f },
    {  0.809017f,-0.309017f, 0.500000f },
    {  0.681718f,-0.147621f, 0.716567f },
    {  0.850651f, 0.000000f, 0.525731f },
    {  0.864188f, 0.442863f,-0.238856f },
    {  0.809017f, 0.309017f,-0.500000f },
    {  0.951056f, 0.162460f,-0.262866f },
    {  0.525731f, 0.000000f,-0.850651f },
    {  0.681718f, 0.147621f,-0.716567f },
    {  0.681718f,-0.147621f,-0.716567f },
    {  0.850651f, 0.000000f,-0.525731f },
    {  0.809017f,-0.309017f,-0.500000f },
    {  0.864188f,-0.442863f,-0.238856f },
    {  0.951056f,-0.162460f,-0.262866f },
    {  0.147621f, 0.716567f,-0.681718f },
    {  0.309017f, 0.500000f,-0.809017f },
    {  0.425325f, 0.688191f,-0.587785f },
    {  0.442863f, 0.238856f,-0.864188f },
    {  0.587785f, 0.425325f,-0.688191f },
    {  0.688191f, 0.587785f,-0.425325f },
    { -0.147621f, 0.716567f,-0.681718f },
    { -0.309017f, 0.500000f,-0.809017f },
    {  0.000000f, 0.525731f,-0.850651f },
    { -0.525731f, 0.000000f,-0.850651f },
    { -0.442863f, 0.238856f,-0.864188f },
    { -0.295242f, 0.000000f,-0.955423f },
    { -0.162460f, 0.262866f,-0.951056f },
    {  0.000000f, 0.000000f,-1.000000f },
    {  0.295242f, 0.000000f,-0.955423f },
    {  0.162460f, 0.262866f,-0.951056f },
    { -0.442863f,-0.238856f,-0.864188f },
    { -0.309017f,-0.500000f,-0.809017f },
    { -0.162460f,-0.262866f,-0.951056f },
    {  0.000000f,-0.850651f,-0.525731f },
    { -0.147621f,-0.716567f,-0.681718f },
    {  0.147621f,-0.716567f,-0.681718f },
    {  0.000000f,-0.525731f,-0.850651f },
    {  0.309017f,-0.500000f,-0.809017f },
    {  0.442863f,-0.238856f,-0.864188f },
    {  0.162460f,-0.262866f,-0.951056f },
    {  0.238856f,-0.864188f,-0.442863f },
    {  0.500000f,-0.809017f,-0.309017f },
    {  0.425325f,-0.688191f,-0.587785f },
    {  0.716567f,-0.681718f,-0.147621f },
    {  0.688191f,-0.587785f,-0.425325f },
    {  0.587785f,-0.425325f,-0.688191f },
    {  0.000000f,-0.955423f,-0.295242f },
    {  0.000000f,-1.000000f, 0.000000f },
    {  0.262866f,-0.951056f,-0.162460f },
    {  0.000000f,-0.850651f, 0.525731f },
    {  0.000000f,-0.955423f, 0.295242f },
    {  0.238856f,-0.864188f, 0.442863f },
    {  0.262866f,-0.951056f, 0.162460f },
    {  0.500000f,-0.809017f, 0.309017f },
    {  0.716567f,-0.681718f, 0.147621f },
    {  0.525731f,-0.850651f, 0.000000f },
    { -0.238856f,-0.864188f,-0.442863f },
    { -0.500000f,-0.809017f,-0.309017f },
    { -0.262866f,-0.951056f,-0.162460f },
    { -0.850651f,-0.525731f, 0.000000f },
    { -0.716567f,-0.681718f,-0.147621f },
    { -0.716567f,-0.681718f, 0.147621f },
    { -0.525731f,-0.850651f, 0.000000f },
    { -0.500000f,-0.809017f, 0.309017f },
    { -0.238856f,-0.864188f, 0.442863f },
    { -0.262866f,-0.951056f, 0.162460f },
    { -0.864188f,-0.442863f, 0.238856f },
    { -0.809017f,-0.309017f, 0.500000f },
    { -0.688191f,-0.587785f, 0.425325f },
    { -0.681718f,-0.147621f, 0.716567f },
    { -0.442863f,-0.238856f, 0.864188f },
    { -0.587785f,-0.425325f, 0.688191f },
    { -0.309017f,-0.500000f, 0.809017f },
    { -0.147621f,-0.716567f, 0.681718f },
    { -0.425325f,-0.688191f, 0.587785f },
    { -0.162460f,-0.262866f, 0.951056f },
    {  0.442863f,-0.238856f, 0.864188f },
    {  0.162460f,-0.262866f, 0.951056f },
    {  0.309017f,-0.500000f, 0.809017f },
    {  0.147621f,-0.716567f, 0.681718f },
    {  0.000000f,-0.525731f, 0.850651f },
    {  0.425325f,-0.688191f, 0.587785f },
    {  0.587785f,-0.425325f, 0.688191f },
    {  0.688191f,-0.587785f, 0.425325f },
    { -0.955423f, 0.295242f, 0.000000f },
    { -0.951056f, 0.162460f, 0.262866f },
    { -1.000000f, 0.000000f, 0.000000f },
    { -0.850651f, 0.000000f, 0.525731f },
    { -0.955423f,-0.295242f, 0.000000f },
    { -0.951056f,-0.162460f, 0.262866f },
    { -0.864188f, 0.442863f,-0.238856f },
    { -0.951056f, 0.162460f,-0.262866f },
    { -0.809017f, 0.309017f,-0.500000f },
    { -0.864188f,-0.442863f,-0.238856f },
    { -0.951056f,-0.162460f,-0.262866f },
    { -0.809017f,-0.309017f,-0.500000f },
    { -0.681718f, 0.147621f,-0.716567f },
    { -0.681718f,-0.147621f,-0.716567f },
    { -0.850651f, 0.000000f,-0.525731f },
    { -0.688191f, 0.587785f,-0.425325f },
    { -0.587785f, 0.425325f,-0.688191f },
    { -0.425325f, 0.688191f,-0.587785f },
    { -0.425325f,-0.688191f,-0.587785f },
    { -0.587785f,-0.425325f,-0.688191f },
    { -0.688191f,-0.587785f,-0.425325f },
};

//
// End Quake 2 data.
//

static particle_t *JitterParticle(int ttl);
static void P_RunEffect(mobj_t *actor, unsigned int effects);
static void P_FlyEffect(mobj_t *actor);
static void P_BFGEffect(mobj_t *actor);
static void P_DripEffect(mobj_t *actor);
static void P_ExplosionParticles(fixed_t, fixed_t, fixed_t, byte, byte);

extern dboolean is_liquid_floor;
extern dboolean is_liquid_ceiling;
extern dboolean water_hit;

//
// P_GenVelocities
//
// Populates the avelocities array with randomly created floating
// point values. Derived from Quake 2. Available under the GNU
// General Public License.
//
static void P_GenVelocities(void)
{
    int i, j;

    for (i = 0; i < NUMVERTEXNORMALS; ++i)
        for (j = 0; j < 3; ++j)
            avelocities[i][j] = M_RandomSMMU() * 0.01f;
}

void P_InitParticleEffects(void)
{
    byte *palette = W_CacheLumpName("PLAYPAL", PU_STATIC);
    struct particleColorList *pc = particleColors;

    // match particle colors to best fit and write back to
    // static variables
    while (pc->color)
    {
        *(pc->color) = FindNearestColor(palette, pc->r, pc->g, pc->b);
        pc++;
    }

    P_GenVelocities();
}

//
// P_UnsetParticlePosition
//
// haleyjd 02/20/04: maintenance of particle sector links,
// necessitated by portals.
//
static void P_UnsetParticlePosition(particle_t *ptcl)
{
    M_DLListRemove((mdllistitem_t *)ptcl);

    ptcl->subsector = NULL;
}

//
// P_SetParticlePosition
//
// haleyjd 02/20/04: maintenance of particle sector links,
// necessitated by portals. Maintaining a subsector_t
// field in the particle_t will be useful in the future,
// I am sure.
//
static void P_SetParticlePosition(particle_t *ptcl)
{
    subsector_t *ss = R_PointInSubsector(ptcl->x, ptcl->y);

    M_DLListInsert((mdllistitem_t *)ptcl, (mdllistitem_t **)(&ss->sector->ptcllist));

    ptcl->subsector = ss;
}

void P_ParticleThinker(void)
{
    int i = activeParticles;
    particle_t *particle;
    particle_t *prev = NULL;
    sector_t *psec;
    fixed_t floorheight;

    while (i != -1)
    {
        particle = Particles + i;
        i = particle->next;

        // haleyjd: unlink the particle from the world
        P_UnsetParticlePosition(particle);

        // haleyjd: particles with fall to ground style don't start
        // fading or counting down their TTL until they hit the floor
        if (!(particle->styleflags & PS_FALLTOGROUND))
        {
            // perform fading
            unsigned int oldtrans = particle->trans;
            particle->trans -= particle->fade;

            // is it time to kill this particle?
            if (oldtrans < particle->trans || --particle->ttl == 0)
            {
                memset(particle, 0, sizeof(particle_t));

                if (prev)
                    prev->next = i;
                else
                    activeParticles = i;

                particle->next = inactiveParticles;
                inactiveParticles = particle - Particles;

                continue;
            }
        }

        // update and link to new position
        particle->x += particle->velx;
        particle->y += particle->vely;
        particle->z += particle->velz;

        P_SetParticlePosition(particle);

        // apply accelerations
        particle->velx += particle->accx;
        particle->vely += particle->accy;
        particle->velz += particle->accz;

        // handle special movement flags (post-position-set)
        psec = particle->subsector->sector;

        // haleyjd 09/04/05: use deep water floor if it is higher
        // than the real floor.
        floorheight =
            (psec->heightsec != -1 &&
            sectors[psec->heightsec].floorheight > psec->floorheight) ?
            sectors[psec->heightsec].floorheight :
            psec->floorheight;

        // did particle hit ground, but is now no longer on it?
        if (particle->styleflags & PS_HITGROUND && particle->z != floorheight)
            particle->z = floorheight;

        // floor clipping
        if (particle->z < floorheight)
        {
            // particles with fall to ground style start ticking now
            if (particle->styleflags & PS_FALLTOGROUND)
                particle->styleflags &= ~PS_FALLTOGROUND;

            // particles with floor clipping may need to stop
            if (particle->styleflags & PS_FLOORCLIP)
            {
                particle->z = floorheight;
                particle->accz = particle->velz = 0;
                particle->styleflags |= PS_HITGROUND;

                // some particles make splashes (FIXME)
                //if(particle->styleflags & PS_SPLASH)
                //   E_PtclTerrainHit(particle);
            }
        }
        prev = particle;
    }
}

void P_RunEffects(void)
{
    int snum = 0;
    thinker_t *currentthinker = &thinkercap;
/*
    // FIXME???
    if (camera)
    {
        subsector_t *ss = R_PointInSubsector(camera->x, camera->y);
        snum = (ss->sector - sectors) * numsectors;
    }
    else*/
    {
        subsector_t *ss = players[displayplayer].mo->subsector;
        snum = (ss->sector - sectors) * numsectors;
    }

    while ((currentthinker = currentthinker->next) != &thinkercap)
    {
        if (currentthinker->function == P_MobjThinker)
        {
            mobj_t *mobj = (mobj_t *)currentthinker;
            int rnum = snum + (mobj->subsector->sector - sectors);

            if (mobj->effects)
            {
                // run only if possibly visible
                if (!(rejectmatrix[rnum >> 3] & (1 << (rnum & 7))))
                    P_RunEffect(mobj, mobj->effects);
            }
        }
    }
}

//
// haleyjd 05/19/02: partially rewrote to not make assumptions
// about struct member order and alignment in memory
//

#define FADEFROMTTL(a)  (FRACUNIT/(a))

#define PARTICLE_VELRND ((FRACUNIT / 4096)  * (M_RandomSMMU() - 128))
#define PARTICLE_ACCRND ((FRACUNIT / 16384) * (M_RandomSMMU() - 128))

static particle_t *JitterParticle(int ttl)
{
    particle_t *particle = newParticle();

    if (particle)
    {
        // Set initial velocities
        particle->velx = PARTICLE_VELRND;
        particle->vely = PARTICLE_VELRND;
        particle->velz = PARTICLE_VELRND;

        // Set initial accelerations
        particle->accx = PARTICLE_ACCRND;
        particle->accy = PARTICLE_ACCRND;
        particle->accz = PARTICLE_ACCRND;

        // fully opaque
        particle->trans = FRACUNIT;

        particle->ttl = ttl;
        particle->fade = FADEFROMTTL(ttl);
    }
    return particle;
}

static void MakeFountain(mobj_t *actor, byte color1, byte color2)
{
    particle_t *particle;

    if (!(leveltime & 1))
        return;

    particle = JitterParticle(51);

    if (particle)
    {
        angle_t an  = M_RandomSMMU() << (24 - ANGLETOFINESHIFT);
        fixed_t out = FixedMul(actor->radius, M_RandomSMMU() << 8);

        particle->x = actor->x + FixedMul(out, finecosine[an]);
        particle->y = actor->y + FixedMul(out, finesine[an]);
        particle->z = actor->z + actor->height + FRACUNIT;

        P_SetParticlePosition(particle);

        if (out < actor->radius / 8)
            particle->velz += FRACUNIT * 10 / 3;
        else
            particle->velz += FRACUNIT * 3;

        particle->accz -= FRACUNIT / 11;

        if (M_RandomSMMU() < 30)
        {
            particle->size = 4;
            particle->color = color2;
        }
        else 
        {
            particle->size = 6;
            particle->color = color1;
        }
        particle->styleflags = 0;
    }
}

static void P_RunEffect(mobj_t *actor, unsigned int effects)
{
    angle_t      moveangle = R_PointToAngle2(0, 0, actor->momx, actor->momy);

    if (actor->effect_flies_can_spawn == true && (((effects & FX_FLIES) && actor->health <= 0)
        /*|| ((effects & FX_FLIESONDEATH) && actor->tics == -1 && actor->movecount >= 4 * TICRATE)*/) && d_spawnflies)
    {
        if (actor->effect_flies_shot == true)
            return;

        if (actor->effect_flies_start_timer < 360)
        {
            actor->effect_flies_start_timer++;
            return;
        }
        else
        {
            P_FlyEffect(actor);

            actor->effect_flies_spawned = true;
            actor->effect_flies_sound_timer++;

            if (actor->effect_flies_sound_timer == 60)
            {
                if (particle_sounds)
                    S_StartSound(actor, sfx_eefly);

                actor->effect_flies_sound_timer = 0;
            }
        }
    }

    if ((effects & FX_ROCKET) && d_drawrockettrails)
    {
        int i;
        int speed;
        particle_t *particle = JitterParticle(3 + (M_RandomSMMU() & 31));

        // Rocket trail
        fixed_t backx = actor->x - FixedMul(finecosine[(moveangle) >> ANGLETOFINESHIFT], actor->radius * 2);
        fixed_t backy = actor->y - FixedMul(finesine[(moveangle) >> ANGLETOFINESHIFT], actor->radius * 2);
        fixed_t backz = actor->z - (actor->height >> 3) * (actor->momz >> 16) + (2 * actor->height) / 3;

        angle_t an = (moveangle + ANG90) >> ANGLETOFINESHIFT;

        if (particle)
        {
            fixed_t pathdist = M_RandomSMMU() << 8;

            particle->x = backx - FixedMul(actor->momx, pathdist);
            particle->y = backy - FixedMul(actor->momy, pathdist);
            particle->z = backz - FixedMul(actor->momz, pathdist);

            P_SetParticlePosition(particle);

            speed = (M_RandomSMMU() - 128) * (FRACUNIT / 200);

            particle->velx += FixedMul(speed, finecosine[an]);
            particle->vely += FixedMul(speed, finesine[an]);
            particle->velz -= FRACUNIT / 36;
            particle->accz -= FRACUNIT / 20;
            particle->color = yellow;
            particle->size = 2;
            particle->styleflags = PS_FULLBRIGHT;
        }

        for (i = 6; i; --i)
        {
            particle_t *iparticle = JitterParticle (3 + (M_RandomSMMU() & 31));

            if (iparticle)
            {
                fixed_t pathdist = M_RandomSMMU() << 8;

                iparticle->x = backx - FixedMul(actor->momx, pathdist);
                iparticle->y = backy - FixedMul(actor->momy, pathdist);
                iparticle->z = backz - FixedMul(actor->momz, pathdist) +
                          (M_RandomSMMU() << 10);

                P_SetParticlePosition(iparticle);

                speed = (M_RandomSMMU() - 128) * (FRACUNIT/200);

                iparticle->velx += FixedMul(speed, finecosine[an]);
                iparticle->vely += FixedMul(speed, finesine[an]);
                iparticle->velz += FRACUNIT/80;
                iparticle->accz += FRACUNIT/40;
                iparticle->color = (M_RandomSMMU() & 7) ? grey2 : grey1;
                iparticle->size = 3;
                iparticle->styleflags = 0;
            }
            else
                break;
        }
    }

    if ((effects & FX_GRENADE) && smoketrails)
    {
        // Grenade trail
        P_DrawSplash2(6,
            actor->x - FixedMul (finecosine[(moveangle) >> ANGLETOFINESHIFT], actor->radius * 2),
            actor->y - FixedMul (finesine[(moveangle) >> ANGLETOFINESHIFT], actor->radius * 2),
            actor->z - (actor->height >> 3) * (actor->momz >> 16) + (2 * actor->height) / 3,
            moveangle + ANG180, 2, 2);
    }

    if ((effects & FX_BFG) && d_drawbfgcloud)
        P_BFGEffect(actor);

    // FIXME
    if ((effects & FX_FOUNTAINMASK) /*&& !(actor->flags2 & MF2_DORMANT)*/)
    {
        // Particle fountain -- can be switched on and off via the
        // MF2_DORMANT flag
        static const byte *fountainColors[16] =
        { 
            &black,  &black,
            &red,    &red1,
            &green,  &green1,
            &blue,   &blue1,
            &yellow, &yellow1,
            &purple, &purple1,
            &black,  &grey3,
            &grey4,  &white
        };

        unsigned int color = (effects & FX_FOUNTAINMASK) >> 15;

        MakeFountain(actor, *fountainColors[color], *fountainColors[color + 1]);
    }

    if ((effects & FX_DRIP) && d_dripblood)
        P_DripEffect(actor);
}

//
// [nitr8] UNUSED
//
/*
void P_DrawSplash(int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int kind)
{
    byte color1;
    byte color2;

    switch (kind)
    {
        // Spark
        case 1:
            color1 = orange;
            color2 = yorange;
            break;

        default:
            return;
    }

    for ( ; count; count--)
    {
        angle_t an;
        particle_t *p = JitterParticle(10);

        if (!p)
            break;

        p->size = 2;
        p->color = (M_RandomSMMU() & 0x80) ? color1 : color2;
        p->styleflags = PS_FULLBRIGHT;
        p->velz -= M_RandomSMMU() * 512;
        p->accz -= FRACUNIT / 8;
        p->accx += (M_RandomSMMU() - 128) * 8;
        p->accy += (M_RandomSMMU() - 128) * 8;
        p->z = z - M_RandomSMMU() * 1024;
        an = (angle + (M_RandomSMMU() << 21)) >> ANGLETOFINESHIFT;
        p->x = x + (M_RandomSMMU() & 15) * finecosine[an];
        p->y = y + (M_RandomSMMU() & 15) * finesine[an];

        P_SetParticlePosition(p);
    }
}
*/

//
// P_BloodDrop
//
// haleyjd 04/01/05: Code originally by SoM that makes a blood drop
// that falls to the floor. Isolated by me into a function, and made
// to use new styleflags that weren't available when this was written.
// This code is under the GPL.
//
static void P_BloodDrop(int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, byte color1, byte color2)
{
    for ( ; count; --count)
    {
        particle_t *p = newParticle();
        angle_t    an;

        if (!p)
            break;

        p->ttl = 48;
        p->fade = FADEFROMTTL(48);
        p->trans = FRACUNIT;
        p->size = 4;
        p->color = (M_RandomSMMU() & 0x80) ? color1 : color2;
        p->velz = 128 * -3000 + M_RandomSMMU();
        p->accz = -(GRAVITY * 100 / 256);
        p->styleflags = PS_FLOORCLIP | PS_FALLTOGROUND;
        p->z = z + (M_RandomSMMU() - 128) * -2400;
        an = (angle + ((M_RandomSMMU() - 128) << 22)) >> ANGLETOFINESHIFT;
        p->x = x + (M_RandomSMMU() & 10) * finecosine[an];
        p->y = y + (M_RandomSMMU() & 10) * finesine[an];

        P_SetParticlePosition(p);
    }
}

//
// P_SmokePuff
//
// haleyjd 09/10/07: SoM's vastly improved smoke puff effect, improved even
// more to replace the god-awful Quake 2-wannabe splash from zdoom.
// This code is under the GPL.
//
void P_SmokePuff(int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int updown)
{
    particle_t *p;
    angle_t an;
    int ttl;
    fixed_t accz;
    dboolean hitwater = false;

    // default: grey puff
    byte color1 = grey1;
    byte color2 = grey5;

//    if (!comp[comp_terrain])
    {
        // 06/21/02: make bullet puff colors responsive to 
        // TerrainTypes -- this is very cool and Quake-2-like ^_^
        int terrain = P_GetTerrainTypeForPoint(x, y, updown);

        switch (terrain)
        {
            case FLOOR_WATER:
                color1 = blue1;
                color2 = blue;
                break;

            case FLOOR_LAVA:
                color1 = orange;
                color2 = mdred;
                break;

            case FLOOR_NUKAGE:
                color1 = green1;
                color2 = green;
                break;

            case FLOOR_BLOOD:
                color1 = mdred;
                color2 = red;
                break;

            case FLOOR_SLIME:
                color1 = maroon1;
                color2 = maroon1;
                break;

            default:
                break;
        }

        if (is_liquid_floor || is_liquid_ceiling)
            hitwater = true;
    }

    // MOARRRR!
    count += M_RandomSMMU() & 15;

    // handle shooting liquids: make it spray up like in the movies
    if (!updown && hitwater)
    {
        // live longer and accelerate downward faster
        ttl  = 30;
        accz = -FRACUNIT/8;
        water_hit = true;
    }
    else
    {
        ttl  = 15;
        accz = -FRACUNIT/22;
        water_hit = false;
    }

    for ( ; count; --count)
    {
        if (!(p = newParticle()))
            break;

         p->ttl = ttl;
         p->fade = FADEFROMTTL(ttl);
         p->trans = FRACUNIT;
         p->size = 2 + M_RandomSMMU() % 5;
         p->color = (M_RandomSMMU() & 0x80) ? color1 : color2;
         p->velz = M_RandomSMMU() * 512;

         // ceiling shot?
         if (updown == 1)
             p->velz = -(p->velz / 4);

         p->accz = accz;
         p->styleflags = 0;

         an = (angle + ((M_RandomSMMU() - 128) << 23)) >> ANGLETOFINESHIFT;
         p->velx = (M_RandomSMMU() * finecosine[an]) >> 11;
         p->vely = (M_RandomSMMU() * finesine[an]) >> 11;
         p->accx = p->velx >> 4;
         p->accy = p->vely >> 4;

         // ceiling shot?
         if (updown == 1)
             p->z = z - (M_RandomSMMU() + 72) * 2000;
         else
             p->z = z + (M_RandomSMMU() + 72) * 2000;

         an = (angle + ((M_RandomSMMU() - 128) << 22)) >> ANGLETOFINESHIFT;
         p->x = x + (M_RandomSMMU() & 14) * finecosine[an];
         p->y = y + (M_RandomSMMU() & 14) * finesine[an];

         P_SetParticlePosition(p);
    }

    // no sparks on liquids
    if (!hitwater)
    {
        count = M_RandomSMMU() & 3;

        for ( ; count; --count)
        {
            fixed_t pathdist = M_RandomSMMU() << 8;
            fixed_t speed;

            if (!(p = JitterParticle(3 + (M_RandomSMMU() % 24))))
                break;

            p->x = x - pathdist;
            p->y = y - pathdist;
            p->z = z - pathdist;

            P_SetParticlePosition(p);

            speed = (M_RandomSMMU() - 128) * (FRACUNIT / 200);
            an = angle >> ANGLETOFINESHIFT;
            p->velx += FixedMul(speed, finecosine[an]);
            p->vely += FixedMul(speed, finesine[an]);

            // on ceiling or wall, fall fast
            // or else (if on floor), throw it upward a bit
            if (updown)
                p->velz -= FRACUNIT / 36;
            else
                p->velz += FRACUNIT / 2;

            p->accz -= FRACUNIT / 20;
            p->color = yellow;
            p->size = 2;
            p->styleflags = PS_FULLBRIGHT;
        }
    }
}

// haleyjd 05/08/03: custom particle blood colors
/*
static struct bloodColor {
    byte *color1;
    byte *color2;
} mobjBloodColors[NUMBLOODCOLORS] = {
    { &red,    &dred2   },
    { &grey1,  &grey5   },
    { &green,  &green1  },
    { &blue,   &blue    },
    { &yellow, &yellow  },
    { &black,  &grey3   },
    { &purple, &purple1 },
    { &grey4,  &white   },
    { &orange, &yorange },
};
*/
void P_BloodSpray(mobj_t *mo, int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle)
{
    byte color1;
    byte color2;
/*
    int bloodcolor = mo->info->blood;

    // get blood colors
    if (bloodcolor < 0 || bloodcolor >= NUMBLOODCOLORS)
        bloodcolor = 0;

    color1 = *(mobjBloodColors[bloodcolor].color1);
    color2 = *(mobjBloodColors[bloodcolor].color2);
*/
    if (fsize == 12361532 || mo->type == MT_BRUISER || mo->type == MT_KNIGHT)
    {
        color1 = green;
        color2 = green1;
    }
    else if (mo->type == MT_HEAD && fsize != 12361532)
    {
        color1 = blue1;
        color2 = blue;
    }
    else
    {
        color1 = red;
        color2 = dred2;
    }

    // haleyjd 04/01/05: at random, throw out drops
    // haleyjd 09/10/07: even if a drop is thrown, do the rest of the effect
    if (M_RandomSMMU() < 72)
        P_BloodDrop(count, x, y, z, angle, color1, color2);

    // swap colors if reversed
    if (color2 < color1)
    {
        int tempcol = color1;
        color1  = color2;
        color2  = tempcol;
    }

    // a LOT more blood.
    count += 3 * ((M_RandomSMMU() & 31) + 1) / 2;

    // haleyjd 07/04/09: randomize z coordinate a bit (128/32 == 4 units)
    z += 3 * FRACUNIT + (M_RandomSMMU() - 128) * FRACUNIT / 32;

    for( ; count; --count)
    {
        angle_t an;
        particle_t *p;

        if (!(p = newParticle()))
            break;

        p->ttl = 15 + M_RandomSMMU() % 6;
        p->fade = FADEFROMTTL(p->ttl);
        p->trans = FRACUNIT;
        p->size = 2 + M_RandomSMMU() % 5;

        // if colors are part of same ramp, use all in between
        if (color1 != color2 && ABS(color2 - color1) <= 16)
            p->color = M_RangeRandom(color1, color2);
        else
            p->color = (M_RandomSMMU() & 0x80) ? color1 : color2;

        p->styleflags = 0;
        an      = (angle + ((M_RandomSMMU() - 128) << 23)) >> ANGLETOFINESHIFT;
        p->velx = (M_RandomSMMU() * finecosine[an]) / 768;
        p->vely = (M_RandomSMMU() * finesine[an]) / 768;
        an      = (angle + ((M_RandomSMMU() - 128) << 22)) >> ANGLETOFINESHIFT;
        p->x    = x + (M_RandomSMMU() % 15) * finecosine[an];
        p->y    = y + (M_RandomSMMU() % 15) * finesine[an];
        p->z    = z + (M_RandomSMMU() - 128) * -3500;
        p->velz = (M_RandomSMMU() < 32) ? M_RandomSMMU() * 140 : M_RandomSMMU() * -128;
        p->accz = -FRACUNIT / 16;

        P_SetParticlePosition(p);
    }
}

void P_DrawSplash2(int count, fixed_t x, fixed_t y, fixed_t z, angle_t angle, int updown, int kind)
{
    byte color1;
    byte color2;
    int zvel;
    int zspread;
    int zadd;

    switch (kind)
    {
        // Smoke
        case 2:
            color1 = grey3;
            color2 = grey1;
            break;

        default:
            return;
    }

    zvel = -128;
    zspread = (updown ? -6000 : 6000);
    zadd = ((updown == 2) ? -128 : 0);

    for ( ; count; count--)
    {
        particle_t *p = newParticle();
        angle_t an;

        if (!p)
            break;

        p->ttl = 12;
        p->fade = FADEFROMTTL(12);
        p->trans = FRACUNIT;
        p->styleflags = 0;
        p->size = 2 + M_RandomSMMU() % 5;
        p->color = (M_RandomSMMU() & 0x80) ? color1 : color2;
        p->velz = M_RandomSMMU() * zvel;
        p->accz = -FRACUNIT / 22;

        if (kind)
        {
            an = (angle + ((M_RandomSMMU() - 128) << 23)) >> ANGLETOFINESHIFT;
            p->velx = (M_RandomSMMU() * finecosine[an]) >> 11;
            p->vely = (M_RandomSMMU() * finesine[an]) >> 11;
            p->accx = p->velx >> 4;
            p->accy = p->vely >> 4;
        }

        p->z = z + (M_RandomSMMU() + zadd) * zspread;
        an = (angle + ((M_RandomSMMU() - 128) << 22)) >> ANGLETOFINESHIFT;
        p->x = x + (M_RandomSMMU() & 31) * finecosine[an];
        p->y = y + (M_RandomSMMU() & 31) * finesine[an];

        P_SetParticlePosition(p);
   }
}

void P_DisconnectEffect(mobj_t *actor)
{
    if (teleport_particle)
    {
        int i;

        for (i = 64; i; i--)
        {
            particle_t *p = JitterParticle (TICRATE * 2);

            if (!p)
                break;

            p->x = actor->x + ((M_RandomSMMU() - 128) << 9) * (actor->radius >> FRACBITS);
            p->y = actor->y + ((M_RandomSMMU() - 128) << 9) * (actor->radius >> FRACBITS);
            p->z = actor->z + (M_RandomSMMU() << 8) * (actor->height >> FRACBITS);

            P_SetParticlePosition(p);

            p->accz -= FRACUNIT / 4096;
            p->color = M_RandomSMMU() < 128 ? yellow : yellow1;
            p->size = 4;
            p->styleflags = PS_FULLBRIGHT;
        }
    }
}

//
// P_FlyEffect
//
// Derived from Quake 2. Available under the GNU General Public License.
//
static void P_FlyEffect(mobj_t *actor)
{
    int i;
    int count;
    particle_t *p;
    vec3_t forward;
    float ltime = (float)leveltime / 50.0f;

    // 07/13/05: ramp flies up over time for flies-on-death effect
    if (actor->effects & FX_FLIESONDEATH)
        count = (actor->movecount - 4 * TICRATE) * 162 / (20 * TICRATE);
    else
        count = 162;

    if (count < 1)
        count = 1;   

    if (count > 162)
        count = 162;

    for (i = 0; i < count; i += 2)
    {
        float angle;
        float sp;
        float sy;
        float cp;
        float cy;
        float dist = 64;

        if (!(p = newParticle()))
            break;

        angle = ltime * avelocities[i][0];
        sy = (float)sin(angle);
        cy = (float)cos(angle);
        angle = ltime * avelocities[i][1];
        sp = (float)sin(angle);
        cp = (float)cos(angle);

        forward[0] = cp * cy;
        forward[1] = cp * sy;
        forward[2] = -sp;

        dist = (float)sin(ltime + i)*64;
        p->x = actor->x + (int)((bytedirs[i][0] * dist + forward[0] * BEAMLENGTH) * FRACUNIT);
        p->y = actor->y + (int)((bytedirs[i][1] * dist + forward[1] * BEAMLENGTH) * FRACUNIT);
        p->z = actor->z + (int)((bytedirs[i][2] * dist + forward[2] * BEAMLENGTH) * FRACUNIT);

        P_SetParticlePosition(p);

        p->velx = p->vely = p->velz = 0;
        p->accx = p->accy = p->accz = 0;
        p->color = black;

        // ???
        p->size = 4;

        p->ttl = 1;
        p->trans = FRACUNIT;
        p->styleflags = 0;
    }
}

//
// P_BFGEffect
//
// Derived from Quake 2. Available under the GNU General Public License.
//
static void P_BFGEffect(mobj_t *actor)
{
    int i;
    particle_t *p;
    vec3_t forward;
    float ltime = (float)leveltime / 30.0f;

    for (i = 0; i < NUMVERTEXNORMALS; i++)
    {
        float angle;
        float sp;
        float sy;
        float cp;
        float cy;
        float dist = 64;

        if (!(p = newParticle()))
            break;

        angle = ltime * avelocities[i][0];
        sy = (float)sin(angle);
        cy = (float)cos(angle);
        angle = ltime * avelocities[i][1];
        sp = (float)sin(angle);
        cp = (float)cos(angle);

        forward[0] = cp * cy;
        forward[1] = cp * sy;
        forward[2] = -sp;

        dist = (float)sin(ltime + i) * 64;
        p->x = actor->x + (int)((bytedirs[i][0] * dist + forward[0] * BEAMLENGTH) * FRACUNIT);
        p->y = actor->y + (int)((bytedirs[i][1] * dist + forward[1] * BEAMLENGTH) * FRACUNIT);
        p->z = actor->z + (15 * FRACUNIT) + (int)((bytedirs[i][2] * dist + forward[2] * BEAMLENGTH) * FRACUNIT);

        P_SetParticlePosition(p);

        p->velx = p->vely = p->velz = 0;
        p->accx = p->accy = p->accz = 0;
        p->color = green;
        p->size = 4;
        p->ttl = 1;
        p->trans = 2 * FRACUNIT / 3;
        p->styleflags = PS_FULLBRIGHT;
    }
}

//
// P_DripEffect
//
// haleyjd 09/05/05: Effect for parameterized particle drip object.
//
// Parameters:
// args[0] = color (palette index)
// args[1] = size
// args[2] = frequency
// args[3] = make splash?
// args[4] = fullbright?
//
int randInRange(int min, int max)
{
    return min + (int) ((double)rand() / (double)RAND_MAX * (max - min + 1));
}

static void P_DripEffect(mobj_t *actor)
{
    dboolean makesplash;
    particle_t *p;

    if (randInRange(0, 1))
        makesplash = true;
    else
        makesplash = false;

    // do not cause a division by zero crash or
    // allow a negative frequency
    if (randInRange(-1, 1) <= 0)
        return;

    if (leveltime % randInRange(50, 100))
        return;

    if (!(p = newParticle()))
        return;

    p->ttl   = 18;
    p->trans = 9 * FRACUNIT / 16;
    p->fade  = p->trans / p->ttl;

    p->color = actor->blood;
    p->size  = randInRange(2, 5);

    p->velz = 128 * -3000;
    p->accz = -GRAVITY;
    p->styleflags = PS_FLOORCLIP | PS_FALLTOGROUND;

    if (makesplash)
        p->styleflags |= PS_SPLASH;

    if (d_brightmaps)
        p->styleflags |= PS_FULLBRIGHT;

    p->x = actor->x;
    p->y = actor->y;

    // monsters
    if (actor->flags & MF_COUNTKILL)
        p->z = 112;
    // hanging bodies
    else
        p->z = actor->subsector->sector->ceilingheight;

    P_SetParticlePosition(p);
}

//
// haleyjd 05/20/02: frame-based particle events system
//
// A field, particle_evt, has been added to the state_t structure
// to provide a numerical indicator of what type of effect a frame
// should trigger. All code related to particle events is available
// under the GNU General Public License.
//

particle_event_t particleEvents[P_EVENT_NUMEVENTS] =
{
    // P_EVENT_NONE
    { NULL,              "pevt_none"    },

    // P_EVENT_ROCKET_EXPLODE
    { P_RocketExplosion, "pevt_rexpl"   },

    // P_EVENT_BFG_EXPLODE
    { P_BFGExplosion,    "pevt_bfgexpl" },
};

//
// P_RunEvent
//
// Called from P_SetMobjState, immediately after the action function
// for the actor's current state has been executed.
//
void P_RunEvent(mobj_t *actor)
{
    int effectNum;

    if (!d_drawparticles)
        return;

    // haleyjd: 
    // if actor->state is NULL, the thing has been removed, or
    // if MIF_NOPTCLEVTS is set, don't run events for this thing
    if (!actor || !actor->state /* || (actor->intflags & MIF_NOPTCLEVTS)*/)
        return;

    effectNum = actor->state->particle_evt;

    if (effectNum < 0 || effectNum >= P_EVENT_NUMEVENTS)
    {
        C_Warning("P_RunEvent: Particle event number out of range");
        return;
    }

    if (effectNum != P_EVENT_NONE)
    {
        if (particleEvents[effectNum].enabled)
            particleEvents[effectNum].func(actor);
    }
}

//
// P_ExplosionParticles
//
// Causes an explosion in a customizable color. Derived from Quake 2's
// rocket/BFG burst code. Available under the GNU General Public
// License.
//
static void P_ExplosionParticles(fixed_t x, fixed_t y, fixed_t z, byte color1, byte color2)
{
    int i;

    for(i = 0; i < 256; i++)
    {
        int rnd;
        particle_t *p = newParticle();

        if (!p)
            break;

        p->ttl = 26;
        p->fade = FADEFROMTTL(26);
        p->trans = FRACUNIT;

        // 2^11 = 2048, 2^12 = 4096
        p->x = x + (((M_RandomSMMU() % 32) - 16) * 4096);
        p->y = y + (((M_RandomSMMU() % 32) - 16) * 4096);
        p->z = z + (((M_RandomSMMU() % 32) - 16) * 4096);

        P_SetParticlePosition(p);

        // note: was (rand() % 384) - 192 in Q2, but DOOM's RNG
        // only outputs numbers from 0 to 255, so it has to be
        // corrected to unbias it and get output from approx.
        // -192 to 191
        rnd = M_RandomSMMU();
        p->velx = (rnd - 192 + (rnd / 2)) * 2048;
        rnd = M_RandomSMMU();
        p->vely = (rnd - 192 + (rnd / 2)) * 2048;
        rnd = M_RandomSMMU();
        p->velz = (rnd - 192 + (rnd / 2)) * 2048;
        p->accx = p->accy = p->accz = 0;
        p->size = (M_RandomSMMU() < 48) ? 6 : 4;
        p->color = (M_RandomSMMU() & 0x80) ? color2 : color1;
        p->styleflags = PS_FULLBRIGHT;
    }
}

void P_RocketExplosion(mobj_t *actor)
{
    P_ExplosionParticles(actor->x, actor->y, actor->z, orange, yorange);
}

void P_BFGExplosion(mobj_t *actor)
{
    P_ExplosionParticles(actor->x, actor->y, actor->z, green, green);
}

