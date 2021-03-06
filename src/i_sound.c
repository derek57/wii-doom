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
// DESCRIPTION:  none
//


#ifdef SDL2
#include <SDL2/SDL_mixer.h>
#else
#include <SDL/SDL_mixer.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "c_io.h"
#include "doomfeatures.h"
#include "doomtype.h"
#include "i_sound.h"
#include "i_video.h"
#include "m_config.h"

#include "doom/s_sound.h"

#include "v_trans.h"
#include "wii-doom.h"


int                      snd_musicdevice = SNDDEVICE_SB;
int                      snd_sfxdevice = SNDDEVICE_SB;

// Sound sample rate to use for digital output (Hz)
int                      snd_samplerate = 44100;

// Maximum number of bytes to dedicate to allocated sound effects.
// (Default: 64MB)
int                      snd_cachesize = 64 * 1024 * 1024;

// Config variable that controls the sound buffer size.
// We default to 28ms (1000 / 35fps = 1 buffer per tic).
int                      snd_maxslicetime_ms = 28;

// External command to invoke to play back music.
char                     *snd_musiccmd = "";

// Low-level sound and music modules we are using
static sound_module_t    *sound_module;
static music_module_t    *music_module;

extern sound_module_t    sound_sdl_module;
extern sound_module_t    sound_pcsound_module;

extern music_module_t    music_sdl_module;
extern music_module_t    music_opl_module;

// For OPL module:
extern int               opl_io_port;

// For native music module:
extern char              *timidity_cfg_path;


// Compiled-in sound modules:
static sound_module_t    *sound_modules[] = 
{
#ifdef FEATURE_SOUND
    &sound_sdl_module,
    &sound_pcsound_module
#endif
    ,
    NULL
};

// Compiled-in music modules:
static music_module_t    *music_modules[] =
{
#ifdef FEATURE_SOUND
    &music_sdl_module,
    &music_opl_module
#endif
    ,
    NULL
};


// Sound modules
dboolean I_PCS_InitSound(dboolean _use_sfx_prefix);
dboolean I_SDL_InitSound(dboolean _use_sfx_prefix);


extern void I_InitTimidityConfig(void);


// Check if a sound device is in the given list of devices
static dboolean SndDeviceInList(snddevice_t device, snddevice_t *list,
                               int len)
{
    int i;

    for (i = 0; i < len; ++i)
    {
        if (device == list[i])
        {
            return true;
        }
    }

    return false;
}

// Find and initialize a sound_module_t appropriate for the setting
// in snd_sfxdevice.
static void InitSfxModule(dboolean use_sfx_prefix)
{
    if (snd_module)
        I_PCS_InitSound(use_sfx_prefix);
    else
        I_SDL_InitSound(use_sfx_prefix);

    sound_module = sound_modules[snd_module];

    C_Output("SFX playing at a sample rate of %.1fkHz on %i channels using %s module.",
            snd_samplerate / 1000.0f, snd_channels, snd_module ? "PC-SPEAKER" : "SDL");
}

// Initialize music according to snd_musicdevice.
static void InitMusicModule(void)
{
    int i;

    music_module = NULL;

    for (i = 0; music_modules[i] != NULL; ++i)
    {
        // Is the music device in the list of devices supported
        // by this module?
        if (SndDeviceInList(snd_musicdevice, 
                            music_modules[i]->sound_devices,
                            music_modules[i]->num_sound_devices))
        {
            // Initialize the module
            if (music_modules[i]->Init())
            {
                if (mus_engine == 1 || mus_engine == 2)
                    C_Output("Using MIDI playback for music.");
                else if (mus_engine == 3)
                    C_Output("Using OGG playback for music.");
                else if (mus_engine == 4)
                    C_Output("Using TIMIDITY for music playback.");

                music_module = music_modules[i];
                return;
            }
        }
    }
}

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void I_InitSound(dboolean use_sfx_prefix)
{  
    // Initialize the sound subsystem.
#ifndef WII
    if (!screensaver_mode)
#endif
        InitSfxModule(use_sfx_prefix);
}

