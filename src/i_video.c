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

#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_stdinc.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "c_io.h"
#include "config.h"
#include "d_deh.h"

#include "doom/doomdef.h"

#include "doomkeys.h"
#include "doomtype.h"

#ifndef WII
#include "i_glscale.h"
#endif

#include "i_joystick.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_timer.h"
#include "i_tinttab.h"
#include "i_video.h"
#include "i_scale.h"
#include "icon.c"
#include "m_argv.h"
#include "m_config.h"

#include "doom/m_menu.h"

#include "m_misc.h"
#include "tables.h"
#include "v_trans.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#ifdef WII
#include <wiiuse/wpad.h>
#include <wiilight.h>


#define WII_LIGHT_OFF  0
#define WII_LIGHT_ON   1


//
// WiiLightControl
//
// [nitr8] UNUSED
//
/*
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
*/
#endif

// Non aspect ratio-corrected modes (direct multiples of 320x200)

#ifndef SDL2
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
#endif


// Intermediate 8-bit buffer that we draw to instead of 'screen'.
// This is used when we are rendering in 32-bit screen mode.
// When in a real 8-bit screen mode, screenbuffer == screen.

static SDL_Surface *screenbuffer = NULL;

// Palette:

static SDL_Color palette[256];
static dboolean palette_to_set;

// display has been set up?

/*static*/ dboolean initialized = false;

// if true, screen buffer is screen->pixels

#ifndef SDL2
//static dboolean native_surface;

// Color depth.

int screen_bpp = 0;

// Automatically adjust video settings if the selected mode is 
// not a valid video mode.

static int autoadjust_video_settings = 1;
#endif

// Screen width and height, from configuration file.

int screen_width = SCREENWIDTH;
int screen_height = SCREENHEIGHT;

// Run in full screen mode?  (int type for config code)

int fullscreen = false;

// Aspect ratio correction mode

int aspect_ratio_correct = false;

// Time to wait for the screen to settle on startup before starting the
// game (ms)

static int startup_delay = 1000;

// If true, we display dots at the bottom of the screen to 
// indicate FPS.

static dboolean display_fps_dots;

// If this is true, the screen is rendered but not blitted to the
// video buffer.

static dboolean noblit;

// disk image patch (either STDISK or STCDROM) and
// background overwritten by the disk to be restored by EndRead

static patch_t *disk;

// The screen mode and scale functions being used

static screen_mode_t *screen_mode;

// If true, keyboard mapping is ignored, like in Vanilla Doom.
// The sensible thing to do is to disable this if you have a non-US
// keyboard.

int vanilla_keyboard_mapping = true;

// Save screenshots in PNG format.

int png_screenshots = 0;

// The screen buffer; this is modified to draw things to the screen

//byte *I_VideoBuffer = NULL;

// Window title

static char *window_title = "";

static int loading_disk_xoffs = 0;
static int loading_disk_yoffs = 0;

byte                    gammatable[GAMMALEVELS][256];

float                   gammalevels[GAMMALEVELS] =
{
    // Darker
    0.50f, 0.55f, 0.60f, 0.65f, 0.70f, 0.75f, 0.80f, 0.85f, 0.90f, 0.95f,

    // No gamma correction
    1.0f,

    // Lighter
    1.05f, 1.10f, 1.15f, 1.20f, 1.25f, 1.30f, 1.35f, 1.40f, 1.45f, 1.50f,
    1.55f, 1.60f, 1.65f, 1.70f, 1.75f, 1.80f, 1.85f, 1.90f, 1.95f, 2.0f
};


// Window resize state.                                        << NEW

#ifndef WII
static dboolean need_resize = false;
static unsigned int resize_w, resize_h;
static unsigned int last_resize_time;

// Is the shift key currently down?

static int shiftdown = 0;

static dboolean window_focused;

// If true, game is running as a screensaver

dboolean screensaver_mode = false;

dboolean mouse_grabbed = false;

// Flag indicating whether the screen is currently visible:
// when the screen isnt visible, don't render the screen

#ifdef SDL2
dboolean screenvisible = true;
#else
dboolean screenvisible;
#endif

// disable mouse?

static dboolean nomouse = false;
int usemouse = 1;

// Bit mask of mouse button state.

static unsigned int mouse_button_state = 0;

// Disallow mouse and joystick movement to cause forward/backward
// motion.  Specified with the '-novert' command line parameter.
// This is an int to allow saving to config file

int novert = 0;

// Grab the mouse? (int type for config code)

static int grabmouse = true;

// Callback function to invoke to determine whether to grab the 
// mouse pointer.

static grabmouse_callback_t grabmouse_callback = NULL;

// Empty mouse cursor

static SDL_Cursor *cursors[2];

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

float mouse_acceleration = 2.0;
int mouse_threshold = 10;

// Window position:

static char *window_position = "";

// SDL video driver name

char *video_driver = "";
#endif

extern dboolean blurred;

// If true, we are rendering the screen using OpenGL hardware scaling
// rather than software mode.

#ifdef WII
static dboolean using_opengl = false;
#elif !defined SDL2
static dboolean using_opengl = true;
#endif

// These are (1) the window (or the full screen) that our game is rendered to
// and (2) the renderer that scales the texture (see below) into this window.

#ifdef SDL2
static SDL_Window *screen;
static SDL_Renderer *renderer;

static const int scancode_translate_table[] = SCANCODE_TO_KEYS_ARRAY;

// These are (1) the 320x200x8 paletted buffer that we draw to (i.e. the one
// that holds I_VideoBuffer), (2) the 320x200x32 RGBA intermediate buffer that
// we blit the former buffer to, (3) the intermediate 320x200 texture that we
// load the RGBA buffer to and that we render into another texture (4) which
// is upscaled by an integer factor UPSCALE using "nearest" scaling and which
// in turn is finally rendered to screen using "linear" scaling.

static SDL_Surface *rgbabuffer = NULL;
static SDL_Texture *texture = NULL;
static SDL_Texture *texture_upscaled = NULL;

static SDL_Rect blit_rect = {
    .h = SCREENHEIGHT,
    .w = SCREENWIDTH,
    .x = 0,
    .y = 0,
};

#else
static SDL_Surface *screen;
#endif

// Only display the disk icon if more then this much bytes have been read
// during the previous tic.

static const int diskicon_threshold = 20*1024;
int diskicon_readbytes = 0;

// Lookup table for mapping ASCII characters to their equivalent when
// shift is pressed on an American layout keyboard:

#ifndef WII
static const char shiftxform[] =
{
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, ' ', '!', '"', '#', '$', '%', '&',
    '"', // shift-'
    '(', ')', '*', '+',
    '<', // shift-,
    '_', // shift--
    '>', // shift-.
    '?', // shift-/
    ')', // shift-0
    '!', // shift-1
    '@', // shift-2
    '#', // shift-3
    '$', // shift-4
    '%', // shift-5
    '^', // shift-6
    '&', // shift-7
    '*', // shift-8
    '(', // shift-9
    ':',
    ':', // shift-;
    '<',
    '+', // shift-=
    '>', '?', '@',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '[', // shift-[
    '!', // shift-backslash - OH MY GOD DOES WATCOM SUCK
    ']', // shift-]
    '"', '_',
    '\'', // shift-`
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '{', '|', '}', '~', 127
};

