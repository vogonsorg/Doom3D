#ifndef T_MD2_H
#define T_MD2_H
#include "doomdef.h"

typedef struct
{
	float	x;
	float	y;
	float	z;
	DWORD	color;
}modelvert_t;

typedef struct
{
	char			name[16];
	modelvert_t		v[1];
}modelframe_t;

typedef struct
{
	int		index;
	float	tu;
	float	tv;
}modeltris_t;

typedef struct model_s model_t;

struct model_s
{
	int				numframes;
	int				numverts;
	int				numtris;
	modeltris_t		*tris;
	int				skin;
	int				*cmds;
	model_t			*weapon;
//variable length
	modelframe_t	*frames[1];
};

typedef struct
{
	model_t	*model;
	int		numframes;
	int		frames[1];
}md2state_t;

#endif
