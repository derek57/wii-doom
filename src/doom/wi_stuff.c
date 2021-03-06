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
//        Intermission screens.
//
//-----------------------------------------------------------------------------


#include <stdio.h>

#include "c_io.h"
#include "d_deh.h"
#include "doomstat.h"
#include "g_game.h"
#include "i_swap.h"
#include "i_system.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_setup.h"
#include "r_local.h"
#include "s_sound.h"

// Data.
#include "sounds.h"

#include "v_trans.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"
#include "wi_stuff.h"
#include "wii-doom.h"
#include "z_zone.h"


//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//

//UNUSED
// in tics
//#define PAUSELEN               (TICRATE * 2) 
//#define SCORESTEP              100
//#define ANIMPERIOD             32

// pixel distance from "(YOU)" to "PLAYER N"
//#define STARDIST               10 
//#define WK                     1

//
// Different vetween registered DOOM (1994) and
//  Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II), which had 34 maps
//  in one episode. So there.
#define NUMEPISODES            4
#define NUMMAPS                9

// GLOBAL LOCATIONS
#define WI_TITLEY              2
#define WI_SPACINGY            33

// SINGPLE-PLAYER STUFF
#define SP_STATSX              50
#define SP_STATSY              50

#define SP_TIMEX               16
#define SP_TIMEY               (ORIGINALHEIGHT - 32)

// NET GAME STUFF
#define NG_STATSY              50
#define NG_STATSX              (32 + SHORT(star->width) / 2 + 32 * !dofrags)

#define NG_SPACINGX            64

// DEATHMATCH STUFF
#define DM_MATRIXX             42
#define DM_MATRIXY             68

#define DM_SPACINGX            40

#define DM_TOTALSX             269

#define DM_KILLERSX            10
#define DM_KILLERSY            100
#define DM_VICTIMSX            5
#define DM_VICTIMSY            50

// States for single-player
#define SP_KILLS               0
#define SP_ITEMS               2
#define SP_SECRET              4
#define SP_FRAGS               6 
#define SP_TIME                8 
#define SP_PAR                 ST_TIME

#define SP_PAUSE               1

// in seconds
#define SHOWNEXTLOCDELAY       4

//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
#define ANIM(type, period, nanims, x, y, nexttic)            \
   { (type), (period), (nanims), { (x), (y) }, (nexttic),    \
     0, { NULL, NULL, NULL }, 0, 0, 0, 0 }


typedef void (*load_callback_t)(char *lumpname, patch_t **variable);

typedef enum
{
    ANIM_ALWAYS,
    ANIM_RANDOM,
    ANIM_LEVEL

} animenum_t;

typedef struct
{
    int      x;
    int      y;
    
} point_t;


//
// Animation.
// There is another anim_t used in p_spec.
//
typedef struct
{
    animenum_t        type;

    // period in tics between animations
    int               period;

    // number of animation frames
    int               nanims;

    // location of animation
    point_t           loc;

    // ALWAYS: n/a,
    // RANDOM: period deviation (< 256),
    // LEVEL: level
    int               data1;

    // ALWAYS: n/a,
    // RANDOM: random base period,
    // LEVEL: n/a
    int               data2; 

    // actual graphics for frames of animations
    patch_t           *p[3]; 

    // following must be initialized to zero before use!

    // next value of bcnt (used in conjunction with period)
    int               nexttic;

    // last drawn animation frame
    int               lastdrawn;

    // next frame number to animate
    int               ctr;
    
    // used by RANDOM and LEVEL when animating
    int               state;  

} anim_t;


static point_t lnodes[NUMEPISODES][NUMMAPS] =
{
    // Episode 0 World Map
    {
        { 185, 164 },        // location of level 0 (CJ)
        { 148, 143 },        // location of level 1 (CJ)
        { 69, 122  },        // location of level 2 (CJ)
        { 209, 102 },        // location of level 3 (CJ)
        { 116, 89  },        // location of level 4 (CJ)
        { 166, 55  },        // location of level 5 (CJ)
        { 71, 56   },        // location of level 6 (CJ)
        { 135, 29  },        // location of level 7 (CJ)
        { 71, 24   }         // location of level 8 (CJ)
    },

    // Episode 1 World Map should go here
    {
        { 254, 25  },        // location of level 0 (CJ)
        { 97, 50   },        // location of level 1 (CJ)
        { 188, 64  },        // location of level 2 (CJ)
        { 128, 78  },        // location of level 3 (CJ)
        { 214, 92  },        // location of level 4 (CJ)
        { 133, 130 },        // location of level 5 (CJ)
        { 208, 136 },        // location of level 6 (CJ)
        { 148, 140 },        // location of level 7 (CJ)
        { 235, 158 }         // location of level 8 (CJ)
    },

    // Episode 2 World Map should go here
    {
        { 156, 168 },        // location of level 0 (CJ)
        { 48, 154  },        // location of level 1 (CJ)
        { 174, 95  },        // location of level 2 (CJ)
        { 265, 75  },        // location of level 3 (CJ)
        { 130, 48  },        // location of level 4 (CJ)
        { 279, 23  },        // location of level 5 (CJ)
        { 198, 48  },        // location of level 6 (CJ)
        { 140, 25  },        // location of level 7 (CJ)
        { 281, 136 }         // location of level 8 (CJ)
    }

};


static anim_t epsd0animinfo[] =
{
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 224, 104, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 184, 160, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 112, 136, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3,  72, 112, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3,  88,  96, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3,  64,  48, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 192,  40, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 136,  16, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3,  80,  16, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3,  64,  24, 0)
};

static anim_t epsd1animinfo[] =
{
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 1),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 2),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 3),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 4),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 5),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 6),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 7),
    ANIM(ANIM_LEVEL, TICRATE / 3, 3, 192, 144, 8),
    ANIM(ANIM_LEVEL, TICRATE / 3, 1, 128, 136, 8)
};

static anim_t epsd2animinfo[] =
{
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 104, 168, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3,  40, 136, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 160,  96, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 104,  80, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 3, 3, 120,  32, 0),
    ANIM(ANIM_ALWAYS, TICRATE / 4, 3,  40,   0, 0)
};

static int NUMANIMS[NUMEPISODES] =
{
    arrlen(epsd0animinfo),
    arrlen(epsd1animinfo),
    arrlen(epsd2animinfo)
};

static anim_t *anims[NUMEPISODES] =
{
    epsd0animinfo,
    epsd1animinfo,
    epsd2animinfo
};


//
// GENERAL DATA
//

//
// Locally used stuff.
//

// specifies current state
static stateenum_t             state;

// contains information passed into intermission
static wbstartstruct_t         *wbs;

static wbplayerstruct_t        *plrs;

// used for general timing
static int                     cnt;  

// used for timing of background animation
static int                     bcnt;

