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

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include <stdio.h>
#include <stdlib.h>

#include "i_sound.h"
#include "i_system.h"

#include "doomfeatures.h"
#include "deh_str.h"

#include "doomstat.h"
#include "doomtype.h"

#include "sounds.h"
#include "s_sound.h"

#include "m_random.h"
#include "m_argv.h"

#include "p_local.h"
#include "w_wad.h"
#include "z_zone.h"

#include "i_oplmusic.h"

// when to clip out sounds
// Does not fit the large outdoor areas.

#define S_CLIPPING_DIST (1200 * FRACUNIT)

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// In the source code release: (160*FRACUNIT).  Changed back to the 
// Vanilla value of 200 (why was this changed?)

#define S_CLOSE_DIST (200 * FRACUNIT)

// The range over which sound attenuates

#define S_ATTENUATOR ((S_CLIPPING_DIST - S_CLOSE_DIST) >> FRACBITS)

// Stereo separation

#define S_STEREO_SWING (96 * FRACUNIT)

#define NORM_PITCH 128
#define NORM_PRIORITY 64
#define NORM_SEP 128

int musicPlaying = 0;            //Is the music playing, or not?

short songlist[148];
short currentsong = 0;

extern boolean opl;
extern boolean forced;
extern boolean fake;

extern int faketracknum;
extern int tracknum;

typedef struct
{
    // sound information (if null, channel avail.)
    sfxinfo_t *sfxinfo;

    // origin of sound
    mobj_t *origin;

    // handle of the sound being played
    int handle;
    
} channel_t;

// The set of channels available

static channel_t *channels;

// Maximum volume of a sound effect.
// Internal default is max out of 0-15.

int sfxVolume = 8;

// Maximum volume of music. 

int musicVolume = 8;

// Internal volume level, ranging from 0-127

static int snd_SfxVolume;

// Whether songs are mus_paused

static boolean mus_paused;        

// Music currently being played

static musicinfo_t *mus_playing = NULL;

// Number of channels to use

int snd_channels = 8;

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

    I_PrecacheSounds(S_sfx, NUMSFX);

    S_SetSfxVolume(sfxVolume);
    S_SetMusicVolume(musicVolume);

    // Allocating the internal channels for mixing
    // (the maximum numer of sounds rendered
    // simultaneously) within zone memory.
    channels = Z_Malloc(snd_channels*sizeof(channel_t), PU_STATIC, 0);

    // Free all channels for use
    for (i=0 ; i<snd_channels ; i++)
    {
        channels[i].sfxinfo = 0;
    }

    // no sounds are playing, and they are not mus_paused
    mus_paused = 0;

    // Note that sounds have not been cached (yet).
    for (i=1 ; i<NUMSFX ; i++)
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
    int i;
    channel_t *c;

    c = &channels[cnum];

    if (c->sfxinfo)
    {
        // stop the sound playing

        if (I_SoundIsPlaying(c->handle))
        {
            I_StopSound(c->handle);
        }

        // check to see if other channels are playing the sound

        for (i=0; i<snd_channels; i++)
        {
            if (cnum != i && c->sfxinfo == channels[i].sfxinfo)
            {
                break;
            }
        }
        
        // degrade usefulness of sound data

        c->sfxinfo->usefulness--;
        c->sfxinfo = NULL;
    }
}

//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//

void S_Start(void)
{
    int cnum;
    int mnum;

    // kill all playing sounds at start of level
    //  (trust me - a good idea)
    for (cnum=0 ; cnum<snd_channels ; cnum++)
    {
        if (channels[cnum].sfxinfo)
        {
            S_StopChannel(cnum);
        }
    }

    // start new music for the level
    mus_paused = 0;

    if (gamemode == commercial)
    {
        mnum = mus_runnin + gamemap - 1;
    }
    else
    {
        int spmus[]=
        {
            // Song - Who? - Where?

            mus_e3m4,        // American     e4m1
            mus_e3m2,        // Romero       e4m2
            mus_e3m3,        // Shawn        e4m3
            mus_e1m5,        // American     e4m4
            mus_e2m7,        // Tim          e4m5
            mus_e2m4,        // Romero       e4m6
            mus_e2m6,        // J.Anderson   e4m7 CHIRON.WAD
            mus_e2m5,        // Shawn        e4m8
            mus_e1m9,        // Tim          e4m9
        };

        if (gameepisode < 4)
        {
            mnum = mus_e1m1 + (gameepisode-1)*9 + gamemap-1;
        }
        else
        {
            mnum = spmus[gamemap-1];
        }
    }        

#ifdef OGG_SUPPORT
    if(opl)
#endif
	S_ChangeMusic(mnum, true);
#ifdef OGG_SUPPORT
    else
	S_StartMP3Music(0, -1); 
#endif
}        

