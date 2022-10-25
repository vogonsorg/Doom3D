#ifndef I_REG_H
#define I_REG_H

int RegReadInt(HKEY hkey, char *name, int def);
void RegReadString(HKEY hkey, char *name, char *buff, int len);
void RegWriteInt(HKEY hkey, char *name, int i);
void RegWriteString(HKEY hkey, char *name, char *s);

#endif