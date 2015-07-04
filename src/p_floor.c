// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
//
// Copyright(C) 2015 by Brad Harding: - (Liquid Sector Animations)
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
//        Floor animation: raising stairs.
//
//-----------------------------------------------------------------------------


#include "c_io.h"
#include "doomdef.h"

// State.
#include "doomstat.h"

#include "p_local.h"
#include "r_state.h"
#include "s_sound.h"

// Data.
#include "sounds.h"

#include "v_trans.h"
#include "z_zone.h"


#define    STAIRS_UNINITIALIZED_CRUSH_FIELD_VALUE    10


fixed_t animatedliquiddiffs[128] =
{
     3211,  3211,  3211,  3211,  3180,  3180,  3119,  3119,
     3027,  3027,  2907,  2907,  2758,  2758,  2582,  2582,
     2382,  2382,  2159,  2159,  1915,  1915,  1653,  1653,
     1374,  1374,  1083,  1083,   781,   781,   471,   471,
      157,   157,  -157,  -157,  -471,  -471,  -781,  -781,
    -1083, -1083, -1374, -1374, -1653, -1653, -1915, -1915,
    -2159, -2159, -2382, -2382, -2582, -2582, -2758, -2758,
    -2907, -2907, -3027, -3027, -3119, -3119, -3180, -3180,
    -3211, -3211, -3211, -3211, -3180, -3180, -3119, -3119,
    -3027, -3027, -2907, -2907, -2758, -2758, -2582, -2582,
    -2382, -2382, -2159, -2159, -1915, -1915, -1653, -1653,
    -1374, -1374, -1083, -1083,  -781,  -781,  -471,  -471,
     -157,  -157,   157,   157,   471,   471,   781,   781,
     1083,  1083,  1374,  1374,  1653,  1653,  1915,  1915,
     2159,  2159,  2382,  2382,  2582,  2582,  2758,  2758,
     2907,  2907,  3027,  3027,  3119,  3119,  3180,  3180
};

static void T_AnimateLiquid(floormove_t *floor)
{
    sector_t    *sector = floor->sector;

    if (d_swirl && isliquid[sector->floorpic]
        && sector->ceiling_height != sector->floor_height)
    {
        if (sector->animate == INT_MAX)
            sector->animate = animatedliquiddiffs[leveltime & 127];
        else
            sector->animate += animatedliquiddiffs[leveltime & 127];
    }
    else
        sector->animate = INT_MAX;
}

static void P_StartAnimatedLiquid(sector_t *sector)
{
    thinker_t       *th;
    floormove_t     *floor;

    for (th = thinkercap.next; th != &thinkercap; th = th->next)
        if (th->function.acp1 == (actionf_p1) T_AnimateLiquid && ((floormove_t *)th)->sector == sector)
            return;

    floor = Z_Malloc(sizeof(*floor), PU_LEVSPEC, 0);
    memset(floor, 0, sizeof(*floor));
    P_AddThinker(&floor->thinker);
    floor->thinker.function.acp1 = (actionf_p1) T_AnimateLiquid;
    floor->sector = sector;
}

void P_InitAnimatedLiquids(void)
{
    int         i;
    sector_t    *sector;

    for (i = 0, sector = sectors; i < numsectors; i++, sector++)
    {
        sector->animate = INT_MAX;
        if (isliquid[sector->floorpic])
            P_StartAnimatedLiquid(sector);
    }
}

//
// FLOORS
//

