//d3dr_plane.c

#include "d3dr_local.h"
#include "d3di_ddraw.h"
#include "m_fixed.h"

#include "c_d3d.h"

#define PLANEVERTEX_ALLOCSIZE	10

plane_t **planes;

typedef struct
{
    fixed_t     x;
    fixed_t     y;
    line_t      *line;
	dboolean	used;
}planevertex_t;

dboolean	OnlySectorLines=true;
dboolean	UseGLNodes;
int			MaxSSVerts=0;

static int              vertexcount;
static planevertex_t    *planevertices=NULL;
static int				maxplanevertices=0;

void R_AddPlaneVertex(fixed_t x, fixed_t y, line_t *line)
{
	planevertex_t	*oldpv;

    if (vertexcount>=maxplanevertices)
	{
		oldpv=planevertices;
		maxplanevertices+=PLANEVERTEX_ALLOCSIZE;
		planevertices=(planevertex_t *)Z_Malloc(maxplanevertices*sizeof(planevertex_t), PU_STATIC, NULL);
		if (oldpv)
		{
			memcpy(planevertices, oldpv, (maxplanevertices-PLANEVERTEX_ALLOCSIZE)*sizeof(planevertex_t));
			Z_Free(oldpv);
		}
	}
    planevertices[vertexcount].x=x;
    planevertices[vertexcount].y=y;
    planevertices[vertexcount].line=line;
    vertexcount++;
}

static fixed_t  px1, px2, px3;
static fixed_t  py1, py2, py3;
static int      vert1, vert2, vert3;

static dboolean UsingPoint(fixed_t x, fixed_t y)
{
    if ((px1==x)&&(py1==y))
        return(true);
    if ((px2==x)&&(py2==y))
        return(true);
    if ((px3==x)&&(py3==y))
        return(true);
    return(false);
}

#define SIGNBIT (1<<31)

static dboolean OnSide(fixed_t dx, fixed_t dy, fixed_t x, fixed_t y)
{
    fixed_t     left;
    fixed_t     right;

    if (!dx)
    {
        if (x <= 0)
            return dy > 0;

        return dy < 0;
    }
    if (!dy)
    {
        if (y <= 0)
            return dx < 0;

        return dx > 0;
    }

    // Try to quickly decide by looking at sign bits.
    if ( (dy ^ dx ^ x ^ y)&0x80000000 )
    {
        if  ( (dy ^ x) & 0x80000000 )
        {
            // (left is negative)
            return 1;
        }
        return 0;
    }

    left = FixedMul ( F2INT(dy), x );
    right = FixedMul ( y , F2INT(dx));

    if (right < left)
    {
        // front side
        return 0;
    }
    // back side
    return 1;
}

static dboolean IsCW(void)
{
    planevertex_t       *v;

    if (!OnSide(px3-px1, py3-py1, px2-px1, py2-py1))
        return(false);
    for (v=planevertices;v<&planevertices[vertexcount];v++)
    {
        if (UsingPoint(v->x, v->y))
            continue;
        if (OnSide(px2-px1, py2-py1, v->x-px1, v->y-py1))
            continue;
        if (OnSide(px3-px2, py3-py2, v->x-px2, v->y-py2))
            continue;
        if (OnSide(px1-px3, py1-py3, v->x-px3, v->y-py3))
            continue;
        return(false);
    }
    return(true);
}

static int GetVertex(fixed_t *px, fixed_t *py, int pos)
{
    if (pos>=vertexcount)
        pos-=vertexcount;
    while (planevertices[pos].used)
    {
        pos++;
        if (pos>=vertexcount)
            pos=0;
    }
    *px=planevertices[pos].x;
    *py=planevertices[pos].y;
    return(pos);
}

static void CopyVertices(plane_t *plane)
{
    WORD        *p;

    p=&plane->indices[plane->numindices];
    plane->numindices+=3;
    *(p++)=vert1;
    *(p++)=vert2;
    *p=vert3;
}

