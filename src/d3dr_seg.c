//d3dr_seg.c

#include "d3dr_local.h"
#include "d3di_ddraw.h"

#include "c_d3d.h"

#include <math.h>

D3DVALUE	*SegLengths;

void R_CalcSegLengths(void)
{
    seg_t	*seg;
    D3DVALUE	*length;
    D3DVALUE	x;
    D3DVALUE	y;
	
    SegLengths=(D3DVALUE *)Z_Malloc(sizeof(D3DVALUE)*numsegs, PU_STATIC, NULL);
    for (seg=segs, length=SegLengths;seg<&segs[numsegs];seg++, length++)
    {
		x=F2D3D(seg->v1->x-seg->v2->x);
		y=F2D3D(seg->v1->y-seg->v2->y);
		*length=(D3DVALUE)sqrt(x*x+y*y);
    }
}