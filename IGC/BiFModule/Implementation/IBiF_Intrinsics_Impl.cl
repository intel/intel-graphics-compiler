/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/


#include "spirv.h"

INLINE float __builtin_spirv_OpenCL_fclamp_f32_f32_f32(float x, float minval, float maxval ){
    return __builtin_spirv_OpenCL_fmin_f32_f32(__builtin_spirv_OpenCL_fmax_f32_f32(x, minval), maxval);
}

INLINE float __builtin_spirv_OpenCL_fmax_common_f32_f32(float x, float y ){
    return __builtin_IB_fmax(x, y);
}

INLINE float __builtin_spirv_OpenCL_fmin_common_f32_f32(float x, float y ){
    return __builtin_IB_fmin(x, y);
}

INLINE
uchar __builtin_spirv_OpenCL_s_abs_i8(char x ){
    return x > 0 ? x : -x;
}

INLINE
ushort __builtin_spirv_OpenCL_s_abs_i16(short x ){
    return x > 0 ? x : -x;
}

INLINE
uint __builtin_spirv_OpenCL_s_abs_i32(int x ){
    return x > 0 ? x : -x;
}

INLINE
char __builtin_spirv_OpenCL_s_add_sat_i8_i8( char x,
                                      char y )
{
    return (char)__builtin_spirv_OpenCL_s_min_i32_i32( __builtin_spirv_OpenCL_s_max_i32_i32( (int)x + (int)y , CHAR_MIN), CHAR_MAX);
}

INLINE
uchar __builtin_spirv_OpenCL_u_add_sat_i8_i8( uchar x,
                                       uchar y )
{
    return (uchar)__builtin_spirv_OpenCL_u_min_i32_i32( __builtin_spirv_OpenCL_u_max_i32_i32( (uint)x + (uint)y , 0), UCHAR_MAX);
}

INLINE
short __builtin_spirv_OpenCL_s_add_sat_i16_i16( short x,
                                         short y )
{
    return (short)__builtin_spirv_OpenCL_s_min_i32_i32( __builtin_spirv_OpenCL_s_max_i32_i32( (int)x + (int)y , SHRT_MIN), SHRT_MAX);
}

INLINE
ushort __builtin_spirv_OpenCL_u_add_sat_i16_i16( ushort x,
                                          ushort y )
{
    return (ushort)__builtin_spirv_OpenCL_u_min_i32_i32(__builtin_spirv_OpenCL_u_max_i32_i32( (uint)x + (uint)y, 0), USHRT_MAX);
}

INLINE
int __builtin_spirv_OpenCL_s_add_sat_i32_i32( int x,
                                       int y )
{
    long tmp = (long) x + (long) y;
    return (int) __builtin_spirv_OpenCL_s_clamp_i64_i64_i64( tmp, (long)INT_MIN, (long)INT_MAX);
}

INLINE
uint __builtin_spirv_OpenCL_u_add_sat_i32_i32( uint x,
                                        uint y )
{
    long tmp = (long) x + (long) y;
    return (uint) __builtin_spirv_OpenCL_s_clamp_i64_i64_i64( tmp, 0L, (long)UINT_MAX);
}

INLINE
uchar __builtin_spirv_OpenCL_ctz_i8(uchar x ){
    return (uchar)__builtin_spirv_OpenCL_ctz_i32((uint)x | (1U << sizeof(x) * 8));
}

INLINE
ushort __builtin_spirv_OpenCL_ctz_i16(ushort x ){
    return (ushort)__builtin_spirv_OpenCL_ctz_i32((uint)x | (1U << sizeof(x) * 8));
}

INLINE
uint __builtin_spirv_OpenCL_ctz_i32(uint x ){
    uint rev = __builtin_IB_bfrev(x);
    return __builtin_spirv_OpenCL_clz_i32(rev);
}