//
// Move a plane (floor or ceiling) and check for crushing
//
result_e
T_MovePlane
( sector_t*      sector,
  fixed_t        speed,
  fixed_t        dest,
  boolean        crush,
  int            floorOrCeiling,
  int            direction )
{
    boolean      flag;
    fixed_t      lastpos;
    fixed_t      destheight; //jff 02/04/98 used to keep floors/ceilings
                             // from moving thru each other
        
    // [AM] Store old sector heights for interpolation.
    sector->oldfloorheight = sector->floor_height;
    sector->oldceilingheight = sector->ceiling_height;
    sector->oldgametic = gametic;

    switch(floorOrCeiling)
    {
      case 0:
        // FLOOR
        switch(direction)
        {
          case -1:
            // DOWN
            if (sector->floor_height - speed < dest)
            {
                lastpos = sector->floor_height;
                sector->floor_height = dest;
                flag = P_ChangeSector(sector,crush);
                if (flag == true)
                {
                    sector->floor_height =lastpos;
                    P_ChangeSector(sector,crush);
                    //return crushed;
                }
                return pastdest;
            }
            else
            {
                lastpos = sector->floor_height;
                sector->floor_height -= speed;
                flag = P_ChangeSector(sector,crush);
                if (flag == true && d_floors)
                {
                    sector->floor_height = lastpos;
                    P_ChangeSector(sector,crush);
                    //e6y: warning about potential desynch
                    if (crush == 10)
                    {
                        C_Printf(CR_GOLD, " T_MovePlane: Stairs which can potentially crush may lead to desynch in compatibility mode.\n");
                    }
                    return crushed;
                }
            }
            break;
                                                
          case 1:
            // UP
            destheight = (d_floors || dest < sector->ceiling_height)?
                          dest : sector->ceiling_height;
            if (sector->floor_height + speed > destheight)
            {
                lastpos = sector->floor_height;
                sector->floor_height = destheight;
                flag = P_ChangeSector(sector,crush);
                if (flag == true)
                {
                    sector->floor_height = lastpos;
                    P_ChangeSector(sector,crush);
                    //return crushed;
                }
                return pastdest;
            }
            else
            {
                // COULD GET CRUSHED
                lastpos = sector->floor_height;
                sector->floor_height += speed;
                flag = P_ChangeSector(sector,crush);
                if (flag == true)
                {
                    /* jff 1/25/98 fix floor crusher */
                    if (d_floors)
                    {
                        //e6y: warning about potential desynch
                        if (crush == STAIRS_UNINITIALIZED_CRUSH_FIELD_VALUE)
                        {
                            C_Printf(CR_GOLD, " T_MovePlane: Stairs which can potentially crush may lead to desynch.\n");
                        }
                        if (crush == true)
                            return crushed;
                    }
                    sector->floor_height = lastpos;
                    P_ChangeSector(sector,crush);      //jff 3/19/98 use faster chk
                    return crushed;
                }
            }
            break;
        }
        break;
                                                                        
      case 1:
        // CEILING
        switch(direction)
        {
          case -1:
            // DOWN
            destheight = (d_floors || dest > sector->floor_height)?
                          dest : sector->floor_height;
            if (sector->ceiling_height - speed < destheight)
            {
                lastpos = sector->ceiling_height;
                sector->ceiling_height = destheight;
                flag = P_ChangeSector(sector,crush);

                if (flag == true)
                {
                    sector->ceiling_height = lastpos;
                    P_ChangeSector(sector,crush);
                    //return crushed;
                }
                return pastdest;
            }
            else
            {
                // COULD GET CRUSHED
                lastpos = sector->ceiling_height;
                sector->ceiling_height -= speed;
                flag = P_ChangeSector(sector,crush);

                if (flag == true)
                {
                    if (crush == true)
                        return crushed;
                    sector->ceiling_height = lastpos;
                    P_ChangeSector(sector,crush);
                    return crushed;
                }
            }
            break;
                                                
          case 1:
            // UP
            if (sector->ceiling_height + speed > dest)
            {
                lastpos = sector->ceiling_height;
                sector->ceiling_height = dest;
                flag = P_ChangeSector(sector,crush);
                if (flag == true)
                {
                    sector->ceiling_height = lastpos;
                    P_ChangeSector(sector,crush);
                    //return crushed;
                }
                return pastdest;
            }
            else
            {
                lastpos = sector->ceiling_height;
                sector->ceiling_height += speed;
                flag = P_ChangeSector(sector,crush);
// UNUSED
#if 0
                if (flag == true)
                {
                    sector->ceiling_height = lastpos;
                    P_ChangeSector(sector,crush);
                    return crushed;
                }
#endif
            }
            break;
        }
        break;
                
    }
    return ok;
}


