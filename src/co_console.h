#ifndef CO_CONSOLE_H
#define CO_CONSOLE_H

#include "d_event.h"

void CO_Init(void);
void CO_AddLine(char *line, int len);
void CO_AddText(char *text);
void CO_Draw(void);
void CO_Ticker(void);
dboolean CO_Responder (event_t* ev);

extern int ConsolePos;

#endif