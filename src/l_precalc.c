//l_precalc.c
//PVS calculation stuff

#include "doomstat.h"
#include "w_wad.h"
#include "z_zone.h"
#include "m_fixed.h"
#include "m_swap.h"
#include "m_sidetest.h"
#include "i_system.h"

#define PVS_BLOCK_SIZE				0x20000
#define MAX_SUBSECTORS				1024
#define MAX_PORTALS					15
#define MAX_PVS_TESTS				2000

typedef struct
{
	vertex_t	*v1;
	vertex_t	*v2;
}portal_t;

typedef struct
{
	int		numsectors;
	int		checksum;
	byte	*data[1];
}pvslump_t;

sector_t	**stack;
portal_t	*portals;

//128k*8bits=>max 1024 subsectors
//byte		PVSBlock[PVS_BLOCK_SIZE];
byte	*PVSBlock=NULL;

static int	NumTests;

//FUDGE:remove mask variable from PVS tests, and make them inline
void L_MarkAsVisible(sector_t *ss1, sector_t *ss2)
{
	int		i1;
	int		i2;
	byte	mask;

	i1=ss1-sectors;
	i2=ss2-sectors;
	mask=0x1<<(i1&0x7);
	PVSBlock[(i2<<7)+((i1>>3))]|=mask;
	mask=0x1<<(i2&0x7);
	PVSBlock[(i1<<7)+((i2>>3))]|=mask;
}


int L_CanSee(int s1, int s2)
{
	byte	mask;

	mask=0x1<<(s1&0x7);
	if (mask&PVSBlock[(s2<<7)+((s1>>3))])
		return(true);
	else
		return(false);
}

static vertex_t	lv[MAX_PORTALS*2];
static vertex_t	rv[MAX_PORTALS*2];

#define VLESS(a, b) ((v[a].x<v[b].x)||((v[a].x==v[b].x)&&(v[a].y<v[b].y)))
#define VSWAP(a, b) {tx=v[a].x;ty=v[a].y;v[a].x=v[b].x;v[a].y=v[b].y;v[b].x=tx;v[b].y=ty;}

void L_SortVerts(vertex_t *v, int l, int r)
{
	int		mid;
	int		a;
	int		b;
	fixed_t	tx;
	fixed_t	ty;
	fixed_t	x;
	fixed_t	y;

	if (VLESS(r, l))
		VSWAP(r, l);
	if (r==l+1)
		return;
	mid=(l+r)/2;
	if (VLESS(r, mid))
		VSWAP(r, mid);
	if (VLESS(mid, l))
		VSWAP(mid, l);
	if (r-l<=2)
		return;
	x=v[mid].x;
	y=v[mid].y;
	a=l+1;
	b=r-1;
	while (true)
	{
		while (((v[a].x<=x)||((v[a].x==x)&&(v[a].y<=y)))&&(a<b))
			a++;
		while (((v[b].x>x)||((v[b].x==x)&&(v[b].y>y)))&&(a<b))
			b--;
		if (b==a)
			break;
		VSWAP(a, b);
	}
	if (l<a)
		L_SortVerts(v, l, a);
	if (a<r-1)
		L_SortVerts(v, a+1, r);
}
#undef VSWAP
#undef VLESS

//points must be >1, this should be checked by calling procedure though...
static int BuildConvexHull(vertex_t *v, int points)
{
	int			n;
	int			i;
	vertex_t	hv[MAX_PORTALS*2];

	n=2;
	hv[0]=v[0];
	i=0;
	while ((i<points)&&(v[i].x==v[0].x)&&(v[i].y==v[0].y))
		i++;
	if (i==points)
		return(0);
	hv[1]=v[i++];

	//hv[1]=v[1];
	for (;i<points;i++)
	{
		if ((v[i-1].x==v[i].x)&&(v[i-1].y==v[i].y))
			continue;
		hv[n]=v[i];
		while (n>=2)
		{
			//check if clockwise
			if (M_OnSide(hv[n].x-hv[n-2].x, hv[n].y-hv[n-2].y, hv[n-1].x-hv[n-2].x, hv[n-1].y-hv[n-2].y))
				break;
			hv[n-1]=hv[n];
			n--;
		}
		n++;
	}

	i=points-2;
	while ((i>=0)&&(v[i].x==v[i+1].x)&&(v[i].y==v[i+1].y))
		i--;
	if (i<0)
		return(n);
	hv[n++]=v[i--];

//	hv[n++]=v[points-2];
	for (;i>=0;i--)
	{
		if ((v[i+1].x==v[i].x)&&(v[i+1].y==v[i].y))
			continue;
		hv[n]=v[i];
		while (n>=2)
		{
			//check if clockwise
			if (M_OnSide(hv[n].x-hv[n-2].x, hv[n].y-hv[n-2].y, hv[n-1].x-hv[n-2].x, hv[n-1].y-hv[n-2].y))
				break;
			hv[n-1]=hv[n];
			n--;
		}
		n++;
	}
	n--;//remove last point as it's a duplicate of first one
	memcpy(v, hv, n*sizeof(vertex_t));
	return(n);
}

