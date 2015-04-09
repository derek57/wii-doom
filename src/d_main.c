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
//	DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//
//-----------------------------------------------------------------------------
/*
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspkernel.h>
*/
#include <SDL/SDL_mixer.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "deh_main.h"
#include "deh_str.h"
#include "doomdef.h"
#include "doomstat.h"

#include "dstrings.h"
#include "doomfeatures.h"
#include "sounds.h"

#include "d_iwad.h"

#include "z_zone.h"
#include "w_main.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"

#include "f_finale.h"
#include "f_wipe.h"

#include "m_argv.h"
#include "m_config.h"
#include "m_controls.h"
#include "m_misc.h"
#include "m_menu.h"
#include "p_saveg.h"

//#include "i_endoom.h"
#include "i_joystick.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"

#include "g_game.h"

#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
/*
#include "net_client.h"
#include "net_dedicated.h"
#include "net_query.h"
*/
#include "p_setup.h"
#include "r_local.h"
#include "statdump.h"

#include "d_main.h"

#include "c_io.h"

#include "w_wad.h"
#include "sys_wpad.h"

//#define printf pspDebugScreenPrintf

typedef uint32_t u32;				///< 32bit unsigned integer

//
// D-DoomLoop()
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//
void D_DoomLoop (void);

// Location where savegames are stored

char *          savegamedir;

// location of IWAD and WAD files

char *          iwadfile;


boolean         nomonsters;	// checkparm of -nomonsters
boolean         respawnparm;	// checkparm of -respawn
boolean         fastparm;	// checkparm of -fast

//extern int soundVolume;
//extern  int	sfxVolume;
//extern  int	musicVolume;

extern  boolean	inhelpscreens;
extern  boolean	devparm_nerve;
extern	boolean finale_music;
extern	boolean aiming_help;

skill_t		startskill;
int             startepisode;
int		startmap;
boolean		autostart;
int             startloadgame;

boolean		advancedemo;

// Store demo, do not accept any inputs
boolean         storedemo;

// "BFG Edition" version of doom2.wad does not include TITLEPIC.
boolean         bfgedition;
//char            *nervewadfile = NULL;

// If true, the main game loop has started.
boolean         main_loop_started = false;

char		wadfile[1024];		// primary wad file
char		mapdir[1024];           // directory of development maps

//int             show_endoom = 0;	// FIXME: ON THE WII THE ENDOOM SCREEN ISN'T BEING SHOWN

int		fsize = 0;
int		fsizerw = 0;
int		wad_message_has_been_shown = 0;
int		dont_show_adding_of_resource_wad = 0;
int		dots_enabled = 0;
int		fps_enabled = 0;
int		display_fps = 0;
int		resource_wad_exists = 0;

boolean		version13 = false;

extern int mp_skill;
extern int warpepi;
extern int warplev;
extern int mus_engine;
/*
extern boolean skillflag;
extern boolean nomonstersflag;
extern boolean fastflag;
extern boolean respawnflag;
extern boolean warpflag;
extern boolean multiplayerflag;
extern boolean deathmatchflag;
extern boolean altdeathflag;

static SceCtrlData pad;
static SceCtrlData lastpad;
*/
extern boolean	opl;
extern boolean	nerve_pwad;

extern int	warped;

extern menu_t*	currentMenu;                          
extern menu_t	CheatsDef;

extern short	itemOn;			// menu item skull is on

void D_ConnectNetGame(void);
void D_CheckNetGame(void);

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents (void)
{
    event_t*	ev;
	
    // IF STORE DEMO, DO NOT ACCEPT INPUT
    if (storedemo)
        return;
	
    while ((ev = D_PopEvent()) != NULL)
    {
	if (M_Responder (ev) || C_Responder (ev))
	    continue;               // menu ate the event
	G_Responder (ev);
    }
}




//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
gamestate_t     wipegamestate = GS_DEMOSCREEN;
extern  boolean setsizeneeded;
extern  int             showMessages;
void R_ExecuteSetViewSize (void);

boolean			redrawsbar;

void D_Display (void)
{
    static  boolean		viewactivestate = false;
    static  boolean		menuactivestate = false;
    static  boolean		inhelpscreensstate = false;
    static  boolean		fullscreen = false;
    static  gamestate_t		oldgamestate = -1;
    static  int			borderdrawcount;
    int				nowtime;
    int				tics;
    int				wipestart;
//    int				y;
    boolean			done;
    boolean			wipe;
/*
    boolean			redrawsbar;

    if (nodrawers)
	return;                    // for comparative timing / profiling
*/		
    redrawsbar = false;
    
    // change the view size if needed
    if (setsizeneeded)
    {
	R_ExecuteSetViewSize ();
	oldgamestate = -1;                      // force background redraw
	borderdrawcount = 3;
    }

    // save the current screen if about to wipe
    if (gamestate != wipegamestate)
    {
	if(dots_enabled == 1)			// ADDED FOR PSP TO PREVENT CRASH...
    	    display_ticker = false;		// ...UPON WIPING SCREEN WITH ENABLED DISPLAY TICKER
	if(fps_enabled == 1)			// ADDED FOR PSP TO PREVENT CRASH...
    	    display_fps = 0;			// ...UPON WIPING SCREEN WITH ENABLED DISPLAY TICKER

	wipe = true;
	wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
    }
    else
    {
	if(dots_enabled == 1)			// ADDED FOR PSP TO PREVENT CRASH...
    	    display_ticker = true;		// ...UPON WIPING SCREEN WITH ENABLED DISPLAY TICKER
	if(fps_enabled == 1)			// ADDED FOR PSP TO PREVENT CRASH...
    	    display_fps = 1;			// ...UPON WIPING SCREEN WITH ENABLED DISPLAY TICKER

	wipe = false;
    }

    if (gamestate == GS_LEVEL && gametic)
	HU_Erase();
    
    // do buffered drawing
    switch (gamestate)
    {
      case GS_LEVEL:
	if (!gametic)
	    break;
	if (automapactive)
	    AM_Drawer ();
//	if (wipe || (viewheight != 200 && fullscreen) )				// CHANGED FOR HIRES
	if (wipe || (scaledviewheight != (200 << hires) && fullscreen) )	// CHANGED FOR HIRES
	    redrawsbar = true;
	if (inhelpscreensstate && !inhelpscreens)
	    redrawsbar = true;              // just put away the help screen
//	ST_Drawer (viewheight == 200, redrawsbar );				// CHANGED FOR HIRES
	ST_Drawer (scaledviewheight == (200 << hires), redrawsbar );		// CHANGED FOR HIRES

	if(warped == 1)
	{
	    paused = true;
	    currentMenu = &CheatsDef;
	    menuactive = 1;
	    itemOn = currentMenu->lastOn;
	    warped = 0;
	}

//	fullscreen = viewheight == 200;						// CHANGED FOR HIRES
	fullscreen = scaledviewheight == (200 << hires);			// CHANGED FOR HIRES
	break;

      case GS_INTERMISSION:
	WI_Drawer ();
	break;

      case GS_FINALE:
	F_Drawer ();
	break;

      case GS_DEMOSCREEN:
	D_PageDrawer ();
	break;

      case GS_CONSOLE:
        break;
    }
    
    // draw buffered stuff to screen
    I_UpdateNoBlit ();
    
    // draw the view directly
    if (gamestate == GS_LEVEL && !automapactive && gametic)
	R_RenderPlayerView (&players[displayplayer]);

    if (gamestate == GS_LEVEL && gametic)
	HU_Drawer ();
    
    // clean up border stuff
    if (gamestate != oldgamestate && gamestate != GS_LEVEL)
	I_SetPalette (W_CacheLumpName (DEH_String("PLAYPAL"),PU_CACHE));

    // see if the border needs to be initially drawn
    if (gamestate == GS_LEVEL && oldgamestate != GS_LEVEL)
    {
	viewactivestate = false;        // view was not active
	R_FillBackScreen ();    // draw the pattern into the back screen
    }

    // see if the border needs to be updated to the screen
//    if (gamestate == GS_LEVEL && !automapactive && scaledviewwidth != 320)		// CHANGED FOR HIRES
    if (gamestate == GS_LEVEL && !automapactive && scaledviewwidth != (320 << hires))	// CHANGED FOR HIRES
    {
	if (menuactive || menuactivestate || !viewactivestate)
	    borderdrawcount = 3;
	if (borderdrawcount)
	{
	    R_DrawViewBorder ();    // erase old menu stuff
	    borderdrawcount--;
	}

    }
/*
    if (testcontrols)
    {
        // Box showing current mouse speed

        V_DrawMouseSpeedBox(testcontrols_mousespeed);
    }
*/
    menuactivestate = menuactive;
    viewactivestate = viewactive;
    inhelpscreensstate = inhelpscreens;
    oldgamestate = wipegamestate = gamestate;
/*    
    // draw pause pic
    if (paused)
    {
	if (automapactive)
	    y = 4;
	else
//	    y = viewwindowy+4;
//	V_DrawPatchDirect(viewwindowx + (scaledviewwidth - 68) / 2, y, 0, 
//                          W_CacheLumpName (DEH_String("M_PAUSE"), PU_CACHE));
	    y = (viewwindowy >> hires)+4;							// CHANGED FOR HIRES
	V_DrawPatchDirect((viewwindowx >> hires) + ((scaledviewwidth >> hires) - 68) / 2, y,	// CHANGED FOR HIRES
                          W_CacheLumpName (DEH_String("M_PAUSE"), PU_CACHE));			// CHANGED FOR HIRES
    }
*/
    C_Drawer();

    // menus go directly to the screen
    M_Drawer ();          // menu is drawn even on top of everything
    NetUpdate ();         // send out any new accumulation


    // normal update
    if (!wipe)
    {
	I_FinishUpdate ();              // page flip or blit buffer
	return;
    }
    
    // wipe update
    wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

    wipestart = I_GetTime () - 1;

    do
    {
	do
	{
	    nowtime = I_GetTime ();
	    tics = nowtime - wipestart;
            I_Sleep(1);
	} while (tics <= 0);
        
	wipestart = nowtime;
	done = wipe_ScreenWipe(wipe_Melt
			       , 0, 0, SCREENWIDTH, SCREENHEIGHT, tics);
	I_UpdateNoBlit ();
	M_Drawer ();                            // menu is drawn even on top of wipes
	I_FinishUpdate ();                      // page flip or blit buffer
    } while (!done);
}

