/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

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

// Relational and Logical Instructions


bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v2i8, )(__bool2 Vector)
{
    return (Vector.s0 | Vector.s1);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v3i8, )(__bool3 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v4i8, )(__bool4 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2 | Vector.s3);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v8i8, )(__bool8 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2 | Vector.s3 |
            Vector.s4 | Vector.s5 | Vector.s6 | Vector.s7);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Any, _v16i8, )(__bool16 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2 | Vector.s3 |
            Vector.s4 | Vector.s5 | Vector.s6 | Vector.s7 |
            Vector.s8 | Vector.s9 | Vector.sa | Vector.sb |
            Vector.sc | Vector.sd | Vector.se | Vector.sf);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v2i8, )(__bool2 Vector)
{
    return (Vector.s0 & Vector.s1);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v3i8, )(__bool3 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v4i8, )(__bool4 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2 & Vector.s3);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v8i8, )(__bool8 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2 & Vector.s3 &
            Vector.s4 & Vector.s5 & Vector.s6 & Vector.s7);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(All, _v16i8, )(__bool16 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2 & Vector.s3 &
            Vector.s4 & Vector.s5 & Vector.s6 & Vector.s7 &
            Vector.s8 & Vector.s9 & Vector.sa & Vector.sb &
            Vector.sc & Vector.sd & Vector.se & Vector.sf);
}

bool  SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _f16, )(half x)
{
    return x != x;
}

bool  SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _f32, )(float x)
{
    return x != x;
}

#if defined(cl_khr_fp64)
bool  SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _f64, )(double x)
{
    return x != x;
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNan, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNan, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNan, __bool, double, f64)
#endif

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _f16, )(half x)
{
    return __builtin_spirv_OpenCL_fabs_f16(x) == (half)(INFINITY);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _f32, )(float x)
{
    return __builtin_spirv_OpenCL_fabs_f32(x) == (float)(INFINITY);
}

#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _f64, )(double x)
{
    return __builtin_spirv_OpenCL_fabs_f64(x) == (double)(INFINITY);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsInf, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsInf, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsInf, __bool, double, f64)
#endif

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _f16, )(half x)
{
    return __builtin_spirv_OpenCL_fabs_f16(x) < (half)(INFINITY);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _f32, )(float x)
{
    return __builtin_spirv_OpenCL_fabs_f32(x) < (float)(INFINITY);
}

#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _f64, )(double x)
{
    return __builtin_spirv_OpenCL_fabs_f64(x) < (double)(INFINITY);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsFinite, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsFinite, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsFinite, __bool, double, f64)
#endif

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _f16, )(half x)
{
    return SPIRV_BUILTIN(IsFinite, _f16, )(x) & (__builtin_spirv_OpenCL_fabs_f16(x) >= HALF_MIN);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _f32, )(float x)
{
    return SPIRV_BUILTIN(IsFinite, _f32, )(x) & (__builtin_spirv_OpenCL_fabs_f32(x) >= FLT_MIN);
}

#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _f64, )(double x)
{
    return SPIRV_BUILTIN(IsFinite, _f64, )(x) & (__builtin_spirv_OpenCL_fabs_f64(x) >= DBL_MIN);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNormal, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNormal, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNormal, __bool, double, f64)
#endif

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _f16, )(half x)
{
    return (as_short( x ) & HALF_SIGN_MASK) != 0;
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _f32, )(float x)
{
    return (as_int( x ) & FLOAT_SIGN_MASK) != 0;
}

#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _f64, )(double x)
{
    return (as_long( x ) & DOUBLE_SIGN_MASK) != 0;
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(SignBitSet, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(SignBitSet, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(SignBitSet, __bool, double, f64)
#endif

bool __builtin_spirv_OpLessOrGreater_f16_f16(half x, half y)
{
    return (x < y) | (x > y);
}

bool __builtin_spirv_OpLessOrGreater_f32_f32(float x, float y)
{
    return (x < y) | (x > y);
}

#if defined(cl_khr_fp64)
bool __builtin_spirv_OpLessOrGreater_f64_f64(double x, double y)
{
    return (x < y) | (x > y);
}
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpLessOrGreater, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpLessOrGreater, __bool, float,  f32)
#if defined(cl_khr_fp64)
GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpLessOrGreater, __bool, double, f64)
#endif

bool __builtin_spirv_OpOrdered_f16_f16(half x, half y)
{
    return (x == x) & (y == y);
}

bool __builtin_spirv_OpOrdered_f32_f32(float x, float y)
{
    return (x == x) & (y == y);
}

#if defined(cl_khr_fp64)
bool __builtin_spirv_OpOrdered_f64_f64(double x, double y)
{
    return (x == x) & (y == y);
}
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpOrdered, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpOrdered, __bool, float,  f32)
#if defined(cl_khr_fp64)
GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpOrdered, __bool, double, f64)
#endif

bool __builtin_spirv_OpUnordered_f16_f16(half x, half y)
{
    return (x != x) | (y != y);
}

bool __builtin_spirv_OpUnordered_f32_f32(float x, float y)
{
    return (x != x) | (y != y);
}

#if defined(cl_khr_fp64)
bool __builtin_spirv_OpUnordered_f64_f64(double x, double y)
{
    return (x != x) | (y != y);
}
#endif

GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpUnordered, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpUnordered, __bool, float,  f32)
#if defined(cl_khr_fp64)
GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpUnordered, __bool, double, f64)
#endif
