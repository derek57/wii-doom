#include <math.h>
#include <ogc/consol.h>
#include <ogc/gu.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gui.h"

int M_snprintf(char *buf, size_t buf_len, const char *s, ...);
int M_vsnprintf(char *buf, size_t buf_len, const char *s, va_list args);

u8 getTextBoxRow(u8 chRow)
{
    return (chRow == AUTOSIZE) ? getConsoleRow() : chRow;
}

u8 getTextBoxColumn(u8 chColumn)
{
    return (chColumn == AUTOSIZE) ? getConsoleColumn() : chColumn;
}

void printLocatedText(u8 chRow, u8 chColumn,
                      struct stConsoleCursorLocation *stTexteLocation,
                      const char *strTexteFormat, ...)
{
    static char strTextBuffer[1024];

    va_list pArguments;
    saveCursorPosition();
    stTexteLocation->intColumn = getTextBoxColumn(chColumn);
    stTexteLocation->intRow = getTextBoxRow(chRow);
    setCursorPosition(stTexteLocation->intRow, stTexteLocation->intColumn);
    va_start(pArguments, strTexteFormat);
    M_vsnprintf(strTextBuffer, sizeof(strTextBuffer), strTexteFormat, pArguments);
    va_end(pArguments);
    printf("%s", strTextBuffer);
    saveCursorPosition();
}

void printStyledText(u8 chRow, u8 chColumn,
                     enum CONSOLE_FONT_COLORS FONT_BGCOLOR,
                     enum CONSOLE_FONT_COLORS FONT_FGCOLOR,
                     enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,
                     struct stConsoleCursorLocation *stTexteLocation,const char *fmt, ...)
{
    static char strTextBuffer[1024];

    va_list pArguments;
    setFontStyle(FONT_BGCOLOR, FONT_FGCOLOR, FONT_WEIGHT);
    va_start(pArguments, fmt);
    M_vsnprintf(strTextBuffer, sizeof(strTextBuffer), fmt, pArguments);
    va_end(pArguments);
    printLocatedText(chRow, chColumn, stTexteLocation, "%s", strTextBuffer);
    resetPreviousFontStyle();
}