//
// Add configuration file variable bindings.
//

void D_BindVariables(void)
{
/*
    int i;

    M_ApplyPlatformDefaults();

    I_BindVideoVariables();
    I_BindJoystickVariables();
    I_BindSoundVariables();
*/
    M_BindBaseControls();
/*
    M_BindWeaponControls();
    M_BindMapControls();
    M_BindMenuControls();
    M_BindChatControls(MAXPLAYERS);

    key_multi_msgplayer[0] = HUSTR_KEYGREEN;
    key_multi_msgplayer[1] = HUSTR_KEYINDIGO;
    key_multi_msgplayer[2] = HUSTR_KEYBROWN;
    key_multi_msgplayer[3] = HUSTR_KEYRED;

#ifdef FEATURE_MULTIPLAYER
    NET_BindVariables();
#endif

    M_BindVariable("mouse_sensitivity",      &mouseSensitivity);
    M_BindVariable("sfx_volume",             &sfxVolume);
    M_BindVariable("music_volume",           &musicVolume);
    M_BindVariable("show_messages",          &showMessages);
    M_BindVariable("screenblocks",           &screenblocks);
    M_BindVariable("detaillevel",            &detailLevel);
    M_BindVariable("snd_channels",           &snd_channels);
    M_BindVariable("vanilla_savegame_limit", &vanilla_savegame_limit);
    M_BindVariable("vanilla_demo_limit",     &vanilla_demo_limit);
    M_BindVariable("show_endoom",            &show_endoom);

    // Multiplayer chat macros

    for (i=0; i<10; ++i)
    {
        char buf[12];

        sprintf(buf, "chatmacro%i", i);
        M_BindVariable(buf, &chat_macros[i]);
    }
*/
}

//
// D_GrabMouseCallback
//
// Called to determine whether to grab the mouse pointer
//
/*
boolean D_GrabMouseCallback(void)
{
    // Drone players don't need mouse focus

    if (drone)
        return false;

    // when menu is active or game is paused, release the mouse 
 
    if (menuactive || paused)
        return false;

    // only grab mouse when playing levels (but not demos)

    return (gamestate == GS_LEVEL) && !demoplayback && !advancedemo;
}
*/

/*static*/ void I_SDL_PollMusic(void);

//
//  D_DoomLoop
//
void D_DoomLoop (void)
{
    if (demorecording)
	G_BeginRecording ();

//    if(devparm)
	if(usb)
	{
	    debugfile = fopen("usb:/apps/wiidoom/debug.txt","w");
	    statsfile = fopen("usb:/apps/wiidoom/stats.txt","w");
	}
	else if(sd)
	{
	    debugfile = fopen("sd:/apps/wiidoom/debug.txt","w");
	    statsfile = fopen("sd:/apps/wiidoom/stats.txt","w");
	}

    main_loop_started = true;

    TryRunTics();
/*
    I_SetWindowTitle(gamedescription);
    I_GraphicsCheckCommandLine();
    I_SetGrabMouseCallback(D_GrabMouseCallback);
*/
    I_InitGraphics();
    I_EnableLoadingDisk();

    V_RestoreBuffer();
    R_ExecuteSetViewSize();

    D_StartGameLoop();
/*
    if (testcontrols)
    {
        wipegamestate = gamestate;
    }
*/
    while (1)
    {
	// check if the OGG music stopped playing
	if(usergame && gamestate != GS_DEMOSCREEN && gamestate != GS_CONSOLE && !finale_music)
	    I_SDL_PollMusic();

	// frame syncronous IO operations
	I_StartFrame ();

	// will run at least one tic
        TryRunTics ();

	// move positional sounds
	S_UpdateSounds (players[consoleplayer].mo);

	// Update display, next frame, with current state.
        if (screenvisible)
            D_Display ();
    }
}



//
//  DEMO LOOP
//
int             demosequence;
int             pagetic;
char                    *pagename;


//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker (void)
{
    if (--pagetic < 0)
	D_AdvanceDemo ();
}



//
// D_PageDrawer
//
void D_PageDrawer (void)
{
    V_DrawPatch (0, 0, W_CacheLumpName(pagename, PU_CACHE));
}


//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo (void)
{
    advancedemo = true;
}


//
// This cycles through the demo sequences.
// FIXME - version dependend demo numbers?
//
void D_DoAdvanceDemo (void)
{
    players[consoleplayer].playerstate = PST_LIVE;  // not reborn
    advancedemo = false;
    usergame = false;               // no save / end game here
    paused = false;
    gameaction = ga_nothing;

    // The Ultimate Doom executable changed the demo sequence to add
    // a DEMO4 demo.  Final Doom was based on Ultimate, so also
    // includes this change; however, the Final Doom IWADs do not
    // include a DEMO4 lump, so the game bombs out with an error
    // when it reaches this point in the demo sequence.

    // However! There is an alternate version of Final Doom that
    // includes a fixed executable.

    if (gameversion == exe_ultimate || gameversion == exe_final)
      demosequence = (demosequence+1)%7;
    else
      demosequence = (demosequence+1)%6;
    
    switch (demosequence)
    {
      case 0:
	if ( gamemode == commercial )
	    pagetic = TICRATE * 11;
	else
	    pagetic = 170;
	gamestate = GS_DEMOSCREEN;
	pagename = DEH_String("TITLEPIC");
	if ( gamemode == commercial )
	{
#ifdef OGG_SUPPORT
	    if(opl)
#endif
		S_StartMusic(mus_dm2ttl);
#ifdef OGG_SUPPORT
	    else
		S_StartMP3Music(2, 0); 
#endif
	}
	else
	{
#ifdef OGG_SUPPORT
	    if(opl)
#endif
		S_StartMusic (mus_intro);
#ifdef OGG_SUPPORT
	    else
		S_StartMP3Music(2, 0);
#endif
	}
	break;
      case 1:
/*
	if (fsize != 4261144 && fsize != 10401760 && fsize != 10396254 && fsize != 4274218 &&
		fsize != 4207819)
	    G_DeferedPlayDemo(DEH_String("demo1"));
	else if(fsize == 4261144)
	    G_DeferedPlayDemo(DEH_String("BT14LEV3"));
	else if(fsize == 10401760)
	    G_DeferedPlayDemo(DEH_String("demo_3_1"));
	else if(fsize == 10396254)
	    G_DeferedPlayDemo(DEH_String("demo_3_1"));
	else if(fsize == 4274218)
	    G_DeferedPlayDemo(DEH_String("demo_4_1"));
	else if(fsize == 4207819)
	    G_DeferedPlayDemo(DEH_String("BT14LEV3"));
*/
	break;
      case 2:
	pagetic = 200;
	gamestate = GS_DEMOSCREEN;
	pagename = DEH_String("CREDIT");
	break;
      case 3:
/*
	if (fsize != 4261144 && fsize != 10401760 && fsize != 10396254 && fsize != 4274218 &&
		fsize != 4207819)
	    G_DeferedPlayDemo(DEH_String("demo2"));
	else if(fsize == 4261144)
	    G_DeferedPlayDemo(DEH_String("demo_1_2"));
	else if(fsize == 10401760)
	    G_DeferedPlayDemo(DEH_String("demo_2_2"));
	else if(fsize == 10396254)
	    G_DeferedPlayDemo(DEH_String("demo_3_2"));
	else if(fsize == 4274218)
	    G_DeferedPlayDemo(DEH_String("demo_4_2"));
	else if(fsize == 4207819)
	    G_DeferedPlayDemo(DEH_String("demo_5_2"));
*/
	break;
      case 4:
	gamestate = GS_DEMOSCREEN;
	if ( gamemode == commercial)
	{
	    pagetic = TICRATE * 11;
	    pagename = DEH_String("TITLEPIC");

#ifdef OGG_SUPPORT
	    if(opl)
#endif
		S_StartMusic(mus_dm2ttl);
#ifdef OGG_SUPPORT
	    else
		S_StartMP3Music(2, 0); 
#endif
	}
	else
	{
	    pagetic = 200;

	    if ( gamemode == retail )
	      pagename = DEH_String("CREDIT");
	    else
	    {
		if(fsize != 12361532)
		    pagename = DEH_String("HELP2");
		else
		    pagename = DEH_String("HELP1");
	    }
	}
	break;
      case 5:
/*
	if (fsize != 4261144 && fsize != 10401760 && fsize != 10396254 && fsize != 4274218 &&
		fsize != 4207819)
	    G_DeferedPlayDemo(DEH_String("demo3"));
	else if(fsize == 4261144)
	    G_DeferedPlayDemo(DEH_String("demo_1_3"));
	else if(fsize == 10401760)
	    G_DeferedPlayDemo(DEH_String("demo_2_3"));
	else if(fsize == 10396254)
	    G_DeferedPlayDemo(DEH_String("demo_3_3"));
	else if(fsize == 4274218)
	    G_DeferedPlayDemo(DEH_String("demo_4_3"));
	else if(fsize == 4207819)
	    G_DeferedPlayDemo(DEH_String("demo_5_3"));
*/
	break;
        // THE DEFINITIVE DOOM Special Edition demo
      case 6:
//	G_DeferedPlayDemo(DEH_String("demo4"));
	break;
    }

    // The Doom 3: BFG Edition version of doom2.wad does not have a
    // TITLETPIC lump. Use INTERPIC instead as a workaround.
    if (bfgedition && !strcasecmp(pagename, "TITLEPIC"))
    {
        pagename = "INTERPIC";
    }

    C_InstaPopup();       // make console go away
}



