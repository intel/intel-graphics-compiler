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

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"


#define _M_LOG2_10      (as_float(0x40549A78))          // 3.321928094887362f
#define _M_LOG10_2      (as_float(0x3e9a209b))          // 0.30103000998497009f

// cos
INLINE double __builtin_spirv_OpenCL_native_cos_f64( double x )
{
    float f = (float)x;
    return __builtin_spirv_OpenCL_native_cos_f32(f);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_cos, double, double, f64 )

// divide

INLINE double __builtin_spirv_OpenCL_native_divide_f64_f64( double x, double y )
{
    return x * __builtin_spirv_OpenCL_native_recip_f64( y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_native_divide, double, double, f64 )

// exp

INLINE double __builtin_spirv_OpenCL_native_exp_f64( double x )
{
    return __builtin_spirv_OpenCL_native_exp2_f32((float)x * M_LOG2E_F);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_exp, double, double, f64 )

// exp10

INLINE double __builtin_spirv_OpenCL_native_exp10_f64( double x )
{
    return __builtin_spirv_OpenCL_native_exp2_f32( (float)x * (float)(_M_LOG2_10) );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_exp10, double, double, f64 )

// exp2

INLINE double __builtin_spirv_OpenCL_native_exp2_f64( double x )
{
    return __builtin_spirv_OpenCL_native_exp2_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_exp2, double, double, f64 )

// log

INLINE double __builtin_spirv_OpenCL_native_log_f64( double x )
{
    return __builtin_spirv_OpenCL_native_log2_f32((float)x) * M_LN2_F;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_log, double, double, f64 )

// log10

INLINE double __builtin_spirv_OpenCL_native_log10_f64( double x )
{
    return __builtin_spirv_OpenCL_native_log2_f32((float)x) * _M_LOG10_2;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_log10, double, double, f64 )

// log2

INLINE double __builtin_spirv_OpenCL_native_log2_f64( double x )
{
    float f = (float)x;
    return __builtin_spirv_OpenCL_native_log2_f32(f);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_log2, double, double, f64 )

// powr

INLINE double __builtin_spirv_OpenCL_native_powr_f64_f64( double x, double y )
{
    return __builtin_spirv_OpenCL_native_powr_f32_f32((float)x, (float)y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_native_powr, double, double, f64 )

// recip

INLINE double __builtin_spirv_OpenCL_native_recip_f64( double x )
{
    return __builtin_spirv_OpenCL_native_recip_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_recip, double, double, f64 )

// rsqrt

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_rsqrt, double, double, f64 )

// sin

INLINE double __builtin_spirv_OpenCL_native_sin_f64( double x )
{
    return __builtin_spirv_OpenCL_native_sin_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_sin, double, double, f64 )

// sqrt

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_sqrt, double, double, f64 )

// tan

INLINE double __builtin_spirv_OpenCL_native_tan_f64( double x )
{
    return __builtin_spirv_OpenCL_native_tan_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_native_tan, double, double, f64 )