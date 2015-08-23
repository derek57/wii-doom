//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
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
// DESCRIPTION:
//        DOOM graphics stuff for SDL.
//


#include <ctype.h>
#include <math.h>
#include <SDL/SDL.h>
#include <SDL/SDL_stdinc.h>
#include <stdlib.h>
#include <string.h>

#include "c_io.h"
#include "config.h"
#include "deh_str.h"
#include "doomdef.h"
#include "doomkeys.h"
#include "doomtype.h"
#include "i_joystick.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_tinttab.h"
#include "i_video.h"
#include "i_scale.h"
#include "m_config.h"
#include "m_menu.h"
#include "m_misc.h"
#include "tables.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include <wiiuse/wpad.h>
#include <wiilight.h>


#define WII_LIGHT_OFF  0
#define WII_LIGHT_ON   1
#define LOADING_DISK_W (16 << hires) // CHANGED FOR HIRES
#define LOADING_DISK_H (16 << hires) // CHANGED FOR HIRES


// WiiLightControl
void WiiLightControl (int state)
{
    switch (state)
    {
        // Turn on Wii Light
        case WII_LIGHT_ON:
            WIILIGHT_SetLevel(255);
            WIILIGHT_TurnOn();
            break;

        // Turn off Wii Light
        case WII_LIGHT_OFF:
        default:
            WIILIGHT_SetLevel(0);
            WIILIGHT_TurnOff();
            WIILIGHT_Toggle();
            break;
    }
}

// Non aspect ratio-corrected modes (direct multiples of 320x200)

static screen_mode_t *screen_modes[] = {
    &mode_scale_1x,
    &mode_scale_2x,
    &mode_scale_3x,
    &mode_scale_4x,
    &mode_scale_5x,
};

// Aspect ratio corrected modes (4:3 ratio)

static screen_mode_t *screen_modes_corrected[] = {

    // Vertically stretched modes (320x200 -> 320x240 and multiples)

    &mode_stretch_1x,
    &mode_stretch_2x,
    &mode_stretch_3x,
    &mode_stretch_4x,
    &mode_stretch_5x,

    // Horizontally squashed modes (320x200 -> 256x200 and multiples)

    &mode_squash_1x,
    &mode_squash_1p5x,        // ADDED FOR HIRES
    &mode_squash_2x,
    &mode_squash_3x,
    &mode_squash_4x,
};

// SDL surface for the screen.

static SDL_Surface *screen;

// Intermediate 8-bit buffer that we draw to instead of 'screen'.
// This is used when we are rendering in 32-bit screen mode.
// When in a real 8-bit screen mode, screenbuffer == screen.

static SDL_Surface *screenbuffer = NULL;

// Palette:

static SDL_Color palette[256];
static boolean palette_to_set;

// display has been set up?

static boolean initialized = false;

// if true, screen buffer is screen->pixels

static boolean native_surface;

// Screen width and height, from configuration file.

int screen_width = SCREENWIDTH;
int screen_height = SCREENHEIGHT;

// Color depth.

int screen_bpp = 0;

// Automatically adjust video settings if the selected mode is 
// not a valid video mode.

static int autoadjust_video_settings = 1;

// Run in full screen mode?  (int type for config code)

int fullscreen = true;

// Aspect ratio correction mode

int aspect_ratio_correct = false;

// Time to wait for the screen to settle on startup before starting the
// game (ms)

static int startup_delay = 1000;

// If true, we display dots at the bottom of the screen to 
// indicate FPS.

static boolean display_fps_dots;

// If this is true, the screen is rendered but not blitted to the
// video buffer.

static boolean noblit;

// disk image data and background overwritten by the disk to be
// restored by EndRead

static byte *disk_image = NULL;
static byte *saved_background;

// The screen mode and scale functions being used

static screen_mode_t *screen_mode;

// If true, keyboard mapping is ignored, like in Vanilla Doom.
// The sensible thing to do is to disable this if you have a non-US
// keyboard.

int vanilla_keyboard_mapping = true;

// Gamma correction level to use

int usegamma = 0;

// Save screenshots in PNG format.

int png_screenshots = 0;

patch_t *disk;

extern int screenSize;

extern int display_fps;

// Set the variable controlling FPS dots.

void I_DisplayFPSDots(boolean dots_on)
{
    display_fps_dots = dots_on;
}

