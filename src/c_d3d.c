//r_d3d.c

char    DllName[]="Direct3D";
#define DLL_CAPS (APICAP_HW|APICAP_FULLSCREEN|APICAP_WINDOW|APICAP_POLYBASED|APICAP_D3DDEVICE)

#include "c_d3d.h"

#include "i_gamma.h"
#include "i_overlay.h"
#include "d3di_video.h"
#include "d3dv_video.h"
#include "d3dr_local.h"
#include "d3df_wipe.h"

apiin_t D_API;

default_t       Defaults[]=
{
		{"UseColorKey", &UseColorKey, false, false},
		{"BilinearFiltering", &BilinearFiltering, true, false},
		{"TextureMip", &TextureMip, 0, false},
		{"SpriteMip", &SpriteMip, 0, false},
		{"MinTextureSize", &MinTextureSize, 1, false},
		{"MaxTextureSize", &MaxTextureSize, 256, false},
		{"ForceSquareTextures", &SquareTextures, true, false},
		{"MD2ini", (int *)&MD2ini, (int)"", true},
		{"NoMD2Weapons", &DisableMD2Weapons, false, false},
		{"skin1", (int *)&PlayerSkins[0].name, (int)"md2\\doomguy\\green.pcx", true},
		{"skin2", (int *)&PlayerSkins[1].name, (int)"md2\\doomguy\\black.pcx", true},
		{"skin3", (int *)&PlayerSkins[2].name, (int)"md2\\doomguy\\brown.pcx", true},
		{"skin4", (int *)&PlayerSkins[3].name, (int)"md2\\doomguy\\red.pcx", true},
		{NULL}
};

dllinfo_t       DllInfo=
{
		DllName,
		DLL_CAPS,
		Defaults
};

void (*I_Error)(char *error, ...);
void (*I_Printf)(char *fmt, ...);
void *(*Z_Malloc)(int size, int tag, void *user);
void (*Z_Free)(void *ptr);
void (*Z_ChangeTag2)(void *ptr, int tag);
void *(*W_CacheLumpNum)(int lump, int tag);
void *(*W_CacheLumpName)(char *name, int tag);
int (*W_CheckNumForName)(char *name);
int (*W_GetNumForName)(char *name);
int (*W_LumpLength)(int lump);
void (*W_ReadLump)(int lump, void *dest);
dboolean (*L_CanSee)(int s1, int s2);

int             numvertexes;
vertex_t        *vertexes;
int             numsegs;
seg_t           *segs;
int             numsectors;
sector_t        *sectors;
int             numsubsectors;
subsector_t     *subsectors;
int             numnodes;
node_t          *nodes;
int             numlines;
line_t          *lines;
int             numsides;
side_t          *sides;

int				NumSplits;

void __stdcall FreeDoomAPI(void)
{
}

void ProcessDAPI(apiin_t *apiin)
{
	memcpy(&D_API, apiin, sizeof(apiin_t));
	I_Printf=D_API.I_Printf;
	I_Error=D_API.I_Error;
	Z_Malloc=D_API.Z_Malloc;
	Z_Free=D_API.Z_Free;
	Z_ChangeTag2=D_API.Z_ChangeTag2;
	W_CacheLumpNum=D_API.W_CacheLumpNum;
	W_CacheLumpName=D_API.W_CacheLumpName;
	W_CheckNumForName=D_API.W_CheckNumForName;
	W_GetNumForName=D_API.W_GetNumForName;
	W_LumpLength=D_API.W_LumpLength;
	W_ReadLump=D_API.W_ReadLump;
	L_CanSee=D_API.L_CanSee;
	NumSplits=D_API.NumSplits;
}

int GetAPIInfo(apiinfo_t *apiinfo)
{
	apiinfo->FreeDoomAPI=FreeDoomAPI;
		apiinfo->info=&DllInfo;
	return(API_OK);
}