static dboolean MouseShouldBeGrabbed()
{
    // never grab the mouse when in screensaver mode
   
    if (screensaver_mode)
        return false;

    // if the window doesn't have focus, never grab it

    if (!window_focused)
        return false;

    // when menu is active or game is paused, release the mouse

    if (menuactive || consoleactive || paused)
        return false;

    // always grab the mouse when full screen (dont want to 
    // see the mouse pointer)

    if (fullscreen)
        return true;

    // Don't grab the mouse if mouse input is disabled

    if (!usemouse || nomouse)
        return false;

    // if we specify not to grab the mouse, never grab

    if (!grabmouse)
        return false;

    // Invoke the grabmouse callback function to determine whether
    // the mouse should be grabbed

    if (grabmouse_callback != NULL)
    {
        return grabmouse_callback();
    }
    else
    {
        return true;
    }
}

void I_SetGrabMouseCallback(grabmouse_callback_t func)
{
    grabmouse_callback = func;
}
#endif

// Set the variable controlling FPS dots.

void I_DisplayFPSDots(dboolean dots_on)
{
    display_fps_dots = dots_on;
}

// Update the value of window_focused when we get a focus event
//
// We try to make ourselves be well-behaved: the grab on the mouse
// is removed if we lose focus (such as a popup window appearing),
// and we dont move the mouse around if we aren't focused either.

#ifndef WII
//#ifndef SDL2
static void UpdateFocus(void)
{
    Uint8 state;

#ifdef SDL2
    state = SDL_GetWindowFlags(screen);
#else
    state = SDL_GetAppState();
#endif

    // We should have input (keyboard) focus and be visible 
    // (not minimized)

#ifdef SDL2
    window_focused = ((state & SDL_WINDOW_INPUT_FOCUS) && (state & SDL_WINDOW_SHOWN));
#else
    window_focused = (state & SDL_APPINPUTFOCUS) && (state & SDL_APPACTIVE);
#endif

    if (!window_focused && !menuactive && gamestate == GS_LEVEL && !paused && !consoleactive)
    {
        blurred = false;
    }

    // Should the screen be grabbed?

#ifndef SDL2
    screenvisible = (state & SDL_APPACTIVE) != 0;
#endif
}
//#endif

// Show or hide the mouse cursor. We have to use different techniques
// depending on the OS.

static void SetShowCursor(dboolean show)
{
    // On Windows, using SDL_ShowCursor() adds lag to the mouse input,
    // so work around this by setting an invisible cursor instead. On
    // other systems, it isn't possible to change the cursor, so this
    // hack has to be Windows-only. (Thanks to entryway for this)

    SDL_ShowCursor(show);

    // When the cursor is hidden, grab the input.

    if (!screensaver_mode)
    {
#ifdef SDL2
        SDL_SetWindowGrab(screen, !show);
#else
        SDL_WM_GrabInput(!show);
#endif
    }
}
#endif

void I_EnableLoadingDisk(int xoffs, int yoffs)
{
    char *disk_name;

    loading_disk_xoffs = xoffs;
    loading_disk_yoffs = yoffs;

    if ((
#ifndef WII
        M_CheckParm("-cdrom") > 0 ||
#endif
        icontype == 1)
       )
        disk_name = "STCDROM";
    else
        disk_name = "STDISK";

    disk = W_CacheLumpName(disk_name, PU_STATIC);
}

//
// Translates the SDL key
//

#ifndef WII
#ifdef SDL2
static int TranslateKey(SDL_Keysym *sym)
#else
static int TranslateKey(SDL_keysym *sym)
#endif
{
#ifdef SDL2
    int scancode = sym->scancode;

    switch (scancode)
    {
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL:
            return KEY_RCTRL;

        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT:
            return KEY_RSHIFT;

        case SDL_SCANCODE_LALT:
            return KEY_LALT;

        case SDL_SCANCODE_RALT:
            return KEY_RALT;

        default:
            if (scancode >= 0 && scancode < arrlen(scancode_translate_table))
            {
                return scancode_translate_table[scancode];
            }
            else
            {
                return 0;
            }
    }
#else
    switch(sym->sym)
    {
      case SDLK_LEFT:        return KEY_LEFTARROW;
      case SDLK_RIGHT:        return KEY_RIGHTARROW;
      case SDLK_DOWN:        return KEY_DOWNARROW;
      case SDLK_UP:        return KEY_UPARROW;
      case SDLK_ESCAPE:        return KEY_ESCAPE;
      case SDLK_RETURN:        return KEY_ENTER;
      case SDLK_TAB:        return KEY_TAB;
      case SDLK_F1:        return KEY_F1;
      case SDLK_F2:        return KEY_F2;
      case SDLK_F3:        return KEY_F3;
      case SDLK_F4:        return KEY_F4;
      case SDLK_F5:        return KEY_F5;
      case SDLK_F6:        return KEY_F6;
      case SDLK_F7:        return KEY_F7;
      case SDLK_F8:        return KEY_F8;
      case SDLK_F9:        return KEY_F9;
      case SDLK_F10:        return KEY_F10;
      case SDLK_F11:        return KEY_F11;
      case SDLK_F12:        return KEY_F12;
      case SDLK_PRINT:  return KEY_PRTSCR;

      case SDLK_BACKSPACE: return KEY_BACKSPACE;
      case SDLK_DELETE:        return KEY_DEL;

      case SDLK_PAUSE:        return KEY_PAUSE;

      case SDLK_EQUALS: return KEY_EQUALS;

      case SDLK_MINUS:          return KEY_MINUS;

      case SDLK_LSHIFT:
      case SDLK_RSHIFT:
        return KEY_RSHIFT;
        
      case SDLK_LCTRL:
      case SDLK_RCTRL:
        return KEY_RCTRL;
        
      case SDLK_LALT:
      case SDLK_RALT:
      case SDLK_LMETA:
      case SDLK_RMETA:
        return KEY_RALT;

      case SDLK_CAPSLOCK: return KEY_CAPSLOCK;
      case SDLK_SCROLLOCK: return KEY_SCRLCK;
      case SDLK_NUMLOCK: return KEY_NUMLOCK;

      case SDLK_KP0: return KEYP_0;
      case SDLK_KP1: return KEYP_1;
      case SDLK_KP2: return KEYP_2;
      case SDLK_KP3: return KEYP_3;
      case SDLK_KP4: return KEYP_4;
      case SDLK_KP5: return KEYP_5;
      case SDLK_KP6: return KEYP_6;
      case SDLK_KP7: return KEYP_7;
      case SDLK_KP8: return KEYP_8;
      case SDLK_KP9: return KEYP_9;

      case SDLK_KP_PERIOD:   return KEYP_PERIOD;
      case SDLK_KP_MULTIPLY: return KEYP_MULTIPLY;
      case SDLK_KP_PLUS:     return KEYP_PLUS;
      case SDLK_KP_MINUS:    return KEYP_MINUS;
      case SDLK_KP_DIVIDE:   return KEYP_DIVIDE;
      case SDLK_KP_EQUALS:   return KEYP_EQUALS;
      case SDLK_KP_ENTER:    return KEYP_ENTER;

      case SDLK_HOME: return KEY_HOME;
      case SDLK_INSERT: return KEY_INS;
      case SDLK_END: return KEY_END;
      case SDLK_PAGEUP: return KEY_PGUP;
      case SDLK_PAGEDOWN: return KEY_PGDN;

#ifdef SDL_HAVE_APP_KEYS
        case SDLK_APP1:        return KEY_F1;
        case SDLK_APP2:        return KEY_F2;
        case SDLK_APP3:        return KEY_F3;
        case SDLK_APP4:        return KEY_F4;
        case SDLK_APP5:        return KEY_F5;
        case SDLK_APP6:        return KEY_F6;
#endif

      default:
        return tolower(sym->sym);
    }
#endif
}
#endif