void I_EnableLoadingDisk(void)
{
    byte *tmpbuf;
    char *disk_name;
    int y;
    char buf[20];

    SDL_VideoDriverName(buf, 15);

    if (!strcmp(buf, "Quartz"))
    {
        // MacOS Quartz gives us pageflipped graphics that screw up the 
        // display when we use the loading disk.  Disable it.
        // This is a gross hack.

        return;
    }
/*
    if (M_CheckParm("-cdrom") > 0)
        disk_name = DEH_String("STCDROM");
    else
*/
        disk_name = DEH_String("STDISK");

    disk = W_CacheLumpName(disk_name, PU_STATIC);

    // Draw the patch into a temporary buffer
    tmpbuf = Z_Malloc(SCREENWIDTH *
             ((disk->height + 1) << hires), PU_STATIC, NULL);        // CHANGED FOR HIRES

    // Draw the disk to the screen:

    V_DrawPatch(0, 0, 0, disk);

    disk_image = Z_Malloc(LOADING_DISK_W * LOADING_DISK_H, PU_STATIC, NULL);
    saved_background = Z_Malloc(LOADING_DISK_W * LOADING_DISK_H, PU_STATIC, NULL);

    for (y=0; y<LOADING_DISK_H; ++y) 
    {
        memcpy(disk_image + LOADING_DISK_W * y,
               tmpbuf + SCREENWIDTH * y,
               LOADING_DISK_W);
    }

    // All done - free the screen buffer and restore the normal 
    // video buffer.

    W_ReleaseLumpName(disk_name);
    Z_Free(tmpbuf);
}

void I_ShutdownGraphics(void)
{
    if (initialized)
    {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);

        initialized = false;
    }
}



//
// I_StartFrame
//
void I_StartFrame (void)
{
    // er?

}

