#ifndef D3DR_MD2_H
#define D3DR_MD2_H

#include "doomdef.h"
#include "t_md2.h"
#include "d3di_ddraw.h"

typedef struct
{
	byte	id;//0x0A
	byte	ver;//5
	byte	enc;
	byte	bpp;
	short	x0;
	short	y0;
	short	x1;
	short	y1;
	short	hres;
	short	vres;
	byte	colormap[48];
	byte	res1;
	byte	planes;
	short	linelen;
	short	palinfo;
	byte	res2[58];
}pcxheader_t;

typedef struct
{
	char	*name;
	int		skin;
}playerskin_t;

void R_InitMD2(char *name);
void R_SafeRead(void *buff, int size, FILE *fh);

extern DOOM3DVERTEX	*ModelVertexBuffer;
extern playerskin_t	PlayerSkins[MAXPLAYERS];

#endif
