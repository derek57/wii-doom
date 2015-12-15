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


#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "am_map.h"

#ifdef WII
#include "../c_io.h"
#else
#include "c_io.h"
#endif

#include "d_main.h"

#ifdef WII
#include "../d_deh.h"
#else
#include "d_deh.h"
#endif

#include "doomdef.h"

#ifdef WII
#include "../doomfeatures.h"
#include "../doomkeys.h"
#else
#include "doomfeatures.h"
#include "doomkeys.h"
#endif

#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"

#ifdef WII
#include "../i_system.h"
#include "../i_timer.h"
#include "../i_video.h"
#include "../m_argv.h"
#include "../m_controls.h"
#else
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_argv.h"
#include "m_controls.h"
#endif

#include "m_menu.h"

#ifdef WII
#include "../m_misc.h"
#else
#include "m_misc.h"
#endif

#include "m_random.h"
#include "p_local.h" 
#include "p_saveg.h"
#include "p_setup.h"
#include "p_tick.h"

// SKY handling - still the wrong place.
#include "r_data.h"
#include "r_sky.h"

#include "s_sound.h"
#include "sounds.h"
#include "st_stuff.h"

// Needs access to LFB.
#ifdef WII
#include "../v_video.h"
#include "../w_wad.h"
#else
#include "v_video.h"
#include "w_wad.h"
#endif

#include "wi_stuff.h"

#ifdef WII
#include "../z_zone.h"
#else
#include "z_zone.h"
#endif

#ifdef WII
#include <wiiuse/wpad.h>
#endif

#define SAVEGAMESIZE     0x2c000
#define MAXPLMOVE        0x32
#define TURBOTHRESHOLD   0x32
#define MAX_JOY_BUTTONS  20
#define BODYQUESIZE      32
#define KEY_1            0x02
#define VERSIONSIZE      16 
/*
#define DEMOMARKER       0x80

// Version code for cph's longtics hack ("v1.91")
#define DOOM_191_VERSION 111
*/

// DOOM Par Times
int pars[4][10] = 
{ 
    {0}, 
    {0,30,75,120,90,165,180,180,30,165}, 
    {0,90,90,90,120,90,360,240,30,170}, 
    {0,90,45,90,150,90,90,165,30,135} 
}; 

// DOOM II Par Times
int cpars[32] =
{
    30,90,120,120,90,150,120,120,270,90,        //  1-10
    210,150,150,150,210,150,420,150,210,150,    // 11-20
    240,150,180,150,150,300,330,420,300,180,    // 21-30
    120,30                                      // 31-32
};
 
// [crispy] Episode 4 par times from the BFG Edition
static int e4pars[10] =
{
    0,165,255,135,150,180,390,135,360,180
};

// [crispy] No Rest For The Living par times from the BFG Edition
static int npars[9] =
{
    75,105,120,105,210,105,165,105,135
};


// Gamestate the last time G_Ticker was called.

gamestate_t     oldgamestate; 
gamestate_t     gamestate; 

gameaction_t    gameaction; 

skill_t         gameskill; 
skill_t         d_skill; 

player_t        players[MAXPLAYERS]; 

wbstartstruct_t wminfo;                 // parms for world map / intermission 

mobj_t*         bodyque[BODYQUESIZE]; 

fixed_t         forwardmve;
fixed_t         sidemve;
fixed_t         angleturn;              // + slow turn 

fixed_t         low_health_forwardmove = 17;
fixed_t         low_health_sidemove = 15;

int		low_health_turnspeed = 5;

// If non-zero, exit the level after this number of minutes.
int             timelimit;

// mouse values are used once 
int             mousex;
int             mousey;

//int                defdemosize;            // [crispy] demo progress bar
int             d_episode; 
int             d_map; 
int             gameepisode; 
int             gamemap; 
int             starttime;              // for comparative timing purposes
int             consoleplayer;          // player taking events and displaying 
int             displayplayer;          // view being displayed 
int             levelstarttic;          // gametic at level start 
int             totalkills, totalitems, totalsecret;    // for intermission 
int             turnspd;
int             joy_a = 1;              // 0
int             joy_r = 2;              // 1
int             joy_plus = 4;           // 2
int             joy_l = 8;              // 3
int             joy_minus = 16;         // 4
int             joy_b = 32;             // 5
int             joy_left = 64;          // 6
int             joy_down = 128;         // 7
int             joy_right = 256;        // 8
int             joy_up = 512;           // 9
int             joy_zr = 1024;          // 10
int             joy_zl = 2048;          // 11
int             joy_home = 4096;        // 12
int             joy_x = 8192;           // 13
int             joy_y = 16384;          // 14
int             joy_1 = 32768;          // 15
int             joy_2 = 65536;          // 16
int             bodyqueslot; 
int             vanilla_savegame_limit = 0; // FIX FOR THE WII: SAVEGAME BUFFER OVERFLOW (GIBS)
//int             vanilla_demo_limit = 1; 
#ifdef WII
int             joybstrafe;
int             joybinvright = 0;
int             joybfire = 1;
int             joybaiminghelp = 2;
int             joybuse = 3;
int             joybmenu = 4;
int             joybflydown = 5;
int             joybleft = 6;
int             joybmap = 7;
int             joybright = 8;
int             joybcenter = 9;
int             joybmapzoomout = 10;
int             joybmapzoomin = 11;
int             joybjump = 12;
int             joybflyup = 13;
int             joybinvleft = 14;
int             joybspeed = 15;
int             joybconsole = 16;
#endif
dboolean        secret_1 = false;
dboolean        secret_2 = false;
dboolean        secretexit; 
dboolean        respawnmonsters;
dboolean        paused; 
dboolean        sendpause;              // send a pause event next tic 
dboolean        sendsave;               // send a save event next tic 
dboolean        usergame;               // ok to save / end game 
//dboolean        timingdemo;             // if true, exit with report on completion  
dboolean        viewactive; 
dboolean        deathmatch;             // only if started as net death 
dboolean        netgame;                // only true if packets are broadcast 
dboolean        playeringame[MAXPLAYERS]; 

dboolean        demorecording; 
dboolean        demoplayback; 
dboolean        netdemo; 
dboolean        singledemo;             // quit after playing a demo from cmdline  
dboolean        precache = true;        // if true, load all graphics at start 
#ifdef WII
dboolean        joyarray[MAX_JOY_BUTTONS + 1]; 
dboolean        *joybuttons = &joyarray[1]; // allow [-1] 
#endif
dboolean        not_walking;
dboolean        turbodetected[MAXPLAYERS];
dboolean        lowres_turn;            // low resolution turning for longtics
/*
dboolean        longtics;               // cph's doom 1.91 longtics hack
#ifndef WII
dboolean        nodrawers;              // for comparative timing purposes 
#endif

char            *demoname;
char            *defdemoname; 
*/
char            savename[256];
/*
byte*           demobuffer;
byte*           demo_p;
byte*           demoend; 
*/
byte            consistancy[MAXPLAYERS][BACKUPTICS]; 

static dboolean mousearray[MAX_MOUSE_BUTTONS + 1];
static dboolean *mousebuttons = &mousearray[1];  // allow [-1]
static dboolean dclickstate;
static dboolean dclickstate2;

static char     savedescription[32]; 

static int      dclicktime;
static int      dclicks; 
static int      dclicktime2;
static int      dclicks2;
static int      savegameslot; 

// joystick values are repeated 
static int      joyxmove;
static int      joyymove;
static int      joyirx;
static int      joyiry;

#ifndef WII
static int *weapon_keys[] = {
    &key_weapon1,
    &key_weapon2,
    &key_weapon3,
    &key_weapon4,
    &key_weapon5,
    &key_weapon6,
    &key_weapon7,
    &key_weapon8
};

// Set to -1 or +1 to switch to the previous or next weapon.

static int next_weapon = 0;
#endif

// Used for prev/next weapon keys.

static const struct
{
    weapontype_t weapon;
    weapontype_t weapon_num;
} weapon_order_table[] = {
    { wp_fist,            wp_fist },
    { wp_chainsaw,        wp_fist },
    { wp_pistol,          wp_pistol },
    { wp_shotgun,         wp_shotgun },
    { wp_supershotgun,    wp_shotgun },
    { wp_chaingun,        wp_chaingun },
    { wp_missile,         wp_missile },
    { wp_plasma,          wp_plasma },
    { wp_bfg,             wp_bfg }
};

extern char     *mapnumandtitle;

extern int      messageToPrint;

extern dboolean done;
extern dboolean netgameflag;
extern dboolean aiming_help;
extern dboolean messageNeedsInput;
extern dboolean setsizeneeded;
extern dboolean map_flag;
extern dboolean transferredsky;
extern dboolean long_tics;
extern dboolean mouse_grabbed;

extern fixed_t  mtof_zoommul; // how far the window zooms in each tic (map coords)
extern fixed_t  ftom_zoommul; // how far the window zooms in each tic (fb coords)

//extern ticcmd_t *netcmds;

extern menu_t   *currentMenu;                          

extern short    itemOn;       // menu item skull is on

extern char*    pagename; 


void ChangeWeaponRight(void)
{
    static player_t*    plyrweap;
    static event_t      kbevent;

    if (gamestate == GS_LEVEL && !menuactive)
    {
        weapontype_t    num;

        plyrweap = &players[consoleplayer];

        num = plyrweap->readyweapon;

        while (1)
        {
            dont_move_forwards = true;

            num++;

            if (num > wp_supershotgun)
                num = wp_fist;

            if (plyrweap->weaponowned[num])
            {
                plyrweap->pendingweapon = num;

                break;
            }
        }
        kbevent.type = ev_keydown;
        kbevent.data1 = KEY_1 + num;

        D_PostEvent(&kbevent);

        dont_move_forwards = false;
    }
}

