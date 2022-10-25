#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "doomtype.h"
#include "t_wad.h"
#include "m_swap.h"
#include "w_compress.h"

#ifdef __MSC_VER__
#define stricmp _stricmp
#endif

void CheckWrite(const void *buff, int size, FILE *fh)
{
	if (fwrite(buff, size, 1, fh)!=1)
	{
		printf("error writing output file\n");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	FILE		*fin;
	FILE		*fout;
	wadinfo_t	header;
	wadinfo_t	cheader;
	filelump_t	*lumpinfo;
	cfilelump_t	*clumpinfo;
	int			nextpos;
	byte		*buff[2];
	int			size;
	int			compsize;
	int			i;
	int			stage;
	char		fname[256];
	int			fast;

	if ((argc<3)||(argc>4))
	{
		printf("Doom WAD file compressor\n");
		printf("Usage: COMPWAD <infile> <outfile> [-fast]\n");
		printf("Note infile and outfile MUST be different\n");
		exit(1);
	}
	i=1;
	fast=false;
	if (stricmp(argv[i], "-fast")==0)
	{
		fast=true;
		i++;
	}
	strcpy(fname, argv[i]);
	if (!strchr(fname, '.'))
		strcat(fname, ".WAD");
	i++;
	fin=fopen(fname, "rb");
	if (!fin)
	{
		printf("Unable to open %s\n", fname);
		exit(1);
	}
	printf("opened %s\n", fname);
	fread(&header, sizeof(wadinfo_t), 1, fin);
	header.numlumps = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);
	printf("type:%.4s\n", header.identification);
	if ((strncmp(header.identification, "IWAD", 4)!=0)&&(strncmp(header.identification, "PWAD", 4)!=0))
	{
		printf("Must be IWAD or PWAD\n");
		fclose(fin);
		exit(1);
	}
	printf("%d lumps\n", header.numlumps);

	if ((stricmp(argv[i], "-fast")==0)&&!fast)
	{
		fast=true;
		i++;
	}
	strcpy(fname, argv[i]);
	if (!strchr(fname, '.'))
		strcat(fname, ".WAD");
	i++;
	fout=fopen(fname, "wb");
	if (!fout)
	{
		fclose(fin);
		printf("Unable to create %s\n", fname);
		exit(1);
	}
	printf("Output file:%s\n", fname);

	if ((i<argc)&&(stricmp(argv[i], "-fast")==0)&&!fast)
	{
		fast=true;
		i++;
	}

	if (i!=argc)
	{
		printf("incorrect number of arguments\n");
		exit(1);
	}

	lumpinfo=(filelump_t *)malloc(sizeof(filelump_t)*header.numlumps);
	clumpinfo=(cfilelump_t *)malloc(sizeof(cfilelump_t)*header.numlumps);
	fseek(fin, header.infotableofs, SEEK_SET);
	fread(lumpinfo, sizeof(filelump_t), header.numlumps, fin);
	nextpos=sizeof(wadinfo_t);
	fseek(fout, nextpos, SEEK_SET);

	if (fast)
		printf("Fast mode\n");
	size=0;
	for (i=0;i<header.numlumps;i++)
	{
		if (size<LONG(lumpinfo[i].size))
			size=LONG(lumpinfo[i].size);
	}
	buff[0]=(byte *)malloc((size*9)/8+1);
	buff[1]=(byte *)malloc((size*9)/8+1);

	for (i=0;i<header.numlumps;i++)
	{
		memcpy(clumpinfo[i].name, lumpinfo[i].name, 8);
		clumpinfo[i].filepos=LONG(nextpos);
		clumpinfo[i].size=lumpinfo[i].size;
		size=LONG(lumpinfo[i].size);
		printf("Lump:%8.8s, size:%8d, ", lumpinfo[i].name, size);
		if (size>0)
		{
			stage=0;
			fseek(fin, LONG(lumpinfo[i].filepos), SEEK_SET);
			if (fread(buff[stage], size, 1, fin)!=1)
			{
				printf("read failed");
				exit(1);
			}
			while (true)
			{
				compsize=W_Compress(buff[1-stage], buff[stage], size);
				if (compsize>=size)
					break;
				stage=1-stage;
				size=compsize;
				if (fast)
					break;
			}

			clumpinfo[i].compressedsize=LONG(size);
			CheckWrite(buff[stage], size, fout);
			nextpos+=size;
			printf("compressed:%8d(%02d%%)\n", size, 100-((size*100)/LONG(clumpinfo[i].size)));
		}
		else
		{
			clumpinfo[i].compressedsize=0;
			printf("\n");
		}
	}

	CheckWrite(clumpinfo, sizeof(cfilelump_t)*header.numlumps, fout);
	fseek(fout, 0, SEEK_SET);

	memcpy(cheader.identification, "CWAD", 4);
	cheader.numlumps=LONG(header.numlumps);
	cheader.infotableofs=LONG(nextpos);
	CheckWrite(&cheader, sizeof(cheader), fout);
	free(buff[0]);
	free(buff[1]);
	fclose(fout);
	fclose(fin);

	return(0);
}