INLINE
int __builtin_spirv_OpenCL_s_mad24_i32_i32_i32( int x,
                                         int y,
                                         int z )
{
    return __builtin_spirv_OpenCL_s_mul24_i32_i32(x, y) + z;
}

INLINE
uint __builtin_spirv_OpenCL_u_mad24_i32_i32_i32( uint x,
                                          uint y,
                                          uint z )
{
    return __builtin_spirv_OpenCL_u_mul24_i32_i32(x, y) + z;
}

INLINE char __builtin_spirv_OpenCL_s_mad_sat_i8_i8_i8(
    char a,
    char b,
    char c)
{
    short res = (short)a * (short)b + (short)c;
    return __builtin_spirv_OpSConvert_Sat_i8_i16(res);
}

INLINE uchar __builtin_spirv_OpenCL_u_mad_sat_i8_i8_i8(
    uchar a,
    uchar b,
    uchar c)
{
    ushort res = (ushort)a * (ushort)b + (ushort)c;
    return __builtin_spirv_OpUConvert_Sat_i8_i16(res);
}

INLINE short __builtin_spirv_OpenCL_s_mad_sat_i16_i16_i16(
    short a,
    short b,
    short c)
{
    int res = (int)a * (int)b + (int)c;
    return __builtin_spirv_OpSConvert_Sat_i16_i32(res);
}

INLINE ushort __builtin_spirv_OpenCL_u_mad_sat_i16_i16_i16(
    ushort a,
    ushort b,
    ushort c)
{
    uint res = (uint)a * (uint)b + (uint)c;
    return __builtin_spirv_OpUConvert_Sat_i16_i32(res);
}

INLINE int __builtin_spirv_OpenCL_s_mad_sat_i32_i32_i32(
    int a,
    int b,
    int c)
{
    long res = (long)a * (long)b + (long)c;
    return __builtin_spirv_OpSConvert_Sat_i32_i64(res);
}

INLINE
uint __builtin_spirv_OpenCL_u_mad_sat_i32_i32_i32( uint a,
                                            uint b,
                                            uint c )
{
    ulong res = (ulong)a * (ulong)b + (ulong)c;
    return __builtin_spirv_OpUConvert_Sat_i32_i64(res);
}

INLINE
char __builtin_spirv_OpenCL_s_max_i8_i8(char x, char y ){
    return (x >= y) ? x : y;
}

INLINE
uchar __builtin_spirv_OpenCL_u_max_i8_i8(uchar x, uchar y ){
    return (x >= y) ? x : y;
}

INLINE
short __builtin_spirv_OpenCL_s_max_i16_i16(short x, short y ){
    return (x >= y) ? x : y;
}

INLINE
ushort __builtin_spirv_OpenCL_u_max_i16_i16(ushort x, ushort y ){
    return (x >= y) ? x : y;
}

INLINE
int __builtin_spirv_OpenCL_s_max_i32_i32(int x, int y ){
    return (x >= y) ? x : y;
}

INLINE
uint __builtin_spirv_OpenCL_u_max_i32_i32(uint x, uint y ){
    return (x >= y) ? x : y;
}

INLINE
long __builtin_spirv_OpenCL_s_max_i64_i64(long x, long y ){
    return (x >= y) ? x : y;
}

INLINE
ulong __builtin_spirv_OpenCL_u_max_i64_i64(ulong x, ulong y ){
    return (x >= y) ? x : y;
}

INLINE
char __builtin_spirv_OpenCL_s_min_i8_i8(char x, char y ){
    return (x <= y) ? x : y;
}

INLINE
uchar __builtin_spirv_OpenCL_u_min_i8_i8(uchar x, uchar y ){
    return (x <= y) ? x : y;
}

INLINE
short __builtin_spirv_OpenCL_s_min_i16_i16(short x, short y ){
    return (x <= y) ? x : y;
}

INLINE
ushort __builtin_spirv_OpenCL_u_min_i16_i16(ushort x, ushort y ){
    return (x <= y) ? x : y;
}

