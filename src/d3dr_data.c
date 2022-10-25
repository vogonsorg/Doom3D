//d3dr_data.c

#include <malloc.h>

#include "doomdef.h"
#include "d3dr_local.h"
#include "t_bsp.h"
#include "t_data.h"
#include "d3di_ddraw.h"
#include "d3di_video.h"
#include "m_swap.h"
#include "m_fixed.h"
#include "d3dv_video.h"

#include "c_d3d.h"

int 	numflats;
int		*texturetranslation;

texture_t				**textures;
LPDIRECT3DTEXTURE2		*FlatTextures;
LPDIRECT3DTEXTURE2		*SpriteTextures[MAXPLAYERS];
skin_t					*Skins;
int						*FlatCounts;
int						*SpriteCounts[MAXPLAYERS];
int						numtextures;
int						NumSkins;

fixed_t			*textureheight;
int				*flattranslation;

int				firstflat;
int				numflats;
int				lastflat;

int				firstspritelump;
int				lastspritelump;
int				numspritelumps;

float			*spritewidth;
float			*spriteoffset;
float			*spritetopoffset;
float			*spriteheight;
int				TextureMip;
int				SpriteMip;
int				MinTextureSize;
int				MaxTextureSize;
dboolean		SquareTextures;
int				TextureCount=0;

void R_DumpTextures(void)
{
	int		i;
	int		p;

	for (i=0;i<numtextures;i++)
		SAFE_RELEASE(textures[i]->ptex);

	for (i=0;i<numflats;i++)
		SAFE_RELEASE(FlatTextures[i]);

	for (p=0;p<MAXPLAYERS;p++)
	{
		for (i=0;i<numspritelumps;i++)
			SAFE_RELEASE(SpriteTextures[p][i]);
	}

	for (i=0;i<NumSkins;i++)
		SAFE_RELEASE(Skins[i].ptex);

	TextureCount=0;
}

#define DUMP_THRESHOLD 35//will early out when finds a texture older than this

LPDIRECTDRAWSURFACE4 R_CreateNewTexture(LPDDSURFACEDESC2 ddsd)
{
	LPDIRECTDRAWSURFACE4	dds;
	LPDIRECT3DTEXTURE2		*pold;
	HRESULT		hres;
	int			oldcount;
	int			i;
	int			p;

	while (true)
	{
		hres=pDD->lpVtbl->CreateSurface(pDD, ddsd, &dds, NULL);
		if (hres==DD_OK)
			return(dds);
		if (hres!=DDERR_OUTOFVIDEOMEMORY)
			I_Error("CreateSurface(texture) Failed");

		pold=NULL;
		oldcount=0;
		for (i=0;i<numspritelumps;i++)
		{
			if (SpriteTextures[0][i]&&(TextureCount-SpriteCounts[0][i]>oldcount))
			{
				pold=&SpriteTextures[0][i];
				oldcount=TextureCount-SpriteCounts[0][i];
				if (oldcount>=DUMP_THRESHOLD)
					break;
			}
		}
		if ((oldcount>1)&&(oldcount<DUMP_THRESHOLD))
		{
			for (i=0;i<numtextures;i++)
			{
				if ((textures[i]->ptex)&&(TextureCount-textures[i]->count>oldcount))
				{
					pold=&textures[i]->ptex;
					oldcount=TextureCount-textures[i]->count;
					if (oldcount>=DUMP_THRESHOLD)
						break;
				}
			}
		}
		if ((oldcount>1)&&(oldcount<DUMP_THRESHOLD))
		{
			for (i=0;i<numflats;i++)
			{
				if (FlatTextures[i]&&(TextureCount-FlatCounts[i]>oldcount))
				{
					pold=&FlatTextures[i];
					oldcount=TextureCount-FlatCounts[i];
					if (oldcount>=DUMP_THRESHOLD)
						break;
				}
			}
		}
		if ((oldcount>1)&&(oldcount<DUMP_THRESHOLD))
		{
			for (i=0;i<NumSkins;i++)
			{
				if (Skins[i].ptex&&(TextureCount-Skins[i].count>oldcount))
				{
					pold=&Skins[i].ptex;
					oldcount=TextureCount-Skins[i].count;
					if (oldcount>35)
						break;
				}
			}
		}
		if ((oldcount>1)&&(oldcount<DUMP_THRESHOLD))
		{
			for (p=1;p<MAXPLAYERS;p++)
			{
				for (i=0;i<numspritelumps;i++)
				{
					if (SpriteTextures[p][i]&&(TextureCount-SpriteCounts[p][i]>oldcount))
					{
						pold=&SpriteTextures[p][i];
						oldcount=TextureCount-SpriteCounts[p][i];
						if (oldcount>=DUMP_THRESHOLD)
							break;
					}
				}
			}
		}
		if (pold)
		{
			SAFE_RELEASE(*pold);
		}
		else
		{
			I_Printf("Texture Memory Full\n");
			return(NULL);
		}
	}
}


