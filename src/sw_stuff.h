#ifndef SW_STUFF_H
#define SW_STUFF_H

#include "doomtype.h"

#ifdef HICOLOR
typedef word pixel_t;
#define PIX2BYTE(x) ((x)<<1)
#define BYTE2PIX(x) ((x)>>1)
#else
#ifndef NOT_HICOLOR
#error Do not include directly, use either sw8_defs.h or sw16_defs.h
#endif
typedef byte pixel_t;
#define PIX2BYTE(x) (x)
#define BYTE2PIX(x) (x)
#endif

extern pixel_t	*BufferScreens[2];

extern	pixel_t*	screens[5];
extern	pixel_t		*MatrixScreens[2];



// Draw a linear block of pixels into the view buffer.
void
V_DrawBlock
( int		x,
  int		y,
  int		scrn,
  int		width,
  int		height,
  pixel_t*		src );

// Reads a linear block of pixels into the view buffer.
void
V_GetBlock
( int		x,
  int		y,
  int		scrn,
  int		width,
  int		height,
  pixel_t*		dest );

void I_ReadScreen (pixel_t* scr);

extern pixel_t **colormaps;

#endif