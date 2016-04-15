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
//        Put all global tate variables here.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>

#ifdef SDL2
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "config.h"
#include "doom/doomdef.h"
#include "doomfeatures.h"
#include "doomtype.h"
#include "i_system.h"
#include "m_fixed.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#include "icon.c"

#define r_bloodsplats_max_default     32768

#ifdef SDL2
SDL_Window *screen;
#endif

char             *net_player_name;

// Window position:
char             *window_position = "";

// Window title
char             *window_title = "";

char             *pwadfile;
char             *gamedescription;

// Time to wait for the screen to settle on startup before starting the
// game (ms)
int              startup_delay = 1000;

// Color depth.
int              screen_bpp = 0;

// Run in full screen mode?  (int type for config code)
int              fullscreen = false;

// Aspect ratio correction mode
int              aspect_ratio_correct = false;

// Automatically adjust video settings if the selected mode is 
// not a valid video mode.
int              autoadjust_video_settings = 1;

// Show messages has default, 0 = off, 1 = on

int              r_bloodsplats_max = r_bloodsplats_max_default;

// Screen width and height, from configuration file.
int              screen_width = SCREENWIDTH;
int              screen_height = SCREENHEIGHT;

int              pixelwidth;
int              pixelheight;
int              runcount = 0;

// specifies whether to follow the player around
int              chaingun_tics = 4;
int              background_color = 231;

// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
int              sfxVolume = 8;

// Maximum volume of music. 
int              musicVolume = 8;

int              window_pos_x = 0;
int              window_pos_y = 0;

int              usejoystick;

#ifndef WII
int              joybfire = 0;
int              joybstrafe = 1;
int              joybuse = 3;
int              joybnextweapon = -1;
int              joybspeed = 2;
int              joybjump = -1;
#endif

dboolean         png_screenshots;
dboolean         aiming_help;
dboolean         correct_lost_soul_bounce;

// Set if homebrew PWAD stuff has been added.
dboolean         showMessages = true;
dboolean         use_vanilla_weapon_change = true;
dboolean         crosshair = false;
dboolean         show_stats = false;
dboolean         followplayer = true;
dboolean         drawgrid = false;
dboolean         modifiedgame;
dboolean         start_respawnparm;
dboolean         start_fastparm;
dboolean         nomonsters;
dboolean         hud;
dboolean         randompitch;
dboolean         opl_stereo_correct;
dboolean         display_ticker;
dboolean         memory_usage;
dboolean         timer_info = false;

dboolean         am_overlay = false;
dboolean         nerve_pwad = false;
dboolean         master_pwad = false;

