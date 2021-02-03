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

#define SQRT_2PI                (as_float(0x40206C98)) // 2.5066282746310007f

// Computes the gamma functions using a Lanczos approximation:
static float __intel_gamma(float z)
{
    float p0 = as_float(0x3f800000);    // 1.0f
    float p1 = as_float(0x42985c35);    // 76.180092f
    float p2 = as_float(0xc2ad02b9);    // -86.505318f
    float p3 = as_float(0x41c01ce0);    // 24.014099
    float p4 = as_float(0xbf9da9a4);    // -1.2317395
    float p5 = as_float(0x3a9e6b99);    // 1.2086510e-3f
    float p6 = as_float(0xb6b508c1);    // -5.3952394e-6f
    float g = 5.0f; // number of coefficients - 2

    z -= 1;

    float x = p0;
    x += p1 / (z + 1);
    x += p2 / (z + 2);
    x += p3 / (z + 3);
    x += p4 / (z + 4);
    x += p5 / (z + 5);
    x += p6 / (z + 6);

    float t = z + g + 0.5f;
    return SQRT_2PI * __builtin_spirv_OpenCL_pow_f32_f32(t, z + 0.5f) * __builtin_spirv_OpenCL_exp_f32(-t) * x;
}

float __builtin_spirv_OpenCL_tgamma_f32( float x )
{
    float ret;
    if ( (x < 0.0f) & (x == __builtin_spirv_OpenCL_floor_f32(x))) {
        ret = __builtin_spirv_OpenCL_nan_i32((uint)0);
    } else {
        float y = 1.0f - x;
        float z = ( x < 0.5f ) ? y : x;
        // Note: z >= 0.5f.
        float g = __intel_gamma(z);

        ret = ( x < 0.5f ) ?
            M_PI_F / ( __builtin_spirv_OpenCL_sinpi_f32(x) * g ) :
            g;

        // Special handling for -0.0f.
        // It may be possible to restrict this to renderscript only,
        // but for now we'll apply it across the board to stay on
        // the safe side, since this built-in is used infrequently.
        ret = ( as_uint(x) == FLOAT_SIGN_MASK ) ? -INFINITY : ret;
    }
    return ret;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tgamma, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_tgamma_f64( double x )
{
    return libclc_tgamma_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tgamma, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_tgamma_f16( half x )
{
    return __builtin_spirv_OpenCL_tgamma_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tgamma, half, half, f16 )

#endif // defined(cl_khr_fp16)