void R_InitTextures(void)
{
    int				*patchlookup;
    int*			maptex;
    int*			maptex1;
    int*			maptex2;
    int				maxoff;
    int				maxoff2;
    int				numtextures1;
    int				numtextures2;
    int*			directory;
    maptexture_t	*mtexture;
    texture_t		*texture;
    char			*name_p;
    char			name[9];
    char			*names;
    int				nummappatches;
    int				i;
    int				offset;
    texpatch_t		*tpatch;
    mappatch_t		*mpatch;
    int				j;

    name[8] = 0;
    names = W_CacheLumpName ("PNAMES", PU_STATIC);
    nummappatches = LONG (*((int *)names));
    name_p = names+4;
    patchlookup = Z_Malloc(nummappatches*sizeof(*patchlookup), PU_STATIC, NULL);

    for (i=0 ; i<nummappatches ; i++)
    {
		strncpy (name,name_p+i*8, 8);
		patchlookup[i] = W_CheckNumForName (name);
    }
    Z_Free (names);

    // Load the map texture definitions from textures.lmp.
    // The data is contained in one or two lumps,
    //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
    maptex1=maptex = W_CacheLumpName ("TEXTURE1", PU_STATIC);
    numtextures1 = LONG(*maptex);
    maxoff = W_LumpLength (W_GetNumForName ("TEXTURE1"));
    directory = maptex+1;

    if (W_CheckNumForName ("TEXTURE2") != -1)
    {
		maptex2 = W_CacheLumpName ("TEXTURE2", PU_STATIC);
		numtextures2 = LONG(*maptex2);
		maxoff2 = W_LumpLength (W_GetNumForName ("TEXTURE2"));
    }
    else
    {
		maptex2 = NULL;
		numtextures2 = 0;
		maxoff2 = 0;
    }
    numtextures = numtextures1 + numtextures2;

    textures=Z_Malloc(numtextures*sizeof(texture_t *), PU_STATIC, NULL);
    //texturewidthmask = Z_Malloc (numtextures*4, PU_STATIC, 0);
    *D_API.ptextureheight=textureheight = Z_Malloc (numtextures*sizeof(int *), PU_STATIC, 0);

    for (i=0;i<numtextures;i++, directory++)
    {
		if (!(i&63))
			I_Printf(".");
		if (i == numtextures1)
		{
			// Start looking in second texture file.
			maptex = maptex2;
			maxoff = maxoff2;
			directory = maptex+1;
		}

		offset = LONG(*directory);
		if (offset > maxoff)
			I_Error ("R_InitTextures: bad texture directory");
		mtexture = (maptexture_t *) ( (byte *)maptex + offset);
		texture=textures[i]=Z_Malloc(sizeof(texture_t)+sizeof(texpatch_t)*(SHORT(mtexture->patchcount)-1), PU_STATIC, NULL);
		texture->width=SHORT(mtexture->width);
		texture->height=SHORT(mtexture->height);
		texture->numpatches=SHORT(mtexture->patchcount);
		memcpy(texture->name, mtexture->name, 8);
		tpatch=texture->patches;
		mpatch=mtexture->patches;
		for (j=0;j<texture->numpatches;j++, mpatch++, tpatch++)
		{
			tpatch->originx=SHORT(mpatch->originx);
			tpatch->originy=SHORT(mpatch->originy);
			tpatch->patch=patchlookup[SHORT(mpatch->patch)];
			if (tpatch->patch == -1)
			{
				I_Error ("R_InitTextures: Missing patch in texture %s",
					texture->name);
			}
		}
		textureheight[i]=INT2F(texture->height);
		texture->ptex=NULL;
    }
    Z_Free(maptex1);
    if (maptex2)
		Z_Free(maptex2);

    Z_Free(patchlookup);
    *D_API.ptexturetranslation=texturetranslation = Z_Malloc ((numtextures+1)*4, PU_STATIC, 0);

    for (i=0 ; i<numtextures ; i++)
		texturetranslation[i] = i;
}

