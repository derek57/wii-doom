#include <sys/stat.h>

#include "doomdef.h"
#include "gui.h"
#include "m_misc.h"
#include "video.h"
#include "xmn_main.h"


#define WADRoot         "WADS/"
#define PWADPath        "WADS/PWAD/"
#define IWADPath        "WADS/IWAD/"


struct stConsoleCursorLocation stTexteLocation;

void Menu_Loop(void);

void drawMain()
{
    printStyledText(2, 0,CONSOLE_FONT_BLUE,CONSOLE_FONT_YELLOW, 
                    CONSOLE_FONT_BOLD,&stTexteLocation,
    "              CHOCOLATE DOOM for NINTENDO WII by NITR8 (RELEASE 1)              ");
    printStyledText(3, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,
                    CONSOLE_FONT_BOLD,&stTexteLocation,
    "  SELECT THE GAME'S MAIN IWAD. YOU CAN ALSO SELECT A PWAD AND DEH FILE TO LOAD  ");
    printStyledText(4, 0,CONSOLE_FONT_BLACK,CONSOLE_FONT_WHITE,
                    CONSOLE_FONT_BOLD,&stTexteLocation,
    "  ----------------------------------------------------------------------------  ");
}

void CreateDirs(void)
{
    char createWADRoot[121];
    char createPWADDir[121];
    char createIWADDir[121];

    M_StringCopy(createWADRoot, WADRoot, sizeof(createWADRoot));
    M_StringCopy(createPWADDir, PWADPath, sizeof(createPWADDir));
    M_StringCopy(createIWADDir, IWADPath, sizeof(createIWADDir));

    createWADRoot[120] = 0;
    createPWADDir[120] = 0;
    createIWADDir[120] = 0;

    mkdir(createWADRoot, 0755);
    mkdir(createPWADDir, 0755);
    mkdir(createIWADDir, 0755);
}


void drawDirectory()
{
    CreateDirs();

    Menu_Loop();

    drawMain();
}

