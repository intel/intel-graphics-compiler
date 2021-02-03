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

// Relational and Logical Instructions


bool __builtin_spirv_OpAny_v2i8(__bool2 Vector)
{
    return (Vector.s0 | Vector.s1);
}

bool __builtin_spirv_OpAny_v3i8(__bool3 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2);
}

bool __builtin_spirv_OpAny_v4i8(__bool4 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2 | Vector.s3);
}

bool __builtin_spirv_OpAny_v8i8(__bool8 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2 | Vector.s3 |
            Vector.s4 | Vector.s5 | Vector.s6 | Vector.s7);
}

bool __builtin_spirv_OpAny_v16i8(__bool16 Vector)
{
    return (Vector.s0 | Vector.s1 | Vector.s2 | Vector.s3 |
            Vector.s4 | Vector.s5 | Vector.s6 | Vector.s7 |
            Vector.s8 | Vector.s9 | Vector.sa | Vector.sb |
            Vector.sc | Vector.sd | Vector.se | Vector.sf);
}

bool __builtin_spirv_OpAll_v2i8(__bool2 Vector)
{
    return (Vector.s0 & Vector.s1);
}

bool __builtin_spirv_OpAll_v3i8(__bool3 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2);
}

bool __builtin_spirv_OpAll_v4i8(__bool4 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2 & Vector.s3);
}

bool __builtin_spirv_OpAll_v8i8(__bool8 Vector)
{
    return (Vector.s0 & Vector.s1 & Vector.s2 & Vector.s3 &
            Vector.s4 & Vector.s5 & Vector.s6 & Vector.s7);
}

bool __builtin_spirv_OpAll_v16i8(__bool16 Vector)
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

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNan, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNan, __bool, float,  f32)

bool __builtin_spirv_OpIsInf_f16(half x)
{
    return __builtin_spirv_OpenCL_fabs_f16(x) == (half)(INFINITY);
}

bool __builtin_spirv_OpIsInf_f32(float x)
{
    return __builtin_spirv_OpenCL_fabs_f32(x) == (float)(INFINITY);
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsInf, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsInf, __bool, float,  f32)

bool __builtin_spirv_OpIsFinite_f16(half x)
{
    return __builtin_spirv_OpenCL_fabs_f16(x) < (half)(INFINITY);
}

bool __builtin_spirv_OpIsFinite_f32(float x)
{
    return __builtin_spirv_OpenCL_fabs_f32(x) < (float)(INFINITY);
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsFinite, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsFinite, __bool, float,  f32)

bool __builtin_spirv_OpIsNormal_f16(half x)
{
    return __builtin_spirv_OpIsFinite_f16(x) & (__builtin_spirv_OpenCL_fabs_f16(x) >= HALF_MIN);
}

bool __builtin_spirv_OpIsNormal_f32(float x)
{
    return __builtin_spirv_OpIsFinite_f32(x) & (__builtin_spirv_OpenCL_fabs_f32(x) >= FLT_MIN);
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNormal, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNormal, __bool, float,  f32)

bool __builtin_spirv_OpSignBitSet_f16(half x)
{
    return (as_short( x ) & HALF_SIGN_MASK) != 0;
}

bool __builtin_spirv_OpSignBitSet_f32(float x)
{
    return (as_int( x ) & FLOAT_SIGN_MASK) != 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpSignBitSet, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpSignBitSet, __bool, float,  f32)

bool __builtin_spirv_OpLessOrGreater_f16_f16(half x, half y)
{
    return (x < y) | (x > y);
}

bool __builtin_spirv_OpLessOrGreater_f32_f32(float x, float y)
{
    return (x < y) | (x > y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpLessOrGreater, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpLessOrGreater, __bool, float,  f32)

bool __builtin_spirv_OpOrdered_f16_f16(half x, half y)
{
    return (x == x) & (y == y);
}

bool __builtin_spirv_OpOrdered_f32_f32(float x, float y)
{
    return (x == x) & (y == y);
}


GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpOrdered, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpOrdered, __bool, float,  f32)

bool __builtin_spirv_OpUnordered_f16_f16(half x, half y)
{
    return (x != x) | (y != y);
}

bool __builtin_spirv_OpUnordered_f32_f32(float x, float y)
{
    return (x != x) | (y != y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpUnordered, __bool, half,   f16)
GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpUnordered, __bool, float,  f32)
