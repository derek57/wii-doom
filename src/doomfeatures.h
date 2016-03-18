// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
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
//     List of features which can be enabled/disabled to slim down the
//     program.
//
//-----------------------------------------------------------------------------


#ifndef DOOM_FEATURES_H
#define DOOM_FEATURES_H


// Enables wad merging (the '-merge' command line parameter)
#ifndef WII
#define FEATURE_WAD_MERGE 1
#endif

// Enables dehacked support ('-deh')
#define FEATURE_DEHACKED 1

// Enables multiplayer support (network games)
//#define FEATURE_MULTIPLAYER 1

// Prints function names entered when running network mode
//#define NET_DEBUG 1

// Enables sound output
#define FEATURE_SOUND 1

// Prints code file and line in case of error (crash)
#define INSTRUMENTED 1

// Checks memory blocks for ZONEID 
#define ZONEIDCHECK 1

// Dumps memory in case of error 
#define HEAPDUMP 1

#define TIMEDIAG 1

// Enables signal handler
//#define PRBOOM_DEBUG 1

// Reports lump locks to ingame console
//#define SIMPLECHECKS 1


#endif /* #ifndef DOOM_FEATURES_H */

