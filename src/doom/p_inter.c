// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
//
// Copyright(C) 2015 by Brad Harding: - (Key Card modification for locked doors)
//                                    - (Liquid Sector Animations)
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
//        Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------


#include <stdlib.h>

#include "am_map.h"
#include "c_io.h"
#include "d_deh.h"

// Data.
#include "doomdef.h"

#include "doomstat.h"
#include "dstrings.h"
#include "hu_stuff.h"

#include "i_system.h"
#include "i_timer.h"
#include "m_random.h"
#include "p_inter.h"
#include "p_local.h"
#include "p_tick.h"
#include "s_sound.h"
#include "sounds.h"
#include "st_stuff.h"
#include "wii-doom.h"


// Ty 03/07/98 - add deh externals
// Maximums and such were hardcoded values.  Need to externalize those for
// dehacked support (and future flexibility).  Most var names came from the key
// strings used in dehacked.
int                initial_health = 100;
int                initial_bullets = 50;
int                maxhealth = MAXHEALTH * 2;
int                max_armor = 200;
int                green_armor_class = 1;
int                blue_armor_class = 2;
int                max_soul = 200;
int                soul_health = 100;
int                mega_health = 200;
int                god_health = 100;
int                idfa_armor = 200;
int                idfa_armor_class = 2;
int                idkfa_armor = 200;
int                idkfa_armor_class = 2;
int                bfgcells = BFGCELLS;

// a weapon is found with two clip loads,
// a big item has five clip loads
int                maxammo[NUMAMMO] = {200, 50, 300, 50};
int                clipammo[NUMAMMO] = {10, 4, 20, 1};
int                cardsfound;
int                species_infighting = 0;


extern dboolean    massacre_cheat_used;


//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

int P_GiveAmmo(player_t *player, ammotype_t ammo, int num)
{
    int    oldammo;
        
    if (ammo == am_noammo)
        return 0;
                
    if (ammo > NUMAMMO)
        C_Error("P_GiveAmmo: bad type %i", ammo);
                
    if (player->ammo[ammo] == player->maxammo[ammo])
        return 0;
                
    if (num)
        num *= clipammo[ammo];
    else
        num = clipammo[ammo] / 2;
    
    if (gameskill == sk_baby || gameskill == sk_nightmare)
    {
        // give double ammo in trainer mode,
        // you'll need in nightmare
        num <<= 1;
    }    
                
    oldammo = player->ammo[ammo];
    player->ammo[ammo] += num;

    if (player->ammo[ammo] > player->maxammo[ammo])
        player->ammo[ammo] = player->maxammo[ammo];

    if (hud && num && ammo == weaponinfo[player->readyweapon].ammo)
        ammohighlight = I_GetTimeMS() + HUD_AMMO_HIGHLIGHT_WAIT;

    // If non zero ammo, 
    // don't change up weapons,
    // player was lower on purpose.
    if (oldammo)
        return num;        

    // We were down to zero,
    // so select a new weapon.
    // Preferences are not user selectable.
    switch (ammo)
    {
        case am_clip:
            if (player->readyweapon == wp_fist)
            {
                if (player->weaponowned[wp_chaingun])
                    player->pendingweapon = wp_chaingun;
                else
                    player->pendingweapon = wp_pistol;
            }

            break;
        
        case am_shell:
            if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
            {
                if (player->weaponowned[wp_shotgun])
                    player->pendingweapon = wp_shotgun;
            }

            break;
        
        case am_cell:
            if (player->readyweapon == wp_fist || player->readyweapon == wp_pistol)
            {
                if (player->weaponowned[wp_plasma])
                    player->pendingweapon = wp_plasma;
            }

            break;
        
        case am_misl:
            if (player->readyweapon == wp_fist)
            {
                if (player->weaponowned[wp_missile])
                    player->pendingweapon = wp_missile;
            }

        default:
            break;
    }
        
    return num;
}

//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
dboolean P_GiveWeapon(player_t *player, weapontype_t weapon, dboolean dropped)
{
    dboolean        gaveammo;
    dboolean        gaveweapon;

    if (netgame && (deathmatch != 2) && !dropped)
    {
        // leave placed weapons forever on net games
        if (player->weaponowned[weapon])
            return false;

        player->bonuscount += BONUSADD;
        player->weaponowned[weapon] = true;

        if (deathmatch)
            P_GiveAmmo(player, weaponinfo[weapon].ammo, 5);
        else
            P_GiveAmmo(player, weaponinfo[weapon].ammo, 2);

        player->pendingweapon = weapon;

        if (!d_sound || player == &players[consoleplayer])
            S_StartSound(NULL, sfx_wpnup);

        return false;
    }

    if (weaponinfo[weapon].ammo != am_noammo)
    {
        // give one clip with a dropped weapon,
        // two clips with a found weapon
        if (dropped)
            gaveammo = P_GiveAmmo(player, weaponinfo[weapon].ammo, 1);
        else
            gaveammo = P_GiveAmmo(player, weaponinfo[weapon].ammo, 2);
    }
    else
        gaveammo = false;
        
    if (player->weaponowned[weapon])
        gaveweapon = false;
    else
    {
        gaveweapon = true;
        player->weaponowned[weapon] = true;
        player->pendingweapon = weapon;
    }
        
    if (gaveweapon || gaveammo)
    {
        if (hud)
            ammohighlight = I_GetTimeMS() + HUD_AMMO_HIGHLIGHT_WAIT;

        return true;
    }
    else
        return false;
}

