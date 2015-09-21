#ifdef WII
#include <math.h>
#include <ogc/color.h>
#include <ogc/consol.h>
#include <ogc/system.h>
#include <ogc/video.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "gui.h"
#include "pngu.h"
#include "video.h"


#define TESTING_CODE 0
#define GUI_BACKGROUND_COLOR CONSOLE_BLACK
#define DEFAULT_FONT_BGCOLOR 0
#define DEFAULT_FONT_FGCOLOR 7
#define DEFAULT_FONT_WEIGHT 0
#define VERSION "3.1"


enum CONSOLE_FONT_COLORS CURRENT_FONT_BGCOLOR  = DEFAULT_FONT_BGCOLOR,
                         PREVIOUS_FONT_BGCOLOR = DEFAULT_FONT_BGCOLOR,
                         CURRENT_FONT_FGCOLOR  = DEFAULT_FONT_FGCOLOR,
                         PREVIOUS_FONT_FGCOLOR = DEFAULT_FONT_FGCOLOR;

enum CONSOLE_FONT_WEIGHTS
                         PREVIOUS_FONT_WEIGHT  = DEFAULT_FONT_WEIGHT,
                         CURRENT_FONT_WEIGHT   = DEFAULT_FONT_WEIGHT;


unsigned char CONSOLE_CURSOR_CURRENT_COLUMN    = 0,
              CONSOLE_CURSOR_PREVIOUS_COLUMN   = 0,
              CONSOLE_CURSOR_CURRENT_ROW       = 0,
              CONSOLE_CURSOR_PREVIOUS_ROW      = 0;


void setFontFgColor(enum CONSOLE_FONT_COLORS FONT_COLOR,
                    enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT)
{
    if (FONT_COLOR == CONSOLE_FONT_CURRENT_COLOR)
    {
        FONT_COLOR = CURRENT_FONT_FGCOLOR;
    }

    if (CONSOLE_FONT_CURRENT_WEIGHT == FONT_WEIGHT)
    {
        FONT_WEIGHT = CURRENT_FONT_WEIGHT;
    }

    PREVIOUS_FONT_FGCOLOR = CURRENT_FONT_FGCOLOR;
    PREVIOUS_FONT_WEIGHT = CURRENT_FONT_WEIGHT;

    printf("\x1b[%u;%um", (u8) FONT_COLOR + 30, FONT_WEIGHT);

    CURRENT_FONT_FGCOLOR = FONT_COLOR;
    CURRENT_FONT_WEIGHT = FONT_WEIGHT;
}

void setFontBgColor(enum CONSOLE_FONT_COLORS FONT_COLOR)
{
    if (FONT_COLOR == CONSOLE_FONT_CURRENT_COLOR)
    {
        FONT_COLOR = CURRENT_FONT_FGCOLOR;
    }

    PREVIOUS_FONT_BGCOLOR = CURRENT_FONT_BGCOLOR;

    printf("\x1b[%um", (u8) FONT_COLOR + 40);

    CURRENT_FONT_BGCOLOR = FONT_COLOR;
}

void setFontStyle(enum CONSOLE_FONT_COLORS FONT_BGCOLOR,
                  enum CONSOLE_FONT_COLORS FONT_FGCOLOR,
                  enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT)
{
    setFontBgColor(FONT_BGCOLOR);
    setFontFgColor(FONT_FGCOLOR, FONT_WEIGHT);
}

void resetPreviousFontStyle()
{
    setFontStyle(PREVIOUS_FONT_BGCOLOR, PREVIOUS_FONT_FGCOLOR, 
                 PREVIOUS_FONT_WEIGHT);
}

void setCursorPosition(u8 intRow, u8 intColumn)
{
    printf("\x1b[%u;%uH", intRow, intColumn);
}

void saveCursorPosition()
{
    CONSOLE_CURSOR_PREVIOUS_COLUMN = CONSOLE_CURSOR_CURRENT_COLUMN;
    CONSOLE_CURSOR_PREVIOUS_ROW = CONSOLE_CURSOR_CURRENT_ROW;
    CONSOLE_CURSOR_CURRENT_COLUMN = getConsoleColumn();
    CONSOLE_CURSOR_CURRENT_ROW = getConsoleRow();
}

int getConsoleColumn()
{
    int intConsoleColumn, intConsoleRow;

    CON_GetPosition(&intConsoleColumn, &intConsoleRow);

    return intConsoleColumn;
}

int getConsoleRow()
{
    int intConsoleColumn, intConsoleRow;

    CON_GetPosition(&intConsoleColumn, &intConsoleRow);

    return intConsoleRow;
}
#endif