INLINE
int __builtin_spirv_OpenCL_s_min_i32_i32(int x, int y ){
    return (x <= y) ? x : y;
}

INLINE
uint __builtin_spirv_OpenCL_u_min_i32_i32(uint x, uint y ){
    return (x <= y) ? x : y;
}

INLINE
long __builtin_spirv_OpenCL_s_min_i64_i64(long x, long y ){
    return (x < y) ? x : y;
}

INLINE
ulong __builtin_spirv_OpenCL_u_min_i64_i64(ulong x, ulong y ){
    return (x < y) ? x : y;
}

INLINE
uint __builtin_spirv_OpenCL_u_mul_hi_i32_i32( uint x,
                                       uint y )
{
    return ((ulong)x * (ulong)y) >> 32;
}

INLINE
int __builtin_spirv_OpenCL_s_mul_hi_i32_i32( int x,
                                      int y )
{
    return ((long)x * (long)y) >> 32;
}

INLINE
uchar __builtin_spirv_OpenCL_popcount_i8(uchar x ){
    return __builtin_IB_popcount_1u8(x);
}

INLINE
ushort __builtin_spirv_OpenCL_popcount_i16(ushort x ){
    return __builtin_IB_popcount_1u16(x);
}

INLINE
uint __builtin_spirv_OpenCL_popcount_i32(uint x ){
    return __builtin_IB_popcount_1u32(x);
}

INLINE
char __builtin_spirv_OpenCL_s_sub_sat_i8_i8( char x,
                                      char y )
{
    short tmp = (short)x - (short)y;
    return (char) __builtin_spirv_OpenCL_s_clamp_i16_i16_i16(tmp, (short)CHAR_MIN, (short)CHAR_MAX);
}

INLINE
uchar __builtin_spirv_OpenCL_u_sub_sat_i8_i8( uchar x,
                                       uchar y )
{
    return ( x <= y ) ? 0 : x - y;
}

INLINE
short __builtin_spirv_OpenCL_s_sub_sat_i16_i16( short x,
                                         short y )
{
    int tmp = (int)x - (int)y;
    return (short) __builtin_spirv_OpenCL_s_clamp_i32_i32_i32( tmp, (int)SHRT_MIN, (int)SHRT_MAX);
}

INLINE
ushort __builtin_spirv_OpenCL_u_sub_sat_i16_i16( ushort x,
                                          ushort y )
{
    return ( x <= y ) ? 0 : x - y;
}

INLINE
int __builtin_spirv_OpenCL_s_sub_sat_i32_i32( int x,
                                       int y )
{
    long tmp = (long)x - (long)y;
    return (int) __builtin_spirv_OpenCL_s_clamp_i64_i64_i64( tmp, (long)INT_MIN, (long)INT_MAX);
}

INLINE
uint __builtin_spirv_OpenCL_u_sub_sat_i32_i32( uint x,
                                        uint y )
{
    return ( x <= y ) ? 0 : x - y;
}

