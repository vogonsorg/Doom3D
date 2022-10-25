//d3dr_draw.c

#include "d3dr_local.h"
#include "d3di_ddraw.h"
#include "d3di_video.h"
#include "i_gamma.h"
#include "t_colorbias.h"
#include "m_fixed.h"

#include "c_d3d.h"

#define NUM_RED_LIGHTMAPS	8
#define NUM_GOLD_LIGHTMAPS	4
#define NUM_GREEN_LIGHTMAPS	1

#define FVF_DOOMTVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)

#define MAKERGBGAMMA(r, g, b, gt) ((((DWORD)gt[r])<<16)|(((DWORD)gt[g])<<8)|gt[b]|0xFF000000)

D3DCOLOR	*Lightmap;

D3DCOLOR	*NormalLightmaps[1]={NULL};
D3DCOLOR	*RedLightmaps[NUM_RED_LIGHTMAPS];
D3DCOLOR	*GoldLightmaps[NUM_GOLD_LIGHTMAPS];
D3DCOLOR	*GreenLightmaps[NUM_GREEN_LIGHTMAPS];

typedef struct
{
	D3DCOLOR	**maps;
	int			nummaps;
}lightmapinfo_t;

lightmapinfo_t	Lightmaps[]=
{
	{NormalLightmaps, 1},
	{RedLightmaps, NUM_RED_LIGHTMAPS},
	{GoldLightmaps, NUM_GOLD_LIGHTMAPS},
	{GreenLightmaps, NUM_GREEN_LIGHTMAPS}
};

typedef struct
{
	D3DVALUE	x;
	D3DVALUE	y;
	D3DVALUE	z;
	D3DVALUE	rhw;
	D3DCOLOR	color;
	D3DVALUE	tu;
	D3DVALUE	tv;
}doomtvertex_t;

DOOM3DVERTEX	WallVertices[5];
DOOM3DVERTEX	*SSectorVertices=NULL;
dboolean		MD2SpriteMatrix=false;
dboolean		DisableMD2Weapons;

//note curskin, curflat, curtexture and cursprite are mutually exclusive
int			curskin;
int			curtexture;
int			curflat;
int			cursprite;
int			curtrans;

void R_StartTextureFrame(void)
{
	TextureCount++;
	curtexture=curflat=cursprite=curskin=-1;
}

//FUDGE: something wrong with my lighting theory...
int R_GetLight(int dist, int level)
{
	int		light;
	int		min;

	min=level>>2;
	if (dist>255)
		dist=255;
	light=level*2-dist;
	if (light<min)
		light=min;
	if (light>255)
		light=255;
	return(light);
}

void R_ClearTexture(void)
{
    HRESULT	hres;

    hres=pD3DDevice->lpVtbl->SetTexture(pD3DDevice, 0, NULL);
    if (hres!=D3D_OK)
		I_Error("SetTexture Failed");
}

