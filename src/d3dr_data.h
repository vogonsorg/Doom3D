#ifndef D3DR_DATA_H
#define D3DR_DATA_H

#include "doomtype.h"
#include "doomdef.h"
#include "t_wad.h"
#include "d3dr_md2.h"
#include <ddraw.h>
#include <d3d.h>

typedef struct
{
	LPDIRECT3DTEXTURE2	ptex;
	int					count;
	char				*name;
}skin_t;

typedef struct
{
    // Block origin (allways UL),
    // which has allready accounted
    // for the internal origin of the patch.
    int		originx;
    int		originy;
    int		patch;
} texpatch_t;

typedef struct
{
    char	name[8];
    int		width;
    int		height;
	int		count;
    LPDIRECT3DTEXTURE2		ptex;
    int		numpatches;
    texpatch_t	patches[1];
} texture_t;

void R_InitData(void);
void R_LoadTexture(texture_t *texture);
void R_LoadFlat(int flatnum);
void R_LoadSprite(int spritenum, int mip, int trans);
void R_LoadSkin(skin_t *skin, int mip);
void R_DumpTextures(void);

int R_RoundPow2(int n, int mip);
LPDIRECTDRAWSURFACE4 R_CreateNewTexture(LPDDSURFACEDESC2 ddsd);
void R_FillTexture(LPDIRECTDRAWSURFACE4 pdds, int destwidth, int destheight, word *data, int srcwidth, int srcheight);

void R_CheckTextureSizeLimits(void);

extern int					skyflatnum;
extern lumpinfo_t			*lumpinfo;
extern texture_t			**textures;
extern LPDIRECT3DTEXTURE2	*FlatTextures;
extern LPDIRECT3DTEXTURE2	*SpriteTextures[MAXPLAYERS];
extern int					*FlatCounts;
extern int					*SpriteCounts[MAXPLAYERS];
extern int*					flattranslation;
extern int*					texturetranslation;
extern int					firstspritelump;
extern int					numspritelumps;
extern int					lastspritelump;
extern float				*spritewidth;
extern float				*spriteoffset;
extern float				*spritetopoffset;
extern float				*spriteheight;
extern int					TextureMip;
extern int					SpriteMip;
extern dboolean				SquareTextures;
extern int					TextureCount;
extern int					MinTextureSize;
extern int					MaxTextureSize;
extern skin_t				*Skins;
extern int					NumSkins;

#endif