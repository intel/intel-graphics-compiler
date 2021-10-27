/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE char SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )( char a, char b, char c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE char __intel_vector_select_helper( char a, char b, char c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, char, char, i8, i8 )

INLINE short SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )( short a, short b, short c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE short __intel_vector_select_helper( short a, short b, short c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, short, short, i16, i16 )

INLINE int SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )( int a, int b, int c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE int __intel_vector_select_helper( int a, int b, int c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, int, int, i32, i32 )

INLINE long SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _i64_i64_i64, )( long a, long b, long c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE long __intel_vector_select_helper( long a, long b, long c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, long, long, i64, i64 )

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( float a, float b, int c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE float __intel_vector_select_helper( float a, float b, int c )
{
    return as_int(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, float, int, f32, i32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( double a, double b, long c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE double __intel_vector_select_helper( double a, double b, long c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, double, long, f64, i64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( half a, half b, short c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE half __intel_vector_select_helper( half a, half b, short c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, half, short, f16, i16 )

#endif // defined(cl_khr_fp16)
