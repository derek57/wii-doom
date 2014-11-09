#ifndef _GUI_H_
#define _GUI_H_
#include <gctypes.h>
#include "video.h"
//#include "libtime.h"
//#include "libgeometry.h"
#include <ogc/lwp.h>
#include <ogc/mutex.h>
#define AUTOSIZE 255
struct stConsoleCursorLocation stTexteLocation;

struct stTimer {
//    clock_t clkStartTime;
    unsigned short int intInterval;
    bool blnRunning;
};
enum CELL_CORNERS {
    TOP_LEFT_CORNER=0,
    TOP_RIGHT_CORNER=3,
    BOTTOM_LEFT_CORNER=12,
    BOTTOM_RIGHT_CORNER=15
};
enum BORDER_SYMBOLS {
    HORIZONTAL_SINGLE_BORDER=196,
    MIDDLE_TOP_SINGLE_JUNCTION=194,
    LEFT_MIDDLE_SINGLE_JUNCTION=195,
    CROSS_SINGLE_JUNCTION=197,
    LEFT_TOP_SINGLE_CORNER=218,
    RIGHT_MIDDLE_SINGLE_JUNCTION=180,
    RIGHT_TOP_SINGLE_CORNER=191,
    MIDDLE_BOTTOM_SINGLE_JUNCTION=193,
    LEFT_BOTTOM_SINGLE_CORNER=192,
    RIGHT_BOTTOM_SINGLE_CORNER=217,
    VERTICAL_SINGLE_BORDER=179
};
enum BORDER_ALTERNATIVE_SYMBOLS {
    HORIZONTAL_DOUBLE_BORDER=205,
    MIDDLE_TOP_DOUBLE_JUNCTION=203,
    LEFT_MIDDLE_DOUBLE_JUNCTION=204,
    CROSS_DOUBLE_JUNCTION=206,
    LEFT_TOP_DOUBLE_CORNER=201,
    RIGHT_MIDDLE_DOUBLE_JUNCTION=185,
    RIGHT_TOP_DOUBLE_CORNER=187,
    MIDDLE_BOTTOM_DOUBLE_JUNCTION=202,
    LEFT_BOTTOM_DOUBLE_CORNER=200,
    RIGHT_BOTTOM_DOUBLE_CORNER=188,
    VERTICAL_DOUBLE_BORDER=186
};
enum BORDERS {
    NO_BORDER=0,
    LEFT_BORDER=1,
    TOP_BORDER=2,
    RIGHT_BORDER=4,
    BOTTOM_BORDER=8,
    ALL_BORDERS=LEFT_BORDER|TOP_BORDER|RIGHT_BORDER|BOTTOM_BORDER
};
enum BORDER_STYLES {
    SINGLE_BORDER=0,
    DOUBLE_BORDER=1,
    STAR_BORDER=42
};
enum FRAME_JUNCTIONS {
    NO_JUNCTION=0,
    HORIZONTAL_JUNCTION=1,
    VERTICAL_JUNCTION=2,
    ALL_JUNCTIONS=HORIZONTAL_JUNCTION|VERTICAL_JUNCTION
};
struct stProgressBar {
    unsigned char chProgressBarTextSize;
    unsigned int intOperationsCount;
    unsigned int intValue;
    unsigned char chProgressBarSize;
    enum CONSOLE_FONT_COLORS PROGRESSBAR_COLOR;
    struct stConsoleCursorLocation stProgressBarLocation;
    struct stConsoleCursorLocation stProgressBarTextLocation;
};
struct stLabel {
    unsigned char chLabelSize;
    struct stConsoleCursorLocation stLabelLocation;
};
struct stTable {
    struct stConsoleCursorLocation stTableLocation;
    unsigned char chColumnsCount;
    unsigned char chRowsCount;
    unsigned char chCellWidth;
    unsigned char chCellHeight;
    unsigned char chSelectedCellRow;
    unsigned char chSelectedCellColumn;
    enum BORDER_STYLES BORDER_STYLE;
    enum CONSOLE_FONT_COLORS BORDER_COLOR;
    enum CONSOLE_FONT_COLORS BGCOLOR;
};
struct stBlinkText {
    char *strBlinkText;
    struct stConsoleCursorLocation stTextLocation;
    enum CONSOLE_FONT_COLORS FONT_FGCOLOR;
    enum CONSOLE_FONT_COLORS FONT_BGCOLOR;
    enum CONSOLE_FONT_COLORS FONT_WEIGHT;
    struct stTimer stBlinkTimer;
    bool blnHide;
};
struct stBlinkTextsGroup {
    unsigned char chBlinkTextsCount;
    mutex_t mtxThread;
    lwp_t intThreadId;
    struct stBlinkText *stBlinkTexts;
};
struct stDynamicStyledText {
    char *strDynamicStyledText;
    struct stConsoleCursorLocation stDynamicStyledTextLocation;
};
struct stCommandsBar {
    struct stConsoleCursorLocation stStatusBarLocation;
    struct stConsoleCursorLocation stCommandsBarLocation;
    unsigned char chCommandsCount;
    unsigned char chCommandItemsCount;
};
u8 getTextBoxRow(u8 chRow);
u8 getTextBoxColumn(u8 chColumn);
u8 getTextBoxAutoWidth(enum BORDERS BORDER_TYPE,const char *strFormatValue,...);
u8 getTextBoxAutoHeight(enum BORDERS BORDER_TYPE,const char *strFormatValue,...);
void getTextBoxPositions(u8 *chMinRow,u8 *chMinColumn,u8 *chMaxRow,u8 *chMaxColumn,enum BORDERS BORDER_TYPE,const char *strFormatValue,...);
void printLocatedText(u8 chRow,u8 chColumn,struct stConsoleCursorLocation *stTexteLocation,const char *strTexteFormat,...);
void printTextInColumnsRange(u8 chRow,u8 chColumn,u8 chMinColumn,u8 chMaxColumn,struct stConsoleCursorLocation *stTexteLocation,const char *strTexteFormat,...);
void printAlignedText(double dbHorizontalAlign,double dbVerticalAlign,u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,bool blnMultiLine,bool blnHideOverflowText,struct stConsoleCursorLocation *stTexteLocation,const char *strTexteFormat,...);
void printStyledText(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,struct stConsoleCursorLocation *stTexteLocation,const char *fmt, ...);
void printAlignedStyledText(double dbColumn,double dbRow,u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,bool blnMultiLine,bool blnHideOverflowText,struct stConsoleCursorLocation *strTexteLocation,const char *strTexteFormat,...);
unsigned char getBorderSymbol(enum BORDER_STYLES BORDER_STYLE,enum BORDER_SYMBOLS BORDER_SYMBOL);
void drawSelectionFrame(u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS BORDER_BGCOLOR);
void drawFrame(u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,enum BORDERS BORDER_TYPE,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS FRAME_COLOR,enum FRAME_JUNCTIONS TOP_LEFT_JUNCTION,enum FRAME_JUNCTIONS TOP_RIGHT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_LEFT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_RIGHT_JUNCTION);
void printTextBox(u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,double dbHorizontalAlign,double dbVerticalAlign,enum BORDERS BORDER_TYPE,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum FRAME_JUNCTIONS TOP_LEFT_JUNCTION,enum FRAME_JUNCTIONS TOP_RIGHT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_LEFT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_RIGHT_JUNCTION,enum CONSOLE_FONT_COLORS BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,bool blnMultiLine,bool blnHideOverflowText,struct stConsoleCursorLocation *stTexteLocation,const char *strFormatValue,...);
void printAlignedTextBox(u8 chFrameMinRow,u8 chFrameMinColumn,u8 chFrameMaxRow,u8 chFrameMaxColumn,double dbRow,double dbColumn,double dbHeight,double dbWidth,double dbHorizontalAlign,double dbVerticalAlign,enum BORDERS BORDER_TYPE,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum FRAME_JUNCTIONS TOP_LEFT_JUNCTION,enum FRAME_JUNCTIONS TOP_RIGHT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_LEFT_JUNCTION,enum FRAME_JUNCTIONS BOTTOM_RIGHT_JUNCTION,enum CONSOLE_FONT_COLORS BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,bool blnMultiLine,bool blnHideOverflowText,struct stConsoleCursorLocation *stTexteLocation,const char *strFormatValue,...);
void drawLabel(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,const char *strLabelCaption,u8 chLabelSize,struct stLabel *stLabelSettings,struct stConsoleCursorLocation *stLabelCaptionLocation);
void printLabel(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,unsigned char chLabelSize,struct stConsoleCursorLocation *stConcatLabelLocation,const char *strFormatLabel,...);
void drawProgressBar(u8 chProgressBarRow,u8 chProgressBarColumn,u8 chProgressBarSize,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,enum CONSOLE_FONT_COLORS PROGRESS_BAR_BGCOLOR,u8 chProgressBarTextSize,u8 chProgressBarTextColumn,u8 chProgressBarTextRow,enum CONSOLE_FONT_COLORS PROGRESSCOLOR,unsigned int intOperationsCount,struct stProgressBar *stProgressBarSettings,struct stConsoleCursorLocation *stProgressBarLabelLocation,const char *strFormatLabel,...);
void updateProgressBar(struct stProgressBar *stProgressBarSettings,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,const char *strFormatProgressBarText,...);
void printLevelsBar(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS LEVELS_BAR_COLOR,enum CONSOLE_FONT_COLORS ACTIVE_LEVEL_COLOR,unsigned char chActiveLevelIndex,unsigned char chLevelsCount,struct stConsoleCursorLocation *stLevelBarLocation,const char *strFirstLevel,...);
unsigned char getTableCellCorner(enum CELL_CORNERS CORNER,enum BORDER_STYLES BORDER_SYLE,u8 chColumnId,u8 chRowId,u8 chColumnsCount,u8 chRowsCount);
unsigned char getTableCellCornerJunction(enum CELL_CORNERS CORNER,u8 chColumnId,u8 chRowId,u8 chColumnsCount,u8 chRowsCount);
void updateTableCell(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS BGCOLOR,enum CONSOLE_FONT_COLORS FONT_COLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,double dbHorizontalAlign,double dbVerticalAlign,struct stTable *stTableSettings,struct stConsoleCursorLocation *stCellTextLocation,const char *strFormatCellText,...);
void drawTable(u8 chRow,u8 chColumn,u8 chColumnsCount,u8 chRowsCount,u8 chCellWidth,u8 chCellHeight,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS BGCOLOR,struct stTable *stTableSettings);
void selectTableCell(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS BORDER_BGCOLOR,struct stTable *stTableSettings);
void drawAlignedTable(double dbHorizontalAlign,double dbVerticalAlign,u8 chMinRow,u8 chMinColumn,u8 chMaxRow,u8 chMaxColumn,u8 chColumnsCount,u8 chRowsCount,u8 chCellWidth,u8 chCellHeight,enum BORDER_STYLES BORDER_STYLE,enum CONSOLE_FONT_COLORS BORDER_COLOR,enum CONSOLE_FONT_COLORS BGCOLOR,struct stTable *stTableSettings);
void printBlinkText(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,unsigned short int intTimerInterval,struct stBlinkText *stBlinkTextSettings,const char *strFormatBlinkText,...);
void runBlinkTexts(struct stBlinkTextsGroup *stBlinkTexts);
void destroyBlinkTexts(struct stBlinkTextsGroup *stBlinkTexts);
void destroyDynamicStyledText(struct stDynamicStyledText *stDynamicStyledTextSettings);
void printDynamicStyledText(u8 chRow,u8 chColumn,enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,struct stDynamicStyledText *stDynamicStyledTextSettings,const char *strFormatText,...);
void updateDynamicStylesText(enum CONSOLE_FONT_COLORS FONT_BGCOLOR,enum CONSOLE_FONT_COLORS FONT_FGCOLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,struct stDynamicStyledText *stDynamicStyledTextSettings);
void drawCommandsBar(unsigned char chCommandsCount,bool blnStatusBar,struct stCommandsBar *stCommandsBarSettings);
void setStatusBar(struct stCommandsBar *stCommandsBarSettings,enum CONSOLE_FONT_COLORS BGCOLOR,enum CONSOLE_FONT_COLORS FONT_COLOR,enum CONSOLE_FONT_WEIGHTS FONT_WEIGHT,const char *strFormatText,...);
void addCommandsBarItem(struct stCommandsBar *stCommandsBarSettings,s32 *intMappedPadKeys,unsigned char chMappedPadKeysCount,const char *strFormatText,...);
#endif
