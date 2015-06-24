#ifndef _SYS_H_
#define _SYS_H_

u32 boot2version;

/* Prototypes */
boolean isIOSstub(u8 ios_number);
boolean loadIOS(int ios);

void Sys_Init(void);
void Sys_Reboot(void);
void Sys_Shutdown(void);
void Sys_LoadMenu(void);

s32  Sys_GetCerts(signed_blob **, u32 *);

#endif
