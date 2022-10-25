/*
d3dr_md2.c
MD2 model loading

Paul Brook
*/

#include <ctype.h>
#include "d3dr_local.h"
#include "d3di_video.h"
#include "c_d3d.h"
#include "t_info.h"

#define MAX_SKINNAME 64

typedef struct
{
	int ident;
	int version;

	int skinwidth;
	int skinheight;
	int framesize;

	int num_skins;
	int num_xyz;
	int num_st;
	int num_tris;
	int num_glcmds;
	int num_frames;

	int ofs_skins;
	int ofs_st;
	int ofs_tris;
	int ofs_frames;
	int ofs_glcmds;
	int ofs_end;

}md2header_t;


typedef struct
{
	short	s;
	short	t;
}md2st_t;

typedef struct
{
	short	v[3];
	short	st[3];
}md2tris_t;

typedef struct
{
	byte	x;
	byte	y;
	byte	z;
	byte	light;
}md2vert_t;

typedef struct
{
	float	x;
	float	y;
	float	z;
}md2v3_t;

typedef struct
{
	md2v3_t		scale;
	md2v3_t		translate;
	char		name[16];
	md2vert_t	verts[1];
}md2frame_t;

typedef struct
{
	char	*name;
	model_t	*model;
}md2name_t;

static md2name_t	*md2names;
static int			nummd2names;
static char			sectionname[256];

int				MaxModelCmds;
DOOM3DVERTEX	*ModelVertexBuffer;
playerskin_t	PlayerSkins[MAXPLAYERS];

void R_SafeRead(void *buff, int size, FILE *fh)
{
	int		c;

	c=fread(buff, 1, size, fh);
	if (c!=size)
		I_Error("Read Error %d/%d", c, size);
}

int R_RegisterSkin(char *name, float *pwidth, float *pheight)
{
	skin_t			*skin;
	pcxheader_t		header;
	FILE			*fh;
	int				i;

	strlwr(name);
	for (i=0;i<NumSkins;i++)
	{
		if (strcmp(name, Skins[i].name)==0)
		{
			break;
		}
	}

	fh=fopen(name, "rb");
	if (!fh)
		return(-1);
	R_SafeRead(&header, sizeof(header), fh);
	if ((header.id!=0x0A)||(header.ver!=5))
	{
		I_Printf("Invalid PCX:%s\n", name);
		fclose(fh);
		return(-1);
	}

	if (pwidth)
		*pwidth=(float)(header.x1+1-header.x0);
	if (pheight)
		*pheight=(float)(header.y1+1-header.y0);
	if (i==NumSkins)
	{
		skin=&Skins[NumSkins++];
		skin->name=strdup(name);
		skin->ptex=NULL;
		skin->count=0;
	}
	fclose(fh);
	return(i);
}

