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
//        Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------


#include <stdlib.h>

#include "am_map.h"
#include "deh_misc.h"
#include "deh_str.h"

// Data.
#include "doomdef.h"

#include "doomstat.h"
#include "dstrings.h"
#include "i_system.h"
#include "m_random.h"
#include "p_inter.h"
#include "p_local.h"
#include "s_sound.h"
#include "sounds.h"
#include "st_stuff.h"


#define BONUSADD        6


// a weapon is found with two clip loads,
// a big item has five clip loads
int                     maxammo[NUMAMMO] = {200, 50, 300, 50};
int                     clipammo[NUMAMMO] = {10, 4, 20, 1};
int                     cardsfound;

extern boolean          massacre_cheat_used;


//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

boolean
P_GiveAmmo
( player_t*        player,
  ammotype_t        ammo,
  int                num )
{
    int                oldammo;
        
    if (ammo == am_noammo)
        return false;
                
    if (ammo > NUMAMMO)
        I_Error ("P_GiveAmmo: bad type %i", ammo);
                
    if ( player->ammo[ammo] == player->maxammo[ammo]  )
        return false;
                
    if (num)
        num *= clipammo[ammo];
    else
        num = clipammo[ammo]/2;
    
    if (gameskill == sk_baby
        || gameskill == sk_nightmare)
    {
        // give double ammo in trainer mode,
        // you'll need in nightmare
        num <<= 1;
    }
    
                
    oldammo = player->ammo[ammo];
    player->ammo[ammo] += num;

    if (player->ammo[ammo] > player->maxammo[ammo])
        player->ammo[ammo] = player->maxammo[ammo];

    // If non zero ammo, 
    // don't change up weapons,
    // player was lower on purpose.
    if (oldammo)
        return true;        

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
        if (player->readyweapon == wp_fist
            || player->readyweapon == wp_pistol)
        {
            if (player->weaponowned[wp_shotgun])
                player->pendingweapon = wp_shotgun;
        }
        break;
        
      case am_cell:
        if (player->readyweapon == wp_fist
            || player->readyweapon == wp_pistol)
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
        
    return true;
}


//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
boolean
P_GiveWeapon
( player_t*        player,
  weapontype_t        weapon,
  boolean        dropped )
{
    boolean        gaveammo;
    boolean        gaveweapon;

    if (netgame
        && (deathmatch!=2)
         && !dropped )
    {
        // leave placed weapons forever on net games
        if (player->weaponowned[weapon])
            return false;

        player->bonuscount += BONUSADD;
        player->weaponowned[weapon] = true;

        if (deathmatch)
            P_GiveAmmo (player, weaponinfo[weapon].ammo, 5);
        else
            P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);
        player->pendingweapon = weapon;

        if (player == &players[consoleplayer])
            S_StartSound (NULL, sfx_wpnup);
        return false;
    }

    if (weaponinfo[weapon].ammo != am_noammo)
    {
        // give one clip with a dropped weapon,
        // two clips with a found weapon
        if (dropped)
            gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 1);
        else
            gaveammo = P_GiveAmmo (player, weaponinfo[weapon].ammo, 2);
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
        
    return (gaveweapon || gaveammo);
}

 

//
// P_GiveBody
// Returns false if the body isn't needed at all
//
boolean
P_GiveBody
( player_t*        player,
  int                num )
{
    if (player->health >= MAXHEALTH)
        return false;
                
    player->health += num;
    if (player->health > MAXHEALTH)
        player->health = MAXHEALTH;
    player->mo->health = player->health;
        
    return true;
}



//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
boolean
P_GiveArmor
( player_t*        player,
  int                armortype )
{
    int                hits;
        
    hits = armortype*100;
    if (player->armorpoints >= hits)
        return false;        // don't pick up
                
    player->armortype = armortype;
    player->armorpoints = hits;
        
    return true;
}


