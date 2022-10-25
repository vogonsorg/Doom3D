#ifndef D_STUFF_H
#define D_STUFF_H

#define NUM_VID_PAGES 2
extern int	ScreenWidth;
extern int	ScreenHeight;
extern int	RedrawCount;

void I_Printf(char *msg, ...);
void D_IncValidCount(void);

#endif