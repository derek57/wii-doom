// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// Copyright (C) 2015 by Brad Harding: - Status Bar changes (Extra HUD)
//                                     - (Key Card modification for locked doors)
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
//        Status bar code.
//        Does the face/direction indicator animatin.
//        Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------


#include <stdio.h>

#include "am_map.h"
#include "c_io.h"
#include "d_deh.h"
#include "doomdef.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "g_game.h"
#include "hu_stuff.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_tinttab.h"
#include "i_video.h"

#ifndef WII
#include "m_cheat.h"
#endif

#include "m_menu.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_inter.h"
#include "p_local.h"
#include "p_tick.h"
#include "r_local.h"
#include "s_sound.h"
#include "sounds.h"
#include "st_lib.h"
#include "st_stuff.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"
#include "wii-doom.h"
#include "z_zone.h"


//
// STATUS BAR DATA
//

// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS                1
#define STARTBONUSPALS              9
#define NUMREDPALS                  8
#define NUMBONUSPALS                4

// Radiation suit, green shift.
#define RADIATIONPAL                13

// N/256*100% probability
//  that the normal face state will change
#define ST_FACEPROBABILITY          96

// For Responder
#define ST_TOGGLECHAT               KEY_ENTER

// Location of status bar
#define ST_X                        0
#define ST_X2                       104
#define ST_FX                       143
#define ST_FY                       169

// Should be set to patch width
//  for tall numbers later on
#define ST_TALLNUMWIDTH             (tallnum[0]->width)

// Number of status faces.
#define ST_NUMPAINFACES             5
#define ST_NUMSTRAIGHTFACES         3
#define ST_NUMTURNFACES             2
#define ST_NUMSPECIALFACES          3

#define ST_FACESTRIDE \
          (ST_NUMSTRAIGHTFACES + ST_NUMTURNFACES + ST_NUMSPECIALFACES)
#define ST_NUMFACES \
          (ST_FACESTRIDE * ST_NUMPAINFACES + ST_NUMEXTRAFACES)

#define ST_NUMEXTRAFACES            2
#define ST_TURNOFFSET               (ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET               (ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET           (ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET            (ST_EVILGRINOFFSET + 1)
#define ST_GODFACE                  (ST_NUMPAINFACES * ST_FACESTRIDE)
#define ST_DEADFACE                 (ST_GODFACE + 1)
#define ST_FACESX                   143
#define ST_FACESY                   168
#define ST_EVILGRINCOUNT            (2 * TICRATE)
#define ST_STRAIGHTFACECOUNT        (TICRATE / 2)
#define ST_TURNCOUNT                (1 * TICRATE)
#define ST_OUCHCOUNT                (1 * TICRATE)
#define ST_RAMPAGEDELAY             (2 * TICRATE)
#define ST_MUCHPAIN                 20


// Location and size of statistics,
//  justified according to widget type.
// Problem is, within which space? STbar? Screen?
// Note: this could be read in by a lump.
//       Problem is, is the stuff rendered
//       into a buffer,
//       or into the frame buffer?

// AMMO number pos.
#define ST_AMMOWIDTH                3        
#define ST_AMMOX                    44
#define ST_AMMOY                    171

// HEALTH number pos.
#define ST_HEALTHWIDTH              3        
#define ST_HEALTHX                  90
#define ST_HEALTHY                  171

// SCORE number pos.
#define ST_SCOREWIDTH               7
#define ST_SCOREX                   102
#define ST_SCOREY                   171

// ITEM number pos.
#define ST_ITEMWIDTH                2
#define ST_ITEMX                    138
#define ST_ITEMY                    171
#define ST_ITEMBGX                  104
#define ST_ITEMBGY                  168

// CHAT pos.
#define ST_CHATBGX                  104
#define ST_CHATBGY                  168

// Weapon pos.
#define ST_ARMSX                    111
#define ST_ARMSY                    172
#define ST_ARMSBGX                  104
#define ST_ARMSBGY                  168
#define ST_ARMSXSPACE               12
#define ST_ARMSYSPACE               10

// Frags pos.
#define ST_FRAGSX                   138
#define ST_FRAGSY                   171        
#define ST_FRAGSWIDTH               2

// ARMOR number pos.
#define ST_ARMORWIDTH               3
#define ST_ARMORX                   221
#define ST_ARMORY                   171

// Key icon positions.
#define ST_KEY0WIDTH                8
#define ST_KEY0HEIGHT               5
#define ST_KEY0X                    239
#define ST_KEY0Y                    171
#define ST_KEY1WIDTH                ST_KEY0WIDTH
#define ST_KEY1X                    239
#define ST_KEY1Y                    181
#define ST_KEY2WIDTH                ST_KEY0WIDTH
#define ST_KEY2X                    239
#define ST_KEY2Y                    191

// Ammunition counter.
#define ST_AMMO0WIDTH               3
#define ST_AMMO0HEIGHT              6
#define ST_AMMO0X                   288
#define ST_AMMO0Y                   173
#define ST_AMMO1WIDTH               ST_AMMO0WIDTH
#define ST_AMMO1X                   288
#define ST_AMMO1Y                   179
#define ST_AMMO2WIDTH               ST_AMMO0WIDTH
#define ST_AMMO2X                   288
#define ST_AMMO2Y                   191
#define ST_AMMO3WIDTH               ST_AMMO0WIDTH
#define ST_AMMO3X                   288
#define ST_AMMO3Y                   185

// Indicate maximum ammunition.
// Only needed because backpack exists.
#define ST_MAXAMMO0WIDTH            3
#define ST_MAXAMMO0HEIGHT           5
#define ST_MAXAMMO0X                314
#define ST_MAXAMMO0Y                173
#define ST_MAXAMMO1WIDTH            ST_MAXAMMO0WIDTH
#define ST_MAXAMMO1X                314
#define ST_MAXAMMO1Y                179
#define ST_MAXAMMO2WIDTH            ST_MAXAMMO0WIDTH
#define ST_MAXAMMO2X                314
#define ST_MAXAMMO2Y                191
#define ST_MAXAMMO3WIDTH            ST_MAXAMMO0WIDTH
#define ST_MAXAMMO3X                314
#define ST_MAXAMMO3Y                185

// pistol
#define ST_WEAPON0X                 110 
#define ST_WEAPON0Y                 172

// shotgun
#define ST_WEAPON1X                 122 
#define ST_WEAPON1Y                 172

// chain gun
#define ST_WEAPON2X                 134 
#define ST_WEAPON2Y                 172

// missile launcher
#define ST_WEAPON3X                 110 
#define ST_WEAPON3Y                 181

// plasma gun
#define ST_WEAPON4X                 122 
#define ST_WEAPON4Y                 181

// bfg
#define ST_WEAPON5X                 134
#define ST_WEAPON5Y                 181

// WPNS title
#define ST_WPNSX                    109 
#define ST_WPNSY                    191

// DETH title
#define ST_DETHX                    109
#define ST_DETHY                    191

//Incoming messages window location
//UNUSED
//#define ST_MSGTEXTX                 (viewwindowx)
//#define ST_MSGTEXTY                 (viewwindowy + viewheight - 18)

#define ST_MSGTEXTX                 0
#define ST_MSGTEXTY                 0

// Dimensions given in characters.
#define ST_MSGWIDTH                 52

// Or shall I say, in lines?
#define ST_MSGHEIGHT                1

#define ST_OUTTEXTX                 0
#define ST_OUTTEXTY                 6

// Width, in characters again.
#define ST_OUTWIDTH                 52 

// Height, in lines. 
#define ST_OUTHEIGHT                1

#define ST_MAPWIDTH                 (strlen(mapnames[(gameepisode - 1) * 9 + (gamemap - 1)]))

#define ST_MAPTITLEX                (ORIGINALWIDTH - ST_MAPWIDTH * ST_CHATFONTWIDTH)

#define ST_MAPTITLEY                0
#define ST_MAPHEIGHT                1