void ChangeWeaponLeft(void)
{
    static player_t*    plyrweap;
    static event_t      kbevent;

    if (gamestate == GS_LEVEL && !menuactive)
    {
        weapontype_t    num;

        plyrweap = &players[consoleplayer];

        num = plyrweap->readyweapon;

        while (1)
        {
            dont_move_forwards = true;

            num--;

            if (num == -1)
                num = wp_supershotgun;

            if (plyrweap->weaponowned[num])
            {
                plyrweap->pendingweapon = num;

                break;
            }
        }
        if(num == wp_supershotgun)
            num = wp_fist;

        kbevent.type = ev_keydown;
        kbevent.data1 = KEY_1 + num;

        D_PostEvent(&kbevent);

        dont_move_forwards = false;
    }
}

#ifndef WII
static dboolean WeaponSelectable(weapontype_t weapon)
{
    // Can't select the super shotgun in Doom 1.

    if (weapon == wp_supershotgun && logical_gamemission == doom)
    {
        return false;
    }

    // These weapons aren't available in shareware.

    if ((weapon == wp_plasma || weapon == wp_bfg)
     && gamemission == doom && gamemode == shareware)
    {
        return false;
    }

    // Can't select a weapon if we don't own it.

    if (!players[consoleplayer].weaponowned[weapon])
    {
        return false;
    }

    // Can't select the fist if we have the chainsaw, unless
    // we also have the berserk pack.

    if (weapon == wp_fist
     && players[consoleplayer].weaponowned[wp_chainsaw]
     && !players[consoleplayer].powers[pw_strength])
    {
        return false;
    }

    return true;
}

static int G_NextWeapon(int direction)
{
    weapontype_t weapon;
    int start_i, i;

    // Find index in the table.

    if (players[consoleplayer].pendingweapon == wp_nochange)
    {
        weapon = players[consoleplayer].readyweapon;
    }
    else
    {
        weapon = players[consoleplayer].pendingweapon;
    }

    for (i=0; i<arrlen(weapon_order_table); ++i)
    {
        if (weapon_order_table[i].weapon == weapon)
        {
            break;
        }
    }

    // Switch weapon. Don't loop forever.
    start_i = i;
    do
    {
        i += direction;
        i = (i + arrlen(weapon_order_table)) % arrlen(weapon_order_table);
    } while (i != start_i && !WeaponSelectable(weapon_order_table[i].weapon));

    return weapon_order_table[i].weapon_num;
}
#endif

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer. 
// If recording a demo, write it out 
// 

void G_BuildTiccmd (ticcmd_t* cmd, int maketic) 
{ 
    dboolean     strafe
#ifdef WII
    ,use
#endif
    ;

#ifndef WII
    int         i, speed;
#endif

    int         forward;
    int         side;
    int         look;
    int         flyheight;

    memset(cmd, 0, sizeof(ticcmd_t));

    cmd->consistancy = 
        consistancy[consoleplayer][maketic%BACKUPTICS]; 
 
    strafe = gamekeydown[key_strafe] || 
#ifdef WII
             joybuttons[joybstrafe] ||
#endif
             mousebuttons[mousebstrafe] ||
             mousebuttons[mousebstrafeleft] || mousebuttons[mousebstraferight] ||
             gamekeydown[key_strafeleft] || gamekeydown[key_straferight];

    // fraggle: support the old "joyb_speed = 31" hack which
    // allowed an autorun effect
#ifndef WII
    speed = key_speed >= NUMKEYS || gamekeydown[key_speed];
#endif
    forward = side = look = flyheight = 0;

    // let movement keys cancel each other out
    if (strafe) 
    { 
        if (gamekeydown[key_right] || mousebuttons[mousebstraferight] || gamekeydown[key_straferight]) 
        {
#ifndef WII
            not_walking = false;
#endif
            // fprintf(stderr, "strafe right\n");
            side += sidemve; 
        }
        if (gamekeydown[key_left] || mousebuttons[mousebstrafeleft] || gamekeydown[key_strafeleft]) 
        {
#ifndef WII
            not_walking = false;
#endif
            // fprintf(stderr, "strafe left\n");
            side -= sidemve; 
        }

        if (joyxmove > 0) 
            side += sidemve; 
        if (joyxmove < 0) 
            side -= sidemve; 
    } 
    else 
    { 
        if (gamekeydown[key_right])
            cmd->angleturn -= turnspd * 128; 
        if (gamekeydown[key_left]) 
            cmd->angleturn += turnspd * 128;

        if (joyxmove > 20) 
            side += sidemve; 
        else if (joyxmove < -20) 
            side -= sidemve; 

        if (joyirx > 0)     // calculate wii IR curve based on input
            cmd->angleturn -= turnspd * joyirx;
        if (joyirx < 0)     // calculate wii IR curve based on input
            cmd->angleturn -= turnspd * joyirx;
    } 

    if (joyymove > 20) 
        forward += forwardmve; 
    else if (joyymove < -20) 
        forward -= forwardmve; 

    if (joyxmove > 20 || joyymove > 20 || joyxmove < -20 || joyymove < -20)
        not_walking = false;
    else
        not_walking = true;

//    extern dboolean dont_move_forwards;
//    extern dboolean dont_move_backwards;

    if (gamekeydown[key_up]) 
    {
        // fprintf(stderr, "up\n");
#ifdef WII
        if(dont_move_forwards == true)
#else
        {
            not_walking = false;
        }
#endif
            forward += forwardmve; 
    }
    if (gamekeydown[key_down]) 
    {
        // fprintf(stderr, "down\n");
#ifdef WII
        if(dont_move_backwards == true)
#else
        {
            not_walking = false;
        }
#endif
            forward -= forwardmve; 
    }
#ifdef WII
    if (joybuttons[joybinvright])
        G_ScreenShot();

    if (joybuttons[joybstrafeleft]) 
    {
        side -= sidemve;
    }

    if (joybuttons[joybstraferight])
    {
        side += sidemve; 
    }
#endif
    if (gamekeydown[key_fire] ||
#ifdef WII
        joybuttons[joybfire] ||
#endif
        mousebuttons[mousebfire]) 
        cmd->buttons |= BT_ATTACK; 

    // villsa [STRIFE] disable running if low on health
    if (players[consoleplayer].health <= 15 && lowhealth)
    {
	forwardmve = low_health_forwardmove;
	sidemve = low_health_sidemove;
	turnspd = low_health_turnspeed;
    }
    else
    {
        if (
#ifdef WII
            joybuttons[joybspeed]
#else
               speed
#endif
           )
        {
            forwardmve = forwardmove * 6;
            sidemve = sidemove * 6;
            turnspd = turnspeed * 4;
        }
        else if(
#ifdef WII
                !joybuttons[joybspeed]
#else
                !speed
#endif
           )
        {
            forwardmve = forwardmove;
            sidemve = sidemove;
            turnspd = turnspeed;
        }
    }

    if (forwardmve < -25)
        forwardmve = -25;
    if (forwardmve > 50)
        forwardmve = 50;

    if (sidemve < -24)
        sidemve = -24;
    if (sidemve > 40)
        sidemve = 40;

    if (cmd->angleturn < -1280)
        cmd->angleturn = -1280;
    if (cmd->angleturn > 1280)
        cmd->angleturn = 1280;

    if(mouselook == 0)
        look = -8;

    // Fly up/down/drop keys
    if (
#ifdef WII
        joybuttons[joybflyup] ||
#endif
        gamekeydown[key_flyup])
    {
        flyheight = 5;          // note that the actual flyheight will be twice this
    }
    if (
#ifdef WII
        joybuttons[joybflydown] ||
#endif
        gamekeydown[key_flydown])
    {
        flyheight = -5;
    }

    if ((
#ifdef WII
         joybuttons[joybjump] ||
#endif
         gamekeydown[key_jump] || mousebuttons[mousebjump]) && !menuactive)
    {
        if(!demoplayback)
            cmd->arti |= AFLAG_JUMP;
    }

    if(!demoplayback)
    {
        if(mousebuttons[mousebnextweapon])
            ChangeWeaponRight();

        if(mousebuttons[mousebprevweapon])
            ChangeWeaponLeft();
    }
    // FOR THE WII: UNUSED BUT WORKING
    if((
#ifdef WII
        joybuttons[joybaiminghelp] ||
#endif
        aiming_help) && !demoplayback && devparm)
    {
        player_t* player = &players[consoleplayer];
        P_AimingHelp(player);
    }

#ifdef WII
    WPADData *data = WPAD_Data(0);

    if(data->exp.type == WPAD_EXP_CLASSIC)
    {
        if(data->btns_d)
        {
            if(joybuttons[joybconsole])
            {
                if(!menuactive)
                {
                    if (!consoleactive)
                    {
                        if(done)
                        {
                            if (consoleheight < CONSOLEHEIGHT && consoledirection == -1)
                            {
                                consoleheight = MAX(1, consoleheight);
                                consoledirection = 1;
                                S_StartSound(NULL, sfx_doropn);
                            }
                        }
                    }
                    else
                    {
                        if(done)
                        {
                            C_HideConsole();
                            S_StartSound(NULL, sfx_dorcls);
                        }
                    }
                }
            }

            if(joybuttons[joybmenu])
            {
                if ((consoleheight == 0 || consoleheight == CONSOLEHEIGHT) && !consoleactive)
                {
                    if (!menuactive)
                    {
                        M_StartControlPanel ();
                        S_StartSound(NULL,sfx_swtchn);
                    }
                    else
                    {
                        currentMenu->lastOn = itemOn;
                        M_ClearMenus ();
                        S_StartSound(NULL,sfx_swtchx);
                    }
                
                    if (messageToPrint)
                    {
                        if (messageNeedsInput)
                        {
                            if(joybuttons[joybmenu])
                            {
                                M_ClearMenus ();
                                messageToPrint = 0;
                                menuactive = false;
                                S_StartSound(NULL,sfx_swtchx);
                            }
                        }
                    }
                }
            }

            if(!demoplayback)
            {
                if(joybuttons[joybright])
                    ChangeWeaponRight();

                if(joybuttons[joybleft])
                    ChangeWeaponLeft();

                if(joybuttons[joybmap])
                {
                    if(!menuactive)
                    {
                        if(!consoleactive)
                        {
                            if(usergame)
                            {
                                AM_Toggle();
                            }
                        }
                    }
                }

                if(automapactive)
                {
                    if(joybuttons[joybmapzoomin])
                    {
                        mtof_zoommul = M_ZOOMIN;
                        ftom_zoommul = M_ZOOMOUT;
                    }

                    if(joybuttons[joybmapzoomout])
                    {
                        mtof_zoommul = M_ZOOMOUT;
                        ftom_zoommul = M_ZOOMIN;
                    }
                }
            }
        }
    }

    if(automapactive)
    {
        if(!(joybuttons[joybmapzoomin] || joybuttons[joybmapzoomout]))
        {
            mtof_zoommul = FRACUNIT;
            ftom_zoommul = FRACUNIT;
        }
    }
#endif

    if (gamekeydown[key_use] ||
#ifdef WII
        joybuttons[joybuse] ||
#endif
        mousebuttons[mousebuse])
    { 
        cmd->buttons |= BT_USE;

        // clear double clicks if hit use button 
        dclicks = 0;                   
    } 

    // If the previous or next weapon button is pressed, the
    // next_weapon variable is set to change weapons when
    // we generate a ticcmd.  Choose a new weapon.

#ifndef WII
    if (gamestate == GS_LEVEL && next_weapon != 0)
    {
        i = G_NextWeapon(next_weapon);
        cmd->buttons |= BT_CHANGE;
        cmd->buttons |= i << BT_WEAPONSHIFT;
    }
    else
    {
        // Check weapon keys.

        for (i=0; i<arrlen(weapon_keys); ++i)
        {
            int key = *weapon_keys[i];

            if (gamekeydown[key])
            {
                cmd->buttons |= BT_CHANGE;
                cmd->buttons |= i<<BT_WEAPONSHIFT;
                break;
            }
        }
    }

    next_weapon = 0;
#endif

    // mouse
    if (mousebuttons[mousebforward]) 
    {
        forward += forwardmve;
    }
    if (mousebuttons[mousebbackward])
    {
        forward -= forwardmve;
    }

    if (dclick_use)
    {
        dboolean bstrafe;

        // forward double click
        if (mousebuttons[mousebforward] != dclickstate && dclicktime > 1 ) 
        { 
            dclickstate = mousebuttons[mousebforward]; 
            if (dclickstate) 
                dclicks++; 
            if (dclicks == 2) 
            { 
                cmd->buttons |= BT_USE; 
                dclicks = 0; 
            } 
            else 
                dclicktime = 0; 
        } 
        else 
        { 
            dclicktime += ticdup; 
            if (dclicktime > 20) 
            { 
                dclicks = 0; 
                dclickstate = 0; 
            } 
        }
        
        // strafe double click
        bstrafe =
            mousebuttons[mousebstrafe] 
#ifdef WII
            || joybuttons[joybstrafe]
#endif
            ; 
        if (bstrafe != dclickstate2 && dclicktime2 > 1 ) 
        { 
            dclickstate2 = bstrafe; 
            if (dclickstate2) 
                dclicks2++; 
            if (dclicks2 == 2) 
            { 
                cmd->buttons |= BT_USE; 
                dclicks2 = 0; 
            } 
            else 
                dclicktime2 = 0; 
        } 
        else 
        { 
            dclicktime2 += ticdup; 
            if (dclicktime2 > 20) 
            { 
                dclicks2 = 0; 
                dclickstate2 = 0; 
            } 
        } 
    }
#ifdef WII
    use = (gamekeydown[key_use]);
    if (use != dclickstate2 && dclicktime2 > 1 )
    {
        dclickstate2 = use;
        if (dclickstate2)
            dclicks2++;
        dclicktime2 = 0;
    }
    else
    {
        dclicktime2 += ticdup;
        if (dclicktime2 > 20)
        {
                dclicks2 = 0;
                dclickstate2 = 0;
        }
    }
    if (strafe)                         // FOR PS VITA: SWITCHED THESE TWO
        side += mousex*0.5;             // <--
    else                                //   |
#else                                   //   |
    if(mouse_grabbed)                   //   |
#endif                                  //   |
        cmd->angleturn -= mousex*0x8;   // <--

#ifdef WII
    forward += mousey; 
#else
    if(!mouselook && mousewalk)
        forward += mousey; 
#endif
    // mouselook, but not when paused
#ifdef WII
    if (joyiry && !paused && mouselook > 0 && players[consoleplayer].playerstate == PST_LIVE)
#else
    if (mouse_grabbed && !paused && mouselook > 0 && players[consoleplayer].playerstate == PST_LIVE)
#endif
    {                                           // ...not when paused & if on
        // We'll directly change the viewing pitch of the console player.
        float adj = 0;

        // initialiser added to prevent compiler warning
        float newlookdir = 0;

        if(!menuactive && !demoplayback)
        {
#ifdef WII
            adj = ((joyiry * 0x4) << 16) / (float) 0x80000000*180*110.0/85.0;
#else
            adj = ((mousey * 0x4) << 16) / (float) 0x80000000*180*110.0/85.0;
#endif
        }

//        extern int mspeed;

        // Speed up the X11 mlook a little.
        adj *= mspeed;

        if (mouselook == 1)
            newlookdir = players[consoleplayer].lookdir + adj;
        else if (mouselook == 2)
            newlookdir = players[consoleplayer].lookdir - adj;

        // vertical view angle taken from p_user.c line 249.
        if (newlookdir > 90)
            newlookdir = 90;
        else if (newlookdir < -110)
            newlookdir = -110;

        players[consoleplayer].lookdir = newlookdir;
    }

    mousex = mousey = 0; 

    if (forward > MAXPLMOVE) 
        forward = MAXPLMOVE; 
    else if (forward < -MAXPLMOVE) 
        forward = -MAXPLMOVE; 
    if (side > MAXPLMOVE) 
        side = MAXPLMOVE; 
    else if (side < -MAXPLMOVE) 
        side = -MAXPLMOVE; 
 
    cmd->forwardmov += forward; 
    cmd->sidemov += side;

    if (players[consoleplayer].playerstate == PST_LIVE)
    {
        if (look < 0)
        {
            look += 16;
        }
        cmd->lookfly = look;
    }

    if (flyheight < 0)
    {
        flyheight += 16;
    }
    cmd->lookfly |= flyheight << 4;

    // special buttons
    if (sendpause) 
    { 
        sendpause = false; 
        cmd->buttons = BT_SPECIAL | BTS_PAUSE; 
    } 

    if (sendsave) 
    { 
        sendsave = false; 
        cmd->buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot<<BTS_SAVESHIFT); 
    } 

    // low-res turning