//
// R_InitFlats
//
void R_InitFlats (void)
{
    int		i;

    firstflat = W_GetNumForName ("F_START") + 1;
    lastflat = W_GetNumForName ("F_END") - 1;
    numflats = lastflat - firstflat + 1;

    // Create translation table for global animation.
    *D_API.pflattranslation=flattranslation = Z_Malloc ((numflats+1)*4, PU_STATIC, 0);
    FlatTextures=Z_Malloc(sizeof(LPDIRECT3DTEXTURE2)*numflats, PU_STATIC, NULL);
    FlatCounts=(int *)Z_Malloc(sizeof(int)*numflats, PU_STATIC, NULL);

    for (i=0 ; i<numflats ; i++)
    {
		flattranslation[i] = i;
		FlatTextures[i]=NULL;
    }
}

void R_InitSpriteLumps(void)
{
	int		i;
	patch_t	*patch;
	int		p;

    firstspritelump = W_GetNumForName ("S_START") + 1;
    lastspritelump = W_GetNumForName ("S_END") - 1;

    numspritelumps = lastspritelump - firstspritelump + 1;
    spritewidth = (float *)Z_Malloc (numspritelumps*sizeof(float), PU_STATIC, 0);
    spriteoffset = (float *)Z_Malloc (numspritelumps*sizeof(float), PU_STATIC, 0);
    spritetopoffset = (float *)Z_Malloc (numspritelumps*sizeof(float), PU_STATIC, 0);
    spriteheight = (float *)Z_Malloc (numspritelumps*sizeof(float), PU_STATIC, 0);

	for (p=0;p<MAXPLAYERS;p++)
	{
		SpriteTextures[p]=(LPDIRECT3DTEXTURE2 *)Z_Malloc(numspritelumps*sizeof(LPDIRECT3DTEXTURE2), PU_STATIC, NULL);
		SpriteCounts[p]=(int *)Z_Malloc(numspritelumps*sizeof(int), PU_STATIC, NULL);
	}
	for (i=0;i<numspritelumps;i++)
	{
		if (!(i&63))
			I_Printf(".");
		for (p=0;p<MAXPLAYERS;p++)
			SpriteTextures[p][i]=NULL;
		patch = W_CacheLumpNum (firstspritelump+i, PU_CACHE);
		spritewidth[i] = (float)SHORT(patch->width);
		spriteoffset[i] = (float)SHORT(patch->leftoffset);
		spritetopoffset[i] = (float)SHORT(patch->topoffset);
		spriteheight[i]=(float)SHORT(patch->height);
	}
}

void R_InitData(void)
{
    R_InitTextures();
    R_InitFlats();
	R_InitSpriteLumps();
}

//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName (char* name)
{
    int		i;
    char	namet[9];

    i = W_CheckNumForName (name);

    if (i == -1)
    {
		namet[8] = 0;
		memcpy (namet, name,8);
		I_Error ("R_FlatNumForName: %s not found",namet);
    }
    return i - firstflat;
}

void R_PrecacheLevelQuick(void)
{
	R_DumpTextures();
}