#define NONE                        -1
#define IDMUS_MAX                   50


int mus[IDMUS_MAX][6] =
{
    /* xy      shareware    registered   commercial   retail      bfgedition   nerve      */
    /* 00 */ { NONE,        NONE,        NONE,        NONE,       NONE,        NONE       },
    /* 01 */ { NONE,        NONE,        mus_runnin,  NONE,       mus_runnin,  mus_messag },
    /* 02 */ { NONE,        NONE,        mus_stalks,  NONE,       mus_stalks,  mus_ddtblu },
    /* 03 */ { NONE,        NONE,        mus_countd,  NONE,       mus_countd,  mus_doom   },
    /* 04 */ { NONE,        NONE,        mus_betwee,  NONE,       mus_betwee,  mus_shawn  },
    /* 05 */ { NONE,        NONE,        mus_doom,    NONE,       mus_doom,    mus_in_cit },
    /* 06 */ { NONE,        NONE,        mus_the_da,  NONE,       mus_the_da,  mus_the_da },
    /* 07 */ { NONE,        NONE,        mus_shawn,   NONE,       mus_shawn,   mus_in_cit },
    /* 08 */ { NONE,        NONE,        mus_ddtblu,  NONE,       mus_ddtblu,  mus_shawn  },
    /* 09 */ { NONE,        NONE,        mus_in_cit,  NONE,       mus_in_cit,  mus_ddtblu },
    /* 10 */ { NONE,        NONE,        mus_dead,    NONE,       mus_dead,    NONE       },
    /* 11 */ { mus_e1m1,    mus_e1m1,    mus_stlks2,  mus_e1m1,   mus_stlks2,  NONE       },
    /* 12 */ { mus_e1m2,    mus_e1m2,    mus_theda2,  mus_e1m2,   mus_theda2,  NONE       },
    /* 13 */ { mus_e1m3,    mus_e1m3,    mus_doom2,   mus_e1m3,   mus_doom2,   NONE       },
    /* 14 */ { mus_e1m4,    mus_e1m4,    mus_ddtbl2,  mus_e1m4,   mus_ddtbl2,  NONE       },
    /* 15 */ { mus_e1m5,    mus_e1m5,    mus_runni2,  mus_e1m5,   mus_runni2,  NONE       },
    /* 16 */ { mus_e1m6,    mus_e1m6,    mus_dead2,   mus_e1m6,   mus_dead2,   NONE       },
    /* 17 */ { mus_e1m7,    mus_e1m7,    mus_stlks3,  mus_e1m7,   mus_stlks3,  NONE       },
    /* 18 */ { mus_e1m8,    mus_e1m8,    mus_romero,  mus_e1m8,   mus_romero,  NONE       },
    /* 19 */ { mus_e1m9,    mus_e1m9,    mus_shawn2,  mus_e1m9,   mus_shawn2,  NONE       },
    /* 20 */ { NONE,        NONE,        mus_messag,  NONE,       mus_messag,  NONE       },
    /* 21 */ { NONE,        mus_e2m1,    mus_count2,  mus_e2m1,   mus_count2,  NONE       },
    /* 22 */ { NONE,        mus_e2m2,    mus_ddtbl3,  mus_e2m2,   mus_ddtbl3,  NONE       },
    /* 23 */ { NONE,        mus_e2m3,    mus_ampie,   mus_e2m3,   mus_ampie,   NONE       },
    /* 24 */ { NONE,        mus_e2m4,    mus_theda3,  mus_e2m4,   mus_theda3,  NONE       },
    /* 25 */ { NONE,        mus_e2m5,    mus_adrian,  mus_e2m5,   mus_adrian,  NONE       },
    /* 26 */ { NONE,        mus_e2m6,    mus_messg2,  mus_e2m6,   mus_messg2,  NONE       },
    /* 27 */ { NONE,        mus_e2m7,    mus_romer2,  mus_e2m7,   mus_romer2,  NONE       },
    /* 28 */ { NONE,        mus_e2m8,    mus_tense,   mus_e2m8,   mus_tense,   NONE       },
    /* 29 */ { NONE,        mus_e2m9,    mus_shawn3,  mus_e2m9,   mus_shawn3,  NONE       },
    /* 30 */ { NONE,        NONE,        mus_openin,  NONE,       mus_openin,  NONE       },
    /* 31 */ { NONE,        mus_e3m1,    mus_evil,    mus_e3m1,   mus_evil,    NONE       },
    /* 32 */ { NONE,        mus_e3m2,    mus_ultima,  mus_e3m2,   mus_ultima,  NONE       },
    /* 33 */ { NONE,        mus_e3m3,    NONE,        mus_e3m3,   mus_read_m,  NONE       },
    /* 34 */ { NONE,        mus_e3m4,    NONE,        mus_e3m4,   NONE,        NONE       },
    /* 35 */ { NONE,        mus_e3m5,    NONE,        mus_e3m5,   NONE,        NONE       },
    /* 36 */ { NONE,        mus_e3m6,    NONE,        mus_e3m6,   NONE,        NONE       },
    /* 37 */ { NONE,        mus_e3m7,    NONE,        mus_e3m7,   NONE,        NONE       },
    /* 38 */ { NONE,        mus_e3m8,    NONE,        mus_e3m8,   NONE,        NONE       },
    /* 39 */ { NONE,        mus_e3m9,    NONE,        mus_e3m9,   NONE,        NONE       },
    /* 40 */ { NONE,        NONE,        NONE,        NONE,       NONE,        NONE       },
    /* 41 */ { NONE,        NONE,        NONE,        mus_e3m4,   NONE,        NONE       },
    /* 42 */ { NONE,        NONE,        NONE,        mus_e3m2,   NONE,        NONE       },
    /* 43 */ { NONE,        NONE,        NONE,        mus_e3m3,   NONE,        NONE       },
    /* 44 */ { NONE,        NONE,        NONE,        mus_e1m5,   NONE,        NONE       },
    /* 45 */ { NONE,        NONE,        NONE,        mus_e2m7,   NONE,        NONE       },
    /* 46 */ { NONE,        NONE,        NONE,        mus_e2m4,   NONE,        NONE       },
    /* 47 */ { NONE,        NONE,        NONE,        mus_e2m6,   NONE,        NONE       },
    /* 48 */ { NONE,        NONE,        NONE,        mus_e2m5,   NONE,        NONE       },
    /* 49 */ { NONE,        NONE,        NONE,        mus_e1m9,   NONE,        NONE       }
};


// main player in game
static player_t               *plyr; 

// ST_Start() has just been called
static dboolean               st_firsttime;

// lump number for PLAYPAL
static int                    lu_palette;

// used for making messages go away
static int                    st_msgcounter = 0;

// number of frags so far in deathmatch
static int                    st_fragscount;

// number of items so far
static int                    st_itemcount;

// number of score so far
static int                    st_scorecount;

// used to use appopriately pained face
static int                    st_oldhealth = -1;

// count until face changes
static int                    st_facecount = 0;

// current face index, used by w_faces
static int                    st_faceindex = 0;

// holds key-type for each key box on bar
static int                    keyboxes[3]; 

// a random number per tick
static int                    st_randomnumber;  

static int                    st_palette = 0;

// used for timing
static unsigned int           st_clock;

// used when in chat 
static st_chatstateenum_t     st_chatstate;

// whether in automap or first-person
static st_stateenum_t         st_gamestate;

// whether left-side main status bar is active
static dboolean               st_statusbaron;

// whether status bar chat is active
static dboolean               st_chat;

// value of st_chat before message popped up
static dboolean               st_oldchat;

// whether chat window has the cursor on
static dboolean               st_cursoron;

// !deathmatch
static dboolean               st_notdeathmatch; 

// !deathmatch && st_statusbaron
static dboolean               st_armson;

// !deathmatch
static dboolean               st_fragson; 

// !deathmatch
static dboolean               st_itemon; 

// !deathmatch
static dboolean               st_chaton; 

// !deathmatch
static dboolean               st_scoreon; 

