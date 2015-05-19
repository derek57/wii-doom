#ifndef _WPAD_H_
#define _WPAD_H_

#include <wiiuse/wpad.h>

/* Prototypes */
s32  Wpad_Init(void);

void Wpad_Disconnect(void);

u32  Wpad_GetButtons(void);
u32  Wpad_WaitButtons(void);

bool Wpad_TimeButton(void);

// Routine to wait for a button from either the Wiimote or a gamecube
// controller. The return value will mimic the WPAD buttons to minimize
// the amount of changes to the original code, that is expecting only
// Wiimote button presses. Note that the "HOME" button on the Wiimote
// is mapped to the "SELECT" button on the Gamecube Ctrl. (wiiNinja 5/15/2009)
u32 WaitButtons(void);

#endif
