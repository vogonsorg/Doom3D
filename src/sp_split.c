//sp_split.c

#include "doomstat.h"
#include "sp_split.h"
#include "c_interface.h"
#include "i_system.h"
#include "hu_stuff.h"
#include "d_main.h"

extern dboolean setsizeneeded;

int		NumSplits=1;
int		ViewWidth=0;
int		ViewHeight;

void S_GetViewPos(int split, int *x, int *y)
{
	if (ViewWidth==0)
		I_Error("S_GetViewPos before S_CalcViewSize");
	switch (NumSplits)
	{
	case 1:
		*x=(ScreenWidth-ViewWidth)/2;
		if (ViewWidth==ScreenWidth)
			*y=0;
		else
			*y=(ST_Y-ViewHeight)/2;
		break;
	case 2:
		*x=(ScreenWidth-ViewWidth)/2;
		if (split&1)
			*y=ScreenHeight/2;
		else
			*y=0;
		break;
	default:
		if (split&2)
			*x=ScreenWidth/2;
		else
			*x=0;
		if (split&1)
			*y=ScreenHeight/2;
		else
			*y=0;
		break;
	}
}

static int setblocks;

void S_CalcViewSize(void)
{
	switch (NumSplits)
	{
	case 1:
		if (setblocks == 11)
		{
			ViewWidth = ScreenWidth;
			ViewHeight = ScreenHeight;
		}
		else
		{
			ViewWidth = setblocks*ScreenWidth/10;
			ViewHeight = (setblocks*ST_Y/10)&~7;
		}
		break;
    case 2:
		ViewHeight=ScreenHeight/2;
		ViewWidth=(ViewHeight*8)/5;
		if (ViewWidth>ScreenWidth)
			ViewWidth=ScreenWidth;
		break;
	default:
		ViewWidth=ScreenWidth/2;
		ViewHeight=ScreenHeight/2;
		break;
	}
	*C_API.pViewWidth=ViewWidth;
	*C_API.pViewHeight=ViewHeight;
}

void S_SetViewSize(int blocks)
{
	setblocks=blocks;
	setsizeneeded=true;
}

void S_DoSetViewSize(void)
{
	setsizeneeded=false;
	S_CalcViewSize();
	HU_ViewMoved();
	C_API.R_ViewSizeChanged();
	D_CheckWipeBusy();
}
