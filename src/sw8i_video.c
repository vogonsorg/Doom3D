// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for DirectX
//
//-----------------------------------------------------------------------------
#include <stdlib.h>
#include <ddraw.h>

#include <stdarg.h>

#include "swi_video.h"

#include "i_gamma.h"
#include "swv_video.h"
#include "i_overlay.h"
#include "swi_ddraw.h"
#include "doomdef.h"

#include "c_sw.h"
#include "sw8_defs.h"

LPDIRECTDRAW4			pDD=NULL;
LPDIRECTDRAWSURFACE4	pDDSPrimary=NULL;
LPDIRECTDRAWSURFACE4	pDDSBack=NULL;
pixel_t					*BufferScreens[2];
int						ScreenNum;
LPDIRECTDRAWSURFACE4	pDDSScreens[2]={NULL, NULL};
LPDIRECTDRAWPALETTE		pDDPalette=NULL;

dboolean				InWindow;
dboolean				SurfaceLost=false;

unsigned int			*Pal32=NULL;

void I_WaitVBL(int count)
{
    Sleep(count);
}

void I_ShutdownGraphics(void)
{
    I_ShutdownOverlay();
    SAFE_RELEASE(pDDSScreens[0]);
    SAFE_RELEASE(pDDSScreens[1]);
    SAFE_RELEASE(pDDPalette);
    SAFE_RELEASE(pDDSPrimary);
    SAFE_RELEASE(pDD);
}



void I_WaitForFlip(void)
{
    HRESULT	hres;

    while (1)
    {
		hres=pDDSPrimary->lpVtbl->GetFlipStatus(pDDSPrimary, DDGFS_ISFLIPDONE);
		if (hres==DD_OK)
			return;
		if (hres!=DDERR_WASSTILLDRAWING)
			return;
			//I_Error("GetFlipStatus Failed");
    }
}

DWORD Loose2RGBBits(DWORD mask)
{
    DWORD	i;

    i=1;
    while (i&&!(i&mask))
		i<<=1;
    return(mask&~(i|(i<<1)));
}

void I_InitScreenSurface(void)
{
    DDSURFACEDESC2	ddsd;
    HRESULT		hres;

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
    ddsd.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
    hres=pDDSPrimary->lpVtbl->GetPixelFormat(pDDSPrimary, &ddsd.ddpfPixelFormat);
    if (hres!=DD_OK)
		I_Error("GetPixelFormat Failed");
    ddsd.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_LPSURFACE|DDSD_PITCH|DDSD_PIXELFORMAT|DDSD_CAPS;
    ddsd.dwWidth=ScreenWidth;
    ddsd.dwHeight=ScreenHeight;
    ddsd.lPitch=PIX2BYTE(ScreenWidth);
    ddsd.lpSurface=BufferScreens[0];
	/*
    ddsd.ddpfPixelFormat.dwFlags=DDPF_PALETTEINDEXED8|DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount=8;
	*/
    ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSScreens[0], NULL);
    if (hres!=DD_OK)
		I_Error("CreateSurface(Screen0) Failed %d(%d, %d)", hres, DDERR_INCOMPATIBLEPRIMARY, DDERR_INVALIDOBJECT);
    ddsd.lpSurface=BufferScreens[1];
    hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSScreens[1], NULL);
    if (hres!=DD_OK)
		I_Error("CreateSurface(Screen1) Failed %d(%d, %d)", hres, DDERR_INCOMPATIBLEPRIMARY, DDERR_INVALIDOBJECT);
}

void I_DoBltWindowed(void)
{
	HRESULT			hres;
	DDSURFACEDESC2	ddsd;
	byte			*src;
	unsigned int	*dest;
	int				remainder;
	byte			*end;
	int				line;

	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize=sizeof(DDSURFACEDESC2);
	//hres=pDDSPrimary->lpVtbl->Lock(pDDSPrimary, D_API.pWindowPos, &ddsd, DDLOCK_WAIT|DDLOCK_WRITEONLY, NULL);
	hres=pDDSBack->lpVtbl->Lock(pDDSBack, NULL, &ddsd, DDLOCK_WAIT|DDLOCK_WRITEONLY|DDLOCK_NOSYSLOCK, NULL);
	if (hres!=DD_OK)
		I_Error("Lock(Intermediate surface) Failed %d(%d %d %d %d %d %d)", hres, DDERR_SURFACEBUSY,DDERR_SURFACELOST,DDERR_INVALIDPARAMS,DDERR_INVALIDOBJECT, DDERR_OUTOFMEMORY, DDERR_WASSTILLDRAWING);
	dest=(unsigned int *)ddsd.lpSurface;
	end=src=screens[0];
	remainder=ddsd.lPitch-ScreenWidth*4;
	for (line=0;line<ScreenHeight;line++)
	{
		end+=ScreenWidth;
		while (src<end)
			*(dest++)=Pal32[*(src++)];
		((char *)dest)+=remainder;
	}
	pDDSBack->lpVtbl->Unlock(pDDSBack, NULL);
	hres=pDDSPrimary->lpVtbl->Blt(pDDSPrimary, D_API.pWindowPos, pDDSBack, NULL, DDBLT_WAIT, NULL);
	if (hres==DDERR_SURFACELOST)
		SurfaceLost=true;
}