//
// D_StartTitle
//
void D_StartTitle (void)
{
    gameaction = ga_nothing;
    demosequence = -1;
    D_AdvanceDemo ();
    C_InstaPopup();       // make console go away
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
    char *deh_sub;
    
    for (i=0; i<arrlen(banners); ++i)
    {
        // Has the banner been replaced?

        deh_sub = DEH_String(banners[i]);
        
        if (deh_sub != banners[i])
        {
            // Has been replaced
            // We need to expand via printf to include the Doom version 
            // number
            // We also need to cut off spaces to get the basic name

            gamename = Z_Malloc(strlen(deh_sub) + 10, PU_STATIC, 0);
            sprintf(gamename, deh_sub, DOOM_VERSION / 100, DOOM_VERSION % 100);

            while (gamename[0] != '\0' && isspace(gamename[0]))
                strcpy(gamename, gamename+1);

            while (gamename[0] != '\0' && isspace(gamename[strlen(gamename)-1]))
                gamename[strlen(gamename) - 1] = '\0';
            
            return gamename;
        }
    }

    return gamename;
}

//
// Find out what version of Doom is playing.
//
/*
void D_IdentifyVersion(void)
{
    // gamemission is set up by the D_FindIWAD function.  But if 
    // we specify '-iwad', we have to identify using 
    // IdentifyIWADByName.  However, if the iwad does not match
    // any known IWAD name, we may have a dilemma.  Try to 
    // identify by its contents.

    if (gamemission == none)
    {
        unsigned int i;

        for (i=0; i<numlumps; ++i)
        {
            if (!strncasecmp(lumpinfo[i].name, "MAP01", 8))
            {
                gamemission = doom2;
                break;
            } 
            else if (!strncasecmp(lumpinfo[i].name, "E1M1", 8))
            {
                gamemission = doom;
                break;
            }
        }

        if (gamemission == none)
        {
            // Still no idea.  I don't think this is going to work.

            I_Error("Unknown or invalid IWAD file.");
        }
    }

    // Make sure gamemode is set up correctly

    if (logical_gamemission == doom)
    {
        // Doom 1.  But which version?

        if (W_CheckNumForName("E4M1") > 0)
        {
            // Ultimate Doom

            gamemode = retail;
        } 
        else if (W_CheckNumForName("E3M1") > 0)
        {
            gamemode = registered;
        }
        else
        {
            gamemode = shareware;
        }
    }
    else
    {
        // Doom 2 of some kind.

        gamemode = commercial;
    }
}
*/
// Set the gamedescription string

void D_SetGameDescription(void)
{
    boolean is_freedoom = W_CheckNumForName("FREEDOOM") >= 0,
            is_freedm = W_CheckNumForName("FREEDM") >= 0;

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
                gamedescription = GetGameName("FreeDM");
            else
                gamedescription = GetGameName("Freedoom: Phase 2");
        }
        else if (logical_gamemission == doom2)
            gamedescription = GetGameName("DOOM 2: Hell on Earth");
        else if (logical_gamemission == pack_plut)
            gamedescription = GetGameName("DOOM 2: Plutonia Experiment"); 
        else if (logical_gamemission == pack_tnt)
            gamedescription = GetGameName("DOOM 2: TNT - Evilution");
        else if (logical_gamemission == pack_nerve)
            gamedescription = GetGameName("DOOM 2: No Rest For The Living");
    }
}

//      print title for every printed line
char            title[128];

static boolean D_AddFile(char *filename)
{
    wad_file_t *handle;

    if(gamemode == shareware || load_extra_wad == 1 || version13 == true)
    {
	if(dont_show_adding_of_resource_wad == 0)
	    printf("         adding %s\n", filename);
    }
    handle = W_AddFile(filename);

    return handle != NULL;
}

// Copyright message banners
// Some dehacked mods replace these.  These are only displayed if they are 
// replaced by dehacked.
/*
static char *copyright_banners[] =
{
    "===========================================================================\n"
    "ATTENTION:  This version of DOOM has been modified.  If you would like to\n"
    "get a copy of the original game, call 1-800-IDGAMES or see the readme file.\n"
    "        You will not receive technical support for modified games.\n"
    "                      press enter to continue\n"
    "===========================================================================\n",

    "===========================================================================\n"
    "                 Commercial product - do not distribute!\n"
    "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
    "===========================================================================\n",

    "===========================================================================\n"
    "                                Shareware!\n"
    "===========================================================================\n"
};

// Prints a message only if it has been modified by dehacked.

void PrintDehackedBanners(void)
{
    size_t i;

    for (i=0; i<arrlen(copyright_banners); ++i)
    {
        char *deh_s;

        deh_s = DEH_String(copyright_banners[i]);

        if (deh_s != copyright_banners[i])
        {
            printf("%s", deh_s);

            // Make sure the modified banner always ends in a newline character.
            // If it doesn't, add a newline.  This fixes av.wad.

            if (deh_s[strlen(deh_s) - 1] != '\n')
            {
                printf("\n");
            }
        }
    }
}

static struct 
{
    char *description;
    char *cmdline;
    GameVersion_t version;
} gameversions[] = {
    {"Doom 1.9",             "1.9",        exe_doom_1_9},
    {"Hacx",                 "hacx",       exe_hacx},
    {"Ultimate Doom",        "ultimate",   exe_ultimate},
    {"Final Doom",           "final",      exe_final},
    {"Final Doom (alt)",     "final2",     exe_final2},
    {"Chex Quest",           "chex",       exe_chex},
    { NULL,                  NULL,         0},
};

// Initialize the game version

static void InitGameVersion(void)
{
    int p;
    int i;

    //! 
    // @arg <version>
    // @category compat
    //
    // Emulate a specific version of Doom.  Valid values are "1.9",
    // "ultimate", "final", "final2", "hacx" and "chex".
    //

    p = M_CheckParmWithArgs("-gameversion", 1);

    if (p)
    {
        for (i=0; gameversions[i].description != NULL; ++i)
        {
            if (!strcmp(myargv[p+1], gameversions[i].cmdline))
            {
                gameversion = gameversions[i].version;
                break;
            }
        }
        
        if (gameversions[i].description == NULL) 
        {
            printf("Supported game versions:\n");

            for (i=0; gameversions[i].description != NULL; ++i)
            {
                printf("\t%s (%s)\n", gameversions[i].cmdline,
                        gameversions[i].description);
            }
            
            I_Error("Unknown game version '%s'", myargv[p+1]);
        }
    }
    else
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
        else if (gamemode == shareware || gamemode == registered)
        {
            // original

            gameversion = exe_doom_1_9;
        }
        else if (gamemode == retail)
        {
            gameversion = exe_ultimate;
        }
        else if (gamemode == commercial)
        {
            if (gamemission == doom2)
            {
                gameversion = exe_doom_1_9;
            }
            else
            {
                // Final Doom: tnt or plutonia
                // Defaults to emulating the first Final Doom executable,
                // which has the crash in the demo loop; however, having
                // this as the default should mean that it plays back
                // most demos correctly.

                gameversion = exe_final;
            }
        }
    }
    
    // The original exe does not support retail - 4th episode not supported

    if (gameversion < exe_ultimate && gamemode == retail)
    {
        gamemode = registered;
    }

    // EXEs prior to the Final Doom exes do not support Final Doom.

    if (gameversion < exe_final && gamemode == commercial
     && (gamemission == pack_tnt || gamemission == pack_plut))
    {
        gamemission = doom2;
    }
}

void PrintGameVersion(void)
{
    int i;

    for (i=0; gameversions[i].description != NULL; ++i)
    {
        if (gameversions[i].version == gameversion)
        {
            printf("Emulating the behavior of the "
                   "'%s' executable.\n", gameversions[i].description);
            break;
        }
    }
}
*/