// used for evil grin
static dboolean               oldweaponsowned[NUMWEAPONS]; 

static dboolean               st_stopped = true;

// main bar left
static patch_t                *sbar;

static patch_t                *sbarmap;
static patch_t                *sbar_left_oldwad;
static patch_t                *sbar_right_oldwad;
static patch_t                *sbara_shotgun;
static patch_t                *sbara_chaingun;
static patch_t                *sbara_missile;
static patch_t                *sbara_plasma;
static patch_t                *sbara_bfg;
static patch_t                *sbara_chainsaw;

// 0-9, short, yellow (,different!) numbers
static patch_t                *shortnum[10];

// 3 key-cards, 3 skulls
static patch_t                *keys[NUMCARDS]; 

// face status patches
static patch_t                *faces[ST_NUMFACES];

// face background
static patch_t                *faceback;

// main item middle
static patch_t                *itembg;

// main chat middle
static patch_t                *chatbg;

// main bar right
static patch_t                *armsbg;

// weapon ownership patches
static patch_t                *arms[6][2]; 

// ready-weapon widget
static st_number_t            w_ready;

// in deathmatch only, summary of frags stats
static st_number_t            w_frags;

// health widget
static st_percent_t           w_health;

// item background
static st_binicon_t           w_itembg; 

// chat background
static st_binicon_t           w_chatbg; 

// arms background
static st_binicon_t           w_armsbg; 

// weapon ownership widgets
static st_multicon_t          w_arms[6];

// face status widget
static st_multicon_t          w_faces; 

// keycard widgets
static st_multicon_t          w_keyboxes[3];

// armor widget
static st_percent_t           w_armor;

// item widget
static st_number_t            w_item;

// score widget
static st_number_t            w_score;

// ammo widgets
static st_number_t            w_ammo[4];

// max ammo widgets
static st_number_t            w_maxammo[4]; 

// 0-9, tall numbers
patch_t                       *tallnum[10];

// tall % sign
patch_t                       *tallpercent;

cheatseq_t cheat_mus = CHEAT("idmus", 2);
cheatseq_t cheat_massacre = CHEAT("idmassacre", 0);
cheatseq_t cheat_god = CHEAT("iddqd", 0);
cheatseq_t cheat_god_beta = CHEAT("tst", 0);
cheatseq_t cheat_fly = CHEAT("idfly", 0);
cheatseq_t cheat_ammo = CHEAT("idkfa", 0);
cheatseq_t cheat_ammo_beta = CHEAT("amo", 0);
cheatseq_t cheat_ammonokey = CHEAT("idfa", 0);
cheatseq_t cheat_noclip = CHEAT("idspispopd", 0);
cheatseq_t cheat_noclip_beta = CHEAT("nc", 0);
cheatseq_t cheat_commercial_noclip = CHEAT("idclip", 0);

cheatseq_t        cheat_powerup[7] =
{
    CHEAT("idbeholdv", 0),
    CHEAT("idbeholds", 0),
    CHEAT("idbeholdi", 0),
    CHEAT("idbeholdr", 0),
    CHEAT("idbeholda", 0),
    CHEAT("idbeholdl", 0),
    CHEAT("idbehold", 0)
};

cheatseq_t cheat_choppers = CHEAT("idchoppers", 0);
cheatseq_t cheat_clev = CHEAT("idclev", 2);
cheatseq_t cheat_mypos = CHEAT("idmypos", 0);
cheatseq_t cheat_goobers = CHEAT("goobers", 0);


int                 prio = 0;
int                 healthhighlight = 0;
int                 ammohighlight = 0;
int                 armorhighlight = 0;
int                 cheat_musnum;

dboolean            mus_cheated;
dboolean            fly_used;

extern channel_t    channels[8];

extern dboolean     BorderNeedRefresh;
extern dboolean     in_slime;
extern dboolean     show_chat_bar;
extern dboolean     done;
extern dboolean     massacre_cheat_used;
extern dboolean     mus_cheat_used;

extern char         massacre_textbuffer[30];

extern void         A_PainDie(mobj_t *actor);


//
// STATUS BAR CODE
//
void ST_Stop (void)
{
    if (st_stopped)
        return;

    I_SetPalette (W_CacheLumpNum (lu_palette, PU_CACHE));

    st_stopped = true;
}

void ST_refreshBackground(void)
{
    plyr = &players[consoleplayer];

    if (st_statusbaron)
    {
        //V_UseBuffer(st_backing_screen);

        if (beta_style)
        {
            if (automapactive)
            {
                V_DrawPatch(ST_X, 0, 4, sbarmap);

                if (plyr->weaponowned[wp_shotgun])
                    V_DrawPatch(110, 4, 4, sbara_shotgun);

                if (plyr->weaponowned[wp_chaingun])
                    V_DrawPatch(110, 10, 4, sbara_chaingun);

                if (plyr->weaponowned[wp_missile])
                    V_DrawPatch(135, 3, 4, sbara_missile);

                if (plyr->weaponowned[wp_plasma])
                    V_DrawPatch(135, 10, 4, sbara_plasma);

                if (plyr->weaponowned[wp_bfg])
                    V_DrawPatch(185, 3, 4, sbara_bfg);

                if (plyr->weaponowned[wp_chainsaw])
                    V_DrawPatch(160, 5, 4, sbara_chainsaw);
            }
            else
            {
                if (fsize == 4207819 || fsize == 4274218 || fsize == 10396254)
                {
                    V_DrawPatch(ST_X, 0, 4, sbar_left_oldwad);
                    V_DrawPatch(104, 0, 4, sbar_right_oldwad);
                }
                else
                    V_DrawPatch(ST_X, 0, 4, sbar);
            }
        }
        else
        {
            if (fsize == 4207819 || fsize == 4274218 || fsize == 10396254)
            {
                V_DrawPatch(ST_X, 0, 4, sbar_left_oldwad);
                V_DrawPatch(104, 0, 4, sbar_right_oldwad);
            }
            else
                V_DrawPatch(ST_X, 0, 4, sbar);
        }

        if (netgame)
            V_DrawPatch(ST_FX, 0, 4, faceback);
        else if (beta_style && !automapactive)
            V_DrawPatch(ST_FX - 1, 1, 4, faceback);

        //V_RestoreBuffer();

        V_CopyRect(ST_X, 0, 4, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y, 0);
    }
}

