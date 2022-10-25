// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//      Do all the WAD I/O, get map description,
//      set up initial state and misc. LUTs.
//
//-----------------------------------------------------------------------------
#ifdef RCSID
static const char
rcsid[] = "$Id: p_setup.c,v 1.5 1997/02/03 22:45:12 b1 Exp $";
#endif


#include <math.h>

#include "z_zone.h"

#include "m_swap.h"
#include "m_bbox.h"
#include "m_fixed.h"

#include "g_game.h"

#include "i_system.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "s_sound.h"

#include "doomstat.h"

#include "t_bsp.h"

#include "c_interface.h"

#include "l_precalc.h"
#include "m_argv.h"

#include "info.h"

void    P_SpawnMapThing (mapthing_t*    mthing);

//
// MAP related Lookup tables.
// Store VERTEXES, LINEDEFS, SIDEDEFS, etc.
//
int             numvertexes;
int				firstglvert;
vertex_t*       vertexes;

int             numsegs;
seg_t*          segs;

int             numsectors;
sector_t*       sectors;

int             numsubsectors;
subsector_t*    subsectors;

int             numnodes;
node_t*         nodes;

int             numlines;
line_t*         lines;

int             numsides;
side_t*         sides;

int				LevelChecksum;

// BLOCKMAP
// Created from axis aligned bounding box
// of the map, a rectangular array of
// blocks of size ...
// Used to speed up collision detection
// by spatial subdivision in 2D.
//
// Blockmap size.
int             bmapwidth;
int             bmapheight;     // size in mapblocks
short*          blockmap;       // int for larger maps
// offsets in blockmap are from here
short*          blockmaplump;
// origin of block map
fixed_t         bmaporgx;
fixed_t         bmaporgy;
// for thing chains
mobj_t**        blocklinks;


// REJECT
// For fast sight rejection.
// Speeds up enemy AI by skipping detailed
//  LineOf Sight calculation.
// Without special effect, this could be
//  used as a PVS lookup as well.
//
byte*           rejectmatrix;


// Maintain single and multi player starting spots.
#define MAX_DEATHMATCH_STARTS   10

mapthing_t      deathmatchstarts[MAX_DEATHMATCH_STARTS];
mapthing_t*     deathmatch_p;
mapthing_t      playerstarts[MAXPLAYERS];





//
// P_LoadVertexes
//
void P_LoadVertexes (int lump, int gllump)
{
	byte*               data;
	int                 i;
	int					glverts;
	mapvertex_t*        ml;
	vertex_t*           li;
	byte				*gldata;
	mapglvert2_t		*mgl;

	// Determine number of lumps:
	//  total lump length / vertex record length.
	if (gllump!=-1)
	{
		gldata=W_CacheLumpNum(gllump, PU_STATIC);
		if (*(int *)gldata==0x32644E67) //"gNd2"
		{
			glverts=(W_LumpLength(gllump)-4)/sizeof(mapglvert2_t);
			mgl=(mapglvert2_t *)(gldata+4);
		}
		else
		{
			glverts=W_LumpLength(gllump)/sizeof(mapvertex_t);
			mgl=NULL;
		}
	}
	else
	{
		glverts=0;
		gldata=NULL;
	}


	firstglvert=W_LumpLength (lump) / sizeof(mapvertex_t);
	*C_API.numvertexes=numvertexes=numvertexes = firstglvert+glverts;

	// Allocate zone memory for buffer.
	*C_API.pvertexes=vertexes = Z_Malloc (numvertexes*sizeof(vertex_t),PU_LEVEL,0);

	// Load data into cache.
	data = W_CacheLumpNum (lump,PU_STATIC);

	ml = (mapvertex_t *)data;
	li = vertexes;

	// Copy and convert vertex coordinates,
	// internal representation as fixed.
	for (i=0 ; i<numvertexes-glverts ; i++, li++, ml++)
	{
		li->x = INT2F(SHORT(ml->x));
		li->y = INT2F(SHORT(ml->y));
	}

	if (glverts)
	{
		if (mgl)
		{
			for (i=0 ; i<glverts ; i++, li++, mgl++)
			{
				li->x = LONG(mgl->x);
				li->y = LONG(mgl->y);
			}
		}
		else
		{
			ml = (mapvertex_t *)gldata;
			for (i=0 ; i<glverts ; i++, li++, ml++)
			{
				li->x = INT2F(SHORT(ml->x));
				li->y = INT2F(SHORT(ml->y));
			}
		}
	}
	// Free buffer memory.
	if (gldata)
		Z_Free(gldata);
	Z_Free (data);
}


