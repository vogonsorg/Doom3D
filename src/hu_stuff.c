// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:  Heads-up displays
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char
rcsid[] = "$Id: hu_stuff.c,v 1.4 1997/02/03 16:47:52 b1 Exp $";
#endif

#include <ctype.h>

#include "doomdef.h"

#include "z_zone.h"

#include "m_swap.h"
#include "m_shift.h"

#include "hu_stuff.h"
#include "hu_lib.h"
#include "st_stuff.h"
#include "m_argv.h"
#include "m_cheat.h"
#include "w_wad.h"

#include "s_sound.h"
#include "sp_split.h"

#include "doomstat.h"

// Data.
#include "dstrings.h"
#include "sounds.h"

//
// Locally used constants, shortcuts.
//
#define HU_TITLE	(mapnames[(gameepisode-1)*9+gamemap-1])
#define HU_TITLE2	(mapnames2[gamemap-1])
#define HU_TITLEP	(mapnamesp[gamemap-1])
#define HU_TITLET	(mapnamest[gamemap-1])
#define HU_TITLEHEIGHT	1
#define HU_TITLEX	0
#define HU_TITLEY	(167 - SHORT(hu_font[0]->height))

#define HU_INPUTTOGGLE	't'
#define HU_INPUTX	HU_MSGX
#define HU_INPUTY	(HU_MSGY + HU_MSGHEIGHT*(SHORT(hu_font[0]->height) +1))
#define HU_INPUTWIDTH	64
#define HU_INPUTHEIGHT	1



char*	chat_macros[] =
{
    HUSTR_CHATMACRO0,
		HUSTR_CHATMACRO1,
		HUSTR_CHATMACRO2,
		HUSTR_CHATMACRO3,
		HUSTR_CHATMACRO4,
		HUSTR_CHATMACRO5,
		HUSTR_CHATMACRO6,
		HUSTR_CHATMACRO7,
		HUSTR_CHATMACRO8,
		HUSTR_CHATMACRO9
};

char*	player_names[] =
{
    HUSTR_PLRGREEN,
	HUSTR_PLRINDIGO,
	HUSTR_PLRBROWN,
	HUSTR_PLRRED
};


patch_t*		hu_font[HU_FONTSIZE];
static hu_textline_t	w_title;
dboolean			chat_on;
static hu_itext_t	w_chat;
static dboolean		always_off = false;
static char		chat_dest[MAXPLAYERS];
static hu_itext_t w_inputbuffer[MAXPLAYERS];

static dboolean		message_on;

static hu_stext_t	w_message[MAX_SPLITS];

extern int			showMessages;

static dboolean		headsupactive = false;

//
// Builtin map names.
// The actual names can be found in DStrings.h.
//

char*	mapnames[] =	// DOOM shareware/registered/retail (Ultimate) names.
{

    HUSTR_E1M1,
	HUSTR_E1M2,
	HUSTR_E1M3,
	HUSTR_E1M4,
	HUSTR_E1M5,
	HUSTR_E1M6,
	HUSTR_E1M7,
	HUSTR_E1M8,
	HUSTR_E1M9,

	HUSTR_E2M1,
	HUSTR_E2M2,
	HUSTR_E2M3,
	HUSTR_E2M4,
	HUSTR_E2M5,
	HUSTR_E2M6,
	HUSTR_E2M7,
	HUSTR_E2M8,
	HUSTR_E2M9,

	HUSTR_E3M1,
	HUSTR_E3M2,
	HUSTR_E3M3,
	HUSTR_E3M4,
	HUSTR_E3M5,
	HUSTR_E3M6,
	HUSTR_E3M7,
	HUSTR_E3M8,
	HUSTR_E3M9,

	HUSTR_E4M1,
	HUSTR_E4M2,
	HUSTR_E4M3,
	HUSTR_E4M4,
	HUSTR_E4M5,
	HUSTR_E4M6,
	HUSTR_E4M7,
	HUSTR_E4M8,
	HUSTR_E4M9,

	"NEWLEVEL",
	"NEWLEVEL",
	"NEWLEVEL",
	"NEWLEVEL",
	"NEWLEVEL",
	"NEWLEVEL",
	"NEWLEVEL",
	"NEWLEVEL",
	"NEWLEVEL"
};

