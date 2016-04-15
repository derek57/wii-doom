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
//        Game completion, final screen animation.
//
//-----------------------------------------------------------------------------


#include <ctype.h>
#include <stdio.h>

// Functions.

#include "c_io.h"

// Data.
#include "d_main.h"

#include "d_deh.h"
#include "doomkeys.h"
#include "doomstat.h"
#include "dstrings.h"
#include "f_finale.h"
#include "hu_stuff.h"
#include "i_swap.h"
#include "i_system.h"
#include "i_video.h"
#include "m_controls.h"
#include "m_misc.h"
#include "m_random.h"
#include "r_state.h"
#include "s_sound.h"
#include "sounds.h"
#include "v_video.h"
#include "w_wad.h"
#include "wii-doom.h"
#include "z_zone.h"


#define MAX_CASTORDER   19
#define TEXTSPEED       3
#define TEXTWAIT        250


typedef struct
{
    char        *name;
    mobjtype_t  type;
} castinfo_t;


castinfo_t       castorder[MAX_CASTORDER];
//castinfo_t       castorderbeta[MAX_CASTORDER];

// Stage of animation:
state_t          *caststate;

unsigned int     finalecount;

dboolean         finale_music;
dboolean         castdeath;
dboolean         castattacking;
dboolean         castdeathflip;

char             *finaletext;
char             *finaleflat;

int              castrot;
int              castnum;
int              casttics;
int              castframes;
int              castonmelee;


extern void      A_RandomJump(mobj_t *actor); 


//
// F_StartFinale
//
void F_StartFinale(void)
{
    gameaction = ga_nothing;
    gamestate = GS_FINALE;
    viewactive = false;
    automapactive = false;

    if (logical_gamemission == doom)
    {
        S_ChangeMusic(mus_victor, true, false);
    }
    else
    {
        S_ChangeMusic(mus_read_m, true, false);
    }

    // Find the right screen and set the text and background

    // Okay - IWAD dependend stuff.
    // This has been changed severly, and
    //  some stuff might have changed in the process.
    switch (gamemode)
    {
        // DOOM 1 - E1, E3 or E4, but each nine missions
        case shareware:
        case registered:
        case retail:
        {
            switch (gameepisode)
            {
                case 1:
                    finaleflat = bgflatE1;
                    finaletext = s_E1TEXT;
                    break;

                case 2:
                    finaleflat = bgflatE2;
                    finaletext = s_E2TEXT;
                    break;

                case 3:
                    finaleflat = bgflatE3;
                    finaletext = s_E3TEXT;
                    break;

                case 4:
                    finaleflat = bgflatE4;
                    finaletext = s_E4TEXT;
                    break;

                default:
                    break;
            }

            break;
        }

        // DOOM II and missions packs with E1, M34
        case commercial:
        {
            // This is regular Doom II
            switch (gamemap)
            {
                case 6:
                    finaleflat = bgflat06;
                    finaletext = (gamemission == pack_tnt ? s_T1TEXT :
                        (gamemission == pack_plut ? s_P1TEXT : s_C1TEXT));

                    break;

                case 8:
                    if (gamemission == pack_nerve)
                    {
                        finaleflat = bgflat06;
                        finaletext = s_N1TEXT;
                    }

                    break;

                case 11:
                    finaleflat = bgflat11;
                    finaletext = (gamemission == pack_tnt ? s_T2TEXT :
                        (gamemission == pack_plut ? s_P2TEXT : s_C2TEXT));

                    break;

                case 20:
                    finaleflat = bgflat20;
                    finaletext = (gamemission == pack_tnt ? s_T3TEXT :
                        (gamemission == pack_plut ? s_P3TEXT : s_C3TEXT));

                    break;

                case 30:
                    finaleflat = bgflat30;
                    finaletext = (gamemission == pack_tnt ? s_T4TEXT :
                        (gamemission == pack_plut ? s_P4TEXT : s_C4TEXT));

                    break;

                case 15:
                    finaleflat = bgflat15;
                    finaletext = (gamemission == pack_tnt ? s_T5TEXT :
                        (gamemission == pack_plut ? s_P5TEXT : s_C5TEXT));

                    break;

                case 31:
                    finaleflat = bgflat31;
                    finaletext = (gamemission == pack_tnt ? s_T6TEXT :
                        (gamemission == pack_plut ? s_P6TEXT : s_C6TEXT));

                    break;

                default:
                    // Ouch.
                    break;
            }

            break;
        }

        // Indeterminate.
        default:
            finaleflat = "F_SKY1";
            finaletext = s_C1TEXT;
            break;
    }

    // Do dehacked substitutions of strings
    finalestage = F_STAGE_TEXT;
    finalecount = 0;
}

