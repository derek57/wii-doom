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


#include <stdio.h>
#include <ctype.h>

// Functions.
#include "deh_str.h"
#include "doomstat.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "i_swap.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"

// Data.
#include "d_main.h"
#include "dstrings.h"
#include "sounds.h"

#include "doomstat.h"
#include "r_state.h"


#define TEXTSPEED       3
#define TEXTWAIT        250

typedef enum
{
    F_STAGE_TEXT,
    F_STAGE_ARTSCREEN,
    F_STAGE_CAST,
} finalestage_t;

typedef struct
{
    GameMission_t mission;
    int episode, level;
    char *background;
    char *text;
} textscreen_t;

static textscreen_t textscreens[] =
{
    { doom,      1, 8,  "FLOOR4_8",  E1TEXT},
    { doom,      2, 8,  "SFLR6_1",   E2TEXT},
    { doom,      3, 8,  "MFLR8_4",   E3TEXT},
    { doom,      4, 8,  "MFLR8_3",   E4TEXT},

    { doom2,     1, 6,  "SLIME16",   C1TEXT},
    { doom2,     1, 11, "RROCK14",   C2TEXT},
    { doom2,     1, 20, "RROCK07",   C3TEXT},
    { doom2,     1, 30, "RROCK17",   C4TEXT},
    { doom2,     1, 15, "RROCK13",   C5TEXT},
    { doom2,     1, 31, "RROCK19",   C6TEXT},

    { pack_tnt,  1, 6,  "SLIME16",   T1TEXT},
    { pack_tnt,  1, 11, "RROCK14",   T2TEXT},
    { pack_tnt,  1, 20, "RROCK07",   T3TEXT},
    { pack_tnt,  1, 30, "RROCK17",   T4TEXT},
    { pack_tnt,  1, 15, "RROCK13",   T5TEXT},
    { pack_tnt,  1, 31, "RROCK19",   T6TEXT},

    { pack_plut, 1, 6,  "SLIME16",   P1TEXT},
    { pack_plut, 1, 11, "RROCK14",   P2TEXT},
    { pack_plut, 1, 20, "RROCK07",   P3TEXT},
    { pack_plut, 1, 30, "RROCK17",   P4TEXT},
    { pack_plut, 1, 15, "RROCK13",   P5TEXT},
    { pack_plut, 1, 31, "RROCK19",   P6TEXT}
};


finalestage_t   finalestage;

// Stage of animation:
state_t*        caststate;

extern patch_t  *hu_font[HU_FONTSIZE];

extern boolean  opl;

unsigned int    finalecount;

boolean         finale_music;
boolean         castdeath;
boolean         castattacking;

char*           finaletext;
char*           finaleflat;

int             castnum;
int             casttics;
int             castframes;
int             castonmelee;

void    F_StartCast (void);
void    F_CastTicker (void);
void    F_CastDrawer (void);
boolean F_CastResponder (event_t *ev);

//
// F_StartFinale
//
void F_StartFinale (void)
{
    size_t i;

    gameaction = ga_nothing;
    gamestate = GS_FINALE;
    viewactive = false;
    automapactive = false;

    if (logical_gamemission == doom)
    {
        S_ChangeMusic(mus_victor, true);
    }
    else
    {
        S_ChangeMusic(mus_read_m, true);
    }

    // Find the right screen and set the text and background

    for (i=0; i<arrlen(textscreens); ++i)
    {
        textscreen_t *screen = &textscreens[i];

        // Hack for Chex Quest

        if (gameversion == exe_chex && screen->mission == doom)
        {
            screen->level = 5;
        }

        if (logical_gamemission == screen->mission
         && (logical_gamemission != doom || gameepisode == screen->episode)
         && gamemap == screen->level)
        {
            finaletext = screen->text;
            finaleflat = screen->background;
        }
    }

    // Do dehacked substitutions of strings
  
    finaletext = DEH_String(finaletext);
    finaleflat = DEH_String(finaleflat);
    
    finalestage = F_STAGE_TEXT;
    finalecount = 0;
}



boolean F_Responder (event_t *event)
{
    if (finalestage == F_STAGE_CAST)
        return F_CastResponder (event);
        
    return false;
}


