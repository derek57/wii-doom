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
//        DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//        plus functions to determine game mode (shareware, registered),
//        parse command line parameters, configure game parameters (turbo),
//        and call the startup functions.
//
//-----------------------------------------------------------------------------


#include <ctype.h>

#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "am_map.h"

#include "c_io.h"
#include "config.h"
#include "d_deh.h"
#include "d_main.h"
#include "d_net.h"
#include "doomdef.h"
#include "doomfeatures.h"
#include "doomstat.h"
#include "dstrings.h"
#include "d_iwad.h"
#include "f_finale.h"
#include "f_wipe.h"
#include "g_game.h"

#ifdef WII
#include "../../wii/gui.h"
#endif

#include "hu_stuff.h"
#include "i_endoom.h"
#include "i_joystick.h"
#include "i_sdlmusic.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_config.h"
#include "m_controls.h"

#include "m_menu.h"
#include "m_argv.h"
#include "m_misc.h"
#include "net_client.h"
#include "net_dedicated.h"
#include "net_query.h"
#include "p_local.h"
#include "p_saveg.h"
#include "p_setup.h"
#include "r_local.h"
#include "s_sound.h"
#include "sounds.h"
#include "st_stuff.h"
#include "statdump.h"

#ifdef WII
#include "../../wii/sys_wpad.h"
#include "../../wii/video.h"
#endif

#include "v_trans.h"
#include "v_video.h"
#include "w_merge.h"
#include "w_main.h"
#include "w_wad.h"

#include "wi_stuff.h"
#include "wii-doom.h"
#include "z_zone.h"


#if !defined(MAX_PATH)
#define MAX_PATH           260
#endif

#define MAXDEHFILES        16


#ifndef WII
/*
static struct 
{
    char *description;
    char *cmdline;
    GameVersion_t version;
} gameversions[] = {
    { "Doom 1.666"      , "1.666"   , exe_doom_1_666 },
    { "Doom 1.7/1.7a"   , "1.7"     , exe_doom_1_7   },
    { "Doom 1.8"        , "1.8"     , exe_doom_1_8   },
    { "Doom 1.9"        , "1.9"     , exe_doom_1_9   },
    { "Hacx"            , "hacx"    , exe_hacx       },
    { "Ultimate Doom"   , "ultimate", exe_ultimate   },
    { "Final Doom"      , "final"   , exe_final      },
    { "Final Doom (alt)", "final2"  , exe_final2     },
    { "Chex Quest"      , "chex"    , exe_chex       },
    {  NULL             , NULL      , 0              }
};
*/

static struct 
{
    char *description;
    GameVersion_t version;
} gameversions[] = {
    { "Doom 1.666"      , exe_doom_1_666 },
    { "Doom 1.7/1.7a"   , exe_doom_1_7   },
    { "Doom 1.8"        , exe_doom_1_8   },
    { "Doom 1.9"        , exe_doom_1_9   },
    { "Hacx"            , exe_hacx       },
    { "Ultimate Doom"   , exe_ultimate   },
    { "Final Doom"      , exe_final      },
    { "Final Doom (alt)", exe_final2     },
    { "Chex Quest"      , exe_chex       },
    {  NULL             , 0              }
};
#endif


static char             dehfiles[MAXDEHFILES][MAX_PATH];
static int              dehfilecount;


skill_t                 startskill;

// wipegamestate can be set to -1 to force a wipe on the next draw
gamestate_t             wipegamestate = GS_DEMOSCREEN;

char                    *pagename = "";

// Location where savegames are stored
char                    *savegamedir;

// location of IWAD and WAD files
char                    *iwadfile;

dboolean                wipe;
dboolean                done;
dboolean                autostart;
dboolean                advancedemo;

// Store demo, do not accept any inputs
dboolean                storedemo;

// "BFG Edition" version of doom2.wad does not include TITLEPIC.
dboolean                bfgedition;

// If true, the main game loop has started.
dboolean                main_loop_started = false;

dboolean                version13 = false;
dboolean                realframe;
dboolean                splashscreen;
dboolean                dots_enabled = false;
dboolean                fps_enabled = false;
dboolean                wad_message_has_been_shown = false;
dboolean                dont_show_adding_of_resource_wad = false;
dboolean                resource_wad_exists = false;

int                     startepisode;
int                     startmap;
int                     startloadgame;
int                     fsize = 0;
int                     fsizerw = 0;
int                     fsizerw2 = 0;
int                     demosequence;
int                     pagetic;
int                     startuptimer;


extern byte             *zone_mem;

extern int              exit_by_reset;
extern int              mp_skill;
extern int              warpepi;
extern int              warplev;
extern int              warped;
extern int              startlump;
extern int              viewheight2;
extern int              oldscreenblocks;
extern int              oldscreenSize;

extern dboolean         merge;
extern dboolean         BorderNeedRefresh;
extern dboolean         setsizeneeded;
extern dboolean         inhelpscreens;
extern dboolean         finale_music;
extern dboolean         show_chat_bar;
extern dboolean         blurred;
extern dboolean         initialized;

extern menu_t           *currentMenu;                          
extern menu_t           CheatsDef;

extern gameaction_t     loadaction;

extern default_collection_t doom_defaults;

// menu item skull is on
extern short            itemOn;


void (*P_BloodSplatSpawner)(fixed_t, fixed_t, int, int, mobj_t *);


//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents(void)
{
    event_t    *ev;

    // IF STORE DEMO, DO NOT ACCEPT INPUT
    if (storedemo)
        return;

    while ((ev = D_PopEvent()) != NULL)
    {
        // menu or console ate the event
        if (M_Responder(ev) || C_Responder(ev))
            continue;

        G_Responder (ev);
    }
}