model_t *R_LoadMD2(char *filename, float offset, float scale)
{
	FILE			*fh;
	model_t			*m;
	md2header_t		header;
	md2st_t			*stvert;
	md2frame_t		*mframe;
	md2tris_t		*tris;
	float			skinwidth;
	float			skinheight;
	int				frame;
	int				i;
	char			skinname[MAX_SKINNAME];
	char			buff[256];
	char			*p;
	int				*cmd;

	fh=fopen(filename, "rb");
	if (!fh)
	{
		I_Printf("Unable to open %s\n", filename);
		return(NULL);
	}
	R_SafeRead(&header, sizeof(md2header_t), fh);

	m=(model_t *)Z_Malloc(sizeof(model_t)+(header.num_frames-1)*sizeof(modelframe_t *), PU_STATIC, NULL);
	m->numframes=header.num_frames;
	m->numverts=header.num_xyz;
	m->numtris=header.num_tris*3;
	fseek(fh, header.ofs_skins, SEEK_SET);
	R_SafeRead(skinname, MAX_SKINNAME, fh);

	strcpy(buff, filename);;
	p=strrchr(buff, '\\');
	if (p)
		p[1]=0;
	else
		buff[0]=0;
	p=strrchr(skinname, '/');
	if (p)
		p++;
	else
		p=skinname;
	strcat(buff, p);
	m->skin=R_RegisterSkin(buff, &skinwidth, &skinheight);
	if (m->skin==-1)
	{
		I_Printf("unable to open %s(%s)\n", buff, filename);
		Z_Free(m);
		fclose(fh);
		return(NULL);
	}

	stvert=(md2st_t *)Z_Malloc(header.num_st*sizeof(md2st_t), PU_STATIC, NULL);
	fseek(fh, header.ofs_st, SEEK_SET);
	R_SafeRead(stvert, header.num_st*sizeof(md2st_t), fh);
	tris=(md2tris_t *)Z_Malloc(header.num_tris*sizeof(md2tris_t), PU_STATIC, NULL);
	R_SafeRead(tris, header.num_tris*sizeof(md2tris_t), fh);
	m->tris=(modeltris_t *)Z_Malloc(header.num_tris*3*sizeof(modeltris_t), PU_STATIC, NULL);
	for (i=0;i<header.num_tris;i++)
	{
		for (frame=0;frame<3;frame++)
		{
			m->tris[i*3+frame].index=tris[i].v[frame];
			m->tris[i*3+frame].tu=stvert[tris[i].st[frame]].s/skinwidth;
			m->tris[i*3+frame].tv=stvert[tris[i].st[frame]].t/skinheight;
		}
	}
	Z_Free(tris);
	Z_Free(stvert);

	mframe=(md2frame_t *)Z_Malloc(header.framesize, PU_STATIC, NULL);
	fseek(fh, header.ofs_frames, SEEK_SET);
	for (frame=0;frame<header.num_frames;frame++)
	{
		R_SafeRead(mframe, header.framesize, fh);
		m->frames[frame]=(modelframe_t *)Z_Malloc(sizeof(modelframe_t)+(header.num_xyz-1)*sizeof(modelvert_t), PU_STATIC, NULL);
		memcpy(m->frames[frame]->name, mframe->name, 16);
		for (i=0;i<header.num_xyz;i++)
		{
			m->frames[frame]->v[i].x=(mframe->verts[i].x*mframe->scale.x+mframe->translate.x)/scale;
			m->frames[frame]->v[i].y=(mframe->verts[i].y*mframe->scale.y+mframe->translate.y)/scale;
			m->frames[frame]->v[i].z=(mframe->verts[i].z*mframe->scale.z+mframe->translate.z)/scale+offset;
		}
	}
	Z_Free(mframe);

	m->cmds=Z_Malloc(header.num_glcmds*sizeof(int), PU_STATIC, NULL);
	R_SafeRead(m->cmds, header.num_glcmds*sizeof(int), fh);
	cmd=m->cmds;
	i=*(cmd++);
	while (i!=0)
	{
		if (i<0)
			i=-i;
		if (i>MaxModelCmds)
			MaxModelCmds=i;
		cmd+=i*3;
		i=*(cmd++);
	}

	fclose(fh);
	m->weapon=NULL;
	return(m);
}

static char *ReadString(char *buff, int len, FILE *fh)
{
	char	*p;

	sectionname[0]=0;
	while (true)
	{
		p=fgets(buff, len, fh);
		if (!p)
			return(NULL);
		if (buff[0]=='#')
			continue;
		buff[strlen(buff)-1]=0;//chop \n
		if (!buff[0])
			continue;
		p=strchr(buff, '=');
		if (p)
		{
			*(p++)=0;
			while (isspace(*p))
				p++;
			return(p);
		}
		strcpy(sectionname, buff+1);
		sectionname[strlen(sectionname)-1]=0;//chop ]
		return(NULL);
	}
}

