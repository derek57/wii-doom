#include <wiiuse/wpad.h>
#include <sdcard/wiisd_io.h>
#include <fat.h>

int wii_main()
{
    PAD_Init();

    // Init the wiimotes
    WPAD_Init();

    WPAD_SetIdleTimeout(60*5); // 5 minutes 

    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);

    WPAD_SetVRes(WPAD_CHAN_ALL, 320, 200);

    // Init the file system
    fatInitDefault();

    return 0;
}
