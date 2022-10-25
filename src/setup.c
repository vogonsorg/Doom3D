#include <windows.h>
#include <d3d.h>
#include <rpcdce.h>
#include <commctrl.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include "setup.h"
#include "i_reg.h"
#include "apiinfo.h"
#include "doomdef.h"
#include "t_split.h"

#define IDC_3DDEVICE		900
#define IDC_DYNAMIC			1000

typedef struct
{
	char		*name;
	HMODULE		hmod;
	DoomAPI_t	DoomAPI;
	apiinfo_t	api;
	HGLOBAL		hTemplate;
	char		guid[39];
}moduleinfo_t;

typedef struct
{
	char	*name;
	int		def;
}keyaction_t;

HINSTANCE	hAppInst=NULL;

char		CommandLine[1024]="";

POINT		Resolutions[]=
{
	{320, 200},
	{320, 240},
	{512, 384},
	{640, 480},
	{800, 600},
	{1024, 768},
	{1152, 864},
	{1280, 1024},
	{1600, 1200},
	{0, 0}
};

int		UseJoystick;
int		UseMouse;
int		MouseCom;
int		UseMouse2;
int		DigiJoy;
int		SplitPlayers=1;

void Moan(char *fmt, ...);

void LoadResolution(HWND hCombo, HKEY hkey)
{
	int		width;
	int		height;
	POINT	*p;
	char	buff[12];

	width=RegReadInt(hkey, "ScreenWidth", 320);
	height=RegReadInt(hkey, "ScreenHeight", 200);
	for (p=Resolutions;p->x;p++)
	{
		if ((p->x==width)&&(p->y==height))
		{
			SendMessage(hCombo, CB_SETCURSEL, p-Resolutions, 0);
			return;
		}
	}
	sprintf(buff, "%dx%d", width, height);
	SetWindowText(hCombo, buff);
}

void SelectDll(HWND hCombo, HKEY hkey)
{
	char			name[256];
	moduleinfo_t	*info;
	int				count;
	int				i;

	name[0]=0;
	RegReadString(hkey, "DllName", name, 256);
	if (strlen(name)<=4)
	{
		SendMessage(hCombo, CB_SETCURSEL, 0, 0);
		return;
	}
	name[strlen(name)-4]=0;
	count=SendMessage(hCombo, CB_GETCOUNT, 0, 0);
	for (i=0;i<count;i++)
	{
		info=(moduleinfo_t *)SendMessage(hCombo, CB_GETITEMDATA, i, 0);
		if (stricmp(info->name, name)==0)
		{
			SendMessage(hCombo, CB_SETCURSEL, i, 0);
			return;
		}
	}
	SendMessage(hCombo, CB_SETCURSEL, 0, 0);
}

void LoadControls(HKEY hkey)
{
	UseMouse=RegReadInt(hkey, "use_mouse", 1);
	UseMouse2=RegReadInt(hkey, "use_mouse2", 2);
	MouseCom=RegReadInt(hkey, "DualMouse", 0);
	UseJoystick=RegReadInt(hkey, "use_joystick", 0);
	DigiJoy=RegReadInt(hkey, "DigiJoy", false);
}

void LoadSettings(HWND hWnd)
{
	HKEY	hkey;
	LONG	rc;

	rc=RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\FudgeFactor\\Doom3D", 0, KEY_QUERY_VALUE, &hkey);
	if (rc!=ERROR_SUCCESS)
		hkey=NULL;

	rc=RegReadInt(hkey, "InWindow", false);
	CheckDlgButton(hWnd, IDC_FULLSCREEN, rc?BST_UNCHECKED:BST_CHECKED);

	rc=RegReadInt(hkey, "FixSprites", true);
	CheckDlgButton(hWnd, IDC_FIXSPRITES, rc?BST_CHECKED:BST_UNCHECKED);

	rc=RegReadInt(hkey, "HighSound", false);
	CheckDlgButton(hWnd, IDC_HISOUND, rc?BST_CHECKED:BST_UNCHECKED);

	rc=RegReadInt(hkey, "HeapSize", 8);
	SetDlgItemInt(hWnd, IDC_HEAPSIZE, rc, false);

	LoadControls(hkey);

	LoadResolution(GetDlgItem(hWnd, IDC_RESOLUTION), hkey);

	SelectDll(GetDlgItem(hWnd, IDC_DLLNAME), hkey);

	if (hkey)
		RegCloseKey(hkey);
}

