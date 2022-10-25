//d3dv_video.c

#include "doomdef.h"
#include "d3dv_video.h"
#include "d3di_ddraw.h"
#include "d3di_video.h"
#include "m_swap.h"
#include "t_map.h"

#include "c_d3d.h"

word	RGB16AutoMapColors[NUM_AM_COLORS];

static byte	*AMScreen=NULL;
//static int	AMPitch;

int			ScreenWidth;
int			ScreenHeight;
dboolean	InWindow;

void
V_CopyRect
( int		srcx,
 int		srcy,
 int		srcscrn,
 int		width,
 int		height,
 int		destx,
 int		desty,
 int		destscrn )
{
    RECT	rsrc;
    HRESULT	hres;

    rsrc.left=srcx;
    rsrc.top=srcy;
    rsrc.right=rsrc.left+width;
    rsrc.bottom=rsrc.top+height;
    hres=pDDSScreens[destscrn]->lpVtbl->BltFast(pDDSScreens[destscrn], destx, desty, pDDSScreens[srcscrn], &rsrc, DDBLTFAST_WAIT);
    if (hres!=DD_OK)
		I_Error("Blt Failed (%d,%d)->(%d,%d) %dx%d", rsrc.left, rsrc.top, destx, desty, width, height);
}

int	ScreenPitch;
int	LockedScreen;

word *V_LockScreen(int scrn, DWORD flags)
{
    DDSURFACEDESC2	ddsd;
    HRESULT		hres;

    LockedScreen=scrn;
    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
    hres=pDDSScreens[scrn]->lpVtbl->Lock(pDDSScreens[scrn], NULL, &ddsd, DDLOCK_WAIT|flags, NULL);
    if (hres!=DD_OK)
		return(NULL);
    ScreenPitch=ddsd.lPitch;
    return((word *)ddsd.lpSurface);
}

void V_Unlock(void)
{
    pDDSScreens[LockedScreen]->lpVtbl->Unlock(pDDSScreens[LockedScreen], NULL);
}

void
V_DrawPatch
( int		x,
 int		y,
 int		scrn,
 patch_t*	patch)
{
    word	*screen;
    int		count;
    int		col;
    column_t*	column;
    byte*	desttop;
    word*	dest;
    byte*	source;
    int		w;

    screen=V_LockScreen(scrn, DDLOCK_WRITEONLY);
	if (!screen)
		return;
    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    col = 0;
    //desttop = screen+y*ScreenWidth+x;
	desttop=((byte *)screen)+(x<<1)+(y*ScreenPitch);

    w = SHORT(patch->width);

    for ( ; col<w ; x++, col++, desttop+=2)
    {
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

		// step through the posts in a column
		while (column->topdelta != 0xff )
		{
			source = (byte *)column + 3;
			//dest = &desttop[column->topdelta*ScreenWidth];
			dest=(word *)(desttop+column->topdelta*ScreenPitch);
			count = column->length;

			while (count--)
			{
				*dest = CurPal16[*(source++)];
				((byte *)dest) += ScreenPitch;
			}
			column = (column_t *)(  (byte *)column + column->length
				+ 4 );
		}
    }
    V_Unlock();
}