// signals to refresh everything for one frame
static int                     firstrefresh; 

static int                     cnt_bonus;
static int                     cnt_score;

static int                     cnt_kills[MAXPLAYERS];
static int                     cnt_items[MAXPLAYERS];
static int                     cnt_secret[MAXPLAYERS];
static int                     cnt_time;
static int                     cnt_par;
static int                     cnt_pause;
static int                     dm_state;
static int                     dm_frags[MAXPLAYERS][MAXPLAYERS];
static int                     dm_totals[MAXPLAYERS];
static int                     cnt_frags[MAXPLAYERS];
static int                     dofrags;
static int                     ng_state;
static int                     sp_state;


// # of commercial levels
static int                     NUMCMAPS; 

// used to accelerate or skip a stage
static int                     acceleratestage;

// wbs->pnum
static int                     me;


//
//        GRAPHICS
//

// You Are Here graphic
static patch_t                 *yah[3]; 

// You Are Here graphic (BETA)
static patch_t                 *byah[3]; 

// splat
static patch_t                 *splat[2];

// %, : graphics
static patch_t                 *percent;
static patch_t                 *colon;

// 0-9 graphic
static patch_t                 *num[10];

// minus sign
static patch_t                 *wiminus;

// "Finished!" graphics
static patch_t                 *finished;

// "Entering" graphic
static patch_t                 *entering; 

// "secret"
static patch_t                 *sp_secret;

 // "Kills", "Scrt", "Items", "Frags"
static patch_t                 *kills;
static patch_t                 *secret;
static patch_t                 *items;
static patch_t                 *frags;
static patch_t                 *bonus;
static patch_t                 *score;

// Time sucks.
static patch_t                 *timepatch;
static patch_t                 *par;
static patch_t                 *sucks;

// "killers", "victims"
static patch_t                 *killers;
static patch_t                 *victims; 

// "Total", your face, your dead face
static patch_t                 *total;
static patch_t                 *star;
static patch_t                 *bstar;

// "red P[1..MAXPLAYERS]"
static patch_t                 *p[MAXPLAYERS];

// "gray P[1..MAXPLAYERS]"
static patch_t                 *bp[MAXPLAYERS];

 // Name graphics of each level (centered)
static patch_t                 **lnames;

// Buffer storing the backdrop
static patch_t                 *background;

static dboolean                snl_pointeron;

extern dboolean                secretexit;
extern dboolean                opl;


//
// CODE
//

// slam background
void WI_slamBackground(int srcscrn, int destscrn)
{
    V_DrawPatch(0, 0, 0, background);

    /*
    // THIS IS WORKING AS IT SHOULD BUT IN THE REAL PR BETA, IT WASN'T IMPLEMENTED
    if (beta_style && gameepisode == 1)
        V_DrawPatch(232, 168, 0, W_CacheLumpName("WILVBX", PU_CACHE));
    */

//  ???
//    memcpy(screens[destscrn], screens[srcscrn], SCREENWIDTH * SCREENHEIGHT);
}

// The ticker is used to detect keys
//  because of timing issues in netgames.
//
// [nitr8] UNUSED
//
/*
dboolean WI_Responder(event_t *ev)
{
    return false;
}
*/

void WI_drawOnLnode(int n, patch_t *c[])
{
    int           i = 0;
    dboolean      fits = false;

    do
    {
        int left = lnodes[wbs->epsd][n].x - SHORT(c[i]->leftoffset);
        int top = lnodes[wbs->epsd][n].y - SHORT(c[i]->topoffset);
        int right = left + SHORT(c[i]->width);
        int bottom = top + SHORT(c[i]->height);

        if (left >= 0 && right < ORIGINALWIDTH
            && top >= 0 && bottom < ORIGINALHEIGHT)
        {
            fits = true;
        }
        else
        {
            i++;
        }

    } while (!fits && i != 2 && c[i] != NULL);

    if (fits && i < 2)
    {
        if (font_shadow == 1)
            V_DrawPatchWithShadow(lnodes[wbs->epsd][n].x,
                    lnodes[wbs->epsd][n].y, 0,
                    c[i], false);
        else
            V_DrawPatch(lnodes[wbs->epsd][n].x,
                    lnodes[wbs->epsd][n].y, 0,
                    c[i]);
    }
    else
    {
        // DEBUG
        C_Error("Could not place patch on level %d", n + 1); 
    }
}

// Draws "<Levelname> Finished!"
void WI_drawLF(void)
{
    int y = WI_TITLEY;
    int titlepatch = P_GetMapTitlePatch(wbs->epsd * 10 + wbs->last + 1); 

    if (titlepatch)
    {
        patch_t *patch = W_CacheLumpNum(titlepatch, PU_STATIC);

        if (font_shadow == 1)
            V_DrawPatchWithShadow((ORIGINALWIDTH - SHORT(patch->width)) / 2 + 1, y + 1, 0, patch, false);
        else
            V_DrawPatch((ORIGINALWIDTH - SHORT(patch->width)) / 2 + 1, y + 1, 0, patch);

        y += SHORT(patch->height) + 2;
    }
    else
    {
        if (gamemode != commercial || wbs->last < NUMCMAPS)
        {
            // draw <LevelName> 
            /*
            if (beta_style && gameepisode == 1 && gamemap < 10)
            {
                // THIS IS WORKING AS IT SHOULD BUT IN THE REAL PR BETA, IT WASN'T IMPLEMENTED
                if (fsize != 12538385 || gamemap != 10)
                {
                    if (font_shadow == 1)
                        V_DrawPatchWithShadow(232, 176, 0, lnames[wbs->last], false);
                    else
                        V_DrawPatch(232, 176, 0, lnames[wbs->last]);
                }
            }
            else
            */
            {
                if (fsize != 12538385 || gamemap != 10)
                {
                    if (font_shadow == 1)
                        V_DrawPatchWithShadow((ORIGINALWIDTH -
                                SHORT(lnames[wbs->last]->width)) / 2,
                                y, 0, lnames[wbs->last], false);
                    else
                        V_DrawPatch((ORIGINALWIDTH -
                                SHORT(lnames[wbs->last]->width)) / 2,
                                y, 0, lnames[wbs->last]);
                }
                else
                {
                    if (font_shadow == 1)
                        V_DrawPatchWithShadow(117, y, 0, W_CacheLumpName("SEWERS", PU_CACHE), false);
                    else
                        V_DrawPatch (117, y, 0, W_CacheLumpName("SEWERS", PU_CACHE));
                }
            }

            // draw "Finished!"
            if ((fsize != 12538385 || gamemap != 10) && !beta_style)
                y += (5 * SHORT(lnames[wbs->last]->height))/4;
            else
            {
                if (beta_style)
                    y = 9;
                else
                    y = 17;
            }
    
            if (font_shadow == 1)
                V_DrawPatchWithShadow((ORIGINALWIDTH - SHORT(finished->width)) / 2,
                        y, 0, finished, false);
            else
                V_DrawPatch((ORIGINALWIDTH - SHORT(finished->width)) / 2,
                        y, 0, finished);
        }
        else if (wbs->last == NUMCMAPS)
        {
            // MAP33 - nothing is displayed!
        }
        else if (wbs->last > NUMCMAPS)
        {
            // > MAP33.  Doom bombs out here with a Bad V_DrawPatch error.
            // I'm pretty sure that doom2.exe is just reading into random
            // bits of memory at this point, but let's try to be accurate
            // anyway.  This deliberately triggers a V_DrawPatch error.
            patch_t tmp = { ORIGINALWIDTH, ORIGINALHEIGHT, 1, 1,
                            { 0, 0, 0, 0, 0, 0, 0, 0 } };

            if (font_shadow == 1)
                V_DrawPatchWithShadow(0, y, 0, &tmp, false);
            else
                V_DrawPatch(0, y, 0, &tmp);
        }
    }

    // draw a splat on taken cities.
    if (beta_style)
    {
        int i;
        int last = gamemap - 1;

        for (i = 1; i <= last; i++)
            WI_drawOnLnode(i, splat);
    }
}