void I_ShutdownGraphics(void)
{
    if (initialized)
    {
#ifndef WII
        SetShowCursor(true);
#endif
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

#ifndef WII
static void UpdateMouseButtonState(unsigned int button, dboolean on)
{
    event_t event;

    if (button < SDL_BUTTON_LEFT || button > MAX_MOUSE_BUTTONS)
    {
        return;
    }

    // Note: button "0" is left, button "1" is right,
    // button "2" is middle for Doom.  This is different
    // to how SDL sees things.

    switch (button)
    {
        case SDL_BUTTON_LEFT:
            button = 0;
            break;

        case SDL_BUTTON_RIGHT:
            button = 1;
            break;

        case SDL_BUTTON_MIDDLE:
            button = 2;
            break;

        default:
            // SDL buttons are indexed from 1.
            --button;
            break;
    }

    // Turn bit representing this button on or off.

    if (on)
    {
        mouse_button_state |= (1 << button);
    }
    else
    {
        mouse_button_state &= ~(1 << button);
    }

    // Post an event with the new button state.

    event.type = ev_mouse;
    event.data1 = mouse_button_state;
    event.data2 = event.data3 = 0;
    D_PostEvent(&event);
}

static int AccelerateMouse(int val)
{
    if (val < 0)
        return -AccelerateMouse(-val);

    if (val > mouse_threshold)
    {
        return (int)((val - mouse_threshold) * mouse_acceleration + mouse_threshold);
    }
    else
    {
        return val;
    }
}

// Get the equivalent ASCII (Unicode?) character for a keypress.

static int GetTypedChar(SDL_Event *event)
{
#ifdef SDL2
    // If we're strictly emulating Vanilla, we should always act like
    // we're using a US layout keyboard (in ev_keydown, data1=data2).
    // Otherwise we should use the native key mapping.
    if (vanilla_keyboard_mapping)
    {
        return TranslateKey(&event->key.keysym);
    }
    else
    {
        int unicode = event->key.keysym.sym;

        if (unicode < 128)
        {
            return unicode;
        }
        else
        {
            return 0;
        }
    }
#else

    // If Vanilla keyboard mapping enabled, the keyboard
    // scan code is used to give the character typed.
    // This does not change depending on keyboard layout.
    // If you have a German keyboard, pressing 'z' will
    // give 'y', for example.  It is desirable to be able
    // to fix this so that people with non-standard 
    // keyboard mappings can type properly.  If vanilla
    // mode is disabled, use the properly translated 
    // version.

    if (vanilla_keyboard_mapping)
    {
        int key = TranslateKey(&event->key.keysym);

        // Is shift held down?  If so, perform a translation.

        if (shiftdown > 0)
        {
            if (key >= 0 && key < arrlen(shiftxform))
            {
                key = shiftxform[key];
            }
            else
            {
                key = 0;
            }
        }

        return key;
    }
    else
    {
        // Unicode value, from key layout.

        return tolower(event->key.keysym.unicode);
    }
#endif
}

static void UpdateShiftStatus(SDL_Event *event)
{
    int change;

    if (event->type == SDL_KEYDOWN)
    {
        change = 1;
    }
    else if (event->type == SDL_KEYUP)
    {
        change = -1;
    }
    else
    {
        return;
    }

    if (event->key.keysym.sym == SDLK_LSHIFT 
     || event->key.keysym.sym == SDLK_RSHIFT)
    {
        shiftdown += change;
    }
}

#ifdef SDL2
static void HandleWindowEvent(SDL_WindowEvent *event)
{
    switch (event->event)
    {
#if 0 // SDL2-TODO
        case SDL_ACTIVEEVENT:
            // need to update our focus state
            UpdateFocus();
            break;
#endif
        case SDL_WINDOWEVENT_EXPOSED:
            palette_to_set = true;
            break;

        case SDL_WINDOWEVENT_RESIZED:
            need_resize = true;
            resize_w = event->data1;
            resize_h = event->data2;
            last_resize_time = SDL_GetTicks();
            break;

        // Don't render the screen when the window is minimized:

        case SDL_WINDOWEVENT_MINIMIZED:
            screenvisible = false;
            break;

        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
            screenvisible = true;
            break;

        // Update the value of window_focused when we get a focus event
        //
        // We try to make ourselves be well-behaved: the grab on the mouse
        // is removed if we lose focus (such as a popup window appearing),
        // and we dont move the mouse around if we aren't focused either.

        case SDL_WINDOWEVENT_ENTER:
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            window_focused = true;
            break;

        case SDL_WINDOWEVENT_LEAVE:
        case SDL_WINDOWEVENT_FOCUS_LOST:
            window_focused = false;
            break;

        default:
            break;
    }
}
#endif

void I_GetEvent(void)
{
    SDL_Event sdlevent;
    event_t event;
#ifdef SDL2
    SDL_Event   *Event = &sdlevent;
#endif
    // possibly not needed
    
    SDL_PumpEvents();

    // put event-grabbing stuff in here
    
    while (SDL_PollEvent(&sdlevent))
    {
        // ignore mouse events when the window is not focused

        if (!window_focused 
         && (sdlevent.type == SDL_MOUSEMOTION
          || sdlevent.type == SDL_MOUSEBUTTONDOWN
          || sdlevent.type == SDL_MOUSEBUTTONUP))
        {
            continue;
        }

        if (screensaver_mode && sdlevent.type == SDL_QUIT)
        {
            I_Quit();
        }

        UpdateShiftStatus(&sdlevent);

        // process event
        
        switch (sdlevent.type)
        {
            case SDL_KEYDOWN:
                // data1 has the key pressed, data2 has the character
                // (shift-translated, etc)
                event.type = ev_keydown;
                event.data1 = TranslateKey(&sdlevent.key.keysym);
                event.data2 = GetTypedChar(&sdlevent);

                // SDL2-TODO: Need to generate a parallel text input event
                // here that can be used for typing text, eg. multiplayer
                // chat and savegame names. This is only for the Vanilla
                // case; we must use the shiftxform table.

                if (event.data1 != 0)
                {
                    D_PostEvent(&event);
                }
                break;

            case SDL_KEYUP:
                event.type = ev_keyup;
                event.data1 = TranslateKey(&sdlevent.key.keysym);

                // data2 is just initialized to zero for ev_keyup.
                // For ev_keydown it's the shifted Unicode character
                // that was typed, but if something wants to detect
                // key releases it should do so based on data1
                // (key ID), not the printable char.

                event.data2 = 0;

                if (event.data1 != 0)
                {
                    D_PostEvent(&event);
                }
                break;

//            case SDL_MOUSEMOTION:
//                event.type = ev_mouse;
//                event.data1 = mouse_button_state;
//                event.data2 = AccelerateMouse(sdlevent.motion.xrel);
//                event.data3 = -AccelerateMouse(sdlevent.motion.yrel);
//                D_PostEvent(&event);
//                break;

            case SDL_MOUSEBUTTONDOWN:
                if (usemouse && !nomouse)
                {
                    UpdateMouseButtonState(sdlevent.button.button, true);
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (usemouse && !nomouse)
                {
                    UpdateMouseButtonState(sdlevent.button.button, false);
                }
                break;
#ifdef SDL2
            case SDL_MOUSEWHEEL:
                if (mouseSensitivity >= 0 || menuactive || consoleactive)
                {
//                    keydown = 0;
                    event.type = ev_mousewheel;
                    event.data1 = Event->wheel.y;
                    event.data2 = 0;
                    event.data3 = 0;
                    D_PostEvent(&event);
                }
                break;
#endif
            case SDL_QUIT:
                C_HideConsoleFast();
                event.type = ev_quit;
                D_PostEvent(&event);
                break;
#ifndef SDL2
            case SDL_ACTIVEEVENT:
                // need to update our focus state
                UpdateFocus();
                break;

            case SDL_VIDEOEXPOSE:
                palette_to_set = true;
                break;

            case SDL_RESIZABLE:
                need_resize = true;
                resize_w = sdlevent.resize.w;
                resize_h = sdlevent.resize.h;
                last_resize_time = SDL_GetTicks();
                break;
#else
            case SDL_WINDOWEVENT:
                if (sdlevent.window.windowID == SDL_GetWindowID(screen))
                {
                    HandleWindowEvent(&sdlevent.window);
                }
                break;
#endif
            default:
                break;
        }
    }
}

// Warp the mouse back to the middle of the screen

static void CenterMouse(void)
{
    // Warp the the screen center

#ifdef SDL2
    int screen_w, screen_h;

    SDL_GetWindowSize(screen, &screen_w, &screen_h);
    SDL_WarpMouseInWindow(screen, screen_w / 2, screen_h / 2);
#else
    SDL_WarpMouse(screen->w / 2, screen->h / 2);
#endif

    // Clear any relative movement caused by warping

    SDL_PumpEvents();
    SDL_GetRelativeMouseState(NULL, NULL);
}

//
// Read the change in mouse state to generate mouse motion events
//
// This is to combine all mouse movement for a tic into one mouse
// motion event.

static void I_ReadMouse(void)
{
    int x, y;
    event_t ev;

    SDL_GetRelativeMouseState(&x, &y);

    if (x != 0 || y != 0) 
    {
        ev.type = ev_mouse;
        ev.data1 = mouse_button_state;
        ev.data2 = AccelerateMouse(x);

        if (!novert)
        {
            ev.data3 = -AccelerateMouse(y);
        }
        else
        {
            ev.data3 = 0;
        }
        
        D_PostEvent(&ev);
    }

    if (MouseShouldBeGrabbed())
    {
        CenterMouse();
    }
}
#endif

//
// I_StartTic
//
void I_StartTic (void)
{
    if (!initialized)
    {
        return;
    }

#ifndef WII
    I_GetEvent();

    if (usemouse && !nomouse)
    {
        I_ReadMouse();
    }
#endif

#ifdef WII
    I_UpdateJoystick();
#endif
}


//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
    // what is this?
}

#ifndef WII
static void UpdateGrab(void)
{
    static dboolean currently_grabbed = false;
    dboolean grab;

    grab = MouseShouldBeGrabbed();

    if (screensaver_mode)
    {
        // Hide the cursor in screensaver mode

        SetShowCursor(false);
    }
    else if (grab && !currently_grabbed)
    {
        SetShowCursor(false);
        CenterMouse();
    }
    else if (!grab && currently_grabbed)
    {
#ifdef SDL2
        int screen_w, screen_h;
#endif
        SetShowCursor(true);

        // When releasing the mouse from grab, warp the mouse cursor to
        // the bottom-right of the screen. This is a minimally distracting
        // place for it to appear - we may only have released the grab
        // because we're at an end of level intermission screen, for
        // example.
#ifdef SDL2
        SDL_GetWindowSize(screen, &screen_w, &screen_h);
        SDL_WarpMouseInWindow(screen, screen_w - 16, screen_h - 16);
#else
        SDL_WarpMouse(screen->w - 16, screen->h - 16);
        SDL_PumpEvents();
#endif
        SDL_GetRelativeMouseState(NULL, NULL);
    }

    currently_grabbed = grab;

    mouse_grabbed = currently_grabbed;
}
#endif

// Update a small portion of the screen
//
// Does stretching and buffer blitting if neccessary
//
// Return true if blit was successful.

#ifndef SDL2
static dboolean BlitArea(int x1, int y1, int scrn, int x2, int y2)
{
    int x_offset, y_offset;
    dboolean result;

    // No blit needed on native surface
/*
    if (native_surface)
    {
        return true;
    }
*/
    x_offset = (screenbuffer->w - screen_mode->width) / 2;
    y_offset = (screenbuffer->h - screen_mode->height) / 2;

    if (SDL_LockSurface(screenbuffer) >= 0)
    {
/*
        I_InitScale(I_VideoBuffer,
                    (byte *) screenbuffer->pixels
                                + (y_offset * screenbuffer->pitch)
                                + x_offset,
                    screenbuffer->pitch);
*/
        I_InitScale(screens[scrn],
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
#endif

void I_BeginRead(void)
{
    if (!initialized || disk == NULL 
#ifndef SDL2
        || using_opengl
#endif
        )
        return;

    // Draw the disk to the screen

    V_DrawPatch((loading_disk_xoffs >> hires), (loading_disk_yoffs >> hires), 0, disk);

    disk_indicator = disk_dirty;
}

#ifdef SDL2
static void CreateUpscaledTexture(int w, int h)
{
    const int w_scale = (w / SCREENWIDTH) + 1;
    const int h_scale = (h / SCREENHEIGHT) + 1;
    int upscale;
    static int upscale_old;

    // When the screen or window dimensions do not match the aspect ratio
    // of the texture, the rendered area is scaled down to fit

    upscale = (w_scale < h_scale) ? w_scale : h_scale;

    // Limit upscaling factor to 6 (1920x1200)

    if (upscale < 2)
    {
        upscale = 2;
    }
    else if (upscale > 6)
    {
        upscale = 6;
    }

    if (upscale == upscale_old)
    {
        return;
    }

    upscale_old = upscale;

    if (texture_upscaled)
    {
        SDL_DestroyTexture(texture_upscaled);
    }

    // Set the scaling quality for rendering the upscaled texture to "linear",
    // which looks much softer and smoother than "nearest" but does a better
    // job at downscaling from the upscaled texture to screen.

    if (render_mode == 1)
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    else if(render_mode == 2)
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    texture_upscaled = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_TARGET,
                                upscale*SCREENWIDTH, upscale*SCREENHEIGHT);
}
#endif

// Pick the modes list to use:

#ifndef SDL2
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

    // In OpenGL mode the rules are different. We can have any
    // resolution, though it needs to match the aspect ratio we
    // expect.

#ifndef WII
    if (using_opengl)
    {
        static screen_mode_t gl_mode;
        int screenheight;

        if (aspect_ratio_correct)
        {
            screenheight = SCREENHEIGHT_4_3;
        }
        else
        {
            screenheight = SCREENHEIGHT;
        }

        gl_mode.width = h * SCREENWIDTH / screenheight;
        gl_mode.height = h;
        gl_mode.InitMode = NULL;
        gl_mode.DrawScreen = NULL;
        gl_mode.poor_quality = false;

        return &gl_mode;
    }
#endif

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

// Adjust to an appropriate fullscreen mode.
// Returns true if successful.

static dboolean AutoAdjustFullscreen(void)
{
    SDL_Rect **modes;
    SDL_Rect *best_mode;
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
        screen_mode_t *screen_mode;

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

    C_Output("I_InitGraphics: %i x %i mode not supported on this machine.",
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
        C_Output("I_InitGraphics: Cannot run at specified mode: %i x %i",
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
        C_Output("I_InitGraphics: %i bpp color depth not supported.",
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
        C_Output("I_InitGraphics: Auto-adjusted to %i x %i x %i bpp.",
               screen_width, screen_height, screen_bpp);

        C_Warning("NOTE: Your video settings have been adjusted.");
    }
}
#endif

// Set video size to a particular scale factor (1x, 2x, 3x, etc.)
//
// [nitr8] UNUSED
//
/*
static void SetScaleFactor(int factor)
{
    int w, h;

    // Pick 320x200 or 320x240, depending on aspect ratio correct

    if (aspect_ratio_correct)
    {
        w = SCREENWIDTH;
        h = SCREENHEIGHT_4_3;
    }
    else
    {
        w = SCREENWIDTH;
        h = SCREENHEIGHT;
    }

    screen_width = w * factor;
    screen_height = h * factor;
}
*/

#ifndef WII
void I_GraphicsCheckCommandLine(void)
{
//    int i;

    //!
    // @vanilla
    //
    // Disable blitting the screen.
    //

    noblit = M_CheckParm ("-noblit"); 

    //!
    // @category video 
    //
    // Grab the mouse when running in windowed mode.
    //
/*
    if (M_CheckParm("-grabmouse"))
    {
        grabmouse = true;
    }

    //!
    // @category video 
    //
    // Don't grab the mouse when running in windowed mode.
    //

    if (M_CheckParm("-nograbmouse"))
    {
        grabmouse = false;
    }

    // default to fullscreen mode, allow override with command line
    // nofullscreen because we love prboom

    //!
    // @category video 
    //
    // Run in a window.
    //

    if (M_CheckParm("-window") || M_CheckParm("-nofullscreen"))
    {
        fullscreen = false;
    }

    //!
    // @category video 
    //
    // Run in fullscreen mode.
    //

    if (M_CheckParm("-fullscreen"))
    {
        fullscreen = true;
    }
*/
    //!
    // @category video 
    //
    // Disable the mouse.
    //

    nomouse = M_CheckParm("-nomouse") > 0;

    //!
    // @category video
    // @arg <x>
    //
    // Specify the screen width, in pixels.
    //
/*
    i = M_CheckParmWithArgs("-width", 1);

    if (i > 0)
    {
        screen_width = atoi(myargv[i + 1]);
    }

    //!
    // @category video
    // @arg <y>
    //
    // Specify the screen height, in pixels.
    //

    i = M_CheckParmWithArgs("-height", 1);

    if (i > 0)
    {
        screen_height = atoi(myargv[i + 1]);
    }

    //!
    // @category video
    // @arg <bpp>
    //
    // Specify the color depth of the screen, in bits per pixel.
    //

#ifndef SDL2
    i = M_CheckParmWithArgs("-bpp", 1);

    if (i > 0)
    {
        screen_bpp = atoi(myargv[i + 1]);
    }

    // Because we love Eternity:

    //!
    // @category video
    //
    // Set the color depth of the screen to 32 bits per pixel.
    //

    if (M_CheckParm("-8in32"))
    {
        screen_bpp = 32;
    }
#endif

    //!
    // @category video
    // @arg <WxY>[wf]
    //
    // Specify the dimensions of the window or fullscreen mode.  An
    // optional letter of w or f appended to the dimensions selects
    // windowed or fullscreen mode.

    i = M_CheckParmWithArgs("-geometry", 1);

    if (i > 0)
    {
        int w, h, s;
        char f;

        s = sscanf(myargv[i + 1], "%ix%i%1c", &w, &h, &f);
        if (s == 2 || s == 3)
        {
            screen_width = w;
            screen_height = h;

            if (s == 3 && f == 'f')
            {
                fullscreen = true;
            }
            else if (s == 3 && f == 'w')
            {
                fullscreen = false;
            }
        }
    }

    //!
    // @category video
    //
    // Don't scale up the screen.
    //

    if (M_CheckParm("-1")) 
    {
        SetScaleFactor(1);
    }

    //!
    // @category video
    //
    // Double up the screen to 2x its normal size.
    //

    if (M_CheckParm("-2")) 
    {
        SetScaleFactor(2);
    }

    //!
    // @category video
    //
    // Double up the screen to 3x its normal size.
    //

    if (M_CheckParm("-3")) 
    {
        SetScaleFactor(3);
    }

    //!
    // @category video
    //
    // Disable vertical mouse movement.
    //

    if (M_CheckParm("-novert"))
    {
        novert = true;
    }

    //!
    // @category video
    //
    // Enable vertical mouse movement.
    //

    if (M_CheckParm("-nonovert"))
    {
        novert = false;
    }
*/
}

// Check if we have been invoked as a screensaver by xscreensaver.

void I_CheckIsScreensaver(void)
{
    char *env;

    env = getenv("XSCREENSAVER_WINDOW");

    if (env != NULL)
    {
        screensaver_mode = true;
    }
}

static void CreateCursors(void)
{
    static Uint8 empty_cursor_data = 0;

    // Save the default cursor so it can be recalled later

    cursors[1] = SDL_GetCursor();

    // Create an empty cursor

    cursors[0] = SDL_CreateCursor(&empty_cursor_data,
                                  &empty_cursor_data,
                                  1, 1, 0, 0);
}

static void SetSDLVideoDriver(void)
{
    // Allow a default value for the SDL video driver to be specified
    // in the configuration file.

    if (strcmp(video_driver, "") != 0)
    {
        char *env_string;

        env_string = M_StringJoin("SDL_VIDEODRIVER=", video_driver, NULL);
        putenv(env_string);
        free(env_string);
    }
}

static void SetWindowPositionVars(void)
{
    int x, y;

    if (window_position == NULL || !strcmp(window_position, ""))
    {
        return;
    }

    if (!strcmp(window_position, "center"))
    {
        putenv("SDL_VIDEO_CENTERED=1");
    }
    else if (sscanf(window_position, "%10i,%10i", &x, &y) == 2)
    {
        char buf[64];

        M_snprintf(buf, sizeof(buf), "SDL_VIDEO_WINDOW_POS=%i,%i", x, y);
        putenv(buf);
    }
}
#endif

#ifndef SDL2
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
#endif

static void SetVideoMode(screen_mode_t *mode, int w, int h)
{
    byte *doompal = W_CacheLumpName("PLAYPAL", PU_CACHE);
    int flags = 0;

#ifdef SDL2
    SDL_RendererInfo        rendererinfo;

    const char *displayname = SDL_GetDisplayName(0);

    C_Output("Using display called '%s'.", displayname);
#endif

    // If we are already running and in a true color mode, we need
    // to free the screenbuffer surface before setting the new mode.

    if (
#ifndef WII
#ifndef SDL2
        !using_opengl &&
#endif
#endif
        screenbuffer != NULL

#ifndef SDL2
        && screen != screenbuffer
#endif
        )
    {
        SDL_FreeSurface(screenbuffer);

#ifdef SDL2
        screenbuffer = NULL;
#endif
    }

#ifdef SDL2
    // Close the current window.

    if (screen != NULL)
    {
        SDL_DestroyWindow(screen);
        screen = NULL;
    }
#endif

    // Perform screen scale setup before changing video mode.

    if (
#ifndef WII
#ifndef SDL2
        !using_opengl &&
#endif
#endif
        mode != NULL && mode->InitMode != NULL
        )
    {
        mode->InitMode(doompal);
    }

    // Set the video mode.

#ifdef SDL2
    flags = 0;
#endif

    if (fullscreen)
    {
#ifdef SDL2
        // This flags means "Never change the screen resolution! Instead,
        // draw to the entire screen by scaling the texture appropriately".
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#else
        flags |= SDL_FULLSCREEN;
#endif
    }
    else
    {
        // In windowed mode, the window can be resized while the game is
        // running. Mac OS X has a quirk where an ugly resize handle is
        // shown in software mode when resizing is enabled, so avoid that.
#ifdef SDL2
        flags |= SDL_WINDOW_RESIZABLE;
#else
        flags |= SDL_RESIZABLE;
#endif
    }

#ifndef WII
#ifndef SDL2
    if (using_opengl)
    {
        flags |= SDL_OPENGL;
        screen_bpp = 32;
    }
    else
    {
        flags |= SDL_SWSURFACE | SDL_DOUBLEBUF;

        if (screen_bpp == 8)
        {
            flags |= SDL_HWPALETTE;
        }
    }
#endif
#endif

#ifdef SDL2
    // Create window and renderer contexts. We set the window title
    // later anyway and leave the window position "undefined". If "flags"
    // contains the fullscreen flag (see above), then w and h are ignored.

    screen = SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED,
                                    SDL_WINDOWPOS_UNDEFINED,
                                    w, h, flags);
#else
    screen = SDL_SetVideoMode(w, h, screen_bpp, flags);
#endif

    if (screen == NULL)
    {
#ifdef SDL2
        I_Error("Error setting video mode %ix%ix: %s\n",
                w, h, SDL_GetError());
#else
        I_Error("Error setting video mode %ix%ix%ibpp: %s\n",
                w, h, screen_bpp, SDL_GetError());
#endif
    }

#ifdef SDL2
    // If we are running fullscreen, the whole screen is our "window".

    if (fullscreen)
    {
        SDL_DisplayMode mode;

        // We do not change the video mode to run fullscreen but scale to fill
        // the desktop that "screen" is assigned to. So, use desktop dimensions
        // to calculate the size of the upscaled texture.

        SDL_GetDesktopDisplayMode(SDL_GetWindowDisplayIndex(screen), &mode);

        h = mode.h;
        w = mode.w;
    }

    // The SDL_RENDERER_TARGETTEXTURE flag is required to render the
    // intermediate texture into the upscaled texture.

    if (d_vsync)
        renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC);
    else
        renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_TARGETTEXTURE);

    if (renderer == NULL)
    {
        I_Error("Error creating renderer for video mode %ix%i: %s\n",
                w, h, SDL_GetError());
    }

    SDL_GetRendererInfo(renderer, &rendererinfo);
/*
    if (M_StringCompare(rendererinfo.name, "direct3d"))
        C_Output("The screen is rendered using hardware acceleration with the Direct3D 9 "
            "API.");
    else*/ if (M_StringCompare(rendererinfo.name, "opengl"))
        C_Output("The screen is rendered using hardware acceleration with the OpenGL API.");
    else if (M_StringCompare(rendererinfo.name, "software"))
        C_Output("The screen is rendered in software mode.");

    if (rendererinfo.flags & SDL_RENDERER_PRESENTVSYNC)
    {
        SDL_DisplayMode displaymode;

        SDL_GetWindowDisplayMode(screen, &displaymode);
        C_Output("The display's refresh is at %iHz.",
            displaymode.refresh_rate);
    }
    else
    {
        if (d_vsync)
        {
            if (M_StringCompare(rendererinfo.name, "software"))
                C_Warning("Vertical synchronization can't be enabled in software.");
            else
                C_Warning("Vertical synchronization can't be enabled.");
        }
        C_Output("The framerate is uncapped.");
    }

    C_Output("The %ix%i screen is scaled up to %i x %i", SCREENWIDTH, SCREENHEIGHT, h * 4 / 3, h);

    // Important: Set the "logical size" of the rendering context. At the same
    // time this also defines the aspect ratio that is preserved while scaling
    // and stretching the texture into the window.

    SDL_RenderSetLogicalSize(renderer,
                             SCREENWIDTH,
                             aspect_ratio_correct ? SCREENHEIGHT_4_3 : SCREENHEIGHT);

    // Blank out the full screen area in case there is any junk in
    // the borders that won't otherwise be overwritten.

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
#endif

#ifndef SDL2
#ifndef WII
    if (using_opengl)
    {
        // Try to initialize OpenGL scaling backend. This can fail,
        // because we need an OpenGL context before we can find out
        // if we have all the extensions that we need to make it work.
        // If it does, then fall back to software mode instead.

        if (!I_GL_InitScale(screen->w, screen->h))
        {
            C_Print(graystring,
                    "Failed to initialize in OpenGL mode. "
                    "Falling back to software mode instead.");
            using_opengl = false;

            // TODO: This leaves us in window with borders around it.
            // We shouldn't call with NULL here; this needs to be refactored
            // so that 'mode' isn't even an argument to this function.
            SetVideoMode(NULL, w, h);
            return;
        }
    }
    else
#endif
    {
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
#endif

#ifdef SDL2
        // Create the 8-bit paletted and the 32-bit RGBA screenbuffer surfaces.

        screenbuffer = SDL_CreateRGBSurface(0,
                                        SCREENWIDTH, SCREENHEIGHT, 8,
                                        0, 0, 0, 0);
        SDL_FillRect(screenbuffer, NULL, 0);

        rgbabuffer = SDL_CreateRGBSurface(0,
                                      SCREENWIDTH, SCREENHEIGHT, 32,
                                      0, 0, 0, 0);
        SDL_FillRect(rgbabuffer, NULL, 0);

        // Set the scaling quality for rendering the intermediate texture into
        // the upscaled texture to "nearest", which is gritty and pixelated and
        // resembles software scaling pretty well.

        if (render_mode == 1)
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        else if(render_mode == 2)
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

        // Create the intermediate texture that the RGBA surface gets loaded into.
        // The SDL_TEXTUREACCESS_STREAMING flag means that this texture's content
        // is going to change frequently.

        texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                SCREENWIDTH, SCREENHEIGHT);

        // Initially create the upscaled texture for rendering to screen

        CreateUpscaledTexture(w, h);
#endif

        // Save screen mode.

        screen_mode = mode;

        // Create the screenbuffer surface; if we have a real 8-bit palettized
        // screen, then we can use the screen as the screenbuffer.

#ifndef SDL2
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
#endif
}

#ifndef WII
#ifndef SDL2
static void ApplyWindowResize(unsigned int w, unsigned int h)
{
    screen_mode_t *mode;

    // Find the biggest screen mode that will fall within these
    // dimensions, falling back to the smallest mode possible if
    // none is found.

    mode = I_FindScreenMode(w, h);

    if (mode == NULL)
    {
        mode = I_FindScreenMode(SCREENWIDTH, SCREENHEIGHT);
    }

    // Reset mode to resize window.

    C_Warning("Resize to %i x %i", mode->width, mode->height);
    SetVideoMode(mode, mode->width, mode->height);

    // Save settings.

    screen_width = mode->width;
    screen_height = mode->height;
}
#endif
#endif

// Ending of I_FinishUpdate() when in software scaling mode.

static void FinishUpdateSoftware(void)
{
    // draw to screen

#ifndef SDL2
    BlitArea(0, 0, 0, SCREENWIDTH, SCREENHEIGHT);
#endif

    if (palette_to_set)
    {
#ifdef SDL2
        SDL_SetPaletteColors(screenbuffer->format->palette, palette, 0, 256);
#else
        SDL_SetColors(screenbuffer, palette, 0, 256);
#endif
        palette_to_set = false;

        // In native 8-bit mode, if we have a palette to set, the act
        // of setting the palette updates the screen

#ifndef SDL2
        if (screenbuffer == screen)
        {
            return;
        }
#endif
    }

#ifdef SDL2
    // Blit from the paletted 8-bit screen buffer to the intermediate
    // 32-bit RGBA buffer that we can load into the texture.

    SDL_LowerBlit(screenbuffer, &blit_rect, rgbabuffer, &blit_rect);

    // Update the intermediate texture with the contents of the RGBA buffer.

    SDL_UpdateTexture(texture, NULL, rgbabuffer->pixels, rgbabuffer->pitch);

    // Make sure the pillarboxes are kept clear each frame.

    SDL_RenderClear(renderer);

    // Render this intermediate texture into the upscaled texture
    // using "nearest" integer scaling.

    SDL_SetRenderTarget(renderer, texture_upscaled);
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    // Finally, render this upscaled texture to screen using linear scaling.

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, texture_upscaled, NULL, NULL);

    // Draw!

    SDL_RenderPresent(renderer);
#else
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
#endif
}

//
// I_FinishUpdate
//
void I_FinishUpdate (int scrn)
{
    int         i;

    if (!initialized)
        return;

    if (noblit)
        return;

#ifndef WII
    if (need_resize && SDL_GetTicks() > last_resize_time + 500)
    {
#ifndef SDL2
        ApplyWindowResize(resize_w, resize_h);
#else
        CreateUpscaledTexture(resize_w, resize_h);
        screen_width = resize_w;
        screen_height = resize_h;
#endif
        need_resize = false;
        palette_to_set = true;
    }

    UpdateGrab();
#endif

    // Don't update the screen if the window isn't visible.
    // Not doing this breaks under Windows when we alt-tab away 
    // while fullscreen.

#ifndef SDL2
    if (!(SDL_GetAppState() & SDL_APPACTIVE))
        return;
#endif

    // [crispy] variable rendering framerate
    if (d_uncappedframerate > UNCAPPED_ON /*&& !singletics*/)
    {
        static int halftics_old;
        int halftics;
        extern int GetAdjustedTimeN (const int N);

        while ((halftics = GetAdjustedTimeN(40 + d_uncappedframerate * 10)) == halftics_old)
        {
            I_Sleep(1);
        }

        halftics_old = halftics;
    }

    // draws little dots on the bottom of the screen

    if (display_fps_dots)
    {
        static int  lasttic;
        int         tics;

        i = I_GetTime();
        tics = i - lasttic;
        lasttic = i;

        if (tics > 20) tics = 20;

        for (i=0 ; i<tics*4 ; i+=4)
//            I_VideoBuffer[ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
            screens[scrn][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;

        for ( ; i<20*4 ; i+=4)
//            I_VideoBuffer[ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
            screens[scrn][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }

    // [AM] Real FPS counter
    if (display_fps)
    {
        static int  lastmili;
        static int  fpscount;
        static char fpsbuf[5];
        int         mili;

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

    if (show_diskicon && disk_indicator == disk_on)
    {
        if (diskicon_readbytes >= diskicon_threshold)
        {
            I_BeginRead();
        }
    }
    else if (disk_indicator == disk_dirty)
    {
        disk_indicator = disk_off;
    }

    diskicon_readbytes = 0;

#ifndef WII
#ifndef SDL2
    if (using_opengl)
    {
//        I_GL_UpdateScreen(I_VideoBuffer, palette);
        I_GL_UpdateScreen(screens[scrn], palette);
        SDL_GL_SwapBuffers();
    }
    else
#endif
#endif
    {
        FinishUpdateSoftware();
    }
}

//
// I_ReadScreen
//
void I_ReadScreen(int scrn, byte* scr)
{
//    memcpy(scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT * sizeof(*scr));
    memcpy(scr, screens[scrn], SCREENWIDTH * SCREENHEIGHT * sizeof(*scr));
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

//
// Given an RGB value, find the closest matching palette index.
//
// [nitr8] UNUSED
//
/*
int I_GetPaletteIndex(int r, int g, int b)
{
    int best, best_diff;
    int i;

    best = 0; best_diff = INT_MAX;

    for (i = 0; i < 256; ++i)
    {
        int diff = (r - palette[i].r) * (r - palette[i].r)
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
*/

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

    for (i=0; i<icon_w * icon_h; ++i)
    {
        if (icon_data[i * 3] != 0x00
         || icon_data[i * 3 + 1] != 0x00
         || icon_data[i * 3 + 2] != 0x00)
        {
            mask[i / 8] |= 1 << (7 - i % 8);
        }
    }

    surface = SDL_CreateRGBSurfaceFrom(icon_data,
                                       icon_w,
                                       icon_h,
                                       24,
                                       icon_w * 3,
                                       0xff << 0,
                                       0xff << 8,
                                       0xff << 16,
                                       0);

    SDL_WM_SetIcon(surface, mask);
#else
    surface = SDL_CreateRGBSurfaceFrom((void *) icon_data, icon_w, icon_h,
                                       32, icon_w * 4,
                                       0xff << 24, 0xff << 16,
                                       0xff << 8, 0xff << 0);

    SDL_SetWindowIcon(screen, surface);
#endif

    SDL_FreeSurface(surface);

#ifndef SDL2
    free(mask);
#endif
}
#endif

//
// Call the SDL function to set the window title, based on 
// the title set with I_SetWindowTitle.
//

void I_SetWindowTitle(char *title)
{
    window_title = title;
}

void I_InitWindowTitle(void)
{
    char *buf;

//    buf = M_StringJoin(gamedescription, " - ", PACKAGE_STRING, NULL);
    buf = M_StringJoin(window_title, " - ", PACKAGE_STRING, NULL);

#ifdef SDL2
    SDL_SetWindowTitle(screen, buf);
#else
    SDL_WM_SetCaption(buf, NULL);
#endif

    free(buf);
}

void I_InitGammaTables(void)
{
    int i;
    int j;

    for (i = 0; i < GAMMALEVELS; ++i)
        for (j = 0; j < 256; ++j)
            gammatable[i][j] = (byte)(pow(j / 255.0, 1.0 / gammalevels[i]) * 255.0 + 0.5);
}

void I_InitGraphics(int scrn)
{
    SDL_Event dummy;
    byte      *doompal = W_CacheLumpName("PLAYPAL", PU_CACHE);

    wad_file_t *playpalwad = lumpinfo[W_CheckNumForName("PLAYPAL")]->wad_file;

#ifndef WII
    char *env;

    // Pass through the XSCREENSAVER_WINDOW environment variable to 
    // SDL_WINDOWID, to embed the SDL window into the Xscreensaver
    // window.

    env = getenv("XSCREENSAVER_WINDOW");

    if (env != NULL)
    {
        char winenv[30];
        int winid;

        sscanf(env, "0x%x", &winid);
        M_snprintf(winenv, sizeof(winenv), "SDL_WINDOWID=%i", winid);

        putenv(winenv);
    }

    SetSDLVideoDriver();
    SetWindowPositionVars();
#endif

    I_InitTintTables(doompal);

    I_InitGammaTables();

    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
    {
        I_Error("Failed to initialize video: %s", SDL_GetError());
    }

    // If we're using OpenGL, call the preinit function now; if it fails
    // then we have to fall back to software mode.

#ifndef SDL2
    if (using_opengl 
#ifndef WII
        && !I_GL_PreInit()
#endif
       )
    {
        using_opengl = false;
    }
#endif

    //
    // Enter into graphics mode.
    //
    // When in screensaver mode, run full screen and auto detect
    // screen dimensions (don't change video mode)
    //

#ifndef WII
    if (screensaver_mode)
    {
        SetVideoMode(NULL, 0, 0);
    }
    else
#endif
    {
        int w, h;

#ifndef SDL2
        if (autoadjust_video_settings)
        {
            I_AutoAdjustSettings();
        }
#endif

        w = screen_width;
        h = screen_height;

#ifndef SDL2
        screen_mode = I_FindScreenMode(w, h);

        if (screen_mode == NULL)
        {
            I_Error("I_InitGraphics: Unable to find a screen mode small "
                    "enough for %ix%i", w, h);
        }

        if (w != screen_mode->width || h != screen_mode->height)
        {
            C_Output("I_InitGraphics: %s (%i x %i within %i x %i)",
                  WindowBoxType(screen_mode, w, h),
                  screen_mode->width, screen_mode->height, w, h);
        }
#endif

        SetVideoMode(screen_mode, w, h);
    }

    C_Output("Scaling to aspect ratio 16:9");
    C_Output("Using 256-color palette from PLAYPAL lump in %s file %s",
            (playpalwad->type == IWAD ? "IWAD" : "PWAD"), uppercase(playpalwad->path));

//    C_Output("        %s.");

    if (usegamma == 10)
        C_Output("Gamma correction is off.");
    else
    {
        static char     buffer[128];

        M_snprintf(buffer, sizeof(buffer), "The gamma correction level is %.2f.",
            gammalevels[usegamma]);
        if (buffer[strlen(buffer) - 1] == '0' && buffer[strlen(buffer) - 2] == '0')
            buffer[strlen(buffer) - 1] = '\0';
        C_Warning(buffer);
    }

#ifndef WII
#ifndef SDL2
    if (!using_opengl)
#endif
#endif
    {
#ifndef SDL2
        SDL_SetColors(screenbuffer, palette, 0, 256);
#endif
        // Start with a clear black screen
        // (screen will be flipped after we set the palette)

        SDL_FillRect(screenbuffer, NULL, 0);
    }

    // Set the palette

    I_SetPalette(doompal);

#ifdef SDL2
    SDL_SetPaletteColors(screenbuffer->format->palette, palette, 0, 256);
#endif

#ifndef WII
    CreateCursors();

#ifndef SDL2
    UpdateFocus();
#endif

    UpdateGrab();
#endif

    // On some systems, it takes a second or so for the screen to settle
    // after changing modes.  We include the option to add a delay when
    // setting the screen mode, so that the game doesn't start immediately
    // with the player unable to see anything.

    if (fullscreen 
#ifndef WII
        && !screensaver_mode
#endif
       )
    {
        SDL_Delay(startup_delay);
    }

    // Check if we have a native surface we can use
    // If we have to lock the screen, draw to a buffer and copy
    // Likewise if the screen pitch is not the same as the width
    // If we have to multiply, drawing is done to a separate 320x200 buf
/*
#ifndef SDL2
    native_surface = 
#ifndef WII
                     !using_opengl &&
#endif
                     screen == screenbuffer
                  && !SDL_MUSTLOCK(screen)
                  && screen_mode == &mode_scale_1x
                  && screen->pitch == SCREENWIDTH
                  && aspect_ratio_correct;
#endif
*/
    // If not, allocate a buffer and copy from that buffer to the
    // screen when we do an update

#ifdef SDL2
//    I_VideoBuffer = screenbuffer->pixels;
    screens[scrn] = screenbuffer->pixels;
/*
#else
    if (native_surface)
    {

//        I_VideoBuffer = (unsigned char *) screen->pixels;

//        I_VideoBuffer += (screen->h - SCREENHEIGHT) / 2;

        screens[scrn] = (unsigned char *) screen->pixels;

        screens[scrn] += (screen->h - SCREENHEIGHT) / 2;
    }
    else
    {
//        I_VideoBuffer = (unsigned char *) Z_Malloc (SCREENWIDTH * SCREENHEIGHT, 
//                                                    PU_STATIC, NULL);

        screens[scrn] = (unsigned char *) Z_Malloc (SCREENWIDTH * SCREENHEIGHT, 
                                                    PU_STATIC, NULL);
    }
*/
#endif

//    V_RestoreBuffer();

    // Clear the screen to black.

    memset(screens[scrn], 0, SCREENWIDTH * SCREENHEIGHT);

    // We need SDL to give us translated versions of keys as well

#ifndef SDL2
    SDL_EnableUNICODE(1);

    // Repeat key presses - this is what Vanilla Doom does
    // Not sure about repeat rate - probably dependent on which DOS
    // driver is used.  This is good enough though.

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
#endif

    // Clear out any events waiting at the start:

    while (SDL_PollEvent(&dummy));

    initialized = true;

    // Call I_ShutdownGraphics on quit

    I_AtExit(I_ShutdownGraphics, true);

#ifdef SDL2
    UpdateFocus();
    UpdateGrab();
#endif
}

