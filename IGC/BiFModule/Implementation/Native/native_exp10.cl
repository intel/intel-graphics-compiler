/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#define _M_LOG2_10      (as_float(0x40549A78))          // 3.321928094887362f
#define _M_LOG2_10_DBL  (as_double(0x400a934f0979a371)) // 3.3219280948873623478703194

INLINE float __attribute__((overloadable)) __spirv_ocl_native_exp10( float x )
{
    return __spirv_ocl_native_exp2( x * (float)(_M_LOG2_10) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp10, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_native_exp10( double x )
{
    return __spirv_ocl_native_exp2( (float)x * (float)(_M_LOG2_10) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp10, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_native_exp10( half x )
{
    return __spirv_ocl_native_exp2( x * (half)(_M_LOG2_10) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp10, half, half, f16 )

#endif // defined(cl_khr_fp16)

#if defined(IGC_SPV_INTEL_bfloat16_arithmetic)
INLINE bfloat __attribute__((overloadable)) __spirv_ocl_native_exp10( bfloat x )
{
    // Regular multiplication is upconverted to float by clang.
    // return __spirv_ocl_native_exp2( x * (bfloat)(_M_LOG2_10) );
    bfloat bf16_LOG2_10 = (bfloat)_M_LOG2_10;
    bfloat result = as_bfloat(__builtin_bf16_mul(as_ushort(x), as_ushort(bf16_LOG2_10)));
    result = __spirv_ocl_native_exp2(result);
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp10, bfloat, bfloat, )
#endif // defined(IGC_SPV_INTEL_bfloat16_arithmetic)
