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
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char
rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";
#endif


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>

#include "doomdef.h"
#include "m_misc.h"
#include "m_argv.h"
//#include "i_video.h"
#include "i_sound.h"
#include "i_input.h"
#include "i_music.h"
#include "i_windoz.h"

#include "d_net.h"
#include "g_game.h"
#include "c_interface.h"
#include "d_stuff.h"
#include "d_main.h"
#include "doomstat.h"
#include <mmsystem.h>

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"


int		AllocHeapSize;

FILE	*DebugFile=NULL;

void
I_Tactile
( int	on,
 int	off,
 int	total)
{

}

ticcmd_t	emptycmd;
ticcmd_t*	I_BaseTiccmd(void)
{
    return &emptycmd;
}


int  I_GetHeapSize (void)
{
    return AllocHeapSize*1024*1024;
}

byte* I_ZoneBase (int*	size)
{
	byte	*heap;
	int		i;
	int		newsize;

	i=M_CheckParm("-heapsize");
	if (i&&(i<myargc-1))
	{
		newsize=atoi(myargv[i+1]);
		if (newsize>4)
			AllocHeapSize=newsize;;
	}
    *size = AllocHeapSize*1024*1024;
	heap=(byte *) malloc (*size);
	if (!heap)
		I_Error("I_ZoneBase: out of memory(requested %d MB)", AllocHeapSize);
    return(heap);
}



//
// I_GetTime
// returns time in 1/70th second tics
// now 1/35th?
//
int  I_GetTime (void)
{
    static int		basetime=0;
    int			time;

    time=timeGetTime();
    if (!basetime)
		basetime = time;
    return((time-basetime)*TICRATE/1000);
}



//
// I_Init
//
void I_Init (void)
{
    I_InitMusic();
    I_InitSound();
    C_API.I_InitGraphics();
    I_InitInput();
}

//
// I_Quit
//
void I_Quit (void)
{
    D_QuitNetGame ();
    I_ShutdownSound();
    I_ShutdownMusic();
    M_SaveDefaults(GameDefaults, NULL);
	M_SaveDefaults(C_API.info->defaults, DllName);
    C_API.I_ShutdownGraphics();
    I_ShutdownInput();
    I_ShutdownWindows();
    exit(0);
}

dboolean	inshowbusy=false;

void I_BeginRead(void)
{
	if (inshowbusy||ShowScreenText)
		return;
	inshowbusy=true;
	C_API.I_BeginRead();
	inshowbusy=false;
	BusyDisk=true;
}


void I_EndRead(void)
{
}

byte*	I_AllocLow(int length)
{
    byte*	mem;

    mem = (byte *)malloc (length);
    memset (mem,0,length);
    return mem;
}


//
// I_Error
//
extern dboolean demorecording;

void I_Error (char *error, ...)
{
    va_list	argptr;
    char	buff[1024];

    if (demorecording)
		G_CheckDemoStatus();
    D_QuitNetGame ();

    if (interfacevalid)
		C_API.I_ShutdownGraphics();

    I_ShutdownSound();
    I_ShutdownMusic();
    I_ShutdownInput();

    // Message first.
    va_start (argptr, error);
    vsprintf (buff, error, argptr);
    va_end (argptr);
	I_Printf("Error:%s\n", buff);
	if (hMainWnd)
    	MessageBox(hMainWnd, buff, "Doom3D - Error", MB_OK);

    // Shutdown. Here might be other errors.

    C_Shutdown();
    I_ShutdownWindows();
    exit(-1);
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
    I_ProcessWindows();
	I_ProcessMusic();
}

//
// I_StartTic
//
void I_StartTic (void)
{
	if (!InBackground)
		I_ProcessInput();
}