void CleanupModule(moduleinfo_t *info)
{
	if (!info)
		return;
	if (info->name)
		free(info->name);
	if (info->api.FreeDoomAPI)
		info->api.FreeDoomAPI();
	if (info->hmod)
		FreeLibrary(info->hmod);
	if (info->hTemplate)
		GlobalFree(info->hTemplate);
	free(info);
}

void AddDll(HWND hCombo, char *name)
{
	moduleinfo_t	*info;
	int				i;

	info=(moduleinfo_t *)malloc(sizeof(moduleinfo_t));
	ZeroMemory(info, sizeof(moduleinfo_t));
	i=strlen(name);
	info->name=(char *)malloc(i-3);
	strncpy(info->name, name, i-4);
	info->name[i-4]=0;
	info->hmod=LoadLibrary(name);
	if (!info->hmod)
	{
		CleanupModule(info);
		return;
	}
	info->DoomAPI=(DoomAPI_t)GetProcAddress(info->hmod, "DoomAPI");
	if (!info->DoomAPI)
	{
		CleanupModule(info);
		return;
	}
	info->api.size=sizeof(apiinfo_t);
	info->api.version=API_INFO_VERSION;
	if (info->DoomAPI(&info->api, NULL)!=API_OK)
	{
		CleanupModule(info);
		return;
	}
	i=SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)info->name);
	SendMessage(hCombo, CB_SETITEMDATA, i, (LPARAM)info);
}

void LoadDlls(HWND hCombo)
{
	WIN32_FIND_DATA	ffd;
	HANDLE			hfind;
	BOOL			more;

	hfind=FindFirstFile("*.dll", &ffd);
	if (hfind==INVALID_HANDLE_VALUE)
		return;

	more=true;
	while (more)
	{
		AddDll(hCombo, ffd.cFileName);
		if (!FindNextFile(hfind, &ffd))
			more=false;
	}
	FindClose(hfind);
}

HHOOK	hTTHook=NULL;
HWND	hToolTip=NULL;

LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	LPMSG	pmsg;
	MSG		msg;

	pmsg=(LPMSG)lParam;
	if (nCode<0)
		return(CallNextHookEx(hTTHook, nCode, wParam, lParam));
	switch (pmsg->message)
	{
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		msg.lParam=pmsg->lParam;
		msg.wParam=pmsg->wParam;
		msg.message=pmsg->message;
		msg.hwnd=pmsg->hwnd;
		SendMessage(hToolTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
		break;
	}
	return(CallNextHookEx(hTTHook, nCode, wParam, lParam));
}

