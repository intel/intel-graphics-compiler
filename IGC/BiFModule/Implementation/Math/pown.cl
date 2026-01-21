/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/pown_s_la.cl"
#include "../IMF/FP32/pown_s_prev.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/pown_d_la.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_pown( float x, int y )
{
    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)))
    {
        // Undefined for x = 0 and y = 0.
        // For x >= 0, or x < 0 and even y, derived implementations implement this as:
        //    exp2(y * log2(x)).
        // For x < 0 and odd y, derived implementations implement this as:
        //    -exp2(y * log2(fabs(x)).
        //
        // This expansion is technically undefined when x == 0, since
        // log2(x) is undefined, however our native log2 returns -inf
        // in this case.  Since exp2( y * -inf ) is zero for finite y,
        // we'll end up with zero, hence the "correct" results.\

        float   pr = __spirv_ocl_fabs( x );

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

        // Check for a positive x by checking the sign bit as an integer,
        // not a float.  This correctly handles x = -0.0f.  Arguably, we
        // don't have to do this since -cl-fast-relaxed-math implies
        // -cl-no-signed-zeros, but it's easy enough to do.
        float nr = -pr;
        nr = ( y & 1 ) ? nr : pr;                   // positive result for even y, else negative result
        float result = ( as_int(x) >= 0 ) ? pr : nr;// positive result for positive x, else unchanged

        return result;
    }
    else
    {
        // Previous version of pown builtin is called here, because new one introduced some critical functional regressions.
        // TODO: The target is to call '__ocl_svml_pownf' here.
        return __ocl_svml_px_pownf1(x, y);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pown, float, float, int, f32, i32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_pown( double x, int y )
{
    return __ocl_svml_pown(x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pown, double, double, int, f64, i32 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_pown( half x, int y )
{
    return __spirv_ocl_pown((float)x, y);
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pown, half, half, int, f16, i32 )

#endif // defined(cl_khr_fp16)