void S_StopSound(mobj_t *origin)
{
    int cnum;

    for (cnum=0 ; cnum<snd_channels ; cnum++)
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
    int                cnum;
    
    channel_t*        c;

    // Find an open channel
    for (cnum=0 ; cnum<snd_channels ; cnum++)
    {
        if (!channels[cnum].sfxinfo)
        {
            break;
        }
        else if (origin && channels[cnum].origin == origin)
        {
            S_StopChannel(cnum);
            break;
        }
    }

    // None available
    if (cnum == snd_channels)
    {
        // Look for lower priority
        for (cnum=0 ; cnum<snd_channels ; cnum++)
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

static int S_AdjustSoundParams(mobj_t *listener, mobj_t *source,
                               int *vol, int *sep)
{
    fixed_t        approx_dist;
    fixed_t        adx;
    fixed_t        ady;
    angle_t        angle;

    // calculate the distance to sound origin
    //  and clip it if necessary
    adx = abs(listener->x - source->x);
    ady = abs(listener->y - source->y);

    // From _GG1_ p.428. Appox. eucledian distance fast.
    approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);
    
    if (gamemap != 8 && approx_dist > S_CLIPPING_DIST)
    {
        return 0;
    }
    
    // angle of source to listener
    angle = R_PointToAngle2(listener->x,
                            listener->y,
                            source->x,
                            source->y);

    if (angle > listener->angle)
    {
        angle = angle - listener->angle;
    }
    else
    {
        angle = angle + (0xffffffff - listener->angle);
    }

    angle >>= ANGLETOFINESHIFT;

    // stereo separation
    *sep = 128 - (FixedMul(S_STEREO_SWING, finesine[angle]) >> FRACBITS);

    // volume calculation
    if (approx_dist < S_CLOSE_DIST)
    {
        *vol = snd_SfxVolume;
    }
    else if (gamemap == 8)
    {
        if (approx_dist > S_CLIPPING_DIST)
        {
            approx_dist = S_CLIPPING_DIST;
        }

        *vol = 15+ ((snd_SfxVolume-15)
                    *((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
            / S_ATTENUATOR;
    }
    else
    {
        // distance effect
        *vol = (snd_SfxVolume
                * ((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
            / S_ATTENUATOR; 
    }
    
    return (*vol > 0);
}

void S_StartSound(void *origin_p, int sfx_id)
{
    sfxinfo_t *sfx;
    mobj_t *origin;
    int rc;
    int sep;
    int cnum;
    int volume;

    origin = (mobj_t *) origin_p;
    volume = snd_SfxVolume;

    // check for bogus sound #
    if (sfx_id < 1 || sfx_id > NUMSFX)
    {
        I_Error("Bad sfx #: %d", sfx_id);
    }

    sfx = &S_sfx[sfx_id];

    // Initialize sound parameters
    if (sfx->link)
    {
        volume += sfx->volume;

        if (volume < 1)
        {
            return;
        }

        if (volume > snd_SfxVolume)
        {
            volume = snd_SfxVolume;
        }
    }


    // Check to see if it is audible,
    //  and if not, modify the params
    if (origin && origin != players[consoleplayer].mo)
    {
        rc = S_AdjustSoundParams(players[consoleplayer].mo,
                                 origin,
                                 &volume,
                                 &sep);

        if (origin->x == players[consoleplayer].mo->x
         && origin->y == players[consoleplayer].mo->y)
        {        
            sep = NORM_SEP;
        }

        if (!rc)
        {
            return;
        }
    }        
    else
    {
        sep = NORM_SEP;
    }

    // kill old sound
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

    if (sfx->lumpnum < 0)
    {
        sfx->lumpnum = I_GetSfxLumpNum(sfx);
    }

    channels[cnum].handle = I_StartSound(sfx, cnum, volume, sep);
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
    int                audible;
    int                cnum;
    int                volume;
    int                sep;
    sfxinfo_t*        sfx;
    channel_t*        c;

    for (cnum=0; cnum<snd_channels; cnum++)
    {
        c = &channels[cnum];
        sfx = c->sfxinfo;

        if (c->sfxinfo)
        {
            if (I_SoundIsPlaying(c->handle))
            {
                // initialize parameters
                volume = snd_SfxVolume;
                sep = NORM_SEP;

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
                    audible = S_AdjustSoundParams(listener,
                                                  c->origin,
                                                  &volume,
                                                  &sep);
                    
                    if (!audible)
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
    S_ChangeMusic(m_id, false);
}

void S_ChangeMusic(int musicnum, int looping)
{
    musicinfo_t *music = NULL;
    char namebuf[9];
    void *handle;

    // The Doom IWAD file has two versions of the intro music: d_intro
    // and d_introa.  The latter is used for OPL playback.

    if (musicnum == mus_intro && (snd_musicdevice == SNDDEVICE_ADLIB
                               || snd_musicdevice == SNDDEVICE_SB))
    {
	if(fsize != 4207819 && fsize != 4274218 && fsize != 10396254)	// HACK: NOT FOR SHARE 1.0 & 1.1 && REG 1.1
	    musicnum = mus_introa;
	else						// HACK: IF SHAREWARE 1.0 OR 1.1
	    musicnum = mus_intro;
    }

    if (musicnum <= mus_None || musicnum >= NUMMUSIC)
    {
        I_Error("Bad music number %d", musicnum);
    }
    else
    {
        music = &S_music[musicnum];
    }

    if (mus_playing == music)
    {
        return;
    }

    // shutdown old music
    S_StopMusic();

    // get lumpnum if neccessary
    if (!music->lumpnum)
    {
        sprintf(namebuf, "d_%s", DEH_String(music->name));
        music->lumpnum = W_GetNumForName(namebuf);
    }

    music->data = W_CacheLumpNum(music->lumpnum, PU_STATIC);

    handle = I_RegisterSong(music->data, W_LumpLength(music->lumpnum));
    music->handle = handle;

#ifdef OGG_SUPPORT
    if(opl)
#endif
	I_PlaySong(handle, looping);
#ifdef OGG_SUPPORT
    else
	S_StartMP3Music(0, -1);
#endif

    mus_playing = music;
}

boolean S_MusicPlaying(void)
{
    return I_MusicIsPlaying();
}

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

#ifdef OGG_SUPPORT
short S_GetLevelSongNum(int episode, int map)
{
    if(episode < 1 || map < 1)
    {
	return 0;
    }
    short songnum = 0;

    if(gamemission == doom)
        songnum = songlist[9*(episode-1)+(map-1)];
    else if(gamemission == pack_tnt)
        songnum = songlist[76+(map-1)];
    else if(gamemission == pack_plut)
        songnum = songlist[112+(map-1)];
    else if(gamemission == doom2)
        songnum = songlist[40+(map-1)];

    return songnum;
}

void musicFinished()
{
   //Music is done!
   musicPlaying = 0;
}

void SDL_InitOGG(void)
{
    int audio_rate = 22050;			//Frequency of audio playback
    int audio_channels = 2;			//2 channels = stereo
    int audio_buffers = 4096;			//Size of the audio buffers in memory

    Uint16 audio_format = AUDIO_S16MSB; 	//Format of the audio we're playing

    songlist[0] = 1, // DOOM: E1M1
    songlist[1] = 2; // DOOM: E1M2
    songlist[2] = 3; // DOOM: E1M3
    songlist[3] = 4; // DOOM: E1M4
    songlist[4] = 5; // DOOM: E1M5
    songlist[5] = 6; // DOOM: E1M6
    songlist[6] = 7; // DOOM: E1M7
    songlist[7] = 8; // DOOM: E1M8
    songlist[8] = 9; // DOOM: E1M9
    songlist[9] = 10; // DOOM: E2M1
    songlist[10] = 11; // DOOM: E2M2
    songlist[11] = 12; // DOOM: E2M3
    songlist[12] = 13; // DOOM: E2M4
    songlist[13] = 7; // DOOM: E2M5
    songlist[14] = 14; // DOOM: E2M6
    songlist[15] = 15; // DOOM: E2M7
    songlist[16] = 16; // DOOM: E2M8
    songlist[17] = 17; // DOOM: E2M9
    songlist[18] = 17; // DOOM: E3M1
    songlist[19] = 18; // DOOM: E3M2
    songlist[20] = 19; // DOOM: E3M3
    songlist[21] = 8; // DOOM: E3M4
    songlist[22] = 7; // DOOM: E3M5
    songlist[23] = 6; // DOOM: E3M6
    songlist[24] = 15; // DOOM: E3M7
    songlist[25] = 20; // DOOM: E3M8
    songlist[26] = 9; // DOOM: E3M9
    songlist[27] = 8; // DOOM: E4M1
    songlist[28] = 18; // DOOM: E4M2
    songlist[29] = 19; // DOOM: E4M3
    songlist[30] = 5; // DOOM: E4M4
    songlist[31] = 15; // DOOM: E4M5
    songlist[32] = 13; // DOOM: E4M6
    songlist[33] = 14; // DOOM: E4M7
    songlist[34] = 7; // DOOM: E4M8
    songlist[35] = 9; // DOOM: E4M9
    songlist[36] = 12; // DOOM: End of level screen
    songlist[37] = 21; // DOOM: Title screen
    songlist[38] = 22; // DOOM: Intermission screen
    songlist[39] = 23; // DOOM: End of game screen
    songlist[40] = 24; // DOOM 2: MAP01
    songlist[41] = 25; // DOOM 2: MAP02
    songlist[42] = 26; // DOOM 2: MAP03
    songlist[43] = 27; // DOOM 2: MAP04
    songlist[44] = 28; // DOOM 2: MAP05
    songlist[45] = 29; // DOOM 2: MAP06
    songlist[46] = 30; // DOOM 2: MAP07
    songlist[47] = 31; // DOOM 2: MAP08
    songlist[48] = 32; // DOOM 2: MAP09
    songlist[49] = 33; // DOOM 2: MAP10
    songlist[50] = 25; // DOOM 2: MAP11
    songlist[51] = 29; // DOOM 2: MAP12
    songlist[52] = 28; // DOOM 2: MAP13
    songlist[53] = 31; // DOOM 2: MAP14
    songlist[54] = 24; // DOOM 2: MAP15
    songlist[55] = 33; // DOOM 2: MAP16
    songlist[56] = 25; // DOOM 2: MAP17
    songlist[57] = 34; // DOOM 2: MAP18
    songlist[58] = 30; // DOOM 2: MAP19
    songlist[59] = 35; // DOOM 2: MAP20
    songlist[60] = 26; // DOOM 2: MAP21
    songlist[61] = 31; // DOOM 2: MAP22
    songlist[62] = 36; // DOOM 2: MAP23
    songlist[63] = 29; // DOOM 2: MAP24
    songlist[64] = 37; // DOOM 2: MAP25
    songlist[65] = 35; // DOOM 2: MAP26
    songlist[66] = 34; // DOOM 2: MAP27
    songlist[67] = 38; // DOOM 2: MAP28
    songlist[68] = 30; // DOOM 2: MAP29
    songlist[69] = 39; // DOOM 2: MAP30
    songlist[70] = 40; // DOOM 2: MAP31
    songlist[71] = 41; // DOOM 2: MAP32
    songlist[72] = 42; // DOOM 2: End of level screen
    songlist[73] = 43; // DOOM 2: Title screen
    songlist[74] = 44; // DOOM 2: Intermission screen
    songlist[75] = 40; // DOOM 2: End of game screen
    songlist[76] = 45; // TNT: MAP01
    songlist[77] = 46; // TNT: MAP02
    songlist[78] = 35; // TNT: MAP03
    songlist[79] = 47; // TNT: MAP04
    songlist[80] = 48; // TNT: MAP05
    songlist[81] = 49; // TNT: MAP06
    songlist[82] = 50; // TNT: MAP07
    songlist[83] = 51; // TNT: MAP08
    songlist[84] = 45; // TNT: MAP09
    songlist[85] = 52; // TNT: MAP10
    songlist[86] = 53; // TNT: MAP11
    songlist[87] = 31; // TNT: MAP12
    songlist[88] = 47; // TNT: MAP13
    songlist[89] = 54; // TNT: MAP14
    songlist[90] = 46; // TNT: MAP15
    songlist[91] = 55; // TNT: MAP16
    songlist[92] = 48; // TNT: MAP17
    songlist[93] = 52; // TNT: MAP18
    songlist[94] = 26; // TNT: MAP19
    songlist[95] = 56; // TNT: MAP20
    songlist[96] = 32; // TNT: MAP21
    songlist[97] = 57; // TNT: MAP22
    songlist[98] = 36; // TNT: MAP23
    songlist[99] = 27; // TNT: MAP24
    songlist[100] = 28; // TNT: MAP25
    songlist[101] = 55; // TNT: MAP26
    songlist[102] = 51; // TNT: MAP27
    songlist[103] = 57; // TNT: MAP28
    songlist[104] = 47; // TNT: MAP29
    songlist[105] = 51; // TNT: MAP30
    songlist[106] = 58; // TNT: MAP31
    songlist[107] = 32; // TNT: MAP32
    songlist[108] = 58; // TNT: End of level screen
    songlist[109] = 59; // TNT: Title screen
    songlist[110] = 60; // TNT: Intermission screen
    songlist[111] = 58; // TNT: End of game screen
    songlist[112] = 2; // Plutonia: MAP01
    songlist[113] = 3; // Plutonia: MAP02
    songlist[114] = 6; // Plutonia: MAP03
    songlist[115] = 4; // Plutonia: MAP04
    songlist[116] = 9; // Plutonia: MAP05
    songlist[117] = 8; // Plutonia: MAP06
    songlist[118] = 10; // Plutonia: MAP07
    songlist[119] = 11; // Plutonia: MAP08
    songlist[120] = 19; // Plutonia: MAP09
    songlist[121] = 7; // Plutonia: MAP10
    songlist[122] = 23; // Plutonia: MAP11
    songlist[123] = 20; // Plutonia: MAP12
    songlist[124] = 18; // Plutonia: MAP13
    songlist[125] = 16; // Plutonia: MAP14
    songlist[126] = 15; // Plutonia: MAP15
    songlist[127] = 17; // Plutonia: MAP16
    songlist[128] = 1; // Plutonia: MAP17
    songlist[129] = 7; // Plutonia: MAP18
    songlist[130] = 5; // Plutonia: MAP19
    songlist[131] = 35; // Plutonia: MAP20
    songlist[132] = 44; // Plutonia: MAP21
    songlist[133] = 31; // Plutonia: MAP22
    songlist[134] = 36; // Plutonia: MAP23
    songlist[135] = 29; // Plutonia: MAP24
    songlist[136] = 37; // Plutonia: MAP25
    songlist[137] = 35; // Plutonia: MAP26
    songlist[138] = 10; // Plutonia: MAP27
    songlist[139] = 11; // Plutonia: MAP28
    songlist[140] = 1; // Plutonia: MAP29
    songlist[141] = 21; // Plutonia: MAP30
    songlist[142] = 8; // Plutonia: MAP31
    songlist[143] = 16; // Plutonia: MAP32
    songlist[144] = 42; // Plutonia: End of level screen
    songlist[145] = 43; // Plutonia: Title screen
    songlist[146] = 44; // Plutonia: Intermission screen
    songlist[147] = 8; // Plutonia: End of game screen

    //Initialize SDL audio
    if (SDL_Init(SDL_INIT_AUDIO) != 0) 
    {
	printf("Unable to initialize SDL: %s\n", SDL_GetError());
    }
	
    //Initialize SDL_mixer with our chosen audio settings
    if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) 
    {
	printf("Unable to initialize audio: %s\n", Mix_GetError());
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-zero-length"	// ADDED FOR THE MP3 MUSIC SWITCHING BUG

void S_StartMP3Music(int type, int mode)
{
    int modex = 0;

    if(mode == -1)
	modex = 0 - 1;
    else
	modex = mode;

    char path[20];

    I_OPL_ShutdownMusic();

    Mix_Music *music;

    if(type == 0)
    {

        if(gameepisode < 1 || gamemap < 1)
            return;

        currentsong = S_GetLevelSongNum(gameepisode, gamemap);
    }
    else if(type == 1)
    {
        if(gamemission == doom)
            currentsong = songlist[36];
        else if(gamemission == pack_tnt)
            currentsong = songlist[108];
        else if(gamemission == pack_plut)
            currentsong = songlist[144];
        else if(gamemission == doom2)
            currentsong = songlist[72];
    }
    else if(type == 2)
    {
        if(gamemission == doom)
            currentsong = songlist[37];
        else if(gamemission == pack_tnt)
            currentsong = songlist[109];
        else if(gamemission == pack_plut)
            currentsong = songlist[145];
        else if(gamemission == doom2)
            currentsong = songlist[73];
    }
    else if(type == 3)
    {
        if(gamemission == doom)
            currentsong = songlist[38];
        else if(gamemission == pack_tnt)
            currentsong = songlist[110];
        else if(gamemission == pack_plut)
            currentsong = songlist[146];
        else if(gamemission == doom2)
            currentsong = songlist[74];
    }
    else if(type == 4)
    {
        if(gamemission == doom)
            currentsong = songlist[39];
        else if(gamemission == pack_tnt)
            currentsong = songlist[111];
        else if(gamemission == pack_plut)
            currentsong = songlist[147];
        else if(gamemission == doom2)
            currentsong = songlist[75];
    }

    if(!forced)
    {
	if(usb)
	    sprintf(path, "usb:/apps/wiidoom/music/song%i.ogg", currentsong);
	else if(sd)
	    sprintf(path, "sd:/apps/wiidoom/music/song%i.ogg", currentsong);
    }
    else
    {
	if(fake)
	{
	    if(usb)
		sprintf(path, "usb:/apps/wiidoom/music/song%i.ogg", faketracknum);
	    else if(sd)
		sprintf(path, "sd:/apps/wiidoom/music/song%i.ogg", faketracknum);
	}
	else
	{
	    if(usb)
		sprintf(path, "usb:/apps/wiidoom/music/song%i.ogg", tracknum);
	    else if(sd)
		sprintf(path, "sd:/apps/wiidoom/music/song%i.ogg", tracknum);
	}
    }
				// FIXME: THIS PRINTF FIXES A BUG WHEN USING MP3 "CHOOSE TRACK" FROM
    				// WITHIN THE MAIN MENU. WHILE THIS COMMAND IS NOT REQUIRED FOR WIIDOOM,
    printf("",tracknum);	// IT IS FOR WIIHERETIC OR ELSE THE GAME CRASHES UPON MUSIC SWITCHING.
    				// ADDED FOR WIIDOOM AS WELL BECAUSE LOADING THE INTERMISSION SCREEN IN
				// ACTIVATED MP3-MODE MAKES THE GAME CRASH IN MOST CASES.
    music = Mix_LoadMUS(path);

    if(music == NULL) 
    {
	printf("Unable to load music file: %s\n", Mix_GetError());
    }

    //Play music!
    if(Mix_PlayMusic(music, modex) == -1) 
    {
	printf("Unable to play music file: %s\n", Mix_GetError());
    }
	
    //The music is playing!
    musicPlaying = 1;
	
    //Make sure that the musicFinished() function is called when the music stops playing
    Mix_HookMusicFinished(musicFinished);
}
#endif