void AddToolTip(HWND hWnd, int ID, char *tip)
{
	TOOLINFO	ti;

	ti.cbSize=sizeof(TOOLINFO);
	ti.uFlags=TTF_IDISHWND|TTF_SUBCLASS;
	ti.hwnd=hWnd;
	ti.uId=(UINT)GetDlgItem(hWnd, ID);
	ti.hinst=NULL;
	ti.lpszText=tip;
	SendMessage(hToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
}

void InitToolTip(HWND hWnd)
{
	hToolTip=CreateWindowEx(0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hWnd, NULL, hAppInst, NULL);
	if (!hToolTip)
		return;

	AddToolTip(hWnd, IDC_RESOLUTION, "Screen Resolution or Window size");
	AddToolTip(hWnd, IDC_STATICN(IDC_RESOLUTION), "Screen Resolution or Window size");
	AddToolTip(hWnd, IDC_FULLSCREEN, "Fullscreen or Windowed");
	AddToolTip(hWnd, IDC_FIXSPRITES, "Sprites-in-a-PWAD fix");
	AddToolTip(hWnd, IDC_HISOUND, "16 or 8 bit sound");
	AddToolTip(hWnd, IDC_DLLNAME, "Rendering DLL");
	AddToolTip(hWnd, IDC_STATICN(IDC_DLLNAME), "Rendering DLL");
	AddToolTip(hWnd, IDC_ADVANCED, "Advanced rendering options");
	AddToolTip(hWnd, IDC_NETGAME, "Set network game options");
	AddToolTip(hWnd, IDC_PARAMETERS, "Additional command line parameters");
	AddToolTip(hWnd, IDC_STATICN(IDC_PARAMETERS), "Additional command line parameters");
	AddToolTip(hWnd, IDOK, "Save settings and run Doom3D");
	AddToolTip(hWnd, IDC_SAVE, "Save settings and exit");
	AddToolTip(hWnd, IDCANCEL, "Exit without saving settings");

	//hTTHook=SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)GetMsgProc, NULL, GetCurrentThreadId());
}

void InitMainDlg(HWND hWnd)
{
	HWND	hctrl;
	char	buff[10];
	POINT	*p;

	hctrl=GetDlgItem(hWnd, IDC_RESOLUTION);
	for (p=Resolutions;p->x;p++)
	{
		sprintf(buff, "%dx%d", p->x, p->y);
		SendMessage(hctrl, CB_ADDSTRING, 0, (LPARAM)buff);
	}
	SendMessage(hctrl, CB_SETCURSEL, 0, 0);
	LoadDlls(GetDlgItem(hWnd, IDC_DLLNAME));
	LoadSettings(hWnd);
	InitToolTip(hWnd);
}

void CleanupMainDlg(HWND hWnd)
{
	HWND			hCombo;
	int				i;
	int				count;
	moduleinfo_t	*info;

	hCombo=GetDlgItem(hWnd, IDC_DLLNAME);
	count=SendMessage(hCombo, CB_GETCOUNT, 0, 0);
	for (i=0;i<count;i++)
	{
		info=(moduleinfo_t *)SendMessage(hCombo, CB_GETITEMDATA, i, 0);
		if (info)
		{
			CleanupModule(info);
			SendMessage(hCombo, CB_SETITEMDATA, i, (LPARAM)NULL);
		}
	}
}

void SaveResolution(HWND hCombo, HKEY hkey)
{
	int		i;
	int		width;
	int		height;
	char	buff[12];
	char	*s;

	i=SendMessage(hCombo, CB_GETCURSEL, 0, 0);
	if (i==CB_ERR)
	{
		GetWindowText(hCombo, buff, 12);
		width=strtol(buff, &s, 10);
		while (*s&&!isdigit(*s))
			s++;
		height=strtol(s, NULL, 10);
		if (!width&&height)
			return;
	}
	else
	{
		width=Resolutions[i].x;
		height=Resolutions[i].y;
	}
	RegWriteInt(hkey, "ScreenWidth", width);
	RegWriteInt(hkey, "ScreenHeight", height);
}

void SaveDllName(HWND hCombo, HKEY hkey)
{
	int				i;
	char			buff[256];
	moduleinfo_t	*info;

	i=SendMessage(hCombo, CB_GETCURSEL, 0, 0);
	if (i==CB_ERR)
		return;
	info=(moduleinfo_t *)SendMessage(hCombo, CB_GETITEMDATA, i, 0);
	strcpy(buff, info->name);
	strcat(buff, ".dll");
	RegWriteString(hkey, "DllName", buff);
}

void SaveControls(HKEY hkey)
{
	RegWriteInt(hkey, "use_mouse", UseMouse);
	RegWriteInt(hkey, "use_mouse2", UseMouse2);
	RegWriteInt(hkey, "DualMouse", MouseCom);
	RegWriteInt(hkey, "use_joystick", UseJoystick);
	RegWriteInt(hkey, "DigiJoy", DigiJoy);
}

void SaveSettings(HWND hWnd)
{
	LONG	rc;
	DWORD	disp;
	HKEY	hkey;

	rc=RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\FudgeFactor\\Doom3D", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &disp);
	if (rc!=ERROR_SUCCESS)
		return;

	SaveResolution(GetDlgItem(hWnd, IDC_RESOLUTION), hkey);

	rc=IsDlgButtonChecked(hWnd, IDC_FULLSCREEN);
	RegWriteInt(hkey, "InWindow", rc!=BST_CHECKED);
	rc=IsDlgButtonChecked(hWnd, IDC_FIXSPRITES);
	RegWriteInt(hkey, "FixSprites", rc==BST_CHECKED);
	rc=IsDlgButtonChecked(hWnd, IDC_HISOUND);
	RegWriteInt(hkey, "HighSound", rc==BST_CHECKED);
	rc=GetDlgItemInt(hWnd, IDC_HEAPSIZE, NULL, false);
	RegWriteInt(hkey, "HeapSize", rc);

	SaveDllName(GetDlgItem(hWnd, IDC_DLLNAME), hkey);

	SaveControls(hkey);

	RegCloseKey(hkey);
}

LPWORD AlignPtr(LPWORD p)
{
	return((LPWORD)((((DWORD)p)+3)&~3));
}

LPWORD WriteWCString(LPWORD p, char *s)
{
//FUDGE: remove 50 char limit?
	p+=MultiByteToWideChar(CP_ACP, 0, s, -1, (LPWSTR)p, 50);
	return(p);
}

LPWORD WriteDlgTemplate(LPDLGTEMPLATE pdt, char *name, int width)
{
	LPWORD				ptr;

	ptr=(LPWORD)(pdt+1);
	pdt->style=DS_MODALFRAME|DS_SETFONT|WS_VISIBLE|WS_POPUP|WS_CAPTION;
	pdt->x=0;
	pdt->y=0;
	pdt->cx=width;
	*(ptr++)=0;
	*(ptr++)=0;
	ptr=WriteWCString(ptr, name);
	*(ptr++)=8;
	ptr=WriteWCString(ptr, "MS Shell Dlg");
	ptr=AlignPtr(ptr);
	return(ptr);
}

LPWORD WriteDlgItem(LPWORD ptr, char *name, int x, int y, int width, int height, DWORD style, int id, int type)
{
	LPDLGITEMTEMPLATE	pdit;

	pdit=(LPDLGITEMTEMPLATE)ptr;
	ptr=(LPWORD)(pdit+1);

	pdit->style=style;
	pdit->x=x;
	pdit->y=y;
	pdit->cx=width;
	pdit->cy=height;
	pdit->id=id;
	*(ptr++)=0xFFFF;
	*(ptr++)=type;
	if (name)
		ptr=WriteWCString(ptr, name);
	else
		*(ptr++)=0;
	*(ptr++)=0;
	ptr=AlignPtr(ptr);
	return(ptr);
}

HGLOBAL MakeAdvancedDlgTemplate(moduleinfo_t *info)
{
	LPDLGTEMPLATE		pdt;
	LPWORD				ptr;
	HGLOBAL				hmem;
	int					n;
	int					y;
	default_t			*def;
	DWORD				style;
	default_t			*defaults;

	defaults=info->api.info->defaults;
//FUDGE:work out required size? - Allready had one version crash cos of this
	hmem=GlobalAlloc(GMEM_ZEROINIT, 2048);
	if (!hmem)
		Moan("Unable to allocate memory");

	pdt=(LPDLGTEMPLATE)GlobalLock(hmem);
	ptr=WriteDlgTemplate(pdt, "Advanced Settings", 146);

	y=4;
	n=0;
	for (def=defaults;def->name;def++)
	{
		ptr=WriteDlgItem(ptr, def->name, 4, y+2, 88, 8, WS_CHILD|WS_VISIBLE, IDC_STATIC, 0x82);

		style=WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL;
		if (n==0)
			style|=WS_GROUP;
		if (!def->isstring)
			style|=ES_NUMBER;
		ptr=WriteDlgItem(ptr, NULL, 92, y, 50, 12, style, IDC_DYNAMIC+n, 0x81);

		y+=16;
		n++;
	}

	ptr=WriteDlgItem(ptr, "3D Display Device", 4, y+2, 88, 8, WS_CHILD|WS_VISIBLE, IDC_STATIC, 0x82);
	ptr=WriteDlgItem(ptr, "&Change...", 92, y, 50, 14, WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_GROUP|BS_PUSHBUTTON, IDC_3DDEVICE, 0x80);
	y+=18;
	n++;
	ptr=WriteDlgItem(ptr, "OK", 12, y, 50, 14, WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_GROUP|BS_DEFPUSHBUTTON, IDOK, 0x80);
	/*ptr=*/WriteDlgItem(ptr, "Cancel", 66, y, 50, 14, WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON, IDCANCEL, 0x80);

	if ((char *)pdt-(char *)ptr>2048)
		Moan("Internal error: Dialog Buffer Overrun\nPlease email pbrook@bigfoot.com");
	pdt->cdit=n*2+2;
	pdt->cy=y+18;

	GlobalUnlock(hmem);
	return(hmem);
}

HRESULT WINAPI DeviceEnumProc(LPGUID pGUID, char *desc, char *name, HWND hList, HMONITOR monitor)
{
	char	buff[256];
	int		i;

	sprintf(buff, "%s(%s)", name, desc);
	i=SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)buff);
	SendMessage(hList, LB_SETITEMDATA, i, (LPARAM)pGUID);
	return(1);
}

