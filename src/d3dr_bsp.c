//d3dr_bsp.c

#include "d3dr_local.h"
#include "d3di_ddraw.h"

#include "c_d3d.h"

sector_t	*frontsector;

//FUDGE: remove bboxes or get working
dboolean R_CheckBBox(fixed_t *bspcoord)
{
	return(true);
}

void R_AddLine(seg_t *line)
{
	DOOM3DVERTEX	*v;
	line_t			*linedef;
	side_t			*sidedef;
	D3DVALUE		top;
	D3DVALUE		bottom;
	D3DVALUE		btop;
	D3DVALUE		bbottom;
	int				height;
	int				width;
	D3DVALUE		length;
	D3DVALUE		rowoffs;
	D3DVALUE		coloffs;
	int				light;
	int				dist;
	int				visible;
	float			x;
	float			y;

	x=F2D3D(line->v1->x);
	y=F2D3D(line->v1->y);
//FUDGE: make subsector vertex list on level init, then dump minisegs?
#if 0
	if (UseGLNodes)
	{
		v=&SSectorVertices[NumSSVerts];
		v->x=x;
		v->y=y;
		v->tu=F2D3D(line->v1->x>>6);
		v->tv=F2D3D(line->v1->y>>6);
		NumSSVerts++;
	}
#endif
	linedef=line->linedef;
	if (!linedef)
		return;
	v=WallVertices;
	sidedef=line->sidedef;
	v[0].x=v[2].x=x;
	v[0].y=v[2].y=y;
	v[1].x=v[3].x=F2D3D(line->v2->x);
	v[1].y=v[3].y=F2D3D(line->v2->y);

	visible=2;
	if (fullbright)
		light=255;
	else
	{
		dist=(int)((x-fviewx)*viewcos+(y-fviewy)*viewsin)/2;
		if (dist<=0)
			visible--;
		light=R_GetLight(dist, frontsector->lightlevel);
	}
	v[0].color=v[2].color=Lightmap[light];
	if (!fullbright)
	{
		dist=(int)((v[1].x-fviewx)*viewcos+(v[1].y-fviewy)*viewsin)/2;
		if (dist<=0)
			visible--;
		light=R_GetLight(dist, frontsector->lightlevel);
	}
	if (!visible)
		return;
	v[1].color=v[3].color=Lightmap[light];
	top=F2D3D(frontsector->ceilingheight);
	bottom=F2D3D(frontsector->floorheight);
	length=SegLengths[line-segs];

	if (line->backsector)
	{
		btop=F2D3D(line->backsector->ceilingheight);
		bbottom=F2D3D(line->backsector->floorheight);
		if ((line->frontsector->ceilingpic==skyflatnum)&&(line->backsector->ceilingpic==skyflatnum))
			btop=top;
		if (top>btop)
		{
			v[0].z=v[1].z=top;
			v[2].z=v[3].z=btop;
			R_SelectTexture(sidedef->toptexture, &width, &height);
			rowoffs=F2D3D(sidedef->rowoffset)/height;
			coloffs=F2D3D(sidedef->textureoffset+line->offset)/width;
			v[0].tu=v[2].tu=coloffs;
			v[1].tu=v[3].tu=length/width+coloffs;
			if (linedef->flags&ML_DONTPEGTOP)
			{
				v[0].tv=v[1].tv=1+rowoffs;
				v[2].tv=v[3].tv=1+rowoffs+(top-btop)/height;
			}
			else
			{
				v[2].tv=v[3].tv=1+rowoffs;
				v[0].tv=v[1].tv=1+rowoffs-(top-btop)/height;
			}
			R_DrawWall();
		}
		if (bottom<bbottom)
		{
			v[0].z=v[1].z=bbottom;
			v[2].z=v[3].z=bottom;
			R_SelectTexture(sidedef->bottomtexture, &width, &height);
			rowoffs=F2D3D(sidedef->rowoffset)/height;
			coloffs=F2D3D(sidedef->textureoffset+line->offset)/width;
			v[0].tu=v[2].tu=coloffs;
			v[1].tu=v[3].tu=length/width+coloffs;
			if (linedef->flags&ML_DONTPEGBOTTOM)
			{
				v[0].tv=v[1].tv=rowoffs+(top-bbottom)/height;
				v[2].tv=v[3].tv=rowoffs+(top-bottom)/height;
			}
			else
			{
				v[0].tv=v[1].tv=rowoffs;
				v[2].tv=v[3].tv=rowoffs+(bbottom-bottom)/height;
			}
			R_DrawWall();
			bottom=bbottom;
		}
		if (top>btop)
			top=btop;

	}
	if (sidedef->midtexture)
	{
		v[0].z=v[1].z=top;
		v[2].z=v[3].z=bottom;

		R_SelectTexture(sidedef->midtexture, &width, &height);
		rowoffs=F2D3D(sidedef->rowoffset)/height;
		coloffs=F2D3D(sidedef->textureoffset+line->offset)/width;
		v[0].tu=v[2].tu=coloffs;
		v[1].tu=v[3].tu=length/width+coloffs;

		if (linedef->flags&ML_DONTPEGBOTTOM)//&&!line->backsector)
		{
			v[0].tv=v[1].tv=1+rowoffs-(top-bottom)/height;
			v[2].tv=v[3].tv=1+rowoffs;
		}
		else
		{
			v[0].tv=v[1].tv=rowoffs;
			v[2].tv=v[3].tv=rowoffs+(top-bottom)/height;
		}

		R_DrawWall();
	}
}