void R_PrecacheLevel(void)
{
    char			*texturepresent;
//    char			*spritepresent;
    char			*flatpresent;
//    thinker_t		*th;
//    spriteframe_t	*sf;

	int			i;
//	int			j;
//	int			k;

	R_DumpTextures();

    flatpresent = alloca(numflats);
    memset (flatpresent,0,numflats);

    for (i=0 ; i<numsectors ; i++)
    {
		flatpresent[sectors[i].floorpic] = 1;
		flatpresent[sectors[i].ceilingpic] = 1;
    }

    for (i=0 ; i<numflats ; i++)
    {
		if (flatpresent[i])
		{
			R_LoadFlat(i);
		}
    }

    texturepresent = alloca(numtextures);
    memset (texturepresent,0, numtextures);

    for (i=0 ; i<numsides ; i++)
    {
		texturepresent[sides[i].toptexture] = 1;
		texturepresent[sides[i].midtexture] = 1;
		texturepresent[sides[i].bottomtexture] = 1;
    }
    for (i=0;i<numtextures;i++)
    {
		if (texturepresent[i])
    		R_LoadTexture(textures[i]);
	}

//FUDGE: only precache sprites that aren't replaced by md2 models
//FUDGE: precache skins where neccy
#if 0
    spritepresent = alloca(numsprites);
    memset (spritepresent,0, numsprites);

    for (th = D_API.thinkercap->next ; th != D_API.thinkercap ; th=th->next)
    {
		if (th->function.acp1 == (actionf_p1)D_API.P_MobjThinker)
			spritepresent[((mobj_t *)th)->sprite] = 1;
    }

    for (i=0 ; i<numsprites ; i++)
    {
		if (!spritepresent[i])
			continue;

		for (j=0 ; j<sprites[i].numframes ; j++)
		{
			sf = &sprites[i].spriteframes[j];
			for (k=0 ; k<8 ; k++)
			{
				R_LoadSprite(sf->lump[k]);
			}
		}
    }
#endif
}

//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
int	R_CheckTextureNumForName (char *name)
{
    int		i;

    // "NoTexture" marker.
    if (name[0] == '-')
		return 0;

    for (i=0 ; i<numtextures ; i++)
		if (!strnicmp (textures[i]->name, name, 8) )//strncasecmp
			return i;

		return -1;
}



//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//  aborts with error message.
//
int	R_TextureNumForName (char* name)
{
    int		i;

    i = R_CheckTextureNumForName (name);

    if (i==-1)
    {
		I_Error ("R_TextureNumForName: %s not found",
			name);
    }
    return i;
}

dboolean R_CopyTexture(texture_t *texture, word *data, word *palette)
{
    word			*dest;
    word			*desttop;
    byte			*src;
    int				x;
    int				x1;
    int				x2;
    int				pos;
    int				count;
    int				pitch;
    patch_t			*patch;
    column_t		*column;
    texpatch_t		*tpatch;

	for (count=0;count<texture->width*texture->height;count++)
		data[count]=TextureClearColor;
    pitch=texture->width;
    for (tpatch=texture->patches;tpatch<&texture->patches[texture->numpatches];tpatch++)
    {
		patch=W_CacheLumpNum(tpatch->patch, PU_CACHE);
		x1=tpatch->originx;
		x2=x1+SHORT(patch->width);
		if (x1<0)
			x=0;
		else
			x=x1;
		if (x2>texture->width)
			x2=texture->width;
		desttop=data+x;
		for (;x<x2;x++, desttop++)
		{
			column=(column_t *)((byte *)patch+LONG(patch->columnofs[x-x1]));
			while (column->topdelta!=0xff)
			{
				src=(byte *)column+3;
				pos=tpatch->originy+column->topdelta;
				count=column->length;
				if (pos<0)
				{
					count+=pos;
					src-=pos;
					pos=0;
				}
				if (pos+count>texture->height)
					count=texture->height-pos;
				dest=desttop+pos*pitch;
				if (count>0)
				{
					while (count--)
					{
						*dest=palette[*(src++)];
						dest+=pitch;
					}
				}
				column=(column_t *)((byte *)column+column->length+4);
			}
		}
    }

	for (count=0;count<texture->width*texture->height;count++)
	{
		if (data[count]==TextureClearColor)
			return(true);
	}
	return(false);
}