//
// MOVE A FLOOR TO IT'S DESTINATION (UP OR DOWN)
//
void T_MoveFloor(floormove_t* floor)
{
    result_e        res;
        
    res = T_MovePlane(floor->sector,
                      floor->speed,
                      floor->floordestheight,
                      floor->crush,0,floor->direction);
    
    if (!(leveltime&7))
        S_StartSound(&floor->sector->soundorg, sfx_stnmov);
    
    if (res == pastdest)
    {
        floor->sector->specialdata = NULL;

        if (floor->direction == 1)
        {
            switch(floor->type)
            {
              case donutRaise:
                floor->sector->special = floor->newspecial;
                floor->sector->floorpic = floor->texture;

                if (isliquid[floor->sector->floorpic])
                    P_StartAnimatedLiquid(floor->sector);

              default:
                break;
            }
        }
        else if (floor->direction == -1)
        {
            switch(floor->type)
            {
              case lowerAndChange:
                floor->sector->special = floor->newspecial;
                floor->sector->floorpic = floor->texture;

                if (isliquid[floor->sector->floorpic])
                    P_StartAnimatedLiquid(floor->sector);

              default:
                break;
            }
        }
        P_RemoveThinker(&floor->thinker);

        S_StartSound(&floor->sector->soundorg, sfx_pstop);
    }

}

