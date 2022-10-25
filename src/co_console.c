//co_console.c

#include "doomstat.h"
#include "co_console.h"
#include "z_zone.h"
#include "hu_font.h"
#include "c_interface.h"
#include "g_actions.h"
#include "m_shift.h"

#define CONSOLE_PROMPTCHAR		']'
#define MAX_CONSOLE_INPUT_LEN	1024
#define MAX_CONSOLE_LINES		256//must be power of 2
#define CONSOLETEXT_MASK		(MAX_CONSOLE_LINES-1)
#define CMD_HISTORY_SIZE		64

typedef struct
{
	int		len;
	char	line[1];
}conline_t;

enum
{
	CST_UP,
	CST_RAISE,
	CST_LOWER,
	CST_DOWN
};

conline_t	**ConsoleText;
int			ConsoleHead;
int			ConsoleWidth;//chars
int			ConsoleHeight=0;//lines
int			ConsolePos=0;//bottom of console, in pixels
char		*ConsoleLineBuffer=NULL;
int			ConsoleLineLen;
int			LineBufferSize=1024;
dboolean	ConsoleState=CST_UP;
char		*ConsoleInputBuff;
int			ConsoleInputLen;
int			MaxConsoleHeight;
int			PrevCommands[CMD_HISTORY_SIZE];
int			PrevCommandHead;
int			NextCommand;

#define CO_FONTWIDTH	8
#define CO_FONTHEIGHT	8

void CO_Init(void)
{
	int		i;

	ConsoleText=(conline_t **)Z_Malloc(sizeof(conline_t *)*MAX_CONSOLE_LINES, PU_STATIC, NULL);
	ConsoleHead=0;
	for (i=0;i<MAX_CONSOLE_LINES;i++)
		ConsoleText[i]=NULL;
	ConsoleLineBuffer=(char *)Z_Malloc(LineBufferSize, PU_STATIC, NULL);
	ConsoleLineLen=0;
	ConsoleInputBuff=(char *)Z_Malloc(MAX_CONSOLE_INPUT_LEN, PU_STATIC, NULL);
	ConsoleInputLen=1;
	ConsoleInputBuff[0]=CONSOLE_PROMPTCHAR;
	MaxConsoleHeight=ScreenHeight/CO_FONTHEIGHT-5;
	ConsoleWidth=ScreenWidth/CO_FONTWIDTH-2;
	for (i=0;i<CMD_HISTORY_SIZE;i++)
		PrevCommands[i]=-1;
	PrevCommandHead=0;
	NextCommand=0;
}

void CO_AddLine(char *line, int len)
{
	conline_t	*cline;
	int			i;
	static dboolean	recursed=false;

	if (!ConsoleLineBuffer)
		return;//not initialised yet
	if (recursed)
		return;//later call to Z_Malloc can fail and call I_Error/I_Printf...
	recursed=true;
	if (!line)
		return;
	if (len==-1)
		len=strlen(line);
	cline=(conline_t *)Z_Malloc(sizeof(conline_t)+len, PU_STATIC, NULL);
	cline->len=len;
	if (len)
		memcpy(cline->line, line, len);
	cline->line[len]=0;
	ConsoleHead=(ConsoleHead+CONSOLETEXT_MASK)&CONSOLETEXT_MASK;
	ConsoleText[ConsoleHead]=cline;
	i=(ConsoleHead+CONSOLETEXT_MASK)&CONSOLETEXT_MASK;
	if (ConsoleText[i])
	{
		Z_Free(ConsoleText[i]);
		ConsoleText[i]=NULL;
	}
	recursed=false;
}

void CO_AddText(char *text)
{
	char	*src;
	char	c;

	if (!ConsoleLineBuffer)
		return;

	src=text;
	c=*(src++);
	while (c)
	{
		if ((c=='\n')||(ConsoleLineLen>=LineBufferSize))
		{
			CO_AddLine(ConsoleLineBuffer, ConsoleLineLen);
			ConsoleLineLen=0;
		}
		if (c!='\n')
			ConsoleLineBuffer[ConsoleLineLen++]=c;

		c=*(src++);
	}
}

