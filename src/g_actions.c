//g_actions.c
//action parsing

#include "g_local.h"
#include "m_keys.h"
#include "z_zone.h"
#include "i_system.h"
#include "i_input.h"
#include "doomstat.h"

#include <string.h>
#include <stdlib.h>

//FUDGE: do controls menu length properly
//if list of actions for menu>=MAX_MENUACTION_LENGTH then won't display any more
#define MAX_MENUACTION_LENGTH 25

#define MAX_CURRENTACTIONS	16

typedef struct action_s action_t;

struct action_s
{
	char			*name;
	actionproc_t	proc;
	action_t		*children[2];
	action_t		*parent;
	int				data;
};

static action_t	*Actions=NULL;

typedef struct alist_s alist_t;

struct alist_s
{
	char		*buff;
	char		*cmd;
	alist_t		*next;
	int			refcount;
//should allocate to required size?
	char		*param[MAX_ACTIONPARAM+1];//NULL terminated list
};

void G_RunAlias(int data, char **param);
void G_DoOptimizeActionTree(void);

alist_t	*CurrentActions[MAX_CURRENTACTIONS];

//these must be in the order key, joy, mouse, mouse2, other
#define KEY_ACTIONPOS		0
#define JOY_ACTIONPOS		NUMKEYS
#define MOUSE_ACTIONPOS		(JOY_ACTIONPOS+JOY_BUTTONS)
#define MOUSE2_ACTIONPOS	(MOUSE_ACTIONPOS+MOUSE_BUTTONS)
#define OTHER_ACTIONPOS		(MOUSE2_ACTIONPOS+MOUSE_BUTTONS)
#define NUM_ACTIONS			(OTHER_ACTIONPOS+OTHER_BUTTONS)

alist_t	*AllActions[NUM_ACTIONS];

alist_t	**KeyActions;
alist_t	**JoyActions;
alist_t	**MouseActions;
alist_t	**Mouse2Actions;
alist_t	**OtherActions;//to allow for other input devices:)

static int	JoyButtons=0;
static int	MouseButtons=0;
static int	Mouse2Buttons=0;

static dboolean	OptimizeTree=false;
dboolean		ButtonAction=false;

void G_InitActions(void)
{
	ZeroMemory(AllActions, NUM_ACTIONS);
	KeyActions=AllActions+KEY_ACTIONPOS;
	JoyActions=AllActions+JOY_ACTIONPOS;
	MouseActions=AllActions+MOUSE_ACTIONPOS;
	Mouse2Actions=AllActions+MOUSE2_ACTIONPOS;
	OtherActions=AllActions+OTHER_ACTIONPOS;

	ZeroMemory(CurrentActions, MAX_CURRENTACTIONS*sizeof(alist_t *));
}

static action_t *FindAction(char *name)
{
	action_t	*tree;
	int			cmp;

	if (!name)
		return(NULL);
	tree=Actions;
	while (tree)
	{
		cmp=strcmp(name, tree->name);
		if (cmp==0)
			break;
		if (cmp>0)
			tree=tree->children[1];
		else
			tree=tree->children[0];
	}
	if (!tree)
		I_Printf("Unknown command \"%s\"\n", name);
	return(tree);
}

char *SkipWhitespace(char *p)
{
	while (*p&&isspace(*p))
		p++;
	return(p);
}

char *FindWhitespace(char *p)
{
	while (*p&&!isspace(*p))
		p++;
	return(p);
}

alist_t *DuplicateActionList(alist_t *al)
{
	alist_t	*p;

	p=al;
	while (p)
	{
		p->refcount++;
		p=p->next;
	}
	return(al);
}

void AddActions(alist_t *actions)
{
	int		slot;

	if (!actions)
		return;

	for (slot=0;slot<MAX_CURRENTACTIONS;slot++)
	{
		if (!CurrentActions[slot])
		{
			CurrentActions[slot]=DuplicateActionList(actions);
			break;
		}
	}
	if (slot==MAX_CURRENTACTIONS)
		I_Printf("command overflow\n");
}

