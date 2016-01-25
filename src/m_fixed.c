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
//        Fixed point implementation.
//
//-----------------------------------------------------------------------------



#include "stdlib.h"

#include "doomtype.h"
#include "i_system.h"
#include "m_fixed.h"




//
// FixedDiv, C version.
//
// [nitr8] UNUSED
//
/*
int SIGN(int a)
{
    return (1 | (a >> 31));
}
*/

int ABS(int a)
{
    int b = a >> 31;

    return ((a ^ b) - b);
}

int BETWEEN(int a, int b, int c)
{
    return MAX(a, MIN(b, c));
}

fixed_t FixedMul(fixed_t a, fixed_t b)
{
    return (((int64_t)a * (int64_t)b) >> FRACBITS);
}

fixed_t FixedDiv(fixed_t a, fixed_t b)
{
    if ((ABS(a) >> 14) >= ABS(b))
        return ((a ^ b) >> 31) ^ INT_MAX;
    else
        return (fixed_t)(((int64_t) a << FRACBITS) / b);
}

int MIN(int a, int b)
{
    a = a - b;
    return (b + (a & (a >> 31)));
}

int MAX(int a, int b)
{
    b = a - b;
    return (a - (b & (b >> 31)));
}

