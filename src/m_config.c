//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
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
//    Configuration file interface.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

//#include "config.h"

#include "doomdef.h"
#include "doomtype.h"
#include "doomkeys.h"
#include "doomfeatures.h"
#include "i_system.h"
//#include "m_argv.h"
#include "m_misc.h"

#include "z_zone.h"

extern boolean devparm;

extern int key_right;
extern int key_left;
extern int key_up;
extern int key_down;
extern int key_strafeleft;
extern int key_fire;
extern int key_use;
extern int key_jump;
extern int key_run;
extern int key_invright;
extern int key_flyup;
extern int key_flydown;
extern int key_useartifact;

//
// DEFAULTS
//

// Location where all configuration data is stored - 
// default.cfg, savegames, etc.

char *configdir;

// Default filenames for configuration files.

static char *default_main_config;
//static char *default_extra_config;
/*
typedef enum 
{
    DEFAULT_INT,
    DEFAULT_INT_HEX,
    DEFAULT_STRING,
    DEFAULT_FLOAT,
    DEFAULT_KEY,
} default_type_t;

typedef struct
{
    // Name of the variable
    char *name;

    // Pointer to the location in memory of the variable
    void *location;

    // Type of the variable
    default_type_t type;

    // If this is a key value, the original integer scancode we read from
    // the config file before translating it to the internal key value.
    // If zero, we didn't read this value from a config file.
    int untranslated;

    // The value we translated the scancode into when we read the 
    // config file on startup.  If the variable value is different from
    // this, it has been changed and needs to be converted; otherwise,
    // use the 'untranslated' value.
    int original_translated;

    // If true, this config variable has been bound to a variable
    // and is being used.
    boolean bound;
} default_t;

typedef struct
{
    default_t *defaults;
    int numdefaults;
    char *filename;
} default_collection_t;
*/

