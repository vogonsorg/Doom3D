//DOOM3D
//Windows routines

#include <stdlib.h>
#include <dinput.h>

#include "doomstat.h"
#include "i_system.h"
#include "i_input.h"
#include "co_console.h"

#define SCREEN_TEXT_SIZE 65536

HWND		hMainWnd;
HINSTANCE	hAppInst;
RECT		WindowPos;
dboolean	InBackground=false;
dboolean	HideMouse=true;
char		*ScreenText=NULL;
dboolean	ShowScreenText=false;

void I_UpdateWindowRect(POINTS p)
{
	WindowPos.left=p.x;
	WindowPos.top=p.y;
	WindowPos.right=p.x+ScreenWidth;
	WindowPos.bottom=p.y+ScreenHeight;
}

static dboolean ScrollScreenText(HDC hdc)
{
	int		count;
	int		y;
	char	*s;
	SIZE	size;
	char	*first;
	char	*p;

	s=ScreenText;
	y=0;
	first=NULL;
	while (*s)
	{
		p=strchr(s, '\n');
		if (!p)
			p=s+strlen(s);
		if (!GetTextExtentExPoint(hdc, s, p-s, WindowPos.right-WindowPos.left, &count, NULL, &size))
			return(false);

		y+=size.cy;
		s=&s[count];
		if (*s=='\n')
			s++;
		if (!first)
			first=s;
	}
	if (first&&(y+size.cy>=WindowPos.bottom-WindowPos.top))
	{
		memmove(ScreenText, first, strlen(first)+1);
		return(true);
	}
	return(false);
}

static void DrawScreenText(void)
{
	PAINTSTRUCT	ps;
	HDC			hdc;
	int			count;
	char		*s;
	SIZE		size;
	int			y;
	char		*p;
	int			scrolled;
	RECT		r;

	hdc=BeginPaint(hMainWnd, &ps);
	if (!hdc)
		return;
	if (!ShowScreenText)
	{
		EndPaint(hMainWnd, &ps);
		return;
	}
	SetTextColor(hdc, PALETTERGB(255, 255, 255));
	SetBkColor(hdc, PALETTERGB(0, 0, 0));
	scrolled=false;
	while (ScrollScreenText(hdc))
		scrolled++;
	if (scrolled)
	{
		r.left=r.top=0;
		r.right=ScreenWidth;
		r.bottom=ScreenHeight;
		FillRect(hdc, &r, GetStockObject(BLACK_BRUSH));
	}
	s=ScreenText;
	y=0;
	while (*s)
	{
		p=strchr(s, '\n');
		if (!p)
			p=s+strlen(s);
		if (!GetTextExtentExPoint(hdc, s, p-s, WindowPos.right-WindowPos.left, &count, NULL, &size))
		{
			EndPaint(hMainWnd, &ps);
			return;
		}

		if (!TextOut(hdc, 0, y, s, count))
		{
			EndPaint(hMainWnd, &ps);
			return;
		}
		y+=size.cy;
		s=&s[count];
		if (*s=='\n')
			s++;
	}
	EndPaint(hMainWnd, &ps);
}

long FAR PASCAL MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_PAINT:
		DrawScreenText();
		return(0);
	case WM_MOVE:
		I_UpdateWindowRect(MAKEPOINTS(lParam));
		return (0);
    case WM_ACTIVATEAPP:
		if (wParam)
		{
			InBackground=false;
			I_ReacquireInput();
			if (InWindow&&GrabMouse)
			{
				ClipCursor(&WindowPos);
			}
		}
		else
		{
			InBackground=true;
			if (InWindow&&GrabMouse)
				ClipCursor(NULL);
		}
		break;

    case WM_CLOSE:
		DestroyWindow(hWnd);
		return(0);
	case WM_DESTROY:
		hMainWnd=NULL;
		PostQuitMessage(0);
		break;
	case WM_SETCURSOR:
		if (HideMouse)
		{
			SetCursor(NULL);
			return(0);
		}
		break;
    case WM_KEYDOWN:
    case WM_KEYUP:
        return(0);
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        if ((wParam==VK_F4)||(wParam==VK_TAB))
			break;
        return(0);
	case WM_MOUSEMOVE:
		HideMouse=true;
		break;
	case WM_NCMOUSEMOVE:
		if (InWindow&&(InBackground||!GrabMouse))
			HideMouse=false;
		break;
    }

    return(DefWindowProc(hWnd, message, wParam, lParam));
}

