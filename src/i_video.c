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
#include "i_video.h"
#include "i_scale.h"
#include "m_config.h"
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

// if true, I_VideoBuffer is screen->pixels

static boolean native_surface;

// Screen width and height, from configuration file.

int screen_width = SCREENWIDTH;
int screen_height = SCREENHEIGHT;

// Color depth.

int screen_bpp = 0;

// Run in full screen mode?  (int type for config code)

int fullscreen = false;

// Aspect ratio correction mode

int aspect_ratio_correct = false;

// Time to wait for the screen to settle on startup before starting the
// game (ms)

static int startup_delay = 1000;

// The screen buffer; this is modified to draw things to the screen

byte *I_VideoBuffer = NULL;

// Flag indicating whether the screen is currently visible:
// when the screen isnt visible, don't render the screen

boolean screenvisible;

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

patch_t *disk;

extern int screenSize;

// Set the variable controlling FPS dots.

void I_DisplayFPSDots(boolean dots_on)
{
    display_fps_dots = dots_on;
}

// Update the value of window_focused when we get a focus event
//
// We try to make ourselves be well-behaved: the grab on the mouse
// is removed if we lose focus (such as a popup window appearing),
// and we dont move the mouse around if we aren't focused either.

static void UpdateFocus(void)
{
    Uint8 state;

    state = SDL_GetAppState();

    // Should the screen be grabbed?

    screenvisible = (state & SDL_APPACTIVE) != 0;
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

    V_UseBuffer(tmpbuf);

    // Draw the disk to the screen:

    V_DrawPatch(0, 0, disk);

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
    V_RestoreBuffer();
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
        I_InitScale(I_VideoBuffer,
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

void I_BeginRead(void)
{
    if (!initialized || disk_image == NULL)
        return;

    if(screenSize > 6)                                          // THIS WORKS GOOD FOR PSP AND
        V_DrawPatch(ORIGWIDTH - LOADING_DISK_W + 16, -1, disk); // IS ALSO A FIX FOR THE WII
}

void I_EndRead(void)
{
    if (!initialized || disk_image == NULL)
        return;

    if(screenSize > 6)                                          // THIS WORKS GOOD FOR PSP AND
        V_DrawPatch(ORIGWIDTH - LOADING_DISK_W + 16, -1, disk); // IS ALSO A FIX FOR THE WII
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

    flags |= SDL_SWSURFACE | SDL_DOUBLEBUF;

    if (screen_bpp == 8)
    {
        flags |= SDL_HWPALETTE;
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

    // Save screen mode.

    screen_mode = mode;

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
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
    static int  lasttic;
    int         tics;
    int         i;

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
            I_VideoBuffer[ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
        for ( ; i<20*4 ; i+=4)
            I_VideoBuffer[ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }

    FinishUpdateSoftware();
}


//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy(scr, I_VideoBuffer, SCREENWIDTH*SCREENHEIGHT);
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

void I_InitGraphics(void)
{
    SDL_Event dummy;
    byte *doompal;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
    {
        I_Error("Failed to initialize video: %s", SDL_GetError());
    }

    //
    // Enter into graphics mode.
    //
    int w, h;

    w = screen_width;
    h = screen_height;

    C_Printf(CR_GRAY, " I_InitGraphics: Resolution is 640 x 400 x 32 bpp\n");
    C_Printf(CR_GRAY, " Scaling to aspect ratio 16:9 in software mode\n");
    C_Printf(CR_GRAY, " Using 256-color palette from PLAYPAL lump.\n");

    if(!usegamma)
        C_Printf(CR_GRAY, " Gamma correction is off.\n");
    else
        C_Printf(CR_GOLD, " Gamma correction is enabled.\n");

    screen_mode = I_FindScreenMode(w, h);

    if (screen_mode == NULL)
    {
        I_Error("I_InitGraphics: Unable to find a screen mode small "
                "enough for %ix%i", w, h);
    }

    if (w != screen_mode->width || h != screen_mode->height)
    {
        C_Printf(CR_GRAY, "I_InitGraphics: %s (%ix%i within %ix%i)\n",
              WindowBoxType(screen_mode, w, h),
              screen_mode->width, screen_mode->height, w, h);
    }
    SetVideoMode(screen_mode, w, h);

    // Set the palette

    doompal = W_CacheLumpName(DEH_String("PLAYPAL"), PU_CACHE);
    I_SetPalette(doompal);

    SDL_SetColors(screenbuffer, palette, 0, 256);

    // Start with a clear black screen
    // (screen will be flipped after we set the palette)

    SDL_FillRect(screenbuffer, NULL, 0);

    UpdateFocus();

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

    if (native_surface)
    {
        I_VideoBuffer = (unsigned char *) screen->pixels;

        I_VideoBuffer += (screen->h - SCREENHEIGHT) / 2;
    }
    else
    {
        I_VideoBuffer = (unsigned char *) Z_Malloc(SCREENWIDTH * SCREENHEIGHT,
                                                   PU_STATIC, NULL);
    }

    V_RestoreBuffer();

    // Clear the screen to black.

    memset(I_VideoBuffer, 0, SCREENWIDTH * SCREENHEIGHT);

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

