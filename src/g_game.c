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
#include "c_io.h"
#include "d_main.h"
#include "deh_str.h"
#include "deh_misc.h"
#include "doomdef.h" 
#include "doomfeatures.h" 
#include "doomkeys.h"
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "f_finale.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_video.h"
#include "m_controls.h"
#include "m_menu.h"
#include "m_misc.h"
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
#include "v_video.h"

#include "w_wad.h"
#include "wi_stuff.h"
#include "z_zone.h"

#include <wiiuse/wpad.h>


#define SAVEGAMESIZE    0x2c000
#define MAXPLMOVE       0x32
#define TURBOTHRESHOLD  0x32
#define MAX_JOY_BUTTONS 20
#define BODYQUESIZE     32
#define KEY_1           0x02
#define VERSIONSIZE     16 
#define DEMOMARKER      0x80


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
 
static int e4pars[10] =
{
    0,165,255,135,150,180,390,135,360,180
};

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

fixed_t         forwardmve;
fixed_t         sidemve;
fixed_t         angleturn;              // + slow turn 

mobj_t*         bodyque[BODYQUESIZE]; 

// If non-zero, exit the level after this number of minutes.
int             timelimit;

// mouse values are used once 
int             mousex;
int             mousey;         

int             d_episode; 
int             d_map; 
int             gameepisode; 
int             gamemap; 
int             starttime;              // for comparative timing purposes
int             consoleplayer;          // player taking events and displaying 
int             displayplayer;          // view being displayed 
int             levelstarttic;          // gametic at level start 
int             totalkills, totalitems, totalsecret;    // for intermission 
int             mouselook;
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
int             vanilla_demo_limit = 1; 
int             key_strafe, joybstrafe;
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

boolean         secret_1 = false;
boolean         secret_2 = false;
boolean         secretexit; 
boolean         respawnmonsters;
boolean         paused; 
boolean         sendpause;              // send a pause event next tic 
boolean         sendsave;               // send a save event next tic 
boolean         usergame;               // ok to save / end game 
boolean         timingdemo;             // if true, exit with report on completion  
boolean         viewactive; 
boolean         deathmatch;             // only if started as net death 
boolean         netgame;                // only true if packets are broadcast 
boolean         playeringame[MAXPLAYERS]; 
boolean         demorecording; 
boolean         demoplayback; 
boolean         netdemo; 
boolean         singledemo;             // quit after playing a demo from cmdline  
boolean         precache = true;        // if true, load all graphics at start 
boolean         joyarray[MAX_JOY_BUTTONS + 1]; 
boolean         *joybuttons = &joyarray[1]; // allow [-1] 
boolean         not_walking;
//boolean         game_startup;

char            demoname[32];
char            savename[256];
char            *defdemoname; 
 

byte*           demobuffer;
byte*           demo_p;
byte*           demoend; 

byte            consistancy[MAXPLAYERS][BACKUPTICS]; 

static boolean  dclickstate2;

static char     savedescription[32]; 

static int      dclicks; 
static int      dclicktime2;
static int      dclicks2;
static int      savegameslot; 

// joystick values are repeated 
static int      joyxmove;
static int      joyymove;
static int      joyirx;
static int      joyiry;

extern int      mspeed;
extern int      turnspeed;
extern int      messageToPrint;

extern boolean  done;
extern boolean  netgameflag;
extern boolean  aiming_help;
extern boolean  messageNeedsInput;
extern boolean  setsizeneeded;
extern boolean  map_flag;

extern fixed_t  forwardmove; 
extern fixed_t  sidemove; 
extern fixed_t  mtof_zoommul; // how far the window zooms in each tic (map coords)
extern fixed_t  ftom_zoommul; // how far the window zooms in each tic (fb coords)

extern ticcmd_t *netcmds;

extern menu_t   *currentMenu;                          

extern short    itemOn;       // menu item skull is on

extern char*    pagename; 


int G_CmdChecksum (ticcmd_t* cmd) 
{ 
    size_t      i;
    int         sum = 0; 
         
    for (i=0 ; i< sizeof(*cmd)/4 - 1 ; i++) 
        sum += ((int *)cmd)[i]; 
                 
    return sum; 
} 

