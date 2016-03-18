#ifndef _SYS_H_
#define _SYS_H_


#include "../src/doomtype.h"


u32 boot2version;


// Prototypes
dboolean isIOSstub(u8 ios_number);
dboolean loadIOS(int ios);

void Sys_Init(void);
void Sys_Reboot(void);
void Sys_Shutdown(void);
void Sys_LoadMenu(void);

s32  Sys_GetCerts(signed_blob **, u32 *);


#endif

