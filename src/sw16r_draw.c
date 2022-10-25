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
//	The actual span/column drawing functions.
//	Here find the main potential for optimization,
//	 e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------

#ifdef RCSID
static const char
rcsid[] = "$Id: r_draw.c,v 1.4 1997/02/03 16:47:55 b1 Exp $";
#endif


#include "doomdef.h"

//#include "i_system.h"
#include "swi_video.h"
#include "t_zone.h"
//#include "w_wad.h"

#include "swr_local.h"
#include "t_split.h"

// Needs access to LFB (guess what).
#include "swv_video.h"
#include "sw16_defs.h"

// State.
//#include "doomstat.h"

#include <stdlib.h>

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//


byte*		viewimage;
int		viewwidth;
int		viewheight;
int		viewwindowx;
int		viewwindowy;
pixel_t		**ylookup;
pixel_t		**ylookuptables[MAX_SPLITS][2];
int		*columnofs;
int		*screencolumnofs;

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
byte		translations[3][256];




//
// R_DrawColumn
// Source is the top of the column to scale.
//
int			dc_colormap;
int			dc_x;
int			dc_yl;
int			dc_yh;
fixed_t			dc_iscale;
fixed_t			dc_texturemid;

// first pixel in a column (possibly virtual)
byte*			dc_source;

// just for profiling
int			dccount;

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//

void R_DrawColumn (void)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;
    pixel_t		*colormap;

    count = dc_yh - dc_yl;

    // Zero length, column does not exceed a pixel.
    if (count < 0)
		return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= ScreenWidth
		|| dc_yl < 0
		|| dc_yh >= ScreenHeight)
		I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows?
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;
    colormap=colormaps[dc_colormap];

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.
    do
    {
		// Re-map color indices from wall texture column
		//  using a lighting/special effects LUT.
		*dest = colormap[dc_source[F2INT(frac)&127]];

		dest += ScreenWidth;
		frac += fracstep;

    } while (count--);
}

// UNUSED.
// Loop unrolled.
#if 0
void R_DrawColumn (void)
{
    int			count;
    byte*		source;
    pixel_t		*dest;
    pixel_t		*colormap;

    unsigned		frac;
    unsigned		fracstep;
    unsigned		fracstep2;
    unsigned		fracstep3;
    unsigned		fracstep4;

    count = dc_yh - dc_yl + 1;

    source = dc_source;
    colormap = colormap[dc_colormap];
    dest = ylookup[dc_yl] + columnofs[dc_x];

    fracstep = dc_iscale<<9;
    frac = (dc_texturemid + (dc_yl-centery)*dc_iscale)<<9;

    fracstep2 = fracstep+fracstep;
    fracstep3 = fracstep2+fracstep;
    fracstep4 = fracstep3+fracstep;

    while (count >= 8)
    {
		dest[0] = colormap[source[frac>>25]];
		dest[ScreenWidth] = colormap[source[(frac+fracstep)>>25]];
		dest[ScreenWidth*2] = colormap[source[(frac+fracstep2)>>25]];
		dest[ScreenWidth*3] = colormap[source[(frac+fracstep3)>>25]];

		frac += fracstep4;

		dest[ScreenWidth*4] = colormap[source[frac>>25]];
		dest[ScreenWidth*5] = colormap[source[(frac+fracstep)>>25]];
		dest[ScreenWidth*6] = colormap[source[(frac+fracstep2)>>25]];
		dest[ScreenWidth*7] = colormap[source[(frac+fracstep3)>>25]];

		frac += fracstep4;
		dest += ScreenWidth<<3;
		count -= 8;
    }

    while (count > 0)
    {
		*dest = colormap[source[frac>>25]];
		dest += ScreenWidth;
		frac += fracstep;
		count--;
    }
}
#endif



//
// Spectre/Invisibility.
//
#define FUZZTABLE		50
#define FUZZOFF	1


int	fuzzoffset[FUZZTABLE] =
{
    FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
		FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
		FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
		FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
		FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
		FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
		FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF
};

int	fuzzpos = 0;