//
// I_StartTic
//
void I_StartTic (void)
{
    if (!initialized)
    {
        return;
    }
    I_UpdateJoystick();
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

// Update a small portion of the screen
//
// Does stretching and buffer blitting if neccessary
//
// Return true if blit was successful.

static boolean BlitArea(int x1, int y1, int x2, int y2)
{
    int x_offset, y_offset;
    boolean result;

    // No blit needed on native surface

    if (native_surface)
    {
        return true;
    }

    x_offset = (screenbuffer->w - screen_mode->width) / 2;
    y_offset = (screenbuffer->h - screen_mode->height) / 2;

    if (SDL_LockSurface(screenbuffer) >= 0)
    {
        I_InitScale(screens[0],
                    (byte *) screenbuffer->pixels
                                + (y_offset * screenbuffer->pitch)
                                + x_offset,
                    screenbuffer->pitch);
        result = screen_mode->DrawScreen(x1, y1, x2, y2);
        SDL_UnlockSurface(screenbuffer);
    }
    else
    {
        result = false;
    }

    return result;
}

static void UpdateRect(int x1, int y1, int x2, int y2)
{
    int x1_scaled, x2_scaled, y1_scaled, y2_scaled;

    // Do stretching and blitting

    if (BlitArea(x1, y1, x2, y2))
    {
        // Update the area

        x1_scaled = (x1 * screen_mode->width) / SCREENWIDTH;
        y1_scaled = (y1 * screen_mode->height) / SCREENHEIGHT;
        x2_scaled = (x2 * screen_mode->width) / SCREENWIDTH;
        y2_scaled = (y2 * screen_mode->height) / SCREENHEIGHT;

        SDL_UpdateRect(screen,
                       x1_scaled, y1_scaled,
                       x2_scaled - x1_scaled,
                       y2_scaled - y1_scaled);
    }
}

void I_BeginRead(void)
{
    byte *screenloc = screens[0]
                    + (SCREENHEIGHT - LOADING_DISK_H) * SCREENWIDTH
                    + (SCREENWIDTH - LOADING_DISK_W);
    int y;

    if (!initialized || disk_image == NULL)
        return;

    // save background and copy the disk image in

    for (y=0; y<LOADING_DISK_H; ++y)
    {
        memcpy(screens[1] + y * LOADING_DISK_W,
               screenloc,
               LOADING_DISK_W);

        memcpy(screenloc,
               disk_image + y * LOADING_DISK_W,
               LOADING_DISK_W);

        screenloc += SCREENWIDTH;
    }

    UpdateRect(SCREENWIDTH - LOADING_DISK_W, SCREENHEIGHT - LOADING_DISK_H,
               SCREENWIDTH, SCREENHEIGHT);
}

void I_EndRead(void)
{
    byte *screenloc = screens[0]
                    + (SCREENHEIGHT - LOADING_DISK_H) * SCREENWIDTH
                    + (SCREENWIDTH - LOADING_DISK_W);
    int y;

    if (!initialized || disk_image == NULL)
        return;

    // save background and copy the disk image in

    for (y=0; y<LOADING_DISK_H; ++y)
    {
        memcpy(screenloc,
               screens[1] + y * LOADING_DISK_W,
               LOADING_DISK_W);

        screenloc += SCREENWIDTH;
    }

    UpdateRect(SCREENWIDTH - LOADING_DISK_W, SCREENHEIGHT - LOADING_DISK_H,
               SCREENWIDTH, SCREENHEIGHT);
}

// Ending of I_FinishUpdate() when in software scaling mode.

static void FinishUpdateSoftware(void)
{
    // draw to screen

    BlitArea(0, 0, SCREENWIDTH, SCREENHEIGHT);

    if (palette_to_set)
    {
        SDL_SetColors(screenbuffer, palette, 0, 256);
        palette_to_set = false;

        // In native 8-bit mode, if we have a palette to set, the act
        // of setting the palette updates the screen

        if (screenbuffer == screen)
        {
            return;
        }
    }

    // In 8in32 mode, we must blit from the fake 8-bit screen buffer
    // to the real screen before doing a screen flip.

    if (screenbuffer != screen)
    {
        SDL_Rect dst_rect;

        // Center the buffer within the full screen space.

        dst_rect.x = (screen->w - screenbuffer->w) / 2;
        dst_rect.y = (screen->h - screenbuffer->h) / 2;

        SDL_BlitSurface(screenbuffer, NULL, screen, &dst_rect);
    }

    SDL_Flip(screen);
}

// Pick the modes list to use:

static void GetScreenModes(screen_mode_t ***modes_list, int *num_modes)
{
    if (aspect_ratio_correct)
    {
        *modes_list = screen_modes_corrected;
        *num_modes = arrlen(screen_modes_corrected);
    }
    else
    {
        *modes_list = screen_modes;
        *num_modes = arrlen(screen_modes);
    }
}

// Find which screen_mode_t to use for the given width and height.

static screen_mode_t *I_FindScreenMode(int w, int h)
{
    screen_mode_t **modes_list;
    screen_mode_t *best_mode;
    int modes_list_length;
    int num_pixels;
    int best_num_pixels;
    int i;

    // Special case: 320x200 and 640x400 are available even if aspect 
    // ratio correction is turned on.  These modes have non-square
    // pixels.

    if (fullscreen)
    {
        if (w == SCREENWIDTH && h == SCREENHEIGHT)
        {
            return &mode_scale_1x;
        }
        else if (w == SCREENWIDTH*2 && h == SCREENHEIGHT*2 && !hires) // CHANGED FOR HIRES
        {
            return &mode_scale_2x;
        }
    }

    GetScreenModes(&modes_list, &modes_list_length);

    // Find the biggest screen_mode_t in the list that fits within these 
    // dimensions

    best_mode = NULL;
    best_num_pixels = 0;

    for (i=0; i<modes_list_length; ++i) 
    {
        // Will this fit within the dimensions? If not, ignore.

        if (modes_list[i]->width > w || modes_list[i]->height > h)
        {
            continue;
        }

        num_pixels = modes_list[i]->width * modes_list[i]->height;

        if (num_pixels > best_num_pixels)
        {
            // This is a better mode than the current one

            best_mode = modes_list[i];
            best_num_pixels = num_pixels;
        }
    }

    return best_mode;
}

static void SetVideoMode(screen_mode_t *mode, int w, int h)
{
    byte *doompal;
    int flags = 0;

    doompal = W_CacheLumpName(DEH_String("PLAYPAL"), PU_CACHE);

    // If we are already running and in a true color mode, we need
    // to free the screenbuffer surface before setting the new mode.

    if (screenbuffer != NULL && screen != screenbuffer)
    {
        SDL_FreeSurface(screenbuffer);
    }

    // Perform screen scale setup before changing video mode.

    if (mode != NULL && mode->InitMode != NULL)
    {
        mode->InitMode(doompal);
    }

    // Set the video mode.

    flags |= SDL_SWSURFACE | SDL_DOUBLEBUF;

    if (screen_bpp == 8)
    {
        flags |= SDL_HWPALETTE;
    }

    if (fullscreen)
    {
        flags |= SDL_FULLSCREEN;
    }
    else
    {
        // In windowed mode, the window can be resized while the game is
        // running. Mac OS X has a quirk where an ugly resize handle is
        // shown in software mode when resizing is enabled, so avoid that.
        flags |= SDL_RESIZABLE;
    }

    screen = SDL_SetVideoMode(w, h, screen_bpp, flags);

    if (screen == NULL)
    {
        I_Error("Error setting video mode %ix%ix%ibpp: %s\n",
                w, h, screen_bpp, SDL_GetError());
    }

    // Blank out the full screen area in case there is any junk in
    // the borders that won't otherwise be overwritten.

    SDL_FillRect(screen, NULL, 0);

    // If mode was not set, it must be set now that we know the
    // screen size.

    if (mode == NULL)
    {
        mode = I_FindScreenMode(screen->w, screen->h);

        if (mode == NULL)
        {
            I_Error("I_InitGraphics: Unable to find a screen mode small "
                    "enough for %ix%i", screen->w, screen->h);
        }

        // Generate lookup tables before setting the video mode.

        if (mode->InitMode != NULL)
        {
            mode->InitMode(doompal);
        }
    }

    // Create the screenbuffer surface; if we have a real 8-bit palettized
    // screen, then we can use the screen as the screenbuffer.

    if (screen->format->BitsPerPixel == 8)
    {
        screenbuffer = screen;
    }
    else
    {
        screenbuffer = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                            mode->width, mode->height, 8,
                                            0, 0, 0, 0);

        SDL_FillRect(screenbuffer, NULL, 0);
    }

    // Save screen mode.

    screen_mode = mode;
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
    static int  lasttic;
    static int  lastmili;
    static int  fpscount;

    static char fpsbuf[5];

    int         tics;
    int         i;
    int         mili;

    if (!initialized)
        return;

    if (noblit)
        return;

    // Don't update the screen if the window isn't visible.
    // Not doing this breaks under Windows when we alt-tab away 
    // while fullscreen.

    if (!(SDL_GetAppState() & SDL_APPACTIVE))
        return;

    // draws little dots on the bottom of the screen

    if (display_fps_dots)
    {
	i = I_GetTime();
	tics = i - lasttic;
	lasttic = i;

	if (tics > 20) tics = 20;

	for (i=0 ; i<tics*4 ; i+=4)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;

	for ( ; i<20*4 ; i+=4)
	    screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }

    // [AM] Real FPS counter
    if (display_fps)
    {
        fpscount += 1;
        i = SDL_GetTicks();
        mili = i - lastmili;

        // Update FPS counter every 100ms
        if (mili >= 100)
        {
            SDL_itoa(((fpscount * 1000) / mili), fpsbuf, 10);
            fpscount = 0;
            lastmili = i;
        }
        M_WriteText(ORIGWIDTH - 30 - (8 * 3), 0, "FPS: ");
        M_WriteText(ORIGWIDTH - (8 * 3), 0, fpsbuf);
    }

    FinishUpdateSoftware();
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy(scr, screens[0], SCREENWIDTH * SCREENHEIGHT);
}


