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
#include "deh_str.h"
#include "doomdef.h"
#include "doomkeys.h"
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "hu_lib.h"
#include "hu_stuff.h"
#include "i_swap.h"
#include "i_video.h"
#include "m_controls.h"
#include "m_misc.h"
#include "s_sound.h"
#include "sounds.h"
#include "st_stuff.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"


//
// Locally used constants, shortcuts.
//
#define HU_TITLE        (mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2       (mapnames_commercial[gamemap-1])
#define HU_TITLEP       (mapnames_commercial[gamemap-1 + 32])
#define HU_TITLET       (mapnames_commercial[gamemap-1 + 64])
#define HU_TITLEN       (mapnames_commercial[gamemap-1 + 96])
#define HU_TITLE_CHEX   (mapnames[gamemap - 1])
#define HU_TITLEHEIGHT  1
#define HU_TITLEX       0
#define HU_TITLEY       (167 - SHORT(hu_font[0]->height))

#define HU_INPUTTOGGLE  't'
#define HU_INPUTX       HU_MSGX
#define HU_INPUTY       (HU_MSGY + HU_MSGHEIGHT*(SHORT(hu_font[0]->height) +1))
#define HU_INPUTWIDTH   64
#define HU_INPUTHEIGHT  1

#define HU_MONSECX       -16
#define HU_MONSECY      (25 - SHORT(hu_font[0]->height))


static player_t*        plr;

static hu_textline_t    w_title;
static hu_textline_t    w_monsec;              // ADDED FOR PSP-STATS

//static hu_itext_t       w_inputbuffer[MAXPLAYERS];

static hu_stext_t       w_message;

static hu_stext_t       w_message_0;
static hu_stext_t       w_message_1;
static hu_stext_t       w_message_2;

static hu_stext_t       w_secret;

//static boolean          always_off = false;
static boolean          message_on;
static boolean          message_nottobefuckedwith;
static boolean          headsupactive = false;
static boolean          secret_on;

static int              message_counter;
static int              secret_counter;

static char             hud_monsecstr[80];     // ADDED FOR PSP-STATS

boolean                 message_dontfuckwithme;
boolean                 show_chat_bar;

patch_t*                hu_font[HU_FONTSIZE];
patch_t*                beta_hu_font[HU_FONTSIZE];

extern int              screenblocks;
extern int              showMessages;
extern int              crosshair;
extern int              show_stats;
extern int              screenSize;
extern int              timer_info;

//extern boolean          game_startup;

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

char*   mapnames[] =    // DOOM shareware/registered/retail (Ultimate) names.
{

    HUSTR_E1M1,
    HUSTR_E1M2,
    HUSTR_E1M3,
    HUSTR_E1M4,
    HUSTR_E1M5,
    HUSTR_E1M6,
    HUSTR_E1M7,
    HUSTR_E1M8,
    HUSTR_E1M9,

    HUSTR_E2M1,
    HUSTR_E2M2,
    HUSTR_E2M3,
    HUSTR_E2M4,
    HUSTR_E2M5,
    HUSTR_E2M6,
    HUSTR_E2M7,
    HUSTR_E2M8,
    HUSTR_E2M9,

    HUSTR_E3M1,
    HUSTR_E3M2,
    HUSTR_E3M3,
    HUSTR_E3M4,
    HUSTR_E3M5,
    HUSTR_E3M6,
    HUSTR_E3M7,
    HUSTR_E3M8,
    HUSTR_E3M9,

    HUSTR_E4M1,
    HUSTR_E4M2,
    HUSTR_E4M3,
    HUSTR_E4M4,
    HUSTR_E4M5,
    HUSTR_E4M6,
    HUSTR_E4M7,
    HUSTR_E4M8,
    HUSTR_E4M9,

    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL",
    "NEWLEVEL"
};

// List of names for levels in commercial IWADs
// (doom2.wad, plutonia.wad, tnt.wad).  These are stored in a
// single large array; WADs like pl2.wad have a MAP33, and rely on
// the layout in the Vanilla executable, where it is possible to
// overflow the end of one array into the next.

