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

#include "IMF/FP32/acosh_s_la.cl"
#include "IMF/FP32/acospi_s_la.cl"
#include "IMF/FP32/asinh_s_la.cl"
#include "IMF/FP32/asinpi_s_la.cl"
#include "IMF/FP32/atanh_s_la.cl"
#include "IMF/FP32/atanpi_s_la.cl"
#include "IMF/FP32/cbrt_s_la.cl"
#include "IMF/FP32/cos_s_la.cl"
#include "IMF/FP32/cos_s_noLUT.cl"
#include "IMF/FP32/cosh_s_la.cl"
#include "IMF/FP32/cospi_s_la.cl"
#include "IMF/FP32/cospi_s_noLUT.cl"
#include "IMF/FP32/erf_s_la.cl"
#include "IMF/FP32/erfc_s_la.cl"
#include "IMF/FP32/exp10_s_la.cl"
#include "IMF/FP32/ln_s_la.cl"
#include "IMF/FP32/log1p_s_la.cl"
#include "IMF/FP32/log2_s_la.cl"
#include "IMF/FP32/log10_s_la.cl"
#include "IMF/FP32/pow_s_la.cl"
#include "IMF/FP32/pow_s_prev.cl"
#include "IMF/FP32/pown_s_la.cl"
#include "IMF/FP32/pown_s_prev.cl"
#include "IMF/FP32/powr_s_la.cl"
#include "IMF/FP32/rootn_s_la.cl"
#include "IMF/FP32/sin_s_la.cl"
#include "IMF/FP32/sin_s_noLUT.cl"
#include "IMF/FP32/sincos_s_la.cl"
#include "IMF/FP32/sincos_s_noLUT.cl"
#include "IMF/FP32/sinh_s_la.cl"
#include "IMF/FP32/sinpi_s_la.cl"
#include "IMF/FP32/sinpi_s_noLUT.cl"
#include "IMF/FP32/tan_s_la.cl"
#include "IMF/FP32/tanpi_s_la.cl"
#include "IMF/FP32/expm1_s_la.cl"
#include "../ExternalLibraries/libclc/trig.cl"
#include "../include/exp_for_hyper.cl"

// acos

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_acos, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_acos, half, half, f16 )

#endif // defined(cl_khr_fp16)

// acosh

float __builtin_spirv_OpenCL_acosh_f32( float x )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // Implemented as log(x + sqrt(x*x - 1)).

#if 1
        // Conformance test checks for NaN, but I don't think we should
        // have to handle this case.
        if( x < 1.0f )
        {
            result = __builtin_spirv_OpenCL_nan_i32((uint)0);
        }
        // Conformance test also checks for this "overflow" case, but
        // I don't think we should have to handle it.
        else if( x > 1500.0f )
        {
            result = __builtin_spirv_OpenCL_log_f32(x) + M_LN2_F;
        }
        else
