//d3di_video.c
#include <io.h>
#include "doomdef.h"
#include "d3dr_local.h"
#include "t_split.h"

//#define SOFTWARE_DEVICE

#ifdef SOFTWARE_DEVICE
#define D3DDEVICEIID IID_IDirect3DRGBDevice
#else
#define D3DDEVICEIID IID_IDirect3DHALDevice
#endif

#include "d3di_video.h"
#include "c_d3d.h"
#include "d3di_ddraw.h"
#include "i_gamma.h"
#include "i_overlay.h"
#include "tables.h"
#include "d3dv_video.h"
#include "d3dr_local.h"
#include "m_swap.h"

typedef struct
{
    word	masks[3];
    int		shifts[3];
}pixelformat_t;

LPDIRECTDRAW4			pDD=NULL;
LPDIRECTDRAWSURFACE4	pDDSPrimary=NULL;
LPDIRECTDRAWSURFACE4	pDDSBack=NULL;
LPDIRECTDRAWSURFACE4	pDDSScreens[5];

LPDIRECT3D3				pD3D=NULL;
LPDIRECT3DDEVICE3		pD3DDevice=NULL;
LPDIRECTDRAWSURFACE4	pDDSZBuffer=NULL;
LPDIRECT3DVIEWPORT3		pD3DViewports[MAX_SPLITS]={NULL};
LPDIRECT3DVIEWPORT3		pD3DVPCurrent;
LPDIRECTDRAWPALETTE		pDDPTexture=NULL;
DDPIXELFORMAT			TexturePixelFormat;
DDPIXELFORMAT			AlphaTexturePixelFormat;
DDPIXELFORMAT			ScreenPixelFormat;

PALETTEENTRY			CurPal[256];
word					CurPal16[256];
word					TexturePal[MAXPLAYERS][256];
word					AlphaTexturePal[MAXPLAYERS][256];
word					TextureAlphaMask;
word					TextureClearColor;
int						TextureAlphaBits=16;
dboolean				UseColorKey;
dboolean				BilinearFiltering;

int						ViewWindowX;
int						ViewWindowY;
int						ViewWidth;
int						ViewHeight;
static byte				*gtable;
dboolean				BusyDisk=false;

BOOL WINAPI DeviceEnumProc(GUID FAR *pguid, char *desc, char *name, void *context, HMONITOR hmonitor)
{
	I_Printf("I_InitDD: Found device %s(%s)\n", name, desc);
	return(TRUE);
}

void I_InitDD(void)
{
    LPDIRECTDRAW		pdd1;
    DDSURFACEDESC2		ddsd;
    HRESULT				hres;
    DDSCAPS2			ddscaps;
    int					i;
	LPDIRECTDRAWCLIPPER	pclipper;

	hres=DirectDrawEnumerateEx(DeviceEnumProc, NULL, DDENUM_NONDISPLAYDEVICES);
	if (hres!=DD_OK)
		I_Error("DirectDrawEnumerateEx Failed");

    for (i=0;i<5;i++)
		pDDSScreens[i]=NULL;
    hres=DirectDrawCreate(*D_API.ppD3DDeviceGUID, &pdd1, NULL);
    if (hres!=DD_OK)
		I_Error("DirectDrawCreate Failed");
    hres=pdd1->lpVtbl->QueryInterface(pdd1, &IID_IDirectDraw4, (void **)&pDD);
    pdd1->lpVtbl->Release(pdd1);
    if (hres!=DD_OK)
		I_Error("QueryInterface(IDirectDraw4) Failed\nCheck DirectX6 or above is installed");
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
		ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE|DDSCAPS_3DDEVICE|DDSCAPS_COMPLEX|DDSCAPS_FLIP;
	    ddsd.dwBackBufferCount=1;
	}
    hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSPrimary, NULL);
    if (hres!=DD_OK)
		I_Error("CreateSurface(Primary) Failed");
	if (InWindow)
	{
	    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
		ddsd.dwSize=sizeof(DDSURFACEDESC2);
	    ddsd.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
		ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_3DDEVICE;
		ddsd.dwWidth=ScreenWidth;
		ddsd.dwHeight=ScreenHeight;
		hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSBack, NULL);
		if (hres!=DD_OK)
			I_Error("CreateSurface(Buffer Screen) Failed");
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
			I_Error("GetAttachedSurface(BackBuffer) Failed");
	}
    pDDSScreens[0]=pDDSBack;
    ZeroMemory(&ScreenPixelFormat, sizeof(DDPIXELFORMAT));
    ScreenPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
    hres=pDDSBack->lpVtbl->GetPixelFormat(pDDSBack, &ScreenPixelFormat);
    if (hres!=DD_OK)
		I_Error("GetPixelFormat Failed");
	if (ScreenPixelFormat.dwRGBBitCount!=16)
		I_Error("Only works with 16-bit desktop");
    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
    ddsd.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
    ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
    ddsd.dwWidth=ScreenWidth;
    ddsd.dwHeight=ScreenHeight;
    for (i=1;i<4;i++)
    {
		hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSScreens[i], NULL);
		if (hres!=DD_OK)
			I_Error("CreateSurface(Screen%d) Failed", i);
    }
    ddsd.dwHeight=ST_HEIGHT;
    hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSScreens[4], NULL);
    if (hres!=DD_OK)
		I_Error("CreateSurface(Screen4) Failed");
}

