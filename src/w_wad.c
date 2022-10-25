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
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

#ifdef RCSID
static const char
rcsid[] = "$Id: w_wad.c,v 1.5 1997/02/03 16:47:57 b1 Exp $";
#endif

#include <ctype.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>
#include <malloc.h>

#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "w_compress.h"
#include "z_zone.h"

#include "c_interface.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"



void I_Printf(char *fmt, ...);

dboolean	FixSprites;

//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t*		lumpinfo;
int			numlumps;


#define MAX_MEMLUMPS	16

void**			lumpcache=NULL;

void
ExtractFileBase
( char*		path,
 char*		dest )
{
    char*	src;
    int		length;

    src = path + strlen(path) - 1;

    // back up until a \ or the start
    while (src != path
		&& *(src-1) != '\\'
		&& *(src-1) != '/')
    {
		src--;
    }

    // copy up to eight characters
    memset (dest,0,8);
    length = 0;

    while (*src && *src != '.')
    {
		if (++length == 9)
			I_Error ("Filename base of %s >8 chars",path);

		*dest++ = toupper((int)*src++);
    }
}





//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//

int			reloadlump;
char*			reloadname=NULL;

void W_LoadCWadLumps(wadinfo_t *header, int handle)
{
    cfilelump_t	*fileinfo;
    lumpinfo_t	*lump_p;
    int			i;
	int			startlump;
	int			length;

    startlump = numlumps;

	header->numlumps = LONG(header->numlumps);
	header->infotableofs = LONG(header->infotableofs);
	length = header->numlumps*sizeof(cfilelump_t);
	fileinfo = alloca (length);
	lseek (handle, header->infotableofs, SEEK_SET);
	read (handle, fileinfo, length);
	numlumps += header->numlumps;


    // Fill in lumpinfo
    *C_API.plumpinfo=lumpinfo = realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));

    if (!lumpinfo)
		I_Error ("Couldn't realloc lumpinfo");

    lump_p = &lumpinfo[startlump];

    for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
    {
		lump_p->handle = handle;
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
		lump_p->compressedsize=LONG(fileinfo->compressedsize);
		if (lump_p->compressedsize==0)//catch old-style cwads
			lump_p->compressedsize=lump_p->size;
		strncpy (lump_p->name, fileinfo->name, 8);
    }
}

dboolean W_AddFile (char *filename)
{
    wadinfo_t		header;
    lumpinfo_t*		lump_p;
    int				i;
    int				handle;
    int				length;
    int				startlump;
    filelump_t*		fileinfo;
    filelump_t		singleinfo;
    int				storehandle;

    // open the file and add to directory
    if ( (handle = open (filename,O_RDONLY | O_BINARY)) == -1)
    {
		I_Printf (" couldn't open '%s'\n",filename);
		return(false);
    }

    I_Printf (" adding %s\n",filename);
    startlump = numlumps;

    if (strcmpi (filename+strlen(filename)-3 , "wad" )&&strcmpi (filename+strlen(filename)-3 , "gwa" ) )
    {
		// single lump file
		fileinfo = &singleinfo;
		singleinfo.filepos = 0;
		singleinfo.size = LONG(filelength(handle));
		ExtractFileBase (filename, singleinfo.name);
		numlumps++;
    }
    else
    {
		// WAD file
		read (handle, &header, sizeof(header));
		if (strncmp(header.identification,"IWAD",4))
		{
			// Homebrew levels?
			if (strncmp(header.identification,"PWAD",4))
			{
				if (strncmp(header.identification, "CWAD", 4)==0)
				{
					W_LoadCWadLumps(&header, handle);
					return(true);
				}
				else
				I_Error ("Wad file %s doesn't have IWAD "
					"or PWAD id\n", filename);
			}

			// ???modifiedgame = true;
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);
		length = header.numlumps*sizeof(filelump_t);
		fileinfo = alloca (length);
		lseek (handle, header.infotableofs, SEEK_SET);
		read (handle, fileinfo, length);
		numlumps += header.numlumps;
    }


    // Fill in lumpinfo
    *C_API.plumpinfo=lumpinfo = realloc (lumpinfo, numlumps*sizeof(lumpinfo_t));

    if (!lumpinfo)
		I_Error ("Couldn't realloc lumpinfo");

    lump_p = &lumpinfo[startlump];

    storehandle = reloadname ? -1 : handle;

    for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
    {
		lump_p->handle = storehandle;
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
		lump_p->compressedsize=LONG(fileinfo->size);
		strncpy (lump_p->name, fileinfo->name, 8);
    }

    if (reloadname)
		close (handle);
	return(true);
}