//
// I_SetPalette
//
void I_SetPalette (byte *doompalette)
{
    int i;

    for (i=0; i<256; ++i)
    {
        // Zero out the bottom two bits of each channel - the PC VGA
        // controller only supports 6 bits of accuracy.

        palette[i].r = gammatable[usegamma][*doompalette++] & ~3;
        palette[i].g = gammatable[usegamma][*doompalette++] & ~3;
        palette[i].b = gammatable[usegamma][*doompalette++] & ~3;
    }

    palette_to_set = true;
}

// Given an RGB value, find the closest matching palette index.

int I_GetPaletteIndex(int r, int g, int b)
{
    int best, best_diff, diff;
    int i;

    best = 0; best_diff = INT_MAX;

    for (i = 0; i < 256; ++i)
    {
        diff = (r - palette[i].r) * (r - palette[i].r)
             + (g - palette[i].g) * (g - palette[i].g)
             + (b - palette[i].b) * (b - palette[i].b);

        if (diff < best_diff)
        {
            best = i;
            best_diff = diff;
        }

        if (diff == 0)
        {
            break;
        }
    }

    return best;
}

static char *WindowBoxType(screen_mode_t *mode, int w, int h)
{
    if (mode->width != w && mode->height != h) 
    {
        return "Windowboxed";
    }
    else if (mode->width == w) 
    {
        return "Letterboxed";
    }
    else if (mode->height == h)
    {
        return "Pillarboxed";
    }
    else
    {
        return "...";
    }
}