void I_QuitSerialFail (void);

// Load the Chex Quest dehacked file, if we are in Chex mode.

static void LoadChexDeh(void)
{
/*
    char *chex_deh = NULL;
    char *sep;
*/
    if (gameversion == exe_chex)
    {
        // Look for chex.deh in the same directory as the IWAD file.
/*
        sep = strrchr(iwadfile, DIR_SEPARATOR);

        if (sep != NULL)
        {
            chex_deh = malloc(strlen(iwadfile) + 9);
            strcpy(chex_deh, iwadfile);
            chex_deh[sep - iwadfile + 1] = '\0';
            strcat(chex_deh, "chex.deh");
        }
        else
        {
            chex_deh = strdup("chex.deh");
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
            I_Error("Unable to find Chex Quest dehacked file (chex.deh).\n"
                    "The dehacked file is required in order to emulate\n"
                    "chex.exe correctly.  It can be found in your nearest\n"
                    "/idgames repository mirror at:\n\n"
                    "   utils/exe_edit/patches/chexdeh.zip");
        }
*/
        if (/*!DEH_LoadFile(chex_deh)*/load_dehacked == 0)
        {
//	    pspDebugScreenClear();

	    printf("\n\n\n");
	    printf(" ===============================================================================");
	    printf("            !!! UNABLE TO FIND CHEX QUEST DEHACKED FILE (CHEX.DEH) !!!          \n");
   	    printf("                                                                                \n");
	    printf("                THIS DEHACKED FILE IS REQUIRED IN ORDER TO EMULATE              ");
	    printf("               CHEX.EXE CORRECTLY.  IT CAN BE FOUND IN YOUR NEAREST             ");
	    printf("                         /IDGAMES REPOSITORY MIRROR AT:                         \n");
	    printf("                                                                                \n");
	    printf("                       UTILS/EXE_EDIT/PATCHES/CHEXDEH.ZIP                       ");
	    printf("                                                                                ");
	    printf("                                QUITTING NOW ...                                ");
	    printf(" ===============================================================================");

	    sleep(5);
//	    sceKernelDelayThread(5000*1000);

	    I_QuitSerialFail();
        }
    }
}

static void LoadNerveWad(void)
{
    int i;
    char lumpname[9];

    if (gamemission != doom2)
        return;

    if (bfgedition && !modifiedgame)
    {
/*
        if (strrchr(iwadfile, DIR_SEPARATOR) != NULL)
        {
            char *dir;
            dir = M_DirName(iwadfile);
            nervewadfile = M_StringJoin(dir, DIR_SEPARATOR_S, "nerve.wad", NULL);
            free(dir);
        }
        else
        {
            nervewadfile = M_StringDuplicate("nerve.wad");
        }

        if (!M_FileExists(nervewadfile))
        {
            free(nervewadfile);
            nervewadfile = D_FindWADByName("nerve.wad");
        }

        if (nervewadfile == NULL)
        {
            return;
        }

        D_AddFile(nervewadfile);
*/
        // rename level name patch lumps out of the way
        for (i = 0; i < 9; i++)
        {
            M_snprintf (lumpname, 9, "CWILV%2.2d", i);
            lumpinfo[W_GetNumForName(lumpname)].name[0] = 'N';
        }
    }
    else
    {
	i = W_GetNumForName("map01");
//	if (!strcmp(lumpinfo[i].wad_file->path, "nerve.wad"))
	{
	    gamemission = pack_nerve;
	    DEH_AddStringReplacement ("TITLEPIC", "INTERPIC");
	}
    }
}

// Function called at exit to display the ENDOOM screen
/*
static void D_Endoom(void)
{
    byte *endoom;

    // Don't show ENDOOM if we have it disabled, or we're running
    // in screensaver or control test mode. Only show it once the
    // game has actually started.

    if (!show_endoom || !main_loop_started
     || screensaver_mode || M_CheckParm("-testcontrols") > 0)
    {
        return;
    }

    if(error_detected)
	endoom = W_CacheLumpName(DEH_String("ERROR"), PU_STATIC);
    else
	endoom = W_CacheLumpName(DEH_String("ENDOOM"), PU_STATIC);

    I_Endoom(endoom);
}
*/
static void LoadHacxDeh(void)
{
    // If this is the HACX IWAD, we need to load the DEHACKED lump.

    if (gameversion == exe_hacx)
    {
        if (!DEH_LoadLumpByName("DEHACKED", true, false))
        {
            I_Error("DEHACKED lump not found.  Please check that this is the "
                    "Hacx v1.2 IWAD.");
        }
    }
}

u32 WaitButtons(void);

void W_CheckSize(int wad);

#include "video.h"
#include "gui.h"

//
// D_DoomMain
//

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

