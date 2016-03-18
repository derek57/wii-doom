#include <ogcsys.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "sys.h"

#include <wiiuse/wpad.h>


/* Constants */
#define MAX_WIIMOTES        4


int start;


//
// [nitr8] UNUSED
//
/*
static void __Wpad_PowerCallback(s32 chan)
{
    // Poweroff console
    Sys_Shutdown();
}

s32 Wpad_Init(void)
{
    s32 ret;

    // Initialize Wiimote subsystem
    ret = WPAD_Init();

    if (ret < 0)
        return ret;

    // Set POWER button callback
    WPAD_SetPowerButtonCallback(__Wpad_PowerCallback);

    return ret;
}
*/

static u32 Wpad_GetButtons(void)
{
    u32 buttons = 0;
    u32 cnt;

    // Scan pads
    WPAD_ScanPads();

    // Get pressed buttons
    for (cnt = 0; cnt < MAX_WIIMOTES; cnt++)
        buttons |= WPAD_ButtonsDown(cnt);

    return buttons;
}

// Routine to wait for a button from either the Wiimote or a gamecube
// controller. The return value will mimic the WPAD buttons to minimize
// the amount of changes to the original code, that is expecting only
// Wiimote button presses. Note that the "HOME" button on the Wiimote
// is mapped to the "SELECT" button on the Gamecube Ctrl. (wiiNinja 5/15/2009)
u32 WaitButtons(void)
{
    u32 buttons = 0;

    // Wait for button pressing
    while (!buttons)
    {
        // Wii buttons
        buttons = Wpad_GetButtons();

        VIDEO_WaitVSync();
    }

    return buttons;
}

