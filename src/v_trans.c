//
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2014 Fabian Greffrath, Paul Haeberli
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
//
// Color translation tables
//


#include <math.h>

#include "doomtype.h"
#include "i_scale.h"
#include "v_trans.h"


#define CTOLERANCE      (0.0001)


typedef struct vect
{
    float x;
    float y;
    float z;

} vect;


// [crispy] here used to be static color translation tables based on
// the ones found in Boom and MBF. Nowadays these are recalculated
// by means of actual color space conversions in r_data:R_InitColormaps().

// this one will be the identity matrix
static byte cr_none[256];

// this one will be the ~50% darker matrix
static byte cr_dark[256];
static byte cr_gray[256];
static byte cr_green[256];
static byte cr_gold[256];
static byte cr_red[256];
static byte cr_blue[256];


byte *crx[] =
{
    (byte *)&cr_none,
    (byte *)&cr_dark,
    (byte *)&cr_gray,
    (byte *)&cr_green,
    (byte *)&cr_gold,
    (byte *)&cr_red,
    (byte *)&cr_blue
};

char **crstr = 0;

/*
Date: Sun, 26 Oct 2014 10:36:12 -0700
From: paul haeberli <paulhaeberli@yahoo.com>
Subject: Re: colors and color conversions
To: Fabian Greffrath <fabian@greffrath.com>

Yes, this seems exactly like the solution I was looking for. I just
couldn't find code to do the HSV->RGB conversion. Speaking of the code,
would you allow me to use this code in my software? The Doom source code
is licensed under the GNU GPL, so this code yould have to be under a
compatible license.

    Yes. I'm happy to contribute this code to your project.  GNU GPL or anything
    compatible sounds fine.

Regarding the conversions, the procedure you sent me will leave grays
(r=g=b) untouched, no matter what I set as HUE, right? Is it possible,
then, to also use this routine to convert colors *to* gray?

    You can convert any color to an equivalent grey by setting the saturation
    to 0.0


    - Paul Haeberli
*/

static void hsv_to_rgb(vect *hsv, vect *rgb)
{
    float h, s, v;

    h = hsv->x;
    s = hsv->y;
    v = hsv->z;
    h *= 360.0;

    if (s < CTOLERANCE)
    {
        rgb->x = v;
        rgb->y = v;
        rgb->z = v;
    }
    else
    {
        int i;
        float f, p, q, t;

        if (h >= 360.0)
            h -= 360.0;

        h /= 60.0;
        i = floor(h);
        f = h - i;
        p = v * (1.0 - s);
        q = v * (1.0 - (s * f));
        t = v * (1.0 - (s * (1.0 - f)));

        switch (i)
        {
            case 0 :
                rgb->x = v;
                rgb->y = t;
                rgb->z = p;
                break;

            case 1 :
                rgb->x = q;
                rgb->y = v;
                rgb->z = p;
                break;

            case 2 :
                rgb->x = p;
                rgb->y = v;
                rgb->z = t;
                break;

            case 3 :
                rgb->x = p;
                rgb->y = q;
                rgb->z = v;
                break;

            case 4 :
                rgb->x = t;
                rgb->y = p;
                rgb->z = v;
                break;

            case 5 :
                rgb->x = v;
                rgb->y = p;
                rgb->z = q;
                break;
        }
    }
}

static void rgb_to_hsv(vect *rgb, vect *hsv)
{
    float h, s, v;
    float cmax, cmin;
    float r, g, b;

    r = rgb->x;
    g = rgb->y;
    b = rgb->z;

    // find the cmax and cmin of r g b
    cmax = r;
    cmin = r;
    cmax = (g > cmax ? g : cmax);
    cmin = (g < cmin ? g : cmin);
    cmax = (b > cmax ? b : cmax);
    cmin = (b < cmin ? b : cmin);

    // value
    v = cmax;

    if (cmax > CTOLERANCE)
        s = (cmax - cmin) / cmax;
    else
    {
        s = 0.0;
        h = 0.0;
    }

    if (s < CTOLERANCE)
        h = 0.0;
    else
    {
        float cdelta = cmax - cmin;
        float rc = (cmax - r) / cdelta;
        float gc = (cmax - g) / cdelta;
        float bc = (cmax - b) / cdelta;

        if (r == cmax)
            h = bc - gc;
        else
        {
            if (g == cmax)
                h = 2.0 + rc - bc;
            else
                h = 4.0 + gc - rc;
        }

        h = h * 60.0;

        if (h < 0.0)
            h += 360.0;
    }

    hsv->x = h / 360.0;
    hsv->y = s;
    hsv->z = v;
}

// Search through the given palette, finding the nearest color that matches
// the given color.
// [crispy] share with v_trans.c:V_Colorize() and r_data.c:R_InitTranMap()
int FindNearestColor(byte *palette, int r, int g, int b)
{
    int best;
    int best_diff;
    int i;

    best = 0;
    best_diff = INT_MAX;

    for (i = 0; i < 256; ++i)
    {
        byte *col = palette + i * 3;
        int diff = (r - col[0]) * (r - col[0])
             + (g - col[1]) * (g - col[1])
             + (b - col[2]) * (b - col[2]);

        if (diff == 0)
        {
            return i;
        }
        else if (diff < best_diff)
        {
            best = i;
            best_diff = diff;
        }
    }

    return best;
}

byte V_Colorize(byte *playpal, int cr, byte source, dboolean keepgray109)
{
    vect rgb, hsv;

    // [crispy] preserve gray drop shadow in IWAD status bar numbers
    if (cr == CRX_NONE || (keepgray109 && source == 109))
        return source;

    rgb.x = playpal[3 * source + 0] / 255.;
    rgb.y = playpal[3 * source + 1] / 255.;
    rgb.z = playpal[3 * source + 2] / 255.;

    rgb_to_hsv(&rgb, &hsv);

    if (cr == CRX_DARK)
        hsv.z *= 0.5;
    else if (cr == CRX_GRAY)
        hsv.y = 0;
    else
    {
        // [crispy] hack colors to full saturation
        hsv.y = 1.0;

        if (cr == CRX_GREEN)
        {
            //hsv.x = ((16.216 * hsv.z) + 100.784) / 360.;
            hsv.x = 135. / 360.;
        }
        else if (cr == CRX_GOLD)
        {
            //hsv.x = ((51.351 * hsv.z) + 8.648) / 360.;
            hsv.x = 45. / 360.;
        }
        else if (cr == CRX_RED)
        {
            hsv.x = 0.;
        }
        else if (cr == CRX_BLUE)
        {
            hsv.x = 240. / 360.;
        }
    }

    hsv_to_rgb(&hsv, &rgb);

    rgb.x *= 255.;
    rgb.y *= 255.;
    rgb.z *= 255.;

    return FindNearestColor(playpal, (int)rgb.x, (int)rgb.y, (int)rgb.z);
}

