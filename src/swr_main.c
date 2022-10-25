
// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Rendering main loop and setup functions,
//	 utility functions (BSP, geometry, trigonometry).
//	See tables.c, too.
//
//-----------------------------------------------------------------------------

#ifdef RCSID
static const char rcsid[] = "$Id: r_main.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";

#endif


#include <stdlib.h>
#include <math.h>


#include "doomdef.h"
//#include "d_stuff.h"
//#include "d_net.h"

#include "t_bbox.h"
#include "m_fixed.h"

#include "swr_local.h"
#include "swr_sky.h"
#include "swi_video.h"

#include "tables.h"


// Fineangles in the ScreenWidth wide window.
#define FIELDOFVIEW		2048



int	ScreenWidth;
int	ScreenHeight;

int			MatrixSeperation;
int			MatrixInterleave;
dboolean	MatrixMode=false;

int			viewangleoffset=0;
fixed_t		viewoffset=0;

// incremented every time a check is made
int			ValidCount;


int			fixedcolormap=-1;
extern int*		walllights;

int			centerx=0;
int			centery=0;

fixed_t			centerxfrac=0;
fixed_t			centeryfrac=0;
fixed_t			projection=0;

int			sscount=0;
int			linecount=0;
int			loopcount=0;

fixed_t			viewx=0;
fixed_t			viewy=0;
fixed_t			viewz=0;

angle_t			viewangle=0;

fixed_t			viewcos=0;
fixed_t			viewsin=0;

player_t*		viewplayer=NULL;

int				playersector;

//
// precalculated math tables
//
angle_t			clipangle=0;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int			*viewangletox=NULL;

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t			*xtoviewangle;


// UNUSED.
// The finetangentgent[angle+FINEANGLES/4] table
// holds the fixed_t tangent values for view angles,
// ranging from MININT to 0 to MAXINT.
// fixed_t		finetangent[FINEANGLES/2];

// fixed_t		finesine[5*FINEANGLES/4];
//fixed_t*		finecosine = &finesine[FINEANGLES/4];


int		scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
int		scalelightfixed[MAXLIGHTSCALE];
int		zlight[LIGHTLEVELS][MAXLIGHTZ];

// bumped light from gun blasts
int			extralight;



void (*colfunc) (void);
void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);
void (*transcolfunc) (void);
//void (*spanfunc) (void);


void R_SetViewAngleOffset(angle_t angle)
{
	viewangleoffset=angle;
}

void R_SetViewOffset(int offset)
{
	viewoffset=INT2F(offset)/10;
}

//
// R_AddPointToBox
// Expand a given bbox
// so that it encloses a given point.
//
void
R_AddPointToBox
( int		x,
 int		y,
 fixed_t*	box )
{
    if (x< box[BOXLEFT])
		box[BOXLEFT] = x;
    if (x> box[BOXRIGHT])
		box[BOXRIGHT] = x;
    if (y< box[BOXBOTTOM])
		box[BOXBOTTOM] = y;
    if (y> box[BOXTOP])
		box[BOXTOP] = y;
}


