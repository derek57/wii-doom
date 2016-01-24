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
//#define CENTERY                        (SCREENHEIGHT/2)

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.
extern byte     *screens[5];

extern byte     *dp_translation;

extern dboolean dp_translucent;

void GetPixelSize(void);

static inline void V_ClearDPTranslation(void)
{
    if (dp_translation)
        dp_translation = NULL;
}

dboolean V_EmptyPatch(patch_t *patch);

//void V_DrawHorizLine(int x, int y, int scrn, int w, int c);

// Allocates buffer screens, call before R_Init.

void V_Init (void);

// Draw a block from the specified source screen to the screen.

void V_CopyRect(int srcx, int srcy, int srcscrn, int width, int height, int destx, int desty, int destscrn);

// Draw a linear block of pixels into the view buffer.

void V_DrawBlock(int x, int y, int scrn, int width, int height, byte* src);

// Temporarily switch to using a different buffer to draw graphics, etc.
/*
void V_UseBuffer(int srcscrn, int destscrn);

// Return to using the normal screen buffer to draw graphics.

void V_RestoreBuffer(int srcscrn, int destscrn);
*/
void V_DrawPatch(int x, int y, int scrn, patch_t* patch);
void V_DrawPatchFlipped(int x, int y, int scrn, patch_t* patch);
//void V_MarkRect(int x, int y, int srcscrn, int width, int height, int destscrn);
void V_DrawConsoleChar(int x, int y, int scrn, patch_t *patch, int color1, int color2, dboolean italics, byte *tinttab);
void V_DrawHUDPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab);
void V_DrawYellowHUDPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab);
void V_DrawTranslucentHUDPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab);
void V_DrawTranslucentHUDNumberPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab);
void V_DrawTranslucentYellowHUDPatch(int x, int y, int scrn, patch_t *patch, byte *tinttab);
/*
void V_ColorBlock(int x, int y, int scrn, int width, int height, byte color);
void V_GetBlock (int x, int y, int scrn, int width, int height, byte *dest);
*/
void V_ScreenShot(int scrn, char *format);
void V_LowGraphicDetail(int height, int scrn);
void V_DrawPatchWithShadow(int x, int y, int scrn, patch_t *patch, dboolean flag);
void V_DrawDistortedBackground(int scrn, char* patchname);
void V_FillRect(int x, int y, int scrn, int width, int height, byte color);

void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);

#endif