static void MarkSectorAsVisible(sector_t *s, dboolean recurse)
{
	line_t		*line;
	int			i;
	line_t		**l2;
	int			c2;
	sector_t	*other;

	for (i=0;i<s->linecount;i++)
	{
		line=s->lines[i];
		if ((line->flags&(ML_TWOSIDED|ML_SECRET))==ML_TWOSIDED)
		{
			if (line->frontsector==s)
				other=line->backsector;
			else
				other=line->frontsector;
			if (other->validcount!=ValidCount)
				c2=other->linecount;
			else
				c2=0;
			l2=other->lines;
			while (c2--)
			{
				(*l2)->flags|=ML_MAPPED;
				l2++;
			}

		}
		line->flags|=ML_MAPPED;
	}

}

void R_Subsector(int num)
{
	subsector_t	*sub;
	seg_t	*line;
	int		count;

	sub=&subsectors[num];
	count=sub->numlines;
	line=&segs[sub->firstline];
	frontsector=sub->sector;

	if (!L_CanSee(playersector, frontsector-sectors))
		return;

	(*D_API.pNumSSDrawn)++;
	// BSP is traversed by subsector.
	// A sector might have been split into several
	//  subsectors during BSP building.
	// Thus we check whether its already added.
	if (frontsector->validcount != ValidCount)
	{
		// Well, now it will be done.
		frontsector->validcount = ValidCount;
		if (playersector==frontsector-sectors)
		{
			MarkSectorAsVisible(frontsector, true);
		}

		if (!UseGLNodes)
			R_DrawSectorPlanes(frontsector);
		R_AddSprites(frontsector);
	}

//	NumSSVerts=0;
	while (count--)
	{
		R_AddLine(line);
		line++;
	}
	if (UseGLNodes)
		R_DrawSubSectorPlanes(sub);
}

void R_RenderBSPNode(int bspnum)
{
	node_t	*bsp;
	int		side;

	if (bspnum&NF_SUBSECTOR)
	{
		if (bspnum==1)
		{
			R_Subsector(0);
			I_Error("hello");
		}
		else
			R_Subsector(bspnum&~NF_SUBSECTOR);
		return;
	}
	bsp=&nodes[bspnum];
	side=R_PointOnSide(viewx, viewy, bsp);
	R_RenderBSPNode(bsp->children[side]);

	if (R_CheckBBox(bsp->bbox[side^1]))
		R_RenderBSPNode(bsp->children[side^1]);
}
