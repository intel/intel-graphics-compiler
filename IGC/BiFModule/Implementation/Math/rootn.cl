/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/rootn_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/rootn_d_la.cl"
#endif // defined(cl_khr_fp64)

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _f32_i32, )( float x, int n )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // Defined for x > 0 and n is nonzero.  Derived
        // implementations implement this as:
        //   exp2(log2( x) / n) for x > 0.
        // Defined for x < 0 and n is odd.  Derived
        // implementations implement this as:
        //  -exp2(log2(-x) / n) for x < 0.
        // Defined as +0 for x = +/-0 and y > 0.
        // Undefined for all other cases.

        float   pr = x;

        // TBD: Which is faster?
        // Note that USC has a pattern match optimization to turn
        // log-mul-exp into pow.  Additionally, there are some specific
        // LLVM optimizations for pow.  So, preferring pow for now.
#if 0
        pr = SPIRV_OCL_BUILTIN(fabs, _f32, )( pr );
        pr = SPIRV_OCL_BUILTIN(log2, _f32, )( pr );
        pr = pr * 1.0f / n;
        pr = SPIRV_OCL_BUILTIN(exp2, _f32, )( pr );
#else
        pr = SPIRV_OCL_BUILTIN(native_powr, _f32_f32, )( pr, 1.0f / n );
#endif

        // For rootn(), we'll return the positive result for both +0.0f and -0.0f.
        float nr = -pr;
        result = ( x >= 0.0f ) ? pr : nr;   // positive result for non-negative x, else negative result
    }
    else
    {
        result = __ocl_svml_rootnf(x, n);
    }
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( rootn, float, float, int, f32, i32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _f64_i32, )( double y, int x )
{
    return __ocl_svml_rootn_v2(y, x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( rootn, double, double, int, f64, i32 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(rootn, _f16_i32, )( half y, int x )
{
    return SPIRV_OCL_BUILTIN(rootn, _f32_i32, )((float)y, x);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( rootn, half, half, int, f16, i32 )

#endif // defined(cl_khr_fp16)