void R_FillTexture(LPDIRECTDRAWSURFACE4 pdds, int destwidth, int destheight, word *data, int srcwidth, int srcheight)
{
    word			*dest;
    word			*src;
    DDSURFACEDESC2	ddsd;
    HRESULT			hres;
    int				x;
    int				y;
    int				pitch;
    fixed_t			xfrac;
    fixed_t			yfrac;
    fixed_t			xstep;
    fixed_t			ystep;

    ddsd.dwSize=sizeof(DDSURFACEDESC2);
    hres=pdds->lpVtbl->Lock(pdds, NULL, &ddsd, DDLOCK_WAIT|DDLOCK_WRITEONLY, NULL);
    if (hres!=DD_OK)
		I_Error("Lock(texture) Falied");
    xstep=FixedDiv(INT2F(srcwidth), INT2F(destwidth));
    ystep=FixedDiv(INT2F(srcheight), INT2F(destheight));
    dest=ddsd.lpSurface;
    pitch=ddsd.lPitch-destwidth*2;
    for (y=0, yfrac=0;y<destheight;y++, yfrac+=ystep)
    {
		src=&data[F2INT(yfrac)*srcwidth];
		for (x=0, xfrac=0;x<destwidth;x++, xfrac+=xstep)
			*(dest++)=src[F2INT(xfrac)];
		((byte *)dest)+=pitch;
    }
    pdds->lpVtbl->Unlock(pdds, NULL);
}

int R_RoundPow2(int n, int mip)
{
    int		mask;

	if (!n)
		n=1;
	if (mip<0)
		return(n);
    mask=0x40000000;
    while ((mask&n)==0)
		mask>>=1;
    if (mask==n)
		mip++;
	mask<<=1;
	if (mip>0)
		mask>>=mip;
    if (mask>MaxTextureSize)
		mask=MaxTextureSize;
	if (mask<MinTextureSize)
		mask=MinTextureSize;
    return(mask);
}

void R_LoadTexture(texture_t *texture)
{
    DDSURFACEDESC2	ddsd;
    HRESULT			hres;
    word			*data;
	word			*palette;
	dboolean		transparent;
	LPDIRECTDRAWSURFACE4	dds;

    if (texture->ptex)
		return;

    data=(word *)Z_Malloc(texture->width*texture->height*2, PU_STATIC, NULL);
    ZeroMemory(data, texture->width*texture->height);
	if (UseColorKey)
		palette=TexturePal[0];
	else
		palette=AlphaTexturePal[0];

	transparent=R_CopyTexture(texture, data, palette);
	if (!(transparent||UseColorKey))
		R_CopyTexture(texture, data, TexturePal[0]);
    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
	ddsd.dwFlags=DDSD_CAPS|DDSD_PIXELFORMAT|DDSD_WIDTH|DDSD_HEIGHT|DDSD_TEXTURESTAGE;
    ddsd.dwWidth=R_RoundPow2(texture->width, TextureMip);
    ddsd.dwHeight=R_RoundPow2(texture->height, TextureMip);
	if (SquareTextures)
	{
		if (ddsd.dwHeight<ddsd.dwWidth)
			ddsd.dwHeight=ddsd.dwWidth;
		if (ddsd.dwHeight>ddsd.dwWidth)
			ddsd.dwWidth=ddsd.dwHeight;
	}
#ifdef SOFTWARE_DEVICE
    ddsd.ddsCaps.dwCaps=DDSCAPS_TEXTURE;
#else
    ddsd.ddsCaps.dwCaps=DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY;
#endif
    ddsd.dwTextureStage=0;
    memcpy(&ddsd.ddpfPixelFormat, &TexturePixelFormat, sizeof(DDPIXELFORMAT));
	if (transparent)
	{
		if (UseColorKey)
		{
			ddsd.dwFlags|=DDSD_CKSRCBLT;
			ddsd.ddckCKSrcBlt.dwColorSpaceLowValue=TextureClearColor;
			ddsd.ddckCKSrcBlt.dwColorSpaceHighValue=TextureClearColor;
		}
		else
		{
		    memcpy(&ddsd.ddpfPixelFormat, &AlphaTexturePixelFormat, sizeof(DDPIXELFORMAT));
		}
	}
	dds=R_CreateNewTexture(&ddsd);
	if (!dds)
	{
		Z_Free(data);
		return;
	}
	R_FillTexture(dds, ddsd.dwWidth, ddsd.dwHeight, data, texture->width, texture->height);
    Z_Free(data);
    hres=dds->lpVtbl->QueryInterface(dds, &IID_IDirect3DTexture2, (void **)&texture->ptex);
    if (hres!=DD_OK)
		I_Error("Flat::QueryInterface(IDirect3DTexture2) Failed");
	dds->lpVtbl->Release(dds);
}

