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

static const char
rcsid[] = "$Id: st_stuff.c,v 1.6 1997/02/03 22:45:13 b1 Exp $";


#include <stdio.h>

#include "am_map.h"
#include "c_io.h"
#include "deh_str.h"
#include "doomdef.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "g_game.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_tinttab.h"
#include "i_video.h"
#include "m_menu.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_inter.h"
#include "p_local.h"
#include "r_local.h"
#include "s_sound.h"
#include "sounds.h"
#include "st_lib.h"
#include "st_stuff.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"
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
          (ST_NUMSTRAIGHTFACES+ST_NUMTURNFACES+ST_NUMSPECIALFACES)
#define ST_NUMFACES \
          (ST_FACESTRIDE*ST_NUMPAINFACES+ST_NUMEXTRAFACES)

#define ST_NUMEXTRAFACES            2
#define ST_TURNOFFSET               (ST_NUMSTRAIGHTFACES)
#define ST_OUCHOFFSET               (ST_TURNOFFSET + ST_NUMTURNFACES)
#define ST_EVILGRINOFFSET           (ST_OUCHOFFSET + 1)
#define ST_RAMPAGEOFFSET            (ST_EVILGRINOFFSET + 1)
#define ST_GODFACE                  (ST_NUMPAINFACES*ST_FACESTRIDE)
#define ST_DEADFACE                 (ST_GODFACE+1)
#define ST_FACESX                   143
#define ST_FACESY                   168
#define ST_EVILGRINCOUNT            (2*TICRATE)
#define ST_STRAIGHTFACECOUNT        (TICRATE/2)
#define ST_TURNCOUNT                (1*TICRATE)
#define ST_OUCHCOUNT                (1*TICRATE)
#define ST_RAMPAGEDELAY             (2*TICRATE)
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
#define ST_MAXAMMO3WIDTH                ST_MAXAMMO0WIDTH
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
// #define ST_MSGTEXTX           (viewwindowx)
// #define ST_MSGTEXTY           (viewwindowy+viewheight-18)

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

#define ST_MAPWIDTH                 (strlen(mapnames[(gameepisode-1)*9+(gamemap-1)]))

#define ST_MAPTITLEX                (ORIGWIDTH - ST_MAPWIDTH * ST_CHATFONTWIDTH) // HIRES

#define ST_MAPTITLEY                0
#define ST_MAPHEIGHT                1


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

// main player in game
static player_t*              plyr; 

// ST_Start() has just been called
static boolean                st_firsttime;

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
static boolean                st_statusbaron;

// whether status bar chat is active
static boolean                st_chat;

// value of st_chat before message popped up
static boolean                st_oldchat;

// whether chat window has the cursor on
static boolean                st_cursoron;

// !deathmatch
static boolean                st_notdeathmatch; 

// !deathmatch && st_statusbaron
static boolean                st_armson;

// !deathmatch
static boolean                st_fragson; 

// !deathmatch
static boolean                st_itemon; 

// !deathmatch
static boolean                st_chaton; 

// !deathmatch
static boolean                st_scoreon; 

// used for evil grin
static boolean                oldweaponsowned[NUMWEAPONS]; 

static boolean                st_stopped = true;

// main bar left
static patch_t*               sbar;

static patch_t*               healthpatch;
static patch_t*               berserkpatch;
static patch_t*               greenarmorpatch;
static patch_t*               bluearmorpatch;
static patch_t*               sbarmap;
static patch_t*               sbar_left_oldwad;
static patch_t*               sbar_right_oldwad;
static patch_t*               sbara_shotgun;
static patch_t*               sbara_chaingun;
static patch_t*               sbara_missile;
static patch_t*               sbara_plasma;
static patch_t*               sbara_bfg;
static patch_t*               sbara_chainsaw;

// 0-9, tall numbers
static patch_t*               tallnum[10];

// tall % sign
static patch_t*               tallpercent;

// 0-9, short, yellow (,different!) numbers
static patch_t*               shortnum[10];