//
// P_InitCards
//
void P_InitCards(player_t *player)
{
    int i;

    for (i = 0; i < NUMCARDS; i++)
        player->cards[i] = 0;

    cardsfound = 0;

    for (i = 0; i < numsectors; i++)
    {
        mobj_t  *thing = sectors[i].thinglist;

        while (thing)
        {
            switch (thing->sprite)
            {
                case SPR_BKEY:
                    player->cards[it_bluecard] = 0;
                    break;
                case SPR_RKEY:
                    player->cards[it_redcard] = 0;
                    break;
                case SPR_YKEY:
                    player->cards[it_yellowcard] = 0;
                    break;
                case SPR_BSKU:
                case SPR_BBSK:
                    player->cards[it_blueskull] = 0;
                    break;
                case SPR_RSKU:
                case SPR_BRSK:
                    player->cards[it_redskull] = 0;
                    break;
                case SPR_YSKU:
                case SPR_BYSK:
                    player->cards[it_yellowskull] = 0;
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
            case 26:
            case 32:
            case 99:
            case 133:
                if (!player->cards[it_blueskull])
                    player->cards[it_bluecard] = 0;
                break;
            case 28:
            case 33:
            case 134:
            case 135:
                if (!player->cards[it_redskull])
                    player->cards[it_redcard] = 0;
                break;
            case 27:
            case 34:
            case 136:
            case 137:
                if (!player->cards[it_yellowskull])
                    player->cards[it_yellowcard] = 0;
                break;
        }
    }
}


//
// P_GiveCard
//
void
P_GiveCard
( player_t*        player,
  card_t        card )
{
    if (player->cards[card])
        return;
    
    player->bonuscount = BONUSADD;
    player->cards[card] = ++cardsfound;

    if (card == player->neededcard)
        player->neededcard = player->neededcardflash = 0;
}


//
// P_GivePower
//
boolean
P_GivePower
( player_t*        player,
  int /*powertype_t*/        power )
{
    if (power == pw_invulnerability)
    {
        player->powers[power] = INVULNTICS;
        return true;
    }
    
    if (power == pw_invisibility)
    {
        if(beta_style)
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
        P_GiveBody (player, 100);
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
            player->flyheight = 10;     // thrust the player in the air a bit
        }
        return (true);
    }

    if (player->powers[power])
        return false;        // already got it
                
    player->powers[power] = 1;
    return true;
}



