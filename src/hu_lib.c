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
// DESCRIPTION:  heads-up text and input code
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char
rcsid[] = "$Id: hu_lib.c,v 1.3 1997/01/26 07:44:58 b1 Exp $";
#endif

#include <ctype.h>

#include "doomdef.h"
#include "d_stuff.h"

//#include "v_video.h"
#include "m_swap.h"

#include "hu_lib.h"
//#include "r_local.h"
//#include "r_draw.h"
#include "c_interface.h"
#include "doomstat.h"
//extern dboolean       automapactive;  // in AM_map.c

void HUlib_init(void)
{
}

void HUlib_clearTextLine(hu_textline_t* t)
{
	t->len = 0;
	t->l[0] = 0;
//	t->needsupdate = true;
}

void
HUlib_initTextLine
( hu_textline_t*        t,
  patch_t**             f,
  int                   sc )
{
	t->f = f;
	t->sc = sc;
	HUlib_clearTextLine(t);
}

dboolean
HUlib_addCharToTextLine
( hu_textline_t*        t,
  char                  ch )
{

	if (t->len == HU_MAXLINELENGTH)
		return false;
	else
	{
		t->l[t->len++] = ch;
		t->l[t->len] = 0;
//		t->needsupdate = 4;
		return true;
	}

}

dboolean HUlib_delCharFromTextLine(hu_textline_t* t)
{

	if (!t->len) return false;
	else
	{
		t->l[--t->len] = 0;
//		t->needsupdate = 4;
		return true;
	}

}

void HUlib_drawTextLine(hu_textline_t *l, int x, int y, dboolean drawcursor, int xmax)
{

	int                 i;
	int                 w;
	unsigned char       c;

	if (NumSplits==1)
		xmax=320;
	for (i=0;i<l->len;i++)
	{
		c = toupper(l->l[i]);
		if (c != ' '
			&& c >= l->sc
			&& c <= '_')
		{
			w = SHORT(l->f[c - l->sc]->width);
			if (x+w > xmax)
				break;
			if (NumSplits==1)
				C_API.V_DrawPatchBig(x, y, FG, l->f[c - l->sc]);
			else
				C_API.V_DrawPatch(x, y, FG, l->f[c - l->sc]);
			x += w;
		}
		else
		{
			x += 4;
			if (x >= xmax)
				break;
		}
	}

	// draw the cursor if requested
	if (drawcursor
		&& x + SHORT(l->f['_' - l->sc]->width) <= xmax)
	{
		if (NumSplits==1)
			C_API.V_DrawPatchBig(x, y, FG, l->f['_' - l->sc]);
		else
			C_API.V_DrawPatch(x, y, FG, l->f['_' - l->sc]);
	}
}


// sorta called by HU_Erase and just better darn get things straight
void HUlib_eraseTextLine(hu_textline_t* l)
{
//FUDGE: could just disable smaller screen sizes...
#if 0
	int                 lh;
	int                 y;
	int                 yoffset;

	// Only erases when NOT in automap and the screen is reduced,
	// and the text must either need updating or refreshing
	// (because of a recent change back from the automap)

	if (!automapactive &&
		viewwindowx && l->needsupdate)
	{
		lh = SHORT(l->f[0]->height) + 1;
		for (y=l->y,yoffset=y*ScreenWidth ; y<l->y+lh ; y++,yoffset+=ScreenWidth)
		{
			if (y < viewwindowy || y >= viewwindowy + viewheight)
				R_VideoErase(yoffset, ScreenWidth); // erase entire line
			else
			{
				R_VideoErase(yoffset, viewwindowx); // erase left border
				R_VideoErase(yoffset + viewwindowx + viewwidth, viewwindowx);
				// erase right border
			}
		}
	}
	if (l->needsupdate) l->needsupdate--;
#endif
}

#include "i_system.h"

void
HUlib_initSText
( hu_stext_t*   s,
  int           x,
  int           y,
  int           h,
  patch_t**     font,
  int           startchar,
  dboolean*     on )
{

	int i;

	if (h>HU_MAXLINES)
			I_Error("too many lines");
	s->h = h;
	s->on = on;
	s->laston = true;
	s->cl = 0;
	s->x=x;
	s->y=y;
	for (i=0;i<h;i++)
	{
//ABCD:i*(SHORT(font[0]->height)+1)
		HUlib_initTextLine(&s->l[i],
		   font, startchar);
	}

}

