#ifndef __PSP_H__
#define __PSP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

//#include <pspctrl.h>


#define    LINESIZE        512
#define    PIXELSIZE       1
#define FRAMESIZE                          0x44000
#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 272

unsigned long new_pad;
unsigned long old_pad;
unsigned long now_pad;
//static SceCtrlData ctl;

enum { 
    TYPE_DIR=0x10, 
    TYPE_FILE=0x20 
};
 
typedef struct dirent { 
    unsigned long unk0; 
    unsigned long type; 
    unsigned long size; 
    unsigned long unk[19]; 
    char name[0x108]; 
	unsigned char dmy[128];
} dirent_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PSP_H__ */
