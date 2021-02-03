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

bool  __builtin_spirv_OpIsNan_f64(double x)
{
    return x != x;
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNan, __bool, double, f64)

bool __builtin_spirv_OpIsInf_f64(double x)
{
    return __builtin_spirv_OpenCL_fabs_f64(x) == (double)(INFINITY);
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsInf, __bool, double, f64)

bool __builtin_spirv_OpIsFinite_f64(double x)
{
    return __builtin_spirv_OpenCL_fabs_f64(x) < (double)(INFINITY);
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsFinite, __bool, double, f64)

bool __builtin_spirv_OpIsNormal_f64(double x)
{
    return __builtin_spirv_OpIsFinite_f64(x) & (__builtin_spirv_OpenCL_fabs_f64(x) >= DBL_MIN);
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpIsNormal, __bool, double, f64)

bool __builtin_spirv_OpSignBitSet_f64(double x)
{
    return (as_long( x ) & DOUBLE_SIGN_MASK) != 0;
}

GENERATE_VECTOR_FUNCTIONS_1ARG(__builtin_spirv_OpSignBitSet, __bool, double, f64)

bool __builtin_spirv_OpLessOrGreater_f64_f64(double x, double y)
{
    return (x < y) | (x > y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpLessOrGreater, __bool, double, f64)

bool __builtin_spirv_OpOrdered_f64_f64(double x, double y)
{
    return (x == x) & (y == y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpOrdered, __bool, double, f64)

bool __builtin_spirv_OpUnordered_f64_f64(double x, double y)
{
    return (x != x) | (y != y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS(__builtin_spirv_OpUnordered, __bool, double, f64)
