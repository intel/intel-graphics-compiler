/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../include/BiF_Definitions.cl"
#include "../../Headers/spirv.h"

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f32_f32_p1i32, )( float         xx,
                                            float         yy,
                                            __global int* quo )
{
    float result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0f) )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )(SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(0.0f, xx), xx, __intel_relaxed_isinf(yy));
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f32, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f32, )(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(0.0f, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f32, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f32, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        float x = SPIRV_OCL_BUILTIN(fabs, _f32, )(xx);
        float y = SPIRV_OCL_BUILTIN(fabs, _f32, )(yy);
        int ex, ey;
        ex = SPIRV_OCL_BUILTIN(ilogb, _f32, )( x );
        ey = SPIRV_OCL_BUILTIN(ilogb, _f32, )( y );
        float xr = x;
        float yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(xr, ex - ey);
          }
        }
        if( (yr < 2.0f*xr) | ( (yr == 2.0f*xr) & (q & 0x00000001) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(xr, ey);
        }

        int qout = q & 0x0000007f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0f )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f32_v2f32_p1v2i32, )( float2         xx,
                                                   float2         yy,
                                                   __global int2* quo )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f32_v3f32_p1v3i32, )( float3         xx,
                                                   float3         yy,
                                                   __global int3* quo )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f32_v4f32_p1v4i32, )( float4         xx,
                                                   float4         yy,
                                                   __global int4* quo )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f32_v8f32_p1v8i32, )( float8         xx,
                                                   float8         yy,
                                                   __global int8* quo )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f32_v16f32_p1v16i32, )( float16         xx,
                                                       float16         yy,
                                                       __global int16* quo )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )( float          xx,
                                            float          yy,
                                            __private int* quo )
{
    float result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0f) )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )(SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(0.0f, xx), xx, __intel_relaxed_isinf(yy));
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f32, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f32, )(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(0.0f, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f32, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f32, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        float x = SPIRV_OCL_BUILTIN(fabs, _f32, )(xx);
        float y = SPIRV_OCL_BUILTIN(fabs, _f32, )(yy);
        int ex, ey;
        ex = SPIRV_OCL_BUILTIN(ilogb, _f32, )( x );
        ey = SPIRV_OCL_BUILTIN(ilogb, _f32, )( y );
        float xr = x;
        float yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(xr, ex - ey);
          }
        }
        if( (yr < 2.0f*xr) | ( (yr == 2.0f*xr) && (q & 0x00000001) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(xr, ey);
        }

        int qout = q & 0x0000007f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0f )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f32_v2f32_p0v2i32, )( float2          xx,
                                                   float2          yy,
                                                   __private int2* quo )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f32_v3f32_p0v3i32, )( float3          xx,
                                                   float3          yy,
                                                   __private int3* quo )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f32_v4f32_p0v4i32, )( float4          xx,
                                                   float4          yy,
                                                   __private int4* quo )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f32_v8f32_p0v8i32, )( float8          xx,
                                                   float8          yy,
                                                   __private int8* quo )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f32_v16f32_p0v16i32, )( float16          xx,
                                                       float16          yy,
                                                       __private int16* quo )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f32_f32_p3i32, )( float        xx,
                                            float        yy,
                                            __local int* quo )
{
    float result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0f) )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )(SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(0.0f, xx), xx, __intel_relaxed_isinf(yy));
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f32, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f32, )(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(0.0f, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f32, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f32, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        float x = SPIRV_OCL_BUILTIN(fabs, _f32, )(xx);
        float y = SPIRV_OCL_BUILTIN(fabs, _f32, )(yy);
        int ex, ey;
        ex = SPIRV_OCL_BUILTIN(ilogb, _f32, )( x );
        ey = SPIRV_OCL_BUILTIN(ilogb, _f32, )( y );
        float xr = x;
        float yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(xr, ex - ey);
          }
        }
        if( (yr < 2.0f*xr) | ( (yr == 2.0f*xr) && (q & 0x00000001) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(xr, ey);
        }

        int qout = q & 0x0000007f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0f )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f32_v2f32_p3v2i32, )( float2        xx,
                                                   float2        yy,
                                                   __local int2* quo )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f32_v3f32_p3v3i32, )( float3        xx,
                                                   float3        yy,
                                                   __local int3* quo )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f32_v4f32_p3v4i32, )( float4        xx,
                                                   float4        yy,
                                                   __local int4* quo )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f32_v8f32_p3v8i32, )( float8        xx,
                                                   float8        yy,
                                                   __local int8* quo )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f32_v16f32_p3v16i32, )( float16        xx,
                                                       float16        yy,
                                                       __local int16* quo )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE float SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f32_f32_p4i32, )( float          xx,
                                            float          yy,
                                            __generic int* quo )
{
    float result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0f )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0f) )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )(SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(0.0f, xx), xx, __intel_relaxed_isinf(yy));
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f32, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f32, )(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = SPIRV_OCL_BUILTIN(copysign, _f32_f32, )(0.0f, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f32, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f32, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        float x = SPIRV_OCL_BUILTIN(fabs, _f32, )(xx);
        float y = SPIRV_OCL_BUILTIN(fabs, _f32, )(yy);
        int ex, ey;
        ex = SPIRV_OCL_BUILTIN(ilogb, _f32, )( x );
        ey = SPIRV_OCL_BUILTIN(ilogb, _f32, )( y );
        float xr = x;
        float yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(xr, ex - ey);
          }
        }
        if( (yr < 2.0f*xr) | ( (yr == 2.0f*xr) && (q & 0x00000001) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(xr, ey);
        }

        int qout = q & 0x0000007f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0f )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

