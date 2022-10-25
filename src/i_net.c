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
// Platform Specific Network code - Win32 version
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char
rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "i_system.h"
#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"

#include "doomstat.h"

#include "i_reg.h"
#include "c_interface.h"

#include "i_net.h"

#include <winsock.h>

#define MAPPED_PACKETS_PER_NODE	32
#define NUM_MAPPED_PACKETS (MAPPED_PACKETS_PER_NODE*MAXNETNODES)

int		DoomPort=0x666;

SOCKET	SendSocket;
SOCKET	RecvSocket;

SOCKADDR_IN sendaddress[MAXNETNODES];

int		MappedNode=-1;

typedef struct
{
	int			from;
	int			to;
	doomdata_t	dd;
}packetpacket_t;

#define NO_PACKET	-1

typedef struct
{
	int			numnodes;
	int			packets[MAXNETNODES];
	int			numpackets[MAXNETNODES];
	int			free;
}mapfileheader_t;

typedef struct
{
	int			next;
	int			from;
	int			length;
	doomdata_t	data;
}mapfilepacket_t;

HANDLE			hMemoryMap=NULL;
byte			*pSharedMemory=NULL;
mapfileheader_t	*pSharedHeader;
mapfilepacket_t	*SharedPackets;
HANDLE			hNetMutex=NULL;

void (*NetSend)(void);
void (*NetRecv)(void);

void I_CheckIfDrone(int flags)
{
	if (M_CheckParm("-left")||(flags&NDF_LEFT))
	{
		doomcom->drone=true;
		ShowGun=false;
		C_API.R_SetViewAngleOffset(ANG90);
	}
	else if (M_CheckParm("-right")||(flags&NDF_RIGHT))
	{
		doomcom->drone=true;
		ShowGun=false;
		C_API.R_SetViewAngleOffset(ANG270);
	}
	else if (M_CheckParm("-back")||(flags&NDF_BACK))
	{
		doomcom->drone=true;
		ShowGun=false;
		C_API.R_SetViewAngleOffset(ANG180);
	}
	else if (M_CheckParm("-drone")||(flags&NDF_DRONE))
	{
		doomcom->drone=true;
	}
}

SOCKET UDPSocket(void)
{
	SOCKET	s;

	s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s==INVALID_SOCKET)
		I_Error("Can't create socket");
	return(s);
}

void BindToLocalPort(SOCKET s, int port)
{
	int			v;
	SOCKADDR_IN	address;

	ZeroMemory(&address, sizeof(address));
	address.sin_family=AF_INET;
	address.sin_addr.s_addr=INADDR_ANY;
	address.sin_port=port;

	v=bind(s, (SOCKADDR *)&address, sizeof(address));
	if (v==SOCKET_ERROR)
		I_Error("BindToLocalPort failed");
}

void PacketSend(void)
{
	int			c;
	doomdata_t	sw;

	sw.checksum=htonl(netbuffer->checksum);
	sw.player=netbuffer->player;
	sw.retransmitfrom=netbuffer->retransmitfrom;
	sw.starttic=netbuffer->starttic;
	sw.numtics=netbuffer->numtics;
	for (c=0;c<netbuffer->numtics;c++)
	{
		sw.cmds[c].forwardmove=netbuffer->cmds[c].forwardmove;
		sw.cmds[c].sidemove=netbuffer->cmds[c].sidemove;
		sw.cmds[c].angleturn=htons(netbuffer->cmds[c].angleturn);
		sw.cmds[c].consistancy=htons(netbuffer->cmds[c].consistancy);
		sw.cmds[c].chatchar=netbuffer->cmds[c].chatchar;
		sw.cmds[c].buttons=netbuffer->cmds[c].buttons;
	}


	c=sendto(SendSocket, (char *)&sw, doomcom->datalength
		,0, (SOCKADDR *)&sendaddress[doomcom->remotenode]
		,sizeof(sendaddress[doomcom->remotenode]));
//FUDGERETURN
	if (c==SOCKET_ERROR)
		I_Error("PacketSend Failed");
}