//
// P_LoadSegs
//
void P_LoadSegs (int lump)
{
	byte*               data;
	int                 i;
	mapseg_t*           ml;
	seg_t*              li;
	line_t*             ldef;
	int                 linedef;
	int                 side;

	*C_API.numsegs=numsegs = W_LumpLength (lump) / sizeof(mapseg_t);
	*C_API.psegs=segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);
	memset (segs, 0, numsegs*sizeof(seg_t));
	data = W_CacheLumpNum (lump,PU_STATIC);

	ml = (mapseg_t *)data;
	li = segs;
	for (i=0 ; i<numsegs ; i++, li++, ml++)
	{
		li->v1 = &vertexes[SHORT(ml->v1)];
		li->v2 = &vertexes[SHORT(ml->v2)];

		li->angle = INT2F(SHORT(ml->angle));
		li->offset = INT2F(SHORT(ml->offset));
		linedef = SHORT(ml->linedef);
		ldef = &lines[linedef];
		li->linedef = ldef;
		side = SHORT(ml->side);
		li->sidedef = &sides[ldef->sidenum[side]];
		li->frontsector = sides[ldef->sidenum[side]].sector;
		if (ldef-> flags & ML_TWOSIDED)
			li->backsector = sides[ldef->sidenum[side^1]].sector;
		else
			li->backsector = 0;
		li->partner=NULL;
	}

	Z_Free (data);
}

static vertex_t *GetGLVertex(int num)
{
	if (num&0x8000)
		num=(num&0x7FFF)+firstglvert;
	return(&vertexes[num]);
}

static fixed_t GetOffset(vertex_t *v1, vertex_t *v2)
{
	float	a;
	float	b;

	a=F2D3D(v1->x-v2->x);
	b=F2D3D(v1->y-v2->y);
	a=(float)sqrt(a*a+b*b);
	return((fixed_t)(a*FRACUNIT));
}

static subsector_t *FindSegSubsector(int seg)
{
	subsector_t		*ss;

	for (ss=subsectors;ss<&subsectors[numsubsectors];ss++)
	{
		if ((ss->firstline<=seg)&&(ss->firstline+ss->numlines>seg))
			return(ss);
	}
	return(NULL);
}

//
// P_LoadGLSegs
//
void P_LoadGLSegs (int lump)
{
	byte*               data;
	int                 i;
	mapglseg_t*         ml;
	seg_t*              li;
	line_t*             ldef;
	int                 linedef;
	int                 side;

	*C_API.numsegs=numsegs = W_LumpLength (lump) / sizeof(mapglseg_t);
	*C_API.psegs=segs = Z_Malloc (numsegs*sizeof(seg_t),PU_LEVEL,0);
	memset (segs, 0, numsegs*sizeof(seg_t));
	data = W_CacheLumpNum (lump,PU_STATIC);

	ml = (mapglseg_t *)data;
	li = segs;
	for (i=0 ; i<numsegs ; i++, li++, ml++)
	{
		li->v1 = GetGLVertex(SHORT(ml->v1));
		li->v2 = GetGLVertex(SHORT(ml->v2));

		li->angle=0;//not needed with d3d renderer

		linedef = SHORT(ml->linedef);
		if (linedef==-1)
		{
			li->linedef=NULL;
			li->sidedef=NULL;
			li->frontsector=NULL;
			li->backsector=NULL;
		}
		else
		{
			ldef = &lines[linedef];
			li->linedef = ldef;
			side = SHORT(ml->side);
			if (side)
				li->offset=GetOffset(li->v1, ldef->v2);
			else
				li->offset=GetOffset(li->v1, ldef->v1);
			li->sidedef = &sides[ldef->sidenum[side]];
			li->frontsector = sides[ldef->sidenum[side]].sector;
			if (ldef-> flags & ML_TWOSIDED)
				li->backsector = sides[ldef->sidenum[side^1]].sector;
			else
				li->backsector = 0;
		}
		if (ml->partner==-1)
			li->partner=NULL;
		else
		{
			li->partner=FindSegSubsector(SHORT(ml->partner));
			if (!li->partner)
				I_Printf("P_LoadGLSegs:partner seg subsector not found\n");
		}
	}

	Z_Free (data);
}