// Draws "Entering <LevelName>"
void WI_drawEL(void)
{
    int y = WI_TITLEY;
    int titlepatch = P_GetMapTitlePatch(wbs->epsd * 10 + wbs->next + 1);

    // draw "Entering"
    if (titlepatch)
    {
        patch_t *patch = W_CacheLumpNum(titlepatch, PU_STATIC);

        if (font_shadow == 1)
            V_DrawPatchWithShadow((ORIGINALWIDTH - SHORT(patch->width)) / 2 + 1, y + 1, 0, patch, false);
        else
            V_DrawPatch((ORIGINALWIDTH - SHORT(patch->width)) / 2 + 1, y + 1, 0, patch);
    }
    else
    {
        if (!beta_style) 
        {
            if (font_shadow == 1)
                V_DrawPatchWithShadow((ORIGINALWIDTH - SHORT(entering->width)) / 2,
                        y, 0,
                        entering, false);
            else
                V_DrawPatch((ORIGINALWIDTH - SHORT(entering->width)) / 2,
                        y, 0,
                        entering);
        }

        // draw level
        /*
        if (beta_style && gameepisode == 1 && gamemap < 10)
        {
            // THIS IS WORKING AS IT SHOULD BUT IN THE REAL PR BETA, IT WASN'T IMPLEMENTED
            if (font_shadow == 1)
                V_DrawPatchWithShadow(232, 176, 0, lnames[wbs->next], false);
            else
                V_DrawPatch(232, 176, 0, lnames[wbs->next]);
        }
        else
        */
        {
            if (fsize == 12538385 && gamemap == 1 && secretexit)
                y = 17;
            else
                y += (5 * SHORT(lnames[wbs->next]->height)) / 4;
    
            if ((fsize == 14683458 || fsize == 14677988 || fsize == 14691821) &&
                        gamemode == commercial && gamemap == 2 && secretexit)
            {
                if (font_shadow == 1)
                    V_DrawPatchWithShadow(119, y + 1, 0, W_CacheLumpName("CWILV32", PU_CACHE), false);
                else
                    V_DrawPatch(119, y + 1, 0, W_CacheLumpName("CWILV32", PU_CACHE));
            }
            else if (fsize == 12538385 && gamemode == retail && gameepisode == 1 &&
                    gamemap == 1 && secretexit)
            {
                if (font_shadow == 1)
                    V_DrawPatchWithShadow(117, y, 0, W_CacheLumpName("SEWERS", PU_CACHE), false);
                else
                    V_DrawPatch(117, y, 0, W_CacheLumpName("SEWERS", PU_CACHE));
            }
            else
            {
                if (font_shadow == 1)
                    V_DrawPatchWithShadow((ORIGINALWIDTH - SHORT(lnames[wbs->next]->width)) / 2,
                        y, 0, lnames[wbs->next], false);
                else
                    V_DrawPatch((ORIGINALWIDTH - SHORT(lnames[wbs->next]->width)) / 2,
                            y, 0, lnames[wbs->next]);
            }
        }
    }
}

void WI_initAnimatedBack(void)
{
    int           i;

    if (gamemode == commercial)
        return;

    if (wbs->epsd > 2)
        return;

    for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
    {
        anim_t *a = &anims[wbs->epsd][i];

        // init variables
        a->ctr = -1;

        // specify the next time to draw it
        if (a->type == ANIM_ALWAYS)
            a->nexttic = bcnt + 1 + (M_Random() % a->period);
        else if (a->type == ANIM_RANDOM)
            a->nexttic = bcnt + 1 + a->data2 + (M_Random() % a->data1);
        else if (a->type == ANIM_LEVEL)
            a->nexttic = bcnt + 1;
    }
}

void WI_updateAnimatedBack(void)
{
    int           i;

    if (gamemode == commercial)
        return;

    if (wbs->epsd > 2)
        return;

    for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
    {
        anim_t *a = &anims[wbs->epsd][i];

        if (bcnt == a->nexttic)
        {
            switch (a->type)
            {
                case ANIM_ALWAYS:
                    if (++a->ctr >= a->nanims)
                        a->ctr = 0;

                    a->nexttic = bcnt + a->period;
                    break;

                case ANIM_RANDOM:
                    a->ctr++;

                    if (a->ctr == a->nanims)
                    {
                        a->ctr = -1;
                        a->nexttic = bcnt + a->data2 + (M_Random() % a->data1);
                    }
                    else
                        a->nexttic = bcnt + a->period;

                    break;
                
                case ANIM_LEVEL:
                    // gawd-awful hack for level anims
                    if (!(state == StatCount && i == 7)
                        && wbs->next == a->data1)
                    {
                        a->ctr++;

                        if (a->ctr == a->nanims)
                            a->ctr--;

                        a->nexttic = bcnt + a->period;
                    }

                    break;
            }
        }
    }
}