static dboolean SplitPolygon(plane_t *plane, dboolean backwards)
{
/*
    int         mid;
    int         end;
    int         start;
*/
    int         loopvert;
    int         numleft;

    numleft=vertexcount;
    for (loopvert=0;loopvert<vertexcount;loopvert++)
        planevertices[loopvert].used=false;
    vert1=GetVertex(&px1, &py1, 0);
    vert2=loopvert=GetVertex(&px2, &py2, 1);
    vert3=GetVertex(&px3, &py3, 2);
    while (numleft>3)
    {
        if (IsCW()^backwards)
        {
            planevertices[vert2].used=true;
            CopyVertices(plane);
            px2=px3;
            py2=py3;
            numleft--;
            loopvert=vert3;
        }
        else
        {
            vert1=vert2;
            px1=px2;
            py1=py2;
            px2=px3;
            py2=py3;
            if (vert3==loopvert)
                return(false);
        }
        vert2=vert3;
        vert3=GetVertex(&px3, &py3, vert2+1);
    }
    if (!(IsCW()^backwards))
    {
        px2=px3;
        py2=py3;
        vert3=vert2;
        vert2=GetVertex(&px3, &py3, vert2);
    }
    CopyVertices(plane);
	return(true);
}

static dboolean UsedLine(line_t *line)
{
    planevertex_t       *v;


    for (v=planevertices;v<&planevertices[vertexcount];v++)
    {
        if (v->line==line)
            return(true);
    }
    return(false);
}
/*
vertex_t *R_FindPoly(plane_t *plane, vertex_t *start, vertex_t *next)
{
	vertex_t	*v;

	v=next;
	while ((v->outlets==1)&&(v!=lastnode))
	{
		advance(&v);
	}
	if (v->outlets<1)
		I_Error("dead end");
	if (v==start)
		return(v);
	if (v!=
	for (i=0;i<v->outlets;i++)
	{
		if (R_FindPoly(plane, v, v->outlet[i])==start)
		{
			AddPoly();
		}
	}
}
*/


/*
x1+a*dx1=x2+b*dx2
y1+a*dy1=y2+b*dy2

a*dx1-b*dx2)=x2-x1
a*dy1-b*dy2)=y2-y1

a.dx1/dx2-b=(x2-x1)/dx2
a.dy1/dy2-b=(y2-y1)/dy2

a=((x2-x1)/dx2-(y2-y1)/dy2)/(dx1/dx2-dy1/dy2)

x=x1+dx1*a
y=y1+dy1*a

x1+a*dx1=x2
y1+a*dy1=y2+b*dy2
a=(x2-x1)/dx1
b=(y1-y2+a*dy1)/dy2
*/
fixed_t GetPointPos(node_t *n1, node_t *n2)
{
	if ((n1->dx==n2->dx)&&(n1->dy==n2->dy))
		return(MAXINT);
	if ((n1->dx==0)||(n2->dx==0)||(n1->dy==0)||(n2->dy==0))
		I_Error("/0");
	return(FixedDiv(FixedDiv(n2->x-n1->x, n2->dx)-FixedDiv(n2->y-n1->y, n2->dy), FixedDiv(n1->dx, n2->dx)-FixedDiv(n1->dy, n2->dy)));
}

typedef struct
{
	fixed_t	x;
	fixed_t	y;
	int		n1;
	int		n2;
}ssvertex_t;

#define MNO	256
node_t		*ssnodes[MNO];
int			sspos[MNO];
ssvertex_t	sspoints[MNO];
int			sspointcount;

dboolean GetIntercept(node_t *n1, node_t *n2, ssvertex_t *v)
{
	fixed_t		a;

	//if ((n1->dx==n2->dx)&&(n1->dy==n2->dy))
	if (FixedMul(n1->dx, n2->dy)==FixedMul(n2->dx, n1->dy))
		return(false);//parallel

	if (n2->dx==0)
	{
		v->x=n2->x;
		v->y=n1->y+FixedMul(n1->dy, FixedDiv(n2->x-n1->x, n1->dx));
		return(true);
	}
	if (n2->dy==0)
	{
		v->x=n1->x+FixedMul(n1->dx, FixedDiv(n2->y-n1->y, n1->dy));
		v->y=n2->y;
		return(true);
	}
	a=FixedDiv(FixedDiv(n2->x-n1->x, n2->dx)-FixedDiv(n2->y-n1->y, n2->dy), FixedDiv(n1->dx, n2->dx)-FixedDiv(n1->dy, n2->dy));
	v->x=n1->x+FixedMul(a, n1->dx);
	v->y=n1->y+FixedMul(a, n1->dy);
	return(true);
}