void DerefSingleAction(alist_t *al)
{
	al->refcount--;
	if (al->refcount<=0)
	{
		if (al->buff)
		{
			if (al->next)
				al->next->buff=al->buff;
			else
				Z_Free(al->buff);
		}
		Z_Free(al);
	}
}

alist_t *DoRunActions(alist_t *al, dboolean free)
{
	alist_t		*next;
	action_t	*action;

	while (al)
	{
		next=al->next;
		if (strcmp(al->cmd, "wait")==0)
			break;

		action=FindAction(al->cmd);
		if (action)
			action->proc(action->data, al->param);
		if (free)
			DerefSingleAction(al);
		al=next;
	}
	if (al)//reached wait command
	{
		if (free)
			DerefSingleAction(al);
		return(next);
	}
	return(al);
}

void TryActions(alist_t *al, dboolean up)
{
	if (!al)
		return;

	if (up)
	{
		action_t	*action;
		char	buff[256];

		if (al->next||(al->cmd[0]!='+'))
			return;

		strcpy(buff, al->cmd);
		buff[0]='-';
		action=FindAction(buff);
		if (action)
			action->proc(action->data, al->param);
		return;
	}
	AddActions(DoRunActions(al, false));
}

void DerefActionList(alist_t *al)
{
	alist_t	*next;

	while (al)
	{
		next=al->next;
		al->refcount--;
		if (al->refcount<=0)
		{
			if (al->buff)
				Z_Free(al->buff);
			Z_Free(al);
		}
		al=next;
	}
}

void G_ActionTicker()
{
	int		slot;

	for (slot=0;slot<MAX_CURRENTACTIONS;slot++)
	{
		if (CurrentActions[slot])
			CurrentActions[slot]=DoRunActions(CurrentActions[slot], true);
	}
//FUDGE: re-enable tree optimization-don't know why disabled!
	if (OptimizeTree)
		G_DoOptimizeActionTree();
}

static void ProcessButtonActions(alist_t **actions, int b, int ob)
{
	int			mask;

	ButtonAction=true;
	ob^=b;
	for (mask=1;mask;mask<<=1, actions++)
	{
		if (ob&mask)
		{
			TryActions(*actions, (b&mask)==0);
		}
	}
	ButtonAction=false;
}

dboolean G_ActionResponder(event_t *ev)
{
	switch (ev->type)
	{
	case ev_keyup:
	case ev_keydown:
		if ((ev->data1<0)||(ev->data1>=NUMKEYS))
			break;
		TryActions(KeyActions[ev->data1], ev->type==ev_keyup);
		break;
	case ev_joystick:
		ProcessButtonActions(JoyActions, ev->data1, JoyButtons);
		JoyButtons=ev->data1;
		if (DigiJoy)
			G_DoCmdDigiJoyMove(UseJoystick-1, ev->data2, ev->data3);
		else
			G_DoCmdJoyMove(UseJoystick-1, ev->data2, ev->data3);
		break;
	case ev_mouse:
		ProcessButtonActions(MouseActions, ev->data1, MouseButtons);
		MouseButtons=ev->data1;
		G_DoCmdMouseMove(UseMouse[0]-1, ev->data2, ev->data3);
		break;
	case ev_mouse2:
		ProcessButtonActions(Mouse2Actions, ev->data1, Mouse2Buttons);
		Mouse2Buttons=ev->data1;
		G_DoCmdMouseMove(UseMouse[1]-1, ev->data2, ev->data3);
		break;
	}
	return(false);
}

char	*NextToken(char *s, dboolean *pquoted)//null terminates current token
{
	char	*p;

	p=s;
//FUDGE: recognize escape chars in cmds
	if (*pquoted)
	{
		while (*p&&(*p!='"'))
			p++;
		if (*p=='"')
			*(p++)=0;
	}
	while (*p&&(*p!=';')&&!isspace(*p))
		p++;
	if (isspace(*p))
	{
		*(p++)=0;
		p=SkipWhitespace(p);
	}
	if (*p=='"')
	{
		*pquoted=true;
		p++;
	}
	else
		*pquoted=false;
	return(p);
}