char *mapnames_commercial[] =
{
    // DOOM 2 map names.

    HUSTR_1,
    HUSTR_2,
    HUSTR_3,
    HUSTR_4,
    HUSTR_5,
    HUSTR_6,
    HUSTR_7,
    HUSTR_8,
    HUSTR_9,
    HUSTR_10,
    HUSTR_11,
        
    HUSTR_12,
    HUSTR_13,
    HUSTR_14,
    HUSTR_15,
    HUSTR_16,
    HUSTR_17,
    HUSTR_18,
    HUSTR_19,
    HUSTR_20,
        
    HUSTR_21,
    HUSTR_22,
    HUSTR_23,
    HUSTR_24,
    HUSTR_25,
    HUSTR_26,
    HUSTR_27,
    HUSTR_28,
    HUSTR_29,
    HUSTR_30,
    HUSTR_31,
    HUSTR_32,

    // Plutonia WAD map names.

    PHUSTR_1,
    PHUSTR_2,
    PHUSTR_3,
    PHUSTR_4,
    PHUSTR_5,
    PHUSTR_6,
    PHUSTR_7,
    PHUSTR_8,
    PHUSTR_9,
    PHUSTR_10,
    PHUSTR_11,
        
    PHUSTR_12,
    PHUSTR_13,
    PHUSTR_14,
    PHUSTR_15,
    PHUSTR_16,
    PHUSTR_17,
    PHUSTR_18,
    PHUSTR_19,
    PHUSTR_20,
        
    PHUSTR_21,
    PHUSTR_22,
    PHUSTR_23,
    PHUSTR_24,
    PHUSTR_25,
    PHUSTR_26,
    PHUSTR_27,
    PHUSTR_28,
    PHUSTR_29,
    PHUSTR_30,
    PHUSTR_31,
    PHUSTR_32,
    
    // TNT WAD map names.

    THUSTR_1,
    THUSTR_2,
    THUSTR_3,
    THUSTR_4,
    THUSTR_5,
    THUSTR_6,
    THUSTR_7,
    THUSTR_8,
    THUSTR_9,
    THUSTR_10,
    THUSTR_11,
        
    THUSTR_12,
    THUSTR_13,
    THUSTR_14,
    THUSTR_15,
    THUSTR_16,
    THUSTR_17,
    THUSTR_18,
    THUSTR_19,
    THUSTR_20,
        
    THUSTR_21,
    THUSTR_22,
    THUSTR_23,
    THUSTR_24,
    THUSTR_25,
    THUSTR_26,
    THUSTR_27,
    THUSTR_28,
    THUSTR_29,
    THUSTR_30,
    THUSTR_31,
    THUSTR_32,

    NHUSTR_1,
    NHUSTR_2,
    NHUSTR_3,
    NHUSTR_4,
    NHUSTR_5,
    NHUSTR_6,
    NHUSTR_7,
    NHUSTR_8,
    NHUSTR_9
};

void HU_Init(void)
{

    int       i;
    int       j;
    char      buffer[9];

    // load the heads-up font
    j = HU_FONTSTART;
    for (i=0;i<HU_FONTSIZE;i++)
    {
        DEH_snprintf(buffer, 9, "STCFN%.3d", j++);
        hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);

        // haleyjd 09/18/10: load beta_hu_font as well
        buffer[2] = 'B';
        beta_hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }
}

void HU_Stop(void)
{
    headsupactive = false;
}

void HU_Start(void)
{
//    int       i;
    char*     s;
    char*     t;

    if (headsupactive)
        HU_Stop();

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

    // create the secret message widget
    HUlib_initSText(&w_secret,
                    88, 86, HU_MSGHEIGHT,
                    hu_font,
                    HU_FONTSTART, &secret_on);

    // create the map title widget
    HUlib_initTextLine(&w_title,
                       HU_TITLEX, HU_TITLEY,
                       hu_font,
                       HU_FONTSTART);
    
    HUlib_initTextLine(&w_monsec,
                       HU_MONSECX, HU_MONSECY,
                       hu_font,
                       HU_FONTSTART);

    switch ( logical_gamemission )
    {
      case doom:
        s = HU_TITLE;
        break;
      case doom2:
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
      default:
         s = "Unknown level";
         break;
    }

    // Chex.exe always uses the episode 1 level title
    // eg. E2M1 gives the title for E1M1

    if (gameversion == exe_chex)
    {
        s = HU_TITLE_CHEX;
    }

    // dehacked substitution to get modified level name

    t = hud_monsecstr;

    s = DEH_String(s);
    
    t = DEH_String(t);

    if((fsize != 12538385 &&
        fsize != 14691821 &&
        fsize != 14677988 &&
        fsize != 14683458) ||
        (fsize == 12538385 && gamemap != 10) ||
        ((fsize == 14683458 || fsize == 14677988 || fsize == 14691821) && gamemap != 33))
    {
        while (*s)
            HUlib_addCharToTextLine(&w_title, *(s++));
    }

    while (*t)
        HUlib_addCharToTextLine(&w_monsec, *(t++));
/*
    // create the inputbuffer widgets
    for (i=0 ; i<MAXPLAYERS ; i++)
        HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, &always_off);
*/
    headsupactive = true;

}

