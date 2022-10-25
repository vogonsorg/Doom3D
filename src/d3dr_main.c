//d3dr_main.c

#include "doomdef.h"
#include "d3dr_local.h"
#include "d3di_ddraw.h"
#include "d3dv_video.h"
#include "d3di_video.h"
#include "m_fixed.h"
#include "tables.h"

#include "c_d3d.h"

lumpinfo_t	*lumpinfo;
int			skyflatnum;
int			skytexture;

fixed_t		viewx=0;
fixed_t		viewy=0;
float		fviewx=0;
float		fviewy=0;
float		fviewz=0;
angle_t		viewangle=0;

angle_t		viewangleoffset=0;
float		viewoffset=0;
float		viewsin;
float		viewcos;
dboolean	fullbright=false;
dboolean	invul=false;
char		*MD2ini;
player_t	*renderplayer;
int			playersector;

int			ValidCount;

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table.

//

//Note not same as software version, which gets angle from (viewx, viewy) rather than (0, 0)
angle_t
R_PointToAngle
( fixed_t	x,
  fixed_t	y )
{
    if ( (!x) && (!y) )
	return 0;

    if (x>= 0)
    {
	// x >=0
	if (y>= 0)
	{
	    // y>= 0

	    if (x>y)
	    {
		// octant 0
		return tantoangle[ SlopeDiv(y,x)];
	    }
	    else
	    {
		// octant 1
		return ANG90-1-tantoangle[ SlopeDiv(x,y)];
	    }
	}
	else
	{
	    // y<0
	    y = -y;

	    if (x>y)
	    {
		// octant 8
		return 0-tantoangle[SlopeDiv(y,x)];
	    }
	    else
	    {
		// octant 7
		return ANG270+tantoangle[ SlopeDiv(x,y)];
	    }
	}
    }
    else
    {
	// x<0
	x = -x;

	if (y>= 0)
	{
	    // y>= 0
	    if (x>y)
	    {
		// octant 3
		return ANG180-1-tantoangle[ SlopeDiv(y,x)];
	    }
	    else
	    {
		// octant 2
		return ANG90+ tantoangle[ SlopeDiv(x,y)];
	    }
	}
	else
	{
	    // y<0
	    y = -y;

	    if (x>y)
	    {
		// octant 4
		return ANG180+tantoangle[ SlopeDiv(y,x)];
	    }
	    else
	    {
		 // octant 5
		return ANG270-1-tantoangle[ SlopeDiv(x,y)];
	    }
	}
    }
    //return 0;
}



//
// R_PointOnSide
// Traverse BSP (sub) tree,
//  check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
int
R_PointOnSide
( fixed_t	x,
  fixed_t	y,
  node_t*	node )
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;

    if (!node->dx)
    {
	if (x <= node->x)
	    return node->dy > 0;

	return node->dy < 0;
    }
    if (!node->dy)
    {
	if (y <= node->y)
	    return node->dx < 0;

	return node->dx > 0;
    }

    dx = (x - node->x);
    dy = (y - node->y);

    // Try to quickly decide by looking at sign bits.
    if ( (node->dy ^ node->dx ^ dx ^ dy)&0x80000000 )
    {
	if  ( (node->dy ^ dx) & 0x80000000 )
	{
	    // (left is negative)
	    return 1;
	}
	return 0;
    }

    left = FixedMul ( F2INT(node->dy), dx );
    right = FixedMul ( dy , F2INT(node->dx));

    if (right < left)
    {
	// front side
	return 0;
    }
    // back side
    return 1;
}


angle_t
R_PointToAngle2
( fixed_t	x1,
  fixed_t	y1,
  fixed_t	x2,
  fixed_t	y2 )
{
    return R_PointToAngle (x2-x1, y2-y1);
}

void R_Init(void)
{
    R_InitData();
    R_InitDraw();
	R_InitLightmaps();
	R_InitMD2(MD2ini);
}

void R_ViewSizeChanged(void)
{
	I_SetRenderViewports();
}

