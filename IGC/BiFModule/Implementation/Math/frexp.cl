/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __attribute__((overloadable)) __spirv_ocl_frexp( float         x,
                                       __global int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __spirv_ocl_copysign( temp, x );

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
        int lz = __spirv_ocl_clz( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_frexp( float2         x,
                                            __global int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_frexp( float3         x,
                                            __global int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_frexp( float4         x,
                                            __global int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_frexp( float8         x,
                                            __global int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_frexp( float16         x,
                                               __global int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

float __attribute__((overloadable)) __spirv_ocl_frexp( float          x,
                                       __private int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __spirv_ocl_copysign( temp, x );

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
        int lz = __spirv_ocl_clz( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_frexp( float2          x,
                                            __private int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_frexp( float3          x,
                                            __private int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_frexp( float4          x,
                                            __private int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_frexp( float8          x,
                                            __private int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_frexp( float16          x,
                                               __private int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

float __attribute__((overloadable)) __spirv_ocl_frexp( float        x,
                                       __local int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __spirv_ocl_copysign( temp, x );

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
        int lz = __spirv_ocl_clz( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}


INLINE float2 __attribute__((overloadable)) __spirv_ocl_frexp( float2        x,
                                            __local int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_frexp( float3        x,
                                            __local int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_frexp( float4        x,
                                            __local int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

float8 __attribute__((overloadable)) __spirv_ocl_frexp( float8        x,
                                            __local int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

float16 __attribute__((overloadable)) __spirv_ocl_frexp( float16        x,
                                               __local int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

float __attribute__((overloadable)) __spirv_ocl_frexp( float          x,
                                       __generic int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __spirv_ocl_copysign( temp, x );

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
        int lz = __spirv_ocl_clz( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE float2 __attribute__((overloadable)) __spirv_ocl_frexp( float2          x,
                                            __generic int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float3 __attribute__((overloadable)) __spirv_ocl_frexp( float3          x,
                                            __generic int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float4 __attribute__((overloadable)) __spirv_ocl_frexp( float4          x,
                                            __generic int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float8 __attribute__((overloadable)) __spirv_ocl_frexp( float8          x,
                                            __generic int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE float16 __attribute__((overloadable)) __spirv_ocl_frexp( float16          x,
                                               __generic int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

half __attribute__((overloadable)) __spirv_ocl_frexp( half          x,
                                      __global int* exp )
{
    half temp;
    if( __spirv_IsNormal( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __spirv_ocl_copysign( temp, x );

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
        short lz = __spirv_ocl_clz( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_frexp( half2          x,
                                           __global int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_frexp( half3          x,
                                           __global int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_frexp( half4          x,
                                           __global int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_frexp( half8          x,
                                           __global int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_frexp( half16          x,
                                              __global int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

half __attribute__((overloadable)) __spirv_ocl_frexp( half           x,
                                      __private int* exp )
{
    half temp;
    if( __spirv_IsNormal( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __spirv_ocl_copysign( temp, x );

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
        short lz = __spirv_ocl_clz( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_frexp( half2           x,
                                           __private int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_frexp( half3           x,
                                           __private int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_frexp( half4           x,
                                           __private int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_frexp( half8           x,
                                           __private int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_frexp( half16           x,
                                              __private int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

half __attribute__((overloadable)) __spirv_ocl_frexp( half         x,
                                      __local int* exp )
{
    half temp;
    if( __spirv_IsNormal( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __spirv_ocl_copysign( temp, x );

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
        short lz = __spirv_ocl_clz( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_frexp( half2         x,
                                           __local int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_frexp( half3         x,
                                           __local int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_frexp( half4         x,
                                           __local int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_frexp( half8         x,
                                           __local int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_frexp( half16         x,
                                              __local int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half __attribute__((overloadable)) __spirv_ocl_frexp( half           x,
                                      __generic int* exp )
{
    half temp;
    if( __spirv_IsNormal( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __spirv_ocl_copysign( temp, x );

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
        short lz = __spirv_ocl_clz( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 __attribute__((overloadable)) __spirv_ocl_frexp( half2           x,
                                           __generic int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half3 __attribute__((overloadable)) __spirv_ocl_frexp( half3           x,
                                           __generic int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half4 __attribute__((overloadable)) __spirv_ocl_frexp( half4           x,
                                           __generic int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half8 __attribute__((overloadable)) __spirv_ocl_frexp( half8           x,
                                           __generic int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

INLINE half16 __attribute__((overloadable)) __spirv_ocl_frexp( half16           x,
                                              __generic int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ocl_frexp( double        x,
                                        __global int* exp )
{
    double temp;
    if( __spirv_IsNormal( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, 0.5, (long)(temp == 1.0) );
        temp = __spirv_ocl_copysign( temp, x );

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
        long lz = __spirv_ocl_clz( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       0.5, (long)(temp == 1.0) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 __attribute__((overloadable)) __spirv_ocl_frexp( double2        x,
                                             __global int2* exp )
{
    double2 temp;
    int2 exp_temp;
    int temp_ptr;
    double2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double3 __attribute__((overloadable)) __spirv_ocl_frexp( double3        x,
                                             __global int3* exp )
{
    double3 temp;
    int3 exp_temp;
    int temp_ptr;
    double3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double4 __attribute__((overloadable)) __spirv_ocl_frexp( double4        x,
                                             __global int4* exp )
{
    double4 temp;
    int4 exp_temp;
    int temp_ptr;
    double4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double8 __attribute__((overloadable)) __spirv_ocl_frexp( double8        x,
                                             __global int8* exp )
{
    double8 temp;
    int8 exp_temp;
    int temp_ptr;
    double8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double16 __attribute__((overloadable)) __spirv_ocl_frexp( double16        x,
                                                __global int16* exp )
{
    double16 temp;
    int16 exp_temp;
    int temp_ptr;
    double16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double __attribute__((overloadable)) __spirv_ocl_frexp( double         x,
                                        __private int* exp )
{
    double temp;
    if( __spirv_IsNormal( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, 0.5, (long)(temp == 1.0) );
        temp = __spirv_ocl_copysign( temp, x );

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
        long lz = __spirv_ocl_clz( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       0.5, (long)(temp == 1.0) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 __attribute__((overloadable)) __spirv_ocl_frexp( double2         x,
                                             __private int2* exp )
{
    double2 temp;
    int2 exp_temp;
    int temp_ptr;
    double2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double3 __attribute__((overloadable)) __spirv_ocl_frexp( double3         x,
                                             __private int3* exp )
{
    double3 temp;
    int3 exp_temp;
    int temp_ptr;
    double3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double4 __attribute__((overloadable)) __spirv_ocl_frexp( double4         x,
                                             __private int4* exp )
{
    double4 temp;
    int4 exp_temp;
    int temp_ptr;
    double4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double8 __attribute__((overloadable)) __spirv_ocl_frexp( double8         x,
                                             __private int8* exp )
{
    double8 temp;
    int8 exp_temp;
    int temp_ptr;
    double8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double16 __attribute__((overloadable)) __spirv_ocl_frexp( double16         x,
                                                __private int16* exp )
{
    double16 temp;
    int16 exp_temp;
    int temp_ptr;
    double16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double __attribute__((overloadable)) __spirv_ocl_frexp( double       x,
                                        __local int* exp )
{
    double temp;
    if( __spirv_IsNormal( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, 0.5, (long)(temp == 1.0) );
        temp = __spirv_ocl_copysign( temp, x );

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
        long lz = __spirv_ocl_clz( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       0.5, (long)(temp == 1.0) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 __attribute__((overloadable)) __spirv_ocl_frexp( double2       x,
                                             __local int2* exp )
{
    double2 temp;
    int2 exp_temp;
    int temp_ptr;
    double2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double3 __attribute__((overloadable)) __spirv_ocl_frexp( double3       x,
                                             __local int3* exp )
{
    double3 temp;
    int3 exp_temp;
    int temp_ptr;
    double3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double4 __attribute__((overloadable)) __spirv_ocl_frexp( double4       x,
                                             __local int4* exp )
{
    double4 temp;
    int4 exp_temp;
    int temp_ptr;
    double4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double8 __attribute__((overloadable)) __spirv_ocl_frexp( double8       x,
                                             __local int8* exp )
{
    double8 temp;
    int8 exp_temp;
    int temp_ptr;
    double8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double16 __attribute__((overloadable)) __spirv_ocl_frexp( double16       x,
                                                __local int16* exp )
{
    double16 temp;
    int16 exp_temp;
    int temp_ptr;
    double16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double __attribute__((overloadable)) __spirv_ocl_frexp( double       x,
                                        __generic int* exp )
{
    double temp;
    if( __spirv_IsNormal( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = __spirv_ocl_select( temp, 0.5, (long)(temp == 1.0) );
        temp = __spirv_ocl_copysign( temp, x );

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
        long lz = __spirv_ocl_clz( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = __spirv_ocl_select( temp,
                       0.5, (long)(temp == 1.0) );
        temp = __spirv_ocl_copysign( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 __attribute__((overloadable)) __spirv_ocl_frexp( double2       x,
                                             __generic int2* exp )
{
    double2 temp;
    int2 exp_temp;
    int temp_ptr;
    double2 in, out1;
    int2 out2;
    in = x;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double3 __attribute__((overloadable)) __spirv_ocl_frexp( double3       x,
                                             __generic int3* exp )
{
    double3 temp;
    int3 exp_temp;
    int temp_ptr;
    double3 in, out1;
    int3 out2;
    in = x;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double4 __attribute__((overloadable)) __spirv_ocl_frexp( double4       x,
                                             __generic int4* exp )
{
    double4 temp;
    int4 exp_temp;
    int temp_ptr;
    double4 in, out1;
    int4 out2;
    in = x;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double8 __attribute__((overloadable)) __spirv_ocl_frexp( double8       x,
                                             __generic int8* exp )
{
    double8 temp;
    int8 exp_temp;
    int temp_ptr;
    double8 in, out1;
    int8 out2;
    in = x;
    for(uint i = 0; i < 8; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

double16 __attribute__((overloadable)) __spirv_ocl_frexp( double16       x,
                                                __generic int16* exp )
{
    double16 temp;
    int16 exp_temp;
    int temp_ptr;
    double16 in, out1;
    int16 out2;
    in = x;
    for(uint i = 0; i < 16; i++)
    {
        out1[i] = __spirv_ocl_frexp( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp = out1;
    exp_temp = out2;
    *exp = exp_temp;
    return temp;
}

#endif // __OPENCL_C_VERSION__ >= CL_VERSION_2_0

#endif // defined(cl_khr_fp64)