// Adjust to an appropriate fullscreen mode.
// Returns true if successful.

static boolean AutoAdjustFullscreen(void)
{
    SDL_Rect **modes;
    SDL_Rect *best_mode;
    screen_mode_t *screen_mode;
    int diff, best_diff;
    int i;

    modes = SDL_ListModes(NULL, SDL_FULLSCREEN);

    // No fullscreen modes available at all?

    if (modes == NULL || modes == (SDL_Rect **) -1 || *modes == NULL)
    {
        return false;
    }

    // Find the best mode that matches the mode specified in the
    // configuration file

    best_mode = NULL;
    best_diff = INT_MAX;

    for (i=0; modes[i] != NULL; ++i)
    {
        //printf("%ix%i?\n", modes[i]->w, modes[i]->h);

        // What screen_mode_t would be used for this video mode?

        screen_mode = I_FindScreenMode(modes[i]->w, modes[i]->h);

        // Never choose a screen mode that we cannot run in, or
        // is poor quality for fullscreen

        if (screen_mode == NULL || screen_mode->poor_quality)
        {
        //    printf("\tUnsupported / poor quality\n");
            continue;
        }

        // Do we have the exact mode?
        // If so, no autoadjust needed

        if (screen_width == modes[i]->w && screen_height == modes[i]->h)
        {
        //    printf("\tExact mode!\n");
            return true;
        }

        // Is this mode better than the current mode?

        diff = (screen_width - modes[i]->w) * (screen_width - modes[i]->w)
             + (screen_height - modes[i]->h) * (screen_height - modes[i]->h);

        if (diff < best_diff)
        {
        //    printf("\tA valid mode\n");
            best_mode = modes[i];
            best_diff = diff;
        }
    }

    if (best_mode == NULL)
    {
        // Unable to find a valid mode!

        return false;
    }

    C_Printf(CR_GRAY, " I_InitGraphics: %i x %i mode not supported on this machine.\n",
           screen_width, screen_height);

    screen_width = best_mode->w;
    screen_height = best_mode->h;

    return true;
}

// Auto-adjust to a valid windowed mode.

static void AutoAdjustWindowed(void)
{
    screen_mode_t *best_mode;

    // Find a screen_mode_t to fit within the current settings

    best_mode = I_FindScreenMode(screen_width, screen_height);

    if (best_mode == NULL)
    {
        // Nothing fits within the current settings.
        // Pick the closest to 320x200 possible.

        best_mode = I_FindScreenMode(SCREENWIDTH, SCREENHEIGHT_4_3);
    }

    // Switch to the best mode if necessary.

    if (best_mode->width != screen_width || best_mode->height != screen_height)
    {
        C_Printf(CR_GRAY, " I_InitGraphics: Cannot run at specified mode: %i x %i\n",
               screen_width, screen_height);

        screen_width = best_mode->width;
        screen_height = best_mode->height;
    }
}

// Auto-adjust to a valid color depth.

static void AutoAdjustColorDepth(void)
{
    SDL_Rect **modes;
    SDL_PixelFormat format;
    const SDL_VideoInfo *info;
    int flags;

    // If screen_bpp=0, we should use the current (default) pixel depth.
    // Fetch it from SDL.

    if (screen_bpp == 0)
    {
        info = SDL_GetVideoInfo();

        if (info != NULL && info->vfmt != NULL)
        {
            screen_bpp = info->vfmt->BitsPerPixel;
        }
    }

    if (fullscreen)
    {
        flags = SDL_FULLSCREEN;
    }
    else
    {
        flags = 0;
    }

    format.BitsPerPixel = screen_bpp;
    format.BytesPerPixel = (screen_bpp + 7) / 8;

    // Are any screen modes supported at the configured color depth?

    modes = SDL_ListModes(&format, flags);

    // If not, we must autoadjust to something sensible.

    if (modes == NULL)
    {
        C_Printf(CR_GRAY, " I_InitGraphics: %i bpp color depth not supported.\n",
               screen_bpp);

        info = SDL_GetVideoInfo();

        if (info != NULL && info->vfmt != NULL)
        {
            screen_bpp = info->vfmt->BitsPerPixel;
        }
    }
}

// If the video mode set in the configuration file is not available,
// try to choose a different mode.