char*	mapnames2[] =	// DOOM 2 map names.
{
    HUSTR_1,
	HUSTR_2,
	HUSTR_3,
	HUSTR_4,
	HUSTR_5,
	HUSTR_6,
	HUSTR_7,
	HUSTR_8,
	HUSTR_9,
	HUSTR_10,
	HUSTR_11,

	HUSTR_12,
	HUSTR_13,
	HUSTR_14,
	HUSTR_15,
	HUSTR_16,
	HUSTR_17,
	HUSTR_18,
	HUSTR_19,
	HUSTR_20,

	HUSTR_21,
	HUSTR_22,
	HUSTR_23,
	HUSTR_24,
	HUSTR_25,
	HUSTR_26,
	HUSTR_27,
	HUSTR_28,
	HUSTR_29,
	HUSTR_30,
	HUSTR_31,
	HUSTR_32
};


char*	mapnamesp[] =	// Plutonia WAD map names.
{
    PHUSTR_1,
	PHUSTR_2,
	PHUSTR_3,
	PHUSTR_4,
	PHUSTR_5,
	PHUSTR_6,
	PHUSTR_7,
	PHUSTR_8,
	PHUSTR_9,
	PHUSTR_10,
	PHUSTR_11,

	PHUSTR_12,
	PHUSTR_13,
	PHUSTR_14,
	PHUSTR_15,
	PHUSTR_16,
	PHUSTR_17,
	PHUSTR_18,
	PHUSTR_19,
	PHUSTR_20,

	PHUSTR_21,
	PHUSTR_22,
	PHUSTR_23,
	PHUSTR_24,
	PHUSTR_25,
	PHUSTR_26,
	PHUSTR_27,
	PHUSTR_28,
	PHUSTR_29,
	PHUSTR_30,
	PHUSTR_31,
	PHUSTR_32
};


char *mapnamest[] =	// TNT WAD map names.
{
    THUSTR_1,
	THUSTR_2,
	THUSTR_3,
	THUSTR_4,
	THUSTR_5,
	THUSTR_6,
	THUSTR_7,
	THUSTR_8,
	THUSTR_9,
	THUSTR_10,
	THUSTR_11,

	THUSTR_12,
	THUSTR_13,
	THUSTR_14,
	THUSTR_15,
	THUSTR_16,
	THUSTR_17,
	THUSTR_18,
	THUSTR_19,
	THUSTR_20,

	THUSTR_21,
	THUSTR_22,
	THUSTR_23,
	THUSTR_24,
	THUSTR_25,
	THUSTR_26,
	THUSTR_27,
	THUSTR_28,
	THUSTR_29,
	THUSTR_30,
	THUSTR_31,
	THUSTR_32
};


char frenchKeyMap[128]=
{
    0,
		1,2,3,4,5,6,7,8,9,10,
		11,12,13,14,15,16,17,18,19,20,
		21,22,23,24,25,26,27,28,29,30,
		31,
		' ','!','"','#','$','%','&','%','(',')','*','+',';','-',':','!',
		'0','1','2','3','4','5','6','7','8','9',':','M','<','=','>','?',
		'@','Q','B','C','D','E','F','G','H','I','J','K','L',',','N','O',
		'P','A','R','S','T','U','V','Z','X','Y','W','^','\\','$','^','_',
		'@','Q','B','C','D','E','F','G','H','I','J','K','L',',','N','O',
		'P','A','R','S','T','U','V','Z','X','Y','W','^','\\','$','^',127
};

char ForeignTranslation(unsigned char ch)
{
    return ch < 128 ? frenchKeyMap[ch] : ch;
}

void HU_Init(void)
{

    int		i;
    int		j;
    char	buffer[9];

    // load the heads-up font
    j = HU_FONTSTART;
    for (i=0;i<HU_FONTSIZE;i++)
    {
		sprintf(buffer, "STCFN%.3d", j++);
		hu_font[i] = (patch_t *) W_CacheLumpName(buffer, PU_STATIC);
    }

    ShowFPS=M_CheckParm("-fps");
}

void HU_Stop(void)
{
    headsupactive = false;
}

void HU_ViewMoved(void)
{
	int		split;
    int		x;
    int		y;

	for (split=0;split<NumSplits;split++)
	{
	    S_GetViewPos(split, &x, &y);
	    x+=HU_MSGX;
	    y+=HU_MSGY;
	    w_message[split].x=x;
	    w_message[split].y=y;
	}
}