static int CountBits(DWORD i)
{
	DWORD	mask;
	int		count;

	count=0;
	for (mask=1;mask;mask<<=1)
	{
		if (mask&i)
			count++;
	}
	return(count);
}

static HRESULT CALLBACK TextureFormatEnum(LPDDPIXELFORMAT ddpf, dboolean *ptexture8bit)
{
	int		bits;
//FUDGE: 8bt
#if 0
    if (ddpf->dwFlags==(DDPF_RGB|DDPF_PALETTEINDEXED8))
    {
		*ptexture8bit=true;
		memcpy(&TexturePixelFormat, ddpf, sizeof(DDPIXELFORMAT));
		return(D3DENUMRET_CANCEL);
    }
#endif
    if ((ddpf->dwFlags==(DDPF_RGB|DDPF_ALPHAPIXELS))&&(ddpf->dwRGBBitCount==16))
	{
		bits=CountBits(ddpf->dwRGBAlphaBitMask);
		if (bits<TextureAlphaBits)
		{
			memcpy(&AlphaTexturePixelFormat, ddpf, sizeof(DDPIXELFORMAT));
			TextureAlphaBits=bits;
		}
	}
	else if ((ddpf->dwFlags==DDPF_RGB)&&(ddpf->dwRGBBitCount==16))
	{
		if (!TexturePixelFormat.dwFlags)
			memcpy(&TexturePixelFormat, ddpf, sizeof(DDPIXELFORMAT));
	}
    return(D3DENUMRET_OK);
}

void I_SetViewMatrix(float x, float y, float z, angle_t angle)
{
    D3DMATRIX	m;
    float		s;
    float		c;
	float		dx;
	float		dy;

    ZeroMemory(&m, sizeof(D3DMATRIX));

    s=F2D3D(finesine[(viewangle-angle)>>ANGLETOFINESHIFT]);
    c=F2D3D(finecosine[(viewangle-angle)>>ANGLETOFINESHIFT]);
    m._11=s;
    m._12=c;
    m._22=s;
    m._21=-c;
    //m._11=m._22=1;
    m._33=m._44=1;
	dx=x-fviewx;
	dy=y-fviewy;
	m._41=(dx+viewoffset)*viewsin-dy*viewcos;
	m._42=(dx-viewoffset)*viewcos+dy*viewsin;
    //m._41=dx;
    //m._42=dy;
    m._43=z-fviewz;
    pD3DDevice->lpVtbl->SetTransform(pD3DDevice, D3DTRANSFORMSTATE_WORLD, &m);
}