void ChangeWeaponRight(void)
{
    static player_t*    plyrweap;
    static event_t      kbevent;

    weapontype_t        num;

    if (gamestate == GS_LEVEL && !menuactive)
    {
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

    weapontype_t        num;

    if (gamestate == GS_LEVEL && !menuactive)
    {
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

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer. 
// If recording a demo, write it out 
// 

void G_BuildTiccmd (ticcmd_t* cmd, int maketic) 
{ 
    boolean     strafe, use;

    int         forward;
    int         side;
    int         look;
    int         flyheight;

    memset(cmd, 0, sizeof(ticcmd_t));

    cmd->consistancy = 
        consistancy[consoleplayer][maketic%BACKUPTICS]; 
 
    strafe = gamekeydown[key_strafe] || joybuttons[joybstrafe];

    forward = side = look = flyheight = 0;

    // let movement keys cancel each other out
    if (strafe) 
    { 
        if (gamekeydown[key_right]) 
        {
            // fprintf(stderr, "strafe right\n");
            side += sidemove; 
        }
        if (gamekeydown[key_left]) 
        {
            // fprintf(stderr, "strafe left\n");
            side -= sidemove; 
        }

        if (joyxmove > 0) 
            side += sidemve; 
        if (joyxmove < 0) 
            side -= sidemve; 
    } 
    else 
    { 
        if (gamekeydown[key_right])
            cmd->angleturn -= angleturn; 
        if (gamekeydown[key_left]) 
            cmd->angleturn += angleturn; 

        if (joyxmove > 20) 
        {
            side += sidemove; 
            not_walking = false;
        }
        else if (joyxmove < -20) 
        {
            side -= sidemove; 
            not_walking = false;
        }
        else
            not_walking = true;

        if (joyirx > 0)     // calculate wii IR curve based on input
            cmd->angleturn -= turnspd * joyirx;
        if (joyirx < 0)     // calculate wii IR curve based on input
            cmd->angleturn -= turnspd * joyirx;
    } 

    extern boolean dont_move_forwards;
    extern boolean dont_move_backwards;

    if (gamekeydown[key_up]) 
    {
        // fprintf(stderr, "up\n");
        if(dont_move_forwards == true)
            forward += forwardmove; 
    }
    if (gamekeydown[key_down]) 
    {
        // fprintf(stderr, "down\n");
        if(dont_move_backwards == true)
            forward -= forwardmove; 
    }

    if (joyymove > 20) 
    {
        forward += forwardmve; 
        not_walking = false;
    }
    else if (joyymove < -20) 
    {
        forward -= forwardmve; 
        not_walking = false;
    }
    else
        not_walking = true;

    if (joybuttons[joybstrafeleft]) 
    {
        side -= sidemove;
    }

    if (joybuttons[joybstraferight])
    {
        side += sidemove; 
    }

    if (gamekeydown[key_fire] || joybuttons[joybfire]) 
        cmd->buttons |= BT_ATTACK; 

    if (joybuttons[joybspeed]) 
    {
        forwardmve = forwardmove * 6;
        sidemve = sidemove * 6;
    }
    else if(!joybuttons[joybspeed])
    {
        forwardmve = forwardmove;
        sidemve = sidemove;
    }
    turnspd = turnspeed;

    if(mouselook == 0)
        look = -8;

    // Fly up/down/drop keys
    if (joybuttons[joybflyup])
    {
        flyheight = 5;          // note that the actual flyheight will be twice this
    }
    if (joybuttons[joybflydown])
    {
        flyheight = -5;
    }

    if (joybuttons[joybjump] && !menuactive)
    {
        if(!demoplayback)
            cmd->arti |= AFLAG_JUMP;
    }

    // FOR THE WII: UNUSED BUT WORKING
    if((joybuttons[joybaiminghelp] || aiming_help) && !demoplayback && devparm)
    {
        player_t* player = &players[consoleplayer];
        P_AimingHelp(player);
    }

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
                            C_SetConsole();
                            S_StartSound(NULL, sfx_doropn);
                        }
                    }
                }
            }

            if(joybuttons[joybmenu])
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

            if(!demoplayback)
            {
                if(joybuttons[joybright])
                    ChangeWeaponRight();

                if(joybuttons[joybleft])
                    ChangeWeaponLeft();

                if(joybuttons[joybmap])
                {
                    if (!automapactive)
                    {
                        if(!menuactive)
                        {
                            if(usergame)
                                AM_Start ();
                        }
                    }
                    else
                    {
                        if(!menuactive)
                        {
                            AM_Stop ();
                            if(beta_style)
                            {
                                ST_doRefresh();
                                R_SetViewSize (screenblocks, detailLevel);
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

    if (gamekeydown[key_use] || joybuttons[joybuse])
    { 
        cmd->buttons |= BT_USE;

        // clear double clicks if hit use button 
        dclicks = 0;                   
    } 

    use = gamekeydown[key_use];
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
        cmd->angleturn -= mousex*0x8;   // <--

    forward += mousey; 

    // mouselook, but not when paused
    if (joyiry && !paused && mouselook > 0 && players[consoleplayer].playerstate == PST_LIVE)
    {                                           // ...not when paused & if on
        // We'll directly change the viewing pitch of the console player.
        float adj = 0;

        if(!menuactive)
            adj = ((joyiry * 0x4) << 16) / (float) 0x80000000*180*110.0/85.0;

        // initialiser added to prevent compiler warning
        float newlookdir = 0;

        extern int mspeed;

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
 
    cmd->forwardmove += forward; 
    cmd->sidemove += side;

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
} 
 
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsizeof-pointer-memaccess"

//
// G_Responder  
// Get info needed to make ticcmd_ts for the players.
// 
boolean G_Responder (event_t* ev) 
{ 
    if (gamestate == GS_LEVEL) 
    { 
#if 0 
        if (devparm && ev->type == ev_keydown && ev->data1 == ';') 
        { 
            G_DeathMatchSpawnPlayer (0); 
            return true; 
        } 
#endif 
        if (HU_Responder (ev)) 
            return true;        // chat ate the event 
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

    switch (ev->type) 
    { 
      case ev_keydown: 
        if (ev->data1 <NUMKEYS) 
        {
            gamekeydown[ev->data1] = true; 
        }

        return true;    // eat key down events 
 
      case ev_keyup: 
        if (ev->data1 <NUMKEYS) 
            gamekeydown[ev->data1] = false; 
        return false;   // always let key up events filter down 
                 
      case ev_mouse: 
        mousex = ev->data2*(mouseSensitivity+5)/10; 
        mousey = ev->data3*(mouseSensitivity+5)/10; 
        return true;    // eat events 

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

      default: 
        break; 
    } 
 
    return false; 
} 
 
 
 
//
// DEMO RECORDING 
// 

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

    cmd->angleturn = ((unsigned char) *demo_p++)<<8; 

    cmd->buttons = (unsigned char)*demo_p++; 
    cmd->lookfly = (unsigned char) *demo_p++;
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

    demo_start = demo_p;

    *demo_p++ = cmd->forwardmove; 
    *demo_p++ = cmd->sidemove; 

    *demo_p++ = cmd->angleturn >> 8; 

    *demo_p++ = cmd->buttons; 
    *demo_p++ = cmd->lookfly;

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
 
//
// G_CheckSpot  
// Returns false if the player cannot be respawned
// at the given mapthing_t spot  
// because something is occupying it 
//
 
boolean
G_CheckSpot
( int             playernum,
  mapthing_t*     mthing ) 
{ 
    fixed_t       x;
    fixed_t       y; 
    subsector_t*  ss; 
    unsigned      an; 
    mobj_t*       mo; 
    int           i;
        
    if (!players[playernum].mo)
    {
        // first spawn of level, before corpses
        for (i=0 ; i<playernum ; i++)
            if (players[i].mo->x == mthing->x << FRACBITS
                && players[i].mo->y == mthing->y << FRACBITS)
                return false;
        return true;
    }
                
    x = mthing->x << FRACBITS; 
    y = mthing->y << FRACBITS; 
         
    players[playernum].mo->flags2 &= ~MF2_PASSMOBJ;
    if (!P_CheckPosition (players[playernum].mo, x, y) ) 
    {
        players[playernum].mo->flags2 |= MF2_PASSMOBJ;
        return false;
    }
    players[playernum].mo->flags2 |= MF2_PASSMOBJ;
 
    // flush an old corpse if needed 
    if (bodyqueslot >= BODYQUESIZE) 
        P_RemoveMobj (bodyque[bodyqueslot%BODYQUESIZE]); 
    bodyque[bodyqueslot%BODYQUESIZE] = players[playernum].mo; 
    bodyqueslot++; 
        
    // spawn a teleport fog 
    ss = R_PointInSubsector (x,y); 
    an = ( ANG45 * (((unsigned int) mthing->angle)/45) ) >> ANGLETOFINESHIFT; 
 
    mo = P_SpawnMobj (x+20*finecosine[an], y+20*finesine[an] 
                      , ss->sector->floor_height 
                      , MT_TFOG); 
         
    if (players[consoleplayer].viewz != 1) 
        S_StartSound (mo, sfx_telept);  // don't start sound on first frame 
 
    return true; 
} 

//
// G_DoReborn 
// 
void G_DoReborn (int playernum) 
{ 
    int                             i; 
         
    if (!netgame)
    {
        // reload the level from scratch
        gameaction = ga_loadlevel;  
    }
    else 
    {
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
    int             i; 

    // Set the sky map.
    // First thing, we have a dummy sky texture name,
    //  a flat. The data is in the WAD only because
    //  we look for an actual index, instead of simply
    //  setting one.

    skyflatnum = R_FlatNumForName(DEH_String(SKYFLATNAME));

    // The "Sky never changes in Doom II" bug was fixed in
    // the id Anthology version of doom2.exe for Final Doom.
    if ((gamemode == commercial) && (gameversion == exe_final2))
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

        skytexturename = DEH_String(skytexturename);

        skytexture = R_TextureNumForName(skytexturename);
    }

    levelstarttic = gametic;        // for time calculation
    
    if (wipegamestate == GS_LEVEL) 
        wipegamestate = -1;             // force a wipe 

    gamestate = GS_LEVEL; 

    for (i=0 ; i<MAXPLAYERS ; i++) 
    { 
        if (playeringame[i] && players[i].playerstate == PST_DEAD) 
            players[i].playerstate = PST_REBORN; 
        memset (players[i].frags,0,sizeof(players[i].frags)); 
    } 
                 
    P_SetupLevel (gameepisode, gamemap, 0, gameskill);    
    displayplayer = consoleplayer;                // view the guy you are playing    
    gameaction = ga_nothing; 
    Z_CheckHeap ();
    
    // clear cmd building stuff

    memset (gamekeydown, 0, sizeof(gamekeydown)); 
    joyxmove = joyymove = joyirx = joyiry = 0; 
    mousex = mousey = 0; 
    sendpause = sendsave = paused = false; 

    memset (joybuttons, 0, sizeof(joybuttons)); 

//    C_InstaPopup();  // pop up the console
} 

void G_DoNewGame (void) 
{
    demoplayback = false; 
    netdemo = false;
    netgame = false;
/*
    if(netgameflag)        // FIXME: WHAT IS THIS???
        netgame = true;
*/
    deathmatch = false;

    playeringame[1] = playeringame[2] = playeringame[3] = 0;

    respawnparm = start_respawnparm;
    fastparm = start_fastparm;

    nomonsters = false;
    consoleplayer = 0;
    G_InitNew (d_skill, d_episode, d_map); 
    gameaction = ga_nothing; 
} 

// Generate a string describing a demo version

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

void G_DoPlayDemo (void) 
{ 
    skill_t skill; 
    int             i, episode, map; 
    int demoversion;
         
    gameaction = ga_nothing; 
    demobuffer = demo_p = W_CacheLumpName (defdemoname, PU_STATIC); 

    if (fsize != 10396254 && fsize != 10399316 && fsize != 4207819 && fsize != 4274218 &&
        fsize != 4225504 && fsize != 4225460)
    {
        demoversion = *demo_p++;

        char *message = "Demo is from a different game version!\n"
                        "(read %i, should be %i)\n"
                        "\n"
                        "*** You may need to upgrade your version "
                        "of Doom to v1.9. ***\n"
                        "    See: http://doomworld.com/files/patches.shtml\n"
                        "    This appears to be %s.";

        if (fsize != 4234124 && fsize !=  4196020 && fsize != 11159840 && fsize !=  4271324 &&
            fsize != 4211660 && fsize != 14943400 && fsize != 14824716 && fsize != 14612688 &&
            fsize != 14607420)
            I_Error(message, demoversion, DOOM_VERSION,
                            DemoVersionDescription(demoversion));
    }
    
    skill = *demo_p++; 
    episode = *demo_p++; 
    map = *demo_p++; 

    if (fsize != 10396254 && fsize != 10399316 && fsize != 4207819 && fsize != 4274218 &&
        fsize != 4225504 && fsize != 4225460)
    {
        deathmatch = *demo_p++;
        respawnparm = *demo_p++;
        fastparm = *demo_p++;
        nomonsters = *demo_p++;
        consoleplayer = *demo_p++;
    }        

    for (i=0 ; i<MAXPLAYERS ; i++) 
        playeringame[i] = *demo_p++; 

    if (playeringame[1])
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
    int             i; 
         
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
            switch(gamemap)
            {
              case  4: wminfo.next = 8; break;
            }
        else
            switch(gamemap)
            {
              case  9: wminfo.next = 4; break;
              default: wminfo.next = gamemap;
            }
    }
    else
    if ( gamemode == commercial)
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
    if (gamemap == 33)
        // map 33 par time sucks
        wminfo.partime = INT_MAX;
    else if (gamemission == pack_nerve)
        wminfo.partime = TICRATE*npars[gamemap-1];
    else if (gamemode == commercial)
        wminfo.partime = TICRATE*cpars[gamemap-1];
    else if (gameepisode < 4)
        wminfo.partime = TICRATE*pars[gameepisode][gamemap];
    else if (gameepisode == 4)
        wminfo.partime = TICRATE*e4pars[gamemap];
    else
        wminfo.partime = TICRATE*cpars[gamemap];

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
    char *savegame_file;
    char *temp_savegame_file;

    temp_savegame_file = P_TempSaveGameFile();
    savegame_file = P_SaveGameFile(savegameslot);

    // Open the savegame file for writing.  We write to a temporary file
    // and then rename it at the end if it was successfully written.
    // This prevents an existing savegame from being overwritten by 
    // a corrupted one, or if a savegame buffer overrun occurs.

    save_stream = fopen(temp_savegame_file, "wb");

    if (save_stream == NULL)
    {
        return;
    }

    savegame_error = false;

    P_WriteSaveGameHeader(savedescription);
 
    P_ArchivePlayers (); 
    P_ArchiveWorld (); 
    P_ArchiveThinkers (); 
    P_ArchiveSpecials (); 
         
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
    
    gameaction = ga_nothing; 
    strcpy(savedescription, "");

    players[consoleplayer].message = DEH_String(GGSAVED);

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
            G_DoPlayDemo (); 
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
            players[consoleplayer].message = DEH_String("screen shot");
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

            if (demoplayback) 
                G_ReadDemoTiccmd (cmd); 
            if (demorecording) 
                G_WriteDemoTiccmd (cmd);
            
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
                    if (!savedescription[0]) 
                        strcpy (savedescription, "NET GAME"); 
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

      case GS_CONSOLE:
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
    player_t* p; 
    int       i; 
    int       frags[MAXPLAYERS]; 
    int       killcount;
    int       itemcount;
    int       secretcount; 
    unsigned int worldTimer;
         
    worldTimer = players[player].worldTimer;
    memcpy (frags,players[player].frags,sizeof(frags)); 
    killcount = players[player].killcount; 
    itemcount = players[player].itemcount; 
    secretcount = players[player].secretcount; 
    players[player].worldTimer = worldTimer;
         
    p = &players[player]; 
    memset (p, 0, sizeof(*p)); 
 
    memcpy (players[player].frags, frags, sizeof(players[player].frags)); 
    players[player].killcount = killcount; 
    players[player].itemcount = itemcount; 
    players[player].secretcount = secretcount; 
 
    p->usedown = p->attackdown = true;  // don't do anything immediately 
    p->playerstate = PST_LIVE;       
    p->health = deh_initial_health;     // Use dehacked value
    p->readyweapon = p->pendingweapon = wp_pistol; 
    p->weaponowned[wp_fist] = true; 
    p->weaponowned[wp_pistol] = true; 
    p->ammo[am_clip] = deh_initial_bullets; 
    p->lookdir = 0;
    p->recoilpitch = 0;
         
    for (i=0 ; i<NUMAMMO ; i++) 
        p->maxammo[i] = maxammo[i]; 
                 
}

//
// G_InitPlayer 
// Called at the start.
// Called by the game initialization functions.
//
void G_InitPlayer (int player) 
{
    // clear everything else to defaults
    G_PlayerReborn (player); 
}


//
// G_DeathMatchSpawnPlayer 
// Spawns a player at one of the random death match spots 
// called at level load and each death 
//
void G_DeathMatchSpawnPlayer (int playernum) 
{ 
    int     i, j; 
    int     selections; 
         
    selections = deathmatch_p - deathmatchstarts; 
    if (selections < 4) 
        I_Error ("Only %i deathmatch spots, 4 required", selections); 
 
    for (j=0 ; j<20 ; j++) 
    { 
        i = P_Random() % selections; 
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
    // IF NO WOLF3D LEVELS, NO SECRET EXIT!
    if ( (gamemode == commercial)
      && (W_CheckNumForName("map31")<0))
        secretexit = false;
    else
        secretexit = true; 
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
    else
    if ( gamemode == commercial )
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
    strcpy (savename, name); 
    gameaction = ga_loadgame; 
} 


void G_DoLoadGame (void) 
{ 
    int savedleveltime;
         
    gameaction = ga_nothing; 
         
    save_stream = fopen(savename, "rb");

    if (save_stream == NULL)
    {
        return;
    }

    savegame_error = false;

    if (!P_ReadSaveGameHeader())
    {
        fclose(save_stream);
        return;
    }

    savedleveltime = leveltime;
    
    // load a base level 
    G_InitNew (gameskill, gameepisode, gamemap); 
 
    leveltime = savedleveltime;

    // dearchive all the modifications
    P_UnArchivePlayers (); 
    P_UnArchiveWorld (); 
    P_UnArchiveThinkers (); 
    P_UnArchiveSpecials (); 
 
    P_RestoreTargets();

    if (!P_ReadSaveGameEOF())
        I_Error ("Bad savegame");

    fclose(save_stream);
    
    if (setsizeneeded)
        R_ExecuteSetViewSize ();
    
    // draw the pattern into the back screen
    R_FillBackScreen ();   
} 
 

//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string 
//
void
G_SaveGame
( int   slot,
  char* description ) 
{ 
    savegameslot = slot; 
    strcpy (savedescription, description); 
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
} 


void
G_InitNew
( skill_t    skill,
  int        episode,
  int        map )
{
    char     *skytexturename;
    int      i;
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

    player_t *player = &players[consoleplayer];
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
        mobjinfo[MT_HEADSHOT].speed = 20*FRACUNIT;
        mobjinfo[MT_TROOPSHOT].speed = 20*FRACUNIT;
    }
    else if (skill != sk_nightmare && gameskill == sk_nightmare)
    {
        for (i=S_SARG_RUN1 ; i<=S_SARG_PAIN2 ; i++)
            states[i].tics <<= 1;
        mobjinfo[MT_BRUISERSHOT].speed = 15*FRACUNIT;
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

    viewactive = true;

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

    skytexturename = DEH_String(skytexturename);

    skytexture = R_TextureNumForName(skytexturename);


    G_DoLoadLevel ();
}
 

 
void G_RecordDemo(skill_t skill, int numplayers, int episode, int map)
{
    int i;

    G_InitNew(skill, episode, map);
    usergame = false;

    demobuffer = demo_p = Z_Malloc(0x20000, PU_STATIC, NULL);
    *demo_p++ = skill;
    *demo_p++ = episode;
    *demo_p++ = map;

    for (i = 0; i < MAXPLAYERS; i++)
        *demo_p++ = playeringame[i];

    demorecording = true;
}

 
void G_BeginRecording (void) 
{ 
    int             i; 

    demo_p = demobuffer;
        
    // Save the right version code for this demo
 
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
    timingdemo = true; 
    singletics = true; 

    defdemoname = name; 
    gameaction = ga_playdemo; 
} 
 
 
/* 
=================== 
= 
= G_CheckDemoStatus 
= 
= Called after a death or level completion to allow demos to be cleaned up 
= Returns true if a new demo loop action will take place 
=================== 
*/ 
boolean G_CheckDemoStatus(void)
{
    int     i, endtime;
    char    lbmname[10];

    FILE    *test_access;

    strcpy(lbmname,"DEMO00.lmp");

    for (i=1 ; i<=98 ; i++)
    {
        lbmname[4] = i/10 + '0';
        lbmname[5] = i%10 + '0';

        test_access = fopen(lbmname, "rb");
  
        if(test_access)
            break;
    }

    if (i==100)
        I_Error ("G_CheckDemoStatus: Couldn't create a DEMO");

    if (timingdemo)
    {
        endtime = I_GetTime();
        I_Error("timed %i gametics in %i realtics", gametic,
                endtime - starttime);
    }

    if (demoplayback)
    {
        if (singledemo)
            I_Quit();

        W_ReleaseLumpName(defdemoname);
        demoplayback = false;
        respawnparm = start_respawnparm;
        fastparm = start_fastparm;
        D_AdvanceDemo();
        return true;
    }

    if (demorecording)
    {
        *demo_p++ = DEMOMARKER;
        M_WriteFile(lbmname, demobuffer, demo_p - demobuffer);
        Z_Free(demobuffer);
        demorecording = false;
    }

    return false;
}

