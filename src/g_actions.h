#ifndef G_ACTIONS_H
#define G_ACTIONS_H

#define MAX_ACTIONPARAM		2

typedef void (*actionproc_t)(int data, char **param);

void G_InitActions(void);
dboolean G_ActionResponder(event_t *ev);
void G_RegisterAction(char *name, actionproc_t proc, int data);
void G_ActionTicker(void);
void G_ExecuteCommand(char *action);
void G_BindActionByName(char *key, char *action);
dboolean G_BindActionByEvent(event_t *ev, char *action);
void G_ShowBinding(char *key);
void G_GetActionBindings(char *buff, char *action, int split);
int G_ListCommands(void);
int G_Param2Split(char *p);

void G_CmdAlias(int data, char **param);
void G_OutputBindings(FILE *fh);
void G_CmdUnbind(int data, char **param);
void G_CmdUnbindAll(int data, char **param);
void G_DoCmdMouseMove(int split, int x, int y);
void G_DoCmdJoyMove(int split, int x, int y);
void G_DoCmdDigiJoyMove(int split, int x, int y);

extern dboolean	ButtonAction;

#endif