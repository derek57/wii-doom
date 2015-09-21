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
//        Teleportation.
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "doomstat.h"
#include "p_local.h"

// State.
#include "r_state.h"

#include "s_sound.h"

// Data.
#include "sounds.h"


//
// TELEPORTATION
//
int
EV_Teleport
( line_t*        line,
  int            side,
  mobj_t*        thing )
{
    int          i;
    int          tag;
    mobj_t*      m;
    mobj_t*      fog;
    unsigned     an;
    thinker_t*   thinker;
    sector_t*    sector;
    fixed_t      oldx;
    fixed_t      oldy;
    fixed_t      oldz;
    fixed_t      aboveFloor = thing->z - thing->floorz;

    if (thing->flags2 & MF2_NOTELEPORT)
    {
        return (false);
    }

    // don't teleport missiles, blood and gibs
    if((thing->flags & MF_MISSILE) ||
       thing->type == MT_FLESH ||
       thing->type == MT_GORE ||
       thing->type == MT_CHUNK ||
       thing->type == MT_CHUNK_GREEN ||
       thing->type == MT_CHUNK_BLUE)
        return 0;                

    // Don't teleport if hit back of line,
    //  so you can get out of teleporter.
    if (side == 1)                
        return 0;        

    
    tag = line->tag;
    for (i = 0; i < numsectors; i++)
    {
        if (sectors[ i ].tag == tag )
        {
            thinker = thinkercap.next;
            for (thinker = thinkercap.next;
                 thinker != &thinkercap;
                 thinker = thinker->next)
            {
                // not a mobj
                if (thinker->function.acp1 != (actionf_p1)P_MobjThinker)
                    continue;        

                m = (mobj_t *)thinker;
                
                // not a teleportman
                if (m->type != MT_TELEPORTMAN )
                    continue;                

                sector = m->subsector->sector;
                // wrong sector
                if (sector-sectors != i )
                    continue;        

                oldx = thing->x;
                oldy = thing->y;
                oldz = thing->z;
                                
                if (!P_TeleportMove (thing, m->x, m->y, false)) /* killough 8/9/98 */
                    return 0;

                // The first Final Doom executable does not set thing->z
                // when teleporting. This quirk is unique to this
                // particular version; the later version included in
                // some versions of the Id Anthology fixed this.

                if (gameversion != exe_final)
                    thing->z = thing->floorz;

                if (thing->player)
                {
                    if (thing->player->powers[pw_flight] && aboveFloor)
                    {
                        thing->z = thing->floorz + aboveFloor;
                        if (thing->z + thing->height > thing->ceilingz)
                        {
                            thing->z = thing->ceilingz - thing->height;
                        }
                        thing->player->viewz = thing->z + thing->player->viewheight;
                    }
                    else
                    {
                        thing->player->viewz = thing->z + thing->player->viewheight;
                        thing->player->lookdir = 0;
                    }
                }

                // spawn teleport fog at source and destination
                if(!beta_style)
                {
                    fog = P_SpawnMobj (oldx, oldy, oldz, MT_TFOG);
                    S_StartSound (fog, sfx_telept);
                    an = m->angle >> ANGLETOFINESHIFT;
                    fog = P_SpawnMobj (m->x+20*finecosine[an], m->y+20*finesine[an]
                                       , thing->z, MT_TFOG);

                    // emit sound, where?
                    S_StartSound (fog, sfx_telept);
                
                    // don't move for a bit
                    if (thing->player)
                        thing->reactiontime = 18;        

                }
                thing->angle = m->angle;
                if (!(thing->flags2 & MF2_NOFOOTCLIP)
                    && P_GetThingFloorType(thing))
                {
                    thing->flags2 |= MF2_FEETARECLIPPED;
                }
                else if (thing->flags2 & MF2_FEETARECLIPPED)
                {
                    thing->flags2 &= ~MF2_FEETARECLIPPED;
                }
                thing->momx = thing->momy = thing->momz = 0;
                return 1;
            }        
        }
    }
    return 0;
}