alist_t *ParseActions(char *actions)
{
	char		*p;
	int			param;
	alist_t		*al;
	alist_t		*alist;
	dboolean	quoted;

	if (!actions)
		return(NULL);
	p=SkipWhitespace(actions);
	if (!*p)
		return(NULL);
	alist=(alist_t *)Z_Malloc(sizeof(alist_t), PU_STATIC, NULL);
	al=alist;
	al->buff=Z_StrDup(p);
	p=al->buff;
	quoted=false;
	while (true)
	{
		al->cmd=p;
		al->refcount=1;
		param=0;
		p=NextToken(p, &quoted);
		while (*p&&(*p!=';'))
		{
			if (param<MAX_ACTIONPARAM)
				al->param[param++]=p;
			p=NextToken(p, &quoted);
		}
		while (param<=MAX_ACTIONPARAM)
			al->param[param++]=NULL;
		while (*p==';')
			p=SkipWhitespace(&p[1]);
		strlwr(al->cmd);
		if (!*p)
		{
			al->next=NULL;
			return(alist);
		}
		al->next=(alist_t *)Z_Malloc(sizeof(alist_t), PU_STATIC, NULL);
		al=al->next;
		al->buff=NULL;
	}
}

void G_ExecuteCommand(char *action)
{
	alist_t	*al;

	al=ParseActions(action);
	al=DoRunActions(al, true);
	if (al)
	{
		AddActions(al);
		DerefActionList(al);
	}
}

alist_t **FindActionControler(char *name, alist_t **actions, int numbuttons)
{
	int		i;

	if ((name[0]>='1')&&(name[0]<='9'))
	{
		i=atoi(name)-1;
		if (i>numbuttons)
			return(NULL);
	}
	else
		return(NULL);
	return(&actions[i]);
}

alist_t **G_FindKeyByName(char *key)
{
	int			i;
	char		buff[MAX_KEY_NAME_LENGTH];

	if (strnicmp(key, "mouse", 5)==0)
	{
		//gets confused if have >20 mouse buttons:)
		if ((key[5]=='2')&&key[6])
			return(FindActionControler(&key[6], Mouse2Actions, MOUSE_BUTTONS));
		return(FindActionControler(&key[5], MouseActions, MOUSE_BUTTONS));
	}
	if (strnicmp(key, "joy", 3)==0)
		return(FindActionControler(&key[3], JoyActions, JOY_BUTTONS));
	for (i=0;i<NUMKEYS;i++)
	{
		M_GetKeyName(buff, i);
		if (stricmp(key, buff)==0)
		{
			return(&KeyActions[i]);
		}
	}
	return(NULL);
}

void G_BindAction(alist_t **plist, char *action)
{
	alist_t	*al;

	al=ParseActions(action);

	if (plist)
	{
		if (*plist)
			DerefActionList(*plist);
		*plist=al;
	}
	else
	{
		I_Printf("Unknown Key\n");
		DerefActionList(al);
	}
}

static int GetBitNum(int bits)
{
	int		mask;
	int		i;

	for (mask=1, i=0;mask;mask<<=1,i++)
	{
		if (mask&bits)
			return(i);
	}
	return(-1);
}

dboolean G_BindActionByEvent(event_t *ev, char *action)
{
	int		button;
	alist_t	**plist;

	plist=NULL;
	switch (ev->type)
	{
	case ev_keydown:
		plist=&KeyActions[ev->data1];
		break;
	case ev_mouse:
		button=GetBitNum(ev->data1);
		if ((button>=0)&&(button<MOUSE_BUTTONS))
			plist=&MouseActions[button];
		break;
	case ev_mouse2:
		button=GetBitNum(ev->data1);
		if ((button>=0)&&(button<MOUSE_BUTTONS))
			plist=&Mouse2Actions[button];
		break;
	case ev_joystick:
		button=GetBitNum(ev->data1);
		if ((button>=0)&&(button<JOY_BUTTONS))
			plist=&JoyActions[button];
		break;
	}
	if (plist)
	{
		G_BindAction(plist, action);
		return(true);
	}
	return(false);
}