void HU_Start(void)
{

    int		i;
    char*	s;

    if (headsupactive)
		HU_Stop();

    message_on = true;
    chat_on = false;

	for (i=0;i<NumSplits;i++)
	{
	    // create the message widgets
	    HUlib_initSText(&w_message[i],
			0, 0, HU_MSGHEIGHT,
			hu_font,
			HU_FONTSTART, &message_on);
	}
    // create the map title widget
    HUlib_initTextLine(&w_title,
		hu_font,
		HU_FONTSTART);

    switch ( gamemode )
    {
	case shareware:
	case registered:
	case retail:
		s = HU_TITLE;
		break;

		/* FIXME
		case pack_plut:
		s = HU_TITLEP;
		break;
		case pack_tnt:
		s = HU_TITLET;
		break;
		*/

	case commercial:
	default:
		s = HU_TITLE2;
		break;
    }

    while (*s)
		HUlib_addCharToTextLine(&w_title, *(s++));

    // create the chat widget
    HUlib_initIText(&w_chat,
		HU_INPUTX, HU_INPUTY,
		hu_font,
		HU_FONTSTART, &chat_on);

    // create the inputbuffer widgets
    for (i=0 ; i<MAXPLAYERS ; i++)
		HUlib_initIText(&w_inputbuffer[i], 0, 0, 0, 0, &always_off);

    headsupactive = true;
}

void HU_Drawer(dboolean fullscreenhud)
{
	int		split;

	for (split=0;split<NumSplits;split++)
    	HUlib_drawSText(&w_message[split]);
    HUlib_drawIText(&w_chat);
    if (automaponly)
		HUlib_drawTextLine(&w_title, HU_TITLEX, HU_TITLEY, false, ScreenWidth);
    if (fullscreenhud)//hack for fullscreen Q2-style HUD
		ST_DrawHUD();

    ST_DrawFPS();
}

void HU_Erase(void)
{
	int		split;

	for (split=0;split<NumSplits;split++)
    	HUlib_eraseSText(&w_message[split]);
    HUlib_eraseIText(&w_chat);
    HUlib_eraseTextLine(&w_title);

}

char	cheatedmessage[52];

dboolean P_GivePower(player_t *player, int power);

void HU_PlayerCheated(int playernum, int cheat)
{
	int			i;
	player_t	*player;
	char		*msg;

	player=&players[playernum];
	switch(cheat)
	{
	case CHEATCODE_GOD:
		player->cheats ^= CF_GODMODE;
		if (player->cheats & CF_GODMODE)
		{
			if (player->mo)
				player->mo->health = 100;

			player->health = 100;
			player->message = STSTR_DQDON;
		}
		else
			player->message = STSTR_DQDOFF;
		break;
	case CHEATCODE_FA:
		player->armorpoints = 200;
		player->armortype = 2;

		for (i=0;i<NUMWEAPONS;i++)
			player->weaponowned[i] = true;

		for (i=0;i<NUMAMMO;i++)
			player->ammo[i] = player->maxammo[i];

		player->message = STSTR_FAADDED;
		break;
	case CHEATCODE_KFA:
		player->armorpoints = 200;
		player->armortype = 2;

		for (i=0;i<NUMWEAPONS;i++)
			player->weaponowned[i] = true;

		for (i=0;i<NUMAMMO;i++)
			player->ammo[i] = player->maxammo[i];

		for (i=0;i<NUMCARDS;i++)
			player->cards[i] = true;

		player->message = STSTR_KFAADDED;
		break;
	case CHEATCODE_NOCLIP:
		player->cheats ^= CF_NOCLIP;

		if (player->cheats & CF_NOCLIP)
			player->message = STSTR_NCON;
		else
			player->message = STSTR_NCOFF;
		break;
	case CHEATCODE_BEHOLD0:
	case CHEATCODE_BEHOLD1:
	case CHEATCODE_BEHOLD2:
	case CHEATCODE_BEHOLD3:
	case CHEATCODE_BEHOLD4:
	case CHEATCODE_BEHOLD5:
		i=cheat-CHEATCODE_BEHOLD0;
		if (!player->powers[i])
			P_GivePower( player, i);
		else if (i!=pw_strength)
			player->powers[i] = 1;
		else
			player->powers[i] = 0;

		player->message = STSTR_BEHOLDX;
		break;
	case CHEATCODE_CHOPPERS:
		player->weaponowned[wp_chainsaw] = true;
		player->powers[pw_invulnerability] = true;
		player->message = STSTR_CHOPPERS;
		break;
	default:
		sprintf(cheatedmessage, STSTR_NETHURT, playernum+1);
		msg=cheatedmessage;
		i=cheat-CHEATCODE_HURT;
		if ((i<0)||(i>=4))
		{
			msg=STSTR_NOCHEAT;
			i=cheat-CHEATCODE_PUNISH;
			if ((i<0)||(i>=4))
				return;
		}
		player->cheats=0;
		if (!playeringame[i])
			break;
		player=&players[i];
		for (i=0;i<6;i++)
			player->powers[i]=0;
		player->armorpoints = 0;
		player->armortype = 0;
		if (player->mo)
			player->mo->health = 1;
		player->health=1;

		for (i=0;i<NUMWEAPONS;i++)
			player->weaponowned[i] = false;

		for (i=0;i<NUMAMMO;i++)
			player->ammo[i] = 0;

		player->message=msg;
		break;
	}
	if (netgame&&(cheat<CHEATCODE_PUNISH)&&!netcheat)
	{
		sprintf(cheatedmessage, STSTR_NETCHEAT, player+1-players);
		players[consoleplayer].message=cheatedmessage;
		NextCheatCode=CHEATCODE_PUNISH+playernum;
	}
}

