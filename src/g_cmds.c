//g_cmds.c
#include "g_local.h"
#include "doomstat.h"
#include "dstrings.h"
#include "i_system.h"
#include "m_cheat.h"

typedef struct
{
	char			*name;
	actionproc_t	proc;
	int				data;
}cmd_t;

void G_CmdButton(int data, char **param);
void G_CmdMouseMove(int data, char **param);
void G_CmdJoyMove(int data, char **param);
void G_CmdDigiJoyMove(int data, char **param);
void G_CmdWeapon(int data, char **param);
void G_CmdNextWeapon(int data, char **param);
void G_CmdAutorun(int data, char **param);
void G_CmdBind(int data, char **param);
void G_CmdQuit(int data, char **param);
void G_CmdExec(int data, char **param);
void G_CmdList(int data, char **param);
void G_CmdCheat(int data, char **param);
void G_CmdPause(int data, char **param);

cmd_t	Cmds[]=
{
	{"+fire", G_CmdButton, PCKEY_ATTACK},
	{"-fire", G_CmdButton, PCKEY_ATTACK|PCKF_UP},
	{"+strafe", G_CmdButton, PCKEY_STRAFE},
	{"-strafe", G_CmdButton, PCKEY_STRAFE|PCKF_UP},
	{"+use", G_CmdButton, PCKEY_USE},
	{"-use", G_CmdButton, PCKEY_USE|PCKF_UP},
	{"+run", G_CmdButton, PCKEY_RUN},
	{"-run", G_CmdButton, PCKEY_RUN|PCKF_UP},
	{"weapon", G_CmdWeapon, 0},
	{"nextwep", G_CmdNextWeapon, 0},
/*
	{"mousex", G_CmdMouseMove, 0},
	{"mousey", G_CmdMouseMove, 1},
	{"joyx", G_CmdJoyMove, 0},
	{"joyy", G_CmdJoyMove, 1},
	{"digijoyx", G_CmdDigiJoyMove, 0},
	{"digijoyx", G_CmdDigiJoyMove, 1},
*/
	{"+forward", G_CmdButton, PCKEY_FORWARD},
	{"-forward", G_CmdButton, PCKEY_FORWARD|PCKF_UP},
	{"+back", G_CmdButton, PCKEY_BACK},
	{"-back", G_CmdButton, PCKEY_BACK|PCKF_UP},
	{"+left", G_CmdButton, PCKEY_LEFT},
	{"-left", G_CmdButton, PCKEY_LEFT|PCKF_UP},
	{"+right", G_CmdButton, PCKEY_RIGHT},
	{"-right", G_CmdButton, PCKEY_RIGHT|PCKF_UP},
	{"autorun", G_CmdAutorun, 0},//convert to cvar
	{"+strafeleft", G_CmdButton, PCKEY_STRAFELEFT},
	{"-strafeleft", G_CmdButton, PCKEY_STRAFELEFT|PCKF_UP},
	{"+straferight", G_CmdButton, PCKEY_STRAFERIGHT},
	{"-straferight", G_CmdButton, PCKEY_STRAFERIGHT|PCKF_UP},
	{"bind", G_CmdBind, 0},
	{"quit", G_CmdQuit, 0},
	{"exec", G_CmdExec, 0},
	{"alias", G_CmdAlias, 0},
	{"cmdlist", G_CmdList, 0},
	{"unbind", G_CmdUnbind, 0},
	{"unbindall", G_CmdUnbindAll, 0},
	{"god", G_CmdCheat, CHEATCODE_GOD},
	{"noclip", G_CmdCheat, CHEATCODE_NOCLIP},
	{"pause", G_CmdPause, 0},
	{NULL, NULL}
};

void G_InitCmds(void)
{
	cmd_t	*cmd;

	for (cmd=Cmds;cmd->name;cmd++)
	{
		G_RegisterAction(cmd->name, cmd->proc, cmd->data);
	}
}