/*
    if (lowres_turn)
    {
        static signed short carry = 0;
        signed short desired_angleturn;

        desired_angleturn = cmd->angleturn + carry;

        // round angleturn to the nearest 256 unit boundary
        // for recording demos with single byte values for turn

        cmd->angleturn = (desired_angleturn + 128) & 0xff00;

        // Carry forward the error from the reduced resolution to the
        // next tic, so that successive small movements can accumulate.

        carry = desired_angleturn - cmd->angleturn;
    }
*/
} 
 
/*
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsizeof-pointer-memaccess"
*/
//
// G_Responder  
// Get info needed to make ticcmd_ts for the players.
// 
dboolean G_Responder (event_t* ev) 
{ 
#ifndef WII
    // allow spy mode changes even during the demo
    if (gamestate == GS_LEVEL && ev->type == ev_keydown 
     && ev->data1 == key_spy && (singledemo || !deathmatch) )
    {
        // spy mode 
        do 
        { 
            displayplayer++; 
            if (displayplayer == MAXPLAYERS) 
                displayplayer = 0; 
        } while (!playeringame[displayplayer] && displayplayer != consoleplayer); 
        return true; 
    }

    // any other key pops up menu if in demos
    if (gameaction == ga_nothing && !singledemo && 
        (demoplayback || gamestate == GS_DEMOSCREEN) 
        ) 
    { 
        if ((ev->type == ev_keydown && ev->data1 != KEY_RSHIFT) ||  
            (ev->type == ev_mouse && ev->data1) || 
            (ev->type == ev_joystick && ev->data1) ) 
        { 
            M_StartControlPanel (); 
            return true; 
        } 
        return false; 
    } 
#endif
    if (gamestate == GS_LEVEL) 
    { 
#if 0 
        if (devparm && ev->type == ev_keydown && ev->data1 == ';') 
        { 
            G_DeathMatchSpawnPlayer (0); 
            return true; 
        } 
#endif 
/*
        if (HU_Responder (ev)) 
            return true;        // chat ate the event 
*/
        if (ST_Responder (ev)) 
            return true;        // status window ate it 
        if (AM_Responder (ev)) 
            return true;        // automap ate it 
    } 
         
    if (gamestate == GS_FINALE) 
    { 
        if (F_Responder (ev)) 
            return true;        // finale ate the event 
    } 

    // If the next/previous weapon keys are pressed, set the next_weapon
    // variable to change weapons when the next ticcmd is generated.
#ifndef WII
    if((ev->type == ev_keydown && ev->data1 == key_prevweapon) ||
        mousebuttons[mousebprevweapon])
    {
        next_weapon = -1;
    }
    else if((ev->type == ev_keydown && ev->data1 == key_nextweapon) ||
        mousebuttons[mousebnextweapon])
    {
        next_weapon = 1;
    }
#endif
    switch (ev->type) 
    { 
      case ev_keydown: 
        if (ev->data1 == key_pause) 
        { 
            sendpause = true; 
        }
        else if (ev->data1 <NUMKEYS) 
        {
            gamekeydown[ev->data1] = true; 
        }

        return true;    // eat key down events 
 
      case ev_keyup: 
        if (ev->data1 <NUMKEYS) 
        {
#ifndef WII
            not_walking = true;
#endif
            gamekeydown[ev->data1] = false; 
        }
        return false;   // always let key up events filter down 

      case ev_mouse:
#ifndef WII
        mousebuttons[0] = (ev->data1 & 1) > 0;
        mousebuttons[1] = (ev->data1 & 2) > 0;
        mousebuttons[2] = (ev->data1 & 4) > 0;
        mousebuttons[3] = (ev->data1 & 8) > 0;
        mousebuttons[4] = (ev->data1 & 16) > 0;
        if (!automapactive && !menuactive && !paused)
        {
            if (mousebnextweapon < MAX_MOUSE_BUTTONS && mousebuttons[mousebnextweapon])
                next_weapon = 1;
            else if (mousebprevweapon < MAX_MOUSE_BUTTONS && mousebuttons[mousebprevweapon])
                next_weapon = -1;
        }
#endif
        mousex = ev->data2*(mouseSensitivity+5)/10; 
        mousey = ev->data3*(mouseSensitivity+5)/10; 
        return true;    // eat events 
#ifndef WII
      case ev_mousewheel:
        if (!automapactive && !menuactive && !paused)
        {
            if (ev->data1 < 0)
            {
                if (mousebnextweapon == MOUSE_WHEELDOWN)
                    next_weapon = 1;
                else if (mousebprevweapon == MOUSE_WHEELDOWN)
                    next_weapon = -1;
            }
            else if (ev->data1 > 0)
            {
                if (mousebnextweapon == MOUSE_WHEELUP)
                    next_weapon = 1;
                else if (mousebprevweapon == MOUSE_WHEELUP)
                    next_weapon = -1;
            }
        }
        return true;
#else
      case ev_joystick: 
        joybuttons[0] = (ev->data1 & joy_x) > 0;
        joybuttons[1] = (ev->data1 & joy_r) > 0;
        joybuttons[2] = (ev->data1 & joy_plus) > 0;
        joybuttons[3] = (ev->data1 & joy_l) > 0;
        joybuttons[4] = (ev->data1 & joy_minus) > 0;
        joybuttons[5] = (ev->data1 & joy_y) > 0;
        joybuttons[6] = (ev->data1 & joy_left) > 0;
        joybuttons[7] = (ev->data1 & joy_down) > 0;
        joybuttons[8] = (ev->data1 & joy_right) > 0;
        joybuttons[9] = (ev->data1 & joy_up) > 0;
        joybuttons[10] = (ev->data1 & joy_zr) > 0;
        joybuttons[11] = (ev->data1 & joy_zl) > 0;
        joybuttons[12] = (ev->data1 & joy_b) > 0;
        joybuttons[13] = (ev->data1 & joy_a) > 0;
        joybuttons[14] = (ev->data1 & joy_home) > 0;
        joybuttons[15] = (ev->data1 & joy_1) > 0;
        joybuttons[16] = (ev->data1 & joy_2) > 0;
        joyxmove = ev->data2; 
        joyymove = ev->data3; 
        joyirx = ev->data4;
        joyiry = ev->data5;
        return true;    // eat events 
#endif
      default: 
        break; 
    } 
 
    return false; 
} 
 
 
 
