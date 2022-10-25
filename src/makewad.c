/*
makewad.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>

#define false 0
#define true (!false)

#define MAX_LUMPS 1024

typedef unsigned long dword;
typedef unsigned char byte;

typedef struct
{
	int		x;
	int		y;
	int		n;
	dword	*data;
}pic_t;

typedef struct
{
    char		identification[4];
    int			numlumps;
    int			infotableofs;

} wadinfo_t;

typedef struct
{
    int			filepos;
    int			size;
    char		name[8];

} filelump_t;

byte		Buff[256*320];
dword		Palette[256];
filelump_t	lumpinfo[MAX_LUMPS];
int			NumLumps;


#define COLOR_TRANSPARENT 0xffff00

void Moan(char *fmt, ...)
{
    va_list	argptr;
    char	buff[1024];

    va_start (argptr, fmt);
    vsprintf (buff, fmt, argptr);
    va_end (argptr);
	printf("Error:%s\n", buff);
    exit(1);
}

pic_t *ReadPPM(char *name)
{
	FILE	*fh;
	char	header[100];
	char	*p;
	byte	quad[4];
	pic_t	*pic;
	dword	*dest;
	int		i;

	fh=fopen(name, "rb");
	if (!fh)
		Moan("Unable to open %s", name);
	fread(header, 1, 99, fh);
	header[99]=0;
	if (strncmp("P6\n", header, 3)!=0)
		Moan("Invalid Header\n");
	p=&header[3];
	if (*p=='#')
	{
		while (*p!='\n')
			p++;
	}
	pic=(pic_t *)malloc(sizeof(pic_t));
	pic->x=strtol(p, &p, 10);
	pic->y=strtol(p, &p, 10);
	printf("%dx%d\n", pic->x, pic->y);
	pic->n=strtol(p, &p, 10);
	while (isspace(*p))
		p++;
	pic->data=(dword *)malloc(pic->x*pic->y*4);
	fseek(fh, p-header, SEEK_SET);
	dest=pic->data;
	quad[3]=0;
	for (i=0;i<pic->x*pic->y;i++)
	{
		fread(quad, 3, 1, fh);
		*(dest++)=*(dword *)quad;
	}
	return(pic);
}

void LoadPalette(void)
{
	FILE	*fh;
	byte	quad[4];
	int		i;

	fh=fopen("palette.dat", "rb");
	if (!fh)
		Moan("unable to open palette");

	quad[3]=0;
	for (i=0;i<256;i++)
	{
		fread(quad, 3, 1, fh);
		Palette[i]=*(dword *)quad;
	}
	fclose(fh);
}

void FreePic(pic_t *pic)
{
	if (!pic)
		return;
	if (pic->data)
		free(pic->data);
	free(pic);
}

void WriteLong(FILE *fh, int i)
{
	fwrite(&i, 4, 1, fh);
}

void WriteShort(FILE *fh, int i)
{
	fwrite(&i, 2, 1, fh);
}

byte GetIndex(dword rgb)
{
	int		i;
	int		j;
	int		closest;
	int		cindex;
	int		dist;
	int		n;
	byte	*p1;
	byte	*p2;

	cindex=0;
	closest=256*256*3;
	for (i=0;i<256;i++)
	{
		if (Palette[i]==rgb)
			return((byte)i);

		dist=0;
		p1=(byte *)&Palette[i];
		p2=(byte *)&rgb;
		for (j=0;j<4;j++)
		{
			if (p1[j]>p2[j])
				n=p1[j]-p2[j];
			else
				n=p2[j]-p1[j];
			dist+=n*n;
		}
		if (dist<closest)
		{
			cindex=i;
			closest=dist;
		}
	}
	return((byte)cindex);
}

int WritePic(FILE *fh, pic_t *pic, int sprite)
{
	int		len;
	int		x;
	int		y;
	dword	*in;
	byte	*out;
	byte	*post;

	out=Buff;
	WriteShort(fh, pic->x);//width
	WriteShort(fh, pic->y);//height
	if (sprite)
	{
		WriteShort(fh, pic->x/2);//xoffs
		WriteShort(fh, pic->y);//yoffs
	}
	else
	{
		WriteShort(fh, 0);//xoffs
		WriteShort(fh, 0);//yoffs
	}
	for (x=0;x<pic->x; x++)
	{
		WriteLong(fh, 8+(pic->x*4)+out-Buff);
		y=0;
		in=&pic->data[x];
		while (y<pic->y)
		{
			while ((*in==COLOR_TRANSPARENT)&&(y<pic->y))
			{
				in+=pic->x;
				y++;
			}
			if (y==pic->y)
				break;
			*(out++)=(byte)y;
			post=out++;
			*(out++)=0;
			len=0;
			while ((*in!=COLOR_TRANSPARENT)&&(y<pic->y))
			{
				*(out++)=GetIndex(*in);
				in+=pic->x;
				y++;
				len++;
			}
			out++;
			*post=(byte)len;
		}
		*(out++)=-1;
	}
	fwrite(Buff, 1, out-Buff, fh);
	return(8+(pic->x*4)+(out-Buff));
}

void WriteWADHeader(FILE *fh)
{
	wadinfo_t	info;

	fwrite(&info, sizeof(wadinfo_t), 1, fh);//gets filled in at end when know how many lumps
}

void WriteWADDirTable(FILE *fh, int size)
{
	wadinfo_t	info;
	int			pos;

	pos=ftell(fh);
	fwrite(&lumpinfo, sizeof(filelump_t), NumLumps, fh);
	fseek(fh, 0, SEEK_SET);
	memcpy(info.identification, "PWAD", 4);
	info.numlumps=NumLumps;
	info.infotableofs=pos;
	fwrite(&info, sizeof(wadinfo_t), 1, fh);
}

int CopyPic(FILE *fh, char *name, int sprite)
{
	pic_t		*pic;
	int			size;
	char		picname[13];

	sprintf(picname, "%s.ppm", name);
	pic=ReadPPM(picname);
	size=WritePic(fh, pic, sprite);
	FreePic(pic);
	return(size);
}

int CopyBlock(FILE *fh, char *name)
{
	FILE	*fin;
	int		size;
	void	*buff;

	fin=fopen(name, "rb");
	if (!fin)
		Moan("Unable to open %s", fin);
	fseek(fin, 0, SEEK_END);
	size=ftell(fh);
	buff=malloc(size);
	fread(buff, size, 1, fin);
	fclose(fin);
	fwrite(buff, size, 1, fh);
	free(buff);
	return(size);
}

int main(int argc, char *argv[])
{
	FILE		*fout;
	int			size;
	int			iswad;
	FILE		*fin;
	char		*name;
	char		line[1024];

	if (argc==1)
	{
		printf("Usage: makewad <definition file>\n");
	}
	LoadPalette();
	NumLumps=0;
	fin=fopen(argv[1], "rt");
	if (!fin)
		Moan("Unable to open %s", argv[1]);
	fgets(line, 1024, fin);
	line[strlen(line)-1]=0;
	if (line[0]=='!')
	{
		name=&line[1];
		iswad=false;
	}
	else
	{
		name=line;
		iswad=true;
	}
	fout=fopen(name, "wb");
	if (!fout)
		Moan("unable to create %s", line);
	if (iswad)
		WriteWADHeader(fout);
	memset(lumpinfo, 0, sizeof(lumpinfo));
	while (!feof(fin))
	{
		if (!fgets(line, 1024, fin))
			break;
		line[strlen(line)-1]=0;
		printf("Block:%s\n", &line[1]);
		if (!line[0])
			continue;
		lumpinfo[NumLumps].filepos=ftell(fout);
		switch (line[0])
		{
		case 'P':
			size=CopyPic(fout, &line[1], false);
			break;
		case 'S':
			size=CopyPic(fout, &line[1], true);
			break;
		case '0':
			size=0;
			break;
		case '+':
			size=CopyBlock(fout, &line[1]);
			break;
		default:
			Moan("Unknown type:#");
			break;
		}
		strncpy(lumpinfo[NumLumps].name, &line[1], 8);
		lumpinfo[NumLumps].size=size;
		NumLumps++;
		if (NumLumps==MAX_LUMPS)
		{
			printf("Lump Overflow\n");
			break;
		}
	}
	if (iswad)
		WriteWADDirTable(fout, size);
	fclose(fout);
	fclose(fin);
	return(0);
}
