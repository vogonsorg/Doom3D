#ifndef C_DLL_H
#define C_DLL_H

#include "common.h"

extern int ScreenWidth;
extern int ScreenHeight;

extern apiin_t D_API;

extern void (*I_Error)(char *error, ...);
extern void (*I_Printf)(char *fmt, ...);

#endif