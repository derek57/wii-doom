// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//        The status bar widget code.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: st_lib.c,v 1.4 1997/02/03 16:47:56 b1 Exp $";


#include <ctype.h>

#ifdef WII
#include "../d_deh.h"
#else
#include "d_deh.h"
#endif

#include "doomdef.h"
#include "doomstat.h"

#ifdef WII
#include "../i_swap.h"
#include "../i_system.h"
#else
#include "i_swap.h"
#include "i_system.h"
#endif

#include "r_local.h"
#include "st_lib.h"
#include "st_stuff.h"

#ifdef WII
#include "../v_patch.h"
#include "../v_video.h"
#include "../w_wad.h"
#include "../z_zone.h"
#else
#include "v_patch.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#endif


// in AM_map.c

//
// Hack display negative frags.
//  Loads and store the stminus lump.
//
patch_t*                sttminus;


void STlib_init(void)
{
    if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4274218 &&
            fsize != 4225504 && fsize != 4225460 && fsize != 4207819)
        sttminus = (patch_t *) W_CacheLumpName("STTMINUS", PU_STATIC);
}


// ?
void
STlib_initNum
( st_number_t*          n,
  int                   x,
  int                   y,
  patch_t**             pl,
  int*                  num,
  dboolean*              on,
  int                   width )
{
    n->x        = x;
    n->y        = y;
    n->oldnum   = 0;
    n->width    = width;
    n->num      = num;
    n->on       = on;
    n->p        = pl;
}


// 
// A fairly efficient way to draw a number
//  based on differences from the old number.
// Note: worth the trouble?
//
void
STlib_drawNum
( st_number_t*  n,
  dboolean       refresh )
{

    int         numdigits = n->width;
    int         num = *n->num;
    
    int         w = SHORT(n->p[0]->width);
    int         h = SHORT(n->p[0]->height);
    int         x = n->x;
    
    int         neg;

    n->oldnum = *n->num;

    neg = num < 0;

    if (neg)
    {
        if (numdigits == 2 && num < -9)
            num = -9;
        else if (numdigits == 3 && num < -99)
            num = -99;
        
        num = -num;
    }

    // clear the area
    x = n->x - numdigits*w;

    if (n->y - ST_Y < 0)
        I_Error("drawNum: n->y - ST_Y < 0");

    V_CopyRect(x, n->y - ST_Y, st_backing_screen, w*numdigits, h, x, n->y);

    // if non-number, do not draw it
    if (num == 1994)
        return;

    x = n->x;

    // in the special case of 0, you draw 0
    if (!num)
        V_DrawPatch(x - w, n->y, n->p[ 0 ]);

    // draw the new number
    while (num && numdigits--)
    {
        x -= w;
        V_DrawPatch(x, n->y, n->p[ num % 10 ]);
        num /= 10;
    }

    // draw a minus sign if necessary
    if (neg)
        V_DrawPatch(x - 8, n->y, sttminus);
}


//
void
STlib_updateNum
( st_number_t*          n,
  dboolean               refresh )
{
    if (*n->on) STlib_drawNum(n, refresh);
}


//
void
STlib_initPercent
( st_percent_t*         p,
  int                   x,
  int                   y,
  patch_t**             pl,
  int*                  num,
  dboolean*              on,
  patch_t*              percent )
{
    STlib_initNum(&p->n, x, y, pl, num, on, 3);
    p->p = percent;
}




void
STlib_updatePercent
( st_percent_t*         per,
  int                   refresh )
{
    if (refresh && *per->n.on)
        V_DrawPatch(per->n.x, per->n.y, per->p);
    
    STlib_updateNum(&per->n, refresh);
}



void
STlib_initMultIcon
( st_multicon_t*        i,
  int                   x,
  int                   y,
  patch_t**             il,
  int*                  inum,
  dboolean*              on )
{
    i->x        = x;
    i->y        = y;
    i->oldinum  = -1;
    i->inum     = inum;
    i->on       = on;
    i->p        = il;
}



void
STlib_updateMultIcon
( st_multicon_t*        mi,
  dboolean               refresh )
{
    if (*mi->on
        && (mi->oldinum != *mi->inum || refresh)
        && (*mi->inum!=-1))
    {
        if (mi->oldinum != -1)
        {
            int                 w = SHORT(mi->p[mi->oldinum]->width);
            int                 h = SHORT(mi->p[mi->oldinum]->height);
            int                 x = mi->x - SHORT(mi->p[mi->oldinum]->leftoffset);
            int                 y = mi->y - SHORT(mi->p[mi->oldinum]->topoffset);

            if (y - ST_Y < 0)
                I_Error("updateMultIcon: y - ST_Y < 0");

            V_CopyRect(x, y-ST_Y, st_backing_screen, w, h, x, y);
        }
        V_DrawPatch(mi->x, mi->y, mi->p[*mi->inum]);
        mi->oldinum = *mi->inum;
    }

    if(beta_style)
    {
        player_t *player = &players[consoleplayer];

        // "3" = standard lifes in the pre-beta version of the game
        if(3 + player->extra_lifes < 10)
        {
            int             i = 3 + player->extra_lifes;
            char            namebuf[9];

            sprintf(namebuf, "STYSNUM%d", i);
            V_DrawPatch(173, 192,
                    W_CacheLumpName(namebuf, PU_CACHE));
        }
    }
}



void
STlib_initBinIcon
( st_binicon_t*         b,
  int                   x,
  int                   y,
  patch_t*              i,
  dboolean*              val,
  dboolean*              on )
{
    b->x        = x;
    b->y        = y;
    b->oldval   = 0;
    b->val      = val;
    b->on       = on;
    b->p        = i;
}



void
STlib_updateBinIcon
( st_binicon_t*         bi,
  dboolean               refresh )
{
    if (*bi->on
        && (bi->oldval != *bi->val || refresh))
    {
        int x = bi->x - SHORT(bi->p->leftoffset);
        int y = bi->y - SHORT(bi->p->topoffset);
        int w = SHORT(bi->p->width);
        int h = SHORT(bi->p->height);

        if (y - ST_Y < 0)
            I_Error("updateBinIcon: y - ST_Y < 0");

        if (*bi->val)
            V_DrawPatch(bi->x, bi->y, bi->p);
        else
            V_CopyRect(x, y-ST_Y, st_backing_screen, w, h, x, y);

        bi->oldval = *bi->val;
    }

}