static void ReadMD2Names(FILE *fh)
{
	char	buff[256];
	char	*p;
	int		i;

	nummd2names=0;
	p=ReadString(buff, 256, fh);
	if (!p)
	{
		I_Printf("Invalid ini file");
		return;
	}
	nummd2names=atoi(p);
	if (nummd2names==0)
		return;
	md2names=(md2name_t *)malloc(nummd2names*sizeof(md2name_t));
	for (i=0;i<nummd2names;i++)
	{
		p=ReadString(buff, 256, fh);
		if (!p)
			break;
		md2names[i].name=strdup(buff);
		md2names[i].model=NULL;
	}
	nummd2names=i;
	while (p)
		p=ReadString(buff, 256, fh);
	Skins=(skin_t *)Z_Malloc(nummd2names*sizeof(skin_t), PU_STATIC, NULL);
	NumSkins=0;
}

#define MAX_STATES	1024

typedef struct
{
	char		*name;
	model_t		*model;
	md2state_t	*state;
}md2inistate_t;

static int				numstates;
static md2inistate_t	*md2states;

static void AddModelStates(char *p, model_t *model)
{
	char	*q;

	while (*p)
	{
		while (*p&&isspace(*p))
			p++;
		q=p;
		while (*q&&!isspace(*q))
			q++;
		if (*q)
		{
			*(q++)=0;
		}
		if (numstates==MAX_STATES)
			I_Error("Too many MD2 states");
		md2states[numstates].name=strdup(p);
		md2states[numstates].model=model;
		md2states[numstates].state=NULL;
		numstates++;
		p=q;
	}
}

static void ReadSection(md2name_t *md2, FILE *fh)
{
	char	buff[256];
	char	namebuff[256];
	char	weapon[256];
	char	*name;
	char	*p;
	float	offset;
	float	scale;
	static int	count=0;

	count++;
	if ((count&15)==0)
		I_Printf(".");
	scale=1;
	offset=0;
	name=ReadString(namebuff, 256, fh);
	if (strcmp(namebuff, "file")!=0)
		I_Error("file= Expected");
	weapon[0]=0;
	p=ReadString(buff, 256, fh);
	while (*p)
	{
		if (strcmp(buff, "weapon")==0)
			strcpy(weapon, p);
		else if (strcmp(buff, "offset")==0)
			offset=(float)atof(p);
		else if (strcmp(buff, "scale")==0)
			scale=(float)atof(p)/100.0f;
		else if (strcmp(buff, "states")==0)
			break;

		p=ReadString(buff, 256, fh);
	}
	if (!*p)
		I_Error("No states in %s", md2->name);

	md2->model=R_LoadMD2(name, offset, scale);
	if (weapon[0])
		md2->model->weapon=R_LoadMD2(weapon, offset, scale);
	if (md2->model)
		AddModelStates(p, md2->model);
	while (p)
		p=ReadString(buff, 256, fh);
}

static void ReadState(FILE *fh)
{
	int				i;
	md2inistate_t	*st;
	char			*p;
	char			buff[256];
	int				tics;
	int				first;
	int				last;
	int				start;
	int				end;
	int				len;
	char			anim[16];
	model_t			*model;


	st=NULL;
	for (i=0;i<numstates;i++)
	{
		if (md2states[i].state)
			continue;
		if (strcmp(md2states[i].name, sectionname)==0)
		{
			st=&md2states[i];
			break;
		}
	}
	p=ReadString(buff, 256, fh);
	if (!st)
	{
		while (p)
			p=ReadString(buff, 256, fh);
		return;
	}
	start=-1;
	end=-1;
	tics=0;
	anim[0]=0;
	model=st->model;
	while (p)
	{
		if (stricmp(buff, "anim")==0)
		{
			strncpy(anim, p, 15);
			anim[15]=0;
		}
		else if (stricmp(buff, "StartFrame")==0)
			start=atoi(p)-1;
		else if (stricmp(buff, "EndFrame")==0)
			end=atoi(p)-1;
		else if (stricmp(buff, "nbTics")==0)
			tics=atoi(p);
		p=ReadString(buff, 256, fh);
	}
	if (!anim[0])
		return;

	first=-1;
	len=strlen(anim);
	for (i=0;i<model->numframes;i++)
	{
		p=model->frames[i]->name;
		if (strncmp(p, anim, len)==0)
		{
			if (first==-1)
			{
				first=i;
			}
			last=i;
		}
	}
	if (first==-1)
	{
		first=last=0;
	}
#if 1
	if (end!=-1)
		last=end+1;//first+end;
	//first+=start;
	if (start!=-1)
		first=start+1;
#endif
	if (tics==0)
		tics=last+1-first;
	if (tics==0)
		return;
	st->state=(md2state_t *)Z_Malloc(sizeof(md2state_t)+(tics-1)*sizeof(int), PU_STATIC, NULL);
	st->state->numframes=tics;
	st->state->model=model;
	for (i=0;i<tics;i++)
		st->state->frames[i]=first+i;
}

