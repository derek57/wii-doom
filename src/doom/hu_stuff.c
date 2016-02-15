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
// DESCRIPTION:  Heads-up displays
//
//-----------------------------------------------------------------------------


#include <ctype.h>

#include "am_map.h"
#include "c_io.h"
#include "d_deh.h"
#include "doomdef.h"
#include "doomkeys.h"
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "hu_lib.h"
#include "hu_stuff.h"
#include "i_swap.h"
#include "i_tinttab.h"
#include "i_video.h"
#include "m_controls.h"
#include "m_misc.h"

#include "p_setup.h"
#include "s_sound.h"
#include "sounds.h"
#include "st_stuff.h"

// [crispy] colored kills/items/secret/etc. messages
#include "v_trans.h"

 // [crispy] V_ClearDPTranslation()
#include "v_video.h"

#include "w_wad.h"
#include "z_zone.h"

//
// Locally used constants, shortcuts.
//
#define HU_TITLE        (*mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2       (*mapnames2[gamemap-1])
#define HU_TITLE2_BFG   (*mapnames2_bfg[gamemap-1])
#define HU_TITLEP       (*mapnamesp[gamemap-1])
#define HU_TITLET       (*mapnamest[gamemap-1])
#define HU_TITLEN       (*mapnamesn[gamemap-1])
#define HU_TITLEM       (*mapnamesm[gamemap-1])
#define HU_TITLE_HACX   (*mapnamesh[gamemap-1])
#define HU_TITLE_CHEX   (*mapnamesc[(gameepisode-1)*9+gamemap-1])
#define HU_TITLEHEIGHT  1
#define HU_TITLEX       0
#define HU_TITLEY       (167 - SHORT(hu_font[0]->height))

#define HU_INPUTTOGGLE  't'
#define HU_INPUTX       HU_MSGX
#define HU_INPUTY       (HU_MSGY + HU_MSGHEIGHT*(SHORT(hu_font[0]->height) +1))
#define HU_INPUTWIDTH   64
#define HU_INPUTHEIGHT  1

#define HU_STATSX       228
#define HU_STATSY       -18

#define HU_MONSECX1     HU_STATSX + 0
#define HU_MONSECX2     HU_STATSX + 35
#define HU_MONSECX3     HU_STATSX + 45
#define HU_MONSECX4     HU_STATSX + 80

#define HU_MONSTERSY    HU_STATSY + HU_TITLEY - 20 + (25 - SHORT(hu_font[0]->height))
#define HU_ITEMSY       HU_STATSY + HU_TITLEY - 20 + (35 - SHORT(hu_font[0]->height))
#define HU_SECRETSY     HU_STATSY + HU_TITLEY - 20 + (45 - SHORT(hu_font[0]->height))

#define AM              "American McGee"
#define JA              "John Anderson"
#define JR              "John Romero"
#define MB              "Michael Bukowski"
#define RH              "Richard Heath"
#define RM              "Russell Meakim"
#define SG              "Shawn Green"
#define SP              "Sandy Petersen"
#define TH              "Tom Hall"
#define TW              "Tim Willits"
#define AMSP            AM", "SP
#define JRTH            JR", "TH
#define SPTH            SP", "TH

char *authors[][3] =
{
    /*        ULT. | DOOM2 | NERVE
              DOOM |  BFG  | PACK.
       00 */ { "",     "",    "" },
    /* 01 */ { "",     SP,    RM },
    /* 02 */ { "",     AM,    RH },
    /* 03 */ { "",     AM,    RM },
    /* 04 */ { "",     AM,    RM },
    /* 05 */ { "",     AM,    RH },
    /* 06 */ { "",     AM,    RH },
    /* 07 */ { "",     AMSP,  RH },
    /* 08 */ { "",     SP,    RH },
    /* 09 */ { "",     SP,    RM },
    /* 10 */ { "",     SPTH,  "" },
    /* 11 */ { JR,     JR,    "" },
    /* 12 */ { JR,     SP,    "" },
    /* 13 */ { JR,     SP,    "" },
    /* 14 */ { JRTH,   AM,    "" },
    /* 15 */ { JR,     JR,    "" },
    /* 16 */ { JR,     SP,    "" },
    /* 17 */ { JR,     JR,    "" },
    /* 18 */ { SPTH,   SP,    "" },
    /* 19 */ { JR,     SP,    "" },
    /* 20 */ { "",     JR,    "" },
    /* 21 */ { SPTH,   SP,    "" },
    /* 22 */ { SPTH,   AM,    "" },
    /* 23 */ { SPTH,   SP,    "" },
    /* 24 */ { SPTH,   SP,    "" },
    /* 25 */ { SP,     SG,    "" },
    /* 26 */ { SP,     JR,    "" },
    /* 27 */ { SPTH,   SP,    "" },
    /* 28 */ { SP,     SP,    "" },
    /* 29 */ { SP,     JR,    "" },
    /* 30 */ { "",     SP,    "" },
    /* 31 */ { "",     SP,    "" },
    /* 32 */ { "",     SP,    "" },
    /* 33 */ { SPTH,   MB,    "" },
    /* 34 */ { "",     "",    "" },
    /* 35 */ { "",     "",    "" },
    /* 36 */ { "",     "",    "" },
    /* 37 */ { "",     "",    "" },
    /* 38 */ { "",     "",    "" },
    /* 39 */ { "",     "",    "" },
    /* 40 */ { "",     "",    "" },
    /* 41 */ { AM,     "",    "" },
    /* 42 */ { JR,     "",    "" },
    /* 43 */ { SG,     "",    "" },
    /* 44 */ { AM,     "",    "" },
    /* 45 */ { TW,     "",    "" },
    /* 46 */ { JR,     "",    "" },
    /* 47 */ { JA,     "",    "" },
    /* 48 */ { SG,     "",    "" },
    /* 49 */ { TW,     "",    "" }
};