void I_InitTransforms(void)
{
    D3DMATRIX	m;
    ZeroMemory(&m, sizeof(D3DMATRIX));
    m._11 =  1.0f;
    m._23 =  1.0f;
    m._32 =  1.0f;
    m._44 =  1.0f;
    pD3DDevice->lpVtbl->SetTransform(pD3DDevice, D3DTRANSFORMSTATE_VIEW, &m);

    ZeroMemory(&m, sizeof(D3DMATRIX));
    m._11 =  1.0f;
    //m._22 =  1.0f;
    m._22 =  ScreenWidth/(float)ScreenHeight;
    m._34 =  1.0f;
    m._33 =  1.00f;
    m._43 = -1.00f;
    pD3DDevice->lpVtbl->SetTransform(pD3DDevice, D3DTRANSFORMSTATE_PROJECTION, &m);
}

static HRESULT CALLBACK EnumZBuffers(LPDDPIXELFORMAT pf, LPDDPIXELFORMAT dest)
{
    if (pf->dwFlags!=DDPF_ZBUFFER)
		return(D3DENUMRET_OK);
    memcpy(dest, pf, sizeof(DDPIXELFORMAT));
    if (pf->dwZBufferBitDepth==16)
		return(D3DENUMRET_CANCEL);
    else
		return(D3DENUMRET_OK);
}

void I_CreateZBuffer(void)
{
    HRESULT		hres;
    DDSURFACEDESC2	ddsd;

    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
    hres=pD3D->lpVtbl->EnumZBufferFormats(pD3D, &D3DDEVICEIID, (LPD3DENUMPIXELFORMATSCALLBACK)EnumZBuffers, &ddsd.ddpfPixelFormat);
    if (!ddsd.ddpfPixelFormat.dwFlags)
		I_Error("Z-Buffering not supported");
    if (hres!=D3D_OK)
		I_Error("EnumZBufferFormats Failed");
    ddsd.dwFlags=DDSD_CAPS|DDSD_PIXELFORMAT|DDSD_WIDTH|DDSD_HEIGHT;
    ddsd.dwHeight=ScreenHeight;
    ddsd.dwWidth=ScreenWidth;
#ifdef SOFTWARE_DEVICE
    ddsd.ddsCaps.dwCaps=DDSCAPS_ZBUFFER|DDSCAPS_SYSTEMMEMORY;
#else
    ddsd.ddsCaps.dwCaps=DDSCAPS_ZBUFFER|DDSCAPS_VIDEOMEMORY;
#endif
    hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSZBuffer, NULL);
    if (hres!=DD_OK)
		I_Error("CreateSurface(ZBuffer) Failed");
    hres=pDDSBack->lpVtbl->AddAttachedSurface(pDDSBack, pDDSZBuffer);
    if (hres!=DD_OK)
		I_Error("AddAttachedSurface(ZBuffer) Failed");
}

void I_SelectViewport(int split)
{
	HRESULT		hres;

	D_API.S_GetViewPos(split, &ViewWindowX, &ViewWindowY);
	pD3DVPCurrent=pD3DViewports[split];
	hres=pD3DDevice->lpVtbl->SetCurrentViewport(pD3DDevice, pD3DVPCurrent);
	if (hres!=D3D_OK)
		I_Error("SetCurrentViewport(%d) failed", split);
}

void I_SetRenderViewports(void)
{
    D3DVIEWPORT2	d3dvp;
	HRESULT			hres;
	int				split;
	int				x;
	int				y;

    ZeroMemory(&d3dvp, sizeof(D3DVIEWPORT2));
    d3dvp.dwSize=sizeof(D3DVIEWPORT2);
    d3dvp.dvClipX=-1;
    d3dvp.dvClipWidth=2;
    d3dvp.dvMinZ=0;
    d3dvp.dvMaxZ=1;

//FUDGE: add clipping for console
	for (split=0;split<NumSplits;split++)
	{
		D_API.S_GetViewPos(split, &x, &y);
	    d3dvp.dvClipY=1;
	    d3dvp.dvClipHeight=2;
		d3dvp.dwX=x;
		d3dvp.dwY=y;
	    d3dvp.dwWidth=ViewWidth;
	    d3dvp.dwHeight=ViewHeight;
	    hres=pD3DViewports[split]->lpVtbl->SetViewport2(pD3DViewports[split], &d3dvp);
	    if (hres!=D3D_OK)
			I_Error("SetViewport2(%d) Failed", split);
	}
}

