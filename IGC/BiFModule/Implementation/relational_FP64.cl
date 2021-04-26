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

bool  SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNan, _f64, )(double x)
{
    return x != x;
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNan, __bool, double, f64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsInf, _f64, )(double x)
{
    return __builtin_spirv_OpenCL_fabs_f64(x) == (double)(INFINITY);
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsInf, __bool, double, f64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsFinite, _f64, )(double x)
{
    return __builtin_spirv_OpenCL_fabs_f64(x) < (double)(INFINITY);
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsFinite, __bool, double, f64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(IsNormal, _f64, )(double x)
{
    return SPIRV_BUILTIN(IsFinite, _f64, )(x) & (__builtin_spirv_OpenCL_fabs_f64(x) >= DBL_MIN);
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(IsNormal, __bool, double, f64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(SignBitSet, _f64, )(double x)
{
    return (as_long( x ) & DOUBLE_SIGN_MASK) != 0;
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_1ARG(SignBitSet, __bool, double, f64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(LessOrGreater, _f64_f64, )(double x, double y)
{
    return (x < y) | (x > y);
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(LessOrGreater, __bool, double, f64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Ordered, _f64_f64, )(double x, double y)
{
    return (x == x) & (y == y);
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Ordered, __bool, double, f64)

bool SPIRV_OVERLOADABLE SPIRV_BUILTIN(Unordered, _f64_f64, )(double x, double y)
{
    return (x != x) | (y != y);
}

SPIRV_GENERATE_VECTOR_FUNCTIONS_2ARGS(Unordered, __bool, double, f64)