//
// F_CastResponder
//
static dboolean F_CastResponder(event_t *ev)
{
    mobjtype_t  type = castorder[castnum].type;

#ifdef WII
    if (ev->type == ev_joystick) 
#else
    if (ev->type != ev_keydown)
#endif
    {
        if ((ev->data1 & 1) == 0) 
            return false;
    }
                
    if (menuactive || paused || consoleactive)
        return false;

    if (ev->type == ev_keydown && ev->data1 != key_use && ev->data1 != key_fire
        && ev->data1 != KEY_LEFTARROW && ev->data1 != KEY_RIGHTARROW && ev->data1 != KEY_ENTER)
        return false;

    if (ev->type == ev_keyup)
        return false;

    if (castdeath)
        // already in dying frames
        return true;
    else
    {
        // rotate (taken from Eternity Engine)
        if (ev->data1 == KEY_LEFTARROW)
        {
            castrot = (castrot == 14 ? 0 : castrot + 2); 

            return true;
        }
        else if (ev->data1 == KEY_RIGHTARROW)
        {
            castrot = (!castrot ? 014 : castrot - 2); 

            return true;
        }
    }

    // go into death frame
    castdeath = true;

    if (d_flipcorpses && type != MT_CHAINGUY && type != MT_CYBORG)
        castdeathflip = rand() & 1;

    //if (!beta_style)
    {
        caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
        casttics = caststate->tics;

        if (casttics == -1 && caststate->action == A_RandomJump)
        {
            if (P_Random() < caststate->misc2)
                caststate = &states [caststate->misc1];
            else
                caststate = &states [caststate->nextstate];

            casttics = caststate->tics;
        }

        castrot = 0;
        castframes = 0;
        castattacking = false;

        if (mobjinfo[castorder[castnum].type].deathsound)
            S_StartSound(NULL, mobjinfo[castorder[castnum].type].deathsound);
    }
    /*
    else    
    {
        caststate = &states[mobjinfo[castorderbeta[castnum].type].deathstate];
        casttics = caststate->tics;
        castrot = 0;
        castframes = 0;
        castattacking = false;

        if (mobjinfo[castorderbeta[castnum].type].deathsound)
            S_StartSound(NULL, mobjinfo[castorderbeta[castnum].type].deathsound);
    }
    */
    return true;
}

dboolean F_Responder(event_t *event)
{
    if (finalestage == F_STAGE_CAST)
        return F_CastResponder(event);
        
    return false;
}