static void SetAlphaTest(D3DCMPFUNC cmpfunc, DWORD ref)
{
	HRESULT		hres;

	hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
	if (hres!=DD_OK)
		I_Error("Alpha testing not available");
	hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_ALPHAFUNC, cmpfunc);
	if (hres!=D3D_OK)
		I_Error("Unable to setup alpha testing");
	hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_ALPHAREF, ref);
	if (hres!=D3D_OK)
		I_Error("Unable to setup alpha testing");
}

void I_SetFilterMode(void)
{
	DWORD		mode;

	if (BilinearFiltering)
		mode=D3DTFG_LINEAR;
	else
		mode=D3DTFG_POINT;
    pD3DDevice->lpVtbl->SetTextureStageState(pD3DDevice, 0, D3DTSS_MAGFILTER, mode);
	if (BilinearFiltering)
		mode=D3DTFN_LINEAR;
	else
		mode=D3DTFN_POINT;
    pD3DDevice->lpVtbl->SetTextureStageState(pD3DDevice, 0, D3DTSS_MINFILTER, mode);
}

static word RGB2HiColor(byte *p, pixelformat_t *pixfmt)
{
    word	res;
    int		i;

    res=0;
    for (i=0;i<3;i++)
    {
		if (pixfmt->shifts[i]<0)
			res|=(gtable[p[i]]>>-pixfmt->shifts[i])&pixfmt->masks[i];
		else
			res|=(gtable[p[i]]<<pixfmt->shifts[i])&pixfmt->masks[i];
    }
    return(res);
}

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

void I_ConvertPixelFormat(pixelformat_t *pixfmt, LPDDPIXELFORMAT ddpf)
{
    int		i;

    pixfmt->masks[0]=(word)ddpf->dwRBitMask;
    pixfmt->masks[1]=(word)ddpf->dwGBitMask;
    pixfmt->masks[2]=(word)ddpf->dwBBitMask;
    for (i=0;i<3;i++)
		pixfmt->shifts[i]=GetShiftBits(pixfmt->masks[i])-8;
}

void I_TranslatePalette2(word *dest, byte *palette, LPDDPIXELFORMAT ddpf, byte *trans)
{
    pixelformat_t	pixelformat;
	int				i;

    I_ConvertPixelFormat(&pixelformat, ddpf);
    for (i=0;i<256;i++)
	{
		dest[i]=(word)(RGB2HiColor(&palette[trans[i]*3], &pixelformat)|ddpf->dwRGBAlphaBitMask);
	}
}

void I_TranslatePalette(word *dest, byte *palette, LPDDPIXELFORMAT ddpf)
{
    pixelformat_t	pixelformat;
	word			*p;

    I_ConvertPixelFormat(&pixelformat, ddpf);
	p=dest;
    while (p<&dest[256])
	{
		*(p++)=(word)(RGB2HiColor(palette, &pixelformat)|ddpf->dwRGBAlphaBitMask);
		palette+=3;
	}
}

void I_SetTexturePalette(void)
{
	byte		*pal;
	int			i;
	int			j;
	byte		trans[7][256];

	pal=W_CacheLumpName ("PLAYPAL",PU_CACHE);
	gtable=gammatable[ManualGamma];
	I_TranslatePalette(TexturePal[0], pal, &TexturePixelFormat);
	I_TranslatePalette(AlphaTexturePal[0], pal, &AlphaTexturePixelFormat);
    for (i=0 ; i<256 ; i++)
    {
		if (i >= 0x70 && i<= 0x7f)
		{
			j=i&0xf;
			// map green ramp to gray, brown, red
			trans[0][i] = 0x60 + j;
			trans[1][i] = 0x40 + j;
			trans[2][i] = 0x20 + j;
			trans[3][i]=0xb0 + j;//light red
			trans[4][i]=0xc8 + (j>>1);//blue
			if (j>4)//yellow
				trans[5][i]=0x40+j;
			else
				trans[5][i]=0xa0+j;
			trans[6][i]=0x50+(j<<1);//white
		}
		else
		{
			// Keep all other colors as is.
			trans[0][i] = trans[1][i] = trans[2][i] = i;
		}
    }
	for (i=1;i<MAXPLAYERS;i++)
	{
		I_TranslatePalette2(TexturePal[i], pal, &TexturePixelFormat, trans[i-1]);
		I_TranslatePalette2(AlphaTexturePal[i], pal, &AlphaTexturePixelFormat, trans[i-1]);
	}
}

