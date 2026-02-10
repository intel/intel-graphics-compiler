/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "spirv.h"
#include "IMF/FP32/asin_s_la.cl"

INLINE float __attribute__((overloadable)) __spirv_ocl_fclamp(float x, float minval, float maxval ){
    return __spirv_ocl_fmin(__spirv_ocl_fmax(x, minval), maxval);
}

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_s_abs(char x ){
    return x > 0 ? x : -x;
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_s_abs(short x ){
    return x > 0 ? x : -x;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_s_abs(int x ){
    // Convert signed to unsigned to have consistent result.
    // Also, make abs(INT32_MIN) well defined : abs(INT32_MIN) = (ui32)(-(i64)INT32_MIN)
    uint ux = (uint)x;
    return x > 0 ? ux : -ux;
}

INLINE
char __attribute__((overloadable)) __spirv_ocl_s_add_sat( char x,
                                      char y )
{
    return (char)__spirv_ocl_s_min( __spirv_ocl_s_max( (int)x + (int)y , CHAR_MIN), CHAR_MAX);
}

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_add_sat( uchar x,
                                       uchar y )
{
    return (uchar)__spirv_ocl_u_min( __spirv_ocl_u_max( (uint)x + (uint)y , 0), UCHAR_MAX);
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_s_add_sat( short x,
                                         short y )
{
    return (short)__spirv_ocl_s_min( __spirv_ocl_s_max( (int)x + (int)y , SHRT_MIN), SHRT_MAX);
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_add_sat( ushort x,
                                          ushort y )
{
    return (ushort)__spirv_ocl_u_min(__spirv_ocl_u_max( (uint)x + (uint)y, 0), USHRT_MAX);
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_add_sat( int x,
                                       int y )
{
    long tmp = (long) x + (long) y;
    return (int) __spirv_ocl_s_clamp( tmp, (long)INT_MIN, (long)INT_MAX);
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_add_sat( uint x,
                                        uint y )
{
    long tmp = (long) x + (long) y;
    return (uint) __spirv_ocl_s_clamp( tmp, 0L, (long)UINT_MAX);
}

INLINE
char __attribute__((overloadable)) __spirv_ocl_ctz(char x ){
    return (char)__spirv_ocl_ctz(as_int((uint)x | (1U << sizeof(x) * 8)));
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_ctz(short x ){
    return (short)__spirv_ocl_ctz(as_int((uint)x | (1U << sizeof(x) * 8)));
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_ctz(int x ){
    uint rev = __builtin_IB_bfrev(x);
    return __spirv_ocl_clz(as_int(rev));
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_mad24( int x,
                                         int y,
                                         int z )
{
    return __spirv_ocl_s_mul24(x, y) + z;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_mad24( uint x,
                                          uint y,
                                          uint z )
{
    return __spirv_ocl_u_mul24(x, y) + z;
}

INLINE char __attribute__((overloadable)) __spirv_ocl_s_mad_sat(
    char a,
    char b,
    char c)
{
    short res = (short)a * (short)b + (short)c;
    return __spirv_SConvert_Rchar_sat(res);
}

INLINE uchar __attribute__((overloadable)) __spirv_ocl_u_mad_sat(
    uchar a,
    uchar b,
    uchar c)
{
    ushort res = (ushort)a * (ushort)b + (ushort)c;
    return __spirv_UConvert_Ruchar_sat(res);
}

INLINE short __attribute__((overloadable)) __spirv_ocl_s_mad_sat(
    short a,
    short b,
    short c)
{
    int res = (int)a * (int)b + (int)c;
    return __spirv_SConvert_Rshort_sat(res);
}

INLINE ushort __attribute__((overloadable)) __spirv_ocl_u_mad_sat(
    ushort a,
    ushort b,
    ushort c)
{
    uint res = (uint)a * (uint)b + (uint)c;
    return __spirv_UConvert_Rushort_sat(res);
}

INLINE int __attribute__((overloadable)) __spirv_ocl_s_mad_sat(
    int a,
    int b,
    int c)
{
    long res = (long)a * (long)b + (long)c;
    return __spirv_SConvert_Rint_sat(res);
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_mad_sat( uint a,
                                            uint b,
                                            uint c )
{
    ulong res = (ulong)a * (ulong)b + (ulong)c;
    return __spirv_UConvert_Ruint_sat(res);
}

INLINE
char __attribute__((overloadable)) __spirv_ocl_s_max(char x, char y ){
    return (x >= y) ? x : y;
}

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_max(uchar x, uchar y ){
    return (x >= y) ? x : y;
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_s_max(short x, short y ){
    return (x >= y) ? x : y;
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_max(ushort x, ushort y ){
    return (x >= y) ? x : y;
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_max(int x, int y ){
    return (x >= y) ? x : y;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_max(uint x, uint y ){
    return (x >= y) ? x : y;
}

INLINE
long __attribute__((overloadable)) __spirv_ocl_s_max(long x, long y ){
    return (x >= y) ? x : y;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_max(ulong x, ulong y ){
    return (x >= y) ? x : y;
}

INLINE
char __attribute__((overloadable)) __spirv_ocl_s_min(char x, char y ){
    return (x <= y) ? x : y;
}

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_min(uchar x, uchar y ){
    return (x <= y) ? x : y;
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_s_min(short x, short y ){
    return (x <= y) ? x : y;
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_min(ushort x, ushort y ){
    return (x <= y) ? x : y;
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_min(int x, int y ){
    return (x <= y) ? x : y;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_min(uint x, uint y ){
    return (x <= y) ? x : y;
}

INLINE
long __attribute__((overloadable)) __spirv_ocl_s_min(long x, long y ){
    return (x < y) ? x : y;
}

INLINE
ulong __attribute__((overloadable)) __spirv_ocl_u_min(ulong x, ulong y ){
    return (x < y) ? x : y;
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_mul_hi( uint x,
                                       uint y )
{
    return ((ulong)x * (ulong)y) >> 32;
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_mul_hi( int x,
                                      int y )
{
    return ((long)x * (long)y) >> 32;
}

INLINE
char __attribute__((overloadable)) __spirv_ocl_popcount(char x ){
    return __builtin_IB_popcount_1u8(x);
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_popcount(short x ){
    return __builtin_IB_popcount_1u16(x);
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_popcount(int x ){
    return __builtin_IB_popcount_1u32(x);
}

INLINE
char __attribute__((overloadable)) __spirv_ocl_s_sub_sat( char x,
                                      char y )
{
    short tmp = (short)x - (short)y;
    return (char) __spirv_ocl_s_clamp(tmp, (short)CHAR_MIN, (short)CHAR_MAX);
}

INLINE
uchar __attribute__((overloadable)) __spirv_ocl_u_sub_sat( uchar x,
                                       uchar y )
{
    return ( x <= y ) ? 0 : x - y;
}

INLINE
short __attribute__((overloadable)) __spirv_ocl_s_sub_sat( short x,
                                         short y )
{
    int tmp = (int)x - (int)y;
    return (short) __spirv_ocl_s_clamp( tmp, (int)SHRT_MIN, (int)SHRT_MAX);
}

INLINE
ushort __attribute__((overloadable)) __spirv_ocl_u_sub_sat( ushort x,
                                          ushort y )
{
    return ( x <= y ) ? 0 : x - y;
}

INLINE
int __attribute__((overloadable)) __spirv_ocl_s_sub_sat( int x,
                                       int y )
{
    long tmp = (long)x - (long)y;
    return (int) __spirv_ocl_s_clamp( tmp, (long)INT_MIN, (long)INT_MAX);
}

INLINE
uint __attribute__((overloadable)) __spirv_ocl_u_sub_sat( uint x,
                                        uint y )
{
    return ( x <= y ) ? 0 : x - y;
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_acos(float x ){
    // The LA acos implementation (IMF/FP32/acos_s_la.cl)
    // seems to be slower on Mandelbulb algorithm..
    float temp1 = 0.0f;
    float temp2 = 0.0f;
    float temp4 = 0.0f;

    float destTemp = 0.0f;

    bool flag = false;

    temp1 = -__spirv_ocl_fabs(x) + 1.0f;

    temp1 = temp1  * 0.5f;

    flag = __spirv_ocl_fabs(x) > 0.575f;

    if( flag )
    {
        temp1 = __spirv_ocl_sqrt(temp1);
    }
    else
    {
        temp1 = __spirv_ocl_fabs(x);
    }

    temp2 = temp1 * temp1;

    destTemp = temp2 * -0.5011622905731201f;

    temp4 = temp2 + -5.478654384613037f;

    destTemp = destTemp + 0.9152014851570129f;

    temp4 = temp2 * temp4;

    destTemp = temp2 * destTemp;

    temp4 = temp4 + 5.491230487823486f;

    destTemp = temp1 * destTemp;

    temp4 = __spirv_ocl_native_recip(temp4);

    destTemp = temp4 * destTemp;

    destTemp = temp1 + destTemp;

    if( flag )
    {
        destTemp = destTemp * 2.0f;
    }
    else
    {
        destTemp = -destTemp + 1.5707963705062866f;
    }

    if( x <= 0.0 )
    {
        destTemp = -destTemp + 3.1415927410125732f;
    }

    return destTemp;
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_asin(float value ){
    return __ocl_svml_asinf(value);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_atan(float value ){
    // The LA atan implementation (IMF/FP32/atan_s_la.cl)
    // seems to be slower on Mandelbulb algorithm..
    float temp1 = 0.0f;
    float temp2 = 0.0f;
    float temp3 = 0.0f;
    float temp4 = 0.0f;

    float destTemp = 0.0f;

    bool flag = __spirv_ocl_fabs(value) > 1.0f;

    temp1 = __spirv_ocl_fabs(value);

    if(flag)
    {
        temp1 = __spirv_ocl_native_recip(temp1);
    }

    temp2 = temp1 * temp1;

    destTemp = temp2 * -0.8233629465103149f;

    temp4 = temp2 + 11.33538818359375f;

    destTemp = destTemp + -5.674867153167725f;

    temp4 = temp4 * temp2;

    destTemp = temp2 * destTemp;

    temp4 = temp4 + 28.84246826171875f;

    destTemp = destTemp + -6.565555095672607f;

    temp4 = temp4 * temp2;

    destTemp = temp2 * destTemp;

    temp4 = temp4 + 19.696670532226562f;

    destTemp = temp1 * destTemp;

    temp4 = __spirv_ocl_native_recip(temp4);

    destTemp = temp4 * destTemp;

    destTemp = destTemp + temp1;

    if(flag)
    {
        destTemp = -destTemp + 1.5707963705062866f;
    }

    if(__spirv_SignBitSet(value))
    {
        destTemp = -__spirv_ocl_fabs(destTemp);
    }

    return destTemp;
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_ceil(float x ){
    return __builtin_IB_frnd_pi(x);
}

INLINE
float FDIV_IEEE( float a,
                           float b )
{
    return __builtin_IB_ieee_divide(a, b);
}

INLINE
double FDIV_IEEE_DOUBLE( double a,
                           double b )
{
    return __builtin_IB_ieee_divide_f64(a, b);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_fabs(float x ){
    return as_float(as_uint(x) & 0x7FFFFFFF);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_floor(float x ){
    return __builtin_IB_frnd_ni(x);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_rint(float x ){
    return __builtin_IB_frnd_ne(x);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_trunc(float x ){
    return __builtin_IB_frnd_zi(x);
}

INLINE
float FSQRT_IEEE( float a )
{
    return __builtin_IB_ieee_sqrt(a);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_cos(float x ){
    return __builtin_IB_native_cosf(x);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_exp2(float x ){
    return __builtin_IB_native_exp2f(x);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_log2(float x ){
    return __builtin_IB_native_log2f(x);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_powr( float x,
                                           float y )
{
    return __builtin_IB_native_powrf(x, y);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_recip(float x ){
    return 1/x;
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_rsqrt(float x ){
    return __spirv_ocl_native_recip(__spirv_ocl_native_sqrt(x));
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_sin(float x ){
    return __builtin_IB_native_sinf(x);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_sqrt(float x ){
    return __builtin_IB_native_sqrtf(x);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_native_tan(float x ){
    return __spirv_ocl_native_divide(__spirv_ocl_native_sin(x), __spirv_ocl_native_cos(x));
}

#ifdef cl_khr_fp16
INLINE half __attribute__((overloadable)) __spirv_ocl_fclamp(half x, half minval, half maxval ){
    return __spirv_ocl_fmin(__spirv_ocl_fmax(x, minval), maxval);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_acos(half x ){
    return (half)__spirv_ocl_acos((float)x);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_asin(half x ){
    return (half)__spirv_ocl_asin((float)x);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_atan(half x ){
    return (half)__spirv_ocl_atan((float)x);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_ceil(half x ){
    return __spirv_FConvert_Rhalf(__spirv_ocl_ceil(__spirv_FConvert_Rfloat(x)));
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_fabs(half x ){
    //return __builtin_IB_fabsh(x);
    ushort mask = 0x7FFF;
    ushort temp = as_ushort(x) & mask;
    return as_half(temp);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_floor(half x ){
    return __spirv_FConvert_Rhalf(__spirv_ocl_floor(__spirv_FConvert_Rfloat(x)));
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_fma( half a,
                                      half b,
                                      half c )
{
    return __builtin_IB_fmah(a, b, c);
}

INLINE
float __attribute__((overloadable)) __spirv_ocl_fma( float a,
                                      float b,
                                      float c )
{
    return __builtin_fmaf(a, b, c);
}

INLINE
double __attribute__((overloadable)) __spirv_ocl_fma( double a,
                                      double b,
                                      double c )
{
    return __builtin_fma(a, b, c);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_mad( half a,
                                      half b,
                                      half c )
{
    //return __builtin_IB_madh(a, b, c);
    return a*b+c;
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_rint(half x ){
    return __spirv_FConvert_Rhalf(__spirv_ocl_rint(__spirv_FConvert_Rfloat(x)));
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_trunc(half x ){
    return __spirv_FConvert_Rhalf(__spirv_ocl_trunc(__spirv_FConvert_Rfloat(x)));
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_native_cos(half x ){
    return __builtin_IB_native_cosh(x);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_native_exp2(half x ){
    return __builtin_IB_native_exp2h(x);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_native_log2(half x ){
    return __builtin_IB_native_log2h(x);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_native_powr( half x,
                                          half y )
{
    return (half)__builtin_IB_native_powrf((float)x, (float)y);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_native_recip(half x ){
    return 1/x;
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_native_rsqrt(half x ){
    return __spirv_ocl_native_recip(__spirv_ocl_native_sqrt(x));
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_native_sin(half x ){
    return __builtin_IB_native_sinh(x);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_native_sqrt(half x ){
    return __builtin_IB_native_sqrth(x);
}

INLINE
half __attribute__((overloadable)) __spirv_ocl_native_tan(half x ){
    return __spirv_ocl_native_divide(__spirv_ocl_native_sin((float)x), __spirv_ocl_native_cos((float)x));
}

#endif

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_fclamp(double x, double minval, double maxval ){
    return __spirv_ocl_fmin(__spirv_ocl_fmax(x, minval), maxval);
}

INLINE
double __attribute__((overloadable)) __spirv_ocl_ceil(double x ){
    //First part of the algorithm performs rounding towards zero by truncating bits in
    //the fractional part of the number.This is done by finding out the position of the
    //fractional bits of the mantissa and masking them out with zeros.

    double roundedToZeroVal = __spirv_ocl_trunc(x);
    unsigned high32Bit = (int)(as_ulong( x ) >> 32);
    ulong fraction = as_ulong(x) - as_ulong(roundedToZeroVal);    // getting fraction

    // Second part is to calculate exponent adjustment based on sign of source

    uint temp5 = (uint)(as_ulong( fraction ));
    uint temp6 = (uint)(as_ulong( fraction ) >> 32);
    uint sign = high32Bit & 0x80000000;
    uint expBias = (sign == 0) ? 0x3ff00000 : 0;
    uint orDst = temp5 | temp6;
    ulong signAdjustedVal = (orDst == 0) ? 0 : (expBias);
    double output = as_double( signAdjustedVal << 32 ) + as_double( roundedToZeroVal );
    return output;
}

INLINE
double __attribute__((overloadable)) __spirv_ocl_fabs(double x ){
    uint2 temp = as_uint2(x);
    temp.s1 = temp.s1 & 0x7FFFFFFF;
    return as_double(temp);
}

INLINE
double __attribute__((overloadable)) __spirv_ocl_floor(double x ){
    //First part of the algorithm performs rounding towards zero by truncating bits in
    //the fractional part of the number.This is done by finding out the position of the
    //fractional bits of the mantissa and masking them out with zeros.

    double roundedToZeroVal = __spirv_ocl_trunc(x);

    ulong fraction = as_ulong(x) - as_ulong( roundedToZeroVal );       // getting fraction

    // Second part is to calculate exponent adjustment based on sign of source

    uint temp5 = (uint)( fraction );
    uint temp6 = (uint)(fraction >> 32);
    unsigned high32Bit = (uint)(as_ulong( x ) >> 32);
    uint sign = high32Bit & 0x80000000;
    uint expBias = (sign != 0) ? 0xbff00000 : 0;
    uint orDst = temp5 | temp6;
    ulong signAdjustedVal = (orDst == 0) ? 0 : (expBias);
    double output = as_double( signAdjustedVal << 32 ) + as_double( roundedToZeroVal );
    return output;
}

INLINE double __attribute__((overloadable)) __spirv_ocl_native_sqrt(double x ){
        return __builtin_IB_native_sqrtd(x);
}

INLINE double __attribute__((overloadable)) __spirv_ocl_native_rsqrt(double x ){
      return (1 / __builtin_IB_native_sqrtd(x));
}

#endif // defined(cl_khr_fp64)

