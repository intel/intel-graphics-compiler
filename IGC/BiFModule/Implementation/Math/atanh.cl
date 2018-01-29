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

    #include "../ExternalLibraries/libclc/doubles.cl"

#endif // defined(cl_khr_fp64)

float __builtin_spirv_OpenCL_atanh_f32( float x )
{
    /*
    Cephes Math Library Release 2.2:  June, 1992
    Copyright 1984, 1987, 1989 by Stephen L. Moshier
    Direct inquiries to 30 Frost Street, Cambridge, MA 02140
    What you see here may be used freely but it comes with no support or
    guarantee.
    */
    float z = __builtin_spirv_OpenCL_fabs_f32(x);
    float result;
    if( z < 1.0e-4f )
    {
        result = x;
    }
    else if( z < 0.5f )
    {
        z = x * x;
        result = (((( 1.81740078349E-1f  * z +
                      8.24370301058E-2f) * z +
                      1.46691431730E-1f) * z +
                      1.99782164500E-1f) * z +
                      3.33337300303E-1f) * z * x +
                      x;
    }
    else if( z < 1.0f )
    {
        result = 0.5f * __builtin_spirv_OpenCL_log_f32( (1.0f + x) / (1.0f - x) );
    }
    else
    {
        result = __builtin_spirv_OpenCL_nan_i32((uint)0);
        result = ( x ==  1.0f ) ?  INFINITY : result;
        result = ( x == -1.0f ) ? -INFINITY : result;
    }

    return result;
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, float, float, f32 )

#if defined(cl_khr_fp64)

INLINE double __builtin_spirv_OpenCL_atanh_f64( double x )
{
        return libclc_atanh_f64(x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, double, double, f64 )

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half __builtin_spirv_OpenCL_atanh_f16( half x )
{
    return __builtin_spirv_OpenCL_atanh_f32((float)x);
}

GENERATE_VECTOR_FUNCTIONS_1ARG_LOOP( __builtin_spirv_OpenCL_atanh, half, half, f16 )

#endif // defined(cl_khr_fp16)