static void ProcessSection(FILE *fh)
{
	int		i;

	for (i=0;i<nummd2names;i++)
	{
		if (strcmp(sectionname, md2names[i].name)==0)
		{
			ReadSection(&md2names[i], fh);
			return;
		}
	}
	ReadState(fh);
}

static void MapAnimations(FILE *fh)
{
	char			buff[256];
	char			*p;
	char			*q;
	int				statenum;
	md2inistate_t	*st;
	state_t			*state;
	int				i;
	int				tics;

	for (statenum=0;statenum<NUMSTATES;statenum++)
	{
		p=ReadString(buff, 256, fh);
		if (!p)
			I_Error("Unexpected end of file (not enough states in [CORRESP])");

		q=p;
		while (*q&&!isspace(*q))
			q++;
		if (*q)
		{
			*(q++)=0;
			tics=atoi(q);
		}
		else
			tics=0;
		state=&D_API.states[statenum];
		state->md2=NULL;
		if (strcmp(p, "-")==0)
			continue;
		for (i=0;i<numstates;i++)
		{
			st=&md2states[i];
			if (!st->state)
				continue;
			if (strcmp(st->name, p)==0)
			{
				state->md2=st->state;
				if (tics)
					state->tics=tics;
				break;
			}
		}
		if (i==numstates)
			I_Printf("Undefined State(%s)\n", p);
	}
}

void R_InitPlayerSkins(void)
{
	int		i;

	for (i=0;i<MAXPLAYERS;i++)
	{
		if (PlayerSkins[i].name&&PlayerSkins[i].name[0])
		{
			PlayerSkins[i].skin=R_RegisterSkin(PlayerSkins[i].name, NULL, NULL);
			if (PlayerSkins[i].skin==-1)
				I_Printf("Unable to open %s\n", PlayerSkins[i].name);
		}
	}
}

void R_InitMD2(char *name)
{
	FILE		*fh;
	char		buff[256];
	int			i;

	MaxModelCmds=0;
	if (!*name)
		return;
	fh=fopen(name, "rt");
	if (!fh)
		return;
	I_Printf("\nR_InitMD2: Parsing %s.", name);
	md2states=(md2inistate_t *)malloc(MAX_STATES*sizeof(md2inistate_t));
	numstates=0;
	ReadString(buff, 256, fh);
	if (strcmp(sectionname, "MD2")!=0)
		I_Error("Expected [MD2]");
	ReadMD2Names(fh);
	I_Printf(".");
	while (strcmp(sectionname, "CORRESP")!=0)
	{
		if (!sectionname[0])
			I_Error("Unexpected end of file");
		ProcessSection(fh);
	}
	I_Printf(".");
	MapAnimations(fh);
	fclose(fh);

	for (i=0;i<nummd2names;i++)
	{
		free(md2names[i].name);
	}
	free(md2names);
	for (i=0;i<numstates;i++)
	{
		free(md2states[i].name);
	}
	free(md2states);
	ModelVertexBuffer=(DOOM3DVERTEX *)malloc(MaxModelCmds*sizeof(DOOM3DVERTEX));
	R_InitPlayerSkins();
}
