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


#ifdef SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "c_io.h"
#include "d_deh.h"
#include "doomfeatures.h"
#include "doomstat.h"
#include "hu_stuff.h"
#include "doomtype.h"
#include "i_sound.h"
#include "i_system.h"
#include "m_misc.h"
#include "m_random.h"
#include "p_local.h"
#include "p_setup.h"
#include "r_defs.h"
#include "s_sound.h"
#include "sounds.h"
#include "v_trans.h"
#include "w_wad.h"
#include "z_zone.h"


// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST (1200 << FRACBITS)

// Distance to origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// In the source code release: (160 * FRACUNIT).  Changed back to the 
// Vanilla value of 200 (why was this changed?)
#define S_CLOSE_DIST    (200 << FRACBITS)

// The range over which sound attenuates
#define S_ATTENUATOR    ((S_CLIPPING_DIST - S_CLOSE_DIST) >> FRACBITS)

// Stereo separation
#define S_STEREO_SWING  (96 << FRACBITS)

#define NORM_PRIORITY   64
#define NORM_SEP        128


// Internal volume level, ranging from 0-127
static int         snd_SfxVolume;

// Whether songs are mus_paused
static dboolean    mus_paused;        

// Music currently being played
static musicinfo_t *mus_playing = NULL;

// Number of channels to use
int                snd_channels = 8;

// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
int                sfxVolume = 8;

// Maximum volume of music. 
int                musicVolume = 8;

// The set of channels available
channel_t          channels[64];

dboolean           sound_warning_printed;


extern dboolean    fake;
extern dboolean    change_anyway;
extern dboolean    initialized;
extern dboolean    splashscreen;

extern int         faketracknum;
extern int         tracknum;


//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init(int sfxVolume, int musicVolume)
{  
    int i;

    I_InitSound(true);
    I_InitMusic();

    if (gameversion == exe_doom_1_666)
    {
        if (logical_gamemission == doom)
        {
            I_SetOPLDriverVer(opl_doom1_1_666);
        }
        else
        {
            I_SetOPLDriverVer(opl_doom2_1_666);
        }
    }
    else
    {
        I_SetOPLDriverVer(opl_doom_1_9);
    }

    I_PrecacheSounds(S_sfx, NUMSFX);

    S_SetSfxVolume(sfxVolume);
    S_SetMusicVolume(musicVolume);

    // Allocating the internal channels for mixing
    // (the maximum number of sounds rendered
    // simultaneously) within zone memory.
    //channels = Z_Malloc(snd_channels * sizeof(channel_t), PU_STATIC, NULL);

    // Free all channels for use
    for (i = 0; i < snd_channels; i++)
    {
        channels[i].sfxinfo = 0;
    }

    // no sounds are playing, and they are not mus_paused
    mus_paused = 0;

    // Note that sounds have not been cached (yet).
    for (i = 1; i < NUMSFX; i++)
    {
        S_sfx[i].lumpnum = S_sfx[i].usefulness = -1;
    }

    I_AtExit(S_Shutdown, true);
}

void S_Shutdown(void)
{
    I_ShutdownSound();
    I_ShutdownMusic();
}

static void S_StopChannel(int cnum)
{
    channel_t *c = &channels[cnum];

    if (c->sfxinfo)
    {
        int i;

        // stop the sound playing
        if (I_SoundIsPlaying(c->handle))
        {
            I_StopSound(c->handle);
        }

        // check to see if other channels are playing the sound
        for (i = 0; i < snd_channels; i++)
        {
            if (cnum != i && c->sfxinfo == channels[i].sfxinfo)
            {
                break;
            }
        }
        
        // degrade usefulness of sound data
        c->sfxinfo->usefulness--;
        c->sfxinfo = NULL;
        c->origin = NULL;
    }
}

void S_StopSounds(void)
{
    int cnum;

    // kill all playing sounds at start of level
    //  (trust me - a good idea)
    for (cnum = 0; cnum < snd_channels; cnum++)
        if (channels[cnum].sfxinfo)
            S_StopChannel(cnum);
}