dboolean         remove_slime_trails = false;
dboolean         d_fixwiggle = false;
dboolean         lowhealth = false;
dboolean         d_recoil = false;
dboolean         d_maxgore = false;
dboolean         d_thrust = false;
dboolean         respawnparm = false;
dboolean         fastparm = false;
dboolean         d_footstep = false;
dboolean         d_footclip = false;
dboolean         d_splash = false;
dboolean         d_translucency = false;
dboolean         d_chkblood = false;
dboolean         d_chkblood2 = false;
dboolean         d_flipcorpses = false;
dboolean         d_secrets = false;
dboolean         beta_style = false;
dboolean         beta_style_mode = false;
dboolean         smoketrails = false;
dboolean         sound_info = false;
dboolean         autodetect_hom = false;
dboolean         d_fallingdamage = false;
dboolean         d_infiniteammo = false;
dboolean         not_monsters = false;
dboolean         overlay_trigger = false;
dboolean         replace_missing = false;
dboolean         d_telefrag = true;
dboolean         d_doorstuck = false;
dboolean         d_resurrectghosts = false;
dboolean         d_limitedghosts = true;
dboolean         d_blockskulls = false;
dboolean         d_blazingsound = false;
dboolean         d_god = true;
dboolean         d_floors = false;
dboolean         d_moveblock = false;
dboolean         d_model = false;
dboolean         d_666 = false;
dboolean         d_maskedanim = false;
dboolean         d_sound = false;
dboolean         d_ouchface = false;
dboolean         show_authors = false;
dboolean         d_shadows = false;
dboolean         d_fixspriteoffsets = false;
dboolean         d_brightmaps = false;
dboolean         d_fixmaperrors = false;
dboolean         d_altlighting = false;
dboolean         allow_infighting = false;
dboolean         last_enemy = false;
dboolean         float_items = false;
dboolean         animated_drop = false;
dboolean         crush_sound = false;
dboolean         disable_noise = false;
dboolean         corpses_nudge = false;
dboolean         corpses_slide = false;
dboolean         corpses_smearblood = false;
dboolean         show_diskicon = true;
dboolean         randomly_colored_playercorpses = false;
dboolean         mousewalk = false;
dboolean         am_rotate = false;
dboolean         jumping = false;
dboolean         general_sound = true;
dboolean         d_centerweapon = false;
dboolean         d_ejectcasings = false;
dboolean         d_statusmap = true;
dboolean         show_title = true;
dboolean         dump_mem = false;
dboolean         dump_con = false;
dboolean         dump_stat = false;
dboolean         printdir = false;
dboolean         d_drawparticles = false;
dboolean         d_drawbfgcloud = false;
dboolean         d_drawrockettrails = false;
dboolean         d_drawrocketexplosions = false;
dboolean         d_drawbfgexplosions = false;
dboolean         d_spawnflies = false;
dboolean         d_dripblood = false;
dboolean         d_vsync = false;
dboolean         particle_sounds = false;
dboolean         map_secret_after = false;
dboolean         enable_autoload = false;
dboolean         enable_autosave = false;
dboolean         drawsplash = false;
dboolean         chexdeh = false;
dboolean         BTSX = false;
dboolean         BTSXE1 = false;
dboolean         BTSXE2 = false;
dboolean         BTSXE2A = false;
dboolean         BTSXE2B = false;
dboolean         BTSXE3 = false;
dboolean         BTSXE3A = false;
dboolean         BTSXE3B = false;
dboolean         autoaim = true;
dboolean         render_mode = true;
dboolean         d_fliplevels = false;
dboolean         d_colblood = false;
dboolean         d_colblood2 = false;
dboolean         d_swirl = false;
dboolean         icontype = false;
dboolean         snd_module = false;
dboolean         opl_type = false;
dboolean         display_fps = false;
dboolean         s_randommusic = false;

// if true, load all graphics at start 
dboolean         precache = true;

// Blocky mode, has default, 0 = high, 1 = normal
dboolean         detailLevel = false;

#ifdef WII
dboolean         show_endoom = false;
#else
dboolean         show_endoom = true;
#endif

fixed_t          forwardmove = 29;
fixed_t          sidemove = 24; 

int              turnspeed = 7;

// temp for screenblocks (0-9)
int              screenSize;
int              screenblocks = 9;

// Gamma correction level to use
int              usegamma = 10;

int              background_type = 0;
int              wipe_type = 2;
int              mouselook;
int              mspeed = 2;
int              mus_engine = 1;
int              snd_chans = 1;
int              sound_channels = 8;
int              use_libsamplerate = 0;
int              gore_amount = 1;
int              font_shadow = 0;
int              stillbob = 0;
int              movebob = 75;

int              bloodsplat_particle = 0;
int              bulletpuff_particle = 0;
int              teleport_particle = 0;
int              d_uncappedframerate = 0;
int              d_spawnteleglit = 0;
int              map_grid_size = 128;

// defaulted values
int              mouseSensitivity = 5;

// jff 1/7/98 default automap colors added

// map background
int              mapcolor_back = 247;

// grid lines color
int              mapcolor_grid = 104;

// normal 1s wall color
int              mapcolor_wall = 23;

// line at floor height change color
int              mapcolor_fchg = 55;

// line at ceiling height change color
int              mapcolor_cchg = 215;

// line at sector with floor = ceiling color
int              mapcolor_clsd = 208;

// red key color
int              mapcolor_rkey = 175;

// blue key color
int              mapcolor_bkey = 204;

// yellow key color
int              mapcolor_ykey = 231;

// red door color (diff from keys to allow option)
int              mapcolor_rdor = 175;

