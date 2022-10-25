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
//	DOOM Network game communication and protocol,
//	all OS independend parts.
//
//-----------------------------------------------------------------------------
#ifdef RCSID

static const char rcsid[] = "$Id: d_net.c,v 1.3 1997/02/03 22:01:47 b1 Exp $";
#endif

#include "m_menu.h"
#include "i_system.h"
//#include "i_video.h"
#include "i_net.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "c_interface.h"
#include "i_windoz.h"
#include "m_argv.h"
#include "t_tables.h"
#include "co_console.h"

#define	NCMD_EXIT		0x80000000
#define	NCMD_RETRANSMIT		0x40000000
#define	NCMD_SETUP		0x20000000
#define	NCMD_KILL		0x10000000	// kill game
#define	NCMD_CHECKSUM	 	0x0fffffff


doomcom_t*	doomcom;
doomdata_t*	netbuffer;		// points inside doomcom
dboolean	ShowGun=true;

//
// NETWORKING
//
// gametic is the tic about to (or currently being) run
// maketic is the tick that hasn't had control made for it yet
// nettics[] has the maketics for all players
//
// a gametic cannot be run until nettics[] > gametic for all players
//
#define	RESENDCOUNT	10
#define	PL_DRONE	0x80	// bit flag in doomdata->player

ticcmd_t	localcmds[MAX_SPLITS][BACKUPTICS];

ticcmd_t	netcmds[MAXPLAYERS][BACKUPTICS];
int         nettics[MAXNETNODES];
dboolean	nodeingame[MAXNETNODES];		// set false as nodes leave game
dboolean	remoteresend[MAXNETNODES];		// set when local needs tics
int			resendto[MAXNETNODES];			// set when remote needs tics
int			resendcount[MAXNETNODES];

int			nodeforplayer[MAXPLAYERS];

int         maketic=0;
int			lastnettic=0;
int			skiptics=0;
int			ticdup=0;
int			maxsend=0;	// BACKUPTICS/(2*ticdup)-1


void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t *cmd, int split);
void D_DoAdvanceDemo (void);

int				reboundpackets;
int				reboundhead;
doomdata_t		reboundstore[MAX_SPLITS];



//
//
//
int NetbufferSize (void)
{
    return (int)&(((doomdata_t *)0)->cmds[netbuffer->numtics]);
}

//
// Checksum
//
unsigned NetbufferChecksum (void)
{
    unsigned		c;
    int		i,l;

    c = 0x1234567;

    // FIXME -endianess?
#ifdef NORMALUNIX
    return 0;			// byte order problems
#endif

    l = (NetbufferSize () - (int)&(((doomdata_t *)0)->retransmitfrom))/4;
    for (i=0 ; i<l ; i++)
		c += ((unsigned *)&netbuffer->retransmitfrom)[i] * (i+1);

    return c & NCMD_CHECKSUM;
}

//
//
//
int ExpandTics (int low)
{
    int	delta;

    delta = low - (maketic&0xff);

    if (delta >= -64 && delta <= 64)
		return (maketic&~0xff) + low;
    if (delta > 64)
		return (maketic&~0xff) - 256 + low;
    if (delta < -64)
		return (maketic&~0xff) + 256 + low;

    I_Error ("ExpandTics: strange value %i at maketic %i",low,maketic);
    return 0;
}



//
// HSendPacket
//
void
HSendPacket
(int	node,
 int	flags )
{
	int		tail;

    netbuffer->checksum = NetbufferChecksum () | flags;

    if (!node)
    {
		if (reboundpackets==MAX_SPLITS)
			I_Error ("too many rebound packets");
		tail=reboundhead+reboundpackets;
		if (tail>=MAX_SPLITS)
			tail-=MAX_SPLITS;
		reboundstore[tail] = *netbuffer;
		reboundpackets++;
		return;
    }

    if (demoplayback)
		return;

    if (!netgame)
		I_Error ("Tried to transmit to another node");

    doomcom->command = CMD_SEND;
    doomcom->remotenode = node;
    doomcom->datalength = NetbufferSize ();

    if (debugfile)
    {
		int		i;
		int		realretrans;
		if (netbuffer->checksum & NCMD_RETRANSMIT)
			realretrans = ExpandTics (netbuffer->retransmitfrom);
		else
			realretrans = -1;

		fprintf (debugfile,"send (%i + %i, R %i) [%i] ",
			ExpandTics(netbuffer->starttic),
			netbuffer->numtics, realretrans, doomcom->datalength);

		for (i=0 ; i<doomcom->datalength ; i++)
			fprintf (debugfile,"%i ",((byte *)netbuffer)[i]);

		fprintf (debugfile,"\n");
    }

    I_NetCmd ();
}