//
// DEMO RECORDING 
// 
/*
void G_ReadDemoTiccmd (ticcmd_t* cmd) 
{ 
    if (*demo_p == DEMOMARKER) 
    {
        // end of demo data stream 
        G_CheckDemoStatus (); 
        return; 
    } 
    cmd->forwardmove = ((signed char)*demo_p++); 
    cmd->sidemove = ((signed char)*demo_p++); 

    // If this is a longtics demo, read back in higher resolution

#ifndef WII
    if (longtics)
    {
        cmd->angleturn = *demo_p++;
        cmd->angleturn |= (*demo_p++) << 8;
    }
    else
#endif
    {
        cmd->angleturn = ((unsigned char) *demo_p++)<<8; 
    }

    cmd->buttons = (unsigned char)*demo_p++; 

//    cmd->lookfly = (unsigned char) *demo_p++;
} 

// Increase the size of the demo buffer to allow unlimited demos

static void IncreaseDemoBuffer(void)
{
    int current_length;
    byte *new_demobuffer;
    byte *new_demop;
    int new_length;

    // Find the current size

    current_length = demoend - demobuffer;
    
    // Generate a new buffer twice the size
    new_length = current_length * 2;
    
    new_demobuffer = Z_Malloc(new_length, PU_STATIC, 0);
    new_demop = new_demobuffer + (demo_p - demobuffer);

    // Copy over the old data

    memcpy(new_demobuffer, demobuffer, current_length);

    // Free the old buffer and point the demo pointers at the new buffer.

    Z_Free(demobuffer);

    demobuffer = new_demobuffer;
    demo_p = new_demop;
    demoend = demobuffer + new_length;
}

void G_WriteDemoTiccmd (ticcmd_t* cmd) 
{ 
    byte *demo_start;
#ifndef WII
    if (gamekeydown[key_demo_quit])           // press q to end demo recording 
        G_CheckDemoStatus (); 
#endif
    demo_start = demo_p;

    *demo_p++ = cmd->forwardmove; 
    *demo_p++ = cmd->sidemove; 
#ifndef WII
    // If this is a longtics demo, record in higher resolution
    if (longtics)
    {
        *demo_p++ = (cmd->angleturn & 0xff);
        *demo_p++ = (cmd->angleturn >> 8) & 0xff;
    }
    else
    {
        *demo_p++ = cmd->angleturn >> 8; 
    }
#else
    *demo_p++ = cmd->angleturn >> 8; 
#endif
    *demo_p++ = cmd->buttons; 
    // reset demo pointer back
    demo_p = demo_start;

    if (demo_p > demoend - 16)
    {
        if (vanilla_demo_limit)
        {
            // no more space 
            G_CheckDemoStatus (); 
            return; 
        }
        else
        {
            // Vanilla demo limit disabled: unlimited
            // demo lengths!

            IncreaseDemoBuffer();
        }
    } 
        
    G_ReadDemoTiccmd (cmd);         // make SURE it is exactly the same 
} 
*/ 
//
// G_CheckSpot  
// Returns false if the player cannot be respawned
// at the given mapthing_t spot  
// because something is occupying it 
//
 
dboolean
G_CheckSpot
( int                playernum,
  mapthing_t*        mthing ) 
{ 
    fixed_t          x;
    fixed_t          y; 
    subsector_t*     ss; 
    mobj_t*          mo; 
        
    if (!players[playernum].mo)
    {
        int i;

        // first spawn of level, before corpses
        for (i=0 ; i<playernum ; i++)
            if (players[i].mo->x == mthing->x << FRACBITS
                && players[i].mo->y == mthing->y << FRACBITS)
                return false;        
        return true;
    }
                
    x = mthing->x << FRACBITS; 
    y = mthing->y << FRACBITS; 
         
    if (!P_CheckPosition (players[playernum].mo, x, y) ) 
        return false; 
 
    // flush an old corpse if needed 
    if (bodyqueslot >= BODYQUESIZE) 
        P_RemoveMobj (bodyque[bodyqueslot%BODYQUESIZE]); 
    bodyque[bodyqueslot%BODYQUESIZE] = players[playernum].mo; 
    bodyqueslot++; 

    // spawn a teleport fog
    ss = R_PointInSubsector (x,y);


    // The code in the released source looks like this:
    //
    //    an = ( ANG45 * (((unsigned int) mthing->angle)/45) )
    //         >> ANGLETOFINESHIFT;
    //    mo = P_SpawnMobj (x+20*finecosine[an], y+20*finesine[an]
    //                     , ss->sector->floorheight
    //                     , MT_TFOG);
    //
    // But 'an' can be a signed value in the DOS version. This means that
    // we get a negative index and the lookups into finecosine/finesine
    // end up dereferencing values in finetangent[].
    // A player spawning on a deathmatch start facing directly west spawns
    // "silently" with no spawn fog. Emulate this.
    //
    // This code is imported from PrBoom+.

    {
        fixed_t xa, ya;
        signed int an;

        // This calculation overflows in Vanilla Doom, but here we deliberately
        // avoid integer overflow as it is undefined behavior, so the value of
        // 'an' will always be positive.
        an = (ANG45 >> ANGLETOFINESHIFT) * ((signed int) mthing->angle / 45);

        switch (an)
        {
            case 4096:  // -4096:
                xa = finetangent[2048];    // finecosine[-4096]
                ya = finetangent[0];       // finesine[-4096]
                break;
            case 5120:  // -3072:
                xa = finetangent[3072];    // finecosine[-3072]
                ya = finetangent[1024];    // finesine[-3072]
                break;
            case 6144:  // -2048:
                xa = finesine[0];          // finecosine[-2048]
                ya = finetangent[2048];    // finesine[-2048]
                break;
            case 7168:  // -1024:
                xa = finesine[1024];       // finecosine[-1024]
                ya = finetangent[3072];    // finesine[-1024]
                break;
            case 0:
            case 1024:
            case 2048:
            case 3072:
                xa = finecosine[an];
                ya = finesine[an];
                break;
            default:
                I_Error("G_CheckSpot: unexpected angle %d\n", an);
                xa = ya = 0;
                break;
        }
        mo = P_SpawnMobj(x + 20 * xa, y + 20 * ya,
                         ss->sector->floorheight, MT_TFOG);
    }

    if (players[consoleplayer].viewz != 1) 
        S_StartSound (mo, sfx_telept);        // don't start sound on first frame 
 
    return true; 
} 

//
// G_DoReborn 
// 
void G_DoReborn (int playernum) 
{ 
    if (!netgame)
    {
        // reload the level from scratch
        gameaction = ga_loadlevel;  
    }
    else 
    {
        int i; 
         
        // respawn at the start

        // first dissasociate the corpse 
        players[playernum].mo->player = NULL;   
                 
        // spawn at random spot if in death match 
        if (deathmatch) 
        { 
            G_DeathMatchSpawnPlayer (playernum); 
            return; 
        } 
                 
        if (G_CheckSpot (playernum, &playerstarts[playernum]) ) 
        { 
            P_SpawnPlayer (&playerstarts[playernum]); 
            return; 
        }
        
        // try to spawn at one of the other players spots 
        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (G_CheckSpot (playernum, &playerstarts[i]) ) 
            { 
                playerstarts[i].type = playernum+1;     // fake as other player 
                P_SpawnPlayer (&playerstarts[i]); 
                playerstarts[i].type = i+1;             // restore 
                return; 
            } 
            // he's going to be inside something.  Too bad.
        }
        P_SpawnPlayer (&playerstarts[playernum]); 
    } 
} 