static dboolean DoIntersect(vertex_t *v1, vertex_t *v2)
{
	fixed_t		dx;
	fixed_t		dy;
	dboolean	left;

	if ((v1[0].x>v2[1].x)||(v1[1].x<v2[0].x))
		return(false);
	if ((v1[0].y>v2[0].y)&&(v1[1].y>v2[0].y)&&(v1[0].y>v2[1].y)&&(v1[1].y>v2[1].y))
		return(false);
	if ((v1[0].y<v2[0].y)&&(v1[1].y<v2[0].y)&&(v1[0].y<v2[1].y)&&(v1[1].y<v2[1].y))
		return(false);

	dx=v1[1].x-v1[0].x;
	dy=v1[1].y-v1[0].y;
	left=M_OnSide(dx, dy, v1[0].x-v2[0].x, v1[0].y-v2[0].y);
	if (left==M_OnSide(dx, dy, v1[0].x-v2[1].x, v1[0].y-v2[1].y))
		return(false);
	dx=v2[1].x-v2[0].x;
	dy=v2[1].y-v2[0].y;
	left=M_OnSide(dx, dy, v2[0].x-v1[0].x, v2[0].y-v1[0].y);
	if (left==M_OnSide(dx, dy, v2[0].x-v1[1].x, v2[0].y-v1[1].y))
		return(false);
	return(true);
}

//assumes v1[0]<v1[1] and v2[0]<v2[1] (x<x)||((x==x)&&(y<y)
static dboolean DoIntersect2(vertex_t *v1, vertex_t *v2)
{
	int		c1;
	int		c2;
	int		m1;
	int		m2;
	int		x;
/*
	y=mx+c  (x1, y1)-(x2, y2)

	m=(y1-y2)/(x1-x2) ***
	
	y1=mx1+c
	c=y1-mx1 ***

	y=mx+c
	y=nx+d

	mx+c=nx+d
	x(m-n)=d-c
	x=(d-c)/(m-n) ***

	solve
*/
	//quick reject
	if ((v1[0].x>v2[1].x)||(v1[1].x<v2[0].x))
		return(false);
	if ((v1[0].y>v2[0].y)&&(v1[1].y>v2[0].y)&&(v1[0].y>v2[1].y)&&(v1[1].y>v2[1].y))
		return(false);
	if ((v1[0].y<v2[0].y)&&(v1[1].y<v2[0].y)&&(v1[0].y<v2[1].y)&&(v1[1].y<v2[1].y))
		return(false);

	if ((v1[0].x==v1[1].x))
	{
		if ((v2[0].x<v1[0].x)&&(v2[1].x>v1[0].x))
		{
			return(true);
		}
		return(false);
	}
	if ((v2[0].x==v2[1].x))
	{
		if ((v1[0].x<v2[0].x)&&(v1[1].x>v2[0].x))
		{
			return(true);
		}
		return(false);
	}
	
	m1=FixedDiv(v1[0].y-v1[1].y, v1[0].x-v1[1].x);
	m2=FixedDiv(v2[0].y-v2[1].y, v2[0].x-v2[1].x);
	c1=v1[0].y-FixedMul(m1, v1[0].x);
	c2=v2[0].y-FixedMul(m2, v2[0].x);
	x=FixedDiv(c2-c1, m1-m2);
	if ((x<v1[0].x)||(x>v1[1].x)||(x<v2[0].x)||(x>v2[1].x))
		return(false);
	return(true);
}

dboolean L_LinesIntersect(vertex_t *v1, int n1, vertex_t *v2, int n2)
{
	int		a;
	int		b;

	a=0;
	b=0;
	while ((a<n1)&&(v1[a+1].x<v2[b].x))
		a++;
	while ((b<n2)&&(v2[b+1].x<v1[a].x))
		b++;
	while ((a<n1)&&(b<n2))
	{
		if (DoIntersect(&v1[a], &v2[b]))
			return(true);
		if (v1[a+1].x<v2[b+1].x)
			a++;
		else
			b++;
	}
	return(false);
}

//not entirely sure if this is OK with vertical lines:(
dboolean L_HullsIntersect(vertex_t *lv, int lh, vertex_t *rv, int rh)
{
	int			lmid;//no. of line segments on top of hull
	int			rmid;
	int			i;
	vertex_t	v1[MAX_PORTALS*2];
	vertex_t	v2[MAX_PORTALS*2];

	for (lmid=1;lmid<lh-1;lmid++)
	{
		if ((lv[lmid+1].x<lv[lmid].x)||((lv[lmid+1].x==lv[lmid].x)&&(lv[lmid+1].y<lv[lmid].y)))
			break;
	}
	v1[0]=lv[0];
	for (i=1;i<=lh-lmid;i++)
	{
		v1[i]=lv[lh-i];
	}

	for (rmid=1;rmid<rh-1;rmid++)
	{
		if ((rv[rmid+1].x<rv[rmid].x)||((rv[rmid+1].x==rv[rmid].x)&&(rv[rmid+1].y<rv[rmid].y)))
			break;
	}
	v1[0]=rv[0];
	for (i=1;i<=rh-rmid;i++)
	{
		v1[i]=rv[rh-i];
	}


	if (L_LinesIntersect(lv, lmid, rv, rmid))
		return(true);
	if (L_LinesIntersect(lv, lmid, v2, rh-rmid))
		return(true);
	if (L_LinesIntersect(v1, lh-lmid, v2, rh-rmid))
		return(true);
	if (L_LinesIntersect(v1, lh-lmid, rv, rmid))
		return(true);
	return(false);
}

