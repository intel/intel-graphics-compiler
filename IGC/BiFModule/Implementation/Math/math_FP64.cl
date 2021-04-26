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

#include "IMF/FP64/acos_d_la.cl"
#include "IMF/FP64/acosh_d_la.cl"
#include "IMF/FP64/acospi_d_la.cl"
#include "IMF/FP64/asin_d_la.cl"
#include "IMF/FP64/asinh_d_la.cl"
#include "IMF/FP64/asinpi_d_la.cl"
#include "IMF/FP64/atan_d_la.cl"
#include "IMF/FP64/atan2_d_la.cl"
#include "IMF/FP64/atanh_d_la.cl"
#include "IMF/FP64/atanpi_d_la.cl"
#include "IMF/FP64/cbrt_d_la.cl"
#include "IMF/FP64/cos_d_la.cl"
#include "IMF/FP64/cosh_d_la.cl"
#include "IMF/FP64/cospi_d_la.cl"
#include "IMF/FP64/erf_d_la.cl"
#include "IMF/FP64/erfc_d_la.cl"
#include "IMF/FP64/exp_d_la.cl"
#include "IMF/FP64/exp10_d_la.cl"
#include "IMF/FP64/exp2_d_la.cl"
#include "IMF/FP64/expm1_d_la.cl"
#include "IMF/FP64/ln_d_la.cl"
#include "IMF/FP64/log10_d_la.cl"
#include "IMF/FP64/log1p_d_la.cl"
#include "IMF/FP64/log2_d_la.cl"
#include "IMF/FP64/pow_d_la.cl"
#include "IMF/FP64/pown_d_la.cl"
#include "IMF/FP64/powr_d_la.cl"
#include "IMF/FP64/rootn_d_la.cl"
#include "IMF/FP64/sin_d_la.cl"
#include "IMF/FP64/sincos_d_la.cl"
#include "IMF/FP64/sinh_d_la.cl"
#include "IMF/FP64/sinpi_d_la.cl"
#include "IMF/FP64/tan_d_la.cl"
#include "IMF/FP64/tanh_d_la.cl"
#include "IMF/FP64/tanpi_d_la.cl"
// we still use libclc fp64 implementations of lgamma and tgamma
#include "ExternalLibraries/libclc/doubles.cl"

// acos
INLINE double __builtin_spirv_OpenCL_acos_f64( double x )
{
    return __ocl_svml_acos(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_acos, double, double, f64 )

// acosh
INLINE double __builtin_spirv_OpenCL_acosh_f64( double x )
{
    return __ocl_svml_acosh(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, double, double, f64 )

// acospi
INLINE double __builtin_spirv_OpenCL_acospi_f64( double x )
{
    return __ocl_svml_acospi(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_acospi, double, double, f64 )

// asin
INLINE double __builtin_spirv_OpenCL_asin_f64( double x )
{
    return __ocl_svml_asin(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asin, double, double, f64 )

// asinh
INLINE double __builtin_spirv_OpenCL_asinh_f64( double x )
{
    return __ocl_svml_asinh(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asinh, double, double, f64 )

// asinpi
INLINE double __builtin_spirv_OpenCL_asinpi_f64( double x )
{
    return __ocl_svml_asinpi(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asinpi, double, double, f64 )

// atan
INLINE double __builtin_spirv_OpenCL_atan_f64( double x )
{
    return __ocl_svml_atan(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atan, double, double, f64 )

// atan2
INLINE double __builtin_spirv_OpenCL_atan2_f64_f64( double y, double x )
{
    return __ocl_svml_atan2(y, x);
}
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_atan2, double, double, double, f64, f64 )

// atan2pi
INLINE double __builtin_spirv_OpenCL_atan2pi_f64_f64( double x, double y )
{
    return M_1_PI * __builtin_spirv_OpenCL_atan2_f64_f64(x, y);
}
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_atan2pi, double, double, f64 )

// atanh
INLINE double __builtin_spirv_OpenCL_atanh_f64( double x )
{
    return __ocl_svml_atanh(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, double, double, f64 )


// atanpi
INLINE double __builtin_spirv_OpenCL_atanpi_f64( double x )
{
    return __ocl_svml_atanpi(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atanpi, double, double, f64 )

// cbrt
INLINE double __builtin_spirv_OpenCL_cbrt_f64( double x )
{
    return __ocl_svml_cbrt(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_cbrt, double, double, f64 )

// ceil
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_ceil, double, double, f64 )

// copysign
INLINE double __builtin_spirv_OpenCL_copysign_f64_f64( double x, double y )
{
    return as_double( (long)((as_long(y) & DOUBLE_SIGN_MASK) + (as_long(x) & ~DOUBLE_SIGN_MASK)) );
}
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_copysign, double, double, f64 )

// cos
INLINE double __builtin_spirv_OpenCL_cos_f64( double x )
{
    return __ocl_svml_cos(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cos, double, double, f64 )

// cosh
INLINE double __builtin_spirv_OpenCL_cosh_f64( double x )
{
    return __ocl_svml_cosh(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cosh, double, double, f64 )

// cospi
INLINE double __builtin_spirv_OpenCL_cospi_f64( double x )
{
    return __ocl_svml_cospi(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cospi, double, double, f64 )

// divide
double __builtin_spirv_divide_cr_f64_f64( double a, double b )
{
    return FDIV_IEEE_DOUBLE(a, b);
}

// erf
INLINE double __builtin_spirv_OpenCL_erf_f64( double x )
{
    return __ocl_svml_erf(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erf, double, double, f64 )

// erfc
INLINE double __builtin_spirv_OpenCL_erfc_f64( double x )
{
    return __ocl_svml_erfc(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erfc, double, double, f64 )

// exp
INLINE double __builtin_spirv_OpenCL_exp_f64( double x )
{
    return __ocl_svml_exp(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_exp, double, double, f64 )

// exp10
INLINE double __builtin_spirv_OpenCL_exp10_f64( double x )
{
    return __ocl_svml_exp10(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, double, double, f64 )

// exp2
INLINE double __builtin_spirv_OpenCL_exp2_f64( double x )
{
    return __ocl_svml_exp2(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp2, double, double, f64 )

// expm1
INLINE double __builtin_spirv_OpenCL_expm1_f64( double x )
{
    return __ocl_svml_expm1(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_expm1, double, double, f64 )

// fabs
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_fabs, double, double, f64 )

// fdim
INLINE double __builtin_spirv_OpenCL_fdim_f64_f64( double x, double y )
{
    double r = x - y;
    double n = __builtin_spirv_OpenCL_nan_i64(0ul);
    int i = __intel_relaxed_isnan(x) | __intel_relaxed_isnan(y);
    r = x > y ? r : 0.0f;
    r = i ? n : r;
    return r;
}
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fdim, double, double, f64 )

// floor
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_floor, double, double, f64 )

// fma
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_fma, double, double, f64 )

// fmax
INLINE double __builtin_spirv_OpenCL_fmax_f64_f64( double x, double y )
{
    return __builtin_IB_dmax(x, y);
}
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmax, double, double, f64 )

// fmin
INLINE double __builtin_spirv_OpenCL_fmin_f64_f64( double x, double y )
{
    return __builtin_IB_dmin(x, y);
}
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmin, double, double, f64 )

// fmod

INLINE double __builtin_spirv_OpenCL_fast_fmod_f64_f64( double xx, double yy )
{
    return __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx, yy);
}

INLINE double2 __builtin_spirv_OpenCL_fast_fmod_v2f64_v2f64( double2 xx, double2 yy )
{
    double2 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s1, yy.s1);
    return temp;
}

INLINE double3 __builtin_spirv_OpenCL_fast_fmod_v3f64_v3f64( double3 xx, double3 yy )
{
    double3 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s2, yy.s2);
    return temp;
}

INLINE double4 __builtin_spirv_OpenCL_fast_fmod_v4f64_v4f64( double4 xx, double4 yy )
{
    double4 temp;
    temp.s0 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s0, yy.s0);
    temp.s1 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s1, yy.s1);
    temp.s2 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s2, yy.s2);
    temp.s3 = __builtin_spirv_OpenCL_fast_fmod_f64_f64(xx.s3, yy.s3);
    return temp;
}

double __builtin_spirv_OpenCL_fmod_f64_f64( double xx, double yy )
{
    double result;

    if( SPIRV_BUILTIN(IsNan, _f64, )(xx) |
        SPIRV_BUILTIN(IsNan, _f64, )(yy) |
        SPIRV_BUILTIN(IsInf, _f64, )(xx) |
        yy == 0.0)
    {
        result = __builtin_spirv_OpenCL_nan_i64((ulong)0);
    }
    else if( SPIRV_BUILTIN(IsInf, _f64, )(yy) |
             (xx == 0.0) )
    {
        result = xx;
    }
    else if( __builtin_spirv_OpenCL_fabs_f64(xx) == __builtin_spirv_OpenCL_fabs_f64(yy) )
    {
        result = __builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx);
    }
    else if (__builtin_spirv_OpenCL_fabs_f64(xx) < __builtin_spirv_OpenCL_fabs_f64(yy))
    {
        result = xx;
    }
    else
    {
        double x = __builtin_spirv_OpenCL_fabs_f64(xx);
        double y = __builtin_spirv_OpenCL_fabs_f64(yy);
        int ex = __builtin_spirv_OpenCL_ilogb_f64( x );
        int ey = __builtin_spirv_OpenCL_ilogb_f64( y );
        double xr = x;
        double yr = y;

        if(ex-ey >= 0)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f64_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32( x, -ex );

            for(int i = ex-ey; i > 0; i--)
            {
                double s = xr - yr;
                xr = ( xr >= yr ) ? s : xr;
                xr = xr + xr;
            }
            double s = xr - yr;
            xr = ( xr >= yr ) ? s : xr;

            xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ey);
        }

        double m = -xr;
        xr = ( xx < 0.0 ) ? m : xr;

        result = xr;
    }

    return result;
}

