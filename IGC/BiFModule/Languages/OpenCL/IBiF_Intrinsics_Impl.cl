/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "spirv.h"

INLINE float OVERLOADABLE clamp( float x, float minval, float maxval )
{
    return SPIRV_OCL_BUILTIN(fclamp, _f32_f32_f32, )(x,minval,maxval);
}

INLINE float OVERLOADABLE max( float x, float y )
{
    return SPIRV_OCL_BUILTIN(fmax_common, _f32_f32, )(x, y);
}

INLINE float OVERLOADABLE min( float x, float y )
{
    return SPIRV_OCL_BUILTIN(fmin_common, _f32_f32, )(x, y);
}

INLINE
uchar OVERLOADABLE abs( char x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _i8, )( x );
}

INLINE
ushort OVERLOADABLE abs( short x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _i16, )( x );
}

INLINE
uint OVERLOADABLE abs( int x )
{
    return SPIRV_OCL_BUILTIN(s_abs, _i32, )( x );
}

INLINE
char OVERLOADABLE add_sat( char x,
                           char y )
{
    return SPIRV_OCL_BUILTIN(s_add_sat, _i8_i8, )( x, y );
}

INLINE
uchar OVERLOADABLE add_sat( uchar x,
                            uchar y )
{
    return SPIRV_OCL_BUILTIN(u_add_sat, _i8_i8, )( x, y );
}

INLINE
short OVERLOADABLE add_sat( short x,
                            short y )
{
    return SPIRV_OCL_BUILTIN(s_add_sat, _i16_i16, )( x, y );
}

INLINE
ushort OVERLOADABLE add_sat( ushort x,
                             ushort y )
{
    return SPIRV_OCL_BUILTIN(u_add_sat, _i16_i16, )( x, y );
}

INLINE
int OVERLOADABLE add_sat( int x,
                          int y )
{
    return SPIRV_OCL_BUILTIN(s_add_sat, _i32_i32, )( x, y );
}

INLINE
uint OVERLOADABLE add_sat( uint x,
                           uint y )
{
    return SPIRV_OCL_BUILTIN(u_add_sat, _i32_i32, )( x, y );
}

INLINE
uchar OVERLOADABLE ctz( uchar x )
{
    return SPIRV_OCL_BUILTIN(ctz, _i8, )( x );
}

INLINE
ushort OVERLOADABLE ctz( ushort x )
{
    return SPIRV_OCL_BUILTIN(ctz, _i16, )( x );
}

INLINE
uint OVERLOADABLE ctz( uint x )
{
    return as_uint(SPIRV_OCL_BUILTIN(ctz, _i32, )( as_int(x) ) );
}

INLINE
int OVERLOADABLE mad24( int x,
                        int y,
                        int z )
{
    return SPIRV_OCL_BUILTIN(s_mad24, _i32_i32_i32, )( x, y, z );
}

INLINE
uint OVERLOADABLE mad24( uint x,
                         uint y,
                         uint z )
{
    return SPIRV_OCL_BUILTIN(u_mad24, _i32_i32_i32, )( x, y, z );
}

INLINE
char OVERLOADABLE mad_sat( char a,
                           char b,
                           char c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _i8_i8_i8, )( a, b, c );
}

INLINE
uchar OVERLOADABLE mad_sat( uchar a,
                            uchar b,
                            uchar c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _i8_i8_i8, )( a, b, c );
}

INLINE
short OVERLOADABLE mad_sat( short a,
                            short b,
                            short c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _i16_i16_i16, )( a, b, c );
}

INLINE
ushort OVERLOADABLE mad_sat( ushort a,
                             ushort b,
                             ushort c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _i16_i16_i16, )( a, b, c );
}

INLINE
int OVERLOADABLE mad_sat( int a,
                          int b,
                          int c )
{
    return SPIRV_OCL_BUILTIN(s_mad_sat, _i32_i32_i32, )( a, b, c );
}