#define NERVE_AUTHORS   authors[gamemap][2]
#define DOOM2_AUTHORS   authors[gamemap][1]
#define UDOOM_AUTHORS   authors[(gameepisode * 10) + gamemap][0]

static player_t*        plr;

static hu_textline_t    w_title;
static hu_textline_t    w_author_title;
static hu_textline_t    w_authors;

static hu_textline_t    w_monsters1;              // ADDED FOR PSP-STATS
static hu_textline_t    w_monsters2;              // ADDED FOR PSP-STATS
static hu_textline_t    w_monsters3;              // ADDED FOR PSP-STATS
static hu_textline_t    w_monsters4;              // ADDED FOR PSP-STATS

static hu_textline_t    w_items1;                 // ADDED FOR PSP-STATS
static hu_textline_t    w_items2;                 // ADDED FOR PSP-STATS
static hu_textline_t    w_items3;                 // ADDED FOR PSP-STATS
static hu_textline_t    w_items4;                 // ADDED FOR PSP-STATS

static hu_textline_t    w_secrets1;               // ADDED FOR PSP-STATS
static hu_textline_t    w_secrets2;               // ADDED FOR PSP-STATS
static hu_textline_t    w_secrets3;               // ADDED FOR PSP-STATS
static hu_textline_t    w_secrets4;               // ADDED FOR PSP-STATS

//static hu_itext_t       w_inputbuffer[MAXPLAYERS];

static hu_stext_t       w_message;

static hu_stext_t       w_message_0;
static hu_stext_t       w_message_1;
static hu_stext_t       w_message_2;

static hu_stext_t       w_secret;

//static dboolean          always_off = false;
static dboolean         message_on;
static dboolean         message_nottobefuckedwith;
static dboolean         headsupactive;
static dboolean         secret_on;

static int              message_counter;
static int              secret_counter;
static int              hudnumoffset;

static char             monstersstr1[80];    // ADDED FOR PSP-STATS
static char             monstersstr2[80];    // ADDED FOR PSP-STATS
static char             monstersstr3[80];    // ADDED FOR PSP-STATS
static char             monstersstr4[80];    // ADDED FOR PSP-STATS

static char             itemsstr1[80];       // ADDED FOR PSP-STATS
static char             itemsstr2[80];       // ADDED FOR PSP-STATS
static char             itemsstr3[80];       // ADDED FOR PSP-STATS
static char             itemsstr4[80];       // ADDED FOR PSP-STATS

static char             secretsstr1[80];     // ADDED FOR PSP-STATS
static char             secretsstr2[80];     // ADDED FOR PSP-STATS
static char             secretsstr3[80];     // ADDED FOR PSP-STATS
static char             secretsstr4[80];     // ADDED FOR PSP-STATS

dboolean                message_dontfuckwithme;
dboolean                show_chat_bar;
dboolean                emptytallpercent;

patch_t*                hu_font[HU_FONTSIZE];
patch_t*                beta_hu_font[HU_FONTSIZEBETA];

extern int              cardsfound;

extern dboolean         blurred;
extern dboolean         mapinfo_lump;
extern dboolean         dont_message_to_console;

static patch_t*         healthpatch;
static patch_t*         berserkpatch;
static patch_t*         greenarmorpatch;
static patch_t*         bluearmorpatch;

extern patch_t*         tallnum[10];
extern patch_t*         tallpercent;

static struct
{
    char        *patchnamea;
    char        *patchnameb;
    patch_t     *patch;
} keypic[NUMCARDS] = {
    { "BKEYA0", "BKEYB0", NULL },
    { "YKEYA0", "YKEYB0", NULL },
    { "RKEYA0", "RKEYB0", NULL },
    { "BSKUA0", "BSKUB0", NULL },
    { "YSKUA0", "YSKUB0", NULL },
    { "RSKUA0", "RSKUB0", NULL }
};

void (*hudfunc)(int, int, int, patch_t *, byte *);
void (*hudnumfunc)(int, int, int, patch_t *, byte *);
void (*godhudfunc)(int, int, int, patch_t *, byte *);

void HU_Init(void)
{

    int       i;
    int       j;
    char      buffer[9];

    // load the heads-up font
    j = HU_FONTSTART;
    for (i=0;i<HU_FONTSIZE;i++)
    {
        M_snprintf(buffer, 9, "STCFN%.3d", j++);
        hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }

    j = HU_FONTSTART;
    for (i=0;i<HU_FONTSIZEBETA;i++)
    {
        M_snprintf(buffer, 9, "STBFN%.3d", j++);
        beta_hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }
}

void HU_Stop(void)
{
    headsupactive = false;
}

patch_t *HU_LoadHUDKeyPatch(int keypicnum)
{
    int lump;

    if (dehacked && (lump = W_CheckNumForName(keypic[keypicnum].patchnamea)) >= 0)
        return W_CacheLumpNum(lump, PU_CACHE);
    else if ((lump = W_CheckNumForName(keypic[keypicnum].patchnameb)) >= 0)
        return W_CacheLumpNum(lump, PU_CACHE);
    else
        return NULL;
}

