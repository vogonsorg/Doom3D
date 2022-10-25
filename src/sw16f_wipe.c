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
//	Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------
#ifdef RCSID

static const char rcsid[] = "$Id: f_wipe.c,v 1.2 1997/02/03 22:45:09 b1 Exp $";
#endif



#include "doomdef.h"

#include "t_zone.h"
#include "swi_video.h"
#include "swv_video.h"

#include "swf_wipe.h"

#include "c_sw.h"

void wipe_StartScreen(void)
{
}

void wipe_EndScreen(void)
{
}

dboolean wipe_ScreenWipe(int ticks)
{
    return(true);
}