void R_InitFuzz(void)
{
    for (fuzzpos=0;fuzzpos<FUZZTABLE;fuzzpos++)
    {
		if (fuzzoffset[fuzzpos]>0)
			fuzzoffset[fuzzpos]=ScreenWidth;
		else
			fuzzoffset[fuzzpos]=-ScreenWidth;
    }
    fuzzpos=0;
}

//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//
void R_DrawFuzzColumn (void)
{
    int			count;
    pixel_t*		dest;

    // Adjust borders. Low...
    if (!dc_yl)
		dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight-1)
		dc_yh = viewheight - 2;

    count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
		return;


#ifdef RANGECHECK
    if ((unsigned)dc_x >= ScreenWidth
		|| dc_yl < 0 || dc_yh >= ScreenHeight)
    {
		I_Error ("R_DrawFuzzColumn: %i to %i at %i",
			dc_yl, dc_yh, dc_x);
    }
#endif


    // Does not work with blocky mode.
    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks like an attempt at dithering,
    //  using the colormap #6 (of 0-31, a bit
    //  brighter than average).
    do
    {
		// Lookup framebuffer, and retrieve
		//  a pixel that is either one column
		//  left or right of the current one.
		// Add index from colormap to index.
		// Rubbish!
		// just swaps a few pixels around+makes them darker
		*dest = dest[fuzzoffset[fuzzpos]]&RGBFuzzMask;
		*dest-=(*dest)>>2;
		// Clamp table lookup index.
		if (++fuzzpos == FUZZTABLE)
			fuzzpos = 0;

		dest += ScreenWidth;

    } while (count--);
}

//
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//
byte*	dc_translation;
byte*	translationtables;

void R_DrawTranslatedColumn (void)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;
    pixel_t		*colormap;

    count = dc_yh - dc_yl;
    if (count < 0)
		return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= ScreenWidth
		|| dc_yl < 0
		|| dc_yh >= ScreenHeight)
    {
		I_Error ( "R_DrawColumn: %i to %i at %i",
			dc_yl, dc_yh, dc_x);
    }

#endif


    dest = ylookup[dc_yl] + columnofs[dc_x];

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;
    colormap=colormaps[dc_colormap];

    // Here we do an additional index re-mapping.
    do
    {
		// Translation tables are used
		//  to map certain colorramps to other ones,
		//  used with PLAY sprites.
		// Thus the "green" ramp of the player 0 sprite
		//  is mapped to gray, red, black/indigo.
		*dest = colormap[dc_translation[dc_source[F2INT(frac)]]];
		dest += ScreenWidth;

		frac += fracstep;
    } while (count--);
}




//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
void R_InitTranslationTables (void)
{
    int		i;

    translationtables = Z_Malloc (256*3+255, PU_STATIC, 0);
    translationtables = (byte *)(( (int)translationtables + 255 )& ~255);

    // translate just the 16 green colors
    for (i=0 ; i<256 ; i++)
    {
		if (i >= 0x70 && i<= 0x7f)
		{
			// map green ramp to gray, brown, red
			translationtables[i] = 0x60 + (i&0xf);
			translationtables [i+256] = 0x40 + (i&0xf);
			translationtables [i+512] = 0x20 + (i&0xf);
		}
		else
		{
			// Keep all other colors as is.
			translationtables[i] = translationtables[i+256]
				= translationtables[i+512] = i;
		}
    }
}




//
// R_DrawSpan
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
int			ds_y;
int			ds_x1;
int			ds_x2;

int			ds_colormap;

fixed_t			ds_xfrac;
fixed_t			ds_yfrac;
fixed_t			ds_xstep;
fixed_t			ds_ystep;

// start of a 64*64 tile image
byte*			ds_source;

// just for profiling
int			dscount;


//
// Draws the actual span.
#if 0
void R_DrawSpan (void)
{
    fixed_t		xfrac;
    fixed_t		yfrac;
    pixel_t*		dest;
    int			count;
    int			spot;
    pixel_t		*colormaps;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
		|| ds_x1<0
		|| ds_x2>=ScreenWidth
		|| (unsigned)ds_y>ScreenHeight)
    {
		I_Error( "R_DrawSpan: %i to %i at %i",
			ds_x1,ds_x2,ds_y);
    }
	//	dscount++;