void HU_Start(void)
{
//    int       i;
    char*     s = "Unknown level";

    char*     t;
    char*     y;
    char*     z;
    char*     l;

    char*     q;
    char*     r;
    char*     w;
    char*     m;

    char*     o;
    char*     p;
    char*     x;
    char*     n;

    char*     u;
    char*     v = "AUTHOR(S):";

    if (headsupactive)
        HU_Stop();

    emptytallpercent = V_EmptyPatch(tallpercent);

    plr = &players[consoleplayer];
    message_on = false;
    message_dontfuckwithme = false;
    message_nottobefuckedwith = false;
    secret_on = false;

    // create the message widget

    if(beta_style)
    {
        HUlib_initSText(&w_message_0,
                    HU_MSGX + 106, HU_MSGY + 179, HU_MSGHEIGHT,
                    beta_hu_font,
                    HU_FONTSTART, &message_on);

        HUlib_initSText(&w_message_1,
                    HU_MSGX + 106, HU_MSGY + 185, HU_MSGHEIGHT,
                    beta_hu_font,
                    HU_FONTSTART, &message_on);

        HUlib_initSText(&w_message_2,
                    HU_MSGX + 106, HU_MSGY + 191, HU_MSGHEIGHT,
                    beta_hu_font,
                    HU_FONTSTART, &message_on);
    }
    else
        HUlib_initSText(&w_message,
                    HU_MSGX, HU_MSGY, HU_MSGHEIGHT,
                    hu_font,
                    HU_FONTSTART, &message_on);

    // [crispy] create the secret message widget
    HUlib_initSText(&w_secret,
                    88, 86, HU_MSGHEIGHT,
                    hu_font,
                    HU_FONTSTART, &secret_on);

    // create the map title widget
    if (show_title)
    {
/*
        if (d_statusmap)
        {
            HUlib_initTextLine(&w_title,
                               HU_TITLEX, HU_TITLEY,
                               hu_font,
                               HU_FONTSTART);
        }
        else
*/
        {
            if (!modifiedgame)
            {
                HUlib_initTextLine(&w_title,
                                   HU_TITLEX, HU_TITLEY + 32,
                                   hu_font,
                                   HU_FONTSTART);
            }
            else
            {
                HUlib_initTextLine(&w_title,
                                   HU_TITLEX, HU_MONSTERSY + 32,
                                   hu_font,
                                   HU_FONTSTART);
            }
        }
    }

    if ((show_authors && gameversion != exe_chex && gameversion != exe_hacx
#ifdef WII
         && load_extra_wad == 0
#endif
         ) ||
        (show_authors && nerve_pwad) || (show_authors && master_pwad))
    {
        if (d_statusmap && automapactive)
        {
            HUlib_initTextLine(&w_author_title,
                               HU_TITLEX, HU_TITLEY - 20,
                               hu_font,
                               HU_FONTSTART);

            HUlib_initTextLine(&w_authors,
                               HU_TITLEX, HU_TITLEY - 10,
                               hu_font,
                               HU_FONTSTART);
        }
        else
        {
            HUlib_initTextLine(&w_author_title,
                               HU_TITLEX, HU_TITLEY + 12,
                               hu_font,
                               HU_FONTSTART);

            HUlib_initTextLine(&w_authors,
                               HU_TITLEX, HU_TITLEY + 22,
                               hu_font,
                               HU_FONTSTART);
        }
    }
/*
    if (d_statusmap)
    {
        HUlib_initTextLine(&w_monsters1,
                           HU_MONSECX1, HU_MONSTERSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_monsters2,
                           HU_MONSECX2, HU_MONSTERSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_monsters3,
                           HU_MONSECX3, HU_MONSTERSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_monsters4,
                           HU_MONSECX4, HU_MONSTERSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_items1,
                           HU_MONSECX1, HU_ITEMSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_items2,
                           HU_MONSECX2, HU_ITEMSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_items3,
                           HU_MONSECX3, HU_ITEMSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_items4,
                           HU_MONSECX4, HU_ITEMSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_secrets1,
                           HU_MONSECX1, HU_SECRETSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_secrets2,
                           HU_MONSECX2, HU_SECRETSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_secrets3,
                           HU_MONSECX3, HU_SECRETSY,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_secrets4,
                           HU_MONSECX4, HU_SECRETSY,
                           hu_font,
                           HU_FONTSTART);
    }
    else
*/
    {
        HUlib_initTextLine(&w_monsters1,
                           HU_MONSECX1, HU_MONSTERSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_monsters2,
                           HU_MONSECX2, HU_MONSTERSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_monsters3,
                           HU_MONSECX3, HU_MONSTERSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_monsters4,
                           HU_MONSECX4, HU_MONSTERSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_items1,
                           HU_MONSECX1, HU_ITEMSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_items2,
                           HU_MONSECX2, HU_ITEMSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_items3,
                           HU_MONSECX3, HU_ITEMSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_items4,
                           HU_MONSECX4, HU_ITEMSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_secrets1,
                           HU_MONSECX1, HU_SECRETSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_secrets2,
                           HU_MONSECX2, HU_SECRETSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_secrets3,
                           HU_MONSECX3, HU_SECRETSY + 32,
                           hu_font,
                           HU_FONTSTART);

        HUlib_initTextLine(&w_secrets4,
                           HU_MONSECX4, HU_SECRETSY + 32,
                           hu_font,
                           HU_FONTSTART);
    }

    if(!modifiedgame)
    {
        switch ( logical_gamemission )
        {
          case doom:
            s = HU_TITLE;
            break;
          case doom2:
            if(bfgedition)
                s = HU_TITLE2_BFG;
            else
                s = HU_TITLE2;
            break;
          case pack_plut:
            s = HU_TITLEP;
            break;
          case pack_tnt:
            s = HU_TITLET;
            break;
          case pack_nerve:
            if (gamemap <= 9)
              s = HU_TITLEN;
            else
              s = HU_TITLE2;
            break;
          case pack_master:
            if (gamemap <= 21)
              s = HU_TITLEM;
            else
              s = HU_TITLE2;
            break;
          default:
            break;
        }
    }

    if (logical_gamemission == doom && gameversion == exe_chex)
    {
        if(gamemap <= 5)
            s = HU_TITLE_CHEX;
        else
            s = HU_TITLE;
    }
    else if (logical_gamemission == doom2 && gameversion == exe_hacx)
    {
        if (gamemap <= 20)
            s = HU_TITLE_HACX;
        else if(gamemap == 31)
            s = "Desiccant Room";
        else
            s = HU_TITLE2;

        if(gamemap == 33)
            s = HU_TITLE2_BFG;
    }

    if (modifiedgame)
    {
        if (mapinfo_lump)
        {
            if(gamemode == commercial)
                s = P_GetMapName(gamemap);
            else
                s = P_GetMapName((gameepisode - 1) * 10 + gamemap);
        }
        else
        {
            char lump[5];

            if(gamemode == commercial)
            {
                if (gamemap < 10)
                    M_snprintf(lump, sizeof(s), "MAP0%i", gamemap);
                else if (gamemap > 9 && gamemap < 20)
                    M_snprintf(lump, sizeof(lump), "MAP1%i", gamemap);
                else if (gamemap > 19 && gamemap < 30)
                    M_snprintf(lump, sizeof(lump), "MAP2%i", gamemap);
                else if (gamemap > 29)
                    M_snprintf(lump, sizeof(lump), "MAP3%i", gamemap);
            }
            else
                M_snprintf(lump, sizeof(lump), "E%iM%i", gameepisode, gamemap);

            s = lump;
        }
    }

    if (gamemission == pack_nerve)
    {
        u = NERVE_AUTHORS;
    }
    else if (gamemission == doom2)
    {
        u = DOOM2_AUTHORS;
    }
    else
    {
        u = UDOOM_AUTHORS;
    }

    // dehacked substitution to get modified level name

    t = monstersstr1;
    y = monstersstr2;
    z = monstersstr3;
    l = monstersstr4;

    q = itemsstr1;
    r = itemsstr2;
    w = itemsstr3;
    m = itemsstr4;

    o = secretsstr1;
    p = secretsstr2;
    x = secretsstr3;
    n = secretsstr4;

    if (((fsize != 12538385 &&
        fsize != 14691821 &&
        fsize != 14677988 &&
        fsize != 14683458) ||
        (fsize == 12538385 && gamemap != 10) ||
        ((fsize == 14683458 || fsize == 14677988 || fsize == 14691821) && gamemap != 33)) && show_title)
    {
        while (*s)
            HUlib_addCharToTextLine(&w_title, *(s++));
    }

    while (*t)
        HUlib_addCharToTextLine(&w_monsters1, *(t++));

    while (*y)
        HUlib_addCharToTextLine(&w_monsters2, *(y++));

    while (*z)
        HUlib_addCharToTextLine(&w_monsters3, *(z++));

    while (*l)
        HUlib_addCharToTextLine(&w_monsters4, *(l++));

    while (*q)
        HUlib_addCharToTextLine(&w_items1, *(q++));

    while (*r)
        HUlib_addCharToTextLine(&w_items2, *(r++));

    while (*w)
        HUlib_addCharToTextLine(&w_items3, *(w++));

    while (*m)
        HUlib_addCharToTextLine(&w_items4, *(m++));

    while (*o)
        HUlib_addCharToTextLine(&w_secrets1, *(o++));

    while (*p)
        HUlib_addCharToTextLine(&w_secrets2, *(p++));

    while (*x)
        HUlib_addCharToTextLine(&w_secrets3, *(x++));

    while (*n)
        HUlib_addCharToTextLine(&w_secrets4, *(n++));

    if ((show_authors && gameversion != exe_chex && gameversion != exe_hacx
#ifdef WII
         && load_extra_wad == 0
#endif
        ) ||
        (show_authors && nerve_pwad) || (show_authors && master_pwad))
    {
        while (*u)
            HUlib_addCharToTextLine(&w_authors, *(u++));

        while (*v)
            HUlib_addCharToTextLine(&w_author_title, *(v++));
    }
/*
    // create the inputbuffer widgets
    for (i=0 ; i<MAXPLAYERS ; i++)
        HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, &always_off);
*/
    if (W_CheckNumForName("MEDIA0"))
        healthpatch = W_CacheLumpNum(W_GetNumForName("MEDIA0"), PU_CACHE);

    if (gamemode != shareware && W_CheckNumForName("PSTRA0"))
        berserkpatch = W_CacheLumpNum(W_GetNumForName("PSTRA0"), PU_CACHE);
    else
        berserkpatch = healthpatch;

    if (W_CheckNumForName("ARM1A0"))
        greenarmorpatch = W_CacheLumpNum(W_GetNumForName("ARM1A0"), PU_CACHE);

    if (W_CheckNumForName("ARM2A0"))
        bluearmorpatch = W_CacheLumpNum(W_GetNumForName("ARM2A0"), PU_CACHE);

    keypic[it_bluecard].patch = HU_LoadHUDKeyPatch(it_bluecard);
    keypic[it_yellowcard].patch = HU_LoadHUDKeyPatch(exe_hacx ? it_yellowcard : it_yellowskull);
    keypic[it_redcard].patch = HU_LoadHUDKeyPatch(it_redcard);

    if (gamemode != shareware)
    {
        keypic[it_blueskull].patch = HU_LoadHUDKeyPatch(it_blueskull);
        keypic[it_yellowskull].patch = HU_LoadHUDKeyPatch(it_yellowskull);
        keypic[it_redskull].patch = HU_LoadHUDKeyPatch(it_redskull);
    }

    headsupactive = true;

    hudnumoffset = (16 - SHORT(tallnum[0]->height)) / 2;
}

