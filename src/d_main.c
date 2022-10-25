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
//	DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//
//-----------------------------------------------------------------------------
#ifdef RCSID

static const char rcsid[] = "$Id: d_main.c,v 1.8 1997/02/03 22:45:09 b1 Exp $";
#endif

#define R_OK		0x04

#define	BGCOLOR		7
#define	FGCOLOR		8


#include <io.h>
//#include <dir.h>

#include "doomdef.h"
#include "doomstat.h"
#include <stdlib.h>

#include "dstrings.h"
#include "sounds.h"

#include "m_shift.h"

#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
//#include "v_video.h"

#include "f_finale.h"
//#include "f_wipe.h"

#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"

#include "i_system.h"
#include "i_sound.h"
//#include "i_video.h"
#include "i_windoz.h"

#include "g_game.h"

#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

#include "p_setup.h"
//#include "r_local.h"
#include "c_interface.h"

#include "d_main.h"

#include "co_console.h"

#include "sp_split.h"
//
// D_DoomLoop()
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//
void D_DoomLoop (void);


char	*wadfiles[MAXWADFILES]={NULL};
int		NumWadFiles=0;
int		NumSSDrawn=0;

dboolean		devparm=false;	// started game with -devparm
dboolean		nomonsters=false;	// checkparm of -nomonsters
dboolean        respawnparm=false;	// checkparm of -respawn
dboolean		fastparm=false;	// checkparm of -fast

dboolean		singletics = false; // debug flag to cancel adaptiveness

dboolean		WipeBusy=false;
dboolean		BusyDisk=false;

//extern int soundVolume;
//extern  int	sfxVolume;
//extern  int	musicVolume;

extern  dboolean	inhelpscreens;

skill_t			startskill;
int             startepisode;
int				startmap;
dboolean		autostart=false;

FILE*			debugfile=NULL;

dboolean		advancedemo=false;


//char			wadfile[1024];		// primary wad file
char			mapdir[1024];		// directory of development maps
char			basedefault[1024];	// default file


void D_CheckNetGame (void);
void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t* cmd, int split);
void D_DoAdvanceDemo (void);


//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//
event_t			events[MAXEVENTS];
int				eventhead=0;
int				eventtail=0;


//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent (event_t* ev)
{
    events[eventhead] = *ev;
    eventhead = (++eventhead)&(MAXEVENTS-1);
}


//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents (void)
{
    event_t*	ev;

    // IF STORE DEMO, DO NOT ACCEPT INPUT
    if ( ( gamemode == commercial )
		&& (W_CheckNumForName("map01")<0) )
		return;

    for ( ; eventtail != eventhead ; eventtail = (++eventtail)&(MAXEVENTS-1) )
    {
		ev = &events[eventtail];
		if (CO_Responder(ev))
			continue;
		if (M_Responder (ev))
			continue;               // menu ate the event
		G_Responder (ev);
    }
}


//
// D_IncValidCount
//

