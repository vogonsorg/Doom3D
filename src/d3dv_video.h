#ifndef D3DV_VIDEO_H
#define D3DV_VIDEO_H

#include "doomtype.h"

#include "doomdef.h"

// Needed because we are refering to patches.
#include "t_matrix.h"
#include "t_bsp.h"

//
// VIDEO
//

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.


extern dboolean	InWindow;


// Allocates buffer screens, call before R_Init.
void V_Init (int width, int height, dboolean windowed, matrixinfo_t *matrixinfo);

typedef struct
{
    byte	topdelta;
    byte	length;
}column_t;



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
