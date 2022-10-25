#ifndef SW16_DEFS_H
#define SW16_DEFS_H

#define HICOLOR
#include "sw_stuff.h"
#include "t_map.h"

extern pixel_t CurrentPalette[256];
extern pixel_t RGBFuzzMask;
extern pixel_t	RGB16AutoMapColors[NUM_AM_COLORS];

void V_TranslatePixels(pixel_t *dest, byte *src, int count);

#endif