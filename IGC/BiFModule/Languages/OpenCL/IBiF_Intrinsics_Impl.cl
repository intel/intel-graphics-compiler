/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "spirv.h"

INLINE float OVERLOADABLE clamp( float x, float minval, float maxval )
{
    return __spirv_ocl_fclamp(x,minval,maxval);
}

INLINE float OVERLOADABLE max( float x, float y )
{
    return __spirv_ocl_fmax_common(x, y);
}

INLINE float OVERLOADABLE min( float x, float y )
{
    return __spirv_ocl_fmin_common(x, y);
}

INLINE
uchar OVERLOADABLE abs( char x )
{
    return __spirv_ocl_s_abs( x );
}

INLINE
ushort OVERLOADABLE abs( short x )
{
    return __spirv_ocl_s_abs( x );
}

INLINE
uint OVERLOADABLE abs( int x )
{
    return __spirv_ocl_s_abs( x );
}

INLINE
char OVERLOADABLE add_sat( char x,
                           char y )
{
    return __spirv_ocl_s_add_sat( x, y );
}

INLINE
uchar OVERLOADABLE add_sat( uchar x,
                            uchar y )
{
    return __spirv_ocl_u_add_sat( x, y );
}

INLINE
short OVERLOADABLE add_sat( short x,
                            short y )
{
    return __spirv_ocl_s_add_sat( x, y );
}

INLINE
ushort OVERLOADABLE add_sat( ushort x,
                             ushort y )
{
    return __spirv_ocl_u_add_sat( x, y );
}

INLINE
int OVERLOADABLE add_sat( int x,
                          int y )
{
    return __spirv_ocl_s_add_sat( x, y );
}

INLINE
uint OVERLOADABLE add_sat( uint x,
                           uint y )
{
    return __spirv_ocl_u_add_sat( x, y );
}

