#ifndef __I_PSP_H__
#define __I_PSP_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void pgInit();
void pgScreenFrame(long mode,long frame);
void pgFillvram(unsigned long color);
void pgScreenFlipV();
void pgScreenFlip();
char *pgGetVramAddr(unsigned long x,unsigned long y);
unsigned short rgb2col(unsigned char r,unsigned char g,unsigned char b);
void pgWaitV();
void pgDrawRectangle(int x, int y, int x2, int y2, unsigned long color);
void mh_print(int x,int y,/*unsigned*/ char *str,int col,int backcol,int fill);
unsigned short rgb2col(unsigned char r,unsigned char g,unsigned char b);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