void R_CopyFlat8(int flatnum, LPDDSURFACEDESC2 pddsd)
{
    byte	*src;
    byte	*dest;
    int		y;

    dest=(byte *)pddsd->lpSurface;
    src=W_CacheLumpNum(firstflat+flatnum, PU_CACHE);
    for (y=0;y<64;y++)
    {
		memcpy(dest, src, 64);
		src+=64;
		dest+=pddsd->lPitch;
    }
}

void R_CopyFlat16(int flatnum, LPDDSURFACEDESC2 pddsd)
{
    byte	*src;
    word	*dest;
    int		pitch;
    int		x;
    int		y;
    int		inc;
	word	*palette;

	palette=TexturePal[0];
    dest=(word *)pddsd->lpSurface;
    pitch=pddsd->lPitch-pddsd->dwWidth*2;
    src=W_CacheLumpNum(firstflat+flatnum, PU_CACHE);
    if (W_LumpLength(firstflat+flatnum)!=4096)
		I_Error("Invalid Flat size: %d", W_LumpLength(firstflat+flatnum));
	inc=64/pddsd->dwWidth;
    for (x=0;x<64;x+=inc)
    {
		for (y=0;y<64;y+=inc)
		{
			*(dest++)=palette[*src];
			src+=inc;
		}
		src+=64*(inc-1);
		((byte *)dest)+=pitch;
    }
}

void R_LoadFlat(int flatnum)
{
    DDSURFACEDESC2			ddsd;
    HRESULT					hres;
    LPDIRECTDRAWSURFACE4	dds;
    int						size;

    if (FlatTextures[flatnum])
		return;
    ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
    ddsd.dwSize=sizeof(DDSURFACEDESC2);
    ddsd.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT|DDSD_TEXTURESTAGE;
    ddsd.dwTextureStage=0;
    memcpy(&ddsd.ddpfPixelFormat, &TexturePixelFormat, sizeof(DDPIXELFORMAT));
    size=64;
	if (TextureMip>1)
		size>>=(TextureMip-1);
	if (size<MinTextureSize)
		size=MinTextureSize;
	ddsd.dwHeight=ddsd.dwWidth=size;
#ifdef SOFTWARE_DEVICE
    ddsd.ddsCaps.dwCaps=DDSCAPS_TEXTURE;
#else
    ddsd.ddsCaps.dwCaps=DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY;
#endif
	dds=R_CreateNewTexture(&ddsd);
	if (!dds)
		return;
    hres=dds->lpVtbl->Lock(dds, NULL, &ddsd, DDLOCK_WAIT|DDLOCK_WRITEONLY, NULL);
    if (hres!=DD_OK)
		I_Error("Lock(Flat) Failed");
//FUDGE:8bt
#if 0
    if (pDDPTexture)
    {
		hres=dds->lpVtbl->SetPalette(dds, pDDPTexture);
		if (hres!=DD_OK)
			I_Error("Flat::SetPalette Failed");
		R_CopyFlat8(flatnum, &ddsd);
    }
    else
#endif

	ddsd.dwWidth=size;
	R_CopyFlat16(flatnum, &ddsd);
    dds->lpVtbl->Unlock(dds, NULL);
    hres=dds->lpVtbl->QueryInterface(dds, &IID_IDirect3DTexture2, (void **)&FlatTextures[flatnum]);
    if (hres!=DD_OK)
		I_Error("Flat::QueryInterface(IDirect3DTexture2) Failed");
	dds->lpVtbl->Release(dds);
}