//
// HGetPacket
// Returns false if no packet is waiting
//
dboolean HGetPacket (void)
{
    if (reboundpackets>0)
    {
		*netbuffer = reboundstore[reboundhead++];
		if (reboundhead==MAX_SPLITS)
			reboundhead=0;
		reboundpackets--;
		doomcom->remotenode = 0;
		return true;
    }

    if (!netgame)
		return false;

    if (demoplayback)
		return false;

	if (doomcom->numnodes==1)
		return false;

    doomcom->command = CMD_GET;
    I_NetCmd ();

    if (doomcom->remotenode == -1)
		return false;

    if (doomcom->datalength != NetbufferSize ())
    {
		if (debugfile)
			fprintf (debugfile,"bad packet length %i\n",doomcom->datalength);
		return false;
    }

    if (NetbufferChecksum () != (netbuffer->checksum&NCMD_CHECKSUM) )
    {
		if (debugfile)
			fprintf (debugfile,"bad packet checksum\n");
		return false;
    }

    if (debugfile)
    {
		int		realretrans;
		int	i;

		if (netbuffer->checksum & NCMD_SETUP)
			fprintf (debugfile,"setup packet\n");
		else
		{
			if (netbuffer->checksum & NCMD_RETRANSMIT)
				realretrans = ExpandTics (netbuffer->retransmitfrom);
			else
				realretrans = -1;

			fprintf (debugfile,"get %i = (%i + %i, R %i)[%i] ",
				doomcom->remotenode,
				ExpandTics(netbuffer->starttic),
				netbuffer->numtics, realretrans, doomcom->datalength);

			for (i=0 ; i<doomcom->datalength ; i++)
				fprintf (debugfile,"%i ",((byte *)netbuffer)[i]);
			fprintf (debugfile,"\n");
		}
    }
    return true;
}


//
// GetPackets
//
char    exitmsg[80];

void GetPackets (void)
{
    int			netconsole;
    int			netnode;
    ticcmd_t	*src, *dest;
    int			realend;
    int			realstart;
	dboolean	drone;
	int			split;

    while ( HGetPacket() )
    {
		if (netbuffer->checksum & NCMD_SETUP)
			continue;		// extra setup packet

		netconsole = netbuffer->player & ~PL_DRONE;
		if (netbuffer->player&PL_DRONE)
			drone=true;
		else
			drone=false;
		netnode = doomcom->remotenode;

		// to save bytes, only the low byte of tic numbers are sent
		// Figure out what the rest of the bytes are
		realstart = ExpandTics (netbuffer->starttic);
		realend = (realstart+netbuffer->numtics);

		// check for exiting the game
		if (netbuffer->checksum & NCMD_EXIT)
		{
			if (!nodeingame[netnode])
				continue;
			nodeingame[netnode] = false;
			if (!drone)
			{
				playeringame[netconsole] = false;
				strcpy (exitmsg, "Player 1 left the game");
				exitmsg[7] += netconsole;
				for (split=0;split<NumSplits;split++)
					players[consoleplayer+split].message = exitmsg;
			}
			if (demorecording)
				G_CheckDemoStatus ();
			continue;
		}

		// check for a remote game kill
		if (netbuffer->checksum & NCMD_KILL)
			I_Error ("Killed by network driver");

		if (!drone)
			nodeforplayer[netconsole] = netnode;

		// check for retransmit request
		if ( resendcount[netnode] <= 0
			&& (netbuffer->checksum & NCMD_RETRANSMIT) )
		{
			resendto[netnode] = ExpandTics(netbuffer->retransmitfrom);
			if (debugfile)
				fprintf (debugfile,"retransmit from %i\n", resendto[netnode]);
			resendcount[netnode] = RESENDCOUNT;
		}
		else
			resendcount[netnode]--;

		// check for out of order / duplicated packet
//FSPLIT: re-implement duplicated packet check
#if 0
		if (realend == nettics[netnode])
			continue;
#else
		nettics[netnode]=realstart;
#endif
		if (realend < nettics[netnode])
		{
			if (debugfile)
				fprintf (debugfile,
				"out of order packet (%i + %i)\n" ,
				realstart,netbuffer->numtics);
			continue;
		}

		// check for a missed packet
		if (realstart > nettics[netnode])
		{
			// stop processing until the other system resends the missed tics
			if (debugfile)
				fprintf (debugfile,
				"missed tics from %i (%i - %i)\n",
				netnode, realstart, nettics[netnode]);
			remoteresend[netnode] = true;
			continue;
		}

		// update command store from the packet
        {
			int		start;

			remoteresend[netnode] = false;

			start = nettics[netnode] - realstart;
			src = &netbuffer->cmds[start];

			while (nettics[netnode] < realend)
			{
				dest = &netcmds[netconsole][nettics[netnode]%BACKUPTICS];
				nettics[netnode]++;
				if (!drone)
					*dest = *src;
				src++;
			}
		}
    }
}