void I_ShutdownSound(void)
{
    if (sound_module != NULL)
    {
        sound_module->Shutdown();
    }
}

int I_GetSfxLumpNum(sfxinfo_t *sfxinfo)
{
    if (sound_module != NULL) 
    {
        return sound_module->GetSfxLumpNum(sfxinfo);
    }
    else
    {
        return 0;
    }
}

void I_UpdateSound(void)
{
    if (sound_module != NULL)
    {
        sound_module->Update();
    }

    if (music_module != NULL && music_module->Poll != NULL)
    {
        music_module->Poll();
    }
}

static void CheckVolumeSeparation(int *vol, int *sep)
{
    if (*sep < 0)
    {
        *sep = 0;
    }
    else if (*sep > 254)
    {
        *sep = 254;
    }

    if (*vol < 0)
    {
        *vol = 0;
    }
    else if (*vol > 127)
    {
        *vol = 127;
    }
}

void I_UpdateSoundParams(int channel, int vol, int sep)
{
    if (sound_module != NULL)
    {
        CheckVolumeSeparation(&vol, &sep);
        sound_module->UpdateSoundParams(channel, vol, sep);
    }
}

int I_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep, int pitch)
{
    if (sound_module != NULL)
    {
        CheckVolumeSeparation(&vol, &sep);
        return sound_module->StartSound(sfxinfo, channel, vol, sep, pitch);
    }
    else
    {
        return 0;
    }
}

void I_StopSound(int channel)
{
    if (sound_module != NULL)
    {
        sound_module->StopSound(channel);
    }
}

dboolean I_SoundIsPlaying(int channel)
{
    if (sound_module != NULL)
    {
        return sound_module->SoundIsPlaying(channel);
    }
    else
    {
        return false;
    }
}

void I_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
    if (sound_module != NULL && sound_module->CacheSounds != NULL)
    {
        sound_module->CacheSounds(sounds, num_sounds);
    }
}

void I_InitMusic(void)
{
    // Initialize the music subsystems.

    // This is kind of a hack. If native MIDI is enabled, set up
    // the TIMIDITY_CFG environment variable here before SDL_mixer
    // is opened.
    if ((snd_musicdevice == SNDDEVICE_GENMIDI || snd_musicdevice == SNDDEVICE_GUS))
    {
        I_InitTimidityConfig();
    }

    InitMusicModule();
}

void I_ShutdownMusic(void)
{
    if (music_module != NULL)
    {
        music_module->Shutdown();
    }
}

void I_SetMusicVolume(int volume)
{
    if (music_module != NULL)
    {
        music_module->SetMusicVolume(volume);
    }
}

void I_PauseSong(void)
{
    if (music_module != NULL)
    {
        music_module->PauseMusic();
    }
}

void I_ResumeSong(void)
{
    if (music_module != NULL)
    {
        music_module->ResumeMusic();
    }
}

void *I_RegisterSong(void *data, int len)
{
    if (music_module != NULL)
    {
        return music_module->RegisterSong(data, len);
    }
    else
    {
        return NULL;
    }
}

void I_UnRegisterSong(void *handle)
{
    if (music_module != NULL)
    {
        music_module->UnRegisterSong(handle);
    }
}

void I_PlaySong(void *handle, dboolean looping)
{
    if (music_module != NULL)
    {
        music_module->PlaySong(handle, looping);
    }
}

void I_StopSong(void)
{
    if (music_module != NULL)
    {
        music_module->StopSong();
    }
}

dboolean I_MusicIsPlaying(void)
{
    if (music_module != NULL)
    {
        return music_module->MusicIsPlaying();
    }
    else
    {
        return false;
    }
}

//
// [nitr8] UNUSED
//
/*
void I_BindSoundVariables(void)
{
}
*/

