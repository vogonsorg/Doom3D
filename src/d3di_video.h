#ifndef D3DI_VIDEO_H
#define D3DI_VIDEO_H

#include "t_fixed.h"
#include "t_tables.h"
#include "t_map.h"

void I_InitGraphics(void);
void I_ShutdownGraphics(void);

void I_StartDrawing(void);
void I_FinishUpdate(void);
void I_SetPalette(byte *palette);
void I_ScreenShot(void);
void I_SetViewMatrix(float x, float y, float z, angle_t angle);
void I_InitTransforms(void);
void I_SetRenderViewports(void);
void I_BeginRead(void);
void I_SelectViewport(int split);

extern dboolean	BilinearFiltering;

extern word	CurPal16[256];
extern word	TexturePal[MAXPLAYERS][256];
extern word	AlphaTexturePal[MAXPLAYERS][256];
extern word	TextureAlphaMask;
extern word	TextureClearColor;
extern dboolean	UseColorKey;
extern word RGB16AutoMapColors[NUM_AM_COLORS];

extern int ViewWidth;
extern int ViewHeight;
extern int ViewWindowX;
extern int ViewWindowY;

#endif