void InitSelectDeviceDlg(HWND hWnd)
{
	HRESULT		hres;
	HWND		hList;

	hList=GetDlgItem(hWnd, IDC_DEVICELIST);
	hres=DirectDrawEnumerateEx((LPDDENUMCALLBACKEX)DeviceEnumProc, hList, DDENUM_NONDISPLAYDEVICES);
	if (hres!=DD_OK)
	{
		MessageBox(NULL, "3D Device Enumeration Failed", "Error", MB_OK|MB_ICONSTOP);
		EndDialog(hWnd, false);
		return;
	}
	if (SendMessage(hList, LB_GETCOUNT, 0, 0)==0)
	{
		MessageBox(NULL, "No 3D Devices detected", "Error", MB_OK|MB_ICONSTOP);
	}
	SendMessage(hList, LB_SETCURSEL, 0, 0);
}

LPGUID GetSelectedDeviceGUID(HWND hWnd)
{
	int		index;
	HWND	hList;

	hList=GetDlgItem(hWnd, IDC_DEVICELIST);
	index=SendMessage(hList, LB_GETCURSEL, 0, 0);
	if (index==LB_ERR)
		return(NULL);
	return((LPGUID)SendMessage(hList, LB_GETITEMDATA, index, 0));
}

BOOL CALLBACK SelectDeviceDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		InitSelectDeviceDlg(hWnd);
		return(TRUE);
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hWnd, (int)GetSelectedDeviceGUID(hWnd));
			return(TRUE);
		case IDCANCEL:
			EndDialog(hWnd, -1);//NULL is valid (primary display device)
			return(TRUE);
		}
		break;
	}
	return(FALSE);
}

