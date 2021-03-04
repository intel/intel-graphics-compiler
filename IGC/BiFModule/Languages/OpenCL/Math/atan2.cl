/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

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

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

// TODO: I think we should be able to use M_PI_F here instead of FLOAT_PI,
// but this does cause very small differences in the results of atan2(),
// since M_PI_F is rounded (and therefore slightly larger than FLOAT_PI).
// We'll need to re-collect some GITS streams if we want to use M_PI_F
// instead.
#define FLOAT_PI                (as_float(0x40490FDA)) // 3.1415926535897930f

float OVERLOADABLE atan2( float y, float x )
{
    return __builtin_spirv_OpenCL_atan2_f32_f32( y, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( atan2, float, float, float )

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE atan2( double y, double x )
{
    return __builtin_spirv_OpenCL_atan2_f64_f64( y, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( atan2, double, double, double )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE atan2( half y, half x )
{
    return __builtin_spirv_OpenCL_atan2_f16_f16( y, x );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( atan2, half, half, half )

#endif // defined(cl_khr_fp16)
