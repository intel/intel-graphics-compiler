/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#define _M_LOG10_2      (as_float(0x3e9a209b))          // 0.30103000998497009f
#define _M_LOG10_2_DBL  (as_double(0x3fd34413509f79ff)) // 0.3010299956639811952137388

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _f32, )( float x )
{
    return SPIRV_OCL_BUILTIN(native_log2, _f32, )(x) * _M_LOG10_2;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_log10, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _f64, )( double x )
{
    return SPIRV_OCL_BUILTIN(native_log2, _f32, )((float)x) * _M_LOG10_2;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_log10, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(native_log10, _f16, )( half x )
{
    return SPIRV_OCL_BUILTIN(native_log2, _f16, )(x) * (half)(_M_LOG10_2);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_1ARGS( native_log10, half, half, f16 )

#endif // defined(cl_khr_fp16)