// 3 key-cards, 3 skulls
static patch_t*               keys[NUMCARDS]; 

// face status patches
static patch_t*               faces[ST_NUMFACES];

// face background
static patch_t*               faceback;

// main item middle
static patch_t*               itembg;

// main chat middle
static patch_t*               chatbg;

// main bar right
static patch_t*               armsbg;

// weapon ownership patches
static patch_t*               arms[6][2]; 

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


patch_t *ST_LoadStatusKeyPatch(int keypicnum)
{
    if (load_dehacked && W_CheckNumForName(keypic[keypicnum].patchnamea) >= 0)
        return W_CacheLumpNum(W_GetNumForName(keypic[keypicnum].patchnamea), PU_CACHE);
    else if (W_CheckNumForName(keypic[keypicnum].patchnameb) >= 0)
        return W_CacheLumpNum(W_GetNumForName(keypic[keypicnum].patchnameb), PU_CACHE);
    else
        return NULL;
}


// graphics are drawn to a backing screen and blitted to the real screen
byte                *st_backing_screen;
	    
boolean             emptytallpercent;

void (*hudfunc)(int, int, patch_t *, byte *);
void (*hudnumfunc)(int, int, patch_t *, byte *);
void (*godhudfunc)(int, int, patch_t *, byte *);

extern channel_t    channels[8];

extern char*        mapnames[];

extern boolean      BorderNeedRefresh;
extern boolean      hud;
extern boolean      in_slime;
extern boolean      show_chat_bar;
extern boolean      done;

extern int          screenSize;
extern int          load_dehacked;
extern int          cardsfound;
extern int          snd_chans;

int                 prio = 0;
int                 healthhighlight = 0;
int                 ammohighlight = 0;
int                 armorhighlight = 0;
            
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
        V_UseBuffer(st_backing_screen);

        if(beta_style)
        {
            if(automapactive)
            {
                V_DrawPatch(ST_X, 0, 4, sbarmap);

                if(plyr->weaponowned[wp_shotgun])
                    V_DrawPatch(110, 4, 4, sbara_shotgun);
                if(plyr->weaponowned[wp_chaingun])
                    V_DrawPatch(110, 10, 4, sbara_chaingun);
                if(plyr->weaponowned[wp_missile])
                    V_DrawPatch(135, 3, 4, sbara_missile);
                if(plyr->weaponowned[wp_plasma])
                    V_DrawPatch(135, 10, 4, sbara_plasma);
                if(plyr->weaponowned[wp_bfg])
                    V_DrawPatch(185, 3, 4, sbara_bfg);
                if(plyr->weaponowned[wp_chainsaw])
                    V_DrawPatch(160, 5, 4, sbara_chainsaw);
            }
            else
            {
                if(fsize == 4207819 || fsize == 4274218 || fsize == 10396254)
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
            if(fsize == 4207819 || fsize == 4274218 || fsize == 10396254)
            {
                V_DrawPatch(ST_X, 0, 4, sbar_left_oldwad);
                V_DrawPatch(104, 0, 4, sbar_right_oldwad);
            }
            else
                V_DrawPatch(ST_X, 0, 4, sbar);
        }

        if (netgame)
            V_DrawPatch(ST_FX, 0, 4, faceback);
        else if(beta_style && !automapactive)
            V_DrawPatch(ST_FX - 1, 1, 4, faceback);

        V_RestoreBuffer();

        V_CopyRect(ST_X, 0, st_backing_screen, ST_WIDTH, ST_HEIGHT, ST_X, ST_Y);
    }
}

static void DrawStatusNumber(int *x, int y, int val, byte *tinttab,
                          void (*hudnumfunc)(int, int, patch_t *, byte *))
{
    int         oldval = val;
    patch_t     *patch;

    if (val > 99)
    {
        patch = tallnum[val / 100];
        hudnumfunc(*x, y, patch, tinttab);
        *x += SHORT(patch->width);
    }
    val %= 100;
    if (val > 9 || oldval > 99)
    {
        patch = tallnum[val / 10];
        hudnumfunc(*x, y, patch, tinttab);
        *x += SHORT(patch->width);
    }
    val %= 10;
    patch = tallnum[val];
    hudnumfunc(*x, y, patch, tinttab);
    *x += SHORT(patch->width);
}