//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
//broken for compressed wads?
void W_Reload (void)
{
    wadinfo_t		header;
    int			lumpcount;
    lumpinfo_t*		lump_p;
    int			i;
    int			handle;
    int			length;
    filelump_t*		fileinfo;

    if (!reloadname)
		return;

	I_Error("Development reloading is broken");
    if ( (handle = open (reloadname,O_RDONLY | O_BINARY)) == -1)
		I_Error ("W_Reload: couldn't open %s",reloadname);

    read (handle, &header, sizeof(header));
    lumpcount = LONG(header.numlumps);
    header.infotableofs = LONG(header.infotableofs);
    length = lumpcount*sizeof(filelump_t);
    fileinfo = alloca (length);
    lseek (handle, header.infotableofs, SEEK_SET);
    read (handle, fileinfo, length);

    // Fill in lumpinfo
    lump_p = &lumpinfo[reloadlump];

    for (i=reloadlump ;
		i<reloadlump+lumpcount ;
		i++,lump_p++, fileinfo++)
    {
		if (lumpcache[i])
			Z_Free (lumpcache[i]);

		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
    }

    close (handle);
}

//fast, but assumes padded to 8 chars:(
#define SameName(n1, n2) ((*(int *)(n1)==*(int *)(n2))&&(*(int *)&(n1)[4]==*(int *)&(n2)[4]))

#define CopyLumps(dest, src, count) memcpy(dest, src, (count)*sizeof(lumpinfo_t))
#define CopyLump(dest, src) CopyLumps(dest, src, 1)

static dboolean IsDummyTag(char *name, char c)
{
	char	s[9];

	s[0]=c;
	s[2]='_';s[3]='E';s[4]='N';s[5]='D';s[6]=0;s[7]=0;
	s[1]='1';
	if (SameName(s, name))
		return(true);
	s[1]='2';
	if (SameName(s, name))
		return(true);
	s[2]='_';s[3]='S';s[4]='T';s[5]='A';s[6]='R';s[7]='T';
	s[1]='1';
	if (SameName(s, name))
		return(true);
	s[1]='2';
	if (SameName(s, name))
		return(true);
	return(false);
}

static dboolean IsTag(char *name, char c, char *tag)
{
	char	s[9];

	memcpy(&s[2], tag, 7);
	s[0]=s[1]=c;
	if (SameName(name, s))
		return(true);
	if (SameName(name, &s[1]))
		return(true);
	return(false);
}

lumpinfo_t *W_MergeLumpGroup(lumpinfo_t *start, char c)
{
	lumpinfo_t	*src;
	lumpinfo_t	*dest;
	lumpinfo_t	*endlump;
	dboolean	copy;
	int			gotstart;

	copy=false;
	gotstart=0;
	src=lumpinfo;
	endlump=NULL;
	dest=start;
	while (src<lumpinfo+numlumps)
	{
		if (IsTag(src->name, c, "_START"))
		{
			if (copy)
				I_Printf("W_FixSprites:multiple %c_START\n", c);
			else if (gotstart)
			{
				if (gotstart==1)
					I_Printf("Merging %c_START Lumps\n", c);
				gotstart++;
			}
			else
			{
				CopyLump(dest, src);
				memcpy(&dest->name[1], "_START", 8);
				dest->name[0]=c;
				dest++;
				gotstart++;
			}
			copy=true;
			src->name[0]=0;
		}
		else if (IsTag(src->name, c, "_END\0\0"))
		{
			copy=false;
			if (!endlump)
				endlump=src;
			src->name[0]=0;
		}
		else if (IsDummyTag(src->name, c))
		{
			src->name[0]=0;
		}
		else if (copy)
		{
			CopyLump(dest++, src);
			src->name[0]=0;
		}
		src++;
	}
	if (endlump)
	{
		CopyLump(dest, endlump);
		memcpy(&dest->name[1], "_END\0\0", 8);
		dest->name[0]=c;
		dest++;
	}
	else
		I_Printf("W_FixSprites:%c_END not found\n", c);
	return(dest);
}

