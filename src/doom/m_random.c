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
//        Random number LUT.
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <time.h>

#include "m_random.h"

//
// M_Random
// Returns a 0-255 number
//

static const unsigned char rndtable[256] = {
    0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66 ,
    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36 ,
    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188 ,
    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224 ,
    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242 ,
    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0 ,
    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235 ,
    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113 ,
    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75 ,
    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196 ,
    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113 ,
    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241 ,
    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224 ,
    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95 ,
    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226 ,
    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36 ,
    17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106 ,
    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136 ,
    120, 163, 236, 249
};

int        rndindex = 0;
int        prndindex = 0;

// Which one is deterministic?
int P_Random (void)
{
    prndindex = (prndindex+1)&0xff;
    return rndtable[prndindex];
}

// P_Random is used throughout all the p_xxx game code.
byte P_Random2 ()
{
    return rndtable[++prndindex];
}

// lot of code used P_Random()-P_Random() since C don't define 
// evaluation order it is compiler depenent so this allow network play 
// between different compilers
int P_SignedRandom ()
{
    int r = P_Random2();
    return r - P_Random2();
}

int M_Random (void)
{
    rndindex = (rndindex+1)&0xff;
    return rndtable[rndindex];
}

int M_RandomInt(int lower, int upper)
{
    return (rand() % (upper - lower + 1) + lower);
}

void M_ClearRandom (void)
{
    prndindex = 0;

    // Seed the M_Random counter from the system time

    rndindex = time(NULL) & 0xff;
}

rng_t rng;     // the random number state

unsigned long rngseed = 1993;   // killough 3/26/98: The seed

int P_RandomSMMU(pr_class_t pr_class)
{
    // killough 2/16/98:  We always update both sets of random number
    // generators, to ensure repeatability if the demo_compatibility
    // flag is changed while the program is running. Changing the
    // demo_compatibility flag does not change the sequences generated,
    // only which one is selected from.
    //
    // All of this RNG stuff is tricky as far as demo sync goes --
    // it's like playing with explosives :) Lee

    unsigned long boom;

    if (pr_class != pr_misc)      // killough 3/31/98
        pr_class = pr_all_in_one;

    boom = rng.seed[pr_class];

    // killough 3/26/98: add pr_class*2 to addend

    rng.seed[pr_class] = boom * 1664525ul + 221297ul + pr_class*2;

    boom >>= 20;

    return boom & 255;
}
