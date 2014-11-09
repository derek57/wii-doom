#ifndef _IMAGE_H_
#define _IMAGE_H_
#include <gctypes.h>
#include "pngu.h"
IMGCTX getPngImageRessources(const void *imgData,PNGUPROP *imgProperties);
bool drawPng(const void *imgData,u32 intLeft,u32 intTop,u32 intWidth,u32 intHeight,void *pFramebuffer);
#endif
