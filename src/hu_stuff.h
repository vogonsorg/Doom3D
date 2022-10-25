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
// DESCRIPTION:  Head up display
//
//-----------------------------------------------------------------------------

#ifndef __HU_STUFF_H__
#define __HU_STUFF_H__

#include "d_event.h"


//
// Globally visible constants.
//
#define HU_FONTSTART	'!'	// the first font characters
#define HU_FONTEND	'_'	// the last font characters

// Calculate # of glyphs in font.
#define HU_FONTSIZE	(HU_FONTEND - HU_FONTSTART + 1)

#define HU_BROADCAST	(MAXPLAYERS+1)

#define HU_MSGREFRESH	KEY_ENTER
#define HU_MSGX			0
#define HU_MSGY			0
//#define HU_MSGWIDTH		64	// in characters
#define HU_MSGHEIGHT	4	// in lines

#define HU_MSGTIMEOUT	(4*TICRATE)
#define HU_CHATMSGTIMEOUT HU_MSGTIMEOUT
//
// HEADS UP TEXT
//

void HU_Init(void);
void HU_Start(void);
void HU_ViewMoved(void);

dboolean HU_Responder(event_t* ev);

void HU_Ticker(void);
void HU_Drawer(dboolean fullscreenhud);
char HU_dequeueChatChar(void);
void HU_Erase(void);


#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