void PacketGet(void)
{
	int			i;
	int			c;
	SOCKADDR_IN	fromaddress;
	int			fromlen;
	doomdata_t	sw;

	fromlen=sizeof(fromaddress);
	c=recvfrom(RecvSocket, (char *)&sw, sizeof(sw), 0, (SOCKADDR *)&fromaddress, &fromlen);
	if (c==SOCKET_ERROR)
	{
		if (WSAGetLastError()!=WSAEWOULDBLOCK)
			I_Error("PacketGet Failed");
		doomcom->remotenode=-1;
		return;
	}

	for (i=0;i<doomcom->numnodes;i++)
	{
		if (fromaddress.sin_addr.s_addr==sendaddress[i].sin_addr.s_addr)
			break;
	}

	if (i==doomcom->numnodes)
	{
		doomcom->remotenode=-1;
		I_Error("Dodgy packet recieved");
		return;
	}
	doomcom->remotenode=i;
	doomcom->datalength=c;

    netbuffer->checksum = ntohl(sw.checksum);
    netbuffer->player = sw.player;
    netbuffer->retransmitfrom = sw.retransmitfrom;
    netbuffer->starttic = sw.starttic;
    netbuffer->numtics = sw.numtics;

    for (c=0;c<netbuffer->numtics;c++)
    {
		netbuffer->cmds[c].forwardmove = sw.cmds[c].forwardmove;
		netbuffer->cmds[c].sidemove = sw.cmds[c].sidemove;
		netbuffer->cmds[c].angleturn = ntohs(sw.cmds[c].angleturn);
		netbuffer->cmds[c].consistancy = ntohs(sw.cmds[c].consistancy);
		netbuffer->cmds[c].chatchar = sw.cmds[c].chatchar;
		netbuffer->cmds[c].buttons = sw.cmds[c].buttons;
    }
}

void AddNodeAddress(char *remotename)
{
	char		*portpos;
	SOCKADDR_IN	*addr;
	HOSTENT 	*hostentry;

	addr=&sendaddress[doomcom->numnodes++];
	portpos=strchr(remotename, ':');
	addr->sin_family=AF_INET;
	if (portpos)
	{
		addr->sin_port=htons((word)atoi(portpos+1));
		*portpos=0;
	}
	else
		addr->sin_port=htons((word)DoomPort);

	if (remotename[0]=='.')
		addr->sin_addr.s_addr=inet_addr(remotename+1);
	else
	{
		addr->sin_addr.s_addr=inet_addr(remotename);
		if (addr->sin_addr.s_addr==INADDR_NONE)
		{
			hostentry=gethostbyname(remotename);
			if (hostentry)
				addr->sin_addr.s_addr=*(int *)hostentry->h_addr_list[0];
		}
	}
	if (addr->sin_addr.s_addr==INADDR_NONE)
		I_Error("Unknown Host (%s)", remotename);
}

void I_InitPacketNetworkStart(void)
{
	WSADATA	wsd;

	I_Printf("I_InitNetwork:Multiplayer\n");
	netgame=true;
	WSAStartup(0x101, &wsd);
	doomcom->id=DOOMCOM_ID;
	doomcom->numnodes=1;
	I_Printf("Looking up Network names...\n");
}

void I_InitPacketNetworkFinish(void)
{
	u_long	v;
	int		i;

	RecvSocket=UDPSocket();
	BindToLocalPort(RecvSocket, htons((word)DoomPort));
	v=true;
	ioctlsocket(RecvSocket, FIONBIO, &v);

	SendSocket=UDPSocket();

	NetSend=PacketSend;
	NetRecv=PacketGet;

	doomcom->keynode=false;
	for (i=0;i<NumSplits;i++)
	{
		if ((doomcom->consoleplayer+i==0)&&!doomcom->drone)
			doomcom->keynode=true;
	}
}

void I_AddCLNetworkAddr(int i)
{
	char	remotename[256];

	while ((i<myargc)&&(myargv[i][0]!='-'))
	{
		strcpy(remotename, myargv[i]);
		AddNodeAddress(remotename);
		i++;
	}
}

void I_InitCLNetwork(int i)
{
	int		p;

	p=M_CheckParm("-port");
	if (p&&(p<myargc-1))
	{
		DoomPort=atoi(myargv[p+1]);
		I_Printf("Using Port %d\n", DoomPort);
	}
	doomcom->consoleplayer=myargv[++i][0]-'1';
	I_InitPacketNetworkStart();
	I_AddCLNetworkAddr(i+1);
	p=M_CheckParm("-players");
	if (p&&(p<myargc-1))
		doomcom->numplayers=atoi(myargv[p+1]);
	else
		doomcom->numplayers=doomcom->numnodes+NumSplits-1;
	I_InitPacketNetworkFinish();
}