void WI_drawAnimatedBack(void)
{
    int           i;

    if (gamemode == commercial)
        return;

    if (wbs->epsd > 2)
        return;

    for (i = 0; i < NUMANIMS[wbs->epsd]; i++)
    {
        anim_t *a = &anims[wbs->epsd][i];

        if (a->ctr >= 0)
            V_DrawPatch(a->loc.x, a->loc.y, 0, a->p[a->ctr]);
    }
}

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//
int WI_drawNum(int x, int y, int n, int digits)
{
    int           fontwidth = SHORT(num[0]->width);
    int           neg;

    if (digits < 0)
    {
        if (!n)
        {
            // make variable-length zeros 1 digit long
            digits = 1;
        }
        else
        {
            int temp = n;

            // figure out # of digits in #
            digits = 0;

            while (temp)
            {
                temp /= 10;
                digits++;
            }
        }
    }

    neg = n < 0;

    if (neg)
        n = -n;

    // if non-number, do not draw it
    if (n == 1994)
        return 0;

    // draw the new number
    while (digits--)
    {
        x -= fontwidth;

        if (font_shadow == 1)
            V_DrawPatchWithShadow(x, y, 0, num[n % 10], false);
        else
            V_DrawPatch(x, y, 0, num[n % 10]);

        n /= 10;
    }

    // draw a minus sign if necessary
    if (neg)
    {
        if (font_shadow == 1)
            V_DrawPatchWithShadow(x -= 8, y, 0, wiminus, false);
        else
            V_DrawPatch(x -= 8, y, 0, wiminus);
    }

    return x;
}

void WI_drawPercent(int x, int y, int p)
{
    if (p < 0)
        return;

    if (font_shadow == 1)
        V_DrawPatchWithShadow(x, y, 0, percent, false);
    else
        V_DrawPatch(x, y, 0, percent);

    WI_drawNum(x, y, p, -1);
}

//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
void WI_drawTime(int x, int y, int t)
{
    if (t < 0)
        return;

    if (t <= 61 * 59)
    {
        int div = 1;

        do
        {
            x = WI_drawNum(x, y, (t / div) % 60, 2) - SHORT(colon->width);
            div *= 60;

            // draw
            if (div == 60 || t / div)
            {
                if (font_shadow == 1)
                    V_DrawPatchWithShadow(x, y, 0, colon, true);
                else
                    V_DrawPatch(x, y, 0, colon);
            }

        } while (t / div);
    }
    else
    {
        // "sucks"
        if (font_shadow == 1)
            V_DrawPatchWithShadow(x - SHORT(sucks->width), y, 0, sucks, false); 
        else
            V_DrawPatch(x - SHORT(sucks->width), y, 0, sucks); 
    }
}

//
// Display level completion bonus and score,
//  or "sucks" message if overflow.
//
void WI_drawExtra(int x, int y, int t)
{
    if (t < 0)
        return;

    do
        x = WI_drawNum(x, y, t, 3);
    while (t);
}

void WI_initNoState(void)
{
    state = NoState;
    acceleratestage = 0;
    cnt = 10;
}

void WI_updateNoState(void)
{
    WI_updateAnimatedBack();

    if (!--cnt)
    {
        // Don't call WI_End yet.  G_WorldDone doesnt immediately 
        // change gamestate, so WI_Drawer is still going to get
        // run until that happens.  If we do that after WI_End
        // (which unloads all the graphics), we're in trouble.
        //WI_End();
        G_WorldDone();
    }
}

void WI_initShowNextLoc(void)
{
    state = ShowNextLoc;
    acceleratestage = 0;
    cnt = SHOWNEXTLOCDELAY * TICRATE;

    WI_initAnimatedBack();
}

void WI_updateShowNextLoc(void)
{
    WI_updateAnimatedBack();

    if (!--cnt || acceleratestage)
        WI_initNoState();
    else
        snl_pointeron = (cnt & 31) < 20;
}

void WI_drawShowNextLoc(void)
{
    WI_slamBackground(1, 0);

    // draw animated background
    WI_drawAnimatedBack(); 

    if (gamemode != commercial)
    {
        int           last;

        if (wbs->epsd > 2)
        {
            WI_drawEL();
            return;
        }

        if (fsize == 12538385 && gamemap == 10)
            last = (wbs->last == 9) ? wbs->next - 1 : wbs->last;
        else
            last = (wbs->last == 8) ? wbs->next - 1 : wbs->last;

        // draw a splat on taken cities.
        if (!beta_style)
        {
            int i;

            for (i = 0; i <= last; i++)
                WI_drawOnLnode(i, splat);
        }

        // splat the secret level?
        if (wbs->didsecret)
        {
            extern dboolean secret_1;
            extern dboolean secret_2;

            if (secret_2)
                WI_drawOnLnode(8, splat);

            if (fsize == 12538385 && secret_1 && gameepisode == 1)
                WI_drawOnLnode(9, splat);
        }

        // draw flashing ptr
        if (snl_pointeron && !beta_style)
        {
            WI_drawOnLnode(wbs->next, yah); 
        }
        else if (beta_style)
            WI_drawOnLnode(wbs->next, byah); 
    }

    if ((
        (gamemission == pack_nerve && wbs->last == 7) ||
        (gamemission == pack_master && wbs->last == 19 && !secretexit) ||
        (gamemission == pack_master && wbs->last == 20)))
        return;

    // draws which level you are entering..
    if ((gamemode != commercial) || wbs->next != 30)
        WI_drawEL();  
}

void WI_drawNoState(void)
{
    snl_pointeron = true;
    WI_drawShowNextLoc();
}

int WI_fragSum(int playernum)
{
    int           i;
    int           frags = 0;
    
    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i] && i!=playernum)
        {
            frags += plrs[playernum].frags[i];
        }
    }
        
    // JDC hack - negative frags.
    frags -= plrs[playernum].frags[playernum];

    // UNUSED if (frags < 0)
    //         frags = 0;

    return frags;
}

void WI_initDeathmatchStats(void)
{
    int           i;
    int           j;

    state = StatCount;
    acceleratestage = 0;
    dm_state = 1;

    cnt_pause = TICRATE;

    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            for (j = 0; j < MAXPLAYERS; j++)
                if (playeringame[j])
                    dm_frags[i][j] = 0;

            dm_totals[i] = 0;
        }
    }
    
    WI_initAnimatedBack();
}

void WI_updateDeathmatchStats(void)
{
    int           i;
    int           j;
    
    WI_updateAnimatedBack();

    if (acceleratestage && dm_state != 4)
    {
        acceleratestage = 0;

        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (playeringame[i])
            {
                for (j = 0; j < MAXPLAYERS; j++)
                    if (playeringame[j])
                        dm_frags[i][j] = plrs[i].frags[j];

                dm_totals[i] = WI_fragSum(i);
            }
        }
        
        S_StartSound(0, sfx_barexp);
        dm_state = 4;
    }
    
    if (dm_state == 2)
    {
        dboolean stillticking;

        if (!(bcnt & 3))
            S_StartSound(0, sfx_pistol);
        
        stillticking = false;

        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (playeringame[i])
            {
                for (j = 0; j < MAXPLAYERS; j++)
                {
                    if (playeringame[j]
                        && dm_frags[i][j] != plrs[i].frags[j])
                    {
                        if (plrs[i].frags[j] < 0)
                            dm_frags[i][j]--;
                        else
                            dm_frags[i][j]++;

                        if (dm_frags[i][j] > 99)
                            dm_frags[i][j] = 99;

                        if (dm_frags[i][j] < -99)
                            dm_frags[i][j] = -99;
                        
                        stillticking = true;
                    }
                }

                dm_totals[i] = WI_fragSum(i);

                if (dm_totals[i] > 99)
                    dm_totals[i] = 99;
                
                if (dm_totals[i] < -99)
                    dm_totals[i] = -99;
            }
        }

        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            dm_state++;
        }
    }
    else if (dm_state == 4)
    {
        if (acceleratestage)
        {
            S_StartSound(0, sfx_slop);

            if (gamemode == commercial)
                WI_initNoState();
            else
                WI_initShowNextLoc();
        }
    }
    else if (dm_state & 1)
    {
        if (!--cnt_pause)
        {
            dm_state++;
            cnt_pause = TICRATE;
        }
    }
}