// Respond to keyboard input events,
//  intercept cheats.
dboolean ST_Responder(event_t *ev)
{
    // Filter automap on/off.
    if (ev->type == ev_keyup && ((ev->data1 & 0xffff0000) == AM_MSGHEADER))
    {
        switch (ev->data1)
        {
            case AM_MSGENTERED:
                st_gamestate = AutomapState;
                st_firsttime = true;
                break;
        
            case AM_MSGEXITED:
                //fprintf(stderr, "AM exited\n");
                st_gamestate = FirstPersonState;
                break;
        }
    }

#ifndef WII
    // if a user keypress...
    else if (ev->type == ev_keydown)
    {
        if (!netgame && gameskill != sk_nightmare)
        {
            int i;

            // 'massacre' cheat for monsters instant death
            if (cht_CheckCheat(&cheat_massacre, ev->data2) && !beta_style)
            {
                thinker_t *thinker;
                int       killcount = 0;

                massacre_cheat_used = true;

                // jff 02/01/98 'em' cheat - kill all monsters
                // partially taken from Chi's .46 port

                // killough 2/7/98: cleaned up code and changed to use dprintf;
                // fixed lost soul bug (Lost Souls left behind when Pain Elementals are killed)
                thinker = &thinkercap;

                while ((thinker=thinker->next) != &thinkercap)
                {
                    if (thinker->function == P_MobjThinker &&
                        ((((mobj_t *)thinker)->flags & MF_COUNTKILL) ||
                        ((mobj_t *)thinker)->type == MT_SKULL ||
                        ((mobj_t *)thinker)->type == MT_BETASKULL))
                    {
                        // killough 3/6/98: kill even if Pain Elemental is dead
                        if (((mobj_t *)thinker)->health > 0)
                        {
                            killcount++;
                            P_DamageMobj((mobj_t *)thinker, NULL, NULL, 10000);
                        }

                        if (((mobj_t *)thinker)->type == MT_PAIN)
                        {
                            // killough 2/8/98
                            A_PainDie((mobj_t *)thinker);

                            P_SetMobjState((mobj_t *)thinker, S_PAIN_DIE6);
                        }
                    }
                }

                // killough 3/22/98: make more intelligent about plural
                // Ty 03/27/98 - string(s) *not* externalized
                sprintf(massacre_textbuffer, "%d MONSTER%s KILLED",
                    killcount, killcount == 1 ? "" : "S");

                players[consoleplayer].message = massacre_textbuffer;
                massacre_cheat_used = false;
            }
            // 'fly' cheat for player flying
            else if (cht_CheckCheat(&cheat_fly, ev->data2) && !beta_style)
            {
                if (!fly_used)
                {
                    P_UseArtifact(plyr, arti_fly);
                    players[consoleplayer].mo->flags2 |= MF2_FLY;
                    players[consoleplayer].mo->flags |= MF_NOGRAVITY;
                    fly_used = true;
                }
                else
                {
                    plyr[consoleplayer].powers[6] = 0;
                    players[consoleplayer].mo->flags2 &= ~MF2_FLY;
                    players[consoleplayer].mo->flags &= ~MF_NOGRAVITY;
                    fly_used = false;
                }
            }
            // 'dqd' cheat for toggleable god mode
            else if (cht_CheckCheat(&cheat_god, ev->data2) && !beta_style)
            {
                // [BH] if player is dead, resurrect them first
                if (!plyr->health)
                    P_ResurrectPlayer(plyr);

                plyr->cheats ^= CF_GODMODE;

                if (aiming_help)
                    aiming_help = false;

                if (plyr->cheats & CF_GODMODE)
                {
                    if (plyr->mo)
                        plyr->mo->health = 100;
          
                    plyr->health = 100;
                    HU_PlayerMessage(s_STSTR_DQDON, true);
                }
                else 
                    HU_PlayerMessage(s_STSTR_DQDOFF, true);
            }
            // 'tst' cheat for toggleable god mode in beta
            else if (cht_CheckCheat(&cheat_god_beta, ev->data2) && beta_style)
            {
                plyr->cheats ^= CF_GODMODE;

                if (plyr->cheats & CF_GODMODE)
                {
                    if (plyr->mo)
                        plyr->mo->health = 100;
          
                    plyr->health = 100;                       
                    plyr->message = STSTR_DQDONBETA;
                }
                else 
                    plyr->message = STSTR_DQDOFFBETA;
            }
            // 'fa' cheat for killer fucking arsenal
            else if (cht_CheckCheat(&cheat_ammonokey, ev->data2) && !beta_style)
            {
                if (fsize == 11159840 || fsize == 12408292 || fsize == 12474561 ||
                    fsize == 12487824 || fsize == 12538385 || fsize == 4234124 ||
                    fsize == 4196020 || fsize == 14943400 || fsize == 14824716 ||
                    fsize == 14612688 || fsize == 14607420 || fsize == 14604584 ||
                    fsize == 14677988 || fsize == 14691821 || fsize == 14683458 ||
                    fsize == 18195736 || fsize == 18654796 || fsize == 18240172 ||
                    fsize == 17420824 || fsize == 28422764 || fsize == 12361532 ||
                    fsize == 19321722)
                {
                    plyr->armorpoints = idfa_armor;
                    plyr->armortype = idfa_armor_class;
        
                    for (i = 0; i < NUMWEAPONS; i++)
                        plyr->weaponowned[i] = true;
        
                    for (i = 0; i < NUMAMMO; i++)
                        plyr->ammo[i] = plyr->maxammo[i];
        
                    HU_PlayerMessage(s_STSTR_FAADDED, true);
                }
            }
            // 'kfa' cheat for key full ammo
            else if (cht_CheckCheat(&cheat_ammo, ev->data2) && !beta_style)
            {
                plyr->armorpoints = idkfa_armor;
                plyr->armortype = idkfa_armor_class;
        
                for (i = 0; i < NUMWEAPONS; i++)
                    plyr->weaponowned[i] = true;
        
                for (i = 0; i < NUMAMMO; i++)
                    plyr->ammo[i] = plyr->maxammo[i];

                P_GiveAllCards(plyr);       
                HU_PlayerMessage(s_STSTR_KFAADDED, true);
            }
            // 'amo' cheat for key full ammo in beta
            else if (cht_CheckCheat(&cheat_ammo_beta, ev->data2) && beta_style)
            {
                plyr->armorpoints = idkfa_armor;
                plyr->armortype = idkfa_armor_class;
        
                for (i = 0; i < NUMWEAPONS; i++)
                    plyr->weaponowned[i] = true;
        
                for (i = 0; i < NUMAMMO; i++)
                    plyr->ammo[i] = plyr->maxammo[i];

                for (i = 0; i < NUMCARDS - 3; i++)
                    plyr->cards[i] = true;

                plyr->message = STSTR_KFAADDEDBETA;
            }
            // 'mus' cheat for changing music
            else if (cht_CheckCheat(&cheat_mus, ev->data2) && !beta_style)
            {
                char        buf[3];

                cht_GetParam(&cheat_mus, buf);

                // [BH] rewritten to use mus[] LUT
                // [BH] fix crash if IDMUS0y and IDMUSx0 entered in DOOM,
                //  IDMUS21 to IDMUS39 entered in shareware, and IDMUS00
                //  entered in DOOM II
                if (buf[0] >= '0' && buf[0] <= '9' && buf[1] >= '0' && buf[1] <= '9')
                {
                    int musnum = (buf[0] - '0') * 10 + (buf[1] - '0');

                    if (musnum < IDMUS_MAX)
                    {
                        if (gamemission == pack_nerve)
                            musnum = mus[musnum][5];
                        else if (bfgedition && gamemission == doom2)
                            musnum = mus[musnum][4];
                        else
                            musnum = mus[musnum][gamemode];

                        cheat_musnum = musnum;
                        mus_cheated = true;

                        if (musnum != NONE)
                        {
                            static char msg[80];

                            S_ChangeMusic(musnum, true, false);

                            M_snprintf(msg, sizeof(msg), s_STSTR_MUS, S_music[musnum].name);
                            HU_PlayerMessage(msg, true);
                        }
                    }
                    else
                        HU_PlayerMessage("invalid music", true);
                }
            }
            // Noclip cheat.
            // For Doom 1, use the idspipsopd cheat; for all others, use idclip
            else if (((logical_gamemission == doom &&
                                cht_CheckCheat(&cheat_noclip, ev->data2)) ||
                      (logical_gamemission != doom &&
                                cht_CheckCheat(&cheat_commercial_noclip, ev->data2))) && !beta_style)
            {
                plyr->cheats ^= CF_NOCLIP;
        
                if (plyr->cheats & CF_NOCLIP)
                {
                    plyr->mo->flags |= MF_NOCLIP;
                    HU_PlayerMessage(s_STSTR_NCON, true);
                }
                else
                {
                    plyr->mo->flags &= ~MF_NOCLIP;
                    HU_PlayerMessage(s_STSTR_NCOFF, true);
                }
            }
            // Noclip cheat in beta.
            else if (cht_CheckCheat(&cheat_noclip_beta, ev->data2) && beta_style)
            {
                plyr->cheats ^= CF_NOCLIP;
        
                if (plyr->cheats & CF_NOCLIP)
                {
                    plyr->mo->flags |= MF_NOCLIP;
                    plyr->message = STSTR_NCONBETA;
                }
                else
                {
                    plyr->mo->flags &= ~MF_NOCLIP;
                    plyr->message = STSTR_NCOFFBETA;
                }
            }
            // [crispy] implement Crispy Doom's "goobers" cheat, ne easter egg
            else if (cht_CheckCheat(&cheat_goobers, ev->data2))
            {
                static char msg[80];

                extern void EV_DoGoobers (void);

                EV_DoGoobers();

                M_snprintf(msg, sizeof(msg), "Get Psyched!");
                plyr->message = msg;
            }

            // 'behold?' power-up cheats
            for (i = 0; i < 6; i++)
            {
                if (cht_CheckCheat(&cheat_powerup[i], ev->data2) && !beta_style)
                {
                    if (!plyr->powers[i])
                        P_GivePower(plyr, i);
                    else if (i!=pw_strength)
                        plyr->powers[i] = 1;
                    else
                        plyr->powers[i] = 0;

                    HU_PlayerMessage(s_STSTR_BEHOLDX, true);
                }
            }

            // 'behold' power-up menu
            if (cht_CheckCheat(&cheat_powerup[6], ev->data2) && !beta_style)
            {
                HU_PlayerMessage(s_STSTR_BEHOLD, true);
            }
            // 'choppers' invulnerability & chainsaw
            else if (cht_CheckCheat(&cheat_choppers, ev->data2) && !beta_style)
            {
                plyr->weaponowned[wp_chainsaw] = true;
                plyr->powers[pw_invulnerability] = true;
                HU_PlayerMessage(s_STSTR_CHOPPERS, true);
            }
            // 'mypos' for player position
            else if (cht_CheckCheat(&cheat_mypos, ev->data2) && !beta_style)
            {
                static char buf[ST_MSGWIDTH];

                M_snprintf(buf, sizeof(buf), "ang=0x%x;x,y=(0x%x,0x%x)",
                        players[consoleplayer].mo->angle,
                        players[consoleplayer].mo->x,
                        players[consoleplayer].mo->y);
                HU_PlayerMessage(buf, true);
            }
        }

        if (!netgame && cht_CheckCheat(&cheat_clev, ev->data2) && !beta_style)
        {
            char        buf[3];

            cht_GetParam(&cheat_clev, buf);

            if (buf[0] == '1' && buf[1] == '0')
            {
                // 'clev10' change-level cheat
                if ((fsize == 12538385 && gamemode == retail) || gamemode == commercial)
                {
                    HU_PlayerMessage(STSTR_CLEV, true);
                    G_DeferedInitNew(gameskill, 1, 10);
                }
                else
                {
                    HU_PlayerMessage("invalid map", true);
                    return false;
                }
            }
            else
            {
                // 'clev' change-level cheat
                char        lump[6];
                int         epsd;
                int         map;

                if (gamemode == commercial)
                {
                    epsd = 1;
                    map = (buf[0] - '0') * 10 + buf[1] - '0';
                    M_snprintf(lump, sizeof(lump), "MAP%c%c", buf[0], buf[1]);
                }
                else
                {
                    epsd = buf[0] - '0';
                    map = buf[1] - '0';
                    M_snprintf(lump, sizeof(lump), "E%cM%c", buf[0], buf[1]);

                    // Chex.exe always warps to episode 1.
                    if (gameversion == exe_chex)
                    {
                        if (epsd > 1)
                        {
                            epsd = 1;
                        }

                        if (map > 5)
                        {
                            map = 5;
                        }
                    }
                }

                // [BH] only allow MAP01 to MAP09 when NERVE.WAD loaded
                if (W_CheckNumForName(lump) < 0
                        || (gamemission == pack_nerve && map > 9)
                        || (BTSX && W_CheckMultipleLumps(lump) == 1))
                {
                    HU_PlayerMessage("invalid map", true);
                    return false;
                }

                // Catch invalid maps.
                if (epsd < 1)
                {
                    HU_PlayerMessage("invalid map", true);
                    return false;
                }

                if (map < 1)
                {
                    HU_PlayerMessage("invalid map", true);
                    return false;
                }

                // Ohmygod - this is not going to work.
                if ((gamemode == retail) && ((epsd > 4) || (map > 9)))
                {
                    HU_PlayerMessage("invalid map", true);
                    return false;
                }

                if ((gamemode == registered) && ((epsd > 3) || (map > 9)))
                {
                    HU_PlayerMessage("invalid map", true);
                    return false;
                }

                if ((gamemode == shareware) && ((epsd > 1) || (map > 9)))
                {
                    HU_PlayerMessage("invalid map", true);
                    return false;
                }

                // The source release has this check as map > 34. However, Vanilla
                // Doom allows IDCLEV up to MAP40 even though it normally crashes.
                if ((gamemode == commercial) && ((epsd > 1) || (map > 40)))
                {
                    HU_PlayerMessage("invalid map", true);
                    return false;
                }

                // So be it.
                HU_PlayerMessage(STSTR_CLEV, true);
                G_DeferedInitNew(gameskill, epsd, map);
            }
        }
    }
#endif

    return false;
}

