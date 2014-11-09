#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <ogc/consol.h>
#include <ogc/gu.h>
#include "gui.h"
/*
#include "libutils.h"
#include "libarray.h"
#include "libmath.h"
*/
#include "libstring.h"
#include "libgeometry.h"

//#include "controllers.h"
#include "libtime.h"

#include <unistd.h>

/*
u8 getTextBoxRow(u8 chRow) {
    return (chRow==AUTOSIZE)?getConsoleRow():chRow;
}
u8 getTextBoxColumn(u8 chColumn) {
    return (chColumn==AUTOSIZE)?getConsoleColumn():chColumn;
}
u8 getTextBoxAutoWidth(enum BORDERS BORDER_TYPE,const char *strFormatValue,...) {
static char strTextBuffer[1024];
va_list pArguments;
unsigned intLinesCount,intMaxColumn;
    va_start(pArguments,strFormatValue);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strFormatValue,pArguments);
    va_end(pArguments);
    intLinesCount=getLinesCount(strTextBuffer,&intMaxColumn);
    return intMaxColumn+((BORDER_TYPE & RIGHT_BORDER)/RIGHT_BORDER)+((BORDER_TYPE & LEFT_BORDER)/LEFT_BORDER);
}
u8 getTextBoxAutoHeight(enum BORDERS BORDER_TYPE,const char *strFormatValue,...) {
static char strTextBuffer[1024];
va_list pArguments;
unsigned intMaxColumn;
    va_start(pArguments,strFormatValue);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strFormatValue,pArguments);
    va_end(pArguments);
    return getLinesCount(strTextBuffer,&intMaxColumn)+((BORDER_TYPE & TOP_BORDER)/TOP_BORDER)+((BORDER_TYPE & BOTTOM_BORDER)/BOTTOM_BORDER);
}
void getTextBoxPositions(u8 *chMinRow,u8 *chMinColumn,u8 *chMaxRow,u8 *chMaxColumn,enum BORDERS BORDER_TYPE,const char *strFormatValue,...) {
static char strTextBuffer[1024];
va_list pArguments;
    va_start(pArguments,strFormatValue);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strFormatValue,pArguments);
    va_end(pArguments);
    *chMinRow=getTextBoxRow(*chMinRow);
    *chMinColumn=getTextBoxColumn(*chMinColumn);
    if (*chMaxRow==AUTOSIZE) {
        *chMaxRow=*chMinRow+getTextBoxAutoHeight(BORDER_TYPE,"%s",strTextBuffer)-1;
    }
    if (*chMaxColumn==AUTOSIZE) {
        *chMaxColumn=*chMinColumn+getTextBoxAutoWidth(BORDER_TYPE,"%s",strTextBuffer)-1;
    }
    if (*chMinColumn>*chMaxColumn) {
        permutePointers((void *) chMinColumn,(void *) chMaxColumn);
    }
    if (*chMinRow>*chMaxRow) {
        permutePointers((void *) chMinRow,(void *) chMaxRow);
    }
}
*/
bool isInRange(double dbValue,double dbMinRangeValue,double dbMaxRangeValue,bool blnIncludeMinRange,bool blnIncludeMaxRange);
double getRoundNumber(double dbValue);