//
// G_DoLoadLevel 
//
void G_DoLoadLevel (void) 
{ 
    int         i, ep; 
    int         map = (gameepisode - 1) * 10 + gamemap;
    char        *author = P_GetMapAuthor(map);

    // Set the sky map.
    // First thing, we have a dummy sky texture name,
    //  a flat. The data is in the WAD only because
    //  we look for an actual index, instead of simply
    //  setting one.

    skyflatnum = R_FlatNumForName(SKYFLATNAME);

    // The "Sky never changes in Doom II" bug was fixed in
    // the id Anthology version of doom2.exe for Final Doom.
/*
    if ((gamemode == commercial) && (gameversion == exe_final2 || gameversion == exe_chex))
    {
        char *skytexturename;

        if (gamemap < 12)
        {
            skytexturename = "SKY1";
        }
        else if (gamemap < 21)
        {
            skytexturename = "SKY2";
        }
        else
        {
            skytexturename = "SKY3";
        }

        skytexture = R_TextureNumForName(skytexturename);
    }
*/
    skytexture = P_GetMapSky1Texture(map);
    if (!skytexture || skytexture == R_CheckTextureNumForName("SKY1TALL"))
    {
        if ((gamemode == commercial) && (gameversion == exe_final2 || gameversion == exe_chex))
        {
            skytexture = R_TextureNumForName("SKY3");
            if (gamemap < 12)
                 skytexture = R_TextureNumForName("SKY1");
            else if (gamemap < 21)
                 skytexture = R_TextureNumForName("SKY2");
        }
        else
        {
            switch (gameepisode)
            {
                default:
                case 1:
                    skytexture = R_TextureNumForName("SKY1");
                    break;
                case 2:
                    skytexture = R_TextureNumForName("SKY2");
                    break;
                case 3:
                    skytexture = R_TextureNumForName("SKY3");
                    break;
                case 4:                             // Special Edition sky
                    skytexture = R_TextureNumForName("SKY4");
                    break;
            }
        }
    }
    skyscrolldelta = P_GetMapSky1ScrollDelta(map);

    levelstarttic = gametic;        // for time calculation
    
    if (wipegamestate == GS_LEVEL) 
        wipegamestate = -1;             // force a wipe 

    gamestate = GS_LEVEL; 

    for (i=0 ; i<MAXPLAYERS ; i++) 
    { 
        turbodetected[i] = false;
        if (playeringame[i] && players[i].playerstate == PST_DEAD) 
            players[i].playerstate = PST_REBORN; 
        memset (players[i].frags,0,sizeof(players[i].frags)); 
    } 

    // initialize the msecnode_t freelist. phares 3/25/98
    // any nodes in the freelist are gone by now, cleared
    // by Z_FreeTags() when the previous level ended or player
    // died.
    P_FreeSecNodeList();

    ep = (gamemode == commercial ? (gamemission == pack_nerve ? 2 : 1) : gameepisode);

    if (author[0])
        C_Output("%s by %s", mapnumandtitle, author);
    else
        C_Output(mapnumandtitle);

    if(beta_style && ep == 1 && gamemap == 3)
            I_Error("W_GetNumForName: E1M3 not found!");

    P_SetupLevel (ep, gamemap);

    skycolfunc = (canmodify && (textureheight[skytexture] >> FRACBITS) == 128 && !transferredsky
        && (gamemode != commercial || gamemap < 21) ? R_DrawFlippedSkyColumn : R_DrawSkyColumn);

    displayplayer = consoleplayer;                // view the guy you are playing    
    gameaction = ga_nothing; 

#if defined WII || defined BOOM_ZONE_HANDLING
    Z_CheckHeap ();
#endif

    // clear cmd building stuff

    memset (gamekeydown, 0, sizeof(gamekeydown)); 
    joyxmove = joyymove = joyirx = joyiry = 0; 
    mousex = mousey = 0; 
    sendpause = sendsave = paused = false; 

    memset(mousearray, 0, sizeof(mousearray));
#ifdef WII
    memset(joyarray, 0, sizeof(joyarray)); 
#endif
    if (consoleactive)
        C_HideConsoleFast();
} 

void G_DoNewGame (void) 
{
    demoplayback = false; 
    netdemo = false;
    netgame = false;

    deathmatch = false;

    playeringame[1] = playeringame[2] = playeringame[3] = 0;

    respawnparm = start_respawnparm;
    fastparm = start_fastparm;

    if(!not_monsters)
        nomonsters = false;
    else
        nomonsters = true;

    consoleplayer = 0;
    G_InitNew (d_skill, d_episode, d_map); 
    gameaction = ga_nothing; 
    infight = false;
} 

// Generate a string describing a demo version
/*
static char *DemoVersionDescription(int version)
{
    static char resultbuf[16];

    switch (version)
    {
        case 104:
            return "v1.4";
        case 105:
            return "v1.5";
        case 106:
            return "v1.6/v1.666";
        case 107:
            return "v1.7/v1.7a";
        case 108:
            return "v1.8";
        case 109:
            return "v1.9";
        default:
            break;
    }

    // Unknown version.  Perhaps this is a pre-v1.4 IWAD?  If the version
    // byte is in the range 0-4 then it can be a v1.0-v1.2 demo.

    if (version >= 0 && version <= 4)
    {
        return "v1.0/v1.1/v1.2";
    }
    else
    {
        sprintf(resultbuf, "%i.%i (unknown)", version / 100, version % 100);
        return resultbuf;
    }
}

// Get the demo version code appropriate for the version set in gameversion.
int G_VanillaVersionCode(void)
{
    switch (gameversion)
    {
        case exe_doom_1_2:
            I_Error("Doom 1.2 does not have a version code!");
        case exe_doom_1_666:
            return 106;
        case exe_doom_1_7:
            return 107;
        case exe_doom_1_8:
            return 108;
        case exe_doom_1_9:
        default:  // All other versions are variants on v1.9:
            return 109;
    }
}

void G_DoPlayDemo (void) 
{ 
    skill_t   skill; 
    int       i, episode, map; 

//#ifndef WII
    int       demoversion;
//#endif

    gameaction = ga_nothing; 
    demobuffer = demo_p = W_CacheLumpName (defdemoname, PU_STATIC); 

    // THESE ARE PRIOR VERSION 1.2
    if (fsize == 4261144  || fsize == 4271324  || fsize == 4211660  ||
        fsize == 10401760 || fsize == 11159840 || fsize == 12408292 ||
        fsize == 12474561 || fsize == 12487824 || fsize == 12538385 ||
        fsize == 4234124  || fsize == 4196020  || fsize == 14943400 ||
        fsize == 14824716 || fsize == 14612688 || fsize == 14607420 ||
        fsize == 14604584 || fsize == 14677988 || fsize == 14691821 ||
        fsize == 14683458 || fsize == 18195736 || fsize == 18654796 ||
        fsize == 18240172 || fsize == 17420824 || fsize == 28422764 ||
        fsize == 12361532 || fsize == 19321722)
    {
        // [crispy] demo progress bar
        defdemosize = 0;
        while (*demo_p++ != DEMOMARKER)
        {
            defdemosize++;
        }
        demo_p = demobuffer;

        demoversion = *demo_p++;

        if (demoversion == G_VanillaVersionCode())
        {
            longtics = false;
        }
        else if (demoversion == DOOM_191_VERSION)
        {
            // demo recorded with cph's modified "v1.91" doom exe
            longtics = true;
        }
        else
        {
            C_Error(" Demo is from a different game version!");
            C_Error(" (read %i, should be %i)", demoversion, G_VanillaVersionCode());
            C_Error(" *** You may need to upgrade your version of Doom to v1.9. ***");
            C_Error(" See: http://doomworld.com/files/patches.shtml");
            C_Error(" This appears to be %s.", DemoVersionDescription(demoversion));
            gameaction = ga_nothing;
            return;
        }
    }

    skill = *demo_p++; 
    episode = *demo_p++; 
    map = *demo_p++; 

    if (fsize == 4261144  || fsize == 4271324  || fsize == 4211660  ||
        fsize == 10401760 || fsize == 11159840 || fsize == 12408292 ||
        fsize == 12474561 || fsize == 12487824 || fsize == 12538385 ||
        fsize == 4234124  || fsize == 4196020  || fsize == 14943400 ||
        fsize == 14824716 || fsize == 14612688 || fsize == 14607420 ||
        fsize == 14604584 || fsize == 14677988 || fsize == 14691821 ||
        fsize == 14683458 || fsize == 18195736 || fsize == 18654796 ||
        fsize == 18240172 || fsize == 17420824 || fsize == 28422764 ||
        fsize == 12361532 || fsize == 19321722)
    {
        deathmatch = *demo_p++;
        respawnparm = *demo_p++;
        fastparm = *demo_p++;
        nomonsters = *demo_p++;
        consoleplayer = *demo_p++;
    }
        
    for (i=0 ; i<MAXPLAYERS ; i++) 
        playeringame[i] = *demo_p++; 

    if (playeringame[1]
#ifndef WII
        || M_CheckParm("-solo-net") > 0
        || M_CheckParm("-netdemo") > 0
#endif
       )
    { 
        netgame = true; 
        netdemo = true; 
    }

    // don't spend a lot of time in loadlevel 
    precache = false;
    G_InitNew (skill, episode, map); 
    precache = true; 
    starttime = I_GetTime (); 

    usergame = false; 
    demoplayback = true; 
} 
*/