int ST_calcPainOffset(void)
{
    int               health;
    static int        lastcalc;
    static int        oldhealth = -1;
    
    health = plyr->health > 100 ? 100 : plyr->health;

    if (health != oldhealth)
    {
        lastcalc = ST_FACESTRIDE * (((100 - health) * ST_NUMPAINFACES) / 101);
        oldhealth = health;
    }

    return lastcalc;
}

//
// This is a not-very-pretty routine which handles
//  the face states and their timing.
// the precedence of expressions is:
//  dead > evil grin > turned head > straight ahead
//
void ST_updateFaceWidget(void)
{
    int                i;
    static int         priority = 0;

    if (priority < 10)
    {
        // dead
        if (!plyr->health)
        {
            priority = 9;
            st_faceindex = ST_DEADFACE;
            st_facecount = 1;
        }
    }

    if (priority < 9)
    {
        if (plyr->bonuscount)
        {
            dboolean doevilgrin = false;

            // picking up bonus
            for (i = 0; i < NUMWEAPONS; i++)
            {
                if (oldweaponsowned[i] != plyr->weaponowned[i])
                {
                    doevilgrin = true;
                    oldweaponsowned[i] = plyr->weaponowned[i];
                }
            }

            if (doevilgrin) 
            {
                // evil grin if just picked up weapon
                priority = 8;
                st_facecount = ST_EVILGRINCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_EVILGRINOFFSET;
            }
        }
    }
  
    if (priority < 8)
    {
        if (plyr->damagecount && plyr->attacker && plyr->attacker != plyr->mo)
        {
            // being attacked
            prio = priority = 7;
            
            // haleyjd 10/12/03: classic DOOM problem of missing OUCH face
            // was due to inversion of this test:
            // if (plyr->health - st_oldhealth > ST_MUCHPAIN)
            // e6y: compatibility optioned
            if ((d_ouchface ? (plyr->health - st_oldhealth):
                (st_oldhealth - plyr->health)) > ST_MUCHPAIN)
            {
                // e6y
                // There are TWO bugs in the ouch face code.
                // Not only was the condition reversed, but the priority system is
                // broken in a way that makes the face not work with monster damage.
                if (!d_ouchface)
                    priority = 8;

                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
            }
            else
            {
                angle_t diffang;
                angle_t badguyangle = R_PointToAngle2(plyr->mo->x,
                                                      plyr->mo->y,
                                                      plyr->attacker->x,
                                                      plyr->attacker->y);
                
                // confusing, aint it?
                if (badguyangle > plyr->mo->angle)
                {
                    // whether right or left
                    diffang = badguyangle - plyr->mo->angle;
                    i = diffang > ANG180; 
                }
                else
                {
                    // whether left or right
                    diffang = plyr->mo->angle - badguyangle;
                    i = diffang <= ANG180; 
                }
                
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset();
                
                if (diffang < ANG45)
                {
                    // head-on    
                    st_faceindex += ST_RAMPAGEOFFSET;
                }
                else if (i)
                {
                    // turn face right
                    st_faceindex += ST_TURNOFFSET;
                }
                else
                {
                    // turn face left
                    st_faceindex += ST_TURNOFFSET+1;
                }
            }
        }
    }
  
    if (priority < 7 ||
       (beta_style && in_slime && !(plyr->cheats & CF_GODMODE)))
    {
        // getting hurt because of your own damn stupidity
        if (plyr->damagecount ||
           (beta_style && in_slime && !(plyr->cheats & CF_GODMODE)))
        {
            // haleyjd 10/12/03: classic DOOM problem of missing OUCH face
            // was due to inversion of this test:
            // if (plyr->health - st_oldhealth > ST_MUCHPAIN)
            // e6y: compatibility optioned
            if (((d_ouchface? (plyr->health - st_oldhealth):
                (st_oldhealth - plyr->health)) > ST_MUCHPAIN) ||
                (beta_style && in_slime && !(plyr->cheats & CF_GODMODE)))
            {
                priority = 7;
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;

                if (beta_style && in_slime)
                    in_slime = false;
            }
            else
            {
                priority = 6;
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
            }
        }
    }
  
    if (priority < 6)
    {
        static int lastattackdown = -1;

        // rapid firing
        if (plyr->attackdown)
        {
            if (lastattackdown == -1)
                lastattackdown = ST_RAMPAGEDELAY;
            else if (!--lastattackdown)
            {
                priority = 5;
                st_faceindex = ST_calcPainOffset() + ST_RAMPAGEOFFSET;
                st_facecount = 1;
                lastattackdown = 1;
            }
        }
        else
            lastattackdown = -1;
    }
  
    if (priority < 5)
    {
        // invulnerability
        if ((plyr->cheats & CF_GODMODE) || plyr->powers[pw_invulnerability])
        {
            priority = 4;

            st_faceindex = ST_GODFACE;
            st_facecount = 1;
        }
    }

    // look left or look right if the facecount has timed out
    if (!st_facecount)
    {
        st_faceindex = ST_calcPainOffset() + (st_randomnumber % 3);
        st_facecount = ST_STRAIGHTFACECOUNT;
        priority = 0;
    }

    st_facecount--;
}