// [crispy] print a bar indicating demo progress at the bottom of the screen
/*
static void HU_DemoProgressBar (int scrn)                // FIXME: BUGGY (crashes)
{
    int i;
    extern char *demo_p, *demobuffer;
    extern int defdemosize;

    i = SCREENWIDTH * (demo_p - demobuffer) / defdemosize;

    V_DrawHorizLine(0, SCREENHEIGHT - 3, scrn, i, 4);     // [crispy] white
    V_DrawHorizLine(0, SCREENHEIGHT - 2, scrn, i, 0);     // [crispy] black
    V_DrawHorizLine(0, SCREENHEIGHT - 1, scrn, i, 4);     // [crispy] white

    V_DrawHorizLine(0, SCREENHEIGHT - 2, scrn, 1, 4);     // [crispy] white start
    V_DrawHorizLine(i - 1, SCREENHEIGHT - 2, scrn, 1, 4); // [crispy] white end
}
*/
void HU_DrawStats(void)
{
    const char *r;
    const char *s;
    const char *t;
    const char *o;

    const char *u;
    const char *v;
    const char *w;
    const char *p;

    const char *x;
    const char *y;
    const char *z;
    const char *q;

    // clear the internal widget text buffer
    HUlib_clearTextLine(&w_monsters1);
    HUlib_clearTextLine(&w_monsters2);
    HUlib_clearTextLine(&w_monsters3);
    HUlib_clearTextLine(&w_monsters4);

    HUlib_clearTextLine(&w_items1);
    HUlib_clearTextLine(&w_items2);
    HUlib_clearTextLine(&w_items3);
    HUlib_clearTextLine(&w_items4);

    HUlib_clearTextLine(&w_secrets1);
    HUlib_clearTextLine(&w_secrets2);
    HUlib_clearTextLine(&w_secrets3);
    HUlib_clearTextLine(&w_secrets4);

    //jff 3/26/98 use ESC not '\' for paths
    // build the init string with fixed colors
    sprintf(monstersstr1, "%d", plr->killcount);
    sprintf(monstersstr2, "/");
    sprintf(monstersstr3, "%d", totalkills);
    sprintf(monstersstr4, "K.");

    sprintf(itemsstr1, "%d", plr->itemcount);
    sprintf(itemsstr2, "/");
    sprintf(itemsstr3, "%d", totalitems);
    sprintf(itemsstr4, "I.");

    sprintf(secretsstr1, "%d", plr->secretcount);
    sprintf(secretsstr2, "/");
    sprintf(secretsstr3, "%d", totalsecret);
    sprintf(secretsstr4, "S.");

    // transfer the init string to the widget
    r = monstersstr1;
    s = monstersstr2;
    t = monstersstr3;
    o = monstersstr4;

    u = itemsstr1;
    v = itemsstr2;
    w = itemsstr3;
    p = itemsstr4;

    x = secretsstr1;
    y = secretsstr2;
    z = secretsstr3;
    q = secretsstr4;

    //jff 2/17/98 initialize kills/items/secret widget
    while (*r)
        HUlib_addCharToTextLine(&w_monsters1, *(r++));

    while (*s)
        HUlib_addCharToTextLine(&w_monsters2, *(s++));

    while (*t)
        HUlib_addCharToTextLine(&w_monsters3, *(t++));

    while (*o)
        HUlib_addCharToTextLine(&w_monsters4, *(o++));

    while (*u)
        HUlib_addCharToTextLine(&w_items1, *(u++));

    while (*v)
        HUlib_addCharToTextLine(&w_items2, *(v++));

    while (*w)
        HUlib_addCharToTextLine(&w_items3, *(w++));

    while (*p)
        HUlib_addCharToTextLine(&w_items4, *(p++));

    while (*x)
        HUlib_addCharToTextLine(&w_secrets1, *(x++));

    while (*y)
        HUlib_addCharToTextLine(&w_secrets2, *(y++));

    while (*z)
        HUlib_addCharToTextLine(&w_secrets3, *(z++));

    while (*q)
        HUlib_addCharToTextLine(&w_secrets4, *(q++));

    // display the kills/items/secrets each frame, if optioned
    if(gamestate == GS_LEVEL && automapactive && !menuactive)
    {
        if(am_overlay || (automapactive && !d_statusmap))
        {
            HUlib_drawTextLine(&w_monsters1, false);
            HUlib_drawTextLine(&w_monsters2, false);
            HUlib_drawTextLine(&w_monsters3, false);
            HUlib_drawTextLine(&w_monsters4, false);

            HUlib_drawTextLine(&w_items1, false);
            HUlib_drawTextLine(&w_items2, false);
            HUlib_drawTextLine(&w_items3, false);
            HUlib_drawTextLine(&w_items4, false);

            HUlib_drawTextLine(&w_secrets1, false);
            HUlib_drawTextLine(&w_secrets2, false);
            HUlib_drawTextLine(&w_secrets3, false);
            HUlib_drawTextLine(&w_secrets4, false);
        }
    }
}