static int StatusNumberWidth(int val)
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

void ST_DrawStatus(void)
{
    int             health = MAX(0, plyr->mo->health);
    int             ammotype = weaponinfo[plyr->readyweapon].ammo;
    int             ammo = plyr->ammo[ammotype];
    int             armor = plyr->armorpoints;
    int             health_x = ST_HEALTH_X;
    int             key = 0;
    int             i = 0;
    static int      healthwait = 0;
    byte            *tinttab;
    int             invulnerability = plyr->powers[pw_invulnerability];
    static boolean  healthanim = false;
    patch_t         *patch;
    int             currenttics = I_GetTime();

    if (d_translucency)
    {
        hudfunc = V_DrawTranslucentStatusPatch;
        hudnumfunc = V_DrawTranslucentStatusNumberPatch;
        godhudfunc = V_DrawTranslucentYellowStatusPatch;
    }
    else
    {
        hudfunc = V_DrawStatusPatch;
        hudnumfunc = V_DrawStatusPatch;
        godhudfunc = V_DrawYellowStatusPatch;
    }

    boolean gamepaused = (menuactive || paused || consoleactive);

    tinttab = (!health || (health <= ST_HEALTH_MIN && healthanim) || health > ST_HEALTH_MIN
        || gamepaused ? tinttab66 : tinttab25);

    patch = (((plyr->readyweapon == wp_fist && plyr->pendingweapon == wp_nochange)
        || plyr->pendingweapon == wp_fist) && plyr->powers[pw_strength] ? berserkpatch : healthpatch);

    if (patch)
    {
        if ((plyr->cheats & CF_GODMODE) || invulnerability > 128 || (invulnerability & 8))
            godhudfunc(health_x, ST_HEALTH_Y - (SHORT(patch->height) - 17), patch, tinttab);
        else
            hudfunc(health_x, ST_HEALTH_Y - (SHORT(patch->height) - 17), patch, tinttab);
        health_x += SHORT(patch->width) + 8;
    }

    if (healthhighlight)
    {
        if (healthhighlight < currenttics)
            healthhighlight = 0;

        DrawStatusNumber(&health_x, ST_HEALTH_Y, health, tinttab, V_DrawStatusPatch);
        if (!emptytallpercent)
            V_DrawStatusPatch(health_x, ST_HEALTH_Y, tallpercent, tinttab);
    }
    else
    {
        DrawStatusNumber(&health_x, ST_HEALTH_Y, health, tinttab, hudnumfunc);
        if (!emptytallpercent)
            hudnumfunc(health_x, ST_HEALTH_Y, tallpercent, tinttab);
    }

    if (health <= ST_HEALTH_MIN && !gamepaused)
    {
        if (healthwait < currenttics)
        {
            healthanim = !healthanim;
            healthwait = currenttics + ST_HEALTH_WAIT * health / ST_HEALTH_MIN + 4;
        }
    }
    else
    {
        healthanim = false;
        healthwait = 0;
    }

    if (plyr->pendingweapon != wp_nochange)
    {
        ammotype = weaponinfo[plyr->pendingweapon].ammo;
        ammo = plyr->ammo[ammotype];
    }

    if (health && ammo && ammotype != am_noammo)
    {
        static int          ammowait = 0;
        static boolean      ammoanim = false;

        tinttab = ((ammo <= ST_AMMO_MIN && ammoanim) || ammo > ST_AMMO_MIN || gamepaused ?
            tinttab66 : tinttab25);

        if(plyr->readyweapon == wp_pistol)
            patch = W_CacheLumpName(DEH_String("CLIPA0"), PU_CACHE);
        else if(plyr->readyweapon == wp_shotgun)
            patch = W_CacheLumpName(DEH_String("SHELA0"), PU_CACHE);
        else if(plyr->readyweapon == wp_chaingun)
            patch = W_CacheLumpName(DEH_String("AMMOA0"), PU_CACHE);
        else if(plyr->readyweapon == wp_missile)
            patch = W_CacheLumpName(DEH_String("ROCKA0"), PU_CACHE);
        else if(plyr->readyweapon == wp_plasma)
            patch = W_CacheLumpName(DEH_String("CELLA0"), PU_CACHE);
        else if(plyr->readyweapon == wp_bfg)
            patch = W_CacheLumpName(DEH_String("CELPA0"), PU_CACHE);
        else if(plyr->readyweapon == wp_supershotgun)
            patch = W_CacheLumpName(DEH_String("SBOXA0"), PU_CACHE);
        else
            patch = W_CacheLumpName(DEH_String("EMPTY"), PU_CACHE);

        int offset_special = 0;

        int offset_width = ST_AMMO_X + 15;
        int offset_height = ST_AMMO_Y + 8;

        int half_patch_width = (SHORT(patch->width) / 2);
        int half_patch_height = (SHORT(patch->height) / 2);

        int ammo_x = offset_width - half_patch_width;
        int ammo_y = offset_height - half_patch_height;

        if (ammo < 200 && ammo > 99)
            offset_special = 3;

        if (patch)
        {
            hudfunc(ammo_x, ammo_y, patch, tinttab);
            ammo_x += offset_width + half_patch_width - (ORIGHEIGHT / 2) + offset_special;
        }

        if (ammohighlight)
        {
            if (ammohighlight < currenttics)
                ammohighlight = 0;

            DrawStatusNumber(&ammo_x, ST_AMMO_Y, ammo, tinttab, V_DrawStatusPatch);
        }
        else
        {
            DrawStatusNumber(&ammo_x, ST_AMMO_Y, ammo, tinttab, hudnumfunc);
        }

        if (ammo <= ST_AMMO_MIN && !gamepaused)
        {
            if (ammowait < currenttics)
            {
                ammoanim = !ammoanim;
                ammowait = currenttics + ST_AMMO_WAIT * ammo / ST_AMMO_MIN + 4;
            }
        }
        else
        {
            ammoanim = false;
            ammowait = 0;
        }
    }

    while (i < NUMCARDS)
    {
        if (plyr->cards[i++] > 0)
            key++;
    }

    if (key || plyr->neededcardflash)
    {
        int                 keypic_x = ST_KEYS_X - 20 * (key - 1);
        static int          keywait = 0;
        static boolean      showkey = false;

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

        if (plyr->neededcardflash)
        {
            patch_t     *patch = keypic[plyr->neededcard].patch;

            if (patch)
            {
                if (!gamepaused)
                {
                    if (keywait < currenttics)
                    {
                        showkey = !showkey;
                        keywait = currenttics + ST_KEY_WAIT;
                        plyr->neededcardflash--;
                    }
                }
                if (showkey)
                    hudfunc(keypic_x - (SHORT(patch->width) + 6), ST_KEYS_Y, patch, tinttab66);
            }
        }
        else
        {
            showkey = false;
            keywait = 0;
        }

        for (i = 0; i < NUMCARDS; i++)
        {
            if (plyr->cards[i] > 0)
            {
                patch_t     *patch = keypic[i].patch;

                if (patch)
                    hudfunc(keypic_x + (SHORT(patch->width) + 6) * (cardsfound - plyr->cards[i]),
                        ST_KEYS_Y, patch, tinttab66);
            }
        }
    }

    if (armor)
    {
        patch_t     *patch = (plyr->armortype == 1 ? greenarmorpatch : bluearmorpatch);
        int         armor_x = ST_ARMOR_X;

        if (patch)
        {
            armor_x -= SHORT(patch->width);
            hudfunc(armor_x, ST_ARMOR_Y - (SHORT(patch->height) - 16), patch, tinttab66);
            armor_x -= 7;
        }

        if (armorhighlight)
        {
            if (armorhighlight < currenttics)
                armorhighlight = 0;

            if (emptytallpercent)
            {
                armor_x -= StatusNumberWidth(armor);
                DrawStatusNumber(&armor_x, ST_ARMOR_Y, armor, tinttab66, V_DrawStatusPatch);
            }
            else
            {
                armor_x -= SHORT(tallpercent->width);
                V_DrawStatusPatch(armor_x, ST_ARMOR_Y, tallpercent, tinttab66);
                armor_x -= StatusNumberWidth(armor);
                DrawStatusNumber(&armor_x, ST_ARMOR_Y, armor, tinttab66, V_DrawStatusPatch);
            }
        }
        else
        {
            if (emptytallpercent)
            {
                armor_x -= StatusNumberWidth(armor);
                DrawStatusNumber(&armor_x, ST_ARMOR_Y, armor, tinttab66, hudnumfunc);
            }
            else
            {
                armor_x -= SHORT(tallpercent->width);
                hudnumfunc(armor_x, ST_ARMOR_Y, tallpercent, tinttab66);
                armor_x -= StatusNumberWidth(armor);
                DrawStatusNumber(&armor_x, ST_ARMOR_Y, armor, tinttab66, hudnumfunc);
            }
        }
    }
}