void WI_drawDeathmatchStats(void)
{
    int           i;
    int           j;
    int           x;
    int           y;
    int           w;

    WI_slamBackground(1, 0);
    
    // draw animated background
    WI_drawAnimatedBack(); 
    WI_drawLF();

    // draw stat titles (top line)
    if (font_shadow == 1)
    {
        V_DrawPatchWithShadow(DM_TOTALSX - SHORT(total->width) / 2,
                DM_MATRIXY - WI_SPACINGY + 10, 0,
                total, false);
    
        V_DrawPatchWithShadow(DM_KILLERSX, DM_KILLERSY, 0, killers, false);
        V_DrawPatchWithShadow(DM_VICTIMSX, DM_VICTIMSY, 0, victims, false);
    }
    else
    {
        V_DrawPatch(DM_TOTALSX - SHORT(total->width) / 2,
                DM_MATRIXY - WI_SPACINGY + 10, 0,
                total);
    
        V_DrawPatch(DM_KILLERSX, DM_KILLERSY, 0, killers);
        V_DrawPatch(DM_VICTIMSX, DM_VICTIMSY, 0, victims);
    }

    // draw P?
    x = DM_MATRIXX + DM_SPACINGX;
    y = DM_MATRIXY;

    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (playeringame[i])
        {
            if (font_shadow == 1)
            {
                V_DrawPatchWithShadow(x - SHORT(p[i]->width) / 2,
                        DM_MATRIXY - WI_SPACINGY, 0,
                        p[i], false);
            
                V_DrawPatchWithShadow(DM_MATRIXX - SHORT(p[i]->width) / 2,
                        y, 0,
                        p[i], false);
            }
            else
            {
                V_DrawPatch(x - SHORT(p[i]->width) / 2,
                        DM_MATRIXY - WI_SPACINGY, 0,
                        p[i]);
            
                V_DrawPatch(DM_MATRIXX - SHORT(p[i]->width) / 2,
                        y, 0,
                        p[i]);
            }

            if (i == me)
            {
                if (font_shadow == 1)
                {
                    V_DrawPatchWithShadow(x - SHORT(p[i]->width) / 2,
                            DM_MATRIXY - WI_SPACINGY, 0,
                            bstar, false);

                    V_DrawPatchWithShadow(DM_MATRIXX - SHORT(p[i]->width) / 2,
                            y, 0,
                            star, false);
                }
                else
                {
                    V_DrawPatch(x - SHORT(p[i]->width) / 2,
                            DM_MATRIXY - WI_SPACINGY, 0,
                            bstar);

                    V_DrawPatch(DM_MATRIXX - SHORT(p[i]->width) / 2,
                            y, 0,
                            star);
                }
            }
        }
        else
        {
            // V_DrawPatch(x - SHORT(bp[i]->width) / 2,
            //   DM_MATRIXY - WI_SPACINGY, 0, bp[i]);
            // V_DrawPatch(DM_MATRIXX - SHORT(bp[i]->width) / 2,
            //   y, 0, bp[i]);
        }

        x += DM_SPACINGX;
        y += WI_SPACINGY;
    }

    // draw stats
    y = DM_MATRIXY + 10;
    w = SHORT(num[0]->width);

    for (i = 0; i < MAXPLAYERS; i++)
    {
        x = DM_MATRIXX + DM_SPACINGX;

        if (playeringame[i])
        {
            for (j = 0; j < MAXPLAYERS; j++)
            {
                if (playeringame[j])
                    WI_drawNum(x + w, y, dm_frags[i][j], 2);

                x += DM_SPACINGX;
            }

            WI_drawNum(DM_TOTALSX + w, y, dm_totals[i], 2);
        }

        y += WI_SPACINGY;
    }
}

void WI_initNetgameStats(void)
{
    int i;

    state = StatCount;
    acceleratestage = 0;
    ng_state = 1;

    cnt_pause = TICRATE;

    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (!playeringame[i])
            continue;

        cnt_kills[i] = cnt_items[i] = cnt_secret[i] = cnt_frags[i] = 0;

        dofrags += WI_fragSum(i);
    }

    dofrags = !!dofrags;

    WI_initAnimatedBack();
}