//
// F_Ticker
//
void F_Ticker (void)
{
    size_t i;
    
    // check for skipping
    if ( (gamemode == commercial)
      && ( finalecount > 50) )
    {
      // go on to the next level
      for (i=0 ; i<MAXPLAYERS ; i++)
        if (players[i].cmd.buttons)
          break;

      if (i < MAXPLAYERS)
      {
        if (gamemap == 30)
          F_StartCast ();
        else
          gameaction = ga_worlddone;
      }
    }
    
    // advance animation
    finalecount++;
        
    if (finalestage == F_STAGE_CAST)
    {
        F_CastTicker ();
        return;
    }
        
    if ( gamemode == commercial)
        return;
                
    if (finalestage == F_STAGE_TEXT
     && finalecount>strlen (finaletext)*TEXTSPEED + TEXTWAIT)
    {
        finalecount = 0;
        finalestage = F_STAGE_ARTSCREEN;
        wipegamestate = -1;                // force a wipe
        if (gameepisode == 3)
        {
            S_StartMusic (mus_bunny);

            finale_music = true;
        }
    }
}



//
// F_TextWrite
//
void F_TextWrite (void)
{
    byte*       src;
    byte*       dest;
    
    int         x,y,w;
    signed int  count;
    char*       ch;
    int         c;
    int         cx;
    int         cy;
    
    // erase the entire screen to a tiled background
    src = W_CacheLumpName ( finaleflat , PU_CACHE);
    dest = I_VideoBuffer;
        
    for (y=0 ; y<SCREENHEIGHT ; y++)
    {
        for (x=0 ; x<SCREENWIDTH/64 ; x++)
        {
            memcpy (dest, src+((y&63)<<6), 64);
            dest += 64;
        }
        if (SCREENWIDTH&63)
        {
            memcpy (dest, src+((y&63)<<6), SCREENWIDTH&63);
            dest += (SCREENWIDTH&63);
        }
    }

    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
    
    // draw some of the text onto the screen
    cx = 10;
    cy = 10;
    ch = finaletext;
        
    count = ((signed int) finalecount - 10) / TEXTSPEED;
    if (count < 0)
        count = 0;
    for ( ; count ; count-- )
    {
        c = *ch++;
        if (!c)
            break;
        if (c == '\n')
        {
            cx = 10;
            cy += 11;
            continue;
        }
                
        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c> HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }
                
        w = SHORT (hu_font[c]->width);
        if (cx+w > ORIGWIDTH)         // CHANGED FOR HIRES
            break;
        V_DrawPatch(cx, cy, hu_font[c]);
        cx+=w;
    }
        
}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
typedef struct
{
    char        *name;
    mobjtype_t  type;
} castinfo_t;

castinfo_t      castorder[] = {
    {CC_ZOMBIE, MT_POSSESSED},
    {CC_SHOTGUN, MT_SHOTGUY},
    {CC_HEAVY, MT_CHAINGUY},
    {CC_IMP, MT_TROOP},
    {CC_DEMON, MT_SERGEANT},
    {CC_LOST, MT_SKULL},
    {CC_CACO, MT_HEAD},
    {CC_HELL, MT_KNIGHT},
    {CC_BARON, MT_BRUISER},
    {CC_ARACH, MT_BABY},
    {CC_PAIN, MT_PAIN},
    {CC_REVEN, MT_UNDEAD},
    {CC_MANCU, MT_FATSO},
    {CC_ARCH, MT_VILE},
    {CC_SPIDER, MT_SPIDER},
    {CC_CYBER, MT_CYBORG},
    {CC_HERO, MT_PLAYER},

    {NULL,0}
};

castinfo_t      castorderbeta[] = {
    {CC_ZOMBIE, MT_BETAPOSSESSED},
    {CC_SHOTGUN, MT_BETASHOTGUY},
    {CC_HEAVY, MT_CHAINGUY},
    {CC_IMP, MT_TROOP},
    {CC_DEMON, MT_SERGEANT},
    {CC_LOST, MT_BETASKULL},
    {CC_CACO, MT_BETAHEAD},
    {CC_HELL, MT_KNIGHT},
    {CC_BARON, MT_BETABRUISER},
    {CC_ARACH, MT_BABY},
    {CC_PAIN, MT_PAIN},
    {CC_REVEN, MT_UNDEAD},
    {CC_MANCU, MT_FATSO},
    {CC_ARCH, MT_VILE},
    {CC_SPIDER, MT_SPIDER},
    {CC_CYBER, MT_CYBORG},
    {CC_HERO, MT_PLAYER},

    {NULL,0}
};