#define CONFIG_VARIABLE_GENERIC(name, type) \
    { #name, NULL, type, 0, 0, false }

#define CONFIG_VARIABLE_KEY(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_KEY)

#define CONFIG_VARIABLE_INT(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_INT)

#define CONFIG_VARIABLE_INT_HEX(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_INT_HEX)

#define CONFIG_VARIABLE_FLOAT(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_FLOAT)

#define CONFIG_VARIABLE_STRING(name) \
    CONFIG_VARIABLE_GENERIC(name, DEFAULT_STRING)

//! @begin_config_file default
/*
extern int key_right, key_left, key_up, key_down;
extern int key_strafeleft, key_jump;
extern int key_fire, key_use;
extern int key_invpop, key_mission;
extern int key_invright, key_invuse;
*/
/*static */default_t	doom_defaults_list[] =
{
    CONFIG_VARIABLE_INT		(sfx_volume),
    CONFIG_VARIABLE_INT		(music_volume),
/*
    CONFIG_VARIABLE_INT		(show_talk),
    CONFIG_VARIABLE_INT		(voice_volume),
*/
    CONFIG_VARIABLE_INT		(screensize),
    CONFIG_VARIABLE_INT		(walking_speed),
    CONFIG_VARIABLE_INT		(turning_speed),
    CONFIG_VARIABLE_INT		(strafing_speed),
    CONFIG_VARIABLE_INT		(freelook_speed),
    CONFIG_VARIABLE_INT		(use_gamma),
    CONFIG_VARIABLE_INT		(mouse_look),
/*
    CONFIG_VARIABLE_INT		(voices),
						// FOR PSP: USED TO BE FOR SET CFG VAR TICKER & FPS...
						// ...BUT CAUSES A CRASH AS SOON AS THE SCREEN WIPES
    CONFIG_VARIABLE_INT		(fps),
    CONFIG_VARIABLE_INT		(framelimit),
*/
    CONFIG_VARIABLE_INT		(map_grid),
    CONFIG_VARIABLE_INT		(follow_player),
    CONFIG_VARIABLE_INT		(showstats),
    CONFIG_VARIABLE_INT		(show_messages),
/*
    CONFIG_VARIABLE_INT		(cpu),
						// FOR PSP: USED TO BE FOR SET CFG VAR TICKER & FPS...
						// ...BUT CAUSES A CRASH AS SOON AS THE SCREEN WIPES
    CONFIG_VARIABLE_INT		(ticker),
*/
    CONFIG_VARIABLE_INT		(map_rotate),
    CONFIG_VARIABLE_INT		(detail),
#ifdef OGG_SUPPORT
    CONFIG_VARIABLE_INT		(opl_mode),
#endif
//    CONFIG_VARIABLE_INT		(btn_layout),
    CONFIG_VARIABLE_INT		(vanilla_weapon_change),
    CONFIG_VARIABLE_INT		(xhair),
    CONFIG_VARIABLE_INT		(jump),
    CONFIG_VARIABLE_INT		(music_engine),
    CONFIG_VARIABLE_INT		(recoil),
    CONFIG_VARIABLE_INT		(monsters_respawn),
    CONFIG_VARIABLE_INT		(fast_monsters),
    CONFIG_VARIABLE_INT		(auto_aim),
/*
    CONFIG_VARIABLE_INT		(key_triangle),
    CONFIG_VARIABLE_INT		(key_cross),
    CONFIG_VARIABLE_INT		(key_square),
    CONFIG_VARIABLE_INT		(key_circle),
    CONFIG_VARIABLE_INT		(key_start),
*/
    CONFIG_VARIABLE_INT		(key_fire),
    CONFIG_VARIABLE_INT		(key_use),
    CONFIG_VARIABLE_INT		(key_menu),
    CONFIG_VARIABLE_INT		(key_weapon_left),
    CONFIG_VARIABLE_INT		(key_automap),
    CONFIG_VARIABLE_INT		(key_weapon_right),
    CONFIG_VARIABLE_INT		(key_automap_zoom_in),
    CONFIG_VARIABLE_INT		(key_automap_zoom_out),
    CONFIG_VARIABLE_INT		(key_jump),
    CONFIG_VARIABLE_INT		(key_run),
    CONFIG_VARIABLE_INT		(key_console),
    CONFIG_VARIABLE_INT		(key_aiminghelp),
/*
    CONFIG_VARIABLE_INT		(key_righttrigger),
    CONFIG_VARIABLE_INT		(key_zl),
    CONFIG_VARIABLE_INT		(key_zr),
    CONFIG_VARIABLE_INT		(key_a),
    CONFIG_VARIABLE_INT		(key_b),
    CONFIG_VARIABLE_INT		(key_x),
    CONFIG_VARIABLE_INT		(key_y),
*/
/*
#ifdef FEATURE_SOUND

    //!
    // Controls whether libsamplerate support is used for performing
    // sample rate conversions of sound effects.  Support for this
    // must be compiled into the program.
    //
    // If zero, libsamplerate support is disabled.  If non-zero,
    // libsamplerate is enabled. Increasing values roughly correspond
    // to higher quality conversion; the higher the quality, the
    // slower the conversion process.  Linear conversion = 1;
    // Zero order hold = 2; Fast Sinc filter = 3; Medium quality
    // Sinc filter = 4; High quality Sinc filter = 5.
    //

    CONFIG_VARIABLE_INT(use_libsamplerate),

    //!
    // Scaling factor used by libsamplerate. This is used when converting
    // sounds internally back into integer form; normally it should not
    // be necessary to change it from the default value. The only time
    // it might be needed is if a PWAD file is loaded that contains very
    // loud sounds, in which case the conversion may cause sound clipping
    // and the scale factor should be reduced. The lower the value, the
    // quieter the sound effects become, so it should be set as high as is
    // possible without clipping occurring.

    CONFIG_VARIABLE_FLOAT(libsamplerate_scale),
#endif
*/
//    CONFIG_VARIABLE_INT		(key_invkey)
/*
    { "key_up",			&key_up,		KEY_UPARROW,		0, 254 },
    { "key_down",		&key_down,		KEY_DOWNARROW,		0, 254 },
    { "key_left",		&key_left,		KEY_LEFTARROW,		0, 254 },
    { "key_right",		&key_right,		KEY_RIGHTARROW,		0, 254 },
    { "key_invright",		&key_invright,		KEY_TRIANGLE,		0, 254 },
    { "key_jump",		&key_jump,		KEY_CROSS,		0, 254 },
    { "key_invpop",		&key_invpop,		KEY_SQUARE,		0, 254 },
    { "key_mission",		&key_mission,		KEY_CIRCLE,		0, 254 },
    { "key_strafeleft",		&key_strafeleft,	KEY_SELECT,		0, 254 },
    { "key_invuse",		&key_invuse,		KEY_START,		0, 254 },
    { "key_use",		&key_use,		KEY_LEFTTRIGGER,	0, 254 },
    { "key_fire",		&key_fire,		KEY_RIGHTTRIGGER,	0, 254 }
*/
};

/*static */default_collection_t doom_defaults =
{
    doom_defaults_list,
    arrlen(doom_defaults_list),
    NULL,
};

/*
    { "key_up",			&key_up,		KEY_UPARROW,		0, 254 },
    { "key_down",		&key_down,		KEY_DOWNARROW,		0, 254 },
    { "key_left",		&key_left,		KEY_LEFTARROW,		0, 254 },
    { "key_right",		&key_right,		KEY_RIGHTARROW,		0, 254 },
    { "key_triangle",		&key_invright,		KEY_TRIANGLE,		0, 254 },
    { "key_cross",		&key_jump,		KEY_CROSS,		0, 254 },
    { "key_square",		&key_invpop,		KEY_SQUARE,		0, 254 },
    { "key_circle",		&key_mission,		KEY_CIRCLE,		0, 254 },
    { "key_select",		&key_strafeleft,	KEY_SELECT,		0, 254 },
    { "key_start",		&key_invuse,		KEY_START,		0, 254 },
    { "key_lefttrigger",	&key_use,		KEY_LEFTTRIGGER,	0, 254 },
    { "key_righttrigger",	&key_fire,		KEY_RIGHTTRIGGER,	0, 254 }
*/
/*
    //!
    // Volume of sound effects, range 0-15.
    //

    CONFIG_VARIABLE_INT(sfx_volume),

    //!
    // Volume of in-game music, range 0-15.
    //

    CONFIG_VARIABLE_INT(music_volume),

    //!
    // @game strife
    //
    // If non-zero, dialogue text is displayed over characters' pictures
    // when engaging actors who have voices.
    //

    CONFIG_VARIABLE_INT(show_talk),

    //!
    // @game strife
    //
    // Volume of voice sound effects, range 0-15.
    //

    CONFIG_VARIABLE_INT(voice_volume),

    //!
    // @game strife
    //
    // Screen size, range 3-11.
    //
    // A value of 11 gives a full-screen view with the status bar not
    // displayed.  A value of 10 gives a full-screen view with the
    // status bar displayed.
    //

    CONFIG_VARIABLE_INT(screensize),

    CONFIG_VARIABLE_INT(movement_speed),

    CONFIG_VARIABLE_INT(freelook_speed),

    //!
    // Gamma correction level.  A value of zero disables gamma
    // correction, while a value in the range 1-4 gives increasing
    // levels of gamma correction.
    //

    CONFIG_VARIABLE_INT(use_gamma),

    CONFIG_VARIABLE_INT(mouselook),
*/
    //!
    // Mouse sensitivity.  This value is used to multiply input mouse
    // movement to control the effect of moving the mouse.
    //
    // The "normal" maximum value available for this through the
    // in-game options menu is 9. A value of 31 or greater will cause
    // the game to crash when entering the options menu.
    //

//    CONFIG_VARIABLE_INT(mouse_sensitivity),

    //!
    // @game doom
    //
    // If non-zero, messages are displayed on the heads-up display
    // in the game ("picked up a clip", etc).  If zero, these messages
    // are not displayed.
    //

//    CONFIG_VARIABLE_INT(show_messages),
/*
    //!
    // Keyboard key to move forward.
    //

    CONFIG_VARIABLE_KEY(key_up),

    //!
    // Keyboard key to move backward.
    //

    CONFIG_VARIABLE_KEY(key_down),

    //!
    // Keyboard key to turn left.
    //

    CONFIG_VARIABLE_KEY(key_left),

    //!
    // Keyboard key to turn right.
    //

    CONFIG_VARIABLE_KEY(key_right),

    //!
    // @game strife
    //
    // Keyboard key to scroll right in the inventory.
    //

    CONFIG_VARIABLE_KEY(key_invRight),

    //!
    // @game hexen
    //
    // Keyboard key to jump.
    //

    CONFIG_VARIABLE_KEY(key_jump),

    //!
    // @game strife
    //
    // Keyboard key to display inventory popup.
    //

    CONFIG_VARIABLE_KEY(key_invPop),

    //!
    // @game strife
    //
    // Keyboard key to display mission objective.
    //

    CONFIG_VARIABLE_KEY(key_mission),

    //!
    // Keyboard key to strafe left.
    //

    CONFIG_VARIABLE_KEY(key_strafeleft),

    //!
    // @game strife
    //
    // Keyboard key to use inventory item.
    //

    CONFIG_VARIABLE_KEY(key_invUse),

    //!
    // Keyboard key to "use" an object, eg. a door or switch.
    //

    CONFIG_VARIABLE_KEY(key_use),

    //!
    // Keyboard key to fire the currently selected weapon.
    //

    CONFIG_VARIABLE_KEY(key_fire),
*/
    //!
    // Keyboard key to strafe right.
    //
/*
    CONFIG_VARIABLE_KEY(key_straferight),

    //!
    // @game strife
    //
    // Keyboard key to use health.
    //

    CONFIG_VARIABLE_KEY(key_useHealth),
*/
    //!
    // @game heretic hexen
    //
    // Keyboard key to fly upward.
    //
/*
    CONFIG_VARIABLE_KEY(key_flyup),

    //!
    // @game heretic hexen
    //
    // Keyboard key to fly downwards.
    //

    CONFIG_VARIABLE_KEY(key_flydown),

    //!
    // @game heretic hexen
    //
    // Keyboard key to center flying.
    //

    CONFIG_VARIABLE_KEY(key_flycenter),

    //!
    // @game heretic hexen
    //
    // Keyboard key to look up.
    //

    CONFIG_VARIABLE_KEY(key_lookup),

    //!
    // @game heretic hexen
    //
    // Keyboard key to look down.
    //

    CONFIG_VARIABLE_KEY(key_lookdown),

    //!
    // @game heretic hexen
    //
    // Keyboard key to center the view.
    //

    CONFIG_VARIABLE_KEY(key_lookcenter),

    //!
    // @game strife
    //
    // Keyboard key to query inventory.
    //

    CONFIG_VARIABLE_KEY(key_invquery),
*/
/*
    //!
    // @game strife
    //
    // Keyboard key to display keys popup.
    //

    CONFIG_VARIABLE_KEY(key_invKey),

    //!
    // @game strife
    //
    // Keyboard key to jump to start of inventory.
    //

    CONFIG_VARIABLE_KEY(key_invHome),

    //!
    // @game strife
    //
    // Keyboard key to jump to end of inventory.
    //

    CONFIG_VARIABLE_KEY(key_invEnd),

    //!
    // @game heretic hexen
    //
    // Keyboard key to scroll left in the inventory.
    //

    CONFIG_VARIABLE_KEY(key_invleft),

    //!
    // @game heretic hexen
    //
    // Keyboard key to scroll right in the inventory.
    //

    CONFIG_VARIABLE_KEY(key_invright),

    //!
    // @game strife
    //
    // Keyboard key to scroll left in the inventory.
    //

    CONFIG_VARIABLE_KEY(key_invLeft),
*/
    //!
    // @game heretic hexen
    //
    // Keyboard key to use the current item in the inventory.
    //

//    CONFIG_VARIABLE_KEY(key_useartifact),

    //!
    // @game strife
    //
    // Keyboard key to drop an inventory item.
    //

//    CONFIG_VARIABLE_KEY(key_invDrop),

    //!
    // @game strife
    //
    // Keyboard key to look up.
    //
/*
    CONFIG_VARIABLE_KEY(key_lookUp),

    //!
    // @game strife
    //
    // Keyboard key to look down.
    //

    CONFIG_VARIABLE_KEY(key_lookDown),
*/
    //!
    // Keyboard key to turn on strafing.  When held down, pressing the
    // key to turn left or right causes the player to strafe left or
    // right instead.
    //

//    CONFIG_VARIABLE_KEY(key_strafe),

    //!
    // Keyboard key to make the player run.
    //

//    CONFIG_VARIABLE_KEY(key_speed),
/*
    //!
    // If non-zero, mouse input is enabled.  If zero, mouse input is
    // disabled.
    //
*/
//    CONFIG_VARIABLE_INT(use_mouse),

    //!
    // Mouse button to fire the currently selected weapon.
    //
/*
    CONFIG_VARIABLE_INT(mouseb_fire),

    //!
    // Mouse button to turn on strafing.  When held down, the player
    // will strafe left and right instead of turning left and right.
    //

    CONFIG_VARIABLE_INT(mouseb_strafe),

    //!
    // Mouse button to move forward.
    //

    CONFIG_VARIABLE_INT(mouseb_forward),

    //!
    // @game hexen strife
    //
    // Mouse button to jump.
    //

    CONFIG_VARIABLE_INT(mouseb_jump),

    //!
    // If non-zero, joystick input is enabled.
    //

    CONFIG_VARIABLE_INT(use_joystick),

    //!
    // Joystick virtual button that fires the current weapon.
    //

    CONFIG_VARIABLE_INT(joyb_fire),

    //!
    // Joystick virtual button that makes the player strafe while
    // held down.
    //

    CONFIG_VARIABLE_INT(joyb_strafe),

    //!
    // Joystick virtual button to "use" an object, eg. a door or switch.
    //

    CONFIG_VARIABLE_INT(joyb_use),

    //!
    // Joystick virtual button that makes the player run while held
    // down.
    //
    // If this has a value of 20 or greater, the player will always run,
    // even if use_joystick is 0.
    //

    CONFIG_VARIABLE_INT(joyb_speed),

    //!
    // @game hexen strife
    //
    // Joystick virtual button that makes the player jump.
    //

    CONFIG_VARIABLE_INT(joyb_jump),

    //!
    // @game doom heretic hexen
    //
    // Screen size, range 3-11.
    //
    // A value of 11 gives a full-screen view with the status bar not
    // displayed.  A value of 10 gives a full-screen view with the
    // status bar displayed.
    //

    CONFIG_VARIABLE_INT(screenblocks),
*/
    //!
    // @game doom
    //
    // Screen detail.  Zero gives normal "high detail" mode, while
    // a non-zero value gives "low detail" mode.
    //
/*
    CONFIG_VARIABLE_INT(detaillevel),

    //!
    // Number of sounds that will be played simultaneously.
    //

    CONFIG_VARIABLE_INT(snd_channels),

    //!
    // Music output device.  A non-zero value gives MIDI sound output,
    // while a value of zero disables music.
    //

    CONFIG_VARIABLE_INT(snd_musicdevice),

    //!
    // Sound effects device.  A value of zero disables in-game sound
    // effects, a value of 1 enables PC speaker sound effects, while
    // a value in the range 2-9 enables the "normal" digital sound
    // effects.
    //

    CONFIG_VARIABLE_INT(snd_sfxdevice),

    //!
    // SoundBlaster I/O port. Unused.
    //

    CONFIG_VARIABLE_INT(snd_sbport),

    //!
    // SoundBlaster IRQ.  Unused.
    //

    CONFIG_VARIABLE_INT(snd_sbirq),

    //!
    // SoundBlaster DMA channel.  Unused.
    //

    CONFIG_VARIABLE_INT(snd_sbdma),

    //!
    // Output port to use for OPL MIDI playback.  Unused.
    //

    CONFIG_VARIABLE_INT(snd_mport),

    //!
    // @game hexen
    //
    // Directory in which to store savegames.
    //

    CONFIG_VARIABLE_STRING(savedir),

    //!
    // @game hexen
    //
    // Controls whether messages are displayed in the heads-up display.
    // If this has a non-zero value, messages are displayed.
    //

    CONFIG_VARIABLE_INT(messageson),

    //!
    // @game strife
    //
    // Name of background flat used by view border.
    //

    CONFIG_VARIABLE_STRING(back_flat),

    //!
    // @game strife
    //
    // Multiplayer nickname (?).
    //

    CONFIG_VARIABLE_STRING(nickname),

    //!
    // Multiplayer chat macro: message to send when alt+0 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro0),

    //!
    // Multiplayer chat macro: message to send when alt+1 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro1),

    //!
    // Multiplayer chat macro: message to send when alt+2 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro2),

    //!
    // Multiplayer chat macro: message to send when alt+3 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro3),

    //!
    // Multiplayer chat macro: message to send when alt+4 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro4),

    //!
    // Multiplayer chat macro: message to send when alt+5 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro5),

    //!
    // Multiplayer chat macro: message to send when alt+6 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro6),

    //!
    // Multiplayer chat macro: message to send when alt+7 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro7),

    //!
    // Multiplayer chat macro: message to send when alt+8 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro8),

    //!
    // Multiplayer chat macro: message to send when alt+9 is pressed.
    //

    CONFIG_VARIABLE_STRING(chatmacro9),

    //!
    // @game strife
    //
    // Serial port number to use for SERSETUP.EXE (unused).
    //

    CONFIG_VARIABLE_INT(comport),
};
*/

//! @begin_config_file extended
/*
static default_t extra_defaults_list[] =
{
    //!
    // @game heretic hexen strife
    //
    // If non-zero, display the graphical startup screen.
    //

    CONFIG_VARIABLE_INT(graphical_startup),

    //!
    // If non-zero, video settings will be autoadjusted to a valid
    // configuration when the screen_width and screen_height variables
    // do not match any valid configuration.
    //

    CONFIG_VARIABLE_INT(autoadjust_video_settings),

    //!
    // If non-zero, the game will run in full screen mode.  If zero,
    // the game will run in a window.
    //

    CONFIG_VARIABLE_INT(fullscreen),

    //!
    // If non-zero, the screen will be stretched vertically to display
    // correctly on a square pixel video mode.
    //

    CONFIG_VARIABLE_INT(aspect_ratio_correct),

    //!
    // Number of milliseconds to wait on startup after the video mode
    // has been set, before the game will start.  This allows the
    // screen to settle on some monitors that do not display an image
    // for a brief interval after changing video modes.
    //

    CONFIG_VARIABLE_INT(startup_delay),

    //!
    // Screen width in pixels.  If running in full screen mode, this is
    // the X dimension of the video mode to use.  If running in
    // windowed mode, this is the width of the window in which the game
    // will run.
    //

    CONFIG_VARIABLE_INT(screen_width),

    //!
    // Screen height in pixels.  If running in full screen mode, this is
    // the Y dimension of the video mode to use.  If running in
    // windowed mode, this is the height of the window in which the game
    // will run.
    //

    CONFIG_VARIABLE_INT(screen_height),

    //!
    // Color depth of the screen, in bits.
    // If this is set to zero, the color depth will be automatically set
    // on startup to the machine's default/native color depth.
    //

    CONFIG_VARIABLE_INT(screen_bpp),

    //!
    // Maximum scale factor for the intermediate buffer used for doing
    // hardware-based scaling. A scale factor of 1 will be very blurry
    // but not use a lot of texture memory; a scale factor of 4 gives
    // pretty much best results.

    CONFIG_VARIABLE_INT(gl_max_scale),
    //!
    // If this is non-zero, the mouse will be "grabbed" when running
    // in windowed mode so that it can be used as an input device.
    // When running full screen, this has no effect.
    //

    CONFIG_VARIABLE_INT(grabmouse),

    //!
    // If non-zero, all vertical mouse movement is ignored.  This
    // emulates the behavior of the "novert" tool available under DOS
    // that performs the same function.
    //

    CONFIG_VARIABLE_INT(novert),

    //!
    // Mouse acceleration factor.  When the speed of mouse movement
    // exceeds the threshold value (mouse_threshold), the speed is
    // multiplied by this value.
    //

    CONFIG_VARIABLE_FLOAT(mouse_acceleration),

    //!
    // Mouse acceleration threshold.  When the speed of mouse movement
    // exceeds this threshold value, the speed is multiplied by an
    // acceleration factor (mouse_acceleration).
    //

    CONFIG_VARIABLE_INT(mouse_threshold),

    //!
    // Sound output sample rate, in Hz.  Typical values to use are
    // 11025, 22050, 44100 and 48000.
    //

    CONFIG_VARIABLE_INT(snd_samplerate),

    //!
    // Maximum number of bytes to allocate for caching converted sound
    // effects in memory. If set to zero, there is no limit applied.
    //

    CONFIG_VARIABLE_INT(snd_cachesize),

    //!
    // Maximum size of the output sound buffer size in milliseconds.
    // Sound output is generated periodically in slices. Higher values
    // might be more efficient but will introduce latency to the
    // sound output. The default is 28ms (one slice per tic with the
    // 35fps timer).

    CONFIG_VARIABLE_INT(snd_maxslicetime_ms),

    //!
    // External command to invoke to perform MIDI playback. If set to
    // the empty string, SDL_mixer's internal MIDI playback is used.
    // This only has any effect when snd_musicdevice is set to General
    // MIDI output.

    CONFIG_VARIABLE_STRING(snd_musiccmd),

    //!
    // The I/O port to use to access the OPL chip.  Only relevant when
    // using native OPL music playback.
    //

    CONFIG_VARIABLE_INT_HEX(opl_io_port),

    //!
    // @game doom heretic strife
    //
    // If non-zero, the ENDOOM text screen is displayed when exiting the
    // game. If zero, the ENDOOM screen is not displayed.
    //

    CONFIG_VARIABLE_INT(show_endoom),

    //!
    // If non-zero, save screenshots in PNG format.
    //

    CONFIG_VARIABLE_INT(png_screenshots),

    //!
    // @game doom strife
    //
    // If non-zero, the Vanilla savegame limit is enforced; if the
    // savegame exceeds 180224 bytes in size, the game will exit with
    // an error.  If this has a value of zero, there is no limit to
    // the size of savegames.
    //

    CONFIG_VARIABLE_INT(vanilla_savegame_limit),

    //!
    // @game doom strife
    //
    // If non-zero, the Vanilla demo size limit is enforced; the game
    // exits with an error when a demo exceeds the demo size limit
    // (128KiB by default).  If this has a value of zero, there is no
    // limit to the size of demos.
    //

    CONFIG_VARIABLE_INT(vanilla_demo_limit),

    //!
    // If non-zero, the game behaves like Vanilla Doom, always assuming
    // an American keyboard mapping.  If this has a value of zero, the
    // native keyboard mapping of the keyboard is used.
    //

    CONFIG_VARIABLE_INT(vanilla_keyboard_mapping),

    //!
    // Name of the SDL video driver to use.  If this is an empty string,
    // the default video driver is used.
    //

    CONFIG_VARIABLE_STRING(video_driver),

    //!
    // Position of the window on the screen when running in windowed
    // mode. Accepted values are: "" (empty string) - don't care,
    // "center" - place window at center of screen, "x,y" - place
    // window at the specified coordinates.

    CONFIG_VARIABLE_STRING(window_position),
*/
/*
#ifdef FEATURE_MULTIPLAYER

    //!
    // Name to use in network games for identification.  This is only
    // used on the "waiting" screen while waiting for the game to start.
    //

    CONFIG_VARIABLE_STRING(player_name),

#endif
*/
    //!
    // Joystick number to use; '0' is the first joystick.  A negative
    // value ('-1') indicates that no joystick is configured.
    //
/*
    CONFIG_VARIABLE_INT(joystick_index),

    //!
    // Joystick axis to use to for horizontal (X) movement.
    //

    CONFIG_VARIABLE_INT(joystick_x_axis),

    //!
    // If non-zero, movement on the horizontal joystick axis is inverted.
    //

    CONFIG_VARIABLE_INT(joystick_x_invert),

    //!
    // Joystick axis to use to for vertical (Y) movement.
    //

    CONFIG_VARIABLE_INT(joystick_y_axis),

    //!
    // If non-zero, movement on the vertical joystick axis is inverted.
    //

    CONFIG_VARIABLE_INT(joystick_y_invert),

    //!
    // Joystick axis to use to for strafing movement.
    //

    CONFIG_VARIABLE_INT(joystick_strafe_axis),

    //!
    // If non-zero, movement on the joystick axis used for strafing
    // is inverted.
    //

    CONFIG_VARIABLE_INT(joystick_strafe_invert),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #0.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button0),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #1.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button1),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #2.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button2),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #3.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button3),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #4.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button4),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #5.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button5),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #6.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button6),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #7.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button7),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #8.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button8),

    //!
    // The physical joystick button that corresponds to joystick
    // virtual button #9.
    //

    CONFIG_VARIABLE_INT(joystick_physical_button9),

    //!
    // Joystick virtual button to make the player strafe left.
    //

    CONFIG_VARIABLE_INT(joyb_strafeleft),

    //!
    // Joystick virtual button to make the player strafe right.
    //

    CONFIG_VARIABLE_INT(joyb_straferight),

    //!
    // Joystick virtual button to activate the menu.
    //

    CONFIG_VARIABLE_INT(joyb_menu_activate),

    //!
    // Joystick virtual button that cycles to the previous weapon.
    //

    CONFIG_VARIABLE_INT(joyb_prevweapon),

    //!
    // Joystick virtual button that cycles to the next weapon.
    //

    CONFIG_VARIABLE_INT(joyb_nextweapon),

    //!
    // Mouse button to strafe left.
    //

    CONFIG_VARIABLE_INT(mouseb_strafeleft),

    //!
    // Mouse button to strafe right.
    //

    CONFIG_VARIABLE_INT(mouseb_straferight),

    //!
    // Mouse button to "use" an object, eg. a door or switch.
    //

    CONFIG_VARIABLE_INT(mouseb_use),

    //!
    // Mouse button to move backwards.
    //

    CONFIG_VARIABLE_INT(mouseb_backward),

    //!
    // Mouse button to cycle to the previous weapon.
    //

    CONFIG_VARIABLE_INT(mouseb_prevweapon),

    //!
    // Mouse button to cycle to the next weapon.
    //

    CONFIG_VARIABLE_INT(mouseb_nextweapon),

    //!
    // If non-zero, double-clicking a mouse button acts like pressing
    // the "use" key to use an object in-game, eg. a door or switch.
    //

    CONFIG_VARIABLE_INT(dclick_use),

    //!
    // Full path to a Timidity configuration file to use for MIDI
    // playback. The file will be evaluated from the directory where
    // it is evaluated, so there is no need to add "dir" commands
    // into it.
    //

    CONFIG_VARIABLE_STRING(timidity_cfg_path),

    //!
    // Path to GUS patch files to use when operating in GUS emulation
    // mode.
    //

    CONFIG_VARIABLE_STRING(gus_patch_path),

    //!
    // Number of kilobytes of RAM to use in GUS emulation mode. Valid
    // values are 256, 512, 768 or 1024.
    //

    CONFIG_VARIABLE_INT(gus_ram_kb),

#endif

    //!
    // Key to pause or unpause the game.
    //

    CONFIG_VARIABLE_KEY(key_pause),

    //!
    // Key that activates the menu when pressed.
    //

    CONFIG_VARIABLE_KEY(key_menu_activate),

    //!
    // Key that moves the cursor up on the menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_up),

    //!
    // Key that moves the cursor down on the menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_down),

    //!
    // Key that moves the currently selected slider on the menu left.
    //

    CONFIG_VARIABLE_KEY(key_menu_left),

    //!
    // Key that moves the currently selected slider on the menu right.
    //

    CONFIG_VARIABLE_KEY(key_menu_right),

    //!
    // Key to go back to the previous menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_back),

    //!
    // Key to activate the currently selected menu item.
    //

    CONFIG_VARIABLE_KEY(key_menu_forward),

    //!
    // Key to answer 'yes' to a question in the menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_confirm),

    //!
    // Key to answer 'no' to a question in the menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_abort),

    //!
    // Keyboard shortcut to bring up the help screen.
    //

    CONFIG_VARIABLE_KEY(key_menu_help),

    //!
    // Keyboard shortcut to bring up the save game menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_save),

    //!
    // Keyboard shortcut to bring up the load game menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_load),

    //!
    // Keyboard shortcut to bring up the sound volume menu.
    //

    CONFIG_VARIABLE_KEY(key_menu_volume),

    //!
    // Keyboard shortcut to toggle the detail level.
    //

    CONFIG_VARIABLE_KEY(key_menu_detail),

    //!
    // Keyboard shortcut to quicksave the current game.
    //

    CONFIG_VARIABLE_KEY(key_menu_qsave),

    //!
    // Keyboard shortcut to end the game.
    //

    CONFIG_VARIABLE_KEY(key_menu_endgame),

    //!
    // Keyboard shortcut to toggle heads-up messages.
    //

    CONFIG_VARIABLE_KEY(key_menu_messages),

    //!
    // Keyboard shortcut to load the last quicksave.
    //

    CONFIG_VARIABLE_KEY(key_menu_qload),

    //!
    // Keyboard shortcut to quit the game.
    //

    CONFIG_VARIABLE_KEY(key_menu_quit),

    //!
    // Keyboard shortcut to toggle the gamma correction level.
    //

    CONFIG_VARIABLE_KEY(key_menu_gamma),

    //!
    // Keyboard shortcut to switch view in multiplayer.
    //

    CONFIG_VARIABLE_KEY(key_spy),

    //!
    // Keyboard shortcut to increase the screen size.
    //

    CONFIG_VARIABLE_KEY(key_menu_incscreen),

    //!
    // Keyboard shortcut to decrease the screen size.
    //

    CONFIG_VARIABLE_KEY(key_menu_decscreen),

    //!
    // Keyboard shortcut to save a screenshot.
    //

    CONFIG_VARIABLE_KEY(key_menu_screenshot),

    //!
    // Key to toggle the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_toggle),

    //!
    // Key to pan north when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_north),

    //!
    // Key to pan south when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_south),

    //!
    // Key to pan east when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_east),

    //!
    // Key to pan west when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_west),

    //!
    // Key to zoom in when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_zoomin),

    //!
    // Key to zoom out when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_zoomout),

    //!
    // Key to zoom out the maximum amount when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_maxzoom),

    //!
    // Key to toggle follow mode when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_follow),

    //!
    // Key to toggle the grid display when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_grid),

    //!
    // Key to set a mark when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_mark),

    //!
    // Key to clear all marks when in the map view.
    //

    CONFIG_VARIABLE_KEY(key_map_clearmark),

    //!
    // Key to select weapon 1.
    //

    CONFIG_VARIABLE_KEY(key_weapon1),

    //!
    // Key to select weapon 2.
    //

    CONFIG_VARIABLE_KEY(key_weapon2),

    //!
    // Key to select weapon 3.
    //

    CONFIG_VARIABLE_KEY(key_weapon3),

    //!
    // Key to select weapon 4.
    //

    CONFIG_VARIABLE_KEY(key_weapon4),

    //!
    // Key to select weapon 5.
    //

    CONFIG_VARIABLE_KEY(key_weapon5),

    //!
    // Key to select weapon 6.
    //

    CONFIG_VARIABLE_KEY(key_weapon6),

    //!
    // Key to select weapon 7.
    //

    CONFIG_VARIABLE_KEY(key_weapon7),

    //!
    // Key to select weapon 8.
    //

    CONFIG_VARIABLE_KEY(key_weapon8),

    //!
    // Key to cycle to the previous weapon.
    //

    CONFIG_VARIABLE_KEY(key_prevweapon),

    //!
    // Key to cycle to the next weapon.
    //

    CONFIG_VARIABLE_KEY(key_nextweapon),

    //!
    // @game hexen
    //
    // Key to use one of each artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_all),

    //!
    // @game hexen
    //
    // Key to use "quartz flask" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_health),

    //!
    // @game hexen
    //
    // Key to use "flechette" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_poisonbag),

    //!
    // @game hexen
    //
    // Key to use "disc of repulsion" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_blastradius),

    //!
    // @game hexen
    //
    // Key to use "chaos device" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_teleport),

    //!
    // @game hexen
    //
    // Key to use "banishment device" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_teleportother),

    //!
    // @game hexen
    //
    // Key to use "porkalator" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_egg),

    //!
    // @game hexen
    //
    // Key to use "icon of the defender" artifact.
    //

    CONFIG_VARIABLE_KEY(key_arti_invulnerability),

    //!
    // Key to re-display last message.
    //

    CONFIG_VARIABLE_KEY(key_message_refresh),

    //!
    // Key to quit the game when recording a demo.
    //

    CONFIG_VARIABLE_KEY(key_demo_quit),

    //!
    // Key to send a message during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msg),

    //!
    // Key to send a message to player 1 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer1),

    //!
    // Key to send a message to player 2 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer2),

    //!
    // Key to send a message to player 3 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer3),

    //!
    // Key to send a message to player 4 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer4),

    //!
    // @game hexen strife
    //
    // Key to send a message to player 5 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer5),

    //!
    // @game hexen strife
    //
    // Key to send a message to player 6 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer6),

    //!
    // @game hexen strife
    //
    // Key to send a message to player 7 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer7),

    //!
    // @game hexen strife
    //
    // Key to send a message to player 8 during multiplayer games.
    //

    CONFIG_VARIABLE_KEY(key_multi_msgplayer8),

};

static default_collection_t extra_defaults =
{
    extra_defaults_list,
    arrlen(extra_defaults_list),
    NULL,
};
*/
// Search a collection for a variable

static default_t *SearchCollection(default_collection_t *collection, char *name)
{
    int i;

    for (i=0; i<collection->numdefaults; ++i) 
    {
        if (!strcmp(name, collection->defaults[i].name))
        {
            return &collection->defaults[i];
        }
    }

    return NULL;
}
/*
// Mapping from DOS keyboard scan code to internal key code (as defined
// in doomkey.h). I think I (fraggle) reused this from somewhere else
// but I can't find where. Anyway, notes:
//  * KEY_PAUSE is wrong - it's in the KEY_NUMLOCK spot. This shouldn't
//    matter in terms of Vanilla compatibility because neither of
//    those were valid for key bindings.
//  * There is no proper scan code for PrintScreen (on DOS machines it
//    sends an interrupt). So I added a fake scan code of 126 for it.
//    The presence of this is important so we can bind PrintScreen as
//    a screenshot key.
static const int scantokey[128] =
{
    0  ,    27,     '1',    '2',    '3',    '4',    '5',    '6',
    '7',    '8',    '9',    '0',    '-',    '=',    KEY_BACKSPACE, 9,
    'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
    'o',    'p',    '[',    ']',    13,		KEY_RCTRL, 'a',    's',
    'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
    '\'',   '`',    KEY_RSHIFT,'\\',   'z',    'x',    'c',    'v',
    'b',    'n',    'm',    ',',    '.',    '/',    KEY_RSHIFT,KEYP_MULTIPLY,
    KEY_RALT,  ' ',  KEY_CAPSLOCK,KEY_F1,  KEY_F2,   KEY_F3,   KEY_F4,   KEY_F5,
    KEY_F6,   KEY_F7,   KEY_F8,   KEY_F9,   KEY_F10,  *//*KEY_NUMLOCK?*//*KEY_PAUSE,KEY_SCRLCK,KEY_HOME,
    KEY_UPARROW,KEY_PGUP,KEY_MINUS,KEY_LEFTARROW,KEYP_5,KEY_RIGHTARROW,KEYP_PLUS,KEY_END,
    KEY_DOWNARROW,KEY_PGDN,KEY_INS,KEY_DEL,0,   0,      0,      KEY_F11,
    KEY_F12,  0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      KEY_PRTSCR, 0
};
*/

static void SaveDefaultCollection(default_collection_t *collection)
{
    default_t *defaults;
    int i, v;
    FILE *f;
	
    f = fopen (collection->filename, "w");
    if (!f)
	return; // can't write the file, but don't complain

    defaults = collection->defaults;
		
    for (i=0 ; i<collection->numdefaults ; i++)
    {
        int chars_written;

        // Ignore unbound variables

        if (!defaults[i].bound)
        {
            continue;
        }

        // Print the name and line up all values at 30 characters

        chars_written = fprintf(f, "%s ", defaults[i].name);

        for (; chars_written < 30; ++chars_written)
            fprintf(f, " ");

        // Print the value

        switch (defaults[i].type) 
        {
            case DEFAULT_KEY:

                // use the untranslated version if we can, to reduce
                // the possibility of screwing up the user's config
                // file
                
                v = * (int *) defaults[i].location;

                if (v == KEY_RSHIFT)
                {
                    // Special case: for shift, force scan code for
                    // right shift, as this is what Vanilla uses.
                    // This overrides the change check below, to fix
                    // configuration files made by old versions that
                    // mistakenly used the scan code for left shift.

                    v = 54;
                }
                else if (defaults[i].untranslated
                      && v == defaults[i].original_translated)
                {
                    // Has not been changed since the last time we
                    // read the config file.

                    v = defaults[i].untranslated;
                }
/*
                else
                {
                    // search for a reverse mapping back to a scancode
                    // in the scantokey table

                    int s;

                    for (s=0; s<128; ++s)
                    {
                        if (scantokey[s] == v)
                        {
                            v = s;
                            break;
                        }
                    }
                }
*/
	        fprintf(f, "%i", v);
                break;

            case DEFAULT_INT:
	        fprintf(f, "%i", * (int *) defaults[i].location);
                break;

            case DEFAULT_INT_HEX:
	        fprintf(f, "0x%x", * (int *) defaults[i].location);
                break;

            case DEFAULT_FLOAT:
                fprintf(f, "%f", * (float *) defaults[i].location);
                break;

            case DEFAULT_STRING:
	        fprintf(f,"\"%s\"", * (char **) (defaults[i].location));
                break;
        }

        fprintf(f, "\n");
    }

    fclose (f);
}

// Parses integer values in the configuration file

static int ParseIntParameter(char *strparm)
{
    int parm;

    if (strparm[0] == '0' && strparm[1] == 'x')
        sscanf(strparm+2, "%x", &parm);
    else
        sscanf(strparm, "%i", &parm);

    return parm;
}

static void SetVariable(default_t *def, char *value)
{
    int intparm;

    // parameter found

    switch (def->type)
    {
        case DEFAULT_STRING:
            * (char **) def->location = strdup(value);
            break;

        case DEFAULT_INT:
        case DEFAULT_INT_HEX:
            * (int *) def->location = ParseIntParameter(value);
            break;

        case DEFAULT_KEY:

            // translate scancodes read from config
            // file (save the old value in untranslated)

            intparm = ParseIntParameter(value);
            def->untranslated = intparm;
/*
            if (intparm >= 0 && intparm < 128)
            {
                intparm = scantokey[intparm];
            }
            else

            {
                intparm = 0;
            }

            def->original_translated = intparm;
*/
            * (int *) def->location = intparm;

            break;

        case DEFAULT_FLOAT:
            * (float *) def->location = (float) atof(value);
            break;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wchar-subscripts"

static void LoadDefaultCollection(default_collection_t *collection)
{
    FILE *f;
    default_t *def;
    char defname[80];
    char strparm[100];

    // read the file in, overriding any set defaults
    f = fopen(collection->filename, "r");

    if (f == NULL)
    {
        // File not opened, but don't complain. 
        // It's probably just the first time they ran the game.

        return;
    }

    while (!feof(f))
    {
        if (fscanf(f, "%79s %99[^\n]\n", defname, strparm) != 2)
        {
            // This line doesn't match

            continue;
        }

        // Find the setting in the list

        def = SearchCollection(collection, defname);

        if (def == NULL || !def->bound)
        {
            // Unknown variable?  Unbound variables are also treated
            // as unknown.

            continue;
        }

        // Strip off trailing non-printable characters (\r characters
        // from DOS text files)

        while (strlen(strparm) > 0 && !isprint(strparm[strlen(strparm)-1]))
        {
            strparm[strlen(strparm)-1] = '\0';
        }

        // Surrounded by quotes? If so, remove them.
        if (strlen(strparm) >= 2
         && strparm[0] == '"' && strparm[strlen(strparm) - 1] == '"')
        {
            strparm[strlen(strparm) - 1] = '\0';
            memmove(strparm, strparm + 1, sizeof(strparm) - 1);
        }

        SetVariable(def, strparm);
    }

    fclose (f);
}

// Set the default filenames to use for configuration files.

void M_SetConfigFilenames(char *main_config/*, char *extra_config*/)
{
    default_main_config = main_config;
//    default_extra_config = extra_config;
}

//
// M_SaveDefaults
//

void M_SaveDefaults (void)
{
    SaveDefaultCollection(&doom_defaults);
//    SaveDefaultCollection(&extra_defaults);
}

//
// Save defaults to alternate filenames
//

void M_SaveDefaultsAlternate(char *main/*, char *extra*/)
{
    char *orig_main;
//    char *orig_extra;

    // Temporarily change the filenames

    orig_main = doom_defaults.filename;
//    orig_extra = extra_defaults.filename;

    doom_defaults.filename = main;
//    extra_defaults.filename = extra;

    M_SaveDefaults();

    // Restore normal filenames

    doom_defaults.filename = orig_main;
//    extra_defaults.filename = orig_extra;
}

//
// M_LoadDefaults
//

void M_LoadDefaults (void)
{
/*
    int i;
 
    // check for a custom default file

    //!
    // @arg <file>
    // @vanilla
    //
    // Load main configuration from the specified file, instead of the
    // default.
    //

    i = M_CheckParmWithArgs("-config", 1);

    if (i)
    {
	doom_defaults.filename = myargv[i+1];
	printf ("	default file: %s\n",doom_defaults.filename);
    }
    else
*/
    {
        doom_defaults.filename
            = M_StringJoin(configdir, default_main_config, NULL);
    }
/*
    if(devparm)
    	printf("saving config in %s\n", doom_defaults.filename);

    //!
    // @arg <file>
    //
    // Load additional configuration from the specified file, instead of
    // the default.
    //

    i = M_CheckParmWithArgs("-extraconfig", 1);

    if (i)
    {
        extra_defaults.filename = myargv[i+1];
        printf("        extra configuration file: %s\n", 
               extra_defaults.filename);
    }
    else
    {
        extra_defaults.filename
            = M_StringJoin(configdir, default_extra_config, NULL);
    }
*/
    LoadDefaultCollection(&doom_defaults);

//    LoadDefaultCollection(&extra_defaults);
}

// Get a configuration file variable by its name

static default_t *GetDefaultForName(char *name)
{
    default_t *result;

    // Try the main list and the extras

    result = SearchCollection(&doom_defaults, name);
/*
    if (result == NULL)
    {
        result = SearchCollection(&extra_defaults, name);
    }
*/
    // Not found? Internal error.

    if (result == NULL)
    {
        I_Error("Unknown configuration variable: '%s'", name);
    }

    return result;
}

//
// Bind a variable to a given configuration file variable, by name.
//

void M_BindVariable(char *name, void *location)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    variable->location = location;
    variable->bound = true;
}

// Set the value of a particular variable; an API function for other
// parts of the program to assign values to config variables by name.

boolean M_SetVariable(char *name, char *value)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound)
    {
        return false;
    }

    SetVariable(variable, value);

    return true;
}