INLINE float2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f32_v2f32_p4v2i32, )( float2          xx,
                                                   float2          yy,
                                                   __generic int2* quo )
{
    float2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE float3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f32_v3f32_p4v3i32, )( float3          xx,
                                                   float3          yy,
                                                   __generic int3* quo )
{
    float3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE float4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f32_v4f32_p4v4i32, )( float4          xx,
                                                   float4          yy,
                                                   __generic int4* quo )
{
    float4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE float8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f32_v8f32_p4v8i32, )( float8          xx,
                                                   float8          yy,
                                                   __generic int8* quo )
{
    float8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

INLINE float16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f32_v16f32_p4v16i32, )( float16          xx,
                                                       float16          yy,
                                                       __generic int16* quo )
{
    float16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

#endif //#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#ifdef cl_khr_fp16
INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f16_f16_p1i32, )( half          xx,
                                           half          yy,
                                           __global int* quo )
{
    float result;
    int   quoVal;
    result = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )((float)xx, (float)yy, (__private int*)&quoVal);
    *quo = (half)quoVal;
    return (half)result;
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f16_v2f16_p1v2i32, )( half2          xx,
                                                  half2          yy,
                                                  __global int2* quo )
{
    half2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f16_v3f16_p1v3i32, )( half3          xx,
                                                  half3          yy,
                                                  __global int3* quo )
{
    half3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f16_v4f16_p1v4i32, )( half4          xx,
                                                  half4          yy,
                                                  __global int4* quo )
{
    half4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f16_v8f16_p1v8i32, )( half8          xx,
                                                  half8          yy,
                                                  __global int8* quo )
{
    half8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f16_v16f16_p1v16i32, )( half16          xx,
                                                      half16          yy,
                                                      __global int16* quo )
{
    half16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )( half           xx,
                                           half           yy,
                                           __private int* quo )
{
    float result;
    int   quoVal;
    result = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )((float)xx, (float)yy, (__private int*)&quoVal);
    *quo = (half)quoVal;
    return (half)result;
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f16_v2f16_p0v2i32, )( half2           xx,
                                                  half2           yy,
                                                  __private int2* quo )
{
    half2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f16_v3f16_p0v3i32, )( half3           xx,
                                                  half3           yy,
                                                  __private int3* quo )
{
    half3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f16_v4f16_p0v4i32, )( half4           xx,
                                                  half4           yy,
                                                  __private int4* quo )
{
    half4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f16_v8f16_p0v8i32, )( half8           xx,
                                                  half8           yy,
                                                  __private int8* quo )
{
    half8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f16_v16f16_p0v16i32, )( half16           xx,
                                                      half16           yy,
                                                      __private int16* quo )
{
    half16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f16_f16_p3i32, )( half         xx,
                                           half         yy,
                                           __local int* quo )
{
    float result;
    int   quoVal;
    result = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p0i32, )((float)xx, (float)yy, (__private int*)&quoVal);
    *quo = (half)quoVal;
    return (half)result;
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f16_v2f16_p3v2i32, )( half2         xx,
                                                  half2         yy,
                                                  __local int2* quo )
{
    half2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f16_v3f16_p3v3i32, )( half3         xx,
                                                  half3         yy,
                                                  __local int3* quo )
{
    half3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f16_v4f16_p3v4i32, )( half4         xx,
                                                  half4         yy,
                                                  __local int4* quo )
{
    half4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f16_v8f16_p3v8i32, )( half8         xx,
                                                  half8         yy,
                                                  __local int8* quo )
{
    half8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f16_v16f16_p3v16i32, )( half16         xx,
                                                      half16         yy,
                                                      __local int16* quo )
{
    half16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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
#endif


#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

INLINE half SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f16_f16_p4i32, )( half           xx,
                                           half           yy,
                                           __generic int* quo )
{
    float result;
    int   quoVal;
    result = SPIRV_OCL_BUILTIN(remquo, _f32_f32_p4i32, )((float)xx, (float)yy, (__generic int*)&quoVal);
    *quo = (half)quoVal;
    return (half)result;
}