static dboolean CanSeeThrough(int levels)
{
	int			r;
	int			lh;
	int			rh;
	static int	count=0;
	fixed_t		x;
	fixed_t		y;

	if (levels<3)
		return(true);

	x=portals[0].v1->x;
	y=portals[0].v1->y;
	for (r=0;r<levels;r++)
	{
		lv[r].x=portals[r].v1->x-x;
		lv[r].y=portals[r].v1->y-y;
		rv[r].x=portals[r].v2->x-x;
		rv[r].y=portals[r].v2->y-y;
	}

	L_SortVerts(lv, 0, levels-1);
	L_SortVerts(rv, 0, levels-1);

	//build convex hull, then check for intersection
	lh=BuildConvexHull(lv, levels);
	rh=BuildConvexHull(rv, levels);

	if ((lh>levels*2)||(rh>levels*2))
		I_Error("Overflow");
	if ((lh<2)||(rh<2))
		return(true);

	if (L_HullsIntersect(lv, lh, rv, rh))
		return(false);
	return(true);
}

static int DoSector(sector_t *s, int level)
{
	line_t		*line;
	int			max;
	int			l;
	int			i;
	sector_t	*partner;

	max=level;
	if (level>=MAX_PORTALS)
		return(level);

	stack[level]=s;
	for (l=0;l<s->linecount;l++)
	{
		line=s->lines[l];
		if (!line->backsector)
			continue;

		if (line->frontsector==s)
		{
			partner=line->backsector;
			portals[level].v1=line->v2;
			portals[level].v2=line->v1;
		}
		else
		{
			partner=line->frontsector;
			portals[level].v1=line->v1;
			portals[level].v2=line->v2;
		}
		for (i=0;i<level;i++)
		{
			if (stack[i]==partner)
				break;
		}
		if (i<level)
			continue;
		if (CanSeeThrough(level+1))
		{
			NumTests++;
			if (NumTests>MAX_PVS_TESTS)
				return(-1);
			L_MarkAsVisible(stack[0], partner);
			i=DoSector(partner, level+1);
			if (i>max)
				max=i;
			if (i==-1)
				return(-1);
		}
	}
	return(max);
}

void L_AllocPVS(void)
{
	if (numsectors>MAX_SUBSECTORS)
		I_Error("Too many sectors for PVS");
	if (!PVSBlock)
		PVSBlock=(byte *)Z_Malloc(PVS_BLOCK_SIZE, PU_STATIC, NULL);
}

void L_ClearPVS(void)
{
	L_AllocPVS();
	memset(PVSBlock, 0xff, PVS_BLOCK_SIZE);
}

dboolean L_LoadPVS(int map, int maplump)
{
	int			lump;
	char		name[6];
	pvslump_t	*pvs;
	int			i;
	int			l;

	L_AllocPVS();
	strcpy(name, "PVS00");
	name[3]=map/10+'0';
	name[4]=map/10+'0';
	lump=W_CheckNumForName(name);
	if (lump==-1)
		return(false);
	if (lump<maplump)
		return(false);
	pvs=(pvslump_t *)W_CacheLumpNum(lump, PU_CACHE);
	if ((LevelChecksum!=pvs->checksum)||(numsectors!=pvs->numsectors))
		return(false);

	l=(numsectors+7)/8;
	for (i=0;i<numsectors;i++)
		memcpy(&PVSBlock[i<<7], &pvs->data[l*i], l);
	return(true);
}

void L_CalcPVS(void)
{
	int		i;
	int		depth;

	L_AllocPVS();
	stack=(sector_t **)Z_Malloc(MAX_PORTALS*sizeof(sector_t *), PU_STATIC, NULL);
	portals=(portal_t *)Z_Malloc(MAX_PORTALS*sizeof(sector_t), PU_STATIC, NULL);

	ZeroMemory(PVSBlock, PVS_BLOCK_SIZE);
	I_Printf("Performing PVS on %d sectors\n", numsectors);
	for (i=0;i<numsectors;i++)
	{
		NumTests=0;
		L_MarkAsVisible(&sectors[i], &sectors[i]);
		I_Printf("#%d", i);
		depth=DoSector(&sectors[i], 0);
		I_Printf("\n");
		if (depth==-1)//give up cos it's taking too long
		{
			for (depth=0;depth<numsubsectors;depth++)
				L_MarkAsVisible(&sectors[i], &sectors[depth]);
		}
	}
	Z_Free(portals);
	Z_Free(stack);
}
