/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "spirv.h"

INLINE float OVERLOADABLE length( float p )
{
    return __builtin_spirv_OpenCL_length_f32( p );
}

// Note: This function is slightly different than hypot().
// It has fewer checks for NaNs.
float OVERLOADABLE length( float2 p )
{
    return __builtin_spirv_OpenCL_length_v2f32( p );
}

float OVERLOADABLE length( float3 p )
{
    return __builtin_spirv_OpenCL_length_v3f32( p );
}

float OVERLOADABLE length( float4 p )
{
    return __builtin_spirv_OpenCL_length_v4f32( p );
}

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE length( double p )
{
    return __builtin_spirv_OpenCL_length_f64( p );
}

// Note: This function is slightly different than hypot().
// It has fewer checks for NaNs.
double OVERLOADABLE length( double2 p )
{
    return __builtin_spirv_OpenCL_length_v2f64( p );
}

double OVERLOADABLE length( double3 p )
{
    return __builtin_spirv_OpenCL_length_v3f64( p );
}

double OVERLOADABLE length( double4 p )
{
    return __builtin_spirv_OpenCL_length_v4f64( p );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE length( half p )
{
    return __builtin_spirv_OpenCL_length_f16( p );
}

// Note: This function is slightly different than hypot().
// It has fewer checks for NaNs.
half OVERLOADABLE length( half2 p )
{
    return __builtin_spirv_OpenCL_length_v2f16( p );
}

half OVERLOADABLE length( half3 p )
{
    return __builtin_spirv_OpenCL_length_v3f16( p );
}

half OVERLOADABLE length( half4 p )
{
    return __builtin_spirv_OpenCL_length_v4f16( p );
}

#endif // defined(cl_khr_fp16)