INLINE half2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f16_v2f16_p4v2i32, )( half2           xx,
                                                  half2           yy,
                                                  __generic int2* quo )
{
    half2 temp;
    float in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

INLINE half3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f16_v3f16_p4v3i32, )( half3           xx,
                                                  half3           yy,
                                                  __generic int3* quo )
{
    half3 temp;
    float in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

INLINE half4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f16_v4f16_p4v4i32, )( half4           xx,
                                                  half4           yy,
                                                  __generic int4* quo )
{
    half4 temp;
    float in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

INLINE half8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f16_v8f16_p4v8i32, )( half8           xx,
                                                  half8           yy,
                                                  __generic int8* quo )
{
    half8 temp;
    float in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

INLINE half16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f16_v16f16_p4v16i32, )( half16           xx,
                                                      half16           yy,
                                                      __generic int16* quo )
{
    half16 temp;
    float in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f16_f16_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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
#endif //#if defined(cl_khr_fp16) && (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f64_f64_p1i32, )( double        xx,
                                             double        yy,
                                             __global int* quo )
{
    double result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0 )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(nan, _i64, )(0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0) )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )(SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, xx), xx, (long)__intel_relaxed_isinf(yy));
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f64, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f64, )(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f64, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f64, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        double x = SPIRV_OCL_BUILTIN(fabs, _f64, )(xx);
        double y = SPIRV_OCL_BUILTIN(fabs, _f64, )(yy);
        int ex, ey;
        ex = SPIRV_OCL_BUILTIN(ilogb, _f64, )( x );
        ey = SPIRV_OCL_BUILTIN(ilogb, _f64, )( y );
        double xr = x;
        double yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(xr, ex - ey);
          }
        }
        if( (yr < 2.0*xr) | ( (yr == 2.0*xr) & (q & 0x1) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(xr, ey);
        }

        uint qout = q & 0x7f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0 )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f64_v2f64_p1v2i32, )( double2         xx,
                                                    double2         yy,
                                                    __global int2*  quo )
{
    double2 temp;
    double in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f64_v3f64_p1v3i32, )( double3        xx,
                                                    double3        yy,
                                                    __global int3* quo )
{
    double3 temp;
    double in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f64_v4f64_p1v4i32, )( double4        xx,
                                                    double4        yy,
                                                    __global int4* quo )
{
    double4 temp;
    double in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f64_v8f64_p1v8i32, )( double8        xx,
                                                    double8        yy,
                                                    __global int8* quo )
{
    double8 temp;
    double in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f64_v16f64_p1v16i32, )( double16        xx,
                                                        double16        yy,
                                                        __global int16* quo )
{
    double16 temp;
    double in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )( double         xx,
                                             double         yy,
                                             __private int* quo )
{
    double result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0 )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(nan, _i64, )(0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0) )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )(SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, xx), xx, (long)__intel_relaxed_isinf(yy));
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f64, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f64, )(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f64, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f64, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        double x = SPIRV_OCL_BUILTIN(fabs, _f64, )(xx);
        double y = SPIRV_OCL_BUILTIN(fabs, _f64, )(yy);
        int ex, ey;
        ex = SPIRV_OCL_BUILTIN(ilogb, _f64, )( x );
        ey = SPIRV_OCL_BUILTIN(ilogb, _f64, )( y );
        double xr = x;
        double yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(xr, ex - ey);
          }
        }
        if( (yr < 2.0*xr) | ( (yr == 2.0*xr) && (q & 0x1) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(xr, ey);
        }

        int qout = q & 0x7f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0f )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f64_v2f64_p0v2i32, )( double2         xx,
                                                    double2         yy,
                                                    __private int2* quo )
{
    double2 temp;
    double in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f64_v3f64_p0v3i32, )( double3         xx,
                                                    double3         yy,
                                                    __private int3* quo )
{
    double3 temp;
    double in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f64_v4f64_p0v4i32, )( double4         xx,
                                                    double4         yy,
                                                    __private int4* quo )
{
    double4 temp;
    double in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f64_v8f64_p0v8i32, )( double8         xx,
                                                    double8         yy,
                                                    __private int8* quo )
{
    double8 temp;
    double in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f64_v16f64_p0v16i32, )( double16         xx,
                                                        double16         yy,
                                                        __private int16* quo )
{
    double16 temp;
    double in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f64_f64_p3i32, )( double       xx,
                                             double       yy,
                                             __local int* quo )
{
    double result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0 )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0) )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )(SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, xx), xx, (long)__intel_relaxed_isinf(yy));
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f64, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f64, )(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f64, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f64, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        double x = SPIRV_OCL_BUILTIN(fabs, _f64, )(xx);
        double y = SPIRV_OCL_BUILTIN(fabs, _f64, )(yy);
        int ex, ey;
        ex = SPIRV_OCL_BUILTIN(ilogb, _f64, )( x );
        ey = SPIRV_OCL_BUILTIN(ilogb, _f64, )( y );
        double xr = x;
        double yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(xr, ex - ey);
          }
        }
        if( (yr < 2.0*xr) | ( (yr == 2.0*xr) && (q & 0x1) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(xr, ey);
        }

        int qout = q & 0x7f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0 )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f64_v2f64_p3v2i32, )( double2       xx,
                                                    double2       yy,
                                                    __local int2* quo )
{
    double2 temp;
    double in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f64_v3f64_p3v3i32, )( double3       xx,
                                                    double3       yy,
                                                    __local int3* quo )
{
    double3 temp;
    double in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f64_v4f64_p3v4i32, )( double4       xx,
                                                    double4       yy,
                                                    __local int4* quo )
{
    double4 temp;
    double in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f64_v8f64_p3v8i32, )( double8       xx,
                                                    double8       yy,
                                                    __local int8* quo )
{
    double8 temp;
    double in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f64_v16f64_p3v16i32, )( double16       xx,
                                                        double16       yy,
                                                        __local int16* quo )
{
    double16 temp;
    double in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p0i32, )(in1[i], in2[i], (__private int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

double SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _f64_f64_p4i32, )( double         xx,
                                             double         yy,
                                             __generic int* quo )
{
    double result;

    if( __intel_relaxed_isnan(xx) |
        __intel_relaxed_isnan(yy) |
        __intel_relaxed_isinf(xx) |
        yy == 0.0 )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(nan, _i32, )(0);
    }
    else if( __intel_relaxed_isinf(yy) | (xx == 0.0) )
    {
        *quo = 0;
        result = SPIRV_OCL_BUILTIN(select, _f64_f64_i64, )(SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, xx), xx, (long)__intel_relaxed_isinf(yy));
    }
    else if( SPIRV_OCL_BUILTIN(fabs, _f64, )(xx) == SPIRV_OCL_BUILTIN(fabs, _f64, )(yy) )
    {
        *quo = (xx == yy) ? 1 : -1;
        result = SPIRV_OCL_BUILTIN(copysign, _f64_f64, )(0.0, xx);
    }
    else
    {
        int signx = SPIRV_BUILTIN(SignBitSet, _f64, )( xx ) ? -1 : 1;
        int signy = SPIRV_BUILTIN(SignBitSet, _f64, )( yy ) ? -1 : 1;
        int signn = (signx == signy) ? 1 : -1;
        double x = SPIRV_OCL_BUILTIN(fabs, _f64, )(xx);
        double y = SPIRV_OCL_BUILTIN(fabs, _f64, )(yy);
        int ex, ey;
        ex = SPIRV_OCL_BUILTIN(ilogb, _f64, )( x );
        ey = SPIRV_OCL_BUILTIN(ilogb, _f64, )( y );
        double xr = x;
        double yr = y;
        uint q = 0;
        if(ex-ey >= -1)
        {
            yr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( y, -ey );
            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )( x, -ex );
            if(ex-ey >= 0)
            {
              int i;
              for(i = ex - ey; i > 0; i--)
              {
                q = q << 1;
                if(xr >= yr)
                {
                    xr = xr - yr;
                    q = q + 1;
                }
                xr = xr + xr;
            }
            q = q << 1;
            if( xr > yr )
            {
                xr = xr - yr;
                q = q + 1;
            }
          }
          else
          {
              xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(xr, ex - ey);
          }
        }
        if( (yr < 2.0*xr) | ( (yr == 2.0*xr) && (q & 0x1) ) )
        {
            xr = xr - yr;
            q = q + 1;
        }
        if (ex - ey >= -1)
        {
            xr = SPIRV_OCL_BUILTIN(ldexp, _f64_i32, )(xr, ey);
        }

        int qout = q & 0x7f;

        if( signn < 0)
        {
            qout = -qout;
        }
        if( xx < 0.0 )
        {
            xr = -xr;
        }

        *quo = qout;
        result = xr;
    }

    return result;
}