void I_DoBlt(void)
{
    RECT	r;
    HRESULT	hres;

    r.left=r.top=0;
    r.right=ScreenWidth;
    r.bottom=ScreenHeight;
	if (!InWindow)
		I_WaitForFlip();

    if (InWindow)
		I_DoBltWindowed();
    else
    {
		hres=
		pDDSBack->lpVtbl->BltFast(pDDSBack, 0, 0, pDDSScreens[ScreenNum], &r, DDBLTFAST_WAIT);
		if (hres==DDERR_SURFACELOST)
			SurfaceLost=true;
    }
    ScreenNum=1-ScreenNum;
    screens[0]=BufferScreens[ScreenNum];
}

void I_StartDrawing(void)
{
	if (SurfaceLost&&(InWindow||!*D_API.pInBackground))
	{
		SurfaceLost=false;
		pDD->lpVtbl->RestoreAllSurfaces(pDD);
	}
	if (!InWindow)
		pDDSPrimary->lpVtbl->Flip(pDDSPrimary, NULL, DDFLIP_WAIT);
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
    I_DoBlt();
}


//
// I_ReadScreen
//
void I_ReadScreen (pixel_t* scr)
{
    memcpy (scr, screens[0], PIX2BYTE(ScreenWidth*ScreenHeight));
}


//
// Palette stuff.
//

int GetShiftBits(DWORD mask)
{
    int	n;

    n=0;
    while ((mask&1)==0)
    {
		mask>>=1;
		n++;
    }
    while (mask&1)
    {
		mask>>=1;
		n++;
    }
    return(n-8);
}

void I_SetPaletteWindowed(byte *palette)
{
	unsigned int	*dest;
	byte			*src;
	int				rbits;
	int				gbits;
	int				bbits;
	DDPIXELFORMAT	ddpf;
	HRESULT			hres;

	ddpf.dwSize=sizeof(DDPIXELFORMAT);
    hres=pDDSBack->lpVtbl->GetPixelFormat(pDDSBack, &ddpf);
    if (hres!=DD_OK)
		I_Error("GetPixelFormat Failed");
	dest=Pal32;
	src=palette;
	rbits=GetShiftBits(ddpf.dwRBitMask);
	gbits=GetShiftBits(ddpf.dwGBitMask);
	bbits=GetShiftBits(ddpf.dwBBitMask);
	while (src<&palette[256*3])
	{
		*dest=((unsigned int)gammatable[ManualGamma][src[0]])<<rbits;
		*dest|=((unsigned int)gammatable[ManualGamma][src[1]])<<gbits;
		*dest|=((unsigned int)gammatable[ManualGamma][src[2]])<<bbits;
		dest++;
		src+=3;
	}
}

//
// I_SetPalette
//
void I_SetPalette(byte* palette)
{
    int			i;
    PALETTEENTRY	pal[256];
    HRESULT		hres;

	if (InWindow)
	{
		I_SetPaletteWindowed(palette);
		return;
	}
    for (i=0;i<256;i++)
    {
		pal[i].peRed=gammatable[ManualGamma][*(palette++)];
		pal[i].peGreen=gammatable[ManualGamma][*(palette++)];
		pal[i].peBlue=gammatable[ManualGamma][*(palette++)];
		pal[i].peFlags=0;
    }
    hres=pDDPalette->lpVtbl->SetEntries(pDDPalette, 0, 0, 256, pal);
    if (hres!=DD_OK)
		I_Error("DDPalette::SetEntries Failed");
}

void I_InitPaletteWindowed(void)
{
	Pal32=(unsigned int *)malloc(1024);
	if (!Pal32)
		I_Error("Out of memory");
}

void I_SelectWindowedMode(void)
{
	HRESULT			hres;
	DDPIXELFORMAT	ddpf;

	ddpf.dwSize=sizeof(DDPIXELFORMAT);
	hres=pDDSPrimary->lpVtbl->GetPixelFormat(pDDSPrimary, &ddpf);
	if (hres!=DD_OK)
		I_Error("GetPixelFormat(Primary) Failed");
	if ((ddpf.dwFlags!=DDPF_RGB)||(ddpf.dwRGBBitCount!=32))
		I_Error("Windowed mode requires 32-bit color");
	I_InitPaletteWindowed();
}