#endif
        {
            result = x * x - 1.0f;
            result = __builtin_spirv_OpenCL_sqrt_f32( result );
            result = x + result;
            result = __builtin_spirv_OpenCL_log_f32( result );
        }
    }
    else
    {
        result = __ocl_svml_acoshf(x);
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_acosh_f16( half x )
{
    return __builtin_spirv_OpenCL_acosh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, half, half, f16 )

#endif // defined(cl_khr_fp16)

// acospi

INLINE float __builtin_spirv_OpenCL_acospi_f32( float x )
{
    return __ocl_svml_acospif(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_acospi, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_acospi_f16( half x )
{
    return M_1_PI_H * __builtin_spirv_OpenCL_acos_f16(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_acospi, half, half, f16 )

#endif // defined(cl_khr_fp16)

// asin

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asin, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asin, half, half, f16 )

#endif // defined(cl_khr_fp16)

// asinh

INLINE float __intel_asinh_f32( float x, bool doFast )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        // Implemented as log(x + sqrt(x*x + 1)).
        // Conformance test checks for this "overflow" case, but
        // I don't think we should have to handle it.
        if( x > 1500.0f )
        {
            result = __builtin_spirv_OpenCL_log_f32(x) + M_LN2_F;
        }
        else
        {
            result = x * x + 1.0f;
            result = __builtin_spirv_OpenCL_sqrt_f32( result );
            result = x + result;
            result = __builtin_spirv_OpenCL_log_f32( result );
        }
    }
    else
    {
        result = __ocl_svml_asinhf(x);
    }

    return result;
}

INLINE float __builtin_spirv_OpenCL_asinh_f32( float x )
{
    return __intel_asinh_f32( x, true );
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_asinh, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_asinh_f16( half x )
{
    return __builtin_spirv_OpenCL_asinh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_asinh, half, half, f16 )

#endif // defined(cl_khr_fp16)

// asinpi

INLINE float __builtin_spirv_OpenCL_asinpi_f32( float x )
{
    return __ocl_svml_asinpif(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asinpi, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_asinpi_f16( half x )
{
    return M_1_PI_H * __builtin_spirv_OpenCL_asin_f16(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asinpi, half, half, f16 )

#endif // defined(cl_khr_fp16)

// atan

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atan, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atan, half, half, f16 )

#endif // defined(cl_khr_fp16)

// atan2

// TODO: I think we should be able to use M_PI_F here instead of FLOAT_PI,
// but this does cause very small differences in the results of atan2(),
// since M_PI_F is rounded (and therefore slightly larger than FLOAT_PI).
// We'll need to re-collect some GITS streams if we want to use M_PI_F
// instead.
#define FLOAT_PI                (as_float(0x40490FDA)) // 3.1415926535897930f

float __builtin_spirv_OpenCL_atan2_f32_f32( float y, float x )
{
    // atan2(y,x) =  atan(y/x)        if  any y, x > 0    atan_yx
    //            =  atan(y/x) + pi   if y >= 0, x < 0    atan_yx + pi
    //            =  atan(y/x) - pi   if y <  0, x < 0    atan_yx - pi
    //            =  pi/2             if y >= 0, x = 0    pi/2
    //            = -pi/2             if y <  0, x = 0    -pi/2
    //  Sign    = (y >= 0) ? 1 : -1;
    //  atan_yx = (x != 0) ? atan(y/x) : Sign*pi/2;
    //  Coeff   = (x < 0)  ? Sign : 0;
    //  dst     = atan_yx + Coeff*pi;

    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // Implemented as:
        //  atan(y/x) for x > 0,
        //  atan(y/x) + M_PI_F for x < 0 and y > 0, and
        //  atan(y/x) -M_PI_F for x < 0 and y < 0.
        float px = __builtin_spirv_OpenCL_atan_f32( y / x );
        float py = px + M_PI_F;
        float ny = px - M_PI_F;

        result = ( y > 0 ) ? py : ny;
        result = ( x > 0 ) ? px : result;
    }
    else
    {
        // The LA atan2 implementation (IMF/FP32/atan2_s_la.cl)
        // seems to be slower on Mandelbulb algorithm..
        if( __intel_relaxed_isnan(x) |
            __intel_relaxed_isnan(y) )
        {
            result = __builtin_spirv_OpenCL_nan_i32(0U);
        }
        else
        {
            float signy = __builtin_spirv_OpenCL_copysign_f32_f32(1.0f, y);
            if( y == 0.0f )
            {
                float signx = __builtin_spirv_OpenCL_copysign_f32_f32(1.0f, x);
                float px = signy * 0.0f;
                float nx = signy * FLOAT_PI;
                // In this case, we need to compare signx against
                // 1.0f, not x > 0.0f, since we need to distinguish
                // between x == +0.0f and x == -0.0f.
                result = ( signx == 1.0f ) ? px : nx;
            }
            else if( x == 0.0f )
            {
                result = signy * ( FLOAT_PI * 0.5f );
            }
            else if( __intel_relaxed_isinf( y ) &
                     __intel_relaxed_isinf( x ) )
            {
                float px = signy * ( FLOAT_PI * 0.25f );
                float nx = signy * ( FLOAT_PI * 0.75f );
                result = ( x > 0.0f ) ? px : nx;
            }
            else
            {
                float px = __builtin_spirv_OpenCL_atan_f32( y / x );
                float nx = __builtin_spirv_OpenCL_mad_f32_f32_f32( signy, FLOAT_PI, px );
                result = ( x > 0.0f ) ? px : nx;
            }
        }
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_atan2, float, float, float, f32, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_atan2_f16_f16( half y, half x )
{
    half result;

    if( __intel_relaxed_isnan((float)x) |
        __intel_relaxed_isnan((float)y) )
    {
        result = __builtin_spirv_OpenCL_nan_i32(0U);
    }
    else
    {
        half signy = __builtin_spirv_OpenCL_copysign_f16_f16(1.0f, y);
        if( y == 0.0f )
        {
            half signx = __builtin_spirv_OpenCL_copysign_f16_f16(1.0f, x);
            half px = signy * 0.0f;
            half nx = signy * FLOAT_PI;
            // In this case, we need to compare signx against
            // 1.0f, not x > 0.0f, since we need to distinguish
            // between x == +0.0f and x == -0.0f.
            result = ( signx == 1.0f ) ? px : nx;
        }
        else if( x == 0.0f )
        {
            result = signy * ( FLOAT_PI * 0.5f );
        }
        else if(__intel_relaxed_isinf((float)y) &
                __intel_relaxed_isinf((float)x))
        {
            half px = signy * ( FLOAT_PI * 0.25f );
            half nx = signy * ( FLOAT_PI * 0.75f );
            result = ( x > 0.0f ) ? px : nx;
        }
        else
        {
            half px = __builtin_spirv_OpenCL_atan_f32( (float)y / (float)x );
            half nx = __builtin_spirv_OpenCL_mad_f32_f32_f32( (float)signy, FLOAT_PI, (float)px );
            result = ( x > 0.0f ) ? px : nx;
        }
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_atan2, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

// atan2pi

INLINE float __builtin_spirv_OpenCL_atan2pi_f32_f32( float x, float y )
{
    return M_1_PI_F * __builtin_spirv_OpenCL_atan2_f32_f32(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_atan2pi, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_atan2pi_f16_f16( half x, half y )
{
    return M_1_PI_H * __builtin_spirv_OpenCL_atan2_f16_f16(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_atan2pi, half, half, f16 )

#endif // defined(cl_khr_fp16)

// atanh

float __builtin_spirv_OpenCL_atanh_f32( float x )
{
    return __ocl_svml_atanhf(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_atanh_f16( half x )
{
    return __builtin_spirv_OpenCL_atanh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, half, half, f16 )

#endif // defined(cl_khr_fp16)

// atanpi

INLINE float __builtin_spirv_OpenCL_atanpi_f32( float x )
{
    return __ocl_svml_atanpif(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atanpi, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_atanpi_f16( half x )
{
    return M_1_PI_H * __builtin_spirv_OpenCL_atan_f16(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atanpi, half, half, f16 )

#endif // defined(cl_khr_fp16)

// cbrt

INLINE float __builtin_spirv_OpenCL_cbrt_f32( float x )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // Implemented as rootn(x, 3).
        result = __builtin_spirv_OpenCL_rootn_f32_i32( x, 3 );
    }
    else
    {
        result = __ocl_svml_cbrtf(x);
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_cbrt, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_cbrt_f16( half x )
{
    return __builtin_spirv_OpenCL_cbrt_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_cbrt, half, half, f16 )

#endif // defined(cl_khr_fp16)

// ceil

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_ceil, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_ceil, half, half, f16 )

#endif // defined(cl_khr_fp16)

// copysign

INLINE float __builtin_spirv_OpenCL_copysign_f32_f32( float x, float y )
{
    return as_float( (int)((as_int(y) & FLOAT_SIGN_MASK) + (as_int(x) & ~FLOAT_SIGN_MASK)) );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_copysign, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_copysign_f16_f16( half x, half y )
{
    return as_half( (short)((as_short(y) & HALF_SIGN_MASK) + (as_short(x) & ~HALF_SIGN_MASK)) );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_copysign, half, half, f16 )

#endif // defined(cl_khr_fp16)

// cos

static INLINE float __intel_cos_f32( float x, bool doFast )
{
    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        return __builtin_spirv_OpenCL_native_cos_f32(x);
    }
    else
    {
        if(__UseMathWithLUT)
        {
            return __ocl_svml_cosf(x);
        }
        else
        {
            float abs_float = __builtin_spirv_OpenCL_fabs_f32(x);
            if( abs_float > 10000.0f )
            {
                return libclc_cos_f32(x);
            }
            else
            {
                return __ocl_svml_cosf_noLUT(x);
            }
        }
    }
}

INLINE float __builtin_spirv_OpenCL_cos_f32( float x )
{
    return __intel_cos_f32(x, true);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cos, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_cos_f16( half x )
{
    return (half)__builtin_spirv_OpenCL_cos_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cos, half, half, f16 )

#endif // defined(cl_khr_fp16)

// cosh

float __builtin_spirv_OpenCL_cosh_f32( float x )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // Implemented as 0.5 * ( exp(x) + exp(-x) ).
        float pexp = __builtin_spirv_OpenCL_exp_f32(  x );
        float nexp = __builtin_spirv_OpenCL_exp_f32( -x );
        result = 0.5f * ( pexp + nexp );
    }
    else
    {
        result = __ocl_svml_coshf(x);
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cosh, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_cosh_f16( half x )
{
    return __builtin_spirv_OpenCL_cosh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cosh, half, half, f16 )

#endif // defined(cl_khr_fp16)

// cospi

INLINE float __builtin_spirv_OpenCL_cospi_f32( float x )
{
    bool useNative = __FastRelaxedMath && (!__APIRS);

    if(useNative)
    {
        return __builtin_spirv_OpenCL_cos_f32( x * M_PI_F );
    }
    else
    {
        if(__UseMathWithLUT)
        {
            return __ocl_svml_cospif(x);
        }
        else
        {
            return __ocl_svml_cospif_noLUT(x);
        }
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cospi, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_cospi_f16( half x )
{
    return __builtin_spirv_OpenCL_cospi_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cospi, half, half, f16 )

#endif // defined(cl_khr_fp16)

// divide_cr

float __builtin_spirv_divide_cr_f32_f32( float a,
                                         float b )
{
    if (!__CRMacros) {
        typedef union binary32
        {
            uint u;
            int  s;
            float f;
        } binary32;

        binary32 fa, fb, s, y0, y1, q0, q1, q, r0, r1, e0, norm2sub;
        int aExp, bExp, scale, flag = 0;
        norm2sub.u = 0x00800000;
        fa.f = a;
        fb.f = b;
        aExp = (fa.u >> 23) & 0xff;
        bExp = (fb.u >> 23) & 0xff;

        // Normalize denormals -- scale by 2^32
        if(!(fa.u & 0x7f800000)) {
            binary32 x; x.u = 0x4f800000;
            fa.f *= x.f;
            aExp = ((fa.u >> 23) & 0xff) - 32;
        }
        if(!(fb.u & 0x7f800000)) {
            binary32 x; x.u = 0x4f800000;
            fb.f *= x.f;
            bExp = ((fb.u >> 23) & 0xff) - 32;
        }

        if ((fa.f != 0.0f) && (aExp != 0xff)) {
            // a is NOT 0 or INF or NAN
            flag = 1;
            // Scale a to [1,2)
            fa.u = (fa.u & 0x807fffff) | 0x3f800000;
        }
        // Initial approximation of 1/b
        if (fb.f == 0.0f) {
            // b = 0, y0 = INF
            flag = 0;
            y0.u  =  (fb.u & 0x80000000) | 0x7f800000;
        } else if (bExp == 0xff) {
            flag = 0;
            if ((fb.u & 0x7fffff) == 0) {
                // b = INF, y0 = 0
                y0.u = (fb.u & 0x80000000);
            } else {
                // b = NaN
                y0.u = fb.u;
            }
        } else {
            // b is not 0 or INF or NAN
            flag &= 1;
            // Scale b to [1,2)
            fb.u  = (fb.u & 0x807fffff) | 0x3f800000;
            y0.f = __builtin_spirv_OpenCL_native_recip_f32(fb.f);
            //printf("y0=0x%08x=%a\n", y0.u, y0.f);
        }
        if (flag) {
            scale =  aExp - bExp;
            //printf("a=0x%08x\n", fa.u);
            //printf("b=0x%08x\n", fb.u);
            // Step(1), q0=a*y0
            q0.f = fa.f * y0.f;
            //printf("q0=0x%08x=%a\n", q0.u, q0.f);
            // Step(2), e0=(1-b*y0)
            e0.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(-fb.f, y0.f, 1.0f);
            //printf("e0=0x%08x=%a\n", e0.u, e0.f);
            // Step(3), y1=y0+e0*y0
            y1.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(e0.f, y0.f, y0.f);
            //printf("y1=0x%08x=%a\n", y1.u, y1.f);
            // Step(4), r0=a-b*q0
            r0.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(-fb.f, q0.f, fa.f);
            //printf("r0=0x%08x=%a\n", r0.u, r0.f);
            // Step(5), q1=q0+r0*y1
            q1.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(r0.f, y1.f, q0.f);
            //printf("q1=0x%08x=%a\n", q1.u, q1.f);
            // Step(6), r1=a-b*q1
            r1.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(-fb.f, q1.f, fa.f);
            //printf("r1=0x%08x=%a\n", r1.u, r1.f);
            // Step(7), q=q1+r1*y1, set user rounding mode here
            q.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(r1.f, y1.f, q1.f);
            //printf("q=0x%08x=%a\n", q.u, q.f);
            // Scale so that 1<= q < 4
            //q.f = q.f * 2;
            //printf("q=0x%08x\n", q.u);
            scale += 0x7e;
            if (scale <= 0) {
                // Underflow
                scale += 0x7f;
                // scale will always be positive now
                s.u = (scale << 23);
                s.f = s.f * norm2sub.f;
            } else if (scale >= 0xff) {
                // Overflow, this is OK since q >= 1
                s.u = 0x7f800000;
                q.f = q.f * 2;
            } else {
                // Normal
                s.u = (scale << 23);
                q.f = q.f * 2;
            }
            //printf("s=0x%08x\n", s.u);
            q.f = q.f * s.f;
            //printf("q=0x%08x\n", q.u);
        } else {
            //printf("fa=0x%08x\n", fa.u);
            //printf("y0=0x%08x\n", y0.u);
            q.f = fa.f * y0.f;
        }
        return q.f;
    } else {
        return FDIV_IEEE(a, b);
    }
}

INLINE
float2 __builtin_spirv_divide_cr_v2f32_v2f32( float2 a,
                                              float2 b )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in2[0] = b.s0;
    in2[1] = b.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE
float3 __builtin_spirv_divide_cr_v3f32_v3f32( float3 a,
                                              float3 b )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in1[2] = a.s2;
    in2[0] = b.s0;
    in2[1] = b.s1;
    in2[2] = b.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE
float4 __builtin_spirv_divide_cr_v4f32_v4f32( float4 a,
                                              float4 b )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in1[2] = a.s2;
    in1[3] = a.s3;
    in2[0] = b.s0;
    in2[1] = b.s1;
    in2[2] = b.s2;
    in2[3] = b.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE
float8 __builtin_spirv_divide_cr_v8f32_v8f32( float8 a,
                                              float8 b )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in1[2] = a.s2;
    in1[3] = a.s3;
    in1[4] = a.s4;
    in1[5] = a.s5;
    in1[6] = a.s6;
    in1[7] = a.s7;
    in2[0] = b.s0;
    in2[1] = b.s1;
    in2[2] = b.s2;
    in2[3] = b.s3;
    in2[4] = b.s4;
    in2[5] = b.s5;
    in2[6] = b.s6;
    in2[7] = b.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE
float16 __builtin_spirv_divide_cr_v16f32_v16f32( float16 a,
                                                 float16 b )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in1[2] = a.s2;
    in1[3] = a.s3;
    in1[4] = a.s4;
    in1[5] = a.s5;
    in1[6] = a.s6;
    in1[7] = a.s7;
    in1[8] = a.s8;
    in1[9] = a.s9;
    in1[10] = a.sa;
    in1[11] = a.sb;
    in1[12] = a.sc;
    in1[13] = a.sd;
    in1[14] = a.se;
    in1[15] = a.sf;
    in2[0] = b.s0;
    in2[1] = b.s1;
    in2[2] = b.s2;
    in2[3] = b.s3;
    in2[4] = b.s4;
    in2[5] = b.s5;
    in2[6] = b.s6;
    in2[7] = b.s7;
    in2[8] = b.s8;
    in2[9] = b.s9;
    in2[10] = b.sa;
    in2[11] = b.sb;
    in2[12] = b.sc;
    in2[13] = b.sd;
    in2[14] = b.se;
    in2[15] = b.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}

// erf

INLINE float __builtin_spirv_OpenCL_erf_f32( float x )
{
    return __ocl_svml_erff(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erf, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_erf_f16( half x )
{
    return __builtin_spirv_OpenCL_erf_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erf, half, half, f16 )

#endif // defined(cl_khr_fp16)

// erfc

float __builtin_spirv_OpenCL_erfc_f32( float x )
{
    return __ocl_svml_erfcf(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erfc, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_erfc_f16( half x )
{
    return __builtin_spirv_OpenCL_erfc_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erfc, half, half, f16 )

#endif // defined(cl_khr_fp16)

// exp

float __builtin_spirv_OpenCL_exp_f32(float x)
{
    if (__FastRelaxedMath)
    {
        return __builtin_spirv_OpenCL_native_exp_f32(x);
    }
    else
    {
        // e^x = 2^(log2(e^x)) = 2^(x * log2(e))
        // We'll compute 2^(x * log2(e)) by splitting x * log2(e)
        //   into a whole part and fractional part.

        // Compute the whole part of x * log2(e)
        // This part is easy!
        float w = __builtin_spirv_OpenCL_trunc_f32( x * M_LOG2E_F );

        // Compute the fractional part of x * log2(e)
        // We have to do this carefully, so we don't lose precision.
        // Compute as:
        //   fract( x * log2(e) ) = ( x - w * C1 - w * C2 ) * log2(e)
        // C1 is the "Cephes Constant", and is close to 1/log2(e)
        // C2 is the difference between the "Cephes Constant" and 1/log2(e)
        const float C1 = as_float( 0x3F317200 );    // 0.693145751953125
        const float C2 = as_float( 0x35BFBE8E );    // 0.000001428606765330187
        float f = x;
        f = __builtin_spirv_OpenCL_fma_f32_f32_f32( w, -C1, f );
        f = __builtin_spirv_OpenCL_fma_f32_f32_f32( w, -C2, f );
        f = f * M_LOG2E_F;

        w = __builtin_spirv_OpenCL_native_exp2_f32( w );   // this should be exact
        f = __builtin_spirv_OpenCL_native_exp2_f32( f );   // this should be close enough

        float res = w * f;
        res = ( x < as_float( 0xC2D20000 ) ) ? as_float( 0x00000000 ) : res;
        res = ( x > as_float( 0x42D20000 ) ) ? as_float( 0x7F800000 ) : res;

        return res;
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_exp_f16( half x )
{
    return __builtin_spirv_OpenCL_exp_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp, half, half, f16 )

#endif // defined(cl_khr_fp16)

// exp2

INLINE float __builtin_spirv_OpenCL_exp2_f32( float x )
{
    return __builtin_spirv_OpenCL_native_exp2_f32(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp2, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_exp2_f16( half x )
{
    return __builtin_spirv_OpenCL_exp2_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp2, half, half, f16 )

#endif // defined(cl_khr_fp16)

// exp10

float __builtin_spirv_OpenCL_exp10_f32( float x )
{
    if(__FastRelaxedMath)
    {
        return __builtin_spirv_OpenCL_native_exp10_f32(x);
    }
    else
    {
        return __ocl_svml_exp10f(x);
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_exp10_f16( half x )
{
    return __builtin_spirv_OpenCL_exp10_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, half, half, f16 )

#endif // defined(cl_khr_fp16)

// expm1

INLINE float __builtin_spirv_OpenCL_expm1_f32( float x )
{
    if(__FastRelaxedMath && (!__APIRS))
    {
        return __builtin_spirv_OpenCL_exp_f32( x ) - 1.0f;
    }
    else
    {
        return __ocl_svml_expm1f(x);
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_expm1, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_expm1_f16( half x )
{
    return __builtin_spirv_OpenCL_expm1_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_expm1, half, half, f16 )

#endif // defined(cl_khr_fp16)

// fabs

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_fabs, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_fabs, half, half, f16 )

#endif // defined(cl_khr_fp16)

// fdim

INLINE float __builtin_spirv_OpenCL_fdim_f32_f32( float x, float y )
{
    float r = x - y;
    float n = __builtin_spirv_OpenCL_nan_i32(0u);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fdim, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_fdim_f16_f16( half x, half y )
{
    half r = x - y;
    half n = __builtin_spirv_OpenCL_nan_i16(0u);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fdim, half, half, f16 )

#endif // defined(cl_khr_fp16)

// floor

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_floor, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_floor, half, half, f16 )

#endif // defined(cl_khr_fp16)

// fma

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_fma, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_fma, half, half, f16 )

#endif // defined(cl_khr_fp16)

// fmax

INLINE float __builtin_spirv_OpenCL_fmax_f32_f32( float x, float y )
{
    return __builtin_IB_fmax(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmax, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_fmax_f16_f16( half x, half y )
{
    return __builtin_IB_HMAX(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmax, half, half, f16 )

#endif // defined(cl_khr_fp16)

// fmin

INLINE float __builtin_spirv_OpenCL_fmin_f32_f32( float x, float y )
{
    return __builtin_IB_fmin(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmin, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_fmin_f16_f16( half x, half y )
{
    return __builtin_IB_HMIN(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmin, half, half, f16 )

#endif // defined(cl_khr_fp16)

// fmod

INLINE
float __builtin_spirv_OpenCL_fast_fmod_f32_f32( float xx, float yy )
{
    float result = xx - yy * __builtin_spirv_OpenCL_trunc_f32( xx / yy );
    return result;
}

INLINE
float2 __builtin_spirv_OpenCL_fast_fmod_v2f32_v2f32( float2 xx, float2 yy )
{
    float2 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s1, yy.s1);
    return temp;
}

INLINE
float3 __builtin_spirv_OpenCL_fast_fmod_v3f32_v3f32( float3 xx, float3 yy )
{
    float3 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s2, yy.s2);
    return temp;
}

INLINE
float4 __builtin_spirv_OpenCL_fast_fmod_v4f32_v4f32( float4 xx, float4 yy )
{
    float4 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s2, yy.s2);
    temp.s3 = __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx.s3, yy.s3);
    return temp;
}


#if defined(cl_khr_fp16)
INLINE
half __builtin_spirv_OpenCL_fast_fmod_f16_f16( half xx, half yy )
{
    return (half)__builtin_spirv_OpenCL_fast_fmod_f32_f32((float)xx, (float)yy);
}

INLINE
half2 __builtin_spirv_OpenCL_fast_fmod_v2f16_v2f16( half2 xx, half2 yy )
{
    half2 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s1, yy.s1);
    return temp;
}

INLINE
half3 __builtin_spirv_OpenCL_fast_fmod_v3f16_v3f16( half3 xx, half3 yy )
{
    half3 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s2, yy.s2);
    return temp;
}

INLINE
half4 __builtin_spirv_OpenCL_fast_fmod_v4f16_v4f16( half4 xx, half4 yy )
{
    half4 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s2, yy.s2);
    temp.s3 = __builtin_spirv_OpenCL_fast_fmod_f16_f16(xx.s3, yy.s3);
    return temp;
}
#endif // cl_khr_fp16

static float __intel_fmod_f32_f32( float xx, float yy, bool doFast )
{
    float result;
    if( __FastRelaxedMath && doFast )
    {
        return __builtin_spirv_OpenCL_fast_fmod_f32_f32(xx, yy);
    }

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f)
    {
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }
    else if( __intel_relaxed_isinf(yy) |
             (xx == 0.0f) )
    {
        result = xx;
    }
    else if( __builtin_spirv_OpenCL_fabs_f32(xx) == __builtin_spirv_OpenCL_fabs_f32(yy) )
    {
        result = __builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx);
    }
    else if (__builtin_spirv_OpenCL_fabs_f32(xx) < __builtin_spirv_OpenCL_fabs_f32(yy))
    {
        result = xx;
    }
    else
    {
        float x = __builtin_spirv_OpenCL_fabs_f32(xx);
        float y = __builtin_spirv_OpenCL_fabs_f32(yy);
        int ex = __builtin_spirv_OpenCL_ilogb_f32( x );
        int ey = __builtin_spirv_OpenCL_ilogb_f32( y );
        float xr = x;
        float yr = y;

        if(ex-ey >= 0)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f32_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32( x, -ex );

            for(int i = ex-ey; i > 0; i--)
            {
                float s = xr - yr;
                xr = ( xr >= yr ) ? s : xr;
                xr = xr + xr;
            }
            float s = xr - yr;
            xr = ( xr >= yr ) ? s : xr;

            xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ey);
        }

        float m = -xr;
        xr = ( xx < 0.0f ) ? m : xr;

        result = xr;
    }

    return result;
}

float __builtin_spirv_OpenCL_fmod_f32_f32( float xx, float yy )
{
    return __intel_fmod_f32_f32( xx, yy, true );
}

// OpFMod is the core version and is identical to OpenCL_fmod except
// it takes the sign from operand 2
INLINE float __builtin_spirv_OpFMod_f32_f32( float x, float y )
{
    return __builtin_spirv_OpenCL_copysign_f32_f32(__builtin_spirv_OpenCL_fmod_f32_f32(x, y), y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_fmod, float, float, float, f32, f32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, float, float, float, f32, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_fmod_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_fmod_f32_f32((float)x, (float)y);
}

INLINE half __builtin_spirv_OpFMod_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_copysign_f16_f16(__builtin_spirv_OpenCL_fmod_f16_f16(x, y), y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_fmod, half, half, half, f16, f16 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

// fract

INLINE float __builtin_spirv_OpenCL_fract_f32_p1f32( float x,
                                       __global float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f32_f32_i32( __builtin_spirv_OpenCL_floor_f32( x ), __builtin_spirv_OpenCL_nan_i32( (uint)0 ), (uint)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f32_f32_i32( (float)__builtin_spirv_OpenCL_fmin_f32_f32( temp, (float)(0x1.fffffep-1f)), (float)__builtin_spirv_OpenCL_copysign_f32_f32((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __builtin_spirv_OpenCL_select_f32_f32_i32( temp, __builtin_spirv_OpenCL_nan_i32((uint)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __builtin_spirv_OpenCL_fract_v2f32_p1v2f32( float2 x,
                                            __global float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( __builtin_spirv_OpenCL_floor_v2f32( x ), __builtin_spirv_OpenCL_nan_v2i32( (uint2)0 ), __convert_uint2(__builtin_spirv_OpIsNan_v2f32( x )) );
    float2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( (float2)__builtin_spirv_OpenCL_fmin_v2f32_v2f32( temp, (float2)(0x1.fffffep-1f)), (float2)__builtin_spirv_OpenCL_copysign_v2f32_v2f32((float2)0.0f, x), __convert_uint2(__builtin_spirv_OpIsInf_v2f32(x)));
    return __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( temp, __builtin_spirv_OpenCL_nan_v2i32((uint2)0), __convert_uint2(__builtin_spirv_OpIsNan_v2f32(x)) );
}

INLINE float3 __builtin_spirv_OpenCL_fract_v3f32_p1v3f32( float3 x,
                                            __global float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( __builtin_spirv_OpenCL_floor_v3f32( x ), __builtin_spirv_OpenCL_nan_v3i32( (uint3)0 ), __convert_uint3(__builtin_spirv_OpIsNan_v3f32( x )) );
    float3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( (float3)__builtin_spirv_OpenCL_fmin_v3f32_v3f32( temp, (float3)(0x1.fffffep-1f)), (float3)__builtin_spirv_OpenCL_copysign_v3f32_v3f32((float3)0.0f, x), __convert_uint3(__builtin_spirv_OpIsInf_v3f32(x)));
    return __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( temp, __builtin_spirv_OpenCL_nan_v3i32((uint3)0), __convert_uint3(__builtin_spirv_OpIsNan_v3f32(x)) );
}

INLINE float4 __builtin_spirv_OpenCL_fract_v4f32_p1v4f32( float4 x,
                                            __global float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( __builtin_spirv_OpenCL_floor_v4f32( x ), __builtin_spirv_OpenCL_nan_v4i32( (uint4)0 ), __convert_uint4(__builtin_spirv_OpIsNan_v4f32( x )) );
    float4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( (float4)__builtin_spirv_OpenCL_fmin_v4f32_v4f32( temp, (float4)(0x1.fffffep-1f)), (float4)__builtin_spirv_OpenCL_copysign_v4f32_v4f32((float4)0.0f, x), __convert_uint4(__builtin_spirv_OpIsInf_v4f32(x)));
    return __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( temp, __builtin_spirv_OpenCL_nan_v4i32((uint4)0), __convert_uint4(__builtin_spirv_OpIsNan_v4f32(x)) );
}

INLINE float8 __builtin_spirv_OpenCL_fract_v8f32_p1v8f32( float8 x,
                                            __global float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( __builtin_spirv_OpenCL_floor_v8f32( x ), __builtin_spirv_OpenCL_nan_v8i32( (uint8)0 ), __convert_uint8(__builtin_spirv_OpIsNan_v8f32( x )) );
    float8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( (float8)__builtin_spirv_OpenCL_fmin_v8f32_v8f32( temp, (float8)(0x1.fffffep-1f)), (float8)__builtin_spirv_OpenCL_copysign_v8f32_v8f32((float8)0.0f, x), __convert_uint8(__builtin_spirv_OpIsInf_v8f32(x)));
    return __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( temp, __builtin_spirv_OpenCL_nan_v8i32((uint8)0), __convert_uint8(__builtin_spirv_OpIsNan_v8f32(x)) );
}

INLINE float16 __builtin_spirv_OpenCL_fract_v16f32_p1v16f32( float16 x,
                                               __global float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( __builtin_spirv_OpenCL_floor_v16f32( x ), __builtin_spirv_OpenCL_nan_v16i32( (uint16)0 ), __convert_uint16(__builtin_spirv_OpIsNan_v16f32( x )) );
    float16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( (float16)__builtin_spirv_OpenCL_fmin_v16f32_v16f32( temp, (float16)(0x1.fffffep-1f)), (float16)__builtin_spirv_OpenCL_copysign_v16f32_v16f32((float16)0.0f, x), __convert_uint16(__builtin_spirv_OpIsInf_v16f32(x)));
    return __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( temp, __builtin_spirv_OpenCL_nan_v16i32((uint16)0), __convert_uint16(__builtin_spirv_OpIsNan_v16f32(x)) );
}

INLINE float __builtin_spirv_OpenCL_fract_f32_p0f32( float x,
                                       __private float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f32_f32_i32( __builtin_spirv_OpenCL_floor_f32( x ), __builtin_spirv_OpenCL_nan_i32( (uint)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f32_f32_i32( (float)__builtin_spirv_OpenCL_fmin_f32_f32( temp, (float)(0x1.fffffep-1f)), (float)__builtin_spirv_OpenCL_copysign_f32_f32((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __builtin_spirv_OpenCL_select_f32_f32_i32( temp, __builtin_spirv_OpenCL_nan_i32((uint)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __builtin_spirv_OpenCL_fract_v2f32_p0v2f32( float2 x,
                                            __private float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( __builtin_spirv_OpenCL_floor_v2f32( x ), __builtin_spirv_OpenCL_nan_v2i32( (uint2)0 ), __convert_uint2(__builtin_spirv_OpIsNan_v2f32( x )) );
    float2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( (float2)__builtin_spirv_OpenCL_fmin_v2f32_v2f32( temp, (float2)(0x1.fffffep-1f)), (float2)__builtin_spirv_OpenCL_copysign_v2f32_v2f32((float2)0.0f, x), __convert_uint2(__builtin_spirv_OpIsInf_v2f32(x)));
    return __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( temp, __builtin_spirv_OpenCL_nan_v2i32((uint2)0), __convert_uint2(__builtin_spirv_OpIsNan_v2f32(x)) );
}

INLINE float3 __builtin_spirv_OpenCL_fract_v3f32_p0v3f32( float3 x,
                                            __private float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( __builtin_spirv_OpenCL_floor_v3f32( x ), __builtin_spirv_OpenCL_nan_v3i32( (uint3)0 ), __convert_uint3(__builtin_spirv_OpIsNan_v3f32( x )) );
    float3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( (float3)__builtin_spirv_OpenCL_fmin_v3f32_v3f32( temp, (float3)(0x1.fffffep-1f)), (float3)__builtin_spirv_OpenCL_copysign_v3f32_v3f32((float3)0.0f, x), __convert_uint3(__builtin_spirv_OpIsInf_v3f32(x)));
    return __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( temp, __builtin_spirv_OpenCL_nan_v3i32((uint3)0), __convert_uint3(__builtin_spirv_OpIsNan_v3f32(x)) );
}

INLINE float4 __builtin_spirv_OpenCL_fract_v4f32_p0v4f32( float4 x,
                                            __private float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( __builtin_spirv_OpenCL_floor_v4f32( x ), __builtin_spirv_OpenCL_nan_v4i32( (uint4)0 ), __convert_uint4(__builtin_spirv_OpIsNan_v4f32( x )) );
    float4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( (float4)__builtin_spirv_OpenCL_fmin_v4f32_v4f32( temp, (float4)(0x1.fffffep-1f)), (float4)__builtin_spirv_OpenCL_copysign_v4f32_v4f32((float4)0.0f, x), __convert_uint4(__builtin_spirv_OpIsInf_v4f32(x)));
    return __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( temp, __builtin_spirv_OpenCL_nan_v4i32((uint4)0), __convert_uint4(__builtin_spirv_OpIsNan_v4f32(x)) );
}

INLINE float8 __builtin_spirv_OpenCL_fract_v8f32_p0v8f32( float8 x,
                                            __private float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( __builtin_spirv_OpenCL_floor_v8f32( x ), __builtin_spirv_OpenCL_nan_v8i32( (uint8)0 ), __convert_uint8(__builtin_spirv_OpIsNan_v8f32( x )) );
    float8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( (float8)__builtin_spirv_OpenCL_fmin_v8f32_v8f32( temp, (float8)(0x1.fffffep-1f)), (float8)__builtin_spirv_OpenCL_copysign_v8f32_v8f32((float8)0.0f, x), __convert_uint8(__builtin_spirv_OpIsInf_v8f32(x)));
    return __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( temp, __builtin_spirv_OpenCL_nan_v8i32((uint8)0), __convert_uint8(__builtin_spirv_OpIsNan_v8f32(x)) );
}

INLINE float16 __builtin_spirv_OpenCL_fract_v16f32_p0v16f32( float16 x,
                                               __private float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( __builtin_spirv_OpenCL_floor_v16f32( x ), __builtin_spirv_OpenCL_nan_v16i32( (uint16)0 ), __convert_uint16(__builtin_spirv_OpIsNan_v16f32( x )) );
    float16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( (float16)__builtin_spirv_OpenCL_fmin_v16f32_v16f32( temp, (float16)(0x1.fffffep-1f)), (float16)__builtin_spirv_OpenCL_copysign_v16f32_v16f32((float16)0.0f, x), __convert_uint16(__builtin_spirv_OpIsInf_v16f32(x)));
    return __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( temp, __builtin_spirv_OpenCL_nan_v16i32((uint16)0), __convert_uint16(__builtin_spirv_OpIsNan_v16f32(x)) );
}

INLINE float __builtin_spirv_OpenCL_fract_f32_p3f32( float x,
                                       __local float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f32_f32_i32( __builtin_spirv_OpenCL_floor_f32( x ), __builtin_spirv_OpenCL_nan_i32( (uint)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f32_f32_i32( (float)__builtin_spirv_OpenCL_fmin_f32_f32( temp, (float)(0x1.fffffep-1f)), (float)__builtin_spirv_OpenCL_copysign_f32_f32((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __builtin_spirv_OpenCL_select_f32_f32_i32( temp, __builtin_spirv_OpenCL_nan_i32((uint)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __builtin_spirv_OpenCL_fract_v2f32_p3v2f32( float2 x,
                                            __local float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( __builtin_spirv_OpenCL_floor_v2f32( x ), __builtin_spirv_OpenCL_nan_v2i32( (uint2)0 ), __convert_uint2(__builtin_spirv_OpIsNan_v2f32( x )) );
    float2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( (float2)__builtin_spirv_OpenCL_fmin_v2f32_v2f32( temp, (float2)(0x1.fffffep-1f)), (float2)__builtin_spirv_OpenCL_copysign_v2f32_v2f32((float2)0.0f, x), __convert_uint2(__builtin_spirv_OpIsInf_v2f32(x)));
    return __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( temp, __builtin_spirv_OpenCL_nan_v2i32((uint2)0), __convert_uint2(__builtin_spirv_OpIsNan_v2f32(x)) );
}

INLINE float3 __builtin_spirv_OpenCL_fract_v3f32_p3v3f32( float3 x,
                                            __local float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( __builtin_spirv_OpenCL_floor_v3f32( x ), __builtin_spirv_OpenCL_nan_v3i32( (uint3)0 ), __convert_uint3(__builtin_spirv_OpIsNan_v3f32( x )) );
    float3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( (float3)__builtin_spirv_OpenCL_fmin_v3f32_v3f32( temp, (float3)(0x1.fffffep-1f)), (float3)__builtin_spirv_OpenCL_copysign_v3f32_v3f32((float3)0.0f, x), __convert_uint3(__builtin_spirv_OpIsInf_v3f32(x)));
    return __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( temp, __builtin_spirv_OpenCL_nan_v3i32((uint3)0), __convert_uint3(__builtin_spirv_OpIsNan_v3f32(x)) );
}

INLINE float4 __builtin_spirv_OpenCL_fract_v4f32_p3v4f32( float4 x,
                                            __local float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( __builtin_spirv_OpenCL_floor_v4f32( x ), __builtin_spirv_OpenCL_nan_v4i32( (uint4)0 ), __convert_uint4(__builtin_spirv_OpIsNan_v4f32( x )) );
    float4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( (float4)__builtin_spirv_OpenCL_fmin_v4f32_v4f32( temp, (float4)(0x1.fffffep-1f)), (float4)__builtin_spirv_OpenCL_copysign_v4f32_v4f32((float4)0.0f, x), __convert_uint4(__builtin_spirv_OpIsInf_v4f32(x)));
    return __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( temp, __builtin_spirv_OpenCL_nan_v4i32((uint4)0), __convert_uint4(__builtin_spirv_OpIsNan_v4f32(x)) );
}

INLINE float8 __builtin_spirv_OpenCL_fract_v8f32_p3v8f32( float8 x,
                                            __local float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( __builtin_spirv_OpenCL_floor_v8f32( x ), __builtin_spirv_OpenCL_nan_v8i32( (uint8)0 ), __convert_uint8(__builtin_spirv_OpIsNan_v8f32( x )) );
    float8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( (float8)__builtin_spirv_OpenCL_fmin_v8f32_v8f32( temp, (float8)(0x1.fffffep-1f)), (float8)__builtin_spirv_OpenCL_copysign_v8f32_v8f32((float8)0.0f, x), __convert_uint8(__builtin_spirv_OpIsInf_v8f32(x)));
    return __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( temp, __builtin_spirv_OpenCL_nan_v8i32((uint8)0), __convert_uint8(__builtin_spirv_OpIsNan_v8f32(x)) );
}

INLINE float16 __builtin_spirv_OpenCL_fract_v16f32_p3v16f32( float16 x,
                                               __local float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( __builtin_spirv_OpenCL_floor_v16f32( x ), __builtin_spirv_OpenCL_nan_v16i32( (uint16)0 ), __convert_uint16(__builtin_spirv_OpIsNan_v16f32( x )) );
    float16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( (float16)__builtin_spirv_OpenCL_fmin_v16f32_v16f32( temp, (float16)(0x1.fffffep-1f)), (float16)__builtin_spirv_OpenCL_copysign_v16f32_v16f32((float16)0.0f, x), __convert_uint16(__builtin_spirv_OpIsInf_v16f32(x)));
    return __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( temp, __builtin_spirv_OpenCL_nan_v16i32((uint16)0), __convert_uint16(__builtin_spirv_OpIsNan_v16f32(x)) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __builtin_spirv_OpenCL_fract_f32_p4f32( float x,
                                       __generic float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f32_f32_i32( __builtin_spirv_OpenCL_floor_f32( x ), __builtin_spirv_OpenCL_nan_i32( (uint)0 ), (int)(__intel_relaxed_isnan( x )) );
    float temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f32_f32_i32( (float)__builtin_spirv_OpenCL_fmin_f32_f32( temp, (float)(0x1.fffffep-1f)), (float)__builtin_spirv_OpenCL_copysign_f32_f32((float)0.0f, x), (int)__intel_relaxed_isinf(x));
    return __builtin_spirv_OpenCL_select_f32_f32_i32( temp, __builtin_spirv_OpenCL_nan_i32((uint)0), (int)(__intel_relaxed_isnan(x)) );
}

INLINE float2 __builtin_spirv_OpenCL_fract_v2f32_p4v2f32( float2 x,
                                            __generic float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( __builtin_spirv_OpenCL_floor_v2f32( x ), __builtin_spirv_OpenCL_nan_v2i32( (uint2)0 ), __convert_uint2(__builtin_spirv_OpIsNan_v2f32( x )) );
    float2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( (float2)__builtin_spirv_OpenCL_fmin_v2f32_v2f32( temp, (float2)(0x1.fffffep-1f)), (float2)__builtin_spirv_OpenCL_copysign_v2f32_v2f32((float2)0.0f, x), __convert_uint2(__builtin_spirv_OpIsInf_v2f32(x)));
    return __builtin_spirv_OpenCL_select_v2f32_v2f32_v2i32( temp, __builtin_spirv_OpenCL_nan_v2i32((uint2)0), __convert_uint2(__builtin_spirv_OpIsNan_v2f32(x)) );
}

INLINE float3 __builtin_spirv_OpenCL_fract_v3f32_p4v3f32( float3 x,
                                            __generic float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( __builtin_spirv_OpenCL_floor_v3f32( x ), __builtin_spirv_OpenCL_nan_v3i32( (uint3)0 ), __convert_uint3(__builtin_spirv_OpIsNan_v3f32( x )) );
    float3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( (float3)__builtin_spirv_OpenCL_fmin_v3f32_v3f32( temp, (float3)(0x1.fffffep-1f)), (float3)__builtin_spirv_OpenCL_copysign_v3f32_v3f32((float3)0.0f, x), __convert_uint3(__builtin_spirv_OpIsInf_v3f32(x)));
    return __builtin_spirv_OpenCL_select_v3f32_v3f32_v3i32( temp, __builtin_spirv_OpenCL_nan_v3i32((uint3)0), __convert_uint3(__builtin_spirv_OpIsNan_v3f32(x)) );
}

INLINE float4 __builtin_spirv_OpenCL_fract_v4f32_p4v4f32( float4 x,
                                            __generic float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( __builtin_spirv_OpenCL_floor_v4f32( x ), __builtin_spirv_OpenCL_nan_v4i32( (uint4)0 ), __convert_uint4(__builtin_spirv_OpIsNan_v4f32( x )) );
    float4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( (float4)__builtin_spirv_OpenCL_fmin_v4f32_v4f32( temp, (float4)(0x1.fffffep-1f)), (float4)__builtin_spirv_OpenCL_copysign_v4f32_v4f32((float4)0.0f, x), __convert_uint4(__builtin_spirv_OpIsInf_v4f32(x)));
    return __builtin_spirv_OpenCL_select_v4f32_v4f32_v4i32( temp, __builtin_spirv_OpenCL_nan_v4i32((uint4)0), __convert_uint4(__builtin_spirv_OpIsNan_v4f32(x)) );
}

INLINE float8 __builtin_spirv_OpenCL_fract_v8f32_p4v8f32( float8 x,
                                            __generic float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( __builtin_spirv_OpenCL_floor_v8f32( x ), __builtin_spirv_OpenCL_nan_v8i32( (uint8)0 ), __convert_uint8(__builtin_spirv_OpIsNan_v8f32( x )) );
    float8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( (float8)__builtin_spirv_OpenCL_fmin_v8f32_v8f32( temp, (float8)(0x1.fffffep-1f)), (float8)__builtin_spirv_OpenCL_copysign_v8f32_v8f32((float8)0.0f, x), __convert_uint8(__builtin_spirv_OpIsInf_v8f32(x)));
    return __builtin_spirv_OpenCL_select_v8f32_v8f32_v8i32( temp, __builtin_spirv_OpenCL_nan_v8i32((uint8)0), __convert_uint8(__builtin_spirv_OpIsNan_v8f32(x)) );
}

INLINE float16 __builtin_spirv_OpenCL_fract_v16f32_p4v16f32( float16 x,
                                               __generic float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( __builtin_spirv_OpenCL_floor_v16f32( x ), __builtin_spirv_OpenCL_nan_v16i32( (uint16)0 ), __convert_uint16(__builtin_spirv_OpIsNan_v16f32( x )) );
    float16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( (float16)__builtin_spirv_OpenCL_fmin_v16f32_v16f32( temp, (float16)(0x1.fffffep-1f)), (float16)__builtin_spirv_OpenCL_copysign_v16f32_v16f32((float16)0.0f, x), __convert_uint16(__builtin_spirv_OpIsInf_v16f32(x)));
    return __builtin_spirv_OpenCL_select_v16f32_v16f32_v16i32( temp, __builtin_spirv_OpenCL_nan_v16i32((uint16)0), __convert_uint16(__builtin_spirv_OpIsNan_v16f32(x)) );
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#ifdef cl_khr_fp16
INLINE half __builtin_spirv_OpenCL_fract_f16_p1f16( half x,
                                      __global half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f16_f16_i16( __builtin_spirv_OpenCL_floor_f16( x ), __builtin_spirv_OpenCL_nan_i16( (ushort)0 ), (short)(__builtin_spirv_OpIsNan_f16( x )) );
    half temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f16_f16_i16( (half)__builtin_spirv_OpenCL_fmin_f16_f16( temp, (half)(0x1.fffffep-1f)), (half)__builtin_spirv_OpenCL_copysign_f16_f16((half)0.0f, x), (short)__builtin_spirv_OpIsInf_f16(x));
    return __builtin_spirv_OpenCL_select_f16_f16_i16( temp, __builtin_spirv_OpenCL_nan_i16((ushort)0), (short)(__builtin_spirv_OpIsNan_f16(x)) );
}

INLINE half2 __builtin_spirv_OpenCL_fract_v2f16_p1v2f16( half2 x,
                                           __global half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( __builtin_spirv_OpenCL_floor_v2f16( x ), __builtin_spirv_OpenCL_nan_v2i16( (ushort2)0 ), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16( x )) );
    half2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( (half2)__builtin_spirv_OpenCL_fmin_v2f16_v2f16( temp, (half2)(0x1.fffffep-1f)), (half2)__builtin_spirv_OpenCL_copysign_v2f16_v2f16((half2)0.0f, x), __convert_ushort2(__builtin_spirv_OpIsInf_v2f16(x)));
    return __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( temp, __builtin_spirv_OpenCL_nan_v2i16((ushort2)0), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16(x)) );
}

INLINE half3 __builtin_spirv_OpenCL_fract_v3f16_p1v3f16( half3 x,
                                           __global half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( __builtin_spirv_OpenCL_floor_v3f16( x ), __builtin_spirv_OpenCL_nan_v3i16( (ushort3)0 ), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16( x )) );
    half3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( (half3)__builtin_spirv_OpenCL_fmin_v3f16_v3f16( temp, (half3)(0x1.fffffep-1f)), (half3)__builtin_spirv_OpenCL_copysign_v3f16_v3f16((half3)0.0f, x), __convert_ushort3(__builtin_spirv_OpIsInf_v3f16(x)));
    return __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( temp, __builtin_spirv_OpenCL_nan_v3i16((ushort3)0), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16(x)) );
}

INLINE half4 __builtin_spirv_OpenCL_fract_v4f16_p1v4f16( half4 x,
                                           __global half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( __builtin_spirv_OpenCL_floor_v4f16( x ), __builtin_spirv_OpenCL_nan_v4i16( (ushort4)0 ), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16( x )) );
    half4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( (half4)__builtin_spirv_OpenCL_fmin_v4f16_v4f16( temp, (half4)(0x1.fffffep-1f)), (half4)__builtin_spirv_OpenCL_copysign_v4f16_v4f16((half4)0.0f, x), __convert_ushort4(__builtin_spirv_OpIsInf_v4f16(x)));
    return __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( temp, __builtin_spirv_OpenCL_nan_v4i16((ushort4)0), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16(x)) );
}

INLINE half8 __builtin_spirv_OpenCL_fract_v8f16_p1v8f16( half8 x,
                                           __global half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( __builtin_spirv_OpenCL_floor_v8f16( x ), __builtin_spirv_OpenCL_nan_v8i16( (ushort8)0 ), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16( x )) );
    half8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( (half8)__builtin_spirv_OpenCL_fmin_v8f16_v8f16( temp, (half8)(0x1.fffffep-1f)), (half8)__builtin_spirv_OpenCL_copysign_v8f16_v8f16((half8)0.0f, x), __convert_ushort8(__builtin_spirv_OpIsInf_v8f16(x)));
    return __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( temp, __builtin_spirv_OpenCL_nan_v8i16((ushort8)0), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16(x)) );
}

INLINE half16 __builtin_spirv_OpenCL_fract_v16f16_p1v16f16( half16 x,
                                              __global half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( __builtin_spirv_OpenCL_floor_v16f16( x ), __builtin_spirv_OpenCL_nan_v16i16( (ushort16)0 ), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16( x )) );
    half16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( (half16)__builtin_spirv_OpenCL_fmin_v16f16_v16f16( temp, (half16)(0x1.fffffep-1f)), (half16)__builtin_spirv_OpenCL_copysign_v16f16_v16f16((half16)0.0f, x), __convert_ushort16(__builtin_spirv_OpIsInf_v16f16(x)));
    return __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( temp, __builtin_spirv_OpenCL_nan_v16i16((ushort16)0), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16(x)) );
}

INLINE half __builtin_spirv_OpenCL_fract_f16_p0f16( half x,
                                      __private half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f16_f16_i16( __builtin_spirv_OpenCL_floor_f16( x ), __builtin_spirv_OpenCL_nan_i16( (ushort)0 ), (short)(__builtin_spirv_OpIsNan_f16( x )) );
    half temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f16_f16_i16( (half)__builtin_spirv_OpenCL_fmin_f16_f16( temp, (half)(0x1.fffffep-1f)), (half)__builtin_spirv_OpenCL_copysign_f16_f16((half)0.0f, x), (short)__builtin_spirv_OpIsInf_f16(x));
    return __builtin_spirv_OpenCL_select_f16_f16_i16( temp, __builtin_spirv_OpenCL_nan_i16((ushort)0), (short)(__builtin_spirv_OpIsNan_f16(x)) );
}

INLINE half2 __builtin_spirv_OpenCL_fract_v2f16_p0v2f16( half2 x,
                                           __private half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( __builtin_spirv_OpenCL_floor_v2f16( x ), __builtin_spirv_OpenCL_nan_v2i16( (ushort2)0 ), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16( x )) );
    half2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( (half2)__builtin_spirv_OpenCL_fmin_v2f16_v2f16( temp, (half2)(0x1.fffffep-1f)), (half2)__builtin_spirv_OpenCL_copysign_v2f16_v2f16((half2)0.0f, x), __convert_ushort2(__builtin_spirv_OpIsInf_v2f16(x)));
    return __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( temp, __builtin_spirv_OpenCL_nan_v2i16((ushort2)0), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16(x)) );
}

INLINE half3 __builtin_spirv_OpenCL_fract_v3f16_p0v3f16( half3 x,
                                           __private half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( __builtin_spirv_OpenCL_floor_v3f16( x ), __builtin_spirv_OpenCL_nan_v3i16( (ushort3)0 ), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16( x )) );
    half3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( (half3)__builtin_spirv_OpenCL_fmin_v3f16_v3f16( temp, (half3)(0x1.fffffep-1f)), (half3)__builtin_spirv_OpenCL_copysign_v3f16_v3f16((half3)0.0f, x), __convert_ushort3(__builtin_spirv_OpIsInf_v3f16(x)));
    return __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( temp, __builtin_spirv_OpenCL_nan_v3i16((ushort3)0), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16(x)) );
}

INLINE half4 __builtin_spirv_OpenCL_fract_v4f16_p0v4f16( half4 x,
                                           __private half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( __builtin_spirv_OpenCL_floor_v4f16( x ), __builtin_spirv_OpenCL_nan_v4i16( (ushort4)0 ), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16( x )) );
    half4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( (half4)__builtin_spirv_OpenCL_fmin_v4f16_v4f16( temp, (half4)(0x1.fffffep-1f)), (half4)__builtin_spirv_OpenCL_copysign_v4f16_v4f16((half4)0.0f, x), __convert_ushort4(__builtin_spirv_OpIsInf_v4f16(x)));
    return __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( temp, __builtin_spirv_OpenCL_nan_v4i16((ushort4)0), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16(x)) );
}

INLINE half8 __builtin_spirv_OpenCL_fract_v8f16_p0v8f16( half8 x,
                                           __private half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( __builtin_spirv_OpenCL_floor_v8f16( x ), __builtin_spirv_OpenCL_nan_v8i16( (ushort8)0 ), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16( x )) );
    half8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( (half8)__builtin_spirv_OpenCL_fmin_v8f16_v8f16( temp, (half8)(0x1.fffffep-1f)), (half8)__builtin_spirv_OpenCL_copysign_v8f16_v8f16((half8)0.0f, x), __convert_ushort8(__builtin_spirv_OpIsInf_v8f16(x)));
    return __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( temp, __builtin_spirv_OpenCL_nan_v8i16((ushort8)0), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16(x)) );
}

INLINE half16 __builtin_spirv_OpenCL_fract_v16f16_p0v16f16( half16 x,
                                              __private half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( __builtin_spirv_OpenCL_floor_v16f16( x ), __builtin_spirv_OpenCL_nan_v16i16( (ushort16)0 ), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16( x )) );
    half16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( (half16)__builtin_spirv_OpenCL_fmin_v16f16_v16f16( temp, (half16)(0x1.fffffep-1f)), (half16)__builtin_spirv_OpenCL_copysign_v16f16_v16f16((half16)0.0f, x), __convert_ushort16(__builtin_spirv_OpIsInf_v16f16(x)));
    return __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( temp, __builtin_spirv_OpenCL_nan_v16i16((ushort16)0), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16(x)) );
}

INLINE half __builtin_spirv_OpenCL_fract_f16_p3f16( half x,
                                      __local half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f16_f16_i16( __builtin_spirv_OpenCL_floor_f16( x ), __builtin_spirv_OpenCL_nan_i16( (ushort)0 ), (short)(__builtin_spirv_OpIsNan_f16( x )) );
    half temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f16_f16_i16( (half)__builtin_spirv_OpenCL_fmin_f16_f16( temp, (half)(0x1.fffffep-1f)), (half)__builtin_spirv_OpenCL_copysign_f16_f16((half)0.0f, x), (short)__builtin_spirv_OpIsInf_f16(x));
    return __builtin_spirv_OpenCL_select_f16_f16_i16( temp, __builtin_spirv_OpenCL_nan_i16((ushort)0), (short)(__builtin_spirv_OpIsNan_f16(x)) );
}

INLINE half2 __builtin_spirv_OpenCL_fract_v2f16_p3v2f16( half2 x,
                                           __local half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( __builtin_spirv_OpenCL_floor_v2f16( x ), __builtin_spirv_OpenCL_nan_v2i16( (ushort2)0 ), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16( x )) );
    half2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( (half2)__builtin_spirv_OpenCL_fmin_v2f16_v2f16( temp, (half2)(0x1.fffffep-1f)), (half2)__builtin_spirv_OpenCL_copysign_v2f16_v2f16((half2)0.0f, x), __convert_ushort2(__builtin_spirv_OpIsInf_v2f16(x)));
    return __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( temp, __builtin_spirv_OpenCL_nan_v2i16((ushort2)0), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16(x)) );
}

INLINE half3 __builtin_spirv_OpenCL_fract_v3f16_p3v3f16( half3 x,
                                           __local half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( __builtin_spirv_OpenCL_floor_v3f16( x ), __builtin_spirv_OpenCL_nan_v3i16( (ushort3)0 ), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16( x )) );
    half3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( (half3)__builtin_spirv_OpenCL_fmin_v3f16_v3f16( temp, (half3)(0x1.fffffep-1f)), (half3)__builtin_spirv_OpenCL_copysign_v3f16_v3f16((half3)0.0f, x), __convert_ushort3(__builtin_spirv_OpIsInf_v3f16(x)));
    return __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( temp, __builtin_spirv_OpenCL_nan_v3i16((ushort3)0), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16(x)) );
}

INLINE half4 __builtin_spirv_OpenCL_fract_v4f16_p3v4f16( half4 x,
                                           __local half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( __builtin_spirv_OpenCL_floor_v4f16( x ), __builtin_spirv_OpenCL_nan_v4i16( (ushort4)0 ), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16( x )) );
    half4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( (half4)__builtin_spirv_OpenCL_fmin_v4f16_v4f16( temp, (half4)(0x1.fffffep-1f)), (half4)__builtin_spirv_OpenCL_copysign_v4f16_v4f16((half4)0.0f, x), __convert_ushort4(__builtin_spirv_OpIsInf_v4f16(x)));
    return __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( temp, __builtin_spirv_OpenCL_nan_v4i16((ushort4)0), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16(x)) );
}

INLINE half8 __builtin_spirv_OpenCL_fract_v8f16_p3v8f16( half8 x,
                                           __local half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( __builtin_spirv_OpenCL_floor_v8f16( x ), __builtin_spirv_OpenCL_nan_v8i16( (ushort8)0 ), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16( x )) );
    half8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( (half8)__builtin_spirv_OpenCL_fmin_v8f16_v8f16( temp, (half8)(0x1.fffffep-1f)), (half8)__builtin_spirv_OpenCL_copysign_v8f16_v8f16((half8)0.0f, x), __convert_ushort8(__builtin_spirv_OpIsInf_v8f16(x)));
    return __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( temp, __builtin_spirv_OpenCL_nan_v8i16((ushort8)0), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16(x)) );
}

INLINE half16 __builtin_spirv_OpenCL_fract_v16f16_p3v16f16( half16 x,
                                              __local half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( __builtin_spirv_OpenCL_floor_v16f16( x ), __builtin_spirv_OpenCL_nan_v16i16( (ushort16)0 ), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16( x )) );
    half16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( (half16)__builtin_spirv_OpenCL_fmin_v16f16_v16f16( temp, (half16)(0x1.fffffep-1f)), (half16)__builtin_spirv_OpenCL_copysign_v16f16_v16f16((half16)0.0f, x), __convert_ushort16(__builtin_spirv_OpIsInf_v16f16(x)));
    return __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( temp, __builtin_spirv_OpenCL_nan_v16i16((ushort16)0), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16(x)) );
}
#endif

#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __builtin_spirv_OpenCL_fract_f16_p4f16( half x,
                                      __generic half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f16_f16_i16( __builtin_spirv_OpenCL_floor_f16( x ), __builtin_spirv_OpenCL_nan_i16( (ushort)0 ), (short)(__builtin_spirv_OpIsNan_f16( x )) );
    half temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f16_f16_i16( (half)__builtin_spirv_OpenCL_fmin_f16_f16( temp, (half)(0x1.fffffep-1f)), (half)__builtin_spirv_OpenCL_copysign_f16_f16((half)0.0f, x), (short)__builtin_spirv_OpIsInf_f16(x));
    return __builtin_spirv_OpenCL_select_f16_f16_i16( temp, __builtin_spirv_OpenCL_nan_i16((ushort)0), (short)(__builtin_spirv_OpIsNan_f16(x)) );
}

INLINE half2 __builtin_spirv_OpenCL_fract_v2f16_p4v2f16( half2 x,
                                           __generic half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( __builtin_spirv_OpenCL_floor_v2f16( x ), __builtin_spirv_OpenCL_nan_v2i16( (ushort2)0 ), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16( x )) );
    half2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( (half2)__builtin_spirv_OpenCL_fmin_v2f16_v2f16( temp, (half2)(0x1.fffffep-1f)), (half2)__builtin_spirv_OpenCL_copysign_v2f16_v2f16((half2)0.0f, x), __convert_ushort2(__builtin_spirv_OpIsInf_v2f16(x)));
    return __builtin_spirv_OpenCL_select_v2f16_v2f16_v2i16( temp, __builtin_spirv_OpenCL_nan_v2i16((ushort2)0), __convert_ushort2(__builtin_spirv_OpIsNan_v2f16(x)) );
}

INLINE half3 __builtin_spirv_OpenCL_fract_v3f16_p4v3f16( half3 x,
                                           __generic half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( __builtin_spirv_OpenCL_floor_v3f16( x ), __builtin_spirv_OpenCL_nan_v3i16( (ushort3)0 ), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16( x )) );
    half3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( (half3)__builtin_spirv_OpenCL_fmin_v3f16_v3f16( temp, (half3)(0x1.fffffep-1f)), (half3)__builtin_spirv_OpenCL_copysign_v3f16_v3f16((half3)0.0f, x), __convert_ushort3(__builtin_spirv_OpIsInf_v3f16(x)));
    return __builtin_spirv_OpenCL_select_v3f16_v3f16_v3i16( temp, __builtin_spirv_OpenCL_nan_v3i16((ushort3)0), __convert_ushort3(__builtin_spirv_OpIsNan_v3f16(x)) );
}

INLINE half4 __builtin_spirv_OpenCL_fract_v4f16_p4v4f16( half4 x,
                                           __generic half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( __builtin_spirv_OpenCL_floor_v4f16( x ), __builtin_spirv_OpenCL_nan_v4i16( (ushort4)0 ), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16( x )) );
    half4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( (half4)__builtin_spirv_OpenCL_fmin_v4f16_v4f16( temp, (half4)(0x1.fffffep-1f)), (half4)__builtin_spirv_OpenCL_copysign_v4f16_v4f16((half4)0.0f, x), __convert_ushort4(__builtin_spirv_OpIsInf_v4f16(x)));
    return __builtin_spirv_OpenCL_select_v4f16_v4f16_v4i16( temp, __builtin_spirv_OpenCL_nan_v4i16((ushort4)0), __convert_ushort4(__builtin_spirv_OpIsNan_v4f16(x)) );
}

INLINE half8 __builtin_spirv_OpenCL_fract_v8f16_p4v8f16( half8 x,
                                           __generic half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( __builtin_spirv_OpenCL_floor_v8f16( x ), __builtin_spirv_OpenCL_nan_v8i16( (ushort8)0 ), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16( x )) );
    half8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( (half8)__builtin_spirv_OpenCL_fmin_v8f16_v8f16( temp, (half8)(0x1.fffffep-1f)), (half8)__builtin_spirv_OpenCL_copysign_v8f16_v8f16((half8)0.0f, x), __convert_ushort8(__builtin_spirv_OpIsInf_v8f16(x)));
    return __builtin_spirv_OpenCL_select_v8f16_v8f16_v8i16( temp, __builtin_spirv_OpenCL_nan_v8i16((ushort8)0), __convert_ushort8(__builtin_spirv_OpIsNan_v8f16(x)) );
}

INLINE half16 __builtin_spirv_OpenCL_fract_v16f16_p4v16f16( half16 x,
                                              __generic half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( __builtin_spirv_OpenCL_floor_v16f16( x ), __builtin_spirv_OpenCL_nan_v16i16( (ushort16)0 ), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16( x )) );
    half16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( (half16)__builtin_spirv_OpenCL_fmin_v16f16_v16f16( temp, (half16)(0x1.fffffep-1f)), (half16)__builtin_spirv_OpenCL_copysign_v16f16_v16f16((half16)0.0f, x), __convert_ushort16(__builtin_spirv_OpIsInf_v16f16(x)));
    return __builtin_spirv_OpenCL_select_v16f16_v16f16_v16i16( temp, __builtin_spirv_OpenCL_nan_v16i16((ushort16)0), __convert_ushort16(__builtin_spirv_OpIsNan_v16f16(x)) );
}
#endif //if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// frexp

float __builtin_spirv_OpenCL_frexp_f32_p1i32( float         x,
                                       __global int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f32_f32_i32( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f32_f32( temp, x );

        *exp = ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - ( FLOAT_BIAS - (int)(1) );
    }
    else if( (x == (float)(0.0f)) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = ( x == (float)(0.0f) ) ? (float)(0.0f) : x;
        *exp = 0;
    }
    else
    {
        int m = as_int( x ) & FLOAT_MANTISSA_MASK;
        int lz = __builtin_spirv_OpenCL_clz_i32( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f32_f32_i32( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f32_f32( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE float2 __builtin_spirv_OpenCL_frexp_v2f32_p1v2i32( float2         x,
                                            __global int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_frexp_v3f32_p1v3i32( float3         x,
                                            __global int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    *exp = exp_temp;
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_frexp_v4f32_p1v4i32( float4         x,
                                            __global int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    *exp = exp_temp;
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_frexp_v8f32_p1v8i32( float8         x,
                                            __global int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float in[8], out1[8];
    int out2[8];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    *exp = exp_temp;
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_frexp_v16f32_p1v16i32( float16         x,
                                               __global int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float in[16], out1[16];
    int out2[16];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    in[8] = x.s8;
    in[9] = x.s9;
    in[10] = x.sa;
    in[11] = x.sb;
    in[12] = x.sc;
    in[13] = x.sd;
    in[14] = x.se;
    in[15] = x.sf;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    temp.s8 = out1[8];
    temp.s9 = out1[9];
    temp.sa = out1[10];
    temp.sb = out1[11];
    temp.sc = out1[12];
    temp.sd = out1[13];
    temp.se = out1[14];
    temp.sf = out1[15];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    exp_temp.s8 = out2[8];
    exp_temp.s9 = out2[9];
    exp_temp.sa = out2[10];
    exp_temp.sb = out2[11];
    exp_temp.sc = out2[12];
    exp_temp.sd = out2[13];
    exp_temp.se = out2[14];
    exp_temp.sf = out2[15];
    *exp = exp_temp;
    return temp;
}

float __builtin_spirv_OpenCL_frexp_f32_p0i32( float          x,
                                       __private int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f32_f32_i32( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f32_f32( temp, x );

        *exp = ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - ( FLOAT_BIAS - (int)(1) );
    }
    else if( (x == (float)(0.0f)) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = ( x == (float)(0.0f) ) ? (float)(0.0f) : x;
        *exp = 0;
    }
    else
    {
        int m = as_int( x ) & FLOAT_MANTISSA_MASK;
        int lz = __builtin_spirv_OpenCL_clz_i32( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f32_f32_i32( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f32_f32( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE float2 __builtin_spirv_OpenCL_frexp_v2f32_p0v2i32( float2          x,
                                            __private int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_frexp_v3f32_p0v3i32( float3          x,
                                            __private int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    *exp = exp_temp;
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_frexp_v4f32_p0v4i32( float4          x,
                                            __private int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    *exp = exp_temp;
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_frexp_v8f32_p0v8i32( float8          x,
                                            __private int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float in[8], out1[8];
    int out2[8];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    *exp = exp_temp;
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_frexp_v16f32_p0v16i32( float16          x,
                                               __private int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float in[16], out1[16];
    int out2[16];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    in[8] = x.s8;
    in[9] = x.s9;
    in[10] = x.sa;
    in[11] = x.sb;
    in[12] = x.sc;
    in[13] = x.sd;
    in[14] = x.se;
    in[15] = x.sf;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    temp.s8 = out1[8];
    temp.s9 = out1[9];
    temp.sa = out1[10];
    temp.sb = out1[11];
    temp.sc = out1[12];
    temp.sd = out1[13];
    temp.se = out1[14];
    temp.sf = out1[15];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    exp_temp.s8 = out2[8];
    exp_temp.s9 = out2[9];
    exp_temp.sa = out2[10];
    exp_temp.sb = out2[11];
    exp_temp.sc = out2[12];
    exp_temp.sd = out2[13];
    exp_temp.se = out2[14];
    exp_temp.sf = out2[15];
    *exp = exp_temp;
    return temp;
}

float __builtin_spirv_OpenCL_frexp_f32_p3i32( float        x,
                                       __local int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f32_f32_i32( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f32_f32( temp, x );

        *exp = ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - ( FLOAT_BIAS - (int)(1) );
    }
    else if( (x == (float)(0.0f)) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = ( x == (float)(0.0f) ) ? (float)(0.0f) : x;
        *exp = 0;
    }
    else
    {
        int m = as_int( x ) & FLOAT_MANTISSA_MASK;
        int lz = __builtin_spirv_OpenCL_clz_i32( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f32_f32_i32( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f32_f32( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}


INLINE float2 __builtin_spirv_OpenCL_frexp_v2f32_p3v2i32( float2        x,
                                            __local int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_frexp_v3f32_p3v3i32( float3        x,
                                            __local int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    *exp = exp_temp;
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_frexp_v4f32_p3v4i32( float4        x,
                                            __local int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    *exp = exp_temp;
    return temp;
}

float8 __builtin_spirv_OpenCL_frexp_v8f32_p3v8i32( float8        x,
                                            __local int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float in[8], out1[8];
    int out2[8];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    *exp = exp_temp;
    return temp;
}

float16 __builtin_spirv_OpenCL_frexp_v16f32_p3v16i32( float16        x,
                                               __local int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float in[16], out1[16];
    int out2[16];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    in[8] = x.s8;
    in[9] = x.s9;
    in[10] = x.sa;
    in[11] = x.sb;
    in[12] = x.sc;
    in[13] = x.sd;
    in[14] = x.se;
    in[15] = x.sf;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    temp.s8 = out1[8];
    temp.s9 = out1[9];
    temp.sa = out1[10];
    temp.sb = out1[11];
    temp.sc = out1[12];
    temp.sd = out1[13];
    temp.se = out1[14];
    temp.sf = out1[15];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    exp_temp.s8 = out2[8];
    exp_temp.s9 = out2[9];
    exp_temp.sa = out2[10];
    exp_temp.sb = out2[11];
    exp_temp.sc = out2[12];
    exp_temp.sd = out2[13];
    exp_temp.se = out2[14];
    exp_temp.sf = out2[15];
    *exp = exp_temp;
    return temp;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __builtin_spirv_OpenCL_frexp_f32_p4i32( float          x,
                                       __generic int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f32_f32_i32( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f32_f32( temp, x );

        *exp = ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - ( FLOAT_BIAS - (int)(1) );
    }
    else if( (x == (float)(0.0f)) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = ( x == (float)(0.0f) ) ? (float)(0.0f) : x;
        *exp = 0;
    }
    else
    {
        int m = as_int( x ) & FLOAT_MANTISSA_MASK;
        int lz = __builtin_spirv_OpenCL_clz_i32( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f32_f32_i32( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f32_f32( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE float2 __builtin_spirv_OpenCL_frexp_v2f32_p4v2i32( float2          x,
                                            __generic int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_frexp_v3f32_p4v3i32( float3          x,
                                            __generic int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    *exp = exp_temp;
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_frexp_v4f32_p4v4i32( float4          x,
                                            __generic int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    *exp = exp_temp;
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_frexp_v8f32_p4v8i32( float8          x,
                                            __generic int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float in[8], out1[8];
    int out2[8];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    *exp = exp_temp;
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_frexp_v16f32_p4v16i32( float16          x,
                                               __generic int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float in[16], out1[16];
    int out2[16];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    in[8] = x.s8;
    in[9] = x.s9;
    in[10] = x.sa;
    in[11] = x.sb;
    in[12] = x.sc;
    in[13] = x.sd;
    in[14] = x.se;
    in[15] = x.sf;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f32_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    temp.s8 = out1[8];
    temp.s9 = out1[9];
    temp.sa = out1[10];
    temp.sb = out1[11];
    temp.sc = out1[12];
    temp.sd = out1[13];
    temp.se = out1[14];
    temp.sf = out1[15];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    exp_temp.s8 = out2[8];
    exp_temp.s9 = out2[9];
    exp_temp.sa = out2[10];
    exp_temp.sb = out2[11];
    exp_temp.sc = out2[12];
    exp_temp.sd = out2[13];
    exp_temp.se = out2[14];
    exp_temp.sf = out2[15];
    *exp = exp_temp;
    return temp;
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

half __builtin_spirv_OpenCL_frexp_f16_p1i32( half          x,
                                      __global int* exp )
{
    half temp;
    if( __builtin_spirv_OpIsNormal_f16( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f16_f16_i16( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f16_f16( temp, x );

        *exp = ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS) - ( HALF_BIAS - (short)(1) );
    }
    else if( (x == (half)(0.0f)) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = ( x == (float)(0.0f) ) ? (float)(0.0f) : x;
        *exp = 0;
    }
    else
    {
        short m = as_short( x ) & HALF_MANTISSA_MASK;
        short lz = __builtin_spirv_OpenCL_clz_i16( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f16_f16_i16( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f16_f16( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 __builtin_spirv_OpenCL_frexp_v2f16_p1v2i32( half2          x,
                                           __global int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_frexp_v3f16_p1v3i32( half3          x,
                                           __global int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    *exp = exp_temp;
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_frexp_v4f16_p1v4i32( half4          x,
                                           __global int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    *exp = exp_temp;
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_frexp_v8f16_p1v8i32( half8          x,
                                           __global int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half in[8], out1[8];
    int out2[8];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    *exp = exp_temp;
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_frexp_v16f16_p1v16i32( half16          x,
                                              __global int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half in[16], out1[16];
    int out2[16];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    in[8] = x.s8;
    in[9] = x.s9;
    in[10] = x.sa;
    in[11] = x.sb;
    in[12] = x.sc;
    in[13] = x.sd;
    in[14] = x.se;
    in[15] = x.sf;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    temp.s8 = out1[8];
    temp.s9 = out1[9];
    temp.sa = out1[10];
    temp.sb = out1[11];
    temp.sc = out1[12];
    temp.sd = out1[13];
    temp.se = out1[14];
    temp.sf = out1[15];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    exp_temp.s8 = out2[8];
    exp_temp.s9 = out2[9];
    exp_temp.sa = out2[10];
    exp_temp.sb = out2[11];
    exp_temp.sc = out2[12];
    exp_temp.sd = out2[13];
    exp_temp.se = out2[14];
    exp_temp.sf = out2[15];
    *exp = exp_temp;
    return temp;
}

half __builtin_spirv_OpenCL_frexp_f16_p0i32( half           x,
                                      __private int* exp )
{
    half temp;
    if( __builtin_spirv_OpIsNormal_f16( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f16_f16_i16( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f16_f16( temp, x );

        *exp = ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS) - ( HALF_BIAS - (short)(1) );
    }
    else if( (x == (half)(0.0f)) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = ( x == (float)(0.0f) ) ? (float)(0.0f) : x;
        *exp = 0;
    }
    else
    {
        short m = as_short( x ) & HALF_MANTISSA_MASK;
        short lz = __builtin_spirv_OpenCL_clz_i16( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f16_f16_i16( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f16_f16( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 __builtin_spirv_OpenCL_frexp_v2f16_p0v2i32( half2           x,
                                           __private int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_frexp_v3f16_p0v3i32( half3           x,
                                           __private int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    *exp = exp_temp;
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_frexp_v4f16_p0v4i32( half4           x,
                                           __private int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    *exp = exp_temp;
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_frexp_v8f16_p0v8i32( half8           x,
                                           __private int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half in[8], out1[8];
    int out2[8];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    *exp = exp_temp;
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_frexp_v16f16_p0v16i32( half16           x,
                                              __private int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half in[16], out1[16];
    int out2[16];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    in[8] = x.s8;
    in[9] = x.s9;
    in[10] = x.sa;
    in[11] = x.sb;
    in[12] = x.sc;
    in[13] = x.sd;
    in[14] = x.se;
    in[15] = x.sf;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    temp.s8 = out1[8];
    temp.s9 = out1[9];
    temp.sa = out1[10];
    temp.sb = out1[11];
    temp.sc = out1[12];
    temp.sd = out1[13];
    temp.se = out1[14];
    temp.sf = out1[15];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    exp_temp.s8 = out2[8];
    exp_temp.s9 = out2[9];
    exp_temp.sa = out2[10];
    exp_temp.sb = out2[11];
    exp_temp.sc = out2[12];
    exp_temp.sd = out2[13];
    exp_temp.se = out2[14];
    exp_temp.sf = out2[15];
    *exp = exp_temp;
    return temp;
}

half __builtin_spirv_OpenCL_frexp_f16_p3i32( half         x,
                                      __local int* exp )
{
    half temp;
    if( __builtin_spirv_OpIsNormal_f16( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f16_f16_i16( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f16_f16( temp, x );

        *exp = ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS) - ( HALF_BIAS - (short)(1) );
    }
    else if( (x == (half)(0.0f)) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = ( x == (float)(0.0f) ) ? (float)(0.0f) : x;
        *exp = 0;
    }
    else
    {
        short m = as_short( x ) & HALF_MANTISSA_MASK;
        short lz = __builtin_spirv_OpenCL_clz_i16( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f16_f16_i16( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f16_f16( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 __builtin_spirv_OpenCL_frexp_v2f16_p3v2i32( half2         x,
                                           __local int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_frexp_v3f16_p3v3i32( half3         x,
                                           __local int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    *exp = exp_temp;
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_frexp_v4f16_p3v4i32( half4         x,
                                           __local int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    *exp = exp_temp;
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_frexp_v8f16_p3v8i32( half8         x,
                                           __local int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half in[8], out1[8];
    int out2[8];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    *exp = exp_temp;
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_frexp_v16f16_p3v16i32( half16         x,
                                              __local int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half in[16], out1[16];
    int out2[16];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    in[8] = x.s8;
    in[9] = x.s9;
    in[10] = x.sa;
    in[11] = x.sb;
    in[12] = x.sc;
    in[13] = x.sd;
    in[14] = x.se;
    in[15] = x.sf;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    temp.s8 = out1[8];
    temp.s9 = out1[9];
    temp.sa = out1[10];
    temp.sb = out1[11];
    temp.sc = out1[12];
    temp.sd = out1[13];
    temp.se = out1[14];
    temp.sf = out1[15];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    exp_temp.s8 = out2[8];
    exp_temp.s9 = out2[9];
    exp_temp.sa = out2[10];
    exp_temp.sb = out2[11];
    exp_temp.sc = out2[12];
    exp_temp.sd = out2[13];
    exp_temp.se = out2[14];
    exp_temp.sf = out2[15];
    *exp = exp_temp;
    return temp;
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half __builtin_spirv_OpenCL_frexp_f16_p4i32( half           x,
                                      __generic int* exp )
{
    half temp;
    if( __builtin_spirv_OpIsNormal_f16( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f16_f16_i16( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f16_f16( temp, x );

        *exp = ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS) - ( HALF_BIAS - (short)(1) );
    }
    else if( (x == (half)(0.0f)) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = ( x == (float)(0.0f) ) ? (float)(0.0f) : x;
        *exp = 0;
    }
    else
    {
        short m = as_short( x ) & HALF_MANTISSA_MASK;
        short lz = __builtin_spirv_OpenCL_clz_i16( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f16_f16_i16( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __builtin_spirv_OpenCL_copysign_f16_f16( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 __builtin_spirv_OpenCL_frexp_v2f16_p4v2i32( half2           x,
                                           __generic int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_frexp_v3f16_p4v3i32( half3           x,
                                           __generic int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    *exp = exp_temp;
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_frexp_v4f16_p4v4i32( half4           x,
                                           __generic int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    *exp = exp_temp;
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_frexp_v8f16_p4v8i32( half8           x,
                                           __generic int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half in[8], out1[8];
    int out2[8];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    *exp = exp_temp;
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_frexp_v16f16_p4v16i32( half16           x,
                                              __generic int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half in[16], out1[16];
    int out2[16];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    in[4] = x.s4;
    in[5] = x.s5;
    in[6] = x.s6;
    in[7] = x.s7;
    in[8] = x.s8;
    in[9] = x.s9;
    in[10] = x.sa;
    in[11] = x.sb;
    in[12] = x.sc;
    in[13] = x.sd;
    in[14] = x.se;
    in[15] = x.sf;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f16_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    temp.s2 = out1[2];
    temp.s3 = out1[3];
    temp.s4 = out1[4];
    temp.s5 = out1[5];
    temp.s6 = out1[6];
    temp.s7 = out1[7];
    temp.s8 = out1[8];
    temp.s9 = out1[9];
    temp.sa = out1[10];
    temp.sb = out1[11];
    temp.sc = out1[12];
    temp.sd = out1[13];
    temp.se = out1[14];
    temp.sf = out1[15];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    exp_temp.s2 = out2[2];
    exp_temp.s3 = out2[3];
    exp_temp.s4 = out2[4];
    exp_temp.s5 = out2[5];
    exp_temp.s6 = out2[6];
    exp_temp.s7 = out2[7];
    exp_temp.s8 = out2[8];
    exp_temp.s9 = out2[9];
    exp_temp.sa = out2[10];
    exp_temp.sb = out2[11];
    exp_temp.sc = out2[12];
    exp_temp.sd = out2[13];
    exp_temp.se = out2[14];
    exp_temp.sf = out2[15];
    *exp = exp_temp;
    return temp;
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// hypot

float __builtin_spirv_OpenCL_hypot_f32_f32( float x, float y )
{
    // Note: This isn't well specified, but apparently conformance
    // tests expect the following behavior:
    // 1) If either input is infinity, return infinity.
    // 2) Else, if either input is NaN, return NaN.
    // 3) Else, if both inputs are zero, return zero.
    // 4) Else, return sqrt( x * x, y * y )

    float result;

    if( __intel_relaxed_isinf( x ) |
        __intel_relaxed_isinf( y ) )
    {
        result = INFINITY;
    }
    else if( __intel_relaxed_isnan( x ) |
             __intel_relaxed_isnan( y ) )
    {
        result = __builtin_spirv_OpenCL_nan_i32(0u);
    }
    else
    {
        x = __builtin_spirv_OpenCL_fabs_f32( x );
        y = __builtin_spirv_OpenCL_fabs_f32( y );
        float maxc = x > y ? x : y;
        float minc = x > y ? y : x;
        if( maxc == 0.0f )
        {
            result = 0.0f;
        }
        else
        {
            // Scale all components down by the biggest component.
            // Compute the length of this scaled vector, then scale
            // back up to compute the actual length.  Since this is
            // a vec2 normalize, we have it a bit easier, since we
            // know the other component is the min component.
            float s = minc / maxc;
            float t = __builtin_spirv_OpenCL_sqrt_f32( __builtin_spirv_OpenCL_mad_f32_f32_f32( s, s, 1.0f ) );
            result = t * maxc;
        }
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_hypot, float, float, float, f32, f32 )

#if defined(cl_khr_fp16)

half __builtin_spirv_OpenCL_hypot_f16_f16( half x, half y )
{
    return (half)__builtin_spirv_OpenCL_hypot_f32_f32(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_hypot, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

// ilogb

int __builtin_spirv_OpenCL_ilogb_f32( float x )
{
    int result = 0;

    if( __intel_relaxed_isnormal( x ) )
    {
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS;
    }
    else if( __intel_relaxed_isnan( x ) |
             __intel_relaxed_isinf( x ) )
    {
        result = FP_ILOGBNAN;
    }
    else if( x == 0.0f )
    {
        result = FP_ILOGB0;
    }
    else
    {
        x = x * ( 1 << FLOAT_MANTISSA_BITS );
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS - FLOAT_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_ilogb, int, float, f32 )

#if defined(cl_khr_fp16)

INLINE int __builtin_spirv_OpenCL_ilogb_f16( half x )
{
    return __builtin_spirv_OpenCL_ilogb_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_ilogb, int, half, f16 )

#endif // defined(cl_khr_fp16)

// ldexp

float __builtin_spirv_OpenCL_ldexp_f32_i32( float x, int n )
{
    int delta = 0;
    float m0 = 1.0f;
    m0 = ( n < (FLT_MIN_EXP+1) ) ? as_float( ( (FLT_MIN_EXP+1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : m0;
    m0 = ( n > (FLT_MAX_EXP-1) ) ? as_float( ( (FLT_MAX_EXP-1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : m0;
    delta = ( n < (FLT_MIN_EXP+1) ) ? (FLT_MIN_EXP+1) : 0;
    delta = ( n > (FLT_MAX_EXP-1) ) ? (FLT_MAX_EXP-1) : delta;
    n -= delta;

    float m1 = 1.0f;
    m1 = ( n < (FLT_MIN_EXP+1) ) ? as_float( ( (FLT_MIN_EXP+1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : m1;
    m1 = ( n > (FLT_MAX_EXP-1) ) ? as_float( ( (FLT_MAX_EXP-1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : m1;
    delta = ( n < (FLT_MIN_EXP+1) ) ? (FLT_MIN_EXP+1) : 0;
    delta = ( n > (FLT_MAX_EXP-1) ) ? (FLT_MAX_EXP-1) : delta;
    n -= delta;

    float mn = as_float( ( n + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS );
    mn = ( n == 0 ) ? 1.0f : mn;
    mn = ( n < (FLT_MIN_EXP+1) ) ? as_float( ( (FLT_MIN_EXP+1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : mn;
    mn = ( n > (FLT_MAX_EXP-1) ) ? as_float( ( (FLT_MAX_EXP-1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : mn;

    float res = x * mn * m0 * m1;

    return res;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV( __builtin_spirv_OpenCL_ldexp, float, float, int, f32, i32 )

#if defined(cl_khr_fp16)

half __builtin_spirv_OpenCL_ldexp_f16_i32( half x, int n )
{
    float mn = as_float( ( n + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS );
    mn = ( n == 0 ) ? 1.0f : mn;
    mn = ( n < (FLT_MIN_EXP+1) ) ? as_float( ( (FLT_MIN_EXP+1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : mn;
    mn = ( n > (FLT_MAX_EXP-1) ) ? as_float( ( (FLT_MAX_EXP-1) + FLOAT_BIAS ) << FLOAT_MANTISSA_BITS ) : mn;

    float res = x * mn;

    return res;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV( __builtin_spirv_OpenCL_ldexp, half, half, int, f16, i32 )

#endif // defined(cl_khr_fp16)

// lgamma_r

INLINE float __builtin_spirv_OpenCL_lgamma_r_f32_p1i32( float         x,
                                          __global int* signp )
{
    int     s;
    float   r = __builtin_spirv_OpenCL_lgamma_r_f32_p0i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p1v2i32( float2         x,
                                               __global int2* signp )
{
    int2    s;
    float2  r = __builtin_spirv_OpenCL_lgamma_r_v2f32_p0v2i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p1v3i32( float3         x,
                                               __global int3* signp )
{
    int3    s;
    float3  r = __builtin_spirv_OpenCL_lgamma_r_v3f32_p0v3i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p1v4i32( float4         x,
                                               __global int4* signp )
{
    int4    s;
    float4  r = __builtin_spirv_OpenCL_lgamma_r_v4f32_p0v4i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p1v8i32( float8         x,
                                               __global int8* signp )
{
    int8    s;
    float8  r = __builtin_spirv_OpenCL_lgamma_r_v8f32_p0v8i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p1v16i32( float16         x,
                                                  __global int16* signp )
{
    int16   s;
    float16 r = __builtin_spirv_OpenCL_lgamma_r_v16f32_p0v16i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float __builtin_spirv_OpenCL_lgamma_r_f32_p3i32( float        x,
                                          __local int* signp )
{
    int     s;
    float   r = __builtin_spirv_OpenCL_lgamma_r_f32_p0i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p3v2i32( float2        x,
                                               __local int2* signp )
{
    int2    s;
    float2  r = __builtin_spirv_OpenCL_lgamma_r_v2f32_p0v2i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p3v3i32( float3        x,
                                               __local int3* signp )
{
    int3    s;
    float3  r = __builtin_spirv_OpenCL_lgamma_r_v3f32_p0v3i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p3v4i32( float4        x,
                                               __local int4* signp )
{
    int4    s;
    float4  r = __builtin_spirv_OpenCL_lgamma_r_v4f32_p0v4i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p3v8i32( float8        x,
                                               __local int8* signp )
{
    int8    s;
    float8  r = __builtin_spirv_OpenCL_lgamma_r_v8f32_p0v8i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p3v16i32( float16        x,
                                                  __local int16* signp )
{
    int16   s;
    float16 r = __builtin_spirv_OpenCL_lgamma_r_v16f32_p0v16i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( float          x,
                                          __private int* signp )
{
    int     s;
    float   r;
    if( __intel_relaxed_isnan(x) )
    {
        r = __builtin_spirv_OpenCL_nan_i32(0U);
        s = 0;
    }
    else
    {
        float g = __builtin_spirv_OpenCL_tgamma_f32(x);
        r = __intel_relaxed_isnan(g) ? INFINITY : __builtin_spirv_OpenCL_native_log_f32(__builtin_spirv_OpenCL_fabs_f32(g));
        s = __builtin_spirv_OpenCL_sign_f32(g);
    }
    signp[0] = s;
    return r;
}

INLINE float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p0v2i32( float2          x,
                                               __private int2* signp )
{
    float2  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 2; i++)
    {
        r_scalar[i] = __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p0v3i32( float3          x,
                                               __private int3* signp )
{
    float3  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 3; i++)
    {
        r_scalar[i] = __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p0v4i32( float4          x,
                                               __private int4* signp )
{
    float4  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 4; i++)
    {
        r_scalar[i] = __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p0v8i32( float8          x,
                                               __private int8* signp )
{
    float8  r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 8; i++)
    {
        r_scalar[i] = __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( px[i], sign_scalar + i );
    }
    return r;
}

INLINE float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p0v16i32( float16          x,
                                                  __private int16* signp )
{
    float16 r;
    const __private float* px = (const __private float*)&x;
    __private int*      sign_scalar = (__private int*)signp;
    __private float*    r_scalar = (__private float*)&r;
    for(uint i = 0; i < 16; i++)
    {
        r_scalar[i] = __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( px[i], sign_scalar + i );
    }
    return r;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __builtin_spirv_OpenCL_lgamma_r_f32_p4i32( float          x,
                                          __generic int* signp )
{
    int     s;
    float   r = __builtin_spirv_OpenCL_lgamma_r_f32_p0i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float2 __builtin_spirv_OpenCL_lgamma_r_v2f32_p4v2i32( float2          x,
                                               __generic int2* signp )
{
    int2    s;
    float2  r = __builtin_spirv_OpenCL_lgamma_r_v2f32_p0v2i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float3 __builtin_spirv_OpenCL_lgamma_r_v3f32_p4v3i32( float3          x,
                                               __generic int3* signp )
{
    int3    s;
    float3  r = __builtin_spirv_OpenCL_lgamma_r_v3f32_p0v3i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float4 __builtin_spirv_OpenCL_lgamma_r_v4f32_p4v4i32( float4          x,
                                               __generic int4* signp )
{
    int4    s;
    float4  r = __builtin_spirv_OpenCL_lgamma_r_v4f32_p0v4i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float8 __builtin_spirv_OpenCL_lgamma_r_v8f32_p4v8i32( float8          x,
                                               __generic int8* signp )
{
    int8    s;
    float8  r = __builtin_spirv_OpenCL_lgamma_r_v8f32_p0v8i32(x, &s);
    signp[0] = s;
    return r;
}

INLINE float16 __builtin_spirv_OpenCL_lgamma_r_v16f32_p4v16i32( float16          x,
                                                  __generic int16* signp )
{
    int16   s;
    float16 r = __builtin_spirv_OpenCL_lgamma_r_v16f32_p0v16i32(x, &s);
    signp[0] = s;
    return r;
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_lgamma_r_f16_p1i32( half          x,
                                         __global int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)( __builtin_spirv_OpenCL_lgamma_r_f32_p1i32( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), signp ) );
}

INLINE half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p1v2i32( half2          x,
                                              __global int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)( __builtin_spirv_OpenCL_lgamma_r_v2f32_p1v2i32( SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(x), signp ) );
}

INLINE half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p1v3i32( half3          x,
                                              __global int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)( __builtin_spirv_OpenCL_lgamma_r_v3f32_p1v3i32( SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(x), signp ) );
}

INLINE half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p1v4i32( half4          x,
                                              __global int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)( __builtin_spirv_OpenCL_lgamma_r_v4f32_p1v4i32( SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(x), signp ) );
}

INLINE half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p1v8i32( half8          x,
                                              __global int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)( __builtin_spirv_OpenCL_lgamma_r_v8f32_p1v8i32( SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(x), signp ) );
}

INLINE half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p1v16i32( half16          x,
                                                 __global int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)( __builtin_spirv_OpenCL_lgamma_r_v16f32_p1v16i32( SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(x), signp ) );
}

INLINE half __builtin_spirv_OpenCL_lgamma_r_f16_p0i32( half           x,
                                         __private int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)( __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), signp ) );
}

INLINE half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p0v2i32( half2           x,
                                              __private int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)( __builtin_spirv_OpenCL_lgamma_r_v2f32_p0v2i32( SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(x), signp ) );
}

INLINE half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p0v3i32( half3           x,
                                              __private int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)( __builtin_spirv_OpenCL_lgamma_r_v3f32_p0v3i32( SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(x), signp ) );
}

INLINE half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p0v4i32( half4           x,
                                              __private int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)( __builtin_spirv_OpenCL_lgamma_r_v4f32_p0v4i32( SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(x), signp ) );
}

INLINE half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p0v8i32( half8           x,
                                              __private int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)( __builtin_spirv_OpenCL_lgamma_r_v8f32_p0v8i32( SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(x), signp ) );
}

INLINE half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p0v16i32( half16           x,
                                                 __private int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)( __builtin_spirv_OpenCL_lgamma_r_v16f32_p0v16i32( SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(x), signp ) );
}

INLINE half __builtin_spirv_OpenCL_lgamma_r_f16_p3i32( half         x,
                                         __local int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)( __builtin_spirv_OpenCL_lgamma_r_f32_p3i32( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), signp ) );
}

INLINE half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p3v2i32( half2         x,
                                              __local int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)( __builtin_spirv_OpenCL_lgamma_r_v2f32_p3v2i32( SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(x), signp ) );
}

INLINE half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p3v3i32( half3         x,
                                              __local int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)( __builtin_spirv_OpenCL_lgamma_r_v3f32_p3v3i32( SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(x), signp ) );
}

INLINE half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p3v4i32( half4         x,
                                              __local int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)( __builtin_spirv_OpenCL_lgamma_r_v4f32_p3v4i32( SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(x), signp ) );
}

INLINE half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p3v8i32( half8         x,
                                              __local int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)( __builtin_spirv_OpenCL_lgamma_r_v8f32_p3v8i32( SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(x), signp ) );
}

INLINE half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p3v16i32( half16         x,
                                                 __local int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)( __builtin_spirv_OpenCL_lgamma_r_v16f32_p3v16i32( SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(x), signp ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __builtin_spirv_OpenCL_lgamma_r_f16_p4i32( half           x,
                                         __generic int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)( __builtin_spirv_OpenCL_lgamma_r_f32_p4i32( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), signp ) );
}

INLINE half2 __builtin_spirv_OpenCL_lgamma_r_v2f16_p4v2i32( half2           x,
                                              __generic int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f16_v2f32, _Rhalf2)( __builtin_spirv_OpenCL_lgamma_r_v2f32_p4v2i32( SPIRV_BUILTIN(FConvert, _v2f32_v2f16, _Rfloat2)(x), signp ) );
}

INLINE half3 __builtin_spirv_OpenCL_lgamma_r_v3f16_p4v3i32( half3           x,
                                              __generic int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f16_v3f32, _Rhalf3)( __builtin_spirv_OpenCL_lgamma_r_v3f32_p4v3i32( SPIRV_BUILTIN(FConvert, _v3f32_v3f16, _Rfloat3)(x), signp ) );
}

INLINE half4 __builtin_spirv_OpenCL_lgamma_r_v4f16_p4v4i32( half4           x,
                                              __generic int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f16_v4f32, _Rhalf4)( __builtin_spirv_OpenCL_lgamma_r_v4f32_p4v4i32( SPIRV_BUILTIN(FConvert, _v4f32_v4f16, _Rfloat4)(x), signp ) );
}

INLINE half8 __builtin_spirv_OpenCL_lgamma_r_v8f16_p4v8i32( half8           x,
                                              __generic int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f16_v8f32, _Rhalf8)( __builtin_spirv_OpenCL_lgamma_r_v8f32_p4v8i32( SPIRV_BUILTIN(FConvert, _v8f32_v8f16, _Rfloat8)(x), signp ) );
}

INLINE half16 __builtin_spirv_OpenCL_lgamma_r_v16f16_p4v16i32( half16           x,
                                                 __generic int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f16_v16f32, _Rhalf16)( __builtin_spirv_OpenCL_lgamma_r_v16f32_p4v16i32( SPIRV_BUILTIN(FConvert, _v16f32_v16f16, _Rfloat16)(x), signp ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // (cl_khr_fp16)

// lgamma

INLINE float __builtin_spirv_OpenCL_lgamma_f32( float x )
{
    float r;
    if( __intel_relaxed_isnan(x) )
    {
        r = __builtin_spirv_OpenCL_nan_i32(0U);
    }
    else
    {
        float g = __builtin_spirv_OpenCL_tgamma_f32(x);
        r = __builtin_spirv_OpIsNan_f32(g) ? INFINITY : __builtin_spirv_OpenCL_native_log_f32(__builtin_spirv_OpenCL_fabs_f32(g));
    }
    return r;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_lgamma, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_lgamma_f16( half x )
{
    return __builtin_spirv_OpenCL_lgamma_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_lgamma, half, half, f16 )

#endif // defined(cl_khr_fp16)

// log

float __builtin_spirv_OpenCL_log_f32( float x )
{
#if 0
    // This version is ever so slightly faster (<1%) than the version below,
    // however it is almost a full ULP less precise in some cases, so we'll
    // stick with the full expansion for now.
    return __builtin_spirv_OpenCL_log2_f32(x) * M_LN2_F;
#else
    float result;

    if(__FastRelaxedMath)
    {
        result = __builtin_spirv_OpenCL_native_log_f32(x);
    }
    //  Denorm checking is to work-around a llvm issue that demote
    //  "(float) x > 0.0f"  to " (half)x > (half)0.0f" (log(half).
    //  This causes the inaccurate result with -cl-denorms-are-zero.
    else if( __intel_relaxed_isfinite(x) &
             ((!__FlushDenormals & (x > 0.0f)) |
              ( __FlushDenormals & (as_int(x) > 0x7FFFFF))) )
    //else if( __intel_relaxed_isfinite(x) & ( x > 0.0f ) )
    {
        if(__UseMathWithLUT)
        {
            result = __ocl_svml_logf(x);
        }
        else
        {
        // We already know that we're positive and finite, so
        // we can use this very cheap check for normal vs.
        // subnormal inputs:
        float s = x * ( 1 << FLOAT_MANTISSA_BITS );
        float e = ( x < FLT_MIN ) ? -FLOAT_MANTISSA_BITS : 0.0f;
        x = ( x < FLT_MIN ) ? s : x;

        const int   magic = 0x3f2aaaab;
        int iX = as_int(x) - magic;
        int iR = ( iX & FLOAT_MANTISSA_MASK ) + magic;

        e += iX >> FLOAT_MANTISSA_BITS;

        float sR = as_float(iR) - 1.0f;

        float sP = as_float(0xbe0402c8);
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0x3e0f335d));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0xbdf9889e));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0x3e0f6b8c));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0xbe2acee6));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0x3e4ce814));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0xbe7fff78));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0x3eaaaa83));
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, as_float(0xbf000000));

        sP = sP * sR;
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( sP, sR, sR);

        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( e, as_float(0x35bfbe8e), sP);
        sP = __builtin_spirv_OpenCL_fma_f32_f32_f32( e, as_float(0x3f317200), sP);

        result = sP;
        }
    }
    else
    {
        // If we get here, we're either infinity, NaN, or negative.
        // The native log2 handles all of these cases.  Note, we don't
        // have to multiply by M_LN2_F, since the result in
        // these cases is NaN or +/- infinity, therefore the multiply
        // is irrelevant and unnecessary.
        result = __builtin_spirv_OpenCL_native_log2_f32(x);
    }

    return result;
#endif
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_log_f16( half x )
{
    return (half)__builtin_spirv_OpenCL_log_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log, half, half, f16 )

#endif // defined(cl_khr_fp16)

// log1p

INLINE float __builtin_spirv_OpenCL_log1p_f32( float x )
{
    if(__FastRelaxedMath && (!__APIRS))
    {
        return __builtin_spirv_OpenCL_log_f32( x + 1.0f );
    }
    else
    {
        return __ocl_svml_log1pf(x);
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log1p, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_log1p_f16( half x )
{
    return __builtin_spirv_OpenCL_log1p_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log1p, half, half, f16 )

#endif // defined(cl_khr_fp16)

// log2

INLINE float __builtin_spirv_OpenCL_log2_f32( float x )
{
    float result;

    if(__FastRelaxedMath)
    {
        result = __builtin_spirv_OpenCL_native_log2_f32(x);
    }
    else
    {
        result = __ocl_svml_log2f(x);
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log2, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_log2_f16( half x )
{
    return __builtin_spirv_OpenCL_log2_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log2, half, half, f16 )

#endif // defined(cl_khr_fp16)

// log10

#define _M_LOG10_E_DBL  (as_double(0x3fdbcb7b1526e50e)) // 0.4342944819032518276511289

INLINE float __builtin_spirv_OpenCL_log10_f32( float x )
{
    float result;

    if(__FastRelaxedMath)
    {
        result = __builtin_spirv_OpenCL_native_log10_f32(x);
    }
    else
    {
        result = __ocl_svml_log10f(x);
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log10, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_log10_f16( half x )
{
    return __builtin_spirv_OpenCL_log10_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log10, half, half, f16 )

#endif // defined(cl_khr_fp16)

// logb

float __builtin_spirv_OpenCL_logb_f32( float x )
{
    float result = 0.0f;

    if( __intel_relaxed_isnormal( x ) )
    {
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS;
    }
    else if( __intel_relaxed_isnan( x ) )
    {
        result = __builtin_spirv_OpenCL_nan_i32(0U);
    }
    else if( __intel_relaxed_isinf( x ) )
    {
        result = INFINITY;
    }
    else if( x == 0.0f )
    {
        result = -INFINITY;
    }
    else
    {
        x = x * ( 1 << FLOAT_MANTISSA_BITS );
        result = ( (as_int(x) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS) - FLOAT_BIAS - FLOAT_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_logb, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_logb_f16( half x )
{
    return (half)__builtin_spirv_OpenCL_logb_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_logb, half, half, f16 )

#endif // defined(cl_khr_fp16)

// mad

INLINE float __builtin_spirv_OpenCL_mad_f32_f32_f32( float a, float b, float c )
{
    return __builtin_spirv_OpenCL_fma_f32_f32_f32(a,b,c);
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mad, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mad, half, half, f16 )

#endif // defined(cl_khr_fp16)

// maxmag

INLINE float __builtin_spirv_OpenCL_maxmag_f32_f32( float x, float y )
{
    float fx = __builtin_spirv_OpenCL_fabs_f32(x);
    float fy = __builtin_spirv_OpenCL_fabs_f32(y);
    float m = __builtin_spirv_OpenCL_fmax_f32_f32(x, y);
    return fx > fy ? x
        : fx < fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_maxmag, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_maxmag_f16_f16( half x, half y )
{
    half fx = __builtin_spirv_OpenCL_fabs_f16(x);
    half fy = __builtin_spirv_OpenCL_fabs_f16(y);
    half m = __builtin_spirv_OpenCL_fmax_f16_f16(x, y);
    return fx > fy ? x
        : fx < fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_maxmag, half, half, f16 )

#endif // defined(cl_khr_fp16)

// minmag

INLINE float __builtin_spirv_OpenCL_minmag_f32_f32( float x, float y )
{
    float fx = __builtin_spirv_OpenCL_fabs_f32(x);
    float fy = __builtin_spirv_OpenCL_fabs_f32(y);
    float m = __builtin_spirv_OpenCL_fmin_f32_f32(x, y);
    return fx < fy ? x
        : fx > fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_minmag, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_minmag_f16_f16( half x, half y )
{
    half fx = __builtin_spirv_OpenCL_fabs_f16(x);
    half fy = __builtin_spirv_OpenCL_fabs_f16(y);
    half m = __builtin_spirv_OpenCL_fmin_f16_f16(x, y);
    return fx < fy ? x
        : fx > fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_minmag, half, half, f16 )

#endif // defined(cl_khr_fp16)

// modf

INLINE float __builtin_spirv_OpenCL_modf_f32_p1f32( float           x,
                                      __global float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f32(x);
    return __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x) ? (float)(0.0f) : x - *iptr), x);
}

INLINE float2 __builtin_spirv_OpenCL_modf_v2f32_p1v2f32( float2           x,
                                           __global float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f32(x);
    float2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_modf_v3f32_p1v3f32( float3           x,
                                           __global float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f32(x);
    float3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_modf_v4f32_p1v4f32( float4           x,
                                           __global float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f32(x);
    float4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_modf_v8f32_p1v8f32( float8           x,
                                           __global float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f32(x);
    float8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_modf_v16f32_p1v16f32( float16           x,
                                              __global float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f32(x);
    float16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s8) ? (float)(0.0f) : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s9) ? (float)(0.0f) : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sa) ? (float)(0.0f) : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sb) ? (float)(0.0f) : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sc) ? (float)(0.0f) : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sd) ? (float)(0.0f) : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.se) ? (float)(0.0f) : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sf) ? (float)(0.0f) : temp.sf), x.sf);
    return temp;
}

INLINE float __builtin_spirv_OpenCL_modf_f32_p0f32( float            x,
                                      __private float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f32(x);
    return __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x) ? (float)(0.0f) : x - *iptr), x);
}

INLINE float2 __builtin_spirv_OpenCL_modf_v2f32_p0v2f32( float2            x,
                                           __private float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f32(x);
    float2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_modf_v3f32_p0v3f32( float3            x,
                                           __private float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f32(x);
    float3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_modf_v4f32_p0v4f32( float4            x,
                                           __private float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f32(x);
    float4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_modf_v8f32_p0v8f32( float8            x,
                                           __private float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f32(x);
    float8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_modf_v16f32_p0v16f32( float16            x,
                                              __private float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f32(x);
    float16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s8) ? (float)(0.0f) : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s9) ? (float)(0.0f) : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sa) ? (float)(0.0f) : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sb) ? (float)(0.0f) : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sc) ? (float)(0.0f) : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sd) ? (float)(0.0f) : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.se) ? (float)(0.0f) : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sf) ? (float)(0.0f) : temp.sf), x.sf);
    return temp;
}

INLINE float __builtin_spirv_OpenCL_modf_f32_p3f32( float          x,
                                      __local float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f32(x);
    return __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x) ? (float)(0.0f) : x - *iptr), x);
}

INLINE float2 __builtin_spirv_OpenCL_modf_v2f32_p3v2f32( float2          x,
                                           __local float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f32(x);
    float2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_modf_v3f32_p3v3f32( float3          x,
                                           __local float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f32(x);
    float3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_modf_v4f32_p3v4f32( float4          x,
                                           __local float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f32(x);
    float4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_modf_v8f32_p3v8f32( float8          x,
                                           __local float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f32(x);
    float8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_modf_v16f32_p3v16f32( float16          x,
                                              __local float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f32(x);
    float16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s8) ? (float)(0.0f) : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s9) ? (float)(0.0f) : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sa) ? (float)(0.0f) : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sb) ? (float)(0.0f) : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sc) ? (float)(0.0f) : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sd) ? (float)(0.0f) : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.se) ? (float)(0.0f) : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sf) ? (float)(0.0f) : temp.sf), x.sf);
    return temp;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __builtin_spirv_OpenCL_modf_f32_p4f32( float            x,
                                      __generic float* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f32(x);
    return __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x) ? (float)(0.0f) : x - *iptr), x);
}

INLINE float2 __builtin_spirv_OpenCL_modf_v2f32_p4v2f32( float2            x,
                                           __generic float2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f32(x);
    float2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_modf_v3f32_p4v3f32( float3            x,
                                           __generic float3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f32(x);
    float3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_modf_v4f32_p4v4f32( float4            x,
                                           __generic float4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f32(x);
    float4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_modf_v8f32_p4v8f32( float8            x,
                                           __generic float8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f32(x);
    float8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_modf_v16f32_p4v16f32( float16            x,
                                              __generic float16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f32(x);
    float16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s0) ? (float)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s1) ? (float)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s2) ? (float)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s3) ? (float)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s4) ? (float)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s5) ? (float)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s6) ? (float)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s7) ? (float)(0.0f) : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s8) ? (float)(0.0f) : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.s9) ? (float)(0.0f) : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sa) ? (float)(0.0f) : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sb) ? (float)(0.0f) : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sc) ? (float)(0.0f) : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sd) ? (float)(0.0f) : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.se) ? (float)(0.0f) : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f32_f32((__intel_relaxed_isinf(x.sf) ? (float)(0.0f) : temp.sf), x.sf);
    return temp;
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

INLINE half __builtin_spirv_OpenCL_modf_f16_p1f16( half           x,
                                     __global half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f16(x);
    return __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x) ? (half)(0.0f) : x - *iptr), x);
}

INLINE half2 __builtin_spirv_OpenCL_modf_v2f16_p1v2f16( half2           x,
                                          __global half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f16(x);
    half2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_modf_v3f16_p1v3f16( half3           x,
                                          __global half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f16(x);
    half3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_modf_v4f16_p1v4f16( half4           x,
                                          __global half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f16(x);
    half4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_modf_v8f16_p1v8f16( half8           x,
                                          __global half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f16(x);
    half8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_modf_v16f16_p1v16f16( half16           x,
                                             __global half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f16(x);
    half16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s8) ? (half)(0.0f) : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s9) ? (half)(0.0f) : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sa) ? (half)(0.0f) : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sb) ? (half)(0.0f) : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sc) ? (half)(0.0f) : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sd) ? (half)(0.0f) : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.se) ? (half)(0.0f) : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sf) ? (half)(0.0f) : temp.sf), x.sf);
    return temp;
}

INLINE half __builtin_spirv_OpenCL_modf_f16_p0f16( half            x,
                                     __private half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f16(x);
    return __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x) ? (half)(0.0f) : x - *iptr), x);
}

INLINE half2 __builtin_spirv_OpenCL_modf_v2f16_p0v2f16( half2            x,
                                          __private half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f16(x);
    half2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_modf_v3f16_p0v3f16( half3            x,
                                          __private half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f16(x);
    half3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_modf_v4f16_p0v4f16( half4            x,
                                          __private half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f16(x);
    half4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_modf_v8f16_p0v8f16( half8            x,
                                          __private half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f16(x);
    half8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_modf_v16f16_p0v16f16( half16            x,
                                             __private half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f16(x);
    half16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s8) ? (half)(0.0f) : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s9) ? (half)(0.0f) : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sa) ? (half)(0.0f) : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sb) ? (half)(0.0f) : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sc) ? (half)(0.0f) : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sd) ? (half)(0.0f) : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.se) ? (half)(0.0f) : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sf) ? (half)(0.0f) : temp.sf), x.sf);
    return temp;
}

INLINE half __builtin_spirv_OpenCL_modf_f16_p3f16( half          x,
                                     __local half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f16(x);
    return __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x) ? (half)(0.0f) : x - *iptr), x);
}

INLINE half2 __builtin_spirv_OpenCL_modf_v2f16_p3v2f16( half2          x,
                                          __local half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f16(x);
    half2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_modf_v3f16_p3v3f16( half3          x,
                                          __local half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f16(x);
    half3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_modf_v4f16_p3v4f16( half4          x,
                                          __local half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f16(x);
    half4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_modf_v8f16_p3v8f16( half8          x,
                                          __local half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f16(x);
    half8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_modf_v16f16_p3v16f16( half16          x,
                                             __local half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f16(x);
    half16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s8) ? (half)(0.0f) : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s9) ? (half)(0.0f) : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sa) ? (half)(0.0f) : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sb) ? (half)(0.0f) : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sc) ? (half)(0.0f) : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sd) ? (half)(0.0f) : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.se) ? (half)(0.0f) : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sf) ? (half)(0.0f) : temp.sf), x.sf);
    return temp;
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __builtin_spirv_OpenCL_modf_f16_p4f16( half            x,
                                     __generic half* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f16(x);
    return __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x) ? (half)(0.0f) : x - *iptr), x);
}

INLINE half2 __builtin_spirv_OpenCL_modf_v2f16_p4v2f16( half2            x,
                                          __generic half2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f16(x);
    half2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_modf_v3f16_p4v3f16( half3            x,
                                          __generic half3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f16(x);
    half3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_modf_v4f16_p4v4f16( half4            x,
                                          __generic half4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f16(x);
    half4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_modf_v8f16_p4v8f16( half8            x,
                                          __generic half8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f16(x);
    half8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_modf_v16f16_p4v16f16( half16            x,
                                             __generic half16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f16(x);
    half16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s0) ? (half)(0.0f) : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s1) ? (half)(0.0f) : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s2) ? (half)(0.0f) : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s3) ? (half)(0.0f) : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s4) ? (half)(0.0f) : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s5) ? (half)(0.0f) : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s6) ? (half)(0.0f) : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s7) ? (half)(0.0f) : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s8) ? (half)(0.0f) : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.s9) ? (half)(0.0f) : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sa) ? (half)(0.0f) : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sb) ? (half)(0.0f) : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sc) ? (half)(0.0f) : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sd) ? (half)(0.0f) : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.se) ? (half)(0.0f) : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f16_f16((__intel_relaxed_isinf(x.sf) ? (half)(0.0f) : temp.sf), x.sf);
    return temp;
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// nan

INLINE float __builtin_spirv_OpenCL_nan_i32( uint nancode )
{
    return as_float( FLOAT_QUIET_NAN );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_nan, float, uint, i32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_nan_i16( ushort nancode )
{
    return as_half( HALF_QUIET_NAN );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_nan, half, ushort, i16 )

#endif // defined(cl_khr_fp16)

// nextafter

float __builtin_spirv_OpenCL_nextafter_f32_f32( float x, float y )
{
    const int maxneg = FLOAT_SIGN_MASK;

    // The way this algorithm works is as follows:
    //
    // - Treat the incoming float as an integer.  Note that the integer
    //   representation of floats are ordered if the integer is interpreted
    //   as being sign-magnitude encoded.
    // - Convert each incoming float from sign-magnitude to twos-complement
    //   encoded, so we can use usual comparison and math operations on them.
    // - Based on the twos complement encoding of the integer representation
    //   of each float, either add or subtract one from the twos-complement
    //   encoding of the integer representation of x.  This gives a twos-
    //   complement encoding of the result.
    // - Convert from the twos-complement encoding of the result back
    //   to a sign-magnitude encoding of the result, and interpret as
    //   a float.  We're done!  Well, almost.
    // - Handle two special-cases:
    //    - When the two floats are equal then there is no delta.
    //    - When either float is NaN the result is NaN.
    //
    // The code is written so it does not need flow control.

    int smix = as_int(x);
    int nx = maxneg - smix;
    int tcix = ( smix < 0 ) ? nx : smix;

    int smiy = as_int(y);
    int ny = maxneg - smiy;
    int tciy = ( smiy < 0 ) ? ny : smiy;

    int delta = ( tcix < tciy ) ? 1 : -1;

    int tcir = tcix + delta;
    int nr = maxneg - tcir;
    int smir = ( tcir < 0 ) ? nr : tcir;

    float result = as_float(smir);
    result = (tcix == tciy) ? y : result;

    {
        float n = __builtin_spirv_OpenCL_nan_i32(0u);
        int test = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_nextafter, float, float, f32 )

#if defined(cl_khr_fp16)

half __builtin_spirv_OpenCL_nextafter_f16_f16( half x, half y )
{
    const short maxneg = HALF_SIGN_MASK;

    short smix = as_short(x);
    short nx = maxneg - smix;
    short tcix = ( smix < 0 ) ? nx : smix;

    short smiy = as_short(y);
    short ny = maxneg - smiy;
    short tciy = ( smiy < 0 ) ? ny : smiy;

    short delta = ( tcix < tciy ) ? 1 : -1;

    short tcir = tcix + delta;
    short nr = maxneg - tcir;
    short smir = ( tcir < 0 ) ? nr : tcir;

    half result = as_half(smir);
    result = (tcix == tciy) ? y : result;

    {
        half n = __builtin_spirv_OpenCL_nan_i32(0u);
        int test = __builtin_spirv_OpIsNan_f16(x) | __builtin_spirv_OpIsNan_f16(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_nextafter, half, half, f16 )

#endif // defined(cl_khr_fp16)

// pow


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
        // Previous version of pow builtin is called here, because new one introduced some critical functional regressions.
        // TODO: The target is to call '__ocl_svml_powf' here.
        return __ocl_svml_px_powf1(x, y);
    }
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pow, float, float, float, f32, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_pow_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_pow_f32_f32((float)x, (float)y );
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pow, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

// pown

INLINE float __builtin_spirv_OpenCL_pown_f32_i32( float x, int y )
{
    if(__FastRelaxedMath && (!__APIRS))
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

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pown, float, float, int, f32, i32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_pown_f16_i32( half x, int y )
{
    return __builtin_spirv_OpenCL_pown_f32_i32((float)x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pown, half, half, int, f16, i32 )

#endif // defined(cl_khr_fp16)

// powr

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
        return __ocl_svml_powrf(x, y);
    }
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_powr, float, float, float, f32, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_powr_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_powr_f32_f32((float)x, (float)y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_powr, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

// remainder

INLINE float __builtin_spirv_OpenCL_remainder_f32_f32( float x, float y )
{
    int temp;
    return __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(x, y, &temp);
}

INLINE float __builtin_spirv_OpFRem_f32_f32( float x, float y )
{
    return __builtin_spirv_OpenCL_fmod_f32_f32(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_remainder, float, float, float, f32, f32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFRem, float, float, float, f32, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_remainder_f16_f16( half y, half x )
{
    return __builtin_spirv_OpenCL_remainder_f32_f32((float)y, (float)x );
}

INLINE half __builtin_spirv_OpFRem_f16_f16( half x, half y )
{
    return __builtin_spirv_OpenCL_fmod_f16_f16(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_remainder, half, half, half, f16, f16 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFRem, half, half, half, f16, f16 )

#endif // defined(cl_khr_fp16)

// remquo

float __builtin_spirv_OpenCL_remquo_f32_f32_p1i32( float         xx,
                                            float         yy,
                                            __global int* quo )
{
    float result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0f) )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_select_f32_f32_i32(__builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx), xx, __intel_relaxed_isinf(yy));
    }
    else if( __builtin_spirv_OpenCL_fabs_f32(xx) == __builtin_spirv_OpenCL_fabs_f32(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = __builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx);
    }
    else
    {
        int signx = __builtin_spirv_OpSignBitSet_f32( xx ) ? -1 : 1;
        int signy = __builtin_spirv_OpSignBitSet_f32( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        float x = __builtin_spirv_OpenCL_fabs_f32(xx);
        float y = __builtin_spirv_OpenCL_fabs_f32(yy);
        int ex, ey;
        ex = __builtin_spirv_OpenCL_ilogb_f32( x );
        ey = __builtin_spirv_OpenCL_ilogb_f32( y );
        float xr = x;
        float yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f32_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ex - ey);
          }
        }
        if( (yr < 2.0f*xr) | ( (yr == 2.0f*xr) & (q & 0x00000001) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ey);
        }

        int qout = q & 0x0000007f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0f )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

INLINE float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p1v2i32( float2         xx,
                                                   float2         yy,
                                                   __global int2* quo )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p1v3i32( float3         xx,
                                                   float3         yy,
                                                   __global int3* quo )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p1v4i32( float4         xx,
                                                   float4         yy,
                                                   __global int4* quo )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p1v8i32( float8         xx,
                                                   float8         yy,
                                                   __global int8* quo )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p1v16i32( float16         xx,
                                                       float16         yy,
                                                       __global int16* quo )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}

float __builtin_spirv_OpenCL_remquo_f32_f32_p0i32( float          xx,
                                            float          yy,
                                            __private int* quo )
{
    float result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0f) )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_select_f32_f32_i32(__builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx), xx, __intel_relaxed_isinf(yy));
    }
    else if( __builtin_spirv_OpenCL_fabs_f32(xx) == __builtin_spirv_OpenCL_fabs_f32(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = __builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx);
    }
    else
    {
        int signx = __builtin_spirv_OpSignBitSet_f32( xx ) ? -1 : 1;
        int signy = __builtin_spirv_OpSignBitSet_f32( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        float x = __builtin_spirv_OpenCL_fabs_f32(xx);
        float y = __builtin_spirv_OpenCL_fabs_f32(yy);
        int ex, ey;
        ex = __builtin_spirv_OpenCL_ilogb_f32( x );
        ey = __builtin_spirv_OpenCL_ilogb_f32( y );
        float xr = x;
        float yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f32_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ex - ey);
          }
        }
        if( (yr < 2.0f*xr) | ( (yr == 2.0f*xr) && (q & 0x00000001) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ey);
        }

        int qout = q & 0x0000007f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0f )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

INLINE float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p0v2i32( float2          xx,
                                                   float2          yy,
                                                   __private int2* quo )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p0v3i32( float3          xx,
                                                   float3          yy,
                                                   __private int3* quo )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p0v4i32( float4          xx,
                                                   float4          yy,
                                                   __private int4* quo )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p0v8i32( float8          xx,
                                                   float8          yy,
                                                   __private int8* quo )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p0v16i32( float16          xx,
                                                       float16          yy,
                                                       __private int16* quo )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}

float __builtin_spirv_OpenCL_remquo_f32_f32_p3i32( float        xx,
                                            float        yy,
                                            __local int* quo )
{
    float result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0f) )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_select_f32_f32_i32(__builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx), xx, __intel_relaxed_isinf(yy));
    }
    else if( __builtin_spirv_OpenCL_fabs_f32(xx) == __builtin_spirv_OpenCL_fabs_f32(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = __builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx);
    }
    else
    {
        int signx = __builtin_spirv_OpSignBitSet_f32( xx ) ? -1 : 1;
        int signy = __builtin_spirv_OpSignBitSet_f32( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        float x = __builtin_spirv_OpenCL_fabs_f32(xx);
        float y = __builtin_spirv_OpenCL_fabs_f32(yy);
        int ex, ey;
        ex = __builtin_spirv_OpenCL_ilogb_f32( x );
        ey = __builtin_spirv_OpenCL_ilogb_f32( y );
        float xr = x;
        float yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f32_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ex - ey);
          }
        }
        if( (yr < 2.0f*xr) | ( (yr == 2.0f*xr) && (q & 0x00000001) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ey);
        }

        int qout = q & 0x0000007f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0f )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

INLINE float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p3v2i32( float2        xx,
                                                   float2        yy,
                                                   __local int2* quo )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p3v3i32( float3        xx,
                                                   float3        yy,
                                                   __local int3* quo )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p3v4i32( float4        xx,
                                                   float4        yy,
                                                   __local int4* quo )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p3v8i32( float8        xx,
                                                   float8        yy,
                                                   __local int8* quo )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p3v16i32( float16        xx,
                                                       float16        yy,
                                                       __local int16* quo )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __builtin_spirv_OpenCL_remquo_f32_f32_p4i32( float          xx,
                                            float          yy,
                                            __generic int* quo )
{
    float result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0f) )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_select_f32_f32_i32(__builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx), xx, __intel_relaxed_isinf(yy));
    }
    else if( __builtin_spirv_OpenCL_fabs_f32(xx) == __builtin_spirv_OpenCL_fabs_f32(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = __builtin_spirv_OpenCL_copysign_f32_f32(0.0f, xx);
    }
    else
    {
        int signx = __builtin_spirv_OpSignBitSet_f32( xx ) ? -1 : 1;
        int signy = __builtin_spirv_OpSignBitSet_f32( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        float x = __builtin_spirv_OpenCL_fabs_f32(xx);
        float y = __builtin_spirv_OpenCL_fabs_f32(yy);
        int ex, ey;
        ex = __builtin_spirv_OpenCL_ilogb_f32( x );
        ey = __builtin_spirv_OpenCL_ilogb_f32( y );
        float xr = x;
        float yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f32_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ex - ey);
          }
        }
        if( (yr < 2.0f*xr) | ( (yr == 2.0f*xr) && (q & 0x00000001) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = __builtin_spirv_OpenCL_ldexp_f32_i32(xr, ey);
        }

        int qout = q & 0x0000007f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0f )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

INLINE float2 __builtin_spirv_OpenCL_remquo_v2f32_v2f32_p4v2i32( float2          xx,
                                                   float2          yy,
                                                   __generic int2* quo )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE float3 __builtin_spirv_OpenCL_remquo_v3f32_v3f32_p4v3i32( float3          xx,
                                                   float3          yy,
                                                   __generic int3* quo )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE float4 __builtin_spirv_OpenCL_remquo_v4f32_v4f32_p4v4i32( float4          xx,
                                                   float4          yy,
                                                   __generic int4* quo )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE float8 __builtin_spirv_OpenCL_remquo_v8f32_v8f32_p4v8i32( float8          xx,
                                                   float8          yy,
                                                   __generic int8* quo )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE float16 __builtin_spirv_OpenCL_remquo_v16f32_v16f32_p4v16i32( float16          xx,
                                                       float16          yy,
                                                       __generic int16* quo )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f32_f32_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16
INLINE half __builtin_spirv_OpenCL_remquo_f16_f16_p1i32( half          xx,
                                           half          yy,
                                           __global int* quo )
{
    float result;
    int   quoVal;
    result = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32((float)xx, (float)yy, (__private int*)&quoVal);
    *quo = (half)quoVal;
    return (half)result;
}

INLINE half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p1v2i32( half2          xx,
                                                  half2          yy,
                                                  __global int2* quo )
{
    half2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p1v3i32( half3          xx,
                                                  half3          yy,
                                                  __global int3* quo )
{
    half3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p1v4i32( half4          xx,
                                                  half4          yy,
                                                  __global int4* quo )
{
    half4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p1v8i32( half8          xx,
                                                  half8          yy,
                                                  __global int8* quo )
{
    half8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p1v16i32( half16          xx,
                                                      half16          yy,
                                                      __global int16* quo )
{
    half16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}

INLINE half __builtin_spirv_OpenCL_remquo_f16_f16_p0i32( half           xx,
                                           half           yy,
                                           __private int* quo )
{
    float result;
    int   quoVal;
    result = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32((float)xx, (float)yy, (__private int*)&quoVal);
    *quo = (half)quoVal;
    return (half)result;
}

INLINE half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p0v2i32( half2           xx,
                                                  half2           yy,
                                                  __private int2* quo )
{
    half2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p0v3i32( half3           xx,
                                                  half3           yy,
                                                  __private int3* quo )
{
    half3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p0v4i32( half4           xx,
                                                  half4           yy,
                                                  __private int4* quo )
{
    half4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p0v8i32( half8           xx,
                                                  half8           yy,
                                                  __private int8* quo )
{
    half8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p0v16i32( half16           xx,
                                                      half16           yy,
                                                      __private int16* quo )
{
    half16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}

INLINE half __builtin_spirv_OpenCL_remquo_f16_f16_p3i32( half         xx,
                                           half         yy,
                                           __local int* quo )
{
    float result;
    int   quoVal;
    result = __builtin_spirv_OpenCL_remquo_f32_f32_p0i32((float)xx, (float)yy, (__private int*)&quoVal);
    *quo = (half)quoVal;
    return (half)result;
}

INLINE half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p3v2i32( half2         xx,
                                                  half2         yy,
                                                  __local int2* quo )
{
    half2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p3v3i32( half3         xx,
                                                  half3         yy,
                                                  __local int3* quo )
{
    half3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p3v4i32( half4         xx,
                                                  half4         yy,
                                                  __local int4* quo )
{
    half4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p3v8i32( half8         xx,
                                                  half8         yy,
                                                  __local int8* quo )
{
    half8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p3v16i32( half16         xx,
                                                      half16         yy,
                                                      __local int16* quo )
{
    half16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __builtin_spirv_OpenCL_remquo_f16_f16_p4i32( half           xx,
                                           half           yy,
                                           __generic int* quo )
{
    float result;
    int   quoVal;
    result = __builtin_spirv_OpenCL_remquo_f32_f32_p4i32((float)xx, (float)yy, (__generic int*)&quoVal);
    *quo = (half)quoVal;
    return (half)result;
}

INLINE half2 __builtin_spirv_OpenCL_remquo_v2f16_v2f16_p4v2i32( half2           xx,
                                                  half2           yy,
                                                  __generic int2* quo )
{
    half2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE half3 __builtin_spirv_OpenCL_remquo_v3f16_v3f16_p4v3i32( half3           xx,
                                                  half3           yy,
                                                  __generic int3* quo )
{
    half3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE half4 __builtin_spirv_OpenCL_remquo_v4f16_v4f16_p4v4i32( half4           xx,
                                                  half4           yy,
                                                  __generic int4* quo )
{
    half4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE half8 __builtin_spirv_OpenCL_remquo_v8f16_v8f16_p4v8i32( half8           xx,
                                                  half8           yy,
                                                  __generic int8* quo )
{
    half8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE half16 __builtin_spirv_OpenCL_remquo_v16f16_v16f16_p4v16i32( half16           xx,
                                                      half16           yy,
                                                      __generic int16* quo )
{
    half16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f16_f16_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// rint

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rint, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rint, half, half, f16 )

#endif // defined(cl_khr_fp16)

// rootn

float __builtin_spirv_OpenCL_rootn_f32_i32( float x, int n )
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
        pr = __builtin_spirv_OpenCL_fabs_f32( pr );
        pr = __builtin_spirv_OpenCL_log2_f32( pr );
        pr = pr * 1.0f / n;
        pr = __builtin_spirv_OpenCL_exp2_f32( pr );
#else
        pr = __builtin_spirv_OpenCL_native_powr_f32_f32( pr, 1.0f / n );
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

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_rootn, float, float, int, f32, i32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_rootn_f16_i32( half y, int x )
{
    return __builtin_spirv_OpenCL_rootn_f32_i32((float)y, x);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_rootn, half, half, int, f16, i32 )

#endif // defined(cl_khr_fp16)

// round

INLINE INLINE float __builtin_spirv_OpenCL_round_f32( float x )
{
    float delta = as_float(0x3EFFFFFF); // one bit less than 0.5f
    float nd = x - delta;
    float pd = x + delta;
    x = ( x < 0 ) ? nd : pd;
    return __builtin_spirv_OpenCL_trunc_f32(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_round, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE INLINE half __builtin_spirv_OpenCL_round_f16( half x )
{
    return __builtin_spirv_OpenCL_round_f32(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_round, half, half, f16 )

#endif // defined(cl_khr_fp16)

// rsqrt

INLINE float __builtin_spirv_OpenCL_rsqrt_f32( float x )
{
    return __builtin_spirv_OpenCL_native_rsqrt_f32(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rsqrt, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_rsqrt_f16( half x )
{
    return __builtin_spirv_OpenCL_native_rsqrt_f16(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rsqrt, half, half, f16 )

#endif // defined(cl_khr_fp16)

// sin

static INLINE float __intel_sin_f32( float x, bool doFast )
{
    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        return __builtin_spirv_OpenCL_native_sin_f32(x);
    }
    else
    {
        if(__UseMathWithLUT)
        {
            return __ocl_svml_sinf(x);
        }
        else
        {
            float abs_float = __builtin_spirv_OpenCL_fabs_f32(x);
            if( abs_float > 10000.0f )
            {
                return libclc_sin_f32(x);
            }
            else
            {
                return __ocl_svml_sinf_noLUT(x);
            }
        }
    }
}

INLINE float __builtin_spirv_OpenCL_sin_f32( float x )
{
    return __intel_sin_f32(x, true);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sin, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_sin_f16( half x )
{
    return (half)__builtin_spirv_OpenCL_sin_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sin, half, half, f16 )

#endif // defined(cl_khr_fp16)

// sincos
static INLINE float __intel_sincos_f32_p0f32( float x, __private float* cosval, bool doFast )
{
    float   sin_x, cos_x;
    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        sin_x = __builtin_spirv_OpenCL_native_sin_f32(x);
        cos_x = __builtin_spirv_OpenCL_native_cos_f32(x);
    }
    else
    {
        if(__UseMathWithLUT)
        {
            __ocl_svml_sincosf(x, &sin_x, &cos_x);
        }
        else
        {
            float abs_float = __builtin_spirv_OpenCL_fabs_f32(x);
            if( abs_float > 10000.0f )
            {
                sin_x = libclc_sin_f32(x);
                cos_x = libclc_cos_f32(x);
            }
            else
            {
                sin_x = __ocl_svml_sincosf_noLUT(x, &cos_x);
            }
        }
    }
    *cosval = cos_x;
    return sin_x;
}

INLINE float __builtin_spirv_OpenCL_sincos_f32_p0f32( float x, __private float* cosval )
{
    return __intel_sincos_f32_p0f32(x, cosval, true);
}

GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( __builtin_spirv_OpenCL_sincos, float, float, float, f32, f32 )

float __builtin_spirv_OpenCL_sincos_f32_p1f32( float           x,
                                        __global float* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

INLINE float __builtin_spirv_OpenCL_sincos_f32_p3f32( float          x,
                                        __local float* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, float, __global, float, f32, p1 )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, float, __local, float, f32, p3 )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float __builtin_spirv_OpenCL_sincos_f32_p4f32( float            x,
                                        __generic float* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, float, __generic, float, f32, p4 )

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_sincos_f16_p0f16( half            x,
                                       __private half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), &cos_x );
    cosval[0] = SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(cos_x);
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(sin_x);
}

GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( __builtin_spirv_OpenCL_sincos, half, half, half, f16, f16 )

INLINE half __builtin_spirv_OpenCL_sincos_f16_p1f16( half           x,
                                       __global half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), &cos_x );
    cosval[0] = SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(cos_x);
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(sin_x);
}

INLINE half __builtin_spirv_OpenCL_sincos_f16_p3f16( half          x,
                                       __local half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), &cos_x );
    cosval[0] = SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(cos_x);
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(sin_x);
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, half, __global, half, f16, p1 )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, half, __local, half, f16, p3 )

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half __builtin_spirv_OpenCL_sincos_f16_p4f16( half            x,
                                       __generic half* cosval )
{
    float   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f32_p0f32( SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(x), &cos_x );
    cosval[0] = SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(cos_x);
    return SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(sin_x);
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, half, __generic, half, f16, p4 )

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp16)

// sinh

float __builtin_spirv_OpenCL_sinh_f32( float x )
{
    float result;

    if(__FastRelaxedMath && (!__APIRS))
    {
        // For most inputs, we'll use the expansion
        //  sinh(x) = 0.5f * ( e^x - e^-x ):
        float pexp = __builtin_spirv_OpenCL_exp_f32(  x );
        float nexp = __builtin_spirv_OpenCL_exp_f32( -x );
        result = 0.5f * ( pexp - nexp );

        // For x close to zero, we'll simply use x.
        // We use 2^-10 as our cutoff value for
        // "close to zero".
        float px = __builtin_spirv_OpenCL_fabs_f32( x );
        result = ( px > as_float(0x3A800000) ) ? result : x;
    }
    else
    {
        result = __ocl_svml_sinhf(x);
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinh, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_sinh_f16( half x )
{
    return __builtin_spirv_OpenCL_sinh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinh, half, half, f16 )

#endif // defined(cl_khr_fp16)

// sinpi

INLINE float __builtin_spirv_OpenCL_sinpi_f32( float x )
{
    bool useNative = __FastRelaxedMath && (!__APIRS);

    if(useNative)
    {
        return __builtin_spirv_OpenCL_sin_f32(x * M_PI_F);
    }
    else
    {
        if(__UseMathWithLUT)
        {
            return __ocl_svml_sinpif(x);
        }
        else
        {
            return __ocl_svml_sinpif_noLUT(x);
        }
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinpi, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_sinpi_f16( half x )
{
    return __builtin_spirv_OpenCL_sinpi_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinpi, half, half, f16 )

#endif // defined(cl_khr_fp16)

// sqrt_cr

extern __constant int __Native64Bit;

#if defined(cl_khr_fp16)

INLINE
half __builtin_spirv_OpenCL_sqrt_cr_f16( half a )
{
    return (half)__builtin_spirv_OpenCL_sqrt_cr_f32((float)a);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sqrt_cr, half, half, f16 )

#endif // define(cl_khr_fp16)

float __builtin_spirv_OpenCL_sqrt_cr_f32( float a )
{
    if (!__CRMacros)
    {
        typedef union binary32
        {
            uint u;
            int  s;
            float f;
        } binary32;

        binary32 fa, y0, onehalf, H0, H1, S0, S1, S, d0, e0;
        int aExp, sExp;
        fa.f = a;
        aExp = (fa.u >> 23) & 0xff;
        uint significand = fa.u & 0x7fffff;

        if (aExp == 0 & significand == 0) {
            /* return +/-zero for +/-zero */
            S.u = fa.u;
        }
        else if (aExp == 0xff) { /* NaN and Inf */
            if ((fa.u & 0x7fffff) == 0) { /* Inf */
                S.u = (fa.u & 0x80000000) ? 0xffc00000 : 0x7f800000;
            } else { /* NaN */
                S.u = fa.u | 0x400000; /* Quiet signalling NaN */
            }
        } else {
            if (fa.u & 0x80000000) { /* Negative normals/denormals */
                if (__FlushDenormals & (aExp == 0))
                    S.u = fa.u & 0x80000000;
                else
                    /* return qNaN for negative normal/denormal values */
                    S.u = 0xffc00000;
            } else if (__FlushDenormals & (aExp == 0)) {
                S.u = 0; // positive denorms
            } else { /* Positive normals/denormals */
                bool denorm = (aExp == 0);

                if (denorm & !__FlushDenormals) {
                    fa.f = __builtin_spirv_OpenCL_ldexp_f32_i32(fa.f, 126);
                }
                else {
                    // Scale a to [1/2, 2)
                    fa.u = (fa.u & 0x00ffffff) | 0x3f000000;
                }

                // Initial approximation
                y0.f = __builtin_spirv_OpenCL_rsqrt_f32(fa.f);
                onehalf.u = 0x3f000000;
                // Step(1), H0 = 1/2y0
                H0.f = onehalf.f * y0.f;
                // Step(2), S0 = a*y0
                S0.f = fa.f * y0.f;
                // Step(3), d0 = 1/2 - S0*H0
                d0.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(-S0.f, H0.f, onehalf.f);
                // Step(4), H1 = H0 + d0*H0
                H1.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(d0.f, H0.f, H0.f);
                // Step(5), S1 = S0 + d0*S0
                S1.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(d0.f, S0.f, S0.f);
                // Step(6), e0 = a - S1*S1
                e0.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(-S1.f, S1.f, fa.f);
                // Step(7), S = S1 + e0*H1
                S.f = __builtin_spirv_OpenCL_fma_f32_f32_f32(e0.f, H1.f, S1.f);

                if (denorm & !__FlushDenormals) {
                    S.f = __builtin_spirv_OpenCL_ldexp_f32_i32(S.f, -126/2);
                }
                else {
                    // Adjust exponent
                    sExp = ((aExp - FLOAT_BIAS) >> 1) + FLOAT_BIAS;
                    S.u = (S.u & 0x007fffff) | (sExp << 23);
                }
            }
        }
        return S.f;
    }
    else
    {
        return FSQRT_IEEE(a);
    }
}

#ifdef cl_fp64_basic_ops

INLINE double __builtin_spirv_OpenCL_sqrt_cr_f64( double x )
{
        return __builtin_spirv_OpenCL_sqrt_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sqrt_cr, double, double, f64 )

#endif // cl_fp64_basic_ops

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sqrt_cr, float, float, f32 )

// sqrt

INLINE float __builtin_spirv_OpenCL_sqrt_f32( float x )
{
    return __builtin_spirv_OpenCL_native_sqrt_f32(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sqrt, float, float, f32 )

#ifdef cl_fp64_basic_ops

INLINE double __builtin_spirv_OpenCL_sqrt_f64( double x )
{
    return __builtin_spirv_OpenCL_native_sqrt_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sqrt, double, double, f64 )

#endif // cl_fp64_basic_ops

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_sqrt_f16( half x )
{
    return __builtin_spirv_OpenCL_native_sqrt_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sqrt, half, half, f16 )

#endif // defined(cl_khr_fp16)

// tan

static INLINE float __intel_tan_f32( float x, bool doFast )
{
    float result;
    if(__FastRelaxedMath && (!__APIRS) && doFast)
    {
        result = __builtin_spirv_OpenCL_native_tan_f32(x);
    }
    else
    {
        if(as_uint(x) == 0x45753168)
        {
            result = as_float(0xBF73F75D);
        }
        else if(as_uint(x) == 0xC5753168)
        {
            result = as_float(0x3F73F75D);
        }
        else
        {
            result = __ocl_svml_tanf(x);
        }
    }
    return result;
}

INLINE float __builtin_spirv_OpenCL_tan_f32( float x )
{
    return __intel_tan_f32(x, true);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_tan_f16( half x )
{
    return __builtin_spirv_OpenCL_tan_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, half, half, f16 )

#endif // defined(cl_khr_fp16)

// tanh

float __builtin_spirv_OpenCL_tanh_f32( float x )
{
    float result;

    if( __intel_relaxed_isnan(x) )
    {
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }
    else if(__builtin_spirv_OpenCL_fabs_f32(x) < as_float(0x3A71E7A0))     // 0.00092279352247715
    {
        result = x;
    }
    else if( __builtin_spirv_OpenCL_fabs_f32(x) < as_float(0x3EACB527) )   // 0.33731958270072937
    {
        float sinhx, coshx;
        {
            float x2 = x * x;
            float x3 = x * x2;
            float x5 = x3 * x2;
            sinhx = (as_float(0x3C088889) * x5) + (as_float(0x3E2AAAAB) * x3) + x;
        }
        {
            float pexp = __intel_exp_for_tanh( x, -2.0f);
            float nexp = __intel_exp_for_tanh(-x, -2.0f);
            coshx = 2.0f * ( pexp + nexp );
        }
        result = sinhx / coshx;
    }
    else if (__builtin_spirv_OpenCL_fabs_f32(x) < as_float(0x41987E0C))    // 19.061546325683594
    {
        float exp2x = __intel_exp_for_hyper(2 * x, 0.0f);
        result = (exp2x - 1) / (exp2x + 1);
    }
    else
    {
        result = (x > 0) ? 1.0f : -1.0f;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanh, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_tanh_f16( half x )
{
    return __builtin_spirv_OpenCL_tanh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanh, half, half, f16 )

#endif // defined(cl_khr_fp16)

// tanpi

INLINE float __builtin_spirv_OpenCL_tanpi_f32( float x )
{
    bool useNative = __FastRelaxedMath && (!__APIRS);

    if(useNative)
    {
        return __builtin_spirv_OpenCL_native_tan_f32(x * M_PI_F);
    }
    else
    {
        return __ocl_svml_tanpif(x);
    }
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanpi, float, float, f32 )

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_tanpi_f16( half x )
{
    return __builtin_spirv_OpenCL_tanpi_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanpi, half, half, f16 )

#endif // defined(cl_khr_fp16)

// tgamma

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

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_tgamma_f16( half x )
{
    return __builtin_spirv_OpenCL_tgamma_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tgamma, half, half, f16 )

#endif // defined(cl_khr_fp16)

// trunc

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_trunc, float, float, f32 )

#if defined(cl_khr_fp16)

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_trunc, half, half, f16 )

#endif // defined(cl_khr_fp16)