//
// F_StartCast
//
void F_StartCast (void)
{
    wipegamestate = -1;    // force a screen wipe
    castnum = 0;
    if(!beta_skulls)
        caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    else
        caststate = &states[mobjinfo[castorderbeta[castnum].type].seestate];
    casttics = caststate->tics;
    castdeath = false;
    finalestage = F_STAGE_CAST;
    castframes = 0;
    castonmelee = 0;
    castattacking = false;

    S_ChangeMusic(mus_evil, true);
}


#pragma GCC diagnostic push                             // FOR PSP: SceIo... operators
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // FOR PSP: SceIo... operators

//
// F_CastTicker
//
void F_CastTicker (void)
{
    int         st;
    int         sfx;
        
    if (--casttics > 0)
        return;         // not time to change state yet
                
    if (caststate->tics == -1 || caststate->nextstate == S_NULL)
    {
        // switch from deathstate to next monster
        castnum++;
        castdeath = false;
        if(!beta_skulls)
        {
            if (castorder[castnum].name == NULL)
                castnum = 0;
            if (mobjinfo[castorder[castnum].type].seesound)
                S_StartSound (NULL, mobjinfo[castorder[castnum].type].seesound);
            caststate = &states[mobjinfo[castorder[castnum].type].seestate];
        }
        else
        {
            if (castorderbeta[castnum].name == NULL)
                castnum = 0;
            if (mobjinfo[castorderbeta[castnum].type].seesound)
                S_StartSound (NULL, mobjinfo[castorderbeta[castnum].type].seesound);
            caststate = &states[mobjinfo[castorderbeta[castnum].type].seestate];
        }
        castframes = 0;
    }
    else
    {
        // just advance to next state in animation
        if (caststate == &states[S_PLAY_ATK1])
        {
            if(!beta_skulls)
                goto stopattack;    // Oh, gross hack!
            else
                goto stopattackbeta;    // Oh, gross hack!
        }
        st = caststate->nextstate;
        caststate = &states[st];
        castframes++;
        
        // sound hacks....
        switch (st)
        {
          case S_PLAY_ATK1:     sfx = sfx_dshtgn; break;
          case S_BETAPOSS_ATK2:     
          case S_POSS_ATK2:     sfx = sfx_pistol; break;
          case S_BETASPOS_ATK2:     
          case S_SPOS_ATK2:     sfx = sfx_shotgn; break;
          case S_VILE_ATK2:     sfx = sfx_vilatk; break;
          case S_SKEL_FIST2:    sfx = sfx_skeswg; break;
          case S_SKEL_FIST4:    sfx = sfx_skepch; break;
          case S_SKEL_MISS2:    sfx = sfx_skeatk; break;
          case S_FATT_ATK8:
          case S_FATT_ATK5:
          case S_FATT_ATK2:     sfx = sfx_firsht; break;
          case S_CPOS_ATK2:
          case S_CPOS_ATK3:
          case S_CPOS_ATK4:     sfx = sfx_shotgn; break;
          case S_TROO_ATK3:     sfx = sfx_claw; break;
          case S_SARG_ATK2:     sfx = sfx_sgtatk; break;
          case S_BETABOSS_ATK2:
          case S_BOSS_ATK2:
          case S_BOS2_ATK2:
          case S_BETAHEAD_ATK2:     
          case S_HEAD_ATK2:     sfx = sfx_firsht; break;
          case S_BSKUL_ATK2:    
          case S_SKULL_ATK2:    sfx = sfx_sklatk; break;
          case S_SPID_ATK2:
          case S_SPID_ATK3:     sfx = sfx_shotgn; break;
          case S_BSPI_ATK2:

               if (fsize != 4261144 && fsize != 4271324 && fsize != 4211660 && fsize != 4207819 &&
                   fsize != 4274218 && fsize != 4225504 && fsize != 4196020 && fsize != 4225460 &&
                   fsize != 4234124)
                   sfx = sfx_plasma; break;
          case S_CYBER_ATK2:
          case S_CYBER_ATK4:
          case S_CYBER_ATK6:    sfx = sfx_rlaunc; break;
          case S_PAIN_ATK3:

                if(fsize != 4261144 && fsize != 4271324 && fsize != 4211660 && fsize != 4207819 &&
                        fsize != 4274218 && fsize != 4225504 && fsize != 4196020 && fsize != 4225460 &&
                        fsize != 4234124)
                    sfx = sfx_sklatk; break;
          default: sfx = 0; break;
        }
                
        if (sfx)
            S_StartSound (NULL, sfx);
    }
        
    if (castframes == 12)
    {
        // go into attack frame
        castattacking = true;
        if(!beta_skulls)
        {
            if (castonmelee)
                caststate=&states[mobjinfo[castorder[castnum].type].meleestate];
            else
                caststate=&states[mobjinfo[castorder[castnum].type].missilestate];
            castonmelee ^= 1;
            if (caststate == &states[S_NULL])
            {
                if (castonmelee)
                    caststate=
                        &states[mobjinfo[castorder[castnum].type].meleestate];
                else
                    caststate=
                        &states[mobjinfo[castorder[castnum].type].missilestate];
            }
        }
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
    }
        
    if (castattacking)
    {
        if(!beta_skulls)
        {
            if (castframes == 24
                || caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
            {
              stopattack:
                castattacking = false;
                castframes = 0;
                caststate = &states[mobjinfo[castorder[castnum].type].seestate];
            }
        }
        else
        {
            if (castframes == 24
                || caststate == &states[mobjinfo[castorderbeta[castnum].type].seestate] )
            {
              stopattackbeta:
                castattacking = false;
                castframes = 0;
                caststate = &states[mobjinfo[castorderbeta[castnum].type].seestate];
            }
        }
    }
        
    casttics = caststate->tics;
    if (casttics == -1)
        casttics = 15;
}


//
// F_CastResponder
//

boolean F_CastResponder (event_t* ev)
{
//    if (ev->type != ev_keydown)
    if (ev->type == ev_joystick) 
    {
        if ((ev->data1 & 1) == 0) 
            return false;
    }
                
    if (castdeath)
        return true;                        // already in dying frames
                
    // go into death frame
    castdeath = true;
    if(!beta_skulls)
    {
        caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
        casttics = caststate->tics;
        castframes = 0;
        castattacking = false;
        if (mobjinfo[castorder[castnum].type].deathsound)
            S_StartSound (NULL, mobjinfo[castorder[castnum].type].deathsound);
    }
    else    
    {
        caststate = &states[mobjinfo[castorderbeta[castnum].type].deathstate];
        casttics = caststate->tics;
        castframes = 0;
        castattacking = false;
        if (mobjinfo[castorderbeta[castnum].type].deathsound)
            S_StartSound (NULL, mobjinfo[castorderbeta[castnum].type].deathsound);
    }
    return true;
}


void F_CastPrint (char* text)
{
    char*        ch;
    int          c;
    int          cx;
    int          w;
    int          width;
    
    // find width
    ch = text;
    width = 0;
        
    while (ch)
    {
        c = *ch++;
        if (!c)
            break;
        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c> HU_FONTSIZE)
        {
            width += 4;
            continue;
        }
                
        w = SHORT (hu_font[c]->width);
        width += w;
    }
    
    // draw it
    cx = 160-width/2;
    ch = text;
    while (ch)
    {
        c = *ch++;
        if (!c)
            break;
        c = toupper(c) - HU_FONTSTART;
        if (c < 0 || c> HU_FONTSIZE)
        {
            cx += 4;
            continue;
        }
                
        w = SHORT (hu_font[c]->width);
        V_DrawPatch(cx, 180, hu_font[c]);
        cx+=w;
    }
        
}


