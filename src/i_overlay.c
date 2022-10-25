//i_overlay.c
//Most of this code is SERIOUSLY broken

#include "doomdef.h"
#include "i_ddraw.h"
#include "c_dll.h"
#include "t_map.h"
#include "v_automap.h"

typedef struct
{
    DWORD	dwFlags;
    DWORD	dwFourCC;
    DWORD	Bits;
    DWORD	Red;
    DWORD	Green;
    DWORD	Blue;
}mypf_t;

LPDIRECTDRAWSURFACE4	pDDSOverlay=NULL;
LPDIRECTDRAWSURFACE4	pDDSOverBack=NULL;

static word	*PixelColors;

byte AutoMapColors[NUM_AM_COLORS]=
{
    256-5*16,
	256-32+7,
	256-47,
	6*16,
	4*16,
	7*16,
	6*16+15,
	0xc8
};

word		PC_RGB16_555[NUM_AM_COLORS]=
{
    0x7C00,
	0x7FE0,
	0x7FFF,
	0x3DEF,
	0x3200,
	0x03E0,
	0x0421,
	0x001F
};

mypf_t OverlayPixelFormats[] =
{
    {DDPF_RGB, 0, 16,  0x7C00, 0x03e0, 0x001F},      // 16-bit RGB 5:5:5
    {DDPF_RGB, 0, 16,  0xF800, 0x07e0, 0x001F},   // 16-bit RGB 5:6:5
    {DDPF_RGB|DDPF_PALETTEINDEXED8, 0, 8, 0, 0, 0},
    {DDPF_RGB, 0, 32, 0x00FF000, 0x0000FF00, 0x000000FF},
    {DDPF_RGB, 0, 24, 0x00FF000, 0x0000FF00, 0x000000FF},
    {DDPF_FOURCC,MAKEFOURCC('U','Y','V','Y'),0,0,0,0}, // UYVY
    {DDPF_FOURCC,MAKEFOURCC('Y','U','Y','2'),0,0,0,0} // YUY2
};

#define NUM_OVERLAY_FORMATS (sizeof(OverlayPixelFormats)/sizeof(OverlayPixelFormats[0]))

static word	*OverlaySurface;
static int	OverlayPitch;

int		AutoMapMode;

static void ClearOverlay(void)
{
    DDBLTFX	ddbltfx;
    HRESULT	hres;

    ZeroMemory(&ddbltfx, sizeof(DDBLTFX));
    ddbltfx.dwSize=sizeof(DDBLTFX);
	//FUDGERETURN
    ddbltfx.dwFillColor=0;
    hres=pDDSOverlay->lpVtbl->Blt(pDDSOverlay, NULL, NULL, NULL, DDBLT_WAIT|DDBLT_ASYNC|DDBLT_COLORFILL, &ddbltfx);
    if (hres!=DD_OK)
		I_Error("Blt(Overlay Fill) Failed");
}

#if 0
static void FlipOverlay(void)
{
    HRESULT	hres;

	//FUDGERETURN
    hres=pDDSOverlay->lpVtbl->Flip(pDDSOverlay, NULL, DDFLIP_WAIT);
    if (hres!=DD_OK)
		I_Error("Flip(overlay) Failed");
}
#endif