void PrintGuid(char *buff, LPGUID guid)
{
	if (guid)
	{
		sprintf(buff, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", guid->Data1, guid->Data2, guid->Data3,
			guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3], guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
	}
	else
		strcpy(buff, NULL_STRING_GUID);
}

void DoChange3DDevice(HWND hWnd, moduleinfo_t *info)
{
	LPGUID	guid;

	guid=(LPGUID)DialogBox(hAppInst, MAKEINTRESOURCE(SELECTDEVICE_DLG), hWnd, (DLGPROC)SelectDeviceDlgProc);
	if ((int)guid!=-1)
		PrintGuid(info->guid, guid);
}

void LoadAdvancedSettings(HWND hWnd, moduleinfo_t *info)
{
	int			id;
	default_t	*def;
	HKEY		hkey;
	char		buff[256];
	LONG		rc;

	id=IDC_DYNAMIC;
	strcpy(buff, "Software\\FudgeFactor\\Doom3D\\");
	strcat(buff, info->name);
	strcat(buff, ".dll");
	rc=RegOpenKeyEx(HKEY_CURRENT_USER, buff, 0, KEY_QUERY_VALUE, &hkey);
	if (rc!=ERROR_SUCCESS)
		hkey=NULL;
	for (def=info->api.info->defaults;def->name;def++)
	{
		if (def->isstring)
		{
			strcpy(buff, (char *)def->defaultvalue);
			RegReadString(hkey, def->name, buff, 256);
			SetDlgItemText(hWnd, id, buff);
		}
		else
		{
			rc=RegReadInt(hkey, def->name, def->defaultvalue);
			SetDlgItemInt(hWnd, id, rc, TRUE);
		}
		id++;
	}
	if (info->api.info->caps&APICAP_D3DDEVICE)
	{
		strcpy(info->guid, NULL_STRING_GUID);
		RegReadString(hkey, "3DDevice", info->guid, 39);
	}
	RegCloseKey(hkey);
}

void SaveAdvancedSettings(HWND hWnd, moduleinfo_t *info)
{
	HKEY		hkey;
	LONG		rc;
	DWORD		disp;
	int			id;
	default_t	*def;
	char		buff[256];

	strcpy(buff, "Software\\FudgeFactor\\Doom3D\\");
	strcat(buff, info->name);
	strcat(buff, ".dll");
	rc=RegCreateKeyEx(HKEY_CURRENT_USER, buff, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkey, &disp);
	if (rc!=ERROR_SUCCESS)
		return;

	id=IDC_DYNAMIC;
	for (def=info->api.info->defaults;def->name;def++)
	{
		if (def->isstring)
		{
			GetDlgItemText(hWnd, id, buff, 256);
			RegWriteString(hkey, def->name, buff);
		}
		else
		{
			rc=GetDlgItemInt(hWnd, id, NULL, TRUE);
			RegWriteInt(hkey, def->name, rc);
		}
		id++;
	}
	if (info->api.info->caps&APICAP_D3DDEVICE)
	{
		RegWriteString(hkey, "3DDevice", info->guid);
	}
	RegCloseKey(hkey);
}

