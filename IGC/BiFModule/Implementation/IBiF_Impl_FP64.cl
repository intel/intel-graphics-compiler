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

#include "IBiF_Macros.cl"

#define INLINE __attribute__((always_inline))

//*****************************************************************************/
// Helper functions
//*****************************************************************************/

float __builtin_IB_frnd_ne(float);

INLINE
float __builtin_spirv_OpenCL_fma_f32_f32_f32( float a, float b, float c )
{
    return __builtin_fmaf(a, b, c);
}

INLINE
double __builtin_spirv_OpenCL_fma_f64_f64_f64( double a, double b, double c )
{
    return __builtin_fma(a, b, c);
}

INLINE
double __builtin_spirv_OpenCL_fmin_f64_f64( double x, double y )
{
    return __builtin_fmin(x, y);
}

INLINE
float __builtin_spirv_OpenCL_fabs_f32(float x ){
    float neg = -x;
    return (x >= 0) ?  x : neg;
}

INLINE
double __builtin_spirv_OpenCL_fabs_f64( double x )
{
    double neg = -x;
    return (x >= 0) ?  x : neg;
}

INLINE
float __builtin_spirv_OpenCL_rint_f32(float x ){
    //return __builtin_rintf(x);
    return __builtin_IB_frnd_ne(x);
}

INLINE
float  __builtin_spirv_OpenCL_sqrt_f32( float x )
{
    return __builtin_sqrtf(x);
}

INLINE
double __builtin_spirv_OpenCL_sqrt_f64( double x )
{
    return __builtin_sqrt(x);
}

//*****************************************************************************/
// Math builtin functions
//*****************************************************************************/

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

INLINE double __builtin_spirv_OpenCL_acos_f64( double x )
{
    return __ocl_svml_acos(x);
}

INLINE double __builtin_spirv_OpenCL_acosh_f64( double x )
{
    return __ocl_svml_acosh(x);
}

INLINE double __builtin_spirv_OpenCL_acospi_f64( double x )
{
    return __ocl_svml_acospi(x);
}

INLINE double __builtin_spirv_OpenCL_asin_f64( double x )
{
    return __ocl_svml_asin(x);
}

INLINE double __builtin_spirv_OpenCL_asinh_f64( double x )
{
    return __ocl_svml_asinh(x);
}

INLINE double __builtin_spirv_OpenCL_asinpi_f64( double x )
{
    return __ocl_svml_asinpi(x);
}

INLINE double __builtin_spirv_OpenCL_atan_f64( double x )
{
    return __ocl_svml_atan(x);
}

INLINE double __builtin_spirv_OpenCL_atan2_f64_f64( double y, double x )
{
    return __ocl_svml_atan2(y, x);
}

INLINE double __builtin_spirv_OpenCL_atan2pi_f64_f64( double x, double y )
{
    return M_1_PI * __builtin_spirv_OpenCL_atan2_f64_f64(x, y);
}

INLINE double __builtin_spirv_OpenCL_atanh_f64( double x )
{
    return __ocl_svml_atanh(x);
}

INLINE double __builtin_spirv_OpenCL_atanpi_f64( double x )
{
    return __ocl_svml_atanpi(x);
}

INLINE double __builtin_spirv_OpenCL_cbrt_f64( double x )
{
    return __ocl_svml_cbrt(x);
}

INLINE double __builtin_spirv_OpenCL_cos_f64( double x )
{
    return __ocl_svml_cos(x);
}

INLINE double __builtin_spirv_OpenCL_cosh_f64( double x )
{
    return __ocl_svml_cosh(x);
}

INLINE double __builtin_spirv_OpenCL_cospi_f64( double x )
{
    return __ocl_svml_cospi(x);
}

INLINE double __builtin_spirv_OpenCL_erf_f64( double x )
{
    return __ocl_svml_erf(x);
}

INLINE double __builtin_spirv_OpenCL_erfc_f64( double x )
{
    return __ocl_svml_erfc(x);
}

INLINE double __builtin_spirv_OpenCL_exp_f64( double x )
{
    return __ocl_svml_exp(x);
}