void HU_Ticker(void)
{

    int 		i, rc;
    char 		c;
    player_t	*player;
    int			split;

	for (split=0;split<NumSplits;split++)
	{
		player=&players[consoleplayer+split];
		HUlib_advanceSText(&w_message[split]);

		if (showMessages)
		{

			// display message if necessary
			if (player->message)
			{
				HUlib_addMessageToSText(&w_message[split], NULL, player->message, HU_MSGTIMEOUT);
				player->message = NULL;
				message_on = true;
			}

		} // else message_on = false;
	}

    // check for incoming chat characters
    //if (netgame)
    {
		for (i=0 ; i<MAXPLAYERS; i++)
		{
			if (!playeringame[i])
				continue;
			c = players[i].cmd.chatchar;
			if (netgame&&(i != consoleplayer)&&(c>0))
			{
				if (c <= HU_BROADCAST)
					chat_dest[i] = c;
				else
				{
					if (c >= 'a' && c <= 'z')
						c = (char) shiftxform[(unsigned char) c];
					rc = HUlib_keyInIText(&w_inputbuffer[i], c);
					if (rc && c == KEY_ENTER)
					{
						if (w_inputbuffer[i].l.len
							&& (((chat_dest[i] > consoleplayer)&&(chat_dest[i]<=consoleplayer+NumSplits))
							|| chat_dest[i] == HU_BROADCAST))
						{
							if (chat_dest[i] == HU_BROADCAST)
							{
								for (split=0;split<NumSplits;split++)
								{
									HUlib_addMessageToSText(&w_message[split],
										player_names[i],
										w_inputbuffer[i].l.l, HU_CHATMSGTIMEOUT);
								}
							}
							else
							{
								split=chat_dest[i]-(consoleplayer+1);
								if ((split>=0)&&(split<NumSplits))
								{
									HUlib_addMessageToSText(&w_message[split],
										player_names[i],
										w_inputbuffer[i].l.l, HU_CHATMSGTIMEOUT);
								}
								else
									split=-1;
							}
							if (split>=0)
							{
								message_on = true;
								if ( gamemode == commercial )
									S_StartSound(NULL, sfx_radio);
								else
									S_StartSound(NULL, sfx_tink);
							}
						}
						HUlib_resetIText(&w_inputbuffer[i]);
					}
				}
			}
			else if (c<0)
			{
				HU_PlayerCheated(i, -c);
			}
			players[i].cmd.chatchar = 0;
		}
    }

}

#define QUEUESIZE		128

static char	chatchars[QUEUESIZE];
static int	head = 0;
static int	tail = 0;