void D_IncValidCount(void)
{
	validcount++;
	(*C_API.pvalidcount)=validcount;
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
gamestate_t     wipegamestate = GS_DEMOSCREEN;
dboolean setsizeneeded=false;
extern  int             showMessages;
//int	RedrawCount;
extern int screenblocks;

void D_CheckWipeBusy(void)
{
	if ((NumSplits==2)||(NumSplits==3))
	{
		WipeBusy=true;
		return;
	}
	if (NumSplits==4)
	{
		WipeBusy=false;
		return;
	}
	if (ViewHeight==ScreenHeight)
	{
		WipeBusy=false;
		return;
	}
	if (ScreenWidth>ST_WIDTH)
	{
		WipeBusy=true;
		return;
	}
	WipeBusy=false;
}

void D_Display (void)
{
    static  dboolean	viewactivestate = false;
    static  dboolean	menuactivestate = false;
    static  dboolean	inhelpscreensstate = false;
    static  int			fullscreen = 0;
    static  gamestate_t	oldgamestate = -1;
    static  int			borderdrawcount;
    static	int			blankviews=0;
    int					nowtime;
    int					tics;
    int					wipestart;
    dboolean			done;
    dboolean			wipe;
    dboolean			redrawsbar;
    static int			fullscreenstate=0;

    if (nodrawers)
		return;                    // for comparative timing / profiling

    C_API.I_StartDrawing();
    redrawsbar = false;

    // change the view size if needed
    if (setsizeneeded)
    {
		S_DoSetViewSize ();
		oldgamestate = -1;                      // force background redraw
		borderdrawcount = 3;
		RedrawCount=NUM_VID_PAGES;
    }

    // save the current screen if about to wipe
    if (gamestate != wipegamestate)
    {
		wipe = true;
		C_API.wipe_StartScreen();
    }
    else
		wipe = false;

    if (gamestate == GS_LEVEL && gametic)
		HU_Erase();

	fullscreen=fullscreenstate;
	fullscreenstate=screenblocks-9;
	if (fullscreenstate<0)
		fullscreenstate=0;
    // do buffered drawing
    switch (gamestate)
    {
	case GS_LEVEL:
		if (!gametic)
			break;
		if (automapactive)
			AM_Drawer ();
		if ((BusyDisk&&WipeBusy)||wipe || (fullscreenstate<2 && fullscreen==2) )
			redrawsbar = true;
		if (inhelpscreensstate && !inhelpscreens)
			redrawsbar = true;              // just put away the help screen
		if (redrawsbar)
			RedrawCount=NUM_VID_PAGES;
		ST_Drawer (fullscreenstate==2, RedrawCount);
		if (RedrawCount)
			RedrawCount--;
		break;

	case GS_INTERMISSION:
		WI_Drawer ();
		break;

	case GS_FINALE:
		F_Drawer ();
		break;

	case GS_DEMOSCREEN:
		D_PageDrawer ();
		break;
    }

	NumSSDrawn=0;
    // draw the view directly
    if (gamestate == GS_LEVEL && !automaponly && gametic)
		C_API.R_RenderPlayerView (&players[displayplayer]);

//FUDGE: hide HUD for network drones
    if (gamestate == GS_LEVEL && gametic)
    {
		HU_Drawer ((fullscreenstate==2)&&!automapactive);
	}

    // clean up border stuff
    if (gamestate != oldgamestate && gamestate != GS_LEVEL)
		C_API.I_SetPalette (W_CacheLumpName ("PLAYPAL",PU_CACHE));

    // see if the border needs drawing
    if (gamestate == GS_LEVEL)
    {
		if (oldgamestate != GS_LEVEL)
	    {
			viewactivestate = false;        // view was not active
			C_API.R_FillBackScreen ();    // draw the pattern into the back screen
	    }
		if (!automaponly && !fullscreenstate)
		{
			if (menuactive || menuactivestate || !viewactivestate)
				borderdrawcount = 3;
			if (borderdrawcount)
			{
				C_API.R_DrawViewBorder ();    // erase old menu stuff
				borderdrawcount--;
			}

		}

		if (blankviews>0)
		{
			C_API.V_BlankEmptyViews();
			blankviews--;
		}
		if ((BusyDisk&&WipeBusy)||menuactive||(oldgamestate!=GS_LEVEL))
			blankviews=NUM_VID_PAGES;

	}


	BusyDisk=false;
    menuactivestate = menuactive;
    viewactivestate = viewactive;
    inhelpscreensstate = inhelpscreens;
    oldgamestate = wipegamestate = gamestate;

    // draw pause pic
    if (paused)
    {
		C_API.V_DrawPatchBig(126,
			50,0,W_CacheLumpName ("M_PAUSE", PU_CACHE));
    }

    // menus go directly to the screen
    M_Drawer ();          // menu is drawn even on top of everything
    //if (ConsoleActive)
    	CO_Draw();
    NetUpdate ();         // send out any new accumulation

    // normal update
    if (!wipe)
    {
		ShowScreenText=false;
		C_API.I_FinishUpdate ();              // page flip or blit buffer
		return;
    }

    // wipe update
    C_API.wipe_EndScreen();

    wipestart = I_GetTime () - 1;

    do
    {
		C_API.I_StartDrawing();
		do
		{
			nowtime = I_GetTime ();
			tics = nowtime - wipestart;
		} while (!tics);
		wipestart = nowtime;
		done=C_API.wipe_ScreenWipe(tics);
		M_Drawer ();                            // menu is drawn even on top of wipes
		ShowScreenText=false;
		C_API.I_FinishUpdate ();                      // page flip or blit buffer
    } while (!done);
}



//
//  D_DoomLoop
//
extern  dboolean         demorecording;

void D_DoomLoop (void)
{
    if (demorecording)
		G_BeginRecording ();

#if 0
    if (M_CheckParm ("-debugfile"))
    {
		char    filename[20];
		sprintf (filename,"debug%i.txt",consoleplayer);
		I_Printf ("debug output to: %s\n",filename);
		debugfile = fopen (filename, "w");
    }
#endif

    while (1)
    {
		// frame syncronous IO operations
		I_StartFrame ();
		// process one or more tics

		if (singletics)
		{
			int		split;

			I_StartTic ();
			D_ProcessEvents ();

			for (split=consoleplayer;split<NumSplits;split++)
				G_BuildTiccmd (&netcmds[split][maketic%BACKUPTICS], split);
			if (advancedemo)
				D_DoAdvanceDemo ();
			M_Ticker ();
			G_Ticker ();
			CO_Ticker();
			gametic++;
			maketic++;
		}
		else
		{
			TryRunTics (); // will run at least one tic
		}

		S_UpdateSounds ();// move positional sounds

		// Update display, next frame, with current state.
		D_Display ();

		I_ProcessSound();
    }
}



//
//  DEMO LOOP
//
int             demosequence;
int             pagetic;
char                    *pagename;


//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker (void)
{
    if (--pagetic < 0)
		D_AdvanceDemo ();
}



//
// D_PageDrawer
//
void D_PageDrawer (void)
{
    C_API.V_DrawPatchBig (0, 0, 0, W_CacheLumpName(pagename, PU_CACHE));
}


//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo (void)
{
    advancedemo = true;
}


//
// This cycles through the demo sequences.
// FIXME - version dependend demo numbers?
//
void D_DoAdvanceDemo (void)
{
	if (NumSplits>1)
		I_Error("Can't play demos in splitscreen mode");
    players[consoleplayer].playerstate = PST_LIVE;  // not reborn
    advancedemo = false;
    usergame = false;               // no save / end game here
    paused = false;
    gameaction = ga_nothing;

    if ( gamemode == retail )
		demosequence = (demosequence+1)%7;
    else
		demosequence = (demosequence+1)%6;

    switch (demosequence)
    {
	case 0:
		if ( gamemode == commercial )
			pagetic = 35 * 11;
		else
			pagetic = 170;
		gamestate = GS_DEMOSCREEN;
		pagename = "TITLEPIC";
		if ( gamemode == commercial )
			S_StartMusic(mus_dm2ttl);
		else
			S_StartMusic (mus_intro);
		break;
	case 1:
		G_DeferedPlayDemo ("demo1");
		break;
	case 2:
		pagetic = 200;
		gamestate = GS_DEMOSCREEN;
		pagename = "CREDIT";
		break;
	case 3:
		G_DeferedPlayDemo ("demo2");
		break;
	case 4:
		gamestate = GS_DEMOSCREEN;
		if ( gamemode == commercial)
		{
			pagetic = 35 * 11;
			pagename = "TITLEPIC";
			S_StartMusic(mus_dm2ttl);
		}
		else
		{
			pagetic = 200;

			if ( gamemode == retail )
				pagename = "CREDIT";
			else
				pagename = "HELP2";
		}
		break;
	case 5:
		G_DeferedPlayDemo ("demo3");
		break;
        // THE DEFINITIVE DOOM Special Edition demo
	case 6:
		G_DeferedPlayDemo ("demo4");
		break;
    }
}



//
// D_StartTitle
//
void D_StartTitle (void)
{
    gameaction = ga_nothing;
    demosequence = -1;
    D_AdvanceDemo ();
}




//      print title for every printed line
char            title[128];



//
// D_AddFile
//
void D_AddFile (char *file)
{
    //int     numwadfiles;
    char    *newfile;

	/*    for (numwadfiles = 0 ; wadfiles[numwadfiles] ; numwadfiles++)
	;
	*/
    newfile = malloc (strlen(file)+1);
    strcpy (newfile, file);

    wadfiles[NumWadFiles++] = newfile;
    wadfiles[NumWadFiles]=NULL;
}

//
// IdentifyVersion
// Checks availability of IWAD files by name,
// to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
//N.B. can't use Z_Malloc yet
typedef struct
{
	GameMode_t	mode;
	char		*name;
}iwad_t;

iwad_t	IWadNames[]=
{
	{commercial, "doom2.wad"},
	{commercial, "plutonia.wad"},
	{commercial, "tnt.wad"},
	{retail, "doom.wad"},
	{retail, "udoom.wad"},
	{shareware, "doom1.wad"},
	{0, NULL}
};

//FUDGE: add -waddir parameter
void IdentifyVersion (void)
{
	iwad_t	*iwad;
	char	name[1024];
	int		p;
	int		len;
	
	p=M_CheckParm("-iwad");
	if (p&&(p<myargc-1)&&(myargv[p+1][0]!='-'))
	{
		iwad=IWadNames;
		strcpy(name, myargv[p+1]);
		strlwr(name);
		len=strlen(name);
		if ((len<=4)||(strcmp(&name[len-4], ".wad")!=0))
			strcat(name, ".wad");
		if (access(name, R_OK))
			I_Error("Unable to open iwad %s", name);
		while (iwad->name)
		{
			if (strcmp(name, iwad->name)==0)
			{
				gamemode=iwad->mode;
				D_AddFile(name);
				return;
			}
			iwad++;
		}
		gamemode=indetermined;
		D_AddFile(name);
		return;
	}

	iwad=IWadNames;
	while (iwad->name)
	{
		if (!access(iwad->name, R_OK))
		{
			gamemode=iwad->mode;
			D_AddFile(iwad->name);
			return;
		}
		iwad++;
	}
	gamemode=indetermined;
}

//
// Find a Response File
//
void FindResponseFile (void)
{
    int             i;
#define MAXARGVS        100

    for (i = 1;i < myargc;i++)
		if (myargv[i][0] == '@')
		{
			FILE *          handle;
			int             size;
			int             k;
			int             index;
			int             indexinfile;
			char    *infile;
			char    *file;
			char    *moreargs[20];
			char    *firstargv;

			// READ THE RESPONSE FILE INTO MEMORY
			handle = fopen (&myargv[i][1],"rb");
			if (!handle)
			{
				I_Printf ("\nNo such response file!");
				exit(1);
			}
			I_Printf("Found response file %s!\n",&myargv[i][1]);
			fseek (handle,0,SEEK_END);
			size = ftell(handle);
			fseek (handle,0,SEEK_SET);
			file = malloc (size);
			fread (file,size,1,handle);
			fclose (handle);

			// KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
			for (index = 0,k = i+1; k < myargc; k++)
				moreargs[index++] = myargv[k];

			firstargv = myargv[0];
			myargv = malloc(sizeof(char *)*MAXARGVS);
			memset(myargv,0,sizeof(char *)*MAXARGVS);
			myargv[0] = firstargv;

			infile = file;
			indexinfile = k = 0;
			indexinfile++;  // SKIP PAST ARGV[0] (KEEP IT)
			do
			{
				myargv[indexinfile++] = infile+k;
				while(k < size &&
					((*(infile+k)>= ' '+1) && (*(infile+k)<='z')))
					k++;
				*(infile+k) = 0;
				while(k < size &&
					((*(infile+k)<= ' ') || (*(infile+k)>'z')))
					k++;
			} while(k < size);

			for (k = 0;k < index;k++)
				myargv[indexinfile++] = moreargs[k];
			myargc = indexinfile;

			// DISPLAY ARGS
			I_Printf("%d command-line args:\n",myargc);
			for (k=1;k<myargc;k++)
				I_Printf("%s\n",myargv[k]);

			break;
		}
}

void CheckVidParm(void)
{
    int		newwidth;
    int		newheight;
    int		p;

    newwidth=newheight=0;
    p=M_CheckParm("-width");
    if (p&&p<myargc-1)
		newwidth=atoi(myargv[p+1]);
    p=M_CheckParm("-height");
    if (p&&p<myargc-1)
		newheight=atoi(myargv[p+1]);
    if (newwidth&&newheight)
    {
		ScreenWidth=newwidth;
		ScreenHeight=newheight;
    }
    if (M_CheckParm("-window"))
		InWindow=true;
    if (M_CheckParm("-fullscreen"))
		InWindow=false;
}

//
// D_DoomMain
//
void D_DoomMain (void)
{
    int             p;
    char            file[256];
	matrixinfo_t	matrixinfo;

    I_InitWindows();

    if (M_CheckParm("-debug"))
    {
		DebugFile=fopen("debug.txt", "wt");
		if (!DebugFile)
			I_Error("Couldn't create debug.txt");
    }

	I_Printf("Args:");
	for (p=1;p<myargc;p++)
	{
		I_Printf(" %s",myargv[p]);
	}
	I_Printf("\n");

    I_Printf ("M_LoadDefaults: Load system defaults.\n");
    M_LoadDefaults(GameDefaults, NULL);// load before initing other systems
	p=M_CheckParm("-split");
	if (p)
	{
		if ((p<myargc-1)&&(myargv[p+1][0]!='-'))
		{
			NumSplits=atoi(myargv[p+1]);
			if (NumSplits<2)
				NumSplits=2;
			else if (NumSplits>MAX_SPLITS)
			{
				I_Printf("Can't have more than %d way splitscreen", MAX_SPLITS);
				NumSplits=MAX_SPLITS;
			}
		}
		else
			NumSplits=2;
	}
	else
		NumSplits=1;

    C_Init();
    M_LoadDefaults(C_API.info->defaults, DllName);

    CheckVidParm();

	I_SetWindowSize();

	S_CalcViewSize();

    FindResponseFile ();

    IdentifyVersion ();

    modifiedgame = false;

	if (M_CheckParm("-nopvs"))
		UsePVS=false;

    if (M_CheckParm("-timedemo"))
		singletics=true;
    nomonsters = M_CheckParm ("-nomonsters");
    respawnparm = M_CheckParm ("-respawn");
    fastparm = M_CheckParm ("-fast");
    devparm = M_CheckParm ("-devparm");
    if (M_CheckParm ("-altdeath"))
		deathmatch = 2;
    else if (M_CheckParm ("-deathmatch"))
		deathmatch = 1;
    switch ( gamemode )
    {
	case retail:
		sprintf (title, "Registered/Ultimate DOOM");
		break;
	case shareware:
		sprintf (title, "Shareware");
		break;
	case registered:
		sprintf (title, "Registered");
		break;
	case commercial:
		sprintf (title, "DOOM 2: Hell on Earth");
		break;
		/*FIXME
		case pack_plut:
		sprintf (title,
		"                   "
		"DOOM 2: Plutonia Experiment v%i.%i"
		"                           ",
		DOOM_MAJOR_VERSION,DOOM_SUBVERSION);
		break;
		case pack_tnt:
		sprintf (title,
		"                     "
		"DOOM 2: TNT - Evilution v%i.%i"
		"                           ",
		DOOM_MAJOR_VERSION,DOOM_SUBVERSION);
		break;
		*/
	default:
		sprintf(title, "Custom IWad");
		break;
    }

    I_Printf("Doom3D v%s (%s)\n", DOOM_VERSION_TEXT, title);

    if (devparm)
		I_Printf(D_DEVSTR);

    // turbo option
    p=M_CheckParm ("-turbo");
    if (p)
    {
		int     scale = 200;
		extern int forwardmove[2];
		extern int sidemove[2];

		if (p<myargc-1)
			scale = atoi (myargv[p+1]);
		if (scale < 10)
			scale = 10;
		if (scale > 400)
			scale = 400;
		I_Printf ("turbo scale: %i%%\n",scale);
		forwardmove[0] = forwardmove[0]*scale/100;
		forwardmove[1] = forwardmove[1]*scale/100;
		sidemove[0] = sidemove[0]*scale/100;
		sidemove[1] = sidemove[1]*scale/100;
    }

    // add any files specified on the command line with -file wadfile
    // to the wad list
    //
    // convenience hack to allow -wart e m to add a wad file
    // prepend a tilde to the filename so wadfile will be reloadable
    p = M_CheckParm ("-wart");
    if (p)
    {
		myargv[p][4] = 'p';     // big hack, change to -warp

		// Map name handling.
		switch (gamemode )
		{
		case shareware:
		case retail:
		case registered:
			sprintf (file,"~"DEVMAPS"E%cM%c.wad",
				myargv[p+1][0], myargv[p+2][0]);
			I_Printf("Warping to Episode %s, Map %s.\n",
				myargv[p+1],myargv[p+2]);
			break;

		case commercial:
		default:
			p = atoi (myargv[p+1]);
			if (p<10)
				sprintf (file,"~"DEVMAPS"cdata/map0%i.wad", p);
			else
				sprintf (file,"~"DEVMAPS"cdata/map%i.wad", p);
			break;
		}
		D_AddFile (file);
    }

    p = M_CheckParm ("-file");
    if (p)
    {
		// the parms after p are wadfile/lump names,
		// until end of parms or another - preceded parm
		modifiedgame = true;            // homebrew levels
		while (++p != myargc && myargv[p][0] != '-')
			D_AddFile (myargv[p]);
    }

    p = M_CheckParm ("-playdemo");

    if (!p)
		p = M_CheckParm ("-timedemo");

    if (p && p < myargc-1)
    {
		sprintf (file,"%s.lmp", myargv[p+1]);
		D_AddFile (file);
		I_Printf("Playing demo %s.lmp.\n",myargv[p+1]);
    }

    // get skill / episode / map from parms
    startskill = sk_medium;
    startepisode = 1;
    startmap = 1;
    autostart = false;


    p = M_CheckParm ("-skill");
    if (p && p < myargc-1)
    {
		startskill = myargv[p+1][0]-'1';
		autostart = true;
    }

    p = M_CheckParm ("-episode");
    if (p && p < myargc-1)
    {
		startepisode = myargv[p+1][0]-'0';
		startmap = 1;
		autostart = true;
    }

    p = M_CheckParm ("-timer");
    if (p && p < myargc-1 && deathmatch)
    {
		int     time;
		time = atoi(myargv[p+1]);
		I_Printf("Levels will end after %d minute",time);
		if (time>1)
			I_Printf("s");
		I_Printf(".\n");
    }

#if 0
    p = M_CheckParm ("-avg");
    if (p && p < myargc-1 && deathmatch)
		I_Printf("Austin Virtual Gaming: Levels will end after 20 minutes\n");
#endif

    p = M_CheckParm ("-warp");
    if (p && p < myargc-1)
    {
		autostart = true;
		if (gamemode == commercial)
		{
			startmap = atoi (myargv[p+1]);
			startepisode=1;
		}
		else if (p<myargc-2)
		{
			startepisode = myargv[p+1][0]-'0';
			startmap = myargv[p+2][0]-'0';
		}
		else
			autostart=false;
    }

    Z_Init ();

    // init subsystems
	CO_Init();
    I_Printf ("V_Init: allocate screens.\n");

	if (M_CheckParm("-matrix"))
	{
		p=M_CheckParm("-minterleave");
		if (p&&p<myargc-1)
			matrixinfo.interleave=atoi(myargv[p+1]);
		else
			matrixinfo.interleave=1;
		p=M_CheckParm("-msep");
		if (p&&p<myargc-1)
			matrixinfo.seperation=atoi(myargv[p+1]);
		else
			matrixinfo.seperation=30;
		C_API.V_Init (ScreenWidth, ScreenHeight, InWindow, &matrixinfo);
	}
	else
		C_API.V_Init (ScreenWidth, ScreenHeight, InWindow, NULL);

    I_Printf ("W_Init: Init WADfiles.\n");
    W_InitMultipleFiles (wadfiles);

	if (gamemode==indetermined)
	{
		if (W_CheckNumForName("e4m1")>=0)
			gamemode=retail;
		else if (W_CheckNumForName("e4m1")>=0)
			gamemode=registered;
		else if (W_CheckNumForName("map01")>=0)
			gamemode=commercial;
		else
			gamemode=shareware;
	}
    // Check for -file in shareware
    if (modifiedgame)
    {
		// These are the lumps that will be checked in IWAD,
		// if any one is not present, execution will be aborted.
		char name[23][8]=
		{
			"e2m1","e2m2","e2m3","e2m4","e2m5","e2m6","e2m7","e2m8","e2m9",
				"e3m1","e3m3","e3m3","e3m4","e3m5","e3m6","e3m7","e3m8","e3m9",
				"dphoof","bfgga0","heada1","cybra1","spida1d1"
		};
		int i;

		if ( gamemode == shareware)
			I_Error("\nYou cannot -file with the shareware "
			"version. Register!");

		// Check for fake IWAD with right name,
		// but w/o all the lumps of the registered version.
		if (gamemode == registered)
			for (i = 0;i < 23; i++)
				if (W_CheckNumForName(name[i])<0)
					I_Error("\nThis is not the registered version.");
    }

    // If additonal PWAD files are used, print modified banner
    if (modifiedgame)
    {
		/*m*/I_Printf (
		"===========================================================================\n"
			"ATTENTION:  This version of DOOM has been modified.  If you would like to\n"
			"get a copy of the original game, call 1-800-IDGAMES or see the readme file.\n"
			"        You will not receive technical support for modified games.\n"
			"                    don't press enter to continue\n"
			"===========================================================================\n"
			);
		//getchar ();
    }

	if (gamemode==retail)
	{
		if (W_CheckNumForName("M_EPI4")<0)
		{
			gamemode=registered;
			I_Printf("Episode 4 not found\n");
		}
	}

    // Check and print which version is executed.
    switch ( gamemode )
    {
	case shareware:
	case indetermined:
		I_Printf (
			"===========================================================================\n"
			"                                Shareware!\n"
			"===========================================================================\n"
			);
		break;
	case registered:
	case retail:
	case commercial:
		I_Printf (
			"===========================================================================\n"
			"                 Commercial product - do not distribute!\n"
			"         Please report software piracy to the SPA: 1-800-388-PIR8\n"
			"===========================================================================\n"
			);
		break;

	default:
		// Ouch.
		break;
    }

    I_Printf ("M_Init: Init miscellaneous info.\n");
    M_Init ();

    M_InitShiftXForm();

    I_Printf ("I_Init: Setting up machine state.\n");
    I_Init ();

    I_Printf ("R_Init: Init DOOM refresh daemon - ");
    C_API.R_Init ();
    S_SetViewSize(screenblocks);

    I_Printf ("\nP_Init: Init Playloop state.\n");
    P_Init ();

    G_Init();

    I_Printf ("D_CheckNetGame: Checking network game status.\n");
    D_CheckNetGame ();

    I_Printf ("S_Init: Setting up sound.\n");
    S_Init (snd_SfxVolume /* *8 */, snd_MusicVolume /* *8*/ );

    I_Printf ("HU_Init: Setting up heads up display.\n");
    HU_Init ();

    I_Printf ("ST_Init: Init status bar.\n");
    ST_Init ();

    // check for a driver that wants intermission stats
    p = M_CheckParm ("-statcopy");
    if (p && p<myargc-1)
    {
		// for statistics driver
		extern  void*	statcopy;

		statcopy = (void*)atoi(myargv[p+1]);
		I_Printf ("External statistics registered.\n");
    }

    // start the apropriate game based on parms
    p = M_CheckParm ("-record");

    if (p && p < myargc-1)
    {
		G_RecordDemo (myargv[p+1]);
		autostart = true;
    }

    p = M_CheckParm ("-playdemo");
    if (p && p < myargc-1)
    {
		singledemo = true;              // quit after one demo
		G_DeferedPlayDemo (myargv[p+1]);
		D_DoomLoop ();  // never returns
    }

    p = M_CheckParm ("-timedemo");
    if (p && p < myargc-1)
    {
		G_TimeDemo (myargv[p+1]);
		D_DoomLoop ();  // never returns
    }

    p = M_CheckParm ("-loadgame");
    if (p && p < myargc-1)
    {
		if (M_CheckParm("-cdrom"))
			sprintf(file, "c:\\doomdata\\"SAVEGAMENAME"%c.dsg",myargv[p+1][0]);
		else
			sprintf(file, SAVEGAMENAME"%c.dsg",myargv[p+1][0]);
		G_LoadGame (file);
    }

	if (M_CheckParm("-nogun"))
	{
		ShowGun=false;
	}

	p=M_CheckParm("-viewangle");
	if (p&&(p<myargc-1))
	{
		C_API.R_SetViewAngleOffset(atoi(myargv[p+1]));
	}

	p=M_CheckParm("-viewoffs");
	if (p&&(p<myargc-1))
	{
		C_API.R_SetViewOffset(atoi(myargv[p+1]));
	}

    if ( gameaction != ga_loadgame )
    {
		if (autostart||netgame)
		{
			if (!netgame)
			{
				playeringame[1] = playeringame[2] = playeringame[3] = false;
				playeringame[0]=true;
			}
			//G_DeferedInitNew (startskill, startepisode, startmap);
			G_InitNew (startskill, startepisode, startmap);
		}
		else
			D_StartTitle ();                // start up intro loop

    }
    D_CheckWipeBusy();

    D_DoomLoop ();  // never returns
}