static void DrawHUDNumber(int *x, int y, int scrn, int val, byte *tinttab,
                          void (*hudnumfunc)(int, int, int, patch_t *, byte *))
{
    int         oldval = val;
    patch_t     *patch;

    if (val > 99)
    {
        patch = tallnum[val / 100];
        hudnumfunc(*x, y, scrn, patch, tinttab);
        *x += SHORT(patch->width);
    }
    val %= 100;
    if (val > 9 || oldval > 99)
    {
        patch = tallnum[val / 10];
        hudnumfunc(*x, y, scrn, patch, tinttab);
        *x += SHORT(patch->width);
    }
    val %= 10;
    patch = tallnum[val];
    hudnumfunc(*x, y, scrn, patch, tinttab);
    *x += SHORT(patch->width);
}

static int HUDNumberWidth(int val)
{
    int oldval = val;
    int width = 0;

    if (val > 99)
        width += SHORT(tallnum[val / 100]->width);
    val %= 100;
    if (val > 9 || oldval > 99)
        width += SHORT(tallnum[val / 10]->width);
    val %= 10;
    width += SHORT(tallnum[val]->width);
    return width;
}

void HU_DrawHUD(void)
{
    int                 health = MAX(0, plr->mo->health);
    int                 ammotype = weaponinfo[plr->readyweapon].ammo;
    int                 ammo = plr->ammo[ammotype];
    int                 armor = plr->armorpoints;
    int                 health_x = HUD_HEALTH_X;
    int                 keys = 0;
    int                 i = 0;
    byte                *tinttab;
    int                 invulnerability = plr->powers[pw_invulnerability];
    static dboolean     healthanim = false;
    patch_t             *patch;
    dboolean            gamepaused = (menuactive || paused /*|| consoleactive*/);
    int                 currenttime = I_GetTimeMS();

    if (am_overlay && (show_authors || show_stats || show_title))
        return;

    if (d_translucency)
    {
        hudfunc = V_DrawTranslucentHUDPatch;
        hudnumfunc = V_DrawTranslucentHUDNumberPatch;
        godhudfunc = V_DrawTranslucentYellowHUDPatch;
    }
    else
    {
        hudfunc = V_DrawHUDPatch;
        hudnumfunc = V_DrawHUDPatch;
        godhudfunc = V_DrawYellowHUDPatch;
    }

    tinttab = (!health || (health <= HUD_HEALTH_MIN && healthanim) || health > HUD_HEALTH_MIN ?
        tinttab66 : tinttab25);

    patch = (((plr->readyweapon == wp_fist && plr->pendingweapon == wp_nochange) || plr->pendingweapon == wp_fist)
        && plr->powers[pw_strength] ? berserkpatch : healthpatch);

    if (patch)
    {
        if ((plr->cheats & CF_GODMODE) || invulnerability > 128 || (invulnerability & 8))
            godhudfunc(health_x, HUD_HEALTH_Y - (SHORT(patch->height) - 17), 0, patch, tinttab);
        else
            hudfunc(health_x, HUD_HEALTH_Y - (SHORT(patch->height) - 17), 0, patch, tinttab);

        health_x += SHORT(patch->width) + 8;
    }

    if (healthhighlight > currenttime)
    {
        DrawHUDNumber(&health_x, HUD_HEALTH_Y + hudnumoffset, 0, health, tinttab, V_DrawHUDPatch);

        if (!emptytallpercent)
            V_DrawHUDPatch(health_x, HUD_HEALTH_Y + hudnumoffset, 0, tallpercent, tinttab);
    }
    else
    {
        DrawHUDNumber(&health_x, HUD_HEALTH_Y + hudnumoffset, 0, health, tinttab, hudnumfunc);

        if (!emptytallpercent)
            hudnumfunc(health_x, HUD_HEALTH_Y + hudnumoffset, 0, tallpercent, tinttab);
    }

    if (!gamepaused)
    {
        static int healthwait;

        if (health <= HUD_HEALTH_MIN)
        {
            if (healthwait < currenttime)
            {
                healthanim = !healthanim;
                healthwait = currenttime + HUD_HEALTH_WAIT * health / HUD_HEALTH_MIN + 115;
            }
        }
        else
        {
            healthanim = false;
            healthwait = 0;
        }
    }

    if (plr->pendingweapon != wp_nochange)
    {
        ammotype = weaponinfo[plr->pendingweapon].ammo;
        ammo = plr->ammo[ammotype];
    }

    if (health && ammo && ammotype != am_noammo)
    {
        static dboolean     ammoanim = false;
        int                 offset_special = 0;
        int                 ammo_x;

        tinttab = ((ammo <= HUD_AMMO_MIN && ammoanim) || ammo > HUD_AMMO_MIN ? tinttab66 :
            tinttab25);

        if (ammo < 200 && ammo > 99)
            offset_special = 3;

        if(plr->readyweapon == wp_pistol)
            patch = W_CacheLumpName("CLIPA0", PU_CACHE);
        else if(plr->readyweapon == wp_shotgun)
            patch = W_CacheLumpName("SHELA0", PU_CACHE);
        else if(plr->readyweapon == wp_chaingun)
            patch = W_CacheLumpName("AMMOA0", PU_CACHE);
        else if(plr->readyweapon == wp_missile)
            patch = W_CacheLumpName("ROCKA0", PU_CACHE);
        else if(plr->readyweapon == wp_plasma)
            patch = W_CacheLumpName("CELLA0", PU_CACHE);
        else if(plr->readyweapon == wp_bfg)
            patch = W_CacheLumpName("CELPA0", PU_CACHE);
        else if(plr->readyweapon == wp_supershotgun)
            patch = W_CacheLumpName("SBOXA0", PU_CACHE);
        else
            patch = W_CacheLumpName("TNT1A0", PU_CACHE);

        ammo_x = HUD_AMMO_X + 15 - (SHORT(patch->width) / 2);

        hudfunc(ammo_x, (HUD_AMMO_Y + 8 - (SHORT(patch->height) / 2)), 0, patch, tinttab);
        ammo_x += HUD_AMMO_X + 15 + (SHORT(patch->width) / 2) - (ORIGHEIGHT / 2) + offset_special;

        if (ammohighlight > currenttime)
            DrawHUDNumber(&ammo_x, HUD_AMMO_Y + hudnumoffset, 0, ammo, tinttab, V_DrawHUDPatch);
        else
            DrawHUDNumber(&ammo_x, HUD_AMMO_Y + hudnumoffset, 0, ammo, tinttab, hudnumfunc);

        if (!gamepaused)
        {
            static int  ammowait;

            if (ammo <= HUD_AMMO_MIN)
            {
                if (ammowait < currenttime)
                {
                    ammoanim = !ammoanim;
                    ammowait = currenttime + HUD_AMMO_WAIT * ammo / HUD_AMMO_MIN + 115;
                }
            }
            else
            {
                ammoanim = false;
                ammowait = 0;
            }
        }
    }

    while (i < NUMCARDS)
        if (plr->cards[i++] > 0)
            keys++;

    if (keys || plr->neededcardflash)
    {
        int                 keypic_x = HUD_KEYS_X - 20 * (keys - 1);
        static int          keywait;
        static dboolean     showkey;

        if (!armor)
            keypic_x += 114;
        else
        {
            if (emptytallpercent)
                keypic_x += SHORT(tallpercent->width);

            if (armor < 10)
                keypic_x += 26;
            else if (armor < 100)
                keypic_x += 12;
        }

        if (plr->neededcardflash)
        {
            patch_t     *patch = keypic[plr->neededcard].patch;

            if (patch)
            {
                if (!gamepaused && keywait < currenttime) 
                {
                    showkey = !showkey;
                    keywait = currenttime + HUD_KEY_WAIT;
                    plr->neededcardflash--;
                }

                if (showkey)
                    hudfunc(keypic_x - (SHORT(patch->width) + 6), HUD_KEYS_Y, 0, patch, tinttab66);
            }
        }
        else
        {
            showkey = false;
            keywait = 0;
        }

        for (i = 0; i < NUMCARDS; i++)
            if (plr->cards[i] > 0)
            {
                patch_t     *patch = keypic[i].patch;

                if (patch)
                    hudfunc(keypic_x + (SHORT(patch->width) + 6) * (cardsfound - plr->cards[i]),
                        HUD_KEYS_Y, 0, patch, tinttab66);
            }
    }

    if (armor)
    {
        patch_t     *patch = (plr->armortype == 1 ? greenarmorpatch : bluearmorpatch);
        int         armor_x = HUD_ARMOR_X;

        if (patch)
        {
            armor_x -= SHORT(patch->width);
            hudfunc(armor_x, HUD_ARMOR_Y - (SHORT(patch->height) - 16), 0, patch, tinttab66);
            armor_x -= 7;
        }

        if (armorhighlight > currenttime)
        {
            if (emptytallpercent)
            {
                armor_x -= HUDNumberWidth(armor);
                DrawHUDNumber(&armor_x, HUD_ARMOR_Y + hudnumoffset, 0, armor, tinttab66, V_DrawHUDPatch);
            }
            else
            {
                armor_x -= SHORT(tallpercent->width);
                V_DrawHUDPatch(armor_x, HUD_ARMOR_Y + hudnumoffset, 0, tallpercent, tinttab66);
                armor_x -= HUDNumberWidth(armor);
                DrawHUDNumber(&armor_x, HUD_ARMOR_Y + hudnumoffset, 0, armor, tinttab66, V_DrawHUDPatch);
            }
        }
        else if (emptytallpercent)
        {
            armor_x -= HUDNumberWidth(armor);
            DrawHUDNumber(&armor_x, HUD_ARMOR_Y + hudnumoffset, 0, armor, tinttab66, hudnumfunc);
        }
        else
        {
            armor_x -= SHORT(tallpercent->width);
            hudnumfunc(armor_x, HUD_ARMOR_Y + hudnumoffset, 0, tallpercent, tinttab66);
            armor_x -= HUDNumberWidth(armor);
            DrawHUDNumber(&armor_x, HUD_ARMOR_Y + hudnumoffset, 0, armor, tinttab66, hudnumfunc);
        }
    }
}

