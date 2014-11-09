#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <ogc/color.h>
#include <ogc/consol.h>
#include <ogc/video.h>
#include <ogc/system.h>

#include "image.h"
#include "pngu.h"

#include "video.h"
#include "d2x-cios-installer.h"
#include "libgeometry.h"
#include "gui.h"
enum CONSOLE_FONT_COLORS CURRENT_FONT_BGCOLOR=DEFAULT_FONT_BGCOLOR,PREVIOUS_FONT_BGCOLOR=DEFAULT_FONT_BGCOLOR,CURRENT_FONT_FGCOLOR=DEFAULT_FONT_FGCOLOR,PREVIOUS_FONT_FGCOLOR=DEFAULT_FONT_FGCOLOR;
enum CONSOLE_FONT_WEIGHTS PREVIOUS_FONT_WEIGHT=DEFAULT_FONT_WEIGHT,CURRENT_FONT_WEIGHT=DEFAULT_FONT_WEIGHT;
unsigned char CONSOLE_CURSOR_CURRENT_COLUMN=0,CONSOLE_CURSOR_PREVIOUS_COLUMN=0,CONSOLE_CURSOR_CURRENT_ROW=0,CONSOLE_CURSOR_PREVIOUS_ROW=0;
void clearConsole() {
	printf("\x1b[2J");
	fflush(stdout);
}
void clearConsoleLine() {
	printf("\r\x1b[2K\r");
	fflush(stdout);
}
void setFontFgColor(enum CONSOLE_FONT_COLORS FONT_COLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT) {
    if (FONT_COLOR==CONSOLE_FONT_CURRENT_COLOR) {
        FONT_COLOR=CURRENT_FONT_FGCOLOR;
    }
    if (CONSOLE_FONT_CURRENT_WEIGHT==FONT_WEIGHT) {
        FONT_WEIGHT=CURRENT_FONT_WEIGHT;
    }
    PREVIOUS_FONT_FGCOLOR=CURRENT_FONT_FGCOLOR;
    PREVIOUS_FONT_WEIGHT=CURRENT_FONT_WEIGHT;
    printf("\x1b[%u;%um",(u8) FONT_COLOR+30,FONT_WEIGHT);
    CURRENT_FONT_FGCOLOR=FONT_COLOR;
    CURRENT_FONT_WEIGHT=FONT_WEIGHT;
}
void setFontBgColor(enum CONSOLE_FONT_COLORS FONT_COLOR) {
    if (FONT_COLOR==CONSOLE_FONT_CURRENT_COLOR) {
        FONT_COLOR=CURRENT_FONT_FGCOLOR;
    }
    PREVIOUS_FONT_BGCOLOR=CURRENT_FONT_BGCOLOR;
    printf("\x1b[%um",(u8) FONT_COLOR+40);
    CURRENT_FONT_BGCOLOR=FONT_COLOR;
}
void setFontStyle(enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT) {
    setFontBgColor(FONT_BGCOLOR);
    setFontFgColor(FONT_FGCOLOR,FONT_WEIGHT);
}
void resetDefaultFontSyle() {
    setFontStyle(DEFAULT_FONT_BGCOLOR,DEFAULT_FONT_FGCOLOR,DEFAULT_FONT_WEIGHT);
}
void resetPreviousFgColor() {
    setFontFgColor(PREVIOUS_FONT_FGCOLOR,PREVIOUS_FONT_WEIGHT);
}
void resetPreviousBgColor() {
    setFontBgColor(PREVIOUS_FONT_BGCOLOR);
}
void resetPreviousFontStyle() {
    setFontStyle(PREVIOUS_FONT_BGCOLOR,PREVIOUS_FONT_FGCOLOR,PREVIOUS_FONT_WEIGHT);
}
void initConsole(const void *imgBgData,enum CONSOLE_COLORS CONSOLE_COLOR,const char *strSplashScreenMessage,double dbLeft,double dbTop,double dbWidth,double dbHeight) {
PNGUPROP imgProperties;
IMGCTX imgContext;
void *pFramebuffer=NULL;
GXRModeObj *pRmode=NULL;
double dbConsoleFrameX[2]={dbLeft,dbLeft},dbConsoleFrameY[2]={dbTop,dbTop},dbBgImgXScaleFactor=1,dbBgImgYScaleFactor=1,dbReferenceWidth,dbReferenceHeight;
int intConsoleColumnsCount,intConsoleRowsCount;
struct stConsoleCursorLocation stTexteLocation;
    VIDEO_Init();
    pRmode=VIDEO_GetPreferredMode(NULL);
    pFramebuffer=MEM_K0_TO_K1(SYS_AllocateFramebuffer(pRmode));
    VIDEO_ClearFrameBuffer(pRmode,pFramebuffer,CONSOLE_COLOR);
    VIDEO_Configure(pRmode);
    VIDEO_SetNextFramebuffer(pFramebuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(pRmode->viTVMode&VI_NON_INTERLACE) {
        VIDEO_WaitVSync();
    }
    dbWidth=fabs(dbWidth);
    dbHeight=fabs(dbHeight);
    if ((imgContext=getPngImageRessources(imgBgData,&imgProperties))) {
        dbReferenceWidth=imgProperties.imgWidth;
        dbReferenceHeight=imgProperties.imgHeight;
        dbBgImgXScaleFactor=pRmode->fbWidth/imgProperties.imgWidth;
        dbBgImgYScaleFactor=pRmode->xfbHeight/imgProperties.imgHeight;
    }
    else {
        dbReferenceWidth=pRmode->fbWidth;
        dbReferenceHeight=pRmode->xfbHeight;
    }
    if (dbWidth<=1) {
        dbWidth=dbReferenceWidth*dbWidth;
        dbConsoleFrameX[0]=0;
        dbConsoleFrameX[1]=dbWidth;
    }
    if (dbHeight<=1) {
        dbHeight=dbReferenceHeight*dbHeight;
        dbConsoleFrameY[0]=0;
        dbConsoleFrameY[1]=dbHeight;
    }
    CON_InitEx(pRmode,(s32) getRoundNumber(getPolyContainerPosition(&dbConsoleFrameX[0],2,0,dbReferenceWidth,dbLeft)*dbBgImgXScaleFactor),(s32) getRoundNumber(getPolyContainerPosition(&dbConsoleFrameY[0],2,0,dbReferenceHeight,dbTop)*dbBgImgYScaleFactor),(s32) getRoundNumber(dbWidth*dbBgImgXScaleFactor),(s32) getRoundNumber(dbHeight*dbBgImgYScaleFactor));
    if (imgContext) {
        PNGU_DECODE_TO_COORDS_YCbYCr(imgContext,0,0,imgProperties.imgWidth,imgProperties.imgHeight,pRmode->fbWidth,pRmode->xfbHeight,pFramebuffer);
        PNGU_ReleaseImageContext(imgContext);
    }
    resetDefaultFontSyle();
    if (*strSplashScreenMessage) {
        CON_GetMetrics(&intConsoleColumnsCount,&intConsoleRowsCount);
        printAlignedText(ALIGN_CENTER,ALIGN_MIDDLE,0,0,intConsoleRowsCount-1,intConsoleColumnsCount-1,true,true,&stTexteLocation,"%s",strSplashScreenMessage);
    }

}
void setCursorPosition(u8 intRow,u8 intColumn) {
    printf("\x1b[%u;%uH",intRow,intColumn);
}
void saveCursorPosition() {
    CONSOLE_CURSOR_PREVIOUS_COLUMN=CONSOLE_CURSOR_CURRENT_COLUMN;
    CONSOLE_CURSOR_PREVIOUS_ROW=CONSOLE_CURSOR_CURRENT_ROW;
    CONSOLE_CURSOR_CURRENT_COLUMN=getConsoleColumn();
    CONSOLE_CURSOR_CURRENT_ROW=getConsoleRow();
}
void resetSavedPreviousCursorPosition() {
    setCursorPosition(CONSOLE_CURSOR_PREVIOUS_ROW,CONSOLE_CURSOR_PREVIOUS_COLUMN);
    saveCursorPosition();
}
void resetSavedCursorPosition() {
    setCursorPosition(CONSOLE_CURSOR_CURRENT_ROW,CONSOLE_CURSOR_CURRENT_COLUMN);
}
int getConsoleColumnsCount() {
int intConsoleColumnsCount,intConsoleRowsCount;
    CON_GetMetrics(&intConsoleColumnsCount,&intConsoleRowsCount);
    return intConsoleColumnsCount;
}
int getConsoleRowsCount() {
int intConsoleColumnsCount,intConsoleRowsCount;
    CON_GetMetrics(&intConsoleColumnsCount,&intConsoleRowsCount);
    return intConsoleRowsCount;
}
int getConsoleColumn() {
int intConsoleColumn,intConsoleRow;
    CON_GetPosition(&intConsoleColumn,&intConsoleRow);
    return intConsoleColumn;
}
int getConsoleRow() {
int intConsoleColumn,intConsoleRow;
    CON_GetPosition(&intConsoleColumn,&intConsoleRow);
    return intConsoleRow;
}
enum CONSOLE_FONT_COLORS getSavedBgColor() {
    return CURRENT_FONT_BGCOLOR;
}
enum CONSOLE_FONT_COLORS getSavedFgColor() {
    return CURRENT_FONT_FGCOLOR;
}
enum CONSOLE_FONT_WEIGHTS getSavedFontWeight() {
    return CURRENT_FONT_WEIGHT;
}
u8 getSavedConsoleRow() {
    return CONSOLE_CURSOR_CURRENT_ROW;
}
u8 getSavedConsoleColumn() {
    return CONSOLE_CURSOR_CURRENT_COLUMN;
}