// Get the value of a variable.

int M_GetIntVariable(char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || (variable->type != DEFAULT_INT && variable->type != DEFAULT_INT_HEX))
    {
        return 0;
    }

    return *((int *) variable->location);
}

const char *M_GetStrVariable(char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || variable->type != DEFAULT_STRING)
    {
        return NULL;
    }

    return *((const char **) variable->location);
}

float M_GetFloatVariable(char *name)
{
    default_t *variable;

    variable = GetDefaultForName(name);

    if (variable == NULL || !variable->bound
     || variable->type != DEFAULT_FLOAT)
    {
        return 0;
    }

    return *((float *) variable->location);
}

// Get the path to the default configuration dir to use, if NULL
// is passed to M_SetConfigDir.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

static char *GetDefaultConfigDir(void)
{

#if !defined(_WIN32) || defined(_WIN32_WCE)

    // Configuration settings are stored in ~/.chocolate-doom/,
    // except on Windows, where we behave like Vanilla Doom and
    // save in the current directory.

    char *homedir;
    char *result;

    homedir = getenv("HOME");

    if (homedir != NULL)
    {
        // put all configuration in a config directory off the
        // homedir

        result = M_StringJoin(homedir, DIR_SEPARATOR_S,
                              "." "chocolate-doom", DIR_SEPARATOR_S, NULL);

        return result;
    }
    else
#endif // #ifndef _WIN32

    {
/*
	if(devparm)
	    printf("FROM M_CONFIG.O: HOME-DIR IS: %s\n", homedir);
*/
	if(usb)
	    return strdup("usb:/apps/wiidoom/");
	else if(sd)
	    return strdup("sd:/apps/wiidoom/");
    }
}

