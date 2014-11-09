/*
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h>
*/
#include "xmn_fntb.h"
#include "xmn_PSPm.h"

char	*pg_vramtop = (char *)0x04000000;
long	pg_screenmode;
long	pg_showframe;
long	pg_drawframe;
/*
void pgScreenFrame(long mode, long frame)
{
    pg_screenmode = mode;

    frame = (frame ? 1 : 0);

    pg_showframe = frame;

    if (mode == 0)
    {
	//screen off
	pg_drawframe = frame;

	sceDisplaySetFrameBuf(0, 0, 0, 1);
    }
    else if (mode == 1)
    {
	//show/draw same
	pg_drawframe = frame;

	sceDisplaySetFrameBuf(pg_vramtop + (pg_showframe ? FRAMESIZE : 0), LINESIZE, PIXELSIZE, 1);
    }
    else if (mode == 2)
    {
	//show/draw different
	pg_drawframe = (frame ? 0 : 1);

	sceDisplaySetFrameBuf(pg_vramtop + (pg_showframe ? FRAMESIZE : 0), LINESIZE, PIXELSIZE, 1);
    }
}

void pgInit()
{
    sceDisplaySetMode(0, SCREEN_WIDTH, SCREEN_HEIGHT);

    pgScreenFrame(0, 0);
}
*/
char *pgGetVramAddr(unsigned long x, unsigned long y)
{
    return pg_vramtop + (pg_drawframe ? FRAMESIZE : 0) +
	    x * PIXELSIZE * 2 + y * LINESIZE * 2 + 0x40000000;
}
/*
void pgFillvram(unsigned long color)
{
    char *vptr0;		//pointer to vram
    unsigned long i;

    vptr0 = pgGetVramAddr(0, 0);

    for (i = 0; i < FRAMESIZE / 2; i++)
    {
	*(unsigned short *)vptr0 = color;

	vptr0 += PIXELSIZE * 2;
    }
}

void pgScreenFlip()
{
    pg_showframe = (pg_showframe ? 0 : 1);
    pg_drawframe = (pg_drawframe ? 0 : 1);

    sceDisplaySetFrameBuf(pg_vramtop + (pg_showframe ? FRAMESIZE : 0), LINESIZE, PIXELSIZE, 0);
}

void pgScreenFlipV()
{
    pgScreenFlip();
}
*/
unsigned short rgb2col(unsigned char r, unsigned char g, unsigned char b)
{
    return ((((b >> 3) & 0x1F) << 10) + (((g >> 3) & 0x1F) << 5) + (((r >> 3) & 0x1F) << 0) + 0x8000);
}
/*
void pgWaitV()
{
    sceDisplayWaitVblankStart();
}
*/
unsigned short num2elisa(unsigned short c)
{
    if (c > 4374)
	return 0x6b;
    else if (c >= 1410)
	return c + (0x20c - 1410);
    else if (c >= 690)
	return 0x6b;
    else if (c >= 658)
	return c + (0x1ec - 658);
    else if (c >= 612)
	return c + (0x1cb - 612);
    else if (c >= 564)
	return c + (0x1aa - 564);
    else if (c >= 502)
	return c + (0x192 - 502);
    else if (c >= 470)
	return c + (0x17a - 470);
    else if (c >= 376)
	return c + (0x124 - 376);
    else if (c >= 282)
	return c + (0xd1 - 282);
    else if (c >= 252)
	return c + (0xb7 - 252);
    else if (c >= 220)
	return c + (0x9d - 220);
    else if (c >= 203)
	return c + (0x93 - 203);
    else if (c >= 187)
	return 0x92;
    else if (c >= 175)
	return c + (0x8a - 203);
    else if (c >= 153)
	return c + (0x7b - 153);
    else if (c >= 135)
	return c + (0x74 - 135);
    else if (c >= 119)
	return c + (0x6c - 119);
    else
	return c;
}

void Draw_Char_Hankaku(int x, int y, unsigned char ch, int col, int backcol, int fill)
{
    unsigned short *vr;
    unsigned char  *fnt;
    unsigned char  pt;
    int x1, y1;

    // mapping
    if (ch < 0x20)
	ch = 0;
    else if (ch < 0x80)
	ch -= 0x20;
    else if (ch < 0xa0)
	ch = 0;
    else
	ch -= 0x40;

    fnt = (unsigned char *) & hankaku_font10[ch * 10];

    // draw
    vr = (unsigned short *)pgGetVramAddr(x, y);

    for (y1 = 0; y1 < 10; y1++)
    {
	pt = *fnt++;

	for (x1 = 0; x1 < 5; x1++)
	{
	    if (pt & 1)
		*vr = col;
	    else
	    {
		if (fill)
		    *vr = backcol;
	    }
	    vr++;

	    pt = pt >> 1;
	}
	vr += LINESIZE - 5;
    }
}

void Draw_Char_Zenkaku(int x, int y, unsigned char u, unsigned char d, int col, int backcol, int fill)
{
    unsigned short *vr;
    unsigned short *fnt;
    unsigned short pt;
    int x1, y1;

    // mapping
    if (d > 0x7F)
	d--;

    d -= 0x40;
    u -= 0x81;

    fnt = (unsigned short *) & zenkaku_font10[num2elisa(u * 0xbc + d) * 10];

    // draw
    vr = (unsigned short *)pgGetVramAddr(x, y);

    for (y1 = 0; y1 < 10; y1++)
    {
	pt = *fnt++;

	for(x1 = 0; x1 < 10; x1++)
	{
	    if (pt & 1)
		*vr = col;
	    else
	    {
		if (fill)
		    *vr = backcol;
	    }
	    vr++;

	    pt = pt >> 1;
	}
	vr += LINESIZE - 10;
    }
}
/*
void mh_print(int x, int y, unsigned char *str, int col, int backcol, int fill)
{
    unsigned char ch = 0;
    unsigned char bef = 0;

    while (*str != 0)
    {
	ch = *str++;

	if (bef != 0)
	{
	    Draw_Char_Zenkaku(x, y, bef, ch, col, backcol, fill);

	    x += 10;
	    bef = 0;
	}
	else
	{
	    if (((ch >= 0x80) && (ch < 0xa0)) || (ch >= 0xe0))
		bef = ch;
	    else
	    {
		Draw_Char_Hankaku(x, y, ch, col, backcol, fill);
		x += 5;
	    }
	}
    }
}
*/
void pgDrawRectangle(int x, int y, int x2, int y2, unsigned long color)
{
    unsigned char* vr = (unsigned char*)pgGetVramAddr(x, y);
    int i;
    int diff = x2 - x;

    for (i = 0; i < diff; i++)
    {
	*(unsigned short*)vr = rgb2col(255, 255, 0);
	vr += PIXELSIZE * 2;
    }
}

