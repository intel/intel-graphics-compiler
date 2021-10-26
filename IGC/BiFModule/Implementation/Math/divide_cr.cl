/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float __builtin_spirv_divide_cr_f32_f32( float a,
                                         float b )
{
    if (!__CRMacros) {
        typedef union binary32
        {
            uint u;
            int  s;
            float f;
        } binary32;

        binary32 fa, fb, s, y0, y1, q0, q1, q, r0, r1, e0, norm2sub;
        int aExp, bExp, scale, flag = 0;
        norm2sub.u = 0x00800000;
        fa.f = a;
        fb.f = b;
        aExp = (fa.u >> 23) & 0xff;
        bExp = (fb.u >> 23) & 0xff;

        // Normalize denormals -- scale by 2^32
        if(!(fa.u & 0x7f800000)) {
            binary32 x; x.u = 0x4f800000;
            fa.f *= x.f;
            aExp = ((fa.u >> 23) & 0xff) - 32;
        }
        if(!(fb.u & 0x7f800000)) {
            binary32 x; x.u = 0x4f800000;
            fb.f *= x.f;
            bExp = ((fb.u >> 23) & 0xff) - 32;
        }

        if ((fa.f != 0.0f) && (aExp != 0xff)) {
            // a is NOT 0 or INF or NAN
            flag = 1;
            // Scale a to [1,2)
            fa.u = (fa.u & 0x807fffff) | 0x3f800000;
        }
        // Initial approximation of 1/b
        if (fb.f == 0.0f) {
            // b = 0, y0 = INF
            flag = 0;
            y0.u  =  (fb.u & 0x80000000) | 0x7f800000;
        } else if (bExp == 0xff) {
            flag = 0;
            if ((fb.u & 0x7fffff) == 0) {
                // b = INF, y0 = 0
                y0.u = (fb.u & 0x80000000);
            } else {
                // b = NaN
                y0.u = fb.u;
            }
        } else {
            // b is not 0 or INF or NAN
            flag &= 1;
            // Scale b to [1,2)
            fb.u  = (fb.u & 0x807fffff) | 0x3f800000;
            y0.f = SPIRV_OCL_BUILTIN(native_recip, _f32, )(fb.f);
            //printf("y0=0x%08x=%a\n", y0.u, y0.f);
        }
        if (flag) {
            scale =  aExp - bExp;
            //printf("a=0x%08x\n", fa.u);
            //printf("b=0x%08x\n", fb.u);
            // Step(1), q0=a*y0
            q0.f = fa.f * y0.f;
            //printf("q0=0x%08x=%a\n", q0.u, q0.f);
            // Step(2), e0=(1-b*y0)
            e0.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-fb.f, y0.f, 1.0f);
            //printf("e0=0x%08x=%a\n", e0.u, e0.f);
            // Step(3), y1=y0+e0*y0
            y1.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(e0.f, y0.f, y0.f);
            //printf("y1=0x%08x=%a\n", y1.u, y1.f);
            // Step(4), r0=a-b*q0
            r0.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-fb.f, q0.f, fa.f);
            //printf("r0=0x%08x=%a\n", r0.u, r0.f);
            // Step(5), q1=q0+r0*y1
            q1.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(r0.f, y1.f, q0.f);
            //printf("q1=0x%08x=%a\n", q1.u, q1.f);
            // Step(6), r1=a-b*q1
            r1.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(-fb.f, q1.f, fa.f);
            //printf("r1=0x%08x=%a\n", r1.u, r1.f);
            // Step(7), q=q1+r1*y1, set user rounding mode here
            q.f = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(r1.f, y1.f, q1.f);
            //printf("q=0x%08x=%a\n", q.u, q.f);
            // Scale so that 1<= q < 4
            //q.f = q.f * 2;
            //printf("q=0x%08x\n", q.u);
            scale += 0x7e;
            if (scale <= 0) {
                // Underflow
                scale += 0x7f;
                // scale will always be positive now
                s.u = (scale << 23);
                s.f = s.f * norm2sub.f;
            } else if (scale >= 0xff) {
                // Overflow, this is OK since q >= 1
                s.u = 0x7f800000;
                q.f = q.f * 2;
            } else {
                // Normal
                s.u = (scale << 23);
                q.f = q.f * 2;
            }
            //printf("s=0x%08x\n", s.u);
            q.f = q.f * s.f;
            //printf("q=0x%08x\n", q.u);
        } else {
            //printf("fa=0x%08x\n", fa.u);
            //printf("y0=0x%08x\n", y0.u);
            q.f = fa.f * y0.f;
        }
        return q.f;
    } else {
        return FDIV_IEEE(a, b);
    }
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