//
// D_PageDrawer
//
static void D_PageDrawer(void)
{
    if (splashscreen)
    {
        I_SetPalette(W_CacheLumpName("SPLSHPAL", PU_CACHE) + (pagetic >= 95 ? pagetic - 95 :
            (pagetic < 10 ? 10 - pagetic - 1 : 0)) * 768);

        V_DrawPatch(0, 0, 0, W_CacheLumpName("SPLASH", PU_CACHE));
    }
    else
        V_DrawPatch(0, 0, 0, W_CacheLumpName(pagename, PU_CACHE));
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//
static void D_Display(void)
{
    static  dboolean            viewactivestate;
    static  dboolean            menuactivestate;
    static  dboolean            inhelpscreensstate = false;
    static  dboolean            fullscreen = false;

    // [crispy] shade menu background
    static  char                menushade;

    static  gamestate_t         oldgamestate = -1;
    static  int                 borderdrawcount;
    static  int                 saved_gametic = -1;
    int                         nowtime;
    int                         tics;
    int                         wipestart;
    dboolean                    redrawsbar = false;
/*
#ifndef WII
    // for comparative timing / profiling
    if (nodrawers)
        return;
#endif
*/
    realframe = (!d_uncappedframerate || gametic > saved_gametic);

    if (realframe)
    {
        saved_gametic = gametic;
    }

    // change the view size if needed
    if (setsizeneeded)
    {
        R_ExecuteSetViewSize();

        // force background redraw
        oldgamestate = -1;

        borderdrawcount = 3;
    }

    // save the current screen if about to wipe
    if (!beta_style)
    {
        if (gamestate != wipegamestate)
        {
            // ADDED CHECKS FOR PSP TO PREVENT CRASH...
            // ...UPON WIPING SCREEN WITH ENABLED DISPLAY TICKER
            if (dots_enabled)
            {
                display_ticker = false;
            }

            if (fps_enabled)
            {
                display_fps = false;
            }

            wipe = true;
            wipe_StartScreen();
        }
        else
        {
            // ADDED CHECKS FOR PSP TO PREVENT CRASH...
            // ...UPON WIPING SCREEN WITH ENABLED DISPLAY TICKER
            if (dots_enabled)
            {
                display_ticker = true;
            }

            if (fps_enabled)
            {
                display_fps = true;
            }

            wipe = false;
        }
    }

    if (gamestate == GS_LEVEL && gametic)
    {
        HU_Erase();
    }

    // do buffered drawing
    switch (gamestate)
    {
        case GS_LEVEL:
            if (!gametic)
            {
                break;
            }

            // draw the view directly
            if (automapactive && !am_overlay)
            {
                // update automap while playing (full mode)
                R_RenderPlayerView(&players[consoleplayer]);
            }

            if (automapactive)
            {
                AM_Drawer();
            }

            if (wipe || (scaledviewheight != (ORIGINALWIDTH << hires) && fullscreen) ||
                disk_indicator == disk_dirty || (automapactive && screenSize < 8))
            {
                redrawsbar = true;
            }

            // just put away the help screen
            if (inhelpscreensstate && !inhelpscreens)
            {
                redrawsbar = true;
            }

            ST_Drawer(scaledviewheight == (ORIGINALWIDTH << hires), redrawsbar);

            fullscreen = scaledviewheight == (ORIGINALWIDTH << hires);
            break;

        case GS_INTERMISSION:
            WI_Drawer();
            break;

        case GS_FINALE:
            F_Drawer();
            break;

        case GS_DEMOSCREEN:
            D_PageDrawer();
            break;
    }

    // draw buffered stuff to screen
    I_UpdateNoBlit();

    // draw the view directly
    if (gamestate == GS_LEVEL && (!automapactive || am_overlay) && gametic)
    {
        // [crispy] update automap while playing (overlay mode)
        R_RenderPlayerView(&players[displayplayer]);
    }

    // clean up border stuff
    if (gamestate != oldgamestate && gamestate != GS_LEVEL && !splashscreen)
    {
        I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
    }

    // [crispy] in automap overlay mode,
    // the HUD is drawn on top of everything else
    if (gamestate == GS_LEVEL)
    {
        // see if the border needs to be initially drawn
        //if (oldgamestate != GS_LEVEL)
        {
            // view was not active
            viewactivestate = false;

            // draw the pattern into the back screen
            R_FillBackScreen();
        }

        // see if the border needs to be updated to the screen
        if ((!automapactive || am_overlay) /*&& scaledviewwidth != (320 << hires)*/)
        {
            if (menuactive || menuactivestate || !viewactivestate || consoleheight > CONSOLETOP)
            {
                borderdrawcount = 3;
            }

            if (detailLevel)
            {
                V_LowGraphicDetail(viewheight2 * SCREENWIDTH);
            }

            if (borderdrawcount)
            {
                // erase old menu stuff
                R_DrawViewBorder();

                borderdrawcount--;
            }
        }

        if (gametic)
        {
            HU_Drawer();
        }

        if (usergame)
        {
            if (hud && screenSize == 8)
            {
                if (am_overlay || !automapactive)
                {
                    HU_DrawHUD();
                }
            }

            if (!menuactive && devparm && sound_info && !automapactive)
            {
                ST_DrawSoundInfo();
            }
        }
    }

    menuactivestate = menuactive;
    viewactivestate = viewactive;
    inhelpscreensstate = inhelpscreens;
    oldgamestate = wipegamestate = gamestate;

    // [crispy] in automap overlay mode,
    // draw the automap and HUD on top of everything else
    if (am_overlay && !menuactive)
    {
        AM_Drawer();
        HU_Drawer();

        if (usergame)
        {
            if (screenSize < 8)
            {
                ST_doRefresh();
            }
        }

        // [crispy] force redraw of status bar and border
        viewactivestate = false;
        inhelpscreensstate = true;

        // erase old menu stuff
        R_DrawViewBorder();

        if (show_stats)
        {
            HU_DrawStats();
        }

        if (timer_info)
        {
            AM_DrawWorldTimer();
        }
    }

    // [crispy] shade background when a menu is active or the game is paused
    if (((paused || menuactive) && background_type == 1) || inhelpscreens)
    {
        static int firsttic;

        if (!automapactive || am_overlay)
        {
            int y;

            for (y = 0; y < SCREENWIDTH * SCREENHEIGHT; y++)
            {
                screens[0][y] = colormaps[0][menushade * 256 + screens[0][y]];
            }
        }

        if (menushade < 16 && gametic != firsttic)
        {
            menushade += ticdup;
            firsttic = gametic;
        }

        // [crispy] force redraw of status bar and border
        viewactivestate = false;
        inhelpscreensstate = true;
    }
    else if (menushade)
    {
        menushade = 0;
    }

    if (!wipe)
    {
        C_Drawer();
    }

    // draw pause pic

#ifndef WII
    if (paused)
    {
        M_DarkBackground();

        if (!inhelpscreens)
        {
            int     y;
            patch_t *patch = W_CacheLumpName("M_PAUSE", PU_CACHE);

            if (automapactive)
                y = 4;
            else
                y = viewwindowy + 4;

            if (font_shadow == 1)
            {
                V_DrawPatchWithShadow((ORIGINALWIDTH - SHORT(patch->width)) / 2,
                        y, 0, patch, false);
            }
            else
            {
                V_DrawPatch((ORIGINALWIDTH - SHORT(patch->width)) / 2,
                        y, 0, patch);
            }
        }
    }
#endif

    // menus go directly to the screen
    // menu is drawn even on top of everything
    M_Drawer();

    // send out any new accumulation
    NetUpdate();

    if (screenSize < 8 && !wipe && !beta_style)
    {
        if ((usergame && !inhelpscreens && gamestate == GS_LEVEL) /*|| demoplayback*/)
        {
            ST_doRefresh();
        }
    }
/*
    // [crispy] demo progress bar
    if (demoplayback)
        HU_DemoProgressBar(0);
*/
    if (usergame && oldscreenblocks && !inhelpscreens && !menuactive && screenSize != oldscreenSize && !am_overlay)
    {
        screenSize = oldscreenSize;
        screenblocks = oldscreenblocks;
        R_SetViewSize(screenblocks);
    }

    // normal update
    if (!wipe)
    {
        // page flip or blit buffer
        I_FinishUpdate();

        return;
    }

    // wipe update
    wipe_EndScreen();

    wipestart = I_GetTime() - 1;

    do
    {
        do
        {
            nowtime = I_GetTime();
            tics = nowtime - wipestart;
            I_Sleep(1);

        } while (tics <= 0);

        wipestart = nowtime;
        done = wipe_ScreenWipe(wipe_type, tics);
        blurred = false;
        C_Drawer();
        I_UpdateNoBlit();

        // menu is drawn even on top of wipes
        M_Drawer();

        // page flip or blit buffer
        I_FinishUpdate();

    } while (!done);

    if (beta_style && done)
    {
        show_chat_bar = false;
    }

#ifdef WII
    if (done)
    {
        BorderNeedRefresh = true;

        if (warped == 1)
        {
            paused = true;
            currentMenu = &CheatsDef;
            menuactive = 1;
            itemOn = currentMenu->lastOn;
            warped = 0;
        }
    }
#endif

    if (loadaction != ga_nothing)
        G_LoadedGameMessage();
}

//
// Add configuration file variable bindings.
//
static void D_BindVariables(void)
{
    int i;

    M_BindBaseControls();
    M_BindChatControls(MAXPLAYERS);

    key_multi_msgplayer[0] = HUSTR_KEYGREEN;
    key_multi_msgplayer[1] = HUSTR_KEYINDIGO;
    key_multi_msgplayer[2] = HUSTR_KEYBROWN;
    key_multi_msgplayer[3] = HUSTR_KEYRED;

#ifdef FEATURE_MULTIPLAYER
    NET_BindVariables();
#endif

    // Multiplayer chat macros
    for (i = 0; i < 10; ++i)
    {
        char buf[12];

        M_snprintf(buf, sizeof(buf), "chatmacro%i", i);
        M_BindStringVariable(buf, &chat_macros[i]);
    }
}

//
// D_GrabMouseCallback
//
// Called to determine whether to grab the mouse pointer
//
#ifndef WII
static dboolean D_GrabMouseCallback(void)
{
    // Drone players don't need mouse focus
    if (drone)
    {
        return false;
    }

    // when menu is active or game is paused, release the mouse 
    if (menuactive || paused)
    {
        return false;
    }

    // only grab mouse when playing levels (but not demos)
    return (gamestate == GS_LEVEL) /*&& !demoplayback*/ && !advancedemo;
}
#endif

//
// D-DoomLoop()
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//
void D_DoomLoop(void)
{
    if (bfgedition &&
        (/*demorecording || (gameaction == ga_playdemo) ||*/ netgame))
    {
        C_Warning("WARNING: You are playing using one of the Doom Classic");
        C_Warning("IWAD files shipped with the Doom 3: BFG Edition. These are");
        C_Warning("known to be incompatible with the regular IWAD files and");
        C_Warning("may cause demos and network games to get out of sync.");
    }
/*
    if (demorecording)
        G_BeginRecording();
*/

#ifdef WII
    if (usb)
    {
        debugfile = fopen("usb:/apps/wiidoom/debug.txt", "w");
    }
    else if (sd)
    {
        debugfile = fopen("sd:/apps/wiidoom/debug.txt", "w");
    }

#else

    debugfile = fopen("debug.txt", "w");
#endif

    main_loop_started = true;

    TryRunTics();

#ifndef WII
    I_SetWindowTitle(gamedescription);

    if (!beta_style)
    {
        I_GraphicsCheckCommandLine();
    }

    I_SetGrabMouseCallback(D_GrabMouseCallback);

    if (beta_style)
    {
        printf(" I_StartupGraphics\n");
    }
#endif

    I_InitGraphics();

    //V_RestoreBuffer();

    R_ExecuteSetViewSize();

    D_StartGameLoop();

    // Set title and up icon. This has to be done
    // before the call to SDL_SetVideoMode.

#ifndef WII
    I_InitWindowTitle();
    I_InitWindowIcon();
#endif

    while (1)
    {
#ifdef WII
        if (exit_by_reset)
        {
            break;
        }
#endif

        if (usergame && gamestate != GS_DEMOSCREEN && !finale_music)
        {
            I_SDL_PollMusic();
        }

        snd_channels = sound_channels;

        // frame syncronous IO operations
        I_StartFrame();

        // will run at least one tic
        TryRunTics();

        // move positional sounds
        S_UpdateSounds(players[consoleplayer].mo);

        // Update display, next frame, with current state.

#ifndef WII
        if (screenvisible)
#endif
        {
            D_Display();
        }
    }
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo(void)
{
    advancedemo = true;
}

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker(void)
{
    if (!menuactive && !consoleheight)
    {
        if (--pagetic < 0)
        {
            D_AdvanceDemo();
        }
    }
}

//
// This cycles through the demo sequences.
// FIXME - version dependend demo numbers?
//
void D_DoAdvanceDemo(void)
{
    // not reborn
    players[consoleplayer].playerstate = PST_LIVE;

    // no save / end game here
    usergame = false;

    advancedemo = false;
    paused = false;
    gameaction = ga_nothing;
    blurred = false;

    // The Ultimate Doom executable changed the demo sequence to add
    // a DEMO4 demo.  Final Doom was based on Ultimate, so also
    // includes this change; however, the Final Doom IWADs do not
    // include a DEMO4 lump, so the game bombs out with an error
    // when it reaches this point in the demo sequence.

    // However! There is an alternate version of Final Doom that
    // includes a fixed executable.
    if (gameversion == exe_ultimate || gameversion == exe_final)
    {
        demosequence = (demosequence + 1) % 7;
    }
    else
    {
        demosequence = (demosequence + 1) % 6;
    }

    switch (demosequence)
    {
        case -1:
            gamestate = GS_DEMOSCREEN;

            if (drawsplash)
            {
                pagetic = TICRATE * 3;
                splashscreen = true;
            }
            else
            {
                pagetic = 0;
                splashscreen = false;
            }

            break;

        case 0:
            if (gamemode == commercial)
            {
                pagetic = TICRATE * 11;
            }
            else
            {
                pagetic = 170;
            }

            gamestate = GS_DEMOSCREEN;

            if (nerve_pwad)
            {
                pagename = "INTERPIC";
            }
            else
            {
                pagename = "TITLEPIC";
            }

            if (splashscreen)
            {
                I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
                splashscreen = false;
            }

            if (gamemode == commercial)
            {
                S_StartMusic(mus_dm2ttl);
            }
            else
            {
                S_StartMusic(mus_intro);
            }

            break;

        case 1:
            //G_DeferedPlayDemo("demo1");
            break;

        case 2:
            pagetic = 200;
            gamestate = GS_DEMOSCREEN;
            pagename = "CREDIT";

            break;

        case 3:
            //G_DeferedPlayDemo("demo2");
            break;

        case 4:
            gamestate = GS_DEMOSCREEN;

            if (gamemode == commercial)
            {
                pagetic = TICRATE * 11;

                if (nerve_pwad)
                {
                    pagename = "INTERPIC";
                }
                else
                {
                    pagename = "TITLEPIC";
                }

                S_StartMusic(mus_dm2ttl);
            }
            else
            {
                pagetic = 200;

                if (gamemode == retail)
                {
                    pagename = "CREDIT";
                }
                else
                {
                    if (fsize != 12361532)
                    {
                        pagename = "HELP2";
                    }
                    else
                    {
                        pagename = "HELP1";
                    }
                }
            }

            break;

        case 5:
            //G_DeferedPlayDemo("demo3");
            break;

        // THE DEFINITIVE DOOM Special Edition demo
        case 6:
            //G_DeferedPlayDemo("demo4");
            break;
    }

    // The Doom 3: BFG Edition version of doom2.wad does not have a
    // TITLETPIC lump. Use INTERPIC instead as a workaround.
    if (bfgedition && M_StringCompare(pagename, "TITLEPIC")
        && W_CheckNumForName("titlepic") < 0)
    {
        pagename = "INTERPIC";
    }
}

//
// D_StartTitle
//
void D_StartTitle(void)
{
    gameaction = ga_nothing;

    if (drawsplash && !initialized)
        demosequence = -2;
    else
        demosequence = -1;

    D_AdvanceDemo();
}

// Strings for dehacked replacements of the startup banner
//
// These are from the original source: some of them are perhaps
// not used in any dehacked patches
static char *banners[] =
{
    // doom2.wad
    "                         "
    "DOOM 2: Hell on Earth v%i.%i"
    "                           ",
    // doom1.wad
    "                            "
    "DOOM Shareware Startup v%i.%i"
    "                           ",
    // doom.wad
    "                            "
    "DOOM Registered Startup v%i.%i"
    "                           ",
    // Registered DOOM uses this
    "                          "
    "DOOM System Startup v%i.%i"
    "                          ",
    // doom.wad (Ultimate DOOM)
    "                         "
    "The Ultimate DOOM Startup v%i.%i"
    "                        ",
    // tnt.wad
    "                     "
    "DOOM 2: TNT - Evilution v%i.%i"
    "                           ",
    // plutonia.wad
    "                   "
    "DOOM 2: Plutonia Experiment v%i.%i"
    "                           ",
};

//
// Get game name: if the startup banner has been replaced, use that.
// Otherwise, use the name given
// 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

static char *GetGameName(char *gamename)
{
    size_t i;
    
    for (i = 0; i < arrlen(banners); ++i)
    {
        char *deh_sub = banners[i];

        // Has the banner been replaced?        
        if (deh_sub != banners[i])
        {
            size_t gamename_size = strlen(deh_sub) + 10;
            //int version = G_VanillaVersionCode();

            // Has been replaced.
            // We need to expand via printf to include the Doom version number
            // We also need to cut off spaces to get the basic name
            gamename = Z_Malloc(gamename_size, PU_STATIC, NULL);
            M_snprintf(gamename, gamename_size, deh_sub);

            while (gamename[0] != '\0' && isspace(gamename[0]))
            {
                memmove(gamename, gamename + 1, gamename_size - 1);
            }

            while (gamename[0] != '\0' && isspace(gamename[strlen(gamename)-1]))
            {
                gamename[strlen(gamename) - 1] = '\0';
            }

            return gamename;
        }
    }

    return gamename;
}

// Set the gamedescription string
static void D_SetGameDescription(void)
{
    dboolean is_freedoom = W_CheckNumForName("FREEDOOM") >= 0;
    dboolean is_freedm = W_CheckNumForName("FREEDM") >= 0;

    gamedescription = "Unknown";

    if (logical_gamemission == doom)
    {
        // Doom 1.  But which version?
        if (is_freedoom)
        {
            gamedescription = GetGameName("Freedoom: Phase 1");
        }
        else if (gamemode == retail)
        {
            // Ultimate Doom
            gamedescription = GetGameName("The Ultimate DOOM");
        } 
        else if (gamemode == registered)
        {
            gamedescription = GetGameName("DOOM Registered");
        }
        else if (gamemode == shareware)
        {
            gamedescription = GetGameName("DOOM Shareware");
        }
    }
    else
    {
        // Doom 2 of some kind.  But which mission?
        if (is_freedoom)
        {
            if (is_freedm)
            {
                gamedescription = GetGameName("FreeDM");
            }
            else
            {
                gamedescription = GetGameName("Freedoom: Phase 2");
            }
        }
        else if (logical_gamemission == doom2)
        {
            gamedescription = GetGameName("DOOM 2: Hell on Earth");
        }
        else if (logical_gamemission == pack_plut)
        {
            gamedescription = GetGameName("DOOM 2: Plutonia Experiment"); 
        }
        else if (logical_gamemission == pack_tnt)
        {
            gamedescription = GetGameName("DOOM 2: TNT - Evilution");
        }
        else if (logical_gamemission == pack_nerve)
        {
            gamedescription = GetGameName("DOOM 2: No Rest For The Living");
        }
        else if (logical_gamemission == pack_master)
        {
            gamedescription = GetGameName("Master Levels for DOOM 2");
        }
    }
}

static dboolean D_AddFile(char *filename, dboolean automatic)
{
    wad_file_t *handle;

    if ((gamemode == shareware ||
#ifdef WII
       load_extra_wad ||
#endif
       version13) && !beta_style
       )
    {
//        if (!dont_show_adding_of_resource_wad)
        {
            printf("         adding %s\n", filename);
        }
    }

    handle = W_AddFile(filename, automatic);

    return handle != NULL;
}

// Load the Chex Quest dehacked file, if we are in Chex mode.
static void LoadChexDeh(void)
{
    if (gameversion == exe_chex)
    {
#ifndef WII
        char *chex_deh = NULL;
        char *sep = strrchr(iwadfile, DIR_SEPARATOR);

        // Look for chex.deh in the same directory as the IWAD file.
        if (sep != NULL)
        {
            size_t chex_deh_len = strlen(iwadfile) + 9;

            chex_deh = malloc(chex_deh_len);
            M_StringCopy(chex_deh, iwadfile, chex_deh_len);
            chex_deh[sep - iwadfile + 1] = '\0';
            M_StringConcat(chex_deh, "chex.deh", chex_deh_len);
        }
        else
        {
            chex_deh = M_StringDuplicate("chex.deh");
        }

        // If the dehacked patch isn't found, try searching the WAD
        // search path instead.  We might find it...
        if (!M_FileExists(chex_deh))
        {
            free(chex_deh);
            chex_deh = D_FindWADByName("chex.deh");
        }

        // Still not found?
        if (chex_deh == NULL)
        {
            I_Error("\nUnable to find Chex Quest dehacked file (chex.deh).\n"
                    "The dehacked file is required in order to emulate\n"
                    "chex.exe correctly.  It can be found in your nearest\n"
                    "/idgames repository mirror at:\n\n"
                    "   utils/exe_edit/patches/chexdeh.zip\n");
        }

        LoadDehFile(chex_deh);

#else

        if (devparm_chex)
        {
            if (usb)
            {
                D_AddFile("usb:/apps/wiidoom/IWAD/CHEX/CHEX.DEH", true);
            }
            else if (sd)
            {
                D_AddFile("sd:/apps/wiidoom/IWAD/CHEX/CHEX.DEH", true);
            }
        }
#endif
    }
}

/*
static void G_CheckDemoStatusAtExit(void)
{
    G_CheckDemoStatus();
}

#ifndef WII
static void SetMissionForPackName(char *pack_name)
{
    int i;

    static const struct
    {
        char *name;
        int  mission;
    } packs[] = {
        { "doom2"   , doom2     },
        { "tnt"     , pack_tnt  },
        { "plutonia", pack_plut }
    };

    for (i = 0; i < arrlen(packs); ++i)
    {
        if (M_StringCompare(pack_name, packs[i].name))
        {
            gamemission = packs[i].mission;
            return;
        }
    }

    printf("Valid mission packs are:\n");

    for (i = 0; i < arrlen(packs); ++i)
    {
        printf("\t%s\n", packs[i].name);
    }

    I_Error("Unknown mission pack name: %s", pack_name);
}
#endif
*/

static void D_ProcessDehInWad(void)
{
    int i;

    if (chexdeh 
/*
#ifndef WII
        || M_ParmExists("-nodeh")
#endif
*/
       )
    {
        return;
    }

    for (i = 0; i < numlumps; ++i)
    {
        if (!strncasecmp(lumpinfo[i]->name, "DEHACKED", 8))
        {
            ProcessDehFile(NULL, i);
        }
    }
}

static dboolean DehFileProcessed(char *path)
{
    int i;

    for (i = 0; i < dehfilecount; ++i)
    {
        if (M_StringCompare(path, dehfiles[i]))
        {
            return true;
        }
    }

    return false;
}

void LoadDehFile(char *path)
{
    if ((gameversion == exe_chex || gamemission == pack_chex) && !path)
    {
        I_Error("Failed to load chex.deh needed for emulating chex.exe.");
    }

    if (
/*
#ifndef WII
        !M_ParmExists("-nodeh") &&
#endif
*/
        !HasDehackedLump(path))
    {
        char    *dehpath = M_StringReplace(path, ".wad", ".bex");

        if (M_FileExists(dehpath) && !DehFileProcessed(dehpath))
        {
            if (fsize == 12361532)
            {
                chexdeh = true;
            }

            ProcessDehFile(dehpath, 0);

            if (dehfilecount < MAXDEHFILES)
            {
                M_StringCopy(dehfiles[dehfilecount++], dehpath, MAX_PATH);
            }
        }
        else
        {
            // ???
            //char    *dehpath = M_StringReplace(path, ".wad", ".deh");

            if (M_FileExists(dehpath) && !DehFileProcessed(dehpath))
            {
                ProcessDehFile(dehpath, 0);

                if (dehfilecount < MAXDEHFILES)
                {
                    M_StringCopy(dehfiles[dehfilecount++], dehpath, MAX_PATH);
                }
            }
        }
    }
}

dboolean D_IsDehFile(char *filename)
{
    return (M_StringCompare(filename + strlen(filename) - 4, ".deh")
        || M_StringCompare(filename + strlen(filename) - 4, ".bex"));
}

static void D_ProcessDehCommandLine(void)
{
#ifndef WII
    int p = M_CheckParm("-deh");

    if (beta_style)
    {
        return;
    }

    if (p || (p = M_CheckParm("-bex")))
#else
    if (load_dehacked)
#endif
    {
#ifndef WII
        dboolean        deh = true;

        while (++p < myargc)
        {
            if (*myargv[p] == '-')
            {
                deh = (M_StringCompare(myargv[p], "-deh") || M_StringCompare(myargv[p], "-bex"));
            }
            else if (deh)
            {
                ProcessDehFile(myargv[p], 0);
            }
        }
#else
        ProcessDehFile(dehacked_file, 0);
#endif
    }
}

#ifndef WII
static void PrintGameVersion(void)
{
    int i;

    for (i = 0; gameversions[i].description != NULL; ++i)
    {
        if (gameversions[i].version == gameversion)
        {
            C_Output("Emulating the behavior of the "
                   "'%s' executable.", gameversions[i].description);
            break;
        }
    }
}

// Function called at exit to display the ENDOOM screen
static void D_Endoom(void)
{
    byte *endoom;

    // Don't show ENDOOM if we have it disabled, or we're running
    // in screensaver or control test mode. Only show it once the
    // game has actually started.
    if (!show_endoom || !main_loop_started || screensaver_mode)
    {
        return;
    }

    if (beta_style)
    {
        endoom = W_CacheLumpName("BENDOOM", PU_STATIC);
    }
    else
    {
        endoom = W_CacheLumpName("ENDOOM", PU_STATIC);
    }

    I_Endoom(endoom);
}
#endif

static void D_ParseStartupString(const char *string)
{
    size_t      i;
    size_t      start;
    size_t      len = strlen(string);

    for (i = 0, start = 0; i < len; ++i)
    {
        if (string[i] == '\n' || i == len - 1)
        {
            C_Output("%s", M_SubString(string, start, i - start));
            start = i + 1;
        }
    }
}

//
// D_DoomMain
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

void D_DoomMain(void)
{
    FILE *fprw = NULL;
//    char demolumpname[9];

#ifndef WII
    FILE *iwad;
    int p;

    I_AtExit(D_Endoom, false);
#endif

    if (devparm_doom || devparm_net_doom)
    {
        fsize = 10399316;
    }
    else if (devparm_doom2 || devparm_net_doom2)
    {
        fsize = 14943400;
    }
    else if (devparm_chex || devparm_net_chex)
    {
        fsize = 12361532;
    }
    else if (devparm_hacx || devparm_net_hacx)
    {
        fsize = 19321722;
    }
    else if (devparm_freedoom2 || devparm_net_freedoom2)
    {
        fsize = 28422764;
    }
    else if (devparm_tnt || devparm_net_tnt)
    {
        fsize = 18654796;
    }
    else if (devparm_plutonia || devparm_net_plutonia)
    {
        fsize = 18240172;
    }

    printf(" \n");

#ifdef WII

    if (usb)
    {
        fprw = fopen("usb:/apps/wiidoom/pspdoom.wad", "rb");
    }
    else if (sd)
    {
        fprw = fopen("sd:/apps/wiidoom/pspdoom.wad", "rb");
    }

#else

    fprw = fopen("pspdoom.wad", "rb");

#endif

    if (fprw)
    {
        resource_wad_exists = true;

        fclose(fprw);

        W_CheckSize(0);
    }
    else
    {
        resource_wad_exists = false;
    }

    if (print_resource_pwad_error)
    {
        printf("\n\n\n\n\n");
        printf(" ===============================================================================");

#ifndef WII

        printf("\n");

#endif

        printf("                         !!! WRONG RESOURCE PWAD FILE !!!                       ");

#ifndef WII

        printf("\n");

#endif

        printf("                   PLEASE COPY THE FILE 'PSPDOOM.WAD' THAT CAME                 ");

#ifndef WII

        printf("\n");

#endif

        printf("                    WITH THIS RELEASE, INTO THE GAME DIRECTORY                  \n");
        printf("                                                                                \n");
        printf("                                QUITTING NOW ...                                ");

#ifndef WII

        printf("\n");

#endif

        printf(" ===============================================================================");

#ifndef WII

        printf("\n");

#endif

        sleep(5);

        I_QuitSerialFail();
    }

    if (!resource_wad_exists)
    {
        printf("\n\n\n\n\n");
        printf(" ===============================================================================");

#ifndef WII

        printf("\n");

#endif

        printf("              WARNING: RESOURCE PWAD FILE 'PSPDOOM.WAD' MISSING!!!              ");

#ifndef WII

        printf("\n");

#endif

        printf("               PLEASE COPY THIS FILE INTO THE GAME'S DIRECTORY!!!               \n");
        printf("                                                                                \n");
        printf("                               QUITTING NOW ...                                 ");

#ifndef WII

        printf("\n");

#endif

        printf(" ===============================================================================");

#ifndef WII

        printf("\n");

#endif

        sleep(5);

        I_QuitSerialFail();
    }

#ifdef FEATURE_MULTIPLAYER
    //!
    // @category net
    //
    // Start a dedicated server, routing packets but not participating
    // in the game itself.
    //
    if (M_CheckParm("-dedicated") > 0)
    {
        printf("Dedicated server mode.\n");
        NET_DedicatedServer();

        // Never returns
    }

    //!
    // @category net
    //
    // Query the Internet master server for a global list of active
    // servers.
    //
    if (M_CheckParm("-search"))
    {
        NET_MasterQuery();
        exit(0);
    }

    //!
    // @arg <address>
    // @category net
    //
    // Query the status of the server running on the given IP
    // address.
    //
    p = M_CheckParmWithArgs("-query", 1);

    if (p)
    {
        NET_QueryAddress(myargv[p + 1]);
        exit(0);
    }

    //!
    // @category net
    //
    // Search the local LAN for running servers.
    //
    if (M_CheckParm("-localsearch"))
    {
        NET_LANQuery();
        exit(0);
    }

#endif

#ifndef WII

    if (M_CheckParm("-pressrelease"))
    {
        beta_style_mode = true;
    }
    else
    {
        beta_style_mode = false;
    }

    if (beta_style_mode)
    {
        if (gamemode != registered && gamemode != retail)
        {

#ifdef SDL2
            char *msgbuf = "WARNING: YOU ARE TRYING TO RUN IN PR BETA MODE, USING A 'NON-DOOM 1-IWAD'.\n"
                           "YOU NEED TO HAVE AT LEAST THE REGISTERED VERSION OF DOOM 1!\n"
                           "BETA MODE WILL BE DISABLED!";

            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PACKAGE_NAME, msgbuf, NULL);
#else
            printf("\n WARNING: YOU ARE TRYING TO RUN IN PR BETA MODE, USING A 'NON-DOOM 1-IWAD'.");
            printf("\n YOU NEED TO HAVE AT LEAST THE REGISTERED VERSION OF DOOM 1!");
            printf("\n BETA MODE WILL BE DISABLED!\n\n");
#endif

            beta_style_mode = false;
        }

        if (beta_style_mode)
        {
            printf(" heap size: 0x3cbba10 \n");
        }
    }

    if (M_CheckParm("-devparm"))
    {
        devparm = true;
    }

#endif

    respawnparm = false;
    fastparm = false;

    //!
    // @vanilla
    //
    // Disable monsters.
    //
#ifndef WII
    if (M_CheckParm("-nomonsters"))
    {
        nomonsters = true;
    }
#endif

    //!
    // @vanilla
    //
    // Monsters respawn after being killed.
    //
#ifndef WII
    if (M_CheckParm("-respawn"))
    {
        respawnparm = true;
    }
#endif

    //!
    // @vanilla
    //
    // Monsters move faster.
    //
#ifndef WII
    if (M_CheckParm("-fast"))
    {
        fastparm = true;
    }
#endif

    if (beta_style_mode && devparm)
    {
        devparm = false;
        nomonsters = false;
        respawnparm = false;
        fastparm = false;
    }

    //! 
    // @vanilla
    //
    // Developer mode.  F1 saves a screenshot in the current working
    // directory.
    //
    I_DisplayFPSDots(devparm);

    if (devparm)
    {
        display_ticker = true;
    }

    //!
    // @category net
    // @vanilla
    //
    // Start a deathmatch game.
    //
#ifndef WII
    if (M_CheckParm("-deathmatch"))
    {
        deathmatch = 1;
    }

    //!
    // @category net
    // @vanilla
    //
    // Start a deathmatch 2.0 game.  Weapons do not stay in place and
    // all items respawn after 30 seconds.
    //
    if (M_CheckParm("-altdeath"))
    {
        deathmatch = 2;
    }
#endif

    // Auto-detect the configuration dir.
    M_SetConfigDir(NULL);

/*
    // FIXME: forwardmove / sidemove
    //!
    // @arg <x>
    // @vanilla
    //
    // Turbo mode.  The player's speed is multiplied by x%.  If unspecified,
    // x defaults to 200.  Values are rounded up to 10 and down to 400.
    //

#ifndef WII
    if ((p = M_CheckParm("-turbo")))
    {
        int     scale = 200;
        extern int forwardmove[2];
        extern int sidemove[2];
        
        if (p < myargc - 1)
        {
            scale = atoi (myargv[p + 1]);
        }

        if (scale < 10)
        {
            scale = 10;
        }

        if (scale > 400)
        {
            scale = 400;
        }

        printf("turbo scale: %i%%\n", scale);

        forwardmove[0] = forwardmove[0] * scale / 100;
        forwardmove[1] = forwardmove[1] * scale / 100;
        sidemove[0] = sidemove[0] * scale / 100;
        sidemove[1] = sidemove[1] * scale / 100;
    }
#endif
*/    

    // init subsystems
    V_Init();

    // Load configuration files before initialising other subsystems.
    M_SetConfigFilenames("default.cfg");
    D_BindVariables();

    C_PrintCompileDate();
    C_PrintSDLVersions();

    M_LoadDefaults();

    if (LoadDefaultCollection(&doom_defaults))
        C_Output("Loaded VARs from %s.", uppercase(doom_defaults.filename));
    else
        C_Output("%s not found. Using defaults for all VARs and creating %s.",
            uppercase(doom_defaults.filename), uppercase(doom_defaults.filename));

    if (runcount < 32768)
    {
        runcount++;
    }

    if (!devparm && aiming_help != 0)
    {
        aiming_help = 0;
    }

    if (mus_engine == 1 || mus_engine == 2)
    {
        snd_musicdevice = SNDDEVICE_SB;

        if (mus_engine == 1)
        {
            opl_type = false;
        }
        else
        {
            opl_type = true;
        }
    }
    else if (mus_engine == 3)
    {
        snd_musicdevice = SNDDEVICE_GENMIDI;
    }
    else if (mus_engine == 4)
    {
        snd_musicdevice = SNDDEVICE_GUS;
    }

    if (beta_style_mode)
    {
        musicVolume = 8;
        sfxVolume = 8;
        showMessages = true;        
        drawgrid = false;
        followplayer = true;
        show_stats = false;
        timer_info = false;
        use_vanilla_weapon_change = true;
        chaingun_tics = 4;
        crosshair = false;
        d_colblood = false;
        d_colblood2 = false;
        d_swirl = false;
        autoaim = true;
        background_type = 0;
        icontype = false;
        wipe_type = 2;
        mouselook = 0;
        mspeed = 2;
        mus_engine = 1;
        snd_module = false;
        snd_chans = 1;
        sound_channels = 8;
        opl_type = false;
        use_libsamplerate = 0;
        gore_amount = 1;
        display_fps = false;
        font_shadow = 0;
        show_endoom = true;
        forwardmove = 29;
        sidemove = 24; 
        turnspeed = 7;
        detailLevel = false;
        screenblocks = 10;
        screenSize = 7;
        usegamma = 10;
        mouseSensitivity = 5;
        r_bloodsplats_max = 32768;
        correct_lost_soul_bounce = true;
        dots_enabled = false;

        display_ticker = false;
        am_overlay = false;
        nerve_pwad = false;
        master_pwad = false;
        d_recoil = false;
        d_maxgore = false;
        d_thrust = false;
        respawnparm = false;
        fastparm = false;
        d_footstep = false;
        d_footclip = false;
        d_splash = false;
        d_translucency = false;
        d_chkblood = false;
        d_chkblood2 = false;
        d_uncappedframerate = 0;
        d_flipcorpses = false;
        d_secrets = false;
        smoketrails = false;
        sound_info = false;
        autodetect_hom = false;
        d_fallingdamage = false;
        d_infiniteammo = false;
        not_monsters = false;
        overlay_trigger = false;
        replace_missing = false;
        d_telefrag = true;
        d_doorstuck = true;
        d_resurrectghosts = false;
        d_limitedghosts = true;
        d_blockskulls = false;
        d_blazingsound = false;
        d_god = true;
        d_floors = false;
        d_moveblock = false;
        d_model = false;
        d_666 = false;
        d_maskedanim = false;
        d_sound = false;
        d_ouchface = false;
        show_authors = false;
        d_shadows = false;
        d_fixspriteoffsets = false;
        d_brightmaps = false;
        d_fixmaperrors = false;
        d_altlighting = false;
        allow_infighting = false;
        last_enemy = false;
        float_items = false;
        animated_drop = false;
        crush_sound = false;
        disable_noise = false;
        corpses_nudge = false;
        corpses_slide = false;
        corpses_smearblood = false;
        show_diskicon = true;
        randomly_colored_playercorpses = false;
        mousewalk = true;
        am_rotate = false;
        jumping = false;
        general_sound = false;
        lowhealth = false;
        d_fixwiggle = false;
        d_centerweapon = false;
        d_ejectcasings = false;
        d_statusmap = true;
        show_title = false;
        render_mode = true;
        d_drawparticles = false;
        d_drawbfgcloud = false;
        d_drawrockettrails = false;
        d_drawrocketexplosions = false;
        d_drawbfgexplosions = false;
        d_spawnflies = false;
        d_dripblood = false;
        d_vsync = false;
        particle_sounds = false;
        bloodsplat_particle = 0;
        bulletpuff_particle = 0;
        teleport_particle = 0;
        aiming_help = false;
        d_spawnteleglit = 0;
        png_screenshots = false;
        opl_stereo_correct = false;
        randompitch = false;
        hud = false;
        remove_slime_trails = false;

        beta_style = true;
    }
    else
    {
        beta_style = false;
    }

#ifdef WII
    if (fsize != 4261144  &&  // DOOM BETA v1.4
        fsize != 4271324  &&  // DOOM BETA v1.5
        fsize != 4211660  &&  // DOOM BETA v1.6
        fsize != 10396254 &&  // DOOM REGISTERED v1.1
        fsize != 10399316 &&  // DOOM REGISTERED v1.2
        fsize != 10401760 &&  // DOOM REGISTERED v1.6
        fsize != 11159840 &&  // DOOM REGISTERED v1.8
        fsize != 12408292 &&  // DOOM REGISTERED v1.9 (THE ULTIMATE DOOM)
        fsize != 12474561 &&  // DOOM REGISTERED (BFG-XBOX360 EDITION)
        fsize != 12487824 &&  // DOOM REGISTERED (BFG-PC EDITION)
        fsize != 12538385 &&  // DOOM REGISTERED (XBOX EDITION)
        fsize != 4207819  &&  // DOOM SHAREWARE v1.0
        fsize != 4274218  &&  // DOOM SHAREWARE v1.1
        fsize != 4225504  &&  // DOOM SHAREWARE v1.2
        fsize != 4225460  &&  // DOOM SHAREWARE v1.25 (SYBEX RELEASE)
        fsize != 4234124  &&  // DOOM SHAREWARE v1.666
        fsize != 4196020  &&  // DOOM SHAREWARE v1.8
        fsize != 14943400 &&  // DOOM 2 REGISTERED v1.666
        fsize != 14824716 &&  // DOOM 2 REGISTERED v1.666 (GERMAN VERSION)
        fsize != 14612688 &&  // DOOM 2 REGISTERED v1.7
        fsize != 14607420 &&  // DOOM 2 REGISTERED v1.8 (FRENCH VERSION)
        fsize != 14604584 &&  // DOOM 2 REGISTERED v1.9
        fsize != 14677988 &&  // DOOM 2 REGISTERED (BFG-PSN EDITION)
        fsize != 14691821 &&  // DOOM 2 REGISTERED (BFG-PC EDITION)
        fsize != 14683458 &&  // DOOM 2 REGISTERED (XBOX EDITION)
        fsize != 18195736 &&  // FINAL DOOM - TNT v1.9 (WITH YELLOW KEYCARD BUG)
        fsize != 18654796 &&  // FINAL DOOM - TNT v1.9 (WITHOUT YELLOW KEYCARD BUG)
        fsize != 18240172 &&  // FINAL DOOM - PLUTONIA v1.9 (WITH DEATHMATCH STARTS)
        fsize != 17420824 &&  // FINAL DOOM - PLUTONIA v1.9 (WITHOUT DEATHMATCH STARTS)

//        fsize != 19801320 &&  // FREEDOOM v0.6.4
//        fsize != 27704188 &&  // FREEDOOM v0.7 RC 1
//        fsize != 27625596 &&  // FREEDOOM v0.7
//        fsize != 28144744 &&  // FREEDOOM v0.8 BETA 1
//        fsize != 28592816 &&  // FREEDOOM v0.8
//        fsize != 19362644 &&  // FREEDOOM v0.8 PHASE 1

        fsize != 28422764 &&  // FREEDOOM v0.8 PHASE 2
        fsize != 12361532 &&  // CHEX QUEST

//        fsize != 9745831  &&  // HACX SHAREWARE v1.0
//        fsize != 21951805 &&  // HACX REGISTERED v1.0
//        fsize != 22102300 &&  // HACX REGISTERED v1.1

        fsize != 19321722)    // HACX REGISTERED v1.2
    {
        printf("\n\n\n\n\n");
        printf(" ===============================================================================");
        printf("            WARNING: DOOM / DOOM 2 / TNT / PLUTONIA IWAD FILE MISSING,          ");
        printf("                         NOT SELECTED OR WRONG IWAD !!!                         \n");
        printf("                                                                                \n");
        printf("                                QUITTING NOW ...                                ");
        printf(" ===============================================================================");

        sleep(5);

        I_QuitSerialFail();
    }
    else if (fsize == 10396254   || // DOOM REGISTERED v1.1
             fsize == 10399316   || // DOOM REGISTERED v1.2
             fsize == 4207819    || // DOOM SHAREWARE v1.0
             fsize == 4274218    || // DOOM SHAREWARE v1.1
             fsize == 4225504    || // DOOM SHAREWARE v1.2
             fsize == 4225460)      // DOOM SHAREWARE v1.25 (SYBEX RELEASE)
    {
        if (!beta_style)
        {
            printStyledText(1, 1, CONSOLE_FONT_BLUE, CONSOLE_FONT_YELLOW,
            CONSOLE_FONT_BOLD, &stTexteLocation,
            "                          DOOM Operating System v1.2                           ");
        }
    }
    else if (fsize == 4261144    || // DOOM BETA v1.4
             fsize == 4271324    || // DOOM BETA v1.5
             fsize == 4211660    || // DOOM BETA v1.6
             fsize == 10401760   || // DOOM REGISTERED v1.6
             fsize == 11159840   || // DOOM REGISTERED v1.8
             fsize == 4234124    || // DOOM SHAREWARE v1.666
             fsize == 4196020)      // DOOM SHAREWARE v1.8
    {
        if (!beta_style)
        {
            printStyledText(1, 1, CONSOLE_FONT_RED, CONSOLE_FONT_WHITE,
            CONSOLE_FONT_BOLD, &stTexteLocation,
            "                           DOOM System Startup v1.4                            ");
        }

        version13 = true;
    }
    else if (fsize == 12408292   || // DOOM REGISTERED v1.9 (THE ULTIMATE DOOM)
             fsize == 12538385   || // DOOM REGISTERED (XBOX EDITION)
             fsize == 12487824   || // DOOM REGISTERED (BFG-PC EDITION)
             fsize == 12474561      // DOOM REGISTERED (BFG-XBOX360 EDITION)
//                                 ||
//             fsize == 19362644      // FREEDOOM v0.8 PHASE 1
            )
    {
        if (!beta_style)
        {
            printStyledText(1, 1, CONSOLE_FONT_WHITE, CONSOLE_FONT_RED,
            CONSOLE_FONT_BOLD, &stTexteLocation,
            "                           DOOM System Startup v1.9                            ");
        }

        version13 = true;
    }
    else if (fsize == 14943400   ||  // DOOM 2 REGISTERED v1.666
             fsize == 14824716   ||  // DOOM 2 REGISTERED v1.666 (GERMAN VERSION)
             fsize == 14612688   ||  // DOOM 2 REGISTERED v1.7
             fsize == 14607420)      // DOOM 2 REGISTERED v1.8 (FRENCH VERSION)
    {
        if (!beta_style)
        {
            printStyledText(1, 1, CONSOLE_FONT_RED, CONSOLE_FONT_WHITE,
            CONSOLE_FONT_BOLD, &stTexteLocation,
            "                         DOOM 2: Hell on Earth v1.666                          ");
        }

        version13 = true;
    }
    else if (fsize == 14604584   ||  // DOOM 2 REGISTERED v1.9
             fsize == 14677988   ||  // DOOM 2 REGISTERED (BFG-PSN EDITION)
             fsize == 14691821   ||  // DOOM 2 REGISTERED (BFG-PC EDITION)
             fsize == 14683458   ||  // DOOM 2 REGISTERED (XBOX EDITION)

//             fsize == 9745831    ||  // HACX SHAREWARE v1.0
//             fsize == 21951805   ||  // HACX REGISTERED v1.0
//             fsize == 22102300   ||  // HACX REGISTERED v1.1

             fsize == 19321722   ||  // HACX REGISTERED v1.2

//             fsize == 19801320   ||  // FREEDOOM v0.6.4
//             fsize == 27704188   ||  // FREEDOOM v0.7 RC 1
//             fsize == 27625596   ||  // FREEDOOM v0.7
//             fsize == 28144744   ||  // FREEDOOM v0.8 BETA 1
//             fsize == 28592816   ||  // FREEDOOM v0.8

             fsize == 28422764)      // FREEDOOM v0.8 PHASE 2
    {
        if (!beta_style)
        {
            if (
//                fsize == 9745831    ||  // HACX SHAREWARE v1.0
//                fsize == 21951805   ||  // HACX REGISTERED v1.0
//                fsize == 22102300   ||  // HACX REGISTERED v1.1

                fsize == 19321722)      // HACX REGISTERED v1.2
            {
                printStyledText(1, 1, CONSOLE_FONT_WHITE, CONSOLE_FONT_RED,
                                CONSOLE_FONT_BOLD, &stTexteLocation,
                "                          HACX:  Twitch n' Kill v1.2                           ");
            }
            else
            {
                printStyledText(1, 1, CONSOLE_FONT_WHITE, CONSOLE_FONT_RED,
                                CONSOLE_FONT_BOLD, &stTexteLocation,
                "                          DOOM 2: Hell on Earth v1.9                           ");
            }
        }

        version13 = true;
    }
    else if (fsize == 18195736   ||  // FINAL DOOM - TNT v1.9 (WITH YELLOW KEYCARD BUG)
             fsize == 18654796   ||  // FINAL DOOM - TNT v1.9 (WITHOUT YELLOW KEYCARD BUG)
             fsize == 12361532)      // CHEX QUEST
    {
        if (!beta_style)
        {
            if (fsize == 12361532)
            {
                printStyledText(1, 1, CONSOLE_FONT_WHITE, CONSOLE_FONT_BLACK,
                                CONSOLE_FONT_BOLD, &stTexteLocation,
                "                            Chex (R) Quest Startup                             ");
            }
            else
            {
                printStyledText(1, 1, CONSOLE_FONT_WHITE, CONSOLE_FONT_BLACK,
                                CONSOLE_FONT_BOLD, &stTexteLocation,
                "                          DOOM 2: TNT Evilution v1.9                           ");
            }
        }

        version13 = true;
    }

    else if (fsize == 18240172   ||  // FINAL DOOM - PLUTONIA v1.9 (WITH DEATHMATCH STARTS)
             fsize == 17420824)      // FINAL DOOM - PLUTONIA v1.9 (WITHOUT DEATHMATCH STARTS)
    {
        if (!beta_style)
        {
            printStyledText(1, 1, CONSOLE_FONT_WHITE, CONSOLE_FONT_BLACK,
                            CONSOLE_FONT_BOLD, &stTexteLocation,
            "                       DOOM 2: Plutonia Experiment v1.9                        ");
        }

        version13 = true;
    }

    if (!beta_style)
    {
        if (devparm || devparm_net)
        {
            printf(D_DEVSTR);
            printf("\n");
        }

        printf(" V_Init: allocate screens.\n");
        printf(" M_LoadDefaults: Load system defaults.\n");
        printf(" Z_Init: Init zone memory allocation daemon. \n");
        printf(" heap size: 0x3cdb000 \n");
        printf(" W_Init: Init WADfiles.\n");
    }
#endif

    if (fsize == 28422764 || fsize == 19321722 || fsize == 12361532)
    {
        beta_style = false;
    }

    // Save configuration at exit.
    if (!beta_style)
    {
        I_AtExit(M_SaveDefaults, false);
    }

    modifiedgame = false;

    P_InitExtraMobjs(); 

    D_ProcessDehCommandLine();

#ifndef WII
    setbuf(stdout, NULL);

    // Find main IWAD file and load it.
    iwadfile = D_FindIWAD(IWAD_MASK_DOOM, &gamemission);

    if (devparm)
    {
        if (M_CheckParm("-devparm_doom"))
        {
            iwadfile = "/home/user/WIIDOOM/src/IWAD/DOOM/Reg/v12/DOOM.WAD";
        }
        else if (M_CheckParm("-devparm_doom2"))
        {
            iwadfile = "/home/user/WIIDOOM/src/IWAD/DOOM2/v1666/DOOM2.WAD";
        }
        else if (M_CheckParm("-devparm_chex"))
        {
            iwadfile = "/home/user/WIIDOOM/src/IWAD/CHEX/CHEX.WAD";
        }
        else if (M_CheckParm("-devparm_hacx"))
        {
            iwadfile = "/home/user/WIIDOOM/src/IWAD/HACX/v12/HACX.WAD";
        }
        else if (M_CheckParm("-devparm_freedoom2"))
        {
            iwadfile = "/home/user/WIIDOOM/src/IWAD/FREEDOOM/v08p2/FREEDOOM2.WAD";
        }
        else if (M_CheckParm("-devparm_tnt"))
        {
            iwadfile = "/home/user/WIIDOOM/src/IWAD/TNT/v19_NEW/TNT.WAD";
        }
        else if (M_CheckParm("-devparm_plutonia"))
        {
            iwadfile = "/home/user/WIIDOOM/src/IWAD/PLUTONIA/v19_NEW/PLUTONIA.WAD";
        }
    }

    // None found?
    if (iwadfile == NULL)
    {
        I_Error("\n Game mode indeterminate.  No IWAD file was found.  Try\n"
                " specifying one with the '-iwad' command line parameter.\n");
    }

    iwad = fopen(iwadfile, "r");

    if (iwad)
    {
        // file pointer at the end of file
        fseek(iwad, 0, 2);

        // take a position of file pointer un size variable
        fsize = ftell(iwad);

        fclose(iwad);
    }

    W_CheckCorrectIWAD(doom);

    if (runcount < 2)
    {
        C_Output("~Wii-DOOM~ has been run %s", (!runcount ? "once" : "twice"));
    }
    else
    {
        C_Output("~Wii-DOOM~ has been run %s times", commify(runcount + 1));
    }

    D_AddFile(iwadfile, true);
#endif

    if (fsize == 4207819         ||  // DOOM SHAREWARE v1.0
        fsize == 4274218         ||  // DOOM SHAREWARE v1.1
        fsize == 4225504         ||  // DOOM SHAREWARE v1.2
        fsize == 4225460         ||  // DOOM SHAREWARE v1.25 (SYBEX RELEASE)
        fsize == 4234124         ||  // DOOM SHAREWARE v1.666
        fsize == 4196020         ||  // DOOM SHAREWARE v1.8
        fsize == 4261144         ||  // DOOM BETA v1.4
        fsize == 4271324         ||  // DOOM BETA v1.5
        fsize == 4211660)            // DOOM BETA v1.6
    {
        gamemode = shareware;
        gamemission = doom;
        gameversion = exe_doom_1_9;
        nerve_pwad = false;
        master_pwad = false;
    }
    else if (fsize == 10396254   ||  // DOOM REGISTERED v1.1
             fsize == 10399316   ||  // DOOM REGISTERED v1.2
             fsize == 10401760   ||  // DOOM REGISTERED v1.6
             fsize == 11159840)      // DOOM REGISTERED v1.8
    {
        gamemode = registered;
        gamemission = doom;
        gameversion = exe_doom_1_9;
        nerve_pwad = false;
        master_pwad = false;
    }
    else if (fsize == 12408292   ||  // DOOM REGISTERED v1.9 (THE ULTIMATE DOOM)
             fsize == 12538385   ||  // DOOM REGISTERED (XBOX EDITION)
             fsize == 12487824   ||  // DOOM REGISTERED (BFG-PC EDITION)
             fsize == 12474561       // DOOM REGISTERED (BFG-XBOX360 EDITION)
/*
                                 ||
             fsize == 19362644
*/
                              )      // FREEDOOM v0.8 PHASE 1
    {
        gamemode = retail;
        gamemission = doom;
        gameversion = exe_ultimate;
        nerve_pwad = false;
        master_pwad = false;
    }
    else if (fsize == 14943400   ||  // DOOM 2 REGISTERED v1.666
             fsize == 14824716   ||  // DOOM 2 REGISTERED v1.666 (GERMAN VERSION)
             fsize == 14612688   ||  // DOOM 2 REGISTERED v1.7
             fsize == 14607420   ||  // DOOM 2 REGISTERED v1.8 (FRENCH VERSION)
             fsize == 14604584   ||  // DOOM 2 REGISTERED v1.9
             fsize == 14677988   ||  // DOOM 2 REGISTERED (BFG-PSN EDITION)
             fsize == 14691821   ||  // DOOM 2 REGISTERED (BFG-PC EDITION)
             fsize == 14683458   ||  // DOOM 2 REGISTERED (XBOX EDITION)
/*
             fsize == 19801320   ||  // FREEDOOM v0.6.4
             fsize == 27704188   ||  // FREEDOOM v0.7 RC 1
             fsize == 27625596   ||  // FREEDOOM v0.7
             fsize == 28144744   ||  // FREEDOOM v0.8 BETA 1
             fsize == 28592816   ||  // FREEDOOM v0.8
*/
             fsize == 28422764)      // FREEDOOM v0.8 PHASE 2
    {
        gamemode = commercial;
        gamemission = doom2;
        gameversion = exe_doom_1_9;
    }
    else if (fsize == 18195736   ||  // FINAL DOOM - TNT v1.9 (WITH YELLOW KEYCARD BUG)
             fsize == 18654796)      // FINAL DOOM - TNT v1.9 (WITHOUT YELLOW KEYCARD BUG)
    {
        gamemode = commercial;
        gamemission = pack_tnt;
        gameversion = exe_final;
        nerve_pwad = false;
        master_pwad = false;
    }
    else if (fsize == 18240172   ||  // FINAL DOOM - PLUTONIA v1.9 (WITH DEATHMATCH STARTS)
             fsize == 17420824)      // FINAL DOOM - PLUTONIA v1.9 (WITHOUT DEATHMATCH STARTS)
    {
        gamemode = commercial;
        gamemission = pack_plut;
        gameversion = exe_final;
        nerve_pwad = false;
        master_pwad = false;
    }
    else if (fsize == 12361532)      // CHEX QUEST
    {
        gamemode = shareware;
        gamemission = pack_chex;
        gameversion = exe_chex;
        nerve_pwad = false;
        master_pwad = false;

        if (d_ejectcasings)
        {
            d_ejectcasings = false;
        }

        LoadChexDeh();
    }
    else if (
/*
             fsize == 9745831    ||  // HACX SHAREWARE v1.0
             fsize == 21951805   ||  // HACX REGISTERED v1.0
             fsize == 22102300   ||  // HACX REGISTERED v1.1
*/
             fsize == 19321722)      // HACX REGISTERED v1.2
    {
        gamemode = commercial;
        gamemission = pack_hacx;
        gameversion = exe_hacx;
        nerve_pwad = false;
        master_pwad = false;
    }

#ifndef WII
    //! 
    // @arg <version>
    // @category compat
    //
    // Emulate a specific version of Doom.  Valid values are "1.9",
    // "ultimate", "final", "final2", "hacx" and "chex".
    //
/*
    if (!beta_style)
    {
        p = M_CheckParmWithArgs("-gameversion", 1);
    }

    if (p)
    {
        for (i = 0; gameversions[i].description != NULL; ++i)
        {
            if (!strcmp(myargv[p + 1], gameversions[i].cmdline))
            {
                gameversion = gameversions[i].version;
                break;
            }
        }
        
        if (gameversions[i].description == NULL) 
        {
            printf("Supported game versions:\n");

            for (i = 0; gameversions[i].description != NULL; ++i)
            {
                printf("\t%s (%s)\n", gameversions[i].cmdline,
                        gameversions[i].description);
            }
            
            I_Error("Unknown game version '%s'", myargv[p + 1]);
        }
    }
    else
*/
    {
        // Determine automatically
        if (gamemission == pack_chex)
        {
            // chex.exe - identified by iwad filename
            gameversion = exe_chex;
        }
        else if (gamemission == pack_hacx)
        {
            // hacx.exe: identified by iwad filename
            gameversion = exe_hacx;
        }
        else if (gamemode == shareware || gamemode == registered
              || (gamemode == commercial && gamemission == doom2))
        {
            char demolumpname[9];
            int i;

            // original
            gameversion = exe_doom_1_9;

            // Detect version from demo lump
            for (i = 1; i <= 3; ++i)
            {
                M_snprintf(demolumpname, 6, "demo%i", i);

                if (W_CheckNumForName(demolumpname) > 0)
                {
                    byte *demolump = W_CacheLumpName(demolumpname, PU_STATIC);
                    int  demoversion = demolump[0];

                    dboolean status = true;

                    W_ReleaseLumpName(demolumpname);

                    switch (demoversion)
                    {
                        case 106:
                            gameversion = exe_doom_1_666;
                            break;

                        case 107:
                            gameversion = exe_doom_1_7;
                            break;

                        case 108:
                            gameversion = exe_doom_1_8;
                            break;

                        case 109:
                            gameversion = exe_doom_1_9;
                            break;

                        default:
                            status = false;
                            break;
                    }

                    if (status)
                    {
                        break;
                    }
                }
            }
        }
        else if (gamemode == retail)
        {
            gameversion = exe_ultimate;
        }
        else if (gamemode == commercial)
        {
            // Final Doom: tnt or plutonia
            // Defaults to emulating the first Final Doom executable,
            // which has the crash in the demo loop; however, having
            // this as the default should mean that it plays back
            // most demos correctly.
            gameversion = exe_final;
        }
    }

    if (fsize != 4261144  &&  // DOOM BETA v1.4
        fsize != 4271324  &&  // DOOM BETA v1.5
        fsize != 4211660  &&  // DOOM BETA v1.6
        fsize != 10396254 &&  // DOOM REGISTERED v1.1
        fsize != 10399316 &&  // DOOM REGISTERED v1.2
        fsize != 10401760 &&  // DOOM REGISTERED v1.6
        fsize != 11159840 &&  // DOOM REGISTERED v1.8
        fsize != 12408292 &&  // DOOM REGISTERED v1.9 (THE ULTIMATE DOOM)
        fsize != 12474561 &&  // DOOM REGISTERED (BFG-XBOX360 EDITION)
        fsize != 12487824 &&  // DOOM REGISTERED (BFG-PC EDITION)
        fsize != 12538385 &&  // DOOM REGISTERED (XBOX EDITION)
        fsize != 4207819  &&  // DOOM SHAREWARE v1.0
        fsize != 4274218  &&  // DOOM SHAREWARE v1.1
        fsize != 4225504  &&  // DOOM SHAREWARE v1.2
        fsize != 4225460  &&  // DOOM SHAREWARE v1.25 (SYBEX RELEASE)
        fsize != 4234124  &&  // DOOM SHAREWARE v1.666
        fsize != 4196020  &&  // DOOM SHAREWARE v1.8
        fsize != 14943400 &&  // DOOM 2 REGISTERED v1.666
        fsize != 14824716 &&  // DOOM 2 REGISTERED v1.666 (GERMAN VERSION)
        fsize != 14612688 &&  // DOOM 2 REGISTERED v1.7
        fsize != 14607420 &&  // DOOM 2 REGISTERED v1.8 (FRENCH VERSION)
        fsize != 14604584 &&  // DOOM 2 REGISTERED v1.9
        fsize != 14677988 &&  // DOOM 2 REGISTERED (BFG-PSN EDITION)
        fsize != 14691821 &&  // DOOM 2 REGISTERED (BFG-PC EDITION)
        fsize != 14683458 &&  // DOOM 2 REGISTERED (XBOX EDITION)
        fsize != 18195736 &&  // FINAL DOOM - TNT v1.9 (WITH YELLOW KEYCARD BUG)
        fsize != 18654796 &&  // FINAL DOOM - TNT v1.9 (WITHOUT YELLOW KEYCARD BUG)
        fsize != 18240172 &&  // FINAL DOOM - PLUTONIA v1.9 (WITH DEATHMATCH STARTS)
        fsize != 17420824 &&  // FINAL DOOM - PLUTONIA v1.9 (WITHOUT DEATHMATCH STARTS)

//        fsize != 19801320 &&  // FREEDOOM v0.6.4
//        fsize != 27704188 &&  // FREEDOOM v0.7 RC 1
//        fsize != 27625596 &&  // FREEDOOM v0.7
//        fsize != 28144744 &&  // FREEDOOM v0.8 BETA 1
//        fsize != 28592816 &&  // FREEDOOM v0.8
//        fsize != 19362644 &&  // FREEDOOM v0.8 PHASE 1

        fsize != 28422764 &&  // FREEDOOM v0.8 PHASE 2
        fsize != 12361532 &&  // CHEX QUEST

//        fsize != 9745831  &&  // HACX SHAREWARE v1.0
//        fsize != 21951805 &&  // HACX REGISTERED v1.0
//        fsize != 22102300 &&  // HACX REGISTERED v1.1

        fsize != 19321722)    // HACX REGISTERED v1.2
    {
        printf("\n\n\n\n\n");
        printf(" ===============================================================================");
        printf("\n");
        printf("            WARNING: DOOM / DOOM 2 / TNT / PLUTONIA IWAD FILE MISSING,          ");
        printf("\n");
        printf("                         NOT SELECTED OR WRONG IWAD !!!                         \n");
        printf("                                                                                \n");
        printf("                                QUITTING NOW ...                                ");
        printf("\n");
        printf(" ===============================================================================");
        printf("\n");

        sleep(5);

        I_QuitSerialFail();
    }
    else if (fsize == 10396254   || // DOOM REGISTERED v1.1
             fsize == 10399316   || // DOOM REGISTERED v1.2
             fsize == 4207819    || // DOOM SHAREWARE v1.0
             fsize == 4274218    || // DOOM SHAREWARE v1.1
             fsize == 4225504    || // DOOM SHAREWARE v1.2
             fsize == 4225460)      // DOOM SHAREWARE v1.25 (SYBEX RELEASE)
    {
        if (!beta_style)
        {
            printf("%s%s;%s                          DOOM Operating System v1.2                           %s%sm\n",
                COLORIZE_CMD, BACKGROUND_BLUE, TEXT_YELLOW, COLORIZE_CMD, COLORIZE_NORMAL);
        }
    }
    else if (fsize == 4261144    || // DOOM BETA v1.4
             fsize == 4271324    || // DOOM BETA v1.5
             fsize == 4211660    || // DOOM BETA v1.6
             fsize == 10401760   || // DOOM REGISTERED v1.6
             fsize == 11159840   || // DOOM REGISTERED v1.8
             fsize == 4234124    || // DOOM SHAREWARE v1.666
             fsize == 4196020)      // DOOM SHAREWARE v1.8
    {
        if (!beta_style)
        {
            printf("%s%s;%s                           DOOM System Startup v1.4                            %s%sm\n",
                COLORIZE_CMD, BACKGROUND_RED, TEXT_WHITE, COLORIZE_CMD, COLORIZE_NORMAL);
        }

        version13 = true;
    }
    else if (fsize == 12408292   || // DOOM REGISTERED v1.9 (THE ULTIMATE DOOM)
             fsize == 12538385   || // DOOM REGISTERED (XBOX EDITION)
             fsize == 12487824   || // DOOM REGISTERED (BFG-PC EDITION)
             fsize == 12474561      // DOOM REGISTERED (BFG-XBOX360 EDITION)

//                                ||
//             fsize == 19362644      // FREEDOOM v0.8 PHASE 1

             )
    {
        if (!beta_style)
        {
            printf("%s%s;%s                           DOOM System Startup v1.9                            %s%sm\n",
                COLORIZE_CMD, BACKGROUND_WHITE, TEXT_RED, COLORIZE_CMD, COLORIZE_NORMAL);
        }

        version13 = true;
    }
    else if (fsize == 14943400   ||  // DOOM 2 REGISTERED v1.666
             fsize == 14824716   ||  // DOOM 2 REGISTERED v1.666 (GERMAN VERSION)
             fsize == 14612688   ||  // DOOM 2 REGISTERED v1.7
             fsize == 14607420)      // DOOM 2 REGISTERED v1.8 (FRENCH VERSION)
    {
        if (!beta_style)
        {
            printf("%s%s;%s                         DOOM 2: Hell on Earth v1.666                          %s%sm\n",
                COLORIZE_CMD, BACKGROUND_RED, TEXT_WHITE, COLORIZE_CMD, COLORIZE_NORMAL);
        }

        version13 = true;
    }
    else if (fsize == 14604584   ||  // DOOM 2 REGISTERED v1.9
             fsize == 14677988   ||  // DOOM 2 REGISTERED (BFG-PSN EDITION)
             fsize == 14691821   ||  // DOOM 2 REGISTERED (BFG-PC EDITION)
             fsize == 14683458   ||  // DOOM 2 REGISTERED (XBOX EDITION)

//             fsize == 9745831    ||  // HACX SHAREWARE v1.0
//             fsize == 21951805   ||  // HACX REGISTERED v1.0
//             fsize == 22102300   ||  // HACX REGISTERED v1.1

             fsize == 19321722   ||  // HACX REGISTERED v1.2

//             fsize == 19801320   ||  // FREEDOOM v0.6.4
//             fsize == 27704188   ||  // FREEDOOM v0.7 RC 1
//             fsize == 27625596   ||  // FREEDOOM v0.7
//             fsize == 28144744   ||  // FREEDOOM v0.8 BETA 1
//             fsize == 28592816   ||  // FREEDOOM v0.8

             fsize == 28422764)      // FREEDOOM v0.8 PHASE 2
    {
        if (

//             fsize == 9745831    ||  // HACX SHAREWARE v1.0
//             fsize == 21951805   ||  // HACX REGISTERED v1.0
//             fsize == 22102300   ||  // HACX REGISTERED v1.1

             fsize == 19321722)      // HACX REGISTERED v1.2
        {
            if (!beta_style)
            {
                printf("%s%s;%s                          HACX:  Twitch n' Kill v1.2                           %s%sm\n",
                    COLORIZE_CMD, BACKGROUND_WHITE, TEXT_RED, COLORIZE_CMD, COLORIZE_NORMAL);
            }
        }
        else
        {
            if (!beta_style)
            {
                printf("%s%s;%s                          DOOM 2: Hell on Earth v1.9                           %s%sm\n",
                    COLORIZE_CMD, BACKGROUND_WHITE, TEXT_RED, COLORIZE_CMD, COLORIZE_NORMAL);
            }
        }

        version13 = true;
    }
    else if (fsize == 18195736   ||  // FINAL DOOM - TNT v1.9 (WITH YELLOW KEYCARD BUG)
             fsize == 18654796   ||  // FINAL DOOM - TNT v1.9 (WITHOUT YELLOW KEYCARD BUG)
             fsize == 12361532)      // CHEX QUEST
    {
        if (fsize == 12361532)
        {
            if (!beta_style)
            {
                printf("%s%s;%s                            Chex (R) Quest Startup                             %s%sm\n",
                    COLORIZE_CMD, BACKGROUND_WHITE, TEXT_BLACK, COLORIZE_CMD, COLORIZE_NORMAL);
            }
        }
        else
        {
            if (!beta_style)
            {
                printf("%s%s;%s                          DOOM 2: TNT Evilution v1.9                           %s%sm\n",
                    COLORIZE_CMD, BACKGROUND_WHITE, TEXT_BLACK, COLORIZE_CMD, COLORIZE_NORMAL);
            }
        }

        version13 = true;
    }

    else if (fsize == 18240172   ||  // FINAL DOOM - PLUTONIA v1.9 (WITH DEATHMATCH STARTS)
             fsize == 17420824)      // FINAL DOOM - PLUTONIA v1.9 (WITHOUT DEATHMATCH STARTS)
    {
        if (!beta_style)
        {
            printf("%s%s;%s                       DOOM 2: Plutonia Experiment v1.9                        %s%sm\n",
                COLORIZE_CMD, BACKGROUND_WHITE, TEXT_BLACK, COLORIZE_CMD, COLORIZE_NORMAL);
        }

        version13 = true;
    }

    if (!beta_style)
    {
        if (devparm || devparm_net)
        {
            printf(D_DEVSTR);
            printf("\n");
        }

        printf(" V_Init: allocate screens.\n");
        printf(" M_LoadDefaults: Load system defaults.\n");
        printf(" Z_Init: Init zone memory allocation daemon. \n");
        printf(" heap size: 0x3cdb000 \n");
        printf(" W_Init: Init WADfiles.\n");
    }

    correct_lost_soul_bounce = gameversion >= exe_ultimate;

    // The original exe does not support retail - 4th episode not supported
    if (gameversion < exe_ultimate && gamemode == retail)
    {
        gamemode = registered;
    }

    if (gamemode != commercial && fsize != 4234124 && fsize != 4196020 &&
            fsize != 12474561 && fsize != 12487824 && fsize != 11159840 &&
            fsize != 12408292 && fsize != 12538385 && fsize != 12361532 &&
            fsize != 7585664)
    {
        icontype = false;
    }

    I_EnableLoadingDisk(SCREENWIDTH - LOADING_DISK_W, SCREENHEIGHT - LOADING_DISK_H);

    // EXEs prior to the Final Doom exes do not support Final Doom.
    if (gameversion < exe_final && gamemode == commercial
        && (gamemission == pack_tnt || gamemission == pack_plut))
    {
        gamemission = doom2;
    }

/*
    if (gamemode == commercial)
    {
        int p = 0;

        // We can manually override the gamemission that we got from the
        // IWAD detection code. This allows us to eg. play Plutonia 2
        // with Freedoom and get the right level names.

        //!
        // @arg <pack>
        //
        // Explicitly specify a Doom II "mission pack" to run as, instead of
        // detecting it based on the filename. Valid values are: "doom2",
        // "tnt" and "plutonia".
        //
        if (!beta_style)
        {
            p = M_CheckParmWithArgs("-pack", 1);
        }

        if (p > 0)
        {
            SetMissionForPackName(myargv[p + 1]);
        }
    }
*/

#else

    if (devparm || devparm_net)
    {
        if (usb)
        {
            if (devparm_doom)
            {
                D_AddFile("usb:/apps/wiidoom/IWAD/DOOM/Reg/v12/DOOM.WAD", true);
            }
            else if (devparm_doom2)
            {
                D_AddFile("usb:/apps/wiidoom/IWAD/DOOM2/v1666/DOOM2.WAD", true);
            }
            else if (devparm_chex)
            {
                D_AddFile("usb:/apps/wiidoom/IWAD/CHEX/CHEX.WAD", true);
            }
            else if (devparm_hacx)
            {
                D_AddFile("usb:/apps/wiidoom/IWAD/HACX/v12/HACX.WAD", true);
            }
            else if (devparm_freedoom2)
            {
                D_AddFile("usb:/apps/wiidoom/IWAD/FREEDOOM/v08p2/FREEDOOM2.WAD", true);
            }
            else if (devparm_tnt)
            {
                D_AddFile("usb:/apps/wiidoom/IWAD/TNT/v19_NEW/TNT.WAD", true);
            }
            else if (devparm_plutonia)
            {
                D_AddFile("usb:/apps/wiidoom/IWAD/PLUTONIA/v19_NEW/PLUTONIA.WAD", true);
            }
        }
        else if (sd)
        {
            if (devparm_doom)
            {
                D_AddFile("sd:/apps/wiidoom/IWAD/DOOM/Reg/v12/DOOM.WAD", true);
            }
            else if (devparm_doom2)
            {
                D_AddFile("sd:/apps/wiidoom/IWAD/DOOM2/v1666/DOOM2.WAD", true);
            }
            else if (devparm_chex)
            {
                D_AddFile("sd:/apps/wiidoom/IWAD/CHEX/CHEX.WAD", true);
            }
            else if (devparm_hacx)
            {
                D_AddFile("sd:/apps/wiidoom/IWAD/HACX/v12/HACX.WAD", true);
            }
            else if (devparm_freedoom2)
            {
                D_AddFile("sd:/apps/wiidoom/IWAD/FREEDOOM/v08p2/FREEDOOM2.WAD", true);
            }
            else if (devparm_tnt)
            {
                D_AddFile("sd:/apps/wiidoom/IWAD/TNT/v19_NEW/TNT.WAD", true);
            }
            else if (devparm_plutonia)
            {
                D_AddFile("sd:/apps/wiidoom/IWAD/PLUTONIA/v19_NEW/PLUTONIA.WAD", true);
            }
        }
    }
    else
    {
        D_AddFile(target, false);
    }

    if (gamemode != shareware || gameversion == exe_chex)
    {
        if (load_extra_wad)
        {
            if (extra_wad_slot_1_loaded)
            {
                W_MergeFile(extra_wad_1, false);
            }

            if (!nerve_pwad && !master_pwad)
            {
                if (extra_wad_slot_2_loaded)
                {
                    W_MergeFile(extra_wad_2, false);
                }

                if (extra_wad_slot_3_loaded)
                {
                    W_MergeFile(extra_wad_3, false);
                }
            }

            modifiedgame = true;
        }
    }

    if (devparm_nerve)
    {
        D_AddFile("usb:/apps/wiidoom/PWAD/DOOM2/NERVE.WAD", true);
        nerve_pwad = true;
    }
    else if (devparm_master)
    {
        D_AddFile("usb:/apps/wiidoom/PWAD/DOOM2/MASTER.WAD", true);
        master_pwad = true;
    }
#endif

    if (W_CheckNumForName("dmenupic") >= 0)
    {
        bfgedition = true;

        // FIXME
        // BFG Edition changes the names of the secret levels to
        // censor the Wolfenstein references. It also has an extra
        // secret level (MAP33). In Vanilla Doom (meaning the DOS
        // version), MAP33 overflows into the Plutonia level names
        // array, so HUSTR_33 is actually PHUSTR_1.
/*
        DEH_AddStringReplacement(HUSTR_31, "level 31: idkfa");
        DEH_AddStringReplacement(HUSTR_32, "level 32: keen");
        DEH_AddStringReplacement(PHUSTR_1, "level 33: betray");

        // The BFG edition doesn't have the "low detail" menu option (fair
        // enough). But bizarrely, it reuses the M_GDHIGH patch as a label
        // for the options menu (says "Fullscreen:"). Why the perpetrators
        // couldn't just add a new graphic lump and had to reuse this one,
        // I don't know.
        //
        // The end result is that M_GDHIGH is too wide and causes the game
        // to crash. As a workaround to get a minimum level of support for
        // the BFG edition IWADs, use the "ON"/"OFF" graphics instead.
        DEH_AddStringReplacement("M_GDHIGH", "M_MSGON");
        DEH_AddStringReplacement("M_GDLOW", "M_MSGOFF");
*/
    }

    dont_show_adding_of_resource_wad = false;

#ifndef WII
    // Load PWAD files.
    // [crispy] OR'ed
    modifiedgame |= W_ParseCommandLine();
#endif

    if (beta_style /*&& gamemode != shareware && gamemode != commercial*/)
    {
        W_CheckSize(5);

        if (fsizerw2 == 653705)
        {
#ifdef WII
            if (usb)
            {
                W_MergeFile("usb:/apps/wiidoom/doom1extras.wad", true);
            }
            else if (sd)
            {
                W_MergeFile("sd:/apps/wiidoom/doom1extras.wad", true);
            }
#else
            W_MergeFile("doom1extras.wad", true);
#endif
        }
        else
        {
            print_resource_pwad2_error = true;
        }
    }

#ifdef WII
    if (usb)
    {
        W_MergeFile("usb:/apps/wiidoom/pspdoom.wad", true);
    }
    else if (sd)
    {
        W_MergeFile("sd:/apps/wiidoom/pspdoom.wad", true);
    }
#else
    W_MergeFile("pspdoom.wad", true);
#endif

    C_Init();

    if (print_resource_pwad2_error)
    {
        printf("\n\n\n\n\n");
        printf(" ===============================================================================");

#ifndef WII
        printf("\n");
#endif

        printf("                         !!! WRONG RESOURCE PWAD FILE !!!                       ");

#ifndef WII
        printf("\n");
#endif

        printf("                 PLEASE COPY THE FILE 'DOOM1EXTRAS.WAD' THAT CAME               ");

#ifndef WII
        printf("\n");
#endif

        printf("                    WITH THIS RELEASE, INTO THE GAME DIRECTORY                  \n");
        printf("                                                                                \n");
        printf("                                QUITTING NOW ...                                ");

#ifndef WII
        printf("\n");
#endif

        printf(" ===============================================================================");

#ifndef WII
        printf("\n");
#endif

        sleep(5);

        I_QuitSerialFail();
    }

    if (devparm)
    {
        C_Warning(s_D_DEVSTR);
    }

/*
    if (devparm)
    {
        W_PrintDirectory();
    }

#ifndef WII
    //!
    // @arg <demo>
    // @category demo
    // @vanilla
    //
    // Play back the demo named demo.lmp.
    //
    p = M_CheckParmWithArgs("-playdemo", 1);

    if (!p)
    {
        //!
        // @arg <demo>
        // @category demo
        // @vanilla
        //
        // Play back the demo named demo.lmp, determining the framerate
        // of the screen.
        //
        p = M_CheckParmWithArgs("-timedemo", 1);

    }

    if (p)
    {
        char *uc_filename = strdup(myargv[p + 1]);

        M_ForceUppercase(uc_filename);

        // With Vanilla you have to specify the file without extension,
        // but make that optional.
        if (M_StringEndsWith(uc_filename, ".LMP"))
        {
            M_StringCopy(file, myargv[p + 1], sizeof(file));
        }
        else
        {
            M_snprintf(file, sizeof(file), "%s.lmp", myargv[p + 1]);
        }

        free(uc_filename);

        if (D_AddFile(file, true))
        {
            M_StringCopy(demolumpname, lumpinfo[numlumps - 1]->name,
                         sizeof(demolumpname));
        }
        else
        {
            // If file failed to load, still continue trying to play
            // the demo in the same way as Vanilla Doom.  This makes
            // tricks like "-playdemo demo1" possible.
            M_StringCopy(demolumpname, myargv[p + 1], sizeof(demolumpname));
        }

        C_Warning("Playing demo %s.", file);
    }

#endif

    I_AtExit(G_CheckDemoStatusAtExit, true);
*/

    // Generate the WAD hash table.  Speed things up a bit.
    W_GenerateHashTable();

    D_SetGameDescription();
    savegamedir = M_GetSaveGameDir(D_SaveGameIWADName(gamemission));
    C_Output("Savegames will be saved and loaded in %s.", uppercase(savegamedir));
    D_ProcessDehInWad();

#ifndef WII
    // Check for -file in shareware
    if (modifiedgame)
    {
        if (gamemode == shareware)
        {
            I_Error("\nYou cannot -file with the shareware "
                               "version. Register!");
        }

        // Check for fake IWAD with right name,
        // but w/o all the lumps of the registered version. 
        if (gamemode == registered)
        {
            int i;

            for (i = 0; i < 23; i++)
            {
                // These are the lumps that will be checked in IWAD,
                // if any one is not present, execution will be aborted.
                char name[23][8]=
                {
                    "e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
                    "e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
                    "dphoof","bfgga0","heada1","cybra1","spida1d1"
                };

                if (W_CheckNumForName(name[i]) < 0)
                {
                    I_Error("\nThis is not the registered version.");
                }
            }
        }
    }

    if (W_CheckNumForName("SS_START") >= 0 || W_CheckNumForName("FF_END") >= 0)
    {
        C_Warning("WARNING: The loaded WAD file contains modified sprites or floor textures.");
        C_Warning("You may want to use the '-merge' command line option instead of '-file'.");
    }
#endif

    if (fsize == 12361532)
    {
        W_CheckSize(1);

        if (d_maxgore)
        {
            d_maxgore = false;
        }

        if (print_resource_pwad_error)
        {
            printf("\n\n\n\n\n");
            printf(" ===============================================================================");

#ifndef WII
            printf("\n");
#endif

            printf("                         !!! WRONG RESOURCE PWAD FILE !!!                       ");

#ifndef WII
            printf("\n");
#endif

            printf("                   PLEASE COPY THE FILE 'PSPCHEX.WAD' THAT CAME                 ");

#ifndef WII
            printf("\n");
#endif

            printf("                    WITH THIS RELEASE, INTO THE GAME DIRECTORY                  \n");
            printf("                                                                                \n");
            printf("                                QUITTING NOW ...                                ");

#ifndef WII
            printf("\n");
#endif

            printf(" ===============================================================================");

#ifndef WII
            printf("\n");
#endif

            sleep(5);

            I_QuitSerialFail();
        }
        else
        {
#ifdef WII
            if (usb)
            {
                D_AddFile("usb:/apps/wiidoom/pspchex.wad", true);
            }
            else if (sd)
            {
                D_AddFile("sd:/apps/wiidoom/pspchex.wad", true);
            }
#else
            D_AddFile("pspchex.wad", true);
#endif
        }
    }
    else if (fsize == 19321722 /*|| fsize == 9745831 || fsize == 21951805 || fsize == 22102300*/)
    {
        W_CheckSize(2);

        if (print_resource_pwad_error)
        {
            printf("\n\n\n\n\n");
            printf(" ===============================================================================");

#ifndef WII
            printf("\n");
#endif

            printf("                         !!! WRONG RESOURCE PWAD FILE !!!                       ");

#ifndef WII
            printf("\n");
#endif

            printf("                   PLEASE COPY THE FILE 'PSPHACX.WAD' THAT CAME                 ");

#ifndef WII
            printf("\n");
#endif

            printf("                    WITH THIS RELEASE, INTO THE GAME DIRECTORY                  \n");
            printf("                                                                                \n");
            printf("                                QUITTING NOW ...                                ");

#ifndef WII
            printf("\n");
#endif

            printf(" ===============================================================================");

#ifndef WII
            printf("\n");
#endif

            sleep(5);

            I_QuitSerialFail();
        }
        else
        {
#ifdef WII
            if (usb)
            {
                D_AddFile("usb:/apps/wiidoom/psphacx.wad", true);
            }
            else if (sd)
            {
                D_AddFile("sd:/apps/wiidoom/psphacx.wad", true);
            }
#else
            D_AddFile("psphacx.wad", true);
#endif
        }
    }
    else if (fsize == 28422764)
    {
        W_CheckSize(3);

        if (print_resource_pwad_error)
        {
            printf("\n\n\n\n\n");
            printf(" ===============================================================================");

#ifndef WII
            printf("\n");
#endif

            printf("                         !!! WRONG RESOURCE PWAD FILE !!!                       ");

#ifndef WII
            printf("\n");
#endif

            printf("                 PLEASE COPY THE FILE 'PSPFREEDOOM.WAD' THAT CAME               ");

#ifndef WII
            printf("\n");
#endif

            printf("                    WITH THIS RELEASE, INTO THE GAME DIRECTORY                  \n");
            printf("                                                                                \n");
            printf("                                QUITTING NOW ...                                ");

#ifndef WII
            printf("\n");
#endif

            printf(" ===============================================================================");

#ifndef WII
            printf("\n");
#endif

            sleep(5);

            I_QuitSerialFail();
        }
        else
        {
#ifdef WII
            if (usb)
            {
                D_AddFile("usb:/apps/wiidoom/pspfreedoom.wad", true);
            }
            else if (sd)
            {
                D_AddFile("sd:/apps/wiidoom/pspfreedoom.wad", true);
            }
#else
            D_AddFile("pspfreedoom.wad", true);
#endif
        }
    }

    C_Output("V_Init: allocate screens.");
    C_Output("M_LoadDefaults: Load system defaults.");
    C_Output("Z_Init: Init zone memory allocation daemon. ");
    C_Output("heap size: 0x3cdb000 ");
    C_Output("W_Init: Init WADfiles.");

    if (modifiedgame)
    {
#ifdef WII
        while (1)
        {
            uint32_t buttons;

            if (wad_message_has_been_shown)
            {
                goto skip_showing_message;
            }

            if (!beta_style)
            {
                printf(" ===============================================================================");
                printf("    ATTENTION:  This version of DOOM has been modified.  If you would like to   ");
                printf("   get a copy of the original game, call 1-800-IDGAMES or see the readme file.  ");
                printf("            You will not receive technical support for modified games.          ");
                printf("                             press enter to continue                            ");
                printf(" ===============================================================================");
            }
#endif
            C_Output("===============================================================================");
            C_Output("   ATTENTION:  This version of DOOM has been modified.  If you would like to   ");
            C_Output("  get a copy of the original game, call 1-800-IDGAMES or see the readme file.  ");
            C_Output("           You will not receive technical support for modified games.          ");
            C_Output("                            press enter to continue                            ");
            C_Output("===============================================================================");

#ifdef WII
            skip_showing_message: ;

            buttons = WaitButtons();

            if (buttons & WPAD_CLASSIC_BUTTON_A)
            {
                break;
            }

            wad_message_has_been_shown = true;
        }
#else
        if (!beta_style)
        {
            printf(" ===============================================================================\n"
                   "    ATTENTION:  This version of DOOM has been modified.  If you would like to   \n"
                   "   get a copy of the original game, call 1-800-IDGAMES or see the readme file.  \n"
                   "            You will not receive technical support for modified games.          \n"
                   "                             press enter to continue                            \n"
                   " ===============================================================================");

            getchar();
        }
#endif
    }

    if (gamemode == shareware && gameversion != exe_chex)
    {
        if (!beta_style)
        {
            printf("         shareware version.\n");
        }

        C_Output("        shareware version.");
    }
    else if ((gamemode == shareware && gameversion == exe_chex) || gamemode == registered)
    {
        if (!beta_style)
        {
            printf("         registered version.\n");
        }

        C_Output("        registered version.");
    }
    else
    {
        if (!beta_style)
        {
            printf("         commercial version.\n");
        }

        C_Output("        commercial version.");
    }

    if ((gamemode == retail || gamemode == registered) && !beta_style)
    {
        printf(" ===============================================================================");

#ifndef WII
        printf("\n");
#endif

        printf("                 This version is NOT SHAREWARE, do not distribute!              ");

#ifndef WII
        printf("\n");
#endif

        printf("             Please report software piracy to the SPA: 1-800-388-PIR8           ");

#ifndef WII
        printf("\n");
#endif

        printf(" ===============================================================================");

#ifndef WII
        printf("\n");
#endif

        C_Output("===============================================================================");
        C_Output("                This version is NOT SHAREWARE, do not distribute!              ");
        C_Output("            Please report software piracy to the SPA: 1-800-388-PIR8           ");
        C_Output("===============================================================================");
    }
    else if (gamemode == commercial && !beta_style)
    {
        printf(" ===============================================================================");

#ifndef WII
        printf("\n");
#endif

        printf("                                Do not distribute!                              ");

#ifndef WII
        printf("\n");
#endif

        printf("             Please report software piracy to the SPA: 1-800-388-PIR8           ");

#ifndef WII
        printf("\n");
#endif

        printf(" ===============================================================================");

#ifndef WII
        printf("\n");
#endif

        C_Output("===============================================================================");
        C_Output("                               Do not distribute!                              ");
        C_Output("            Please report software piracy to the SPA: 1-800-388-PIR8           ");
        C_Output("===============================================================================");
    }

    // Check for -file in shareware
    if (modifiedgame)
    {        
        if (gamemode == shareware && gameversion != exe_chex)
        {
            I_Error("\nYou cannot -file with the shareware "
                               "version. Register!");
        }

        // Check for fake IWAD with right name,
        // but w/o all the lumps of the registered version. 
        if (gamemode == registered)
        {
            int i;

            for (i = 0; i < 23; i++)
            {
                // These are the lumps that will be checked in IWAD,
                // if any one is not present, execution will be aborted.
                char name[23][8]=
                {
                    "e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
                    "e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
                    "dphoof","bfgga0","heada1","cybra1","spida1d1"
                };

                if (W_CheckNumForName(name[i]) < 0)
                {
                    I_Error("\nThis is not the registered version.");
                }
            }
        }
    }

    // disable any colored blood in Chex Quest,
    // disable modifying Stealth Buzzer and D-Man blood in Hacx
    d_chkblood = (gamemission != pack_chex);
    d_chkblood2 = (gamemission != pack_chex && gamemission != pack_hacx);

    // We need to init the timer right in this place or else
    // running network from multiplayer setup won't work at all
    I_InitTimer();

#ifdef FEATURE_MULTIPLAYER
    C_Network("NET_Init: Init network subsystem.");
    NET_Init();
#endif

    // Initial netgame startup. Connect to server etc.
    D_ConnectNetGame();

    start_respawnparm = respawnparm;
    start_fastparm = fastparm;

    // get skill / episode / map from parms
    startskill = sk_medium;
    startepisode = 1;
    startmap = 1;

/*
    if (devparm || devparm_net)
    {
        autostart = true;
    }
    else
*/
    {
        autostart = false;
    }

    //!
    // @arg <skill>
    // @vanilla
    //
    // Set the game skill, 1-5 (1: easiest, 5: hardest).  A skill of
    // 0 disables all monsters.
    //

#ifndef WII
    p = M_CheckParmWithArgs("-skill", 1);

    if (p)
    {
        startskill = myargv[p + 1][0] - '1';
        autostart = true;
    }

    //!
    // @arg <n>
    // @vanilla
    //
    // Start playing on episode n (1-4)
    //
    if (!beta_style)
    {
        p = M_CheckParmWithArgs("-episode", 1);
    }

    if (p)
    {
        startepisode = myargv[p + 1][0] - '0';
        startmap = 1;
        autostart = true;
    }
#endif

    timelimit = 0;

    //! 
    // @arg <n>
    // @category net
    // @vanilla
    //
    // For multiplayer games: exit each level after n minutes.
    //

#ifndef WII
    if (!beta_style)
    {
        p = M_CheckParmWithArgs("-timer", 1);
    }

    if (p)
    {
        timelimit = atoi(myargv[p + 1]);
    }

    //!
    // @category net
    // @vanilla
    //
    // Austin Virtual Gaming: end levels after 20 minutes.
    //
    if (!beta_style)
    {
        p = M_CheckParm("-avg");
    }

    if (p)
    {
        timelimit = 20;
    }

    //!
    // @arg [<x> <y> | <xy>]
    // @vanilla
    //
    // Start a game immediately, warping to ExMy (Doom 1) or MAPxy
    // (Doom 2)
    //
    if (!beta_style)
    {
        p = M_CheckParmWithArgs("-warp", 1);
    }

    if (p)
    {
        if (gamemode == commercial)
        {
            startmap = atoi (myargv[p + 1]);
        }
        else
        {
            startepisode = myargv[p + 1][0] - '0';

            if (p + 2 < myargc)
            {
                startmap = myargv[p + 2][0] - '0';
            }
            else
            {
                startmap = 1;
            }
        }

        autostart = true;
    }
#endif

    // Undocumented:
    // Invoked by setup to test the controls.
    // Not loading a game

    //!
    // @arg <s>
    // @vanilla
    //
    // Load the game in slot s.
    //

#ifndef WII
    if (!beta_style)
    {
        p = M_CheckParmWithArgs("-loadgame", 1);
    }

    if (p)
    {
        startloadgame = atoi(myargv[p + 1]);
    }
    else
#endif
    {
        // Not loading a game
        startloadgame = -1;
    }

    P_BloodSplatSpawner = (r_blood == r_blood_none || !r_bloodsplats_max ? P_NullBloodSplatSpawner : P_SpawnBloodSplat);

    if (!beta_style)
    {
        printf(" M_Init: Init miscellaneous info.\n");
    }

    C_Output("M_Init: Init miscellaneous info.");
    M_Init();

    if (gameversion == exe_chex)
    {
        if (!beta_style)
        {
            printf(" R_Init: Init Chex(R) Quest refresh daemon - ");
        }

        C_Output("R_Init: Init Chex(R) Quest refresh daemon - ");
    }
    else
    {
        if (!beta_style)
        {
            printf(" R_Init: Init DOOM refresh daemon");

            if (fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 10396254 && fsize != 10399316)
            {
                printf(" - ");
            }
        }

        C_Output("R_Init: Init DOOM refresh daemon");
    }

    R_Init();

    if (!beta_style)
    {
        printf("\n P_Init: Init Playloop state.\n");
    }

    C_Output("P_Init: Init Playloop state.");
    P_Init();

    if (!beta_style)
    {
        printf(" I_Init: Setting up machine state.\n");
    }

    C_Output("I_Init: Setting up machine state.");

    if (!beta_style)
    {
        printf(" I_StartupDPMI\n");
    }

    C_Output("I_StartupDPMI");

    printf(" I_StartupMouse\n");

    if (beta_style)
    {
        printf(" Mouse: detected\n");
        printf(" Allocate DOS memory for CyberMan info\n");
        printf(" CyberMan: DOS block at 0172:0000\n");
        printf(" CyberMan: Wrong mouse driver - no SWIFT support (AX=53c1).\n");
        printf(" I_StartupComm\n");
    }

    C_Output("I_StartupMouse");

    if (!beta_style)
    {
        printf(" I_StartupJoystick\n");
    }

    C_Output("I_StartupJoystick");

    printf(" I_StartupKeyboard\n");
    C_Output("I_StartupKeyboard");

#ifndef WII
    I_CheckIsScreensaver();
#endif

    if (!beta_style)
    {
        printf(" I_StartupTimer\n");
    }

    C_Output("I_StartupTimer");

    startuptimer = I_StartupTimer();

    printf(" I_StartupSound\n");
    C_Output("I_StartupSound");

    if (beta_style)
    {
        printf(" sc[MD].dmxCode=0\n");
        printf(" sc[Sfx].dmxCode=0\n");
        printf(" DMX_Init() returned 1\n");
        printf(" I_StartupTimer\n");
    }

    if (!beta_style)
    {
        printf(" calling DMX_Init\n");
    }

    C_Output("calling DMX_Init");

    if (!beta_style)
    {
        printf(" D_CheckNetGame: Checking network game status.\n");
    }

#ifndef WII
    PrintGameVersion();
#endif

    if (!beta_style)
    {
        printf(" S_Init: Setting up sound.\n");
    }

    C_Output("S_Init: Setting up sound.");

    if (!beta_style)
    {
        S_Init(sfxVolume * 8, musicVolume * 8);

        printf(" HU_Init: Setting up heads up display.\n");
    }

    C_Network("D_CheckNetGame: Checking network game status.");
    D_CheckNetGame();

    C_Output("HU_Init: Setting up heads up display.");
    HU_Init();

    if (!beta_style)
    {
        printf(" ST_Init: Init status bar.\n");
    }

    C_Output("ST_Init: Init status bar.");
    ST_Init();

    // If Doom II without a MAP01 lump, this is a store demo.
    // Moved this here so that MAP01 isn't constantly looked up
    // in the main loop.
    if (gamemode == commercial && W_CheckNumForName("map01") < 0)
    {
        storedemo = true;
    }

/*
    if (M_CheckParmWithArgs("-statdump", 1) && !beta_style)
    {
        I_AtExit(StatDump, true);
        printf("External statistics registered.\n");
    }

#ifndef WII

    //!
    // @arg <x>
    // @category demo
    // @vanilla
    //
    // Record a demo named x.lmp.
    //
    p = M_CheckParmWithArgs("-record", 1);

    if (p)
    {
        G_RecordDemoCmd(myargv[p + 1]);
        autostart = true;
    }

    p = M_CheckParmWithArgs("-playdemo", 1);

    if (p)
    {
        // quit after one demo
        singledemo = true;

        G_DeferedPlayDemo(demolumpname);

        // never returns
        D_DoomLoop();
    }
        
    p = M_CheckParmWithArgs("-timedemo", 1);

    if (p)
    {
        G_TimeDemo(demolumpname);

        // never returns
        D_DoomLoop();
    }

#endif
*/

    // Doom 3: BFG Edition includes modified versions of the classic
    // IWADs which can be identified by an additional DMENUPIC lump.
    // Furthermore, the M_GDHIGH lumps have been modified in a way that
    // makes them incompatible to Vanilla Doom and the modified version
    // of doom2.wad is missing the TITLEPIC lump.
    // We specifically check for DMENUPIC here, before PWADs have been
    // loaded which could probably include a lump of that name.
    if (fsize == 4207819)
    {
        C_Output("Playing 'DOOM SHAREWARE v1.0'.");
    }
    else if (fsize == 4274218)
    {
        C_Output("Playing 'DOOM SHAREWARE v1.1'.");
    }
    else if (fsize == 4225504)
    {
        C_Output("Playing 'DOOM SHAREWARE v1.2'.");
    }
    else if (fsize == 4225460)
    {
        C_Output("Playing 'DOOM SHAREWARE v1.25 (SYBEX RELEASE)'.");
    }
    else if (fsize == 4234124)
    {
        C_Output("Playing 'DOOM SHAREWARE v1.666'.");
    }
    else if (fsize == 4196020)
    {
        C_Output("Playing 'DOOM SHAREWARE v1.8'.");
    }
    else if (fsize == 4261144)
    {
        C_Output("Playing 'DOOM BETA v1.4'.");
    }
    else if (fsize == 4271324)
    {
        C_Output("Playing 'DOOM BETA v1.5'.");
    }
    else if (fsize == 4211660)
    {
        C_Output("Playing 'DOOM BETA v1.6'.");
    }
    else if (fsize == 10396254)
    {
        C_Output("Playing 'DOOM REGISTERED v1.1'.");
    }
    else if (fsize == 10399316)
    {
        C_Output("Playing 'DOOM REGISTERED v1.2'.");
    }
    else if (fsize == 10401760)
    {
        C_Output("Playing 'DOOM REGISTERED v1.6'.");
    }
    else if (fsize == 11159840)
    {
        C_Output("Playing 'DOOM REGISTERED v1.8'.");
    }
    else if (fsize == 12408292)
    {
        C_Output("Playing 'DOOM REGISTERED v1.9 (THE ULTIMATE DOOM)'.");
    }
    else if (fsize == 12538385)
    {
        C_Output("Playing 'DOOM REGISTERED (XBOX EDITION)'.");
    }
    else if (fsize == 12487824)
    {
        C_Output("Playing 'DOOM REGISTERED (BFG-PC EDITION)'.");
    }
    else if (fsize == 12474561)
    {
        C_Output("Playing 'DOOM REGISTERED (BFG-XBOX360 EDITION)'.");
    }
    else if (fsize == 19362644)
    {
        C_Output("Playing 'FREEDOOM v0.8 PHASE 1'.");
    }
    else if (fsize == 14943400)
    {
        C_Output("Playing 'DOOM 2 REGISTERED v1.666'.");
    }
    else if (fsize == 14824716)
    {
        C_Output("Playing 'DOOM 2 REGISTERED v1.666 (GERMAN VERSION)'.");
    }
    else if (fsize == 14612688)
    {
        C_Output("Playing 'DOOM 2 REGISTERED v1.7'.");
    }
    else if (fsize == 14607420)
    {
        C_Output("Playing 'DOOM 2 REGISTERED v1.8 (FRENCH VERSION)'.");
    }
    else if (fsize == 14604584)
    {
        C_Output("Playing 'DOOM 2 REGISTERED v1.9'.");
    }
    else if (fsize == 14677988)
    {
        C_Output("Playing 'DOOM 2 REGISTERED (BFG-PSN EDITION)'.");
    }
    else if (fsize == 14691821)
    {
        C_Output("Playing 'DOOM 2 REGISTERED (BFG-PC EDITION)'.");
    }
    else if (fsize == 14683458)
    {
        C_Output("Playing 'DOOM 2 REGISTERED (XBOX EDITION)'.");
    }
    else if (fsize == 19801320)
    {
        C_Output("Playing 'FREEDOOM v0.6.4'.");
    }
    else if (fsize == 27704188)
    {
        C_Output("Playing 'FREEDOOM v0.7 RC 1'.");
    }
    else if (fsize == 27625596)
    {
        C_Output("Playing 'FREEDOOM v0.7'.");
    }
    else if (fsize == 28144744)
    {
        C_Output("Playing 'FREEDOOM v0.8 BETA 1'.");
    }
    else if (fsize == 28592816)
    {
        C_Output("Playing 'FREEDOOM v0.8'.");
    }
    else if (fsize == 28422764)
    {
        C_Output("Playing 'FREEDOOM v0.8 PHASE 2'.");
    }
    else if (fsize == 18195736)
    {
        C_Output("Playing 'FINAL DOOM - TNT v1.9 (WITH YELLOW KEYCARD BUG)'.");
    }
    else if (fsize == 18654796)
    {
        C_Output("Playing 'FINAL DOOM - TNT v1.9 (WITHOUT YELLOW KEYCARD BUG)'.");
    }
    else if (fsize == 18240172)
    {
        C_Output("Playing 'FINAL DOOM - PLUTONIA v1.9 (WITH DEATHMATCH STARTS)'.");
    }
    else if (fsize == 17420824)
    {
        C_Output("Playing 'FINAL DOOM - PLUTONIA v1.9 (WITHOUT DEATHMATCH STARTS)'.");
    }
    else if (fsize == 12361532)
    {
        C_Output("Playing 'CHEX QUEST'.");
    }
    else if (fsize == 9745831)
    {
        C_Output("Playing 'HACX SHAREWARE v1.0'.");
    }
    else if (fsize == 21951805)
    {
        C_Output("Playing 'HACX REGISTERED v1.0'.");
    }
    else if (fsize == 22102300)
    {
        C_Output("Playing 'HACX REGISTERED v1.1'.");
    }
    else if (fsize == 19321722)
    {
        C_Output("Playing 'HACX REGISTERED v1.2'.");
    }

    if (d_vsync && (d_uncappedframerate == 3 || d_uncappedframerate == 1))
    {
        d_uncappedframerate--;
    }

    if (d_uncappedframerate == 0)
    {
        C_Output("The framerate is capped at 35 FPS.");
    }
    else if (d_uncappedframerate == 1)
    {
        C_Output("The framerate is uncapped");
    }
    else if (d_uncappedframerate == 2)
    {
        C_Output("The framerate is capped at 60 FPS.");
    }
    else if (d_uncappedframerate == 3)
    {
        C_Output("The framerate is capped at 70 FPS.");
    }

    if (startloadgame >= 0)
    {
        char file[256];

        M_StringCopy(file, P_SaveGameFile(startloadgame), sizeof(file));
        G_LoadGame(file);
    }

    if (!beta_style)
    {
        if (gameaction != ga_loadgame)
        {
            if (autostart || netgame)
            {
                G_InitNew(startskill, startepisode, startmap);
            }
            else
            {
                // start up intro loop
                D_StartTitle();
            }
        }
    }
    else
    {
        G_InitNew(startskill, 1, 2);
    }

    startuptimer = I_StartupTimer() - startuptimer;

    C_Output("Startup took %02i:%02i:%02i.%i to complete.",
        (startuptimer / (1000 * 60 * 60)) % 24,
        (startuptimer / (1000 * 60)) % 60,
        (startuptimer / 1000) % 60,
        (startuptimer % 1000) / 10);

    // Ty 04/08/98 - Add 5 lines of misc. data, only if nonblank
    // The expectation is that these will be set in a .bex file
    if (*startup1 || *startup2 || *startup3 || *startup4 || *startup5)
    {
        C_AddConsoleDivider();

        if (*startup1)
        {
            D_ParseStartupString(startup1);
        }

        if (*startup2)
        {
            D_ParseStartupString(startup2);
        }

        if (*startup3)
        {
            D_ParseStartupString(startup3);
        }

        if (*startup4)
        {
            D_ParseStartupString(startup4);
        }

        if (*startup5)
        {
            D_ParseStartupString(startup5);
        }
    }

    // never returns
    D_DoomLoop();
}