INLINE
uint OVERLOADABLE mad_sat( uint a,
                           uint b,
                           uint c )
{
    return SPIRV_OCL_BUILTIN(u_mad_sat, _i32_i32_i32, )( a, b, c );
}

INLINE
char OVERLOADABLE max( char x, char y )
{
    return (x >= y) ? x : y;
}

INLINE
uchar OVERLOADABLE max( uchar x, uchar y )
{
    return (x >= y) ? x : y;
}

INLINE
short OVERLOADABLE max( short x, short y )
{
    return (x >= y) ? x : y;
}

INLINE
ushort OVERLOADABLE max( ushort x, ushort y )
{
    return (x >= y) ? x : y;
}

INLINE
int OVERLOADABLE max( int x, int y )
{
    return (x >= y) ? x : y;
}

INLINE
uint OVERLOADABLE max( uint x, uint y )
{
    return (x >= y) ? x : y;
}

INLINE
long OVERLOADABLE max( long x, long y )
{
    return (x >= y) ? x : y;
}

INLINE
ulong OVERLOADABLE max( ulong x, ulong y )
{
    return (x >= y) ? x : y;
}

INLINE
char OVERLOADABLE min( char x, char y )
{
    return (x <= y) ? x : y;
}

INLINE
uchar OVERLOADABLE min( uchar x, uchar y )
{
    return (x <= y) ? x : y;
}

INLINE
short OVERLOADABLE min( short x, short y )
{
    return (x <= y) ? x : y;
}

INLINE
ushort OVERLOADABLE min( ushort x, ushort y )
{
    return (x <= y) ? x : y;
}

INLINE
int OVERLOADABLE min( int x, int y )
{
    return (x <= y) ? x : y;
}

INLINE
uint OVERLOADABLE min( uint x, uint y )
{
    return (x <= y) ? x : y;
}

INLINE
long OVERLOADABLE min( long x, long y )
{
    return (x < y) ? x : y;
}

INLINE
ulong OVERLOADABLE min( ulong x, ulong y )
{
    return (x < y) ? x : y;
}

// !!! TODO: Why do we need this function?
INLINE
int OVERLOADABLE min( ulong x, int y )
{
    return ((int)x <= y) ? (int)x : y;
}

// !!! TODO: Why do we need this function?
INLINE
uint OVERLOADABLE min( ulong x, uint y )
{
    return ((uint)x <= y) ? (uint)x : y;
}

INLINE
uint OVERLOADABLE mul_hi( uint x,
                          uint y )
{
    return SPIRV_OCL_BUILTIN(u_mul_hi, _i32_i32, )( x, y );
}

INLINE
int OVERLOADABLE mul_hi( int x,
                         int y )
{
    return SPIRV_OCL_BUILTIN(s_mul_hi, _i32_i32, )( x, y );
}

INLINE
uchar OVERLOADABLE popcount( uchar x )
{
    return __builtin_IB_popcount_1u8(x);
}

INLINE
ushort OVERLOADABLE popcount( ushort x )
{
    return __builtin_IB_popcount_1u16(x);
}

INLINE
uint OVERLOADABLE popcount( uint x )
{
    return __builtin_IB_popcount_1u32(x);
}

INLINE
char OVERLOADABLE sub_sat( char x,
                           char y )
{
    return SPIRV_OCL_BUILTIN(s_sub_sat, _i8_i8, )( x, y );
}

INLINE
uchar OVERLOADABLE sub_sat( uchar x,
                            uchar y )
{
    return SPIRV_OCL_BUILTIN(u_sub_sat, _i8_i8, )( x, y );
}

INLINE
short OVERLOADABLE sub_sat( short x,
                            short y )
{
    return SPIRV_OCL_BUILTIN(s_sub_sat, _i16_i16, )( x, y );
}

INLINE
ushort OVERLOADABLE sub_sat( ushort x,
                             ushort y )
{
    return SPIRV_OCL_BUILTIN(u_sub_sat, _i16_i16, )( x, y );
}