INLINE
float __builtin_spirv_OpenCL_acos_f32(float x ){
    float temp1 = 0.0f;
    float temp2 = 0.0f;
    float temp4 = 0.0f;

    float destTemp = 0.0f;

    bool flag = false;

    temp1 = -__builtin_spirv_OpenCL_fabs_f32(x) + 1.0f;

    temp1 = temp1  * 0.5f;

    flag = __builtin_spirv_OpenCL_fabs_f32(x) > 0.575f;

    if( flag )
    {
        temp1 = __builtin_spirv_OpenCL_sqrt_f32(temp1);
    }
    else
    {
        temp1 = __builtin_spirv_OpenCL_fabs_f32(x);
    }

    temp2 = temp1 * temp1;

    destTemp = temp2 * -0.5011622905731201f;

    temp4 = temp2 + -5.478654384613037f;

    destTemp = destTemp + 0.9152014851570129f;

    temp4 = temp2 * temp4;

    destTemp = temp2 * destTemp;

    temp4 = temp4 + 5.491230487823486f;

    destTemp = temp1 * destTemp;

    temp4 = __builtin_spirv_OpenCL_native_recip_f32(temp4);

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
float __builtin_spirv_OpenCL_asin_f32(float value ){
    float temp1 = 0.0f;
    float temp2 = 0.0f;
    float temp4 = 0.04;

    float destTemp = 0.0f;

    bool flag = false;

    temp1 = -__builtin_spirv_OpenCL_fabs_f32(value) + 1.0f;

    temp1 = temp1  * 0.5f;

    flag = __builtin_spirv_OpenCL_fabs_f32(value) > 0.575f;

    if( flag )
    {
        temp1 = __builtin_spirv_OpenCL_sqrt_f32(temp1);
    }
    else
    {
        temp1 = __builtin_spirv_OpenCL_fabs_f32(value);
    }

    temp2 = temp1 * temp1;

    destTemp = temp2 * -0.5011622905731201f;

    temp4 = temp2 + -5.478654384613037f;

    destTemp = destTemp + 0.9152014851570129f;

    temp4 = temp2 * temp4;

    destTemp = temp2 * destTemp;

    temp4 = temp4 + 5.491230487823486f;

    destTemp = temp1 * destTemp;

    temp4 = __builtin_spirv_OpenCL_native_recip_f32(temp4);

    destTemp = temp4 * destTemp;

    destTemp = temp1 + destTemp;


    if( flag )
    {
        destTemp = destTemp * -2.0f;
       destTemp = destTemp + 1.5707963705062866f;
    }

    if( value < 0.0 )
    {
        destTemp = -__builtin_spirv_OpenCL_fabs_f32(destTemp);
    }

    return destTemp;
}

INLINE
float __builtin_spirv_OpenCL_atan_f32(float value ){
    float temp1 = 0.0f;
    float temp2 = 0.0f;
    float temp3 = 0.0f;
    float temp4 = 0.0f;

    float destTemp = 0.0f;

    bool flag = __builtin_spirv_OpenCL_fabs_f32(value) > 1.0f;

    temp1 = __builtin_spirv_OpenCL_fabs_f32(value);

    if(flag)
    {
        temp1 = __builtin_spirv_OpenCL_native_recip_f32(temp1);
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

    temp4 = __builtin_spirv_OpenCL_native_recip_f32(temp4);

    destTemp = temp4 * destTemp;

    destTemp = destTemp + temp1;

    if(flag)
    {
        destTemp = -destTemp + 1.5707963705062866f;
    }

    if(value < 0.0f)
    {
        destTemp = -__builtin_spirv_OpenCL_fabs_f32(destTemp);
    }

    return destTemp;
}

INLINE
float __builtin_spirv_OpenCL_ceil_f32(float x ){
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
float __builtin_spirv_OpenCL_fabs_f32(float x ){
    float neg = -x;
    return (x >= 0) ?  x : neg;
}

INLINE
float __builtin_spirv_OpenCL_floor_f32(float x ){
    return __builtin_IB_frnd_ni(x);
}

INLINE
float __builtin_spirv_OpenCL_rint_f32(float x ){
    return __builtin_IB_frnd_ne(x);
}

INLINE
float __builtin_spirv_OpenCL_trunc_f32(float x ){
    return __builtin_IB_frnd_zi(x);
}

INLINE
float FSQRT_IEEE( float a )
{
    return __builtin_IB_ieee_sqrt(a);
}

INLINE
float __builtin_spirv_OpenCL_native_cos_f32(float x ){
    return __builtin_IB_native_cosf(x);
}

INLINE
float __builtin_spirv_OpenCL_native_exp2_f32(float x ){
    return __builtin_IB_native_exp2f(x);
}

INLINE
float __builtin_spirv_OpenCL_native_log2_f32(float x ){
    return __builtin_IB_native_log2f(x);
}

INLINE
float __builtin_spirv_OpenCL_native_powr_f32_f32( float x,
                                           float y )
{
    return __builtin_IB_native_powrf(x, y);
}

INLINE
float __builtin_spirv_OpenCL_native_recip_f32(float x ){
    return 1/x;
}

INLINE
float __builtin_spirv_OpenCL_native_rsqrt_f32(float x ){
    return __builtin_spirv_OpenCL_native_recip_f32(__builtin_spirv_OpenCL_native_sqrt_f32(x));
}

INLINE
float __builtin_spirv_OpenCL_native_sin_f32(float x ){
    return __builtin_IB_native_sinf(x);
}

INLINE
float __builtin_spirv_OpenCL_native_sqrt_f32(float x ){
    return __builtin_IB_native_sqrtf(x);
}

INLINE
float __builtin_spirv_OpenCL_native_tan_f32(float x ){
    return __builtin_spirv_OpenCL_native_divide_f32_f32(__builtin_spirv_OpenCL_native_sin_f32(x), __builtin_spirv_OpenCL_native_cos_f32(x));
}

#ifdef cl_khr_fp16
INLINE half __builtin_spirv_OpenCL_fclamp_f16_f16_f16(half x, half minval, half maxval ){
    return __builtin_spirv_OpenCL_fmin_f32_f32(__builtin_spirv_OpenCL_fmax_f32_f32(x, minval), maxval);
}

INLINE half __builtin_spirv_OpenCL_fmax_common_f16_f16(half x, half y ){
    return __builtin_IB_HMAX(x, y);
}

INLINE half __builtin_spirv_OpenCL_fmin_common_f16_f16(half x, half y ){
    return __builtin_IB_HMIN(x, y);
}

INLINE
half __builtin_spirv_OpenCL_acos_f16(half x ){
    return (half)__builtin_spirv_OpenCL_acos_f32((float)x);
}

INLINE
half __builtin_spirv_OpenCL_asin_f16(half x ){
    return (half)__builtin_spirv_OpenCL_asin_f32((float)x);
}

INLINE
half __builtin_spirv_OpenCL_atan_f16(half x ){
    return (half)__builtin_spirv_OpenCL_atan_f32((float)x);
}

INLINE
half __builtin_spirv_OpenCL_ceil_f16(half x ){
    return __builtin_spirv_OpFConvert_f16_f32(__builtin_spirv_OpenCL_ceil_f32(__builtin_spirv_OpFConvert_f32_f16(x)));
}

INLINE
half __builtin_spirv_OpenCL_fabs_f16(half x ){
    //return __builtin_IB_fabsh(x);
    return (x >= 0) ?  x : -x;

}

INLINE
half __builtin_spirv_OpenCL_floor_f16(half x ){
    return __builtin_spirv_OpFConvert_f16_f32(__builtin_spirv_OpenCL_floor_f32(__builtin_spirv_OpFConvert_f32_f16(x)));
}

INLINE
half __builtin_spirv_OpenCL_fma_f16_f16_f16( half a,
                                      half b,
                                      half c )
{
    return __builtin_IB_fmah(a, b, c);
}

INLINE 
float __builtin_spirv_OpenCL_fma_f32_f32_f32( float a,
                                      float b,
                                      float c )
{
    return __builtin_fmaf(a, b, c);
}

INLINE 
double __builtin_spirv_OpenCL_fma_f64_f64_f64( double a,
                                      double b,
                                      double c )
{
    return __builtin_fma(a, b, c);
}


INLINE
half __builtin_spirv_OpenCL_mad_f16_f16_f16( half a,
                                      half b,
                                      half c )
{
    //return __builtin_IB_madh(a, b, c);
    return a*b+c;
}

INLINE
half __builtin_spirv_OpenCL_rint_f16(half x ){
    return __builtin_spirv_OpFConvert_f16_f32(__builtin_spirv_OpenCL_rint_f32(__builtin_spirv_OpFConvert_f32_f16(x)));
}

INLINE
half __builtin_spirv_OpenCL_trunc_f16(half x ){
    return __builtin_spirv_OpFConvert_f16_f32(__builtin_spirv_OpenCL_trunc_f32(__builtin_spirv_OpFConvert_f32_f16(x)));
}

INLINE
half __builtin_spirv_OpenCL_native_cos_f16(half x ){
    return __builtin_IB_native_cosh(x);
}

INLINE
half __builtin_spirv_OpenCL_native_exp2_f16(half x ){
    return __builtin_IB_native_exp2h(x);
}

INLINE
half __builtin_spirv_OpenCL_native_log2_f16(half x ){
    return __builtin_IB_native_log2h(x);
}

INLINE
half __builtin_spirv_OpenCL_native_powr_f16_f16( half x,
                                          half y )
{
    return (half)__builtin_IB_native_powrf((float)x, (float)y);
}

INLINE
half __builtin_spirv_OpenCL_native_recip_f16(half x ){
    return 1/x;
}

INLINE
half __builtin_spirv_OpenCL_native_rsqrt_f16(half x ){
    return __builtin_spirv_OpenCL_native_recip_f16(__builtin_spirv_OpenCL_native_sqrt_f16(x));
}

INLINE
half __builtin_spirv_OpenCL_native_sin_f16(half x ){
    return __builtin_IB_native_sinh(x);
}

INLINE
half __builtin_spirv_OpenCL_native_sqrt_f16(half x ){
    return __builtin_IB_native_sqrth(x);
}

INLINE
half __builtin_spirv_OpenCL_native_tan_f16(half x ){
    return __builtin_spirv_OpenCL_native_divide_f32_f32(__builtin_spirv_OpenCL_native_sin_f32((float)x), __builtin_spirv_OpenCL_native_cos_f32((float)x));
}

#endif

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_fclamp_f64_f64_f64(double x, double minval, double maxval ){
    return __builtin_spirv_OpenCL_fmin_f64_f64(__builtin_spirv_OpenCL_fmax_f64_f64(x, minval), maxval);
}

INLINE
double __builtin_spirv_OpenCL_ceil_f64(double x ){
    //First part of the algorithm performs rounding towards zero by truncating bits in
    //the fractional part of the number.This is done by finding out the position of the
    //fractional bits of the mantissa and masking them out with zeros.

    double roundedToZeroVal = __builtin_spirv_OpenCL_trunc_f64(x);
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
double __builtin_spirv_OpenCL_fabs_f64(double x ){
    double neg = -x;
    return (x >= 0) ?  x : neg;
}

INLINE
double __builtin_spirv_OpenCL_floor_f64(double x ){
    //First part of the algorithm performs rounding towards zero by truncating bits in 
    //the fractional part of the number.This is done by finding out the position of the 
    //fractional bits of the mantissa and masking them out with zeros.

    double roundedToZeroVal = __builtin_spirv_OpenCL_trunc_f64(x);

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

INLINE double __builtin_spirv_OpenCL_fmax_common_f64_f64(double x, double y ){
    return __builtin_IB_dmax(x, y);
}

INLINE double __builtin_spirv_OpenCL_fmin_common_f64_f64(double x, double y ){
    //return __builtin_IB_minf(x, y);
    return (x <= y) ? x : y;
}

INLINE double __builtin_spirv_OpenCL_native_sqrt_f64(double x ){
        return __builtin_IB_native_sqrtd(x);
}

INLINE double __builtin_spirv_OpenCL_native_rsqrt_f64(double x ){
      return (1 / __builtin_IB_native_sqrtd(x));
}

#endif // defined(cl_khr_fp64)