//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel (int player) 
{ 
    player_t*  p; 
         
    p = &players[player]; 
         
    memset (p->powers, 0, sizeof (p->powers)); 
    memset (p->cards, 0, sizeof (p->cards)); 
    p->mo->flags &= ~MF_SHADOW; // cancel invisibility 
    p->extralight = 0;          // cancel gun flashes 
    p->fixedcolormap = 0;       // cancel ir gogles 
    p->damagecount = 0;         // no palette changes 
    p->bonuscount = 0; 
    p->lookdir = 0;
} 
 

//
// G_DoCompleted 
//

void G_DoCompleted (void) 
{ 
    int         i;          
    int         map = (gameepisode - 1) * 10 + gamemap;
    int         nextmap = P_GetMapNext(map);
    int         par = P_GetMapPar(map);
    int         secretnextmap = P_GetMapSecretNext(map);

    gameaction = ga_nothing; 
 
    for (i=0 ; i<MAXPLAYERS ; i++) 
        if (playeringame[i]) 
            G_PlayerFinishLevel (i);        // take away cards and stuff 
         
    if (automapactive) 
        AM_Stop (); 
        
    if (gamemode != commercial)
    {
        // Chex Quest ends after 5 levels, rather than 8.

        if (gameversion == exe_chex)
        {
            if (gamemap == 5)
            {
                gameaction = ga_victory;
                return;
            }
        }
        else
        {
            switch(gamemap)
            {
              case 8:
                gameaction = ga_victory;
                return;
              case 9: 
                for (i=0 ; i<MAXPLAYERS ; i++) 
                    players[i].didsecret = true; 
                secret_2 = true;
                break;
              case 10: 
                for (i=0 ; i<MAXPLAYERS ; i++) 
                    players[i].didsecret = true; 
                secret_1 = true;
                break;
            }
        }
    }

//#if 0  Hmmm - why?
    if ( (gamemap == 8)
         && (gamemode != commercial) ) 
    {
        // victory 
        gameaction = ga_victory; 
        return; 
    } 

    if ( (gamemap == 9 || (fsize == 12538385 && gameepisode == 1 && gamemap == 10))
         && (gamemode != commercial) ) 
    {
        // exit secret level 
        for (i=0 ; i<MAXPLAYERS ; i++) 
            players[i].didsecret = true; 

        if(gamemap == 9)
            secret_2 = true;
        else if(gamemap == 10)
            secret_1 = true;
    } 
//#endif
    
         
    wminfo.didsecret = players[consoleplayer].didsecret; 
    wminfo.epsd = gameepisode -1; 
    wminfo.last = gamemap -1;
    
    // wminfo.next is 0 biased, unlike gamemap
    if ( gamemission == pack_nerve )
    {
        if (secretexit)
        {
            switch(gamemap)
            {
              case  4:
                  wminfo.next = 8;
                  break;
            }
        }
        else
        {
            switch(gamemap)
            {
              case  9:
                  wminfo.next = 4;
                  break;
              default:
                  wminfo.next = gamemap;
            }
        }
    }
    else if ( gamemission == pack_master)
    {
        wminfo.next = gamemap;
    }
    else if (secretexit && secretnextmap)
        wminfo.next = secretnextmap - 1;
    else if (nextmap)
        wminfo.next = nextmap - 1;
    else if (gamemode == commercial)
    {
        if (secretexit)
            switch(gamemap)
            {
              if(fsize == 14677988 || fsize == 14683458)
              {
                case 2: wminfo.next = 32; break;
              }
              case 15: wminfo.next = 30; break;
              case 31: wminfo.next = 31; break;
            }
        else
            switch(gamemap)
            {
              if(fsize == 14677988 || fsize == 14683458)
              {
                case 33: wminfo.next = 2; break;
              }
              case 31:
              case 32: wminfo.next = 15; break;
              default: wminfo.next = gamemap;
            }
    }
    else
    {
        if (secretexit) 
        {
            if(fsize == 12538385 && gameepisode == 1 && gamemap == 1)
                wminfo.next = 9;         // go to secret level (IT SHOULD BE 9 / NOT 10)
            else
                wminfo.next = 8;         // go to secret level 
        }
        else if (gamemap == 10)
            wminfo.next = 1;
        else if (gamemap == 9) 
        {
            // returning from secret level 
            switch (gameepisode) 
            { 
              case 1: 
                wminfo.next = 3; 
                break; 
              case 2: 
                wminfo.next = 5; 
                break; 
              case 3: 
                wminfo.next = 6; 
                break; 
              case 4:
                wminfo.next = 2;
                break;
            }                
        } 
        else 
            wminfo.next = gamemap;          // go to next level 
    }
                 
    wminfo.maxkills = totalkills; 
    wminfo.maxitems = totalitems; 
    wminfo.maxsecret = totalsecret; 
    wminfo.maxfrags = 0; 

    // Set par time. Doom episode 4 doesn't have a par time, so this
    // overflows into the cpars array. It's necessary to emulate this
    // for statcheck regression testing.
/*
    if (gamemap == 33 || (gameepisode == 1 && gamemap == 10) || gamemission == pack_master)
        // map 33 par time sucks
        wminfo.partime = INT_MAX;
    else if (gamemission == pack_nerve)
        wminfo.partime = TICRATE*npars[gamemap-1];
    else if (gamemode == commercial)
        wminfo.partime = TICRATE*cpars[gamemap-1];
    else if (gameepisode < 4 && gameepisode != 1 && gamemap != 10)
        wminfo.partime = TICRATE*pars[gameepisode][gamemap];
    else if (gameepisode == 4)
        wminfo.partime = TICRATE*e4pars[gamemap];
    else
        wminfo.partime = TICRATE*cpars[gamemap];
*/
    if (par)
        wminfo.partime = TICRATE * par;
    else
    {
        char lump[5];

        // [BH] have no par time if this level is from a PWAD
        if (gamemode == commercial)
            M_snprintf(lump, sizeof(lump), "MAP%02i", gamemap);
        else
        {
            M_snprintf(lump, sizeof(lump), "E%iM%i", gameepisode, gamemap);

            if(gameepisode == 1 && gamemap == 10)
                wminfo.partime = INT_MAX;
        }

        if (BTSX || (W_CheckMultipleLumps(lump) > 1 && (!nerve_pwad || gamemap > 9) && fsize != 28422764))
            wminfo.partime = 0;
        else if (gamemode == commercial)
        {
            // [BH] get correct par time for No Rest For The Living
            //  and have no par time for TNT and Plutonia
            if (gamemission == pack_nerve && gamemap <= 9)
                wminfo.partime = TICRATE * npars[gamemap - 1];
            else if (gamemission == pack_tnt || gamemission == pack_plut)
                wminfo.partime = 0;
            else if (gamemap == 33 || gamemission == pack_master)
                // map 33 par time sucks
                wminfo.partime = INT_MAX;
            else
                wminfo.partime = TICRATE * cpars[gamemap - 1];
        }
        else if (gameepisode < 4 && gameepisode != 1 && gamemap != 10)
            wminfo.partime = TICRATE * pars[gameepisode][gamemap];
        else if (gameepisode == 4)
            wminfo.partime = TICRATE * e4pars[gamemap];
/*
        else
            wminfo.partime = TICRATE * pars[gameepisode][gamemap];
*/
    }
/*
    if (modifiedgame ||
           (gamemode == commercial && (gamemission == pack_tnt || gamemission == pack_plut)))
        wminfo.partime = 0;
*/
    wminfo.pnum = consoleplayer; 
 
    for (i=0 ; i<MAXPLAYERS ; i++) 
    { 
        wminfo.plyr[i].in = playeringame[i]; 
        wminfo.plyr[i].skills = players[i].killcount; 
        wminfo.plyr[i].sitems = players[i].itemcount; 
        wminfo.plyr[i].ssecret = players[i].secretcount; 
        wminfo.plyr[i].stime = leveltime; 
        memcpy (wminfo.plyr[i].frags, players[i].frags 
                , sizeof(wminfo.plyr[i].frags)); 
    } 
 
    if(beta_style)
    {
        if(gameepisode == 3 && gamemap == 5)
            I_Error("W_GetNumForName: E2M6 not found!");
        else if(gameepisode == 2 && gamemap == 2)
            I_Error("W_GetNumForName: E3M3 not found!");
    }

    gamestate = GS_INTERMISSION; 
    viewactive = false; 
    automapactive = false; 
 
    WI_Start (&wminfo); 
} 

void G_DoWorldDone (void) 
{        
    gamestate = GS_LEVEL; 
    gamemap = wminfo.next+1; 
    G_DoLoadLevel (); 
    gameaction = ga_nothing; 
    viewactive = true; 
} 

void G_DoSaveGame (void) 
{ 
    char        *savegame_file = (consoleactive ? savename : P_SaveGameFile(savegameslot));
    char        *temp_savegame_file = P_TempSaveGameFile();

    // Open the savegame file for writing.  We write to a temporary file
    // and then rename it at the end if it was successfully written.
    // This prevents an existing savegame from being overwritten by 
    // a corrupted one, or if a savegame buffer overrun occurs.
    save_stream = fopen(temp_savegame_file, "wb");

    if (!save_stream)
        return;

    savegame_error = false;

    P_WriteSaveGameHeader(savedescription);
 
    // [crispy] some logging when saving
    {
	const int time = leveltime / TICRATE;

        if(gamemode == commercial)
            C_Error(" G_DoSaveGame: Map %d, Skill %d, Time %d:%02d.",
	            gamemap, gameskill, time/60, time%60);
        else
            C_Error(" G_DoSaveGame: Episode %d, Map %d, Skill %d, Time %d:%02d.",
	            gameepisode, gamemap, gameskill, time/60, time%60);
    }

    P_ArchivePlayers (); 
    P_ArchiveWorld (); 
    P_ArchiveThinkers (); 
    P_ArchiveSpecials (); 
//    P_ArchiveMap();
         
    P_WriteSaveGameEOF();
         
    // Enforce the same savegame size limit as in Vanilla Doom, 
    // except if the vanilla_savegame_limit setting is turned off.

    if (vanilla_savegame_limit && ftell(save_stream) > SAVEGAMESIZE)
    {
        I_Error ("Savegame buffer overrun");
    }
    
    // Finish up, close the savegame file.

    fclose(save_stream);

    // Now rename the temporary savegame file to the actual savegame
    // file, overwriting the old savegame if there was one there.

    remove(savegame_file);
    rename(temp_savegame_file, savegame_file);
    
    if (consoleactive)
        C_Warning(" %s saved.", uppercase(savename));
    else
    {
        static char     buffer[1024];

        M_snprintf(buffer, sizeof(buffer), s_GGSAVED, titlecase(savedescription));
        HU_PlayerMessage(buffer, false);
    }

    gameaction = ga_nothing; 
    M_StringCopy(savedescription, "", sizeof(savedescription));

    players[consoleplayer].message = s_GGSAVED;

    // draw the pattern into the back screen
    R_FillBackScreen ();
} 