int G_Param2Split(char *p)
{
	if (!p)
		return(0);
	if (*p<'2')
		return(0);
	if (*p>='1'+MAX_SPLITS)
		return(MAX_SPLITS-1);
	return(*p-'1');
}

void G_CmdButton(int data, char **param)
{
	playercontrols_t	*pc;
	int					key;

	pc=&Controls[G_Param2Split(param[0])];

	key=data&PCKF_COUNTMASK;

	if (data&PCKF_UP)
	{
		if ((pc->key[key]&PCKF_COUNTMASK)>0)
			pc->key[key]--;
		if (ButtonAction)
			pc->key[key]&=~PCKF_DOUBLEUSE;
	}
	else
	{
		pc->key[key]++;
		if (ButtonAction)
			pc->key[key]|=PCKF_DOUBLEUSE;
	}
}

void G_DoCmdMouseMove(int split, int x, int y)
{
	playercontrols_t	*pc;

	pc=&Controls[split];
	pc->mousex+=x;
	pc->mousey+=y;
}

void G_DoCmdJoyMove(int split, int x, int y)
{
	playercontrols_t	*pc;

	pc=&Controls[split];
	pc->joyx=x;
	pc->joyy=y;
}

static void DigiJoyHelper(playercontrols_t *pc, int delta, int button1, int button2)
{
	if (delta>JOY_THRESHOLD)
	{
		pc->key[button1]&=~PCKF_JOYSTICK;
		pc->key[button2]|=PCKF_JOYSTICK;
	}
	else if (delta<-JOY_THRESHOLD)
	{
		pc->key[button1]|=PCKF_JOYSTICK;
		pc->key[button2]&=~PCKF_JOYSTICK;
	}
	else
	{
		pc->key[button1]&=~PCKF_JOYSTICK;
		pc->key[button2]&=~PCKF_JOYSTICK;
	}
}

void G_DoCmdDigiJoyMove(int split, int x, int y)
{
	playercontrols_t	*pc;

	pc=&Controls[split];
	DigiJoyHelper(pc, x, PCKEY_LEFT, PCKEY_RIGHT);
	DigiJoyHelper(pc, y, PCKEY_FORWARD, PCKEY_BACK);
}

void G_CmdNextWeapon(int data, char **param)
{
	playercontrols_t	*pc;

	pc=&Controls[G_Param2Split(param[0])];
	pc->flags|=PCF_NEXTWEAPON;
}

void G_CmdWeapon(int data, char **param)
{
	playercontrols_t	*pc;
	int					split;
	char				*s;

	s=param[0];
	if (!s)
		return;
	if ((s[0]<'1')||(s[0]>='1'+NUMWEAPONS))
		return;
	split=s[1]-'1';
	if (split<0)
		split=0;
	if (split>=MAX_SPLITS)
		split=MAX_SPLITS-1;
	pc=&Controls[split];
	pc->nextweapon=s[0]-'1';
}

void G_CmdBind(int data, char **param)
{
	if (!param[0])
		return;

	if (!param[1])
	{
		G_ShowBinding(param[0]);
		return;
	}
	G_BindActionByName(param[0], param[1]);
}

void G_CmdAutorun(int data, char **param)
{
	int		split;
	int		mask;

	split=G_Param2Split(param[0]);
	mask=1<<split;
	Autorun^=mask;
	if (Autorun&mask)
		players[consoleplayer+split].message=GGAUTORUNON;
	else
		players[consoleplayer+split].message=GGAUTORUNOFF;
}

void G_CmdQuit(int data, char **param)
{
	I_Quit();
}

void G_CmdExec(int data, char **param)
{
	G_ExecuteFile(param[0]);
}

void G_CmdList(int data, char **param)
{
	int			cmds;

	I_Printf("Available commands:\n");
	cmds=G_ListCommands();
	I_Printf("(%d commands)\n", cmds);
}

void G_CmdCheat(int data, char **param)
{
}

void G_CmdPause(int data, char **param)
{
	sendpause=true;
}
