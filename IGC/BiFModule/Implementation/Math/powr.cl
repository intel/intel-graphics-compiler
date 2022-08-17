/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/powr_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/powr_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _f32_f32, )( float x, float y )
{
    if(__FastRelaxedMath)
    {
        // Undefined for x < 0.
        // Undefined for x = 0 and y = 0.
        // For x >= 0, derived implementations implement this as
        //    exp2(y * log2(x)).
        //
        // This expansion is technically undefined when x == 0, since
        // log2(x) is undefined, however our native log2 returns -inf
        // in this case.  Since exp2( y * -inf ) is zero for finite y,
        // we'll end up with zero, hence the "correct" results.

        // For powr(), we're guaranteed that x >= 0, so no need for fabs().
        float   pr = x;

        // TBD: Which is faster?
        // Note that USC has a pattern match optimization to turn
        // log-mul-exp into pow.  Additionally, there are some specific
        // LLVM optimizations for pow.  So, preferring pow for now.
#if 0
        pr = SPIRV_OCL_BUILTIN(log2, _f32, )( pr );
        pr = y * pr;
        pr = SPIRV_OCL_BUILTIN(exp2, _f32, )( pr );
#else
        pr = SPIRV_OCL_BUILTIN(native_powr, _f32_f32, )( pr, y );
#endif

        // For powr(), we're guaranteed that x >= 0, so no need for
        // sign fixup.
        float result = pr;
        return result;
    }
    else
    {
        return __ocl_svml_powrf(x, y);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( powr, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

INLINE double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _f64_f64, )( double x, double y )
{
    return __ocl_svml_powr(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( powr, double, double, double, f64, f64 )

# endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(powr, _f16_f16, )( half x, half y )
{
    return SPIRV_OCL_BUILTIN(powr, _f32_f32, )((float)x, (float)y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( powr, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)
