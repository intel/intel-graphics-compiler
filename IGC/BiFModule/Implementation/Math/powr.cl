/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/powr_s_la.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/powr_d_la.cl"
    #include "../IMF/FP64/powr_d_la_noLUT.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_powr( float x, float y )
{
    if(BIF_FLAG_CTRL_GET(FastRelaxedMath))
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
        pr = __spirv_ocl_log2( pr );
        pr = y * pr;
        pr = __spirv_ocl_exp2( pr );
#else
        pr = __spirv_ocl_native_powr( pr, y );
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

INLINE double __attribute__((overloadable)) __spirv_ocl_powr( double x, double y )
{
    double result;
    if (BIF_FLAG_CTRL_GET(UseHighAccuracyMath)) {
        result = __ocl_svml_powr_noLUT(x, y);
    } else {
        result = __ocl_svml_powr(x, y);
    }
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( powr, double, double, double, f64, f64 )

# endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_powr( half x, half y )
{
    return __spirv_ocl_powr((float)x, (float)y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( powr, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

