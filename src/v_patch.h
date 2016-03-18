//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
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
// DESCRIPTION:
//      Refresh/rendering module, shared data struct definitions.
//


#ifndef V_PATCH_H
#define V_PATCH_H


// Patches.
// A patch holds one or more columns.
// Patches are used for sprites and all masked pictures,
// and we compose textures from the TEXTURE1/2 lists
// of patches.

typedef struct 
{ 
    // bounding box size 
    short                width;
    short                height; 

    // pixels to the left of origin 
    short                leftoffset;

    // pixels below the origin 
    short                topoffset;

    // only [width] used
    int                  columnofs[8];

    // the [0] is &columnofs[width] 
    byte                 *pixels[4];

    // mip levels
    byte                 *pic;

} PACKEDATTR patch_t;

// posts are runs of non masked source pixels
typedef struct
{
    // -1 is the last post in a column
    byte                 topdelta;

    // length data bytes follows
    byte                 length;

} PACKEDATTR post_t;

// column_t is a list of 0 or more post_t, (byte)-1 terminated
typedef post_t        column_t;


#endif 