#endif


    xfrac = ds_xfrac;
    yfrac = ds_yfrac;

    dest = ylookup[ds_y] + columnofs[ds_x1];
    colormap=colormaps[ds_colormap];

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

    do
    {
		// Current texture index in u,v.
		spot = ((yfrac>>(FRACBITS-6))&(63*64)) + (F2INT(xfrac)&63);

		// Lookup pixel from flat texture tile,
		//  re-index using light/colormap.
		*dest++ = colormap[[ds_source[spot]];

		// Next step in u,v.
		xfrac += ds_xstep;
		yfrac += ds_ystep;

    } while (count--);
}



// UNUSED.
// Loop unrolled by 4.
//#if 0
#else
void R_DrawSpan (void)
{
    unsigned	position, step;

    byte*	source;
    pixel_t	*colormap;
    pixel_t*	dest;

    unsigned	count;
    unsigned	spot;
    unsigned	xtemp;
    unsigned	ytemp;

    position = ((ds_xfrac<<10)&0xffff0000) | ((ds_yfrac>>6)&0xffff);
    step = ((ds_xstep<<10)&0xffff0000) | ((ds_ystep>>6)&0xffff);

    source = ds_source;
    colormap = colormaps[ds_colormap];
    dest = ylookup[ds_y] + columnofs[ds_x1];
    count = ds_x2 - ds_x1 + 1;

    while (count >= 4)
    {
		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		dest[0] = colormap[source[spot]];

		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		dest[1] = colormap[source[spot]];

		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		dest[2] = colormap[source[spot]];

		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		dest[3] = colormap[source[spot]];

		count -= 4;
		dest += 4;
    }
    while (count > 0)
    {
		ytemp = position>>4;
		ytemp = ytemp & 4032;
		xtemp = position>>26;
		spot = xtemp | ytemp;
		position += step;
		*dest++ = colormap[source[spot]];
		count--;
    }
}
#endif


static int ic=0;
//
// R_InitBuffer
// Creates lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void R_InitBuffer(void)
{
	int		split;
	int		i;
	int		x;
	int		y;
	pixel_t	*start[2];

	ic++;
	if (MatrixMode)
	{
		if (NumSplits>1)
			I_Error("Can't combine matrix and splitscreen");
		start[0]=MatrixScreens[0];
		start[1]=MatrixScreens[1];
	}
	else
	{
		start[0]=BufferScreens[0];
		start[1]=BufferScreens[1];
	}

	for (split=0;split<NumSplits;split++)
	{
		D_API.S_GetViewPos(split, &x, &y);
	    // Column offset. For windows.
	    for (i=0 ; i<viewheight ; i++)
		{
			ylookuptables[split][0][i] = start[0] + (i+y)*ScreenWidth;
			ylookuptables[split][1][i] = start[1] + (i+y)*ScreenWidth;
		}
	}
}

