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

int __builtin_spirv_OpenCL_ilogb_f32( float x )
{
    int result = 0;

    if( __intel_relaxed_isnormal( x ) )
    {
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS;
    }
    else if( __intel_relaxed_isnan( x ) |
             __intel_relaxed_isinf( x ) )
    {
        result = FP_ILOGBNAN;
    }
    else if( x == 0.0f )
    {
        result = FP_ILOGB0;
    }
    else
    {
        x = x * ( 1 << FLOAT_MANTISSA_BITS );
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS - FLOAT_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_ilogb, int, float, f32 )

#if defined(cl_khr_fp64)

int __builtin_spirv_OpenCL_ilogb_f64( double x )
{
    int result = 0;

    if( __builtin_spirv_OpIsNormal_f64( x ) )
    {
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS;
    }
    else if( __builtin_spirv_OpIsNan_f64( x ) | __builtin_spirv_OpIsInf_f64( x ) )
    {
        result = FP_ILOGBNAN;
    }
    else if( x == 0.0 )
    {
        result = FP_ILOGB0;
    }
    else
    {
        x = x * ( 1UL << DOUBLE_MANTISSA_BITS );
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_ilogb, int, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE int __builtin_spirv_OpenCL_ilogb_f16( half x )
{
    return __builtin_spirv_OpenCL_ilogb_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_ilogb, int, half, f16 )

#endif // defined(cl_khr_fp16)
