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
// DESCRIPTION:
//	Gamma correction LUT.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------


#ifndef __SWV_VIDEO__
#define __SWV_VIDEO__

#include "doomtype.h"

#include "doomdef.h"

// Needed because we are refering to patches.
#include "swr_data.h"

extern char DllName[];

//
// VIDEO
//

#define CENTERY			(ScreenHeight/2)


// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.



extern  int	dirtybox[4];



// Allocates buffer screens, call before R_Init.
void V_Init (int width, int height, dboolean windowed, matrixinfo_t *matrixinfo);


void
V_CopyRect
( int		srcx,
  int		srcy,
  int		srcscrn,
  int		width,
  int		height,
  int		destx,
  int		desty,
  int		destscrn );

void
V_DrawPatch
( int		x,
  int		y,
  int		scrn,
  patch_t*	patch);

//nasyt hack to draw menus etc. right size at high res
void V_DrawPatchBig(int x, int y, int scrn, patch_t *patch);

void
V_DrawPatchFlipped
( int		x,
  int		y,
  int		scrn,
  patch_t*	patch );

void
V_DrawPatchCol
( int		x,
  patch_t*	patch,
  int		col );


void
V_MarkRect
( int		x,
  int		y,
  int		width,
  int		height );

void V_TileFlat(char *flatname);
void V_BltScreen(int srcscrn, int destscrn);
void V_BlankLine(int x, int y, int length);
void V_BlankEmptyViews(void);
void V_DrawConsoleBackground(void);

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
