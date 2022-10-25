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
//
// $Log:$
//
// DESCRIPTION:
//      Main loop menu stuff.
//      Default Config File.
//      PCX Screenshots.
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char
rcsid[] = "$Id: m_misc.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";
#endif

#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <stdlib.h>
#include <ctype.h>

#include "doomdef.h"

#include "z_zone.h"

#include "m_swap.h"
#include "m_argv.h"

#include "w_wad.h"

#include "i_reg.h"
#include "i_system.h"
#include "i_input.h"
//#include "i_music.h"
//#include "i_video.h"
//#include "v_video.h"

#include "am_map.h"

// State.
#include "doomstat.h"

// Data.
#include "dstrings.h"

#include "c_interface.h"

#include "m_misc.h"

#include "g_controls.h"
#include "g_actions.h"
#include "hu_font.h"

//
// M_DrawText
// Returns the final X coordinate
// HU_Init must have been called to init the font
//
int
M_DrawText
( int           x,
  int           y,
  char*         string )
{
	int         c;
	int         w;

	while (*string)
	{
		c = toupper(*string) - HU_FONTSTART;
		string++;
		if (c < 0 || c> HU_FONTSIZE)
		{
			x += 4;
			continue;
		}

		w = SHORT (hu_font[c]->width);
		if (x+w > ScreenWidth)
			break;
		C_API.V_DrawPatchBig(x, y, 0, hu_font[c]);
		x+=w;
	}

	return x;
}




//
// M_WriteFile
//
#ifndef O_BINARY
#define O_BINARY 0
#endif

dboolean
M_WriteFile
( char const*   name,
  void*         source,
  int           length )
{
	int         handle;
	int         count;

	handle = open ( name, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);

	if (handle == -1)
		return false;

	count = write (handle, source, length);
	close (handle);

	if (count < length)
		return false;

	return true;
}


//
// M_ReadFile
//
int
M_ReadFile
( char const*   name,
  byte**        buffer )
{
	int handle, count, length;
	struct stat fileinfo;
	byte                *buf;

	handle = open (name, O_RDONLY | O_BINARY, 0666);
	if (handle == -1)
		I_Error ("Couldn't read file %s", name);
	if (fstat (handle,&fileinfo) == -1)
		I_Error ("Couldn't read file %s", name);
	length = fileinfo.st_size;
	buf = Z_Malloc (length, PU_STATIC, NULL);
	count = read (handle, buf, length);
	close (handle);

	if (count < length)
		I_Error ("Couldn't read file %s", name);

	*buffer = buf;
	return length;
}


//
// DEFAULTS
//
extern int		DualMouse;

extern int		Autorun;

extern int      viewwidth;
extern int      viewheight;

extern int      mouseSensitivity;
extern int      showMessages;

extern int      screenblocks;

extern int      showMessages;

// machine-independent sound params
extern  int     numChannels;


extern char*    chat_macros[];


extern int MidiDevice;
extern dboolean HighSound;

default_t       GameDefaults[] =
{
	{"mouse_sensitivity",&mouseSensitivity, 5, false},
	{"sfx_volume",&snd_SfxVolume, 15, false},
	{"music_volume",&snd_MusicVolume, 8, false},
	{"show_messages",&showMessages, 1, false},


	{"use_mouse",&UseMouse[0], 1, false},
	{"use_mouse2", &UseMouse[1], 2, false},
	{"DualMouse", &DualMouse, 0, false},
	{"DigiJoy", &DigiJoy, false, false},

	{"use_joystick",&UseJoystick, 0, false},

	{"Autorun", &Autorun, 0, false},

	{"chatmacro0", (int *) &chat_macros[0], (int) HUSTR_CHATMACRO0, true},
	{"chatmacro1", (int *) &chat_macros[1], (int) HUSTR_CHATMACRO1, true},
	{"chatmacro2", (int *) &chat_macros[2], (int) HUSTR_CHATMACRO2, true},
	{"chatmacro3", (int *) &chat_macros[3], (int) HUSTR_CHATMACRO3, true},
	{"chatmacro4", (int *) &chat_macros[4], (int) HUSTR_CHATMACRO4, true},
	{"chatmacro5", (int *) &chat_macros[5], (int) HUSTR_CHATMACRO5, true},
	{"chatmacro6", (int *) &chat_macros[6], (int) HUSTR_CHATMACRO6, true},
	{"chatmacro7", (int *) &chat_macros[7], (int) HUSTR_CHATMACRO7, true},
	{"chatmacro8", (int *) &chat_macros[8], (int) HUSTR_CHATMACRO8, true},
	{"chatmacro9", (int *) &chat_macros[9], (int) HUSTR_CHATMACRO9, true},

	{"ScreenWidth", &ScreenWidth, 320, false},
	{"ScreenHeight", &ScreenHeight, 200, false},
	{"InWindow", &InWindow, false, false},
	{"screenblocks",&screenblocks, 9, false},

	{"DllName", (int *) &DllName, (int)DEF_DLL_NAME, true},
	{"GammaLevel", &GammaLevel, 0, false},
	{"AllowFloatingAutoMap", &AutoMapCanFloat, false, false},
	{"MidiDevice", &MidiDevice, 0, false},
	{"HighSound", &HighSound, false, false},
	{"FixSprites", &FixSprites, true, false},
	{"HeapSize", &AllocHeapSize, 8, false},
	{"UsePVS", &UsePVS, false, false},

	{NULL, NULL, false, false}
};