void HUlib_addLineToSText(hu_stext_t* s, int tics)
{

//	int i;

	// add a clear line
	s->cl++;
	if (s->cl >= s->h)
		s->cl = 0;
	HUlib_clearTextLine(&s->l[s->cl]);
	s->count[s->cl]=tics;

#if 0
	// everything needs updating
	for (i=0 ; i<s->h ; i++)
		s->l[i].needsupdate = 4;
#endif
}

void HUlib_addMessageToSText(hu_stext_t *s, char * prefix, char *msg, int tics)
{
	HUlib_addLineToSText(s, tics);
	if (prefix)
		while (*prefix)
			HUlib_addCharToTextLine(&s->l[s->cl], *(prefix++));

	while (*msg)
		HUlib_addCharToTextLine(&s->l[s->cl], *(msg++));
}

void HUlib_advanceSText(hu_stext_t *s)
{
	int		i;

	for (i=0;i<s->h;i++)
	{
		if (!--s->count[i])
			HUlib_clearTextLine(&s->l[i]);
	}
}

void HUlib_drawSText(hu_stext_t* s)
{
	int 			line;
	int				i;
	hu_textline_t	*l;
	int				y;

	if (!*s->on)
		return; // if not on, don't draw

	y=s->y;
	line=s->cl+1;
	// draw everything
	for (i=0;i<s->h;i++)
	{
		if (line>=s->h)
			line=0;
		l = &s->l[line];

		// need a decision made here on whether to skip the draw
		if (l->len)
		{
			HUlib_drawTextLine(l, s->x, y, false, s->x+ViewWidth); // no cursor, please
			y+=SHORT(l->f[0]->height)+1;
		}
		line++;
	}

}

void HUlib_eraseSText(hu_stext_t* s)
{

	int i;

	for (i=0 ; i<s->h ; i++)
	{
#if 0
		if (s->laston && !*s->on)
			s->l[i].needsupdate = 4;
#endif
		HUlib_eraseTextLine(&s->l[i]);
	}
	s->laston = *s->on;

}

void
HUlib_initIText
( hu_itext_t*   it,
  int           x,
  int           y,
  patch_t**     font,
  int           startchar,
  dboolean*     on )
{
//	it->lm = 0;// default left margin is start of text
	it->on = on;
	it->laston = true;
	it->x=x;
	it->y=y;
	HUlib_initTextLine(&it->l, font, startchar);
}


// The following deletion routines adhere to the left margin restriction
void HUlib_delCharFromIText(hu_itext_t* it)
{
//	if (it->l.len != it->lm)
		HUlib_delCharFromTextLine(&it->l);
}

void HUlib_eraseLineFromIText(hu_itext_t* it)
{
//	while (it->lm != it->l.len)
//		HUlib_delCharFromTextLine(&it->l);
	it->l.len=0;
}

// Resets left margin as well
void HUlib_resetIText(hu_itext_t* it)
{
	//it->lm = 0;
	HUlib_clearTextLine(&it->l);
}

/*
void
HUlib_addPrefixToIText
( hu_itext_t*   it,
  char*         str )
{
	while (*str)
		HUlib_addCharToTextLine(&it->l, *(str++));
	it->lm = it->l.len;
}
*/
// wrapper function for handling general keyed input.
// returns true if it ate the key
dboolean
HUlib_keyInIText
( hu_itext_t*   it,
  unsigned char ch )
{

	if (ch >= ' ' && ch <= '_')
		HUlib_addCharToTextLine(&it->l, (char) ch);
	else
		if (ch == KEY_BACKSPACE)
			HUlib_delCharFromIText(it);
		else
			if (ch != KEY_ENTER)
				return false; // did not eat key

	return true; // ate the key

}

void HUlib_drawIText(hu_itext_t* it)
{

	hu_textline_t *l = &it->l;

	if (!*it->on)
		return;
	HUlib_drawTextLine(l, it->x, it->y, true, ScreenWidth); // draw the line w/ cursor

}

void HUlib_eraseIText(hu_itext_t* it)
{
#if 0
	if (it->laston && !*it->on)
		it->l.needsupdate = 4;
#endif
	HUlib_eraseTextLine(&it->l);
	it->laston = *it->on;
}