static void I_AutoAdjustSettings(void)
{
    int old_screen_w, old_screen_h, old_screen_bpp;

    old_screen_w = screen_width;
    old_screen_h = screen_height;
    old_screen_bpp = screen_bpp;

    // Possibly adjust color depth.

    AutoAdjustColorDepth();

    // If we are running fullscreen, try to autoadjust to a valid fullscreen
    // mode.  If this is impossible, switch to windowed.

    if (fullscreen && !AutoAdjustFullscreen())
    {
        fullscreen = 0;
    }

    // If we are running windowed, pick a valid window size.

    if (!fullscreen)
    {
        AutoAdjustWindowed();
    }

    // Have the settings changed?  Show a message.

    if (screen_width != old_screen_w || screen_height != old_screen_h
     || screen_bpp != old_screen_bpp)
    {
        C_Printf(CR_GRAY, " I_InitGraphics: Auto-adjusted to %i x %i x %i bpp.\n",
               screen_width, screen_height, screen_bpp);

        C_Printf(CR_GOLD, " NOTE: Your video settings have been adjusted.\n");
    }
}

void I_InitGraphics(void)
{
    SDL_Event dummy;
    byte      *doompal = W_CacheLumpName("PLAYPAL", PU_CACHE);

    wad_file_t *playpalwad = lumpinfo[W_CheckNumForName("PLAYPAL")]->wad_file;

    I_InitTintTables(doompal);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
    {
        I_Error("Failed to initialize video: %s", SDL_GetError());
    }

    //
    // Enter into graphics mode.
    //
    int w, h;

    if (autoadjust_video_settings)
    {
        I_AutoAdjustSettings();
    }

    w = screen_width;
    h = screen_height;

    C_Printf(CR_GRAY, " Scaling to aspect ratio 16:9 in software mode\n");
    C_Printf(CR_GRAY, " Using 256-color palette from PLAYPAL lump in %s file %s",
            (playpalwad->type == IWAD ? "IWAD" : "PWAD"), uppercase(playpalwad->path));

//    C_Printf(CR_GRAY, "         %s.");

    if(!usegamma)
        C_Printf(CR_GRAY, " Gamma correction is off.\n");
    else
        C_Printf(CR_GOLD, " Gamma correction is enabled.\n");

    screen_mode = I_FindScreenMode(w, h);

    if (screen_mode == NULL)
    {
        I_Error("I_InitGraphics: Unable to find a screen mode small "
                "enough for %i x %i", w, h);
    }

    if (w != screen_mode->width || h != screen_mode->height)
    {
        C_Printf(CR_GRAY, " I_InitGraphics: %s (%i x %i within %i x %i)\n",
              WindowBoxType(screen_mode, w, h),
              screen_mode->width, screen_mode->height, w, h);
    }
    SetVideoMode(screen_mode, w, h);

    // Start with a clear black screen
    // (screen will be flipped after we set the palette)

    SDL_FillRect(screenbuffer, NULL, 0);

    // Set the palette

    I_SetPalette(doompal);

    SDL_SetColors(screenbuffer, palette, 0, 256);

    // On some systems, it takes a second or so for the screen to settle
    // after changing modes.  We include the option to add a delay when
    // setting the screen mode, so that the game doesn't start immediately
    // with the player unable to see anything.

    if (fullscreen)
    {
        SDL_Delay(startup_delay);
    }

    // Check if we have a native surface we can use
    // If we have to lock the screen, draw to a buffer and copy
    // Likewise if the screen pitch is not the same as the width
    // If we have to multiply, drawing is done to a separate 320x200 buf

    native_surface = screen == screenbuffer
                  && !SDL_MUSTLOCK(screen)
                  && screen_mode == &mode_scale_1x
                  && screen->pitch == SCREENWIDTH
                  && aspect_ratio_correct;

    // If not, allocate a buffer and copy from that buffer to the
    // screen when we do an update

    screens[0] = Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);
    memset(screens[0], 0, SCREENWIDTH * SCREENHEIGHT);

    // We need SDL to give us translated versions of keys as well

    SDL_EnableUNICODE(1);

    // Repeat key presses - this is what Vanilla Doom does
    // Not sure about repeat rate - probably dependent on which DOS
    // driver is used.  This is good enough though.

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    // Clear out any events waiting at the start:

    while (SDL_PollEvent(&dummy));

    initialized = true;

    // Call I_ShutdownGraphics on quit

    I_AtExit(I_ShutdownGraphics, true);
}