// W_FixSprites
// Concatenate all sprite lumps into one.
// Allows sprites-in-a-PWAD
//
void W_FixSprites(void)
{
	lumpinfo_t	*lumps;
	lumpinfo_t	*dest;
	lumpinfo_t	*src;

	lumps=(lumpinfo_t *)Z_Malloc(numlumps*sizeof(lumpinfo_t), PU_STATIC, NULL);
	dest=W_MergeLumpGroup(lumps, 'S');
	dest=W_MergeLumpGroup(dest, 'F');
	dest=W_MergeLumpGroup(dest, 'P');

	src=lumpinfo;
	while (src<lumpinfo+numlumps)
	{
		if (src->name[0])
		{
			CopyLump(dest++, src);
		}
		src++;
	}
	numlumps=dest-lumps;
	CopyLumps(lumpinfo, lumps, numlumps);
	Z_Free(lumps);
}

//
// W_AddMemLumps
// adds buildin lumps
//
BOOL CALLBACK EnumResourceProc(HANDLE hModule, LPCTSTR type, LPTSTR name, LONG param)
{
	HRSRC	hrsrc;
	HGLOBAL	hdata;

	hrsrc=FindResource(NULL, name, "DOOMLUMP");
	if (!hrsrc)
	{
		I_Error("W_Init: Error finding memory lump resource %s", name);
		return(TRUE);
	}
	hdata=LoadResource(NULL, hrsrc);
	if (!hdata)
		I_Error("W_Init: Error loading memory lump resource %s", name);
	lumpinfo[numlumps].position=(int)LockResource(hdata);
	if (!lumpinfo[numlumps].position)
		I_Error("W_Init: Error locking memory lump resource %s", name);
	memset(lumpinfo[numlumps].name, ' ', 8);
	strncpy(lumpinfo[numlumps].name, name, 8);
	lumpinfo[numlumps].size=SizeofResource(NULL, hrsrc);
	lumpinfo[numlumps].handle=-2;
	numlumps++;
	return(TRUE);
}

void W_AddMemLumps(void)
{
	numlumps=0;
	*C_API.plumpinfo=lumpinfo = malloc (MAX_MEMLUMPS*sizeof(lumpinfo_t));
    ZeroMemory(lumpinfo, MAX_MEMLUMPS*sizeof(lumpinfo_t));
	EnumResourceNames(NULL, "DOOMLUMP", EnumResourceProc, 0);
}


//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles (char** filenames)
{
    int		size;
	char	name[1024];
	char	*ext;

	W_AddMemLumps();

    for ( ; *filenames ; filenames++)
    {
		strcpy(name, *filenames);
		if (strlen(name)<=4)
			ext=NULL;
		else
		{
			ext=&name[strlen(name)-4];
			if (stricmp(ext, ".wad")!=0)
				ext=NULL;
		}
		if (!(W_AddFile (name)||ext))
		{
			ext=&name[strlen(name)];
			strcpy(ext, ".wad");
			W_AddFile(name);
		}
		if (!(AllowGLNodes&&ext))
			continue;
		strcpy(ext, ".gwa");
		W_AddFile(name);
	}

	if (FixSprites)
		W_FixSprites();
    if (!numlumps)
		I_Error ("W_InitFiles: no files found");

    // set up caching
    size = numlumps * sizeof(*lumpcache);

    lumpcache = malloc (size);
    if (!lumpcache)
		I_Error ("Couldn't allocate lumpcache");

    memset (lumpcache,0, size);
}




//
// W_InitFile
// Just initialize from a single file.
//
void W_InitFile (char* filename)
{
    char*	names[2];

    names[0] = filename;
    names[1] = NULL;
    W_InitMultipleFiles (names);
}



//
// W_NumLumps
//
int W_NumLumps (void)
{
    return numlumps;
}



//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName (char* name)
{
    union {
		char	s[9];
		int	x[2];

    } name8;

    int		v1;
    int		v2;
    lumpinfo_t*	lump_p;

    // make the name into two integers for easy compares
    strncpy (name8.s,name,8);

    // in case the name was a full 8 chars
    name8.s[8] = 0;

    // case insensitive
    strupr (name8.s);

    v1 = name8.x[0];
    v2 = name8.x[1];


    // scan backwards so patch lump files take precedence
    lump_p = lumpinfo + numlumps;

    while (lump_p-- != lumpinfo)
    {
		if ( *(int *)lump_p->name == v1
			&& *(int *)&lump_p->name[4] == v2)
		{
			return lump_p - lumpinfo;
		}
    }

    // TFB. Not found.
    return -1;
}




