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
//	Gamma correction LUT stuff.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------

#ifdef RCSID
static const char
rcsid[] = "$Id: v_video.c,v 1.5 1997/02/03 22:45:13 b1 Exp $";
#endif

char	DllName[]="Hacked 16 Bit";

#include <stdlib.h>

//#include "i_system.h"
#include "swr_local.h"

#include "doomdef.h"
//#include "doomdata.h"
//#include "d_stuff.h"

#include "m_swap.h"
//#include "m_bbox.h"
//#include "m_argv.h"

#include "swv_video.h"
#include "swi_video.h"
#include "sw16_defs.h"
#include "t_map.h"

#define TRANS_PIXEL(x) (CurrentPalette[x])

// Each screen is [ScreenWidth*ScreenHeight];
pixel_t*			screens[5];
pixel_t				*MatrixScreens[2];

//int				dirtybox[4];

pixel_t	RGB16AutoMapColors[NUM_AM_COLORS];

void V_TranslatePixels(pixel_t *dest, byte *src, int count)
{
    byte	*end;

    end=&src[count];
    while (src<end)
		*(dest++)=CurrentPalette[*(src++)];
}

//FUDGE: remove alltogether?
//
// V_MarkRect
//
void
V_MarkRect
( int		x,
 int		y,
 int		width,
 int		height )
{
	//    M_AddToBox (dirtybox, x, y);
	//    M_AddToBox (dirtybox, x+width-1, y+height-1);
}


//
// V_CopyRect
//
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
    pixel_t*	src;
    pixel_t*	dest;

#ifdef RANGECHECK
    if (srcx<0
		||srcx+width >ScreenWidth
		|| srcy<0
		|| srcy+height>ScreenHeight
		||destx<0||destx+width >ScreenWidth
		|| desty<0
		|| desty+height>ScreenHeight
		|| (unsigned)srcscrn>4
		|| (unsigned)destscrn>4)
    {
		I_Error ("Bad V_CopyRect");
    }
#endif
    V_MarkRect (destx, desty, width, height);

    src = screens[srcscrn]+ScreenWidth*srcy+srcx;
    dest = screens[destscrn]+ScreenWidth*desty+destx;

    for ( ; height>0 ; height--)
    {
		memcpy (dest, src, PIX2BYTE(width));
		src += ScreenWidth;
		dest += ScreenWidth;
    }
}


//
// V_DrawPatch
// Masks a column based masked pic to the screen.
//
void
V_DrawPatch
( int		x,
 int		y,
 int		scrn,
 patch_t*	patch )
{

    int		count;
    int		col;
    column_t*	column;
    pixel_t*	desttop;
    pixel_t*	dest;
    byte*	source;
    int		w;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);
#ifdef RANGECHECK
    if (x<0
		||x+SHORT(patch->width) >ScreenWidth
		|| y<0
		|| y+SHORT(patch->height)>ScreenHeight
		|| (unsigned)scrn>4)
    {
		I_Printf("Patch at %d,%d exceeds LFB\n", x,y );
		// No I_Error abort - what is up with TNT.WAD?
		I_Printf("V_DrawPatch: bad patch (ignored)\n");
		return;
    }
#endif

    if (!scrn)
		V_MarkRect (x, y, SHORT(patch->width), SHORT(patch->height));

    col = 0;
    desttop = screens[scrn]+y*ScreenWidth+x;

    w = SHORT(patch->width);

    for ( ; col<w ; x++, col++, desttop++)
    {
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));

		// step through the posts in a column
		while (column->topdelta != 0xff )
		{
			source = (byte *)column + 3;
			dest = &desttop[column->topdelta*ScreenWidth];
			count = column->length;

			while (count--)
			{
				*dest = TRANS_PIXEL(*source++);
				dest += ScreenWidth;
			}
			column = (column_t *)(  (byte *)column + column->length
				+ 4 );
		}
    }
}