//
// NetUpdate
// Builds ticcmds for console player,
// sends out a packet
//
int      gametime=0;

void NetUpdate (void)
{
    int             nowtime;
    int             newtics;
    int				i,j;
    int				realstart;
    int				gameticdiv;
    int				split;

    // check time
    nowtime = I_GetTime ()/ticdup;
    newtics = nowtime - gametime;
    gametime = nowtime;

    if (newtics <= 0) 	// nothing new to update
		goto listen;

    if (skiptics <= newtics)
    {
		newtics -= skiptics;
		skiptics = 0;
    }
    else
    {
		skiptics -= newtics;
		newtics = 0;
    }

    // build new ticcmds for console player(s)
    gameticdiv = gametic/ticdup;
    for (i=0 ; i<newtics ; i++)
    {
		I_StartTic ();
		D_ProcessEvents ();
		if (maketic - gameticdiv >= BACKUPTICS/2-1)
			break;          // can't hold any more
		//I_Printf ("mk:%i ",maketic);
		for (split=0;split<NumSplits;split++)
			G_BuildTiccmd (&localcmds[split][maketic%BACKUPTICS], split);
		maketic++;
    }

    if (singletics)
		return;         // singletic update is syncronous


    // send the packet to the other nodes
	for (i=0 ; i<doomcom->numnodes ; i++)
		if (nodeingame[i])
		{
			netbuffer->starttic = realstart = resendto[i];
			netbuffer->numtics = maketic - realstart;
			if (netbuffer->numtics > BACKUPTICS)
				I_Error ("NetUpdate: netbuffer->numtics > BACKUPTICS");

			resendto[i] = maketic - doomcom->extratics;

			for (split=0;split<NumSplits;split++)
			{
				netbuffer->player = consoleplayer+split;
				if (doomcom->drone)
					netbuffer->player|=PL_DRONE;
				for (j=0 ; j< netbuffer->numtics ; j++)
					netbuffer->cmds[j] =
					localcmds[split][(realstart+j)%BACKUPTICS];

				if (remoteresend[i])
				{
					netbuffer->retransmitfrom = nettics[i];
					HSendPacket (i, NCMD_RETRANSMIT);
				}
				else
				{
					netbuffer->retransmitfrom = 0;
					HSendPacket (i, 0);
				}
			}
		}
		// listen for other packets
listen:
		GetPackets ();
}



//
// CheckAbort
//
void CheckAbort (void)
{
    event_t *ev;
    int		stoptic;

    stoptic = I_GetTime () + 2;
    while (I_GetTime() < stoptic)
	{
		Sleep(1);
		I_StartTic ();
	    I_ProcessWindows();
	}

    I_StartTic ();
    for ( ; eventtail != eventhead
		; eventtail = (++eventtail)&(MAXEVENTS-1) )
    {
		ev = &events[eventtail];
		if (ev->type == ev_keydown && ev->data1 == KEY_ESCAPE)
			I_Error ("Network game synchronization aborted.");
    }
}


