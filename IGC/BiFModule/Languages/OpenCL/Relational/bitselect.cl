/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"


INLINE
char OVERLOADABLE bitselect( char a,
                             char b,
                             char c )
{
    char temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, char, char )

INLINE
uchar OVERLOADABLE bitselect( uchar a,
                              uchar b,
                              uchar c )
{
    uchar temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, uchar, uchar )

INLINE
short OVERLOADABLE bitselect( short a,
                              short b,
                              short c )
{
    short temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, short, short )

INLINE
ushort OVERLOADABLE bitselect( ushort a,
                               ushort b,
                               ushort c )
{
    ushort temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, ushort, ushort )

INLINE
int OVERLOADABLE bitselect( int a,
                            int b,
                            int c )
{
    int temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, int, int )

INLINE
uint OVERLOADABLE bitselect( uint a,
                             uint b,
                             uint c )
{
    uint temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, uint, uint )

INLINE
long OVERLOADABLE bitselect( long a,
                             long b,
                             long c )
{
    long temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, long, long )

INLINE
ulong OVERLOADABLE bitselect( ulong a,
                              ulong b,
                              ulong c )
{
    ulong temp;
    temp = (c & b) | (~c & a);
    return temp;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, ulong, ulong )

INLINE
float OVERLOADABLE bitselect( float a,
                              float b,
                              float c )
{
    return as_float( bitselect(as_int(a), as_int(b), as_int(c)) );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, float, float )

#if defined(cl_khr_fp64)

INLINE
double OVERLOADABLE bitselect( double a,
                               double b,
                               double c )
{
    return as_double( bitselect(as_long(a), as_long(b), as_long(c)) );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, double, double )

#endif // defined(cl_khr_fp64)

#ifdef cl_khr_fp16

INLINE
half OVERLOADABLE bitselect( half a,
                             half b,
                             half c )
{
    return as_half( bitselect(as_short(a), as_short(b), as_short(c)) );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( bitselect, half, half )

#endif
