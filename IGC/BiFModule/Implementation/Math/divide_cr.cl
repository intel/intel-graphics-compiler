/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __builtin_spirv_divide_cr_f32_f32( float a,
                                         float b )
{
    return FDIV_IEEE(a, b);
}

INLINE
float2 __builtin_spirv_divide_cr_v2f32_v2f32( float2 a,
                                              float2 b )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in2[0] = b.s0;
    in2[1] = b.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE
float3 __builtin_spirv_divide_cr_v3f32_v3f32( float3 a,
                                              float3 b )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in1[2] = a.s2;
    in2[0] = b.s0;
    in2[1] = b.s1;
    in2[2] = b.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE
float4 __builtin_spirv_divide_cr_v4f32_v4f32( float4 a,
                                              float4 b )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in1[2] = a.s2;
    in1[3] = a.s3;
    in2[0] = b.s0;
    in2[1] = b.s1;
    in2[2] = b.s2;
    in2[3] = b.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE
float8 __builtin_spirv_divide_cr_v8f32_v8f32( float8 a,
                                              float8 b )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in1[2] = a.s2;
    in1[3] = a.s3;
    in1[4] = a.s4;
    in1[5] = a.s5;
    in1[6] = a.s6;
    in1[7] = a.s7;
    in2[0] = b.s0;
    in2[1] = b.s1;
    in2[2] = b.s2;
    in2[3] = b.s3;
    in2[4] = b.s4;
    in2[5] = b.s5;
    in2[6] = b.s6;
    in2[7] = b.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    return temp;
}

INLINE
float16 __builtin_spirv_divide_cr_v16f32_v16f32( float16 a,
                                                 float16 b )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    in1[0] = a.s0;
    in1[1] = a.s1;
    in1[2] = a.s2;
    in1[3] = a.s3;
    in1[4] = a.s4;
    in1[5] = a.s5;
    in1[6] = a.s6;
    in1[7] = a.s7;
    in1[8] = a.s8;
    in1[9] = a.s9;
    in1[10] = a.sa;
    in1[11] = a.sb;
    in1[12] = a.sc;
    in1[13] = a.sd;
    in1[14] = a.se;
    in1[15] = a.sf;
    in2[0] = b.s0;
    in2[1] = b.s1;
    in2[2] = b.s2;
    in2[3] = b.s3;
    in2[4] = b.s4;
    in2[5] = b.s5;
    in2[6] = b.s6;
    in2[7] = b.s7;
    in2[8] = b.s8;
    in2[9] = b.s9;
    in2[10] = b.sa;
    in2[11] = b.sb;
    in2[12] = b.sc;
    in2[13] = b.sd;
    in2[14] = b.se;
    in2[15] = b.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = __builtin_spirv_divide_cr_f32_f32(in1[i], in2[i]);
    }
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    temp.s4 = out[4];
    temp.s5 = out[5];
    temp.s6 = out[6];
    temp.s7 = out[7];
    temp.s8 = out[8];
    temp.s9 = out[9];
    temp.sa = out[10];
    temp.sb = out[11];
    temp.sc = out[12];
    temp.sd = out[13];
    temp.se = out[14];
    temp.sf = out[15];
    return temp;
}


#if defined(cl_khr_fp64)
double __builtin_spirv_divide_cr_f64_f64( double a,
                                          double b )
{
    return FDIV_IEEE_DOUBLE(a, b);
}

#endif //defined(cl_khr_fp64)