static int S_GetMusicNum(void)
{
    static int mnum;

    if (gamemode == commercial && gamemission == pack_nerve)
    {
        int nmus[]=
        {
            mus_messag,
            mus_ddtblu,
            mus_doom,
            mus_shawn,
            mus_in_cit,
            mus_the_da,
            mus_in_cit,
            mus_shawn2,
            mus_ddtbl2
        };

        mnum = nmus[(s_randommusic ? M_RandomIntNoRepeat(1, 9, mnum) : gamemap) - 1];
    }
    else if (gamemode == commercial)
    {
        mnum = mus_runnin + (s_randommusic ? M_RandomIntNoRepeat(1, 32, mnum) : gamemap) - 1;
    }
    else
    {
        int spmus[]=
        {
            // Who? -       Where?
            // Song

            // American     e4m1
            mus_e3m4,

            // Romero       e4m2
            mus_e3m2,

            // Shawn        e4m3
            mus_e3m3,

            // American     e4m4
            mus_e1m5,

            // Tim          e4m5
            mus_e2m7,

            // Romero       e4m6
            mus_e2m4,

            // J.Anderson   e4m7 CHIRON.WAD
            mus_e2m6,

            // Shawn        e4m8
            mus_e2m5,

            // Tim          e4m9
            mus_e1m9
        };

        if (gameepisode < 4)
        {
            mnum = mus_e1m1 + (s_randommusic ? M_RandomIntNoRepeat(1, 21, mnum) :
                (gameepisode - 1) * 9 + gamemap) - 1;
        }
        else
        {
            mnum = spmus[(s_randommusic ? M_RandomIntNoRepeat(1, 28, mnum) : gamemap) - 1];
        }
    }

    return mnum;
//    S_ChangeMusic(mnum, true, true);
}        

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
    // kill all playing sounds at start of level
    //  (trust me - a good idea)
    S_StopSounds();

    // start new music for the level
    mus_paused = false;

    S_ChangeMusic(S_GetMusicNum(), !s_randommusic, true);
}

void S_StopSound(mobj_t *origin)
{
    int cnum;

    for (cnum = 0; cnum < snd_channels; cnum++)
    {
        if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
        {
            S_StopChannel(cnum);
            break;
        }
    }
}

//
// S_GetChannel :
//   If none available, return -1.  Otherwise channel #.
//
static int S_GetChannel(mobj_t *origin, sfxinfo_t *sfxinfo)
{
    // channel number to use
    int               cnum;
    
    channel_t         *c;

    // Find an open channel
    for (cnum = 0; cnum < snd_channels && channels[cnum].sfxinfo; ++cnum)
    {
        if (origin && channels[cnum].origin == origin
            && channels[cnum].sfxinfo->singularity == sfxinfo->singularity)
        {
            S_StopChannel(cnum);
            break;
        }
    }

    // None available
    if (cnum == snd_channels)
    {
        // Look for lower priority
        for (cnum = 0; cnum < snd_channels; ++cnum)
        {
            if (channels[cnum].sfxinfo->priority >= sfxinfo->priority)
            {
                break;
            }
        }

        if (cnum == snd_channels)
        {
            // FUCK!  No lower priority.  Sorry, Charlie.    
            return -1;
        }
        else
        {
            // Otherwise, kick out lower priority.
            S_StopChannel(cnum);
        }
    }

    c = &channels[cnum];

    // channel is decided to be cnum.
    c->sfxinfo = sfxinfo;
    c->origin = origin;

    return cnum;
}

//
// Changes volume and stereo-separation variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
static int S_AdjustSoundParams(mobj_t *listener, mobj_t *source, int *vol, int *sep)
{
    fixed_t     dist;
    fixed_t     adx;
    fixed_t     ady;
    angle_t     angle;

    if (!listener)
        return 0;

    // calculate the distance to sound origin
    //  and clip it if necessary
    // killough 11/98: scale coordinates down before calculations start
    // killough 12/98: use exact distance formula instead of approximation
    adx = ABS((listener->x >> FRACBITS) - (source->x >> FRACBITS));
    ady = ABS((listener->y >> FRACBITS) - (source->y >> FRACBITS));

    if (ady > adx)
    {
        dist = adx;
        adx = ady;
        ady = dist;
    }

    dist = (adx ? FixedDiv(adx, finesine[(tantoangle[FixedDiv(ady, adx) >> DBITS]
        + ANG90) >> ANGLETOFINESHIFT]) : 0);
    
    if (dist > (S_CLIPPING_DIST >> FRACBITS))
        return 0;

    // angle of source to listener
    angle = R_PointToAngle2(listener->x, listener->y, source->x, source->y);

    if (angle <= listener->angle)
        angle += 0xffffffff;

    angle -= listener->angle;
    angle >>= ANGLETOFINESHIFT;

    // stereo separation
    *sep = NORM_SEP - FixedMul(S_STEREO_SWING >> FRACBITS, finesine[angle]);

    // volume calculation
    if (dist < (S_CLOSE_DIST >> FRACBITS))
        *vol = snd_SfxVolume;
    else if (gamemap == 8)
    {
        if (dist > S_CLIPPING_DIST)
            dist = S_CLIPPING_DIST;

        *vol = 15 + ((snd_SfxVolume - 15) * ((S_CLIPPING_DIST - dist) >> FRACBITS))
            / S_ATTENUATOR;
    }
    else
        *vol = snd_SfxVolume * ((S_CLIPPING_DIST >> FRACBITS) - dist) / S_ATTENUATOR;

    return (*vol > 0);
}