INLINE double __builtin_spirv_OpFMod_f64_f64( double x, double y )
{
    return __builtin_spirv_OpenCL_copysign_f64_f64(__builtin_spirv_OpenCL_fmod_f64_f64(x, y), y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_fmod, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFMod, double, double, double, f64, f64 )

// fract

double __builtin_spirv_OpenCL_fract_f64_p1f64( double x,
                                        __global double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_floor_f64( x ), __builtin_spirv_OpenCL_nan_i64( (ulong)0 ), (long)(SPIRV_BUILTIN(IsNan, _f64, )( x )) );
    double temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_fmin_f64_f64( temp, 0x1.fffffffffffffp-1), __builtin_spirv_OpenCL_copysign_f64_f64(0.0, x), (long)SPIRV_BUILTIN(IsInf, _f64, )(x));
    return __builtin_spirv_OpenCL_select_f64_f64_i64( temp, __builtin_spirv_OpenCL_nan_i64((ulong)0), (long)(SPIRV_BUILTIN(IsNan, _f64, )(x)) );
}

double2 __builtin_spirv_OpenCL_fract_v2f64_p1v2f64( double2 x,
                                             __global double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( __builtin_spirv_OpenCL_floor_v2f64( x ), __builtin_spirv_OpenCL_nan_v2i64( (ulong2)0 ), __convert_ulong2(SPIRV_BUILTIN(IsNan, _v2f64, )( x )) );
    double2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( (double2)__builtin_spirv_OpenCL_fmin_v2f64_v2f64( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__builtin_spirv_OpenCL_copysign_v2f64_v2f64((double2)0.0, x), __convert_ulong2(SPIRV_BUILTIN(IsInf, _v2f64, )(x)));
    return __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( temp, __builtin_spirv_OpenCL_nan_v2i64((ulong2)0), __convert_ulong2(SPIRV_BUILTIN(IsNan, _v2f64, )(x)) );
}

double3 __builtin_spirv_OpenCL_fract_v3f64_p1v3f64( double3 x,
                                             __global double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( __builtin_spirv_OpenCL_floor_v3f64( x ), __builtin_spirv_OpenCL_nan_v3i64( (ulong3)0 ), __convert_ulong3(SPIRV_BUILTIN(IsNan, _v3f64, )( x )) );
    double3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( (double3)__builtin_spirv_OpenCL_fmin_v3f64_v3f64( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__builtin_spirv_OpenCL_copysign_v3f64_v3f64((double3)0.0, x), __convert_ulong3(SPIRV_BUILTIN(IsInf, _v3f64, )(x)));
    return __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( temp, __builtin_spirv_OpenCL_nan_v3i64((ulong3)0), __convert_ulong3(SPIRV_BUILTIN(IsNan, _v3f64, )(x)) );
}

double4 __builtin_spirv_OpenCL_fract_v4f64_p1v4f64( double4 x,
                                             __global double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( __builtin_spirv_OpenCL_floor_v4f64( x ), __builtin_spirv_OpenCL_nan_v4i64( (ulong4)0 ), __convert_ulong4(SPIRV_BUILTIN(IsNan, _v4f64, )( x )) );
    double4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( (double4)__builtin_spirv_OpenCL_fmin_v4f64_v4f64( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__builtin_spirv_OpenCL_copysign_v4f64_v4f64((double4)0.0, x), __convert_ulong4(SPIRV_BUILTIN(IsInf, _v4f64, )(x)));
    return __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( temp, __builtin_spirv_OpenCL_nan_v4i64((ulong4)0), __convert_ulong4(SPIRV_BUILTIN(IsNan, _v4f64, )(x)) );
}

double8 __builtin_spirv_OpenCL_fract_v8f64_p1v8f64( double8 x,
                                             __global double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( __builtin_spirv_OpenCL_floor_v8f64( x ), __builtin_spirv_OpenCL_nan_v8i64( (ulong8)0 ), __convert_ulong8(SPIRV_BUILTIN(IsNan, _v8f64, )( x )) );
    double8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( (double8)__builtin_spirv_OpenCL_fmin_v8f64_v8f64( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__builtin_spirv_OpenCL_copysign_v8f64_v8f64((double8)0.0, x), __convert_ulong8(SPIRV_BUILTIN(IsInf, _v8f64, )(x)));
    return __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( temp, __builtin_spirv_OpenCL_nan_v8i64((ulong8)0), __convert_ulong8(SPIRV_BUILTIN(IsNan, _v8f64, )(x)) );
}

double16 __builtin_spirv_OpenCL_fract_v16f64_p1v16f64( double16 x,
                                                __global double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( __builtin_spirv_OpenCL_floor_v16f64( x ), __builtin_spirv_OpenCL_nan_v16i64( (ulong16)0 ), __convert_ulong16(SPIRV_BUILTIN(IsNan, _v16f64, )( x )) );
    double16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( (double16)__builtin_spirv_OpenCL_fmin_v16f64_v16f64( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__builtin_spirv_OpenCL_copysign_v16f64_v16f64((double16)0.0, x), __convert_ulong16(SPIRV_BUILTIN(IsInf, _v16f64, )(x)));
    return __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( temp, __builtin_spirv_OpenCL_nan_v16i64((ulong16)0), __convert_ulong16(SPIRV_BUILTIN(IsNan, _v16f64, )(x)) );
}

double __builtin_spirv_OpenCL_fract_f64_p0f64( double x,
                                        __private double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_floor_f64( x ), __builtin_spirv_OpenCL_nan_i64( (ulong)0 ), (ulong)(SPIRV_BUILTIN(IsNan, _f64, )( x )) );
    double temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f64_f64_i64( (double)__builtin_spirv_OpenCL_fmin_f64_f64( temp, (double)(0x1.fffffffffffffp-1)), (double)__builtin_spirv_OpenCL_copysign_f64_f64((double)0.0, x), (long)SPIRV_BUILTIN(IsInf, _f64, )(x));
    return __builtin_spirv_OpenCL_select_f64_f64_i64( temp, __builtin_spirv_OpenCL_nan_i64((ulong)0), (ulong)(SPIRV_BUILTIN(IsNan, _f64, )(x)) );
}

double2 __builtin_spirv_OpenCL_fract_v2f64_p0v2f64( double2 x,
                                             __private double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( __builtin_spirv_OpenCL_floor_v2f64( x ), __builtin_spirv_OpenCL_nan_v2i64( (ulong2)0 ), __convert_ulong2(SPIRV_BUILTIN(IsNan, _v2f64, )( x )) );
    double2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( (double2)__builtin_spirv_OpenCL_fmin_v2f64_v2f64( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__builtin_spirv_OpenCL_copysign_v2f64_v2f64((double2)0.0, x), __convert_ulong2(SPIRV_BUILTIN(IsInf, _v2f64, )(x)));
    return __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( temp, __builtin_spirv_OpenCL_nan_v2i64((ulong2)0), __convert_ulong2(SPIRV_BUILTIN(IsNan, _v2f64, )(x)) );
}

double3 __builtin_spirv_OpenCL_fract_v3f64_p0v3f64( double3 x,
                                             __private double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( __builtin_spirv_OpenCL_floor_v3f64( x ), __builtin_spirv_OpenCL_nan_v3i64( (ulong3)0 ), __convert_ulong3(SPIRV_BUILTIN(IsNan, _v3f64, )( x )) );
    double3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( (double3)__builtin_spirv_OpenCL_fmin_v3f64_v3f64( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__builtin_spirv_OpenCL_copysign_v3f64_v3f64((double3)0.0, x), __convert_ulong3(SPIRV_BUILTIN(IsInf, _v3f64, )(x)));
    return __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( temp, __builtin_spirv_OpenCL_nan_v3i64((ulong3)0), __convert_ulong3(SPIRV_BUILTIN(IsNan, _v3f64, )(x)) );
}

double4 __builtin_spirv_OpenCL_fract_v4f64_p0v4f64( double4 x,
                                             __private double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( __builtin_spirv_OpenCL_floor_v4f64( x ), __builtin_spirv_OpenCL_nan_v4i64( (ulong4)0 ), __convert_ulong4(SPIRV_BUILTIN(IsNan, _v4f64, )( x )) );
    double4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( (double4)__builtin_spirv_OpenCL_fmin_v4f64_v4f64( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__builtin_spirv_OpenCL_copysign_v4f64_v4f64((double4)0.0, x), __convert_ulong4(SPIRV_BUILTIN(IsInf, _v4f64, )(x)));
    return __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( temp, __builtin_spirv_OpenCL_nan_v4i64((ulong4)0), __convert_ulong4(SPIRV_BUILTIN(IsNan, _v4f64, )(x)) );
}

double8 __builtin_spirv_OpenCL_fract_v8f64_p0v8f64( double8 x,
                                             __private double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( __builtin_spirv_OpenCL_floor_v8f64( x ), __builtin_spirv_OpenCL_nan_v8i64( (ulong8)0 ), __convert_ulong8(SPIRV_BUILTIN(IsNan, _v8f64, )( x )) );
    double8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( (double8)__builtin_spirv_OpenCL_fmin_v8f64_v8f64( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__builtin_spirv_OpenCL_copysign_v8f64_v8f64((double8)0.0, x), __convert_ulong8(SPIRV_BUILTIN(IsInf, _v8f64, )(x)));
    return __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( temp, __builtin_spirv_OpenCL_nan_v8i64((ulong8)0), __convert_ulong8(SPIRV_BUILTIN(IsNan, _v8f64, )(x)) );
}

double16 __builtin_spirv_OpenCL_fract_v16f64_p0v16f64( double16 x,
                                                __private double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( __builtin_spirv_OpenCL_floor_v16f64( x ), __builtin_spirv_OpenCL_nan_v16i64( (ulong16)0 ), __convert_ulong16(SPIRV_BUILTIN(IsNan, _v16f64, )( x )) );
    double16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( (double16)__builtin_spirv_OpenCL_fmin_v16f64_v16f64( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__builtin_spirv_OpenCL_copysign_v16f64_v16f64((double16)0.0, x), __convert_ulong16(SPIRV_BUILTIN(IsInf, _v16f64, )(x)));
    return __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( temp, __builtin_spirv_OpenCL_nan_v16i64((ulong16)0), __convert_ulong16(SPIRV_BUILTIN(IsNan, _v16f64, )(x)) );
}

double __builtin_spirv_OpenCL_fract_f64_p3f64( double x,
                                        __local double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_floor_f64( x ), __builtin_spirv_OpenCL_nan_i64( (ulong)0 ), (long)(SPIRV_BUILTIN(IsNan, _f64, )( x )) );
    double temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f64_f64_i64( (double)__builtin_spirv_OpenCL_fmin_f64_f64( temp, (double)(0x1.fffffffffffffp-1)), (double)__builtin_spirv_OpenCL_copysign_f64_f64((double)0.0, x), (long)SPIRV_BUILTIN(IsInf, _f64, )(x));
    return __builtin_spirv_OpenCL_select_f64_f64_i64( temp, __builtin_spirv_OpenCL_nan_i64((ulong)0), (long)(SPIRV_BUILTIN(IsNan, _f64, )(x)) );
}

double2 __builtin_spirv_OpenCL_fract_v2f64_p3v2f64( double2 x,
                                             __local double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( __builtin_spirv_OpenCL_floor_v2f64( x ), __builtin_spirv_OpenCL_nan_v2i64( (ulong2)0 ), __convert_ulong2(SPIRV_BUILTIN(IsNan, _v2f64, )( x )) );
    double2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( (double2)__builtin_spirv_OpenCL_fmin_v2f64_v2f64( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__builtin_spirv_OpenCL_copysign_v2f64_v2f64((double2)0.0, x), __convert_ulong2(SPIRV_BUILTIN(IsInf, _v2f64, )(x)));
    return __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( temp, __builtin_spirv_OpenCL_nan_v2i64((ulong2)0), __convert_ulong2(SPIRV_BUILTIN(IsNan, _v2f64, )(x)) );
}

double3 __builtin_spirv_OpenCL_fract_v3f64_p3v3f64( double3 x,
                                             __local double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( __builtin_spirv_OpenCL_floor_v3f64( x ), __builtin_spirv_OpenCL_nan_v3i64( (ulong3)0 ), __convert_ulong3(SPIRV_BUILTIN(IsNan, _v3f64, )( x )) );
    double3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( (double3)__builtin_spirv_OpenCL_fmin_v3f64_v3f64( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__builtin_spirv_OpenCL_copysign_v3f64_v3f64((double3)0.0, x), __convert_ulong3(SPIRV_BUILTIN(IsInf, _v3f64, )(x)));
    return __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( temp, __builtin_spirv_OpenCL_nan_v3i64((ulong3)0), __convert_ulong3(SPIRV_BUILTIN(IsNan, _v3f64, )(x)) );
}

double4 __builtin_spirv_OpenCL_fract_v4f64_p3v4f64( double4 x,
                                             __local double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( __builtin_spirv_OpenCL_floor_v4f64( x ), __builtin_spirv_OpenCL_nan_v4i64( (ulong4)0 ), __convert_ulong4(SPIRV_BUILTIN(IsNan, _v4f64, )( x )) );
    double4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( (double4)__builtin_spirv_OpenCL_fmin_v4f64_v4f64( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__builtin_spirv_OpenCL_copysign_v4f64_v4f64((double4)0.0, x), __convert_ulong4(SPIRV_BUILTIN(IsInf, _v4f64, )(x)));
    return __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( temp, __builtin_spirv_OpenCL_nan_v4i64((ulong4)0), __convert_ulong4(SPIRV_BUILTIN(IsNan, _v4f64, )(x)) );
}

double8 __builtin_spirv_OpenCL_fract_v8f64_p3v8f64( double8 x,
                                             __local double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( __builtin_spirv_OpenCL_floor_v8f64( x ), __builtin_spirv_OpenCL_nan_v8i64( (ulong8)0 ), __convert_ulong8(SPIRV_BUILTIN(IsNan, _v8f64, )( x )) );
    double8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( (double8)__builtin_spirv_OpenCL_fmin_v8f64_v8f64( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__builtin_spirv_OpenCL_copysign_v8f64_v8f64((double8)0.0, x), __convert_ulong8(SPIRV_BUILTIN(IsInf, _v8f64, )(x)));
    return __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( temp, __builtin_spirv_OpenCL_nan_v8i64((ulong8)0), __convert_ulong8(SPIRV_BUILTIN(IsNan, _v8f64, )(x)) );
}

double16 __builtin_spirv_OpenCL_fract_v16f64_p3v16f64( double16 x,
                                                __local double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( __builtin_spirv_OpenCL_floor_v16f64( x ), __builtin_spirv_OpenCL_nan_v16i64( (ulong16)0 ), __convert_ulong16(SPIRV_BUILTIN(IsNan, _v16f64, )( x )) );
    double16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( (double16)__builtin_spirv_OpenCL_fmin_v16f64_v16f64( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__builtin_spirv_OpenCL_copysign_v16f64_v16f64((double16)0.0, x), __convert_ulong16(SPIRV_BUILTIN(IsInf, _v16f64, )(x)));
    return __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( temp, __builtin_spirv_OpenCL_nan_v16i64((ulong16)0), __convert_ulong16(SPIRV_BUILTIN(IsNan, _v16f64, )(x)) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __builtin_spirv_OpenCL_fract_f64_p4f64( double x,
                                        __generic double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_f64_f64_i64( __builtin_spirv_OpenCL_floor_f64( x ), __builtin_spirv_OpenCL_nan_i64( (ulong)0 ), (long)(SPIRV_BUILTIN(IsNan, _f64, )( x )) );
    double temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_f64_f64_i64( (double)__builtin_spirv_OpenCL_fmin_f64_f64( temp, (double)(0x1.fffffffffffffp-1)), (double)__builtin_spirv_OpenCL_copysign_f64_f64((double)0.0, x), (long)SPIRV_BUILTIN(IsInf, _f64, )(x));
    return __builtin_spirv_OpenCL_select_f64_f64_i64( temp, __builtin_spirv_OpenCL_nan_i64((ulong)0), (long)(SPIRV_BUILTIN(IsNan, _f64, )(x)) );
}

double2 __builtin_spirv_OpenCL_fract_v2f64_p4v2f64( double2 x,
                                             __generic double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( __builtin_spirv_OpenCL_floor_v2f64( x ), __builtin_spirv_OpenCL_nan_v2i64( (ulong2)0 ), __convert_ulong2(SPIRV_BUILTIN(IsNan, _v2f64, )( x )) );
    double2 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( (double2)__builtin_spirv_OpenCL_fmin_v2f64_v2f64( temp, (double2)(0x1.fffffffffffffp-1)), (double2)__builtin_spirv_OpenCL_copysign_v2f64_v2f64((double2)0.0, x), __convert_ulong2(SPIRV_BUILTIN(IsInf, _v2f64, )(x)));
    return __builtin_spirv_OpenCL_select_v2f64_v2f64_v2i64( temp, __builtin_spirv_OpenCL_nan_v2i64((ulong2)0), __convert_ulong2(SPIRV_BUILTIN(IsNan, _v2f64, )(x)) );
}

double3 __builtin_spirv_OpenCL_fract_v3f64_p4v3f64( double3 x,
                                             __generic double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( __builtin_spirv_OpenCL_floor_v3f64( x ), __builtin_spirv_OpenCL_nan_v3i64( (ulong3)0 ), __convert_ulong3(SPIRV_BUILTIN(IsNan, _v3f64, )( x )) );
    double3 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( (double3)__builtin_spirv_OpenCL_fmin_v3f64_v3f64( temp, (double3)(0x1.fffffffffffffp-1)), (double3)__builtin_spirv_OpenCL_copysign_v3f64_v3f64((double3)0.0, x), __convert_ulong3(SPIRV_BUILTIN(IsInf, _v3f64, )(x)));
    return __builtin_spirv_OpenCL_select_v3f64_v3f64_v3i64( temp, __builtin_spirv_OpenCL_nan_v3i64((ulong3)0), __convert_ulong3(SPIRV_BUILTIN(IsNan, _v3f64, )(x)) );
}

double4 __builtin_spirv_OpenCL_fract_v4f64_p4v4f64( double4 x,
                                             __generic double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( __builtin_spirv_OpenCL_floor_v4f64( x ), __builtin_spirv_OpenCL_nan_v4i64( (ulong4)0 ), __convert_ulong4(SPIRV_BUILTIN(IsNan, _v4f64, )( x )) );
    double4 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( (double4)__builtin_spirv_OpenCL_fmin_v4f64_v4f64( temp, (double4)(0x1.fffffffffffffp-1)), (double4)__builtin_spirv_OpenCL_copysign_v4f64_v4f64((double4)0.0, x), __convert_ulong4(SPIRV_BUILTIN(IsInf, _v4f64, )(x)));
    return __builtin_spirv_OpenCL_select_v4f64_v4f64_v4i64( temp, __builtin_spirv_OpenCL_nan_v4i64((ulong4)0), __convert_ulong4(SPIRV_BUILTIN(IsNan, _v4f64, )(x)) );
}

double8 __builtin_spirv_OpenCL_fract_v8f64_p4v8f64( double8 x,
                                             __generic double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( __builtin_spirv_OpenCL_floor_v8f64( x ), __builtin_spirv_OpenCL_nan_v8i64( (ulong8)0 ), __convert_ulong8(SPIRV_BUILTIN(IsNan, _v8f64, )( x )) );
    double8 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( (double8)__builtin_spirv_OpenCL_fmin_v8f64_v8f64( temp, (double8)(0x1.fffffffffffffp-1)), (double8)__builtin_spirv_OpenCL_copysign_v8f64_v8f64((double8)0.0, x), __convert_ulong8(SPIRV_BUILTIN(IsInf, _v8f64, )(x)));
    return __builtin_spirv_OpenCL_select_v8f64_v8f64_v8i64( temp, __builtin_spirv_OpenCL_nan_v8i64((ulong8)0), __convert_ulong8(SPIRV_BUILTIN(IsNan, _v8f64, )(x)) );
}

double16 __builtin_spirv_OpenCL_fract_v16f64_p4v16f64( double16 x,
                                                __generic double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( __builtin_spirv_OpenCL_floor_v16f64( x ), __builtin_spirv_OpenCL_nan_v16i64( (ulong16)0 ), __convert_ulong16(SPIRV_BUILTIN(IsNan, _v16f64, )( x )) );
    double16 temp = x - *iptr;
    temp = __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( (double16)__builtin_spirv_OpenCL_fmin_v16f64_v16f64( temp, (double16)(0x1.fffffffffffffp-1)), (double16)__builtin_spirv_OpenCL_copysign_v16f64_v16f64((double16)0.0, x), __convert_ulong16(SPIRV_BUILTIN(IsInf, _v16f64, )(x)));
    return __builtin_spirv_OpenCL_select_v16f64_v16f64_v16i64( temp, __builtin_spirv_OpenCL_nan_v16i64((ulong16)0), __convert_ulong16(SPIRV_BUILTIN(IsNan, _v16f64, )(x)) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// frexp

double __builtin_spirv_OpenCL_frexp_f64_p1i32( double        x,
                                        __global int* exp )
{
    double temp;
    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f64_f64_i64( temp, 0.5, (long)(temp == 1.0) );
        temp = __builtin_spirv_OpenCL_copysign_f64_f64( temp, x );

        *exp = ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - ( DOUBLE_BIAS - (long)(1) );
    }
    else if( (x == 0.0) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = (x == 0.0) ? 0.0 : x;
        *exp = 0;
    }
    else
    {
        long m = as_long( x ) & DOUBLE_MANTISSA_MASK;
        long lz = __builtin_spirv_OpenCL_clz_i64( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f64_f64_i64( temp,
                       0.5, (long)(temp == 1.0) );
        temp = __builtin_spirv_OpenCL_copysign_f64_f64( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 __builtin_spirv_OpenCL_frexp_v2f64_p1v2i32( double2        x,
                                             __global int2* exp )
{
    double2 temp;
    int2 exp_temp;
    int temp_ptr;
    double in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

double3 __builtin_spirv_OpenCL_frexp_v3f64_p1v3i32( double3        x,
                                             __global int3* exp )
{
    double3 temp;
    int3 exp_temp;
    int temp_ptr;
    double in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double4 __builtin_spirv_OpenCL_frexp_v4f64_p1v4i32( double4        x,
                                             __global int4* exp )
{
    double4 temp;
    int4 exp_temp;
    int temp_ptr;
    double in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double8 __builtin_spirv_OpenCL_frexp_v8f64_p1v8i32( double8        x,
                                             __global int8* exp )
{
    double8 temp;
    int8 exp_temp;
    int temp_ptr;
    double in[8], out1[8];
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
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double16 __builtin_spirv_OpenCL_frexp_v16f64_p1v16i32( double16        x,
                                                __global int16* exp )
{
    double16 temp;
    int16 exp_temp;
    int temp_ptr;
    double in[16], out1[16];
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
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double __builtin_spirv_OpenCL_frexp_f64_p0i32( double         x,
                                        __private int* exp )
{
    double temp;
    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f64_f64_i64( temp, 0.5, (long)(temp == 1.0) );
        temp = __builtin_spirv_OpenCL_copysign_f64_f64( temp, x );

        *exp = ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - ( DOUBLE_BIAS - (long)(1) );
    }
    else if( (x == 0.0) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = ( x == 0.0 ) ? 0.0 : x;
        *exp = 0;
    }
    else
    {
        long m = as_long( x ) & DOUBLE_MANTISSA_MASK;
        long lz = __builtin_spirv_OpenCL_clz_i64( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f64_f64_i64( temp,
                       0.5, (long)(temp == 1.0) );
        temp = __builtin_spirv_OpenCL_copysign_f64_f64( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 __builtin_spirv_OpenCL_frexp_v2f64_p0v2i32( double2         x,
                                             __private int2* exp )
{
    double2 temp;
    int2 exp_temp;
    int temp_ptr;
    double in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

double3 __builtin_spirv_OpenCL_frexp_v3f64_p0v3i32( double3         x,
                                             __private int3* exp )
{
    double3 temp;
    int3 exp_temp;
    int temp_ptr;
    double in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double4 __builtin_spirv_OpenCL_frexp_v4f64_p0v4i32( double4         x,
                                             __private int4* exp )
{
    double4 temp;
    int4 exp_temp;
    int temp_ptr;
    double in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double8 __builtin_spirv_OpenCL_frexp_v8f64_p0v8i32( double8         x,
                                             __private int8* exp )
{
    double8 temp;
    int8 exp_temp;
    int temp_ptr;
    double in[8], out1[8];
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
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double16 __builtin_spirv_OpenCL_frexp_v16f64_p0v16i32( double16         x,
                                                __private int16* exp )
{
    double16 temp;
    int16 exp_temp;
    int temp_ptr;
    double in[16], out1[16];
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
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double __builtin_spirv_OpenCL_frexp_f64_p3i32( double       x,
                                        __local int* exp )
{
    double temp;
    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f64_f64_i64( temp, 0.5, (long)(temp == 1.0) );
        temp = __builtin_spirv_OpenCL_copysign_f64_f64( temp, x );

        *exp = ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - ( DOUBLE_BIAS - (long)(1) );
    }
    else if( (x == 0.0) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = (x == 0.0) ? 0.0 : x;
        *exp = 0;
    }
    else
    {
        long m = as_long( x ) & DOUBLE_MANTISSA_MASK;
        long lz = __builtin_spirv_OpenCL_clz_i64( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f64_f64_i64( temp,
                       0.5, (long)(temp == 1.0) );
        temp = __builtin_spirv_OpenCL_copysign_f64_f64( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 __builtin_spirv_OpenCL_frexp_v2f64_p3v2i32( double2       x,
                                             __local int2* exp )
{
    double2 temp;
    int2 exp_temp;
    int temp_ptr;
    double in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

double3 __builtin_spirv_OpenCL_frexp_v3f64_p3v3i32( double3       x,
                                             __local int3* exp )
{
    double3 temp;
    int3 exp_temp;
    int temp_ptr;
    double in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double4 __builtin_spirv_OpenCL_frexp_v4f64_p3v4i32( double4       x,
                                             __local int4* exp )
{
    double4 temp;
    int4 exp_temp;
    int temp_ptr;
    double in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double8 __builtin_spirv_OpenCL_frexp_v8f64_p3v8i32( double8       x,
                                             __local int8* exp )
{
    double8 temp;
    int8 exp_temp;
    int temp_ptr;
    double in[8], out1[8];
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
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double16 __builtin_spirv_OpenCL_frexp_v16f64_p3v16i32( double16       x,
                                                __local int16* exp )
{
    double16 temp;
    int16 exp_temp;
    int temp_ptr;
    double in[16], out1[16];
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
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p0i32( in[i], (__private int*)&temp_ptr );
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

double __builtin_spirv_OpenCL_frexp_f64_p4i32( double       x,
                                        __generic int* exp )
{
    double temp;
    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = __builtin_spirv_OpenCL_select_f64_f64_i64( temp, 0.5, (long)(temp == 1.0) );
        temp = __builtin_spirv_OpenCL_copysign_f64_f64( temp, x );

        *exp = ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - ( DOUBLE_BIAS - (long)(1) );
    }
    else if( (x == 0.0) | __intel_relaxed_isinf( x ) | __intel_relaxed_isnan( x ) )
    {
        temp = (x == 0.0) ? 0.0 : x;
        *exp = 0;
    }
    else
    {
        long m = as_long( x ) & DOUBLE_MANTISSA_MASK;
        long lz = __builtin_spirv_OpenCL_clz_i64( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = __builtin_spirv_OpenCL_select_f64_f64_i64( temp,
                       0.5, (long)(temp == 1.0) );
        temp = __builtin_spirv_OpenCL_copysign_f64_f64( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 __builtin_spirv_OpenCL_frexp_v2f64_p4v2i32( double2       x,
                                             __generic int2* exp )
{
    double2 temp;
    int2 exp_temp;
    int temp_ptr;
    double in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p4i32( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

double3 __builtin_spirv_OpenCL_frexp_v3f64_p4v3i32( double3       x,
                                             __generic int3* exp )
{
    double3 temp;
    int3 exp_temp;
    int temp_ptr;
    double in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p4i32( in[i], (__generic int*)&temp_ptr );
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

double4 __builtin_spirv_OpenCL_frexp_v4f64_p4v4i32( double4       x,
                                             __generic int4* exp )
{
    double4 temp;
    int4 exp_temp;
    int temp_ptr;
    double in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p4i32( in[i], (__generic int*)&temp_ptr );
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

double8 __builtin_spirv_OpenCL_frexp_v8f64_p4v8i32( double8       x,
                                             __generic int8* exp )
{
    double8 temp;
    int8 exp_temp;
    int temp_ptr;
    double in[8], out1[8];
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
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p4i32( in[i], (__generic int*)&temp_ptr );
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

double16 __builtin_spirv_OpenCL_frexp_v16f64_p4v16i32( double16       x,
                                                __generic int16* exp )
{
    double16 temp;
    int16 exp_temp;
    int temp_ptr;
    double in[16], out1[16];
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
        out1[i] = __builtin_spirv_OpenCL_frexp_f64_p4i32( in[i], (__generic int*)&temp_ptr );
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

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

// hypot

double __builtin_spirv_OpenCL_hypot_f64_f64( double x, double y )
{
    // Note: This isn't well specified, but apparently conformance
    // tests expect the following behavior:
    // 1) If either input is infinity, return infinity.
    // 2) Else, if either input is NaN, return NaN.
    // 3) Else, if both inputs are zero, return zero.
    // 4) Else, return sqrt( x * x, y * y )

    // Find the biggest absolute component:
    double2 p = (double2)( x, y );
    double2 a = __builtin_spirv_OpenCL_fabs_v2f64( p );
    double maxc = a.x > a.y ? a.x : a.y;
    double minc = a.x > a.y ? a.y : a.x;

    double result;
    if( SPIRV_BUILTIN(IsInf, _f64, )(p.x) |
        SPIRV_BUILTIN(IsInf, _f64, )(p.y) )
    {
        result = INFINITY;
    }
    else if( SPIRV_BUILTIN(IsNan, _f64, )( minc ) )
    {
        result = __builtin_spirv_OpenCL_nan_i32(0u);
    }
    else
    {
        // Scale by the biggest component.
        // Compute the length of this scaled vector, then scale
        // back up to compute the actual length.
        double s = minc / maxc;
        double t = __builtin_spirv_OpenCL_sqrt_f64( __builtin_spirv_OpenCL_mad_f64_f64_f64( s, s, 1.0 ) );
        result = t * maxc;

        result = ( maxc == 0.0 ) ? 0.0 : result;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_hypot, double, double, double, f64, f64 )

// ilogb

int __builtin_spirv_OpenCL_ilogb_f64( double x )
{
    int result = 0;

    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS;
    }
    else if( SPIRV_BUILTIN(IsNan, _f64, )( x ) | SPIRV_BUILTIN(IsInf, _f64, )( x ) )
    {
        result = FP_ILOGBNAN;
    }
    else if( x == 0.0 )
    {
        result = FP_ILOGB0;
    }
    else
    {
        x = x * ( 1UL << DOUBLE_MANTISSA_BITS );
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_ilogb, int, double, f64 )

// ldexp

double __builtin_spirv_OpenCL_ldexp_f64_i32( double x, int n )
{
    int delta = 0;
    double m0 = 1.0;
    m0 = ( n < (DBL_MIN_EXP+1) ) ? as_double( (long)( (DBL_MIN_EXP+1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : m0;
    m0 = ( n > (DBL_MAX_EXP-1) ) ? as_double( (long)( (DBL_MAX_EXP-1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : m0;
    delta = ( n < (DBL_MIN_EXP+1) ) ? (DBL_MIN_EXP+1) : 0;
    delta = ( n > (DBL_MAX_EXP-1) ) ? (DBL_MAX_EXP-1) : delta;
    n -= delta;

    double m1 = 1.0;
    m1 = ( n < (DBL_MIN_EXP+1) ) ? as_double( (long)( (DBL_MIN_EXP+1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : m1;
    m1 = ( n > (DBL_MAX_EXP-1) ) ? as_double( (long)( (DBL_MAX_EXP-1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : m1;
    delta = ( n < (DBL_MIN_EXP+1) ) ? (DBL_MIN_EXP+1) : 0;
    delta = ( n > (DBL_MAX_EXP-1) ) ? (DBL_MAX_EXP-1) : delta;
    n -= delta;

    double mn = as_double( (long)( n + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS );
    mn = ( n == 0 ) ? 1.0 : mn;
    mn = ( n < (DBL_MIN_EXP+1) ) ? as_double( (long)( (DBL_MIN_EXP+1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : mn;
    mn = ( n > (DBL_MAX_EXP-1) ) ? as_double( (long)( (DBL_MAX_EXP-1) + DOUBLE_BIAS ) << DOUBLE_MANTISSA_BITS ) : mn;

    double res = x * mn * m0 * m1;

    return res;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV( __builtin_spirv_OpenCL_ldexp, double, double, int, f64, i32 )

// lgamma
INLINE double __builtin_spirv_OpenCL_lgamma_f64( double x )
{
    return libclc_lgamma_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_lgamma, double, double, f64 )

// lgamma_r

double __builtin_spirv_OpenCL_lgamma_r_f64_p1i32( double        x,
                                           __global int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)( __builtin_spirv_OpenCL_lgamma_r_f32_p1i32( SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(x), signp ) );
}

double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p1v2i32( double2        x,
                                                __global int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)( __builtin_spirv_OpenCL_lgamma_r_v2f32_p1v2i32( SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(x), signp ) );
}

double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p1v3i32( double3        x,
                                                __global int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)( __builtin_spirv_OpenCL_lgamma_r_v3f32_p1v3i32( SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(x), signp ) );
}

double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p1v4i32( double4        x,
                                                __global int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)( __builtin_spirv_OpenCL_lgamma_r_v4f32_p1v4i32( SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(x), signp ) );
}

double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p1v8i32( double8        x,
                                                __global int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)( __builtin_spirv_OpenCL_lgamma_r_v8f32_p1v8i32( SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(x), signp ) );
}

double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p1v16i32( double16        x,
                                                   __global int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)( __builtin_spirv_OpenCL_lgamma_r_v16f32_p1v16i32( SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(x), signp ) );
}

double __builtin_spirv_OpenCL_lgamma_r_f64_p0i32( double         x,
                                           __private int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)( __builtin_spirv_OpenCL_lgamma_r_f32_p0i32( SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(x), signp ) );
}

double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p0v2i32( double2         x,
                                                __private int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)( __builtin_spirv_OpenCL_lgamma_r_v2f32_p0v2i32( SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(x), signp ) );
}

double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p0v3i32( double3         x,
                                                __private int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)( __builtin_spirv_OpenCL_lgamma_r_v3f32_p0v3i32( SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(x), signp ) );
}

double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p0v4i32( double4         x,
                                                __private int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)( __builtin_spirv_OpenCL_lgamma_r_v4f32_p0v4i32( SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(x), signp ) );
}

double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p0v8i32( double8         x,
                                                __private int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)( __builtin_spirv_OpenCL_lgamma_r_v8f32_p0v8i32( SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(x), signp ) );
}

double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p0v16i32( double16         x,
                                                   __private int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)( __builtin_spirv_OpenCL_lgamma_r_v16f32_p0v16i32( SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(x), signp ) );
}

double __builtin_spirv_OpenCL_lgamma_r_f64_p3i32( double       x,
                                           __local int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)( __builtin_spirv_OpenCL_lgamma_r_f32_p3i32( SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(x), signp ) );
}

double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p3v2i32( double2       x,
                                                __local int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)( __builtin_spirv_OpenCL_lgamma_r_v2f32_p3v2i32( SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(x), signp ) );
}

double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p3v3i32( double3       x,
                                                __local int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)( __builtin_spirv_OpenCL_lgamma_r_v3f32_p3v3i32( SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(x), signp ) );
}

double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p3v4i32( double4       x,
                                                __local int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)( __builtin_spirv_OpenCL_lgamma_r_v4f32_p3v4i32( SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(x), signp ) );
}

double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p3v8i32( double8       x,
                                                __local int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)( __builtin_spirv_OpenCL_lgamma_r_v8f32_p3v8i32( SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(x), signp ) );
}

double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p3v16i32( double16       x,
                                                   __local int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)( __builtin_spirv_OpenCL_lgamma_r_v16f32_p3v16i32( SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(x), signp ) );
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __builtin_spirv_OpenCL_lgamma_r_f64_p4i32( double         x,
                                           __generic int* signp )
{
    return SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)( __builtin_spirv_OpenCL_lgamma_r_f32_p4i32( SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(x), signp ) );
}

double2 __builtin_spirv_OpenCL_lgamma_r_v2f64_p4v2i32( double2         x,
                                                __generic int2* signp )
{
    return SPIRV_BUILTIN(FConvert, _v2f64_v2f32, _Rdouble2)( __builtin_spirv_OpenCL_lgamma_r_v2f32_p4v2i32( SPIRV_BUILTIN(FConvert, _v2f32_v2f64, _Rfloat2)(x), signp ) );
}

double3 __builtin_spirv_OpenCL_lgamma_r_v3f64_p4v3i32( double3         x,
                                                __generic int3* signp )
{
    return SPIRV_BUILTIN(FConvert, _v3f64_v3f32, _Rdouble3)( __builtin_spirv_OpenCL_lgamma_r_v3f32_p4v3i32( SPIRV_BUILTIN(FConvert, _v3f32_v3f64, _Rfloat3)(x), signp ) );
}

double4 __builtin_spirv_OpenCL_lgamma_r_v4f64_p4v4i32( double4         x,
                                                __generic int4* signp )
{
    return SPIRV_BUILTIN(FConvert, _v4f64_v4f32, _Rdouble4)( __builtin_spirv_OpenCL_lgamma_r_v4f32_p4v4i32( SPIRV_BUILTIN(FConvert, _v4f32_v4f64, _Rfloat4)(x), signp ) );
}

double8 __builtin_spirv_OpenCL_lgamma_r_v8f64_p4v8i32( double8         x,
                                                __generic int8* signp )
{
    return SPIRV_BUILTIN(FConvert, _v8f64_v8f32, _Rdouble8)( __builtin_spirv_OpenCL_lgamma_r_v8f32_p4v8i32( SPIRV_BUILTIN(FConvert, _v8f32_v8f64, _Rfloat8)(x), signp ) );
}

double16 __builtin_spirv_OpenCL_lgamma_r_v16f64_p4v16i32( double16         x,
                                                   __generic int16* signp )
{
    return SPIRV_BUILTIN(FConvert, _v16f64_v16f32, _Rdouble16)( __builtin_spirv_OpenCL_lgamma_r_v16f32_p4v16i32( SPIRV_BUILTIN(FConvert, _v16f32_v16f64, _Rfloat16)(x), signp ) );
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// log
INLINE double __builtin_spirv_OpenCL_log_f64( double x )
{
    return __ocl_svml_log(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log, double, double, f64 )

// log10
INLINE double __builtin_spirv_OpenCL_log10_f64( double x )
{
    return __ocl_svml_log10_v2(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log10, double, double, f64 )

// log1p
INLINE double __builtin_spirv_OpenCL_log1p_f64( double x )
{
    return __ocl_svml_log1p(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log1p, double, double, f64 )

// log2
INLINE double __builtin_spirv_OpenCL_log2_f64( double x )
{
    return __ocl_svml_log2_v2(x);
}
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log2, double, double, f64 )

// logb

double __builtin_spirv_OpenCL_logb_f64( double x )
{
    double result = 0.0;

    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS;
    }
    else if( SPIRV_BUILTIN(IsNan, _f64, )( x ) )
    {
        result = __builtin_spirv_OpenCL_nan_i64(0UL);
    }
    else if( SPIRV_BUILTIN(IsInf, _f64, )( x ) )
    {
        result = INFINITY;
    }
    else if( x == 0.0 )
    {
        result = -INFINITY;
    }
    else
    {
        x = x * ( 1UL << DOUBLE_MANTISSA_BITS );
        result = ( (as_long(x) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS) - DOUBLE_BIAS - DOUBLE_MANTISSA_BITS;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_logb, double, double, f64 )

// mad

INLINE double __builtin_spirv_OpenCL_mad_f64_f64_f64( double a, double b, double c )
{
    return __builtin_spirv_OpenCL_fma_f64_f64_f64(a,b,c);
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mad, double, double, f64 )

// maxmag

INLINE double __builtin_spirv_OpenCL_maxmag_f64_f64( double x, double y )
{
    double fx = __builtin_spirv_OpenCL_fabs_f64(x);
    double fy = __builtin_spirv_OpenCL_fabs_f64(y);
    double m = __builtin_spirv_OpenCL_fmax_f64_f64(x, y);
    return fx > fy ? x
        : fx < fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_maxmag, double, double, f64 )

// minmag

INLINE double __builtin_spirv_OpenCL_minmag_f64_f64( double x, double y )
{
    double fx = __builtin_spirv_OpenCL_fabs_f64(x);
    double fy = __builtin_spirv_OpenCL_fabs_f64(y);
    double m = __builtin_spirv_OpenCL_fmin_f64_f64(x, y);
    return fx < fy ? x
        : fx > fy ? y
        : m;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_minmag, double, double, f64 )

// modf

double __builtin_spirv_OpenCL_modf_f64_p1f64( double           x,
                                       __global double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f64(x);
    return __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x) ? 0.0 : x - *iptr), x);
}

double2 __builtin_spirv_OpenCL_modf_v2f64_p1v2f64( double2           x,
                                            __global double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f64(x);
    double2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    return temp;
}

double3 __builtin_spirv_OpenCL_modf_v3f64_p1v3f64( double3           x,
                                            __global double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f64(x);
    double3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    return temp;
}

double4 __builtin_spirv_OpenCL_modf_v4f64_p1v4f64( double4           x,
                                            __global double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f64(x);
    double4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    return temp;
}

double8 __builtin_spirv_OpenCL_modf_v8f64_p1v8f64( double8           x,
                                            __global double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f64(x);
    double8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    return temp;
}

double16 __builtin_spirv_OpenCL_modf_v16f64_p1v16f64( double16           x,
                                               __global double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f64(x);
    double16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s8) ? 0.0 : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s9) ? 0.0 : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sa) ? 0.0 : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sb) ? 0.0 : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sc) ? 0.0 : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sd) ? 0.0 : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.se) ? 0.0 : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sf) ? 0.0 : temp.sf), x.sf);
    return temp;
}

double __builtin_spirv_OpenCL_modf_f64_p0f64( double            x,
                                       __private double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f64(x);
    return __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x) ? 0.0 : x - *iptr), x);
}

double2 __builtin_spirv_OpenCL_modf_v2f64_p0v2f64( double2            x,
                                            __private double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f64(x);
    double2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    return temp;
}

double3 __builtin_spirv_OpenCL_modf_v3f64_p0v3f64( double3            x,
                                            __private double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f64(x);
    double3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    return temp;
}

double4 __builtin_spirv_OpenCL_modf_v4f64_p0v4f64( double4            x,
                                            __private double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f64(x);
    double4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    return temp;
}

double8 __builtin_spirv_OpenCL_modf_v8f64_p0v8f64( double8            x,
                                            __private double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f64(x);
    double8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    return temp;
}

double16 __builtin_spirv_OpenCL_modf_v16f64_p0v16f64( double16            x,
                                               __private double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f64(x);
    double16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s8) ? 0.0 : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s9) ? 0.0 : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sa) ? 0.0 : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sb) ? 0.0 : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sc) ? 0.0 : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sd) ? 0.0 : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.se) ? 0.0 : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sf) ? 0.0 : temp.sf), x.sf);
    return temp;
}

double __builtin_spirv_OpenCL_modf_f64_p3f64( double          x,
                                       __local double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f64(x);
    return __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x) ? 0.0 : x - *iptr), x);
}

double2 __builtin_spirv_OpenCL_modf_v2f64_p3v2f64( double2        x,
                                          __local double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f64(x);
    double2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    return temp;
}

double3 __builtin_spirv_OpenCL_modf_v3f64_p3v3f64( double3          x,
                                            __local double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f64(x);
    double3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    return temp;
}

double4 __builtin_spirv_OpenCL_modf_v4f64_p3v4f64( double4          x,
                                            __local double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f64(x);
    double4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    return temp;
}

double8 __builtin_spirv_OpenCL_modf_v8f64_p3v8f64( double8          x,
                                            __local double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f64(x);
    double8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    return temp;
}

double16 __builtin_spirv_OpenCL_modf_v16f64_p3v16f64( double16          x,
                                               __local double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f64(x);
    double16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s8) ? 0.0 : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s9) ? 0.0 : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sa) ? 0.0 : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sb) ? 0.0 : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sc) ? 0.0 : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sd) ? 0.0 : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.se) ? 0.0 : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sf) ? 0.0 : temp.sf), x.sf);
    return temp;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __builtin_spirv_OpenCL_modf_f64_p4f64( double            x,
                                       __generic double* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_f64(x);
    return __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x) ? 0.0 : x - *iptr), x);
}

double2 __builtin_spirv_OpenCL_modf_v2f64_p4v2f64( double2            x,
                                            __generic double2* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v2f64(x);
    double2 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    return temp;
}

double3 __builtin_spirv_OpenCL_modf_v3f64_p4v3f64( double3            x,
                                            __generic double3* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v3f64(x);
    double3 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    return temp;
}

double4 __builtin_spirv_OpenCL_modf_v4f64_p4v4f64( double4            x,
                                            __generic double4* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v4f64(x);
    double4 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    return temp;
}

double8 __builtin_spirv_OpenCL_modf_v8f64_p4v8f64( double8            x,
                                            __generic double8* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v8f64(x);
    double8 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    return temp;
}

double16 __builtin_spirv_OpenCL_modf_v16f64_p4v16f64( double16            x,
                                               __generic double16* iptr )
{
    *iptr = __builtin_spirv_OpenCL_trunc_v16f64(x);
    double16 temp = x - *iptr;
    temp.s0 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s0) ? 0.0 : temp.s0), x.s0);
    temp.s1 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s1) ? 0.0 : temp.s1), x.s1);
    temp.s2 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s2) ? 0.0 : temp.s2), x.s2);
    temp.s3 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s3) ? 0.0 : temp.s3), x.s3);
    temp.s4 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s4) ? 0.0 : temp.s4), x.s4);
    temp.s5 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s5) ? 0.0 : temp.s5), x.s5);
    temp.s6 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s6) ? 0.0 : temp.s6), x.s6);
    temp.s7 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s7) ? 0.0 : temp.s7), x.s7);
    temp.s8 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s8) ? 0.0 : temp.s8), x.s8);
    temp.s9 = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.s9) ? 0.0 : temp.s9), x.s9);
    temp.sa = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sa) ? 0.0 : temp.sa), x.sa);
    temp.sb = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sb) ? 0.0 : temp.sb), x.sb);
    temp.sc = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sc) ? 0.0 : temp.sc), x.sc);
    temp.sd = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sd) ? 0.0 : temp.sd), x.sd);
    temp.se = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.se) ? 0.0 : temp.se), x.se);
    temp.sf = __builtin_spirv_OpenCL_copysign_f64_f64((__intel_relaxed_isinf(x.sf) ? 0.0 : temp.sf), x.sf);
    return temp;
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// nan