void G_BindActionByName(char *key, char *action)
{
	G_BindAction(G_FindKeyByName(key), action);
}

static void OutputActions(FILE * fh, alist_t *al, char *name)
{
	int		i;

	fprintf(fh, "bind %s \"", name);
	while (al)
	{
		fprintf(fh, "%s", al->cmd);
		for (i=0;al->param[i];i++)
		{
			fprintf(fh, " %s", al->param[i]);
		}
		al=al->next;
		if (al)
			fprintf(fh, " ; ");
	}
	fprintf(fh, "\"\n");
}

void G_OutputBindings(FILE *fh)
{
	int			i;
	alist_t		*al;
	char		name[MAX_KEY_NAME_LENGTH];

	for (i=0;i<NUMKEYS;i++)
	{
		al=KeyActions[i];
		if (!al)
			continue;
		M_GetKeyName(name, i);
		OutputActions(fh, al, name);
	}
	strcpy(name, "mouse");
	for (i=0;i<MOUSE_BUTTONS;i++)
	{
		al=MouseActions[i];
		if (al)
		{
			name[5]=i+'1';
			name[6]=0;
			OutputActions(fh, al, name);
		}
	}
	for (i=0;i<MOUSE_BUTTONS;i++)
	{
		al=Mouse2Actions[i];
		if (al)
		{
			name[5]='2';
			name[6]=i+'1';
			name[7]=0;
			OutputActions(fh, al, name);
		}
	}
	strcpy(name, "joy");
	for (i=0;i<JOY_BUTTONS;i++)
	{
		al=JoyActions[i];
		if (!al)
			continue;
		name[4]=0;
		sprintf(name+3, "%d", i+1);
		OutputActions(fh, al, name);
	}
}

void G_PrintActions(alist_t *al)
{
	int		i;

	while (al)
	{
		I_Printf("%s", al->cmd);
		for (i=0;al->param[i];i++)
		{
			I_Printf(" %s", al->param[i]);
		}
		al=al->next;
		if (al)
			I_Printf(" ; ");
	}
}

void G_ShowBinding(char *key)
{
	alist_t	**alist;

	alist=G_FindKeyByName(key);
	if (!alist)
	{
		I_Printf("Unknown key:%s\n", key);
		return;
	}
	if (!*alist)
	{
		I_Printf("%s is not bound\n", key);
		return;
	}
	I_Printf("%s = ", key);
	G_PrintActions(*alist);
	I_Printf("\n");
}

static int			NumActions;
static action_t	**ActionBuffer;

int CountActions(action_t *action)
{
	int		num;

	if (!action)
		return(0);
	num=1;
	if (action->children[0])
		num+=CountActions(action->children[0]);
	if (action->children[1])
		num+=CountActions(action->children[1]);
	return(num);
}

void DumpActions(action_t *action)
{
	if (!action)
		return;
	if (action->children[0])
		DumpActions(action->children[0]);
	ActionBuffer[NumActions++]=action;
	if (action->children[1])
		DumpActions(action->children[1]);
}

action_t *RebuildActions(int left, int right)
{
	int			mid;
	action_t	*action;

	mid=(left+right)/2;
	action=ActionBuffer[mid];
	if (left<mid)
	{
		action->children[0]=RebuildActions(left, mid-1);
		action->children[0]->parent=action;
	}
	else
		action->children[0]=NULL;
	if (mid<right)
	{
		action->children[1]=RebuildActions(mid+1, right);
		action->children[1]->parent=action;
	}
	else
		action->children[1]=NULL;
	return(action);

}

void G_OptimizeActionTree(void)
{
	OptimizeTree=true;
}