INLINE double __builtin_spirv_OpenCL_exp10_f64( double x )
{
    return __ocl_svml_exp10(x);
}

INLINE double __builtin_spirv_OpenCL_exp2_f64( double x )
{
    return __ocl_svml_exp2(x);
}

INLINE double __builtin_spirv_OpenCL_expm1_f64( double x )
{
    return __ocl_svml_expm1(x);
}

INLINE double __builtin_spirv_OpenCL_log_f64( double x )
{
    return __ocl_svml_log(x);
}

INLINE double __builtin_spirv_OpenCL_log10_f64( double x )
{
    return __ocl_svml_log10_v2(x);
}

INLINE double __builtin_spirv_OpenCL_log1p_f64( double x )
{
    return __ocl_svml_log1p(x);
}

INLINE double __builtin_spirv_OpenCL_log2_f64( double x )
{
    return __ocl_svml_log2_v2(x);
}

INLINE double __builtin_spirv_OpenCL_pow_f64_f64( double x, double y )
{
    return __ocl_svml_pow(x, y);
}

INLINE double __builtin_spirv_OpenCL_pown_f64_i32( double x, int y )
{
    return __ocl_svml_pown(x, y);
}

INLINE double __builtin_spirv_OpenCL_powr_f64_f64( double x, double y )
{
    return __ocl_svml_powr(x, y);
}

INLINE double __builtin_spirv_OpenCL_rootn_f64_i32( double y, int x )
{
    return __ocl_svml_rootn_v2(y, x);
}

INLINE double __builtin_spirv_OpenCL_sin_f64( double x )
{
    return __ocl_svml_sin(x);
}

INLINE double __builtin_spirv_OpenCL_sincos_f64_p0f64( double x, __private double* cosval )
{
    double sin_x, cos_x;
    __ocl_svml_sincos(x, &sin_x, &cos_x);
    *cosval = cos_x;
    return sin_x;
}

double __builtin_spirv_OpenCL_sincos_f64_p3f64( double x, __local double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f64_p0f64( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

INLINE double __builtin_spirv_OpenCL_sincos_f64_p1f64( double x, __global double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f64_p0f64( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

INLINE double __builtin_spirv_OpenCL_sincos_f64_p4f64( double x, __generic double* cosval )
{
    double   sin_x, cos_x;
    sin_x = __builtin_spirv_OpenCL_sincos_f64_p0f64( x, &cos_x );
    cosval[0] = cos_x;
    return sin_x;
}

INLINE double __builtin_spirv_OpenCL_sinh_f64( double x )
{
    return __ocl_svml_sinh(x);
}

INLINE double __builtin_spirv_OpenCL_sinpi_f64( double x )
{
    return __ocl_svml_sinpi(x);
}

INLINE double __builtin_spirv_OpenCL_tan_f64( double x )
{
    return __ocl_svml_tan(x);
}

INLINE double __builtin_spirv_OpenCL_tanh_f64( double x )
{
    return __ocl_svml_tanh(x);
}

INLINE double __builtin_spirv_OpenCL_tanpi_f64( double x )
{
    return __ocl_svml_tanpi(x);
}


GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_acos, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_acospi, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asin, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asinh, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_asinpi, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atan, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_atan2, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_atan2pi, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_atanpi, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_cbrt, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cos, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cosh, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_cospi, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erf, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_erfc, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_exp, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp10, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_exp2, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_expm1, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log10, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log1p, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_log2, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pow, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_pown, double, double, int, f64, i32 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_powr, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_2ARGS_VV_LOOP( __builtin_spirv_OpenCL_rootn, double, double, int, f64, i32 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sin, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1VAL_1PTRARG_LOOP( __builtin_spirv_OpenCL_sincos, double, double, double, f64, f64 )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, double, __global, double, f64, p1 )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, double, __local, double, f64, p3 )
GENERATE_VECTOR_FUNCTIONS_1VALARG_1PTRARG( __builtin_spirv_OpenCL_sincos, double, __generic, double, f64, p4 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinh, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_sinpi, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tan, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanh, double, double, f64 )
GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_tanpi, double, double, f64 )