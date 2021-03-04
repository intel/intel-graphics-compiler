/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

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