void V_DrawPatchBig(int x, int y, int scrn, patch_t *patch)
{
    fixed_t	xstep;
    fixed_t	ystep;
    fixed_t	xcount;
    fixed_t	ycount;
    int		count;
    int		col;
    column_t*	column;
    pixel_t	*desttop;
    pixel_t	*dest;
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
    if (!scrn)
		V_MarkRect (x, y, ScreenWidth, ScreenHeight);

    w = SHORT(patch->width);
    xcount=0;
    desttop=&screens[scrn][x+y*ScreenWidth];
    col=0;
    while ((col<w)&&(x<ScreenWidth))
    {
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
		while (column->topdelta!=0xff)
		{
			dy=F2INT(column->topdelta*ys);
			dest=desttop+dy*ScreenWidth;
			src=(byte *)column+3;
			count=column->length;
			ycount=0;
			while (count&&(y+dy<ScreenHeight))
			{
				*dest=TRANS_PIXEL(*src);
				dest+=ScreenWidth;
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
		desttop++;
		x++;
		xcount+=xstep;
		if (xcount>=FRACUNIT)
		{
			xcount-=FRACUNIT;
			col++;
		}
    }
}

//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//
void
V_DrawPatchFlipped
( int		x,
 int		y,
 int		scrn,
 patch_t*	patch )
{

    int		count;
    int		col;
    column_t*	column;
    pixel_t*	desttop;
    pixel_t*	dest;
    byte*	source;
    int		w;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);
#ifdef RANGECHECK
    if (x<0
		||x+SHORT(patch->width) >ScreenWidth
		|| y<0
		|| y+SHORT(patch->height)>ScreenHeight
		|| (unsigned)scrn>4)
    {
		I_Printf("Patch origin %d,%d exceeds LFB\n", x,y );
		I_Error ("Bad V_DrawPatch in V_DrawPatchFlipped");
    }
#endif

    if (!scrn)
		V_MarkRect (x, y, SHORT(patch->width), SHORT(patch->height));

    col = 0;
    desttop = screens[scrn]+y*ScreenWidth+x;

    w = SHORT(patch->width);

    for ( ; col<w ; x++, col++, desttop++)
    {
		column = (column_t *)((byte *)patch + LONG(patch->columnofs[w-1-col]));

		// step through the posts in a column
		while (column->topdelta != 0xff )
		{
			source = (byte *)column + 3;
			dest = desttop + column->topdelta*ScreenWidth;
			count = column->length;

			while (count--)
			{
				*dest = TRANS_PIXEL(*source++);
				dest += ScreenWidth;
			}
			column = (column_t *)(  (byte *)column + column->length
				+ 4 );
		}
    }
}



//
// V_DrawPatchCol
//
void
V_DrawPatchCol
( int		x,
 patch_t*	patch,
 int		col )
{
    column_t*	column;
    byte*	source;
    pixel_t*	dest;
    pixel_t*	desttop;
    int		count;

    column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
    desttop = screens[0]+x;

    // step through the posts in a column
    while (column->topdelta != 0xff )
    {
		source = (byte *)column + 3;
		dest = &desttop[column->topdelta*ScreenWidth];
		count = column->length;

		while (count--)
		{
			*dest = TRANS_PIXEL(*source++);
			dest += ScreenWidth;
		}
		column = (column_t *)(  (byte *)column + column->length + 4 );
    }
}


//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//
void
V_DrawBlock
( int		x,
 int		y,
 int		scrn,
 int		width,
 int		height,
 pixel_t*		src )
{
    pixel_t*	dest;

#ifdef RANGECHECK
    if (x<0
		||x+width >ScreenWidth
		|| y<0
		|| y+height>ScreenHeight
		|| (unsigned)scrn>4 )
    {
		I_Error ("Bad V_DrawBlock");
    }
#endif

    V_MarkRect (x, y, width, height);

    dest = screens[scrn] + y*ScreenWidth+x;

    while (height--)
    {
		memcpy (dest, src, PIX2BYTE(width));
		src += width;
		dest += ScreenWidth;
    }
}



//
// V_GetBlock
// Gets a linear block of pixels from the view buffer.
//
void
V_GetBlock
( int		x,
 int		y,
 int		scrn,
 int		width,
 int		height,
 pixel_t*		dest )
{
    pixel_t*	src;

#ifdef RANGECHECK
    if (x<0
		||x+width >ScreenWidth
		|| y<0
		|| y+height>ScreenHeight
		|| (unsigned)scrn>4 )
    {
		I_Error ("Bad V_DrawBlock");
    }
#endif

    src = screens[scrn] + y*ScreenWidth+x;

    while (height--)
    {
		memcpy (dest, src, PIX2BYTE(width));
		src += ScreenWidth;
		dest += width;
    }
}

void V_TileFlat(char *flatname)
{
    byte	*src;
    pixel_t	*dest;
    int		y;
    int		x;

    src=W_CacheLumpName(flatname, PU_CACHE);
    dest=screens[0];
    for (y=0;y<ScreenWidth;y++)
    {
		for (x=0;x<ScreenWidth-63;x+=64)
		{
			V_TranslatePixels(dest, &src[(y&63)<<6], 64);
			dest+=64;
		}
		x=ScreenWidth&63;
		if (x)
		{
			V_TranslatePixels(dest, &src[(y&63)<<6], x);
			dest+=x;
		}
    }
}

void V_BltScreen(int srcscrn, int destscrn)
{
    memcpy(screens[destscrn], screens[srcscrn], PIX2BYTE(ScreenWidth*ScreenHeight));
}

void V_BlankLine(int x, int y, int length)
{
    pixel_t	*dest;
    pixel_t	*end;

    dest=&screens[0][x+ScreenWidth*y];
    end=&dest[length];
    while (dest<end)
		*(dest++)=0;
}

void V_PutDot(int x, int y, int c)
{
	screens[0][x+y*ScreenWidth]=RGB16AutoMapColors[c];
}

void V_ClearAM(void)
{
	memset(screens[0], 0, PIX2BYTE(ScreenWidth*ScreenHeight));
}

void V_StartAM(void)
{
}

void V_FinishAM(void)
{
}

//
// V_Init
//
void V_Init (int width, int height, dboolean windowed, matrixinfo_t *matrixinfo)
{
    int		i;
    pixel_t	*base;

    InWindow=windowed;
    ScreenWidth=width;
    ScreenHeight=height;
    base = (pixel_t *)malloc(PIX2BYTE(ScreenWidth*ScreenHeight*4));

    for (i=0 ; i<4 ; i++)
		screens[i] = base + i*ScreenWidth*ScreenHeight;
    screens[4]=(pixel_t *)malloc(PIX2BYTE(ScreenWidth*ST_HEIGHT));
	if (matrixinfo)
	{
		if (NumSplits>1)
			I_Error("Can't use matrix in splitscreen mode");
		MatrixInterleave=matrixinfo->interleave;
		MatrixSeperation=matrixinfo->seperation;
		MatrixMode=true;
		MatrixScreens[0]=(pixel_t *)malloc(PIX2BYTE(ScreenWidth*ScreenHeight));
		MatrixScreens[1]=(pixel_t *)malloc(PIX2BYTE(ScreenWidth*ScreenHeight));
	}
}

//SWCOMMON:
void V_BlankEmptyViews(void)
{
	int		y;
	int		w;

	if (NumSplits==2)
	{
		w=viewwindowx+viewwidth;
		for (y=0;y<ScreenHeight;y++)
		{
			V_BlankLine(0, y, viewwindowx);
			V_BlankLine(w, y, ScreenWidth-w);
		}
	}
	else if (NumSplits==3)
	{
		w=ScreenWidth/2;
		for (y=ScreenHeight/2;y<ScreenHeight;y++)
		{
			V_BlankLine(w, y, w);
		}
	}
}

//SWCOMMON:
void V_DrawConsoleBackground(void)
{
	int		y;

	for (y=0;y<*D_API.pConsolePos;y+=2)
		V_BlankLine(0, y, ScreenWidth);
}
