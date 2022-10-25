#include <windows.h>

int RegReadInt(HKEY hkey, char *name, int def)
{
	int		value;
	LONG	rc;
	DWORD	size;
	DWORD	type;

	if (!hkey)
		return(def);
	size=4;
	rc=RegQueryValueEx(hkey, name, NULL, &type, (LPBYTE)&value, &size);
	if ((rc!=ERROR_SUCCESS)||(type!=REG_DWORD))
		return(def);
	return(value);
}

void RegReadString(HKEY hkey, char *name, char *buff, int len)
{
	char	val[1024];
	DWORD	size;
	DWORD	type;
	LONG	rc;

	if (!hkey)
		return;
	size=len;
	if (size>1024)
		size=1024;
	rc=RegQueryValueEx(hkey, name, NULL, &type, (LPBYTE)val, &size);
	if (rc!=ERROR_SUCCESS)
		return;
	if (type!=REG_SZ)
		return;
	strcpy(buff, val);
}

void RegWriteInt(HKEY hkey, char *name, int i)
{
	RegSetValueEx(hkey, name, 0, REG_DWORD, (LPBYTE)&i, 4);
}

void RegWriteString(HKEY hkey, char *name, char *s)
{
	RegSetValueEx(hkey, name, 0, REG_SZ, (LPBYTE)s, strlen(s)+1);
}
