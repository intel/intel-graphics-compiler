/*========================== begin_copyright_notice ============================

Copyright (c) 2016-2021 Intel Corporation

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

INLINE float OVERLOADABLE half_tan( float x )
{
    return __builtin_spirv_OpenCL_half_tan_f32( x );
}

INLINE float2 OVERLOADABLE half_tan( float2 x )
{
    return __builtin_spirv_OpenCL_half_tan_v2f32( x );
}

INLINE float3 OVERLOADABLE half_tan( float3 x )
{
    return __builtin_spirv_OpenCL_half_tan_v3f32( x );
}

INLINE float4 OVERLOADABLE half_tan( float4 x )
{
    return __builtin_spirv_OpenCL_half_tan_v4f32( x );
}

INLINE float8 OVERLOADABLE half_tan( float8 x )
{
    return __builtin_spirv_OpenCL_half_tan_v8f32( x );
}

INLINE float16 OVERLOADABLE half_tan( float16 x )
{
    return __builtin_spirv_OpenCL_half_tan_v16f32( x );
}
