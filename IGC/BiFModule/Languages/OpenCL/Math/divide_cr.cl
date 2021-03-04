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

float OVERLOADABLE divide_cr( float a,
                              float b )
{
    return __builtin_spirv_divide_cr_f32_f32( a, b );
}

INLINE
float2 OVERLOADABLE divide_cr( float2 a,
                               float2 b )
{
    return __builtin_spirv_divide_cr_v2f32_v2f32( a, b );
}

INLINE
float3 OVERLOADABLE divide_cr( float3 a,
                               float3 b )
{
    return __builtin_spirv_divide_cr_v3f32_v3f32( a, b );
}

INLINE
float4 OVERLOADABLE divide_cr( float4 a,
                               float4 b )
{
    return __builtin_spirv_divide_cr_v4f32_v4f32( a, b );
}

INLINE
float8 OVERLOADABLE divide_cr( float8 a,
                               float8 b )
{
    return __builtin_spirv_divide_cr_v8f32_v8f32( a, b );
}

INLINE
float16 OVERLOADABLE divide_cr( float16 a,
                                float16 b )
{
    return __builtin_spirv_divide_cr_v16f32_v16f32( a, b );
}
