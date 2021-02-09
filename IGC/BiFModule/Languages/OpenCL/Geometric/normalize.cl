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

INLINE float OVERLOADABLE normalize( float p )
{
    return __builtin_spirv_OpenCL_normalize_f32( p );
}

float2 OVERLOADABLE normalize( float2 p )
{
    return __builtin_spirv_OpenCL_normalize_v2f32( p );
}

float3 OVERLOADABLE normalize( float3 p )
{
    return __builtin_spirv_OpenCL_normalize_v3f32( p );
}

float4 OVERLOADABLE normalize( float4 p )
{
    return __builtin_spirv_OpenCL_normalize_v4f32( p );
}

#if defined(cl_khr_fp64)

INLINE double OVERLOADABLE normalize( double p )
{
    return __builtin_spirv_OpenCL_normalize_f64( p );
}

double2 OVERLOADABLE normalize( double2 p )
{
    return __builtin_spirv_OpenCL_normalize_v2f64( p );
}

double3 OVERLOADABLE normalize( double3 p )
{
    return __builtin_spirv_OpenCL_normalize_v3f64( p );
}

double4 OVERLOADABLE normalize( double4 p )
{
    return __builtin_spirv_OpenCL_normalize_v4f64( p );
}

#endif // defined(cl_khr_fp64)

#if defined(cl_khr_fp16)

INLINE half OVERLOADABLE normalize( half p )
{
    return __builtin_spirv_OpenCL_normalize_f16( p );
}

half2 OVERLOADABLE normalize( half2 p )
{
    return __builtin_spirv_OpenCL_normalize_v2f16( p );
}

half3 OVERLOADABLE normalize( half3 p )
{
    return __builtin_spirv_OpenCL_normalize_v3f16( p );
}

half4 OVERLOADABLE normalize( half4 p )
{
    return __builtin_spirv_OpenCL_normalize_v4f16( p );
}

#endif // defined(cl_khr_fp16)