//nasty hack to draw menus etc. right size at high res
void V_DrawPatchBig(int x, int y, int scrn, patch_t *patch)
{
    word	*screen;
    fixed_t	xstep;
    fixed_t	ystep;
    fixed_t	xcount;
    fixed_t	ycount;
    int		count;
    int		col;
    column_t*	column;
    byte	*desttop;
    word	*dest;
    byte*	src;
    int		w;
    int		dy;
    fixed_t	xs;
    fixed_t	ys;

    xs=INT2F(ScreenWidth)/320;
    ys=INT2F(ScreenHeight)/200;
    xstep=INT2F(320)/ScreenWidth;
    ystep=INT2F(200)/ScreenHeight;
    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);
    x=F2INT(x*xs);
    y=F2INT(y*ys);

    w = SHORT(patch->width);
    xcount=0;
    screen=V_LockScreen(scrn, DDLOCK_WRITEONLY);
	if (!screen)
		return;
    //desttop=&screen[x+y*ScreenWidth];
	desttop=((byte *)screen)+(x<<1)+(y*ScreenPitch);
    col=0;
    while ((col<w)&&(x<ScreenWidth))
    {
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
		while (column->topdelta!=0xff)
		{
			dy=F2INT(column->topdelta*ys);
			dest=(word *)(desttop+dy*ScreenPitch);
			src=(byte *)column+3;
			count=column->length;
			ycount=0;
			while (count&&(y+dy<ScreenHeight))
			{
				*dest=CurPal16[*src];
				((byte *)dest)+=ScreenPitch;
				dy++;
				ycount+=ystep;
				if (ycount>=FRACUNIT)
				{
					ycount-=FRACUNIT;
					src++;
					count--;
				}
			}
			column = (column_t *)((byte *)column + column->length + 4);
		}
		desttop+=2;
		x++;
		xcount+=xstep;
		if (xcount>=FRACUNIT)
		{
			xcount-=FRACUNIT;
			col++;
		}
    }
    V_Unlock();
}

void
V_DrawPatchFlipped
( int		x,
 int		y,
 int		scrn,
 patch_t*	patch )
{
}

void
V_DrawPatchCol
( int		x,
 patch_t*	patch,
 int		col )
{
}


void V_TileFlat(char *flatname)
{
}

void V_BltScreen(int srcscrn, int destscrn)
{
    pDDSScreens[destscrn]->lpVtbl->BltFast(pDDSScreens[destscrn], 0, 0, pDDSScreens[srcscrn], NULL, DDBLTFAST_WAIT);
}

void V_BlankLine(int x, int y, int length)
{
    DDBLTFX	ddbltfx;
    RECT	r;

    r.left=x;
    r.top=y;
    r.right=r.left+length;
    r.bottom=r.top+1;
    ZeroMemory(&ddbltfx, sizeof(DDBLTFX));
    ddbltfx.dwSize=sizeof(DDBLTFX);
    ddbltfx.dwFillColor=0;
    pDDSScreens[0]->lpVtbl->Blt(pDDSScreens[0], &r, NULL, NULL, DDBLT_WAIT|DDBLT_ASYNC|DDBLT_COLORFILL, &ddbltfx);
}

void
V_MarkRect
( int		x,
 int		y,
 int		width,
 int		height )
{
}

void V_PutDot(int x, int y, int c)
{
	*(word *)(AMScreen+(x<<1)+y*ScreenPitch)=RGB16AutoMapColors[c];
}

void V_ClearAM(void)
{
	DDBLTFX	ddbltfx;
	HRESULT	hres;
	RECT	r;

	ddbltfx.dwSize=sizeof(DDBLTFX);
	ddbltfx.dwFillColor=0;
	r.left=0;
	r.right=ScreenWidth;
	r.top=0;
	r.bottom=ST_Y;
	hres=pDDSScreens[0]->lpVtbl->Blt(pDDSScreens[0], &r, NULL, NULL, DDBLT_WAIT|DDBLT_COLORFILL, &ddbltfx);
	if (hres!=DD_OK)
		I_Error("Blt(AMFill) Failed");
}

void V_StartAM(void)
{
	AMScreen=(byte *)V_LockScreen(0, DDLOCK_WAIT|DDLOCK_WRITEONLY);
}

void V_FinishAM(void)
{
	V_Unlock();
	AMScreen=NULL;
}

