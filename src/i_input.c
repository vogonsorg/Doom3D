//DOOM3D
//Input Routines

#include <stdlib.h>
#include <dinput.h>

#include "doomstat.h"
#include "i_system.h"
#include "i_windoz.h"
#include "d_main.h"
#include "m_argv.h"
#include "t_fixed.h"

#define KEYBOARD_BUFFERSIZE 64

dboolean	GrabMouse;
int			UseJoystick;
int			UseMouse[2];
dboolean	DigiJoy;
int			DualMouse;

LPDIRECTINPUT			pDI=NULL;
LPDIRECTINPUTDEVICE		pDIMouse=NULL;
LPDIRECTINPUTDEVICE		pDIKeyboard=NULL;
LPDIRECTINPUTDEVICE2	pDIJoystick=NULL;

HANDLE					MouseCom=INVALID_HANDLE_VALUE;
dboolean				MouseMode;//false=microsoft, true=mouse systems

const char qwerty[]="qwertyuiop";
const char asdf[]="asdfghjkl;'";
const char zxcv[]="\\zxcvbnm,./";

int TranslateKey(int key)
{
	int		rc;

    switch(key)
    {
	case DIK_LEFT:		rc = KEY_LEFTARROW;     break;
	case DIK_RIGHT:		rc = KEY_RIGHTARROW;    break;
	case DIK_DOWN:		rc = KEY_DOWNARROW;     break;
	case DIK_UP:		rc = KEY_UPARROW;       break;
	case DIK_ESCAPE:	rc = KEY_ESCAPE;        break;
	case DIK_NUMPADENTER:
	case DIK_RETURN:	rc = KEY_ENTER;         break;
	case DIK_TAB:		rc = KEY_TAB;           break;
	case DIK_F1:		rc = KEY_F1;            break;
	case DIK_F2:		rc = KEY_F2;            break;
	case DIK_F3:		rc = KEY_F3;            break;
	case DIK_F4:		rc = KEY_F4;            break;
	case DIK_F5:		rc = KEY_F5;            break;
	case DIK_F6:		rc = KEY_F6;            break;
	case DIK_F7:		rc = KEY_F7;            break;
	case DIK_F8:		rc = KEY_F8;            break;
	case DIK_F9:		rc = KEY_F9;            break;
	case DIK_F10:		rc = KEY_F10;           break;
	case DIK_F11:		rc = KEY_F11;           break;
	case DIK_F12:		rc = KEY_F12;           break;

	case DIK_BACK:		rc = KEY_BACKSPACE;     break;
	case DIK_GRAVE:		rc='~';					break;

	//case DIK_PAUSE:		rc = KEY_PAUSE;         break;

	case DIK_NUMPADEQUALS:
	case DIK_EQUALS:	rc = KEY_EQUALS;        break;

	case DIK_SUBTRACT:
	case DIK_MINUS:		rc = KEY_MINUS;         break;

	case DIK_LSHIFT:
	case DIK_RSHIFT:	rc = KEY_SHIFT;			break;

	case DIK_LCONTROL:
	case DIK_RCONTROL:	rc = KEY_CTRL;			break;

	case DIK_LALT:
	case DIK_RALT:		rc = KEY_ALT;			break;
	case DIK_SPACE:		rc = ' ';				break;
	case DIK_0:			rc='0';					break;
	case DIK_CAPITAL:	rc=KEY_CAPS;			break;
	case DIK_INSERT:	rc=KEY_INS;				break;
	case DIK_DELETE:	rc=KEY_DEL;				break;
	case DIK_HOME:		rc=KEY_HOME;			break;
	case DIK_END:		rc=KEY_END;				break;
	case DIK_PRIOR:		rc=KEY_PGUP;			break;
	case DIK_NEXT:		rc=KEY_PGDOWN;			break;

	case DIK_NUMPAD0:	rc=KEY_KP0;				break;
	case DIK_NUMPAD1:	rc=KEY_KP1;				break;
	case DIK_NUMPAD2:	rc=KEY_KP2;				break;
	case DIK_NUMPAD3:	rc=KEY_KP3;				break;
	case DIK_NUMPAD4:	rc=KEY_KP4;				break;
	case DIK_NUMPAD5:	rc=KEY_KP5;				break;
	case DIK_NUMPAD6:	rc=KEY_KP6;				break;
	case DIK_NUMPAD7:	rc=KEY_KP7;				break;
	case DIK_NUMPAD8:	rc=KEY_KP8;				break;
	case DIK_NUMPAD9:	rc=KEY_KP9;				break;

// these two aren't standard, but seem to work...
	case 0x56:			rc='\\';				break;
	case 0xc5:			rc=KEY_PAUSE;			break;

	default:
		if ((key>=DIK_1)&&(key<=DIK_9))
			rc=((key-DIK_1)%10)+'1';
		else if ((key>=DIK_Q)&&(key<=DIK_RBRACKET))
			rc=qwerty[key-DIK_Q];
		else if ((key>=DIK_A)&&(key<=DIK_APOSTROPHE))
			rc=asdf[key-DIK_A];
		else if ((key>=DIK_BACKSLASH)&&(key<=DIK_SLASH))
			rc=zxcv[key-DIK_BACKSLASH];
		else
			rc=0;
	}
	return(rc);
}