//
// F_StartCast
//
static void F_StartCast(void)
{
    castorder[0].name = s_CC_ZOMBIE,  castorder[0].type = MT_POSSESSED;
    castorder[1].name = s_CC_SHOTGUN, castorder[1].type = MT_SHOTGUY;
    castorder[2].name = s_CC_HEAVY,   castorder[2].type = MT_CHAINGUY;
    castorder[3].name = s_CC_IMP,     castorder[3].type = MT_TROOP;
    castorder[4].name = s_CC_DEMON,   castorder[4].type = MT_SERGEANT;
    castorder[5].name = s_CC_SPECTRE, castorder[5].type = MT_SHADOWS;
    castorder[6].name = s_CC_LOST,    castorder[6].type = MT_SKULL;
    castorder[7].name = s_CC_CACO,    castorder[7].type = MT_HEAD;
    castorder[8].name = s_CC_HELL,    castorder[8].type = MT_KNIGHT;
    castorder[9].name = s_CC_BARON,   castorder[9].type = MT_BRUISER;
    castorder[10].name = s_CC_ARACH,  castorder[10].type = MT_BABY;
    castorder[11].name = s_CC_PAIN,   castorder[11].type = MT_PAIN;
    castorder[12].name = s_CC_REVEN,  castorder[12].type = MT_UNDEAD;
    castorder[13].name = s_CC_MANCU,  castorder[13].type = MT_FATSO;
    castorder[14].name = s_CC_ARCH,   castorder[14].type = MT_VILE;
    castorder[15].name = s_CC_SPIDER, castorder[15].type = MT_SPIDER;
    castorder[16].name = s_CC_CYBER,  castorder[16].type = MT_CYBORG;
    castorder[17].name = s_CC_HERO,   castorder[17].type = MT_PLAYER;
    castorder[18].name = NULL,        castorder[18].type = 0;

    /*
    castorderbeta[0].name = s_CC_ZOMBIE,  castorderbeta[0].type = MT_BETAPOSSESSED;
    castorderbeta[1].name = s_CC_SHOTGUN, castorderbeta[1].type = MT_BETASHOTGUY;
    castorderbeta[2].name = s_CC_HEAVY,   castorderbeta[2].type = MT_CHAINGUY;
    castorderbeta[3].name = s_CC_IMP,     castorderbeta[3].type = MT_TROOP;
    castorderbeta[4].name = s_CC_DEMON,   castorderbeta[4].type = MT_SERGEANT;
    castorderbeta[5].name = s_CC_SPECTRE, castorderbeta[5].type = MT_SHADOWS;
    castorderbeta[6].name = s_CC_LOST,    castorderbeta[6].type = MT_BETASKULL;
    castorderbeta[7].name = s_CC_CACO,    castorderbeta[7].type = MT_BETAHEAD;
    castorderbeta[8].name = s_CC_HELL,    castorderbeta[8].type = MT_KNIGHT;
    castorderbeta[9].name = s_CC_BARON,   castorderbeta[9].type = MT_BETABRUISER;
    castorderbeta[10].name = s_CC_ARACH,  castorderbeta[10].type = MT_BABY;
    castorderbeta[11].name = s_CC_PAIN,   castorderbeta[11].type = MT_PAIN;
    castorderbeta[12].name = s_CC_REVEN,  castorderbeta[12].type = MT_UNDEAD;
    castorderbeta[13].name = s_CC_MANCU,  castorderbeta[13].type = MT_FATSO;
    castorderbeta[14].name = s_CC_ARCH,   castorderbeta[14].type = MT_VILE;
    castorderbeta[15].name = s_CC_SPIDER, castorderbeta[15].type = MT_SPIDER;
    castorderbeta[16].name = s_CC_CYBER,  castorderbeta[16].type = MT_CYBORG;
    castorderbeta[17].name = s_CC_HERO,   castorderbeta[17].type = MT_PLAYER;
    castorderbeta[18].name = NULL,        castorderbeta[18].type = 0;
    */

    // force a screen wipe
    wipegamestate = -1;

    castnum = 0;
    //if (!beta_style)
        caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    /*
    else
        caststate = &states[mobjinfo[castorderbeta[castnum].type].seestate];
    */

    casttics = caststate->tics;
    castrot = 0;
    castdeath = false;
    castdeathflip = false;
    finalestage = F_STAGE_CAST;
    castframes = 0;
    castonmelee = 0;
    castattacking = false;

    S_ChangeMusic(mus_evil, true, false);
}