void R_DrawViewBorder(void)
{
    if ((ViewWidth == ScreenWidth)||(NumSplits>1))
		return;

    // copy top
	V_CopyRect(0, 0, 1, ScreenWidth, ViewWindowY, 0, 0, 0);

    // left
	V_CopyRect(0, ViewWindowY, 1, ViewWindowX, ViewHeight, 0, ViewWindowY, 0);

	//right
	V_CopyRect(ViewWindowX+ViewWidth, ViewWindowY, 1, ScreenWidth-(ViewWindowX+ViewWidth), ViewHeight, ViewWindowX+ViewWidth, ViewWindowY, 0);

    // bottom
	V_CopyRect(0, ViewWindowY+ViewHeight, 1, ScreenWidth, ScreenHeight-(ViewWindowY+ViewHeight+ST_HEIGHT), 0, ViewWindowY+ViewHeight, 0);
}

void R_SetSkyTexture(char *name)
{
    skytexture=R_TextureNumForName(name);
}

//
// R_PointInSubsector
//
subsector_t*
R_PointInSubsector
( fixed_t	x,
  fixed_t	y )
{
    node_t*	node;
    int		side;
    int		nodenum;

    // single subsector is a special case
    if (!numnodes)
	return subsectors;

    nodenum = numnodes-1;

    while (! (nodenum & NF_SUBSECTOR) )
    {
	node = &nodes[nodenum];
	side = R_PointOnSide (x, y, node);
	nodenum = node->children[side];
    }

    return &subsectors[nodenum & ~NF_SUBSECTOR];
}

void R_SetViewAngleOffset(angle_t angle)
{
	viewangleoffset=angle;
}

void R_SetViewOffset(int offset)
{
	viewoffset=((float)offset)/10.0f;
}

void R_SetupLevel(dboolean hasglnodes)
{
    R_CalcSegLengths();
    if (hasglnodes)
    {
		UseGLNodes=true;
		R_CountSubsectorVerts();
	}
	else
    {
    	R_GeneratePlanes();
    	UseGLNodes=false;
	}
}

void R_SetupFrame(player_t *player)
{
	R_StartTextureFrame();
	viewangle=player->mo->angle+viewangleoffset;
	viewx=player->mo->x;
	viewy=player->mo->y;
	fviewx=F2D3D(viewx);
	fviewy=F2D3D(viewy);
	fviewz=F2D3D(player->viewz);

    viewsin = F2D3D(finesine[viewangle>>ANGLETOFINESHIFT]);
    viewcos = F2D3D(finecosine[viewangle>>ANGLETOFINESHIFT]);
    I_SetViewMatrix(0, 0, 0, 0);
	D_API.D_IncValidCount();
	if (player->fixedcolormap==FCM_FULLBRIGHT)
		fullbright=true;
	else
		fullbright=false;
	if (player->fixedcolormap==FCM_INVUL)
	{
		if (!invul)
		{
			pD3DDevice->lpVtbl->SetTextureStageState(pD3DDevice, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE|D3DTA_COMPLEMENT);
		}
		invul=true;
	}
	else
	{
		if (invul)
		{
			pD3DDevice->lpVtbl->SetTextureStageState(pD3DDevice, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		}
		invul=false;
	}
	playersector=player->mo->subsector->sector-sectors;
}

static void R_DoRenderPlayerView(player_t *player)
{
	renderplayer=player;
    R_SetupFrame(player);
    R_ClearView();
	R_ClearSprites();
    R_BeginScene();
    R_RenderBSPNode(numnodes-1);

	R_StartSprites();
	R_RenderSprites();

	if (*D_API.pShowGun)
		R_RenderPlayerSprites(player);
	R_EndSprites();
	R_DrawSky();

    R_EndScene();
}

static int		LastConsoleHeight=0;

void R_RenderPlayerView(player_t *player)
{
	int		split;
	int		height;

	height=*D_API.pConsolePos;
	if (LastConsoleHeight!=height)
	{
		LastConsoleHeight=height;
		I_SetRenderViewports();
	}

	for (split=0;split<NumSplits;split++)
	{
		D_API.S_GetViewPos(split, &ViewWindowX, &ViewWindowY);
		I_SelectViewport(split);
		R_DoRenderPlayerView(player);
		player++;
	}
}