void I_ReacquireInput(void)
{
    if (pDIMouse)
		pDIMouse->lpVtbl->Acquire(pDIMouse);
    if (pDIKeyboard)
		pDIKeyboard->lpVtbl->Acquire(pDIKeyboard);
}

void I_InitKeyboard(void)
{
    HRESULT	hres;
    DIPROPDWORD	dipdw;

    hres=pDI->lpVtbl->CreateDevice(pDI, &GUID_SysKeyboard, &pDIKeyboard, NULL);
    if (hres!=DI_OK)
		I_Error("CreateDevice(SysKeyboard) Failed");
    hres=pDIKeyboard->lpVtbl->SetCooperativeLevel(pDIKeyboard, hMainWnd, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
    if (hres!=DI_OK)
		I_Error("Keyboard::SetCooperativeLevel Failed");
    hres=pDIKeyboard->lpVtbl->SetDataFormat(pDIKeyboard, &c_dfDIKeyboard);
    if (hres!=DI_OK)
		I_Error("Keyboard::SetDataFormat Failed");
    dipdw.diph.dwSize=sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize=sizeof(DIPROPHEADER);
    dipdw.diph.dwObj=0;
    dipdw.diph.dwHow=DIPH_DEVICE;
    dipdw.dwData=KEYBOARD_BUFFERSIZE;
    hres=pDIKeyboard->lpVtbl->SetProperty(pDIKeyboard, DIPROP_BUFFERSIZE, &dipdw.diph);
    if (hres!=DI_OK)
		I_Error("Keyboard::SetProperty(BufferSize) Failed");
    pDIKeyboard->lpVtbl->Acquire(pDIKeyboard);
}

void I_InitMouse(void)
{
    HRESULT	hres;

    hres=pDI->lpVtbl->CreateDevice(pDI, &GUID_SysMouse, &pDIMouse, NULL);
    if (hres!=DI_OK)
	{
		I_Printf("I_InitMouse:CreateDevice(SysMouse) Failed - mouse input disabled");
		pDIMouse=NULL;
		return;
	}
    GrabMouse=false;
	if (!InWindow)
		hres=pDIMouse->lpVtbl->SetCooperativeLevel(pDIMouse, hMainWnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND);
    if (InWindow||(hres!=DI_OK))
    {
		if (!M_CheckParm("-nograbmouse"))
			GrabMouse=true;
		hres=pDIMouse->lpVtbl->SetCooperativeLevel(pDIMouse, hMainWnd, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
		if (hres!=DI_OK)
		{
			I_Printf("I_InitMouse:SetCooperativeLevel Failed - mouse input disabled");
			SAFE_RELEASE(pDIMouse);
			return;
		}
    }
    hres=pDIMouse->lpVtbl->SetDataFormat(pDIMouse, &c_dfDIMouse);
    if (hres!=DI_OK)
	{
		I_Printf("I_InitMouse:SetDataFormat Failed - mouse input disabled");
		SAFE_RELEASE(pDIMouse);
		return;
	}
    pDIMouse->lpVtbl->Acquire(pDIMouse);
}

static dboolean	foundjoy;

BOOL CALLBACK JoystickEnumProc(LPDIDEVICEINSTANCE ddi, LPVOID guid)
{
	memcpy(guid, &ddi->guidInstance, sizeof(GUID));
	foundjoy=true;
	I_Printf("I_InitInput:Using joystick:%s\n", ddi->tszInstanceName);
	return(DIENUM_STOP);
}

int		JoyXHigh;
int		JoyXLow;
int		JoyYHigh;
int		JoyYLow;

void I_InitJoystick(void)
{
	HRESULT				hres;
	GUID				guid;
	LPDIRECTINPUTDEVICE	pdid;
	DIPROPRANGE			dipr;

	foundjoy=false;
	hres=pDI->lpVtbl->EnumDevices(pDI, DIDEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)JoystickEnumProc, &guid, DIEDFL_ATTACHEDONLY);
	if ((hres!=DI_OK)||!foundjoy)
	{
		I_Printf("I_InitInput:No joysticks found\n");
		return;
	}

    hres=pDI->lpVtbl->CreateDevice(pDI, &guid, &pdid, NULL);
    if (hres!=DI_OK)
		I_Error("CreateDevice(Joystick) Failed");
	hres=pdid->lpVtbl->QueryInterface(pdid, &IID_IDirectInputDevice2, (void **)&pDIJoystick);
	if (hres!=DI_OK)
		I_Error("Joystick::QueryInterface(IDirectInputDevice2) Failed");
	pdid->lpVtbl->Release(pdid);

	if (!InWindow)
		hres=pDIJoystick->lpVtbl->SetCooperativeLevel(pDIJoystick, hMainWnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND);
    if (InWindow||(hres!=DI_OK))
    {
		hres=pDIJoystick->lpVtbl->SetCooperativeLevel(pDIJoystick, hMainWnd, DISCL_NONEXCLUSIVE|DISCL_FOREGROUND);
		if (hres!=DI_OK)
			I_Error("Joystick::SetCooperativeLevel Failed");
    }
    hres=pDIJoystick->lpVtbl->SetDataFormat(pDIJoystick, &c_dfDIJoystick);
    if (hres!=DI_OK)
		I_Error("Joystick::SetDataFormat Failed");

	dipr.diph.dwSize=sizeof(DIPROPRANGE);
	dipr.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipr.diph.dwObj=DIJOFS_X;
	dipr.diph.dwHow=DIPH_BYOFFSET;
	hres=pDIJoystick->lpVtbl->GetProperty(pDIJoystick, DIPROP_RANGE, (LPDIPROPHEADER)&dipr);
	if (hres!=DI_OK)
		I_Error("GetProperty(DIPROP_RANGEX) Failed");
	JoyXLow=(dipr.lMin+dipr.lMax)/2;
	JoyXHigh=dipr.lMax-JoyXLow;
	dipr.diph.dwSize=sizeof(DIPROPRANGE);
	dipr.diph.dwHeaderSize=sizeof(DIPROPHEADER);
	dipr.diph.dwObj=DIJOFS_X;
	dipr.diph.dwHow=DIPH_BYOFFSET;
	hres=pDIJoystick->lpVtbl->GetProperty(pDIJoystick, DIPROP_RANGE, (LPDIPROPHEADER)&dipr);
	if (hres!=DI_OK)
		I_Error("GetProperty(DIPROP_RANGEY) Failed");
	JoyYLow=(dipr.lMin+dipr.lMax)/2;
	JoyYHigh=dipr.lMax-JoyYLow;

    pDIJoystick->lpVtbl->Acquire(pDIJoystick);
}

void I_InitMouse2(void)
{
	char			comname[5];
	DCB				dcb;
	COMMTIMEOUTS	cto;

	strcpy(comname, "COM");
	if (DualMouse<0)
	{
		comname[3]='0'-DualMouse;
		MouseMode=1;
	}
	else
	{
		comname[3]='0'+DualMouse;
		MouseMode=0;
	}
	comname[4]=0;

	MouseCom=CreateFile(comname, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (MouseCom==INVALID_HANDLE_VALUE)
	{
		I_Printf("I_InitMouse2:Unable to open %s\n", comname);
		return;
	}
	dcb.DCBlength=sizeof(DCB);
	GetCommState(MouseCom, &dcb);
	dcb.BaudRate=1200;
	if (MouseMode)
		dcb.ByteSize=8;
	else
		dcb.ByteSize=7;
	dcb.StopBits=2;

	SetCommState(MouseCom, &dcb);
	if (!SetupComm(MouseCom, 256, 4))
	{
		I_Printf("I:InitMouse2:SetupCom failed\n");
		CloseHandle(MouseCom);
		MouseCom=INVALID_HANDLE_VALUE;
		return;
	}
	cto.ReadIntervalTimeout=MAXDWORD;
	cto.ReadTotalTimeoutMultiplier=0;
	cto.ReadTotalTimeoutConstant=0;
	cto.WriteTotalTimeoutMultiplier=0;
	cto.WriteTotalTimeoutConstant=0;
	if (!SetCommTimeouts(MouseCom, &cto))
	{
		printf("I_InitMouse2:Unable to set com timeouts\n");
		CloseHandle(MouseCom);
		MouseCom=INVALID_HANDLE_VALUE;
		return;
	}
	I_Printf("I_InitInput:Dual mouse on %s (%s mode)\n", comname, MouseMode?"Mouse systems":"Microsoft");
}

void I_InitInput(void)
{
    HRESULT	hres;

    hres=DirectInputCreate(hAppInst, DIRECTINPUT_VERSION, &pDI, NULL);
    if (hres!=DI_OK)
		I_Error("DirectInputCreate Failed");
	if (UseMouse[0])
		I_InitMouse();
    I_InitKeyboard();
	if (UseJoystick)
		I_InitJoystick();
	if (DualMouse&&UseMouse[1])
		I_InitMouse2();
}

void I_ShutdownInput(void)
{
	if (pDIJoystick)
	{
		pDIJoystick->lpVtbl->Unacquire(pDIJoystick);
		pDIJoystick->lpVtbl->Release(pDIJoystick);
		pDIJoystick=NULL;
	}
    if (pDIKeyboard)
    {
		pDIKeyboard->lpVtbl->Unacquire(pDIKeyboard);
		pDIKeyboard->lpVtbl->Release(pDIKeyboard);
		pDIKeyboard=NULL;
    }
    if (pDIMouse)
    {
		pDIMouse->lpVtbl->Unacquire(pDIMouse);
		pDIMouse->lpVtbl->Release(pDIMouse);
		pDIMouse=NULL;
    }
    SAFE_RELEASE(pDI);
    if (MouseCom!=INVALID_HANDLE_VALUE)
    {
		CloseHandle(MouseCom);
		MouseCom=INVALID_HANDLE_VALUE;
	}
}

void I_ProcessJoystick(void)
{
	DIJOYSTATE	dijs;
	int			i;
	HRESULT		hres;
	event_t		event;
	static int	oldb=0;

	hres=pDIJoystick->lpVtbl->Poll(pDIJoystick);
    if (hres==DIERR_INPUTLOST)
    {
		hres=pDIJoystick->lpVtbl->Acquire(pDIJoystick);
		if (hres!=DI_OK)
			return;
	    hres=pDIJoystick->lpVtbl->GetDeviceState(pDIJoystick, sizeof(DIJOYSTATE), &dijs);
    }
    if (hres!=DI_OK)
		return;

    hres=pDIJoystick->lpVtbl->GetDeviceState(pDIJoystick, sizeof(DIJOYSTATE), &dijs);
    if (hres!=DI_OK)
		return;

	event.data1=0;
	for (i=0;i<JOYSTICK_BUTTONS;i++)
	{
		if (dijs.rgbButtons[i]&0x80)
			event.data1|=1<<i;
	}

	event.type=ev_joystick;
	event.data2=(int)(FRACUNIT*((dijs.lX-JoyXLow)/(float)JoyXHigh));
	event.data3=(int)(FRACUNIT*((dijs.lY-JoyYLow)/(float)JoyYHigh));
	if ((event.data1==oldb)&&(event.data2==0)&&(event.data3==0))
		return;

	oldb=event.data1;
	if (event.type==ev_joystick)
		D_PostEvent(&event);
}

void I_ProcessMouse(void)
{
    event_t			event;
    HRESULT			hres;
    DIMOUSESTATE	dims;
    int				i;
	static int		oldb=0;

    if (GrabMouse)
		SetCursorPos(ScreenWidth/2+WindowPos.left, ScreenHeight/2+WindowPos.top);

    hres=pDIMouse->lpVtbl->GetDeviceState(pDIMouse, sizeof(DIMOUSESTATE), &dims);
    if (hres==DIERR_INPUTLOST)
    {
		hres=pDIMouse->lpVtbl->Acquire(pDIMouse);
		if (hres!=DI_OK)
			return;
		hres=pDIMouse->lpVtbl->GetDeviceState(pDIMouse, sizeof(DIMOUSESTATE), &dims);
    }
    if (hres!=DI_OK)
		return;
    event.type=ev_mouse;
    event.data1=0;
    for (i=0;i<MOUSE_BUTTONS;i++)
    {
		if (dims.rgbButtons[i]&0x80)
			event.data1|=1<<i;
    }
    event.data2=dims.lX<<2;
    event.data3=-dims.lY<<2;

    if ((event.data1==oldb)&&(event.data2==0)&&(event.data3==0))
    	return;
    oldb=event.data1;
    D_PostEvent(&event);
}

void I_ProcessKeyboard(void)
{
    DWORD			num;
    HRESULT			hres;
    DIDEVICEOBJECTDATA		data[KEYBOARD_BUFFERSIZE];
    LPDIDEVICEOBJECTDATA	pdata;
    event_t			event;

    num=KEYBOARD_BUFFERSIZE;
    hres=pDIKeyboard->lpVtbl->GetDeviceData(pDIKeyboard, sizeof(DIDEVICEOBJECTDATA), data, &num, 0);
    if (hres==DIERR_INPUTLOST)
    {
		hres=pDIKeyboard->lpVtbl->Acquire(pDIKeyboard);
		if (hres!=DI_OK)
			return;
		num=KEYBOARD_BUFFERSIZE;
		hres=pDIKeyboard->lpVtbl->GetDeviceData(pDIKeyboard, sizeof(DIDEVICEOBJECTDATA), data, &num, 0);
    }
    if (hres!=DI_OK)
		return;
    for (pdata=data;pdata<&data[num];pdata++)
    {
		if (pdata->dwData&0x80)
			event.type=ev_keydown;
		else
			event.type=ev_keyup;
		event.data1=TranslateKey(pdata->dwOfs);
		D_PostEvent(&event);
    }
}

byte	MouseInputData[5];
int		MouseInputPos=0;

int		MouseX2;
int		MouseY2;
int		Mouse2Buttons=0;

void I_ProcessMicrosoftMouseData(void)
{
	Mouse2Buttons=((MouseInputData[0]&0x10)>>3)|((MouseInputData[0]&0x20)>>5);
	MouseX2+=(char)(MouseInputData[1]+((MouseInputData[0]&0x3)<<6));
	MouseY2+=(char)(MouseInputData[2]+((MouseInputData[0]&0xc)<<4));
}

void I_ProcessMouseSystemsData(void)
{
	byte	b;

	b=~MouseInputData[0];
	Mouse2Buttons=((b&4)>>2)|((b&1)<<1)|((b&2)<<1);
	MouseX2+=(char)MouseInputData[1];
	MouseX2+=(char)MouseInputData[3];
	MouseY2+=(char)MouseInputData[2];
	MouseY2+=(char)MouseInputData[4];
}

void I_ProcessMouse2(void)
{
	event_t			event;
	DWORD			size;
	int				omb;
	int				datasize;

	MouseX2=0;
	MouseY2=0;
	omb=Mouse2Buttons;
	if (MouseMode)
		datasize=5;
	else
		datasize=3;
	size=1;
	while (size>0)
	{
		if (ReadFile(MouseCom, &MouseInputData[MouseInputPos], datasize-MouseInputPos, &size, NULL))
		{
			MouseInputPos+=size;
			if (MouseInputPos>=(MouseMode?5:3))
			{
				MouseInputPos=0;
				if (MouseMode)
					I_ProcessMouseSystemsData();
				else
					I_ProcessMicrosoftMouseData();
			}
		}
		else
		{
			I_Printf("I_ProcessMouse2:Read error\n");
			size=0;
		}
	}

	if ((MouseX2==0)&&(MouseY2==0)&&(Mouse2Buttons==omb))
		return;

    event.type=ev_mouse2;
    event.data1=Mouse2Buttons;
    event.data2=MouseX2<<2;
    event.data3=-MouseY2<<2;
    D_PostEvent(&event);
}

void I_ProcessInput(void)
{
	if (pDIJoystick)
		I_ProcessJoystick();
    if (pDIMouse)
		I_ProcessMouse();
    if (pDIKeyboard)
		I_ProcessKeyboard();
	if (MouseCom!=INVALID_HANDLE_VALUE)
		I_ProcessMouse2();
}
