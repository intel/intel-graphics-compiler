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

bool  __builtin_spirv_OpIsNan_f16(half x)
{
    return x != x;
}

bool  __builtin_spirv_OpIsNan_f32(float x)
{
    return x != x;
}

#if defined(cl_khr_fp64)
bool  __builtin_spirv_OpIsNan_f64(double x)
{
    return x != x;
}
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNan, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNan, __bool, float,  f32)
#if defined(cl_khr_fp64)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNan, __bool, double, f64)
#endif

bool __builtin_spirv_OpIsInf_f16(half x)
{
    return __builtin_spirv_OpenCL_fabs_f16(x) == (half)(INFINITY);
}

bool __builtin_spirv_OpIsInf_f32(float x)
{
    return __builtin_spirv_OpenCL_fabs_f32(x) == (float)(INFINITY);
}

#if defined(cl_khr_fp64)
bool __builtin_spirv_OpIsInf_f64(double x)
{
    return __builtin_spirv_OpenCL_fabs_f64(x) == (double)(INFINITY);
}
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsInf, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsInf, __bool, float,  f32)
#if defined(cl_khr_fp64)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsInf, __bool, double, f64)
#endif

bool __builtin_spirv_OpIsFinite_f16(half x)
{
    return __builtin_spirv_OpenCL_fabs_f16(x) < (half)(INFINITY);
}

bool __builtin_spirv_OpIsFinite_f32(float x)
{
    return __builtin_spirv_OpenCL_fabs_f32(x) < (float)(INFINITY);
}

#if defined(cl_khr_fp64)
bool __builtin_spirv_OpIsFinite_f64(double x)
{
    return __builtin_spirv_OpenCL_fabs_f64(x) < (double)(INFINITY);
}
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsFinite, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsFinite, __bool, float,  f32)
#if defined(cl_khr_fp64)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsFinite, __bool, double, f64)
#endif

bool __builtin_spirv_OpIsNormal_f16(half x)
{
    return __builtin_spirv_OpIsFinite_f16(x) & (__builtin_spirv_OpenCL_fabs_f16(x) >= HALF_MIN);
}

bool __builtin_spirv_OpIsNormal_f32(float x)
{
    return __builtin_spirv_OpIsFinite_f32(x) & (__builtin_spirv_OpenCL_fabs_f32(x) >= FLT_MIN);
}

#if defined(cl_khr_fp64)
bool __builtin_spirv_OpIsNormal_f64(double x)
{
    return __builtin_spirv_OpIsFinite_f64(x) & (__builtin_spirv_OpenCL_fabs_f64(x) >= DBL_MIN);
}
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNormal, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNormal, __bool, float,  f32)
#if defined(cl_khr_fp64)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNormal, __bool, double, f64)
#endif

bool __builtin_spirv_OpSignBitSet_f16(half x)
{
    return (as_short( x ) & HALF_SIGN_MASK) != 0;
}

bool __builtin_spirv_OpSignBitSet_f32(float x)
{
    return (as_int( x ) & FLOAT_SIGN_MASK) != 0;
}

#if defined(cl_khr_fp64)
bool __builtin_spirv_OpSignBitSet_f64(double x)
{
    return (as_long( x ) & DOUBLE_SIGN_MASK) != 0;
}
#endif

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpSignBitSet, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpSignBitSet, __bool, float,  f32)
#if defined(cl_khr_fp64)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpSignBitSet, __bool, double, f64)
#endif

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _f16_f16, )(half x, half y)
{
    return (x < y) | (x > y);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _f32_f32, )(float x, float y)
{
    return (x < y) | (x > y);
}

#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _f64_f64, )(double x, double y)
{
    return (x < y) | (x > y);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(LessOrGreater, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(LessOrGreater, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(LessOrGreater, __bool, double, f64)
#endif

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _f16_f16, )(half x, half y)
{
    return (x == x) & (y == y);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _f32_f32, )(float x, float y)
{
    return (x == x) & (y == y);
}

#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _f64_f64, )(double x, double y)
{
    return (x == x) & (y == y);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Ordered, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Ordered, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Ordered, __bool, double, f64)
#endif

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _f16_f16, )(half x, half y)
{
    return (x != x) | (y != y);
}

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _f32_f32, )(float x, float y)
{
    return (x != x) | (y != y);
}

#if defined(cl_khr_fp64)
bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _f64_f64, )(double x, double y)
{
    return (x != x) | (y != y);
}
#endif

SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Unordered, __bool, half,   f16)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Unordered, __bool, float,  f32)
#if defined(cl_khr_fp64)
SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Unordered, __bool, double, f64)
#endif