//
// P_LoadSubsectors
//
void P_LoadSubsectors (int lump)
{
	byte*               data;
	int                 i;
	mapsubsector_t*     ms;
	subsector_t*        ss;

	*C_API.numsubsectors=numsubsectors = W_LumpLength (lump) / sizeof(mapsubsector_t);
	*C_API.psubsectors=subsectors = Z_Malloc (numsubsectors*sizeof(subsector_t),PU_LEVEL,0);
	data = W_CacheLumpNum (lump,PU_STATIC);

	ms = (mapsubsector_t *)data;
	memset (subsectors,0, numsubsectors*sizeof(subsector_t));
	ss = subsectors;

	for (i=0 ; i<numsubsectors ; i++, ss++, ms++)
	{
		ss->numlines = SHORT(ms->numsegs);
		ss->firstline = SHORT(ms->firstseg);
	}

	Z_Free (data);
}



//
// P_LoadSectors
//
void P_LoadSectors (int lump)
{
	byte*               data;
	int                 i;
	mapsector_t*        ms;
	sector_t*           ss;

	*C_API.numsectors=numsectors = W_LumpLength (lump) / sizeof(mapsector_t);
	*C_API.psectors=sectors = Z_Malloc (numsectors*sizeof(sector_t),PU_LEVEL,0);
	memset (sectors, 0, numsectors*sizeof(sector_t));
	data = W_CacheLumpNum (lump,PU_STATIC);

	ms = (mapsector_t *)data;
	ss = sectors;
	for (i=0 ; i<numsectors ; i++, ss++, ms++)
	{
		ss->floorheight = INT2F(SHORT(ms->floorheight));
		ss->ceilingheight = INT2F(SHORT(ms->ceilingheight));
		ss->floorpic = C_API.R_FlatNumForName(ms->floorpic);
		ss->ceilingpic = C_API.R_FlatNumForName(ms->ceilingpic);
		ss->lightlevel = SHORT(ms->lightlevel);
		ss->special = SHORT(ms->special);
		ss->tag = SHORT(ms->tag);
		ss->thinglist = NULL;
	}

	Z_Free (data);
}


//
// P_LoadNodes
//
void P_LoadNodes (int lump)
{
	byte*       data;
	int         i;
	int         j;
	int         k;
	mapnode_t*  mn;
	node_t*     no;

	*C_API.numnodes=numnodes = W_LumpLength (lump) / sizeof(mapnode_t);
	*C_API.pnodes=nodes = Z_Malloc (numnodes*sizeof(node_t),PU_LEVEL,0);
	data = W_CacheLumpNum (lump,PU_STATIC);

	mn = (mapnode_t *)data;
	no = nodes;

	for (i=0 ; i<numnodes ; i++, no++, mn++)
	{
		no->x = INT2F(SHORT(mn->x));
		no->y = INT2F(SHORT(mn->y));
		no->dx = INT2F(SHORT(mn->dx));
		no->dy = INT2F(SHORT(mn->dy));
		for (j=0 ; j<2 ; j++)
		{
			no->children[j] = SHORT(mn->children[j]);
			for (k=0 ; k<4 ; k++)
				no->bbox[j][k] = INT2F(SHORT(mn->bbox[j][k]));
		}
	}

	Z_Free (data);
}


//
// P_LoadThings
//
void P_LoadThings (int lump)
{
	byte*               data;
	int                 i;
	mapthing_t*         mt;
	int                 numthings;
	dboolean            spawn;

	data = W_CacheLumpNum (lump,PU_STATIC);
	numthings = W_LumpLength (lump) / sizeof(mapthing_t);

	mt = (mapthing_t *)data;
	for (i=0 ; i<numthings ; i++, mt++)
	{
		spawn = true;

		// Do not spawn cool, new monsters if !commercial
		if ( gamemode != commercial)
		{
			switch(mt->type)
			{
			  case 68:  // Arachnotron
			  case 64:  // Archvile
			  case 88:  // Boss Brain
			  case 89:  // Boss Shooter
			  case 69:  // Hell Knight
			  case 67:  // Mancubus
			  case 71:  // Pain Elemental
			  case 65:  // Former Human Commando
			  case 66:  // Revenant
			  case 84:  // Wolf SS
				spawn = false;
				break;
			}
		}
		if (spawn == false)
			break;

		// Do spawn all other stuff.
		mt->x = SHORT(mt->x);
		mt->y = SHORT(mt->y);
		mt->angle = SHORT(mt->angle);
		mt->type = SHORT(mt->type);
		mt->options = SHORT(mt->options);

		P_SpawnMapThing (mt);
	}

	Z_Free (data);
}


