//I_Music.c
//Doom music stuff
#include "z_zone.h"

#define MAX_MIDI_EVENTS 2048

#include "i_system.h"
#include "i_sound.h"
#include "i_windoz.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_swap.h"
#include "w_wad.h"

#include "doomdef.h"
#include "doomstat.h"
#include "d_stuff.h"

#include <stdlib.h>

#include <mmsystem.h>

HMIDISTRM       hMidiStream=NULL;
int				MidiDevice;

#define MIDI_BLOCK_SIZE 2048
#define MAX_BLOCKS 1

typedef struct
{
	DWORD	time;
	DWORD	ID;
	byte	data[3];
	byte	type;
}MidiEvent_t;

typedef struct
{
	DWORD	ID;          // identifier "MUS" 0x1A
	WORD	scoreLen;
	WORD	scoreStart;
	WORD	channels;	// count of primary channels
	WORD	sec_channels;	// count of secondary channels
	WORD	instrCnt;
	WORD	dummy;
}musheader_t;

typedef struct
{
	int			numevents;
	int			nextevent;
	MidiEvent_t	*midievents;
	MIDIHDR		header[2];
}songinfo_t;

byte		MidiControlers[10]={0, 0, 1, 7, 10, 11, 91, 93, 64, 67};
dboolean	started=false;

songinfo_t		*CurrentSong=NULL;
dboolean		loopsong=false;

byte XLateMUSControl(byte control)
{
	switch (control)
	{
	case 10:
		return(120);
	case 11:
		return(123);
	case 12:
		return(126);
	case 13:
		return(127);
	case 14:
		return(121);
	}
	I_Error("Unknown control %d", control);
	return(0);
}

static int GetSongLength(byte *data)
{
	dboolean	done;
	int			events;
	musheader_t	*header;
	dboolean	time;

	header=(musheader_t *)data;
	data+=header->scoreStart;
	events=0;
	done=false;
	time=false;
	while (!done)
	{
		if (*data&0x80)
			time=true;
		switch((*(data++)>>4)&7)
		{
		case 1:
			if (*(data++)&0x80)
				data++;
			break;
		case 0:
		case 2:
		case 3:
			data++;
			break;
		case 4:
			data+=2;
			break;
		default:
			done=true;
			break;
		}
		events++;
		if (time)
		{
			while (*data&0x80)
				data++;
			data++;
			time=false;
		}
	}
	return(events+2);
}

static void Mus2Midi(byte *MusData, MidiEvent_t *MidiEvents)
{
	musheader_t	*header;
	byte		*score;
	int			spos;
	MidiEvent_t	*event;
	int			channel;
	int			etype;
	int			delta;
	int			finished;
	byte		channelvol[16];
	int			count;

	count=GetSongLength(MusData);
	header=(musheader_t *)MusData;
	if (header->ID!=0x1A53554D)//"MUS"<EOF>
		I_Error("Not a MUS file");
	score=(byte *)&MusData[header->scoreStart];
	event=MidiEvents;
	event->time=0;
	event->ID=0;
	event->type=MEVT_TEMPO;
	event->data[0]=0x00;
	event->data[1]=0x80;//not sure how to work this out, should be 140bpm
	event->data[2]=0x02;//but it's guessed so it sounds about right
	event++;

	event->time=0;
	event->ID=0;
	event->type=MEVT_TEMPO;
	event->data[0]=0x00;
	event->data[1]=0x80;//not sure how to work this out, should be 140bpm
	event->data[2]=0x02;//but it's guessed so it sounds about right
	event++;

	delta=0;
	spos=0;

	finished=false;
	while (true)
	{
		event->time=delta;
		delta=0;
		event->ID=0;
		etype=(score[spos]>>4)&7;
		event->type=MEVT_SHORTMSG;
		channel=score[spos]&15;
		if (channel==9)
			channel=15;
		else if (channel==15)
			channel=9;
		if (score[spos]&0x80)
			delta=-1;
		spos++;
		switch(etype)
		{
		case 0:
			event->data[0]=channel|0x80;
			event->data[1]=score[spos++];
			event->data[2]=channelvol[channel];
			break;
		case 1:
			event->data[0]=channel|0x90;
			event->data[1]=score[spos]&127;
			if (score[spos]&128)
				channelvol[channel]=score[++spos];
			spos++;
			event->data[2]=channelvol[channel];
			break;
		case 2:
			event->data[0]=channel|0xe0;
			event->data[1]=(score[spos]<<7)&0x7f;
			event->data[2]=score[spos++]>>7;
			break;
		case 3:
			event->data[0]=channel|0xb0;
			event->data[1]=XLateMUSControl(score[spos++]);
			event->data[2]=0;
			break;
		case 4:
			if (score[spos])
			{
				event->data[0]=channel|0xb0;
				event->data[1]=MidiControlers[score[spos++]];
				event->data[2]=score[spos++];
			}
			else
			{
				event->data[0]=channel|0xc0;
				spos++;
				event->data[1]=score[spos++];
				event->data[2]=64;
			}
			break;
		default:
			finished=true;
			break;
		}
		if (finished)
			break;

		event++;
		count--;
		if (count<0)
			I_Error("Overflow");
		if (delta==-1)
		{
			delta=0;
			while (score[spos]&128)
			{
				delta<<=7;
				delta+=score[spos++]&127;
			}
			delta+=score[spos++];
		}
	}
}

