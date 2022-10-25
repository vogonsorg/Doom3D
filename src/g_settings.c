//g_settings.c
#include "g_local.h"
#include "z_zone.h"
#include "hu_font.h"
#include "c_interface.h"
#include "d_stuff.h"
#include "m_argv.h"

char	*ConfigFileName="config.cfg";

char	DefaultConfig[]=
#include "defconfig.inc"
;

void G_ExecuteMultipleCommands(char *data)
{
	char	*p;
	char	*q;
	char	c;
	char	line[1024];

	p=data;
	c=*p;
	while (c)
	{
		q=line;
		c=*(p++);
		while (c&&(c!='\n'))
		{
			if (c!='\r')
				*(q++)=c;
			c=*(p++);
		}
		*q=0;
		if (line[0])
			G_ExecuteCommand(line);
	}
}

dboolean G_ExecuteFile(char *name)
{
	FILE	*fh;
	char	*buff;
	int		len;

	if (!name)
		return(false);
	fh=fopen(name, "rb");
	if (!fh)
	{
		I_Printf("Unable to execute %s\n", name);
		return(false);
	}
	I_Printf("Executing %s\n", name);
	fseek(fh, 0, SEEK_END);
	len=ftell(fh);
	fseek(fh, 0, SEEK_SET);
	buff=Z_Malloc(len+1, PU_STATIC, NULL);
	fread(buff, 1, len, fh);
	buff[len]=0;
	G_ExecuteMultipleCommands(buff);
	Z_Free(buff);
	return(true);
}

void G_LoadSettings(void)
{
	int		p;

	ConfigFileName="config.cfg";
	p=M_CheckParm("-config");
	if (p&&(p<myargc-1))
	{
		if (myargv[p+1][0]!='-')
			ConfigFileName=myargv[p+1];
	}
	if (!G_ExecuteFile(ConfigFileName))
	{
		G_ExecuteMultipleCommands(DefaultConfig);
	}
	G_ExecuteFile("autoexec.cfg");
}
