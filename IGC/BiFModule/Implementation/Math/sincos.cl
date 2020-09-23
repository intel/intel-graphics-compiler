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
#include "../SVMLReleaseOnly/svml/Math/svml_sincos.cl"

    #include "../ExternalLibraries/libclc/trig.cl"

static INLINE float __intel_sincos_f32_p0f32( float x, __private float* cosval, bool doFast )
{
    float   sin_x, cos_x;
    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        sin_x = __builtin_spirv_OpenCL_native_sin_f32(x);
        cos_x = __builtin_spirv_OpenCL_native_cos_f32(x);
    }
    else
    {
            float abs_float = __builtin_spirv_OpenCL_fabs_f32(x);
            if( abs_float > 10000.0f )
            {
                sin_x = libclc_sin_f32(x);
                cos_x = libclc_cos_f32(x);
            }
            else
            {
                sin_x = precise_sincosf(x, &cos_x);
            }
    }
    *cosval = cos_x;
    return sin_x;
}

INLINE float __builtin_spirv_OpenCL_sincos_f32_p0f32( float x, __private float* cosval )
{
    return __intel_sincos_f32_p0f32(x, cosval, true);
}

GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( __builtin_spirv_OpenCL_sincos, float, float, float, f32, f32 )

float __builtin_spirv_OpenCL_sincos_f32_p1f32( float           x,
                                        __global float* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

INLINE float __builtin_spirv_OpenCL_sincos_f32_p3f32( float          x,
                                        __local float* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, float, __global, float, f32, p1 )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, float, __local, float, f32, p3 )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __builtin_spirv_OpenCL_sincos_f32_p4f32( float            x,
                                        __generic float* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, float, __generic, float, f32, p4 )

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_sincos_f16_p0f16( half            x,
                                       __private half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( __builtin_spirv_OpFConvert_f32_f16(x), &cos_x );
    cosval[0] = __builtin_spirv_OpFConvert_f16_f32(cos_x);
    return __builtin_spirv_OpFConvert_f16_f32(sin_x);
}

GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( __builtin_spirv_OpenCL_sincos, half, half, half, f16, f16 )

INLINE half __builtin_spirv_OpenCL_sincos_f16_p1f16( half           x,
                                       __global half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( __builtin_spirv_OpFConvert_f32_f16(x), &cos_x );
    cosval[0] = __builtin_spirv_OpFConvert_f16_f32(cos_x);
    return __builtin_spirv_OpFConvert_f16_f32(sin_x);
}

INLINE half __builtin_spirv_OpenCL_sincos_f16_p3f16( half          x,
                                       __local half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( __builtin_spirv_OpFConvert_f32_f16(x), &cos_x );
    cosval[0] = __builtin_spirv_OpFConvert_f16_f32(cos_x);
    return __builtin_spirv_OpFConvert_f16_f32(sin_x);
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, half, __global, half, f16, p1 )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, half, __local, half, f16, p3 )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __builtin_spirv_OpenCL_sincos_f16_p4f16( half            x,
                                       __generic half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( __builtin_spirv_OpFConvert_f32_f16(x), &cos_x );
    cosval[0] = __builtin_spirv_OpFConvert_f16_f32(cos_x);
    return __builtin_spirv_OpFConvert_f16_f32(sin_x);
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, half, __generic, half, f16, p4 )

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp16)

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_sincos_f64_p0f64( double            x,
                                         __private double* cosval )
{
    double sin_x, cos_x;

        sin_x = __builtin_spirv_OpenCL_sin_f64(x);
        cos_x = __builtin_spirv_OpenCL_cos_f64(x);

    *cosval = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( __builtin_spirv_OpenCL_sincos, double, double, double, f64, f64 )

double __builtin_spirv_OpenCL_sincos_f64_p3f64( double          x,
                                         __local double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f64_p0f64( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

INLINE double __builtin_spirv_OpenCL_sincos_f64_p1f64( double           x,
                                         __global double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f64_p0f64( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, double, __global, double, f64, p1 )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, double, __local, double, f64, p3 )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE double __builtin_spirv_OpenCL_sincos_f64_p4f64( double          x,
                                         __generic double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f64_p0f64( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, double, __generic, double, f64, p4 )

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp64)