void WI_updateNetgameStats(void)
{
    int           i;
    int           fsum;
    
    dboolean       stillticking;

    WI_updateAnimatedBack();

    if (acceleratestage && ng_state != 10)
    {
        acceleratestage = 0;

        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
            cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
            cnt_secret[i] = (plrs[i].ssecret * 100) / wbs->maxsecret;

            if (dofrags)
                cnt_frags[i] = WI_fragSum(i);
        }

        S_StartSound(0, sfx_barexp);
        ng_state = 10;
    }

    if (ng_state == 2)
    {
        if (!(bcnt & 3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_kills[i] += 2;

            if (cnt_kills[i] >= (plrs[i].skills * 100) / wbs->maxkills)
                cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
            else
                stillticking = true;
        }
        
        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            ng_state++;
        }
    }
    else if (ng_state == 4)
    {
        if (!(bcnt & 3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_items[i] += 2;

            if (cnt_items[i] >= (plrs[i].sitems * 100) / wbs->maxitems)
                cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            ng_state++;
        }
    }
    else if (ng_state == 6)
    {
        if (!(bcnt & 3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_secret[i] += 2;

            if (cnt_secret[i] >= (plrs[i].ssecret * 100) / wbs->maxsecret)
                cnt_secret[i] = (plrs[i].ssecret * 100) / wbs->maxsecret;
            else
                stillticking = true;
        }
        
        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            ng_state += 1 + 2 * !dofrags;
        }
    }
    else if (ng_state == 8)
    {
        if (!(bcnt & 3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i = 0; i < MAXPLAYERS; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_frags[i] += 1;

            if (cnt_frags[i] >= (fsum = WI_fragSum(i)))
                cnt_frags[i] = fsum;
            else
                stillticking = true;
        }
        
        if (!stillticking)
        {
            S_StartSound(0, sfx_pldeth);
            ng_state++;
        }
    }
    else if (ng_state == 10)
    {
        if (acceleratestage)
        {
            S_StartSound(0, sfx_sgcock);

            if (gamemode == commercial)
                WI_initNoState();
            else
                WI_initShowNextLoc();
        }
    }
    else if (ng_state & 1)
    {
        if (!--cnt_pause)
        {
            ng_state++;
            cnt_pause = TICRATE;
        }
    }
}

void WI_drawNetgameStats(void)
{
    int           i;
    int           x;
    int           y;
    int           pwidth = SHORT(percent->width);

    WI_slamBackground(1, 0);
    
    // draw animated background
    WI_drawAnimatedBack(); 

    WI_drawLF();

    // draw stat titles (top line)
    if (font_shadow == 1)
    {
        V_DrawPatchWithShadow(NG_STATSX + NG_SPACINGX - SHORT(kills->width),
                NG_STATSY, 0, kills, false);

        V_DrawPatchWithShadow(NG_STATSX + 2 * NG_SPACINGX - SHORT(items->width),
                NG_STATSY, 0, items, false);

        V_DrawPatchWithShadow(NG_STATSX + 3 * NG_SPACINGX - SHORT(secret->width),
                NG_STATSY, 0, secret, false);
    }
    else
    {
        V_DrawPatch(NG_STATSX + NG_SPACINGX - SHORT(kills->width),
                NG_STATSY, 0, kills);

        V_DrawPatch(NG_STATSX + 2 * NG_SPACINGX - SHORT(items->width),
                NG_STATSY, 0, items);

        V_DrawPatch(NG_STATSX + 3 * NG_SPACINGX - SHORT(secret->width),
                NG_STATSY, 0, secret);
    }

    if (dofrags)
    {
        if (font_shadow == 1)
            V_DrawPatchWithShadow(NG_STATSX + 4 * NG_SPACINGX - SHORT(frags->width),
                    NG_STATSY, 0, frags, false);
        else
            V_DrawPatch(NG_STATSX + 4 * NG_SPACINGX - SHORT(frags->width),
                    NG_STATSY, 0, frags);
    }

    // draw stats
    y = NG_STATSY + SHORT(kills->height);

    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (!playeringame[i])
            continue;

        x = NG_STATSX;

        if (font_shadow == 1)
            V_DrawPatchWithShadow(x - SHORT(p[i]->width), y, 0, p[i], false);
        else
            V_DrawPatch(x - SHORT(p[i]->width), y, 0, p[i]);

        if (i == me)
        {
            if (font_shadow == 1)
                V_DrawPatchWithShadow(x - SHORT(p[i]->width), y, 0, star, false);
            else
                V_DrawPatch(x - SHORT(p[i]->width), y, 0, star);
        }

        x += NG_SPACINGX;
        WI_drawPercent(x - pwidth, y + 10, cnt_kills[i]);

        x += NG_SPACINGX;
        WI_drawPercent(x - pwidth, y + 10, cnt_items[i]);

        x += NG_SPACINGX;
        WI_drawPercent(x - pwidth, y + 10, cnt_secret[i]);

        x += NG_SPACINGX;

        if (dofrags)
            WI_drawNum(x, y + 10, cnt_frags[i], -1);

        y += WI_SPACINGY;
    }
}

void WI_initStats(void)
{
    state = StatCount;
    acceleratestage = 0;
    sp_state = 1;
    cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
    cnt_time = cnt_par = cnt_bonus = cnt_score = -1;

    if (beta_style)
        cnt_pause = (TICRATE / 2);
    else
        cnt_pause = TICRATE;

    WI_initAnimatedBack();
}

void WI_updateStats(void)
{
    WI_updateAnimatedBack();

    if (acceleratestage && sp_state != 10)
    {
        acceleratestage = 0;
        cnt_bonus = cnt_score = 500;
        cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
        cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
        cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
        cnt_time = (int)plrs[me].stime / TICRATE;
        cnt_par = (int)wbs->partime / TICRATE;
        S_StartSound(0, sfx_barexp);
        sp_state = 10;
    }

    if (sp_state == 2)
    {
        cnt_kills[0] += 2;

        if (!(bcnt & 3))
            S_StartSound(0, sfx_pistol);

        if (cnt_kills[0] >= (plrs[me].skills * 100) / wbs->maxkills)
        {
            cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
            S_StartSound(0, sfx_barexp);
            sp_state++;
        }
    }
    else if (sp_state == 4)
    {
        cnt_items[0] += 2;

        if (!(bcnt & 3))
            S_StartSound(0, sfx_pistol);

        if (cnt_items[0] >= (plrs[me].sitems * 100) / wbs->maxitems)
        {
            cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
            S_StartSound(0, sfx_barexp);
            sp_state++;
        }
    }
    else if (sp_state == 6)
    {
        cnt_secret[0] += 2;

        if (!(bcnt & 3))
            S_StartSound(0, sfx_pistol);

        if (cnt_secret[0] >= (plrs[me].ssecret * 100) / wbs->maxsecret)
        {
            cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
            S_StartSound(0, sfx_barexp);
            sp_state++;
        }
    }

    else if (sp_state == 8)
    {
        if (!(bcnt & 3))
            S_StartSound(0, sfx_pistol);

        cnt_time += 3;

        if (cnt_time >= plrs[me].stime / TICRATE)
            cnt_time = plrs[me].stime / TICRATE;

        cnt_par += 3;

        if (cnt_par >= wbs->partime / TICRATE)
        {
            cnt_par = wbs->partime / TICRATE;

            if (cnt_time >= plrs[me].stime / TICRATE)
            {
                S_StartSound(0, sfx_barexp);
                sp_state++;
            }
        }
    }
    else if (sp_state == 10)
    {
        if (acceleratestage)
        {
            S_StartSound(0, sfx_sgcock);

            if (gamemode == commercial)
                WI_initNoState();
            else
                WI_initShowNextLoc();
        }
    }
    else if (sp_state & 1)
    {
        if (!--cnt_pause)
        {
            sp_state++;

            if (beta_style)
                cnt_pause = (TICRATE / 2);
            else
                cnt_pause = TICRATE;
        }
    }
}

