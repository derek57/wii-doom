#ifdef WII
#ifndef _GUI_H_
#define _GUI_H_

#include <gctypes.h>
#include "video.h"
#include <ogc/lwp.h>
#include <ogc/mutex.h>

#define AUTOSIZE 255

struct stConsoleCursorLocation stTexteLocation;

struct stTimer
{
    unsigned short int intInterval;
    boolean blnRunning;
};

u8 getTextBoxRow(u8 chRow);
u8 getTextBoxColumn(u8 chColumn);

void printLocatedText(u8 chRow,u8 chColumn,
                      struct stConsoleCursorLocation *stTexteLocation,
                      const char *strTexteFormat,...);

void printStyledText(u8 chRow, u8 chColumn, enum CONSOLE_FONT_COLORS FONT_BGCOLOR,
                     enum CONSOLE_FONT_COLORS FONT_FGCOLOR,
                     enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,
                     struct stConsoleCursorLocation *stTexteLocation,
                     const char *fmt, ...);

#endif
#endif

