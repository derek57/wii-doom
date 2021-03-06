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
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------


#ifndef __HULIB__
#define __HULIB__


// We are referring to patches.
#include "r_defs.h"

#include "v_patch.h"


// font stuff
#define HU_CHARERASE        KEY_BACKSPACE

#define HU_MAXLINES         4
#define HU_MAXLINELENGTH    80


//
// Typedefs of widgets
//

// Text Line widget
//  (parent of Scrolling Text and Input Text widgets)
typedef struct
{
    // left-justified position of scrolling text window
    int                  x;
    int                  y;
    
    // font
    patch_t              **f;

    // start character
    int                  sc;

    // line of text
    char                 l[HU_MAXLINELENGTH + 1];

    // current line length
    int                  len;

    // whether this line needs to be udpated
    int                  needsupdate;              

} hu_textline_t;

// Scrolling Text window widget
//  (child of Text Line widget)
typedef struct
{
    // text lines to draw
    hu_textline_t        l[HU_MAXLINES];

    // height in lines
    int                  h;

    // current line number
    int                  cl;

    // pointer to dboolean stating whether to update window
    dboolean             *on;

    // last value of *->on.
    dboolean             laston;

} hu_stext_t;

// Input Text Line widget
//  (child of Text Line widget)
typedef struct
{
    // text line to input on
    hu_textline_t        l;

     // left margin past which I am not to delete characters
    int                  lm;

    // pointer to dboolean stating whether to update window
    dboolean             *on; 

    // last value of *->on;
    dboolean             laston;

} hu_itext_t;


//
// Widget creation, access, and update routines
//

// initializes heads-up widget library
void HUlib_init(void);

//
// textline code
//

// clear a line of text
void HUlib_clearTextLine(hu_textline_t *t);

// draws tline
void HUlib_drawTextLine(hu_textline_t *l, dboolean drawcursor);

// ?
void HUlib_initSText(hu_stext_t *s, int x, int y, int h, patch_t **font, int startchar, dboolean *on);

// ?
void HUlib_addMessageToSText(hu_stext_t *s, char *prefix, char *msg);

// draws stext
void HUlib_drawSText(hu_stext_t *s);

// erases all stext lines
void HUlib_eraseSText(hu_stext_t *s); 

// Input Text Line widget routines
void HUlib_initIText(hu_itext_t *it, int x, int y, patch_t **font, int startchar, dboolean *on);

// enforces left margin
void HUlib_delCharFromIText(hu_itext_t *it);

// enforces left margin
void HUlib_eraseLineFromIText(hu_itext_t *it);

// resets line and left margin
void HUlib_resetIText(hu_itext_t *it);

// left of left-margin
void HUlib_addPrefixToIText(hu_itext_t *it, char *str);

// erases all itext lines
void HUlib_eraseIText(hu_itext_t *it); 

void HUlib_initTextLine(hu_textline_t *t, int x, int y, patch_t **f, int sc);
void HUlib_drawIText(hu_itext_t *it);

// returns success
dboolean HUlib_addCharToTextLine(hu_textline_t *t, char ch);
dboolean HUlib_delCharFromTextLine(hu_textline_t *t);

// whether eaten
dboolean HUlib_keyInIText(hu_itext_t *it, unsigned char ch);

#endif