void WI_drawStats(void)
{
    // line height
    int lh = 3 * SHORT(num[0]->height) / 2;

    WI_slamBackground(1, 0);

    // draw animated background
    WI_drawAnimatedBack();

    WI_drawLF();

    if (font_shadow == 1)
        V_DrawPatchWithShadow(SP_STATSX, SP_STATSY, 0, kills, false);
    else
    {
        if (beta_style)
            V_DrawPatch(95, 37, 0, kills);
        else
            V_DrawPatch(SP_STATSX, SP_STATSY, 0, kills);
    }

    if (beta_style)
        WI_drawPercent(ORIGINALWIDTH - SP_STATSX - 138, SP_STATSY + 7, cnt_kills[0]);
    else
        WI_drawPercent(ORIGINALWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);

    if (font_shadow == 1)
        V_DrawPatchWithShadow(SP_STATSX, SP_STATSY + lh, 0, items, false);
    else
    {
        if (beta_style)
            V_DrawPatch(151, 37, 0, items);
        else
            V_DrawPatch(SP_STATSX, SP_STATSY + lh, 0, items);
    }

    if (beta_style)
        WI_drawPercent(ORIGINALWIDTH - SP_STATSX - 82, SP_STATSY + 7, cnt_items[0]);
    else
        WI_drawPercent(ORIGINALWIDTH - SP_STATSX, SP_STATSY + lh, cnt_items[0]);

    if (!beta_style)
    {
        if (font_shadow == 1)
            V_DrawPatchWithShadow(SP_STATSX, SP_STATSY + 2 * lh, 0, sp_secret, false);
        else
            V_DrawPatch(SP_STATSX, SP_STATSY + 2 * lh, 0, sp_secret);
    }
    else
        V_DrawPatch(205, 37, 0, secret);

    if (beta_style)
        WI_drawPercent(ORIGINALWIDTH - SP_STATSX - 27, SP_STATSY + 7, cnt_secret[0]);
    else
        WI_drawPercent(ORIGINALWIDTH - SP_STATSX, SP_STATSY + 2 * lh, cnt_secret[0]);

    if (font_shadow == 1)
        V_DrawPatchWithShadow(SP_TIMEX, SP_TIMEY, 0, timepatch, false);
    else
    {
        if (beta_style)
        {
            V_DrawPatch(7, SP_TIMEY - 24, 0, timepatch);
            V_DrawPatch(13, SP_TIMEY - 6, 0, bonus);
            V_DrawPatch(16, SP_TIMEY + 10, 0, score);
        }
        else
            V_DrawPatch(SP_TIMEX, SP_TIMEY, 0, timepatch);
    }

    if (beta_style)
    {
        WI_drawTime(ORIGINALWIDTH / 2 - SP_TIMEX, SP_TIMEY - 24, cnt_time);
        WI_drawExtra(ORIGINALWIDTH / 2 - SP_TIMEX + 100, SP_TIMEY - 6, cnt_bonus);
        WI_drawExtra(ORIGINALWIDTH / 2 - SP_TIMEX + 99, SP_TIMEY + 10, cnt_score);
    }
    else
        WI_drawTime(ORIGINALWIDTH / 2 - SP_TIMEX, SP_TIMEY, cnt_time);

    if (wbs->epsd < 3)
    {
        if (font_shadow == 1)
            V_DrawPatchWithShadow(ORIGINALWIDTH / 2 + SP_TIMEX, SP_TIMEY, 0, par, false);
        else
        {
            if (beta_style)
            {
                V_DrawPatch(ORIGINALWIDTH / 2 + SP_TIMEX, SP_TIMEY - 24, 0, par);

                WI_drawTime(ORIGINALWIDTH - SP_TIMEX, SP_TIMEY - 24, cnt_par);
            }
            else
            {
                V_DrawPatch(ORIGINALWIDTH / 2 + SP_TIMEX, SP_TIMEY, 0, par);

                WI_drawTime(ORIGINALWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);
            }
        }
    }
}

void WI_checkForAccelerate(void)
{
    if (!menuactive && !paused && !consoleactive)
    {
        int       i;
        player_t  *player;

        // check for button presses to skip delays
        for (i = 0, player = players; i < MAXPLAYERS; i++, player++)
        {
            if (playeringame[i])
            {
                if (player->cmd.buttons & BT_ATTACK)
                {
                    if (!player->attackdown)
                        acceleratestage = 1;

                    player->attackdown = true;
                }
                else
                    player->attackdown = false;

                if (player->cmd.buttons & BT_USE)
                {
                    if (!player->usedown)
                        acceleratestage = 1;

                    player->usedown = true;
                }
                else
                    player->usedown = false;
            }
        }
    }
}

// Updates stuff each tick
void WI_Ticker(void)
{
    if (menuactive || paused || consoleactive)
        return;

    // counter for general background animation
    bcnt++;  

    if (bcnt == 1)
    {
        // intermission music
        if (gamemode == commercial)
        {
            S_ChangeMusic(mus_dm2int, true, false);
        }
        else
        {
            S_ChangeMusic(mus_inter, true, false); 
        }
    }

    WI_checkForAccelerate();

    switch (state)
    {
        case StatCount:
            if (deathmatch)
                WI_updateDeathmatchStats();
            else if (netgame)
                WI_updateNetgameStats();
            else
                WI_updateStats();

            break;
        
        case ShowNextLoc:
            WI_updateShowNextLoc();
            break;
        
        case NoState:
            WI_updateNoState();
            break;
    }
}

