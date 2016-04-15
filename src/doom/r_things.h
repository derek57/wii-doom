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


#if !defined(__R_THINGS__)
#define __R_THINGS__


// OK, now this is an interesting point:
//
// DOOM Retro uses "#define NUMVISSPRITES   0x20000" here as default.
// It's the same as if we would use "16384 * 8" (131072) instead.
//
// BUT WARNING: THIS IS NOT WORKING FOR THE WII AS IT WON'T RUN THE
// COMPILED "BOOT.DOL" (ELF) FILE WITH THOSE VALUES!
//
// SO WE LOWER THIS TO "8192 * 8" (65536) which is the same as 0x10000.

#ifdef WII
#define NUMVISSPRITES   0x10000
#else
#define NUMVISSPRITES   0x20000
#endif


void R_InitParticles(void);
void R_ClearParticles(void);
void R_AddPSprites(void);
void R_DrawSprites(void);
void R_InitSprites(void);
void R_ClearSprites(void);
void R_DrawPlayerSprites(void);
void R_DrawMasked(void);

void R_ProjectSprite(mobj_t *thing);
void R_ProjectBloodSplat(mobj_t *thing);
void R_ProjectShadow(mobj_t *thing);


// Constant arrays used for psprite clipping
//  and initializing clipping.
extern int      negonearray[SCREENWIDTH];
extern int      screenheightarray[SCREENWIDTH];

// vars for R_DrawMaskedColumn
extern int      *mfloorclip;
extern int      *mceilingclip;

extern int64_t  sprtopscreen;

extern fixed_t  spryscale;
extern fixed_t  pspritexscale;
extern fixed_t  pspriteyscale;
extern fixed_t  pspriteiscale;
extern fixed_t  viewheightfrac;

extern dboolean r_playersprites;


#endif

