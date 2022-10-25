//DOOM3D
//system specific input interface
#ifndef __I_WINDOZ__
#define __I_WINDOZ__


#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

extern HWND hMainWnd;
extern HINSTANCE hAppInst;
extern RECT WindowPos;
extern dboolean ShowScreenText;

void I_InitWindows (void);

void I_ShutdownWindows(void);//:)

void I_ProcessWindows(void);

void I_SetWindowSize(void);
#endif
