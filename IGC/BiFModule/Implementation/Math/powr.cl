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

    #include "../ExternalLibraries/sun/sun_pow.cl"

INLINE float __builtin_spirv_OpenCL_powr_f32_f32( float x, float y )
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
        pr = __builtin_spirv_OpenCL_log2_f32( pr );
        pr = y * pr;
        pr = __builtin_spirv_OpenCL_exp2_f32( pr );
#else
        pr = __builtin_spirv_OpenCL_native_powr_f32_f32( pr, y );
#endif

        // For powr(), we're guaranteed that x >= 0, so no need for
        // sign fixup.
        float result = pr;
        return result;
    }
    else
    {
            if ( (__builtin_spirv_OpSignBitSet_f32(x) && x != 0) ||
                 (x == 0 && y == 0) ||
                 (x == 1 && __builtin_spirv_OpIsInf_f32(y)) ||
                 (__builtin_spirv_OpIsInf_f32(x) && y == 0) ||
                 __builtin_spirv_OpIsNan_f32(x) ||
                 __builtin_spirv_OpIsNan_f32(y) )
                return __builtin_spirv_OpenCL_nan_i32(0);
            else if (x == 0 && y < 0)
                return as_float(0x7f800000); //+infinity
            else
                return __builtin_spirv_OpenCL_pow_f32_f32( x, y );
    }
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_powr, float, float, float, f32, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_powr_f64_f64( double x, double y )
{
            if ( (__builtin_spirv_OpSignBitSet_f64(x) && x != 0) ||
                 (x == 0 && y == 0) ||
                 (x == 1 && __builtin_spirv_OpIsInf_f64(y)) ||
                 (__builtin_spirv_OpIsInf_f64(x) && y == 0) ||
                 __builtin_spirv_OpIsNan_f64(x) ||
                 __builtin_spirv_OpIsNan_f64(y) )
                return __builtin_spirv_OpenCL_nan_i64(0);
            else if (x == 0 && y < 0)
                return as_double(0x7ff0000000000000); //+infinity
            else
                return sun_pow( x, y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_powr, double, double, double, f64, f64 )

# endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_powr_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_powr_f32_f32((float)x, (float)y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_powr, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)
