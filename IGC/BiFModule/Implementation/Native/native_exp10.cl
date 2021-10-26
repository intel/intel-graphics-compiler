/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#define _M_LOG2_10      (as_float(0x40549A78))          // 3.321928094887362f
#define _M_LOG2_10_DBL  (as_double(0x400a934f0979a371)) // 3.3219280948873623478703194

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _f32, )( float x )
{
    return SPIRV_OCL_BUILTIN(native_exp2, _f32, )( x * (float)(_M_LOG2_10) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp10, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _f64, )( double x )
{
    return SPIRV_OCL_BUILTIN(native_exp2, _f32, )( (float)x * (float)(_M_LOG2_10) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp10, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_exp10, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(native_exp2, _f16, )( x * (half)(_M_LOG2_10) );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_exp10, half, half, f16 )

#endif // defined(cl_khr_fp16)
