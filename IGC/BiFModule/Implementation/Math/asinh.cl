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

#include "../IMF/FP32/asinh_s_la.cl"

INLINE float __intel_asinh_f32( float x, bool doFast )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        // Implemented as log(x + sqrt(x*x + 1)).
        // Conformance test checks for this "overflow" case, but
        // I don't think we should have to handle it.
        if( x > 1500.0f )
        {
            result = __builtin_spirv_OpenCL_log_f32(x) + M_LN2_F;
        }
        else
        {
            result = x * x + 1.0f;
            result = __builtin_spirv_OpenCL_sqrt_f32( result );
            result = x + result;
            result = __builtin_spirv_OpenCL_log_f32( result );
        }
    }
    else
    {
        result = __ocl_svml_asinhf(x);
    }

    return result;
}

INLINE float __builtin_spirv_OpenCL_asinh_f32( float x )
{
    return __intel_asinh_f32( x, true );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_asinh, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_asinh_f16( half x )
{
    return __builtin_spirv_OpenCL_asinh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_asinh, half, half, f16 )

#endif // defined(cl_khr_fp16)