//
// M_SaveDefaults
//
void M_SaveDefaults (default_t *defaults, char *name)
{
	HKEY        regkey;
	LONG        rc;
	DWORD       disposition;
	char        buffer[1024];
	default_t	*def;
	FILE		*fh;

	strcpy(buffer, "Software\\FudgeFactor\\Doom3D");
	if (name)
		strcat(strcat((char *)buffer, "\\"), name);
	rc=RegCreateKeyEx(HKEY_CURRENT_USER, buffer, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &regkey, &disposition);
	if (rc==ERROR_SUCCESS)
	{
		for (def=defaults;def->name;def++)
		{
			if (def->isstring)
				RegWriteString(regkey, def->name, *(char **)def->location);
			else
				RegWriteInt(regkey, def->name, *def->location);
		}
		RegCloseKey(regkey);
	}

	fh=fopen(ConfigFileName, "wt");
	if (fh)
	{
		G_OutputBindings(fh);
		fclose(fh);
	}
}

LPGUID M_ParseGUID(char *buff, LPGUID guid)
{
	char	s[39/*13*/];
	int		i;
	int		j;

	if (stricmp(buff, NULL_STRING_GUID)==0)
		return(NULL);

	sscanf(buff, "{%8x-%4x-%4x-%2x%2x-%12c}", &guid->Data1, &guid->Data2, &guid->Data3, &i, &j, s);
	guid->Data4[0]=i;
	guid->Data4[1]=j;

	for (i=0;i<6;i++)
	{
		sscanf(&s[i*2], "%2x", &j);
		guid->Data4[i+2]=j;
	}
	return(guid);
}

GUID	D3DDeviceGUID;
//
// M_LoadDefaults
//
void M_LoadDefaults (default_t *defaults, char *name)
{
	HKEY        hkey;
	LONG        rc;
	char		buff[1024];
	default_t	*def;

	if (M_CheckParm("-safe"))
		hkey=NULL;
	else
	{
		strcpy(buff, "Software\\FudgeFactor\\Doom3D");
		if (name)
			strcat(strcat(buff, "\\"), name);

		rc=RegOpenKeyEx(HKEY_CURRENT_USER, buff, 0, KEY_QUERY_VALUE, &hkey);
		if (rc!=ERROR_SUCCESS)
			hkey=NULL;
	}

//FUDGE: better way of loading 3D Device GUID? - does not work if call with name!=dll name
	if (name)
	{
		if (C_API.info->caps&APICAP_D3DDEVICE)
		{
			strcpy(buff, NULL_STRING_GUID);
			RegReadString(hkey, "3DDevice", buff, 39);
			//FUDGE:assign device guid
			pD3DDeviceGUID=M_ParseGUID(buff, &D3DDeviceGUID);
		}
	}
	for (def=defaults;def->name;def++)
	{
		if (def->isstring)
		{
			strcpy(buff, (char *)def->defaultvalue);
			RegReadString(hkey, def->name, buff, 1024);
			if (strcmp(buff, (char *)def->defaultvalue)==0)
				*def->location=def->defaultvalue;
			else
				*def->location=(int)strdup(buff);
		}
		else
		{
			*def->location=RegReadInt(hkey, def->name, def->defaultvalue);
		}
	}
	if (hkey)
		RegCloseKey(hkey);
}

//
// SCREEN SHOTS
//


typedef struct
{
	char                manufacturer;
	char                version;
	char                encoding;
	char                bits_per_pixel;

	unsigned short      xmin;
	unsigned short      ymin;
	unsigned short      xmax;
	unsigned short      ymax;

	unsigned short      hres;
	unsigned short      vres;

	unsigned char       palette[48];

	char                reserved;
	char                color_planes;
	unsigned short      bytes_per_line;
	unsigned short      palette_type;

	char                filler[58];
	unsigned char       data;           // unbounded
} pcx_t;


//
// WritePCXfile
//
void
WritePCXfile
( char*         filename,
  byte*         data,
  int           width,
  int           height,
  byte*         palette )
{
	int         i;
	int         length;
	pcx_t*      pcx;
	byte*       pack;

	pcx = Z_Malloc (width*height*2+1000, PU_STATIC, NULL);

	pcx->manufacturer = 0x0a;           // PCX id
	pcx->version = 5;                   // 256 color
	pcx->encoding = 1;                  // uncompressed
	pcx->bits_per_pixel = 8;            // 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = SHORT(width-1);
	pcx->ymax = SHORT(height-1);
	pcx->hres = SHORT(width);
	pcx->vres = SHORT(height);
	memset (pcx->palette,0,sizeof(pcx->palette));
	pcx->color_planes = 1;              // chunky image
	pcx->bytes_per_line = SHORT(width);
	pcx->palette_type = SHORT(2);       // not a grey scale
	memset (pcx->filler,0,sizeof(pcx->filler));


	// pack the image
	pack = &pcx->data;

	for (i=0 ; i<width*height ; i++)
	{
		if ( (*data & 0xc0) != 0xc0)
			*pack++ = *data++;
		else
		{
			*pack++ = 0xc1;
			*pack++ = *data++;
		}
	}

	// write the palette
	*pack++ = 0x0c;     // palette ID byte
	for (i=0 ; i<768 ; i++)
		*pack++ = *palette++;

	// write output file
	length = pack - (byte *)pcx;
	M_WriteFile (filename, pcx, length);

	Z_Free (pcx);
}


//
// M_ScreenShot
//
void M_ScreenShot (void)
{
	C_API.I_ScreenShot();
}

