/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE sincos( float            x,
                           __private float* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f32_p0f32, )( x, cosval );
}

GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( sincos, float, float, float )

float OVERLOADABLE sincos( float           x,
                           __global float* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f32_p1f32, )( x, cosval );
}

INLINE float OVERLOADABLE sincos( float          x,
                           __local float* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f32_p3f32, )( x, cosval );
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, float, __global, float )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, float, __local, float )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float OVERLOADABLE sincos( float            x,
                           __generic float* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f32_p4f32, )( x, cosval );
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, float, __generic, float )

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE sincos( half            x,
                          __private half* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f16_p0f16, )( x, cosval );
}

GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( sincos, half, half, half )

INLINE half OVERLOADABLE sincos( half           x,
                          __global half* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f16_p1f16, )( x, cosval );
}

INLINE half OVERLOADABLE sincos( half          x,
                          __local half* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f16_p3f16, )( x, cosval );
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, half, __global, half )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, half, __local, half )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half OVERLOADABLE sincos( half            x,
                          __generic half* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f16_p4f16, )( x, cosval );
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, half, __generic, half )

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp16)

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE sincos( double            x,
                            __private double* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f64_p0f64, )( x, cosval );
}

GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( sincos, double, double, double )

double OVERLOADABLE sincos( double          x,
                            __local double* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f64_p3f64, )( x, cosval );
}

INLINE double OVERLOADABLE sincos( double           x,
                            __global double* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f64_p1f64, )( x, cosval );
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, double, __local, double )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, double, __global, double )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE double OVERLOADABLE sincos( double          x,
                            __generic double* cosval )
{
    return SPIRV_OCL_BUILTIN(sincos, _f64_p4f64, )( x, cosval );
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( sincos, double, __generic, double )

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp64)
