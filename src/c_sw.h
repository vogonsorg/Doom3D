#ifndef C_SW_H
#define C_SW_H

#include "c_dll.h"
#include "t_zone.h"

#define Z_ChangeTag(p,t) \
{ \
      if (( (memblock_t *)( (byte *)(p) - sizeof(memblock_t)))->id!=0x1d4a11) \
	  I_Error("Z_CT at "__FILE__":%i",__LINE__); \
	  Z_ChangeTag2(p,t); \
};


extern void *(*Z_Malloc)(int size, int tag, void *user);
extern void (*Z_Free)(void *ptr);
extern void (*Z_ChangeTag2)(void *ptr, int tag);
extern void *(*W_CacheLumpNum)(int lump, int tag);
extern void *(*W_CacheLumpName)(char *name, int tag);
extern int (*W_CheckNumForName)(char *name);
extern int (*W_GetNumForName)(char *name);
extern int (*W_LumpLength)(int lump);
extern void (*W_ReadLump)(int lump, void *dest);
extern dboolean (*L_CanSee)(int s1, int s2);

extern int	NumSplits;

#endif