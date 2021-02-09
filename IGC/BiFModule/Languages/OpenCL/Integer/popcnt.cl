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


INLINE char OVERLOADABLE popcount( char x )
{
    return popcount( as_uchar( x ) );
}

INLINE short OVERLOADABLE popcount( short x )
{
    return popcount( as_ushort( x ) );
}

INLINE int OVERLOADABLE popcount( int x )
{
    return popcount( as_uint( x ) );
}

INLINE long OVERLOADABLE popcount( long x )
{
    return popcount( as_ulong( x ) );
}

INLINE ulong OVERLOADABLE popcount( ulong x )
{
    uint2 v = as_uint2( x );
    return popcount( v.x ) + popcount( v.y );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( popcount, char, char )
GENERATE_VECTOR_FUNCTIONS_1ARG( popcount, uchar, uchar )
GENERATE_VECTOR_FUNCTIONS_1ARG( popcount, short, short )
GENERATE_VECTOR_FUNCTIONS_1ARG( popcount, ushort, ushort )
GENERATE_VECTOR_FUNCTIONS_1ARG( popcount, int, int )
GENERATE_VECTOR_FUNCTIONS_1ARG( popcount, uint, uint )
GENERATE_VECTOR_FUNCTIONS_1ARG( popcount, long, long )
GENERATE_VECTOR_FUNCTIONS_1ARG( popcount, ulong, ulong )
