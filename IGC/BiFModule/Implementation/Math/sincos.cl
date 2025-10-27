/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/sincos_s_la.cl"
#include "../IMF/FP32/sincos_s_noLUT.cl"
#include "../ExternalLibraries/libclc/trig.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/sincos_d_la.cl"
    #include "../IMF/FP64/sincos_d_la_noLUT.cl"
#endif

static INLINE float __intel_sincos_f32_p0f32( float x, __private float* cosval, bool doFast )
{
    float   sin_x, cos_x;
    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)) && doFast)
    {
        sin_x = __spirv_ocl_native_sin(x);
        cos_x = __spirv_ocl_native_cos(x);
    }
    else  if(BIF_FLAG_CTRL_GET(UseHighAccuracyMath))
    {
        sin_x = __ocl_svml_sincosf_noLUT(x, &cos_x);
    }
    else  if(BIF_FLAG_CTRL_GET(UseMathWithLUT))
    {
        __ocl_svml_sincosf(x, &sin_x, &cos_x);
    }
    else
    {
        float abs_float = __spirv_ocl_fabs(x);
        if( abs_float > 10000.0f )
        {
            sin_x = libclc_sin_f32(x);
            cos_x = libclc_cos_f32(x);
        }
        else
        {
            sin_x = __ocl_svml_sincosf_noLUT(x, &cos_x);
        }
    }
    *cosval = cos_x;
    return sin_x;
}

INLINE float __attribute__((overloadable)) __spirv_ocl_sincos( float x, __private float* cosval )
{
    return __intel_sincos_f32_p0f32(x, cosval, true);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( sincos, float, float, float, f32, f32 )

float __attribute__((overloadable)) __spirv_ocl_sincos( float           x,
                                        __global float* cosval )
{
    float   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

INLINE float __attribute__((overloadable)) __spirv_ocl_sincos( float          x,
                                        __local float* cosval )
{
    float   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, float, __global, float, f32, p1 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, float, __local, float, f32, p3 )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __attribute__((overloadable)) __spirv_ocl_sincos( float            x,
                                        __generic float* cosval )
{
    float   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, float, __generic, float, f32, p4 )

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_sincos( half            x,
                                       __private half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( __spirv_FConvert_Rfloat(x), &cos_x );
    cosval[0] = __spirv_FConvert_Rhalf(cos_x);
    return __spirv_FConvert_Rhalf(sin_x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( sincos, half, half, half, f16, f16 )

INLINE half __attribute__((overloadable)) __spirv_ocl_sincos( half           x,
                                       __global half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( __spirv_FConvert_Rfloat(x), &cos_x );
    cosval[0] = __spirv_FConvert_Rhalf(cos_x);
    return __spirv_FConvert_Rhalf(sin_x);
}

INLINE half __attribute__((overloadable)) __spirv_ocl_sincos( half          x,
                                       __local half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( __spirv_FConvert_Rfloat(x), &cos_x );
    cosval[0] = __spirv_FConvert_Rhalf(cos_x);
    return __spirv_FConvert_Rhalf(sin_x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, half, __global, half, f16, p1 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, half, __local, half, f16, p3 )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __attribute__((overloadable)) __spirv_ocl_sincos( half            x,
                                       __generic half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( __spirv_FConvert_Rfloat(x), &cos_x );
    cosval[0] = __spirv_FConvert_Rhalf(cos_x);
    return __spirv_FConvert_Rhalf(sin_x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, half, __generic, half, f16, p4 )

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp16)

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_sincos( double            x,
                                         __private double* cosval )
{
    double sin_x, cos_x;

    if (BIF_FLAG_CTRL_GET(UseHighAccuracyMath)) {
        __ocl_svml_sincos_noLUT(x, &sin_x, &cos_x);
    } else {
        __ocl_svml_sincos(x, &sin_x, &cos_x);
    }

    *cosval = cos_x;
    return sin_x;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( sincos, double, double, double, f64, f64 )

double __attribute__((overloadable)) __spirv_ocl_sincos( double          x,
                                         __local double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

INLINE double __attribute__((overloadable)) __spirv_ocl_sincos( double           x,
                                         __global double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, double, __global, double, f64, p1 )
GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, double, __local, double, f64, p3 )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE double __attribute__((overloadable)) __spirv_ocl_sincos( double          x,
                                         __generic double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __spirv_ocl_sincos( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, double, __generic, double, f64, p4 )

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp64)
