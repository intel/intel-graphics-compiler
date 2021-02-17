/*========================== begin_copyright_notice ============================

Copyright (c) 2010-2021 Intel Corporation

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

#define DOUBLE_BITS             (64)
#define HALF_BITS               (16)
#define HALF_SIGN_MASK          ((short)(0x8000))
#define DOUBLE_SIGN_MASK        (0x8000000000000000L)
#define HALF_MANTISSA_MASK      ((short)(0x03FF))
#define DOUBLE_EXPONENT_MASK    (0x7FF0000000000000L)
#define DOUBLE_MANTISSA_BITS    (52)
#define HALF_MANTISSA_BITS      (10)

#define INLINE __attribute__((always_inline))

INLINE half __precompiled_convert_f64_to_f16(double FloatValue)
{
  double a = FloatValue;
  ushort hp = (as_ulong(a) >> (DOUBLE_BITS - HALF_BITS)) & HALF_SIGN_MASK;
  double abs_a = as_double((as_ulong(a) & ~DOUBLE_SIGN_MASK));

  // +/- nan
  if (abs_a != abs_a)
  {
    hp = hp | (as_ulong(abs_a) >> (DOUBLE_BITS - HALF_BITS));
    hp = hp | HALF_MANTISSA_MASK; // Lose and QNANs
  }
  // Overflow
  else if (abs_a >= 0x1.ffep+15)  // max half RTE
  {
    hp = hp | 0x7c00; // exp := 0x1f
  }
  // Underflow
  else if (abs_a <= 0x1p-25)      // min half RTE
  {
    // nothing
  }
  else if (abs_a < 0x1p-24)       // smallest, non-zero exact half
  {
    // Very small denormal
    hp = hp | 0x0001;
  }
  else if (abs_a < 0x1p-14)       // larger half denormals
  {
    // Shift implicit bit into fract component of a half
    hp = hp | as_ulong(abs_a * 0x1p-1050);
  }
  else
  {
    a = a * 0x1p+42;
    a = as_double(as_ulong(a) & DOUBLE_EXPONENT_MASK);
    abs_a = abs_a + a;
    a = abs_a - a;

    a = a * 0x1p-1008;
    hp = hp | (as_ulong(a) >> (DOUBLE_MANTISSA_BITS - HALF_MANTISSA_BITS));
  }

  return as_half(hp);
}

INLINE ulong __precompiled_abs_i64( long x )
{
    return (ulong)((x >= 0) ? x : -x);
}

static INLINE ulong  __precompiled_emulate_udiv( ulong N, ulong D, private ulong* Rem )
{
    ulong Q = 0;
    ulong R = 0;
    int I = 63;

    // If all values fit inside 32 bits then
    // cast to 32 bit values instead of emulating.
    if( N <= 0xFFFFFFFF && D <= 0xFFFFFFFF )
    {
        Q = (uint)N / (uint)D;
        R = (uint)N % (uint)D;
    }
    else
    {
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
    }

    if (Rem)
        *Rem = R;

    return Q;
}

INLINE ulong  __precompiled_udiv( ulong N, ulong D )
{
    return __precompiled_emulate_udiv(N, D, 0);
}

// vector versions so we can call these functions before
// scalarization occurs
INLINE ulong2 __precompiled_udiv2( ulong2 N, ulong2 D )
{
    ulong2 result;

    result.s0 = __precompiled_udiv( N.s0, D.s0 );
    result.s1 = __precompiled_udiv( N.s1, D.s1 );

    return result;
}

INLINE ulong3 __precompiled_udiv3( ulong3 N, ulong3 D )
{
    ulong3 result;

    result.s0 = __precompiled_udiv( N.s0, D.s0 );
    result.s1 = __precompiled_udiv( N.s1, D.s1 );
    result.s2 = __precompiled_udiv( N.s2, D.s2 );

    return result;
}

INLINE ulong4 __precompiled_udiv4( ulong4 N, ulong4 D )
{
    ulong4 result;

    result.s01 = __precompiled_udiv2( N.s01, D.s01 );
    result.s23 = __precompiled_udiv2( N.s23, D.s23 );

    return result;
}

INLINE ulong8 __precompiled_udiv8( ulong8 N, ulong8 D )
{
    ulong8 result;

    result.s0123 = __precompiled_udiv4( N.s0123, D.s0123 );
    result.s4567 = __precompiled_udiv4( N.s4567, D.s4567 );

    return result;
}

INLINE ulong16  __precompiled_udiv16( ulong16 N, ulong16 D )
{
    ulong16 result;

    result.s01234567 = __precompiled_udiv8( N.s01234567, D.s01234567 );
    result.s89abcdef = __precompiled_udiv8( N.s89abcdef, D.s89abcdef );

    return result;
}

// ----------------------------------------------------------------------------
INLINE ulong __precompiled_umod( ulong N, ulong D )
{
    ulong R = 0;
    __precompiled_emulate_udiv(N, D, &R);
    return R;
}

// vector versions so we can call these functions before
// scalarization occurs
INLINE ulong2 __precompiled_umod2( ulong2 N, ulong2 D )
{
    ulong2 result;

    result.s0 = __precompiled_umod( N.s0, D.s0 );
    result.s1 = __precompiled_umod( N.s1, D.s1 );

    return result;
}

INLINE ulong3  __precompiled_umod3( ulong3 N, ulong3 D )
{
    ulong3 result;

    result.s0 = __precompiled_umod( N.s0, D.s0 );
    result.s1 = __precompiled_umod( N.s1, D.s1 );
    result.s2 = __precompiled_umod( N.s2, D.s2 );

    return result;
}

INLINE ulong4  __precompiled_umod4( ulong4 N, ulong4 D )
{
    ulong4 result;

    result.s01 = __precompiled_umod2( N.s01, D.s01 );
    result.s23 =__precompiled_umod2( N.s23, D.s23 );

    return result;
}

INLINE ulong8 __precompiled_umod8( ulong8 N, ulong8 D )
{
    ulong8 result;

    result.s0123 = __precompiled_umod4( N.s0123, D.s0123 );
    result.s4567 = __precompiled_umod4( N.s4567, D.s4567 );

    return result;
}

INLINE ulong16 __precompiled_umod16( ulong16 N, ulong16 D )
{
    ulong16 result;

    result.s01234567 = __precompiled_umod8( N.s01234567, D.s01234567 );
    result.s89abcdef = __precompiled_umod8( N.s89abcdef, D.s89abcdef );

    return result;
}

// ----------------------------------------------------------------------------
INLINE long  __precompiled_sdiv( long N, long D )
{
    ulong Q = __precompiled_emulate_udiv( __precompiled_abs_i64(N), __precompiled_abs_i64(D), 0 );

    ulong SIGN =  ( N ^ D ) & 0x8000000000000000;

    if( SIGN )
    {
        return -Q;

    }
    return as_long( Q );
}

// vector versions so we can call these functions before
// scalarization occurs
INLINE long2 __precompiled_sdiv2( long2 N, long2 D )
{
    long2 result;

    result.s0 = __precompiled_sdiv( N.s0, D.s0 );
    result.s1 = __precompiled_sdiv( N.s1, D.s1 );

    return result;
}

INLINE long3 __precompiled_sdiv3( long3 N, long3 D )
{
    long3 result;

    result.s0 = __precompiled_sdiv( N.s0, D.s0 );
    result.s1 = __precompiled_sdiv( N.s1, D.s1 );
    result.s2 = __precompiled_sdiv( N.s2, D.s2 );

    return result;
}

INLINE long4  __precompiled_sdiv4( long4 N, long4 D )
{
    long4 result;

    result.s01 = __precompiled_sdiv2( N.s01, D.s01 );
    result.s23 = __precompiled_sdiv2( N.s23, D.s23 );

    return result;
}

INLINE long8 __precompiled_sdiv8( long8 N, long8 D )
{
    long8 result;

    result.s0123 = __precompiled_sdiv4( N.s0123, D.s0123 );
    result.s4567 = __precompiled_sdiv4( N.s4567, D.s4567 );

    return result;
}

INLINE long16  __precompiled_sdiv16( long16 N, long16 D )
{
    long16 result;

    result.s01234567 = __precompiled_sdiv8( N.s01234567, D.s01234567 );
    result.s89abcdef = __precompiled_sdiv8( N.s89abcdef, D.s89abcdef );

    return result;
}

// ----------------------------------------------------------------------------
INLINE long __precompiled_smod( long N, long D )
{
    ulong R = 0;
   __precompiled_emulate_udiv(__precompiled_abs_i64(N), __precompiled_abs_i64(D), &R);

    uchar SIGN = (N < 0);
    return SIGN ? -R : R;
}

// vector versions so we can call these functions before
// scalarization occurs
INLINE long2  __precompiled_smod2( long2 N, long2 D )
{
    long2 result;

    result.s0 = __precompiled_smod( N.s0, D.s0 );
    result.s1 = __precompiled_smod( N.s1, D.s1 );

    return result;
}

INLINE long3  __precompiled_smod3( long3 N, long3 D )
{
    long3 result;

    result.s0 = __precompiled_smod( N.s0, D.s0 );
    result.s1 = __precompiled_smod( N.s1, D.s1 );
    result.s2 = __precompiled_smod( N.s2, D.s2 );

    return result;
}

INLINE long4 __precompiled_smod4( long4 N, long4 D )
{
    long4 result;

    result.s01 = __precompiled_smod2( N.s01, D.s01 );
    result.s23 = __precompiled_smod2( N.s23, D.s23 );

    return result;
}

INLINE long8  __precompiled_smod8( long8 N, long8 D )
{
    long8 result;

    result.s0123 = __precompiled_smod4( N.s0123, D.s0123 );
    result.s4567 = __precompiled_smod4( N.s4567, D.s4567 );

    return result;
}

INLINE long16  __precompiled_smod16( long16 N, long16 D )
{
    long16 result;

    result.s01234567 = __precompiled_smod8( N.s01234567, D.s01234567 );
    result.s89abcdef = __precompiled_smod8( N.s89abcdef, D.s89abcdef );

    return result;
}
