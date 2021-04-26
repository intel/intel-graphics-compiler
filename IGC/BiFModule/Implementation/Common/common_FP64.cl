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


// fclamp
GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_fclamp, double, double, f64 )

// degrees
double __builtin_spirv_OpenCL_degrees_f64(double r ){
    return ONE_EIGHTY_OVER_PI_DBL * r;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_degrees, double, double, f64 )

// fmax
INLINE double __builtin_spirv_OpenCL_fmax_common_f64_f64(double x, double y) {
    return __builtin_IB_dmax(x, y);
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmax_common, double, double, f64 )

// fmin
INLINE double __builtin_spirv_OpenCL_fmin_common_f64_f64(double x, double y) {
    //return __builtin_IB_minf(x, y);
    return (x <= y) ? x : y;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_fmin_common, double, double, f64 )

// mix
INLINE double __builtin_spirv_OpenCL_mix_f64_f64_f64(double x, double y, double a ){
    return __builtin_spirv_OpenCL_mad_f64_f64_f64( ( y - x ), a, x );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_mix, double, double, f64 )

// radians
INLINE double __builtin_spirv_OpenCL_radians_f64(double d ){
    return PI_OVER_ONE_EIGHTY_DBL * d;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_radians, double, double, f64 )

// sign
INLINE double __builtin_spirv_OpenCL_sign_f64( double x )
{
    double result = x;
    result = ( x > 0.0 ) ?  1.0 : result;
    result = ( x < 0.0 ) ? -1.0 : result;
    result = __intel_relaxed_isnan(x) ? 0.0 : result;
    return result ;
}

GENERATE_VECTOR_FUNCTIONS_1ARG( __builtin_spirv_OpenCL_sign, double, double, f64 )

// smoothstep
INLINE double __builtin_spirv_OpenCL_smoothstep_f64_f64_f64(double edge0, double edge1, double x ){
    double div = (x - edge0) / (edge1 - edge0);
    double temp = __builtin_spirv_OpenCL_fclamp_f64_f64_f64( div, 0.0, 1.0 );
    return temp * temp * __builtin_spirv_OpenCL_mad_f64_f64_f64( -2.0, temp, 3.0 );
}

GENERATE_VECTOR_FUNCTIONS_3ARGS( __builtin_spirv_OpenCL_smoothstep, double, double, f64 )

// step
INLINE double __builtin_spirv_OpenCL_step_f64_f64(double edge, double x ){
    return x < edge ? 0.0 : 1.0;
}

GENERATE_VECTOR_FUNCTIONS_2ARGS( __builtin_spirv_OpenCL_step, double, double, f64 )