void HU_Drawer(void)
{
    if(!automapactive && !demoplayback && crosshair == 1)
    {
        if(screenSize < 8)
            V_DrawPatch(158, 82, 0, W_CacheLumpName("XHAIR", PU_CACHE));
        else
            V_DrawPatch(158, 98, 0, W_CacheLumpName("XHAIR", PU_CACHE));
    }

    if(beta_style)
    {
        HUlib_drawSText(&w_message_0);
        HUlib_drawSText(&w_message_1);
        HUlib_drawSText(&w_message_2);
    }
    else
    {
        if(gamestate == GS_LEVEL)
            HUlib_drawSText(&w_message);
        else
            HU_Erase();
    }

    dp_translation = crx[CRX_GOLD];
    HUlib_drawSText(&w_secret);
    V_ClearDPTranslation();

    if (!menuactive)
    {
        if (((automapactive ||
             ((am_overlay && (show_authors 
#ifdef WII
               && load_extra_wad == 0
#endif
              ))) ||
              (show_authors && nerve_pwad) || (show_authors && master_pwad))))
        {
            if(((!modifiedgame || nerve_pwad) && !d_statusmap) || (am_overlay && !modifiedgame))
            {
                HUlib_drawTextLine(&w_author_title, false);
                HUlib_drawTextLine(&w_authors, false);
            }

            if((!beta_style && show_title && automapactive && !d_statusmap) || am_overlay)
                HUlib_drawTextLine(&w_title, false);
        }

        // display the hud kills/items/secret display if optioned
        if (show_stats == 1)
            HU_DrawStats();

        if (timer_info == 1)
            AM_DrawWorldTimer();
    }
    V_ClearDPTranslation();

    if (dp_translucent)
        dp_translucent = false;
/*
    // [crispy] demo progress bar        // FIXME (crashing)
    if (demoplayback)
        HU_DemoProgressBar(0);
*/
}