void D_DoomMain (void)
{
    FILE *fprw;

//    int             p;
    char            file[256];
//    char            demolumpname[9];

    if(devparm)
	fsize = 10399316;
//	fsize = 14943400;

//    I_AtExit(D_Endoom, false);

    printf(" ");

    printf("\n");
//    printf("\n");

//    W_CheckSize(0);

    if(usb)
	fprw = fopen("usb:/apps/wiidoom/pspdoom.wad","rb");
    else if(sd)
	fprw = fopen("sd:/apps/wiidoom/pspdoom.wad","rb");

    if(fprw)
    {
	resource_wad_exists = 1;

	fclose(fprw);

	W_CheckSize(0);
    }
    else
    {
    	resource_wad_exists = 0;
    }

    if(print_resource_pwad_error)
    {
	printf("\n\n\n\n\n");
	printf(" ===============================================================================");
	printf("                         !!! WRONG RESOURCE PWAD FILE !!!                       ");
	printf("                   PLEASE COPY THE FILE 'PSPDOOM.WAD' THAT CAME                 ");
	printf("                    WITH THIS RELEASE, INTO THE GAME DIRECTORY                  \n");
   	printf("                                                                                \n");
	printf("                                QUITTING NOW ...                                ");
	printf(" ===============================================================================");

	sleep(5);
//	sceKernelDelayThread(5000*1000);

	I_QuitSerialFail();
    }

    if (fsize != 4261144	&&	// DOOM BETA v1.4
	fsize != 4271324	&&	// DOOM BETA v1.5
	fsize != 4211660	&&	// DOOM BETA v1.6
	fsize != 10396254	&&	// DOOM REGISTERED v1.1
	fsize != 10399316	&&	// DOOM REGISTERED v1.2
	fsize != 10401760	&&	// DOOM REGISTERED v1.6
	fsize != 11159840	&&	// DOOM REGISTERED v1.8
	fsize != 12408292	&&	// DOOM REGISTERED v1.9 (THE ULTIMATE DOOM)
	fsize != 12474561	&&	// DOOM REGISTERED (BFG-XBOX360 EDITION)
	fsize != 12487824	&&	// DOOM REGISTERED (BFG-PC EDITION)
	fsize != 12538385	&&	// DOOM REGISTERED (XBOX EDITION)
	fsize != 4207819	&&	// DOOM SHAREWARE v1.0
	fsize != 4274218	&&	// DOOM SHAREWARE v1.1
	fsize != 4225504	&&	// DOOM SHAREWARE v1.2
	fsize != 4225460	&&	// DOOM SHAREWARE v1.25 (SYBEX RELEASE)
	fsize != 4234124	&&	// DOOM SHAREWARE v1.666
	fsize != 4196020	&&	// DOOM SHAREWARE v1.8
	fsize != 14943400	&&	// DOOM 2 REGISTERED v1.666
	fsize != 14824716	&&	// DOOM 2 REGISTERED v1.666 (GERMAN VERSION)
	fsize != 14612688	&&	// DOOM 2 REGISTERED v1.7
	fsize != 14607420	&&	// DOOM 2 REGISTERED v1.8 (FRENCH VERSION)
	fsize != 14604584	&&	// DOOM 2 REGISTERED v1.9
	fsize != 14677988	&&	// DOOM 2 REGISTERED (BFG-PSN EDITION)
	fsize != 14691821	&&	// DOOM 2 REGISTERED (BFG-PC EDITION)
	fsize != 14683458	&&	// DOOM 2 REGISTERED (XBOX EDITION)
	fsize != 18195736	&&	// FINAL DOOM - TNT v1.9 (WITH YELLOW KEYCARD BUG)
	fsize != 18654796	&&	// FINAL DOOM - TNT v1.9 (WITHOUT YELLOW KEYCARD BUG)
	fsize != 18240172	&&	// FINAL DOOM - PLUTONIA v1.9 (WITH DEATHMATCH STARTS)
	fsize != 17420824	&&	// FINAL DOOM - PLUTONIA v1.9 (WITHOUT DEATHMATCH STARTS)
/*
	fsize != 19801320	&&	// FREEDOOM v0.6.4
	fsize != 27704188	&&	// FREEDOOM v0.7 RC 1
	fsize != 27625596	&&	// FREEDOOM v0.7
	fsize != 28144744	&&	// FREEDOOM v0.8 BETA 1
	fsize != 28592816	&&	// FREEDOOM v0.8
	fsize != 19362644	&&	// FREEDOOM v0.8 PHASE 1
*/
	fsize != 28422764	&&	// FREEDOOM v0.8 PHASE 2
	fsize != 12361532	&&	// CHEX QUEST
/*
	fsize != 9745831	&&	// HACX SHAREWARE v1.0
	fsize != 21951805	&&	// HACX REGISTERED v1.0
	fsize != 22102300	&&	// HACX REGISTERED v1.1
*/
	fsize != 19321722)		// HACX REGISTERED v1.2
    {
	printf("\n\n\n\n\n");
	printf(" ===============================================================================");
	printf("            WARNING: DOOM / DOOM 2 / TNT / PLUTONIA IWAD FILE MISSING,          ");
	printf("                         NOT SELECTED OR WRONG IWAD !!!                         \n");
	printf("                                                                                \n");
	printf("                                QUITTING NOW ...                                ");
	printf(" ===============================================================================");

	sleep(5);
//	sceKernelDelayThread(5000*1000);

	I_QuitSerialFail();
    }
    else if(fsize == 10396254	||	// DOOM REGISTERED v1.1
	    fsize == 10399316	||	// DOOM REGISTERED v1.2
	    fsize == 4207819	||	// DOOM SHAREWARE v1.0
	    fsize == 4274218	||	// DOOM SHAREWARE v1.1
	    fsize == 4225504	||	// DOOM SHAREWARE v1.2
	    fsize == 4225460)		// DOOM SHAREWARE v1.25 (SYBEX RELEASE)
    {
/*
    	pspDebugScreenSetTextColor(0x00FFFF);	// yellow
    	pspDebugScreenSetBackColor(0xFF0000);	// blue

    	printf("                    DOOM Operating System v1.2                    ");
*/
	printStyledText(1, 1,CONSOLE_FONT_BLUE,CONSOLE_FONT_YELLOW,CONSOLE_FONT_BOLD,&stTexteLocation,"                          DOOM Operating System v1.2                           ");
    }
    else if(fsize == 4261144	||	// DOOM BETA v1.4
	    fsize == 4271324	||	// DOOM BETA v1.5
	    fsize == 4211660	||	// DOOM BETA v1.6
	    fsize == 10401760	||	// DOOM REGISTERED v1.6
	    fsize == 11159840	||	// DOOM REGISTERED v1.8
	    fsize == 4234124	||	// DOOM SHAREWARE v1.666
	    fsize == 4196020)		// DOOM SHAREWARE v1.8
    {
/*
    	pspDebugScreenSetTextColor(0xFFFFFF);	// white
    	pspDebugScreenSetBackColor(0x0000FF);	// red
*/
	printStyledText(1, 1,CONSOLE_FONT_RED,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"                           DOOM System Startup v1.4                            ");

	version13 = true;
    }
    else if(fsize == 12408292	||	// DOOM REGISTERED v1.9 (THE ULTIMATE DOOM)
	    fsize == 12538385	||	// DOOM REGISTERED (XBOX EDITION)
	    fsize == 12487824	||	// DOOM REGISTERED (BFG-PC EDITION)
	    fsize == 12474561		// DOOM REGISTERED (BFG-XBOX360 EDITION)
/*
				||
	    fsize == 19362644		// FREEDOOM v0.8 PHASE 1
*/
	    )
    {
/*
    	pspDebugScreenSetTextColor(0x0000FF);	// red
    	pspDebugScreenSetBackColor(0xD0D0D0);	// grey

    	printf("                     DOOM System Startup v1.9                     ");
*/
	printStyledText(1, 1,CONSOLE_FONT_WHITE,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"                           DOOM System Startup v1.9                            ");

	version13 = true;
    }
    else if(fsize == 14943400	||	// DOOM 2 REGISTERED v1.666
	    fsize == 14824716	||	// DOOM 2 REGISTERED v1.666 (GERMAN VERSION)
	    fsize == 14612688	||	// DOOM 2 REGISTERED v1.7
	    fsize == 14607420)		// DOOM 2 REGISTERED v1.8 (FRENCH VERSION)
    {
/*
    	pspDebugScreenSetTextColor(0xFFFFFF);	// white
    	pspDebugScreenSetBackColor(0x0000FF);	// red

    	printf("                   DOOM 2: Hell on Earth v1.666                   ");
*/
	printStyledText(1, 1,CONSOLE_FONT_RED,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"                         DOOM 2: Hell on Earth v1.666                          ");

	version13 = true;
    }
    else if(fsize == 14604584	||	// DOOM 2 REGISTERED v1.9
	    fsize == 14677988	||	// DOOM 2 REGISTERED (BFG-PSN EDITION)
	    fsize == 14691821	||	// DOOM 2 REGISTERED (BFG-PC EDITION)
	    fsize == 14683458	||	// DOOM 2 REGISTERED (XBOX EDITION)
/*
	    fsize == 9745831	||	// HACX SHAREWARE v1.0
	    fsize == 21951805	||	// HACX REGISTERED v1.0
	    fsize == 22102300	||	// HACX REGISTERED v1.1
*/
	    fsize == 19321722	||	// HACX REGISTERED v1.2
/*
	    fsize == 19801320	||	// FREEDOOM v0.6.4
	    fsize == 27704188	||	// FREEDOOM v0.7 RC 1
	    fsize == 27625596	||	// FREEDOOM v0.7
	    fsize == 28144744	||	// FREEDOOM v0.8 BETA 1
	    fsize == 28592816	||	// FREEDOOM v0.8
*/
	    fsize == 28422764)		// FREEDOOM v0.8 PHASE 2
    {
/*
    	pspDebugScreenSetTextColor(0x0000FF);	// red
    	pspDebugScreenSetBackColor(0xD0D0D0);	// grey
*/
	if     (
/*
		fsize == 9745831	||	// HACX SHAREWARE v1.0
		fsize == 21951805	||	// HACX REGISTERED v1.0
		fsize == 22102300	||	// HACX REGISTERED v1.1
*/
		fsize == 19321722)		// HACX REGISTERED v1.2
//	    printf("                    HACX:  Twitch n' Kill v1.2                    ");
	    printStyledText(1, 1,CONSOLE_FONT_WHITE,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"                          HACX:  Twitch n' Kill v1.2                           ");
	else
//	    printf("                    DOOM 2: Hell on Earth v1.9                    ");
	    printStyledText(1, 1,CONSOLE_FONT_WHITE,CONSOLE_FONT_RED,CONSOLE_FONT_BOLD,&stTexteLocation,"                          DOOM 2: Hell on Earth v1.9                           ");

	version13 = true;
    }
    else if(fsize == 18195736	||	// FINAL DOOM - TNT v1.9 (WITH YELLOW KEYCARD BUG)
	    fsize == 18654796	||	// FINAL DOOM - TNT v1.9 (WITHOUT YELLOW KEYCARD BUG)
	    fsize == 12361532)		// CHEX QUEST
    {
/*
    	pspDebugScreenSetTextColor(0x7F7F7F);	// dark grey
    	pspDebugScreenSetBackColor(0xD0D0D0);	// light grey
*/
	if(fsize == 12361532)
//	    printf("                      Chex (R) Quest Startup                      ");
	    printStyledText(1, 1,CONSOLE_FONT_WHITE,CONSOLE_FONT_BLACK,CONSOLE_FONT_BOLD,&stTexteLocation,"                            Chex (R) Quest Startup                             ");
	else
//	    printf("                    DOOM 2: TNT Evilution v1.9                    ");
	    printStyledText(1, 1,CONSOLE_FONT_WHITE,CONSOLE_FONT_BLACK,CONSOLE_FONT_BOLD,&stTexteLocation,"                          DOOM 2: TNT Evilution v1.9                           ");

	version13 = true;
    }

    else if(fsize == 18240172	||	// FINAL DOOM - PLUTONIA v1.9 (WITH DEATHMATCH STARTS)
	    fsize == 17420824)		// FINAL DOOM - PLUTONIA v1.9 (WITHOUT DEATHMATCH STARTS)
    {
/*
    	pspDebugScreenSetTextColor(0x7F7F7F);	// dark grey
    	pspDebugScreenSetBackColor(0xD0D0D0);	// light grey
*/
//    	printf("                 DOOM 2: Plutonia Experiment v1.9                 ");
	printStyledText(1, 1,CONSOLE_FONT_WHITE,CONSOLE_FONT_BLACK,CONSOLE_FONT_BOLD,&stTexteLocation,"                       DOOM 2: Plutonia Experiment v1.9                        ");

	version13 = true;
    }
/*
    pspDebugScreenSetTextColor(0xD0D0D0);	// grey
    pspDebugScreenSetBackColor(0x000000);	// black

    printf("\n");
*/
//    printStyledText(1, 1,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,CONSOLE_FONT_BOLD,&stTexteLocation,"                           DOOM System Startup v1.4                            ");

    if (resource_wad_exists == 0)
    {
	printf("\n\n\n\n\n");
	printf(" ===============================================================================");
	printf("              WARNING: RESOURCE PWAD FILE 'PSPDOOM.WAD' MISSING!!!              ");
	printf("               PLEASE COPY THIS FILE INTO THE GAME'S DIRECTORY!!!               \n");
	printf("                                                                                \n");
	printf("                               QUITTING NOW ...                                 ");
	printf(" ===============================================================================");

	sleep(5);
//	sceKernelDelayThread(5000*1000);

	I_QuitSerialFail();
    }

    // print banner
/*
    I_PrintBanner(PACKAGE_STRING);

    DEH_printf("Z_Init: Init zone memory allocation daemon. \n");
*/
    Z_Init ();

    C_Init();

    C_Printf(" ");
/*
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
        NET_QueryAddress(myargv[p+1]);
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
*/
#ifdef FEATURE_DEHACKED
//    printf("DEH_Init: Init Dehacked support.\n");
    if(load_dehacked == 1)
	DEH_LoadFile(dehacked_file);

    if(fsize == 19321722)
	DEH_Init();
#endif
/*
    iwadfile = D_FindIWAD(IWAD_MASK_DOOM, &gamemission);

    // None found?

    if (iwadfile == NULL)
    {
        I_Error("Game mode indeterminate.  No IWAD file was found.  Try\n"
                "specifying one with the '-iwad' command line parameter.\n");
    }
*/
    modifiedgame = false;

    //!
    // @vanilla
    //
    // Disable monsters.
    //
/*
    nomonsters = M_CheckParm ("-nomonsters");

    if(nomonstersflag)
	nomonsters = true;

    //!
    // @vanilla
    //
    // Monsters respawn after being killed.
    //

    respawnparm = M_CheckParm ("-respawn");

    if(respawnflag)
	respawnparm = true;

    //!
    // @vanilla
    //
    // Monsters move faster.
    //

    fastparm = M_CheckParm ("-fast");

    if(fastflag)
	fastparm = true;

    //! 
    // @vanilla
    //
    // Developer mode.  F1 saves a screenshot in the current working
    // directory.
    //

    devparm = M_CheckParm ("-devparm");

    I_DisplayFPSDots(devparm);

    //!
    // @category net
    // @vanilla
    //
    // Start a deathmatch game.
    //

    if (M_CheckParm ("-deathmatch"))

    if(deathmatchflag)
	deathmatch = 1;

    //!
    // @category net
    // @vanilla
    //
    // Start a deathmatch 2.0 game.  Weapons do not stay in place and
    // all items respawn after 30 seconds.
    //

    if (M_CheckParm ("-altdeath"))

    if(altdeathflag)
	deathmatch = 2;
*/
    if (devparm)
	/*DEH_printf*/printf(D_DEVSTR);
/*    
    // find which dir to use for config files

#ifdef _WIN32

    //!
    // @platform windows
    // @vanilla
    //
    // Save configuration data and savegames in c:\doomdata,
    // allowing play from CD.
    //

    if (M_CheckParm("-cdrom") > 0)
    {
        printf(D_CDROM);

        M_SetConfigDir("c:\\doomdata\\");
    }
    else
#endif
*/
    {
        // Auto-detect the configuration dir.

        M_SetConfigDir(NULL);
    }
    
    //!
    // @arg <x>
    // @vanilla
    //
    // Turbo mode.  The player's speed is multiplied by x%.  If unspecified,
    // x defaults to 200.  Values are rounded up to 10 and down to 400.
    //
/*
    if ( (p=M_CheckParm ("-turbo")) )
    {
	int     scale = 200;
	extern int forwardmove[2];
	extern int sidemove[2];
	
	if (p<myargc-1)
	    scale = atoi (myargv[p+1]);
	if (scale < 10)
	    scale = 10;
	if (scale > 400)
	    scale = 400;
        DEH_printf("turbo scale: %i%%\n", scale);
	forwardmove[0] = forwardmove[0]*scale/100;
	forwardmove[1] = forwardmove[1]*scale/100;
	sidemove[0] = sidemove[0]*scale/100;
	sidemove[1] = sidemove[1]*scale/100;
    }
*/    
    // init subsystems
    /*DEH_printf*/printf(" V_Init: allocate screens.\n");
    V_Init ();

    // Load configuration files before initialising other subsystems.
    /*DEH_printf*/printf(" M_LoadDefaults: Load system defaults.\n");
    M_SetConfigFilenames("default.cfg");//, PROGRAM_PREFIX "doom.cfg");
    D_BindVariables();
    M_LoadDefaults();

    if(!devparm && aiming_help != 0)
	aiming_help = 0;

    if(mus_engine > 1)
	mus_engine = 2;
    else if(mus_engine < 2)
	mus_engine = 1;

    if(mus_engine == 1)
	snd_musicdevice = SNDDEVICE_SB;
    else
	snd_musicdevice = SNDDEVICE_GENMIDI;

    // Save configuration at exit.
    I_AtExit(M_SaveDefaults, false);

    /*DEH_printf*/printf(" Z_Init: Init zone memory allocation daemon. \n");
    /*DEH_printf*/printf(" heap size: 0x3cdb000 \n");

    /*DEH_printf*/printf(" W_Init: Init WADfiles.\n");

    if (fsize == 4207819	||	// DOOM SHAREWARE v1.0
	fsize == 4274218	||	// DOOM SHAREWARE v1.1
	fsize == 4225504	||	// DOOM SHAREWARE v1.2
	fsize == 4225460	||	// DOOM SHAREWARE v1.25 (SYBEX RELEASE)
	fsize == 4234124	||	// DOOM SHAREWARE v1.666
	fsize == 4196020	||	// DOOM SHAREWARE v1.8
	fsize == 4261144	||	// DOOM BETA v1.4
	fsize == 4271324	||	// DOOM BETA v1.5
	fsize == 4211660)		// DOOM BETA v1.6
    {
	gamemode = shareware;
	gamemission = doom;
	gameversion = exe_doom_1_9;
	nerve_pwad = false;
    }
    else if(fsize == 10396254	||	// DOOM REGISTERED v1.1
	    fsize == 10399316	||	// DOOM REGISTERED v1.2
	    fsize == 10401760	||	// DOOM REGISTERED v1.6
	    fsize == 11159840)		// DOOM REGISTERED v1.8
    {
	gamemode = registered;
	gamemission = doom;
	gameversion = exe_doom_1_9;
	nerve_pwad = false;
    }
    else if(fsize == 12408292	||	// DOOM REGISTERED v1.9 (THE ULTIMATE DOOM)
	    fsize == 12538385	||	// DOOM REGISTERED (XBOX EDITION)
	    fsize == 12487824	||	// DOOM REGISTERED (BFG-PC EDITION)
	    fsize == 12474561		// DOOM REGISTERED (BFG-XBOX360 EDITION)
/*
				||
	    fsize == 19362644
*/
			     )		// FREEDOOM v0.8 PHASE 1
    {
	gamemode = retail;
	gamemission = doom;
	gameversion = exe_ultimate;
	nerve_pwad = false;
    }
    else if(fsize == 14943400	||	// DOOM 2 REGISTERED v1.666
	    fsize == 14824716	||	// DOOM 2 REGISTERED v1.666 (GERMAN VERSION)
	    fsize == 14612688	||	// DOOM 2 REGISTERED v1.7
	    fsize == 14607420	||	// DOOM 2 REGISTERED v1.8 (FRENCH VERSION)
	    fsize == 14604584	||	// DOOM 2 REGISTERED v1.9
	    fsize == 14677988	||	// DOOM 2 REGISTERED (BFG-PSN EDITION)
	    fsize == 14691821	||	// DOOM 2 REGISTERED (BFG-PC EDITION)
	    fsize == 14683458	||	// DOOM 2 REGISTERED (XBOX EDITION)
/*
	    fsize == 19801320	||	// FREEDOOM v0.6.4
	    fsize == 27704188	||	// FREEDOOM v0.7 RC 1
	    fsize == 27625596	||	// FREEDOOM v0.7
	    fsize == 28144744	||	// FREEDOOM v0.8 BETA 1
	    fsize == 28592816	||	// FREEDOOM v0.8
*/
	    fsize == 28422764)		// FREEDOOM v0.8 PHASE 2
    {
	gamemode = commercial;
	gamemission = doom2;
	gameversion = exe_doom_1_9;
    }
    else if(fsize == 18195736	||	// FINAL DOOM - TNT v1.9 (WITH YELLOW KEYCARD BUG)
	    fsize == 18654796)		// FINAL DOOM - TNT v1.9 (WITHOUT YELLOW KEYCARD BUG)
    {
	gamemode = commercial;
	gamemission = pack_tnt;
	gameversion = exe_final;
	nerve_pwad = false;
    }
    else if(fsize == 18240172	||	// FINAL DOOM - PLUTONIA v1.9 (WITH DEATHMATCH STARTS)
	    fsize == 17420824)		// FINAL DOOM - PLUTONIA v1.9 (WITHOUT DEATHMATCH STARTS)
    {
	gamemode = commercial;
	gamemission = pack_plut;
	gameversion = exe_final;
	nerve_pwad = false;
    }
    else if(fsize == 12361532)		// CHEX QUEST
    {
	gamemode = shareware;
	gamemission = pack_chex;
	gameversion = exe_chex;
	nerve_pwad = false;
    }
    else if(
/*	    fsize == 9745831	||	// HACX SHAREWARE v1.0
	    fsize == 21951805	||	// HACX REGISTERED v1.0
	    fsize == 22102300	||	// HACX REGISTERED v1.1
*/
	    fsize == 19321722)		// HACX REGISTERED v1.2
    {
	gamemode = commercial;
	gamemission = pack_hacx;
	gameversion = exe_hacx;
	nerve_pwad = false;
    }

//    InitGameVersion();

    if(devparm)
    {
	if(usb)
	    D_AddFile("usb:/apps/wiidoom/IWAD/DOOM/Reg/v12/DOOM.WAD");
//	    D_AddFile("usb:/apps/wiidoom/IWAD/DOOM2/v1666/DOOM2.WAD");
	else if(sd)
	    D_AddFile("sd:/apps/wiidoom/IWAD/DOOM/Reg/v12/DOOM.WAD");
//	    D_AddFile("sd:/apps/wiidoom/IWAD/DOOM2/v1666/DOOM2.WAD");
    }
    else
	D_AddFile(target);

    if(gamemode != shareware || (gamemode == shareware && gameversion == exe_chex))
    {
	if(load_extra_wad == 1)
	{
	    opl = 1;

	    if(extra_wad_slot_1_loaded == 1)
		D_AddFile(extra_wad_1);

	    if(!nerve_pwad)
	    {
		if(extra_wad_slot_2_loaded == 1)
		    D_AddFile(extra_wad_2);

		if(extra_wad_slot_3_loaded == 1)
		    D_AddFile(extra_wad_3);
	    }
	    modifiedgame = /*W_ParseCommandLine()*/ true;
	}
    }

    if(devparm_nerve)
	D_AddFile("usb:/apps/wiidoom/PWAD/DOOM2/NERVE.WAD");

    dont_show_adding_of_resource_wad = 0;

    if(usb)
	D_AddFile("usb:/apps/wiidoom/pspdoom.wad");
    else if(sd)
	D_AddFile("sd:/apps/wiidoom/pspdoom.wad");

    if(show_deh_loading_message == 1 /*&& devparm*/)
	printf("         adding %s\n", dehacked_file);

/*
    // Debug:
//    W_PrintDirectory();

    //!
    // @arg <demo>
    // @category demo
    // @vanilla
    //
    // Play back the demo named demo.lmp.
    //

    p = M_CheckParmWithArgs ("-playdemo", 1);

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
        if (!strcasecmp(myargv[p+1] + strlen(myargv[p+1]) - 4, ".lmp"))
        {
            strcpy(file, myargv[p + 1]);
        }
        else
        {
	    sprintf (file,"%s.lmp", myargv[p+1]);
        }

	if (D_AddFile (file))
        {
            strncpy(demolumpname, lumpinfo[numlumps - 1].name, 8);
            demolumpname[8] = '\0';

            printf("Playing demo %s.\n", file);
        }
        else
        {
            // If file failed to load, still continue trying to play
            // the demo in the same way as Vanilla Doom.  This makes
            // tricks like "-playdemo demo1" possible.

            strncpy(demolumpname, myargv[p + 1], 8);
            demolumpname[8] = '\0';
        }

    }
*/
    I_AtExit((atexit_func_t) G_CheckDemoStatus, true);

    // Generate the WAD hash table.  Speed things up a bit.

    W_GenerateHashTable();
/*
    if (!M_ParmExists("-nodeh"))
	LoadNerveWad();

    D_IdentifyVersion();
*/
    if(fsize == 12361532)
	LoadChexDeh();

    if(
//	fsize == 9745831 || fsize == 21951805 || fsize == 22102300 ||
	fsize == 19321722)
	LoadHacxDeh();

    D_SetGameDescription();
    savegamedir = M_GetSaveGameDir(D_SaveGameIWADName(gamemission));

    if(fsize == 12361532)
    {
	W_CheckSize(1);

	if(print_resource_pwad_error)
	{
	    printf("\n\n\n\n\n");
	    printf(" ===============================================================================");
	    printf("                         !!! WRONG RESOURCE PWAD FILE !!!                       ");
	    printf("                   PLEASE COPY THE FILE 'PSPCHEX.WAD' THAT CAME                 ");
	    printf("                    WITH THIS RELEASE, INTO THE GAME DIRECTORY                  \n");
   	    printf("                                                                                \n");
	    printf("                                QUITTING NOW ...                                ");
	    printf(" ===============================================================================");

	    sleep(5);
//	    sceKernelDelayThread(5000*1000);

	    I_QuitSerialFail();
	}
	else
	{
	    if(usb)
		D_AddFile("usb:/apps/wiidoom/pspchex.wad");
	    else if(sd)
		D_AddFile("sd:/apps/wiidoom/pspchex.wad");
	}
    }
    else if(fsize == 19321722)
    {
	W_CheckSize(2);

	if(print_resource_pwad_error)
	{
	    printf("\n\n\n\n\n");
	    printf(" ===============================================================================");
	    printf("                         !!! WRONG RESOURCE PWAD FILE !!!                       ");
	    printf("                   PLEASE COPY THE FILE 'PSPHACX.WAD' THAT CAME                 ");
	    printf("                    WITH THIS RELEASE, INTO THE GAME DIRECTORY                  \n");
   	    printf("                                                                                \n");
	    printf("                                QUITTING NOW ...                                ");
	    printf(" ===============================================================================");

	    sleep(5);
//	    sceKernelDelayThread(5000*1000);

	    I_QuitSerialFail();
	}
	else
	{
	    if(usb)
		D_AddFile("usb:/apps/wiidoom/psphacx.wad");
	    else if(sd)
		D_AddFile("sd:/apps/wiidoom/psphacx.wad");
	}
    }
    else if(fsize == 28422764)
    {
	W_CheckSize(3);

	if(print_resource_pwad_error)
	{
	    printf("\n\n\n\n\n");
	    printf(" ===============================================================================");
	    printf("                         !!! WRONG RESOURCE PWAD FILE !!!                       ");
	    printf("                 PLEASE COPY THE FILE 'PSPFREEDOOM.WAD' THAT CAME               ");
	    printf("                    WITH THIS RELEASE, INTO THE GAME DIRECTORY                  \n");
   	    printf("                                                                                \n");
	    printf("                                QUITTING NOW ...                                ");
	    printf(" ===============================================================================");

	    sleep(5);
//	    sceKernelDelayThread(5000*1000);

	    I_QuitSerialFail();
	}
	else
	{
	    if(usb)
		D_AddFile("usb:/apps/wiidoom/pspfreedoom.wad");
	    else if(sd)
		D_AddFile("sd:/apps/wiidoom/pspfreedoom.wad");
	}
    }

    if(gamemode == shareware && gameversion != exe_chex)
	/*DEH_printf*/printf("         shareware version.\n");
    else if((gamemode == shareware && gameversion == exe_chex) || gamemode == registered)
	/*DEH_printf*/printf("         registered version.\n");
    else
	/*DEH_printf*/printf("         commercial version.\n");

    if(gamemode == retail || gamemode == registered)
    {
	printf(" ===============================================================================");
	printf("                 This version is NOT SHAREWARE, do not distribute!              ");
	printf("             Please report software piracy to the SPA: 1-800-388-PIR8           ");
	printf(" ===============================================================================");
    }
    else if(gamemode == commercial)
    {
	printf(" ===============================================================================");
    	printf("                                Do not distribute!                              ");
    	printf("             Please report software piracy to the SPA: 1-800-388-PIR8           ");
    	printf(" ===============================================================================");
    }

    if(modifiedgame)
    {
	while(1)
	{
//	    sceCtrlReadBufferPositive(&pad, 1);

    	    if(wad_message_has_been_shown == 1)
    	        goto skip_showing_message;
	
	    printf(" ===============================================================================");
	    printf("    ATTENTION:  This version of DOOM has been modified.  If you would like to   ");
	    printf("   get a copy of the original game, call 1-800-IDGAMES or see the readme file.  ");
	    printf("            You will not receive technical support for modified games.          ");
	    printf("                             press enter to continue                            ");
	    printf(" ===============================================================================");

	    skip_showing_message:
	    {
	    }

	    u32 buttons = WaitButtons();

	    if (buttons & WPAD_CLASSIC_BUTTON_A)
		break;

	    WaitButtons();

	    wad_message_has_been_shown = 1;
	}
    }

    // Check for -file in shareware
    if (modifiedgame)
    {
	// These are the lumps that will be checked in IWAD,
	// if any one is not present, execution will be aborted.
	char name[23][8]=
	{
	    "e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
	    "e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
	    "dphoof","bfgga0","heada1","cybra1","spida1d1"
	};
	int i;
	
	if ( gamemode == shareware && gameversion != exe_chex)
	    I_Error(DEH_String("\nYou cannot -file with the shareware "
			       "version. Register!"));

	// Check for fake IWAD with right name,
	// but w/o all the lumps of the registered version. 
	if (gamemode == registered)
	    for (i = 0;i < 23; i++)
		if (W_CheckNumForName(name[i])<0)
		    I_Error(DEH_String("\nThis is not the registered version."));
    }
/*
    if (W_CheckNumForName("SS_START") >= 0
     || W_CheckNumForName("FF_END") >= 0)
    {
        I_PrintDivider();
        printf(" WARNING: The loaded WAD file contains modified sprites or\n"
               " floor textures.  You may want to use the '-merge' command\n"
               " line option instead of '-file'.\n");
    }

    I_PrintStartupBanner(gamedescription);
    PrintDehackedBanners();
#ifdef FEATURE_MULTIPLAYER
    printf ("NET_Init: Init network subsystem.\n");
    NET_Init ();
#endif
*/
    // Initial netgame startup. Connect to server etc.
    D_ConnectNetGame();

    // get skill / episode / map from parms
    startskill = sk_medium;
    startepisode = 1;
    startmap = 1;

    if(devparm)
	autostart = true;
    else
	autostart = false;

    //!
    // @arg <skill>
    // @vanilla
    //
    // Set the game skill, 1-5 (1: easiest, 5: hardest).  A skill of
    // 0 disables all monsters.
    //
/*
    p = M_CheckParmWithArgs("-skill", 1);

    if (skillflag)
    {
	startskill = mp_skill;
	autostart = true;
    }

    //!
    // @arg <n>
    // @vanilla
    //
    // Start playing on episode n (1-4)
    //

    p = M_CheckParmWithArgs("-episode", 1);

    if (p)
    {
	startepisode = myargv[p+1][0]-'0';
	startmap = 1;
	autostart = true;
    }
*/	
    timelimit = 0;

    //! 
    // @arg <n>
    // @category net
    // @vanilla
    //
    // For multiplayer games: exit each level after n minutes.
    //
/*
    p = M_CheckParmWithArgs("-timer", 1);

    if (p)
    {
	timelimit = atoi(myargv[p+1]);
    }

    //!
    // @category net
    // @vanilla
    //
    // Austin Virtual Gaming: end levels after 20 minutes.
    //

    p = M_CheckParm ("-avg");

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

    p = M_CheckParmWithArgs("-warp", 1);

    if (warpflag)
    {
        if (gamemode == commercial)
            startmap = warplev;
        else
        {
            startepisode = warpepi;

            if (p + 2 < myargc)
            {
                startmap = warplev;
            }
            else
            {
                startmap = 1;
            }

        }
        autostart = true;
    }

    // Undocumented:
    // Invoked by setup to test the controls.

    p = M_CheckParm("-testcontrols");

    if (p > 0)
    {
        startepisode = 1;
        startmap = 1;
        autostart = true;
        testcontrols = true;
    }

    // Check for load game parameter
    // We do this here and save the slot number, so that the network code
    // can override it or send the load slot to other players.

    //!
    // @arg <s>
    // @vanilla
    //
    // Load the game in slot s.
    //

    p = M_CheckParmWithArgs("-loadgame", 1);
    
    if (p)
    {
        startloadgame = atoi(myargv[p+1]);
    }
    else
*/
    {
        // Not loading a game
        startloadgame = -1;
    }

    /*DEH_printf*/printf(" M_Init: Init miscellaneous info.\n");
    M_Init ();

    if(gameversion == exe_chex)
    /*DEH_printf*/printf(" R_Init: Init Chex(R) Quest refresh daemon - ");
    else
    /*DEH_printf*/printf(" R_Init: Init DOOM refresh daemon - ");
    R_Init ();

    /*DEH_printf*/printf("\n P_Init: Init Playloop state.\n");
    P_Init ();

//    I_Sleep(1000);

    /*DEH_printf*/printf(" I_Init: Setting up machine state.\n");
//    I_CheckIsScreensaver();

    /*DEH_printf*/printf(" I_StartupDPMI\n");

    /*DEH_printf*/printf(" I_StartupMouse\n");

    /*DEH_printf*/printf(" I_StartupJoystick\n");
//    I_InitJoystick();

    /*DEH_printf*/printf(" I_StartupKeyboard\n");

    /*DEH_printf*/printf(" I_StartupTimer\n");
    I_InitTimer();

    /*DEH_printf*/printf(" I_StartupSound\n");
//    I_Sleep(500);
    /*DEH_printf*/printf(" calling DMX_Init\n");

    /*DEH_printf*/printf(" D_CheckNetGame: Checking network game status.\n");
    D_CheckNetGame ();
//    I_Sleep(250);

    /*DEH_printf*/printf(" S_Init: Setting up sound.\n");
    S_Init (sfxVolume * 8, musicVolume * 8);

#ifdef OGG_SUPPORT
    Mix_VolumeMusic(musicVolume + 12);
    SDL_InitOGG();
#endif

//    PrintGameVersion();

    /*DEH_printf*/printf(" HU_Init: Setting up heads up display.\n");
    HU_Init ();

    /*DEH_printf*/printf(" ST_Init: Init status bar.\n");
    ST_Init ();

    // If Doom II without a MAP01 lump, this is a store demo.
    // Moved this here so that MAP01 isn't constantly looked up
    // in the main loop.

    if (gamemode == commercial && W_CheckNumForName("map01") < 0)
        storedemo = true;

    // Doom 3: BFG Edition includes modified versions of the classic
    // IWADs which can be identified by an additional DMENUPIC lump.
    // Furthermore, the M_GDHIGH lumps have been modified in a way that
    // makes them incompatible to Vanilla Doom and the modified version
    // of doom2.wad is missing the TITLEPIC lump.
    // We specifically check for DMENUPIC here, before PWADs have been
    // loaded which could probably include a lump of that name.

//    if (gamemode == commercial && W_CheckNumForName("titlepic") < 0)
    if (W_CheckNumForName("dmenupic") >= 0)
    {
//        printf("BFG Edition: Using INTERPIC instead of TITLEPIC.\n");
        bfgedition = true;

        // BFG Edition changes the names of the secret levels to
        // censor the Wolfenstein references. It also has an extra
        // secret level (MAP33). In Vanilla Doom (meaning the DOS
        // version), MAP33 overflows into the Plutonia level names
        // array, so HUSTR_33 is actually PHUSTR_1.

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
    }

    if(nerve_pwad)
	LoadNerveWad();

//    if (M_CheckParmWithArgs("-statdump", 1))
    if(devparm)
    {
        I_AtExit(StatDump, true);
//        DEH_printf("External statistics registered.\n");
    }
/*
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
	G_RecordDemo (myargv[p+1]);
	autostart = true;
    }

    p = M_CheckParmWithArgs("-playdemo", 1);
    if (p)
    {
	singledemo = true;              // quit after one demo
	G_DeferedPlayDemo (demolumpname);
	D_DoomLoop ();  // never returns
    }
	
    p = M_CheckParmWithArgs("-timedemo", 1);
    if (p)
    {
	G_TimeDemo (demolumpname);
	D_DoomLoop ();  // never returns
    }
*/	
    if (startloadgame >= 0)
    {
        strcpy(file, P_SaveGameFile(startloadgame));
	G_LoadGame (file);
    }
	
    if (gameaction != ga_loadgame )
    {
	if (autostart || netgame)
	    G_InitNew (startskill, startepisode, startmap);
	else
	    D_StartTitle ();                // start up intro loop
    }

    D_DoomLoop ();  // never returns
}