INLINE
int OVERLOADABLE sub_sat( int x,
                          int y )
{
    return SPIRV_OCL_BUILTIN(s_sub_sat, _i32_i32, )( x, y );
}

INLINE
uint OVERLOADABLE sub_sat( uint x,
                           uint y )
{
    return SPIRV_OCL_BUILTIN(u_sub_sat, _i32_i32, )( x, y );
}

INLINE
float OVERLOADABLE acos( float x )
{
    return SPIRV_OCL_BUILTIN(acos, _f32, )( x );
}

INLINE
float OVERLOADABLE asin( float value )
{
    return SPIRV_OCL_BUILTIN(asin, _f32, )( value );
}

INLINE
float OVERLOADABLE atan( float value )
{
    return SPIRV_OCL_BUILTIN(atan, _f32, )( value );
}

INLINE
float OVERLOADABLE ceil( float x )
{
    return SPIRV_OCL_BUILTIN(ceil, _f32, )( x );
}

INLINE
float OVERLOADABLE fabs( float x )
{
    return SPIRV_OCL_BUILTIN(fabs, _f32, )( x );
}

INLINE
float OVERLOADABLE floor( float x )
{
    return SPIRV_OCL_BUILTIN(floor, _f32, )( x );
}

INLINE
float OVERLOADABLE rint( float x )
{
    return SPIRV_OCL_BUILTIN(rint, _f32, )( x );
}

INLINE
float OVERLOADABLE trunc( float x )
{
    return SPIRV_OCL_BUILTIN(trunc, _f32, )( x );
}

INLINE
float OVERLOADABLE native_cos( float x )
{
    return SPIRV_OCL_BUILTIN(native_cos, _f32, )( x );
}

INLINE
float OVERLOADABLE native_exp2( float x )
{
    return SPIRV_OCL_BUILTIN(native_exp2, _f32, )( x );
}

INLINE
float OVERLOADABLE native_log2( float x )
{
    return SPIRV_OCL_BUILTIN(native_log2, _f32, )( x );
}

INLINE
float OVERLOADABLE native_powr( float x,
                                float y )
{
    return SPIRV_OCL_BUILTIN(native_powr, _f32_f32, )( x, y );
}

INLINE
float OVERLOADABLE native_recip( float x )
{
    return SPIRV_OCL_BUILTIN(native_recip, _f32, )( x );
}

INLINE
float OVERLOADABLE native_rsqrt( float x )
{
    return SPIRV_OCL_BUILTIN(native_rsqrt, _f32, )( x );
}

INLINE
float OVERLOADABLE native_sin( float x )
{
    return SPIRV_OCL_BUILTIN(native_sin, _f32, )( x );
}

INLINE
float OVERLOADABLE native_sqrt( float x )
{
    return SPIRV_OCL_BUILTIN(native_sqrt, _f32, )( x );
}

INLINE
float OVERLOADABLE native_tan( float x )
{
    return SPIRV_OCL_BUILTIN(native_tan, _f32, )( x );
}

#ifdef cl_khr_fp16
INLINE half OVERLOADABLE clamp( half x, half minval, half maxval )
{
    return fmin( fmax( x, minval), maxval);
}

INLINE half OVERLOADABLE max( half x, half y )
{
    return __builtin_IB_HMAX(x, y);
}

INLINE half OVERLOADABLE min( half x, half y )
{
    return __builtin_IB_HMIN(x, y);
}

INLINE
half OVERLOADABLE acos( half x )
{
    return SPIRV_OCL_BUILTIN(acos, _f16, )( x );
}

INLINE
half OVERLOADABLE asin( half x )
{
    return SPIRV_OCL_BUILTIN(asin, _f16, )( x );
}

INLINE
half OVERLOADABLE atan( half x )
{
    return SPIRV_OCL_BUILTIN(atan, _f16, )( x );
}

