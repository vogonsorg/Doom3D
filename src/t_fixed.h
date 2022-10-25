#ifndef T_FIXED_H
#define T_FIXED_H

//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS		16
#define FRACUNIT		(1<<FRACBITS)

#define INT2F(x)		((x)<<FRACBITS)
#define F2INT(x)		((x)>>FRACBITS)
#define F2D3D(x) 		(((float)(x))/FRACUNIT)

typedef int fixed_t;

#endif