BOOL CALLBACK AdvancedDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	moduleinfo_t	*info;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowLong(hWnd, DWL_USER, lParam);
		LoadAdvancedSettings(hWnd, (moduleinfo_t *)lParam);
		return(TRUE);
	case WM_COMMAND:
		info=(moduleinfo_t *)GetWindowLong(hWnd, DWL_USER);
		switch (LOWORD(wParam))
		{
		case IDOK:
			SaveAdvancedSettings(hWnd, info);
			EndDialog(hWnd, TRUE);
			return(TRUE);
		case IDCANCEL:
			EndDialog(hWnd, TRUE);
			return(TRUE);
		case IDC_3DDEVICE:
			DoChange3DDevice(hWnd, info);
			return(TRUE);
		}
		break;
	}
	return(FALSE);
}

void DoAdvancedDlg(HWND hWnd)
{
	moduleinfo_t	*info;
	int				i;

	i=SendDlgItemMessage(hWnd, IDC_DLLNAME, CB_GETCURSEL, 0, 0);
	if (i==CB_ERR)
		return;
	info=(moduleinfo_t *)SendDlgItemMessage(hWnd, IDC_DLLNAME, CB_GETITEMDATA, i, 0);
	if (!info)
		return;

	if (!info->hTemplate)
		info->hTemplate=MakeAdvancedDlgTemplate(info);
	if (-1==DialogBoxIndirectParam(hAppInst, info->hTemplate, hWnd, (DLGPROC)AdvancedDlgProc, (LPARAM)info))
		Moan("Help!");
}

void DroneToggle(HWND hWnd)
{
	if (IsDlgButtonChecked(hWnd, IDC_DRONE)==BST_UNCHECKED)
	{
		CheckDlgButton(hWnd, IDC_LEFT, BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_RIGHT, BST_UNCHECKED);
		CheckDlgButton(hWnd, IDC_BACK, BST_UNCHECKED);
	}
}

void DroneTypeToggle(HWND hWnd, int id)
{
	if (IsDlgButtonChecked(hWnd, id)==BST_CHECKED)
	{
		CheckDlgButton(hWnd, IDC_DRONE, BST_CHECKED);
		if (id!=IDC_LEFT)
			CheckDlgButton(hWnd, IDC_LEFT, BST_UNCHECKED);
		if (id!=IDC_RIGHT)
			CheckDlgButton(hWnd, IDC_RIGHT, BST_UNCHECKED);
		if (id!=IDC_BACK)
			CheckDlgButton(hWnd, IDC_BACK, BST_UNCHECKED);
	}
}

void InitNetgameDlg(HWND hWnd)
{
	HKEY	hkey;
	LONG	rc;
	int		i;
	char	buff[256];
	char	name[7];

	rc=RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\FudgeFactor\\Doom3D\\Netgame", 0, KEY_QUERY_VALUE, &hkey);
	if (rc!=ERROR_SUCCESS)
		hkey=NULL;

	SendDlgItemMessage(hWnd, IDC_NUMPLAYERS, EM_SETLIMITTEXT, 1, 0);
	rc=RegReadInt(hkey, "NumPlayers", 2);
	SetDlgItemInt(hWnd, IDC_NUMPLAYERS, rc, FALSE);

	SendDlgItemMessage(hWnd, IDC_PLAYER, EM_SETLIMITTEXT, 1, 0);
	rc=RegReadInt(hkey, "Player", 1);
	SetDlgItemInt(hWnd, IDC_PLAYER, rc, FALSE);

	rc=RegReadInt(hkey, "Port", 0x666);
	SetDlgItemInt(hWnd, IDC_PORTNUM, rc, FALSE);

	for (i=0;i<NUM_NETREG_NODES;i++)
	{
		sprintf(name, "Node%d", i);
		buff[0]=0;
		RegReadString(hkey, name, buff, 256);
		SetDlgItemText(hWnd, IDC_NODE1+i, buff);
	}

	rc=RegReadInt(hkey, "Flags", false);
	if (rc&NDF_DRONE)
		CheckDlgButton(hWnd, IDC_DRONE, rc?BST_CHECKED:BST_UNCHECKED);
	if (rc&NDF_BACK)
		CheckDlgButton(hWnd, IDC_BACK, rc?BST_CHECKED:BST_UNCHECKED);
	if (rc&NDF_LEFT)
		CheckDlgButton(hWnd, IDC_LEFT, rc?BST_CHECKED:BST_UNCHECKED);
	if (rc&NDF_RIGHT)
		CheckDlgButton(hWnd, IDC_RIGHT, rc?BST_CHECKED:BST_UNCHECKED);

	if (rc&NDF_DEATH2)
		CheckDlgButton(hWnd, IDC_DEATH2, BST_CHECKED);
	else if (rc&NDF_DEATHMATCH)
		CheckDlgButton(hWnd, IDC_DEATH2, BST_CHECKED);
	else
		CheckDlgButton(hWnd, IDC_COOP, BST_CHECKED);

	rc=RegReadInt(hkey, "SplitPlayers", 1);
	SetDlgItemInt(hWnd, IDC_SPLITSCREEN, rc, FALSE);

	if (hkey)
		RegCloseKey(hkey);
}

