/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE int OVERLOADABLE any( char x )
{
    char c = x;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( char2 x )
{
    char c = x.s0 | x.s1;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( char3 x )
{
    char c = x.s0 | x.s1 | x.s2;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( char4 x )
{
    char c = x.s0 | x.s1 | x.s2 | x.s3;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( char8 x )
{
    char c = x.s0 | x.s1 | x.s2 | x.s3 | x.s4 | x.s5 | x.s6 | x.s7;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( char16 x )
{
    char c = x.s0 | x.s1 | x.s2 | x.s3 | x.s4 | x.s5 | x.s6 | x.s7 |
             x.s8 | x.s9 | x.sa | x.sb | x.sc | x.sd | x.se | x.sf;
    return (c < 0) ? 1 : 0;
}


INLINE int OVERLOADABLE any( short x )
{
    short c = x;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( short2 x )
{
    short c = x.s0 | x.s1;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( short3 x )
{
    short c = x.s0 | x.s1 | x.s2;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( short4 x )
{
    short c = x.s0 | x.s1 | x.s2 | x.s3;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( short8 x )
{
    short c = x.s0 | x.s1 | x.s2 | x.s3 | x.s4 | x.s5 | x.s6 | x.s7;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( short16 x )
{
    short c = x.s0 | x.s1 | x.s2 | x.s3 | x.s4 | x.s5 | x.s6 | x.s7 |
              x.s8 | x.s9 | x.sa | x.sb | x.sc | x.sd | x.se | x.sf;
    return (c < 0) ? 1 : 0;
}


INLINE int OVERLOADABLE any( int x )
{
    int c = x;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( int2 x )
{
    int c = x.s0 | x.s1;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( int3 x )
{
    int c = x.s0 | x.s1 | x.s2;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( int4 x )
{
    int c = x.s0 | x.s1 | x.s2 | x.s3;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( int8 x )
{
    int c = x.s0 | x.s1 | x.s2 | x.s3 | x.s4 | x.s5 | x.s6 | x.s7;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( int16 x )
{
    int c = x.s0 | x.s1 | x.s2 | x.s3 | x.s4 | x.s5 | x.s6 | x.s7 |
            x.s8 | x.s9 | x.sa | x.sb | x.sc | x.sd | x.se | x.sf;
    return (c < 0) ? 1 : 0;
}


INLINE int OVERLOADABLE any( long x )
{
    long c = x;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( long2 x )
{
    long c = x.s0 | x.s1;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( long3 x )
{
    long c = x.s0 | x.s1 | x.s2;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( long4 x )
{
    long c = x.s0 | x.s1 | x.s2 | x.s3;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( long8 x )
{
    long c = x.s0 | x.s1 | x.s2 | x.s3 | x.s4 | x.s5 | x.s6 | x.s7;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE any( long16 x )
{
    long c = x.s0 | x.s1 | x.s2 | x.s3 | x.s4 | x.s5 | x.s6 | x.s7 |
             x.s8 | x.s9 | x.sa | x.sb | x.sc | x.sd | x.se | x.sf;
    return (c < 0) ? 1 : 0;
}