// Common load/unload function.  Iterates over all the graphics
// lumps to be loaded/unloaded into memory.
static void WI_loadUnloadData(load_callback_t callback)
{
    int     i;
    char    name[9];
    anim_t  *a;

    if (nerve_pwad && gamemission == pack_nerve)
    {
        for (i = 0; i < 9; i++)
        {
            M_snprintf(name, 9, "NWILV%2.2d", i);
            callback(name, &lnames[i]);
        }
    }
    else if (gamemode == commercial)
    {
        for (i = 0; i < NUMCMAPS; i++)
        {
            M_snprintf(name, 9, "CWILV%2.2d", i);
            callback(name, &lnames[i]);
        }
    }
    else
    {
        for (i = 0; i < NUMMAPS; i++)
        {
            /*
            // THIS IS WORKING AS IT SHOULD BUT IN THE REAL PR BETA, IT WASN'T IMPLEMENTED
            if (beta_style && gameepisode == 1)
                M_snprintf(name, 9, "WIBLV0%d", i);
            else
            */
                M_snprintf(name, 9, "WILV%d%d", wbs->epsd, i);
            callback(name, &lnames[i]);
        }

        if (beta_style)
        {
            // you are here
            callback("WIBURH0", &byah[0]);

            // you are here (alt.)
            callback("WIBURH0", &byah[1]);
        }
        else
        {
            // you are here
            callback("WIURH0", &yah[0]);

            // you are here (alt.)
            callback("WIURH1", &yah[1]);
        }

        // splat
        if (beta_style)
            callback("WIUWH0", &splat[0]);
        else
            callback("WISPLAT", &splat[0]);

        if (wbs->epsd < 3)
        {
            int j;

            for (j = 0; j < NUMANIMS[wbs->epsd]; j++)
            {
                a = &anims[wbs->epsd][j];

                for (i = 0; i < a->nanims; i++)
                {
                    // MONDO HACK!
                    if (wbs->epsd != 1 || j != 8)
                    {
                        // animations
                        M_snprintf(name, 9, "WIA%d%.2d%.2d", wbs->epsd, j, i);
                        callback(name, &a->p[i]);
                    }
                    else
                    {
                        // HACK ALERT!
                        a->p[i] = anims[1][4].p[i];
                    }
                }
            }
        }
    }

    // More hacks on minus sign.
    if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 && fsize != 4274218 &&
        fsize != 4225504 && fsize != 4225460 && fsize != 4207819)
        callback("WIMINUS", &wiminus);

    for (i = 0; i < 10; i++)
    {
        // numbers 0-9
        M_snprintf(name, 9, "WINUM%d", i);
        callback(name, &num[i]);
    }

    // percent sign
    callback("WIPCNT", &percent);

    // "finished"
    callback("WIF", &finished);

    // "entering"
    callback("WIENTER", &entering);

    // "kills"
    callback("WIOSTK", &kills);

    // "scrt"
    callback("WIOSTS", &secret);

     // "secret"
    callback("WISCRT2", &sp_secret);

    // french wad uses WIOBJ (?)
    if (W_CheckNumForName("WIOBJ") >= 0)
    {
        // "items"
        if (netgame && !deathmatch)
            callback("WIOBJ", &items);
        else
            callback("WIOSTI", &items);
    }
    else
    {
        callback("WIOSTI", &items);
    }

    // "score"
    callback("WISCORE", &score);

    // "frgs"
    callback("WIBONUS", &bonus);

    // "frgs"
    callback("WIFRGS", &frags);

    // ":"
    callback("WICOLON", &colon);

    // "time"
    callback("WITIME", &timepatch);

    // "sucks"
    callback("WISUCKS", &sucks);

    // "par"
    callback("WIPAR", &par);

    // "killers" (vertical)
    callback("WIKILRS", &killers);

    // "victims" (horiz)
    callback("WIVCTMS", &victims);

    // "total"
    callback("WIMSTT", &total);

    for (i = 0; i < MAXPLAYERS; i++)
    {
        // "1, 2, 3, 4"
        M_snprintf(name, 9, "STPB%d", i);
        callback(name, &p[i]);

        // "1, 2, 3, 4"
        M_snprintf(name, 9, "WIBP%d", i + 1);
        callback(name, &bp[i]);
    }

    // Background image
    if (gamemode == commercial)
    {
        M_StringCopy(name, "INTERPIC", 9);
        name[8] = '\0';
    }
    else if (gamemode == retail && wbs->epsd == 3)
    {
        M_StringCopy(name, "INTERPIC", 9);
        name[8] = '\0';
    }
    else
    {
        if (beta_style && gameepisode == 1)
            M_snprintf(name, 9, "WIBMAP0");
        else
            M_snprintf(name, 9, "WIMAP%d", wbs->epsd);
    }

    // Draw backdrop and save to a temporary buffer
    callback(name, &background);
}

static void WI_loadCallback(char *name, patch_t **variable)
{
    *variable = W_CacheLumpName(name, PU_STATIC);
}

void WI_loadData(void)
{
    if (gamemode == commercial)
    {
        if (fsize != 14677988 && fsize != 14683458 && fsize != 14691821)
            NUMCMAPS = 32;
        else
            NUMCMAPS = 33;

        lnames = (patch_t **) Z_Malloc(sizeof(patch_t *) * NUMCMAPS,
                                       PU_STATIC, NULL);
    }
    else
    {
        lnames = (patch_t **) Z_Malloc(sizeof(patch_t *) * NUMMAPS,
                                       PU_STATIC, NULL);
    }

    WI_loadUnloadData(WI_loadCallback);

    // These two graphics are special cased because we're sharing
    // them with the status bar code

    // your face
    star = W_CacheLumpName("STFST01", PU_STATIC);

    // dead face
    bstar = W_CacheLumpName("STFDEAD0", PU_STATIC);
}

static void WI_unloadCallback(char *name, patch_t **variable)
{
    W_ReleaseLumpName(name);
    *variable = NULL;
}

void WI_unloadData(void)
{
    WI_loadUnloadData(WI_unloadCallback);

    // We do not free these lumps as they are shared with the status
    // bar code.
   
    // W_ReleaseLumpName("STFST01");
    // W_ReleaseLumpName("STFDEAD0");
}

void WI_End(void)
{
    WI_unloadData();
}

void WI_Drawer (void)
{
    switch (state)
    {
        case StatCount:
            if (deathmatch)
                WI_drawDeathmatchStats();
            else if (netgame)
                WI_drawNetgameStats();
            else
                WI_drawStats();

            break;
        
        case ShowNextLoc:
            WI_drawShowNextLoc();
            break;
        
        case NoState:
            WI_drawNoState();
            break;
    }
}

// RNGCHECK():
//  Return 0 if value is outside the range of high and low; else 1.

#ifdef RANGECHECK
static int RNGCHECK(int value, char *name, int low, int high)
{
    if ((value < low) || (value > high))
    {
        C_Warning("WI_initVariables: %s outside valid range of %d - %d", name, low, high);

        return(0);
    }

    return(1);
}
#endif

void WI_initVariables(wbstartstruct_t *wbstartstruct)
{
    wbs = wbstartstruct;

#ifdef RANGECHECK
    if (gamemode != commercial)
    {
        if (gamemode == retail)
            RNGCHECK(wbs->epsd, "wbs->epsd", 0, 3);
        else
            RNGCHECK(wbs->epsd, "wbs->epsd", 0, 2);
    }
    else
    {
        RNGCHECK(wbs->last, "wbs->epsd", 0, 8);
        RNGCHECK(wbs->next, "wbs->epsd", 0, 8);
    }

    RNGCHECK(wbs->pnum, "wbs->epsd", 0, MAXPLAYERS);
    RNGCHECK(wbs->pnum, "wbs->epsd", 0, MAXPLAYERS);
#endif

    acceleratestage = 0;
    cnt = bcnt = 0;
    firstrefresh = 1;
    me = wbs->pnum;
    plrs = wbs->plyr;

    if (!wbs->maxkills)
        wbs->maxkills = 1;

    if (!wbs->maxitems)
        wbs->maxitems = 1;

    if (!wbs->maxsecret)
        wbs->maxsecret = 1;

    if (gamemode != retail)
        if (wbs->epsd > 2)
            wbs->epsd -= 3;
}

void WI_Start(wbstartstruct_t *wbstartstruct)
{
    WI_initVariables(wbstartstruct);
    WI_loadData();

    if (deathmatch)
        WI_initDeathmatchStats();
    else if (netgame)
        WI_initNetgameStats();
    else
        WI_initStats();
}

