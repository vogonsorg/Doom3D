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

#include <io.h>
#include <stdarg.h>
//#include "doomstat.h"
//#include "i_system.h"
//#include "i_input.h"
#include "swi_video.h"
//#include "i_windoz.h"
#include "swv_video.h"
//#include "m_argv.h"
//#include "d_main.h"

#include "i_gamma.h"
#include "swr_main.h"
#include "i_overlay.h"
#include "swi_ddraw.h"

#include "doomdef.h"

#include "c_sw.h"

#include "sw16_defs.h"

LPDIRECTDRAW4			pDD=NULL;
LPDIRECTDRAWSURFACE4	pDDSPrimary=NULL;
LPDIRECTDRAWSURFACE4	pDDSBack=NULL;
pixel_t					*BufferScreens[2];
int						ScreenNum;
LPDIRECTDRAWSURFACE4	pDDSScreens[2]={NULL, NULL};
DDPIXELFORMAT			ScreenPixelFormat;

dboolean				InWindow;
dboolean				SurfaceLost=false;

pixel_t					CurrentPalette[256];
pixel_t					RGBFuzzMask;

void I_WaitVBL(int count)
{
    Sleep(count);
}

void I_ShutdownGraphics(void)
{
    I_ShutdownOverlay();
    SAFE_RELEASE(pDDSScreens[0]);
    SAFE_RELEASE(pDDSScreens[1]);
    SAFE_RELEASE(pDDSPrimary);
    SAFE_RELEASE(pDD);
}



void I_BltScreen(LPDDSURFACEDESC2 ddsd)
{
    pixel_t	*src;
    pixel_t	*dest;
    int		y;

    src=screens[0];
    dest=(pixel_t *)ddsd->lpSurface;

    for (y=0;y<ScreenHeight;y++)
    {
		memcpy(dest, src, PIX2BYTE(ScreenWidth));
		dest+=ddsd->lPitch;
		src+=ScreenWidth;
    }
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
	return(mask&(mask<<2));
}

void I_InitScreenSurface(void)
{
    DDSURFACEDESC2	ddsd;
    HRESULT		hres;

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
    memcpy(&ddsd.ddpfPixelFormat, &ScreenPixelFormat, sizeof(DDPIXELFORMAT));
	if (InWindow&&((ddsd.ddpfPixelFormat.dwFlags!=DDPF_RGB)||(ddsd.ddpfPixelFormat.dwRGBBitCount!=16)))
		I_Error("Windowed mode requires 16-bit color");
    RGBFuzzMask=(pixel_t)Loose2RGBBits(ddsd.ddpfPixelFormat.dwRBitMask);
    RGBFuzzMask|=Loose2RGBBits(ddsd.ddpfPixelFormat.dwGBitMask);
    RGBFuzzMask|=Loose2RGBBits(ddsd.ddpfPixelFormat.dwBBitMask);
    ddsd.dwFlags=DDSD_WIDTH|DDSD_HEIGHT|DDSD_LPSURFACE|DDSD_PITCH|DDSD_PIXELFORMAT|DDSD_CAPS;
    ddsd.dwWidth=ScreenWidth;
    ddsd.dwHeight=ScreenHeight;
    ddsd.lPitch=PIX2BYTE(ScreenWidth);
    ddsd.lpSurface=BufferScreens[0];
    ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSScreens[0], NULL);
    if (hres!=DD_OK)
		I_Error("CreateSurface(Screen0) Failed");
    ddsd.lpSurface=BufferScreens[1];
    hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSScreens[1], NULL);
    if (hres!=DD_OK)
		I_Error("CreateSurface(Screen1) Failed");
}

void I_DoBltWindowed(void)
{
	HRESULT		hres;

	hres=pDDSPrimary->lpVtbl->Blt(pDDSPrimary, D_API.pWindowPos, pDDSScreens[ScreenNum], NULL, DDBLT_ASYNC|DDBLT_WAIT, NULL);
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
		//hres=pDDSBack->lpVtbl->Blt(pDDSBack, NULL, pDDSScreens[ScreenNum], NULL, DDBLT_ASYNC|DDBLT_WAIT, NULL);
		hres=pDDSBack->lpVtbl->BltFast(pDDSBack, 0, 0, pDDSScreens[ScreenNum], &r, DDBLTFAST_WAIT);
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
    return(n);
}

int			RGBBitShifts[3];
DWORD		RGBBitMasks[3];

word RGB2HiColor(byte *p)
{
    word	res;
    int		i;

    res=0;
    for (i=0;i<3;i++)
    {
		if (RGBBitShifts[i]<0)
			res|=(p[i]>>-RGBBitShifts[i])&RGBBitMasks[i];
		else
			res|=(p[i]<<RGBBitShifts[i])&RGBBitMasks[i];
    }
    return(res);
}

//
// I_SetPalette
//
void I_SetPalette (byte* palette)
{
    word		*dest;
    byte		temp[3];
    int			i;
    int			map;
    byte		*table;

    table=gammatable[ManualGamma];
    for (dest=CurrentPalette;dest<&CurrentPalette[256];dest++)
    {
		for (i=0;i<3;i++)
			temp[i]=table[palette[i]];
		*dest=RGB2HiColor(temp);
		for (map=0;map<NumColorMaps;map++)
		{
			for (i=0;i<3;i++)
				temp[i]=table[((int)palette[i])*(NumColorMaps+1-map)/(NumColorMaps+1)];
			colormaps[map][dest-CurrentPalette]=RGB2HiColor(temp);
		}
		temp[0]=temp[1]=temp[2]=table[255-(((int)palette[0]+(int)palette[1]+(int)palette[2])/3)];
		colormaps[NumColorMaps][dest-CurrentPalette]=RGB2HiColor(temp);
        palette+=3;
    }

	for (i=0;i<NUM_AM_COLORS;i++)
		RGB16AutoMapColors[i]=CurrentPalette[AutoMapColors[i]];
}