//
// D_ArbitrateNetStart
//
void D_ArbitrateNetStart (void)
{
    int			i;
    dboolean	gotinfo[MAXNETNODES];

    autostart = true;
    memset (gotinfo,0,sizeof(gotinfo));

    //if (doomcom->consoleplayer||doomcom->drone)
    if (doomcom->keynode)
    {
		// key player, send the setup info
		I_Printf ("sending network start info...\n");
		do
		{
			CheckAbort ();
			for (i=1 ; i<doomcom->numnodes ; i++)
			{
				netbuffer->retransmitfrom = startskill;
				if (deathmatch)
					netbuffer->retransmitfrom |= (deathmatch<<6);
				if (nomonsters)
					netbuffer->retransmitfrom |= 0x20;
				if (respawnparm)
					netbuffer->retransmitfrom |= 0x10;
				netbuffer->starttic = startepisode * 64 + startmap;
				netbuffer->player = DOOM_NET_VERSION;
				netbuffer->numtics = 0;
				HSendPacket (i, NCMD_SETUP);
			}

#if 1
			Sleep(100);
			for(i = 10 ; i  &&  HGetPacket(); --i)
			{
				if((doomcom->remotenode&0x7f) < MAXNETNODES)
					gotinfo[doomcom->remotenode&0x7f] = true;
			}
#else
			while (HGetPacket ())
			{
				gotinfo[netbuffer->player&0x7f] = true;
			}
#endif

			for (i=1 ; i<doomcom->numnodes ; i++)
				if (!gotinfo[i])
					break;
		} while (i < doomcom->numnodes);
    }
	else
    {
		// listen for setup info from key player
		I_Printf ("listening for network start info...\n");
		while (1)
		{
			CheckAbort ();
			if (!HGetPacket ())
				continue;
			if (netbuffer->checksum & NCMD_SETUP)
			{
				if (netbuffer->player != DOOM_NET_VERSION)
					I_Error ("Different DOOM versions cannot play a net game!");
				startskill = netbuffer->retransmitfrom & 15;
				deathmatch = (netbuffer->retransmitfrom & 0xc0) >> 6;
				nomonsters = (netbuffer->retransmitfrom & 0x20) > 0;
				respawnparm = (netbuffer->retransmitfrom & 0x10) > 0;
				startmap = netbuffer->starttic & 0x3f;
				startepisode = netbuffer->starttic >> 6;
				return;
			}
		}
    }
}

//
// D_CheckNetGame
// Works out player numbers among the net participants
//

void D_CheckNetGame (void)
{
    int             i;

	reboundhead=0;
	reboundpackets=0;

    for (i=0 ; i<MAXNETNODES ; i++)
    {
		nodeingame[i] = false;
       	nettics[i] = 0;
		remoteresend[i] = false;	// set when local needs tics
		resendto[i] = 0;		// which tic to start sending
    }

	if (M_CheckParm("-netkill"))
	{
		netkill=true;
		netcheat=true;
	}
	if (M_CheckParm("-netcheat"))
		netcheat=true;
    // I_InitNetwork sets doomcom and netgame
    I_InitNetwork ();
    if (doomcom->id != DOOMCOM_ID)
		I_Error ("Doomcom buffer invalid!");
	if (doomcom->numplayers==1)
		netcheat=true;

    netbuffer = &doomcom->data;
	consoleplayer=doomcom->consoleplayer;

	if (doomcom->numplayers>MAXPLAYERS)
		I_Error("Can't have more than %d players", MAXPLAYERS);
	if (consoleplayer+NumSplits>doomcom->numplayers)
		I_Error("Players must be numbered 1-%d", doomcom->numplayers);

    if (netgame)
		D_ArbitrateNetStart ();

    I_Printf ("startskill %i  deathmatch: %i  startmap: %i  startepisode: %i\n",
		startskill, deathmatch, startmap, startepisode);

    // read values out of doomcom
    ticdup = doomcom->ticdup;
    maxsend = BACKUPTICS/(2*ticdup)-1;
    if (maxsend<1)
		maxsend = 1;
    for (i=0 ; i<doomcom->numplayers ; i++)
		playeringame[i] = true;
    for (i=0 ; i<doomcom->numnodes ; i++)
		nodeingame[i] = true;

	if (NumSplits>1)
	{
		I_Printf("Splitscreen Mode, players %d-%d of %d (%d nodes)\n", doomcom->consoleplayer+1, doomcom->consoleplayer+NumSplits, doomcom->numplayers, doomcom->numnodes);
	}
	else
	{
	    I_Printf ("player %i of %i (%i nodes)\n",
			doomcom->consoleplayer+1, doomcom->numplayers, doomcom->numnodes);
	}
}