void SaveNetgameSettings(HWND hWnd)
{
	HKEY	hkey;
	LONG	rc;
	int		i;
	char	buff[256];
	char	name[7];

	rc=RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\FudgeFactor\\Doom3D\\Netgame", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hkey, (LPDWORD)&i);
	if (rc!=ERROR_SUCCESS)
		return;
	rc=GetDlgItemInt(hWnd, IDC_NUMPLAYERS, NULL, FALSE);
	RegWriteInt(hkey, "NumPlayers", rc);

	rc=GetDlgItemInt(hWnd, IDC_PLAYER, NULL, FALSE);
	RegWriteInt(hkey, "Player", rc);

	rc=GetDlgItemInt(hWnd, IDC_PORTNUM, NULL, FALSE);
	RegWriteInt(hkey, "Port", rc);

	rc=0;
	for (i=0;i<NUM_NETREG_NODES;i++)
	{
		GetDlgItemText(hWnd, IDC_NODE1+i, buff, 256);
		if (buff[0])
			rc++;
		sprintf(name, "Node%d", i);
		RegWriteString(hkey, name, buff);
	}

	if (rc)
		i=0;
	else
		i=NDF_SPLITONLY;
	if (IsDlgButtonChecked(hWnd, IDC_DRONE)==BST_CHECKED)
		i|=NDF_DRONE;
	else if (IsDlgButtonChecked(hWnd, IDC_BACK)==BST_CHECKED)
		i|=NDF_BACK;
	else if (IsDlgButtonChecked(hWnd, IDC_LEFT)==BST_CHECKED)
		i|=NDF_LEFT;
	else if (IsDlgButtonChecked(hWnd, IDC_RIGHT)==BST_CHECKED)
		i|=NDF_RIGHT;

	if (IsDlgButtonChecked(hWnd, IDC_DEATH2)==BST_CHECKED)
		i|=NDF_DEATH2;
	else if (IsDlgButtonChecked(hWnd, IDC_DEATHMATCH)==BST_CHECKED)
		i|=NDF_DEATHMATCH;

	RegWriteInt(hkey, "Flags", i);

	SplitPlayers=GetDlgItemInt(hWnd, IDC_SPLITSCREEN, NULL, FALSE);
	if (SplitPlayers<1)
		SplitPlayers=1;
	else if (SplitPlayers>MAX_SPLITS)
		SplitPlayers=MAX_SPLITS;
	RegWriteInt(hkey, "SplitPlayers", SplitPlayers);

	RegCloseKey(hkey);
}

BOOL CALLBACK NetgameDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		InitNetgameDlg(hWnd);
		return(TRUE);
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_DRONE:
			DroneToggle(hWnd);
			return(TRUE);
		case IDC_LEFT:
		case IDC_RIGHT:
		case IDC_BACK:
			DroneTypeToggle(hWnd, LOWORD(wParam));
			return(TRUE);
		case IDOK:
			SaveNetgameSettings(hWnd);
			EndDialog(hWnd, TRUE);
			return(TRUE);
		case IDCANCEL:
			SplitPlayers=1;
			EndDialog(hWnd, FALSE);
			return(TRUE);
		}
		break;
	}
	return(FALSE);
}