void R_SetColorBias(int type, int strength)
{
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


int
R_PointOnSegSide
( fixed_t	x,
 fixed_t	y,
 seg_t*	line )
{
    fixed_t	lx;
    fixed_t	ly;
    fixed_t	ldx;
    fixed_t	ldy;
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;

    lx = line->v1->x;
    ly = line->v1->y;

    ldx = line->v2->x - lx;
    ldy = line->v2->y - ly;

    if (!ldx)
    {
		if (x <= lx)
			return ldy > 0;

		return ldy < 0;
    }
    if (!ldy)
    {
		if (y <= ly)
			return ldx < 0;

		return ldx > 0;
    }

    dx = (x - lx);
    dy = (y - ly);

    // Try to quickly decide by looking at sign bits.
    if ( (ldy ^ ldx ^ dx ^ dy)&0x80000000 )
    {
		if  ( (ldy ^ dx) & 0x80000000 )
		{
			// (left is negative)
			return 1;
		}
		return 0;
    }

    left = FixedMul ( F2INT(ldy), dx );
    right = FixedMul ( dy , F2INT(ldx));

    if (right < left)
    {
		// front side
		return 0;
    }
    // back side
    return 1;
}


//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table.

//




angle_t
R_PointToAngle
( fixed_t	x,
 fixed_t	y )
{
    x -= viewx;
    y -= viewy;

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


angle_t
R_PointToAngle2
( fixed_t	x1,
 fixed_t	y1,
 fixed_t	x2,
 fixed_t	y2 )
{
    viewx = x1;
    viewy = y1;

    return R_PointToAngle (x2, y2);
}


fixed_t
R_PointToDist
( fixed_t	x,
 fixed_t	y )
{
    int		angle;
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	temp;
    fixed_t	dist;

    dx = abs(x - viewx);
    dy = abs(y - viewy);

    if (dy>dx)
    {
		temp = dx;
		dx = dy;
		dy = temp;
    }

    angle = (tantoangle[ FixedDiv(dy,dx)>>DBITS ]+ANG90) >> ANGLETOFINESHIFT;

    // use as cosine
    dist = FixedDiv (dx, finesine[angle] );

    return dist;
}




//
// R_InitPointToAngle
//
void R_InitPointToAngle (void)
{
    // UNUSED - now getting from tables.c
#if 0
    int	i;
    long	t;
    float	f;
	//
	// slope (tangent) to angle lookup
	//
    for (i=0 ; i<=SLOPERANGE ; i++)
    {
		f = atan( (float)i/SLOPERANGE )/(3.141592657*2);
		t = 0xffffffff*f;
		tantoangle[i] = t;
    }
#endif
}


//
// R_ScaleFromGlobalAngle
// Returns the texture mapping scale
//  for the current line (horizontal span)
//  at the given angle.
// rw_distance must be calculated first.
//
fixed_t R_ScaleFromGlobalAngle (angle_t visangle)
{
    fixed_t		scale;
    int			anglea;
    int			angleb;
    int			sinea;
    int			sineb;
    fixed_t		num;
    int			den;

    // UNUSED
#if 0
	{
		fixed_t		dist;
		fixed_t		z;
		fixed_t		sinv;
		fixed_t		cosv;

		sinv = finesine[(visangle-rw_normalangle)>>ANGLETOFINESHIFT];
		dist = FixedDiv (rw_distance, sinv);
		cosv = finecosine[(viewangle-visangle)>>ANGLETOFINESHIFT];
		z = abs(FixedMul (dist, cosv));
		scale = FixedDiv(projection, z);
		return scale;
	}
#endif

    anglea = ANG90 + (visangle-viewangle);
    angleb = ANG90 + (visangle-rw_normalangle);

    // both sines are allways positive
    sinea = finesine[anglea>>ANGLETOFINESHIFT];
    sineb = finesine[angleb>>ANGLETOFINESHIFT];
    num = FixedMul(projection,sineb);
    den = FixedMul(rw_distance,sinea);

    if (den > F2INT(num))
    {
		scale = FixedDiv (num, den);

		if (scale > 64*FRACUNIT)
			scale = 64*FRACUNIT;
		else if (scale < 256)
			scale = 256;
    }
    else
		scale = 64*FRACUNIT;

    return scale;
}



//
// R_InitTables
//
void R_InitTables (void)
{
    // UNUSED: now getting from tables.c
#if 0
    int		i;
    float	a;
    float	fv;
    int		t;

    // viewangle tangent table
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
		a = (i-FINEANGLES/4+0.5)*PI*2/FINEANGLES;
		fv = FRACUNIT*tan (a);
		t = fv;
		finetangent[i] = t;
    }

    // finesine table
    for (i=0 ; i<5*FINEANGLES/4 ; i++)
    {
		// OPTIMIZE: mirror...
		a = (i+0.5)*PI*2/FINEANGLES;
		t = FRACUNIT*sin (a);
		finesine[i] = t;
    }
#endif

}



//
// R_InitTextureMapping
//
void R_InitTextureMapping (void)
{
    int			i;
    int			x;
    int			t;
    fixed_t		focallength;

    // Use tangent table to generate viewangletox:
    //  viewangletox will give the next greatest x
    //  after the view angle.
    //
    // Calc focallength
    //  so FIELDOFVIEW angles covers ScreenWidth.
    focallength = FixedDiv (centerxfrac,
		finetangent[FINEANGLES/4+FIELDOFVIEW/2] );

    for (i=0 ; i<FINEANGLES/2 ; i++)
    {
		if (finetangent[i] > FRACUNIT*2)
			t = -1;
		else if (finetangent[i] < -FRACUNIT*2)
			t = viewwidth+1;
		else
		{
			t = FixedMul (finetangent[i], focallength);
			t = F2INT(centerxfrac - t+FRACUNIT-1);

			if (t < -1)
				t = -1;
			else if (t>viewwidth+1)
				t = viewwidth+1;
		}
		viewangletox[i] = t;
    }

    // Scan viewangletox[] to generate xtoviewangle[]:
    //  xtoviewangle will give the smallest view angle
    //  that maps to x.
    for (x=0;x<=viewwidth;x++)
    {
		i = 0;
		while (viewangletox[i]>x)
			i++;
		xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
    }

    // Take out the fencepost cases from viewangletox.
    for (i=0 ; i<FINEANGLES/2 ; i++)
    {

		if (viewangletox[i] == -1)
			viewangletox[i] = 0;
		else if (viewangletox[i] == viewwidth+1)
			viewangletox[i]  = viewwidth;
    }

    clipangle = xtoviewangle[0];
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP		2

void R_InitLightTables (void)
{
    int		i;
    int		j;
    int		level;
    int		startmap;
    int		scale;

    // Calculate the light levels to use
    //  for each level / distance combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
		startmap = ((LIGHTLEVELS-1-i)*2)*NumColorMaps/LIGHTLEVELS;
		for (j=0 ; j<MAXLIGHTZ ; j++)
		{
			scale = FixedDiv ((ScreenWidth*FRACUNIT/2), (j+1)<<LIGHTZSHIFT);
			scale >>= LIGHTSCALESHIFT;
			level = startmap - scale/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NumColorMaps)
				level = NumColorMaps-1;

			zlight[i][j] = level;
		}
    }
}



