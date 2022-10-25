//r_colormaps.c

#include "doomdef.h"
#include "swr_local.h"
#include "sw16_defs.h"

int NumColorMaps=32;

pixel_t	**colormaps=NULL;

//
// R_InitColormaps
//
void R_InitColormaps (void)
{
    int		i;

    if (!colormaps)
    {
	colormaps=Z_Malloc(sizeof(pixel_t *)*(NumColorMaps+1), PU_STATIC, NULL);
	colormaps[0]=Z_Malloc(PIX2BYTE(256*(NumColorMaps+1)), PU_STATIC, NULL);
	for (i=1;i<=NumColorMaps;i++)
	    colormaps[i]=&colormaps[0][i*256];
    }
}
