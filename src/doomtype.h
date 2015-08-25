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
//        Simple basic typedefs, isolated here to make it easier
//         separating modules.
//    
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__


#include <strings.h>


// #define macros to provide functions missing in Windows.
// Outside Windows, we use strings.h for str[n]casecmp.



//
// The packed attribute forces structures to be packed into the minimum 
// space necessary.  If this is not done, the compiler may align structure
// fields differently to optimize memory access, inflating the overall
// structure size.  It is important to use the packed attribute on certain
// structures where alignment is important, particularly data read/written
// to disk.
//

#ifdef __GNUC__
#define PACKEDATTR __attribute__((packed,gcc_struct))
#else
#define PACKEDATTR
#endif

// C99 integer types; with gcc we just use this.  Other compilers 
// should add conditional statements that define the C99 types.

// What is really wanted here is stdint.h; however, some old versions
// of Solaris don't have stdint.h and only have inttypes.h (the 
// pre-standardisation version).  inttypes.h is also in the C99 
// standard and defined to include stdint.h, so include this. 

#include <inttypes.h>

#include <gctypes.h>                // FIXME: WARNING, THE WII DEVKITPRO (DEVKITPPC)
                                    // HAS A PROBLEM WITH THE "BOOLEAN" TYPEDEF HERE
//#ifdef __cplusplus                // SO WE USE THE DEFINES FROM THE DEVKIT INSTEAD
                                    // OF THOSE DEFINED RIGHT HERE FROM CHOCOLATE.
// Use builtin bool type with C++.
/*
typedef bool boolean;

#else

typedef enum
{
    false, 
    true
} boolean;

#endif
*/
typedef uint8_t byte;

typedef struct {
  byte checksum;       // Simple checksum of the entire packet
  byte type;           /* Type of packet */
  byte reserved[2];        /* Was random in prboom <=2.2.4, now 0 */
  unsigned int tic;        // Timestamp
} PACKEDATTR packet_header_t;

typedef unsigned long ULONG;
typedef unsigned short USHORT;

#define INT64  long long

#include <limits.h>

#define DIR_SEPARATOR '/'
#define DIR_SEPARATOR_S "/"
#define PATH_SEPARATOR ':'

#define arrlen(array) (sizeof(array) / sizeof(*array))

#endif