//
// P_TouchSpecialThing
//
void
P_TouchSpecialThing
( mobj_t*        special,
  mobj_t*        toucher )
{
    player_t*        player;
    int                i;
    fixed_t        delta;
    int                sound;
                
    delta = special->z - toucher->z;

    if (delta > toucher->height
        || delta < -8*FRACUNIT)
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
        if (!P_GiveArmor (player, deh_green_armor_class))
            return;
        player->message = DEH_String(GOTARMOR);
        break;
                
      case SPR_ARM2:
        if (!P_GiveArmor (player, deh_blue_armor_class))
            return;
        player->message = DEH_String(GOTMEGA);
        break;
        
        // bonus items
      case SPR_BON1:
        player->health++;                // can go over 100%
        if (player->health > deh_max_health)
            player->health = deh_max_health;
        player->mo->health = player->health;
        player->message = DEH_String(GOTHTHBONUS);
        break;
        
      case SPR_BON2:
        player->armorpoints++;                // can go over 100%
        if (player->armorpoints > deh_max_armor)
            player->armorpoints = deh_max_armor;
        // deh_green_armor_class only applies to the green armor shirt;
        // for the armor helmets, armortype 1 is always used.
        if (!player->armortype)
            player->armortype = 1;
        player->message = DEH_String(GOTARMBONUS);
        break;
        
      case SPR_SOUL:
        if(beta_style)
        {
            player->health = 100;
            player->mo->health = player->health;
            player->extra_lifes++;
            ST_doRefresh();
        }
        else
        {
            player->health += deh_soulsphere_health;
            if (player->health > deh_max_soulsphere)
                player->health = deh_max_soulsphere;
            player->mo->health = player->health;
            player->message = DEH_String(GOTSUPER);
        }

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
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
        player->health = deh_megasphere_health;
        player->mo->health = player->health;
        // We always give armor type 2 for the megasphere; dehacked only 
        // affects the MegaArmor.
        P_GiveArmor (player, 2);
        player->message = DEH_String(GOTMSPHERE);

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
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
        if (!player->cards[it_bluecard])
            player->message = DEH_String(GOTBLUECARD);
        P_GiveCard (player, it_bluecard);
        if (!netgame)
            break;
        return;
        
      case SPR_YKEY:
        if (!player->cards[it_yellowcard])
            player->message = DEH_String(GOTYELWCARD);
        P_GiveCard (player, it_yellowcard);
        if (!netgame)
            break;
        return;
        
      case SPR_RKEY:
        if (!player->cards[it_redcard])
            player->message = DEH_String(GOTREDCARD);
        P_GiveCard (player, it_redcard);
        if (!netgame)
            break;
        return;
        
      case SPR_BSKU:
      case SPR_BBSK:
        if (!player->cards[it_blueskull])
            player->message = DEH_String(GOTBLUESKUL);
        P_GiveCard (player, it_blueskull);
        if (!netgame)
            break;
        return;
        
      case SPR_YSKU:
      case SPR_BYSK:
        if (!player->cards[it_yellowskull])
            player->message = DEH_String(GOTYELWSKUL);
        P_GiveCard (player, it_yellowskull);
        if (!netgame)
            break;
        return;
        
      case SPR_RSKU:
      case SPR_BRSK:
        if (!player->cards[it_redskull])
            player->message = DEH_String(GOTREDSKULL);
        P_GiveCard (player, it_redskull);
        if (!netgame)
            break;
        return;
        
        // medikits, heals
      case SPR_STIM:
        if (!P_GiveBody (player, 10))
            return;
        player->message = DEH_String(GOTSTIM);
        break;
        
      case SPR_MEDI:
        if (!P_GiveBody (player, 25))
            return;

        if (player->health < 25)
            player->message = DEH_String(GOTMEDINEED);
        else
            player->message = DEH_String(GOTMEDIKIT);
        break;

        
        // power ups
      case SPR_PINV:
      case SPR_BPNV:
        if (!P_GivePower (player, pw_invulnerability))
            return;
        player->message = DEH_String(GOTINVUL);

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
            sound = sfx_getpow;
        else
            sound = sfx_itemup;
        break;
        
      case SPR_PSTR:
        if (!P_GivePower (player, pw_strength))
            return;
        player->message = DEH_String(GOTBERSERK);
        if (player->readyweapon != wp_fist)
            player->pendingweapon = wp_fist;

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
            sound = sfx_getpow;
        else
            sound = sfx_itemup;
        break;
        
      case SPR_PINS:
      case SPR_BPNS:
        if (!P_GivePower (player, pw_invisibility))
            return;
        player->message = DEH_String(GOTINVIS);

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
            sound = sfx_getpow;
        else
            sound = sfx_itemup;
        break;
        
      case SPR_SUIT:
        if (!P_GivePower (player, pw_ironfeet))
            return;
        player->message = DEH_String(GOTSUIT);

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
            sound = sfx_getpow;
        else
            sound = sfx_itemup;
        break;
        
      case SPR_PMAP:
        if (!P_GivePower (player, pw_allmap))
            return;
        player->message = DEH_String(GOTMAP);

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
            sound = sfx_getpow;
        else
            sound = sfx_itemup;
        break;
        
      case SPR_PVIS:
        if (!P_GivePower (player, pw_infrared))
            return;
        player->message = DEH_String(GOTVISOR);

        if(fsize != 10396254 && fsize != 10399316 && fsize != 10401760 &&
                fsize != 4261144 && fsize != 4271324 && fsize != 4211660 &&
                fsize != 4207819 && fsize != 4274218 && fsize != 4225504 &&
                fsize != 4225460)
            sound = sfx_getpow;
        else
            sound = sfx_itemup;
        break;
        
        // ammo
      case SPR_CLIP:
        if (special->flags & MF_DROPPED)
        {
            if (!P_GiveAmmo (player,am_clip,0))
                return;
        }
        else
        {
            if (!P_GiveAmmo (player,am_clip,1))
                return;
        }
        player->message = DEH_String(GOTCLIP);
        break;
        
      case SPR_AMMO:
        if (!P_GiveAmmo (player, am_clip,5))
            return;
        player->message = DEH_String(GOTCLIPBOX);
        break;
        
      case SPR_ROCK:
        if (!P_GiveAmmo (player, am_misl,1))
            return;
        player->message = DEH_String(GOTROCKET);
        break;
        
      case SPR_BROK:
        if (!P_GiveAmmo (player, am_misl,5))
            return;
        player->message = DEH_String(GOTROCKBOX);
        break;
        
      case SPR_CELL:
      case SPR_BCLL:
        if (!P_GiveAmmo (player, am_cell,1))
            return;
        player->message = DEH_String(GOTCELL);
        break;
        
      case SPR_CELP:
        if (!P_GiveAmmo (player, am_cell,5))
            return;
        player->message = DEH_String(GOTCELLBOX);
        break;
        
      case SPR_SHEL:
      case SPR_BSHL:
        if (!P_GiveAmmo (player, am_shell,1))
            return;
        player->message = DEH_String(GOTSHELLS);
        break;
        
      case SPR_SBOX:
      case SPR_BBOX:
        if (!P_GiveAmmo (player, am_shell,5))
            return;
        player->message = DEH_String(GOTSHELLBOX);
        break;
        
      case SPR_BPAK:
        if (!player->backpack)
        {
            for (i=0 ; i<NUMAMMO ; i++)
                player->maxammo[i] *= 2;
            player->backpack = true;
        }
        for (i=0 ; i<NUMAMMO ; i++)
            P_GiveAmmo (player, i, 1);
        player->message = DEH_String(GOTBACKPACK);
        break;
        
        // weapons
      case SPR_BFUG:
        if (!P_GiveWeapon (player, wp_bfg, false) )
            return;
        player->message = DEH_String(GOTBFG9000);
        sound = sfx_wpnup;        
        break;
        
      case SPR_MGUN:
        if (!P_GiveWeapon (player, wp_chaingun,
            (special->flags & MF_DROPPED) != 0) )
            return;
        player->message = DEH_String(GOTCHAINGUN);
        sound = sfx_wpnup;        
        break;
        
      case SPR_CSAW:
        if (!P_GiveWeapon (player, wp_chainsaw, false) )
            return;
        player->message = DEH_String(GOTCHAINSAW);
        sound = sfx_wpnup;        
        break;
        
      case SPR_LAUN:
        if (!P_GiveWeapon (player, wp_missile, false) )
            return;
        player->message = DEH_String(GOTLAUNCHER);
        sound = sfx_wpnup;        
        break;
        
      case SPR_PLAS:
        if (!P_GiveWeapon (player, wp_plasma, false) )
            return;
        player->message = DEH_String(GOTPLASMA);
        sound = sfx_wpnup;        
        break;
        
      case SPR_SHOT:
        if (!P_GiveWeapon (player, wp_shotgun,
            (special->flags & MF_DROPPED) != 0) )
            return;
        player->message = DEH_String(GOTSHOTGUN);
        sound = sfx_wpnup;        
        break;
                
      case SPR_SGN2:
        if (!P_GiveWeapon (player, wp_supershotgun,
            (special->flags & MF_DROPPED) != 0) )
            return;
        player->message = DEH_String(GOTSHOTGUN2);
        sound = sfx_wpnup;        
        break;
                
      case SPR_BND1:
        if(player->item < 100);
            player->item++;
        player->message = DEH_String(GOTDAGGER);
        break;

      case SPR_BND2:
        if(player->item < 100);
            player->item++;
        player->message = DEH_String(GOTSKULLCHEST);
        break;

      case SPR_BND3:
        if(player->item < 100);
            player->item++;
        player->message = DEH_String(GOTSCEPTRE);
        break;

      case SPR_BND4:
        if(player->item < 100);
            player->item++;
        player->message = DEH_String(GOTBIBLE);
        break;

      default:
        I_Error ("P_SpecialThing: Unknown gettable thing");
    }
        
    if (special->flags & MF_COUNTITEM)
        player->itemcount++;
    P_RemoveMobj (special);
    player->bonuscount += BONUSADD;
    if (player == &players[consoleplayer])
        S_StartSound (NULL, sound);
}


