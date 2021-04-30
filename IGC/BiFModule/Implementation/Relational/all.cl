/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"

INLINE int OVERLOADABLE all( char x )
{
    char c = x;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( char2 x )
{
    char c = x.s0 & x.s1;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( char3 x )
{
    char c = x.s0 & x.s1 & x.s2;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( char4 x )
{
    char c = x.s0 & x.s1 & x.s2 & x.s3;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( char8 x )
{
    char c = x.s0 & x.s1 & x.s2 & x.s3 & x.s4 & x.s5 & x.s6 & x.s7;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( char16 x )
{
    char c = x.s0 & x.s1 & x.s2 & x.s3 & x.s4 & x.s5 & x.s6 & x.s7 &
             x.s8 & x.s9 & x.sa & x.sb & x.sc & x.sd & x.se & x.sf;
    return (c < 0) ? 1 : 0;
}


INLINE int OVERLOADABLE all( short x )
{
    short c = x;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( short2 x )
{
    short c = x.s0 & x.s1;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( short3 x )
{
    short c = x.s0 & x.s1 & x.s2;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( short4 x )
{
    short c = x.s0 & x.s1 & x.s2 & x.s3;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( short8 x )
{
    short c = x.s0 & x.s1 & x.s2 & x.s3 & x.s4 & x.s5 & x.s6 & x.s7;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( short16 x )
{
    short c = x.s0 & x.s1 & x.s2 & x.s3 & x.s4 & x.s5 & x.s6 & x.s7 &
              x.s8 & x.s9 & x.sa & x.sb & x.sc & x.sd & x.se & x.sf;
    return (c < 0) ? 1 : 0;
}


INLINE int OVERLOADABLE all( int x )
{
    int c = x;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( int2 x )
{
    int c = x.s0 & x.s1;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( int3 x )
{
    int c = x.s0 & x.s1 & x.s2;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( int4 x )
{
    int c = x.s0 & x.s1 & x.s2 & x.s3;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( int8 x )
{
    int c = x.s0 & x.s1 & x.s2 & x.s3 & x.s4 & x.s5 & x.s6 & x.s7;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( int16 x )
{
    int c = x.s0 & x.s1 & x.s2 & x.s3 & x.s4 & x.s5 & x.s6 & x.s7 &
            x.s8 & x.s9 & x.sa & x.sb & x.sc & x.sd & x.se & x.sf;
    return (c < 0) ? 1 : 0;
}


INLINE int OVERLOADABLE all( long x )
{
    long c = x;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( long2 x )
{
    long c = x.s0 & x.s1;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( long3 x )
{
    long c = x.s0 & x.s1 & x.s2;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( long4 x )
{
    long c = x.s0 & x.s1 & x.s2 & x.s3;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( long8 x )
{
    long c = x.s0 & x.s1 & x.s2 & x.s3 & x.s4 & x.s5 & x.s6 & x.s7;
    return (c < 0) ? 1 : 0;
}

INLINE int OVERLOADABLE all( long16 x )
{
    long c = x.s0 & x.s1 & x.s2 & x.s3 & x.s4 & x.s5 & x.s6 & x.s7 &
             x.s8 & x.s9 & x.sa & x.sb & x.sc & x.sd & x.se & x.sf;
    return (c < 0) ? 1 : 0;
}
