#ifndef D3DR_STATE_H
#define D3DR_STATE_H

#include "t_fixed.h"

extern int		numvertexes;
extern vertex_t		*vertexes;
extern int		numsegs;
extern seg_t		*segs;
extern int		numsectors;
extern sector_t		*sectors;
extern int		numsubsectors;
extern subsector_t	*subsectors;
extern int		numnodes;
extern node_t		*nodes;
extern int		numlines;
extern line_t		*lines;
extern int		numsides;
extern side_t		*sides;

extern fixed_t		viewx;
extern fixed_t		viewy;
extern float		fviewx;
extern float		fviewy;
extern float		fviewz;
extern angle_t		viewangle;

extern angle_t		viewangleoffset;

#endif