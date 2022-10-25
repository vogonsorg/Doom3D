#ifndef SWI_OVERLAY_H
#define SWI_OVERLAY_H

#include "t_map.h"

extern byte AutoMapColors[NUM_AM_COLORS];

void I_InitOverlay(void);
void I_ShutdownOverlay(void);

void AM_PutDot(int x, int y, int color);
void AM_Clear(void);
void AM_StartDrawing(void);
void AM_FinishDrawing(void);
gpd_t AM_GetPutDot(void);
int AM_GetWidth(void);
int AM_GetHeight(void);
void AM_SetMode(int mode);

#endif