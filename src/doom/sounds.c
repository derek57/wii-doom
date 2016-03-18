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
//        Created by a sound utility.
//        Kept as a sample, DOOM2 sounds.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>

#include "doomtype.h"
#include "sounds.h"


//
// Information about all the music
//

#define MUSIC(name) \
    { name, 0, NULL, NULL }

#define SOUND(name, priority) \
    { NULL, name, priority, NULL, -1, -1, 0, 0, -1, NULL }

#define SOUND_LINK(name, priority, link_id, pitch, volume) \
    { NULL, name, priority, &S_sfx[link_id], pitch, volume, 0, 0, -1, NULL }


musicinfo_t S_music[] =
{
    MUSIC(NULL),
    MUSIC("e1m1"),
    MUSIC("e1m2"),
    MUSIC("e1m3"),
    MUSIC("e1m4"),
    MUSIC("e1m5"),
    MUSIC("e1m6"),
    MUSIC("e1m7"),
    MUSIC("e1m8"),
    MUSIC("e1m9"),
    MUSIC("e2m1"),
    MUSIC("e2m2"),
    MUSIC("e2m3"),
    MUSIC("e2m4"),
    MUSIC("e2m5"),
    MUSIC("e2m6"),
    MUSIC("e2m7"),
    MUSIC("e2m8"),
    MUSIC("e2m9"),
    MUSIC("e3m1"),
    MUSIC("e3m2"),
    MUSIC("e3m3"),
    MUSIC("e3m4"),
    MUSIC("e3m5"),
    MUSIC("e3m6"),
    MUSIC("e3m7"),
    MUSIC("e3m8"),
    MUSIC("e3m9"),
    MUSIC("inter"),
    MUSIC("intro"),
    MUSIC("bunny"),
    MUSIC("victor"),
    MUSIC("introa"),
    MUSIC("runnin"),
    MUSIC("stalks"),
    MUSIC("countd"),
    MUSIC("betwee"),
    MUSIC("doom"),
    MUSIC("the_da"),
    MUSIC("shawn"),
    MUSIC("ddtblu"),
    MUSIC("in_cit"),
    MUSIC("dead"),
    MUSIC("stlks2"),
    MUSIC("theda2"),
    MUSIC("doom2"),
    MUSIC("ddtbl2"),
    MUSIC("runni2"),
    MUSIC("dead2"),
    MUSIC("stlks3"),
    MUSIC("romero"),
    MUSIC("shawn2"),
    MUSIC("messag"),
    MUSIC("count2"),
    MUSIC("ddtbl3"),
    MUSIC("ampie"),
    MUSIC("theda3"),
    MUSIC("adrian"),
    MUSIC("messg2"),
    MUSIC("romer2"),
    MUSIC("tense"),
    MUSIC("shawn3"),
    MUSIC("openin"),
    MUSIC("evil"),
    MUSIC("ultima"),
    MUSIC("read_m"),
    MUSIC("dm2ttl"),
    MUSIC("dm2int"), 
    MUSIC("credit")
};

//
// Information about all the sfx
//

