//m_sidetest.c
//tests vhich side of a line a point is on.
#include "doomtype.h"
#include "m_fixed.h"

dboolean M_OnSide(fixed_t dx, fixed_t dy, fixed_t x, fixed_t y)
{
    fixed_t     left;
    fixed_t     right;

    if (!dx)
    {
        if (x <= 0)
            return dy > 0;

        return dy < 0;
    }
    if (!dy)
    {
        if (y <= 0)
            return dx < 0;

        return dx > 0;
    }

    // Try to quickly decide by looking at sign bits.
    if ( (dy ^ dx ^ x ^ y)&0x80000000 )
    {
        if  ( (dy ^ x) & 0x80000000 )
        {
            // (left is negative)
            return 1;
        }
        return 0;
    }

    left = FixedMul ( F2INT(dy), x );
    right = FixedMul ( y , F2INT(dx));

    if (right < left)
    {
        // front side
        return 0;
    }
    // back side
    return 1;
}