//
// F_CastTicker
//
static void F_CastTicker(void)
{        
    if (--casttics > 0)
        // not time to change state yet
        return;
                
    if (caststate->tics == -1 || caststate->nextstate == S_NULL)
    {
        // switch from deathstate to next monster
        castnum++;
        castdeath = false;
        castdeathflip = false;

        //if (!beta_style)
        {
            if (castorder[castnum].name == NULL)
                castnum = 0;

            if (mobjinfo[castorder[castnum].type].seesound)
                S_StartSound(NULL, mobjinfo[castorder[castnum].type].seesound);

            caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        }
        /*
        else
        {
            if (castorderbeta[castnum].name == NULL)
                castnum = 0;

            if (mobjinfo[castorderbeta[castnum].type].seesound)
                S_StartSound(NULL, mobjinfo[castorderbeta[castnum].type].seesound);

            caststate = &states[mobjinfo[castorderbeta[castnum].type].seestate];
        }
        */

        castframes = 0;
    }
    else
    {
        int         st;
        int         sfx = 0;

        // just advance to next state in animation
        if (!castdeath && caststate == &states[S_PLAY_ATK1]) 
        {
            // Oh, gross hack!
            //if (!beta_style)
                goto stopattack;
            /*
            else
                goto stopattackbeta;
            */
        }

        if (caststate->action == A_RandomJump && P_Random() < caststate->misc2) 
            st = caststate->misc1;
        else
            st = caststate->nextstate;

        caststate = &states[st];
        castframes++;
        
        // sound hacks....
        switch (st)
        {
            case S_PLAY_ATK1:
                sfx = sfx_dshtgn;
                break;

            //case S_BETAPOSS_ATK2:     
            case S_POSS_ATK2:
                sfx = sfx_pistol;
                break;

            //case S_BETASPOS_ATK2:     
            case S_SPOS_ATK2:
                sfx = sfx_shotgn;
                break;

            case S_VILE_ATK2:
                sfx = sfx_vilatk;
                break;

            case S_SKEL_FIST2:
                sfx = sfx_skeswg;
                break;

            case S_SKEL_FIST4:
                sfx = sfx_skepch;
                break;

            case S_SKEL_MISS2:
                sfx = sfx_skeatk;
                break;

            case S_FATT_ATK8:
            case S_FATT_ATK5:
            case S_FATT_ATK2:
                sfx = sfx_firsht;
                break;

            case S_CPOS_ATK2:
            case S_CPOS_ATK3:
            case S_CPOS_ATK4:
                sfx = sfx_shotgn;
                break;

            case S_TROO_ATK3:
                sfx = sfx_claw;
                break;

            case S_SARG_ATK2:
                sfx = sfx_sgtatk;
                break;

            //case S_BETAHEAD_ATK2:     
            //case S_BETABOSS_ATK2:
            case S_BOSS_ATK2:
            case S_BOS2_ATK2:
            case S_HEAD_ATK2:
                sfx = sfx_firsht;
                break;

            case S_BSKUL_ATK2:    
            case S_SKULL_ATK2:
                sfx = sfx_sklatk;
                break;

            case S_SPID_ATK2:
            case S_SPID_ATK3:
                sfx = sfx_shotgn;
                break;

            case S_BSPI_ATK2:
                if (fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                    fsize != 4274218 && fsize != 4225504 && fsize != 4196020 &&
                    fsize != 4207819 && fsize != 4225460 && fsize != 4234124)
                    sfx = sfx_plasma;

                break;

            case S_CYBER_ATK2:
            case S_CYBER_ATK4:
            case S_CYBER_ATK6:
                sfx = sfx_rlaunc;
                break;

            case S_PAIN_ATK3:
                if (fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                    fsize != 4274218 && fsize != 4225504 && fsize != 4196020 &&
                    fsize != 4225460 && fsize != 4207819 && fsize != 4234124)
                    sfx = sfx_sklatk;

                break;

            default:
                sfx = 0;
                break;
        }
                
        if (sfx)
            S_StartSound(NULL, sfx);
    }
        
    if (!castdeath && castframes == 12) 
    {
        // go into attack frame
        castattacking = true;

        //if (!beta_style)
        {
            if (castonmelee)
                caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
            else
                caststate = &states[mobjinfo[castorder[castnum].type].missilestate];

            castonmelee ^= 1;

            if (caststate == &states[S_NULL])
            {
                if (castonmelee)
                    caststate = &states[mobjinfo[castorder[castnum].type].meleestate];
                else
                    caststate = &states[mobjinfo[castorder[castnum].type].missilestate];
            }
        }
        /*
        else
        {
            if (castonmelee)
                caststate=&states[mobjinfo[castorderbeta[castnum].type].meleestate];
            else
                caststate=&states[mobjinfo[castorderbeta[castnum].type].missilestate];

            castonmelee ^= 1;

            if (caststate == &states[S_NULL])
            {
                if (castonmelee)
                    caststate=
                        &states[mobjinfo[castorderbeta[castnum].type].meleestate];
                else
                    caststate=
                        &states[mobjinfo[castorderbeta[castnum].type].missilestate];
            }
        }
        */
    }
        
    if (castattacking)
    {
        //if (!beta_style)
        {
            if (castframes == 24
                || caststate == &states[mobjinfo[castorder[castnum].type].seestate])
            {
                stopattack:
                    castattacking = false;
                    castframes = 0;
                    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
            }
        }
        /*
        else
        {
            if (castframes == 24
                || caststate == &states[mobjinfo[castorderbeta[castnum].type].seestate])
            {
                stopattackbeta:
                    castattacking = false;
                    castframes = 0;
                    caststate = &states[mobjinfo[castorderbeta[castnum].type].seestate];
            }
        }
        */
    }
        
    casttics = caststate->tics;

    if (casttics == -1)
    {
        if (caststate->action == A_RandomJump)
        {
            if (P_Random() < caststate->misc2)
                caststate = &states[caststate->misc1];
            else
                caststate = &states[caststate->nextstate];

            casttics = caststate->tics;
        }

        if (casttics == -1)
            casttics = 15;
   }
}