void HU_queueChatChar(char c)
{
    if (((head + 1) & (QUEUESIZE-1)) == tail)
    {
		players[consoleplayer].message = HUSTR_MSGU;
    }
    else
    {
		chatchars[head] = c;
		head = (head + 1) & (QUEUESIZE-1);
    }
}

char HU_dequeueChatChar(void)
{
    char c;

    if (head != tail)
    {
		c = chatchars[tail];
		tail = (tail + 1) & (QUEUESIZE-1);
    }
    else
    {
		c = 0;
    }

    return c;
}

dboolean HU_Responder(event_t *ev)
{

    static char		lastmessage[HU_MAXLINELENGTH+1];
    char*		macromessage;
    dboolean		eatkey = false;
    static dboolean	shiftdown = false;
    static dboolean	altdown = false;
    unsigned char 	c;
    int			i;
    int			numplayers;

    static char		destination_keys[MAXPLAYERS] =
    {
		HUSTR_KEYGREEN,
		HUSTR_KEYINDIGO,
		HUSTR_KEYBROWN,
		HUSTR_KEYRED
    };

    static int		num_nobrainers = 0;

    numplayers = 0;
    for (i=0 ; i<MAXPLAYERS ; i++)
		numplayers += playeringame[i];

    if (ev->data1 == KEY_SHIFT)
    {
		shiftdown = ev->type == ev_keydown;
		return false;
    }
    else if (ev->data1 == KEY_ALT)
    {
		altdown = ev->type == ev_keydown;
		return false;
    }

    if (ev->type != ev_keydown)
		return false;

    if (!chat_on)
    {
		if (ev->data1 == HU_MSGREFRESH)
		{
//FUDGE: get repeat last message thing working
			eatkey = true;
		}
		else if (netgame && ev->data1 == HU_INPUTTOGGLE)
		{
			eatkey = chat_on = true;
			HUlib_resetIText(&w_chat);
			HU_queueChatChar(HU_BROADCAST);
		}
		else if (netgame && numplayers > 2)
		{
			for (i=0; i<MAXPLAYERS ; i++)
			{
				if (ev->data1 == destination_keys[i])
				{
					if (playeringame[i] && i!=consoleplayer)
					{
						eatkey = chat_on = true;
						HUlib_resetIText(&w_chat);
						HU_queueChatChar((char)(i+1));
						break;
					}
					else if (i == consoleplayer)
					{
						num_nobrainers++;
						if (num_nobrainers < 3)
							players[consoleplayer].message = HUSTR_TALKTOSELF1;
						else if (num_nobrainers < 6)
							players[consoleplayer].message = HUSTR_TALKTOSELF2;
						else if (num_nobrainers < 9)
							players[consoleplayer].message = HUSTR_TALKTOSELF3;
						else if (num_nobrainers < 32)
							players[consoleplayer].message = HUSTR_TALKTOSELF4;
						else
							players[consoleplayer].message = HUSTR_TALKTOSELF5;
					}
				}
			}
		}
    }
    else
    {
		c = ev->data1;
		// send a macro
		if (altdown)
		{
			c = c - '0';
			if (c > 9)
				return false;

			macromessage = chat_macros[c];

			// kill last message with a '\n'
			HU_queueChatChar(KEY_ENTER); // DEBUG!!!

			// send the macro message
			while (*macromessage)
				HU_queueChatChar(*macromessage++);
			HU_queueChatChar(KEY_ENTER);

			// leave chat mode and notify that it was sent
			chat_on = false;
			strcpy(lastmessage, chat_macros[c]);
			players[consoleplayer].message = lastmessage;
			eatkey = true;
		}
		else
		{
			if (language==french)
				c = ForeignTranslation(c);
			if (shiftdown || (c >= 'a' && c <= 'z'))
				c = shiftxform[c];
			eatkey = HUlib_keyInIText(&w_chat, c);
			if (eatkey)
			{
				// static unsigned char buf[20]; // DEBUG
				HU_queueChatChar(c);

			}
			if (c == KEY_ENTER)
			{
				chat_on = false;
				if (w_chat.l.len)
				{
					strcpy(lastmessage, w_chat.l.l);
					players[consoleplayer].message = lastmessage;
				}
			}
			else if (c == KEY_ESCAPE)
				chat_on = false;
		}
    }

    return eatkey;

}
