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

#if defined(cl_khr_fp64)
    #include "../IMF/FP64/acosh_d_la.cl"
#endif // defined(cl_khr_fp64)

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
        /*
        Cephes Math Library Release 2.2:  June, 1992
        Copyright 1984, 1987, 1989 by Stephen L. Moshier
        Direct inquiries to 30 Frost Street, Cambridge, MA 02140
        What you see here may be used freely but it comes with no support or
        guarantee.
        */
        if (__intel_relaxed_isnan(x))
        {
            result = __builtin_spirv_OpenCL_nan_i32((uint)0);
        }
        else if( x < 1.0f )
        {
            result = __builtin_spirv_OpenCL_nan_i32((uint)0);
        }
        else if( x > 1500.0f )
        {
            result = __builtin_spirv_OpenCL_log_f32(x) + M_LN2_F;
        }
        else
        {
            result = x - 1.0f;
            if( result < 0.5f )
            {
                result = (((( 1.7596881071E-3f  * result -
                        7.5272886713E-3f) * result +
                        2.6454905019E-2f) * result -
                        1.1784741703E-1f) * result +
                        1.4142135263E0f)  * __builtin_spirv_OpenCL_native_sqrt_f32( result );
            }
            else
            {
                result = __builtin_spirv_OpenCL_native_sqrt_f32( result*(x+1.0f) );
                result = __builtin_spirv_OpenCL_log_f32(x + result);
            }
        }
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_acosh_f64( double x )
{
    return __ocl_svml_acosh(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_acosh_f16( half x )
{
    return __builtin_spirv_OpenCL_acosh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_acosh, half, half, f16 )

#endif // defined(cl_khr_fp16)