//
// P_LoadLineDefs
// Also counts secret lines for intermissions.
//
void P_LoadLineDefs (int lump)
{
	byte*               data;
	int                 i;
	maplinedef_t*       mld;
	line_t*             ld;
	vertex_t*           v1;
	vertex_t*           v2;

	*C_API.numlines=numlines = W_LumpLength (lump) / sizeof(maplinedef_t);
	*C_API.plines=lines = Z_Malloc (numlines*sizeof(line_t),PU_LEVEL,0);
	memset (lines, 0, numlines*sizeof(line_t));
	data = W_CacheLumpNum (lump,PU_STATIC);

	mld = (maplinedef_t *)data;
	ld = lines;
	for (i=0 ; i<numlines ; i++, mld++, ld++)
	{
		ld->flags = SHORT(mld->flags);
		ld->special = SHORT(mld->special);
		ld->tag = SHORT(mld->tag);
		v1 = ld->v1 = &vertexes[SHORT(mld->v1)];
		v2 = ld->v2 = &vertexes[SHORT(mld->v2)];
		ld->dx = v2->x - v1->x;
		ld->dy = v2->y - v1->y;

		if (!ld->dx)
			ld->slopetype = ST_VERTICAL;
		else if (!ld->dy)
			ld->slopetype = ST_HORIZONTAL;
		else
		{
			if (FixedDiv (ld->dy , ld->dx) > 0)
				ld->slopetype = ST_POSITIVE;
			else
				ld->slopetype = ST_NEGATIVE;
		}

		if (v1->x < v2->x)
		{
			ld->bbox[BOXLEFT] = v1->x;
			ld->bbox[BOXRIGHT] = v2->x;
		}
		else
		{
			ld->bbox[BOXLEFT] = v2->x;
			ld->bbox[BOXRIGHT] = v1->x;
		}

		if (v1->y < v2->y)
		{
			ld->bbox[BOXBOTTOM] = v1->y;
			ld->bbox[BOXTOP] = v2->y;
		}
		else
		{
			ld->bbox[BOXBOTTOM] = v2->y;
			ld->bbox[BOXTOP] = v1->y;
		}

		ld->sidenum[0] = SHORT(mld->sidenum[0]);
		ld->sidenum[1] = SHORT(mld->sidenum[1]);

		if (ld->sidenum[0] != -1)
			ld->frontsector = sides[ld->sidenum[0]].sector;
		else
			ld->frontsector = 0;

		if (ld->sidenum[1] != -1)
			ld->backsector = sides[ld->sidenum[1]].sector;
		else
			ld->backsector = 0;
	}

	Z_Free (data);
}


//
// P_LoadSideDefs
//
void P_LoadSideDefs (int lump)
{
	byte*               data;
	int                 i;
	mapsidedef_t*       msd;
	side_t*             sd;

	*C_API.numsides=numsides = W_LumpLength (lump) / sizeof(mapsidedef_t);
	*C_API.psides=sides = Z_Malloc (numsides*sizeof(side_t),PU_LEVEL,0);
	memset (sides, 0, numsides*sizeof(side_t));
	data = W_CacheLumpNum (lump,PU_STATIC);

	msd = (mapsidedef_t *)data;
	sd = sides;
	for (i=0 ; i<numsides ; i++, msd++, sd++)
	{
		sd->textureoffset = INT2F(SHORT(msd->textureoffset));
		sd->rowoffset = INT2F(SHORT(msd->rowoffset));
		sd->toptexture = C_API.R_TextureNumForName(msd->toptexture);
		sd->bottomtexture = C_API.R_TextureNumForName(msd->bottomtexture);
		sd->midtexture = C_API.R_TextureNumForName(msd->midtexture);
		sd->sector = &sectors[SHORT(msd->sector)];
	}

	Z_Free (data);
}