// blue door color (of enabling one but not other)
int              mapcolor_bdor = 204;

// yellow door color
int              mapcolor_ydor = 231;

// teleporter line color
int              mapcolor_tele = 119;

// secret sector boundary color
int              mapcolor_secr = 252;

// jff 4/23/98 add exit line color
int              mapcolor_exit = 0;

// computer map unseen line color
int              mapcolor_unsn = 104;

// line with no floor / ceiling changes
int              mapcolor_flat = 88;

// general sprite color
int              mapcolor_sprt = 112;

// item sprite color
int              mapcolor_item = 231;

// enemy sprite color
int              mapcolor_enemy = 177;

// crosshair color
int              mapcolor_hair = 208;

// single player arrow color
int              mapcolor_sngl = 208;

// color for player arrow
int              mapcolor_plyr[4] = { 112, 88, 64, 32 };

// The number of tics that have been run (using RunTic) so far.
int              gametic;

// player taking events and displaying 
int              consoleplayer;

// view being displayed 
int              displayplayer;


float            libsamplerate_scale = 0.65;


// Set the application icon

#ifndef WII
void I_InitWindowIcon(void)
{
    SDL_Surface *surface;

#ifndef SDL2
    Uint8 *mask;
    int i;

    // Generate the mask
    mask = malloc(icon_w * icon_h / 8);
    memset(mask, 0, icon_w * icon_h / 8);

    for (i = 0; i < icon_w * icon_h; ++i)
    {
        if (icon_data[i * 3] != 0x00
            || icon_data[i * 3 + 1] != 0x00
            || icon_data[i * 3 + 2] != 0x00)
        {
            mask[i / 8] |= 1 << (7 - i % 8);
        }
    }

    surface = SDL_CreateRGBSurfaceFrom(icon_data, icon_w, icon_h, 24,
                                       icon_w * 3, 0xff << 0, 0xff << 8,
                                       0xff << 16, 0);

    SDL_WM_SetIcon(surface, mask);

#else

    surface = SDL_CreateRGBSurfaceFrom((void *) icon_data, icon_w, icon_h,
                                       32, icon_w * 4, 0xff << 24, 0xff << 16,
                                       0xff << 8, 0xff << 0);

    SDL_SetWindowIcon(screen, surface);
#endif

    SDL_FreeSurface(surface);

#ifndef SDL2
    free(mask);
#endif
}
#endif

void I_InitWindowTitle(void)
{
    char *buf = M_StringJoin(window_title, " - ", PACKAGE_STRING, NULL);

#ifdef SDL2
    SDL_SetWindowTitle(screen, buf);
#else
    SDL_WM_SetCaption(buf, NULL);
#endif

    free(buf);
}

void Z_DumpHistory(char *buf)
{
    int i, j;
    char s[1024];

    strcat(buf, "\n");

    for (i = 0; i < NUM_HISTORY_TYPES; i++)
    {
        sprintf(s, "\nLast several %s:\n\n", desc[i]);
        strcat(buf, s);

        for (j = 0; j < ZONE_HISTORY; j++)
        {
            int k = (history_index[i] - j - 1) & (ZONE_HISTORY - 1);

            if (file_history[i][k])
            {
                sprintf(s, "File: %s, Line: %d\n", file_history[i][k], line_history[i][k]);
                strcat(buf, s);
            }
        }
    }
}

int ABS(int a)
{
    int b = a >> 31;

    return ((a ^ b) - b);
}

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength(lumpindex_t lump)
{
    if (lump >= numlumps)
    {
        I_Error ("W_LumpLength: %i >= numlumps", lump);
    }

    return lumpinfo[lump]->size;
}

#ifdef HEAPDUMP
void W_PrintLump(FILE *fp, void *p)
{
    int i;

    for (i = 0; i < numlumps; i++)
        if (lumpinfo[i]->cache == p)
        {
            fprintf(fp, " %8.8s %6d %2u %6d", lumpinfo[i]->name,
                    W_LumpLength(i), lumpinfo[i]->locks, gametic - lumpinfo[i]->locktic);
            return;
        }

    fprintf(fp, " not found");
}
#endif