void I_InitD3D(void)
{
    HRESULT			hres;
    dboolean		texture8bit;
//    PALETTEENTRY	pal[256];
	D3DDEVICEDESC	ddesc;
	D3DDEVICEDESC	heldesc;
	int				split;

    hres=pDD->lpVtbl->QueryInterface(pDD, &IID_IDirect3D3, (void **)&pD3D);
    if (hres!=DD_OK)
		I_Error("QueryInterface(IDirect3D3) Failed");
    I_CreateZBuffer();
    hres=pD3D->lpVtbl->CreateDevice(pD3D, &D3DDEVICEIID, pDDSBack, &pD3DDevice, NULL);
    if (hres!=D3D_OK)
		I_Error("Direct3D::CreateDevice Failed");

	for (split=0;split<NumSplits;split++)
	{
	    hres=pD3D->lpVtbl->CreateViewport(pD3D, &pD3DViewports[split], NULL);
	    if (hres!=D3D_OK)
			I_Error("CreateViewport(%d) Failed", split);
	    hres=pD3DDevice->lpVtbl->AddViewport(pD3DDevice, pD3DViewports[split]);
	    if (hres!=DD_OK)
			I_Error("AddViewport(%d) Failed", split);
	}
    hres=pD3DDevice->lpVtbl->SetCurrentViewport(pD3DDevice, pD3DViewports[0]);
    if (hres!=D3D_OK)
		I_Error("SetCurentViewport Failed");
    ZeroMemory(&TexturePixelFormat, sizeof(DDPIXELFORMAT));
    ZeroMemory(&AlphaTexturePixelFormat, sizeof(DDPIXELFORMAT));
    texture8bit=false;
    hres=pD3DDevice->lpVtbl->EnumTextureFormats(pD3DDevice, (LPD3DENUMPIXELFORMATSCALLBACK)TextureFormatEnum, &texture8bit);
    if (hres!=DD_OK)
		I_Error("EnumTextureFormats Failed");
    if (TexturePixelFormat.dwFlags==0)
		I_Error("Unable to find suitable texture format");
	if (!UseColorKey)
	{
		if (AlphaTexturePixelFormat.dwFlags==0)
			I_Error("Unable to find suitable alpha texture format");
	}
//FUDGE:8bt
#if 0
    if (texture8bit)
    {
		hres=pDD->lpVtbl->CreatePalette(pDD, DDPCAPS_ALLOW256|DDPCAPS_8BIT, pal, &pDDPTexture, NULL);
		if (hres!=DD_OK)
			I_Error("CreatePalette Failed");
    }
    else
#endif
		pDDPTexture=NULL;
    I_InitTransforms();
    hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);
    if (hres!=D3D_OK)
		I_Error("ZBufferEnable Failed");
	I_SetFilterMode();
	if (UseColorKey)
	{
		hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_COLORKEYENABLE, TRUE);
		if (hres!=D3D_OK)
			I_Error("Color keying not suported");
		TextureClearColor=(word)(TexturePixelFormat.dwRBitMask|TexturePixelFormat.dwBBitMask);
	}
	else
	{
		hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		if (hres!=D3D_OK)
			I_Error("Unable to enable alpha blending");
		hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
		if (hres!=D3D_OK)
			I_Error("Unable to set source blend factor");
		hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
		if (hres!=D3D_OK)
			I_Error("Unable to set dest blend factor");
		TextureAlphaMask=(word)AlphaTexturePixelFormat.dwRGBAlphaBitMask;
	    TextureClearColor=0;
		ddesc.dwSize=sizeof(D3DDEVICEDESC);
		heldesc.dwSize=sizeof(D3DDEVICEDESC);
		hres=pD3DDevice->lpVtbl->GetCaps(pD3DDevice, &ddesc, &heldesc);
		if (hres!=D3D_OK)
			I_Error("D3DDevice::GetCaps Failed");

		if (ddesc.dpcTriCaps.dwAlphaCmpCaps&D3DPCMPCAPS_GREATER)
			SetAlphaTest(D3DCMP_GREATER, 0);
		else if (ddesc.dpcTriCaps.dwAlphaCmpCaps&D3DPCMPCAPS_GREATEREQUAL)
			SetAlphaTest(D3DCMP_GREATEREQUAL, 1);
		else if (ddesc.dpcTriCaps.dwAlphaCmpCaps&D3DPCMPCAPS_NOTEQUAL)
			SetAlphaTest(D3DCMP_NOTEQUAL, 0);
	}

	I_SetTexturePalette();

	R_CheckTextureSizeLimits();
}