//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker (void) 
{ 
    int        i;
    int        buf; 
    ticcmd_t*  cmd;
    
    // do player reborns if needed
    for (i=0 ; i<MAXPLAYERS ; i++) 
        if (playeringame[i] && players[i].playerstate == PST_REBORN) 
            G_DoReborn (i);
    
    P_MapEnd();

    // do things to change the game state
    while (gameaction != ga_nothing) 
    { 
        switch (gameaction) 
        { 
          case ga_loadlevel: 
            G_DoLoadLevel (); 
            break; 
          case ga_newgame: 
            G_DoNewGame (); 
            break; 
          case ga_loadgame: 
            G_DoLoadGame (); 
            break; 
          case ga_savegame: 
            G_DoSaveGame (); 
            break; 
          case ga_playdemo: 
//            G_DoPlayDemo (); 
            break; 
          case ga_completed: 
            G_DoCompleted (); 
            break; 
          case ga_victory: 
            F_StartFinale (); 
            break; 
          case ga_worlddone: 
            G_DoWorldDone (); 
            break; 
          case ga_screenshot: 
#ifdef WII
            if(usb)
                V_ScreenShot("usb:/apps/wiidoom/screenshots/DOOM%02i.%s"); 
            else if(sd)
                V_ScreenShot("sd:/apps/wiidoom/screenshots/DOOM%02i.%s"); 
#else
            V_ScreenShot("DOOM%02i.%s"); 
#endif
            players[consoleplayer].message = "screen shot";
            gameaction = ga_nothing; 
            break; 
          case ga_nothing: 
            break; 
        } 
    }
    
    // get commands, check consistancy,
    // and build new consistancy check
    buf = (gametic/ticdup)%BACKUPTICS; 
 
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i]) 
        { 
            cmd = &players[i].cmd; 

            memcpy(cmd, &netcmds[i], sizeof(ticcmd_t));
/*
            if (demoplayback) 
                G_ReadDemoTiccmd (cmd); 
            if (demorecording) 
                G_WriteDemoTiccmd (cmd);
*/
#ifndef WII
            // check for turbo cheats

            // check ~ 4 seconds whether to display the turbo message. 
            // store if the turbo threshold was exceeded in any tics
            // over the past 4 seconds.  offset the checking period
            // for each player so messages are not displayed at the
            // same time.

            if (cmd->forwardmov > TURBOTHRESHOLD)
            {
                turbodetected[i] = true;
            }

            if ((gametic & 31) == 0 
             && ((gametic >> 5) % MAXPLAYERS) == i
             && turbodetected[i])
            {
                turbodetected[i] = false;
            }
#endif
            if (netgame && !netdemo && !(gametic%ticdup) ) 
            { 
                if (gametic > BACKUPTICS 
                    && consistancy[i][buf] != cmd->consistancy) 
                { 
                    I_Error ("consistency failure (%i should be %i)",
                             cmd->consistancy, consistancy[i][buf]); 
                } 
                if (players[i].mo) 
                    consistancy[i][buf] = players[i].mo->x; 
                else 
                    consistancy[i][buf] = rndindex; 
            } 
        }
    }

    // check for special buttons
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i]) 
        { 
            if (players[i].cmd.buttons & BT_SPECIAL) 
            { 
                switch (players[i].cmd.buttons & BT_SPECIALMASK) 
                { 
                  case BTS_PAUSE: 
                    paused ^= 1; 
                    if (paused) 
                        S_PauseSound (); 
                    else 
                        S_ResumeSound (); 
                    break; 
                                         
                  case BTS_SAVEGAME: 
                    // [crispy] never override savegames by demo playback
                    if (demoplayback)
                        break;

                    if (!savedescription[0]) 
                        M_StringCopy(savedescription, "NET GAME", sizeof(savedescription));
                    savegameslot =  
                        (players[i].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT; 
                    gameaction = ga_savegame; 
                    break; 
                } 
            } 
        }
    }

    // Have we just finished displaying an intermission screen?

    if (oldgamestate == GS_INTERMISSION && gamestate != GS_INTERMISSION)
    {
        WI_End();
    }

    oldgamestate = gamestate;

    // do main actions
    switch (gamestate) 
    { 
      case GS_LEVEL: 
        P_Ticker (); 
        ST_Ticker (); 
        AM_Ticker (); 
        HU_Ticker ();            
        break; 
         
      case GS_INTERMISSION: 
        WI_Ticker (); 
        break; 
                         
      case GS_FINALE: 
        F_Ticker (); 
        break; 
 
      case GS_DEMOSCREEN: 
        D_PageTicker (); 
        break;
    }
} 
 
 
//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//
 
 


//
// G_PlayerReborn
// Called after a player dies 
// almost everything is cleared and initialized 
//
void G_PlayerReborn (int player) 
{
    player_t* p = &players[player]; 
    int       i; 
    int       frags[MAXPLAYERS]; 
    int       killcount = p->killcount;
    int       itemcount = p->itemcount;
    int       secretcount = p->secretcount; 
    unsigned int worldTimer = p->worldTimer;

    memcpy (frags,p->frags,sizeof(frags)); 
    p->worldTimer = worldTimer;

    memset (p, 0, sizeof(*p)); 

    memcpy (p->frags, frags, sizeof(p->frags)); 
    p->killcount = killcount; 
    p->itemcount = itemcount; 
    p->secretcount = secretcount; 

    p->usedown = p->attackdown = true;  // don't do anything immediately 
    p->playerstate = PST_LIVE;       
    p->health = initial_health;     // Use dehacked value
    p->readyweapon = p->pendingweapon = wp_pistol; 
    p->weaponowned[wp_fist] = true; 
    p->weaponowned[wp_pistol] = true; 
    p->ammo[am_clip] = initial_bullets; 
    p->lookdir = 0;
    p->recoilpitch = 0;

    for (i=0 ; i<NUMAMMO ; i++) 
        p->maxammo[i] = maxammo[i]; 

    infight = false;
}

//
// G_InitPlayer 
// Called at the start.
// Called by the game initialization functions.
//
// nitr8 [UNUSED]
//
/*
void G_InitPlayer (int player) 
{
    // clear everything else to defaults
    G_PlayerReborn (player); 
}
*/

//
// G_DeathMatchSpawnPlayer 
// Spawns a player at one of the random death match spots 
// called at level load and each death 
//
void G_DeathMatchSpawnPlayer (int playernum) 
{ 
    int     j; 
    int     selections; 
         
    selections = deathmatch_p - deathmatchstarts; 
    if (selections < 4) 
        I_Error ("Only %i deathmatch spots, 4 required", selections); 
 
    for (j=0 ; j<20 ; j++) 
    { 
        int i = P_Random() % selections; 

        if (G_CheckSpot (playernum, &deathmatchstarts[i]) ) 
        { 
            deathmatchstarts[i].type = playernum+1; 
            P_SpawnPlayer (&deathmatchstarts[i]); 
            return; 
        } 
    } 
 
    // no good spot, so the player will probably get stuck 
    P_SpawnPlayer (&playerstarts[playernum]); 
} 
 
 
void G_ScreenShot (void) 
{ 
    gameaction = ga_screenshot; 
} 
 
void G_ExitLevel (void) 
{ 
    player_t *player = &players[consoleplayer];
    player->item = 0;

//    C_Warning(" G_ExitLevel: Free Memory (0x%x)", Z_FreeMemory());

    if(consoleactive)
        C_HideConsoleFast();
/*
    if(gamemap > 1)
        game_startup = false;
*/
    secretexit = false; 
    gameaction = ga_completed; 
} 

// Here's for the german edition.
void G_SecretExitLevel (void) 
{ 
//    C_Warning(" G_SecretExitLevel: Free Memory (0x%x)", Z_FreeMemory());

    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    secretexit = !(gamemode == commercial && W_CheckNumForName("MAP31") < 0); 
    gameaction = ga_completed; 
} 
 

//
// G_WorldDone 
//
void G_WorldDone (void) 
{ 
    gameaction = ga_worlddone; 

    if (secretexit) 
        players[consoleplayer].didsecret = true; 

    if ( gamemission == pack_nerve )
    {
        switch (gamemap)
        {
          case 8:
            F_StartFinale ();
            break;
        }
    }
    else if ( gamemission == pack_master )
    {
        switch (gamemap)
        {
          case 20:
            if (secretexit)
                break;
          case 21:
            F_StartFinale ();
            break;
        }
    }
    else if ( gamemode == commercial )
    {
        switch (gamemap)
        {
          case 15:
          case 31:
            if (!secretexit)
                break;
          case 6:
          case 11:
          case 20:
          case 30:
            F_StartFinale ();
            break;
        }
    }
} 
 

//
// G_InitFromSavegame
// Can be called by the startup code or the menu task. 
//
void G_LoadGame (char* name) 
{ 
    M_StringCopy(savename, name, sizeof(savename));
    gameaction = ga_loadgame; 
} 