//
// R_FillBackScreen
// Fills the back screen with a pattern
//  for variable screen sizes
// Also draws a beveled edge.
//
void R_FillBackScreen (void)
{
    byte*	src;
    pixel_t*	dest;
    int		x;
    int		y;
    patch_t*	patch;

    // DOOM border patch.
    char	name1[] = "FLOOR7_2";

    // DOOM II border patch.
    char	name2[] = "GRNROCK";

    char*	name;

    if ((viewwidth == ScreenWidth)||(NumSplits>1))
		return;

    if ( *D_API.gamemode == commercial)
		name = name2;
    else
		name = name1;

    src = W_CacheLumpName (name, PU_CACHE);
    dest = screens[1];

    for (y=0 ; y<ST_Y ; y++)
    {
		for (x=0 ; x<ScreenWidth/64 ; x++)
		{
			V_TranslatePixels (dest, &src[(y&63)<<6], 64);
			dest += 64;
		}

		if (ScreenWidth&63)
		{
			V_TranslatePixels (dest, &src[(y&63)<<6], ScreenWidth&63);
			dest += (ScreenWidth&63);
		}
    }

    patch = W_CacheLumpName ("brdr_t",PU_CACHE);

    for (x=0 ; x<viewwidth ; x+=8)
		V_DrawPatch (viewwindowx+x,viewwindowy-8,1,patch);
    patch = W_CacheLumpName ("brdr_b",PU_CACHE);

    for (x=0 ; x<viewwidth ; x+=8)
		V_DrawPatch (viewwindowx+x,viewwindowy+viewheight,1,patch);
    patch = W_CacheLumpName ("brdr_l",PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
		V_DrawPatch (viewwindowx-8,viewwindowy+y,1,patch);
    patch = W_CacheLumpName ("brdr_r",PU_CACHE);

    for (y=0 ; y<viewheight ; y+=8)
		V_DrawPatch (viewwindowx+viewwidth,viewwindowy+y,1,patch);


    // Draw beveled edge.
    V_DrawPatch (viewwindowx-8,
		viewwindowy-8,
		1,
		W_CacheLumpName ("brdr_tl",PU_CACHE));

    V_DrawPatch (viewwindowx+viewwidth,
		viewwindowy-8,
		1,
		W_CacheLumpName ("brdr_tr",PU_CACHE));

    V_DrawPatch (viewwindowx-8,
		viewwindowy+viewheight,
		1,
		W_CacheLumpName ("brdr_bl",PU_CACHE));

    V_DrawPatch (viewwindowx+viewwidth,
		viewwindowy+viewheight,
		1,
		W_CacheLumpName ("brdr_br",PU_CACHE));
}


//
// Copy a screen buffer.
//
void
R_VideoErase
( unsigned	ofs,
 int		count )
{
	// LFB copy.
	// This might not be a good idea if memcpy
	//  is not optiomal, e.g. byte by byte on
	//  a 32bit CPU, as GNU GCC/Linux libc did
	//  at one point.
    memcpy (screens[0]+ofs, screens[1]+ofs, PIX2BYTE(count));
}


//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void
V_MarkRect
( int		x,
 int		y,
 int		width,
 int		height );

void R_DrawViewBorder (void)
{
    int		top;
    int		side;
    int		ofs;
    int		i;

    if (viewwidth == ScreenWidth)
		return;

    top = (ST_Y-viewheight)/2;
    side = (ScreenWidth-viewwidth)/2;

    // copy top and one line of left side
    R_VideoErase (0, top*ScreenWidth+side);

    // copy one line of right side and bottom
    ofs = (viewheight+top)*ScreenWidth-side;
    R_VideoErase (ofs, top*ScreenWidth+side);

    // copy sides using wraparound
    ofs = top*ScreenWidth + ScreenWidth-side;
    side <<= 1;

    for (i=1 ; i<viewheight ; i++)
    {
		R_VideoErase (ofs, side);
		ofs += ScreenWidth;
    }

    // ?
    V_MarkRect (0,0,ScreenWidth, ST_Y);
}

void R_SetupDrawFrame(int num, int split)
{
    ylookup=ylookuptables[split][num];
    columnofs=&screencolumnofs[viewwindowx];
}

void R_InitDraw(void)
{
	int		i;

	for (i=0;i<NumSplits;i++)
	{
//FUDGE: malloc one big lump for all ylookups?
	    ylookuptables[i][0]=(pixel_t **)malloc(ScreenHeight*sizeof(pixel_t *));
	    ylookuptables[i][1]=(pixel_t **)malloc(ScreenHeight*sizeof(pixel_t *));
	}
    screencolumnofs=(int *)malloc(ScreenWidth*sizeof(int));
    for (i=0;i<ScreenWidth;i++)
    	screencolumnofs[i]=i;
}

void R_MergeMatrixViews(void)
{
	pixel_t	*dest;
	int		line;
	int		count;
	int		srcnum;
	pixel_t	*src[2];

	dest=screens[ScreenNum]+viewwindowx+viewwindowy*ScreenWidth;
	for (srcnum=0;srcnum<2;srcnum++)
		src[srcnum]=MatrixScreens[srcnum]+viewwindowx+viewwindowy*ScreenWidth;
	srcnum=0;
	count=0;
	for (line=0;line<viewheight;line++)
	{
		memcpy(dest, src[srcnum], PIX2BYTE(viewwidth));
		src[0]+=ScreenWidth;
		src[1]+=ScreenWidth;
		dest+=ScreenWidth;
		count++;
		if (count>=MatrixInterleave)
		{
			count=0;
			srcnum=1-srcnum;
		}
	}
}