void R_CopySprite(patch_t *patch, word *data, word *palette)
{
	int			pitch;
	int			count;
	int			x;
	int			x2;
	word		*desttop;
	column_t	*column;
	byte		*src;
	word		*dest;

	for (count=0;count<patch->width*patch->height;count++)
		data[count]=TextureClearColor;

	x2=pitch=SHORT(patch->width);
	desttop=data;
	for (x=0;x<x2;x++, desttop++)
	{
		column=(column_t *)((byte *)patch+LONG(patch->columnofs[x]));
		while (column->topdelta!=0xff)
		{
			src=(byte *)column+3;
			count=column->length;
			dest=desttop+column->topdelta*pitch;
			if (count>0)
			{
				while (count--)
				{
					*dest=palette[*(src++)];
					dest+=pitch;
				}
			}
			column=(column_t *)((byte *)column+column->length+4);
		}
	}
}

void R_LoadSprite(int spritenum, int mip, int trans)
{
	patch_t					*patch;
	int						width;
	int						height;
	word					*data;
	DDSURFACEDESC2			ddsd;
	HRESULT					hres;
	LPDIRECTDRAWSURFACE4	pdds;
	word					*palette;

	if (SpriteTextures[trans][spritenum])
		return;
	patch=W_CacheLumpNum(spritenum+firstspritelump, PU_STATIC);
	width=SHORT(patch->width);
	height=SHORT(patch->height);
    data=(word *)Z_Malloc(width*height*2, PU_STATIC, NULL);
	if (UseColorKey)
		palette=TexturePal[trans];
	else
		palette=AlphaTexturePal[trans];
    R_CopySprite(patch, data, palette);
	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize=sizeof(DDSURFACEDESC2);
    ddsd.dwFlags=DDSD_CAPS|DDSD_PIXELFORMAT|DDSD_WIDTH|DDSD_HEIGHT|DDSD_TEXTURESTAGE;
    ddsd.dwWidth=R_RoundPow2(width, mip);
    ddsd.dwHeight=R_RoundPow2(height, mip);
	if (SquareTextures)
	{
		if (ddsd.dwHeight<ddsd.dwWidth)
			ddsd.dwHeight=ddsd.dwWidth;
		if (ddsd.dwHeight>ddsd.dwWidth)
			ddsd.dwWidth=ddsd.dwHeight;
	}
#ifdef SOFTWARE_DEVICE
    ddsd.ddsCaps.dwCaps=DDSCAPS_TEXTURE;
#else
    ddsd.ddsCaps.dwCaps=DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY;
#endif
    ddsd.dwTextureStage=0;
	if (UseColorKey)
	{
		ddsd.dwFlags|=DDSD_CKSRCBLT;
		ddsd.ddckCKSrcBlt.dwColorSpaceLowValue=TextureClearColor;
		ddsd.ddckCKSrcBlt.dwColorSpaceHighValue=TextureClearColor;
		memcpy(&ddsd.ddpfPixelFormat, &TexturePixelFormat, sizeof(DDPIXELFORMAT));
	}
	else
	{
		memcpy(&ddsd.ddpfPixelFormat, &AlphaTexturePixelFormat, sizeof(DDPIXELFORMAT));
	}
	pdds=R_CreateNewTexture(&ddsd);
	if (!pdds)
	{
		Z_Free(data);
		Z_Free(patch);
		return;
	}
	R_FillTexture(pdds, ddsd.dwWidth, ddsd.dwHeight, data, width, height);
	Z_Free(data);
	Z_Free(patch);
    hres=pdds->lpVtbl->QueryInterface(pdds, &IID_IDirect3DTexture2, (void **)&SpriteTextures[trans][spritenum]);
    if (hres!=DD_OK)
		I_Error("Sprite::QueryInterface(IDirect3DTexture2) Failed");
	pdds->lpVtbl->Release(pdds);
}

