/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __TWOPOWN_CL__
#define __TWOPOWN_CL__

/******************************************************************
Function: twopown

Description: This function calculates 2^n for integer values of n.

******************************************************************/
float twopown( int n )
{
    float result;
    int man, exp;

    if( n > 126 )
    {
        exp = 255;
        man = 0;
    }
    else if( n > -127 )
    {
        exp = n + 127;
        man = 0;
    }
    else if( n > -149 )
    {
        exp = 0;
        man = 0x00400000 >> abs( n + 127 );
    }
    else
    {
        exp = 0;
        man = 0;
    }

    return as_float( (exp << FLOAT_MANTISSA_BITS ) | man );
}


#endif
