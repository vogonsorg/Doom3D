#include <string.h>

#include "doomtype.h"
#include "t_wad.h"
#include "m_swap.h"

int W_Uncompress(byte *dest, byte *src, int size)
{
	byte	window[2048];
	byte	temp[33];
	byte	*windowpos;
	byte	mask;
	byte	flags;
	byte	*end;
	byte	*pat;
	int		count;
	int		c2;
	int		outsize;

	mask=0;
	outsize=0;
	end=src+size;
	windowpos=window;
	while (src<end)
	{
		if (!mask)
		{
			mask=0x80;
			flags=*(src++);
		}
		if (mask&flags)
		{
			count=((*(word *)src)&0x001F)+1;
			pat=windowpos-(((*(word *)src)>>5)+1);
			if (pat<window)
				pat+=2048;
			if (&pat[count]>&window[2048])
				c2=&window[2048]-pat;
			else
				c2=count;

			memcpy(dest, pat, c2);
			memcpy(temp, pat, c2);
			if (c2!=count)
			{
				memcpy(dest+c2, window, count-c2);
				memcpy(temp+c2, window, count-c2);
			}
			if (&windowpos[count]>&window[2048])
				c2=&window[2048]-windowpos;
			else
				c2=count;
			memcpy(windowpos, temp, c2);
			windowpos+=count;
			if (c2!=count)
				memcpy(window, temp+c2, count-c2);
			if (windowpos>=&window[2048])
				windowpos-=2048;
			dest+=count;
			outsize+=count;
			src+=2;
		}
		else
		{
			*(dest++)=*src;
			*(windowpos++)=*src;
			src++;
			if (windowpos==&window[2048])
				windowpos=window;
			outsize++;
		}
		mask>>=1;
	}
	return(outsize);
}

static byte *FindPattern(byte *start, byte *pat, int size, int *foundsize)
{
	int		maxsize;
	byte	*s;
	byte	*maxpos;
	int		i;

	s=pat-1;
	maxsize=0;
	maxpos=NULL;
	while (s>=start)
	{
		for (i=0;(i<size)&&(&s[i]<pat);i++)
		{
			if (s[i]!=pat[i])
				break;
		}
		if (i<size)
		{
			if (i>maxsize)
			{
				maxsize=i;
				maxpos=s;
			}
		}
		s--;
	}
	if (foundsize)
		*foundsize=maxsize;
	return(maxpos);
}

int W_Compress(byte *dest, byte *src, int size)
{
	byte	*start;
	byte	*pat;
	int		patsize;
	int		destsize;
	byte	mask;
	byte	*flags;

	destsize=0;
	start=src;
	mask=0;
	while (size>0)
	{
		if (!mask)
		{
			mask=0x80;
			flags=dest++;
			*flags=0;
			destsize++;
		}
		pat=FindPattern(start, src, (size>32)?33:size, &patsize);
		if (pat&&(patsize>1))
		{
			*flags|=mask;
			*(short *)dest=((src-(pat+1))<<5)|(patsize-1);
			destsize+=2;
			dest+=2;
			size-=patsize;
			src+=patsize;
		}
		else
		{
			*(dest++)=*(src++);
			destsize++;
			size--;
		}
		mask>>=1;
		if (src-start>=2048)
			start=src-2047;
	}
	return(destsize);
}

