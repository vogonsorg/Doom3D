#ifndef G_CONTROLS_H
#define G_CONTROLS_H

#include "doomdef.h"
//#include "t_keys.h"
#include "t_split.h"

#define NUMKEYS		256

#define PCKF_JOYSTICK	0x8000
#define PCKF_DOUBLEUSE	0x4000
#define PCKF_UP			0x8000
#define PCKF_COUNTMASK	0x00ff

typedef enum
{
	PCKEY_ATTACK,
	PCKEY_USE,
	PCKEY_STRAFE,
	PCKEY_FORWARD,
	PCKEY_BACK,
	PCKEY_LEFT,
	PCKEY_RIGHT,
	PCKEY_STRAFELEFT,
	PCKEY_STRAFERIGHT,
	PCKEY_RUN,
	NUM_PCKEYS
}pckeys_t;

typedef struct
{
	int			mousex;
	int			mousey;
	int			joyx;
	int			joyy;
	int			key[NUM_PCKEYS];
	int			nextweapon;
	int			sdclicktime;
	int			fdclicktime;
	int			flags;
}playercontrols_t;

#define PCF_NEXTWEAPON	0x01
#define PCF_FDCLICK		0x02
#define PCF_FDCLICK2	0x04
#define PCF_SDCLICK		0x08
#define PCF_SDCLICK2	0x10

extern playercontrols_t	Controls[MAX_SPLITS];
extern char *ConfigFileName;

#endif
