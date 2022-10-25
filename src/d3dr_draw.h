#ifndef D3DR_DRAW_H
#define D3DR_DRAW_H

#include "d3di_ddraw.h"

void R_ClearView(void);
void R_DrawWall(void);
void R_BeginScene(void);
void R_EndScene(void);
void R_SelectTexture(int texnum, int *width, int *height);
void R_DrawSectorPlanes(sector_t *sector);
void R_DrawSubSectorPlanes(subsector_t *sub);
void R_InitDraw(void);
void R_InitLightmaps(void);
void R_RecalcLightmaps(void);
void R_DrawSprite(mobj_t *thing);
void R_RenderPlayerSprites(player_t *player);
void R_StartSprites(void);
void R_EndSprites(void);
void R_DrawSky(void);
void R_SetColorBias(int type, int strength);
int R_GetLight(int dist, int level);
void R_StartTextureFrame(void);

extern DOOM3DVERTEX	WallVertices[5];
extern D3DCOLOR		*Lightmap;
extern dboolean		MD2SpriteMatrix;
extern DOOM3DVERTEX	*SSectorVertices;
extern dboolean		DisableMD2Weapons;

#endif