//
// P_LoadBlockMap
//
void P_LoadBlockMap (int lump)
{
	int         i;
	int         count;

	blockmaplump = W_CacheLumpNum (lump,PU_LEVEL);
	blockmap = blockmaplump+4;
	count = W_LumpLength (lump)/2;

	for (i=0 ; i<count ; i++)
		blockmaplump[i] = SHORT(blockmaplump[i]);

	bmaporgx = INT2F(blockmaplump[0]);
	bmaporgy = INT2F(blockmaplump[1]);
	bmapwidth = blockmaplump[2];
	bmapheight = blockmaplump[3];

	// clear out mobj chains
	count = sizeof(*blocklinks)* bmapwidth*bmapheight;
	blocklinks = Z_Malloc (count,PU_LEVEL, 0);
	memset (blocklinks, 0, count);
}



//
// P_GroupLines
// Builds sector line lists and subsector sector numbers.
// Finds block bounding boxes for sectors.
//
void P_GroupLines (void)
{
	line_t**            linebuffer;
	int                 i;
	int                 j;
	int                 total;
	line_t*             li;
	sector_t*           sector;
	subsector_t*        ss;
	seg_t*              seg;
	fixed_t             bbox[4];
	int                 block;

	// look up sector number for each subsector
	ss = subsectors;
	for (i=0 ; i<numsubsectors ; i++, ss++)
	{
		ss->sector=NULL;
		for (j=0;j<ss->numlines;j++)
		{
			seg = &segs[ss->firstline+j];
			if (seg->sidedef)//could be a miniseg...
			{
				ss->sector = seg->sidedef->sector;
				break;
			}
		}
		if (!ss->sector)
			I_Error("blank subsector");
/*
		for (j=0;j<ss->numlines;j++)//fill in miniseg sectors
		{
			seg = &segs[ss->firstline+j];
			if (!seg->sidedef)
				seg->frontsector=seg->backsector=ss->sector;
		}
*/
	}

	// count number of lines in each sector
	li = lines;
	total = 0;
	for (i=0 ; i<numlines ; i++, li++)
	{
		total++;
		li->frontsector->linecount++;

		if (li->backsector && li->backsector != li->frontsector)
		{
			li->backsector->linecount++;
			total++;
		}
	}

	// build line tables for each sector
	linebuffer = Z_Malloc (total*4, PU_LEVEL, 0);
	sector = sectors;
	for (i=0 ; i<numsectors ; i++, sector++)
	{
		M_ClearBox (bbox);
		sector->lines = linebuffer;
		li = lines;
		for (j=0 ; j<numlines ; j++, li++)
		{
			if (li->frontsector == sector || li->backsector == sector)
			{
				*linebuffer++ = li;
				M_AddToBox (bbox, li->v1->x, li->v1->y);
				M_AddToBox (bbox, li->v2->x, li->v2->y);
			}
		}
		if (linebuffer - sector->lines != sector->linecount)
			I_Error ("P_GroupLines: miscounted");

		// set the degenmobj_t to the middle of the bounding box
		sector->soundorg.x = (bbox[BOXRIGHT]+bbox[BOXLEFT])/2;
		sector->soundorg.y = (bbox[BOXTOP]+bbox[BOXBOTTOM])/2;

		// adjust bounding box to map blocks
		block = (bbox[BOXTOP]-bmaporgy+MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block >= bmapheight ? bmapheight-1 : block;
		sector->blockbox[BOXTOP]=block;

		block = (bbox[BOXBOTTOM]-bmaporgy-MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXBOTTOM]=block;

		block = (bbox[BOXRIGHT]-bmaporgx+MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block >= bmapwidth ? bmapwidth-1 : block;
		sector->blockbox[BOXRIGHT]=block;

		block = (bbox[BOXLEFT]-bmaporgx-MAXRADIUS)>>MAPBLOCKSHIFT;
		block = block < 0 ? 0 : block;
		sector->blockbox[BOXLEFT]=block;
	}

}

void P_SetupPVS(int episode, int map, int maplump)
{
	int			level;
	dboolean	loaded;

	if (M_CheckParm("-genpvs"))
	{
		L_CalcPVS();
		return;
	}

	if (gamemode==commercial)
		level=map;
	else
		level=map+episode*10;

	loaded=L_LoadPVS(level, maplump);
	if (!loaded)
		L_ClearPVS();
}

void P_CalcLevelChecksum(void)
{
	vertex_t	*v;

	LevelChecksum=0;
	for (v=vertexes;v<&vertexes[numvertexes];v++)
		LevelChecksum+=v->x+v->y;
}

//
// P_SetupLevel
//
void
P_SetupLevel
( int           episode,
  int           map,
  int           playermask,
  skill_t       skill)
{
	int         i;
	char        lumpname[9];
	char		gllumpname[9];
	int         lumpnum;
	int			gllumpnum;

	totalkills = totalitems = totalsecret = wminfo.maxfrags = 0;
	wminfo.partime = 180;
	for (i=0 ; i<MAXPLAYERS ; i++)
	{
		players[i].killcount = players[i].secretcount
			= players[i].itemcount = 0;
	}

	// Initial height of PointOfView
	// will be set by player think.
	for (i=0;i<NumSplits;i++)
		players[consoleplayer+i].viewz = 1;

	// Make sure all sounds are stopped before Z_FreeTags.
	S_Start ();


#if 0 // UNUSED
	if (debugfile)
	{
		Z_FreeTags (PU_LEVEL, MAXINT);
		Z_FileDumpHeap (debugfile);
	}
	else
#endif
		Z_FreeTags (PU_LEVEL, PU_PURGELEVEL-1);


	// UNUSED W_Profile ();
	P_InitThinkers ();

	// if working with a devlopment map, reload it
	W_Reload ();

	// find map name
	if ( gamemode == commercial)
	{
		if (map<10)
			sprintf (lumpname,"map0%i", map);
		else
			sprintf (lumpname,"map%i", map);
	}
	else
	{
		lumpname[0] = 'E';
		lumpname[1] = '0' + episode;
		lumpname[2] = 'M';
		lumpname[3] = '0' + map;
		lumpname[4] = 0;
	}

	lumpnum = W_GetNumForName (lumpname);

	if (AllowGLNodes)
	{
		sprintf(gllumpname, "GL_%s", lumpname);
		gllumpnum=W_CheckNumForName(gllumpname);
		if (gllumpnum<lumpnum)
			gllumpnum=-1;
	}
	else
		gllumpnum=-1;

	leveltime = 0;

	// note: most of this ordering is important
	P_LoadBlockMap (lumpnum+ML_BLOCKMAP);
	P_LoadVertexes (lumpnum+ML_VERTEXES, (gllumpnum==-1)?-1:(gllumpnum+MGL_VERTS));
	P_LoadSectors (lumpnum+ML_SECTORS);
	P_LoadSideDefs (lumpnum+ML_SIDEDEFS);
	P_LoadLineDefs (lumpnum+ML_LINEDEFS);
	if (gllumpnum==-1)
		P_LoadSubsectors (lumpnum+ML_SSECTORS);
	else
		P_LoadSubsectors (gllumpnum+MGL_SSECTORS);
	if (gllumpnum==-1)
		P_LoadNodes (lumpnum+ML_NODES);
	else
		P_LoadNodes (gllumpnum+MGL_NODES);
	if (gllumpnum==-1)
		P_LoadSegs (lumpnum+ML_SEGS);
	else
		P_LoadGLSegs(gllumpnum+MGL_SEGS);

	rejectmatrix = W_CacheLumpNum (lumpnum+ML_REJECT,PU_LEVEL);
	P_GroupLines ();

	bodyqueslot = 0;
	deathmatch_p = deathmatchstarts;
	P_LoadThings (lumpnum+ML_THINGS);

	// if deathmatch, randomly spawn the active players
	if (deathmatch)
	{
		for (i=0 ; i<MAXPLAYERS ; i++)
			if (playeringame[i])
			{
				players[i].mo = NULL;
				G_DeathMatchSpawnPlayer (i);
			}

	}

	// clear special respawning que
	iquehead = iquetail = 0;

	// set up world state
	P_SpawnSpecials ();

	// build subsector connect matrix
	//  UNUSED P_ConnectSubsectors ();

	P_CalcLevelChecksum();
	if ((gllumpnum!=-1)&&UsePVS)
		P_SetupPVS(episode, map, lumpnum);
	else
		L_ClearPVS();

	// preload graphics
	if (precache&&!demoplayback)
		C_API.R_PrecacheLevel ();
	else
		C_API.R_PrecacheQuick();

	C_API.R_SetupLevel(gllumpnum!=-1);
	//I_Printf ("free memory: 0x%x\n", Z_FreeMemory());
}



//
// P_Init
//
void P_Init (void)
{
	P_InitSwitchList ();
	P_InitPicAnims ();
	C_API.R_InitSprites (sprnames);
}
