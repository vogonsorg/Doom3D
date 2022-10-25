#ifndef T_ITEMS_H
#define T_ITEMS_H

#include "doomdef.h"

typedef struct
{
    ammotype_t	ammo;
    int		upstate;
    int		downstate;
    int		readystate;
    int		atkstate;
    int		flashstate;

} weaponinfo_t;

#endif