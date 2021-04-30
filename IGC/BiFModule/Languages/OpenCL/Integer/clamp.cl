/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE char OVERLOADABLE clamp( char x, char minval, char maxval )
{
    return min(max(x, minval), maxval);
}

INLINE uchar OVERLOADABLE clamp( uchar x, uchar minval, uchar maxval )
{
    return min(max(x, minval), maxval);
}

INLINE short OVERLOADABLE clamp( short x, short minval, short maxval )
{
    return min(max(x, minval), maxval);
}

INLINE ushort OVERLOADABLE clamp( ushort x, ushort minval, ushort maxval )
{
    return min(max(x, minval), maxval);
}

INLINE int OVERLOADABLE clamp( int x, int minval, int maxval )
{
    return min(max(x, minval), maxval);
}

INLINE uint OVERLOADABLE clamp( uint x, uint minval, uint maxval )
{
    return min(max(x, minval), maxval);
}

INLINE long OVERLOADABLE clamp( long x, long minval, long maxval )
{
    return min(max(x, minval), maxval);
}

INLINE ulong OVERLOADABLE clamp( ulong x, ulong minval, ulong maxval )
{
    return min(max(x, minval), maxval);
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, char, char )
GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, uchar, uchar )
GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, short, short )
GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, ushort, ushort )
GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, int, int )
GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, uint, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, long, long )
GENERATE_VECTOR_FUNCTIONS_3ARGS( clamp, ulong, ulong )

GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, char, char, char )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, uchar, uchar, uchar )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, short, short, short )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, ushort, ushort, ushort )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, int, int, int )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, uint, uint, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, long, long, long )
GENERATE_VECTOR_FUNCTIONS_3ARGS_VSS( clamp, ulong, ulong, ulong )
