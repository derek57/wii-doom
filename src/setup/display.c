//
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


#include <stdlib.h>
#include <string.h>

#ifdef SDL2
#include <SDL2/SDL.h>
#endif

#include "textscreen.h"
#include "m_config.h"
#include "m_misc.h"
#include "mode.h"
#include "display.h"
#include "config.h"
#include "wii-doom.h"


#define WINDOW_HELP_URL "https://www.chocolate-doom.org/setup-display"


static char *video_driver = "";

static int system_video_env_set;


// Set the SDL_VIDEODRIVER environment variable
void SetDisplayDriver(void)
{
    static int first_time = 1;

    if (first_time)
    {
        system_video_env_set = getenv("SDL_VIDEODRIVER") != NULL;

        first_time = 0;
    }
    
    // Don't override the command line environment, if it has been set.
    if (system_video_env_set)
    {
        return;
    }

    // Use the value from the configuration file, if it has been set.
    if (strcmp(video_driver, "") != 0)
    {
        char *env_string;

        env_string = M_StringJoin("SDL_VIDEODRIVER=", video_driver, NULL);
        putenv(env_string);
        free(env_string);
    }
}

// FIXME
void BindDisplayVariables(void)
{
    M_BindIntVariable("autoadjust_video_settings", &autoadjust_video_settings);
    M_BindIntVariable("aspect_ratio_correct",      &aspect_ratio_correct);
    M_BindIntVariable("fullscreen",                &fullscreen);
    M_BindIntVariable("screenwidth",               &screen_width);
    M_BindIntVariable("screenheight",              &screen_height);
    M_BindIntVariable("screen_bpp",                &screen_bpp);
    M_BindIntVariable("startup_delay",             &startup_delay);
    M_BindStringVariable("video_driver",           &video_driver);
    M_BindStringVariable("window_position",        &window_position);
    M_BindIntVariable("use_gamma",                  &usegamma);
    M_BindBooleanVariable("png_screenshot",           &png_screenshots);


    if (gamemission == doom)
    {
        M_BindBooleanVariable("endoom_screen",               &show_endoom);
    }

    if (gamemission == doom)
    {
        M_BindBooleanVariable("diskicon",             &show_diskicon);
    }
}