//
// P_GiveBody
// Returns false if the body isn't needed at all
//
dboolean P_GiveBody(player_t *player, int num)
{
    if (player->health >= MAXHEALTH)
        return false;
                
    player->health += num;

    if (player->health > MAXHEALTH)
        player->health = MAXHEALTH;

    player->mo->health = player->health;
    healthhighlight = I_GetTimeMS() + HUD_HEALTH_HIGHLIGHT_WAIT;
    return true;
}

//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
dboolean P_GiveArmor(player_t *player, armortype_t armortype)
{
    int    hits = armortype * 100;

    if (player->armorpoints >= hits)
        // don't pick up
        return false;
                
    player->armortype = armortype;
    player->armorpoints = hits;
    armorhighlight = I_GetTimeMS() + HUD_ARMOR_HIGHLIGHT_WAIT;
    return true;
}

//
// P_InitCards
//
void P_InitCards(player_t *player)
{
    int i;

    for (i = 0; i < NUMCARDS; i++)
        player->cards[i] = CARDNOTINMAP;

    cardsfound = 0;

    for (i = 0; i < numsectors; i++)
    {
        mobj_t  *thing = sectors[i].thinglist;

        while (thing)
        {
            switch (thing->sprite)
            {
                case SPR_BKEY:
                    player->cards[it_bluecard] = CARDNOTFOUNDYET;
                    break;

                case SPR_RKEY:
                    player->cards[it_redcard] = CARDNOTFOUNDYET;
                    break;

                case SPR_YKEY:
                    player->cards[it_yellowcard] = CARDNOTFOUNDYET;
                    break;

                case SPR_BSKU:
                case SPR_BBSK:
                    player->cards[it_blueskull] = CARDNOTFOUNDYET;
                    break;

                case SPR_RSKU:
                case SPR_BRSK:
                    player->cards[it_redskull] = CARDNOTFOUNDYET;
                    break;

                case SPR_YSKU:
                case SPR_BYSK:
                    player->cards[it_yellowskull] = CARDNOTFOUNDYET;
                    break;

                default:
                    break;
            }

            thing = thing->snext;
        }
    }

    for (i = 0; i < numlines; i++)
    {
        line_t  *line = &lines[i];

        switch (line->special)
        {
            case DR_Door_Blue_OpenWaitClose:
            case D1_Door_Blue_OpenStay:
            case SR_Door_Blue_OpenStay_Fast:
            case S1_Door_Blue_OpenStay_Fast:
                if (player->cards[it_blueskull] == CARDNOTINMAP)
                    player->cards[it_bluecard] = CARDNOTFOUNDYET;

                break;

            case DR_Door_Red_OpenWaitClose:
            case D1_Door_Red_OpenStay:
            case SR_Door_Red_OpenStay_Fast:
            case S1_Door_Red_OpenStay_Fast:
                if (player->cards[it_redskull] == CARDNOTINMAP)
                    player->cards[it_redcard] = CARDNOTFOUNDYET;

                break;

            case DR_Door_Yellow_OpenWaitClose:
            case D1_Door_Yellow_OpenStay:
            case SR_Door_Yellow_OpenStay_Fast:
            case S1_Door_Yellow_OpenStay_Fast:
                if (player->cards[it_yellowskull] == CARDNOTINMAP)
                    player->cards[it_yellowcard] = CARDNOTFOUNDYET;

                break;
        }
    }
}

//
// P_GiveCard
//
void P_GiveCard(player_t *player, card_t card)
{
    player->bonuscount = BONUSADD;
    player->cards[card] = ++cardsfound;

    if (card == player->neededcard)
        player->neededcard = player->neededcardflash = 0;
}

void P_GiveAllCards(player_t *player)
{
    int         i;
    dboolean    skulliscard = true;

    for (i = 0; i < numlines; ++i)
        if (lines[i].special >= GenLockedBase
            && !((lines[i].special & LockedNKeys) >> LockedNKeysShift))
        {
            skulliscard = false;
            break;
        }

    for (i = NUMCARDS - 1; i >= 0; --i)
        if (player->cards[i] != CARDNOTINMAP && player->cards[i] == CARDNOTFOUNDYET)
        {
            if (skulliscard && ((i == it_blueskull && player->cards[it_bluecard] != CARDNOTINMAP)
                || (i == it_redskull && player->cards[it_redcard] != CARDNOTINMAP)
                || (i == it_yellowskull && player->cards[it_yellowcard] != CARDNOTINMAP)))
                continue;

            P_GiveCard(player, i);
        }
}