void G_DoLoadGame (void) 
{ 
    int savedleveltime = leveltime;
         
    gameaction = ga_nothing; 
         
    save_stream = fopen(savename, "rb");

    if (!save_stream)
        return;

    savegame_error = false;

    if (!P_ReadSaveGameHeader(savedescription))
    {
        fclose(save_stream);
        return;
    }

    // load a base level 
    G_InitNew (gameskill, gameepisode, gamemap); 
 
    leveltime = savedleveltime;

    // dearchive all the modifications
    P_UnArchivePlayers (); 
    P_UnArchiveWorld (); 
    P_UnArchiveThinkers (); 
    P_UnArchiveSpecials (); 
//    P_UnArchiveMap();

    // [crispy] restore mobj->target and mobj->tracer pointers
    P_RestoreTargets();

    P_MapEnd();

    if (!P_ReadSaveGameEOF())
        I_Error ("Bad savegame");

    fclose(save_stream);
    
    if (setsizeneeded)
        R_ExecuteSetViewSize ();
    
    // draw the pattern into the back screen
    R_FillBackScreen ();   

    if (consoleactive)
    {
        C_Output(" %s loaded.", uppercase(savename));
        C_HideConsoleFast();
    }
} 
 

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string 
//
void
G_SaveGame
( int   slot,
  char* description,
  char* name ) 
{ 
    M_StringCopy(savename, (consoleactive ? name : ""), sizeof(savename));
    savegameslot = slot; 
    M_StringCopy(savedescription, description, sizeof(savedescription));
    sendsave = true; 
} 
 
//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set. 
//
 
void
G_DeferedInitNew
( skill_t    skill,
  int        episode,
  int        map) 
{ 
    d_skill = skill; 
    d_episode = episode; 
    d_map = map; 
    gameaction = ga_newgame; 
    infight = false;
} 


void
G_InitNew
( skill_t    skill,
  int        episode,
  int        map )
{
    char     *skytexturename;
    int      i;
    player_t *player;
/*
    if(episode > 0 && episode < 5 && map == 1)
        game_startup = true;
    else if(map > 1)
        game_startup = false;
*/
    if (paused)
    {
        paused = false;
        S_ResumeSound ();
    }

    player = &players[consoleplayer];
    player->nextextra = EXTRAPOINTS;
    player->item = 0;
    player->score = 0;
    player->extra_lifes = 0;

    /*
    // Note: This commented-out block of code was added at some point
    // between the DOS version(s) and the Doom source release. It isn't
    // found in disassemblies of the DOS version and causes IDCLEV and
    // the -warp command line parameter to behave differently.
    // This is left here for posterity.

    if (skill > sk_nightmare)
        skill = sk_nightmare;

    // This was quite messy with SPECIAL and commented parts.
    // Supposedly hacks to make the latest edition work.
    // It might not work properly.
    if (episode < 1)
      episode = 1;

    if ( gamemode == retail )
    {
      if (episode > 4)
        episode = 4;
    }
    else if ( gamemode == shareware )
    {
      if (episode > 1)
           episode = 1;        // only start episode 1 on shareware
    }
    else
    {
      if (episode > 3)
        episode = 3;
    }
    */

    if (map < 1)
        map = 1;

    if (fsize != 12538385 || (fsize == 12538385 && gameepisode > 1))
    {
        if (map > 9 && gamemode != commercial && !map_flag)
            map = 9;
        else if(map_flag)
            map = 10;
    }
    else
    {
        if (fsize == 12538385 && gameepisode == 1 && map > 10 && gamemode != commercial)
            map = 10;
    }

    map_flag = false;

    M_ClearRandom ();

    if (skill == sk_nightmare || respawnparm )
        respawnmonsters = true;
    else
        respawnmonsters = false;

    if (fastparm || (skill == sk_nightmare && gameskill != sk_nightmare) )
    {
        for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
            states[i].tics >>= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 20*FRACUNIT;
        mobjinfo[MT_BETABRUISERSHOT].speed = 20*FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT;
    }
    else if (skill != sk_nightmare && gameskill == sk_nightmare)
    {
        for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
            states[i].tics <<= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT;
        mobjinfo[MT_BETABRUISERSHOT].speed = 15*FRACUNIT;
        mobjinfo[MT_HEADSHOT].speed = 10*FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 10*FRACUNIT;
    }

    // force players to be initialized upon first level load
    for (i=0 ; i<MAXPLAYERS ; i++)
        players[i].playerstate = PST_REBORN;

    usergame = true;                // will be set false if a demo
    paused = false;
    demoplayback = false;
    automapactive = false;
    viewactive = true;
    gameepisode = episode;
    gamemap = map;
    gameskill = skill;

//    viewactive = true;

    // Set the sky to use.
    //
    // Note: This IS broken, but it is how Vanilla Doom behaves.
    // See http://doomwiki.org/wiki/Sky_never_changes_in_Doom_II.
    //
    // Because we set the sky here at the start of a game, not at the
    // start of a level, the sky texture never changes unless we
    // restore from a saved game.  This was fixed before the Doom
    // source release, but this IS the way Vanilla DOS Doom behaves.

    if (gamemode == commercial)
    {
        if (gamemap < 12)
            skytexturename = "SKY1";
        else if (gamemap < 21)
            skytexturename = "SKY2";
        else
            skytexturename = "SKY3";
    }
    else
    {
        switch (gameepisode)
        {
          default:
          case 1:
            skytexturename = "SKY1";
            break;
          case 2:
            skytexturename = "SKY2";
            break;
          case 3:
            skytexturename = "SKY3";
            break;
          case 4:        // Special Edition sky
            skytexturename = "SKY4";
            break;
        }
    }

    skytexture = R_TextureNumForName(skytexturename);

    G_DoLoadLevel ();
}
/*
#ifndef WII
void G_RecordDemoCmd(char *name)
{
    size_t demoname_size;
    int i;
    int maxsize;

    usergame = false;
    demoname_size = strlen(name) + 5;
    demoname = Z_Malloc(demoname_size, PU_STATIC, NULL);
    M_snprintf(demoname, demoname_size, "%s.lmp", name);
    maxsize = 0x20000;

    //!
    // @arg <size>
    // @category demo
    // @vanilla
    //
    // Specify the demo buffer size (KiB)
    //

    i = M_CheckParmWithArgs("-maxdemo", 1);
    if (i)
        maxsize = atoi(myargv[i+1])*1024;
    demobuffer = Z_Malloc (maxsize,PU_STATIC,NULL); 
    demoend = demobuffer + maxsize;
        
    demorecording = true; 
} 
#endif

void G_RecordDemo(skill_t skill, int episode, int map)
{
    G_InitNew(skill, episode, map);
    usergame = false;

    demobuffer = Z_Malloc(0x20000, PU_STATIC, NULL);

    demorecording = true;
}
 
void G_BeginRecording (void) 
{ 
    int             i; 

    //!
    // @category demo
    //
    // Record a high resolution "Doom 1.91" demo.
    //

#ifndef WII
    longtics = M_CheckParm("-longtics") != 0;
#else
    longtics = long_tics;
#endif

    // If not recording a longtics demo, record in low res

    lowres_turn = !longtics;

    demo_p = demobuffer;

    // Save the right version code for this demo

    if (longtics)
    {
        *demo_p++ = DOOM_191_VERSION;
    }
    else
    {
        *demo_p++ = G_VanillaVersionCode();
    }
 
    if (fsize != 10396254 && fsize != 10399316 && fsize != 4207819 && fsize != 4274218 &&
        fsize != 4225504 && fsize != 4225460)
    {
        *demo_p++ = DOOM_VERSION;
    }

    *demo_p++ = gameskill; 
    *demo_p++ = gameepisode; 
    *demo_p++ = gamemap;

    if (fsize != 10396254 && fsize != 10399316 && fsize != 4207819 && fsize != 4274218 &&
        fsize != 4225504 && fsize != 4225460)
    {
        *demo_p++ = deathmatch; 
        *demo_p++ = respawnparm;
        *demo_p++ = fastparm;
        *demo_p++ = nomonsters;
        *demo_p++ = consoleplayer;
    }
         
    for (i=0 ; i<MAXPLAYERS ; i++) 
        *demo_p++ = playeringame[i];                  
} 
 

//
// G_PlayDemo 
//

void G_DeferedPlayDemo (char* name) 
{ 
    defdemoname = name; 
    gameaction = ga_playdemo; 
} 

//
// G_TimeDemo 
//
void G_TimeDemo (char* name) 
{
#ifndef WII
    //!
    // @vanilla 
    //
    // Disable rendering the screen entirely.
    //

    nodrawers = M_CheckParm ("-nodraw"); 
#endif

    timingdemo = true; 

    defdemoname = name; 
    gameaction = ga_playdemo; 
} 

 

//=================== 
//= 
//= G_CheckDemoStatus 
//= 
//= Called after a death or level completion to allow demos to be cleaned up 
//= Returns true if a new demo loop action will take place 
//=================== 
 
dboolean G_CheckDemoStatus (void) 
{ 
    int             endtime; 
         
    if (timingdemo) 
    { 
        float fps;
        int realtics;

        endtime = I_GetTime (); 
        realtics = endtime - starttime;
        fps = ((float) gametic * TICRATE) / realtics;

        // Prevent recursive calls
        timingdemo = false;
        demoplayback = false;

        I_Error ("timed %i gametics in %i realtics (%f fps)",
                 gametic, realtics, fps);
    } 
         
    if (demoplayback) 
    { 
        W_ReleaseLumpName(defdemoname);
        demoplayback = false; 
        netdemo = false;
        netgame = false;
        deathmatch = false;
        playeringame[1] = playeringame[2] = playeringame[3] = 0;
        respawnparm = false;
        fastparm = false;
        nomonsters = false;
        consoleplayer = 0;
        
        if (singledemo) 
            I_Quit (); 
        else 
            D_AdvanceDemo (); 

        return true; 
    } 
 
    if (demorecording) 
    { 
        *demo_p++ = DEMOMARKER; 
        M_WriteFile (demoname, demobuffer, demo_p - demobuffer); 
        Z_Free (demobuffer); 
        demorecording = false; 

#ifndef WII
        I_Error (" Demo %s recorded",demoname); 
#endif
    } 
         
    return false; 
} 
*/

