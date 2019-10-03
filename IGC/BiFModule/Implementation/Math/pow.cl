/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/


#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

#include "../SVMLReleaseOnly/svml/Math/svml_pow.cl"

#if defined(cl_khr_fp64)
    #include "../ExternalLibraries/sun/sun_pow.cl"
#endif // defined(cl_khr_fp64)

INLINE float __builtin_spirv_OpenCL_pow_f32_f32( float x, float y )
{
    if(__FastRelaxedMath && (!__APIRS))
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

        float   pr = __builtin_spirv_OpenCL_fabs_f32( x );

        // TBD: Which is faster?
        // Note that USC has a pattern match optimization to turn
        // log-mul-exp into pow.  Additionally, there are some specific
        // LLVM optimizations for pow.  So, preferring pow for now.
#if 0
        pr = __builtin_spirv_OpenCL_log2_f32( pr );
        pr = y * pr;
        pr = __builtin_spirv_OpenCL_exp2_f32( pr );
#else
        pr = __builtin_spirv_OpenCL_native_powr_f32_f32( pr, y );
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
    else
    {
        bool precisionWA =
            ( ( as_uint(y) == 0x39D0A3C4 ) | ( as_uint(y) == 0x3ada1700 ) | ( as_uint(y) == 0x3b81ef38 ) | ( as_uint(y) == 0x3b2434e1 ) | ( as_uint(y) == 0x3D7319F7 ) ) &
            ( ( as_uint(x) >= 0x3f7ff65f ) & ( as_uint(x) <  0x3F800000 ) );
        if ( precisionWA )
        {
            return as_float(0x3F7FFFFF);
        }
        return __ocl_svml_px_powf1(x, y);
    }
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pow, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_pow_f64_f64( double x, double y )
{
        return sun_pow(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pow, double, double, double, f64, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_pow_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_pow_f32_f32((float)x, (float)y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pow, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)
