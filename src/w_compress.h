#ifndef W_COMPRESS_H
#define W_COMPRESS_H

#include "doomtype.h"

int W_Uncompress(byte *dest, byte *src, int size);

//returns compressed size, max. compressed size=uncompressed size*9/8
int W_Compress(byte *dest, byte *src, int size);


#endif