void permutePointers(void *varValue1,void *varValue2) {
void *varTemp;
    varTemp=varValue1;
    varValue1=varValue2;
    varValue2=varTemp;
}
void printRepeatString(unsigned int intRepeatsCount,const char *strFormatValue,...) {
static char strTextBuffer[1024];
va_list pArguments;
    va_start(pArguments,strFormatValue);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strFormatValue,pArguments);
    va_end(pArguments);
    while (intRepeatsCount) {
        intRepeatsCount--;
        printf("%s",strTextBuffer);
    }
}
void printLocatedText(u8 chRow,u8 chColumn,struct stConsoleCursorLocation *stTexteLocation,const char *strTexteFormat,...) {
static char strTextBuffer[1024];
va_list pArguments;
    saveCursorPosition();
    stTexteLocation->intColumn=getTextBoxColumn(chColumn);
    stTexteLocation->intRow=getTextBoxRow(chRow);
    setCursorPosition(stTexteLocation->intRow,stTexteLocation->intColumn);
    va_start(pArguments,strTexteFormat);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strTexteFormat,pArguments);
    va_end(pArguments);
    printf("%s",strTextBuffer);
    saveCursorPosition();
}
void printTextInColumnsRange(u8 chRow,u8 chColumn,u8 chMinColumn,u8 chMaxColumn,struct stConsoleCursorLocation *stTexteLocation,const char *strTexteFormat,...) {
static char strTextBuffer[1024];
va_list pArguments;
    if (chMinColumn>chMaxColumn) {
        permutePointers((void *) &chMinColumn,(void *) &chMaxColumn);
    }
    va_start(pArguments,strTexteFormat);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strTexteFormat,pArguments);
    va_end(pArguments);
    if (isInRange(chColumn,(double) chMinColumn-strlen(strTextBuffer)+1,chMaxColumn,true,true)) {
        if (chColumn>chMinColumn) {
            strTextBuffer[1+chMaxColumn-(chColumn-chMinColumn)]=0;
            printLocatedText(chRow,chColumn,stTexteLocation,"%s",strTextBuffer);
        }
        else {
            printLocatedText(chRow,chColumn,stTexteLocation,"%s",&strTextBuffer[chMinColumn-chColumn]);
        }
        resetSavedPreviousCursorPosition();
    }
}
void printAlignedText(double dbHorizontalAlign,double dbVerticalAlign,u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,bool blnMultiLine,bool blnHideOverflowText,struct stConsoleCursorLocation *stTexteLocation,const char *strTexteFormat,...) {
static char strTextBuffer[1024];
va_list pArguments;
unsigned int intBreakLinesCount;
char **strBreakLines;
double dbTextContainerX[2]={0,0},dbTextContainerY[2]={0,0};
    va_start(pArguments,strTexteFormat);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strTexteFormat,pArguments);
    va_end(pArguments);
    if (chMinColumn>chMaxColumn) {
        permutePointers((void *) &chMinColumn,(void *) &chMaxColumn);
    }
    if (chMinRow>chMaxRow) {
        permutePointers((void *) &chMinRow,(void *) &chMaxRow);
    }
    dbTextContainerX[1]=strlen(strTextBuffer)-1;
    if (blnMultiLine) {
        intBreakLinesCount=chMaxRow-chMinRow+1;
        if ((strBreakLines=getBreakStrings(strTextBuffer,' ',chMaxColumn-chMinColumn+1,&intBreakLinesCount))!=NULL) {
            dbTextContainerY[1]=intBreakLinesCount-1;
            dbHorizontalAlign=getRoundNumber(getPolyContainerPosition(&dbTextContainerX[0],2,chMinColumn,chMaxColumn,dbHorizontalAlign));
            dbVerticalAlign=getRoundNumber(getPolyContainerPosition(&dbTextContainerY[0],2,chMinRow,chMaxRow,dbVerticalAlign));
            if (blnHideOverflowText) {
                while ((intBreakLinesCount) && (isInRange(dbVerticalAlign,chMinRow,chMaxRow,true,true))) {
                    printTextInColumnsRange(dbVerticalAlign,dbHorizontalAlign,chMinColumn,chMaxColumn,stTexteLocation,"%s",strTextBuffer);
                    dbVerticalAlign++;
                    intBreakLinesCount--;
                }
            }
            else {
                while (intBreakLinesCount) {
                    printLocatedText(dbVerticalAlign,dbHorizontalAlign,stTexteLocation,"%s",strTextBuffer);
                    resetSavedPreviousCursorPosition();
                    dbVerticalAlign++;
                    intBreakLinesCount--;
                }
            }
            free(strBreakLines);
            strBreakLines=NULL;
        }
    }
    else {
        dbHorizontalAlign=getRoundNumber(getPolyContainerPosition(&dbTextContainerX[0],2,chMinColumn,chMaxColumn,dbHorizontalAlign));
        dbVerticalAlign=getRoundNumber(getPolyContainerPosition(&dbTextContainerY[0],2,chMinRow,chMaxRow,dbVerticalAlign));
        if (blnHideOverflowText) {
            if (isInRange(dbVerticalAlign,chMinRow,chMaxRow,true,true)) {
                printTextInColumnsRange(dbVerticalAlign,dbHorizontalAlign,chMinColumn,chMaxColumn,stTexteLocation,"%s",strTextBuffer);
            }
        }
        else {
            printLocatedText(dbVerticalAlign,dbHorizontalAlign,stTexteLocation,"%s",strTextBuffer);
            resetSavedPreviousCursorPosition();
        }
    }
}
void printStyledText(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,struct stConsoleCursorLocation *stTexteLocation,const char *fmt, ...) {
static char strTextBuffer[1024];
va_list pArguments;
    setFontStyle(FONT_BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT);
    va_start(pArguments,fmt);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),fmt,pArguments);
    va_end(pArguments);
    printLocatedText(chRow,chColumn,stTexteLocation,"%s",strTextBuffer);
    resetPreviousFontStyle();
}
void printAlignedStyledText(double dbColumn,double dbRow,u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,bool blnMultiLine,bool blnHideOverflowText,struct stConsoleCursorLocation *stTexteLocation,const char *strTexteFormat,...) {
static char strTextBuffer[1024];
va_list pArguments;
    va_start(pArguments,strTexteFormat);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strTexteFormat,pArguments);
    va_end(pArguments);
    setFontStyle(FONT_BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT);
    printAlignedText(dbColumn,dbRow,chMinRow,chMinColumn,chMaxRow,chMaxColumn,blnMultiLine,blnHideOverflowText,stTexteLocation,"%s",strTextBuffer);
    resetPreviousFontStyle();
}
unsigned char getBorderSymbol(enum BORDER_STYLES BORDER_STYLE,enum BORDER_SYMBOLS BORDER_SYMBOL) {
unsigned char varout=0;
    if (BORDER_SYMBOL) {
        switch(BORDER_STYLE) {
            case SINGLE_BORDER:
                varout=BORDER_SYMBOL;
                break;
            case DOUBLE_BORDER:
                switch(BORDER_SYMBOL) {
                    case HORIZONTAL_SINGLE_BORDER:
                        varout=HORIZONTAL_DOUBLE_BORDER;
                        break;
                    case MIDDLE_TOP_SINGLE_JUNCTION:
                        varout=MIDDLE_TOP_DOUBLE_JUNCTION;
                        break;
                    case LEFT_MIDDLE_SINGLE_JUNCTION:
                        varout=LEFT_MIDDLE_DOUBLE_JUNCTION;
                        break;
                    case CROSS_SINGLE_JUNCTION:
                        varout=CROSS_DOUBLE_JUNCTION;
                        break;
                    case LEFT_TOP_SINGLE_CORNER:
                        varout=LEFT_TOP_DOUBLE_CORNER;
                        break;
                    case RIGHT_MIDDLE_SINGLE_JUNCTION:
                        varout=RIGHT_MIDDLE_DOUBLE_JUNCTION;
                        break;
                    case RIGHT_TOP_SINGLE_CORNER:
                        varout=RIGHT_TOP_DOUBLE_CORNER;
                        break;
                    case MIDDLE_BOTTOM_SINGLE_JUNCTION:
                        varout=MIDDLE_BOTTOM_DOUBLE_JUNCTION;
                        break;
                    case LEFT_BOTTOM_SINGLE_CORNER:
                        varout=LEFT_BOTTOM_DOUBLE_CORNER;
                        break;
                    case RIGHT_BOTTOM_SINGLE_CORNER:
                        varout=RIGHT_BOTTOM_DOUBLE_CORNER;
                        break;
                    case VERTICAL_SINGLE_BORDER:
                        varout=VERTICAL_DOUBLE_BORDER;
                        break;
                }
                break;
            default:
                varout=BORDER_STYLE;
        }
    }
    return varout;
}
void drawSelectionFrame(u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS BORDER_BGCOLOR) {
u8 chColumnsCount;
    if (chMinColumn>chMaxColumn) {
        permutePointers((void *) &chMinColumn,(void *) &chMaxColumn);
    }
    if (chMinRow>chMaxRow) {
        permutePointers((void *) &chMinRow,(void *) &chMaxRow);
    }
    chColumnsCount=chMaxColumn-chMinColumn+1;
    saveCursorPosition();
    setFontStyle(BORDER_BGCOLOR,BORDER_COLOR,CONSOLE_FONT_BOLD);
    setCursorPosition(chMinRow,chMinColumn);
    printRepeatString(chColumnsCount,"%c",getBorderSymbol(BORDER_STYLE,HORIZONTAL_SINGLE_BORDER));
    if (chMinRow!=chMaxRow) {
        setCursorPosition(chMinRow,chMinColumn);
        printf("%c",getBorderSymbol(BORDER_STYLE,LEFT_TOP_SINGLE_CORNER));
        setCursorPosition(chMinRow,chMaxColumn);
        printf("%c",getBorderSymbol(BORDER_STYLE,RIGHT_TOP_SINGLE_CORNER));
    }
    while (chMinRow<chMaxRow) {
        chMinRow++;
        setCursorPosition(chMinRow,chMinColumn);
        printf("%c",getBorderSymbol(BORDER_STYLE,VERTICAL_SINGLE_BORDER));
        setCursorPosition(chMinRow,chMaxColumn);
        printf("%c",getBorderSymbol(BORDER_STYLE,VERTICAL_SINGLE_BORDER));
    }
    setCursorPosition(chMaxRow,chMinColumn);
    printRepeatString(chColumnsCount,"%c",getBorderSymbol(BORDER_STYLE,HORIZONTAL_SINGLE_BORDER));
    if (chMinColumn!=chMaxColumn) {
        setCursorPosition(chMaxRow,chMinColumn);
        printf("%c",getBorderSymbol(BORDER_STYLE,LEFT_BOTTOM_SINGLE_CORNER));
        setCursorPosition(chMaxRow,chMaxColumn);
        printf("%c",getBorderSymbol(BORDER_STYLE,RIGHT_BOTTOM_SINGLE_CORNER));
    }
    resetPreviousFontStyle();
    resetSavedCursorPosition();
}
void drawFrame(u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,enum BORDERS BORDER_TYPE,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS FRAME_COLOR,enum FRAME_JUNCTIONS TOP_LEFT_JUNCTION,enum FRAME_JUNCTIONS TOP_RIGHT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_LEFT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_RIGHT_JUNCTION) {
static char strLeftBorder[2],strRightBorder[2];
unsigned char chLeftBorder=VERTICAL_SINGLE_BORDER*(BORDER_TYPE & LEFT_BORDER)/LEFT_BORDER,chRightBorder=VERTICAL_SINGLE_BORDER *(BORDER_TYPE & RIGHT_BORDER)/RIGHT_BORDER;
s16 intFrameColumnsCount,intMinRow,intMaxRow;
struct stConsoleCursorLocation stTexteLocation;
    if (chMinColumn>chMaxColumn) {
        permutePointers((void *) &chMinColumn,(void *) &chMaxColumn);
    }
    if (chMinRow>chMaxRow) {
        permutePointers((void *) &chMinRow,(void *) &chMaxRow);
    }
    intMinRow=chMinRow;
    intMaxRow=chMaxRow;
    setFontStyle(FRAME_COLOR,BORDER_COLOR,CONSOLE_FONT_BOLD);
    intFrameColumnsCount=chMaxColumn-chMinColumn+1;
    saveCursorPosition();
    if (BORDER_TYPE & TOP_BORDER) {
        setCursorPosition(chMinRow,chMinColumn);
        printRepeatString(intFrameColumnsCount,"%c",getBorderSymbol(BORDER_STYLE,HORIZONTAL_SINGLE_BORDER));
        if (chLeftBorder) {
            switch ((unsigned char) (TOP_LEFT_JUNCTION)) {
                case HORIZONTAL_JUNCTION:
                    printLocatedText(chMinRow,chMinColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,MIDDLE_TOP_SINGLE_JUNCTION));
                    break;
                case VERTICAL_JUNCTION:
                    printLocatedText(chMinRow,chMinColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,LEFT_MIDDLE_SINGLE_JUNCTION));
                    break;
                case ALL_JUNCTIONS:
                    printLocatedText(chMinRow,chMinColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,CROSS_SINGLE_JUNCTION));
                    break;
                default:
                    printLocatedText(chMinRow,chMinColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,LEFT_TOP_SINGLE_CORNER));
            }
            resetSavedPreviousCursorPosition();
        }
        if (chRightBorder) {
            switch ((unsigned char) (TOP_RIGHT_JUNCTION)) {
                case HORIZONTAL_JUNCTION:
                    printLocatedText(chMinRow,chMaxColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,MIDDLE_TOP_SINGLE_JUNCTION));
                    break;
                case VERTICAL_JUNCTION:
                    printLocatedText(chMinRow,chMaxColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,RIGHT_MIDDLE_SINGLE_JUNCTION));
                    break;
                case ALL_JUNCTIONS:
                    printLocatedText(chMinRow,chMaxColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,CROSS_SINGLE_JUNCTION));
                    break;
                default:
                    printLocatedText(chMinRow,chMaxColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,RIGHT_TOP_SINGLE_CORNER));
            }
            resetSavedPreviousCursorPosition();
        }
        intMinRow++;
    }
    if (BORDER_TYPE & BOTTOM_BORDER) {
        setCursorPosition(chMaxRow,chMinColumn);
        printRepeatString(intFrameColumnsCount,"%c",getBorderSymbol(BORDER_STYLE,HORIZONTAL_SINGLE_BORDER));
        if (chLeftBorder) {
            switch ((unsigned char) (BOTTOM_LEFT_JUNCTION)) {
                case HORIZONTAL_JUNCTION:
                    printLocatedText(chMaxRow,chMinColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,MIDDLE_BOTTOM_SINGLE_JUNCTION));
                    break;
                case VERTICAL_JUNCTION:
                    printLocatedText(chMaxRow,chMinColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,LEFT_MIDDLE_SINGLE_JUNCTION));
                    break;
                case ALL_JUNCTIONS:
                    printLocatedText(chMaxRow,chMinColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,CROSS_SINGLE_JUNCTION));
                    break;
                default:
                    printLocatedText(chMaxRow,chMinColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,LEFT_BOTTOM_SINGLE_CORNER));
            }
            resetSavedPreviousCursorPosition();
        }
        if (chRightBorder) {
            switch ((unsigned char) (BOTTOM_RIGHT_JUNCTION)) {
                case HORIZONTAL_JUNCTION:
                    printLocatedText(chMaxRow,chMaxColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,MIDDLE_BOTTOM_SINGLE_JUNCTION));
                    break;
                case VERTICAL_JUNCTION:
                    printLocatedText(chMaxRow,chMaxColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,RIGHT_MIDDLE_SINGLE_JUNCTION));
                    break;
                case ALL_JUNCTIONS:
                    printLocatedText(chMaxRow,chMaxColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,CROSS_SINGLE_JUNCTION));
                    break;
                default:
                    printLocatedText(chMaxRow,chMaxColumn,&stTexteLocation,"%c",getBorderSymbol(BORDER_STYLE,RIGHT_BOTTOM_SINGLE_CORNER));
            }
            resetSavedPreviousCursorPosition();
        }
        intMaxRow--;
    }
    if ((intMaxRow-intMinRow+1)>0) {
        snprintf(strLeftBorder,sizeof(strLeftBorder),"%c",getBorderSymbol(BORDER_STYLE,chLeftBorder));
        snprintf(strRightBorder,sizeof(strRightBorder),"%c",getBorderSymbol(BORDER_STYLE,chRightBorder));
        intFrameColumnsCount=MAX(intFrameColumnsCount-strlen(strLeftBorder)-strlen(strRightBorder),0);
        while (intMinRow<=intMaxRow) {
            setCursorPosition(intMinRow,chMinColumn);
            printf("%s%*s%s",strLeftBorder,intFrameColumnsCount,"",strRightBorder);
            intMinRow++;
        }
    }
    resetSavedCursorPosition();
    resetPreviousFontStyle();
}
void printTextBox(u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,double dbHorizontalAlign,double dbVerticalAlign,enum BORDERS BORDER_TYPE,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum FRAME_JUNCTIONS TOP_LEFT_JUNCTION,enum FRAME_JUNCTIONS TOP_RIGHT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_LEFT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_RIGHT_JUNCTION,enum CONSOLE_FONT_COLORS BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,bool blnMultiLine,bool blnHideOverflowText,struct stConsoleCursorLocation *stTexteLocation,const char *strFormatValue,...) {
static char strTextBuffer[1024];
int intMinRow,intMaxRow,intMinColumn,intMaxColumn;
va_list pArguments;
    va_start(pArguments,strFormatValue);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strFormatValue,pArguments);
    va_end(pArguments);
    getTextBoxPositions(&chMinRow,&chMinColumn,&chMaxRow,&chMaxColumn,BORDER_TYPE,"%s",strTextBuffer);
    intMinRow=chMinRow+(BORDER_TYPE & TOP_BORDER)/TOP_BORDER;
    intMaxRow=chMaxRow-(BORDER_TYPE & BOTTOM_BORDER)/BOTTOM_BORDER;
    intMinColumn=chMinColumn+(BORDER_TYPE & LEFT_BORDER)/LEFT_BORDER;
    intMaxColumn=chMaxColumn-(BORDER_TYPE & RIGHT_BORDER)/RIGHT_BORDER;
    drawFrame(chMinRow,chMinColumn,chMaxRow,chMaxColumn,BORDER_TYPE,BORDER_STYLE,BORDER_COLOR,BGCOLOR,TOP_LEFT_JUNCTION,TOP_RIGHT_JUNCTION,BOTTOM_LEFT_JUNCTION,BOTTOM_RIGHT_JUNCTION);
    if ((intMaxColumn-intMinColumn+1>0) && (intMaxRow-intMinRow+1>0)) {
        printAlignedStyledText(dbHorizontalAlign,dbVerticalAlign,intMinRow,intMinColumn,intMaxRow,intMaxColumn,BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT,blnMultiLine,blnHideOverflowText,stTexteLocation,"%s",strTextBuffer);
    }
}
void printAlignedTextBox(u8 chFrameMinRow,u8 chFrameMinColumn,u8 chFrameMaxRow,u8 chFrameMaxColumn,double dbRow,double dbColumn,double dbHeight,double dbWidth,double dbHorizontalAlign,double dbVerticalAlign,enum BORDERS BORDER_TYPE,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum FRAME_JUNCTIONS TOP_LEFT_JUNCTION,enum FRAME_JUNCTIONS TOP_RIGHT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_LEFT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_RIGHT_JUNCTION,enum CONSOLE_FONT_COLORS BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,bool blnMultiLine,bool blnHideOverflowText,struct stConsoleCursorLocation *stTexteLocation,const char *strFormatValue,...) {
static char strTextBuffer[1024];
va_list pArguments;
double dbTextContainerX[2]={dbColumn,dbColumn},dbTextContainerY[2]={dbRow,dbRow};
    va_start(pArguments,strFormatValue);
    vsnprintf(strTextBuffer,sizeof(strTextBuffer),strFormatValue,pArguments);
    va_end(pArguments);
    if (chFrameMinRow>chFrameMaxRow) {
        permutePointers((void *) &chFrameMinRow,(void *) &chFrameMaxRow);
    }
    if (chFrameMinColumn>chFrameMaxColumn) {
        permutePointers((void *) &chFrameMinColumn,(void *) &chFrameMaxColumn);
    }
    dbWidth=fabs(dbWidth);
    dbHeight=fabs(dbHeight);
    if (dbWidth<=1) {
        dbWidth=(double) (chFrameMaxColumn-chFrameMinColumn+1)*dbWidth;
        dbTextContainerX[0]=0;
        dbTextContainerX[1]=dbWidth;
    }
    if (dbHeight<=1) {
        dbHeight=(double) (chFrameMaxRow-chFrameMinRow+1)*dbHeight;
        dbTextContainerY[0]=0;
        dbTextContainerY[1]=dbHeight;
    }
    if (dbWidth==AUTOSIZE) {
        dbWidth=getTextBoxAutoWidth(BORDER_TYPE,"%s",strTextBuffer);
        dbTextContainerX[0]=0;
        dbTextContainerX[1]=dbWidth;
    }
    if (dbHeight==AUTOSIZE) {
        dbHeight=getTextBoxAutoHeight(BORDER_TYPE,"%s",strTextBuffer);
        dbTextContainerY[0]=0;
        dbTextContainerY[1]=dbHeight;
    }
    dbColumn=getRoundNumber(getPolyContainerPosition(&dbTextContainerX[0],2,chFrameMinColumn,chFrameMaxColumn,dbColumn));
    dbRow=getRoundNumber(getPolyContainerPosition(&dbTextContainerY[0],2,chFrameMinRow,chFrameMaxRow,dbRow));
    printTextBox(dbRow,dbColumn,getRoundNumber(dbRow+dbHeight-1),getRoundNumber(dbColumn+dbWidth-1),dbHorizontalAlign,dbVerticalAlign,BORDER_TYPE,BORDER_STYLE,BORDER_COLOR,TOP_LEFT_JUNCTION,TOP_RIGHT_JUNCTION,BOTTOM_LEFT_JUNCTION,BOTTOM_RIGHT_JUNCTION,BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT,blnMultiLine,blnHideOverflowText,stTexteLocation,"%s",strTextBuffer);
}
void drawLabel(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,const char *strLabelCaption,u8 chLabelSize,struct stLabel *stLabelSettings,struct stConsoleCursorLocation *stLabelCaptionLocation) {
    printStyledText(chRow,chColumn,FONT_BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT,stLabelCaptionLocation,strLabelCaption);
    CON_GetPosition(&stLabelSettings->stLabelLocation.intColumn,&stLabelSettings->stLabelLocation.intRow);
    stLabelSettings->chLabelSize=chLabelSize;
    printf("%*s",(unsigned int) stLabelSettings->chLabelSize,"");
}
void printLabel(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,unsigned char chLabelSize,struct stConsoleCursorLocation *stConcatLabelLocation,const char *strFormatLabel,...) {
static char strLabelText[256];
va_list pArguments;
unsigned char chLabelLength;
    saveCursorPosition();
    va_start(pArguments,strFormatLabel);
    vsnprintf(strLabelText,chLabelSize+1,strFormatLabel,pArguments);
    va_end(pArguments);
    setCursorPosition(getTextBoxRow(chRow),getTextBoxColumn(chColumn));
    setFontStyle(FONT_BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT);
    printf("%.*s",(unsigned int) chLabelSize,strLabelText);
    CON_GetPosition(&stConcatLabelLocation->intColumn,&stConcatLabelLocation->intRow);
    resetPreviousFontStyle();
    chLabelLength=strlen(strLabelText);
    printf("%*s",(unsigned int) ((chLabelLength>chLabelSize)?0:chLabelSize-chLabelLength),"");
    resetSavedCursorPosition();
}
void drawProgressBar(u8 chProgressBarRow,u8 chProgressBarColumn,u8 chProgressBarSize,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,enum CONSOLE_FONT_COLORS PROGRESS_BAR_BGCOLOR,u8 chProgressBarTextSize,u8 chProgressBarTextColumn,u8 chProgressBarTextRow,enum CONSOLE_FONT_COLORS PROGRESSCOLOR,unsigned int intOperationsCount,struct stProgressBar *stProgressBarSettings,struct stConsoleCursorLocation *stProgressBarLabelLocation,const char *strFormatLabel,...) {
static char strLabel[51];
va_list pArguments;
    va_start(pArguments,strFormatLabel);
    vsnprintf(strLabel,sizeof(strLabel),strFormatLabel,pArguments);
    va_end(pArguments);
    printStyledText(chProgressBarRow,chProgressBarColumn,FONT_BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT,stProgressBarLabelLocation,"%s",strLabel);
    printf(" ");
    CON_GetPosition(&stProgressBarSettings->stProgressBarLocation.intColumn,&stProgressBarSettings->stProgressBarLocation.intRow);
    setFontBgColor(PROGRESS_BAR_BGCOLOR);
    stProgressBarSettings->chProgressBarSize=chProgressBarSize;
    printf("%*s",(unsigned int) chProgressBarSize,"");
    resetPreviousBgColor();
    stProgressBarSettings->intValue=0;
    stProgressBarSettings->chProgressBarTextSize=chProgressBarTextSize;
    stProgressBarSettings->stProgressBarTextLocation.intColumn=chProgressBarTextColumn;
    stProgressBarSettings->stProgressBarTextLocation.intRow=chProgressBarTextRow;
    stProgressBarSettings->PROGRESSBAR_COLOR=PROGRESSCOLOR;
    stProgressBarSettings->intOperationsCount=intOperationsCount;
}
void updateProgressBar(struct stProgressBar *stProgressBarSettings,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,const char *strFormatProgressBarText,...) {
static char strProgressBarText[256];
struct stConsoleCursorLocation stTexteLocation;
va_list pArguments;
    if (stProgressBarSettings->intValue<stProgressBarSettings->intOperationsCount) {
        saveCursorPosition();
        stProgressBarSettings->intValue=stProgressBarSettings->intValue+1;
        setCursorPosition(stProgressBarSettings->stProgressBarLocation.intRow,stProgressBarSettings->stProgressBarLocation.intColumn);
        setFontBgColor(stProgressBarSettings->PROGRESSBAR_COLOR);
        printf("%*s",(unsigned int) getRoundNumber((double)stProgressBarSettings->intValue*(double) stProgressBarSettings->chProgressBarSize/(double) stProgressBarSettings->intOperationsCount),"");
        resetPreviousBgColor();
        resetSavedCursorPosition();
        va_start(pArguments,strFormatProgressBarText);
        vsnprintf(strProgressBarText,stProgressBarSettings->chProgressBarTextSize+1,strFormatProgressBarText,pArguments);
        va_end(pArguments);
        printLabel(stProgressBarSettings->stProgressBarTextLocation.intRow,stProgressBarSettings->stProgressBarTextLocation.intColumn,FONT_BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT,stProgressBarSettings->chProgressBarTextSize,&stTexteLocation,"%s",strProgressBarText);
    }
}
void printLevelsBar(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS LEVELS_BAR_COLOR,enum CONSOLE_FONT_COLORS ACTIVE_LEVEL_COLOR,unsigned char chActiveLevelIndex,unsigned char chLevelsCount,struct stConsoleCursorLocation *stLevelBarLocation,const char *strFirstLevel,...) {
va_list pArguments;
unsigned short int intLevelIndex=0;
const char *strCurrentLevel=strFirstLevel;
    va_start(pArguments,strFirstLevel);
    setFontFgColor((intLevelIndex==chActiveLevelIndex)?ACTIVE_LEVEL_COLOR:LEVELS_BAR_COLOR,CONSOLE_FONT_BOLD);
    printLocatedText(chRow,chColumn,stLevelBarLocation,"%s",strCurrentLevel);
    resetPreviousFgColor();
    intLevelIndex++;
    while (intLevelIndex<chLevelsCount) {
        strCurrentLevel=va_arg(pArguments,const char *);
        setFontFgColor(LEVELS_BAR_COLOR,CONSOLE_FONT_BOLD);
        printf(" > ");
        resetPreviousFgColor();
        setFontFgColor((intLevelIndex==chActiveLevelIndex)?ACTIVE_LEVEL_COLOR:LEVELS_BAR_COLOR,CONSOLE_FONT_BOLD);
        printf("%s",strCurrentLevel);
        resetPreviousFgColor();
        intLevelIndex++;
    }
    va_end(pArguments);
}
unsigned char getTableCellCorner(enum CELL_CORNERS CORNER,enum BORDER_STYLES BORDER_SYLE,u8 chColumnId,u8 chRowId,u8 chColumnsCount,u8 chRowsCount) {
enum BORDER_SYMBOLS chTableCellCorners[16]={LEFT_TOP_SINGLE_CORNER,MIDDLE_TOP_SINGLE_JUNCTION,MIDDLE_TOP_SINGLE_JUNCTION,RIGHT_TOP_SINGLE_CORNER,LEFT_MIDDLE_SINGLE_JUNCTION,CROSS_SINGLE_JUNCTION,CROSS_SINGLE_JUNCTION,RIGHT_MIDDLE_SINGLE_JUNCTION,LEFT_MIDDLE_SINGLE_JUNCTION,CROSS_SINGLE_JUNCTION,CROSS_SINGLE_JUNCTION,RIGHT_MIDDLE_SINGLE_JUNCTION,LEFT_BOTTOM_SINGLE_CORNER,MIDDLE_BOTTOM_SINGLE_JUNCTION,MIDDLE_BOTTOM_SINGLE_JUNCTION,RIGHT_BOTTOM_SINGLE_CORNER};
unsigned char i;
unsigned char varout=0;
    if ((chRowId<chRowsCount) && (chColumnId<chColumnsCount)) {
        if (chRowsCount<3) {
            for (i=0;i<4;i++) {
                chTableCellCorners[chRowsCount*4+i]=chTableCellCorners[12+i];
            }
        }
        if (chColumnsCount<3) {
            for (i=0;i<4;i++) {
                chTableCellCorners[i*4+chColumnsCount]=chTableCellCorners[i*4+3];
            }
        }
        varout=getBorderSymbol(BORDER_SYLE,chTableCellCorners[CORNER/3+MIN(chRowId,(chRowsCount-1-chRowId)?1:2)*4+MIN(chColumnId,(chColumnsCount-1-chColumnId)?1:2)]);
    }
    return varout;
}
unsigned char getTableCellCornerJunction(enum CELL_CORNERS CORNER,u8 chColumnId,u8 chRowId,u8 chColumnsCount,u8 chRowsCount) {
enum FRAME_JUNCTIONS chTableCellCornerJunctions[16]={NO_JUNCTION,HORIZONTAL_JUNCTION,HORIZONTAL_JUNCTION,NO_JUNCTION,VERTICAL_JUNCTION,ALL_JUNCTIONS,ALL_JUNCTIONS,VERTICAL_JUNCTION,VERTICAL_JUNCTION,ALL_JUNCTIONS,ALL_JUNCTIONS,VERTICAL_JUNCTION,NO_JUNCTION,HORIZONTAL_JUNCTION,HORIZONTAL_JUNCTION,NO_JUNCTION};
unsigned char i;
unsigned char varout=NO_JUNCTION;
    if ((chRowId<chRowsCount) && (chColumnId<chColumnsCount)) {
        if (chRowsCount<3) {
            for (i=0;i<4;i++) {
                chTableCellCornerJunctions[chRowsCount*4+i]=chTableCellCornerJunctions[12+i];
            }
        }
        if (chColumnsCount<3) {
            for (i=0;i<4;i++) {
                chTableCellCornerJunctions[i*4+chColumnsCount]=chTableCellCornerJunctions[i*4+3];
            }
        }
        varout=chTableCellCornerJunctions[CORNER/3+MIN(chRowId,(chRowsCount-1-chRowId)?1:2)*4+MIN(chColumnId,(chColumnsCount-1-chColumnId)?1:2)];
    }
    return varout;
}
void updateTableCell(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS BGCOLOR,enum CONSOLE_FONT_COLORS FONT_COLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,double dbHorizontalAlign,double dbVerticalAlign,struct stTable *stTableSettings,struct stConsoleCursorLocation *stCellTextLocation,const char *strFormatCellText,...) {
static char strCellText[1024];
va_list pArguments;
    if ((chRow<stTableSettings->chRowsCount) && (chColumn<stTableSettings->chColumnsCount)) {
        va_start(pArguments,strFormatCellText);
        vsnprintf(strCellText,sizeof(strCellText),strFormatCellText,pArguments);
        va_end(pArguments);
        printTextBox(1+stTableSettings->stTableLocation.intRow+chRow*stTableSettings->chCellHeight,1+stTableSettings->stTableLocation.intColumn+stTableSettings->chCellWidth*chColumn,stTableSettings->stTableLocation.intRow+(chRow+1)*stTableSettings->chCellHeight-1,stTableSettings->stTableLocation.intColumn+stTableSettings->chCellWidth*(chColumn+1)-1,dbHorizontalAlign,dbVerticalAlign,NO_BORDER,stTableSettings->BORDER_STYLE,stTableSettings->BORDER_COLOR,NO_JUNCTION,NO_JUNCTION,NO_JUNCTION,NO_JUNCTION,BGCOLOR,FONT_COLOR,FONT_WEIGHT,true,true,stCellTextLocation,"%s",strCellText);
    }
}
void drawTable(u8 chRow,u8 chColumn,u8 chColumnsCount,u8 chRowsCount,u8 chCellWidth,u8 chCellHeight,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS BGCOLOR,struct stTable *stTableSettings) {
    stTableSettings->stTableLocation.intColumn=getTextBoxColumn(chColumn);
    stTableSettings->stTableLocation.intRow=getTextBoxRow(chRow);
    stTableSettings->chColumnsCount=chColumnsCount;
    stTableSettings->chRowsCount=chRowsCount;
    stTableSettings->chCellWidth=chCellWidth;
    stTableSettings->chCellHeight=chCellHeight;
    stTableSettings->BORDER_STYLE=BORDER_STYLE;
    stTableSettings->BORDER_COLOR=BORDER_COLOR;
    stTableSettings->BGCOLOR=BGCOLOR;
    stTableSettings->chSelectedCellColumn=0;
    stTableSettings->chSelectedCellRow=0;
    if (chColumnsCount && chRowsCount && chCellWidth && chCellHeight) {
        while (chRowsCount) {
            chRowsCount--;
            chColumnsCount=stTableSettings->chColumnsCount;
            while (chColumnsCount) {
                chColumnsCount--;
                drawFrame(stTableSettings->stTableLocation.intRow+chRowsCount*chCellHeight,stTableSettings->stTableLocation.intColumn+chCellWidth*chColumnsCount,stTableSettings->stTableLocation.intRow+(chRowsCount+1)*chCellHeight,stTableSettings->stTableLocation.intColumn+chCellWidth*(chColumnsCount+1),ALL_BORDERS,BORDER_STYLE,BORDER_COLOR,BGCOLOR,getTableCellCornerJunction(TOP_LEFT_CORNER,chColumnsCount,chRowsCount,stTableSettings->chColumnsCount,stTableSettings->chRowsCount),getTableCellCornerJunction(TOP_RIGHT_CORNER,chColumnsCount,chRowsCount,stTableSettings->chColumnsCount,stTableSettings->chRowsCount),getTableCellCornerJunction(BOTTOM_LEFT_CORNER,chColumnsCount,chRowsCount,stTableSettings->chColumnsCount,stTableSettings->chRowsCount),getTableCellCornerJunction(BOTTOM_RIGHT_CORNER,chColumnsCount,chRowsCount,stTableSettings->chColumnsCount,stTableSettings->chRowsCount));
            }
        }
    }
}
void selectTableCell(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS BORDER_BGCOLOR,struct stTable *stTableSettings) {
struct stConsoleCursorLocation stCornerLocation;
u8 chSelectedCellRow,chSelectedCellColumn;
    if ((chRow<stTableSettings->chRowsCount) && (chColumn<stTableSettings->chColumnsCount)) {
        chSelectedCellRow=stTableSettings->stTableLocation.intRow+stTableSettings->chSelectedCellRow*stTableSettings->chCellHeight;
        chSelectedCellColumn=stTableSettings->stTableLocation.intColumn+stTableSettings->chSelectedCellColumn*stTableSettings->chCellWidth;
        drawSelectionFrame(chSelectedCellRow,chSelectedCellColumn,chSelectedCellRow+stTableSettings->chCellHeight,chSelectedCellColumn+stTableSettings->chCellWidth,stTableSettings->BORDER_STYLE,stTableSettings->BORDER_COLOR,stTableSettings->BGCOLOR);
        printStyledText(chSelectedCellRow,chSelectedCellColumn,stTableSettings->BGCOLOR,stTableSettings->BORDER_COLOR,CONSOLE_FONT_BOLD,&stCornerLocation,"%c",getTableCellCorner(TOP_LEFT_CORNER,stTableSettings->BORDER_STYLE,stTableSettings->chSelectedCellColumn,stTableSettings->chSelectedCellRow,stTableSettings->chColumnsCount,stTableSettings->chRowsCount));
        resetSavedPreviousCursorPosition();
        printStyledText(chSelectedCellRow,chSelectedCellColumn+stTableSettings->chCellWidth,stTableSettings->BGCOLOR,stTableSettings->BORDER_COLOR,CONSOLE_FONT_BOLD,&stCornerLocation,"%c",getTableCellCorner(TOP_RIGHT_CORNER,stTableSettings->BORDER_STYLE,stTableSettings->chSelectedCellColumn,stTableSettings->chSelectedCellRow,stTableSettings->chColumnsCount,stTableSettings->chRowsCount));
        resetSavedPreviousCursorPosition();
        printStyledText(chSelectedCellRow+stTableSettings->chCellHeight,chSelectedCellColumn,stTableSettings->BGCOLOR,stTableSettings->BORDER_COLOR,CONSOLE_FONT_BOLD,&stCornerLocation,"%c",getTableCellCorner(BOTTOM_LEFT_CORNER,stTableSettings->BORDER_STYLE,stTableSettings->chSelectedCellColumn,stTableSettings->chSelectedCellRow,stTableSettings->chColumnsCount,stTableSettings->chRowsCount));
        resetSavedPreviousCursorPosition();
        printStyledText(chSelectedCellRow+stTableSettings->chCellHeight,chSelectedCellColumn+stTableSettings->chCellWidth,stTableSettings->BGCOLOR,stTableSettings->BORDER_COLOR,CONSOLE_FONT_BOLD,&stCornerLocation,"%c",getTableCellCorner(BOTTOM_RIGHT_CORNER,stTableSettings->BORDER_STYLE,stTableSettings->chSelectedCellColumn,stTableSettings->chSelectedCellRow,stTableSettings->chColumnsCount,stTableSettings->chRowsCount));
        resetSavedPreviousCursorPosition();
        stTableSettings->chSelectedCellRow=stTableSettings->stTableLocation.intRow+chRow*stTableSettings->chCellHeight;
        stTableSettings->chSelectedCellColumn=stTableSettings->stTableLocation.intColumn+chColumn*stTableSettings->chCellWidth;
        drawSelectionFrame(stTableSettings->chSelectedCellRow,stTableSettings->chSelectedCellColumn,stTableSettings->chSelectedCellRow+stTableSettings->chCellHeight,stTableSettings->chSelectedCellColumn+stTableSettings->chCellWidth,stTableSettings->BORDER_STYLE,BORDER_COLOR,BORDER_BGCOLOR);
        printStyledText(stTableSettings->chSelectedCellRow,stTableSettings->chSelectedCellColumn,BORDER_BGCOLOR,BORDER_COLOR,CONSOLE_FONT_BOLD,&stCornerLocation,"%c",getTableCellCorner(TOP_LEFT_CORNER,stTableSettings->BORDER_STYLE,chColumn,chRow,stTableSettings->chColumnsCount,stTableSettings->chRowsCount));
        resetSavedPreviousCursorPosition();
        printStyledText(stTableSettings->chSelectedCellRow,stTableSettings->chSelectedCellColumn+stTableSettings->chCellWidth,BORDER_BGCOLOR,BORDER_COLOR,CONSOLE_FONT_BOLD,&stCornerLocation,"%c",getTableCellCorner(TOP_RIGHT_CORNER,stTableSettings->BORDER_STYLE,chColumn,chRow,stTableSettings->chColumnsCount,stTableSettings->chRowsCount));
        resetSavedPreviousCursorPosition();
        printStyledText(stTableSettings->chSelectedCellRow+stTableSettings->chCellHeight,stTableSettings->chSelectedCellColumn,BORDER_BGCOLOR,BORDER_COLOR,CONSOLE_FONT_BOLD,&stCornerLocation,"%c",getTableCellCorner(BOTTOM_LEFT_CORNER,stTableSettings->BORDER_STYLE,chColumn,chRow,stTableSettings->chColumnsCount,stTableSettings->chRowsCount));
        resetSavedPreviousCursorPosition();
        printStyledText(stTableSettings->chSelectedCellRow+stTableSettings->chCellHeight,stTableSettings->chSelectedCellColumn+stTableSettings->chCellWidth,BORDER_BGCOLOR,BORDER_COLOR,CONSOLE_FONT_BOLD,&stCornerLocation,"%c",getTableCellCorner(BOTTOM_RIGHT_CORNER,stTableSettings->BORDER_STYLE,chColumn,chRow,stTableSettings->chColumnsCount,stTableSettings->chRowsCount));
        resetSavedPreviousCursorPosition();
        stTableSettings->chSelectedCellRow=chRow;
        stTableSettings->chSelectedCellColumn=chColumn;
    }
}
void drawAlignedTable(double dbHorizontalAlign,double dbVerticalAlign,u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,u8 chColumnsCount,u8 chRowsCount,u8 chCellWidth,u8 chCellHeight,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS BGCOLOR,struct stTable *stTableSettings) {
double dbTextContainerX[2]={0,chCellWidth*chColumnsCount},dbTextContainerY[2]={0,chRowsCount*chCellHeight};
    if (chMinColumn>chMaxColumn) {
        permutePointers((void *) &chMinColumn,(void *) &chMaxColumn);
    }
    if (chMinRow>chMaxRow) {
        permutePointers((void *) &chMinRow,(void *) &chMaxRow);
    }
    drawTable(getRoundNumber(getPolyContainerPosition(&dbTextContainerY[0],2,chMinRow,chMaxRow,dbVerticalAlign)),getRoundNumber(getPolyContainerPosition(&dbTextContainerX[0],2,chMinColumn,chMaxColumn,dbHorizontalAlign)),chColumnsCount,chRowsCount,chCellWidth,chCellHeight,BORDER_STYLE,BORDER_COLOR,BGCOLOR,stTableSettings);
}
void *updateBlinkTexts(void *stBlinkTexts) {
bool blnBlinking=true;
unsigned char i;
unsigned int intMinInterval=-1;
struct stBlinkTextsGroup *pBlinkTextsGroup=(struct stBlinkTextsGroup *) stBlinkTexts;
    for (i=0;i<pBlinkTextsGroup->chBlinkTextsCount;i++) {
        intMinInterval=MIN(pBlinkTextsGroup->stBlinkTexts[i].stBlinkTimer.intInterval,intMinInterval);
    }
    intMinInterval=intMinInterval*1000;
    while (blnBlinking) {
        if (LWP_MutexLock(pBlinkTextsGroup->mtxThread)) {
            blnBlinking=false;
            for (i=0;i<pBlinkTextsGroup->chBlinkTextsCount;i++) {
                if ((blnBlinking=pBlinkTextsGroup->stBlinkTexts[i].stBlinkTimer.blnRunning)) {
                    if (isExpiredTimer(&pBlinkTextsGroup->stBlinkTexts[i].stBlinkTimer)) {
                        printStyledText(pBlinkTextsGroup->stBlinkTexts[i].stTextLocation.intRow,pBlinkTextsGroup->stBlinkTexts[i].stTextLocation.intColumn,pBlinkTextsGroup->stBlinkTexts[i].FONT_BGCOLOR,pBlinkTextsGroup->stBlinkTexts[i].FONT_FGCOLOR,pBlinkTextsGroup->stBlinkTexts[i].FONT_WEIGHT,&pBlinkTextsGroup->stBlinkTexts[i].stTextLocation,"%*s",strlen(pBlinkTextsGroup->stBlinkTexts[i].strBlinkText),pBlinkTextsGroup->stBlinkTexts[i].blnHide?"":pBlinkTextsGroup->stBlinkTexts[i].strBlinkText);
                        resetSavedPreviousCursorPosition();
                        pBlinkTextsGroup->stBlinkTexts[i].blnHide=!pBlinkTextsGroup->stBlinkTexts[i].blnHide;
                        fflush(stdout);
                    }
                }
                else {
                    blnBlinking=(blnBlinking || (pBlinkTextsGroup->stBlinkTexts[i].strBlinkText!=NULL));
                }
            }
            LWP_MutexUnlock(pBlinkTextsGroup->mtxThread);
        }
        usleep(intMinInterval);
    }
    return NULL;
}
void printBlinkText(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,unsigned short int intTimerInterval,struct stBlinkText *stBlinkTextSettings,const char *strFormatBlinkText,...) {
va_list pArguments;
static char strBlinkText[1024];
struct stConsoleCursorLocation stTexteLocation;
    va_start(pArguments,strFormatBlinkText);
    vsnprintf(strBlinkText,sizeof(strBlinkText),strFormatBlinkText,pArguments);
    va_end(pArguments);
    stBlinkTextSettings->strBlinkText=getCloneString(strBlinkText);
    stBlinkTextSettings->stTextLocation.intColumn=getTextBoxColumn(chColumn);
    stBlinkTextSettings->stTextLocation.intRow=getTextBoxRow(chRow);
    stBlinkTextSettings->FONT_FGCOLOR=FONT_FGCOLOR;
    stBlinkTextSettings->FONT_BGCOLOR=FONT_BGCOLOR;
    stBlinkTextSettings->FONT_WEIGHT=FONT_WEIGHT;
    stBlinkTextSettings->blnHide=false;
    initTimer(&stBlinkTextSettings->stBlinkTimer,intTimerInterval);
    printStyledText(stBlinkTextSettings->stTextLocation.intRow,stBlinkTextSettings->stTextLocation.intColumn,FONT_BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT,&stTexteLocation,"%s",strBlinkText);
}
void runBlinkTexts(struct stBlinkTextsGroup *stBlinkTexts) {
    stBlinkTexts->intThreadId=LWP_THREAD_NULL;
    stBlinkTexts->mtxThread=LWP_MUTEX_NULL;
    if (!LWP_MutexInit(&stBlinkTexts->mtxThread,false)) {
        LWP_CreateThread(&stBlinkTexts->intThreadId,(void *(*)(void *)) updateBlinkTexts,(void *)stBlinkTexts,NULL,0,64);
    }
}
void destroyBlinkTexts(struct stBlinkTextsGroup *stBlinkTexts) {
unsigned char i;
    if (stBlinkTexts->intThreadId!=LWP_THREAD_NULL) {
        while (LWP_MutexLock(stBlinkTexts->mtxThread)) {}
        for (i=0;i<stBlinkTexts->chBlinkTextsCount;i++) {
            stBlinkTexts->stBlinkTexts[i].stBlinkTimer.blnRunning=false;
            free(stBlinkTexts->stBlinkTexts[i].strBlinkText);
            stBlinkTexts->stBlinkTexts[i].strBlinkText=NULL;
        }
        LWP_MutexUnlock(stBlinkTexts->mtxThread);
        LWP_JoinThread(stBlinkTexts->intThreadId,NULL);
        LWP_MutexDestroy(stBlinkTexts->mtxThread);
        stBlinkTexts->intThreadId=LWP_THREAD_NULL;
        stBlinkTexts->mtxThread=LWP_MUTEX_NULL;
    }
}
void destroyDynamicStyledText(struct stDynamicStyledText *stDynamicStyledTextSettings) {
    if (stDynamicStyledTextSettings->strDynamicStyledText!=NULL) {
        free(stDynamicStyledTextSettings->strDynamicStyledText);
        stDynamicStyledTextSettings->strDynamicStyledText=NULL;
    }
}
void printDynamicStyledText(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,struct stDynamicStyledText *stDynamicStyledTextSettings,const char *strFormatText,...) {
va_list pArguments;
static char strDynamicStyledText[256];
    va_start(pArguments,strFormatText);
    vsnprintf(strDynamicStyledText,sizeof(strDynamicStyledText),strFormatText,pArguments);
    va_end(pArguments);
    stDynamicStyledTextSettings->strDynamicStyledText=getCloneString(strDynamicStyledText);
    printStyledText(chRow,chColumn,FONT_BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT,&stDynamicStyledTextSettings->stDynamicStyledTextLocation,"%s",strDynamicStyledText);
}
void updateDynamicStylesText(enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,struct stDynamicStyledText *stDynamicStyledTextSettings) {
    printStyledText(stDynamicStyledTextSettings->stDynamicStyledTextLocation.intRow,stDynamicStyledTextSettings->stDynamicStyledTextLocation.intColumn,FONT_BGCOLOR,FONT_FGCOLOR,FONT_WEIGHT,&stDynamicStyledTextSettings->stDynamicStyledTextLocation,"%s",stDynamicStyledTextSettings->strDynamicStyledText);
    resetSavedPreviousCursorPosition();
}
/*
void drawCommandsBar(unsigned char chCommandsCount,bool blnStatusBar,struct stCommandsBar *stCommandsBarSettings) {
int intConsoleColumnsCount,intConsoleRowsCount;
    CON_GetMetrics(&intConsoleColumnsCount,&intConsoleRowsCount);
    stCommandsBarSettings->chCommandsCount=chCommandsCount;
    stCommandsBarSettings->chCommandItemsCount=0;
    stCommandsBarSettings->stStatusBarLocation.intColumn=0;
    intConsoleColumnsCount--;
    saveCursorPosition();
    if (blnStatusBar) {
        intConsoleRowsCount--;
        stCommandsBarSettings->stStatusBarLocation.intRow=intConsoleRowsCount;
        setCursorPosition(intConsoleRowsCount,0);
        printf("%*s",intConsoleColumnsCount,"");
        intConsoleRowsCount--;
        setCursorPosition(intConsoleRowsCount,0);
        printRepeatString(intConsoleColumnsCount,"%c",HORIZONTAL_SINGLE_BORDER);
    }
    else {
        stCommandsBarSettings->stStatusBarLocation.intRow=-1;
    }
    if (chCommandsCount) {
        stCommandsBarSettings->stCommandsBarLocation.intRow=intConsoleRowsCount-getRoundNumber(((double)chCommandsCount)/2);
        stCommandsBarSettings->stCommandsBarLocation.intColumn=0;
        while (intConsoleRowsCount>stCommandsBarSettings->stCommandsBarLocation.intRow) {
            intConsoleRowsCount--;
            setCursorPosition(intConsoleRowsCount,0);
            printf("%*s",intConsoleColumnsCount,"");
        }
        setCursorPosition(stCommandsBarSettings->stCommandsBarLocation.intRow-1,0);
        printRepeatString(intConsoleColumnsCount,"%c",HORIZONTAL_SINGLE_BORDER);
    }
    resetSavedCursorPosition();
}
void setStatusBar(struct stCommandsBar *stCommandsBarSettings,enum CONSOLE_FONT_COLORS BGCOLOR,enum CONSOLE_FONT_COLORS FONT_COLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,const char *strFormatText,...) {
va_list pArguments;
static char strStatusText[256];
struct stConsoleCursorLocation stConcatLabelLocation;
    if (stCommandsBarSettings->stStatusBarLocation.intRow!=-1) {
        va_start(pArguments,strFormatText);
        vsnprintf(strStatusText,sizeof(strStatusText),strFormatText,pArguments);
        va_end(pArguments);
        printLabel(stCommandsBarSettings->stStatusBarLocation.intRow,stCommandsBarSettings->stStatusBarLocation.intColumn,BGCOLOR,FONT_COLOR,FONT_WEIGHT,getConsoleColumnsCount()-1,&stConcatLabelLocation,"%s",strStatusText);
    }
}
void addCommandsBarItem(struct stCommandsBar *stCommandsBarSettings,s32 *intMappedPadKeys,unsigned char chMappedPadKeysCount,const char *strFormatText,...) {
char chColumnCounts,chPadsKeyLabels,*strPadsKeyLabels[CONTROLLERS_COUNT];
va_list pArguments;
static char strCommandText[256];
    if (stCommandsBarSettings->chCommandItemsCount<stCommandsBarSettings->chCommandsCount) {
        saveCursorPosition();
        chColumnCounts=((getConsoleColumnsCount()-1)/2)-1;
        setCursorPosition(stCommandsBarSettings->stCommandsBarLocation.intRow+stCommandsBarSettings->chCommandItemsCount/2,stCommandsBarSettings->stCommandsBarLocation.intColumn+(stCommandsBarSettings->chCommandItemsCount % 2)*(2+chColumnCounts));
        while (chMappedPadKeysCount) {
            chMappedPadKeysCount--;
            chPadsKeyLabels=getPadsKeyLabels(intMappedPadKeys[chMappedPadKeysCount],&strPadsKeyLabels[0]);
            chColumnCounts=chColumnCounts-printJoinedStringArrayValues("%s","/",(const char **) &strPadsKeyLabels[0],chPadsKeyLabels);
            if (chMappedPadKeysCount) {
                printf("|");
                chColumnCounts--;
            }
        }
        if (chColumnCounts>0) {
            va_start(pArguments,strFormatText);
            vsnprintf(strCommandText,sizeof(strCommandText),strFormatText,pArguments);
            va_end(pArguments);
            printf("%-*.*s",(unsigned int) chColumnCounts,(unsigned int) chColumnCounts,strCommandText);
        }
        resetSavedCursorPosition();
        stCommandsBarSettings->chCommandItemsCount=stCommandsBarSettings->chCommandItemsCount+1;
    }
}

*/