//
// HANDLE FLOOR TYPES
//
int
EV_DoFloor
( line_t*        line,
  floor_e        floortype )
{
    int          secnum;
    int          rtn;
    int          i;
    sector_t*    sec;
    floormove_t* floor;

    secnum = -1;
    rtn = 0;
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];
                
        // ALREADY MOVING?  IF SO, KEEP GOING...
        if (sec->specialdata)
            continue;
        
        // new floor thinker
        rtn = 1;
        floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
        P_AddThinker (&floor->thinker);
        sec->specialdata = floor;
        floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
        floor->type = floortype;
        floor->crush = false;

        switch(floortype)
        {
          case lowerFloor:
            floor->direction = -1;
            floor->sector = sec;
            floor->speed = FLOORSPEED;
            floor->floordestheight = 
                P_FindHighestFloorSurrounding(sec);
            break;

          case lowerFloorToLowest:
            floor->direction = -1;
            floor->sector = sec;
            floor->speed = FLOORSPEED;
            floor->floordestheight = 
                P_FindLowestFloorSurrounding(sec);
            break;

          case turboLower:
            floor->direction = -1;
            floor->sector = sec;
            floor->speed = FLOORSPEED * 4;
            floor->floordestheight = 
                P_FindHighestFloorSurrounding(sec);
            if (floor->floordestheight != sec->floor_height)
                floor->floordestheight += 8*FRACUNIT;
            break;

          case raiseFloorCrush:
            floor->crush = true;
          case raiseFloor:
            floor->direction = 1;
            floor->sector = sec;
            floor->speed = FLOORSPEED;
            floor->floordestheight = 
                P_FindLowestCeilingSurrounding(sec);
            if (floor->floordestheight > sec->ceiling_height)
                floor->floordestheight = sec->ceiling_height;
            floor->floordestheight -= (8*FRACUNIT)*
                (floortype == raiseFloorCrush);
            break;

          case raiseFloorTurbo:
            floor->direction = 1;
            floor->sector = sec;
            floor->speed = FLOORSPEED*4;
            floor->floordestheight = 
                P_FindNextHighestFloor(sec,sec->floor_height);
            break;

          case raiseFloorToNearest:
            floor->direction = 1;
            floor->sector = sec;
            floor->speed = FLOORSPEED;
            floor->floordestheight = 
                P_FindNextHighestFloor(sec,sec->floor_height);
            break;

          case raiseFloor24:
            floor->direction = 1;
            floor->sector = sec;
            floor->speed = FLOORSPEED;
            floor->floordestheight = floor->sector->floor_height +
                24 * FRACUNIT;
            break;
          case raiseFloor512:
            floor->direction = 1;
            floor->sector = sec;
            floor->speed = FLOORSPEED;
            floor->floordestheight = floor->sector->floor_height +
                512 * FRACUNIT;
            break;

          case raiseFloor24AndChange:
            floor->direction = 1;
            floor->sector = sec;
            floor->speed = FLOORSPEED;
            floor->floordestheight = floor->sector->floor_height +
                24 * FRACUNIT;
            sec->floorpic = line->frontsector->floorpic;
            sec->special = line->frontsector->special;
            break;

          case raiseToTexture:
          {
              int        minsize = INT_MAX;
              side_t*        side;
                                
              /* jff 3/13/98 no ovf */
              if (!d_model)
                  minsize = 32000<<FRACBITS;

              floor->direction = 1;
              floor->sector = sec;
              floor->speed = FLOORSPEED;
              for (i = 0; i < sec->linecount; i++)
              {
                  if (twoSided (secnum, i) )
                  {
                      side = getSide(secnum,i,0);
                      // jff 8/14/98 don't scan texture 0, its not real
                      if (side->bottomtexture >= 0 ||
                              (d_model && !side->bottomtexture))
                          if (textureheight[side->bottomtexture] < 
                              minsize)
                              minsize = 
                                  textureheight[side->bottomtexture];
                      side = getSide(secnum,i,1);
                      // jff 8/14/98 don't scan texture 0, its not real
                      if (side->bottomtexture >= 0 ||
                              (d_model && !side->bottomtexture))
                          if (textureheight[side->bottomtexture] < 
                              minsize)
                              minsize = 
                                  textureheight[side->bottomtexture];
                  }
              }
              if (d_model)
                  floor->floordestheight = floor->sector->floor_height + minsize;
              else
              {
                  floor->floordestheight =
                          (floor->sector->floor_height>>FRACBITS) + (minsize >> FRACBITS);
                  if (floor->floordestheight > 32000)
                      floor->floordestheight = 32000;      //jff 3/13/98 do not
                  floor->floordestheight <<= FRACBITS;     // allow height overflow
              }
          }
          break;
          
          case lowerAndChange:
            floor->direction = -1;
            floor->sector = sec;
            floor->speed = FLOORSPEED;
            floor->floordestheight = 
                P_FindLowestFloorSurrounding(sec);
            floor->texture = sec->floorpic;

            for (i = 0; i < sec->linecount; i++)
            {
                if ( twoSided(secnum, i) )
                {
                    if (getSide(secnum,i,0)->sector-sectors == secnum)
                    {
                        // DEACTIVATED THIS PREVIOUSLY INTRODUCED BUG-FIX DUE TO THE FACT THAT THE REAL
                        // CAUSE OF THIS CRASH DOESN'T REALLY SEEM TO COME FROM THIS FUNCTION. IT LOOKS
                        // LIKE IT'S BEING CAUSED BY THE CALL TO THE "I_ERROR" FUNCTION IN THE
                        // "DEFAULT" CASE OF FUNCTION "P_PlayerInSpecialSector" IN FILE "P_SPEC.C"
                        // WHICH CALL HAS NOW BEEN DEACTIVATED AND IT LOOKS LIKE THIS IS WORKING IN
                        // MOST OF THE CASES THAT HAPPENED TO ME WHILE TESTING THIS PORT ON THE WII.
/*
                        // HACK: THIS "IF"-CONDITION HAS BEEN INSERTED AS A RESULT THAT THE DOOM IWAD's
                        // FROM v1.1 UP TO v1.6 HAVE A BUG ON EPISODE 3 - MAP 1 WHERE THE MAP's LINE
                        // WITH NUMBER 50 TRIGGERS TWO SECTORS INSTEAD OF JUST ONE SECTOR THAT IT
                        // SHOULD. v1.666 IS THE FIRST VERSION OF THE DOOM IWAD WHICH DOESN'T HAVE THIS
                        // BUG ANY MORE. SO THE SECTORS BEING TRIGGERED ARE THE SECTORS WITH NUMBER 8
                        // AND 18. WHILE SECTOR 18 MAKES SENSE, SECTOR 8 DOESN'T. WITHOUT THIS
                        // "IF"-CONDITION, THE LINE WITH NUMBER 50 CAUSES SECTORS 8 AND 18 TO LOWER TO
                        // THE NEXT ADJACENT FLOOR TEXTURE AND TYPE. SECTOR 8 IS ALMOST LOWERED WHEN
                        // STARTING THE MAP, SO IT DOESN'T MAKE ANY SENSE WHY TO LOWER THIS SECTOR WHEN
                        // CROSSING LINE NUMBER 50. OTHERWISE AND IN THE END AS SOON AS YOU WALK INTO
                        // SECTOR 8 IN THE LOWERED STATE, THE GAME USUALLY CRASHES.
                        if(gamemode == registered && gameepisode == 3 && gamemap == 1 && secnum != 8)
*/
                            sec = getSector(secnum,i,1);

                        if (sec->floor_height == floor->floordestheight)
                        {
                            floor->texture = sec->floorpic;
                            floor->newspecial = sec->special;
                            break;
                        }
                    }
                    else
                    {
                        sec = getSector(secnum,i,0);

                        if (sec->floor_height == floor->floordestheight)
                        {
                            floor->texture = sec->floorpic;
                            floor->newspecial = sec->special;
                            break;
                        }
                    }
                }
            }
          default:
            break;
        }
    }
    return rtn;
}




