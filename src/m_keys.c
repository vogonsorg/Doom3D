//m_keys.c

#include "m_keys.h"
#include "doomdef.h"

typedef struct
{
	int		code;
	char	*name;
}keyinfo_t;

static keyinfo_t	Keys[]=
{
	{KEY_RIGHTARROW,	"Right"},
	{KEY_LEFTARROW,		"Left"},
	{KEY_UPARROW,		"Up"},
	{KEY_DOWNARROW,		"Down"},
	{KEY_ESCAPE,		"Escape"},
	{KEY_ENTER,			"Enter"},
	{KEY_TAB,			"Tab"},
	{KEY_BACKSPACE,		"Backsp"},
	{KEY_PAUSE,			"Pause"},
	{KEY_SHIFT,			"Shift"},
	{KEY_ALT,			"Alt"},
	{KEY_CTRL,			"Ctrl"},
	{KEY_EQUALS,		"+"},
	{KEY_MINUS,			"-"},
	{KEY_ENTER,			"Enter"},
	{KEY_CAPS,			"Caps"},
	{KEY_INS,			"Ins"},
	{KEY_DEL,			"Del"},
	{KEY_HOME,			"Home"},
	{KEY_END,			"End"},
	{KEY_PGUP,			"PgUp"},
	{KEY_PGDOWN,		"PgDn"},
	{';',				";"},
	{'\'',				"'"},
	{'#',				"#"},
	{'\\',				"\\"},
	{',',				","},
	{'.',				"."},
	{'/',				"/"},
	{'[',				"["},
	{']',				"]"},
	{'*',				"*"},
	{' ',				"Space"},
	{KEY_F1,			"F1"},
	{KEY_F2,			"F2"},
	{KEY_F3,			"F3"},
	{KEY_F4,			"F4"},
	{KEY_F5,			"F5"},
	{KEY_F6,			"F6"},
	{KEY_F7,			"F7"},
	{KEY_F8,			"F8"},
	{KEY_F9,			"F9"},
	{KEY_F10,			"F10"},
	{KEY_F11,			"F11"},
	{KEY_F12,			"F12"},
	{KEY_KP0,			"KP0"},
	{KEY_KP1,			"KP1"},
	{KEY_KP2,			"KP2"},
	{KEY_KP3,			"KP3"},
	{KEY_KP4,			"KP4"},
	{KEY_KP5,			"KP5"},
	{KEY_KP6,			"KP6"},
	{KEY_KP7,			"KP7"},
	{KEY_KP8,			"KP8"},
	{KEY_KP9,			"KP9"},
	{0,NULL}
};

int M_GetKeyName(char *buff, int key)
{
	keyinfo_t	*pkey;

	if (((key>='a')&&(key<='z'))||((key>='0')&&(key<='9')))
	{
		buff[0]=(char)toupper(key);
		buff[1]=0;
		return(true);
	}
	for (pkey=Keys;pkey->name;pkey++)
	{
		if (pkey->code==key)
		{
			strcpy(buff, pkey->name);
			return(true);
		}
	}
	sprintf(buff, "Key%02x", key);
	return(false);
}
