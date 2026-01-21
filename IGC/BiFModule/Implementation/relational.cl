/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Relational and Logical Instructions

bool __attribute__((overloadable)) __spirv_Any(__bool2 Vector)
{
    return (Vector.s0 | Vector.s1);
}

bool __attribute__((overloadable)) __spirv_Any(__bool3 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2);
}

bool __attribute__((overloadable)) __spirv_Any(__bool4 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2 | Vector.s3);
}

bool __attribute__((overloadable)) __spirv_Any(__bool8 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2 | Vector.s3 |
            Vector.s4 | Vector.s5 | Vector.s6 | Vector.s7);
}

bool __attribute__((overloadable)) __spirv_Any(__bool16 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2 | Vector.s3 |
            Vector.s4 | Vector.s5 | Vector.s6 | Vector.s7 |
            Vector.s8 | Vector.s9 | Vector.sa | Vector.sb |
            Vector.sc | Vector.sd | Vector.se | Vector.sf);
}

bool __attribute__((overloadable)) __spirv_All(__bool2 Vector)
{
    return (Vector.s0 & Vector.s1);
}

bool __attribute__((overloadable)) __spirv_All(__bool3 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2);
}

bool __attribute__((overloadable)) __spirv_All(__bool4 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2 & Vector.s3);
}

bool __attribute__((overloadable)) __spirv_All(__bool8 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2 & Vector.s3 &
            Vector.s4 & Vector.s5 & Vector.s6 & Vector.s7);
}

bool __attribute__((overloadable)) __spirv_All(__bool16 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2 & Vector.s3 &
            Vector.s4 & Vector.s5 & Vector.s6 & Vector.s7 &
            Vector.s8 & Vector.s9 & Vector.sa & Vector.sb &
            Vector.sc & Vector.sd & Vector.se & Vector.sf);
}

bool  __attribute__((overloadable)) __spirv_IsNan(half x)
{
    return x != x;
}

bool  __attribute__((overloadable)) __spirv_IsNan(float x)
{
    return x != x;
}

#if defined(cl_khr_fp64)
bool  __attribute__((overloadable)) __spirv_IsNan(double x)
{
    return x != x;
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNan, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNan, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNan, __bool, double, f64)
#endif

bool __attribute__((overloadable)) __spirv_IsInf(half x)
{
    return __spirv_ocl_fabs(x) == (half)(INFINITY);
}

bool __attribute__((overloadable)) __spirv_IsInf(float x)
{
    return __spirv_ocl_fabs(x) == (float)(INFINITY);
}

#if defined(cl_khr_fp64)
bool __attribute__((overloadable)) __spirv_IsInf(double x)
{
    return __spirv_ocl_fabs(x) == (double)(INFINITY);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsInf, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsInf, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsInf, __bool, double, f64)
#endif

bool __attribute__((overloadable)) __spirv_IsFinite(half x)
{
    return __spirv_ocl_fabs(x) < (half)(INFINITY);
}

bool __attribute__((overloadable)) __spirv_IsFinite(float x)
{
    return __spirv_ocl_fabs(x) < (float)(INFINITY);
}

#if defined(cl_khr_fp64)
bool __attribute__((overloadable)) __spirv_IsFinite(double x)
{
    return __spirv_ocl_fabs(x) < (double)(INFINITY);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsFinite, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsFinite, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsFinite, __bool, double, f64)
#endif

bool __attribute__((overloadable)) __spirv_IsNormal(half x)
{
    return __spirv_IsFinite(x) & (__spirv_ocl_fabs(x) >= HALF_MIN);
}

bool __attribute__((overloadable)) __spirv_IsNormal(float x)
{
    return __spirv_IsFinite(x) & (__spirv_ocl_fabs(x) >= FLT_MIN);
}

#if defined(cl_khr_fp64)
bool __attribute__((overloadable)) __spirv_IsNormal(double x)
{
    return __spirv_IsFinite(x) & (__spirv_ocl_fabs(x) >= DBL_MIN);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNormal, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNormal, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNormal, __bool, double, f64)
#endif

bool __attribute__((overloadable)) __spirv_SignBitSet(half x)
{
    return (as_short( x ) & HALF_SIGN_MASK) != 0;
}

bool __attribute__((overloadable)) __spirv_SignBitSet(float x)
{
    return (as_int( x ) & FLOAT_SIGN_MASK) != 0;
}

#if defined(cl_khr_fp64)
bool __attribute__((overloadable)) __spirv_SignBitSet(double x)
{
    return (as_long( x ) & DOUBLE_SIGN_MASK) != 0;
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(SignBitSet, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(SignBitSet, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(SignBitSet, __bool, double, f64)
#endif

bool __attribute__((overloadable)) __spirv_LessOrGreater(half x, half y)
{
    return (x < y) | (x > y);
}

bool __attribute__((overloadable)) __spirv_LessOrGreater(float x, float y)
{
    return (x < y) | (x > y);
}

#if defined(cl_khr_fp64)
bool __attribute__((overloadable)) __spirv_LessOrGreater(double x, double y)
{
    return (x < y) | (x > y);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(LessOrGreater, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(LessOrGreater, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(LessOrGreater, __bool, double, f64)
#endif

bool __attribute__((overloadable)) __spirv_Ordered(half x, half y)
{
    return (x == x) & (y == y);
}

bool __attribute__((overloadable)) __spirv_Ordered(float x, float y)
{
    return (x == x) & (y == y);
}

#if defined(cl_khr_fp64)
bool __attribute__((overloadable)) __spirv_Ordered(double x, double y)
{
    return (x == x) & (y == y);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Ordered, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Ordered, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Ordered, __bool, double, f64)
#endif

bool __attribute__((overloadable)) __spirv_Unordered(half x, half y)
{
    return (x != x) | (y != y);
}

bool __attribute__((overloadable)) __spirv_Unordered(float x, float y)
{
    return (x != x) | (y != y);
}

#if defined(cl_khr_fp64)
bool __attribute__((overloadable)) __spirv_Unordered(double x, double y)
{
    return (x != x) | (y != y);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Unordered, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Unordered, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Unordered, __bool, double, f64)
#endif