// print a bar indicating demo progress at the bottom of the screen
static void HU_DemoProgressBar (void)
{
    int i;
    extern char *demo_p, *demobuffer;
    extern int defdemosize;

    i = SCREENWIDTH * (demo_p - demobuffer) / defdemosize;

    V_DrawHorizLine(0, SCREENHEIGHT - 3, i, 4);     // white
    V_DrawHorizLine(0, SCREENHEIGHT - 2, i, 0);     // black
    V_DrawHorizLine(0, SCREENHEIGHT - 1, i, 4);     // white

    V_DrawHorizLine(0, SCREENHEIGHT - 2, 1, 4);     // white start
    V_DrawHorizLine(i - 1, SCREENHEIGHT - 2, 1, 4); // white end
}

void HU_DrawStats(void)
{
    const char *t;

    M_StringCopy(hud_monsecstr, "", sizeof(hud_monsecstr));
    t = hud_monsecstr;

    // clear the internal widget text buffer
    HUlib_clearTextLine(&w_monsec);

    //jff 3/26/98 use ESC not '\' for paths
    // build the init string with fixed colors
    sprintf
    (
    hud_monsecstr,
    " \x1b\x3 KILLS: %d/%d \x1b\x3 ITEMS: %d/%d \x1b\x3 SECRETS: %d/%d",
    plr->killcount,totalkills,
    plr->itemcount,totalitems,
    plr->secretcount,totalsecret
    );

    // transfer the init string to the widget
    t = hud_monsecstr;

    //jff 2/17/98 initialize kills/items/secret widget
    while (*t)
        HUlib_addCharToTextLine(&w_monsec, *(t++));

    // display the kills/items/secrets each frame, if optioned
    HUlib_drawTextLine(&w_monsec, false);
}


void HU_Drawer(void)
{
    if(!automapactive && !demoplayback && crosshair == 1)
    {
        if(screenSize < 8)
            V_DrawPatch(158, 82, 0, W_CacheLumpName(DEH_String("XHAIR"), PU_CACHE));
        else
            V_DrawPatch(158, 98, 0, W_CacheLumpName(DEH_String("XHAIR"), PU_CACHE));
    }

    // translucent messages for translucent HUD
    if  (d_translucency && screenblocks > TRANSLUCENT_HUD &&
            (!automapactive || (automapactive && am_overlay)))
        dp_translucent = true;

    V_ClearDPTranslation();

    if(beta_style)
    {
        HUlib_drawSText(&w_message_0);
        HUlib_drawSText(&w_message_1);
        HUlib_drawSText(&w_message_2);
    }
    else
        HUlib_drawSText(&w_message);

    dp_translation = crx[CRX_GOLD];
    HUlib_drawSText(&w_secret);
    V_ClearDPTranslation();

    if (automapactive)
    {
        HUlib_drawTextLine(&w_title, false);

        // display the hud kills/items/secret display if optioned
        if (show_stats == 1)
            HU_DrawStats();

        if (timer_info == 1)
            AM_DrawWorldTimer();
    }
    V_ClearDPTranslation();

    if (dp_translucent)
        dp_translucent = false;

    // demo progress bar
    if (demoplayback)
	HU_DemoProgressBar();
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

        if(beta_style)
        {
            show_chat_bar = false;
            ST_doRefresh();
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
                ST_doRefresh();
            }
            else
                HUlib_addMessageToSText(&w_message, 0, plr->message);

            C_Printf(CR_GREEN, " %s\n", plr->message);
            plr->message = 0;
            message_on = true;
            message_counter = HU_MSGTIMEOUT;
            message_nottobefuckedwith = message_dontfuckwithme;
            message_dontfuckwithme = 0;
        }

    } // else message_on = false;
}

boolean HU_Responder(event_t *ev)
{
    boolean       eatkey = false;

    int           i;
    int           numplayers;
    
    numplayers = 0;

    for (i=0 ; i<MAXPLAYERS ; i++)
        numplayers += playeringame[i];

    return eatkey;
}

// hu_newlevel called when we enter a new level
// determine the level name and display it in
// the console
void HU_NewLevel()
{
    char*       s;

    switch ( logical_gamemission )
    {
      case doom:
        s = HU_TITLE;
        break;
      case doom2:
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
      default:
         s = "Unknown level";
         break;
    }

    // Chex.exe always uses the episode 1 level title
    // eg. E2M1 gives the title for E1M1

    if (gameversion == exe_chex)
    {
        s = HU_TITLE_CHEX;
    }
    // print the new level name into the console

    C_Printf(CR_GRAY, "\n");

    C_AddConsoleDivider();

    C_Printf(CR_GRAY, "\n");

    C_Printf(CR_GRAY, " %s\n", s);

    C_Printf(CR_GRAY, " \n");
}