//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName (char* name)
{
    int	i;

    i = W_CheckNumForName (name);

    if (i == -1)
		I_Error ("W_GetNumForName: %s not found!", name);

    return i;
}


//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength (int lump)
{
    if (lump >= numlumps)
		I_Error ("W_LumpLength: %i >= numlumps",lump);

    return lumpinfo[lump].size;
}

void W_ReadMemLump(lumpinfo_t *l, void *dest)
{
	void	*data;

	data=(void *)l->position;
	memcpy(dest, data, l->size);
}

//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void
W_ReadLump
( int		lump,
 void*		dest )
{
    int			c;
    lumpinfo_t*	l;
    int			handle;
	byte		*buff[2];
	int			size;
	int			stage;

    if (lump >= numlumps)
		I_Error ("W_ReadLump: %i >= numlumps",lump);

    l = lumpinfo+lump;

	if (l->handle == -2)
	{
		W_ReadMemLump(l, dest);
		return;
	}

    I_BeginRead ();

    if (l->handle == -1)
    {
		// reloadable file, so use open / read / close
		if ( (handle = open (reloadname,O_RDONLY | O_BINARY)) == -1)
			I_Error ("W_ReadLump: couldn't open %s",reloadname);
    }
    else
		handle = l->handle;

    lseek (handle, l->position, SEEK_SET);

	stage=0;
	size=l->compressedsize;
	buff[0]=dest;
	if (size!=l->size)
	{
		buff[1]=Z_Malloc(l->size, PU_STATIC, NULL);
		if (!buff)
			I_Error("out of memory");
	}
	else
		buff[1]=NULL;

	c=read(handle, buff[0], size);
	if (c!=size)
		I_Error ("W_ReadLump: only read %i of %i on lump %i", c, size, lump);
	while (size<l->size)
	{
		size=W_Uncompress(buff[1-stage], buff[stage], size);
		stage=1-stage;
	}
	if (size!=l->size)
		I_Error("W_Readlump:size mismatch");

	if (stage!=0)
		memcpy(buff[0], buff[1], size);

	if (buff[1])
		Z_Free(buff[1]);

/*
		c = read (handle, dest, l->size);

	    if (c < l->size)
			I_Error ("W_ReadLump: only read %i of %i on lump %i", c,l->size,lump);
*/

    if (l->handle == -1)
		close (handle);

    // ??? I_EndRead ();
}




//
// W_CacheLumpNum
//
void*
W_CacheLumpNum
( int		lump,
 int		tag )
{
    if (lump >= numlumps)
		I_Error ("W_CacheLumpNum: %i >= numlumps",lump);

    if (!lumpcache[lump])
    {
		// read the lump in
		//I_Printf ("cache miss on lump %i\n",lump);
		Z_Malloc (W_LumpLength (lump), tag, &lumpcache[lump]);
		W_ReadLump (lump, lumpcache[lump]);
    }
    else
    {
		//I_Printf ("cache hit on lump %i\n",lump);
		Z_ChangeTag (lumpcache[lump],tag);
    }

    return lumpcache[lump];
}



//
// W_CacheLumpName
//
void*
W_CacheLumpName
( char*		name,
 int		tag )
{
    return W_CacheLumpNum (W_GetNumForName(name), tag);
}


//
// W_Profile
//
int		info[2500][10];
int		profilecount;

void W_Profile (void)
{
    int		i;
    memblock_t*	block;
    void*	ptr;
    char	ch;
    FILE*	f;
    int		j;
    char	name[9];


    for (i=0 ; i<numlumps ; i++)
    {
		ptr = lumpcache[i];
		if (!ptr)
		{
			continue;
		}
		else
		{
			block = (memblock_t *) ( (byte *)ptr - sizeof(memblock_t));
			if (block->tag < PU_PURGELEVEL)
				ch = 'S';
			else
				ch = 'P';
		}
		info[i][profilecount] = ch;
    }
    profilecount++;

    f = fopen ("waddump.txt","w");
    name[8] = 0;

    for (i=0 ; i<numlumps ; i++)
    {
		memcpy (name,lumpinfo[i].name,8);

		for (j=0 ; j<8 ; j++)
			if (!name[j])
				break;

			for ( ; j<8 ; j++)
				name[j] = ' ';

			fprintf (f,"%s ",name);

			for (j=0 ; j<profilecount ; j++)
				fprintf (f,"    %c",info[i][j]);

			fprintf (f,"\n");
    }
    fclose (f);
}


