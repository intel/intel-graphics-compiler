/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __BIF_64BIT_DIV_EMULATION__
#define __BIF_64BIT_DIV_EMULATION__

static INLINE ulong OVERLOADABLE __builtin_IB_emulate_udiv( ulong N, ulong D, private ulong* Rem )
{
    ulong Q = 0;
    ulong R = 0;
    int I = 63;

    while( I != -1 )
    {
        R = R << 1;

        R = (R & ~1UL) | ( ( N >> I ) & 1UL );

        if( R >= D )
        {
            R = R - D;
            Q |= ( 1UL << I );
        }

        I = I - 1;
    }

    if (Rem)
        *Rem = R;

    return Q;
}

INLINE ulong OVERLOADABLE __builtin_IB_udiv( ulong N, ulong D )
{
    return __builtin_IB_emulate_udiv(N, D, 0);
}

// vector versions so we can call these functions before
// scalarization occurs
INLINE ulong2 OVERLOADABLE __builtin_IB_udiv( ulong2 N, ulong2 D )
{
    ulong2 result;

    result.s0 = __builtin_IB_udiv( N.s0, D.s0 );
    result.s1 = __builtin_IB_udiv( N.s1, D.s1 );

    return result;
}

INLINE ulong3 OVERLOADABLE __builtin_IB_udiv( ulong3 N, ulong3 D )
{
    ulong3 result;

    result.s0 = __builtin_IB_udiv( N.s0, D.s0 );
    result.s1 = __builtin_IB_udiv( N.s1, D.s1 );
    result.s2 = __builtin_IB_udiv( N.s2, D.s2 );

    return result;
}

INLINE ulong4 OVERLOADABLE __builtin_IB_udiv( ulong4 N, ulong4 D )
{
    ulong4 result;

    result.s01 = __builtin_IB_udiv( N.s01, D.s01 );
    result.s23 = __builtin_IB_udiv( N.s23, D.s23 );

    return result;
}

INLINE ulong8 OVERLOADABLE __builtin_IB_udiv( ulong8 N, ulong8 D )
{
    ulong8 result;

    result.s0123 = __builtin_IB_udiv( N.s0123, D.s0123 );
    result.s4567 = __builtin_IB_udiv( N.s4567, D.s4567 );

    return result;
}

INLINE ulong16 OVERLOADABLE __builtin_IB_udiv( ulong16 N, ulong16 D )
{
    ulong16 result;

    result.s01234567 = __builtin_IB_udiv( N.s01234567, D.s01234567 );
    result.s89abcdef = __builtin_IB_udiv( N.s89abcdef, D.s89abcdef );

    return result;
}

// ----------------------------------------------------------------------------
INLINE ulong OVERLOADABLE __builtin_IB_umod( ulong N, ulong D )
{
    ulong R = 0;
    __builtin_IB_emulate_udiv(N, D, &R);
    return R;
}

// vector versions so we can call these functions before
// scalarization occurs
INLINE ulong2 OVERLOADABLE __builtin_IB_umod( ulong2 N, ulong2 D )
{
    ulong2 result;

    result.s0 = __builtin_IB_umod( N.s0, D.s0 );
    result.s1 = __builtin_IB_umod( N.s1, D.s1 );

    return result;
}

INLINE ulong3 OVERLOADABLE __builtin_IB_umod( ulong3 N, ulong3 D )
{
    ulong3 result;

    result.s0 = __builtin_IB_umod( N.s0, D.s0 );
    result.s1 = __builtin_IB_umod( N.s1, D.s1 );
    result.s2 = __builtin_IB_umod( N.s2, D.s2 );

    return result;
}

INLINE ulong4 OVERLOADABLE __builtin_IB_umod( ulong4 N, ulong4 D )
{
    ulong4 result;

    result.s01 = __builtin_IB_umod( N.s01, D.s01 );
    result.s23 = __builtin_IB_umod( N.s23, D.s23 );

    return result;
}

INLINE ulong8 OVERLOADABLE __builtin_IB_umod( ulong8 N, ulong8 D )
{
    ulong8 result;

    result.s0123 = __builtin_IB_umod( N.s0123, D.s0123 );
    result.s4567 = __builtin_IB_umod( N.s4567, D.s4567 );

    return result;
}

INLINE ulong16 OVERLOADABLE __builtin_IB_umod( ulong16 N, ulong16 D )
{
    ulong16 result;

    result.s01234567 = __builtin_IB_umod( N.s01234567, D.s01234567 );
    result.s89abcdef = __builtin_IB_umod( N.s89abcdef, D.s89abcdef );

    return result;
}