//
// F_Ticker
//
void F_Ticker(void)
{    
    if (menuactive || paused || consoleactive)
        return;

    // check for skipping
    if ((gamemode == commercial) && (finalecount > 50))
    {
        size_t i;

        // go on to the next level
        for (i = 0; i < MAXPLAYERS; i++)
            if (players[i].cmd.buttons)
                break;

        if (i < MAXPLAYERS)
        {
            if (gamemission == pack_nerve && gamemap == 8)
                F_StartCast();
            else if (gamemission == pack_master && (gamemap == 20 || gamemap == 21))
                F_StartCast();
            else if (gamemap == 30)
                F_StartCast();
            else
                gameaction = ga_worlddone;
        }
    }

    // advance animation
    finalecount++;
        
    if (finalestage == F_STAGE_CAST)
    {
        F_CastTicker();
        return;
    }
        
    if (gamemode == commercial)
        return;
                
    if (finalestage == F_STAGE_TEXT
        && finalecount > strlen(finaletext) * TEXTSPEED + TEXTWAIT)
    {
        finalecount = 0;
        finalestage = F_STAGE_ARTSCREEN;

        // force a wipe
        wipegamestate = -1;

        if (gameepisode == 3)
        {
            S_StartMusic(mus_bunny);
            finale_music = true;
        }
    }
}

