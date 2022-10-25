#ifndef D3DR_MAIN_H
#define D3DR_MAIN_H

#include "d3dr_data.h"
#include "t_bsp.h"
#include "t_player.h"
#include "t_wad.h"

void R_Init(void);

void R_FillBackScreen(void);
void R_DrawViewBorder(void);
void R_ViewSizeChanged(void);
void R_RenderPlayerView(player_t *player);
void R_SetSkyTexture(char *name);
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
angle_t R_PointToAngle2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
angle_t R_PointToAngle(fixed_t x, fixed_t y);//note difference from sw version
int R_FlatNumForName(char *name);
int R_TextureNumForName(char *name);
int R_CheckTextureNumForName(char *name);
void R_PrecacheLevelQuick(void);
void R_PrecacheLevel(void);
int R_PointOnSide(fixed_t x, fixed_t y, node_t *node);
void R_SetupLevel(dboolean hasglnodes);
void R_SetViewAngleOffset(angle_t angle);
void R_SetViewOffset(int offset);

extern float	viewsin;
extern float	viewcos;
extern float	viewoffset;
extern dboolean	fullbright;
extern dboolean	invul;
extern int		skytexture;
extern int		ValidCount;
extern char		*MD2ini;
extern player_t	*renderplayer;
extern int		playersector;

#endif