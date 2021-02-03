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

float __builtin_spirv_OpenCL_logb_f32( float x )
{
    float result = 0.0f;

    if( __intel_relaxed_isnormal( x ) )
    {
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS;
    }
    else if( __intel_relaxed_isnan( x ) )
    {
        result = __builtin_spirv_OpenCL_nan_i32(0U);
    }
    else if( __intel_relaxed_isinf( x ) )
    {
        result = INFINITY;
    }
    else if( x == 0.0f )
    {
        result = -INFINITY;
    }
    else
    {
        x = x * ( 1 << FLOAT_MANTISSA_BITS );
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS - FLOAT_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_logb, float, float, f32 )

#if defined(cl_khr_fp64)

double __builtin_spirv_OpenCL_logb_f64( double x )
{
    double result = 0.0;

    if( __builtin_spirv_OpIsNormal_f64( x ) )
    {
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS;
    }
    else if( __builtin_spirv_OpIsNan_f64( x ) )
    {
        result = __builtin_spirv_OpenCL_nan_i64(0UL);
    }
    else if( __builtin_spirv_OpIsInf_f64( x ) )
    {
        result = INFINITY;
    }
    else if( x == 0.0 )
    {
        result = -INFINITY;
    }
    else
    {
        x = x * ( 1UL << DOUBLE_MANTISSA_BITS );
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_logb, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_logb_f16( half x )
{
    return (half)__builtin_spirv_OpenCL_logb_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_logb, half, half, f16 )

#endif // defined(cl_khr_fp16)
