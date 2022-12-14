#ifndef T_PSPR_H
#define T_PSPR_H

#include "t_fixed.h"
#include "t_info.h"

//
// Frame flags:
// handles maximum brightness (torches, muzzle flare, light sources)
//
#define FF_FULLBRIGHT	0x8000	// flag in thing->frame
#define FF_FRAMEMASK	0x7fff



//
// Overlay psprites are scaled shapes
// drawn directly on the view screen,
// coordinates are given for a 320*200 view screen.
//
typedef enum
{
    ps_weapon,
    ps_flash,
    NUMPSPRITES

} psprnum_t;

typedef struct
{
    state_t*	state;	// a NULL state means not active
    int		tics;
	int		frame;
    fixed_t	sx;
    fixed_t	sy;

} pspdef_t;

#endif