double2 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v2f64_v2f64_p4v2i32, )( double2         xx,
                                                    double2         yy,
                                                    __generic int2* quo )
{
    double2 temp;
    double in1[2], in2[2], out[2];
    int out2[2];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    for(uint i = 0; i < 2; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    temp.s0 = out[0];
    temp.s1 = out[1];
    return temp;
}

double3 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v3f64_v3f64_p4v3i32, )( double3         xx,
                                                    double3         yy,
                                                    __generic int3* quo )
{
    double3 temp;
    double in1[3], in2[3], out[3];
    int out2[3];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    for(uint i = 0; i < 3; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    return temp;
}

double4 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v4f64_v4f64_p4v4i32, )( double4         xx,
                                                    double4         yy,
                                                    __generic int4* quo )
{
    double4 temp;
    double in1[4], in2[4], out[4];
    int out2[4];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    for(uint i = 0; i < 4; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    temp.s0 = out[0];
    temp.s1 = out[1];
    temp.s2 = out[2];
    temp.s3 = out[3];
    return temp;
}

double8 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v8f64_v8f64_p4v8i32, )( double8         xx,
                                                    double8         yy,
                                                    __generic int8* quo )
{
    double8 temp;
    double in1[8], in2[8], out[8];
    int out2[8];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    for(uint i = 0; i < 8; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
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

double16 SPIRV_OVERLOADABLE SPIRV_OCL_BUILTIN(remquo, _v16f64_v16f64_p4v16i32, )( double16         xx,
                                                        double16         yy,
                                                        __generic int16* quo )
{
    double16 temp;
    double in1[16], in2[16], out[16];
    int out2[16];
    in1[0] = xx.s0;
    in1[1] = xx.s1;
    in1[2] = xx.s2;
    in1[3] = xx.s3;
    in1[4] = xx.s4;
    in1[5] = xx.s5;
    in1[6] = xx.s6;
    in1[7] = xx.s7;
    in1[8] = xx.s8;
    in1[9] = xx.s9;
    in1[10] = xx.sa;
    in1[11] = xx.sb;
    in1[12] = xx.sc;
    in1[13] = xx.sd;
    in1[14] = xx.se;
    in1[15] = xx.sf;
    in2[0] = yy.s0;
    in2[1] = yy.s1;
    in2[2] = yy.s2;
    in2[3] = yy.s3;
    in2[4] = yy.s4;
    in2[5] = yy.s5;
    in2[6] = yy.s6;
    in2[7] = yy.s7;
    in2[8] = yy.s8;
    in2[9] = yy.s9;
    in2[10] = yy.sa;
    in2[11] = yy.sb;
    in2[12] = yy.sc;
    in2[13] = yy.sd;
    in2[14] = yy.se;
    in2[15] = yy.sf;
    for(uint i = 0; i < 16; i++)
    {
        out[i] = SPIRV_OCL_BUILTIN(remquo, _f64_f64_p4i32, )(in1[i], in2[i], (__generic int*)&out2[i]);
    }
    quo->s0 = out2[0];
    quo->s1 = out2[1];
    quo->s2 = out2[2];
    quo->s3 = out2[3];
    quo->s4 = out2[4];
    quo->s5 = out2[5];
    quo->s6 = out2[6];
    quo->s7 = out2[7];
    quo->s8 = out2[8];
    quo->s9 = out2[9];
    quo->sa = out2[10];
    quo->sb = out2[11];
    quo->sc = out2[12];
    quo->sd = out2[13];
    quo->se = out2[14];
    quo->sf = out2[15];
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

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

#endif // defined(cl_khr_fp64)