//dumps into array, then rebuilds tree to minimize tree depth
void G_DoOptimizeActionTree(void)
{
	int		count;
	//lots of nice recursive procedures:)
	count=CountActions(Actions);
	if (count==0)
		return;
	ActionBuffer=(action_t **)Z_Malloc(count*sizeof(action_t *), PU_STATIC, NULL);
	NumActions=0;
	DumpActions(Actions);
	Actions=RebuildActions(0, count-1);
	Actions->parent=NULL;
	Z_Free(ActionBuffer);
}

//does not free children, or remove from tree
void G_FreeAction(action_t *action)
{
	if (!action)
		return;
	if (action->name)
		Z_Free(action->name);
	if (action->proc==G_RunAlias)
		DerefActionList((alist_t *)action->data);
	Z_Free(action);
}

//does not alter with->children
static void ReplaceActionWith(action_t *action, action_t *with)
{
	if (action==with)
		return;
	if (!action)
		I_Error("Replaced NULL action");

	if (action->parent)
	{
		if (action->parent->children[0]==action)
			action->parent->children[0]=with;
		else
			action->parent->children[1]=with;
	}
	else
		Actions=with;
	if (with)
		with->parent=action->parent;
}

static void AddAction(action_t *action)
{
	action_t	*tree;
	int			cmp;
	int			child;

	action->parent=NULL;
	action->children[0]=action->children[1]=NULL;
	if (Actions)
	{
		tree=Actions;
		while (tree)
		{
			cmp=strcmp(action->name, tree->name);
			if (cmp==0)
			{
				ReplaceActionWith(tree, action);

				for (child=0;child<2;child++)
				{
					action->children[child]=tree->children[child];
					if (tree->children[child])
						tree->children[child]->parent=action;
					tree->children[child]=NULL;
				}
				G_FreeAction(tree);
				tree=NULL;
			}
			else
			{
				if (cmp>0)
					child=1;
				else
					child=0;
				if (tree->children[child])
					tree=tree->children[child];
				else
				{
					action->parent=tree;
					tree->children[child]=action;
					tree=NULL;
					G_OptimizeActionTree();
				}
			}
		}
	}
	else
	{
		Actions=action;
	}
}

void G_RegisterAction(char *name, actionproc_t proc, int data)
{
	action_t	*action;

	action=(action_t *)Z_Malloc(sizeof(action_t), PU_STATIC, NULL);
	action->name=strlwr(Z_StrDup(name));
	action->proc=proc;
	action->data=data;
	AddAction(action);
}

void G_RunAlias(int data, char **param)
{
	AddActions(DoRunActions((alist_t *)data, false));
}

void G_ShowAliases(action_t	*action)
{
	if (!action)
		return;

	if (action->children[0])
		G_ShowAliases(action->children[0]);
	if ((action->proc==G_RunAlias)&&action->data)
	{
		I_Printf(" %s = ", action->name);
		G_PrintActions((alist_t *)action->data);
		I_Printf("\n");
	}
	if (action->children[1])
		G_ShowAliases(action->children[1]);
}

void G_UnregisterAction(char *name)
{
	action_t	*action;
	action_t	*tree;
	char		buff[256];

	strcpy(buff, name);
	strlwr(buff);
	action=FindAction(buff);
	if (!action)
		return;
	if (!action->children[0])
	{
		ReplaceActionWith(action, action->children[1]);
	}
	else if (!action->children[1])
	{
		ReplaceActionWith(action, action->children[0]);
	}
	else
	{
		tree=action->children[1];
		while (tree->children[0])
			tree=tree->children[0];
		tree->children[0]=action->children[0];
		action->children[0]->parent=tree;
		G_OptimizeActionTree();
	}
	G_FreeAction(action);
}

void G_CmdAlias(int data, char **param)
{
	alist_t		*al;

	if (!param[0])
	{
		I_Printf("Current Aliases:\n");
		G_ShowAliases(Actions);
		return;
	}
	al=ParseActions(param[1]);
	if (!al)
		G_UnregisterAction(param[0]);
	else
		G_RegisterAction(param[0], G_RunAlias, (int)al);
}

