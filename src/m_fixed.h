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
//        Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------


#ifndef __M_FIXED__
#define __M_FIXED__




//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS                16
#define FRACUNIT                (1<<FRACBITS)
#define FIXED2DOUBLE(x)         (x / (double)FRACUNIT)

typedef int fixed_t;

fixed_t FixedMul        (fixed_t a, fixed_t b);
fixed_t FixedDiv        (fixed_t a, fixed_t b);

int BETWEEN(int a, int b, int c);
int MIN(int a, int b);
int MAX(int a, int b);
unsigned int SafeAdd(unsigned int a, unsigned int b);
int ABS(int a);
int SIGN(int a);


#endif
