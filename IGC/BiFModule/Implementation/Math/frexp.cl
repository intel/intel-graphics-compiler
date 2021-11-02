/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f32_p1i32, )( float         x,
                                       __global int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )( temp, x );

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
        int lz = SPIRV_OCL_BUILTIN(clz, _i32, )( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f32_p1v2i32, )( float2         x,
                                            __global int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f32_p1v3i32, )( float3         x,
                                            __global int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f32_p1v4i32, )( float4         x,
                                            __global int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f32_p1v8i32, )( float8         x,
                                            __global int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float in[8], out1[8];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f32_p1v16i32, )( float16         x,
                                               __global int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float in[16], out1[16];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( float          x,
                                       __private int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )( temp, x );

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
        int lz = SPIRV_OCL_BUILTIN(clz, _i32, )( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f32_p0v2i32, )( float2          x,
                                            __private int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f32_p0v3i32, )( float3          x,
                                            __private int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f32_p0v4i32, )( float4          x,
                                            __private int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f32_p0v8i32, )( float8          x,
                                            __private int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float in[8], out1[8];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f32_p0v16i32, )( float16          x,
                                               __private int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float in[16], out1[16];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f32_p3i32, )( float        x,
                                       __local int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )( temp, x );

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
        int lz = SPIRV_OCL_BUILTIN(clz, _i32, )( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}


INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f32_p3v2i32, )( float2        x,
                                            __local int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f32_p3v3i32, )( float3        x,
                                            __local int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f32_p3v4i32, )( float4        x,
                                            __local int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f32_p3v8i32, )( float8        x,
                                            __local int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float in[8], out1[8];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f32_p3v16i32, )( float16        x,
                                               __local int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float in[16], out1[16];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p0i32, )( in[i], (__private int*)&temp_ptr );
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

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f32_p4i32, )( float          x,
                                       __generic int* exp )
{
    float temp;
    if( __intel_relaxed_isnormal( x ) )
    {
        temp = as_float( (int)(( as_int( x ) & FLOAT_MANTISSA_MASK ) + FLOAT_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp, (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )( temp, x );

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
        int lz = SPIRV_OCL_BUILTIN(clz, _i32, )( m );
        int non_mantissa_bits = FLOAT_BITS - FLOAT_MANTISSA_BITS;
        temp = as_float( (int)(( ( as_int( x ) << ( lz - (non_mantissa_bits - (int)(1)) ) ) & FLOAT_MANTISSA_MASK ))  );
        temp = as_float( (int)(( as_int( temp ) + FLOAT_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )( temp,
                       (float)(0.5f), (int)(temp == (float)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )( temp,
                         x );

        *exp = ( ( (as_int( x ) & FLOAT_EXPONENT_MASK ) >> FLOAT_MANTISSA_BITS ) - ( FLOAT_BIAS - (int)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f32_p4v2i32, )( float2          x,
                                            __generic int2* exp )
{
    float2 temp;
    int2 exp_temp;
    int temp_ptr;
    float in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p4i32, )( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f32_p4v3i32, )( float3          x,
                                            __generic int3* exp )
{
    float3 temp;
    int3 exp_temp;
    int temp_ptr;
    float in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f32_p4v4i32, )( float4          x,
                                            __generic int4* exp )
{
    float4 temp;
    int4 exp_temp;
    int temp_ptr;
    float in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f32_p4v8i32, )( float8          x,
                                            __generic int8* exp )
{
    float8 temp;
    int8 exp_temp;
    int temp_ptr;
    float in[8], out1[8];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f32_p4v16i32, )( float16          x,
                                               __generic int16* exp )
{
    float16 temp;
    int16 exp_temp;
    int temp_ptr;
    float in[16], out1[16];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f32_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16

half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f16_p1i32, )( half          x,
                                      __global int* exp )
{
    half temp;
    if( SPIRV_BUILTIN(IsNormal, _f16, )( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f16_f16, )( temp, x );

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
        short lz = SPIRV_OCL_BUILTIN(clz, _i16, )( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f16_f16, )( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f16_p1v2i32, )( half2          x,
                                           __global int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f16_p1v3i32, )( half3          x,
                                           __global int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f16_p1v4i32, )( half4          x,
                                           __global int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f16_p1v8i32, )( half8          x,
                                           __global int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half in[8], out1[8];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f16_p1v16i32, )( half16          x,
                                              __global int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half in[16], out1[16];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( half           x,
                                      __private int* exp )
{
    half temp;
    if( SPIRV_BUILTIN(IsNormal, _f16, )( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f16_f16, )( temp, x );

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
        short lz = SPIRV_OCL_BUILTIN(clz, _i16, )( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f16_f16, )( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f16_p0v2i32, )( half2           x,
                                           __private int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f16_p0v3i32, )( half3           x,
                                           __private int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f16_p0v4i32, )( half4           x,
                                           __private int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f16_p0v8i32, )( half8           x,
                                           __private int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half in[8], out1[8];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f16_p0v16i32, )( half16           x,
                                              __private int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half in[16], out1[16];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f16_p3i32, )( half         x,
                                      __local int* exp )
{
    half temp;
    if( SPIRV_BUILTIN(IsNormal, _f16, )( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f16_f16, )( temp, x );

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
        short lz = SPIRV_OCL_BUILTIN(clz, _i16, )( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f16_f16, )( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f16_p3v2i32, )( half2         x,
                                           __local int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f16_p3v3i32, )( half3         x,
                                           __local int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f16_p3v4i32, )( half4         x,
                                           __local int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f16_p3v8i32, )( half8         x,
                                           __local int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half in[8], out1[8];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f16_p3v16i32, )( half16         x,
                                              __local int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half in[16], out1[16];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p0i32, )( in[i], (__private int*)&temp_ptr );
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
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f16_p4i32, )( half           x,
                                      __generic int* exp )
{
    half temp;
    if( SPIRV_BUILTIN(IsNormal, _f16, )( x ) )
    {
        temp = as_half( (short)(( as_short( x ) & HALF_MANTISSA_MASK ) + HALF_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp, (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f16_f16, )( temp, x );

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
        short lz = SPIRV_OCL_BUILTIN(clz, _i16, )( m );
        short non_mantissa_bits = HALF_BITS - HALF_MANTISSA_BITS;
        temp = as_half( (short)(( ( as_short( x ) << ( lz - (non_mantissa_bits - (short)(1)) ) ) & HALF_MANTISSA_MASK ))  );
        temp = as_half( (short)(( as_short( temp ) + HALF_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f16_f16_i16, )( temp,
                       (half)(0.5f), (short)(temp == (half)(1.0f)) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f16_f16, )( temp,
                         x );

        *exp = ( ( (as_short( x ) & HALF_EXPONENT_MASK ) >> HALF_MANTISSA_BITS ) - ( HALF_BIAS - (short)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f16_p4v2i32, )( half2           x,
                                           __generic int2* exp )
{
    half2 temp;
    int2 exp_temp;
    int temp_ptr;
    half in[2], out1[2];
    int out2[2];
    in[0] = x.s0;
    in[1] = x.s1;
    for(uint i = 0; i < 2; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p4i32, )( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f16_p4v3i32, )( half3           x,
                                           __generic int3* exp )
{
    half3 temp;
    int3 exp_temp;
    int temp_ptr;
    half in[3], out1[3];
    int out2[3];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    for(uint i = 0; i < 3; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f16_p4v4i32, )( half4           x,
                                           __generic int4* exp )
{
    half4 temp;
    int4 exp_temp;
    int temp_ptr;
    half in[4], out1[4];
    int out2[4];
    in[0] = x.s0;
    in[1] = x.s1;
    in[2] = x.s2;
    in[3] = x.s3;
    for(uint i = 0; i < 4; i++)
    {
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f16_p4v8i32, )( half8           x,
                                           __generic int8* exp )
{
    half8 temp;
    int8 exp_temp;
    int temp_ptr;
    half in[8], out1[8];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f16_p4v16i32, )( half16           x,
                                              __generic int16* exp )
{
    half16 temp;
    int16 exp_temp;
    int temp_ptr;
    half in[16], out1[16];
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f16_p4i32, )( in[i], (__generic int*)&temp_ptr );
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
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f64_p1i32, )( double        x,
                                        __global int* exp )
{
    double temp;
    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp, 0.5, (long)(temp == 1.0) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )( temp, x );

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
        long lz = SPIRV_OCL_BUILTIN(clz, _i64, )( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp,
                       0.5, (long)(temp == 1.0) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f64_p1v2i32, )( double2        x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f64_p1v3i32, )( double3        x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f64_p1v4i32, )( double4        x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f64_p1v8i32, )( double8        x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f64_p1v16i32, )( double16        x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( double         x,
                                        __private int* exp )
{
    double temp;
    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp, 0.5, (long)(temp == 1.0) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )( temp, x );

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
        long lz = SPIRV_OCL_BUILTIN(clz, _i64, )( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp,
                       0.5, (long)(temp == 1.0) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f64_p0v2i32, )( double2         x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f64_p0v3i32, )( double3         x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f64_p0v4i32, )( double4         x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f64_p0v8i32, )( double8         x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f64_p0v16i32, )( double16         x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f64_p3i32, )( double       x,
                                        __local int* exp )
{
    double temp;
    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp, 0.5, (long)(temp == 1.0) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )( temp, x );

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
        long lz = SPIRV_OCL_BUILTIN(clz, _i64, )( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp,
                       0.5, (long)(temp == 1.0) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f64_p3v2i32, )( double2       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f64_p3v3i32, )( double3       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f64_p3v4i32, )( double4       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f64_p3v8i32, )( double8       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f64_p3v16i32, )( double16       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p0i32, )( in[i], (__private int*)&temp_ptr );
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

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _f64_p4i32, )( double       x,
                                        __generic int* exp )
{
    double temp;
    if( SPIRV_BUILTIN(IsNormal, _f64, )( x ) )
    {
        temp = as_double( (long)(( as_long( x ) & DOUBLE_MANTISSA_MASK ) + DOUBLE_NEG_ONE_EXP_MASK) );
        temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp, 0.5, (long)(temp == 1.0) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )( temp, x );

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
        long lz = SPIRV_OCL_BUILTIN(clz, _i64, )( m );
        long non_mantissa_bits = DOUBLE_BITS - DOUBLE_MANTISSA_BITS;
        temp = as_double( (long)(( ( as_long( x ) << ( lz - (non_mantissa_bits - (long)(1)) ) ) & DOUBLE_MANTISSA_MASK ))  );
        temp = as_double( (long)(( as_long( temp ) + DOUBLE_NEG_ONE_EXP_MASK )) );
        temp = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )( temp,
                       0.5, (long)(temp == 1.0) );
        temp = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )( temp,
                         x );

        *exp = ( ( (as_long( x ) & DOUBLE_EXPONENT_MASK ) >> DOUBLE_MANTISSA_BITS ) - ( DOUBLE_BIAS - (long)(1) ) ) - ( lz - non_mantissa_bits );
    }
    return temp;
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v2f64_p4v2i32, )( double2       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p4i32, )( in[i], (__generic int*)&temp_ptr );
        out2[i] = temp_ptr;
    }
    temp.s0 = out1[0];
    temp.s1 = out1[1];
    exp_temp.s0 = out2[0];
    exp_temp.s1 = out2[1];
    *exp = exp_temp;
    return temp;
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v3f64_p4v3i32, )( double3       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v4f64_p4v4i32, )( double4       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v8f64_p4v8i32, )( double8       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(frexp, _v16f64_p4v16i32, )( double16       x,
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
        out1[i] = SPIRV_OCL_BUILTIN(frexp, _f64_p4i32, )( in[i], (__generic int*)&temp_ptr );
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

#endif // defined(cl_khr_fp64)