static mobj_t *GetSoundListener(void)
{
    static degenmobj_t dummy_listener;

    // If we are at the title screen, the console player doesn't have an
    // object yet, so return a pointer to a static dummy listener instead.
    if (players[consoleplayer].mo != NULL)
    {
        return players[consoleplayer].mo;
    }
    else
    {
        dummy_listener.x = 0;
        dummy_listener.y = 0;
        dummy_listener.z = 0;

        return (mobj_t *)&dummy_listener;
    }
}

void S_StartSoundAtVolume(mobj_t *origin, int sfx_id, int pitch, int volume)
{
    sfxinfo_t   *sfx;
    mobj_t      *player = players[consoleplayer].mo;
    int         sep;
    int         cnum;
    int         handle;

    // check for bogus sound #
    if (sfx_id < 1 || sfx_id > NUMSFX)
    {
        C_Error("Bad sfx #: %d", sfx_id);
    }

    sfx = &S_sfx[sfx_id];

    // Initialize sound parameters
    if (sfx->link)
    {
        volume += sfx->volume;

        if (volume < 1)
            return;

        if (volume > snd_SfxVolume)
            volume = snd_SfxVolume;
    }


    // Check to see if it is audible,
    //  and if not, modify the params
    if (!origin || origin == player)
        sep = NORM_SEP;
    else if (!S_AdjustSoundParams(player, origin, &volume, &sep))
        return;
    else if (origin->x == player->x && origin->y == player->y)
        sep = NORM_SEP;

    // hacks to vary the sfx pitches
    if (sfx_id >= sfx_sawup && sfx_id <= sfx_sawhit)
    {
        pitch += 8 - (M_Random() & 15);
    }
    else if (sfx_id != sfx_itemup && sfx_id != sfx_tink)
    {
        pitch += 16 - (M_Random() & 31);
    }

    pitch = Clamp(pitch);

    // kill old sound
    if (d_sound)
    {
        for (cnum = 0; cnum < snd_channels; cnum++)
        {
            if (channels[cnum].sfxinfo && channels[cnum].sfxinfo->singularity == sfx->singularity
                && channels[cnum].origin == origin)
            {
                S_StopChannel(cnum);
                break;
            }
        }
    }
    else
        S_StopSound(origin);

    // try to find a channel
    cnum = S_GetChannel(origin, sfx);

    if (cnum < 0)
    {
        return;
    }

    // increase the usefulness
    if (sfx->usefulness++ < 0)
    {
        sfx->usefulness = 1;
    }

    // Get lumpnum if necessary
    // killough 2/28/98: make missing sounds non-fatal
    if (sfx->lumpnum < 0 && (sfx->lumpnum = I_GetSfxLumpNum(sfx)) < 0)
        return;

    // Assigns the handle to one of the channels in the
    //  mix/output buffer.
    // e6y: [Fix] Crash with zero-length sounds.
    if ((handle = I_StartSound(sfx, cnum, volume, sep, pitch)) != -1)
    {
        channels[cnum].handle = handle;
        channels[cnum].pitch = pitch;
    }

    if (!menuactive && devparm && !automapactive && sound_info && gamestate == GS_LEVEL && usergame)
    {
        mobj_t *listener = GetSoundListener();

        if (listener != NULL && origin != NULL)
        {
            int priority = S_sfx[sfx_id].priority;
            int absx = ABS(origin->x - listener->x);
            int absy = ABS(origin->y - listener->y);
            int dist = absx + absy - (absx > absy ? absy >> 1 : absx >> 1);

            dist >>= FRACBITS;
            priority *= (10 - (dist / 160));

            channels[cnum].sound_id = sfx_id;
            channels[cnum].priority = priority;
        }
    }
}        

void S_StartSound(mobj_t *mobj, int sfx_id)
{
    if (splashscreen && sfx_id == sfx_swtchn)
        return;

    S_StartSoundAtVolume(mobj, sfx_id, (mobj ? mobj->pitch : NORM_PITCH), snd_SfxVolume);
}

void S_StartSectorSound(degenmobj_t *degenmobj, int sfx_id)
{
    S_StartSoundAtVolume((mobj_t *)degenmobj, sfx_id, NORM_PITCH, snd_SfxVolume);
}

//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound(void)
{
    if (mus_playing && !mus_paused)
    {
        I_PauseSong();
        mus_paused = true;
    }
}

void S_ResumeSound(void)
{
    if (mus_playing && mus_paused)
    {
        I_ResumeSong();
        mus_paused = false;
    }
}

