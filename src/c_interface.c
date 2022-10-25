//c_interface.c

#include "doomdef.h"
#include "common.h"
#include "co_console.h"
#include "i_system.h"
#include "w_wad.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_windoz.h"
#include "m_random.h"
#include "m_argv.h"
#include "doomstat.h"
#include "p_local.h"
#include "d_main.h"
#include "sp_split.h"
#include "info.h"
#include "l_precalc.h"

apiout_t	C_API;
HINSTANCE	hDll=NULL;
dboolean	interfacevalid=false;
char		*DllName;
dboolean	AllowGLNodes=false;
LPGUID		pD3DDeviceGUID=NULL;


void C_InitDAPI(apiin_t *apiin)
{
    apiin->size=sizeof(apiin_t);
    apiin->version=D_API_VERSION;
    apiin->I_Printf=I_Printf;
    apiin->I_Error=I_Error;
    apiin->Z_Malloc=Z_Malloc;
    apiin->Z_Free=Z_Free;
    apiin->Z_ChangeTag2=Z_ChangeTag2;
    apiin->W_CacheLumpNum=W_CacheLumpNum;
    apiin->W_CacheLumpName=W_CacheLumpName;
    apiin->W_CheckNumForName=W_CheckNumForName;
    apiin->W_GetNumForName=W_GetNumForName;
    apiin->W_LumpLength=W_LumpLength;
    apiin->W_ReadLump=W_ReadLump;
    apiin->M_Random=M_Random;
    apiin->P_MobjThinker=P_MobjThinker;
    apiin->D_IncValidCount=D_IncValidCount;
    apiin->S_GetViewPos=S_GetViewPos;
	apiin->L_CanSee=L_CanSee;
    apiin->NetUpdate=NetUpdate;
    apiin->thinkercap=&thinkercap;
    apiin->gamemode=&gamemode;
    apiin->ptextureheight=&textureheight;
    apiin->ptexturetranslation=&texturetranslation;
    apiin->pflattranslation=&flattranslation;
    apiin->modifiedgame=&modifiedgame;
    apiin->pMainWnd=&hMainWnd;
    apiin->pGammaLevel=&GammaLevel;
	apiin->pWindowPos=&WindowPos;
	apiin->pShowGun=&ShowGun;
	apiin->pInBackground=&InBackground;
	apiin->states=states;
	apiin->players=players;
	apiin->NumSplits=NumSplits;
	apiin->pConsolePos=&ConsolePos;
	apiin->ppD3DDeviceGUID=&pD3DDeviceGUID;
	apiin->pFixSprites=&FixSprites;
	apiin->pNumSSDrawn=&NumSSDrawn;
}

void C_ProcessCAPI(apiout_t *apiout)
{
	if (InWindow&&!(apiout->info->caps&APICAP_WINDOW))
		InWindow=false;
	if (!(InWindow||(apiout->info->caps&APICAP_FULLSCREEN)))
	{
		if (apiout->info->caps&APICAP_WINDOW)
			InWindow=true;
		else
			I_Error("Neither Windowed or fullscreen mode supported");
	}
	if ((apiout->info->caps&APICAP_POLYBASED)&&!M_CheckParm("-nogfn"))
		AllowGLNodes=true;
}

void C_Init(void)
{
    apiin_t	apiin;
    int		rc;
    int (__stdcall *DoomAPI)(apiin_t *apiin, apiout_t *apiout);
	char	*errstr;
	int		p;
	char	*name;

    C_API.size=sizeof(apiout_t);
    C_API.version=C_API_VERSION;
    C_API.FreeDoomAPI=NULL;

    p=M_CheckParm("-dll");
    if (p&&(p<myargc-1))
    	name=myargv[p+1];
    else
    	name=DllName;

	hDll=LoadLibrary(name);
    if (!hDll)
		I_Error("Unable to load %s", name);
    (void *)DoomAPI=GetProcAddress(hDll, "DoomAPI");
    if (!DoomAPI)
		I_Error("error in %s (DoomAPI not found)", name);
    C_InitDAPI(&apiin);
    rc=DoomAPI(&apiin, &C_API);
    if (rc)
	{
		switch (rc)
		{
		case APIERR_NULLPARM:
		case APIERR_NULLPARM2:
		case APIERR_WRONGSIZE:
			errstr="Invalid Parameters";
			break;
		case APIERR_WRONGVERSION:
			errstr="Incorrect Version";
			break;
		default:
			errstr="Unknown Error";
			break;
		}
		I_Error("%s::DoomAPI Failed(%s)", name, errstr);
	}
    if ((C_API.size!=sizeof(apiout_t))||(C_API.version!=C_API_VERSION))
		I_Error("%s interface is wrong version", name);
    interfacevalid=true;
    C_ProcessCAPI(&C_API);
	I_Printf("C_Init: %s \"%s\"\n", name, C_API.info->name);
}

void C_Shutdown(void)
{
    if (interfacevalid&&C_API.FreeDoomAPI)
		C_API.FreeDoomAPI();
//FUDGE: for some reason this crashes...
    if (hDll)
		FreeLibrary(hDll);
    hDll=NULL;
    interfacevalid=false;
}