void I_AddRegNetworkAddr(HKEY hkey)
{
	char	remotename[256];
	char	name[7];
	int		i;

	for (i=0;i<NUM_NETREG_NODES;i++)
	{
		remotename[0]=0;
		sprintf(name, "Node%d", i);
		RegReadString(hkey, name, remotename, 256);
		if (remotename[0])
		{
			AddNodeAddress(remotename);
		}
	}
}

void SplitSend(void)
{
	I_Error("SplitSend should never happen");
}

void SplitGet(void)
{
	I_Error("SplitGet should never happen");
}

void I_InitSplitNetwork(void)
{
	doomcom->consoleplayer=0;
	doomcom->numnodes=1;
	doomcom->numplayers=NumSplits;

	doomcom->id=DOOMCOM_ID;
	netgame=true;
	I_Printf("SplitScreen mode\n");

	doomcom->keynode=true;
	NetSend=SplitSend;
	NetRecv=SplitGet;
}

void I_InitRegNetwork(void)
{
	HKEY	hkey;
	LONG	rc;
	int		flags;

	rc=RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\FudgeFactor\\Doom3D\\Netgame", 0, KEY_QUERY_VALUE, &hkey);
	if (rc!=ERROR_SUCCESS)
		I_Error("Netgame Registry entries not valid, rerun setup");

	doomcom->numplayers=RegReadInt(hkey, "NumPlayers", 0);
	if (!doomcom->numplayers)
		I_Error("Netgame Registry entries not valid, rerun setup");

	DoomPort=RegReadInt(hkey, "Port", DoomPort);
	doomcom->consoleplayer=RegReadInt(hkey, "Player", 1)-1;
	flags=RegReadInt(hkey, "Flags", 0);
	if (flags&NDF_DEATHMATCH)
		deathmatch=1;
	if (flags&NDF_DEATH2)
		deathmatch=2;
	if (flags&NDF_SPLITONLY)
	{
		RegCloseKey(hkey);
		I_InitSplitNetwork();
		return;
	}
	I_CheckIfDrone(flags);
	I_InitPacketNetworkStart();
	I_AddRegNetworkAddr(hkey);
	I_InitPacketNetworkFinish();
	RegCloseKey(hkey);
}

void MapFileSend(void)
{
	int				next;
	mapfilepacket_t	*packet;
	int				other;

	other=doomcom->remotenode;
	if (other==0)
		other=MappedNode;
	else if (other<=MappedNode)
		other--;
	WaitForSingleObject(hNetMutex, INFINITE);
	if ((pSharedHeader->free!=NO_PACKET)&&(pSharedHeader->numpackets[other]<MAPPED_PACKETS_PER_NODE))
	{
		next=pSharedHeader->packets[other];
		pSharedHeader->packets[other]=pSharedHeader->free;
		packet=&SharedPackets[pSharedHeader->free];
		pSharedHeader->free=packet->next;
		packet->from=MappedNode;
		packet->next=next;
		packet->length=doomcom->datalength;
		memcpy(&packet->data, netbuffer, sizeof(doomdata_t));
		pSharedHeader->numpackets[other]++;
	}
	ReleaseMutex(hNetMutex);
}

void MapFileGet(void)
{
	int				srcpacket;

	WaitForSingleObject(hNetMutex, INFINITE);
	srcpacket=pSharedHeader->packets[MappedNode];
	if (srcpacket==NO_PACKET)
	{
		doomcom->remotenode=-1;
	}
	else
	{
		doomcom->remotenode=SharedPackets[srcpacket].from;
		if (doomcom->remotenode==MappedNode)
			doomcom->remotenode=0;
		else if (doomcom->remotenode<MappedNode)
			doomcom->remotenode++;

		doomcom->datalength=SharedPackets[srcpacket].length;
		memcpy(netbuffer, &SharedPackets[srcpacket].data, sizeof(doomdata_t));
		pSharedHeader->packets[MappedNode]=SharedPackets[srcpacket].next;
		SharedPackets[srcpacket].next=pSharedHeader->free;
		pSharedHeader->free=srcpacket;
		pSharedHeader->numpackets[MappedNode]--;
	}
	ReleaseMutex(hNetMutex);
}