void ST_updateWidgets(void)
{
    // means "n/a"
    static int         largeammo = 1994;

    int                i;

    // must redirect the pointer if the ready weapon has changed.
    // if (w_ready.data != plyr->readyweapon)
    // {
    if (weaponinfo[plyr->readyweapon].ammo == am_noammo)
        w_ready.num = &largeammo;
    else
        w_ready.num = &plyr->ammo[weaponinfo[plyr->readyweapon].ammo];
    // {
    // static int tic = 0;
    // static int dir = -1;
    //
    // if (!(tic & 15))
    //   plyr->ammo[weaponinfo[plyr->readyweapon].ammo] += dir;
    //
    // if (plyr->ammo[weaponinfo[plyr->readyweapon].ammo] == -100)
    //   dir = 1;
    //
    // tic++;
    // }
    w_ready.data = plyr->readyweapon;

    // if (*w_ready.on)
    //  STlib_updateNum(&w_ready, true);
    //
    // refresh weapon change
    //  }

    // update keycard multiple widgets
    for (i = 0; i < 3; i++)
    {
        keyboxes[i] = (plyr->cards[i] > 0 ? i : -1);

        if (plyr->cards[i + 3] > 0)
            keyboxes[i] = i + 3;
    }

    // refresh everything if this is him coming back to life
    ST_updateFaceWidget();

    // used by the w_armsbg & w_itembg & w_chatbg widget
    st_notdeathmatch = !deathmatch;
    
    // used by w_arms[] widgets
    st_armson = st_statusbaron && !deathmatch;

    // used by w_frags widget
    st_fragson = deathmatch && !st_statusbaron; 
    st_fragscount = 0;

    // used by w_item widget
    st_itemon = beta_style && st_statusbaron && !deathmatch;
    st_itemcount = plyr->item;

    // used by w_chat widget
    st_chaton = beta_style && st_statusbaron && show_chat_bar && !deathmatch;

    // used by w_score widget
    st_scoreon = beta_style && st_statusbaron && automapactive && !deathmatch;
    st_scorecount = plyr->score; 

    for (i = 0; i < MAXPLAYERS; i++)
    {
        if (i != consoleplayer)
            st_fragscount += plyr->frags[i];
        else
            st_fragscount -= plyr->frags[i];
    }

    // get rid of chat window if up because of message
    if (!--st_msgcounter)
        st_chat = st_oldchat;
}

void ST_Ticker(void)
{
    st_clock++;
    st_randomnumber = M_Random();
    ST_updateWidgets();
    st_oldhealth = plyr->health;
}

void ST_doPaletteStuff(void)
{

    int                palette;
    int                cnt = plyr->damagecount;

    if (plyr->powers[pw_strength] && !beta_style)
    {
        // slowly fade the berzerk out
        int                bzc = 12 - (plyr->powers[pw_strength] >> 6);

        if (bzc > cnt)
            cnt = bzc;
    }
        
    if (cnt)
    {
        palette = (cnt + 7) >> 3;
        
        if (palette >= NUMREDPALS)
            palette = NUMREDPALS - 1;

        palette += STARTREDPALS;
    }
    else if (plyr->bonuscount)
    {
        palette = (plyr->bonuscount + 7) >> 3;

        if (palette >= NUMBONUSPALS)
            palette = NUMBONUSPALS - 1;

        palette += STARTBONUSPALS;
    }
    else if ((plyr->powers[pw_ironfeet] > 4 * 32
              || (plyr->powers[pw_ironfeet] & 8)) && !beta_style)
        palette = RADIATIONPAL;
    else if ((plyr->powers[pw_infrared] > 4 * 32
              || (plyr->powers[pw_infrared] & 8)) && beta_style)
        palette = RADIATIONPAL;
    else
        palette = 0;

    if (palette != st_palette)
    {
        byte     *pal;

        st_palette = palette;
        pal = (byte *)W_CacheLumpNum(lu_palette, PU_CACHE) + palette * 768;
        I_SetPalette(pal);
    }
}

//
// Completely changed for PRE-BETA functionality
// maybe needs more optimization but at least it's working
//
void ST_drawWidgets(dboolean refresh)
{
    if ((beta_style && !automapactive) || !beta_style)
    {
        int                i;

        // used by w_arms[] widgets
        st_armson = st_statusbaron && !deathmatch;

        // used by w_frags widget
        st_fragson = deathmatch && !st_statusbaron; 

        // used by w_item widget
        st_itemon = beta_style && st_statusbaron && !deathmatch; 

        // used by w_chat widget
        st_chaton = beta_style && st_statusbaron && show_chat_bar && !deathmatch;

        // used by w_score widget
        st_scoreon = beta_style && st_statusbaron && !deathmatch;

        STlib_updateNum(&w_ready, refresh);

        STlib_updatePercent(&w_health, refresh);

        if (beta_style)
            STlib_updateBinIcon(&w_chatbg, refresh);
        else
        {
            STlib_updateBinIcon(&w_armsbg, refresh);

            for (i = 0; i < 6; i++)
                STlib_updateMultIcon(&w_arms[i], refresh);
        }

        if (beta_style && !show_chat_bar)
        {
                STlib_updateBinIcon(&w_itembg, refresh);
                STlib_updateNum(&w_item, refresh);
        }

        if ((beta_style && !show_chat_bar) || !beta_style)
        {
            STlib_updateMultIcon(&w_faces, refresh);

            STlib_updatePercent(&w_armor, refresh);

            for (i = 0; i < 3; i++)
                STlib_updateMultIcon(&w_keyboxes[i], refresh);

            for (i = 0; i < 4; i++)
            {
                STlib_updateNum(&w_ammo[i], refresh);
                STlib_updateNum(&w_maxammo[i], refresh);
            }

            STlib_updateNum(&w_frags, refresh);
        }
    }

    if (beta_style && automapactive)
    {
        STlib_updateBinIcon(&w_chatbg, refresh);

        STlib_updateNum(&w_score, refresh);
    }
}