void I_InitScreenText(void)
{
	ScreenText=(char *)malloc(SCREEN_TEXT_SIZE);
	if (!ScreenText)
		exit(1);
	ScreenText[0]=0;
	ShowScreenText=true;
}

void I_InitWindows(void)
{
    WNDCLASS	wc;
    int		rc;

	I_InitScreenText();
    wc.style=CS_DBLCLKS;
    wc.lpfnWndProc=MainWndProc;
    wc.cbClsExtra=0;
    wc.cbWndExtra=0;
    wc.hInstance=hAppInst;
    wc.hIcon=LoadIcon(hAppInst, "DOOM3D_ICON");
    //wc.hCursor=LoadCursor(NULL, IDC_ARROW);
    wc.hCursor=NULL;
    wc.hbrBackground=GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName=NULL;
    wc.lpszClassName="Doom3DClass";
    rc=RegisterClass(&wc);
    if (!rc)
		I_Error("RegisterClass Failed");

    hMainWnd=CreateWindowEx(CS_HREDRAW|CS_VREDRAW,
		"Doom3DClass",
		"Doom3D",
		WS_BORDER|WS_CAPTION|WS_SYSMENU|WS_OVERLAPPED, 0, 0,
		320, 200,
		NULL, NULL, hAppInst, NULL);

    if (!hMainWnd)
		I_Error("CreateWindowEx Failed");
    ShowWindow(hMainWnd, SW_SHOWNORMAL);
	GetClientRect(hMainWnd, &WindowPos);
    UpdateWindow(hMainWnd);
}

void I_ShutdownWindows(void)//not really:)
{
	ClipCursor(NULL);
	if (hMainWnd)
    	DestroyWindow(hMainWnd);
}

void I_ProcessWindows(void)
{
    MSG		msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
		if (msg.message==WM_QUIT)
			I_Error("Quit");
		//TranslateMessage(&msg);
		DispatchMessage(&msg);
    }
}

void I_SetWindowSize(void)
{
	RECT	r;
	int		width;
	int		height;
	POINT	p;

	GetClientRect(hMainWnd, &r);
	width=r.right;
	height=r.bottom;
	GetWindowRect(hMainWnd, &r);
	width=ScreenWidth+(r.right-r.left)-width;
	height=ScreenHeight+(r.bottom-r.top)-height;
	MoveWindow(hMainWnd, r.left, r.top, width, height, true);
	UpdateWindow(hMainWnd);
	p.x=0;
	p.y=0;
	ClientToScreen(hMainWnd, &p);
	WindowPos.left=p.x;
	WindowPos.top=p.y;
	WindowPos.right=p.x+ScreenWidth;
	WindowPos.bottom=p.y+ScreenHeight;
	if (GrabMouse&&!InBackground)
		ClipCursor(&WindowPos);
}

/*
void Info(char *fmt, ...)
{
	char	buff[1024];
    va_list	va;

    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);
	if (MessageBox(hMainWnd, buff, "Info", MB_OKCANCEL)==IDCANCEL)
		exit(0);
}
*/

void I_CloseDubugFile(void)
{
	I_Printf("debug file closed");
	if (DebugFile)
		fclose(DebugFile);
	DebugFile=NULL;
}

void I_Printf(char *fmt, ...)
{
	char	buff[1024];
    va_list	va;
	int		len;
	char	*s;

    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);
    if (DebugFile)
	{
		fputs(buff, DebugFile);
	    fflush(DebugFile);
	}
	CO_AddText(buff);
	if (!ShowScreenText)
		return;
	len=strlen(buff);
	if (len>1024)
		I_Error("screen text buffer overflow(fatal)");
	s=ScreenText;
	while (s&&(strlen(s)+len+1>SCREEN_TEXT_SIZE))
	{
		s=strchr(ScreenText, '\n');
		if (s)
			s++;
	}

	if (s)
		memmove(ScreenText, s, strlen(s)+1);
	else
		ScreenText[0]=0;

	strcat(ScreenText, buff);

	if (!RedrawWindow(hMainWnd, NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW))//RDW_INVALIDATE|RDW_UPDATENOW))
		I_Error("RedrawWindow Failed");
}