static int ListCommandRecurse(action_t *action)
{
	int		count;

	if (!action)
		return(0);

	count=1;
	if (action->children[0])
		count+=ListCommandRecurse(action->children[0]);

	if (action->name[0]=='-')
		count--;
	else
		I_Printf(" %s\n", action->name);

	if (action->children[1])
		count+=ListCommandRecurse(action->children[1]);

	return(count);
}

int G_ListCommands(void)
{
	return(ListCommandRecurse(Actions));
}

void G_CmdUnbind(int data, char **param)
{
	alist_t		**alist;

	if (!param[0])
	{
		I_Printf(" unbind <key>\n");
		return;
	}
	alist=G_FindKeyByName(param[0]);
	if (!alist)
	{
		I_Printf("Unknown Key:%s\n", param[0]);
		return;
	}
	if (*alist)
	{
		DerefActionList(*alist);
		*alist=NULL;
	}
}

static void UnbindActions(alist_t **alist, int num)
{
	int		i;

	for (i=0;i<num;i++)
	{
		if (!alist[i])
			continue;
		DerefActionList(*alist);
		return;
	}
}

void G_CmdUnbindAll(int data, char **param)
{
	UnbindActions(AllActions, NUM_ACTIONS);
}

static dboolean IsSameAction(char *cmd, int split, alist_t *al)
{
	if (!al)
		return(false);
	if (split!=G_Param2Split(al->param[0]))
		return(false);
	if (stricmp(cmd, al->cmd)!=0)
		return(false);
	return(true);
}

void G_GetActionName(char *buff, int n)
{
	*buff=0;
	if (n>=NUM_ACTIONS)
		return;
	if (n>=OTHER_ACTIONPOS)
	{
		sprintf(buff, "other%02x", n-OTHER_ACTIONPOS);
		return;
	}
	if (n>=MOUSE2_ACTIONPOS)
	{
		sprintf(buff, "mouse2%d", n-MOUSE2_ACTIONPOS);
		return;
	}
	if (n>=MOUSE_ACTIONPOS)
	{
		sprintf(buff, "mouse2%d", n-MOUSE_ACTIONPOS);
		return;
	}
	if (n>=JOY_ACTIONPOS)
	{
		sprintf(buff, "joy%d", n-JOY_ACTIONPOS);
		return;
	}
	if (n>=KEY_ACTIONPOS)
	{
		M_GetKeyName(buff, n-KEY_ACTIONPOS);
		return;
	}
}

void G_GetActionBindings(char *buff, char *action, int split)
{
	int		i;
	char	*p;

	p=buff;
	*p=0;
	for (i=0;i<NUMKEYS;i++)
	{
		if (IsSameAction(action, split, KeyActions[i]))
		{
			if (p!=buff)
				*(p++)=',';
			M_GetKeyName(p, i);
			p+=strlen(p);
			if (p-buff>=MAX_MENUACTION_LENGTH)
				return;
		}
	}
	for (i=0;i<MOUSE_BUTTONS;i++)
	{
		if (IsSameAction(action, split, MouseActions[i]))
		{
			if (p!=buff)
				*(p++)=',';
			strcpy(p, "mouse?");
			p[5]=i+'1';
			p+=6;
			if (p-buff>=MAX_MENUACTION_LENGTH)
				return;
		}
		if (IsSameAction(action, split, Mouse2Actions[i]))
		{
			if (p!=buff)
				*(p++)=',';
			strcpy(p, "mouse2?");
			p[6]=i+'1';
			p+=7;
			if (p-buff>=MAX_MENUACTION_LENGTH)
				return;
		}
	}
	for (i=0;i<JOY_BUTTONS;i++)
	{
		if (IsSameAction(action, split, JoyActions[i]))
		{
			if (p!=buff)
				*(p++)=',';
			strcpy(p, "joy?");
			p[3]=i+'1';
			p+=4;
			if (p-buff>=MAX_MENUACTION_LENGTH)
				return;
			return;
		}
	}
}
