/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"
#include "../IMF/FP32/pow_s_la.cl"
#include "../IMF/FP32/pow_s_la_noLUT.cl"
#include "../IMF/FP32/pow_s_prev.cl"

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/pow_d_la.cl"
    #include "../IMF/FP64/pow_d_la_noLUT.cl"
#endif // defined(cl_khr_fp64)

INLINE float __attribute__((overloadable)) __spirv_ocl_pow( float x, float y )
{
    if(BIF_FLAG_CTRL_GET(FastRelaxedMath) && (!BIF_FLAG_CTRL_GET(APIRS)))
    {
        // Undefined for x = 0 and y = 0.
        // Undefined for x < 0 and noninteger y.
        // For x >= 0, or x < 0 and even y, derived implementations implement this as:
        //    exp2(y * log2(x)).
        // For x < 0 and odd y, derived implementations implement this as:
        //    -exp2(y * log2(fabs(x)).
        //
        // This expansion is technically undefined when x == 0, since
        // log2(x) is undefined, however our native log2 returns -inf
        // in this case.  Since exp2( y * -inf ) is zero for finite y,
        // we'll end up with zero, hence the "correct" results.

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
        int iy = (int)y;
        float nr = -pr;
        nr = ( iy & 1 ) ? nr : pr;                  // positive result for even y, else negative result
        float result = ( as_int(x) >= 0 ) ? pr : nr;// positive result for positive x, else unchanged

        return result;
    }
    else if (BIF_FLAG_CTRL_GET(UseHighAccuracyMath))
    {
        return __ocl_svml_powf_noLUT(x, y);
    }
    else
    {
        bool precisionWA =
            ( ( as_uint(y) == 0x39D0A3C4 ) | ( as_uint(y) == 0x3ada1700 ) | ( as_uint(y) == 0x3b81ef38 ) | ( as_uint(y) == 0x3b2434e1 ) | ( as_uint(y) == 0x3D7319F7 ) ) &
            ( ( as_uint(x) >= 0x3f7ff65f ) & ( as_uint(x) <  0x3F800000 ) );
        if ( precisionWA )
        {
            return as_float(0x3F7FFFFF);
        }
        // Previous version of pow builtin is called here, because new one introduced some critical functional regressions.
        // TODO: The target is to call '__ocl_svml_powf' here.
        return __ocl_svml_px_powf1(x, y);
    }
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pow, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

INLINE double __attribute__((overloadable)) __spirv_ocl_pow( double x, double y )
{
    double result;
    if (BIF_FLAG_CTRL_GET(UseHighAccuracyMath)) {
        result = __ocl_svml_pow_noLUT(x, y);
    } else {
        result = __ocl_svml_pow(x, y);
    }
    return result;
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pow, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __attribute__((overloadable)) __spirv_ocl_pow( half x, half y )
{
    return __spirv_ocl_pow((float)x, (float)y );
}

GENERATE_SPIRV_OCL_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( pow, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

