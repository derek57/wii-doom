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
// DESCRIPTION:  heads-up text and input code
//
//-----------------------------------------------------------------------------


#include <ctype.h>

#include "doomdef.h"

#ifdef WII
#include "../doomkeys.h"
#else
#include "doomkeys.h"
#endif

#include "hu_lib.h"

#ifdef WII
#include "../i_swap.h"
#else
#include "i_swap.h"
#endif

#include "r_draw.h"
#include "r_local.h"

#ifdef WII
#include "../v_video.h"
#else
#include "v_video.h"
#endif

// boolean : whether the screen is always erased
#define noterased viewwindowx

extern boolean        automapactive;        // in AM_map.c

void HUlib_init(void)
{
}

void HUlib_clearTextLine(hu_textline_t* t)
{
    t->len = 0;
    t->l[0] = 0;
    t->needsupdate = true;
}

void
HUlib_initTextLine
( hu_textline_t*        t,
  int                   x,
  int                   y,
  patch_t**             f,
  int                   sc )
{
    t->x = x;
    t->y = y;
    t->f = f;
    t->sc = sc;
    HUlib_clearTextLine(t);
}

boolean
HUlib_addCharToTextLine
( hu_textline_t*        t,
  char                  ch )
{

    if (t->len == HU_MAXLINELENGTH)
        return false;
    else
    {
        t->l[t->len++] = ch;
        t->l[t->len] = 0;
        t->needsupdate = 4;
        return true;
    }

}

boolean HUlib_delCharFromTextLine(hu_textline_t* t)
{

    if (!t->len) return false;
    else
    {
        t->l[--t->len] = 0;
        t->needsupdate = 4;
        return true;
    }

}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

void
HUlib_drawTextLine
( hu_textline_t*        l,
  boolean               drawcursor )
{

    int                 i;
    int                 w;
    int                 x;
    unsigned char       c;

    // draw the new stuff
    x = l->x;
    for (i=0;i<l->len;i++)
    {
        c = toupper(l->l[i]);
        if (c != ' '
            && c >= l->sc
            && c <= '_')
        {
            w = SHORT(l->f[c - l->sc]->width);
            if (x+w > ORIGWIDTH)    // CHANGED FOR HIRES
                break;
            V_DrawPatch(x, l->y, l->f[c - l->sc]);
            x += w;
        }
        else
        {
            x += 4;
            if (x >= ORIGWIDTH)                              // CHANGED FOR HIRES
                break;
        }
    }

    // draw the cursor if requested
    if (drawcursor                                           // CHANGED FOR HIRES
        && x + SHORT(l->f['_' - l->sc]->width) <= ORIGWIDTH) // CHANGED FOR HIRES
    {
        V_DrawPatch(x, l->y, l->f['_' - l->sc]);
    }
}


void
HUlib_initSText
( hu_stext_t*   s,
  int           x,
  int           y,
  int           h,
  patch_t**     font,
  int           startchar,
  boolean*      on )
{

    int i;

    s->h = h;
    s->on = on;
    s->laston = true;
    s->cl = 0;
    for (i=0;i<h;i++)
        HUlib_initTextLine(&s->l[i],
                           x, y - i*(SHORT(font[0]->height)+1),
                           font, startchar);

}

void HUlib_addLineToSText(hu_stext_t* s)
{

    int i;

    // add a clear line
    if (++s->cl == s->h)
        s->cl = 0;
    HUlib_clearTextLine(&s->l[s->cl]);

    // everything needs updating
    for (i=0 ; i<s->h ; i++)
        s->l[i].needsupdate = 4;

}

void
HUlib_addMessageToSText
( hu_stext_t*   s,
  char*         prefix,
  char*         msg )
{
    HUlib_addLineToSText(s);
    if (prefix)
        while (*prefix)
            HUlib_addCharToTextLine(&s->l[s->cl], *(prefix++));

    while (*msg)
        HUlib_addCharToTextLine(&s->l[s->cl], *(msg++));
}

void HUlib_drawSText(hu_stext_t* s)
{
    int i, idx;
    hu_textline_t *l;

    if (!*s->on)
        return; // if not on, don't draw

    // draw everything
    for (i=0 ; i<s->h ; i++)
    {
        idx = s->cl - i;
        if (idx < 0)
            idx += s->h; // handle queue of lines
        
        l = &s->l[idx];

        // need a decision made here on whether to skip the draw
        HUlib_drawTextLine(l, false); // no cursor, please
    }

}

void HUlib_eraseSText(hu_stext_t* s)
{

    int i;

    for (i=0 ; i<s->h ; i++)
    {
        if (s->laston && !*s->on)
            s->l[i].needsupdate = 4;
    }
    s->laston = *s->on;

}
/*
void
HUlib_initIText
( hu_itext_t*   it,
  int           x,
  int           y,
  patch_t**     font,
  int           startchar,
  boolean*      on )
{
    it->lm = 0; // default left margin is start of text
    it->on = on;
    it->laston = true;
    HUlib_initTextLine(&it->l, x, y, font, startchar);
}


// The following deletion routines adhere to the left margin restriction
void HUlib_delCharFromIText(hu_itext_t* it)
{
    if (it->l.len != it->lm)
        HUlib_delCharFromTextLine(&it->l);
}

void HUlib_eraseLineFromIText(hu_itext_t* it)
{
    while (it->lm != it->l.len)
        HUlib_delCharFromTextLine(&it->l);
}

// Resets left margin as well
void HUlib_resetIText(hu_itext_t* it)
{
    it->lm = 0;
    HUlib_clearTextLine(&it->l);
}

void
HUlib_addPrefixToIText
( hu_itext_t*   it,
  char*         str )
{
    while (*str)
        HUlib_addCharToTextLine(&it->l, *(str++));
    it->lm = it->l.len;
}

// wrapper function for handling general keyed input.
// returns true if it ate the key
boolean
HUlib_keyInIText
( hu_itext_t*   it,
  unsigned char ch )
{
    ch = toupper(ch);

    if (ch >= ' ' && ch <= '_') 
        HUlib_addCharToTextLine(&it->l, (char) ch);
    else 
        if (ch == KEY_BACKSPACE) 
            HUlib_delCharFromIText(it);
        else 
            if (ch != KEY_ENTER) 
                return false; // did not eat key

    return true; // ate the key

}

void HUlib_drawIText(hu_itext_t* it)
{

    hu_textline_t *l = &it->l;

    if (!*it->on)
        return;
    HUlib_drawTextLine(l, true); // draw the line w/ cursor

}

void HUlib_eraseIText(hu_itext_t* it)
{
    if (it->laston && !*it->on)
        it->l.needsupdate = 4;
    HUlib_eraseTextLine(&it->l);
    it->laston = *it->on;
}
*/