INLINE double __builtin_spirv_OpenCL_nan_i64( ulong nancode )
{
    return as_double( DOUBLE_QUIET_NAN );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_nan, double, ulong, i64 )

// nextafter

double __builtin_spirv_OpenCL_nextafter_f64_f64( double x, double y )
{
    const long maxneg = DOUBLE_SIGN_MASK;

    long smix = as_long(x);
    long nx = maxneg - smix;
    long tcix = ( smix < 0 ) ? nx : smix;

    long smiy = as_long(y);
    long ny = maxneg - smiy;
    long tciy = ( smiy < 0 ) ? ny : smiy;

    long delta = ( tcix < tciy ) ? 1 : -1;

    long tcir = tcix + delta;
    long nr = maxneg - tcir;
    long smir = ( tcir < 0 ) ? nr : tcir;

    double result = as_double(smir);
    result = (tcix == tciy) ? y : result;

    {
        double n = __builtin_spirv_OpenCL_nan_i64(0ul);
        int test = SPIRV_BUILTIN(IsNan, _f64, )(x) | SPIRV_BUILTIN(IsNan, _f64, )(y);
        result = test ? n : result;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_nextafter, double, double, f64 )

//pow

INLINE double __builtin_spirv_OpenCL_pow_f64_f64( double x, double y )
{
    return __ocl_svml_pow(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pow, double, double, double, f64, f64 )

// pown

INLINE double __builtin_spirv_OpenCL_pown_f64_i32( double x, int y )
{
    return __ocl_svml_pown(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pown, double, double, int, f64, i32 )


// powr
INLINE double __builtin_spirv_OpenCL_powr_f64_f64( double x, double y )
{
    return __ocl_svml_powr(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_powr, double, double, double, f64, f64 )

// remainder

INLINE double __builtin_spirv_OpenCL_remainder_f64_f64( double x, double y )
{
    int temp;
    return __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(x, y, &temp);
}

INLINE double __builtin_spirv_OpFRem_f64_f64( double x, double y )
{
    return __builtin_spirv_OpenCL_fmod_f64_f64(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_remainder, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpFRem, double, double, double, f64, f64 )

// remquo

double __builtin_spirv_OpenCL_remquo_f64_f64_p1i32( double        xx,
                                             double        yy,
                                             __global int* quo )
{
    double result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0 )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_nan_i64((ulong)0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0) )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_select_f64_f64_i64(__builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx), xx, (long)__intel_relaxed_isinf(yy));
    }
    else if( __builtin_spirv_OpenCL_fabs_f64(xx) == __builtin_spirv_OpenCL_fabs_f64(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = __builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f64, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f64, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        double x = __builtin_spirv_OpenCL_fabs_f64(xx);
        double y = __builtin_spirv_OpenCL_fabs_f64(yy);
        int ex, ey;
        ex = __builtin_spirv_OpenCL_ilogb_f64( x );
        ey = __builtin_spirv_OpenCL_ilogb_f64( y );
        double xr = x;
        double yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f64_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32( x, -ex );
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
              xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ex - ey);
          }
        }
        if( (yr < 2.0*xr) | ( (yr == 2.0*xr) & (q & 0x1) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ey);
        }

        uint qout = q & 0x7f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0 )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p1v2i32( double2         xx,
                                                    double2         yy,
                                                    __global int2*  quo )
{
    double2 temp;
    double in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p1v3i32( double3        xx,
                                                    double3        yy,
                                                    __global int3* quo )
{
    double3 temp;
    double in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p1v4i32( double4        xx,
                                                    double4        yy,
                                                    __global int4* quo )
{
    double4 temp;
    double in1[4], in2[4], out[4];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
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

double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p1v8i32( double8        xx,
                                                    double8        yy,
                                                    __global int8* quo )
{
    double8 temp;
    double in1[8], in2[8], out[8];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
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

double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p1v16i32( double16        xx,
                                                        double16        yy,
                                                        __global int16* quo )
{
    double16 temp;
    double in1[16], in2[16], out[16];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
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

double __builtin_spirv_OpenCL_remquo_f64_f64_p0i32( double         xx,
                                             double         yy,
                                             __private int* quo )
{
    double result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0 )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_nan_i64((ulong)0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0) )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_select_f64_f64_i64(__builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx), xx, (long)__intel_relaxed_isinf(yy));
    }
    else if( __builtin_spirv_OpenCL_fabs_f64(xx) == __builtin_spirv_OpenCL_fabs_f64(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = __builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f64, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f64, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        double x = __builtin_spirv_OpenCL_fabs_f64(xx);
        double y = __builtin_spirv_OpenCL_fabs_f64(yy);
        int ex, ey;
        ex = __builtin_spirv_OpenCL_ilogb_f64( x );
        ey = __builtin_spirv_OpenCL_ilogb_f64( y );
        double xr = x;
        double yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f64_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32( x, -ex );
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
              xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ex - ey);
          }
        }
        if( (yr < 2.0*xr) | ( (yr == 2.0*xr) && (q & 0x1) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ey);
        }

        int qout = q & 0x7f;

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

double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p0v2i32( double2         xx,
                                                    double2         yy,
                                                    __private int2* quo )
{
    double2 temp;
    double in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p0v3i32( double3         xx,
                                                    double3         yy,
                                                    __private int3* quo )
{
    double3 temp;
    double in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p0v4i32( double4         xx,
                                                    double4         yy,
                                                    __private int4* quo )
{
    double4 temp;
    double in1[4], in2[4], out[4];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
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

double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p0v8i32( double8         xx,
                                                    double8         yy,
                                                    __private int8* quo )
{
    double8 temp;
    double in1[8], in2[8], out[8];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
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

double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p0v16i32( double16         xx,
                                                        double16         yy,
                                                        __private int16* quo )
{
    double16 temp;
    double in1[16], in2[16], out[16];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
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

double __builtin_spirv_OpenCL_remquo_f64_f64_p3i32( double       xx,
                                             double       yy,
                                             __local int* quo )
{
    double result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0 )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0) )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_select_f64_f64_i64(__builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx), xx, (long)__intel_relaxed_isinf(yy));
    }
    else if( __builtin_spirv_OpenCL_fabs_f64(xx) == __builtin_spirv_OpenCL_fabs_f64(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = __builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f64, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f64, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        double x = __builtin_spirv_OpenCL_fabs_f64(xx);
        double y = __builtin_spirv_OpenCL_fabs_f64(yy);
        int ex, ey;
        ex = __builtin_spirv_OpenCL_ilogb_f64( x );
        ey = __builtin_spirv_OpenCL_ilogb_f64( y );
        double xr = x;
        double yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f64_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32( x, -ex );
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
              xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ex - ey);
          }
        }
        if( (yr < 2.0*xr) | ( (yr == 2.0*xr) && (q & 0x1) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ey);
        }

        int qout = q & 0x7f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0 )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p3v2i32( double2       xx,
                                                    double2       yy,
                                                    __local int2* quo )
{
    double2 temp;
    double in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p3v3i32( double3       xx,
                                                    double3       yy,
                                                    __local int3* quo )
{
    double3 temp;
    double in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p3v4i32( double4       xx,
                                                    double4       yy,
                                                    __local int4* quo )
{
    double4 temp;
    double in1[4], in2[4], out[4];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
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

double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p3v8i32( double8       xx,
                                                    double8       yy,
                                                    __local int8* quo )
{
    double8 temp;
    double in1[8], in2[8], out[8];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
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

double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p3v16i32( double16       xx,
                                                        double16       yy,
                                                        __local int16* quo )
{
    double16 temp;
    double in1[16], in2[16], out[16];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p0i32(in1[i], in2[i], (__private int*)&out2[i]);
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

double __builtin_spirv_OpenCL_remquo_f64_f64_p4i32( double         xx,
                                             double         yy,
                                             __generic int* quo )
{
    double result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0 )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0) )
    {
        *quo = 0;
        result = __builtin_spirv_OpenCL_select_f64_f64_i64(__builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx), xx, (long)__intel_relaxed_isinf(yy));
    }
    else if( __builtin_spirv_OpenCL_fabs_f64(xx) == __builtin_spirv_OpenCL_fabs_f64(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = __builtin_spirv_OpenCL_copysign_f64_f64(0.0, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f64, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f64, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        double x = __builtin_spirv_OpenCL_fabs_f64(xx);
        double y = __builtin_spirv_OpenCL_fabs_f64(yy);
        int ex, ey;
        ex = __builtin_spirv_OpenCL_ilogb_f64( x );
        ey = __builtin_spirv_OpenCL_ilogb_f64( y );
        double xr = x;
        double yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = __builtin_spirv_OpenCL_ldexp_f64_i32( y, -ey );
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32( x, -ex );
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
              xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ex - ey);
          }
        }
        if( (yr < 2.0*xr) | ( (yr == 2.0*xr) && (q & 0x1) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = __builtin_spirv_OpenCL_ldexp_f64_i32(xr, ey);
        }

        int qout = q & 0x7f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0 )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

double2 __builtin_spirv_OpenCL_remquo_v2f64_v2f64_p4v2i32( double2         xx,
                                                    double2         yy,
                                                    __generic int2* quo )
{
    double2 temp;
    double in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

double3 __builtin_spirv_OpenCL_remquo_v3f64_v3f64_p4v3i32( double3         xx,
                                                    double3         yy,
                                                    __generic int3* quo )
{
    double3 temp;
    double in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

double4 __builtin_spirv_OpenCL_remquo_v4f64_v4f64_p4v4i32( double4         xx,
                                                    double4         yy,
                                                    __generic int4* quo )
{
    double4 temp;
    double in1[4], in2[4], out[4];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
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

double8 __builtin_spirv_OpenCL_remquo_v8f64_v8f64_p4v8i32( double8         xx,
                                                    double8         yy,
                                                    __generic int8* quo )
{
    double8 temp;
    double in1[8], in2[8], out[8];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
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

double16 __builtin_spirv_OpenCL_remquo_v16f64_v16f64_p4v16i32( double16         xx,
                                                        double16         yy,
                                                        __generic int16* quo )
{
    double16 temp;
    double in1[16], in2[16], out[16];
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
        out[i] = __builtin_spirv_OpenCL_remquo_f64_f64_p4i32(in1[i], in2[i], (__generic int*)&out2[i]);
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

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// rint

INLINE double __builtin_spirv_OpenCL_rint_f64( double x )
{
  double absolute_x;
  double rounded_int;
  uint exp_;

  // abs value
  absolute_x = as_double(as_ulong(x) & ~DOUBLE_SIGN_MASK);

  // round to nearest int if mantissa contains fractional parts
  exp_ = as_ulong(absolute_x) >> DOUBLE_MANTISSA_BITS;
  double nearest_int = 0.5 * (double)((exp_ < DOUBLE_MANTISSA_BITS + DOUBLE_BIAS) & 1);
  rounded_int = __builtin_spirv_OpenCL_trunc_f64(absolute_x + nearest_int);

  // get the parity bit; does src has a fraction equal to 0.5?
  uint parity = ((ulong)rounded_int) & 0x1;
  uint has_a_half = ((rounded_int - absolute_x) == 0.5) & 0x1;

  // if so, adjust the previous, truncated round, to the nearest even
  rounded_int = rounded_int - 1.0 * (double)(has_a_half & parity);

  // reapply the sign
  ulong sign = as_ulong(x) & DOUBLE_SIGN_MASK;
  rounded_int = as_double(sign | as_ulong(rounded_int));

  return rounded_int;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rint, double, double, f64 )

// rootn
INLINE double __builtin_spirv_OpenCL_rootn_f64_i32( double y, int x )
{
    return __ocl_svml_rootn_v2(y, x);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_rootn, double, double, int, f64, i32 )

// round

INLINE INLINE double __builtin_spirv_OpenCL_round_f64( double x )
{
    double delta = as_double(0x3FDFFFFFFFFFFFFFl);   // one bit less than 0.5
    double nd = x - delta;
    double pd = x + delta;
    x = ( x < 0 ) ? nd : pd;
    return __builtin_spirv_OpenCL_trunc_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_round, double, double, f64 )

// rsqrt

INLINE double __builtin_spirv_OpenCL_rsqrt_f64( double x )
{
    return __builtin_spirv_OpenCL_native_rsqrt_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_rsqrt, double, double, f64 )

// sin
INLINE double __builtin_spirv_OpenCL_sin_f64( double x )
{
    return __ocl_svml_sin(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sin, double, double, f64 )

// sincos
INLINE double __builtin_spirv_OpenCL_sincos_f64_p0f64( double x, __private double* cosval )
{
    double sin_x, cos_x;
    __ocl_svml_sincos(x, &sin_x, &cos_x);
    *cosval = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( __builtin_spirv_OpenCL_sincos, double, double, double, f64, f64 )

double __builtin_spirv_OpenCL_sincos_f64_p3f64( double x, __local double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f64_p0f64( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, double, __local, double, f64, p3 )



INLINE double __builtin_spirv_OpenCL_sincos_f64_p1f64( double x, __global double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f64_p0f64( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, double, __global, double, f64, p1 )


INLINE double __builtin_spirv_OpenCL_sincos_f64_p4f64( double x, __generic double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f64_p0f64( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, double, __generic, double, f64, p4 )

// sinh

INLINE double __builtin_spirv_OpenCL_sinh_f64( double x )
{
    return __ocl_svml_sinh(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinh, double, double, f64 )

// sinpi

INLINE double __builtin_spirv_OpenCL_sinpi_f64( double x )
{
    return __ocl_svml_sinpi(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinpi, double, double, f64 )

// sqrt_cr

#ifdef cl_fp64_basic_ops

INLINE double __builtin_spirv_OpenCL_sqrt_cr_f64( double x )
{
        return __builtin_spirv_OpenCL_sqrt_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sqrt_cr, double, double, f64 )

#endif // cl_fp64_basic_ops

// sqrt

#ifdef cl_fp64_basic_ops

INLINE double __builtin_spirv_OpenCL_sqrt_f64( double x )
{
    return __builtin_spirv_OpenCL_native_sqrt_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sqrt, double, double, f64 )

#endif // cl_fp64_basic_ops

// tan

INLINE double __builtin_spirv_OpenCL_tan_f64( double x )
{
    return __ocl_svml_tan(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, double, double, f64 )

// tanh

INLINE double __builtin_spirv_OpenCL_tanh_f64( double x )
{
    return __ocl_svml_tanh(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanh, double, double, f64 )

// tanpi

INLINE double __builtin_spirv_OpenCL_tanpi_f64( double x )
{
    return __ocl_svml_tanpi(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanpi, double, double, f64 )

// tgamma

INLINE double __builtin_spirv_OpenCL_tgamma_f64( double x )
{
    return libclc_tgamma_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tgamma, double, double, f64 )

// trunc

INLINE double __builtin_spirv_OpenCL_trunc_f64(double x )
{
    //Algorithm performs rounding towards zero by truncating bits in the fractional part
    // of the number.This is done by finding out the position of the fractional bits of
    //the mantissa and masking them out with zeros.

    signed high32Bit = (int)(as_long( x ) >> 32);
    //int physicalexp = (int)__builtin_IB_ubfe( 11,20,high32bit );
    uint physicalExp = ( high32Bit & 0x7fffffff ) >> 20;             // Extract the physical exponent.
    int  logicalExp = (int)physicalExp - DOUBLE_BIAS;                // Extract the logical exponent.

    // Compute shift amount.
    signed shiftAmountHigh32bit = -logicalExp + 52;
    signed shiftAmountLow32bit  = -logicalExp + 20;

    shiftAmountHigh32bit = ( shiftAmountHigh32bit > 32 ) ? 32 : shiftAmountHigh32bit;
    shiftAmountLow32bit  = ( shiftAmountLow32bit  > 20 ) ? 20 : shiftAmountLow32bit;
    shiftAmountHigh32bit = ( shiftAmountHigh32bit > 0 )  ? shiftAmountHigh32bit : 0;
    shiftAmountLow32bit  = ( shiftAmountLow32bit  > 0 )  ? shiftAmountLow32bit  : 0;

    // Create final mask.
    // All-ones initial mask.
    signed shiftedValHigh32bit = 0xFFFFFFFF << shiftAmountHigh32bit;
    signed shiftedValLow32bit  = 0xFFFFFFFF << shiftAmountLow32bit;
    signed temp1 = 0;
    signed temp2 = 0;
    signed maskValLow32bit = 0;
    signed maskValHigh32bit = 0x80000000;   // Initialize sign only mask. If exponent < 0 , override the mask with sign-only mask.
    temp1 = ((shiftAmountHigh32bit != 32) ? shiftedValHigh32bit : 0x00000000); // If exponent = 32, mask with 0.
    if(shiftAmountLow32bit != 32)
    {
        temp2 = shiftedValLow32bit;
        if(!(logicalExp < 0))
        {
            maskValLow32bit  = temp1;
            maskValHigh32bit = temp2;
        }
    }

    uint andDst1 = (uint)(as_ulong( x )) & maskValLow32bit;
    uint andDst2 = high32Bit & maskValHigh32bit;
    //combining low and high 32-bits forming 64-bit val.
    ulong temp3 = (ulong)andDst2 << 32;
    ulong temp4 = ((ulong)andDst1) ;
    ulong roundedToZeroVal = temp3 | as_ulong( temp4 );    // Rounding towards zero is completed at this point
    return as_double( roundedToZeroVal );
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_trunc, double, double, f64 )