//
// R_ViewSizeChanged
//
void R_ViewSizeChanged(void)
{
    fixed_t	cosadj;
    fixed_t	dy;
    int		i;
    int		j;
    int		level;
    int		startmap;

    centery = viewheight/2;
    centerx = viewwidth/2;
    centerxfrac = INT2F(centerx);
    centeryfrac = INT2F(centery);
    projection = centerxfrac;

    colfunc = basecolfunc = R_DrawColumn;
    fuzzcolfunc = R_DrawFuzzColumn;
    transcolfunc = R_DrawTranslatedColumn;
//    spanfunc = R_DrawSpan;

    R_InitBuffer ();

    R_InitTextureMapping ();

    // psprite scales
    if ((viewheight==ScreenHeight)||(NumSplits>1))
    {
		pspritescale = FRACUNIT*viewheight/200;
		pspriteiscale = FRACUNIT*200/viewheight;
    }
    else
    {
		pspritescale = FRACUNIT*(viewheight+ST_HEIGHT)/200;
		pspriteiscale = FRACUNIT*200/(viewheight+ST_HEIGHT);
    }

    // thing clipping
    for (i=0 ; i<viewwidth ; i++)
		screenheightarray[i] = viewheight;

    // planes
    for (i=0 ; i<viewheight ; i++)
    {
		dy = (INT2F(i-viewheight/2))+FRACUNIT/2;
		dy = abs(dy);
		yslope[i] = FixedDiv (viewwidth/2*FRACUNIT, dy);
    }

    for (i=0 ; i<viewwidth ; i++)
    {
		cosadj = abs(finecosine[xtoviewangle[i]>>ANGLETOFINESHIFT]);
		distscale[i] = FixedDiv (FRACUNIT,cosadj);
    }

    // Calculate the light levels to use
    //  for each level / scale combination.
    for (i=0 ; i< LIGHTLEVELS ; i++)
    {
		startmap = ((LIGHTLEVELS-1-i)*2)*NumColorMaps/LIGHTLEVELS;
		for (j=0 ; j<MAXLIGHTSCALE ; j++)
		{
			level = startmap - j*ScreenWidth/viewwidth/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NumColorMaps)
				level = NumColorMaps-1;

			scalelight[i][j] = level;
		}
    }
}