// Respond to keyboard input events,
//  intercept cheats.
boolean
ST_Responder (event_t* ev)
{
    // Filter automap on/off.
    if (ev->type == ev_keyup
            && ((ev->data1 & 0xffff0000) == AM_MSGHEADER))
    {
        switch(ev->data1)
        {
            case AM_MSGENTERED:
            st_gamestate = AutomapState;
            st_firsttime = true;
            break;
        
            case AM_MSGEXITED:
            //        fprintf(stderr, "AM exited\n");
            st_gamestate = FirstPersonState;
            break;
        }
    }
    return false;
}



int ST_calcPainOffset(void)
{
    int                health;
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

    angle_t            badguyangle;
    angle_t            diffang;

    boolean            doevilgrin;

    static int         lastattackdown = -1;
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
            // picking up bonus
            doevilgrin = false;

            for (i=0;i<NUMWEAPONS;i++)
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
        if (plyr->damagecount
            && plyr->attacker
            && plyr->attacker != plyr->mo)
        {
            // being attacked
            prio = priority = 7;
            
            // haleyjd 10/12/03: classic DOOM problem of missing OUCH face
            // was due to inversion of this test:
            // if(plyr->health - st_oldhealth > ST_MUCHPAIN)
            // e6y: compatibility optioned
            if((d_ouchface?
                (plyr->health - st_oldhealth):
                (st_oldhealth - plyr->health)) > ST_MUCHPAIN)
            {
                // e6y
                // There are TWO bugs in the ouch face code.
                // Not only was the condition reversed, but the priority system is
                // broken in a way that makes the face not work with monster damage.
                if(!d_ouchface)
                    priority = 8;

                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;
            }
            else
            {
                badguyangle = R_PointToAngle2(plyr->mo->x,
                                              plyr->mo->y,
                                              plyr->attacker->x,
                                              plyr->attacker->y);
                
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
                } // confusing, aint it?

                
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
            // if(plyr->health - st_oldhealth > ST_MUCHPAIN)
            // e6y: compatibility optioned
            if(((d_ouchface?
                (plyr->health - st_oldhealth):
                (st_oldhealth - plyr->health)) > ST_MUCHPAIN) ||
                (beta_style && in_slime && !(plyr->cheats & CF_GODMODE)))
            {
                priority = 7;
                st_facecount = ST_TURNCOUNT;
                st_faceindex = ST_calcPainOffset() + ST_OUCHOFFSET;

                if(beta_style && in_slime)
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
        // rapid firing
        if (plyr->attackdown)
        {
            if (lastattackdown==-1)
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
        if ((plyr->cheats & CF_GODMODE)
            || plyr->powers[pw_invulnerability])
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

    static int         largeammo = 1994; // means "n/a"
    int                i;

    // must redirect the pointer if the ready weapon has changed.
    //  if (w_ready.data != plyr->readyweapon)
    //  {
    if (weaponinfo[plyr->readyweapon].ammo == am_noammo)
        w_ready.num = &largeammo;
    else
        w_ready.num = &plyr->ammo[weaponinfo[plyr->readyweapon].ammo];
    //{
    // static int tic=0;
    // static int dir=-1;
    // if (!(tic&15))
    //   plyr->ammo[weaponinfo[plyr->readyweapon].ammo]+=dir;
    // if (plyr->ammo[weaponinfo[plyr->readyweapon].ammo] == -100)
    //   dir = 1;
    // tic++;
    // }
    w_ready.data = plyr->readyweapon;

    // if (*w_ready.on)
    //  STlib_updateNum(&w_ready, true);
    // refresh weapon change
    //  }

    // update keycard multiple widgets
    for (i=0;i<3;i++)
    {
        keyboxes[i] = (plyr->cards[i] > 0 ? i : -1);

        if (plyr->cards[i + 3] > 0)
            keyboxes[i] = i+3;
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

    for (i=0 ; i<MAXPLAYERS ; i++)
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

void ST_Ticker (void)
{

    st_clock++;
    st_randomnumber = M_Random();
    ST_updateWidgets();
    st_oldhealth = plyr->health;

}

void ST_doPaletteStuff(void)
{

    int                palette;
    byte*              pal;
    int                cnt;
    int                bzc;

    cnt = plyr->damagecount;

    if (plyr->powers[pw_strength] && !beta_style)
    {
        // slowly fade the berzerk out
        bzc = 12 - (plyr->powers[pw_strength]>>6);

        if (bzc > cnt)
            cnt = bzc;
    }
        
    if (cnt)
    {
        palette = (cnt+7)>>3;
        
        if (palette >= NUMREDPALS)
            palette = NUMREDPALS-1;

        palette += STARTREDPALS;
    }

    else if (plyr->bonuscount)
    {
        palette = (plyr->bonuscount+7)>>3;

        if (palette >= NUMBONUSPALS)
            palette = NUMBONUSPALS-1;

        palette += STARTBONUSPALS;
    }

    else if ( (plyr->powers[pw_ironfeet] > 4*32
              || plyr->powers[pw_ironfeet]&8) && !beta_style)
        palette = RADIATIONPAL;

    else if ( (plyr->powers[pw_infrared] > 4*32
              || plyr->powers[pw_infrared]&8) && beta_style)
        palette = RADIATIONPAL;

    else
        palette = 0;

    if (palette != st_palette)
    {
        st_palette = palette;
        pal = (byte *) W_CacheLumpNum (lu_palette, PU_CACHE)+palette*768;
        I_SetPalette (pal);
    }

}

//
// Completely changed for PRE-BETA functionality
// maybe needs more optimization but at least it's working
//
void ST_drawWidgets(boolean refresh)
{
    int                i;

    if((beta_style && !automapactive) || !beta_style)
    {
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

        if(beta_style)
            STlib_updateBinIcon(&w_chatbg, refresh);
        else
        {
            STlib_updateBinIcon(&w_armsbg, refresh);

            for (i=0;i<6;i++)
                STlib_updateMultIcon(&w_arms[i], refresh);
        }

        if(beta_style && !show_chat_bar)
        {
                STlib_updateBinIcon(&w_itembg, refresh);
                STlib_updateNum(&w_item, refresh);
        }

        if((beta_style && !show_chat_bar) || !beta_style)
        {
            STlib_updateMultIcon(&w_faces, refresh);

            STlib_updatePercent(&w_armor, refresh);

            for (i=0;i<3;i++)
                STlib_updateMultIcon(&w_keyboxes[i], refresh);

            for (i=0;i<4;i++)
            {
                STlib_updateNum(&w_ammo[i], refresh);
                STlib_updateNum(&w_maxammo[i], refresh);
            }

            STlib_updateNum(&w_frags, refresh);
        }
    }

    if(beta_style && automapactive)
    {
        STlib_updateBinIcon(&w_chatbg, refresh);

        STlib_updateNum(&w_score, refresh);
    }
}

void ST_doRefresh(void)
{
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
    ChanInfo_t *c;

    s->numchannels = 8;
    s->volume = sfxVolume;
    for (i = 0; i < 8; i++)
    {
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
    ChanInfo_t *c;
    char text[32];
    int x;
    int y;
    int xPos[7] = { 1, 75, 112, 156, 200, 230, 260 };

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
    M_WriteText(xPos[x++], 30, "ID");
    M_WriteText(xPos[x++], 30, "PRI");
    M_WriteText(xPos[x++], 30, "DIST");
    for (i = 0; i < s.numchannels; i++)
    {
        c = &s.chan[i];
        x = 0;
        y = 40 + i * 10;
        if (c->mo == NULL)
        {                       // Channel is unused
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
        M_snprintf(text, sizeof(text), "%d", (int) c->id);
        M_WriteText(xPos[x++], y, text);
        M_snprintf(text, sizeof(text), "%d", c->priority);
        M_WriteText(xPos[x++], y, text);
        M_snprintf(text, sizeof(text), "%d", c->distance);
        M_WriteText(xPos[x++], y, text);
    }
//    UpdateState |= I_FULLSCRN;
    BorderNeedRefresh = true;
}

void ST_Drawer (boolean fullscreen, boolean refresh)
{
    st_statusbaron = (!fullscreen) || automapactive;
    st_firsttime = st_firsttime || refresh;

    // Do red-/gold-shifts from damage/items
    ST_doPaletteStuff();

    // If just after ST_Start(), refresh all
    if (st_firsttime || beta_style || (scaledviewheight == SCREENHEIGHT && viewactive))
        ST_doRefresh();
    // Otherwise, update as little as possible
    else if(!beta_style)
        ST_diffDraw();
}

typedef void (*load_callback_t)(char *lumpname, patch_t **variable); 

// Iterates through all graphics to be loaded or unloaded, along with
// the variable they use, invoking the specified callback function.

static void ST_loadUnloadGraphics(load_callback_t callback)
{

    int		i;
    int		j;
    int		facenum;
    
    char	namebuf[9];

    // Load the numbers, tall and short
    for (i=0;i<10;i++)
    {
	DEH_snprintf(namebuf, 9, "STTNUM%d", i);
        callback(namebuf, &tallnum[i]);

	DEH_snprintf(namebuf, 9, "STYSNUM%d", i);
        callback(namebuf, &shortnum[i]);
    }

    // Load percent key.
    //Note: why not load STMINUS here, too?

    callback(DEH_String("STTPRCNT"), &tallpercent);
    emptytallpercent = V_EmptyPatch(tallpercent);

    // key cards
    for (i=0;i<NUMCARDS;i++)
    {
	DEH_snprintf(namebuf, 9, "STKEYS%d", i);
        callback(namebuf, &keys[i]);
    }

    // item background
    if(beta_style)
    {
        callback(DEH_String("STITEM"), &itembg);
        callback(DEH_String("STCHAT"), &chatbg);
    }
    else
    {
        // arms background
        callback(DEH_String("STARMS"), &armsbg);

        // arms ownership widgets
        for (i=0; i<6; i++)
        {
	    DEH_snprintf(namebuf, 9, "STGNUM%d", i+2);

	    // gray #
            callback(namebuf, &arms[i][0]);

            // yellow #
            arms[i][1] = shortnum[i+2]; 
        }
    }

    if(beta_style && !netgame)
        DEH_snprintf(namebuf, 9, "STFB4", consoleplayer);
    else
        DEH_snprintf(namebuf, 9, "STFB%d", consoleplayer);

    // face backgrounds for different color players
    callback(namebuf, &faceback);

    // status bar background bits
    // HACK: IF SHAREWARE 1.0 OR 1.1
    if(fsize == 4207819 || fsize == 4274218 || fsize == 10396254)
    {
        callback(DEH_String("STMBARL"), &sbar_left_oldwad);
        callback(DEH_String("STMBARR"), &sbar_right_oldwad);
    }
    else
    {
        callback(DEH_String("STBAR"), &sbar);
    }

    if(beta_style)
    {
        callback(DEH_String("ST_AMAP"), &sbarmap);
        callback(DEH_String("STWEAP0"), &sbara_shotgun);
        callback(DEH_String("STWEAP1"), &sbara_chaingun);
        callback(DEH_String("STWEAP2"), &sbara_missile);
        callback(DEH_String("STWEAP3"), &sbara_plasma);
        callback(DEH_String("STWEAP5"), &sbara_bfg);
        callback(DEH_String("STWEAP4"), &sbara_chainsaw);
    }

    // face states
    facenum = 0;
    for (i=0; i<ST_NUMPAINFACES; i++)
    {
	for (j=0; j<ST_NUMSTRAIGHTFACES; j++)
	{
	    DEH_snprintf(namebuf, 9, "STFST%d%d", i, j);
            callback(namebuf, &faces[facenum]);
            ++facenum;
	}
	DEH_snprintf(namebuf, 9, "STFTR%d0", i);	// turn right
        callback(namebuf, &faces[facenum]);
        ++facenum;
	DEH_snprintf(namebuf, 9, "STFTL%d0", i);	// turn left
        callback(namebuf, &faces[facenum]);
        ++facenum;
	DEH_snprintf(namebuf, 9, "STFOUCH%d", i);	// ouch!
        callback(namebuf, &faces[facenum]);
        ++facenum;
	DEH_snprintf(namebuf, 9, "STFEVL%d", i);	// evil grin ;)
        callback(namebuf, &faces[facenum]);
        ++facenum;
	DEH_snprintf(namebuf, 9, "STFKILL%d", i);	// pissed off
        callback(namebuf, &faces[facenum]);
        ++facenum;
    }

    callback(DEH_String("STFGOD0"), &faces[facenum]);
    ++facenum;
    callback(DEH_String("STFDEAD0"), &faces[facenum]);
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

    for (i=0;i<NUMWEAPONS;i++)
        oldweaponsowned[i] = plyr->weaponowned[i];

    for (i=0;i<3;i++)
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
                  ST_AMMOWIDTH );

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
    for(i=0;i<6;i++)
    {
        STlib_initMultIcon(&w_arms[i],
                           ST_ARMSX+(i%3)*ST_ARMSXSPACE,
                           ST_ARMSY+(i/3)*ST_ARMSYSPACE,
                           arms[i],
                           &plyr->weaponowned[i+1],
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
    if(beta_style)
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
    if(beta_style)
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

    keypic[it_bluecard].patch = ST_LoadStatusKeyPatch(it_bluecard);
    keypic[it_yellowcard].patch = ST_LoadStatusKeyPatch(exe_hacx ? it_yellowcard : it_yellowskull);
    keypic[it_redcard].patch = ST_LoadStatusKeyPatch(it_redcard);

    if (gamemode != shareware)
    {
        keypic[it_blueskull].patch = ST_LoadStatusKeyPatch(it_blueskull);
        keypic[it_yellowskull].patch = ST_LoadStatusKeyPatch(it_yellowskull);
        keypic[it_redskull].patch = ST_LoadStatusKeyPatch(it_redskull);
    }
}

void ST_Init (void)
{
    ST_loadData();

    screens[4] = Z_Malloc((ST_WIDTH << hires) * (ST_HEIGHT << hires), PU_STATIC, 0);

    st_backing_screen = screens[4];
}
