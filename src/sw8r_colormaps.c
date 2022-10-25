//r_colormaps.c

#include <stdlib.h>
#include "doomdef.h"
#include "swr_local.h"
#include "sw8_defs.h"

int NumColorMaps=32;

pixel_t	**colormaps=NULL;

//
// R_InitColormaps
//
void R_InitColormaps (void)
{
    int	lump, length;
    byte	*data;
    int		i;

    // Load in the light tables,
    //  256 byte align tables.
    if (!colormaps)
    {
	lump = W_GetNumForName("COLORMAP");
	length = W_LumpLength (lump);
	data=Z_Malloc(length, PU_STATIC, NULL);
	colormaps=Z_Malloc(length*sizeof(pixel_t *)/256, PU_STATIC, NULL);
	W_ReadLump (lump, data);
	colormaps[0]=data;
	for (i=1;i<length/256;i++)
	    colormaps[i]=&colormaps[0][i*256];
    }
}