void SetupNetgame(HWND hWnd)
{
	if (IsDlgButtonChecked(hWnd, IDC_NETGAME)!=BST_CHECKED)
		return;
	if (!DialogBox(hAppInst, MAKEINTRESOURCE(NETGAME_DLG), hWnd, (DLGPROC)NetgameDlgProc))
	{
		CheckDlgButton(hWnd, IDC_NETGAME, BST_UNCHECKED);
	}
}

void InitControlsDlg(HWND hWnd)
{
	SetDlgItemInt(hWnd, IDC_JOYSTICK, UseJoystick, FALSE);
	CheckDlgButton(hWnd, IDC_ANALOGUEJOY, DigiJoy?BST_UNCHECKED:BST_CHECKED);
	SetDlgItemInt(hWnd, IDC_MOUSE, UseMouse, FALSE);
	SetDlgItemInt(hWnd, IDC_MOUSECOM, MouseCom, TRUE);
	SetDlgItemInt(hWnd, IDC_MOUSE2, UseMouse2, FALSE);
}

void EndControlsDlg(HWND hWnd)
{
	UseMouse=GetDlgItemInt(hWnd, IDC_MOUSE, NULL, FALSE);
	MouseCom=GetDlgItemInt(hWnd, IDC_MOUSECOM, NULL, TRUE);
	UseMouse2=GetDlgItemInt(hWnd, IDC_MOUSE2, NULL, FALSE);
	UseJoystick=GetDlgItemInt(hWnd, IDC_JOYSTICK, NULL, FALSE);
	if (IsDlgButtonChecked(hWnd, IDC_ANALOGUEJOY)==BST_CHECKED)
		DigiJoy=false;
	else
		DigiJoy=true;
}

BOOL CALLBACK ControlsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		InitControlsDlg(hWnd);
		return(TRUE);
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndControlsDlg(hWnd);
			EndDialog(hWnd, TRUE);
			break;
		}
	}
	return(FALSE);
}

void DoControlsDlg(HWND hWnd)
{
	DialogBox(hAppInst, MAKEINTRESOURCE(CONTROLS_DLG), hWnd, (DLGPROC)ControlsDlgProc);
}

void BuildCommandLine(HWND hWnd)
{
	strcpy(CommandLine, "Doom3D.exe ");
	GetDlgItemText(hWnd, IDC_PARAMETERS, CommandLine+11, 1005);
	if (IsDlgButtonChecked(hWnd, IDC_NETGAME)==BST_CHECKED)
		strcat(CommandLine, " -netreg");
	if (SplitPlayers>1)
		sprintf(&CommandLine[strlen(CommandLine)], " -split %d", SplitPlayers);
}

BOOL CALLBACK MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		InitMainDlg(hWnd);
		return(TRUE);
	case WM_DESTROY:
		CleanupMainDlg(hWnd);
		return(TRUE);
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_ADVANCED:
			DoAdvancedDlg(hWnd);
			return(TRUE);
		case IDC_NETGAME:
			SetupNetgame(hWnd);
			return(TRUE);
		case IDC_CONTROLS:
			DoControlsDlg(hWnd);
			return(0);
		case IDOK:
			SaveSettings(hWnd);
			BuildCommandLine(hWnd);
			EndDialog(hWnd, TRUE);
			return(TRUE);
		case IDC_SAVE:
			SaveSettings(hWnd);
			EndDialog(hWnd, FALSE);
		case IDCANCEL:
			EndDialog(hWnd, FALSE);
			return(TRUE);
		}
		break;
	}
	return(FALSE);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	PROCESS_INFORMATION	pi;
	STARTUPINFO			si;
	int					rc;

	hAppInst=hInstance;
	InitCommonControls();
	rc=DialogBox(hAppInst, MAKEINTRESOURCE(MAIN_DLG), NULL, (DLGPROC)MainDlgProc);
	if (!rc)
		return(0);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb=sizeof(STARTUPINFO);
	if (CreateProcess(NULL, CommandLine, NULL, NULL,
		FALSE, CREATE_DEFAULT_ERROR_MODE|DETACHED_PROCESS,
		NULL, NULL, &si, &pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	return(0);
}

void Moan(char *fmt, ...)
{
	va_list	va;
	char	buff[1024];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);
	MessageBox(NULL, buff, "Error", MB_OK);
	exit(1);
}