sfxinfo_t S_sfx[] =
{
    // S_sfx[0] needs to be a dummy for odd reasons.
    SOUND("none",     0),
    SOUND("pistol",  64),
    SOUND("shotgn",  64),
    SOUND("sgcock",  64),
    SOUND("dshtgn",  64),
    SOUND("dbopn",   64),
    SOUND("dbcls",   64),
    SOUND("dbload",  64),
    SOUND("plasma",  64),
    SOUND("bfg",     64),
    SOUND("sawup",   64),
    SOUND("sawidl", 118),
    SOUND("sawful",  64),
    SOUND("sawhit",  64),
    SOUND("rlaunc",  64),
    SOUND("rxplod",  70),
    SOUND("firsht",  70),
    SOUND("firxpl",  70),
    SOUND("pstart", 100),
    SOUND("pstop",  100),
    SOUND("doropn", 100),
    SOUND("dorcls", 100),
    SOUND("stnmov", 119),
    SOUND("swtchn",  78),
    SOUND("swtchx",  78),
    SOUND("plpain",  96),
    SOUND("dmpain",  96),
    SOUND("popain",  96),
    SOUND("vipain",  96),
    SOUND("mnpain",  96),
    SOUND("pepain",  96),
    SOUND("slop",    78),
    SOUND("itemup",  78),
    SOUND("wpnup",   78),
    SOUND("oof",     96),
    SOUND("telept",  32),
    SOUND("posit1",  98),
    SOUND("posit2",  98),
    SOUND("posit3",  98),
    SOUND("bgsit1",  98),
    SOUND("bgsit2",  98),
    SOUND("sgtsit",  98),
    SOUND("cacsit",  98),
    SOUND("brssit",  94),
    SOUND("cybsit",  92),
    SOUND("spisit",  90),
    SOUND("bspsit",  90),
    SOUND("kntsit",  90),
    SOUND("vilsit",  90),
    SOUND("mansit",  90),
    SOUND("pesit",   90),
    SOUND("sklatk",  70),
    SOUND("sgtatk",  70),
    SOUND("skepch",  70),
    SOUND("vilatk",  70),
    SOUND("claw",    70),
    SOUND("skeswg",  70),
    SOUND("pldeth",  32),
    SOUND("pdiehi",  32),
    SOUND("podth1",  70),
    SOUND("podth2",  70),
    SOUND("podth3",  70),
    SOUND("bgdth1",  70),
    SOUND("bgdth2",  70),
    SOUND("sgtdth",  70),
    SOUND("cacdth",  70),
    SOUND("skldth",  70),
    SOUND("brsdth",  32),
    SOUND("cybdth",  32),
    SOUND("spidth",  32),
    SOUND("bspdth",  32),
    SOUND("vildth",  32),
    SOUND("kntdth",  32),
    SOUND("pedth",   32),
    SOUND("skedth",  32),
    SOUND("posact", 120),
    SOUND("bgact",  120),
    SOUND("dmact",  120),
    SOUND("bspact", 100),
    SOUND("bspwlk", 100),
    SOUND("vilact", 100),
    SOUND("noway",   78),
    SOUND("barexp",  60),
    SOUND("punch",   64),
    SOUND("hoof",    70),
    SOUND("metal",   70),
    SOUND_LINK("chgun", 64, sfx_pistol, 150, 0),
    SOUND("tink",    60),
    SOUND("bdopn",  100),
    SOUND("bdcls",  100),
    SOUND("itmbk",  100),
    SOUND("flame",   32),
    SOUND("flamst",  32),
    SOUND("getpow",  60),
    SOUND("bospit",  70),
    SOUND("boscub",  70),
    SOUND("bossit",  70),
    SOUND("bospn",   70),
    SOUND("bosdth",  70),
    SOUND("manatk",  70),
    SOUND("mandth",  70),
    SOUND("sssit",   70),
    SOUND("ssdth",   70),
    SOUND("keenpn",  70),
    SOUND("keendt",  70),
    SOUND("skeact",  70),
    SOUND("skesit",  70),
    SOUND("skeatk",  70),
    SOUND("radio",   60),

    SOUND("splsh0",  60),
    SOUND("splsh1",  60),
    SOUND("splsh2",  60),
    SOUND("splsh3",  60),
    SOUND("splsh4",  60),
    SOUND("splsh5",  60),
    SOUND("splsh6",  60),
    SOUND("splsh7",  60),
    SOUND("splsh8",  60),
    SOUND("splsh9",  60),

    // landing in slime
    SOUND("burn",    10),

    // landing in water
    SOUND("gloop",   10),

    SOUND("step0",   64),
    SOUND("step1",   64),
    SOUND("step2",   64),
    SOUND("step3",   64),

    // walking inside water
    SOUND("water",   96),

    // walking inside slime
    SOUND("lava",    96),

    // [crispy] play DSSECRET if available
    SOUND("secret",  60),

    SOUND("jump",    64),

    // killough 11/98: dog sounds
    SOUND("dgsit",   98),
    SOUND("dgatk",   70),
    SOUND("dgact",  120),
    SOUND("dgdth",   70),
    SOUND("dgpain",  96),

    SOUND("eefly",   96),

    // walking inside sludge
    SOUND("muck",    96),

    SOUND("squish",  96),
    SOUND("ric1",    96),
    SOUND("ric2",    96),
    SOUND("ric3",    96),
    SOUND("ric4",    96),
    SOUND("ric5",    96),
    SOUND("ric6",    96)
};