//
// BUILD A STAIRCASE!
//
int
EV_BuildStairs
( line_t*        line,
  stair_e        type )
{
    int          secnum;
    int          height;
    int          i;
    int          newsecnum;
    int          texture;
    int          ok;
    int          rtn;
    
    sector_t*    sec;
    sector_t*    tsec;

    floormove_t* floor;
    
    fixed_t      stairsize = 0;
    fixed_t      speed = 0;

    secnum = -1;
    rtn = 0;
    while ((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0)
    {
        sec = &sectors[secnum];
                
        // ALREADY MOVING?  IF SO, KEEP GOING...
        if (sec->specialdata)
            continue;
        
        // new floor thinker
        rtn = 1;
        floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);
        P_AddThinker (&floor->thinker);
        sec->specialdata = floor;
        floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
        floor->direction = 1;
        floor->sector = sec;
        switch(type)
        {
          case build8:
            speed = FLOORSPEED/4;
            stairsize = 8*FRACUNIT;
            break;
          case turbo16:
            speed = FLOORSPEED*4;
            stairsize = 16*FRACUNIT;
            break;
        }
        floor->speed = speed;
        height = sec->floor_height + stairsize;
        floor->floordestheight = height;
                
        // Initialize
        floor->type = lowerFloor;
        floor->crush = true;

        texture = sec->floorpic;
        
        // Find next sector to raise
        // 1.        Find 2-sided line with same sector side[0]
        // 2.        Other side is the next sector to raise
        do
        {
            ok = 0;
            for (i = 0;i < sec->linecount;i++)
            {
                if ( !((sec->lines[i])->flags & ML_TWOSIDED) )
                    continue;
                                        
                tsec = (sec->lines[i])->frontsector;
                newsecnum = tsec-sectors;
                
                if (secnum != newsecnum)
                    continue;

                tsec = (sec->lines[i])->backsector;
                newsecnum = tsec - sectors;

                if (tsec->floorpic != texture)
                    continue;
                                        
                height += stairsize;

                if (tsec->specialdata)
                    continue;
                                        
                sec = tsec;
                secnum = newsecnum;
                floor = Z_Malloc (sizeof(*floor), PU_LEVSPEC, 0);

                P_AddThinker (&floor->thinker);

                sec->specialdata = floor;
                floor->thinker.function.acp1 = (actionf_p1) T_MoveFloor;
                floor->direction = 1;
                floor->sector = sec;
                floor->speed = speed;
                floor->floordestheight = height;

                // Initialize
                floor->type = lowerFloor;
                floor->crush = true;

                ok = 1;
                break;
            }
        } while(ok);
    }
    return rtn;
}