void HU_Erase(void)
{
    if(beta_style)
    {
        HUlib_eraseSText(&w_message_0);
        HUlib_eraseSText(&w_message_1);
        HUlib_eraseSText(&w_message_2);
    }
    else
        HUlib_eraseSText(&w_message);

    HUlib_eraseSText(&w_secret);
}

void HU_Ticker(void)
{
    // tick down message counter if message is up
    if (message_counter && !--message_counter)
    {
        message_on = false;
        message_nottobefuckedwith = false;
        blurred = false;

        if(beta_style)
        {
            show_chat_bar = false;
        }
    }

    if (secret_counter && !--secret_counter)
    {
        secret_on = false;
    }

    if (showMessages || message_dontfuckwithme)
    {
        // display message if necessary
        if (plr->message
            && !strncmp(plr->message, HUSTR_SECRETFOUND, 21))
        {
            HUlib_addMessageToSText(&w_secret, 0, plr->message);
            plr->message = 0;
            secret_on = true;
            secret_counter = HU_MSGTIMEOUT >> 1;
        }
        else if ((plr->message && !message_nottobefuckedwith)
            || (plr->message && message_dontfuckwithme))
        {
            if(beta_style)
            {
                if(plr->messages[1])
                {
                    plr->messages[0] = plr->messages[1];
                    HUlib_addMessageToSText(&w_message_0, 0, plr->messages[0]);
                }

                if(plr->messages[2])
                {
                    plr->messages[1] = plr->messages[2];
                    HUlib_addMessageToSText(&w_message_1, 0, plr->messages[1]);
                }
                plr->messages[2] = plr->message;
                HUlib_addMessageToSText(&w_message_2, 0, plr->messages[2]);

                show_chat_bar = true;
            }
            else
                HUlib_addMessageToSText(&w_message, 0, plr->message);

            plr->message = 0;
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            message_nottobefuckedwith = message_dontfuckwithme;
            message_dontfuckwithme = 0;
        }

    } // else message_on = false;
}
/*
dboolean HU_Responder(event_t *ev)
{
    dboolean       eatkey = false;

    int           i;

    int           numplayers;
    
    numplayers = 0;

    for (i=0 ; i<MAXPLAYERS ; i++)
        numplayers += playeringame[i];

    return eatkey;
}
*/
// hu_newlevel called when we enter a new level
// determine the level name and display it in
// the console
void HU_NewLevel()
{
    char       *s = "Unknown level";

    if(!modifiedgame)
    {
        switch ( logical_gamemission )
        {
          case doom:
            s = HU_TITLE;
            break;
          case doom2:
             if(bfgedition)
                 s = HU_TITLE2_BFG;
             else
                 s = HU_TITLE2;
             break;
          case pack_plut:
            s = HU_TITLEP;
            break;
          case pack_tnt:
            s = HU_TITLET;
            break;
          case pack_nerve:
            if (gamemap <= 9)
              s = HU_TITLEN;
            else
            {
              if(bfgedition)
                s = HU_TITLE2_BFG;
              else
                s = HU_TITLE2;
            }
            break;
          case pack_master:
            if (gamemap <= 21)
              s = HU_TITLEM;
            else
              s = HU_TITLE2;
            break;
          default:
            break;
        }
    }

    if (logical_gamemission == doom && gameversion == exe_chex)
    {
        if(gamemap <= 5)
            s = HU_TITLE_CHEX;
        else
            s = HU_TITLE;
    }
    else if (logical_gamemission == doom2 && gameversion == exe_hacx)
    {
        if (gamemap <= 20)
            s = HU_TITLE_HACX;
        else if(gamemap == 31)
            s = "Desiccant Room";
        else
            s = HU_TITLE2;

        if(gamemap == 33)
            s = HU_TITLE2_BFG;
    }

    // print the new level name into the console

    if (modifiedgame)
    {
        if (mapinfo_lump)
        {
            if(gamemode == commercial)
                s = P_GetMapName(gamemap);
            else
                s = P_GetMapName((gameepisode - 1) * 10 + gamemap);
        }
        else
        {
            char lump[5];

            if(gamemode == commercial)
            {
                if (gamemap < 10)
                    M_snprintf(lump, sizeof(s), "MAP0%i", gamemap);
                else if (gamemap > 9 && gamemap < 20)
                    M_snprintf(lump, sizeof(lump), "MAP1%i", gamemap);
                else if (gamemap > 19 && gamemap < 30)
                    M_snprintf(lump, sizeof(lump), "MAP2%i", gamemap);
                else if (gamemap > 29)
                    M_snprintf(lump, sizeof(lump), "MAP3%i", gamemap);
            }
            else
                M_snprintf(lump, sizeof(lump), "E%iM%i", gameepisode, gamemap);

            s = lump;
        }
    }

    C_Output("");

    C_AddConsoleDivider();

    C_Output("");

    if(gameepisode == 1 && gamemap == 10 && fsize == 12538385)
        C_Output("%s", "E1M10: Sewers");
    else
        C_Output("%s", uppercase(s));

    C_Output("");
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

void HU_PlayerMessage(char *message, dboolean ingame)
{
    static char buffer[1024];
    char        lastchar;

    if (message[0] == '%' && message[1] == 's')
        M_snprintf(buffer, sizeof(buffer), message, playername);
    else
        M_StringCopy(buffer, message, sizeof(buffer));

    buffer[0] = toupper(buffer[0]);
    lastchar = buffer[strlen(buffer) - 1];

    if (plr && !consoleactive && !message_dontfuckwithme)
        plr->message = buffer;

    if (ingame)
        C_PlayerMessage("%s%s", buffer, (lastchar == '.' || lastchar == '!' ? "" : "."));
    else
        C_Output("%s%s", buffer, (lastchar == '.' || lastchar == '!' ? "" : "."));
}