int __stdcall DoomAPI(apiin_t *apiin, apiout_t *apiout)
{
	if (!apiin)
		return(APIERR_NULLPARM);
	if ((apiin->size==sizeof(apiinfo_t))&&(apiin->version==API_INFO_VERSION)&&!apiout)
		return(GetAPIInfo((apiinfo_t*)apiin));
	if (!apiout)
		return(APIERR_NULLPARM2);
	if ((apiin->size>=8)&&(apiin->version!=D_API_VERSION))
		return(APIERR_WRONGVERSION);
	if ((apiin->size!=sizeof(apiin_t))||(apiout->size!=sizeof(apiout_t)))
		return(APIERR_WRONGSIZE);
	ProcessDAPI(apiin);
	apiout->version=C_API_VERSION;
	apiout->FreeDoomAPI=FreeDoomAPI;
	apiout->info=&DllInfo;
	apiout->skyflatnum=&skyflatnum;
	apiout->I_StartDrawing=I_StartDrawing;
	apiout->I_FinishUpdate=I_FinishUpdate;
	apiout->I_SetPalette=I_SetPalette;
	apiout->I_InitGraphics=I_InitGraphics;
	apiout->I_ShutdownGraphics=I_ShutdownGraphics;
	apiout->I_ScreenShot=I_ScreenShot;
	apiout->I_SetGamma=I_SetGamma;
	apiout->I_BeginRead=I_BeginRead;
	apiout->R_SetColorBias=R_SetColorBias;
	apiout->R_Init=R_Init;
	apiout->R_FillBackScreen=R_FillBackScreen;
	apiout->R_DrawViewBorder=R_DrawViewBorder;
	apiout->R_RenderPlayerView=R_RenderPlayerView;
	apiout->R_SetSkyTexture=R_SetSkyTexture;
	apiout->R_PointInSubsector=R_PointInSubsector;
	apiout->R_PointToAngle2=R_PointToAngle2;
	apiout->R_FlatNumForName=R_FlatNumForName;
	apiout->R_TextureNumForName=R_TextureNumForName;
	apiout->R_CheckTextureNumForName=R_CheckTextureNumForName;
	apiout->R_PrecacheLevel=R_PrecacheLevel;
	apiout->R_PrecacheQuick=R_PrecacheLevelQuick;
	apiout->R_InitSprites=R_InitSprites;
	apiout->R_SetupLevel=R_SetupLevel;
	apiout->R_SetViewAngleOffset=R_SetViewAngleOffset;
	apiout->R_SetViewOffset=R_SetViewOffset;
	apiout->R_ViewSizeChanged=R_ViewSizeChanged;
	apiout->V_BlankEmptyViews=V_BlankEmptyViews;
	apiout->V_Init=V_Init;
	apiout->V_DrawPatch=V_DrawPatch;
	apiout->V_DrawPatchFlipped=V_DrawPatchFlipped;
	apiout->V_DrawPatchBig=V_DrawPatchBig;
	apiout->V_DrawPatchCol=V_DrawPatchCol;
	apiout->V_CopyRect=V_CopyRect;
	apiout->V_TileFlat=V_TileFlat;
	apiout->V_MarkRect=V_MarkRect;
	apiout->V_BlankLine=V_BlankLine;
	apiout->V_BltScreen=V_BltScreen;
	apiout->V_DrawConsoleBackground=V_DrawConsoleBackground;
	apiout->wipe_StartScreen=wipe_StartScreen;
	apiout->wipe_EndScreen=wipe_EndScreen;
	apiout->wipe_ScreenWipe=wipe_ScreenWipe;
	apiout->AM_Clear=AM_Clear;
	apiout->AM_StartDrawing=AM_StartDrawing;
	apiout->AM_FinishDrawing=AM_FinishDrawing;
	apiout->AM_SetMode=AM_SetMode;
	apiout->AM_GetWidth=AM_GetWidth;
	apiout->AM_GetHeight=AM_GetHeight;
	apiout->AM_GetPutDot=AM_GetPutDot;
	apiout->numvertexes=&numvertexes;
	apiout->pvertexes=&vertexes;
	apiout->numsegs=&numsegs;
	apiout->psegs=&segs;
	apiout->numsectors=&numsectors;
	apiout->psectors=&sectors;
	apiout->numsubsectors=&numsubsectors;
	apiout->psubsectors=&subsectors;
	apiout->numnodes=&numnodes;
	apiout->pnodes=&nodes;
	apiout->numlines=&numlines;
	apiout->plines=&lines;
	apiout->numsides=&numsides;
	apiout->psides=&sides;
	apiout->plumpinfo=&lumpinfo;
	apiout->pvalidcount=&ValidCount;
	apiout->pViewWidth=&ViewWidth;
	apiout->pViewHeight=&ViewHeight;
	return(API_OK);
}