//
// Updates music & sounds
//
void S_UpdateSounds(mobj_t *listener)
{
    int                cnum;

    I_UpdateSound();

    for (cnum = 0; cnum < snd_channels; ++cnum)
    {
        channel_t       *c = &channels[cnum];
        sfxinfo_t       *sfx = c->sfxinfo;

        if (sfx)
        {
            if (I_SoundIsPlaying(c->handle))
            {
                // initialize parameters
                int volume = snd_SfxVolume;
                int sep = NORM_SEP;

                if (sfx->link)
                {
                    volume += sfx->volume;

                    if (volume < 1)
                    {
                        S_StopChannel(cnum);
                        continue;
                    }
                    else if (volume > snd_SfxVolume)
                    {
                        volume = snd_SfxVolume;
                    }
                }

                // check non-local sounds for distance clipping
                //  or modify their params
                if (c->origin && listener != c->origin)
                {
                    if (!S_AdjustSoundParams(listener, c->origin, &volume, &sep))
                    {
                        S_StopChannel(cnum);
                    }
                    else
                    {
                        I_UpdateSoundParams(c->handle, volume, sep);
                    }
                }
            }
            else
            {
                // if channel is allocated but sound has stopped,
                //  free it
                S_StopChannel(cnum);
            }
        }
    }

    if (s_randommusic && !I_MusicIsPlaying())
        S_ChangeMusic(S_GetMusicNum(), false, false);
}

void S_SetMusicVolume(int volume)
{
    if (volume < 0 || volume > 127)
    {
        I_Error("Attempt to set music volume at %d",
                volume);
    }    

    I_SetMusicVolume(volume);
}

void S_SetSfxVolume(int volume)
{
    if (volume < 0 || volume > 127)
    {
        I_Error("Attempt to set sfx volume at %d", volume);
    }

    snd_SfxVolume = volume;
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(int m_id)
{
    S_ChangeMusic(m_id, false, false);
}

void S_ChangeMusic(int musicnum, int looping, dboolean mapstart)
{
    musicinfo_t *music = NULL;
    void        *handle = NULL;
    int         mapinfomusic; 
    int         gameepi;

    if (!initialized || splashscreen)
        return;

    // The Doom IWAD file has two versions of the intro music: d_intro
    // and d_introa.  The latter is used for OPL playback.
    if (musicnum == mus_intro && (snd_musicdevice == SNDDEVICE_ADLIB
                               || snd_musicdevice == SNDDEVICE_SB))
    {
        // HACK: NOT FOR SHARE 1.0 & 1.1 & REG 1.1
        if (fsize != 4207819 && fsize != 4274218 && fsize != 10396254)
            musicnum = mus_introa;
        // HACK: IF SHAREWARE 1.0 OR 1.1
        else
            musicnum = mus_intro;
    }

    if (musicnum <= mus_None || musicnum >= NUMMUSIC)
    {
        C_Warning("Bad music number %d", musicnum);
        return;
    }
    else
    {
        music = &S_music[musicnum];
    }

    if (mus_playing == music && !change_anyway)
    {
        return;
    }

    change_anyway = false;

    // shutdown old music
    S_StopMusic();

    // get lumpnum if neccessary
    if (gamemode == commercial)
        gameepi = gameepisode - 1;
    else
        gameepi = gameepisode;

    if (mapstart && (mapinfomusic = P_GetMapMusic((gameepi - 1) * 10 + gamemap)) > 0) 
        music->lumpnum = mapinfomusic;
    else if (!music->lumpnum)
    {
        char namebuf[9];

        M_snprintf(namebuf, sizeof(namebuf), "d_%s", music->name);
        music->lumpnum = W_GetNumForName(namebuf);
    }

    if (music->lumpnum != -1)
    {
        // Load & register it
        music->data = W_CacheLumpNum(music->lumpnum, PU_STATIC);
        handle = I_RegisterSong(music->data, W_LumpLength(music->lumpnum));
    }

    if (!handle && !sound_warning_printed)
    {
        C_Warning("D_%s music lump can't be played.", uppercase(music->name));
        C_Warning("Maybe you forgot running the game with the 'sudo' command");
        sound_warning_printed = true;
        return;
    }

    music->handle = handle;

    I_PlaySong(handle, looping);

    mus_playing = music;
}

//
// [nitr8] UNUSED
//
/*
dboolean S_MusicPlaying(void)
{
    return I_MusicIsPlaying();
}
*/

void S_StopMusic(void)
{
    if (mus_playing)
    {
        if (mus_paused)
        {
            I_ResumeSong();
        }

        I_StopSong();
        I_UnRegisterSong(mus_playing->handle);
        W_ReleaseLumpNum(mus_playing->lumpnum);
        mus_playing->data = NULL;
        mus_playing = NULL;
    }
}