// ----------------------------------------------------------------------------
INLINE long OVERLOADABLE __builtin_IB_sdiv( long N, long D )
{
    ulong Q = __builtin_IB_emulate_udiv( SPIRV_OCL_BUILTIN(s_abs, _i64, )(N), SPIRV_OCL_BUILTIN(s_abs, _i64, )(D), 0 );

    ulong SIGN =  ( N ^ D ) & 0x8000000000000000;

    if( SIGN )
    {
        return -Q;

    }
    return as_long( Q );
}

// vector versions so we can call these functions before
// scalarization occurs
INLINE long2 OVERLOADABLE __builtin_IB_sdiv( long2 N, long2 D )
{
    long2 result;

    result.s0 = __builtin_IB_sdiv( N.s0, D.s0 );
    result.s1 = __builtin_IB_sdiv( N.s1, D.s1 );

    return result;
}

INLINE long3 OVERLOADABLE __builtin_IB_sdiv( long3 N, long3 D )
{
    long3 result;

    result.s0 = __builtin_IB_sdiv( N.s0, D.s0 );
    result.s1 = __builtin_IB_sdiv( N.s1, D.s1 );
    result.s2 = __builtin_IB_sdiv( N.s2, D.s2 );

    return result;
}

INLINE long4 OVERLOADABLE __builtin_IB_sdiv( long4 N, long4 D )
{
    long4 result;

    result.s01 = __builtin_IB_sdiv( N.s01, D.s01 );
    result.s23 = __builtin_IB_sdiv( N.s23, D.s23 );

    return result;
}

INLINE long8 OVERLOADABLE __builtin_IB_sdiv( long8 N, long8 D )
{
    long8 result;

    result.s0123 = __builtin_IB_sdiv( N.s0123, D.s0123 );
    result.s4567 = __builtin_IB_sdiv( N.s4567, D.s4567 );

    return result;
}

INLINE long16 OVERLOADABLE __builtin_IB_sdiv( long16 N, long16 D )
{
    long16 result;

    result.s01234567 = __builtin_IB_sdiv( N.s01234567, D.s01234567 );
    result.s89abcdef = __builtin_IB_sdiv( N.s89abcdef, D.s89abcdef );

    return result;
}

// ----------------------------------------------------------------------------
INLINE long OVERLOADABLE __builtin_IB_smod( long N, long D )
{
    ulong R = 0;
    __builtin_IB_emulate_udiv(SPIRV_OCL_BUILTIN(s_abs, _i64, )(N), SPIRV_OCL_BUILTIN(s_abs, _i64, )(D), &R);

    uchar SIGN = (N < 0);
    return SIGN ? -R : R;
}

// vector versions so we can call these functions before
// scalarization occurs
INLINE long2 OVERLOADABLE __builtin_IB_smod( long2 N, long2 D )
{
    long2 result;

    result.s0 = __builtin_IB_smod( N.s0, D.s0 );
    result.s1 = __builtin_IB_smod( N.s1, D.s1 );

    return result;
}

INLINE long3 OVERLOADABLE __builtin_IB_smod( long3 N, long3 D )
{
    long3 result;

    result.s0 = __builtin_IB_smod( N.s0, D.s0 );
    result.s1 = __builtin_IB_smod( N.s1, D.s1 );
    result.s2 = __builtin_IB_smod( N.s2, D.s2 );

    return result;
}

INLINE long4 OVERLOADABLE __builtin_IB_smod( long4 N, long4 D )
{
    long4 result;

    result.s01 = __builtin_IB_smod( N.s01, D.s01 );
    result.s23 = __builtin_IB_smod( N.s23, D.s23 );

    return result;
}

INLINE long8 OVERLOADABLE __builtin_IB_smod( long8 N, long8 D )
{
    long8 result;

    result.s0123 = __builtin_IB_smod( N.s0123, D.s0123 );
    result.s4567 = __builtin_IB_smod( N.s4567, D.s4567 );

    return result;
}

INLINE long16 OVERLOADABLE __builtin_IB_smod( long16 N, long16 D )
{
    long16 result;

    result.s01234567 = __builtin_IB_smod( N.s01234567, D.s01234567 );
    result.s89abcdef = __builtin_IB_smod( N.s89abcdef, D.s89abcdef );

    return result;
}

#endif
