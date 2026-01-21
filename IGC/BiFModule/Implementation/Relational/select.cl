/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

INLINE char __attribute__((overloadable)) __spirv_ocl_select( char a, char b, char c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE char __intel_vector_select_helper( char a, char b, char c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, char, char, i8, i8 )

INLINE short __attribute__((overloadable)) __spirv_ocl_select( short a, short b, short c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE short __intel_vector_select_helper( short a, short b, short c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, short, short, i16, i16 )

INLINE int __attribute__((overloadable)) __spirv_ocl_select( int a, int b, int c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE int __intel_vector_select_helper( int a, int b, int c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, int, int, i32, i32 )

INLINE long __attribute__((overloadable)) __spirv_ocl_select( long a, long b, long c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE long __intel_vector_select_helper( long a, long b, long c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, long, long, i64, i64 )

INLINE float __attribute__((overloadable)) __spirv_ocl_select( float a, float b, int c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE float __intel_vector_select_helper( float a, float b, int c )
{
    return as_int(c) < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, float, int, f32, i32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_select( double a, double b, long c )
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

INLINE half __attribute__((overloadable)) __spirv_ocl_select( half a, half b, short c )
{
    return c ? b : a;
}

static INLINE OVERLOADABLE half __intel_vector_select_helper( half a, half b, short c )
{
    return c < 0 ? b : a;
}

GENERATE_VECTOR_FUNCTIONS_3ARGS_SELECT( __intel_vector_select_helper, half, short, f16, i16 )

#endif // defined(cl_khr_fp16)