dboolean CO_Responder(event_t* ev)
{
	static dboolean	shiftdown=false;
	int				c;

	if ((ev->type!=ev_keyup)&&(ev->type!=ev_keydown))
		return(false);

	c=ev->data1;
	if (ev->data1==KEY_SHIFT)
		shiftdown=(ev->type==ev_keydown);

	switch (ConsoleState)
	{
	case CST_DOWN:
	case CST_LOWER:
		if (ev->type==ev_keydown)
		{
			switch (c)
			{
			case '~':
				ConsoleState=CST_RAISE;
				break;
			case KEY_ESCAPE:
				ConsoleInputLen=1;
				break;
			case KEY_ENTER:
				if (ConsoleInputLen<=1)
					break;
				ConsoleInputBuff[ConsoleInputLen]=0;
				CO_AddLine(ConsoleInputBuff, ConsoleInputLen);
				PrevCommands[PrevCommandHead]=ConsoleHead;
				PrevCommandHead++;
				NextCommand=PrevCommandHead;
				if (PrevCommandHead>=CMD_HISTORY_SIZE)
					PrevCommandHead=0;
				PrevCommands[PrevCommandHead]=-1;
				G_ExecuteCommand(&ConsoleInputBuff[1]);
				ConsoleInputLen=1;
				break;
			case KEY_UPARROW:
				c=NextCommand-1;
				if (c<0)
					c=CMD_HISTORY_SIZE-1;
				if (PrevCommands[c]==-1)
					break;
				NextCommand=c;
				c=PrevCommands[NextCommand];
				if (ConsoleText[c])
				{
					ConsoleInputLen=ConsoleText[c]->len;
					memcpy(ConsoleInputBuff, ConsoleText[PrevCommands[NextCommand]]->line, ConsoleInputLen);
				}
				break;
			case KEY_DOWNARROW:
				if (PrevCommands[NextCommand]==-1)
					break;
				c=NextCommand+1;
				if (c>=CMD_HISTORY_SIZE)
					c=0;
				if (PrevCommands[c]==-1)
					break;
				NextCommand=c;
				ConsoleInputLen=ConsoleText[PrevCommands[NextCommand]]->len;
				memcpy(ConsoleInputBuff, ConsoleText[PrevCommands[NextCommand]]->line, ConsoleInputLen);
				break;
			case KEY_BACKSPACE:
				if (ConsoleInputLen>1)
					ConsoleInputLen--;
				break;
			default:
				c=toupper(c);
				if (((c<HU_FONTSTART)||(c>HU_FONTEND))&&(c!=' '))
					break;
				if (shiftdown)
					c=shiftxform[c];
				ConsoleInputBuff[ConsoleInputLen++]=c;
				if (ConsoleInputLen>=MAX_CONSOLE_INPUT_LEN)
					ConsoleInputLen=MAX_CONSOLE_INPUT_LEN-1;
				break;
			}
		}
		return(true);
	case CST_UP:
	case CST_RAISE:
		if (c=='~')
		{
			if (ev->type==ev_keydown)
				ConsoleState=CST_LOWER;
			return(true);
		}
		break;
	}
	return(false);
}

void CO_Ticker(void)
{
	if (!ConsoleLineBuffer)
		return;

	switch (ConsoleState)
	{
	case CST_LOWER:
		ConsoleHeight++;
		if (ConsoleHeight>=MaxConsoleHeight)
			ConsoleState=CST_DOWN;
		break;
	case CST_RAISE:
		ConsoleHeight--;
		if (ConsoleHeight<=0)
			ConsoleState=CST_UP;
		break;
	}
	if (ConsoleHeight>0)
		ConsolePos=(ConsoleHeight+1)*CO_FONTHEIGHT;
	else
		ConsolePos=0;
}

void CO_Draw(void)
{
	int		line;
	int		y;
	int		i;
	char	c;
	int		x;
	int		len;
	int		lnum;
	int		xmax;
	patch_t	*patch;

	C_API.V_DrawConsoleBackground();

	if (!ConsoleLineBuffer)
		return;
	if (ConsoleHeight<=0)
		return;

	xmax=ConsoleWidth*CO_FONTWIDTH;
	line=ConsoleHead;
	lnum=ConsoleHeight;
	while (ConsoleText[line]&&(lnum>0))
	{
		x=CO_FONTWIDTH;
		len=ConsoleText[line]->len;
		lnum-=(len-1)/ConsoleWidth+1;
		i=0;
		if (lnum<0)
		{
			i-=ConsoleWidth*lnum;//Note lnum -ve, so subtract:)
			lnum=0;
		}
		y=lnum*CO_FONTHEIGHT;
		for (;i<len;i++)
		{
			c=toupper(ConsoleText[line]->line[i]);
			if ((c>=HU_FONTSTART)&&(c<=HU_FONTEND))
			{
				patch=hu_font[c-HU_FONTSTART];
				C_API.V_DrawPatch(x+((CO_FONTWIDTH-patch->width)>>1), y, 0, patch);
			}
			x+=CO_FONTWIDTH;
			if (x>xmax)
			{
				x=CO_FONTWIDTH;
				y+=CO_FONTHEIGHT;
			}
		}
		line=(line+1)&CONSOLETEXT_MASK;
	}

	x=CO_FONTWIDTH;
	y=ConsoleHeight*CO_FONTHEIGHT;
	for (i=0;i<ConsoleInputLen;i++)
	{
		c=toupper(ConsoleInputBuff[i]);
		if ((c>=HU_FONTSTART)&&(c<=HU_FONTEND)&&(x<xmax))
		{
			patch=hu_font[c-HU_FONTSTART];
			C_API.V_DrawPatch(x+((CO_FONTWIDTH-patch->width)>>1), y, 0, patch);
		}
		x+=CO_FONTWIDTH;
	}
	if (x<xmax)
		C_API.V_DrawPatch(x, y, 0, hu_font['_'-HU_FONTSTART]);
}
