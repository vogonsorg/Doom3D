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
// DESCRIPTION:
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char
rcsid[] = "$Id: i_main.c,v 1.4 1997/02/03 22:45:10 b1 Exp $";
#endif

#include "doomdef.h"

#include  <stdio.h>
#include <mmsystem.h>
#include <dinput.h>
#include <ddraw.h>
#include <dsound.h>


#include "m_argv.h"
#include "d_main.h"
#include "i_windoz.h"

#define MAX_ARGS 256

char	*ArgBuffer[MAX_ARGS+1];

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	char	*p;
	char	*start;
	char	**arg;

	hAppInst=hInstance;

	arg=ArgBuffer;
	p=lpCmdLine;
	myargc=1;
	*(arg++)="Doom3D";

	while (*p&&(myargc<MAX_ARGS))
	{
		while (*p&&isspace(*p))
			p++;
		if (!*p)
			break;
		start=p;
		while (*p&&!isspace(*p))
			p++;
		*arg=(char *)malloc(p-start+1);
		(*arg)[p-start]=0;
		strncpy(*arg, start, p-start);
		arg++;
		myargc++;
	}
	*arg=NULL;
	myargv=ArgBuffer;

	D_DoomMain ();

	return 0;
}
