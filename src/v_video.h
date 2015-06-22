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
//        Gamma correction LUT.
//        Functions to draw patches (by post) directly to screen.
//        Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------


#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomtype.h"

// Needed because we are refering to patches.
#include "v_patch.h"


//
// VIDEO
//
#define CENTERY                        (SCREENHEIGHT/2)


// haleyjd 08/28/10: implemented for Strife support
// haleyjd 08/28/10: Patch clipping callback, implemented to support Choco
// Strife.
typedef boolean (*vpatchclipfunc_t)(patch_t *, int, int);


// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.
extern byte *screens[5];
extern byte *xlatab;
extern byte *tinttable;
extern byte *dp_translation;

extern boolean dp_translucent;

static inline void V_ClearDPTranslation(void)
{
    if (dp_translation)
        dp_translation = NULL;
}

boolean V_EmptyPatch(patch_t *patch);

void V_DrawHorizLine(int x, int y, int w, int c);

// Allocates buffer screens, call before R_Init.
void V_Init (void);

// Draw a block from the specified source screen to the screen.

void V_CopyRect(int srcx, int srcy, int srcscrn, int width,
                int height, int destx, int desty, int destscrn ) ;

void V_DrawPatch(int x, int y, int scrn, patch_t* patch ) ;

void V_DrawPatchFlipped(int x, int y, int scrn, patch_t* patch ) ;

// Draw a linear block of pixels into the view buffer.

void V_DrawBlock(int x, int y, int scrn, int width, int height, byte* src ) ;

void V_MarkRect(int x, int y, int width, int height);

void V_DrawConsoleChar(int x, int y, patch_t *patch, byte color, boolean italics, int translucency,
                       boolean inverted);

void V_DrawStatusPatch(int x, int y, patch_t *patch, byte *tinttab);

void V_DrawYellowStatusPatch(int x, int y, patch_t *patch, byte *tinttab);

void V_DrawTranslucentStatusPatch(int x, int y, patch_t *patch, byte *tinttab);

void V_DrawTranslucentStatusNumberPatch(int x, int y, patch_t *patch, byte *tinttab);

void V_DrawTranslucentYellowStatusPatch(int x, int y, patch_t *patch, byte *tinttab);

void V_ColorBlock(int x, int y, int scrn, int width, int height, byte color);

void V_GetBlock (int x, int y, int scrn, int width, int height, byte *dest);

void V_LoadXlaTable(void);

#endif