void ST_doRefresh(void)
{
    if (!d_statusmap && (automapactive || am_overlay))
        return;

    st_firsttime = false;

    // draw status bar background to off-screen buff
    ST_refreshBackground();

    // and refresh all widgets
    ST_drawWidgets(true);
}

void ST_diffDraw(void)
{
    // update all widgets
    ST_drawWidgets(false);
}

//
// ST_DrawSoundInfo
//
// Displays sound debugging information.
//
void ST_GetChannelInfo(sfxinfo_t *s)
{
    int i;

    s->numchannels = 8;
    s->volume = sfxVolume;

    for (i = 0; i < 8; i++)
    {
        ChanInfo_t *c;

        c = &s->chan[i];
        c->id = channels[i].sound_id;
        c->priority = channels[i].priority;
        c->name = S_sfx[c->id].name;
        c->mo = channels[i].origin;

        if (c->mo != NULL)
        {
            c->distance = P_AproxDistance(c->mo->x - viewx, c->mo->y - viewy)
                >> FRACBITS;
        }
        else
        {
            c->distance = 0;
        }
    }
}

void ST_DrawSoundInfo(void)
{
    int i;
    sfxinfo_t s;
    char text[32];
    int x;
    int xPos[8] = { 1, 55, 92, 136, 180, 225, 255, 285 };

    if (leveltime & 16)
    {
        M_WriteText(xPos[0], 20, "*** SOUND DEBUG INFO ***");
    }

    ST_GetChannelInfo(&s);

    if (s.numchannels == 0)
    {
        return;
    }

    x = 0;

    M_WriteText(xPos[x++], 30, "NAME");
    M_WriteText(xPos[x++], 30, "MO.T");
    M_WriteText(xPos[x++], 30, "MO.X");
    M_WriteText(xPos[x++], 30, "MO.Y");
    M_WriteText(xPos[x++], 30, "MO.Z");
    M_WriteText(xPos[x++], 30, "ID");
    M_WriteText(xPos[x++], 30, "PRI");
    M_WriteText(xPos[x++], 30, "DIST");

    for (i = 0; i < s.numchannels; i++)
    {
        ChanInfo_t *c;
        int y;

        c = &s.chan[i];
        x = 0;
        y = 40 + i * 10;

        if (c->mo == NULL)
        {
            // Channel is unused
            M_WriteText(xPos[0], y, "------");
            continue;
        }

        M_snprintf(text, sizeof(text), "%s", c->name);
        M_ForceUppercase(text);
        M_WriteText(xPos[x++], y, text);

        M_snprintf(text, sizeof(text), "%d", c->mo->type);
        M_WriteText(xPos[x++], y, text);

        M_snprintf(text, sizeof(text), "%d", c->mo->x >> FRACBITS);
        M_WriteText(xPos[x++], y, text);

        M_snprintf(text, sizeof(text), "%d", c->mo->y >> FRACBITS);
        M_WriteText(xPos[x++], y, text);

        M_snprintf(text, sizeof(text), "%d", c->mo->z >> FRACBITS);
        M_WriteText(xPos[x++], y, text);

        M_snprintf(text, sizeof(text), "%d", (int) c->id);
        M_WriteText(xPos[x++], y, text);

        M_snprintf(text, sizeof(text), "%d", c->priority);
        M_WriteText(xPos[x++], y, text);

        M_snprintf(text, sizeof(text), "%d", c->distance);
        M_WriteText(xPos[x++], y, text);
    }

    //UpdateState |= I_FULLSCRN;
    BorderNeedRefresh = true;
}

void ST_Drawer(dboolean fullscreen, dboolean refresh)
{
    st_statusbaron = (!fullscreen) || automapactive;
    st_firsttime = st_firsttime || refresh;

    // Do red-/gold-shifts from damage/items
    ST_doPaletteStuff();

    // If just after ST_Start(), refresh all
    if (st_firsttime || beta_style || (scaledviewheight == SCREENHEIGHT && viewactive))
    {
        if (screenSize < 8 || (automapactive && !am_overlay && d_statusmap))
        {
            if ((usergame && !menuactive) /*|| demoplayback*/)
                ST_doRefresh();
        }
    }
    // Otherwise, update as little as possible
    else if (!beta_style)
        ST_diffDraw();
}

typedef void (*load_callback_t)(char *lumpname, patch_t **variable); 

// Iterates through all graphics to be loaded or unloaded, along with
// the variable they use, invoking the specified callback function.

static void ST_loadUnloadGraphics(load_callback_t callback)
{

    int         i;
    int         j;
    int         facenum;
    
    char        namebuf[9];

    // Load the numbers, tall and short
    for (i = 0; i < 10; i++)
    {
        M_snprintf(namebuf, 9, "STTNUM%d", i);
        callback(namebuf, &tallnum[i]);

        M_snprintf(namebuf, 9, "STYSNUM%d", i);
        callback(namebuf, &shortnum[i]);
    }

    // Load percent key.
    // Note: why not load STMINUS here, too?
    callback("STTPRCNT", &tallpercent);

    // key cards
    for (i = 0; i < NUMCARDS; i++)
    {
        M_snprintf(namebuf, 9, "STKEYS%d", i);
        callback(namebuf, &keys[i]);
    }

    // item background
    if (beta_style)
    {
        callback("STITEM", &itembg);
        callback("STCHAT", &chatbg);
    }
    else
    {
        // arms background
        callback("STARMS", &armsbg);

        // arms ownership widgets
        for (i = 0; i < 6; i++)
        {
            M_snprintf(namebuf, 9, "STGNUM%d", i + 2);

            // gray #
            callback(namebuf, &arms[i][0]);

            // yellow #
            arms[i][1] = shortnum[i+2]; 
        }
    }

    if (beta_style && !netgame)
        M_snprintf(namebuf, 9, "STFB4", consoleplayer);
    else
        M_snprintf(namebuf, 9, "STFB%d", consoleplayer);

    // face backgrounds for different color players
    callback(namebuf, &faceback);

    // status bar background bits
    // HACK: IF SHAREWARE 1.0 OR 1.1
    if (fsize == 4207819 || fsize == 4274218 || fsize == 10396254)
    {
        callback("STMBARL", &sbar_left_oldwad);
        callback("STMBARR", &sbar_right_oldwad);
    }
    else
    {
        callback("STBAR", &sbar);
    }

    if (beta_style)
    {
        callback("ST_AMAP", &sbarmap);
        callback("STWEAP0", &sbara_shotgun);
        callback("STWEAP1", &sbara_chaingun);
        callback("STWEAP2", &sbara_missile);
        callback("STWEAP3", &sbara_plasma);
        callback("STWEAP5", &sbara_bfg);
        callback("STWEAP4", &sbara_chainsaw);
    }

    // face states
    facenum = 0;

    for (i = 0; i < ST_NUMPAINFACES; i++)
    {
        for (j = 0; j < ST_NUMSTRAIGHTFACES; j++)
        {
            M_snprintf(namebuf, 9, "STFST%d%d", i, j);
            callback(namebuf, &faces[facenum]);
            ++facenum;
        }

        // turn right
        M_snprintf(namebuf, 9, "STFTR%d0", i);
        callback(namebuf, &faces[facenum]);
        ++facenum;

        // turn left
        M_snprintf(namebuf, 9, "STFTL%d0", i);
        callback(namebuf, &faces[facenum]);
        ++facenum;

        // ouch!
        M_snprintf(namebuf, 9, "STFOUCH%d", i);
        callback(namebuf, &faces[facenum]);
        ++facenum;

        // evil grin ;)
        M_snprintf(namebuf, 9, "STFEVL%d", i);
        callback(namebuf, &faces[facenum]);
        ++facenum;

        // pissed off
        M_snprintf(namebuf, 9, "STFKILL%d", i);
        callback(namebuf, &faces[facenum]);
        ++facenum;
    }

    callback("STFGOD0", &faces[facenum]);
    ++facenum;
    callback("STFDEAD0", &faces[facenum]);
    ++facenum;
}