//
// KillMobj
//
void
P_KillMobj
( mobj_t*        source,
  mobj_t*        target )
{
    mobjtype_t   item;
    mobj_t*      mo;
    int          t;
    int          i;

    target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);
    target->flags2 &= ~MF2_PASSMOBJ;

    if (target->type != MT_SKULL && target->type != MT_BETASKULL)
        target->flags &= ~MF_NOGRAVITY;

    target->flags |= MF_CORPSE|MF_DROPOFF;
    target->height >>= 2;

    if (source && source->player)
    {
        // count for intermission
        if (target->flags & MF_COUNTKILL)
            source->player->killcount++;        

        if (target->player)
            source->player->frags[target->player-players]++;

        if (beta_style && !massacre_cheat_used && target->flags & MF_COUNTKILL)
        {
            if(target->type == MT_POSSESSED)
                i = 200;

            if(target->type == MT_SHOTGUY)
                i = 400;

            if(target->type == MT_TROOP)
                i = 600;

            if(target->type == MT_CHAINGUY)
                i = 800;

            if(target->type == MT_SKULL || target->type == MT_BETASKULL)
                i = 1000;

            if(target->type == MT_SERGEANT)
                i = 1500;

            if(target->type == MT_KNIGHT)
                i = 2000;

            if(target->type == MT_BRUISER || target->type == MT_BETABRUISER)
                i = 2500;

            if(target->type == MT_BABY)
                i = 3000;

            if(target->type == MT_UNDEAD)
                i = 3500;

            if(target->type == MT_HEAD || target->type == MT_BETAHEAD)
                i = 4000;

            if(target->type == MT_PAIN)
                i = 4500;

            if(target->type == MT_FATSO)
                i = 5000;

            if(target->type == MT_VILE)
                i = 5500;

            if(target->type == MT_SPIDER)
                i = 10000;

            if(target->type == MT_CYBORG)
                i = 20000;

            if(source->player->score < 10000000)
                source->player->score += i;

            while (source->player->score >= source->player->nextextra)
            {
                source->player->nextextra += EXTRAPOINTS;
                source->player->extra_lifes += 1;

                if (source->player->score > 0 && source->player->score < EXTRAPOINTS)
                    source->player->extra_lifes = 0;

                ST_doRefresh();
            }
        }
    }
    else if (!netgame && (target->flags & MF_COUNTKILL) )
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
        P_DropWeapon (target->player);

        if (target->player == &players[consoleplayer]
            && automapactive)
        {
            // don't die in auto map,
            // switch view prior to dying
            AM_Stop ();
        }
        
    }
    else
        target->flags2 &= ~MF2_NOFLOATBOB;

    int minhealth = target->info->spawnhealth;

    // Make Lost Soul and Pain Elemental explosions translucent
    if (target->type == MT_BETASKULL ||
        target->type == MT_SKULL ||
        target->type == MT_PAIN ||
        target->type == MT_BETABARREL ||
        target->type == MT_BARREL)
        target->flags |= MF_TRANSLUCENT;

    // increase chance of gibbing
    if(d_maxgore)
    {
        minhealth >>= 1;

        if (target->health < -minhealth && -target->info->spawnhealth 
            && target->info->xdeathstate && !beta_style)
        {
            P_SetMobjState (target, target->info->xdeathstate);
        }
        else
        {
            P_SetMobjState (target, target->info->deathstate);
        }

        if(target->type != MT_BARREL && target->type != MT_BETABARREL)
        {
            t = P_Random() % 7;

            S_StartSound(target, sfx_splsh0 + t);
        }
    }
    else
    {
        if (target->health < -target->info->spawnhealth 
            && target->info->xdeathstate && !beta_style)
        {
            P_SetMobjState (target, target->info->xdeathstate);
        }
        else
            P_SetMobjState (target, target->info->deathstate);
    }

    target->tics -= P_Random()&3;

    // randomize corpse health
    if (!netgame)
        target->health -= target->tics & 1;

    if (target->tics < 1)
        target->tics = 1;
                
    //        I_StartSound (&actor->r, actor->info->deathsound);

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

    mo = P_SpawnMobj (target->x,target->y,ONFLOORZ, item);
    mo->flags |= MF_DROPPED;        // special versions of items

    if ((rand() & 1))
        mo->flags2 |= MF2_MIRRORED;
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
void
P_DamageMobj
( mobj_t*        target,
  mobj_t*        inflictor,
  mobj_t*        source,
  int                 damage )
{
    unsigned        ang;
    int                saved;
    player_t*        player;
    fixed_t        thrust;
    int                temp;

    if ( !(target->flags & MF_SHOOTABLE) )
        return;        // shouldn't happen...
                
    if (target->health <= 0)
        return;

    if ( target->flags & MF_SKULLFLY )
    {
        target->momx = target->momy = target->momz = 0;
    }
        
    player = target->player;
    if (player && gameskill == sk_baby)
        damage >>= 1;         // take half damage in trainer mode
                

    // Some close combat weapons should not
    // inflict thrust and push the victim out of reach,
    // thus kick away unless using the chainsaw.
    if (inflictor
        && !(target->flags & MF_NOCLIP)
        && (!source
            || !source->player
            || source->player->readyweapon != wp_chainsaw))
    {
        ang = R_PointToAngle2 ( inflictor->x,
                                inflictor->y,
                                target->x,
                                target->y);
                
        thrust = damage*(FRACUNIT>>3)*100/target->info->mass;

        // make fall forwards sometimes
        if ( damage < 40
             && damage > target->health
             && target->z - inflictor->z > 64*FRACUNIT
             && (P_Random ()&1) )
        {
            ang += ANG180;
            thrust *= 4;
        }
                
        ang >>= ANGLETOFINESHIFT;
        target->momx += FixedMul (thrust, finecosine[ang]);
        target->momy += FixedMul (thrust, finesine[ang]);

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
    
    // player specific
    if (player)
    {
        // end of game hell hack
        if (target->subsector->sector->special == 11
            && damage >= target->health)
        {
            damage = target->health - 1;
        }
        

        // Below certain threshold,
        // ignore damage in GOD mode, or with INVUL power.
        if ( damage < 1000
             && ( (player->cheats&CF_GODMODE)
                  || player->powers[pw_invulnerability] ) )
        {
            return;
        }
        
        if (player->armortype)
        {
            if (player->armortype == 1)
                saved = damage/3;
            else
                saved = damage/2;
            
            if (player->armorpoints <= saved)
            {
                // armor is used up
                saved = player->armorpoints;
                player->armortype = 0;
            }
            player->armorpoints -= saved;
            damage -= saved;
        }
        player->health -= damage;         // mirror mobj health here for Dave
        if (player->health < 0)
            player->health = 0;
        
        player->attacker = source;
        player->damagecount += damage;        // add damage after armor / invuln

        if (player->damagecount > 100)
            player->damagecount = 100;        // teleport stomp does 10k points...
        
        temp = damage < 100 ? damage : 100;

        if (player == &players[consoleplayer])
            I_Tactile (40,10,40+temp*2);
    }
    
    // do the damage        
    target->health -= damage;        
    if (target->health <= 0)
    {
        P_KillMobj (source, target);
        return;
    }

    if ( (P_Random () < target->info->painchance)
         && !(target->flags&MF_SKULLFLY) )
    {
        target->flags |= MF_JUSTHIT;        // fight back!
        
        P_SetMobjState (target, target->info->painstate);
    }
                        
    target->reactiontime = 0;                // we're awake now...        

    if ( (!target->threshold || target->type == MT_VILE)
         && source && source != target
         && source->type != MT_VILE)
    {
        // if not intent on another player,
        // chase after this one
        target->target = source;
        target->threshold = BASETHRESHOLD;
        if (target->state == &states[target->info->spawnstate]
            && target->info->seestate != S_NULL)
            P_SetMobjState (target, target->info->seestate);
    }
}

