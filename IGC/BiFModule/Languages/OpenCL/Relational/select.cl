/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE char OVERLOADABLE select( char a, char b, char c )
{
    return SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )( a, b, c );
}

static INLINE char OVERLOADABLE __intel_vector_select_helper( char a, char b, char c )
{
    return c < 0 ? b : a;
}

INLINE char OVERLOADABLE select( char a, char b, uchar c )
{
    return SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )( a, b, as_char(c) );
}

static INLINE char OVERLOADABLE __intel_vector_select_helper( char a, char b, uchar c )
{
    return as_char(c) < 0 ? b : a;
}

INLINE uchar OVERLOADABLE select( uchar a, uchar b, char c )
{
    return SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )( as_char(a), as_char(b), c );
}

static INLINE uchar OVERLOADABLE __intel_vector_select_helper( uchar a, uchar b, char c )
{
    return c < 0 ? b : a;
}

INLINE uchar OVERLOADABLE select( uchar a, uchar b, uchar c )
{
    return SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )( as_char(a), as_char(b), as_char(c) );
}

static INLINE uchar OVERLOADABLE __intel_vector_select_helper( uchar a, uchar b, uchar c )
{
    return as_char(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, char, char )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, char, uchar )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, uchar, char )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, uchar, uchar )


INLINE short OVERLOADABLE select( short a, short b, short c )
{
    return SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )( a, b, c );
}

static INLINE short OVERLOADABLE __intel_vector_select_helper( short a, short b, short c )
{
    return c < 0 ? b : a;
}

INLINE short OVERLOADABLE select( short a, short b, ushort c )
{
    return SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )( a, b, as_short(c) );
}

static INLINE short OVERLOADABLE __intel_vector_select_helper( short a, short b, ushort c )
{
    return as_short(c) < 0 ? b : a;
}

INLINE ushort OVERLOADABLE select( ushort a, ushort b, short c )
{
    return SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )( as_short(a), as_short(b), c );
}

static INLINE ushort OVERLOADABLE __intel_vector_select_helper( ushort a, ushort b, short c )
{
    return c < 0 ? b : a;
}

INLINE ushort OVERLOADABLE select( ushort a, ushort b, ushort c )
{
    return SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )( as_short(a), as_short(b), as_short(c) );
}

static INLINE ushort OVERLOADABLE __intel_vector_select_helper( ushort a, ushort b, ushort c )
{
    return as_short(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, short, short )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, short, ushort )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, ushort, short )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, ushort, ushort )


INLINE int OVERLOADABLE select( int a, int b, int c )
{
    return SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )( a, b, c );
}

static INLINE int OVERLOADABLE __intel_vector_select_helper( int a, int b, int c )
{
    return c < 0 ? b : a;
}

INLINE int OVERLOADABLE select( int a, int b, uint c )
{
    return SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )( a, b, as_int(c) );
}

static INLINE int OVERLOADABLE __intel_vector_select_helper( int a, int b, uint c )
{
    return as_int(c) < 0 ? b : a;
}

INLINE uint OVERLOADABLE select( uint a, uint b, int c )
{
    return SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )( as_int(a), as_int(b), c );
}

static INLINE uint OVERLOADABLE __intel_vector_select_helper( uint a, uint b, int c )
{
    return c < 0 ? b : a;
}

INLINE uint OVERLOADABLE select( uint a, uint b, uint c )
{
    return SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )( as_int(a), as_int(b), as_int(c) );
}

static INLINE uint OVERLOADABLE __intel_vector_select_helper( uint a, uint b, uint c )
{
    return as_int(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, int, int )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, int, uint )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, uint, int )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, uint, uint )


INLINE long OVERLOADABLE select( long a, long b, long c )
{
    return SPIRV_OCL_BUILTIN(select, _i64_i64_i64, )( a, b, c );
}

static INLINE long OVERLOADABLE __intel_vector_select_helper( long a, long b, long c )
{
    return c < 0 ? b : a;
}

INLINE long OVERLOADABLE select( long a, long b, ulong c )
{
    return SPIRV_OCL_BUILTIN(select, _i64_i64_i64, )( a, b, as_long(c) );
}

static INLINE long OVERLOADABLE __intel_vector_select_helper( long a, long b, ulong c )
{
    return as_long(c) < 0 ? b : a;
}

INLINE ulong OVERLOADABLE select( ulong a, ulong b, long c )
{
    return SPIRV_OCL_BUILTIN(select, _i64_i64_i64, )( as_long(a), as_long(b), c );
}

static INLINE ulong OVERLOADABLE __intel_vector_select_helper( ulong a, ulong b, long c )
{
    return c < 0 ? b : a;
}

INLINE ulong OVERLOADABLE select( ulong a, ulong b, ulong c )
{
    return SPIRV_OCL_BUILTIN(select, _i64_i64_i64, )( as_long(a), as_long(b), as_long(c) );
}

static INLINE ulong OVERLOADABLE __intel_vector_select_helper( ulong a, ulong b, ulong c )
{
    return as_long(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, long, long )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, long, ulong )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, ulong, long )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, ulong, ulong )


INLINE float OVERLOADABLE select( float a, float b, int c )
{
    return SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( a, b, c );
}

static INLINE float OVERLOADABLE __intel_vector_select_helper( float a, float b, int c )
{
    return c < 0 ? b : a;
}

INLINE float OVERLOADABLE select( float a, float b, uint c )
{
    return SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( a, b, c );
}

static INLINE float OVERLOADABLE __intel_vector_select_helper( float a, float b, uint c )
{
    return as_int(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, float, int )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, float, uint )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE select( double a, double b, long c )
{
    return SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( a, b, c );
}

static INLINE double OVERLOADABLE __intel_vector_select_helper( double a, double b, long c )
{
    return c < 0 ? b : a;
}

INLINE double OVERLOADABLE select( double a, double b, ulong c )
{
    return SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( a, b, c );
}

static INLINE double OVERLOADABLE __intel_vector_select_helper( double a, double b, ulong c )
{
    return as_long(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, double, long )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, double, ulong )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE select( half a, half b, short c )
{
    return SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( a, b, c );
}

static INLINE half OVERLOADABLE __intel_vector_select_helper( half a, half b, short c )
{
    return c < 0 ? b : a;
}

INLINE half OVERLOADABLE select( half a, half b, ushort c )
{
    return SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( a, b, as_short(c) );
}

static INLINE half OVERLOADABLE __intel_vector_select_helper( half a, half b, ushort c )
{
    return as_short(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, half, short )
GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, half, ushort )

#endif // defined(cl_khr_fp16)