void I_InitGraphics(void)
{
    I_InitDD();
    I_InitD3D();
	I_SetGamma();
    I_InitOverlay();
}

void I_ShutdownGraphics(void)
{
    int		i;

    I_ShutdownOverlay();
    SAFE_RELEASE(pDDSZBuffer);
//FUDGE:8bt
//    SAFE_RELEASE(pDDPTexture);
	if (pD3DViewports[0])
	{
		for (i=0;i<NumSplits;i++)
	    SAFE_RELEASE(pD3DViewports[i]);
	}
    SAFE_RELEASE(pD3DDevice);
    SAFE_RELEASE(pD3D);
    SAFE_RELEASE(pDDSPrimary);
    if (pDD)
    {
		for (i=1;i<5;i++)
			SAFE_RELEASE(pDDSScreens[i]);
		SAFE_RELEASE(pDD);
    }
}

void I_SetPalette(byte *palette)
{
//    PALETTEENTRY	*dest;
//    HRESULT			hres;
	int				i;

    gtable=gammatable[ManualGamma];
	I_TranslatePalette(CurPal16, palette, &ScreenPixelFormat);
#if 0
	dest=CurPal;
//FUDGE:8bt
    if (pDDPTexture)
	{
	    while (dest<&CurPal[256])
	    {
			dest->peRed=gtable[palette[0]];
			dest->peGreen=gtable[palette[1]];
			dest->peBlue=gtable[palette[2]];
			dest->peFlags=0;
			dest++;
			palette+=3;
		}
		hres=pDDPTexture->lpVtbl->SetEntries(pDDPTexture, 0, 0, 256, CurPal);
		if (hres!=DD_OK)
			I_Error("Palette::SetEntries Failed");
    }
#endif
	for (i=0;i<NUM_AM_COLORS;i++)
		RGB16AutoMapColors[i]=CurPal16[AutoMapColors[i]];
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
    }
}

void I_StartDrawing(void)
{
	if (!InWindow)
		I_WaitForFlip();
}

void I_FinishUpdate(void)
{
//	HRESULT		hres;


//FUDGERETURN
	if (InWindow)
	{
		/*hres=*/
		pDDSPrimary->lpVtbl->Blt(pDDSPrimary, D_API.pWindowPos, pDDSBack, NULL, DDBLT_WAIT, NULL);
	}
	else
	{
		/*hres=*/
		pDDSPrimary->lpVtbl->Flip(pDDSPrimary, NULL, DDFLIP_WAIT);
	}
	BusyDisk=false;
}