//
// P_GivePower
//
dboolean P_GivePower(player_t *player, int power)
{
    if (power == pw_invulnerability)
    {
        player->powers[power] = INVULNTICS;
        return true;
    }
    
    if (power == pw_invisibility)
    {
        if (beta_style)
            player->powers[power] = INVULNTICS;
        else
            player->powers[power] = INVISTICS;

        player->mo->flags |= MF_SHADOW;
        return true;
    }
    
    if (power == pw_infrared)
    {
        player->powers[power] = INFRATICS;
        return true;
    }
    
    if (power == pw_ironfeet)
    {
        player->powers[power] = IRONTICS;
        return true;
    }
    
    if (power == pw_strength)
    {
        P_GiveBody(player, 100);
        player->powers[power] = 1;
        return true;
    }

    if (power == pw_flight)
    {
        player->powers[power] = FLIGHTTICS;
        player->mo->flags2 |= MF2_FLY;
        player->mo->flags |= MF_NOGRAVITY;

        if (player->mo->z <= player->mo->floorz)
        {
            // thrust the player in the air a bit
            player->flyheight = 10;
        }

        return (true);
    }

    if (player->powers[power])
        // already got it
        return false;
                
    player->powers[power] = 1;
    return true;
}

//
// P_TouchSpecialThing
//
void P_TouchSpecialThing(mobj_t *special, mobj_t *toucher)
{
    player_t     *player;
    int          i;
    fixed_t      delta = special->z - toucher->z;
    int          sound;
    int          ammo;

    if (delta > toucher->height || delta < -8 * FRACUNIT)
    {
        // out of reach
        return;
    }
        
    sound = sfx_itemup;        
    player = toucher->player;

    // Dead thing touching.
    // Can happen with a sliding player corpse.
    if (toucher->health <= 0)
        return;

    // Identify by sprite.
    switch (special->sprite)
    {
        // armor
        case SPR_ARM1:
            if (!P_GiveArmor(player, green_armor_class))
                return;

            if (beta_style)
                player->message = GOTARMORBETA;
            else
                HU_PlayerMessage(s_GOTARMOR, true);

            break;
                
        case SPR_ARM2:
            if (!P_GiveArmor(player, blue_armor_class))
                return;

            if (beta_style)
                player->message = GOTMEGABETA;
            else
                HU_PlayerMessage(s_GOTMEGA, true);

            break;
        
        // bonus items
        case SPR_BON1:
            // can go over 100%
            player->health++;

            if (player->health > maxhealth)
            {
                if (fsize == 10396254 || fsize == 10399316 || fsize == 4207819 ||
                    fsize == 4274218 || fsize == 4225504)
                    player->health = maxhealth - 1;
                else
                    player->health = maxhealth;
            }
            else
                healthhighlight = I_GetTimeMS() + HUD_HEALTH_HIGHLIGHT_WAIT;

            player->mo->health = player->health;

            if (beta_style)
                player->message = GOTHTHBONUS;
            else
                HU_PlayerMessage(s_GOTHTHBONUS, true);

            break;
        
        case SPR_BON2:
            // can go over 100%
            player->armorpoints++;

            if (player->armorpoints > max_armor)
                player->armorpoints = max_armor;
            else
                armorhighlight = I_GetTimeMS() + HUD_ARMOR_HIGHLIGHT_WAIT;

            // green_armor_class only applies to the green armor shirt;
            // for the armor helmets, armortype 1 is always used.
            if (!player->armortype)
                player->armortype = GREENARMOR;

            if (beta_style)
                player->message = GOTARMBONUS;
            else
                HU_PlayerMessage(s_GOTARMBONUS, true);

            break;
        
        case SPR_SOUL:
            if (beta_style)
            {
                player->health = 100;
                player->mo->health = player->health;
                healthhighlight = I_GetTimeMS() + HUD_HEALTH_HIGHLIGHT_WAIT;
                player->extra_lifes++;
                player->message = GOTSUPERBETA;
            }
            else
            {
                player->health += soul_health;

                if (player->health > max_soul)
                    player->health = max_soul;

                if ((fsize == 10396254 || fsize == 10399316 || fsize == 4207819 ||
                    fsize == 4274218 || fsize == 4225504) && player->health == maxhealth)
                    player->health = player->health - 1;

                player->mo->health = player->health;
                HU_PlayerMessage(s_GOTSUPER, true);
            }

            if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
                sound = sfx_getpow;
            else
                sound = sfx_itemup;

            break;
        
        case SPR_MEGA:
            if (gamemode != commercial)
                return;

            player->health = mega_health;
            player->mo->health = player->health;

            // We always give armor type 2 for the megasphere; dehacked only 
            // affects the MegaArmor.
            P_GiveArmor(player, 2);
            HU_PlayerMessage(s_GOTMSPHERE, true);

            if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
                sound = sfx_getpow;
            else
                sound = sfx_itemup;

            break;
        
        // cards
        // leave cards for everyone
        case SPR_BKEY:
            if (player->cards[it_bluecard] <= 0)
            {
                if (beta_style)
                    player->message = GOTBLUECARDBETA;
                else
                    HU_PlayerMessage(s_GOTBLUECARD, true);

                P_GiveCard(player, it_bluecard);
    
                if (!netgame)
                    break;
            }

            return;
        
        case SPR_YKEY:
            if (player->cards[it_yellowcard] <= 0)
            {
                if (beta_style)
                    player->message = GOTYELWCARDBETA;
                else
                    HU_PlayerMessage(s_GOTYELWCARD, true);

                P_GiveCard(player, it_yellowcard);
    
                if (!netgame)
                    break;
            }

            return;
        
        case SPR_RKEY:
            if (player->cards[it_redcard] <= 0)
            {
                if (beta_style)
                    player->message = GOTREDCARDBETA;
                else
                    HU_PlayerMessage(s_GOTREDCARD, true);

                P_GiveCard(player, it_redcard);
    
                if (!netgame)
                    break;
            }

            return;
        
        case SPR_BSKU:
        case SPR_BBSK:
            if (player->cards[it_blueskull] <= 0)
            {
                if (beta_style)
                    player->message = GOTBLUESKULBETA;
                else
                    HU_PlayerMessage(s_GOTBLUESKUL, true);

                P_GiveCard(player, it_blueskull);

                if (!netgame)
                    break;
            }

            return;
        
        case SPR_YSKU:
        case SPR_BYSK:
            if (player->cards[it_yellowskull] <= 0)
            {
                if (beta_style)
                    player->message = GOTYELWSKULBETA;
                else
                    HU_PlayerMessage(s_GOTYELWSKUL, true);

                P_GiveCard(player, it_yellowskull);

                if (!netgame)
                    break;
            }

            return;
        
        case SPR_RSKU:
        case SPR_BRSK:
            if (player->cards[it_redskull] <= 0)
            {
                if (beta_style)
                    player->message = GOTREDSKULLBETA;
                else
                    HU_PlayerMessage(s_GOTREDSKULL, true);

                P_GiveCard(player, it_redskull);

                if (!netgame)
                    break;
            }

            return;
        
        // medikits, heals
        case SPR_STIM:
            if (!P_GiveBody(player, 10))
                return;

            if (beta_style)
                player->message = GOTSTIMBETA;
            else
                HU_PlayerMessage(s_GOTSTIM, true);

            break;
        
        case SPR_MEDI:
            if (!P_GiveBody(player, 25))
                return;

            if (player->health < 25)
            {
                if (beta_style)
                    player->message = GOTMEDINEEDBETA;
                else
                    HU_PlayerMessage(s_GOTMEDINEED, true);
            }
            else
            {
                if (beta_style)
                    player->message = GOTMEDIKITBETA;
                else
                    HU_PlayerMessage(s_GOTMEDIKIT, true);
            }

            break;
        
        // power ups
        case SPR_PINV:
        case SPR_BPNV:
            if (!P_GivePower(player, pw_invulnerability))
                return;

            if (beta_style)
                player->message = GOTINVUL;
            else
                HU_PlayerMessage(s_GOTINVUL, true);
    
            if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
                sound = sfx_getpow;
            else
                sound = sfx_itemup;

            break;
        
        case SPR_PSTR:
            if (!P_GivePower(player, pw_strength))
                return;

            if (beta_style)
                player->message = GOTBERSERK;
            else
                HU_PlayerMessage(s_GOTBERSERK, true);

            if (player->readyweapon != wp_fist)
                player->pendingweapon = wp_fist;

            if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
                sound = sfx_getpow;
            else
                sound = sfx_itemup;

            break;
        
        case SPR_PINS:
        case SPR_BPNS:
            if (!P_GivePower(player, pw_invisibility))
                return;

            if (beta_style)
                player->message = GOTINVIS;
            else
                HU_PlayerMessage(s_GOTINVIS, true);

            if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
                sound = sfx_getpow;
            else
                sound = sfx_itemup;

            break;
        
        case SPR_SUIT:
            if (!P_GivePower(player, pw_ironfeet))
                return;

            if (beta_style)
                player->message = GOTSUIT;
            else
                HU_PlayerMessage(s_GOTSUIT, true);

            if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
                sound = sfx_getpow;
            else
                sound = sfx_itemup;

            break;
        
        case SPR_PMAP:
            if (!P_GivePower(player, pw_allmap))
                return;

            if (beta_style)
                player->message = GOTMAP;
            else
                HU_PlayerMessage(s_GOTMAP, true);

            if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
                sound = sfx_getpow;
            else
                sound = sfx_itemup;

            break;
        
        case SPR_PVIS:
            if (!P_GivePower(player, pw_infrared))
                return;

            if (beta_style)
                player->message = GOTVISOR;
            else
                HU_PlayerMessage(s_GOTVISOR, true);
    
            if (fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
                sound = sfx_getpow;
            else
                sound = sfx_itemup;

            break;
        
        // ammo
        case SPR_CLIP:
            if (!(ammo = P_GiveAmmo(player, am_clip, (special->flags & MF_DROPPED) == 0)))
                return;

            if (beta_style)
                player->message = GOTCLIP;
            else
            {
                if (ammo == clipammo[am_clip] || (deh_strlookup[p_GOTCLIP].assigned && dehacked))
                    HU_PlayerMessage(s_GOTCLIP, true);
                else
                    HU_PlayerMessage((ammo == clipammo[am_clip] / 2 ? s_GOTHALFCLIP : s_GOTCLIPX2), true);
            }

            break;
        
        case SPR_AMMO:
            if (!P_GiveAmmo(player, am_clip,5))
                return;

            if (beta_style)
                player->message = GOTCLIPBOX;
            else
                HU_PlayerMessage(s_GOTCLIPBOX, true);

            break;
        
        case SPR_ROCK:
            if (!(ammo = P_GiveAmmo(player, am_misl, 1)))
                return;

            if (beta_style)
                player->message = GOTROCKET;
            else
            {
                if (ammo == clipammo[am_misl] || (deh_strlookup[p_GOTROCKET].assigned && dehacked))
                    HU_PlayerMessage(s_GOTROCKET, true);
                else
                    HU_PlayerMessage(s_GOTROCKETX2, true);
            }

            break;
        
        case SPR_BROK:
            if (!P_GiveAmmo(player, am_misl, 5))
                return;

            if (beta_style)
                player->message = GOTROCKBOX;
            else
                HU_PlayerMessage(s_GOTROCKBOX, true);

            break;
        
        case SPR_CELL:
        case SPR_BCLL:
            if (!(ammo = P_GiveAmmo(player, am_cell, 1)))
                return;

            if (beta_style)
                player->message = GOTCELL;
            else
            {
                if (ammo == clipammo[am_cell] || (deh_strlookup[p_GOTCELL].assigned && dehacked))
                    HU_PlayerMessage(s_GOTCELL, true);
                else
                    HU_PlayerMessage(s_GOTCELLX2, true);
            }

            break;
        
        case SPR_CELP:
            if (!P_GiveAmmo(player, am_cell, 5))
                return;

            if (beta_style)
                player->message = GOTCELLBOX;
            else
                HU_PlayerMessage(s_GOTCELLBOX, true);

            break;
        
        case SPR_SHEL:
        case SPR_BSHL:
            if (!(ammo = P_GiveAmmo(player, am_shell, 1)))
                return;

            if (!message_dontfuckwithme)
            {
                if (beta_style)
                    player->message = GOTSHELLS;
                else
                {
                    if (ammo == clipammo[am_shell] || (deh_strlookup[p_GOTSHELLS].assigned && dehacked))
                        HU_PlayerMessage(s_GOTSHELLS, true);
                    else
                        HU_PlayerMessage(s_GOTSHELLSX2, true);
                }
            }

            break;
        
        case SPR_SBOX:
        case SPR_BBOX:
            if (!P_GiveAmmo(player, am_shell, 5))
                return;

            if (beta_style)
                player->message = GOTSHELLBOX;
            else
                HU_PlayerMessage(s_GOTSHELLBOX, true);

            break;
        
        case SPR_BPAK:
            if (!player->backpack)
            {
                for (i = 0; i < NUMAMMO; i++)
                    player->maxammo[i] *= 2;

                player->backpack = true;
            }

            for (i = 0; i < NUMAMMO; i++)
                P_GiveAmmo(player, i, 1);

            if (beta_style)
                player->message = GOTBACKPACK;
            else
                HU_PlayerMessage(s_GOTBACKPACK, true);

            break;
        
        // weapons
        case SPR_BFUG:
            if (!P_GiveWeapon(player, wp_bfg, false))
                return;

            if (beta_style)
                player->message = GOTBFG9000;
            else
                HU_PlayerMessage(s_GOTBFG9000, true);

            sound = sfx_wpnup;
            break;
        
        case SPR_MGUN:
            if (!P_GiveWeapon(player, wp_chaingun, (special->flags & MF_DROPPED) != 0))
                return;

            if (beta_style)
                player->message = GOTCHAINGUN;
            else
                HU_PlayerMessage(s_GOTCHAINGUN, true);

            sound = sfx_wpnup;        
            break;
        
        case SPR_CSAW:
            if (!P_GiveWeapon(player, wp_chainsaw, false))
                return;

            if (beta_style)
                player->message = GOTCHAINSAW;
            else
                HU_PlayerMessage(s_GOTCHAINSAW, true);

            sound = sfx_wpnup;        
            break;
        
        case SPR_LAUN:
            if (!P_GiveWeapon(player, wp_missile, false))
                return;

            if (beta_style)
                player->message = GOTLAUNCHER;
            else
                HU_PlayerMessage(s_GOTLAUNCHER, true);

            sound = sfx_wpnup;        
            break;
        
        case SPR_PLAS:
            if (!P_GiveWeapon(player, wp_plasma, false))
                return;

            if (beta_style)
                player->message = GOTPLASMA;
            else
                HU_PlayerMessage(s_GOTPLASMA, true);

            sound = sfx_wpnup;        
            break;
        
        case SPR_SHOT:
            if (!P_GiveWeapon(player, wp_shotgun, (special->flags & MF_DROPPED) != 0))
                return;

            if (beta_style)
                player->message = GOTSHOTGUN;
            else
                HU_PlayerMessage(s_GOTSHOTGUN, true);

            sound = sfx_wpnup;        
            break;
                
        case SPR_SGN2:
            if (!P_GiveWeapon(player, wp_supershotgun, (special->flags & MF_DROPPED) != 0))
                return;

            HU_PlayerMessage(s_GOTSHOTGUN2, true);
            sound = sfx_wpnup;        
            break;
                
        case SPR_BND1:
            if (player->item < 100);
                player->item++;

            if (beta_style)
                player->message = GOTDAGGER;
            else
                HU_PlayerMessage(GOTDAGGER, true);

            break;

        case SPR_BND2:
            if (player->item < 100);
                player->item++;

            if (beta_style)
                player->message = GOTSKULLCHEST;
            else
                HU_PlayerMessage(GOTSKULLCHEST, true);

            break;

        case SPR_BON3:
            if (player->item < 100);
                player->item++;

            if (beta_style)
                player->message = GOTSCEPTRE;
            else
                HU_PlayerMessage(GOTSCEPTRE, true);

            break;

        case SPR_BON4:
            if (player->item < 100);
                player->item++;

            if (beta_style)
                player->message = GOTBIBLE;
            else
                HU_PlayerMessage(GOTBIBLE, true);

            break;

        default:
            C_Error("P_TouchSpecialThing: Unknown gettable thing");
            return;
    }

    if (special->flags & MF_COUNTITEM)
        player->itemcount++;

    P_RemoveMobj(special);
    player->bonuscount += BONUSADD;

    if (!d_sound || player == &players[consoleplayer])
        S_StartSound(NULL, sound);
}

//
// KillMobj
//
void P_KillMobj(mobj_t *source, mobj_t *target)
{
    mobjtype_t  item;
    mobjinfo_t  *info = &mobjinfo[target->type]; 
    mobj_t      *mo;
    int         minhealth;
    dboolean    e6y = false;
  
    if (target->player && source && target->health < -target->info->spawnhealth /*&&
            !demorecording && !demoplayback*/)
    {
        angle_t ang = R_PointToAngle2(target->x, target->y, source->x, source->y) - target->angle;
        e6y = (ang > (unsigned)(ANG180 - ANG45) && ang < (unsigned)(ANG180 + ANG45));
    }

    target->flags &= ~(MF_SHOOTABLE | MF_FLOAT | MF_SKULLFLY);
    target->flags2 &= ~MF2_PASSMOBJ;

    if (target->type != MT_SKULL && target->type != MT_BETASKULL)
        target->flags &= ~MF_NOGRAVITY;

    target->flags |= (MF_CORPSE | MF_DROPOFF);
    target->height >>= 2;

    // killough 8/29/98: remove from threaded list
    P_UpdateThinker(&target->thinker);

    if (target->type != MT_BARREL && d_maxgore)
    {
        if (!(target->flags & MF_SHADOW))
            target->bloodsplats = CORPSEBLOODSPLATS;

        if (d_flipcorpses && target->type != MT_CHAINGUY && target->type != MT_CYBORG)
        {
            static int prev;
            int        r = M_RandomInt(1, 10);

            if (r <= 5 + prev)
            {
                prev--;
                target->flags2 |= MF2_MIRRORED;

                if (target->shadow)
                    target->shadow->flags2 |= MF2_MIRRORED;
            }
            else
                prev++;
        }
    }

    if (source && source->player)
    {
        // count for intermission
        if (target->flags & MF_COUNTKILL)
            source->player->killcount++;        

        if (target->player)
            source->player->frags[target->player-players]++;

        if (beta_style && !massacre_cheat_used && (target->flags & MF_COUNTKILL))
        {
            int i;

            if (target->type == MT_POSSESSED)
                i = 200;

            if (target->type == MT_SHOTGUY)
                i = 400;

            if (target->type == MT_TROOP)
                i = 600;

            if (target->type == MT_CHAINGUY)
                i = 800;

            if (target->type == MT_SKULL || target->type == MT_BETASKULL)
                i = 1000;

            if (target->type == MT_SERGEANT)
                i = 1500;

            if (target->type == MT_KNIGHT)
                i = 2000;

            if (target->type == MT_BRUISER || target->type == MT_BETABRUISER)
                i = 2500;

            if (target->type == MT_BABY)
                i = 3000;

            if (target->type == MT_UNDEAD)
                i = 3500;

            if (target->type == MT_HEAD || target->type == MT_BETAHEAD)
                i = 4000;

            if (target->type == MT_PAIN)
                i = 4500;

            if (target->type == MT_FATSO)
                i = 5000;

            if (target->type == MT_VILE)
                i = 5500;

            if (target->type == MT_SPIDER)
                i = 10000;

            if (target->type == MT_CYBORG)
                i = 20000;

            if (source->player->score < 10000000)
                source->player->score += i;

            while (source->player->score >= source->player->nextextra)
            {
                source->player->nextextra += EXTRAPOINTS;
                source->player->extra_lifes += 1;

                if (source->player->score > 0 && source->player->score < EXTRAPOINTS)
                    source->player->extra_lifes = 0;
            }
        }
    }
    else if (!netgame && (target->flags & MF_COUNTKILL))
    {
        // count all monster deaths,
        // even those caused by other monsters
        players[0].killcount++;
    }
    
    if (target->player)
    {
        // count environment kills against you
        if (!source)        
            target->player->frags[target->player-players]++;
                        
        target->flags &= ~MF_SOLID;
        target->flags2 &= ~MF2_FLY;
        target->player->powers[pw_flight] = 0;
        target->player->playerstate = PST_DEAD;
        P_DropWeapon(target->player);

        if (target->player == &players[consoleplayer] && automapactive)
        {
            // don't die in auto map,
            // switch view prior to dying
            AM_Stop();
        }        
    }
    else
        target->flags2 &= ~MF2_NOLIQUIDBOB;

    if ((target->type == MT_BARREL || target->type == MT_PAIN ||
        target->type == MT_SKULL || target->type == MT_BETASKULL) && target->shadow)
        P_RemoveMobjShadow(target);

    minhealth = target->info->spawnhealth;

    // Make Lost Soul and Pain Elemental explosions translucent
    if (((target->type == MT_BETASKULL || target->type == MT_SKULL || target->type == MT_PAIN) && d_translucency) ||
        target->type == MT_BETABARREL || target->type == MT_BARREL)
        target->flags |= MF_TRANSLUCENT;

    // increase chance of gibbing
    if (e6y)
    {
        P_SetMobjState(target, S_PLAY_GDIE1);
    }
    else
    {
        int gibhealth = info->gibhealth;

        if (d_maxgore)
        {
            minhealth >>= 1;

            if (target->health < -minhealth && -target->info->spawnhealth 
                && target->info->xdeathstate && !beta_style)
            {
                // no spawning or flies on bodies in xdeathstate (feels more like Quake 2)
                target->effect_flies_can_spawn = false;
                P_SetMobjState(target, target->info->xdeathstate);
            }
            else
            {
                P_SetMobjState(target, target->info->deathstate);
            }
    
            if (target->type != MT_BARREL && target->type != MT_BETABARREL)
            {
                int t = P_Random() % 7;

                if (!snd_module && !target->state->dehacked)
                    S_StartSound(target, sfx_splsh0 + t);
            }
        }
        else
        {
            if (gibhealth < 0 && target->health < gibhealth && info->xdeathstate && !beta_style)
            {
                // no spawning or flies on bodies in xdeathstate (feels more like Quake 2)
                target->effect_flies_can_spawn = false;
                P_SetMobjState(target, info->xdeathstate);
            }
            else
                P_SetMobjState(target, info->deathstate);
        }
    }

    target->tics -= P_Random() & 3;

    // randomize corpse health
    if (!netgame)
        target->health -= target->tics & 1;

    if (target->tics < 1)
        target->tics = 1;
                
    //I_StartSound(&actor->r, actor->info->deathsound);

    // In Chex Quest, monsters don't drop items.

    if (gameversion == exe_chex || beta_style)
    {
        return;
    }

    // Drop stuff.
    // This determines the kind of object spawned
    // during the death frame of a thing.
    switch (target->type)
    {
        case MT_WOLFSS:
        case MT_BETAPOSSESSED:
        case MT_POSSESSED:
            item = MT_CLIP;
            break;
        
        case MT_BETASHOTGUY:
        case MT_SHOTGUY:
            item = MT_SHOTGUN;
            break;
        
        case MT_CHAINGUY:
            item = MT_CHAINGUN;
            break;
        
        default:
            return;
    }

    if (animated_drop)
    {
        mo = P_SpawnMobj(target->x, target->y, target->floorz + FRACUNIT * target->height / 2, item);
        mo->momx += P_Random() << 8;
        mo->momy += P_Random() << 8;
        mo->momz = FRACUNIT * 5 + (P_Random() << 10);
        mo->angle = target->angle + ((P_Random() - P_Random()) << 20);
    }
    else
        mo = P_SpawnMobj (target->x, target->y, ONFLOORZ, item);

    // special versions of items
    mo->flags |= MF_DROPPED;

    if ((rand() & 1) && d_flipcorpses)
    {
        mo->flags2 |= MF2_MIRRORED;

        if (mo->shadow)
            mo->shadow->flags2 |= MF2_MIRRORED;
    }
}

//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
void P_DamageMobj(mobj_t *target, mobj_t *inflictor, mobj_t *source, int damage)
{
    player_t    *splayer = NULL;
    player_t    *tplayer;
    int         flags = target->flags;
    dboolean    corpse = flags & MF_CORPSE; 
    int         type = target->type;
    mobjinfo_t  *info = &mobjinfo[type]; 

    if (!(flags & (MF_SHOOTABLE | MF_BOUNCES)) && (!corpse || !corpses_slide))
        // shouldn't happen...
        return;

    if (type == MT_BARREL && corpse)
        return;
/*
    if (target->health <= 0)
        return;
*/
    if (flags & MF_SKULLFLY)
        target->momx = target->momy = target->momz = 0;
        
    if (source)
        splayer = source->player;

    tplayer = target->player;

    if (tplayer && gameskill == sk_baby)
        // take half damage in trainer mode
        damage >>= (damage > 1);

    // Some close combat weapons should not
    // inflict thrust and push the victim out of reach,
    // thus kick away unless using the chainsaw.
    if (inflictor && !(flags & MF_NOCLIP) && (!source || !splayer
        || splayer->readyweapon != wp_chainsaw))
    {
        unsigned int    ang = R_PointToAngle2(inflictor->x, inflictor->y, target->x, target->y);
        fixed_t         thrust = damage * (FRACUNIT >> 3) * 100 / info->mass; 

        // make fall forwards sometimes
        if (damage < 40 && damage > target->health
            && target->z - inflictor->z > 64 * FRACUNIT && (P_Random() & 1))
        {
            ang += ANG180;
            thrust *= 4;
        }
                
        ang >>= ANGLETOFINESHIFT;
        target->momx += FixedMul(thrust, finecosine[ang]);
        target->momy += FixedMul(thrust, finesine[ang]);

        // killough 11/98: thrust objects hanging off ledges
        if ((target->flags2 & MF2_FALLING) && target->gear >= MAXGEAR)
            target->gear = 0;

        if (source)
        {
            int dist;
            int z;

            if (source == target)
            {
                viewx = inflictor->x;
                viewy = inflictor->y;
                z = inflictor->z;
            }
            else
            {
                viewx = source->x;
                viewy = source->y;
                z = source->z;
            }

            dist = R_PointToDist(target->x, target->y);

            if (target->flags2 & MF2_FEETARECLIPPED)
                z += FOOTCLIPSIZE;

            viewx = 0;
            viewy = z;
            ang = R_PointToAngle(dist, target->z);

            ang >>= ANGLETOFINESHIFT;
            target->momz += FixedMul(thrust, finesine[ang]);
        }
    }
    
    if (corpse)
        return;

    // player specific
    if (tplayer)
    {
        int     damagecount;

        // end of game hell hack
        if (target->subsector->sector->special == DamageNegative10Or20PercentHealthAndEndLevel
            && damage >= target->health)
            damage = target->health - 1;

        // Below certain threshold,
        // ignore damage in GOD mode, or with INVUL power.
        if ((tplayer->cheats & CF_GODMODE)
            || (damage < 1000 && tplayer->powers[pw_invulnerability]))
            return;

        // killough 3/26/98: make god mode 100% god mode in non-compat mode
        if ((damage < 1000 || (!d_god && (tplayer->cheats & CF_GODMODE))) &&
            ((tplayer->cheats & CF_GODMODE) || tplayer->powers[pw_invulnerability]))
            return;
        
        if (tplayer->armortype)
        {
            int saved = damage / (tplayer->armortype == GREENARMOR ? 3 : 2);
            
            if (tplayer->armorpoints <= saved)
            {
                // armor is used up
                saved = tplayer->armorpoints;
                tplayer->armortype = NOARMOR;
            }

            tplayer->armorpoints -= saved;
            damage -= saved;
        }

        // mirror mobj health here for Dave
        tplayer->health = MAX(0, tplayer->health - damage);
        
        tplayer->attacker = source;

        // add damage after armor / invuln
        damagecount = tplayer->damagecount + damage;

        // damagecount gets decremented before
        if (damage > 0 && damagecount < 2)
             // being used so needs to be at least 2
             damagecount = 2;

        // teleport stomp does 10k points...
        damagecount = MIN(damagecount, 100);

        tplayer->damagecount = damagecount;
    }
    
    // do the damage
    target->health -= damage;

    if (!(target->effects & FX_DRIP))
        target->effects |= FX_DRIP;

    if (target->health <= 0)
    {
        int gibhealth = info->gibhealth; 

        if (d_chkblood && d_colblood)
        {
            if (type == MT_BRUISER || type == MT_BETABRUISER || type == MT_KNIGHT)
                target->colfunc = redtogreencolfunc;
            else if (type == MT_BARREL || type == MT_PAIN || type == MT_SKULL || type == MT_BETASKULL)
                target->colfunc = tlredcolfunc;
        }

        if (target->effects & FX_DRIP)
            target->effects &= ~FX_DRIP;

        // [crispy] the lethal pellet of a point-blank SSG blast
        // gets an extra damage boost for the occasional gib chance
        if (splayer && splayer->readyweapon == wp_supershotgun && info->xdeathstate
            && P_CheckMeleeRange(target) && damage >= 10 && gibhealth < 0 && d_maxgore)
            target->health = gibhealth - 1;

        P_KillMobj(source, target);
        return;
    }

    if (P_Random() < info->painchance && !(flags & MF_SKULLFLY))
    {
        // fight back!
        target->flags |= MF_JUSTHIT;
        
        P_SetMobjState(target, info->painstate);
    }
                        
    // we're awake now...
    target->reactiontime = 0;

    if ((!target->threshold || type == MT_VILE)
        && source && source != target && source->type != MT_VILE)
    {
        // if not intent on another player,
        // chase after this one
        if (last_enemy)
        {
            if (!target->lastenemy || target->lastenemy->health <= 0 || !target->lastenemy->player)
                // remember last enemy - killough
                P_SetTarget(&target->lastenemy, target->target);
        }

        // killough 11/98
        P_SetTarget(&target->target, source);

        target->threshold = BASETHRESHOLD;

        if (target->state == &states[info->spawnstate] && info->seestate != S_NULL)
            P_SetMobjState(target, info->seestate);
    }
}

