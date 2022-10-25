//common.h
//common rendering(+screen manipulation) elements

#ifndef COMMON_H
#define COMMON_H

#include "doomdef.h"
#include "t_player.h"
#include "t_bsp.h"
#include "t_misc.h"
#include "t_wad.h"
#include "t_matrix.h"
#include "t_info.h"
#include "apiinfo.h"

//D_API
typedef struct
{
    int size;
    int version;
    void (*I_Printf)(char *fmt, ...);
    void (*I_Error)(char *error, ...);
    void *(*Z_Malloc)(int size, int tag, void *user);
    void (*Z_Free)(void *ptr);
    void (*Z_ChangeTag2)(void *ptr, int tag);
    void *(*W_CacheLumpNum)(int lump, int tag);
    void *(*W_CacheLumpName)(char *name, int tag);
    int (*W_CheckNumForName)(char *name);
    int (*W_GetNumForName)(char *name);
    int (*W_LumpLength)(int lump);
    void (*W_ReadLump)(int lump, void *dest);
    int (*M_Random)(void);
    void (*P_MobjThinker)(mobj_t *mobj);
	void (*D_IncValidCount)(void);
    void (*NetUpdate)(void);
    void (*S_GetViewPos)(int split, int *x, int *y);
	dboolean (*L_CanSee)(int s1, int s2);
    thinker_t *thinkercap;
    GameMode_t *gamemode;

    fixed_t **ptextureheight;
    int **ptexturetranslation;
    int **pflattranslation;
    int *modifiedgame;
    HWND *pMainWnd;
    int *pGammaLevel;
	LPRECT pWindowPos;
	dboolean *pShowGun;
	dboolean *pInBackground;
	state_t *states;
	player_t *players;
	int NumSplits;
	int *pConsolePos;
	LPGUID *ppD3DDeviceGUID;
	dboolean *pFixSprites;
	int	*pNumSSDrawn;
}apiin_t;

typedef void (*gpd_t)(int, int, int);

//C_API
typedef struct
{
    int size;
    int version;
    void (__stdcall *FreeDoomAPI)(void);
	dllinfo_t *info;
    int *skyflatnum;
    void (*I_StartDrawing)(void);
    void (*I_FinishUpdate)(void);
    void (*I_SetPalette)(byte *palette);
    void (*I_InitGraphics)(void);
    void (*I_ShutdownGraphics)(void);
    void (*I_ScreenShot)(void);
    void (*I_SetGamma)(void);//I_SetPalette is called if returns true
    void (*I_BeginRead)(void);
	void (*R_SetColorBias)(int type, int strength);
    void (*R_Init)(void);
    void (*R_FillBackScreen)(void);
    void (*R_DrawViewBorder)(void);
    void (*R_RenderPlayerView)(player_t* player);
    void (*R_SetSkyTexture)(char *name);
    subsector_t *(*R_PointInSubsector)(fixed_t x, fixed_t y);
    angle_t (*R_PointToAngle2)(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
    int (*R_FlatNumForName)(char *name);
    int (*R_TextureNumForName)(char *name);
    int (*R_CheckTextureNumForName)(char *name);
    void (*R_PrecacheLevel)(void);
    void (*R_PrecacheQuick)(void);
    void (*R_InitSprites)(char **namelist);
    void (*R_SetupLevel)(dboolean hasglnodes);
	void (*R_SetViewAngleOffset)(angle_t angle);
	void (*R_SetViewOffset)(int offset);
	void (*R_ViewSizeChanged)(void);
	void (*V_BlankEmptyViews)(void);
    void (*V_Init)(int width, int height, dboolean windowed, matrixinfo_t *matrixinfo);
    void (*V_DrawPatch)(int x, int y, int scrn, patch_t* patch);
    void (*V_DrawPatchFlipped)(int x, int y, int scrn, patch_t* patch);
    void (*V_DrawPatchBig)(int x, int y, int scrn, patch_t* patch);
    void (*V_DrawPatchCol)(int x, patch_t *patch, int col);
    void (*V_CopyRect)(int srcx, int srcy, int srcscrn, int width, int height, int destx, int desty, int destscrn);
    void (*V_TileFlat)(char *flatname);
    void (*V_MarkRect)(int x, int y, int width, int height);
    void (*V_BlankLine)(int x, int y, int length);
    void (*V_BltScreen)(int srcscrn, int destscrn);
	void (*V_DrawConsoleBackground)(void);
    void (*wipe_StartScreen)(void);
    void (*wipe_EndScreen)(void);
    dboolean (*wipe_ScreenWipe)(int ticks);
    void (*AM_Clear)(void);
    void (*AM_StartDrawing)(void);
    void (*AM_FinishDrawing)(void);
    void (*AM_SetMode)(int mode);
    int (*AM_GetWidth)(void);
    int (*AM_GetHeight)(void);
    gpd_t (*AM_GetPutDot)(void);
    int *numvertexes;
    vertex_t **pvertexes;
    int *numsegs;
    seg_t **psegs;
    int *numsectors;
    sector_t **psectors;
    int *numsubsectors;
    subsector_t **psubsectors;
    int *numnodes;
    node_t **pnodes;
    int *numlines;
    line_t **plines;
    int *numsides;
    side_t **psides;
    lumpinfo_t **plumpinfo;
	int *pvalidcount;
	int	*pViewWidth;
	int	*pViewHeight;
}apiout_t;

//int __stdcall DoomAPI(apiin_t d_api*, apiout_t *c_api);//nonzero on error

#endif