INLINE
half OVERLOADABLE ceil( half x )
{
    return SPIRV_OCL_BUILTIN(ceil, _f16, )( x );
}

INLINE
half OVERLOADABLE fabs( half x )
{
    return SPIRV_OCL_BUILTIN(fabs, _f16, )( x );
}

INLINE
half OVERLOADABLE floor( half x )
{
    return SPIRV_OCL_BUILTIN(floor, _f16, )( x );
}

INLINE
half OVERLOADABLE fma( half a,
                       half b,
                       half c )
{
    return SPIRV_OCL_BUILTIN(fma, _f16_f16_f16, )( a, b, c );
}

INLINE
half OVERLOADABLE mad( half a,
                       half b,
                       half c )
{
    return SPIRV_OCL_BUILTIN(mad, _f16_f16_f16, )( a, b, c );
}

INLINE
half OVERLOADABLE rint( half x )
{
    return SPIRV_OCL_BUILTIN(rint, _f16, )( x );
}

INLINE
half OVERLOADABLE trunc( half x )
{
    return SPIRV_OCL_BUILTIN(trunc, _f16, )( x );
}

INLINE
half OVERLOADABLE native_cos( half x )
{
    return SPIRV_OCL_BUILTIN(native_cos, _f16, )( x );
}

INLINE
half OVERLOADABLE native_exp2( half x )
{
    return SPIRV_OCL_BUILTIN(native_exp2, _f16, )( x );
}

INLINE
half OVERLOADABLE native_log2( half x )
{
    return SPIRV_OCL_BUILTIN(native_log2, _f16, )( x );
}

INLINE
half OVERLOADABLE native_powr( half x,
                               half y )
{
    return SPIRV_OCL_BUILTIN(native_powr, _f16_f16, )( x, y );
}

INLINE
half OVERLOADABLE native_recip( half x )
{
    return SPIRV_OCL_BUILTIN(native_recip, _f16, )( x );
}

INLINE
half OVERLOADABLE native_rsqrt( half x )
{
    return SPIRV_OCL_BUILTIN(native_rsqrt, _f16, )( x );
}

INLINE
half OVERLOADABLE native_sin( half x )
{
    return SPIRV_OCL_BUILTIN(native_sin, _f16, )( x );
}

INLINE
half OVERLOADABLE native_sqrt( half x )
{
    return SPIRV_OCL_BUILTIN(native_sqrt, _f16, )( x );
}

INLINE
half OVERLOADABLE native_tan( half x )
{
    return SPIRV_OCL_BUILTIN(native_tan, _f16, )( x );
}

#endif

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE clamp( double x, double minval, double maxval )
{
    return fmin( fmax( x, minval), maxval);
}

INLINE
double OVERLOADABLE ceil( double x )
{
    return SPIRV_OCL_BUILTIN(ceil, _f64, )( x );
}

INLINE
double OVERLOADABLE fabs( double x )
{
    return SPIRV_OCL_BUILTIN(fabs, _f64, )( x );
}

INLINE
double OVERLOADABLE floor( double x )
{
    return SPIRV_OCL_BUILTIN(floor, _f64, )( x );
}

INLINE
double OVERLOADABLE trunc( double x )
{
    return SPIRV_OCL_BUILTIN(trunc, _f64, )( x );
}

INLINE double OVERLOADABLE max( double x, double y )
{
    return (x >= y) ? x : y;
}

INLINE double OVERLOADABLE min( double x, double y )
{
    return __builtin_IB_dmin(x, y);
}

INLINE double OVERLOADABLE native_sqrt( double x )
{
    return SPIRV_OCL_BUILTIN(native_sqrt, _f64, )( x );
}

INLINE double OVERLOADABLE native_rsqrt( double x )
{
    return SPIRV_OCL_BUILTIN(native_rsqrt, _f64, )( x );
}

#endif // defined(cl_khr_fp64)