void I_InitOverlay(void)
{
#if 0
    DDCAPS		ddcaps;
    DDSURFACEDESC2	ddsd;
    DDSCAPS2		ddscaps;
    int			i;
    HRESULT		hres;
    DDCOLORKEY		ddck;
    RECT		r;

    ZeroMemory(&ddcaps, sizeof(DDCAPS));
    ddcaps.dwSize=sizeof(DDCAPS);
    hres=pDD->lpVtbl->GetCaps(pDD, &ddcaps, NULL);
    if (hres!=DD_OK)
		I_Error("DirectDraw::GetCaps Failed");
    if (ddcaps.dwCaps&DDCAPS_OVERLAY)
		I_Printf("Overlays supported\n");
    else
		I_Printf("Overlays not supported\n");

    I_Printf("Max      %d\n", ddcaps.dwMaxVisibleOverlays);
    I_Printf("Current  %d\n", ddcaps.dwCurrVisibleOverlays);
    I_Printf("Source Alignment:\n");
    I_Printf("Boundary %d\n", (ddcaps.dwCaps&DDCAPS_ALIGNBOUNDARYSRC)?ddcaps.dwAlignBoundarySrc:-1);
    I_Printf("Size     %d\n", (ddcaps.dwCaps&DDCAPS_ALIGNSIZESRC)?ddcaps.dwAlignSizeSrc:-1);
    I_Printf("Destination Alignment:\n");
    I_Printf("Boundary  %d\n", (ddcaps.dwCaps&DDCAPS_ALIGNBOUNDARYDEST)?ddcaps.dwAlignBoundaryDest:-1);
    I_Printf("Size      %d\n", (ddcaps.dwCaps&DDCAPS_ALIGNSIZESRC)?ddcaps.dwAlignSizeSrc:-1);
    I_Printf("Stretch:\n");
    I_Printf("Min       %d\n", (ddcaps.dwCaps&DDCAPS_OVERLAYSTRETCH)?ddcaps.dwMinOverlayStretch:-1);
    I_Printf("Max       %d\n", (ddcaps.dwCaps&DDCAPS_OVERLAYSTRETCH)?ddcaps.dwMaxOverlayStretch:-1);

    ZeroMemory(&ddsd, sizeof(ddsd));
    ddsd.dwSize=sizeof(ddsd);
    ddsd.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_BACKBUFFERCOUNT;
    ddsd.ddsCaps.dwCaps=DDSCAPS_OVERLAY|DDSCAPS_VIDEOMEMORY|DDSCAPS_COMPLEX|DDSCAPS_FLIP;
    ddsd.dwWidth=ScreenWidth;
    ddsd.dwHeight=ScreenHeight;
    ddsd.dwBackBufferCount=1;
    ddsd.ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
//FUDGE: add support for all different overlay formats
    //for (i=0;i<NUM_OVERLAY_FORMATS;i++)
    for (i=0;i<1;i++)
    {
		memcpy(&ddsd.ddpfPixelFormat.dwFlags, &OverlayPixelFormats[i].dwFlags, sizeof(mypf_t));
		hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSOverlay, NULL);
		if (hres==DD_OK)
			break;
    }
    if (hres!=DD_OK)
		pDDSOverlay=NULL;
    if (!pDDSOverlay)
		return;
    ddck.dwColorSpaceLowValue=ddck.dwColorSpaceHighValue=0;
    hres=pDDSOverlay->lpVtbl->SetColorKey(pDDSOverlay, DDCKEY_SRCOVERLAY, &ddck);
    if (hres!=DD_OK)
		I_Error("SetColorKey(overlay) Failed");
    ZeroMemory(&ddscaps, sizeof(DDSCAPS2));
    ddscaps.dwCaps=DDSCAPS_BACKBUFFER;
    hres=pDDSOverlay->lpVtbl->GetAttachedSurface(pDDSOverlay, &ddscaps, &pDDSOverBack);
    if (hres!=DD_OK)
		I_Error("GetAttachedSurface(OverBack) Failed");
    ClearOverlay();
    FlipOverlay();
    r.left=r.top=0;
    r.right=ScreenWidth;
    r.bottom=ScreenHeight;
    hres=pDDSOverlay->lpVtbl->UpdateOverlay(pDDSOverlay, NULL, pDDSPrimary, &r, DDOVER_SHOW|DDOVER_KEYSRC, NULL);
    if (hres!=DD_OK)
		I_Error("UpdateOverlay Failed");
    PixelColors=PC_RGB16_555;
#endif
}

void I_ShutdownOverlay(void)
{
    SAFE_RELEASE(pDDSOverlay);
}

void AM_OLPutDot(int x, int y, byte color)
{
    if (color>=NUM_AM_COLORS)
		I_Error("Invalid automap color %d", color);
    OverlaySurface[y*OverlayPitch+x]=PixelColors[color];
}

void AM_Clear(void)
{
	if (AutoMapMode==AMM_SOLID)
	{
		V_ClearAM();
		return;
	}
	if (AutoMapMode!=AMM_FLOATING)
		return;
    ClearOverlay();
}

void AM_StartDrawing(void)
{
    DDSURFACEDESC2	ddsd;
    HRESULT		hres;

	if (AutoMapMode==AMM_SOLID)
	{
		V_StartAM();
		return;
	}
	if (AutoMapMode!=AMM_FLOATING)
		return;
	if (!pDDSOverlay)
		I_Error("no overlay");
    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
    hres=pDDSOverlay->lpVtbl->Lock(pDDSOverlay, NULL, &ddsd, DDLOCK_WAIT|DDLOCK_WRITEONLY|DDLOCK_NOSYSLOCK, NULL);
    if (hres!=DD_OK)
		I_Error("DirectDraw::Lock(Overlay) Failed");
    OverlaySurface=(word *)ddsd.lpSurface;
    OverlayPitch=ddsd.lPitch>>1;
}

void AM_FinishDrawing(void)
{
	if (AutoMapMode==AMM_SOLID)
	{
		V_FinishAM();
		return;
	}
	if (AutoMapMode!=AMM_FLOATING)
		return;
    pDDSOverlay->lpVtbl->Unlock(pDDSOverlay, NULL);
    //FlipOverlay();
}

int AM_GetWidth(void)
{
    return(ScreenWidth);
}

int AM_GetHeight(void)
{
    return(ScreenHeight-ST_HEIGHT);
}

void AM_SetMode(int mode)
{
    switch(mode)
    {
	case AMM_FLOATING:
	case AMM_SOLID:
		AutoMapMode=mode;
		break;
	default:
		AutoMapMode=AMM_OFF;
		AM_Clear();
		break;
    }
}

gpd_t AM_GetPutDot(void)
{
	switch (AutoMapMode)
	{
	case AMM_FLOATING:
		return((gpd_t)AM_OLPutDot);
	case AMM_SOLID:
		return((gpd_t)V_PutDot);
	default:
		return(NULL);
	}
}
