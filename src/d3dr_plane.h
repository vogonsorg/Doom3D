#ifndef D3DR_PLANE_H
#define D3DR_PLANE_H

#include "d3di_ddraw.h"

typedef struct
{
    int				numpoints;
    DOOM3DVERTEX	*points;
    int				numindices;
    WORD			*indices;
} plane_t;

void R_GeneratePlanes(void);
void R_CountSubsectorVerts(void);

extern plane_t	**planes;
extern dboolean	UseGLNodes;

#endif