void R_ClearView(void)
{
    D3DRECT	r;

    r.x1=0;
    r.y1=0;
    r.x2=ScreenWidth;//clipped to viewport rectangle
    r.y2=ScreenHeight;
	//not checking return value 'cos of bugs on my PowerVR card
    pD3DVPCurrent->lpVtbl->Clear2(pD3DVPCurrent, 1, &r, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
}

void R_BeginScene(void)
{
    HRESULT	hres;

    hres=pD3DDevice->lpVtbl->BeginScene(pD3DDevice);
    if (hres!=D3D_OK)
		I_Error("BeginScene Failed");
}

void R_EndScene(void)
{
    HRESULT	hres;

    hres=pD3DDevice->lpVtbl->EndScene(pD3DDevice);
    if (hres!=D3D_OK)
		I_Error("EndScene Failed");
}

void R_DrawWall(void)
{
    pD3DDevice->lpVtbl->DrawPrimitive(pD3DDevice, D3DPT_TRIANGLESTRIP, FVF_DOOM3DVERTEX, WallVertices, 4, D3DDP_DONOTLIGHT|D3DDP_DONOTUPDATEEXTENTS);
}

void R_SetColorBias(int type, int strength)
{
	lightmapinfo_t	*info;

	if (strength>255)
		strength=255;
	info=&Lightmaps[type];
	Lightmap=info->maps[(strength*info->nummaps)/256];
}

void R_RecalcLightmaps(void)
{
	D3DCOLOR	*color;
	int			i;
	int			lmap;
	int			j;
	int			high;
	int			low;
	byte		*gtable;

	gtable=gammatable[ManualGamma];
	color=NormalLightmaps[0];
	if (!color)
		return;
	for (i=0;i<256;i++)
		*(color++)=MAKERGBGAMMA(i, i, i, gtable);
	for (lmap=0;lmap<NUM_RED_LIGHTMAPS;lmap++)
	{
		color=RedLightmaps[lmap];
		for (i=0;i<256;i++)
		{
			j=((lmap+1)*92)/NUM_RED_LIGHTMAPS;
			high=i+j;
			low=i-j;
			if (high>=255)
			{
				low+=255-high;
				high=255;
			}
			if (low<0)
				low=0;
			*(color++)=MAKERGBGAMMA(high, low, low, gtable);
		}
	}
	for (lmap=0;lmap<NUM_GOLD_LIGHTMAPS;lmap++)
	{
		color=GoldLightmaps[lmap];
		for (i=0;i<256;i++)
		{
			j=((lmap+1)*64)/NUM_GOLD_LIGHTMAPS;
			high=i+j;
			low=i-j;
			if (high>=255)
			{
				low+=255-high;
				high=255;
			}
			if (low<0)
				low=0;
			*(color++)=MAKERGBGAMMA(high, high, low, gtable);
		}
	}
	for (lmap=0;lmap<NUM_GREEN_LIGHTMAPS;lmap++)
	{
		color=GreenLightmaps[lmap];
		for (i=0;i<256;i++)
		{
			j=((lmap+1)*48)/NUM_GREEN_LIGHTMAPS;
			high=i+j;
			low=i-j;
			if (high>=255)
			{
				low+=255-high;
				high=255;
			}
			if (low<0)
				low=0;
			*(color++)=MAKERGBGAMMA(low, high, low, gtable);
		}
	}
}

void R_InitLightmaps(void)
{
	int			i;

	NormalLightmaps[0]=Z_Malloc(256*sizeof(D3DCOLOR), PU_STATIC, NULL);

	for (i=0;i<NUM_RED_LIGHTMAPS;i++)
		RedLightmaps[i]=Z_Malloc(256*sizeof(D3DCOLOR), PU_STATIC, NULL);

	for (i=0;i<NUM_GOLD_LIGHTMAPS;i++)
		GoldLightmaps[i]=Z_Malloc(256*sizeof(D3DCOLOR), PU_STATIC, NULL);
	for (i=0;i<NUM_GREEN_LIGHTMAPS;i++)
		GreenLightmaps[i]=Z_Malloc(256*sizeof(D3DCOLOR), PU_STATIC, NULL);
	R_RecalcLightmaps();
}

void R_InitDraw(void)
{
//    D3DVERTEXBUFFERDESC	vbd;
//    HRESULT		hres;

    curtexture=-1;
    curflat=-1;
	cursprite=-1;
	curskin=-1;

/*
    ZeroMemory(&vbd, sizeof(D3DVERTEXBUFFERDESC));
    vbd.dwSize=sizeof(D3DVERTEXBUFFERDESC);
    vbd.dwCaps=0;
    vbd.dwFVF=FVF_DOOM3DVERTEX;
    vbd.dwNumVertices=4;
    hres=pD3D->lpVtbl->CreateVertexBuffer(pD3D, &vbd, &WallVB, 0, NULL);
    if (hres!=D3D_OK)
		I_Error("CreateVertexBuffer(Wall) Failed");
*/
}

extern int numtextures;
extern int numflats;

void R_SelectTexture(int texnum, int *width, int *height)
{
    texture_t	*texture;
    HRESULT	hres;

    texnum=texturetranslation[texnum];
    texture=textures[texnum];
    if (width)
		*width=texture->width;
    if (height)
		*height=texture->height;
	texture->count=TextureCount;
    if (curtexture==texnum)
		return;
    if (!texture->ptex)
		R_LoadTexture(texture);
	//FUDGERETURN
    hres=pD3DDevice->lpVtbl->SetTexture(pD3DDevice, 0, texture->ptex);
    if (hres!=D3D_OK)
		I_Error("SetTexture Failed");
    curtexture=texnum;
    curflat=-1;
	cursprite=-1;
	curskin=-1;
}

void R_SelectFlat(int flatnum)
{
    HRESULT	hres;

    flatnum=flattranslation[flatnum];
	FlatCounts[flatnum]=TextureCount;
    if (flatnum==curflat)
		return;
    if (!FlatTextures[flatnum])
		R_LoadFlat(flatnum);
	//FUDGERETURN
    hres=pD3DDevice->lpVtbl->SetTexture(pD3DDevice, 0, FlatTextures[flatnum]);
    if (hres!=D3D_OK)
		I_Error("SetTexture Failed");
    curflat=flatnum;
    curtexture=-1;
	cursprite=-1;
	curskin=-1;
}

void R_SelectSprite(int spritenum, int mip, int trans)
{
	SpriteCounts[trans][spritenum]=TextureCount;
	if ((spritenum==cursprite)&&(trans==curtrans))
		return;
	if (!SpriteTextures[trans][spritenum])
		R_LoadSprite(spritenum, mip, trans);

    pD3DDevice->lpVtbl->SetTexture(pD3DDevice, 0, SpriteTextures[trans][spritenum]);

    cursprite=spritenum;
    curtrans=trans;
    curtexture=-1;
	curflat=-1;
	curskin=-1;
}

void R_DrawSectorPlanes(sector_t *sector)
{
    plane_t			*plane;
    DOOM3DVERTEX	*v;
    DOOM3DVERTEX	*lastpoint;
    D3DVALUE		floorz;
    D3DVALUE		ceilingz;
    HRESULT			hres;
	int				light;
	int				dist;
	int				visible;

    plane=planes[sector-sectors];
    if (!plane->numpoints)
		return;

    floorz=F2D3D(sector->floorheight);
    ceilingz=F2D3D(sector->ceilingheight);
    lastpoint=&plane->points[plane->numpoints];
	visible=plane->numpoints;

    for (v=plane->points;v<lastpoint;v++)
	{
		v->z=floorz;

		if (fullbright)
			light=255;
		else
		{
			dist=(int)((v->x-fviewx)*viewcos+(v->y-fviewy)*viewsin)/2;
			if (dist<=0)
				visible--;
			light=R_GetLight(dist, sector->lightlevel);
		}
		v->color=Lightmap[light];
	}
	if (!visible)
		return;
	if (sector->floorpic!=skyflatnum)
	{
	    R_SelectFlat(sector->floorpic);
		//FUDGERETURN
	    hres=pD3DDevice->lpVtbl->DrawIndexedPrimitive(pD3DDevice, D3DPT_TRIANGLELIST, FVF_DOOM3DVERTEX, plane->points, plane->numpoints, plane->indices, plane->numindices, D3DDP_DONOTLIGHT);
	    if (hres!=D3D_OK)
			I_Error("DrawPrimitive(Floor) Failed");
	}
	if (sector->ceilingpic!=skyflatnum)
	{
		for (v=plane->points;v<lastpoint;v++)
			v->z=ceilingz;
		R_SelectFlat(sector->ceilingpic);
		//FUDGERETURN
		hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
		if (hres!=D3D_OK)
			I_Error("SetCullMode(CW) Failed");
		//FUDGERETURN
		hres=pD3DDevice->lpVtbl->DrawIndexedPrimitive(pD3DDevice, D3DPT_TRIANGLELIST, FVF_DOOM3DVERTEX, plane->points, plane->numpoints, plane->indices, plane->numindices, D3DDP_DONOTLIGHT|D3DDP_DONOTUPDATEEXTENTS);
		if (hres!=D3D_OK)
			I_Error("DrawPrimitive(Ceiling) Failed");
		//FUDGERETURN
		hres=pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
		if (hres!=D3D_OK)
			I_Error("SetCullMode(CCW) Failed");
	}
}

void R_SelectSkin(int skinnum)
{
	HRESULT	hres;
	skin_t	*skin;

	skin=&Skins[skinnum];
	skin->count=TextureCount;
	if (curskin==skinnum)
		return;
	skin=&Skins[skinnum];
	if (!skin->ptex)
		R_LoadSkin(skin, SpriteMip);
    hres=pD3DDevice->lpVtbl->SetTexture(pD3DDevice, 0, skin->ptex);
    if (hres!=D3D_OK)
		I_Error("SetTexture Failed");
	curskin=skinnum;
    curtexture=-1;
    curflat=-1;
	cursprite=-1;
}

//
// R_RenderModel
// Draws MD2 Model
// if model==NULL, works out from state
// if skin==-1, uses default skin
// calls itself to draw weapon (if present)
//
void R_RenderModel(md2state_t *state, int framenum, int light, model_t *model, int skin)
{
	int				i;
	modelvert_t		*v;
	modelframe_t	*frame;
	int				count;
	int				type;
	int				*cmd;
	D3DCOLOR		color;
	DOOM3DVERTEX	*mv;

	if (!model)
		model=state->model;
	if (fullbright||(framenum&FF_FULLBRIGHT))
		color=Lightmap[255];
	else
		color=Lightmap[light];

	i=state->frames[framenum&FF_FRAMEMASK];
	if (i>=model->numframes)
	{
		I_Printf("Invalid model frame:%d\n", i);
		return;
	}
	frame=model->frames[i];
	if (skin==-1)
		R_SelectSkin(model->skin);
	else
		R_SelectSkin(skin);
	cmd=model->cmds;
	count=*(cmd++);
	while (count!=0)
	{
		if (count>0)
			type=D3DPT_TRIANGLESTRIP;
		else
		{
			type=D3DPT_TRIANGLEFAN;
			count=-count;
		}

		if (count>2048)
			I_Error("Too many verts in model\n", count);
		mv=ModelVertexBuffer;
		for (i=0;i<count;i++, mv++)
		{
			mv->tu=*(float *)(cmd++);
			mv->tv=*(float *)(cmd++);
			v=&frame->v[*(cmd++)];
			mv->x=v->x;
			mv->y=v->y;
			mv->z=v->z;
			mv->color=color;
		}
		pD3DDevice->lpVtbl->DrawPrimitive(pD3DDevice, type, FVF_DOOM3DVERTEX, ModelVertexBuffer, count, D3DDP_DONOTLIGHT|D3DDP_DONOTUPDATEEXTENTS);
		count=*(cmd++);
	}
	if (model->weapon)
		R_RenderModel(state, framenum, light, model->weapon, -1);
}

void R_DrawMD2Sprite(mobj_t *thing)
{
	float	x;
	float	y;
	int		skin;

	if (thing->type==MT_PLAYER)
		skin=PlayerSkins[(thing->flags&MF_TRANSLATION)>>MF_TRANSSHIFT].skin;
	else
		skin=-1;

	x=F2D3D(thing->x);
	y=F2D3D(thing->y);
	if (((x-fviewx)*viewcos+(y-fviewy)*viewsin)<0)
		return;
    I_SetViewMatrix(x, y, F2D3D(thing->z), thing->angle);
	MD2SpriteMatrix=true;
	R_RenderModel(thing->state->md2, thing->frame, thing->subsector->sector->lightlevel, NULL, skin);
}

void R_DrawSprite(mobj_t *thing)
{
	float			x;
	float			y;
	float			z;
	spritedef_t		*sprdef;
	spriteframe_t	*sprframe;
	angle_t			ang;
	int				spritenum;
	int				rot;
	float			offs;
	HRESULT			hres;
	int				light;
	int				dist;

//FUDGE: draw md2 models while traversing BSP?
	if (thing->state->md2)
	{
		R_DrawMD2Sprite(thing);
		return;
	}

	if (MD2SpriteMatrix)
	{
		I_SetViewMatrix(0, 0, 0, 0);
		MD2SpriteMatrix=false;
	}
	x=F2D3D(thing->x);
	y=F2D3D(thing->y);

	sprdef=&spriteinfo[thing->sprite];
    sprframe = &sprdef->spriteframes[ thing->frame & FF_FRAMEMASK];

    if (sprframe->rotate)
    {
		// choose a different rotation based on player view
		ang = R_PointToAngle (thing->x-viewx, thing->y-viewy);
		rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
    }
    else
    {
		// use single rotation for all views
		rot=0;
    }
	spritenum = sprframe->lump[rot];
	if (sprframe->flip[rot])
		offs=1.0f;
	else
		offs=0.0f;

	if (fullbright||(thing->frame&FF_FULLBRIGHT))
		light=255;
	else
	{
		dist=(int)((x-fviewx)*viewcos+(y-fviewy)*viewsin)/2;
		if (dist<=0)
			return;
		light=R_GetLight(dist, thing->subsector->sector->lightlevel);
	}
	WallVertices[0].color=WallVertices[1].color=WallVertices[2].color=WallVertices[3].color=Lightmap[light];
	WallVertices[0].tu=WallVertices[1].tu=offs;
	WallVertices[2].tu=WallVertices[3].tu=1.0f-offs;
	WallVertices[0].tv=WallVertices[2].tv=1.0f;
	WallVertices[1].tv=WallVertices[3].tv=0.0f;
	offs=-spriteoffset[spritenum];
	WallVertices[0].x=WallVertices[1].x=x+viewsin*offs;
	WallVertices[0].y=WallVertices[1].y=y-viewcos*offs;
	offs+=spritewidth[spritenum];
	WallVertices[2].x=WallVertices[3].x=x+viewsin*offs;
	WallVertices[2].y=WallVertices[3].y=y-viewcos*offs;
	if (thing->z==thing->floorz)
		z=F2D3D(thing->z)+spriteheight[spritenum];
	else
		z=F2D3D(thing->z)+spritetopoffset[spritenum];
	WallVertices[0].z=WallVertices[2].z=z-spriteheight[spritenum];
	WallVertices[1].z=WallVertices[3].z=z;

	R_SelectSprite(spritenum, SpriteMip, (thing->flags&MF_TRANSLATION)>>MF_TRANSSHIFT);
	//FUDGERETURN
	hres=pD3DDevice->lpVtbl->DrawPrimitive(pD3DDevice, D3DPT_TRIANGLESTRIP, FVF_DOOM3DVERTEX, WallVertices, 4, D3DDP_DONOTLIGHT|D3DDP_DONOTUPDATEEXTENTS);
    if (hres!=D3D_OK)
		I_Error("DrawPrimitive(Sprite) Failed");
}

void R_DrawMD2PSprite(pspdef_t *psp, int light)
{
    I_SetViewMatrix(fviewx, fviewy, fviewz-45/*-F2D3D(psp->sy)*/, viewangle);
	MD2SpriteMatrix=true;
	R_RenderModel(psp->state->md2, psp->frame, light, NULL, -1);
}

void R_DrawPSprite(pspdef_t *psp, int light)
{
	doomtvertex_t	v[4];
    spritedef_t*	sprdef;
    spriteframe_t*	sprframe;
    int				spritenum;
    int				flip;
	HRESULT			hres;
	float			left;
	float			right;
	float			top;
	float			bottom;
	D3DCOLOR		color;

	if (psp->state->md2&&!DisableMD2Weapons)
	{
		R_DrawMD2PSprite(psp, light);
		return;
	}

	sprdef = &spriteinfo[psp->state->sprite];
    sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];
	if (psp->state->frame & FF_FULLBRIGHT)
		color=Lightmap[255];
	else
		color=Lightmap[light];
    spritenum=sprframe->lump[0];
    flip=sprframe->flip[0];

	left=ViewWindowX+(F2D3D(psp->sx)-spriteoffset[spritenum])*ViewWidth/320;
	right=left+(spritewidth[spritenum]*ViewWidth/320);
	top=ViewWindowY+(F2D3D(psp->sy)-spritetopoffset[spritenum])*ViewHeight/200;
	bottom=top+(spriteheight[spritenum]*ViewHeight/200);
	v[0].x=v[2].x=left;
	v[1].x=v[3].x=right;
	v[0].y=v[1].y=top;
	v[2].y=v[3].y=bottom;
	v[0].z=v[1].z=v[2].z=v[3].z=0;
	v[0].rhw=v[1].rhw=v[2].rhw=v[3].rhw=1;

	v[0].tu=v[2].tu=(D3DVALUE)flip;
	v[1].tu=v[3].tu=(D3DVALUE)1-flip;
	v[0].tv=v[1].tv=(D3DVALUE)flip;
	v[2].tv=v[3].tv=(D3DVALUE)1-flip;
	v[0].color=v[1].color=v[2].color=v[3].color=color;

    //pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
	R_SelectSprite(spritenum, 0, 0);
	//FUDGERETURN
 	hres=pD3DDevice->lpVtbl->DrawPrimitive(pD3DDevice, D3DPT_TRIANGLESTRIP, FVF_DOOMTVERTEX, v, 4, D3DDP_DONOTLIGHT|D3DDP_DONOTUPDATEEXTENTS);
	if (hres!=D3D_OK)
		I_Error("DrawPrimitive(Player Sprite) Failed");
}