INLINE
uchar OVERLOADABLE ctz( uchar x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
ushort OVERLOADABLE ctz( ushort x )
{
    return __spirv_ocl_ctz( x );
}

INLINE
uint OVERLOADABLE ctz( uint x )
{
    return as_uint(__spirv_ocl_ctz( as_int(x) ) );
}

INLINE
int OVERLOADABLE mad24( int x,
                        int y,
                        int z )
{
    return __spirv_ocl_s_mad24( x, y, z );
}

INLINE
uint OVERLOADABLE mad24( uint x,
                         uint y,
                         uint z )
{
    return __spirv_ocl_u_mad24( x, y, z );
}

INLINE
char OVERLOADABLE mad_sat( char a,
                           char b,
                           char c )
{
    return __spirv_ocl_s_mad_sat( a, b, c );
}

INLINE
uchar OVERLOADABLE mad_sat( uchar a,
                            uchar b,
                            uchar c )
{
    return __spirv_ocl_u_mad_sat( a, b, c );
}

INLINE
short OVERLOADABLE mad_sat( short a,
                            short b,
                            short c )
{
    return __spirv_ocl_s_mad_sat( a, b, c );
}

INLINE
ushort OVERLOADABLE mad_sat( ushort a,
                             ushort b,
                             ushort c )
{
    return __spirv_ocl_u_mad_sat( a, b, c );
}

INLINE
int OVERLOADABLE mad_sat( int a,
                          int b,
                          int c )
{
    return __spirv_ocl_s_mad_sat( a, b, c );
}

INLINE
uint OVERLOADABLE mad_sat( uint a,
                           uint b,
                           uint c )
{
    return __spirv_ocl_u_mad_sat( a, b, c );
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
    return __spirv_ocl_u_mul_hi( x, y );
}

INLINE
int OVERLOADABLE mul_hi( int x,
                         int y )
{
    return __spirv_ocl_s_mul_hi( x, y );
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
    return __spirv_ocl_s_sub_sat( x, y );
}

INLINE
uchar OVERLOADABLE sub_sat( uchar x,
                            uchar y )
{
    return __spirv_ocl_u_sub_sat( x, y );
}

INLINE
short OVERLOADABLE sub_sat( short x,
                            short y )
{
    return __spirv_ocl_s_sub_sat( x, y );
}

INLINE
ushort OVERLOADABLE sub_sat( ushort x,
                             ushort y )
{
    return __spirv_ocl_u_sub_sat( x, y );
}

INLINE
int OVERLOADABLE sub_sat( int x,
                          int y )
{
    return __spirv_ocl_s_sub_sat( x, y );
}

INLINE
uint OVERLOADABLE sub_sat( uint x,
                           uint y )
{
    return __spirv_ocl_u_sub_sat( x, y );
}

INLINE
float OVERLOADABLE acos( float x )
{
    return __spirv_ocl_acos( x );
}

INLINE
float OVERLOADABLE asin( float value )
{
    return __spirv_ocl_asin( value );
}

INLINE
float OVERLOADABLE atan( float value )
{
    return __spirv_ocl_atan( value );
}

INLINE
float OVERLOADABLE ceil( float x )
{
    return __spirv_ocl_ceil( x );
}

INLINE
float OVERLOADABLE fabs( float x )
{
    return __spirv_ocl_fabs( x );
}

INLINE
float OVERLOADABLE floor( float x )
{
    return __spirv_ocl_floor( x );
}

INLINE
float OVERLOADABLE rint( float x )
{
    return __spirv_ocl_rint( x );
}

INLINE
float OVERLOADABLE trunc( float x )
{
    return __spirv_ocl_trunc( x );
}

INLINE
float OVERLOADABLE native_cos( float x )
{
    return __spirv_ocl_native_cos( x );
}

INLINE
float OVERLOADABLE native_exp2( float x )
{
    return __spirv_ocl_native_exp2( x );
}

INLINE
float OVERLOADABLE native_log2( float x )
{
    return __spirv_ocl_native_log2( x );
}

INLINE
float OVERLOADABLE native_powr( float x,
                                float y )
{
    return __spirv_ocl_native_powr( x, y );
}

INLINE
float OVERLOADABLE native_recip( float x )
{
    return __spirv_ocl_native_recip( x );
}

INLINE
float OVERLOADABLE native_rsqrt( float x )
{
    return __spirv_ocl_native_rsqrt( x );
}

INLINE
float OVERLOADABLE native_sin( float x )
{
    return __spirv_ocl_native_sin( x );
}

INLINE
float OVERLOADABLE native_sqrt( float x )
{
    return __spirv_ocl_native_sqrt( x );
}

INLINE
float OVERLOADABLE native_tan( float x )
{
    return __spirv_ocl_native_tan( x );
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
    return __spirv_ocl_acos( x );
}

INLINE
half OVERLOADABLE asin( half x )
{
    return __spirv_ocl_asin( x );
}

INLINE
half OVERLOADABLE atan( half x )
{
    return __spirv_ocl_atan( x );
}

INLINE
half OVERLOADABLE ceil( half x )
{
    return __spirv_ocl_ceil( x );
}

INLINE
half OVERLOADABLE fabs( half x )
{
    return __spirv_ocl_fabs( x );
}

INLINE
half OVERLOADABLE floor( half x )
{
    return __spirv_ocl_floor( x );
}

INLINE
half OVERLOADABLE fma( half a,
                       half b,
                       half c )
{
    return __spirv_ocl_fma( a, b, c );
}

INLINE
half OVERLOADABLE mad( half a,
                       half b,
                       half c )
{
    return __spirv_ocl_mad( a, b, c );
}

INLINE
half OVERLOADABLE rint( half x )
{
    return __spirv_ocl_rint( x );
}

INLINE
half OVERLOADABLE trunc( half x )
{
    return __spirv_ocl_trunc( x );
}

INLINE
half OVERLOADABLE native_cos( half x )
{
    return __spirv_ocl_native_cos( x );
}

INLINE
half OVERLOADABLE native_exp2( half x )
{
    return __spirv_ocl_native_exp2( x );
}

INLINE
half OVERLOADABLE native_log2( half x )
{
    return __spirv_ocl_native_log2( x );
}

INLINE
half OVERLOADABLE native_powr( half x,
                               half y )
{
    return __spirv_ocl_native_powr( x, y );
}

INLINE
half OVERLOADABLE native_recip( half x )
{
    return __spirv_ocl_native_recip( x );
}

INLINE
half OVERLOADABLE native_rsqrt( half x )
{
    return __spirv_ocl_native_rsqrt( x );
}

INLINE
half OVERLOADABLE native_sin( half x )
{
    return __spirv_ocl_native_sin( x );
}

INLINE
half OVERLOADABLE native_sqrt( half x )
{
    return __spirv_ocl_native_sqrt( x );
}

INLINE
half OVERLOADABLE native_tan( half x )
{
    return __spirv_ocl_native_tan( x );
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
    return __spirv_ocl_ceil( x );
}

INLINE
double OVERLOADABLE fabs( double x )
{
    return __spirv_ocl_fabs( x );
}

INLINE
double OVERLOADABLE floor( double x )
{
    return __spirv_ocl_floor( x );
}

INLINE
double OVERLOADABLE trunc( double x )
{
    return __spirv_ocl_trunc( x );
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
    return __spirv_ocl_native_sqrt( x );
}

INLINE double OVERLOADABLE native_rsqrt( double x )
{
    return __spirv_ocl_native_rsqrt( x );
}

#endif // defined(cl_khr_fp64)