void FinishTraverse(int deep, int sector)
{
	int			n;
	int			i;
	int			j;
	ssvertex_t	v;
	node_t		*node;
	ssvertex_t	*ssv;

	if (deep<3)
		return;
	sspointcount=0;
	for (n=1;n<deep;n++)
	{
		node=ssnodes[n];

		for (i=0;i<sspointcount;i++)
		{
			ssv=&sspoints[i];
			if ((i==ssv->n1)||(i==ssv->n2))
				continue;
			if (OnSide(node->dx, node->dy, ssv->x-node->x, ssv->y-node->y)!=sspos[n])
			{
				if (--sspointcount)
					memmove(ssv, ssv+1, (sspointcount-i)*sizeof(ssvertex_t));
			}
		}

		for (i=0;i<n;i++)
		{
			dboolean	want;

			want=true;
			if (!GetIntercept(node, ssnodes[i], &v))
				continue;
			v.n1=n;
			v.n2=i;
			for (j=0;j<n;j++)
			{
				if (i==j)
					continue;
				if (OnSide(ssnodes[j]->dx, ssnodes[j]->dy, v.x-ssnodes[j]->x, v.y-ssnodes[j]->y)!=sspos[j])
					want=false;
			}
			if (want)
			{
				sspoints[sspointcount].y=v.y;
				sspoints[sspointcount].y=v.y;
				sspointcount++;
			}
		}
	}
}

void Traverse(int num, int deep)
{
	node_t	*bsp;

	if (num&NF_SUBSECTOR)
	{
		FinishTraverse(deep-1, num&~NF_SUBSECTOR);
		I_Printf("%5d(%d)\n", deep, sspointcount);
		return;
	}
	bsp=&nodes[num];
	ssnodes[deep]=bsp;
	sspos[num]=0;
	Traverse(bsp->children[0], deep+1);
	sspos[num]=1;
	Traverse(bsp->children[1], deep+1);
}

void R_Dummy(void)
{
	Traverse(numnodes-1, 0);
}