// 
// SetConfigDir:
//
// Sets the location of the configuration directory, where configuration
// files are stored - default.cfg, chocolate-doom.cfg, savegames, etc.
//

void M_SetConfigDir(char *dir)
{
    // Use the directory that was passed, or find the default.

    if (dir != NULL)
    {
        configdir = dir;
    }
    else
    {
        configdir = GetDefaultConfigDir();
    }
/*
    if (strcmp(configdir, "") != 0)
    {
    	if(devparm)
    	    printf("Using %s for configuration and saves\n", configdir);
    }
*/
    // Make the directory if it doesn't already exist:

    M_MakeDirectory(configdir);
}

//
// Calculate the path to the directory to use to store save games.
// Creates the directory as necessary.
//

char *M_GetSaveGameDir(char *iwadname)
{
    char *savegamedir = NULL;
    char *savegameroot;

    // If not "doing" a configuration directory (Windows), don't "do"
    // a savegame directory, either.

    if (!strcmp(configdir, ""))
    {
	savegamedir = strdup("");
    }
    else
    {
        // ~/.chocolate-doom/savegames/

        savegamedir = malloc(strlen(configdir) + 30);
        sprintf(savegamedir, "%ssavegames%c", configdir,
                             DIR_SEPARATOR);

        M_MakeDirectory(savegamedir);

        // eg. ~/.chocolate-doom/savegames/doom2.wad/

        sprintf(savegamedir + strlen(savegamedir), "%s%c",
                iwadname, DIR_SEPARATOR);

	if(usb)
	{
	    savegameroot = SavePathRoot1USB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot2USB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot3USB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot4USB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot5USB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot6USB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot7USB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot8USB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootIWADUSB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootPWADUSB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootD1MusicUSB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootD2MusicUSB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootTNTMusicUSB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootChexMusicUSB;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootHacxMusicUSB;

	    M_MakeDirectory(savegameroot);
	}
	else if(sd)
	{
	    savegameroot = SavePathRoot1SD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot2SD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot3SD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot4SD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot5SD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot6SD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot7SD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRoot8SD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootIWADSD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootPWADSD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootD1MusicSD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootD2MusicSD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootTNTMusicSD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootChexMusicSD;

	    M_MakeDirectory(savegameroot);

	    savegameroot = SavePathRootHacxMusicSD;

	    M_MakeDirectory(savegameroot);
	}

	if(usb)
	{
	    if(fsize == 4261144)
		savegamedir = SavePathBeta14USB;
	    else if(fsize == 4271324)
		savegamedir = SavePathBeta15USB;
	    else if(fsize == 4211660)
		savegamedir = SavePathBeta16USB;
	    else if(fsize == 4207819)
		savegamedir = SavePathShare10USB;
	    else if(fsize == 4274218)
		savegamedir = SavePathShare11USB;
	    else if(fsize == 4225504)
		savegamedir = SavePathShare12USB;
	    else if(fsize == 4225460)
		savegamedir = SavePathShare125USB;
	    else if(fsize == 4234124)
		savegamedir = SavePathShare1666USB;
	    else if(fsize == 4196020)
		savegamedir = SavePathShare18USB;
	    else if(fsize == 10396254)
		savegamedir = SavePathReg11USB;
	    else if(fsize == 10399316)
		savegamedir = SavePathReg12USB;
	    else if(fsize == 10401760)
		savegamedir = SavePathReg16USB;
	    else if(fsize == 11159840)
		savegamedir = SavePathReg18USB;
	    else if(fsize == 12408292)
		savegamedir = SavePathReg19USB;
	    else if(fsize == 12474561)
		savegamedir = SavePathRegBFGXBOX360USB;
	    else if(fsize == 12487824)
		savegamedir = SavePathRegBFGPCUSB;
	    else if(fsize == 12538385)
		savegamedir = SavePathRegXBOXUSB;
	    else if(fsize == 14943400)
		savegamedir = SavePath2Reg1666USB;
	    else if(fsize == 14824716)
		savegamedir = SavePath2Reg1666GUSB;
	    else if(fsize == 14612688)
		savegamedir = SavePath2Reg17USB;
	    else if(fsize == 14607420)
		savegamedir = SavePath2Reg18FUSB;
	    else if(fsize == 14604584)
		savegamedir = SavePath2Reg19USB;
	    else if(fsize == 14677988)
		savegamedir = SavePath2RegBFGPSNUSB;
	    else if(fsize == 14691821)
		savegamedir = SavePath2RegBFGPCUSB;
	    else if(fsize == 14683458)
		savegamedir = SavePath2RegXBOXUSB;
	    else if(fsize == 18195736)
		savegamedir = SavePathTNT191USB;
	    else if(fsize == 18654796)
		savegamedir = SavePathTNT192USB;
	    else if(fsize == 18240172)
		savegamedir = SavePathPLUT191USB;
	    else if(fsize == 17420824)
		savegamedir = SavePathPLUT192USB;
	    else if(fsize == 12361532)
		savegamedir = SavePathChexUSB;

//	    else if(fsize == 9745831)
//		savegamedir = SavePathHacxShare10USB;
//	    else if(fsize == 21951805)
//		savegamedir = SavePathHacxReg10USB;
//	    else if(fsize == 22102300)
//		savegamedir = SavePathHacxReg11USB;

	    else if(fsize == 19321722)
		savegamedir = SavePathHacxReg12USB;

//	    else if(fsize == 19801320)
//		savegamedir = SavePathFreedoom064USB;
//	    else if(fsize == 27704188)
//		savegamedir = SavePathFreedoom07RC1USB;
//	    else if(fsize == 27625596)
//		savegamedir = SavePathFreedoom07USB;
//	    else if(fsize == 28144744)
//		savegamedir = SavePathFreedoom08B1USB;
//	    else if(fsize == 28592816)
//		savegamedir = SavePathFreedoom08USB;
//	    else if(fsize == 19362644)
//		savegamedir = SavePathFreedoom08P1USB;

	    else if(fsize == 28422764)
		savegamedir = SavePathFreedoom08P2USB;
	}
	else if(sd)
	{
	    if(fsize == 4261144)
		savegamedir = SavePathBeta14SD;
	    else if(fsize == 4271324)
		savegamedir = SavePathBeta15SD;
	    else if(fsize == 4211660)
		savegamedir = SavePathBeta16SD;
	    else if(fsize == 4207819)
		savegamedir = SavePathShare10SD;
	    else if(fsize == 4274218)
		savegamedir = SavePathShare11SD;
	    else if(fsize == 4225504)
		savegamedir = SavePathShare12SD;
	    else if(fsize == 4225460)
		savegamedir = SavePathShare125SD;
	    else if(fsize == 4234124)
		savegamedir = SavePathShare1666SD;
	    else if(fsize == 4196020)
		savegamedir = SavePathShare18SD;
	    else if(fsize == 10396254)
		savegamedir = SavePathReg11SD;
	    else if(fsize == 10399316)
		savegamedir = SavePathReg12SD;
	    else if(fsize == 10401760)
		savegamedir = SavePathReg16SD;
	    else if(fsize == 11159840)
		savegamedir = SavePathReg18SD;
	    else if(fsize == 12408292)
		savegamedir = SavePathReg19SD;
	    else if(fsize == 12474561)
		savegamedir = SavePathRegBFGXBOX360SD;
	    else if(fsize == 12487824)
		savegamedir = SavePathRegBFGPCSD;
	    else if(fsize == 12538385)
		savegamedir = SavePathRegXBOXSD;
	    else if(fsize == 14943400)
		savegamedir = SavePath2Reg1666SD;
	    else if(fsize == 14824716)
		savegamedir = SavePath2Reg1666GSD;
	    else if(fsize == 14612688)
		savegamedir = SavePath2Reg17SD;
	    else if(fsize == 14607420)
		savegamedir = SavePath2Reg18FSD;
	    else if(fsize == 14604584)
		savegamedir = SavePath2Reg19SD;
	    else if(fsize == 14677988)
		savegamedir = SavePath2RegBFGPSNSD;
	    else if(fsize == 14691821)
		savegamedir = SavePath2RegBFGPCSD;
	    else if(fsize == 14683458)
		savegamedir = SavePath2RegXBOXSD;
	    else if(fsize == 18195736)
		savegamedir = SavePathTNT191SD;
	    else if(fsize == 18654796)
		savegamedir = SavePathTNT192SD;
	    else if(fsize == 18240172)
		savegamedir = SavePathPLUT191SD;
	    else if(fsize == 17420824)
		savegamedir = SavePathPLUT192SD;
	    else if(fsize == 12361532)
		savegamedir = SavePathChexSD;

//	    else if(fsize == 9745831)
//		savegamedir = SavePathHacxShare10SD;
//	    else if(fsize == 21951805)
//		savegamedir = SavePathHacxReg10SD;
//	    else if(fsize == 22102300)
//		savegamedir = SavePathHacxReg11SD;

	    else if(fsize == 19321722)
		savegamedir = SavePathHacxReg12SD;

//	    else if(fsize == 19801320)
//		savegamedir = SavePathFreedoom064SD;
//	    else if(fsize == 27704188)
//		savegamedir = SavePathFreedoom07RC1SD;
//	    else if(fsize == 27625596)
//		savegamedir = SavePathFreedoom07SD;
//	    else if(fsize == 28144744)
//		savegamedir = SavePathFreedoom08B1SD;
//	    else if(fsize == 28592816)
//		savegamedir = SavePathFreedoom08SD;
//	    else if(fsize == 19362644)
//		savegamedir = SavePathFreedoom08P1SD;

	    else if(fsize == 28422764)
		savegamedir = SavePathFreedoom08P2SD;
	}
	M_MakeDirectory(savegamedir);
    }

    return savegamedir;
}