//
// F_CastDrawer
//

void F_CastDrawer (void)
{
    spritedef_t*     sprdef;
    spriteframe_t*   sprframe;
    int              lump;
    boolean          flip;
    patch_t*         patch;
    
    // erase the entire screen to a background
    V_DrawPatch (0, 0, W_CacheLumpName (DEH_String("BOSSBACK"), PU_CACHE));

    if(!beta_skulls)
        F_CastPrint (DEH_String(castorder[castnum].name));
    else
        F_CastPrint (DEH_String(castorderbeta[castnum].name));

    // draw the current frame in the middle of the screen
    sprdef = &sprites[caststate->sprite];
    sprframe = &sprdef->spriteframes[ caststate->frame & FF_FRAMEMASK];
    lump = sprframe->lump[0];
//    flip = (boolean)sprframe->flip[0];
    flip = (boolean)(sprframe->flip & (1 << 0));

    patch = W_CacheLumpNum (lump+firstspritelump, PU_CACHE);
    if (flip)
        V_DrawPatchFlipped(160, 170, patch);
    else
        V_DrawPatch(160, 170, patch);
}


//
// F_DrawPatchCol
//
void
F_DrawPatchCol
( int           x,
  patch_t*      patch,
  int           col )
{
    column_t*   column;
    byte*       source;
    byte*       dest;
    byte*       desttop;
    int         count, f;     // CHANGED FOR HIRES

    column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
    desttop = I_VideoBuffer + x;

    // step through the posts in a column
    while (column->topdelta != 0xff )
    {
        for (f = 0; f <= hires; f++) // ADDED FOR HIRES
        {                            // ADDED FOR HIRES
            source = (byte *)column + 3;
            dest = desttop + column->topdelta*
                   (SCREENWIDTH << hires) + (x * hires) + f; // CHANGED FOR HIRES
            count = column->length;
                
            while (count--)
            {
                if (hires)           // ADDED FOR HIRES
                {                    // ADDED FOR HIRES
                    *dest = *source; // ADDED FOR HIRES
                    dest += SCREENWIDTH; // ADDED FOR HIRES
                }                    // ADDED FOR HIRES
                *dest = *source++;
                dest += SCREENWIDTH;
            }
        }                            // ADDED FOR HIRES
        column = (column_t *)(  (byte *)column + column->length + 4 );
    }
}