void R_ReadPCXData(byte *data, int size, FILE *fh)
{
	byte	b;
	byte	*p;
	int		i;

	p=data;
	i=0;
	while (i<size)
	{
		R_SafeRead(&b, 1, fh);
		if (b>0xC0)
		{
			b=b-0xC0;
			R_SafeRead(p, 1, fh);
			i+=b;
			while (--b>0)
			{
				p[1]=p[0];
				p++;
			}
			p++;
		}
		else
		{
			*(p++)=b;
			i++;
		}
	}
}

void R_LoadSkin(skin_t *skin, int mip)
{
	LPDIRECTDRAWSURFACE4	pdds;
	DDSURFACEDESC2			ddsd;
	pcxheader_t	header;
	FILE		*fh;
	word		*data;
	byte		*pcxdata;
	word		*dest;
	byte		*src;
	int			x;
	int			y;
	int			width;
	int			height;
	HRESULT		hres;

	fh=fopen(skin->name, "rb");
	if (!fh)
		I_Error("Unable to open %s", skin->name);
	R_SafeRead(&header, sizeof(header), fh);
	if ((header.id!=0x0A)||(header.ver!=5))
		I_Error("Invalid PCX:%s", skin->name);

	width=header.x1+1-header.x0;
	height=header.y1+1-header.y0;
	data=(word *)Z_Malloc(width*height*2, PU_STATIC, NULL);
	pcxdata=(byte *)Z_Malloc(header.linelen*height, PU_STATIC, NULL);
	ZeroMemory(pcxdata, header.linelen*height);
	R_ReadPCXData(pcxdata, header.linelen*height, fh);
	fclose(fh);
	src=pcxdata;
	dest=data;
	for (y=0;y<height;y++)
	{
		for (x=0;x<width;x++)
		{
			*(dest++)=TexturePal[0][*(src++)];
		}
		src+=header.linelen-width;
	}
	Z_Free(pcxdata);

	ZeroMemory(&ddsd, sizeof(DDSURFACEDESC2));
	ddsd.dwSize=sizeof(DDSURFACEDESC2);
    ddsd.dwFlags=DDSD_CAPS|DDSD_PIXELFORMAT|DDSD_WIDTH|DDSD_HEIGHT|DDSD_TEXTURESTAGE;
    ddsd.dwWidth=R_RoundPow2(width, mip);
    ddsd.dwHeight=R_RoundPow2(height, mip);
	if (SquareTextures)
	{
		if (ddsd.dwHeight<ddsd.dwWidth)
			ddsd.dwHeight=ddsd.dwWidth;
		if (ddsd.dwHeight>ddsd.dwWidth)
			ddsd.dwWidth=ddsd.dwHeight;
	}

#ifdef SOFTWARE_DEVICE
    ddsd.ddsCaps.dwCaps=DDSCAPS_TEXTURE;
#else
    ddsd.ddsCaps.dwCaps=DDSCAPS_TEXTURE|DDSCAPS_VIDEOMEMORY;
#endif
    ddsd.dwTextureStage=0;
	memcpy(&ddsd.ddpfPixelFormat, &TexturePixelFormat, sizeof(DDPIXELFORMAT));
	pdds=R_CreateNewTexture(&ddsd);
	if (!pdds)
	{
		Z_Free(data);
		return;
	}
	R_FillTexture(pdds, ddsd.dwWidth, ddsd.dwHeight, data, width, height);
	Z_Free(data);
    hres=pdds->lpVtbl->QueryInterface(pdds, &IID_IDirect3DTexture2, (void **)&skin->ptex);
    if (hres!=DD_OK)
		I_Error("Flat::QueryInterface(IDirect3DTexture2) Failed");
	pdds->lpVtbl->Release(pdds);
}

static int CheckTextureSize(int size)
{
	int			mask;

	if (size<1)
		size=1;
	mask=0x40000000;
	while ((mask&size)==0)
		mask>>=1;
	return(mask);
}

void R_CheckTextureSizeLimits(void)
{
	MinTextureSize=CheckTextureSize(MinTextureSize);
	MaxTextureSize=CheckTextureSize(MaxTextureSize);
}