static void ST_loadCallback(char *lumpname, patch_t **variable)
{
    *variable = W_CacheLumpName(lumpname, PU_STATIC);
}

void ST_loadGraphics(void)
{
    ST_loadUnloadGraphics(ST_loadCallback);
}

void ST_loadData(void)
{
    lu_palette = W_GetNumForName ("PLAYPAL");
    ST_loadGraphics();
}

//
// [nitr8] UNUSED
//
/*
static void ST_unloadCallback(char *lumpname, patch_t **variable)
{
    W_ReleaseLumpName(lumpname);
    *variable = NULL;
}

void ST_unloadGraphics(void)
{
    ST_loadUnloadGraphics(ST_unloadCallback);
}

void ST_unloadData(void)
{
    ST_unloadGraphics();
}
*/

void ST_initData(void)
{
    int                i;

    st_firsttime = true;
    plyr = &players[consoleplayer];

    st_clock = 0;
    st_chatstate = StartChatState;
    st_gamestate = FirstPersonState;

    st_statusbaron = true;
    st_oldchat = st_chat = false;
    st_cursoron = false;

    st_faceindex = 0;
    st_palette = -1;

    st_oldhealth = -1;

    for (i = 0; i < NUMWEAPONS; i++)
        oldweaponsowned[i] = plyr->weaponowned[i];

    for (i = 0; i < 3; i++)
        keyboxes[i] = -1;

    STlib_init();
}

void ST_createWidgets(void)
{
    int i;

    // ready weapon ammo
    STlib_initNum(&w_ready,
                  ST_AMMOX,
                  ST_AMMOY,
                  tallnum,
                  &plyr->ammo[weaponinfo[plyr->readyweapon].ammo],
                  &st_statusbaron,
                  ST_AMMOWIDTH);

    // the last weapon type
    w_ready.data = plyr->readyweapon; 

    // health percentage
    STlib_initPercent(&w_health,
                      ST_HEALTHX,
                      ST_HEALTHY,
                      tallnum,
                      &plyr->health,
                      &st_statusbaron,
                      tallpercent);

    // arms background
    STlib_initBinIcon(&w_armsbg,
                      ST_ARMSBGX,
                      ST_ARMSBGY,
                      armsbg,
                      &st_notdeathmatch,
                      &st_statusbaron);

    // weapons owned
    for (i = 0; i < 6; i++)
    {
        STlib_initMultIcon(&w_arms[i],
                           ST_ARMSX + (i % 3) * ST_ARMSXSPACE,
                           ST_ARMSY + (i / 3) * ST_ARMSYSPACE,
                           arms[i],
                           &plyr->weaponowned[i + 1],
                           &st_armson);
    }

    // frags sum
    STlib_initNum(&w_frags,
                  ST_FRAGSX,
                  ST_FRAGSY,
                  tallnum,
                  &st_fragscount,
                  &st_fragson,
                  ST_FRAGSWIDTH);

    // faces
    if (beta_style)
        STlib_initMultIcon(&w_faces,
                           ST_FACESX - 1,
                           ST_FACESY,
                           faces,
                           &st_faceindex,
                           &st_statusbaron);
    else
        STlib_initMultIcon(&w_faces,
                           ST_FACESX,
                           ST_FACESY,
                           faces,
                           &st_faceindex,
                           &st_statusbaron);

    // armor percentage - should be colored later
    STlib_initPercent(&w_armor,
                      ST_ARMORX,
                      ST_ARMORY,
                      tallnum,
                      &plyr->armorpoints,
                      &st_statusbaron, tallpercent);

    // keyboxes 0-2
    STlib_initMultIcon(&w_keyboxes[0],
                       ST_KEY0X,
                       ST_KEY0Y,
                       keys,
                       &keyboxes[0],
                       &st_statusbaron);
    
    STlib_initMultIcon(&w_keyboxes[1],
                       ST_KEY1X,
                       ST_KEY1Y,
                       keys,
                       &keyboxes[1],
                       &st_statusbaron);

    STlib_initMultIcon(&w_keyboxes[2],
                       ST_KEY2X,
                       ST_KEY2Y,
                       keys,
                       &keyboxes[2],
                       &st_statusbaron);

    // ammo count (all four kinds)
    STlib_initNum(&w_ammo[0],
                  ST_AMMO0X,
                  ST_AMMO0Y,
                  shortnum,
                  &plyr->ammo[0],
                  &st_statusbaron,
                  ST_AMMO0WIDTH);

    STlib_initNum(&w_ammo[1],
                  ST_AMMO1X,
                  ST_AMMO1Y,
                  shortnum,
                  &plyr->ammo[1],
                  &st_statusbaron,
                  ST_AMMO1WIDTH);

    STlib_initNum(&w_ammo[2],
                  ST_AMMO2X,
                  ST_AMMO2Y,
                  shortnum,
                  &plyr->ammo[2],
                  &st_statusbaron,
                  ST_AMMO2WIDTH);
    
    STlib_initNum(&w_ammo[3],
                  ST_AMMO3X,
                  ST_AMMO3Y,
                  shortnum,
                  &plyr->ammo[3],
                  &st_statusbaron,
                  ST_AMMO3WIDTH);

    // max ammo count (all four kinds)
    STlib_initNum(&w_maxammo[0],
                  ST_MAXAMMO0X,
                  ST_MAXAMMO0Y,
                  shortnum,
                  &plyr->maxammo[0],
                  &st_statusbaron,
                  ST_MAXAMMO0WIDTH);

    STlib_initNum(&w_maxammo[1],
                  ST_MAXAMMO1X,
                  ST_MAXAMMO1Y,
                  shortnum,
                  &plyr->maxammo[1],
                  &st_statusbaron,
                  ST_MAXAMMO1WIDTH);

    STlib_initNum(&w_maxammo[2],
                  ST_MAXAMMO2X,
                  ST_MAXAMMO2Y,
                  shortnum,
                  &plyr->maxammo[2],
                  &st_statusbaron,
                  ST_MAXAMMO2WIDTH);
    
    STlib_initNum(&w_maxammo[3],
                  ST_MAXAMMO3X,
                  ST_MAXAMMO3Y,
                  shortnum,
                  &plyr->maxammo[3],
                  &st_statusbaron,
                  ST_MAXAMMO3WIDTH);

    // item, score & chat background
    if (beta_style)
    {
        STlib_initBinIcon(&w_itembg,
                          ST_ITEMBGX,
                          ST_ITEMBGY,
                          itembg,
                          &st_itemon,
                          &st_statusbaron);

        STlib_initNum(&w_item,
                      ST_ITEMX,
                      ST_ITEMY,
                      tallnum,
                      &st_itemcount,
                      &st_itemon,
                      ST_ITEMWIDTH);

        STlib_initBinIcon(&w_chatbg,
                          ST_CHATBGX,
                          ST_CHATBGY,
                          chatbg,
                          &st_chaton,
                          &st_statusbaron);

        STlib_initNum(&w_score,
                      ST_SCOREX,
                      ST_SCOREY,
                      tallnum,
                      &st_scorecount,
                      &st_scoreon,
                      ST_SCOREWIDTH);
    }
}

void ST_Start (void)
{
    if (!st_stopped)
        ST_Stop();

    ST_initData();
    ST_createWidgets();
    st_stopped = false;

    // [crispy] correctly color the status bar face background in multiplayer
    // demos recorded by another player than player 1
    if (netgame && consoleplayer)
    {
        char namebuf[8];

        M_snprintf(namebuf, 7, "STFB%d", consoleplayer);
        faceback = W_CacheLumpName(namebuf, PU_STATIC);
    }
}

void ST_Init(void)
{
    ST_loadData();

    screens[4] = Z_Malloc((ST_WIDTH << hires) * (ST_HEIGHT << hires) * sizeof(*screens[4]), PU_STATIC, NULL);
}