void I_InitGraphics(void)
{
    HRESULT				hres;
    LPDIRECTDRAW		dd;
    DDSURFACEDESC2		ddsd;
    DDSCAPS2			ddscaps;
    PALETTEENTRY		pal[256];
	LPDIRECTDRAWCLIPPER	pclipper;

    if (pDD)
		return;

    hres=DirectDrawCreate(NULL, &dd, NULL);
    if (hres!=DD_OK)
		I_Error("DirectDrawCreate Failed");
    hres=dd->lpVtbl->QueryInterface(dd, &IID_IDirectDraw4, (void **)&pDD);
    if (hres!=DD_OK)
		I_Error("QueryInterface(IDirectDraw4) Failed\nCheck DirectX6 or above is installed");
    dd->lpVtbl->Release(dd);
	if (InWindow)
	{
		hres=pDD->lpVtbl->SetCooperativeLevel(pDD, *D_API.pMainWnd, DDSCL_NORMAL);
		if (hres!=DD_OK)
			I_Error("DirectDraw::SetCooperativeLevel Failed");
	}
	else
	{
		hres=pDD->lpVtbl->SetCooperativeLevel(pDD, *D_API.pMainWnd, DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN);
		if (hres!=DD_OK)
			I_Error("DirectDraw::SetCooperativeLevel Failed");
		hres=pDD->lpVtbl->SetDisplayMode(pDD, ScreenWidth, ScreenHeight, 8, 0, 0);
		if (hres!=DD_OK)
			I_Error("SetDisplayMode(%d, %d) Failed", ScreenWidth, ScreenHeight);
		UpdateWindow(*D_API.pMainWnd);
	}
    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
	if (InWindow)
	{
		ddsd.dwFlags=DDSD_CAPS;
		ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE;
	}
	else
	{
		ddsd.dwFlags=DDSD_CAPS|DDSD_BACKBUFFERCOUNT;
		ddsd.dwBackBufferCount=1;
		ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE|DDSCAPS_COMPLEX|DDSCAPS_FLIP;
	}
    hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSPrimary, NULL);
    if (hres!=DD_OK)
		I_Error("CreateSurface(Primary) Failed");
    ScreenNum=0;
    BufferScreens[0]=screens[0];
    BufferScreens[1]=(pixel_t *)malloc(PIX2BYTE(ScreenWidth*ScreenHeight));
	if (InWindow)
	{
		I_SelectWindowedMode();
		hres=pDD->lpVtbl->CreateClipper(pDD, 0, &pclipper, NULL);
		if (hres!=DD_OK)
			I_Error("CreateClipper Failed");
		pclipper->lpVtbl->SetHWnd(pclipper, 0, *D_API.pMainWnd);
		pDDSPrimary->lpVtbl->SetClipper(pDDSPrimary, pclipper);
		pclipper->lpVtbl->Release(pclipper);
		ddsd.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
		ddsd.dwWidth=ScreenWidth;
		ddsd.dwHeight=ScreenHeight;
		hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSBack, NULL);
		if (hres!=DD_OK)
			I_Error("CreateSurface(Intermediate) failed");
	}
	else
	{
		HDC		hdc;
		ZeroMemory(&ddscaps, sizeof(DDSCAPS2));
		ddscaps.dwCaps=DDSCAPS_BACKBUFFER;
		hres=pDDSPrimary->lpVtbl->GetAttachedSurface(pDDSPrimary, &ddscaps, &pDDSBack);
		if (hres!=DD_OK)
			I_Error("GetAttachedSurface Failed");
		ZeroMemory(pal, 256*sizeof(PALETTEENTRY));

		hdc=GetDC(*D_API.pMainWnd);
		if (!hdc)
			I_Error("CreateDC Failed");
		if (GetSystemPaletteEntries(hdc, 0, 256, pal)!=256)
			I_Error("GetPaletteEntries Failed");
		DeleteDC(hdc);
		hres=pDD->lpVtbl->CreatePalette(pDD, DDPCAPS_8BIT|DDPCAPS_ALLOW256, pal, &pDDPalette, NULL);
		if (hres!=DD_OK)
			I_Error("CreatePalette Failed");
		if (!pDDPalette)
			I_Error("No palette");
		hres=pDDSPrimary->lpVtbl->SetPalette(pDDSPrimary, pDDPalette);
		if (hres!=DD_OK)
			I_Error("SetPalette Failed");
		I_InitScreenSurface();
	}
    I_SetGamma();
    I_InitOverlay();
	if (InWindow)
		pDD->lpVtbl->FlipToGDISurface(pDD);
}

//FUDGE: add screenshot support
void I_ScreenShot(void)
{
}

void I_SetGamma(void)
{
    ManualGamma=*D_API.pGammaLevel;
}

void I_BeginRead(void)
{
}
