#ifndef APIINFO_H
#define APIINFO_H

#include "doomver.h"
#include "t_misc.h"

#define APICAP_HW				0x0001
#define APICAP_WINDOW			0x0002
#define APICAP_FULLSCREEN		0x0004
#define APICAP_FLOATINGAUTOMAP	0x0008
#define APICAP_MATRIX			0x0010
#define APICAP_POLYBASED		0x0020
#define APICAP_D3DDEVICE		0x0040

#define API_OK				0
#define APIERR_NULLPARM		1
#define APIERR_NULLPARM2	2
#define APIERR_WRONGSIZE	3
#define APIERR_WRONGVERSION	4

typedef struct
{
    char		*name;
    int			caps;
	default_t	*defaults;
}dllinfo_t;

typedef struct
{
    int		size;
    int		version;
    void (__stdcall *FreeDoomAPI)(void);
	dllinfo_t	*info;
}apiinfo_t;

typedef int (_stdcall *DoomAPI_t)(apiinfo_t *apiinfo, void *reserved);

#define NUM_NETREG_NODES	7//MAXNETNODES-1

#define NDF_DRONE		0x01
#define NDF_LEFT		0x02
#define NDF_RIGHT		0x04
#define NDF_BACK		0x08
#define NDF_DEATHMATCH	0x10
#define NDF_DEATH2		0x20
#define NDF_SPLITONLY	0x40

//int __stdcall DoomAPI(apiinfo_t *apiinfo, void *reserved=NULL);

#endif
