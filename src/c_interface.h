#ifndef C_INTERFACE_H
#define C_INTERFACE_H

#define DEF_DLL_NAME "c_sw8.dll"

#include "common.h"
#include "doomtype.h"

void C_Init(void);

void C_Shutdown(void);

extern apiout_t C_API;
extern dboolean interfacevalid;
extern char *DllName;
extern dboolean AllowGLNodes;
extern LPGUID	pD3DDeviceGUID;

#endif