void I_InitGraphics(void)
{
    HRESULT				hres;
    LPDIRECTDRAW		dd;
    DDSURFACEDESC2		ddsd;
    DDSCAPS2			ddscaps;
	LPDIRECTDRAWCLIPPER	pclipper;
	int					i;

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
		hres=pDD->lpVtbl->SetDisplayMode(pDD, ScreenWidth, ScreenHeight, 16, 0, 0);
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
		I_Error("Unable to create primary surface");

    ScreenPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
    hres=pDDSPrimary->lpVtbl->GetPixelFormat(pDDSPrimary, &ScreenPixelFormat);
    if (hres!=DD_OK)
		I_Error("GetPixelFormat(Primary) Failed");
    RGBBitMasks[0]=ScreenPixelFormat.dwRBitMask;
    RGBBitMasks[1]=ScreenPixelFormat.dwGBitMask;
    RGBBitMasks[2]=ScreenPixelFormat.dwBBitMask;
    for (i=0;i<3;i++)
		RGBBitShifts[i]=GetShiftBits(RGBBitMasks[i])-8;

    ScreenNum=0;
    BufferScreens[0]=screens[0];
    BufferScreens[1]=(pixel_t *)malloc(PIX2BYTE(ScreenWidth*ScreenHeight));
	if (InWindow)
	{
		hres=pDD->lpVtbl->CreateClipper(pDD, 0, &pclipper, NULL);
		if (hres!=DD_OK)
			I_Error("CreateClipper Failed");
		pclipper->lpVtbl->SetHWnd(pclipper, 0, *D_API.pMainWnd);
		pDDSPrimary->lpVtbl->SetClipper(pDDSPrimary, pclipper);
		pclipper->lpVtbl->Release(pclipper);
	}
	else
	{
	    ZeroMemory(&ddscaps, sizeof(DDSCAPS2));
	    ddscaps.dwCaps=DDSCAPS_BACKBUFFER;
		hres=pDDSPrimary->lpVtbl->GetAttachedSurface(pDDSPrimary, &ddscaps, &pDDSBack);
	    if (hres!=DD_OK)
			I_Error("GetAttachedSurface Failed");
	}
    I_InitScreenSurface();
    I_SetGamma();
    I_InitOverlay();
	if (InWindow)
		pDD->lpVtbl->FlipToGDISurface(pDD);
}

void I_ScreenShot(void)
{
	char				name[13];
	int					i;
	static int			shotnum=0;
	FILE				*fh;
	word				*src;
	byte				*dest;
	byte				*pixbuff;
	int					x;
	int					y;
	int					w;
	int					size;
	BITMAPFILEHEADER	bh;
	BITMAPINFOHEADER	bi;

	while (shotnum<1000)
	{
		sprintf(name, "sshot%03d.bmp", shotnum);
		if (access(name, 0)!=0)
			break;
		shotnum++;
	}

	if (shotnum>=1000)
		return;

	fh=fopen(name, "wb");
	if (!fh)
		return;

	w=(ScreenWidth*3+3)&~3;
	size=w*ScreenHeight;
	w-=ScreenWidth*3;
	pixbuff=(byte *)malloc(size);
	if (!pixbuff)
		I_Error("out of memory");

	src=screens[0]+ScreenWidth*(ScreenHeight-1);
	dest=pixbuff;
	for (y=0;y<ScreenHeight;y++)
	{
		for (x=0;x<ScreenWidth;x++)
		{
			for (i=2;i>=0;i--)//.bmp format is BGR not RGB
			{
				if (RGBBitShifts[i]<0)
					*(dest++)=(byte)(((*src)&RGBBitMasks[i])<<-RGBBitShifts[i]);
				else
					*(dest++)=(byte)(((*src)&RGBBitMasks[i])>>RGBBitShifts[i]);
			}
			src++;
		}
		src-=ScreenWidth*2;
		dest+=w;
	}
	bh.bfType=0x4D42;
	bh.bfSize=size+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	bh.bfReserved1=0;
	bh.bfReserved2=0;
	bh.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	fwrite(&bh, sizeof(BITMAPFILEHEADER), 1, fh);
	bi.biSize=sizeof(BITMAPINFOHEADER);
	bi.biWidth=ScreenWidth;
	bi.biHeight=ScreenHeight;
	bi.biPlanes=1;
	bi.biBitCount=24;
	bi.biCompression=BI_RGB;
	bi.biSizeImage=size;
	bi.biXPelsPerMeter=3000;
	bi.biYPelsPerMeter=3000;
	bi.biClrUsed=0;
	bi.biClrImportant=0;
	fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, fh);
	fwrite(pixbuff, size, 1, fh);
	fclose(fh);
	free(pixbuff);
}

void I_SetGamma(void)
{
    ManualGamma=*D_API.pGammaLevel;
}

void I_BeginRead(void)
{
}
