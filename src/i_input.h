//DOOM3D
//system specific input interface
#ifndef __I_INPUT__
#define __I_INPUT__


#include "doomtype.h"

#ifdef __GNUG__
#pragma interface
#endif

extern dboolean GrabMouse;

void I_InitInput (void);

void I_ShutdownInput(void);

void I_ProcessInput(void);

void I_ReacquireInput(void);

extern int	UseMouse[2];
extern int	UseJoystick;


#endif