void I_InitMapFileNetwork(int p)
{
	char	mapname[25];
	char	mutexname[26];
	int		i;


	p++;
	if (p<myargc)
		doomcom->consoleplayer=myargv[p++][0]-'1';
	else
		doomcom->consoleplayer=0;

	if (p<myargc)
		doomcom->numnodes=myargv[p][0]-'0';
	else
		doomcom->numnodes=2;
	i=M_CheckParm("-players");
	if (i&&(i<myargc-1))
		doomcom->numplayers=atoi(myargv[i+1]);
	else
		doomcom->numplayers=doomcom->numnodes+NumSplits-1;

	doomcom->id=DOOMCOM_ID;
	sprintf(mapname, "DoomSharedMemoryFile%04x", DoomPort);
	sprintf(mutexname, "DoomLocalNetworkMutex%04x", DoomPort);
	netgame=true;
	I_Printf("Multiplayer(Local)\n");

	if (doomcom->consoleplayer||doomcom->drone)
		doomcom->keynode=false;
	else
		doomcom->keynode=true;

	if (doomcom->keynode)
	{
		hMemoryMap=CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE, 0, sizeof(mapfilepacket_t)*NUM_MAPPED_PACKETS+sizeof(mapfileheader_t), mapname);
		hNetMutex=CreateMutex(NULL, FALSE, mutexname);
	}
	else
	{
		hMemoryMap=OpenFileMapping(FILE_MAP_WRITE, FALSE, mapname);
		hNetMutex=OpenMutex(MUTEX_ALL_ACCESS, FALSE, mutexname);
	}
	if (!hNetMutex)
		I_Error("Unable to setup syncronization mutex");

	if (hMemoryMap==NULL)
		I_Error("Unable to map shared memory file");
	pSharedMemory=(byte *)MapViewOfFile(hMemoryMap, FILE_MAP_WRITE, 0, 0, 0);
	if (!pSharedMemory)
		I_Error("FileMapping Failed");

	WaitForSingleObject(hNetMutex, INFINITE);
	pSharedHeader=(mapfileheader_t *)pSharedMemory;
	SharedPackets=(mapfilepacket_t *)&pSharedMemory[sizeof(mapfileheader_t)];
	if (doomcom->keynode)
	{
		MappedNode=0;

		pSharedHeader->numnodes=1;
		pSharedHeader->free=0;
		for (i=0;i<MAXNETNODES;i++)
		{
			pSharedHeader->packets[i]=NO_PACKET;
			pSharedHeader->numpackets[i]=0;
		}
		for (i=0;i<NUM_MAPPED_PACKETS-1;i++)
			SharedPackets[i].next=i+1;
		SharedPackets[NUM_MAPPED_PACKETS-1].next=NO_PACKET;
	}
	else
	{
		MappedNode=pSharedHeader->numnodes;
		pSharedHeader->numnodes++;
	}
	ReleaseMutex(hNetMutex);

	NetSend=MapFileSend;
	NetRecv=MapFileGet;
}

//
// I_InitNetwork
//
void I_InitNetwork (void)
{
	int		p;

	doomcom = malloc (sizeof (*doomcom) );
    memset (doomcom, 0, sizeof(*doomcom) );

	p=M_CheckParm("-dup");
	if (p&&(p<myargc-1))
	{
		doomcom->ticdup=myargv[p+1][0]-'0';
		if (doomcom->ticdup<1)
			doomcom->ticdup=1;
		if (doomcom->ticdup>9)
			doomcom->ticdup=9;
	}
	else
		doomcom->ticdup=1;
    doomcom->extratics=0;

	p=M_CheckParm("-netreg");//dodgy hack to load settings from registry(for setup program)
	if (p)
	{
		I_InitRegNetwork();
		return;
	}

	I_CheckIfDrone(0);
	p=M_CheckParm("-net");
	if (p)
	{
		I_InitCLNetwork(p);
		return;
	}

//for debuging network code on one machine 'cos normal code works out who's who by ip address
	p=M_CheckParm("-netlocal");
	if (p)
	{
		I_InitMapFileNetwork(p);
		return;
	}

	if (NumSplits>1)
	{
		I_InitSplitNetwork();
		return;
	}
	I_Printf("Single Player\n");
	// single player game
	netgame = false;
	doomcom->id = DOOMCOM_ID;
	doomcom->numplayers = doomcom->numnodes = 1;
	doomcom->deathmatch = false;
	doomcom->consoleplayer = 0;

}


void I_NetCmd (void)
{
	if (doomcom->command==CMD_SEND)
	{
		NetSend();
	}
	else if (doomcom->command==CMD_GET)
		NetRecv();
	else
		I_Error("Bad net cmd %d", doomcom->command);
}