void R_RenderPlayerSprites(player_t *player)
{
    int			light;
    pspdef_t*	psp;

    // get light level
    light = player->mo->subsector->sector->lightlevel*2
		;//+extralight;

	if (fullbright||(light>255))
		light=255;
    // add all active psprites
	/*
    for (i=0, psp=player->psprites;
		i<NUMPSPRITES;
		i++,psp++)
	*/
	//for (psp=&player->psprites[NUMPSPRITES-1];psp>=player->psprites;psp--)
	psp=&player->psprites[ps_weapon];
//FUDGE: add weapon flash with models?
	if ((psp->state&&psp->state->md2)&&!DisableMD2Weapons)
		R_DrawPSprite (psp, light);
	else
	{
		for (psp=player->psprites;psp<&player->psprites[NUMPSPRITES];psp++)
		{
			if (psp->state)
				R_DrawPSprite (psp, light);
		}
	}
}

void R_StartSprites(void)
{
	HRESULT		hres;

	hres=pD3DDevice->lpVtbl->SetTextureStageState(pD3DDevice, 0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
	if (hres!=D3D_OK)
		I_Error("SetTextureStageState(Clamp) Failed\n");
}

void R_EndSprites(void)
{
	HRESULT		hres;

	hres=pD3DDevice->lpVtbl->SetTextureStageState(pD3DDevice, 0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
	if (hres!=D3D_OK)
		I_Error("SetTextureStageState(Wrap) Failed\n");
	if (MD2SpriteMatrix)
	{
		I_SetViewMatrix(0, 0, 0, 0);
		MD2SpriteMatrix=false;
	}
}

void R_DrawSky(void)
{
	doomtvertex_t	v[4];
	float			pos;

	v[0].x=v[2].x=(D3DVALUE)ViewWindowX;
	v[1].x=v[3].x=(D3DVALUE)(ViewWindowX+ViewWidth);
	v[0].y=v[1].y=(D3DVALUE)ViewWindowY;
	v[2].y=v[3].y=(D3DVALUE)(ViewWindowY+ViewHeight);
	v[0].z=v[1].z=v[2].z=v[3].z=0.999999f;
	v[0].rhw=v[1].rhw=v[2].rhw=v[3].rhw=1.0f;

	pos=-(viewangle/(float)ANG90);
	while (pos<1.0f)
		pos+=1.0f;
	v[0].tu=v[2].tu=pos;
	v[1].tu=v[3].tu=pos+1.0f;
	v[0].tv=v[1].tv=0.0f;
	v[2].tv=v[3].tv=1.0f;
	v[0].color=v[1].color=v[2].color=v[3].color=Lightmap[255];//D3DRGB(1, 1, 1);

	R_SelectTexture(skytexture, NULL, NULL);
 	pD3DDevice->lpVtbl->DrawPrimitive(pD3DDevice, D3DPT_TRIANGLESTRIP, FVF_DOOMTVERTEX, v, 4, D3DDP_DONOTLIGHT|D3DDP_DONOTUPDATEEXTENTS);
}

void R_DrawSubSectorPlanes(subsector_t *sub)
{
	int				i;
	int				count;
	float			floorz;
	float			x;
	float			y;
	DOOM3DVERTEX	*v;
	int				visible;
	int				dist;
	int				light;
	seg_t			*seg;
	fixed_t			tx;
	fixed_t			ty;

	if (sub->numlines<3)
		return;
    floorz=F2D3D(sub->sector->floorheight);
    count=sub->numlines;
    seg=&segs[sub->firstline];
    v=SSectorVertices;
	visible=count;
    //need to keep texture coords small to avoid floor 'wobble' due to rounding errors on some cards
    //make relative to first vertex, not (0,0) which is arbitary anyway
	tx=(seg->v1->x>>6)&~(FRACUNIT-1);
	ty=(seg->v1->y>>6)&~(FRACUNIT-1);
	while (count--)
	{
		x=F2D3D(seg->v1->x);
		y=F2D3D(seg->v1->y);
		v->x=x;
		v->y=y;
		v->tu=F2D3D((seg->v1->x>>6)-tx);
		v->tv=F2D3D((seg->v1->y>>6)-ty);
		v->z=floorz;
		if (fullbright)
			light=255;
		else
		{
			dist=(int)((x-fviewx)*viewcos+(y-fviewy)*viewsin)/2;
			if (dist<=0)
				visible--;
			light=R_GetLight(dist, sub->sector->lightlevel);
		}
		v->color=Lightmap[light];
		v++;
		seg++;
	}
	if (!visible)
		return;

	if (sub->sector->floorpic!=skyflatnum)
	{
	    R_SelectFlat(sub->sector->floorpic);
		pD3DDevice->lpVtbl->DrawPrimitive(pD3DDevice, D3DPT_TRIANGLEFAN, FVF_DOOM3DVERTEX, SSectorVertices, sub->numlines, D3DDP_DONOTLIGHT|D3DDP_DONOTUPDATEEXTENTS);
	}

	if (sub->sector->ceilingpic!=skyflatnum)
	{
		floorz=F2D3D(sub->sector->ceilingheight);
		for (i=0;i<sub->numlines;i++)
		{
			SSectorVertices[i].z=floorz;
		}
		R_SelectFlat(sub->sector->ceilingpic);
		pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_CULLMODE, D3DCULL_CW);
		pD3DDevice->lpVtbl->DrawPrimitive(pD3DDevice, D3DPT_TRIANGLEFAN, FVF_DOOM3DVERTEX, SSectorVertices, sub->numlines, D3DDP_DONOTLIGHT|D3DDP_DONOTUPDATEEXTENTS);
		pD3DDevice->lpVtbl->SetRenderState(pD3DDevice, D3DRENDERSTATE_CULLMODE, D3DCULL_CCW);
	}
}