//
// F_BunnyScroll
//
void F_BunnyScroll (void)
{
    signed int  scrolled;
    int         x;
    patch_t*    p1;
    patch_t*    p2;
    char        name[10];
    int         stage;
    static int  laststage;
                
    p1 = W_CacheLumpName (DEH_String("PFUB2"), PU_LEVEL);
    p2 = W_CacheLumpName (DEH_String("PFUB1"), PU_LEVEL);

    V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
        
    scrolled = (320 - ((signed int) finalecount-230)/2);
    if (scrolled > 320)
        scrolled = 320;
    if (scrolled < 0)
        scrolled = 0;
                
    for ( x=0 ; x<ORIGWIDTH ; x++) // CHANGED FOR HIRES
    {
        if (x+scrolled < 320)
            F_DrawPatchCol (x, p1, x+scrolled);
        else
            F_DrawPatchCol (x, p2, x+scrolled - 320);                
    }
        
    if (finalecount < 1130)
        return;
    if (finalecount < 1180)
    {
        V_DrawPatch((ORIGWIDTH - 13 * 8) / 2, // CHANGED FOR HIRES
                    (ORIGHEIGHT - 8 * 8) / 2, // CHANGED FOR HIRES
                    W_CacheLumpName(DEH_String("END0"), PU_CACHE)); // CHANGED FOR HIRES
        laststage = 0;
        return;
    }
        
    stage = (finalecount-1180) / 5;
    if (stage > 6)
        stage = 6;
    if (stage > laststage)
    {
        S_StartSound (NULL, sfx_pistol);
        laststage = stage;
    }
        
    DEH_snprintf(name, 10, "END%i", stage);

    V_DrawPatch((ORIGWIDTH - 13 * 8) / 2, // CHANGED FOR HIRES
                (ORIGHEIGHT - 8 * 8) / 2, // CHANGED FOR HIRES
                W_CacheLumpName (name,PU_CACHE)); // CHANGED FOR HIRES
}

static void F_ArtScreenDrawer(void)
{
    char *lumpname;
    
    if (gameepisode == 3)
    {
        F_BunnyScroll();
    }
    else
    {
        switch (gameepisode)
        {
            case 1:
                if (gamemode == retail)
                {
                    lumpname = "CREDIT";
                }
                else
                {
                    if(fsize != 12361532)
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

        lumpname = DEH_String(lumpname);

        V_DrawPatch (0, 0, W_CacheLumpName(lumpname, PU_CACHE));
    }
}

//
// F_Drawer
//
void F_Drawer (void)
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