void R_GeneratePlanes(void)
{
    sector_t    *sector;
    plane_t     *plane;
    fixed_t     firstx;
    fixed_t     firsty;
    line_t      *line;
	line_t		**pline;
    fixed_t     x;
    fixed_t     y;
    int         n;
    int         i;
    DOOM3DVERTEX        *v;

	//R_Dummy();
    sector=sectors;
    planes=(plane_t **)Z_Malloc(numsectors*sizeof(plane_t *), PU_STATIC, NULL);
    for (n=0;n<numsectors;n++, sector++)
    {
        if (!sector->linecount)
            continue;
        line=sector->lines[0];

        if (line->frontsector==sector)
        {
            firstx=line->v1->x;
            firsty=line->v1->y;
            x=line->v2->x;
            y=line->v2->y;
        }
        else
        {
            firstx=line->v2->x;
            firsty=line->v2->y;
            x=line->v1->x;
            y=line->v1->y;
        }
        vertexcount=0;
        R_AddPlaneVertex(x, y, line);
        while ((x!=firstx)||(y!=firsty))
        {
            //for (line=lines;line<&lines[numlines];line++)
            //for (pline=sector->lines;pline<&sector->lines[sector->linecount];pline++)
			if (OnlySectorLines)
				pline=sector->lines;
			else
				line=lines;
			while (OnlySectorLines?(pline<&sector->lines[sector->linecount]):(line<&lines[numlines]))
            {
				if (OnlySectorLines)
					line=*pline;
                if ((OnlySectorLines||(line->frontsector==sector)||(line->backsector==sector))&&(line->frontsector!=line->backsector)&&!UsedLine(line))
                {
                    if ((line->v1->x==x)&&(line->v1->y==y)&&(line->frontsector==sector))
                    //if ((line->v1->x==x)&&(line->v1->y==y))
                    {
                        x=line->v2->x;
                        y=line->v2->y;
                        R_AddPlaneVertex(x, y, line);
                        break;
                    }
                    if ((line->v2->x==x)&&(line->v2->y==y)&&(line->backsector==sector))
                    //if ((line->v2->x==x)&&(line->v2->y==y))
                    {
                        x=line->v1->x;
                        y=line->v1->y;
                        R_AddPlaneVertex(x, y, line);
                        break;
                    }
                }
				if (OnlySectorLines)
					pline++;
				else
					line++;
            }
            if (OnlySectorLines?(pline==&sector->lines[sector->linecount]):(line==&lines[numlines]))
            {
				I_Printf("R_GeneratePlanes Failed(%d/%d)\n", vertexcount, sector->linecount);
#ifdef SHOWPOINTS
				for (i=0;i<sector->linecount;i++)
				{
					I_Printf("(%5d,%5d)-(%5d,%5d)\n", F2INT(sector->lines[i]->v1->x), F2INT(sector->lines[i]->v1->y), F2INT(sector->lines[i]->v2->x), F2INT(sector->lines[i]->v2->y));
				}
#endif
                //vertexcount=0;
                x=firstx;
                y=firsty;
            }
        }
        planes[n]=plane=Z_Malloc(sizeof(plane_t), PU_STATIC, NULL);
		if (vertexcount<3)
		{
			plane->numpoints=0;
			plane->points=NULL;
			plane->numindices=0;
			plane->indices=NULL;
			I_Printf("R_GeneratePlanes Failed(%d)\n", sector->linecount);
			continue;
		}
		if (vertexcount!=sector->linecount)
		{
			I_Printf("R_GeneratePlanes incomplete (%d/%d)\n", vertexcount, sector->linecount);
#ifdef SHOWPOINTS
			for (i=0;i<vertexcount;i++)
			{
				I_Printf(" %5d,%5d\n", F2INT(planevertices[i].x), F2INT(planevertices[i].y));
			}
#endif
		}
        plane->numpoints=vertexcount;
        plane->points=Z_Malloc(sizeof(DOOM3DVERTEX)*plane->numpoints, PU_STATIC, NULL);
        for (i=0, v=plane->points;i<vertexcount;v++, i++)
        {
            x=planevertices[i].x;
            y=planevertices[i].y;
            v->x=F2D3D(x);
            v->y=F2D3D(y);
            v->tu=F2D3D(x>>6);
            v->tv=F2D3D(-y>>6);
        }
        plane->numindices=0;
        plane->indices=(WORD *)Z_Malloc(sizeof(WORD)*(vertexcount-2)*3, PU_STATIC, NULL);
        if (!SplitPolygon(plane, false))
		{
			I_Printf("backwards\n");
			plane->numindices=0;
        	if (!SplitPolygon(plane, true))
			{
				plane->numpoints=0;
				I_Printf("SplitPolygon Failed Twice(%d)\n", vertexcount);
//#ifdef SHOWPOINTS
				for (i=0;i<vertexcount;i++)
				{
					I_Printf(" %5d,%5d\n", F2INT(planevertices[i].x), F2INT(planevertices[i].y));
				}
//#endif
			}
		}
    }
}

void R_CountSubsectorVerts(void)
{
	int			i;
	subsector_t	*sub;
	int			numverts;

	numverts=0;
	for (i=0, sub=subsectors;i<numsubsectors;i++, sub++)
	{
		if (sub->numlines>numverts)
			numverts=sub->numlines;
	}
	if (numverts>MaxSSVerts)
	{
		if (SSectorVertices)
			Z_Free(SSectorVertices);
		SSectorVertices=(DOOM3DVERTEX *)Z_Malloc(numverts*sizeof(DOOM3DVERTEX), PU_STATIC, NULL);
		MaxSSVerts=numverts;
	}
}