void I_ScreenShot(void)
{
	DDSURFACEDESC2		ddsd;
	HRESULT				hres;
	char				name[13];
	int					i;
	static int			shotnum=0;
	FILE				*fh;
	word				*src;
	byte				*dest;
	byte				*pixbuff;
	int					pitch;
	int					x;
	int					y;
	int					w;
	int					size;
	pixelformat_t		pf;
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

    I_ConvertPixelFormat(&pf, &ScreenPixelFormat);

	fh=fopen(name, "wb");
	if (!fh)
		return;

	w=(ScreenWidth*3+3)&~3;
	size=w*ScreenHeight;
	w-=ScreenWidth*3;
	pixbuff=(byte *)malloc(size);
	if (!pixbuff)
	{
		I_Error("out of memory");
	}

	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize=sizeof(DDSURFACEDESC2);
	hres=pDDSBack->lpVtbl->Lock(pDDSBack, NULL, &ddsd, DDLOCK_WAIT|DDLOCK_READONLY, NULL);
	if (hres!=DD_OK)
	{
		free(pixbuff);
		fclose(fh);
		return;
	}
	pitch=ddsd.lPitch+ScreenWidth*2;
	src=(word *)(((byte *)ddsd.lpSurface)+ddsd.lPitch*(ScreenHeight-1));
	dest=pixbuff;
	for (y=0;y<ScreenHeight;y++)
	{
		for (x=0;x<ScreenWidth;x++)
		{
			for (i=2;i>=0;i--)//.bmp format is BGR not RGB
			{
				if (pf.shifts[i]<0)
					*(dest++)=((*src)&pf.masks[i])<<-pf.shifts[i];
				else
					*(dest++)=((*src)&pf.masks[i])>>pf.shifts[i];
			}
			src++;
		}
		((byte *)src)-=pitch;
		dest+=w;
	}
	pDDSBack->lpVtbl->Unlock(pDDSBack, NULL);
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
	R_DumpTextures();
	I_SetTexturePalette();
	R_RecalcLightmaps();
}

LPDIRECTDRAWSURFACE4	pDDSDisk=NULL;
static int				DiskWidth;
static int				DiskHeight;

static void LoadBusyDisk(void)
{
	patch_t			*patch;
	DDSURFACEDESC2	ddsd;
	HRESULT			hres;
	word			*dest;
	int				x;
	column_t		*column;
	int				count;
	byte			*src;


	patch=W_CacheLumpName("STDISK", PU_CACHE);
	DiskWidth=patch->width;
	DiskHeight=patch->height;
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize=sizeof(DDSURFACEDESC2);
	ddsd.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT;
	ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth=patch->width;
	ddsd.dwHeight=patch->height;
	hres=pDD->lpVtbl->CreateSurface(pDD, &ddsd, &pDDSDisk, NULL);
	if (hres!=DD_OK)
		return;

	hres=pDDSDisk->lpVtbl->Lock(pDDSDisk, NULL, &ddsd, DDLOCK_WAIT, NULL);
	if (hres!=DD_OK)
		I_Error("Lock(disk) Failed");

	ZeroMemory(ddsd.lpSurface, DiskWidth*DiskHeight*2);
    for (x=0;x<patch->width;x++)
    {
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[x]));
		while (column->topdelta != 0xff )
		{
			src = (byte *)column + 3;
			dest=((word *)ddsd.lpSurface)+x+column->topdelta*ddsd.lPitch;
			count = column->length;

			while (count--)
			{
				*dest = CurPal16[*(src++)];
				((byte *)dest) += ddsd.lPitch;
			}
			column = (column_t *)((byte *)column + column->length + 4);
		}
	}

	pDDSDisk->lpVtbl->Unlock(pDDSDisk, NULL);
}

void I_BeginRead(void)
{
	RECT	r;

	if (BusyDisk)
		return;
	if (!pDDSDisk)
		LoadBusyDisk();
	if (!pDDSDisk)
		return;

	if (InWindow)
	{
		r.right=D_API.pWindowPos->right;
		r.bottom=D_API.pWindowPos->bottom;
	}
	else
	{
		r.right=ScreenWidth;
		r.bottom=ScreenHeight;
	}
	r.left=r.right-DiskWidth;
	r.top=r.bottom-DiskHeight;
	pDDSPrimary->lpVtbl->Blt(pDDSPrimary, &r, pDDSDisk, NULL, DDBLT_WAIT, NULL);
	BusyDisk=true;
}