//
// R_Init
//


void R_Init (void)
{
    viewangletox=malloc((FINEANGLES/2)*sizeof(*viewangletox));
    xtoviewangle=malloc((ScreenWidth+1)*sizeof(*xtoviewangle));
    R_InitDraw();
    R_InitData ();
    R_InitFuzz();
    I_Printf ("\nR_InitData");
    R_InitPointToAngle ();
    I_Printf ("\nR_InitPointToAngle");
    R_InitTables ();
    // viewwidth / viewheight are set by the defaults
    I_Printf ("\nR_InitTables");

    R_InitPlanes ();
    I_Printf ("\nR_InitPlanes");
    R_InitLightTables ();
    I_Printf ("\nR_InitLightTables");
    R_InitSkyMap ();
    I_Printf ("\nR_InitSkyMap");
    R_InitTranslationTables ();
    I_Printf ("\nR_InitTranslationsTables");
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



//
// R_SetupFrame
//
void R_SetupFrame (player_t* player, fixed_t offset, int frame, int split)
{
    int		i;

	D_API.S_GetViewPos(split, &viewwindowx, &viewwindowy);
	R_SetupDrawFrame(frame, split);

	playersector=player->mo->subsector->sector-sectors;
    viewplayer = player;
    viewangle = player->mo->angle + viewangleoffset;
    extralight = player->extralight;

    viewz = player->viewz;

    viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
    viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

    viewx = player->mo->x+FixedMul(offset, viewsin);
    viewy = player->mo->y-FixedMul(offset, viewcos);

    sscount = 0;

    switch (player->fixedcolormap)
    {
    case FCM_FULLBRIGHT:
		fixedcolormap=0;
		break;
    case FCM_INVUL:
		fixedcolormap=NumColorMaps;
		break;
    default:
		fixedcolormap=-1;
		break;
    }
    if (fixedcolormap!=-1)
    {
		walllights = scalelightfixed;

		for (i=0 ; i<MAXLIGHTSCALE ; i++)
			scalelightfixed[i] = fixedcolormap;
    }
    D_API.D_IncValidCount();
}

void R_SetupLevel(dboolean hasglnodes)
{
}


void R_RenderMatrixView(player_t *player, fixed_t offset, int num)
{
    R_SetupFrame (player, offset, num, 0);

    // Clear buffers.
    R_ClearClipSegs ();
    R_ClearDrawSegs ();
    R_ClearPlanes ();
    R_ClearSprites ();

    // check for new console commands.
    D_API.NetUpdate ();

    // The head node is the last node output.
    R_RenderBSPNode (numnodes-1);

    // Check for new console commands.
    D_API.NetUpdate ();

    R_DrawPlanes ();

    // Check for new console commands.
    D_API.NetUpdate ();

    R_DrawMasked ();

    // Check for new console commands.
    D_API.NetUpdate ();
}

//
// R_RenderView
//
void R_DoRenderPlayerView (player_t* player, int split)
{
	if (MatrixMode)
	{
		R_RenderMatrixView(player, -INT2F(MatrixSeperation)/10, 0);
		R_RenderMatrixView(player, INT2F(MatrixSeperation)/10, 1);
		R_MergeMatrixViews();
		return;
	}
    R_SetupFrame (player, viewoffset, ScreenNum, split);
    // Clear buffers.
    R_ClearClipSegs ();
    R_ClearDrawSegs ();
    R_ClearPlanes ();
    R_ClearSprites ();

    // check for new console commands.
    D_API.NetUpdate ();

    // The head node is the last node output.
    R_RenderBSPNode (numnodes-1);

    // Check for new console commands.
    D_API.NetUpdate ();

    R_DrawPlanes ();

    // Check for new console commands.
    D_API.NetUpdate ();

    R_DrawMasked ();

    // Check for new console commands.
    D_API.NetUpdate ();
}

void R_RenderPlayerView(player_t *player)
{
	int		split;

	for (split=0;split<NumSplits;split++)
		R_DoRenderPlayerView(&player[split], split);
}