//
// D_QuitNetGame
// Called before quitting to leave a net game
// without hanging the other players
//
void D_QuitNetGame (void)
{
    int             i, j;
    int				split;

    if (debugfile)
		fclose (debugfile);

//used to check consoleplayer==-1, seems pointless+didn't fit
    if (!netgame || !usergame || demoplayback)
		return;

    // send a bunch of packets for security
    for (split=0;split<NumSplits;split++)
    {
		netbuffer->player = consoleplayer+split;
		if (doomcom->drone)
			netbuffer->player|=PL_DRONE;
		netbuffer->numtics = 0;
		for (i=0 ; i<4 ; i++)
		{
			for (j=1 ; j<doomcom->numnodes ; j++)
				if (nodeingame[j])
				{
					HSendPacket (j, NCMD_EXIT);
					Sleep(100);
				}
		}
	}
}

extern dboolean advancedemo;


//
// TryRunTics
//
int	frametics[4];
int	frameon=0;
int	frameskip[4];
int	oldnettics=0;

void TryRunTics (void)
{
    int		i;
    int		lowtic;
    int		entertic;
    static int	oldentertics=0;
    int		realtics;
    int		availabletics;
    int		counts;

    // get real tics
    entertic = I_GetTime ()/ticdup;
    realtics = entertic - oldentertics;
    oldentertics = entertic;
    // get available tics
    NetUpdate ();
    lowtic = MAXINT;
    for (i=0 ; i<doomcom->numnodes ; i++)
    {
		if (nodeingame[i])
		{
			if (nettics[i] < lowtic)
				lowtic = nettics[i];
		}
    }
    availabletics = lowtic - gametic/ticdup;
    // decide how many tics to run
    if (realtics < availabletics-1)
		counts = realtics+1;
    else if (realtics < availabletics)
		counts = realtics;
    else
		counts = availabletics;

    if (counts < 1)
		counts = 1;

    frameon++;

    if (debugfile)
		fprintf (debugfile,
		"=======real: %i  avail: %i  game: %i\n",
		realtics, availabletics,counts);

#if 0
    if (!demoplayback)
    {
		// ideally nettics[0] should be 1 - 3 tics above lowtic
		// if we are consistantly slower, speed up time
		for (i=0 ; i<MAXPLAYERS ; i++)
			if (playeringame[i])//wtf???????
				break;
			if (consoleplayer == i)
			{
				// the key player does not adapt
			}
			else
			{
				if (nettics[0] <= nettics[nodeforplayer[i]])
				{
					gametime--;
					// I_Printf ("-");
				}
				frameskip[frameon&3] = (oldnettics > nettics[nodeforplayer[i]]);
				oldnettics = nettics[0];
				if (frameskip[0] && frameskip[1] && frameskip[2] && frameskip[3])
				{
					skiptics = 1;
					// I_Printf ("+");
				}
			}
    }// demoplayback
#endif

    // wait for new tics if needed
    while (lowtic < gametic/ticdup + counts)
    {
		Sleep(1);
		I_ProcessWindows();
		NetUpdate ();
		lowtic = MAXINT;
		for (i=0 ; i<doomcom->numnodes ; i++)
			if (nodeingame[i] && nettics[i] < lowtic)
				lowtic = nettics[i];
			if (lowtic < gametic/ticdup)
				I_Error ("TryRunTics: lowtic < gametic");
			// don't stay in here forever -- give the menu a chance to work
			if (I_GetTime ()/ticdup - entertic >= 20)
			{
				M_Ticker ();
				return;
			}
    }

    // run the count * ticdup dics
    while (counts--)
    {
		for (i=0 ; i<ticdup ; i++)
		{
			if (gametic/ticdup > lowtic)
				I_Error ("gametic>lowtic");
			if (advancedemo)
				D_DoAdvanceDemo ();
			M_Ticker ();
			G_Ticker ();
			CO_Ticker();
			gametic++;

			// modify command for duplicated tics
			if (i != ticdup-1)
			{
				ticcmd_t	*cmd;
				int			buf;
				int			j;

				buf = (gametic/ticdup)%BACKUPTICS;
				for (j=0 ; j<MAXPLAYERS ; j++)
				{
					cmd = &netcmds[j][buf];
					cmd->chatchar = 0;
					if (cmd->buttons & BT_SPECIAL)
						cmd->buttons = 0;
				}
			}
		}
		NetUpdate ();	// check for new console commands
    }
}