//
// F_TextWrite
//
static void F_TextWrite(void)
{    
    byte        *src = W_CacheLumpName(finaleflat, PU_CACHE);
    byte        *dest = screens[0];
    int         w, x, y;
    int         cx = 10;
    int         cy = 10;
    signed int  count = ((signed int) finalecount - 10) / TEXTSPEED;
    char        *ch = finaletext;

    // erase the entire screen to a tiled background
    for (y = 0; y < SCREENHEIGHT; y++)
    {
        for (x = 0; x < SCREENWIDTH / 64; x++)
        {
            memcpy(dest, src + ((y & 63) << 6), 64);
            dest += 64;
        }

        if (SCREENWIDTH & 63)
        {
            memcpy(dest, src + ((y & 63) << 6), SCREENWIDTH & 63);
            dest += (SCREENWIDTH & 63);
        }
    }

    V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);
    
    // draw some of the text onto the screen
    if (count < 0)
        count = 0;

    for (; count; count--)
    {
        int c = *ch++;

        if (!c)
            break;

        if (c == '\n')
        {
            cx = 10;
            cy += 11;
            continue;
        }
                
        c = toupper(c) - HU_FONTSTART;

        if (c < 0 || c > HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }
                
        w = SHORT(hu_font[c]->width);

        if (cx + w > ORIGINALWIDTH)
            break;
        
        if (font_shadow == 1)
            V_DrawPatchWithShadow(cx, cy, 0, hu_font[c], false);
        else
            V_DrawPatch(cx, cy, 0, hu_font[c]);

        cx += w;
    }
}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
static void F_CastPrint(char *text)
{
    char         *ch = text;
    int          c, cx, w;
    int          width = 0;
    
    // find width
    while (ch)
    {
        c = *ch++;

        if (!c)
            break;

        c = toupper(c) - HU_FONTSTART;

        if (c < 0 || c > HU_FONTSIZE)
        {
            width += 4;
            continue;
        }
                
        w = SHORT(hu_font[c]->width);
        width += w;
    }
    
    // draw it
    cx = ORIGINALWIDTH / 2 - width / 2;
    ch = text;

    while (ch)
    {
        c = *ch++;

        if (!c)
            break;

        c = toupper(c) - HU_FONTSTART;

        if (c < 0 || c > HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }
                
        w = SHORT(hu_font[c]->width);

        if (font_shadow == 1)
            V_DrawPatchWithShadow(cx, 180, 0, hu_font[c], false);
        else
            V_DrawPatch(cx, 180, 0, hu_font[c]);

        cx += w;
    }
}

//
// F_CastDrawer
//
static void F_CastDrawer(void)
{
    spritedef_t      *sprdef = &sprites[caststate->sprite];
    spriteframe_t    *sprframe = &sprdef->spriteframes[caststate->frame & FF_FRAMEMASK];
    int              lump;
    dboolean         flip;
    patch_t          *patch;
    int              rot = 0;
    
    // erase the entire screen to a background
    V_DrawPatch (0, 0, 0, W_CacheLumpName (bgcastcall, PU_CACHE));

    //if (!beta_style)
        F_CastPrint(castorder[castnum].name);
    /*
    else
        F_CastPrint(castorderbeta[castnum].name);
    */

    // draw the current frame in the middle of the screen
    if (sprframe->rotate)
        rot = castrot;

    lump = sprframe->lump[rot];
    flip = !!(sprframe->flip & (1 << rot));
    patch = W_CacheLumpNum(lump + firstspritelump, PU_CACHE);

    if (flip || castdeathflip)
        V_DrawPatchFlipped(ORIGINALWIDTH / 2, 170, 0, patch);
    else
        V_DrawPatch(ORIGINALWIDTH / 2, 170, 0, patch);
}

//
// F_DrawPatchCol
//
static void F_DrawPatchCol(int x, patch_t *patch, int col)
{
    int          f;
    column_t     *column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
    byte         *desttop = screens[0] + x;

    // step through the posts in a column
    while (column->topdelta != 0xff)
    {
        for (f = 0; f <= hires; f++)
        {
            int          count = column->length;
            byte         *source = (byte *)column + 3;
            byte         *dest = desttop + column->topdelta * (SCREENWIDTH << hires) + (x * hires) + f;

            while (count--)
            {
                if (hires)
                {
                    *dest = *source;
                    dest += SCREENWIDTH;
                }

                *dest = *source++;
                dest += SCREENWIDTH;
            }
        }

        column = (column_t *)((byte *)column + column->length + 4);
    }
}