//
// MUSIC API.
//
void I_InitMusic(void)
{
	MMRESULT	rc;
	int			numdev;

	if (M_CheckParm("-nomusic"))
		return;

	numdev=midiOutGetNumDevs();
	if (numdev==0)
		return;

	if (MidiDevice>=numdev)
		MidiDevice=numdev-1;
	//rc=midiStreamOpen(&hMidiStream, (LPUINT)&MidiDevice, 1, NULL, NULL, CALLBACK_NULL);
	rc=midiStreamOpen(&hMidiStream, (LPUINT)&MidiDevice, 1, 0, 0, CALLBACK_NULL);
	if (rc!=MMSYSERR_NOERROR)
	{
		hMidiStream=NULL;
		I_Printf("I_InitMusic:music init failed\n");
		return;
	}
	started=false;
}

void I_ShutdownMusic(void)
{
	if (hMidiStream)
	{
		midiStreamStop(hMidiStream);
		started=false;
		midiStreamClose(hMidiStream);
		hMidiStream=NULL;
	}
}

void I_PlaySong(int handle, int looping)
{
	if (!(handle&&hMidiStream))
		return;
	loopsong=looping;
	CurrentSong=(songinfo_t *)handle;
}

void I_PauseSong (int handle)
{
	if (!hMidiStream)
		return;
	//midiStreamPause(hMidiStream);
}

void I_ResumeSong (int handle)
{
	if (!hMidiStream)
		return;
	//midiStreamRestart(hMidiStream);
}

void I_StopMusic(songinfo_t *song)
{
	int			i;

	if (!(song&&hMidiStream))
		return;

	loopsong=false;
	//midiStreamStop(hMidiStream);
	midiOutReset((HMIDIOUT)hMidiStream);
	started=false;

	for (i=0;i<2;i++)
	{
		if (song->header[i].lpData)
		{
			midiOutUnprepareHeader((HMIDIOUT)hMidiStream, &song->header[i], sizeof(MIDIHDR));
			song->header[i].lpData=NULL;
			song->header[i].dwFlags=MHDR_DONE|MHDR_ISSTRM;
		}
	}
	song->nextevent=0;
}

void I_StopSong(int handle)
{
	songinfo_t	*song;

	if (!(handle&&hMidiStream))
		return;

	song=(songinfo_t *)handle;

	I_StopMusic(song);

	if (song==CurrentSong)
		CurrentSong=NULL;
}

void I_UnRegisterSong(int handle)
{
	songinfo_t	*song;

	if (!(handle&&hMidiStream))
		return;

	I_StopSong(handle);

	song=(songinfo_t *)handle;
	free(song->midievents);
	free(song);
}

int I_RegisterSong(void* data)
{
	songinfo_t	*song;
	int			i;

	if (!hMidiStream)
		return(0);
	song=(songinfo_t *)malloc(sizeof(songinfo_t));
	song->numevents=GetSongLength((byte *)data);
	song->nextevent=0;
	song->midievents=(MidiEvent_t *)malloc(song->numevents*sizeof(MidiEvent_t));
	Mus2Midi((byte *)data, song->midievents);
	for (i=0;i<2;i++)
	{
		song->header[i].lpData=NULL;
		song->header[i].dwFlags=MHDR_ISSTRM|MHDR_DONE;
	}
	return((int)song);
}

// Is the song playing?
int I_QrySongPlaying(int handle)
{
	if (CurrentSong)
		return(true);
	else
		return(false);
}

void I_SetMusicVolume(int volume)
{
	snd_MusicVolume = volume;
	// Now set volume on output device.
	// Whatever( snd_MusciVolume );
	if (CurrentSong&&(volume==0)&&started)
		I_StopMusic(CurrentSong);
}

void I_ProcessMusic(void)
{
	LPMIDIHDR	header;
	int			length;
	int			i;
	MMRESULT	rc;

	if ((snd_MusicVolume==0)||!CurrentSong)
		return;

	for (i=0;i<1;i++)
	{
		header=&CurrentSong->header[i];
		if (header->dwFlags&MHDR_DONE)
		{
			if (header->lpData)
				midiOutUnprepareHeader((HMIDIOUT)hMidiStream, header, sizeof(MIDIHDR));
			header->lpData=(void *)&CurrentSong->midievents[CurrentSong->nextevent];
			length=CurrentSong->numevents-CurrentSong->nextevent;
			if (length>MAX_MIDI_EVENTS)
			{
				length=MAX_MIDI_EVENTS;
				CurrentSong->nextevent+=MAX_MIDI_EVENTS;
			}
			else
				CurrentSong->nextevent=0;
			length*=sizeof(MidiEvent_t);
			header->dwBufferLength=length;
			header->dwBytesRecorded=length;
			header->dwFlags=MHDR_ISSTRM;
			rc=midiOutPrepareHeader((HMIDIOUT)hMidiStream, header, sizeof(MIDIHDR));
			if (rc!=MMSYSERR_NOERROR)
				I_Error("midiOutPrepareHeader Failed");
			if (!started)
			{
				midiStreamRestart(hMidiStream);
				started=true;
			}
			rc=midiStreamOut(hMidiStream, header, sizeof(MIDIHDR));
			if (rc!=MMSYSERR_NOERROR)
				I_Error("midiStreamOut Failed");
		}
	}
}
