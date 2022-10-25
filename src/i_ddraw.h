#ifndef I_DDRAW_H
#define I_DDRAW_H

#include <ddraw.h>

extern LPDIRECTDRAW4		pDD;
extern LPDIRECTDRAWSURFACE4	pDDSPrimary;

#define SAFE_RELEASE(x) {if (x) (x)->lpVtbl->Release(x); (x)=NULL;}

#endif