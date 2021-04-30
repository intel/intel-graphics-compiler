/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

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
