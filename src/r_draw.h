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
//        System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __R_DRAW__
#define __R_DRAW__


#define R_ADDRESS(scrn, px, py) \
    (I_VideoBuffer + (viewwindowy + (py)) * SCREENWIDTH + (viewwindowx + (px)))


extern int                dc_x;
extern int                dc_yl;
extern int                dc_yh;
extern int                dc_texheight;
extern int                ds_y;
extern int                ds_x1;
extern int                ds_x2;

// start of a 64*64 tile image
extern byte*              ds_source;                

// first pixel in a column
extern byte*              dc_source;                

extern byte*              translationtables;
extern byte*              dc_translation;

extern lighttable_t*      dc_colormap;
extern lighttable_t*      ds_colormap;

extern fixed_t            ds_xfrac;
extern fixed_t            ds_yfrac;
extern fixed_t            ds_xstep;
extern fixed_t            ds_ystep;
extern fixed_t            dc_blood;
extern fixed_t            dc_iscale;
extern fixed_t            dc_texturemid;


// The span blitting interface.
// Hook in assembler or system specific BLT
//  here.
void         R_DrawColumn (void);
void         R_DrawColumnLow (void);

// The Spectre/Invisibility effect.
void         R_DrawFuzzColumn (void);
void         R_DrawFuzzColumnLow (void);

// Draw with color translation tables,
//  for player sprite rendering,
//  Green/Red/Blue/Indigo shirts.
void         R_DrawTranslatedColumn (void);
void         R_DrawTranslatedColumnLow (void);
void         R_DrawTLColumn (void);

// Span blitting for rows, floor/ceiling.
// No Sepctre effect needed.
void         R_DrawSpan (void);

// Low resolution mode, 160x200?
void         R_DrawSpanLow (void);
void         R_DrawTLColumnLow (void);
void         R_InitBuffer(int width, int height );

// Initialize color translation tables,
//  for player rendering etc.
void         R_InitTranslationTables (void);

// Rendering function.
void         R_FillBackScreen (void);

// If the view size is not full screen, draws a border around it.
void         R_DrawViewBorder (void);

void         R_DrawShadowColumn(void);
void         R_DrawSpectreShadowColumn(void);
void         R_DrawSolidShadowColumn(void);

#endif