//
// F_BunnyScroll
//
static void F_BunnyScroll(void)
{
    signed int  scrolled = (ORIGINALWIDTH - ((signed int)finalecount - 230) / 2);
    int         x;
    patch_t     *p1 = W_CacheLumpName("PFUB2", PU_LEVEL);
    patch_t     *p2 = W_CacheLumpName("PFUB1", PU_LEVEL);
    char        name[10];
    int         stage;
    static int  laststage;
                
    V_MarkRect(0, 0, SCREENWIDTH, SCREENHEIGHT);
        
    if (scrolled > ORIGINALWIDTH)
        scrolled = ORIGINALWIDTH;

    if (scrolled < 0)
        scrolled = 0;
                
    for (x = 0; x < ORIGINALWIDTH; x++)
    {
        if (x + scrolled < ORIGINALWIDTH)
            F_DrawPatchCol(x, p1, x + scrolled);
        else
            F_DrawPatchCol(x, p2, x + scrolled - ORIGINALWIDTH);                
    }
        
    if (finalecount < 1130)
        return;

    if (finalecount < 1180)
    {
        if (font_shadow == 1)
            V_DrawPatchWithShadow((ORIGINALWIDTH - 13 * 8) / 2 + 1, (ORIGINALHEIGHT - 8 * 8) / 2 + 1, 0,
                    W_CacheLumpName("END0", PU_CACHE), false);
        else
            V_DrawPatch((ORIGINALWIDTH - 13 * 8) / 2, (ORIGINALHEIGHT - 8 * 8) / 2, 0,
                    W_CacheLumpName("END0", PU_CACHE));

        laststage = 0;
        return;
    }
        
    stage = (finalecount - 1180) / 5;

    if (stage > 6)
        stage = 6;

    if (stage > laststage)
    {
        S_StartSound(NULL, sfx_pistol);
        laststage = stage;
    }
        
    M_snprintf(name, 10, "END%i", stage);

    if (font_shadow == 1)
        V_DrawPatchWithShadow((ORIGINALWIDTH - 13 * 8) / 2 + 1, (ORIGINALHEIGHT - 8 * 8) / 2 + 1, 0,
                W_CacheLumpName(name, PU_CACHE), false);
    else
        V_DrawPatch((ORIGINALWIDTH - 13 * 8) / 2, (ORIGINALHEIGHT - 8 * 8) / 2, 0,
                W_CacheLumpName(name, PU_CACHE));
}

static void F_ArtScreenDrawer(void)
{    
    if (gameepisode == 3)
    {
        F_BunnyScroll();
    }
    else
    {
        char *lumpname;

        switch (gameepisode)
        {
            case 1:
                if (gamemode == retail)
                {
                    lumpname = "CREDIT";
                }
                else
                {
                    if (fsize != 12361532)
                        lumpname = "HELP2";
                    else
                        lumpname = "HELP1";
                }

                break;

            case 2:
                lumpname = "VICTORY2";
                break;

            case 4:
                lumpname = "ENDPIC";
                break;

            default:
                return;
        }

        V_DrawPatch(0, 0, 0, W_CacheLumpName(lumpname, PU_CACHE));
    }
}

//
// F_Drawer
//
void F_Drawer(void)
{
    switch (finalestage)
    {
        case F_STAGE_CAST:
            F_CastDrawer();
            break;

        case F_STAGE_TEXT:
            F_TextWrite();
            break;

        case F_STAGE_ARTSCREEN:
            F_ArtScreenDrawer();
            break;
    }
}

