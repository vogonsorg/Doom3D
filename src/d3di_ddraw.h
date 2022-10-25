#ifndef D3DI_DDRAW_H
#define D3DI_DDRAW_H

#include "i_ddraw.h"
#include "t_fixed.h"
#include <d3d.h>

typedef struct
{
    D3DVALUE	x;
    D3DVALUE	y;
    D3DVALUE	z;
    D3DCOLOR	color;
    D3DVALUE	tu;
    D3DVALUE	tv;
}DOOM3DVERTEX;

#define FVF_DOOM3DVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

extern LPDIRECTDRAWSURFACE4	pDDSBack;

extern LPDIRECT3D3			pD3D;
extern LPDIRECT3DDEVICE3	pD3DDevice;
extern LPDIRECT3DVIEWPORT3	pD3DVPCurrent;
extern DDPIXELFORMAT		TexturePixelFormat;
extern DDPIXELFORMAT		AlphaTexturePixelFormat;
extern LPDIRECTDRAWPALETTE	pDDPTexture;

extern LPDIRECTDRAWSURFACE4	pDDSScreens[5];

#endif