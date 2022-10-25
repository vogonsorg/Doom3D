#include "d3dr_local.h"
#include "c_d3d.h"
#include "d3di_video.h"

#define MAX_SPRITES	1024

spritedef_t		*spriteinfo;
int				numsprites;

spriteframe_t	sprtemp[29];
int				maxframe;
char*			spritename;

static mobj_t	**SpriteList=NULL;
static mobj_t	**NextSprite=NULL;

//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
void
R_InstallSpriteLump
( int		lump,
 unsigned	frame,
 unsigned	rotation,
 dboolean	flipped )
{
    int		r;

    if (frame >= 29 || rotation > 8)
		I_Error("R_InstallSpriteLump: "
		"Bad frame characters in lump %i", lump);

    if ((int)frame > maxframe)
		maxframe = frame;

    if (rotation == 0)
    {
		// the lump should be used for all rotations
		if ((sprtemp[frame].rotate == false)&&!*D_API.pFixSprites)
			I_Error ("R_InitSprites: Sprite %s frame %c has "
			"multiple rot=0 lump", spritename, 'A'+frame);

		if (sprtemp[frame].rotate == true)
			I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
			"and a rot=0 lump", spritename, 'A'+frame);

		sprtemp[frame].rotate = false;
		for (r=0 ; r<8 ; r++)
		{
			sprtemp[frame].lump[r] = lump - firstspritelump;
			sprtemp[frame].flip[r] = (byte)flipped;
		}
		return;
    }

    // the lump is only used for one rotation
    if (sprtemp[frame].rotate == false)
		I_Error ("R_InitSprites: Sprite %s frame %c has rotations "
		"and a rot=0 lump", spritename, 'A'+frame);

    sprtemp[frame].rotate = true;

    // make 0 based
    rotation--;
    if ((sprtemp[frame].lump[rotation] != -1)&&!*D_API.pFixSprites)
		I_Error ("R_InitSprites: Sprite %s : %c : %c "
		"has two lumps mapped to it",
		spritename, 'A'+frame, '1'+rotation);

    sprtemp[frame].lump[rotation] = lump - firstspritelump;
    sprtemp[frame].flip[rotation] = (byte)flipped;
}


//
// R_InitSprites
// Pass a null terminated list of sprite names
//  (4 chars exactly) to be used.
// Builds the sprite rotation matrixes to account
//  for horizontally flipped sprites.
// Will report an error if the lumps are inconsistant.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//  letter/number appended.
// The rotation character can be 0 to signify no rotations.
//
void R_InitSprites (char** namelist)
{
    char**	check;
    int		i;
    int		l;
    int		intname;
    int		frame;
    int		rotation;
    int		start;
    int		end;
    int		patched;

	if (!SpriteList)
		SpriteList=(mobj_t **)Z_Malloc(MAX_SPRITES*sizeof(mobj_t *), PU_STATIC, NULL);
    // count the number of sprite names
    check = namelist;
    while (*check != NULL)
		check++;

    numsprites = check-namelist;

    if (!numsprites)
		return;

    spriteinfo = Z_Malloc(numsprites *sizeof(*spriteinfo), PU_STATIC, NULL);

    start = firstspritelump-1;
    end = lastspritelump+1;

    // scan all the lump names for each of the names,
    //  noting the highest frame letter.
    // Just compare 4 characters as ints
    for (i=0 ; i<numsprites ; i++)
    {
		spritename = namelist[i];
		memset (sprtemp,-1, sizeof(sprtemp));

		maxframe = -1;
		intname = *(int *)namelist[i];

		// scan the lumps,
		//  filling in the frames for whatever is found
		for (l=start+1 ; l<end ; l++)
		{
			if (*(int *)lumpinfo[l].name == intname)
			{
				frame = lumpinfo[l].name[4] - 'A';
				rotation = lumpinfo[l].name[5] - '0';

				if (*D_API.modifiedgame)
					patched = W_GetNumForName (lumpinfo[l].name);
				else
					patched = l;

				R_InstallSpriteLump (patched, frame, rotation, false);

				if (lumpinfo[l].name[6])
				{
					frame = lumpinfo[l].name[6] - 'A';
					rotation = lumpinfo[l].name[7] - '0';
					R_InstallSpriteLump (l, frame, rotation, true);
				}
			}
		}

		// check the frames that were found for completeness
		if (maxframe == -1)
		{
			spriteinfo[i].numframes = 0;
			continue;
		}

		maxframe++;

		for (frame = 0 ; frame < maxframe ; frame++)
		{
			switch ((int)sprtemp[frame].rotate)
			{
			case -1:
				// no rotations were found for that frame at all
				I_Error ("R_InitSprites: No patches found "
					"for %s frame %c", namelist[i], frame+'A');
				break;

			case 0:
				// only the first rotation is needed
				break;

			case 1:
				// must have all 8 frames
				for (rotation=0 ; rotation<8 ; rotation++)
					if (sprtemp[frame].lump[rotation] == -1)
						I_Error ("R_InitSprites: Sprite %s frame %c "
						"is missing rotations",
						namelist[i], frame+'A');
					break;
			}
		}

		// allocate space for the frames present and copy sprtemp to it
		spriteinfo[i].numframes = maxframe;
		spriteinfo[i].spriteframes =
			Z_Malloc (maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
		memcpy (spriteinfo[i].spriteframes, sprtemp, maxframe*sizeof(spriteframe_t));
    }

}

void R_AddSprites(sector_t *sec)
{
    mobj_t*		thing;
    //int			lightnum;

/*
    lightnum = (sec->lightlevel >> LIGHTSEGSHIFT)+extralight;

    if (lightnum < 0)
		spritelights = scalelight[0];
    else if (lightnum >= LIGHTLEVELS)
		spritelights = scalelight[LIGHTLEVELS-1];
    else
		spritelights = scalelight[lightnum];
*/

    // Handle all things in sector.
    for (thing = sec->thinglist ; thing ; thing = thing->snext)
		*(NextSprite++)=thing;
	if (NextSprite-SpriteList>MAX_SPRITES)
		I_Error("Sprite overflow");
}

void R_ClearSprites(void)
{
	NextSprite=SpriteList;
}

void R_RenderSprites(void)
{
	mobj_t	**pspr;
	mobj_t	*thing;

	for (pspr=SpriteList;pspr<NextSprite;pspr++)
	{
		thing=*pspr;
		if ((thing->type==MT_PLAYER)&&(thing->player==renderplayer))
			continue;
		R_DrawSprite(thing);
	}
}
