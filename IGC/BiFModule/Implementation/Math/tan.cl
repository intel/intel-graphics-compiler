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

    #include "../ExternalLibraries/libclc/trig.cl"
    #if defined(cl_khr_fp64)
        #include "../SVMLReleaseOnly/svml/MathDouble/rf_tan_d_1_g.cl"
    #endif // defined(cl_khr_fp64)

static INLINE float __intel_tan_f32( float x, bool doFast )
{
    float result;
    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        result = __builtin_spirv_OpenCL_native_tan_f32(x);
    }
    else
    {
        if(as_uint(x) == 0x45753168)
        {
            result = as_float(0xBF73F75D);
        }
        else if(as_uint(x) == 0xC5753168)
        {
            result = as_float(0x3F73F75D);
        }
        else
        {
                result = libclc_tan_f32(x);
        }
    }
    return result;
}

INLINE float __builtin_spirv_OpenCL_tan_f32( float x )
{
    return __intel_tan_f32(x, true);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_tan_f64( double x )
{
    return __ocl_svml_rf_tan1(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_tan_f16( half x )
{
    return __builtin_spirv_OpenCL_tan_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, half, half, f16 )

#endif // defined(cl_khr_fp16)
