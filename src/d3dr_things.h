#ifndef D3DR_THINGS_H
#define D3DR_THINGS_H

void R_InitSprites (char** namelist);
void R_AddSprites(sector_t *sec);
void R_RenderSprites(void);
void R_ClearSprites(void);

extern spritedef_t	*spriteinfo;
extern int			numsprites;


#endif