void R_FillBackScreen(void)
{
    byte*		src;
    word*		dest;
    int			x;
    int			y;
    patch_t*	patch;

    // DOOM border patch.
    char	name1[] = "FLOOR7_2";

    // DOOM II border patch.
    char	name2[] = "GRNROCK";

    char*	name;
	int		pitch;

    if ((ViewWidth == ScreenWidth)||(NumSplits>1))
		return;

    if ( *D_API.gamemode == commercial)
		name = name2;
    else
		name = name1;

    src = W_CacheLumpName (name, PU_CACHE);
	dest=V_LockScreen(1, DDLOCK_WRITEONLY);
	if (!dest)
		return;
	pitch=ScreenWidth*2-ScreenPitch;
    for (y=0 ; y<ScreenHeight-ST_HEIGHT ; y++)
    {
		for (x=0 ; x<ScreenWidth ; x++)
		{
			*(dest++)=CurPal16[src[(x&63)+((y&63)<<6)]];
		}
		((byte *)dest)+=pitch;
    }
	V_Unlock();

	//return;

    patch = W_CacheLumpName ("brdr_t",PU_CACHE);

    for (x=0 ; x<ViewWidth ; x+=8)
		V_DrawPatch (ViewWindowX+x,ViewWindowY-8,1,patch);
    patch = W_CacheLumpName ("brdr_b",PU_CACHE);

    for (x=0 ; x<ViewWidth ; x+=8)
		V_DrawPatch (ViewWindowX+x,ViewWindowY+ViewHeight,1,patch);
    patch = W_CacheLumpName ("brdr_l",PU_CACHE);

    for (y=0 ; y<ViewHeight ; y+=8)
		V_DrawPatch (ViewWindowX-8,ViewWindowY+y,1,patch);
    patch = W_CacheLumpName ("brdr_r",PU_CACHE);

    for (y=0 ; y<ViewHeight ; y+=8)
		V_DrawPatch (ViewWindowX+ViewWidth,ViewWindowY+y,1,patch);


    // Draw beveled edge.
    V_DrawPatch (ViewWindowX-8,
		ViewWindowY-8,
		1,
		W_CacheLumpName ("brdr_tl",PU_CACHE));

    V_DrawPatch (ViewWindowX+ViewWidth,
		ViewWindowY-8,
		1,
		W_CacheLumpName ("brdr_tr",PU_CACHE));

    V_DrawPatch (ViewWindowX-8,
		ViewWindowY+ViewHeight,
		1,
		W_CacheLumpName ("brdr_bl",PU_CACHE));

    V_DrawPatch (ViewWindowX+ViewWidth,
		ViewWindowY+ViewHeight,
		1,
		W_CacheLumpName ("brdr_br",PU_CACHE));
}

void V_Init (int width, int height, dboolean windowed, matrixinfo_t *matrixinfo)
{
	InWindow=windowed;
    ScreenWidth=width;
    ScreenHeight=height;
}

void V_BlankEmptyViews(void)
{
    DDBLTFX	ddbltfx;
    RECT	r;

    ZeroMemory(&ddbltfx, sizeof(DDBLTFX));
    ddbltfx.dwSize=sizeof(DDBLTFX);
    ddbltfx.dwFillColor=0;
	if (NumSplits==3)
	{
		r.left=ScreenWidth/2;
	    r.top=ScreenHeight/2;
	    r.right=ScreenWidth;
	    r.bottom=ScreenHeight;
	    pDDSScreens[0]->lpVtbl->Blt(pDDSScreens[0], &r, NULL, NULL, DDBLT_WAIT|DDBLT_ASYNC|DDBLT_COLORFILL, &ddbltfx);
	}
	else if (NumSplits==2)
	{
		r.left=0;
	    r.top=0;
	    r.right=ViewWindowX;
	    r.bottom=ScreenHeight;
	    pDDSScreens[0]->lpVtbl->Blt(pDDSScreens[0], &r, NULL, NULL, DDBLT_WAIT|DDBLT_ASYNC|DDBLT_COLORFILL, &ddbltfx);
		r.left=ViewWindowX+ViewWidth;
	    r.right=ScreenWidth;
	    pDDSScreens[0]->lpVtbl->Blt(pDDSScreens[0], &r, NULL, NULL, DDBLT_WAIT|DDBLT_ASYNC|DDBLT_COLORFILL, &ddbltfx);
	}
}

void V_DrawConsoleBackground(void)
{
	int		y;

	for (y=0;y<*D_API.pConsolePos;y+=2)
		V_BlankLine(0, y, ScreenWidth);
}
