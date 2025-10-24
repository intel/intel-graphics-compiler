/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Arithmetic Instructions

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v2f16_v2f16, )(half2 Vector1, half2 Vector2)
{
    return SPIRV_OCL_BUILTIN(fma, _f16_f16_f16, )(Vector1.x,  Vector2.x, (Vector1.y * Vector2.y));
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v3f16_v3f16, )(half3 Vector1, half3 Vector2)
{
    return SPIRV_OCL_BUILTIN(fma, _f16_f16_f16, )(Vector1.x, Vector2.x,
           SPIRV_OCL_BUILTIN(fma, _f16_f16_f16, )(Vector1.y, Vector2.y, (Vector1.z * Vector2.z)));
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v4f16_v4f16, )(half4 Vector1, half4 Vector2)
{
    return SPIRV_OCL_BUILTIN(fma, _f16_f16_f16, )(Vector1.x, Vector2.x,
           SPIRV_OCL_BUILTIN(fma, _f16_f16_f16, )(Vector1.y, Vector2.y,
           SPIRV_OCL_BUILTIN(fma, _f16_f16_f16, )(Vector1.z, Vector2.z,
                                           (Vector1.w * Vector2.w))));
}

// TODO: should we support beyond vec4 which is what OCL is limited to?
#if 0
half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v8f16_v8f16, )(half8 Vector1, half8 Vector2)
{
}

half SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v16f16_v16f16, )(half16 Vector1, half16 Vector2)
{
}
#endif

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v2f32_v2f32, )(float2 Vector1, float2 Vector2)
{
    return SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Vector1.x,  Vector2.x, (Vector1.y * Vector2.y));
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v3f32_v3f32, )(float3 Vector1, float3 Vector2)
{
    return SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Vector1.x, Vector2.x,
           SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Vector1.y, Vector2.y, (Vector1.z * Vector2.z)));
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v4f32_v4f32, )(float4 Vector1, float4 Vector2)
{
    return SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Vector1.x, Vector2.x,
           SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Vector1.y, Vector2.y,
           SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(Vector1.z, Vector2.z,
                                           (Vector1.w * Vector2.w))));
}

#if 0
float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v8f32_v8f32, )(float8 Vector1, float8 Vector2)
{
}

float SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v16f32_v16f32, )(float16 Vector1, float16 Vector2)
{
}
#endif

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v2f64_v2f64, )(double2 Vector1, double2 Vector2)
{
    return SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Vector1.x,  Vector2.x, (Vector1.y * Vector2.y));
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v3f64_v3f64, )(double3 Vector1, double3 Vector2)
{
    return SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Vector1.x, Vector2.x,
           SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Vector1.y, Vector2.y, (Vector1.z * Vector2.z)));
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v4f64_v4f64, )(double4 Vector1, double4 Vector2)
{
    return SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Vector1.x, Vector2.x,
           SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Vector1.y, Vector2.y,
           SPIRV_OCL_BUILTIN(fma, _f64_f64_f64, )(Vector1.z, Vector2.z,
                                           (Vector1.w * Vector2.w))));
}

#endif

#if 0

#if defined(cl_khr_fp64)
double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v8f64_v8f64, )(double8 Vector1, double8 Vector2)
{
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(Dot, _v16f64_v16f64, )(double16 Vector1, double16 Vector2)
{
}
#endif

#endif

// unsigned

TwoOp_i8    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _i8_i8, )(uchar Operand1, uchar Operand2)
{
    return (TwoOp_i8) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _i8_i8, )(Operand1, Operand2) };
}

TwoOp_v2i8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v2i8_v2i8, )(uchar2 Operand1, uchar2 Operand2)
{
    return (TwoOp_v2i8) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v2i8_v2i8, )(Operand1, Operand2) };
}

TwoOp_v3i8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v3i8_v3i8, )(uchar3 Operand1, uchar3 Operand2)
{
    return (TwoOp_v3i8) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v3i8_v3i8, )(Operand1, Operand2) };
}

TwoOp_v4i8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v4i8_v4i8, )(uchar4 Operand1, uchar4 Operand2)
{
    return (TwoOp_v4i8) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v4i8_v4i8, )(Operand1, Operand2) };
}

TwoOp_v8i8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v8i8_v8i8, )(uchar8 Operand1, uchar8 Operand2)
{
    return (TwoOp_v8i8) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v8i8_v8i8, )(Operand1, Operand2) };
}

TwoOp_v16i8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v16i8_v16i8, )(uchar16 Operand1, uchar16 Operand2)
{
    return (TwoOp_v16i8) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v16i8_v16i8, )(Operand1, Operand2) };
}

TwoOp_i16    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _i16_i16, )(ushort Operand1, ushort Operand2)
{
    return (TwoOp_i16) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _i16_i16, )(Operand1, Operand2) };
}

TwoOp_v2i16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v2i16_v2i16, )(ushort2 Operand1, ushort2 Operand2)
{
    return (TwoOp_v2i16) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v2i16_v2i16, )(Operand1, Operand2) };
}

TwoOp_v3i16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v3i16_v3i16, )(ushort3 Operand1, ushort3 Operand2)
{
    return (TwoOp_v3i16) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v3i16_v3i16, )(Operand1, Operand2) };
}

TwoOp_v4i16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v4i16_v4i16, )(ushort4 Operand1, ushort4 Operand2)
{
    return (TwoOp_v4i16) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v4i16_v4i16, )(Operand1, Operand2) };
}

TwoOp_v8i16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v8i16_v8i16, )(ushort8 Operand1, ushort8 Operand2)
{
    return (TwoOp_v8i16) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v8i16_v8i16, )(Operand1, Operand2) };
}

TwoOp_v16i16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v16i16_v16i16, )(ushort16 Operand1, ushort16 Operand2)
{
    return (TwoOp_v16i16) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v16i16_v16i16, )(Operand1, Operand2) };
}

TwoOp_i32    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _i32_i32, )(uint Operand1, uint Operand2)
{
    return (TwoOp_i32) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _i32_i32, )(Operand1, Operand2) };
}

TwoOp_v2i32  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v2i32_v2i32, )(uint2 Operand1, uint2 Operand2)
{
    return (TwoOp_v2i32) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v2i32_v2i32, )(Operand1, Operand2) };
}

TwoOp_v3i32  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v3i32_v3i32, )(uint3 Operand1, uint3 Operand2)
{
    return (TwoOp_v3i32) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v3i32_v3i32, )(Operand1, Operand2) };
}

TwoOp_v4i32  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v4i32_v4i32, )(uint4 Operand1, uint4 Operand2)
{
    return (TwoOp_v4i32) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v4i32_v4i32, )(Operand1, Operand2) };
}

TwoOp_v8i32  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v8i32_v8i32, )(uint8 Operand1, uint8 Operand2)
{
    return (TwoOp_v8i32) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v8i32_v8i32, )(Operand1, Operand2) };
}

TwoOp_v16i32 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v16i32_v16i32, )(uint16 Operand1, uint16 Operand2)
{
    return (TwoOp_v16i32) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v16i32_v16i32, )(Operand1, Operand2) };
}

TwoOp_i64    SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _i64_i64, )(ulong Operand1, ulong Operand2)
{
    return (TwoOp_i64) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _i64_i64, )(Operand1, Operand2) };
}

TwoOp_v2i64  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v2i64_v2i64, )(ulong2 Operand1, ulong2 Operand2)
{
    return (TwoOp_v2i64) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v2i64_v2i64, )(Operand1, Operand2) };
}

TwoOp_v3i64  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v3i64_v3i64, )(ulong3 Operand1, ulong3 Operand2)
{
    return (TwoOp_v3i64) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v3i64_v3i64, )(Operand1, Operand2) };
}

TwoOp_v4i64  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v4i64_v4i64, )(ulong4 Operand1, ulong4 Operand2)
{
    return (TwoOp_v4i64) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v4i64_v4i64, )(Operand1, Operand2) };
}

TwoOp_v8i64  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v8i64_v8i64, )(ulong8 Operand1, ulong8 Operand2)
{
    return (TwoOp_v8i64) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v8i64_v8i64, )(Operand1, Operand2) };
}

TwoOp_v16i64 SPIRV_OVERLOADABLE SPIRV_BUILTIN(UMulExtended, _v16i64_v16i64, )(ulong16 Operand1, ulong16 Operand2)
{
    return (TwoOp_v16i64) { Operand1 * Operand2, SPIRV_OCL_BUILTIN(u_mul_hi, _v16i64_v16i64, )(Operand1, Operand2) };
}

// signed

TwoOp_i8    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _i8_i8, )(char Operand1, char Operand2)
{
    return (TwoOp_i8) { as_uchar((char)(Operand1 * Operand2)), as_uchar(SPIRV_OCL_BUILTIN(s_mul_hi, _i8_i8, )(Operand1, Operand2)) };
}

TwoOp_v2i8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v2i8_v2i8, )(char2 Operand1, char2 Operand2)
{
    return (TwoOp_v2i8) { as_uchar2(Operand1 * Operand2), as_uchar2(SPIRV_OCL_BUILTIN(s_mul_hi, _v2i8_v2i8, )(Operand1, Operand2)) };
}

TwoOp_v3i8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v3i8_v3i8, )(char3 Operand1, char3 Operand2)
{
    return (TwoOp_v3i8) { as_uchar3(Operand1 * Operand2), as_uchar3(SPIRV_OCL_BUILTIN(s_mul_hi, _v3i8_v3i8, )(Operand1, Operand2)) };
}

TwoOp_v4i8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v4i8_v4i8, )(char4 Operand1, char4 Operand2)
{
    return (TwoOp_v4i8) { as_uchar4(Operand1 * Operand2), as_uchar4(SPIRV_OCL_BUILTIN(s_mul_hi, _v4i8_v4i8, )(Operand1, Operand2)) };
}

TwoOp_v8i8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v8i8_v8i8, )(char8 Operand1, char8 Operand2)
{
    return (TwoOp_v8i8) { as_uchar8(Operand1 * Operand2), as_uchar8(SPIRV_OCL_BUILTIN(s_mul_hi, _v8i8_v8i8, )(Operand1, Operand2)) };
}

TwoOp_v16i8 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v16i8_v16i8, )(char16 Operand1, char16 Operand2)
{
    return (TwoOp_v16i8) { as_uchar16(Operand1 * Operand2), as_uchar16(SPIRV_OCL_BUILTIN(s_mul_hi, _v16i8_v16i8, )(Operand1, Operand2)) };
}

TwoOp_i16    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _i16_i16, )(short Operand1, short Operand2)
{
    return (TwoOp_i16) { as_ushort((short)(Operand1 * Operand2)), as_ushort(SPIRV_OCL_BUILTIN(s_mul_hi, _i16_i16, )(Operand1, Operand2)) };
}

TwoOp_v2i16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v2i16_v2i16, )(short2 Operand1, short2 Operand2)
{
    return (TwoOp_v2i16) { as_ushort2(Operand1 * Operand2), as_ushort2(SPIRV_OCL_BUILTIN(s_mul_hi, _v2i16_v2i16, )(Operand1, Operand2)) };
}

TwoOp_v3i16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v3i16_v3i16, )(short3 Operand1, short3 Operand2)
{
    return (TwoOp_v3i16) { as_ushort3(Operand1 * Operand2), as_ushort3(SPIRV_OCL_BUILTIN(s_mul_hi, _v3i16_v3i16, )(Operand1, Operand2)) };
}

TwoOp_v4i16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v4i16_v4i16, )(short4 Operand1, short4 Operand2)
{
    return (TwoOp_v4i16) { as_ushort4(Operand1 * Operand2), as_ushort4(SPIRV_OCL_BUILTIN(s_mul_hi, _v4i16_v4i16, )(Operand1, Operand2)) };
}

TwoOp_v8i16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v8i16_v8i16, )(short8 Operand1, short8 Operand2)
{
    return (TwoOp_v8i16) { as_ushort8(Operand1 * Operand2), as_ushort8(SPIRV_OCL_BUILTIN(s_mul_hi, _v8i16_v8i16, )(Operand1, Operand2)) };
}

TwoOp_v16i16 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v16i16_v16i16, )(short16 Operand1, short16 Operand2)
{
    return (TwoOp_v16i16) { as_ushort16(Operand1 * Operand2), as_ushort16(SPIRV_OCL_BUILTIN(s_mul_hi, _v16i16_v16i16, )(Operand1, Operand2)) };
}

TwoOp_i32    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _i32_i32, )(int Operand1, int Operand2)
{
    return (TwoOp_i32) { as_uint(Operand1 * Operand2), as_uint(SPIRV_OCL_BUILTIN(s_mul_hi, _i32_i32, )(Operand1, Operand2)) };
}

TwoOp_v2i32  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v2i32_v2i32, )(int2 Operand1, int2 Operand2)
{
    return (TwoOp_v2i32) { as_uint2(Operand1 * Operand2), as_uint2(SPIRV_OCL_BUILTIN(s_mul_hi, _v2i32_v2i32, )(Operand1, Operand2)) };
}

TwoOp_v3i32  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v3i32_v3i32, )(int3 Operand1, int3 Operand2)
{
    return (TwoOp_v3i32) { as_uint3(Operand1 * Operand2), as_uint3(SPIRV_OCL_BUILTIN(s_mul_hi, _v3i32_v3i32, )(Operand1, Operand2)) };
}

TwoOp_v4i32  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v4i32_v4i32, )(int4 Operand1, int4 Operand2)
{
    return (TwoOp_v4i32) { as_uint4(Operand1 * Operand2), as_uint4(SPIRV_OCL_BUILTIN(s_mul_hi, _v4i32_v4i32, )(Operand1, Operand2)) };
}

TwoOp_v8i32  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v8i32_v8i32, )(int8 Operand1, int8 Operand2)
{
    return (TwoOp_v8i32) { as_uint8(Operand1 * Operand2), as_uint8(SPIRV_OCL_BUILTIN(s_mul_hi, _v8i32_v8i32, )(Operand1, Operand2)) };
}

TwoOp_v16i32 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v16i32_v16i32, )(int16 Operand1, int16 Operand2)
{
    return (TwoOp_v16i32) { as_uint16(Operand1 * Operand2), as_uint16(SPIRV_OCL_BUILTIN(s_mul_hi, _v16i32_v16i32, )(Operand1, Operand2)) };
}

TwoOp_i64    SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _i64_i64, )(long Operand1, long Operand2)
{
    return (TwoOp_i64) { as_ulong(Operand1 * Operand2), as_ulong(SPIRV_OCL_BUILTIN(s_mul_hi, _i64_i64, )(Operand1, Operand2)) };
}

TwoOp_v2i64  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v2i64_v2i64, )(long2 Operand1, long2 Operand2)
{
    return (TwoOp_v2i64) { as_ulong2(Operand1 * Operand2), as_ulong2(SPIRV_OCL_BUILTIN(s_mul_hi, _v2i64_v2i64, )(Operand1, Operand2)) };
}

TwoOp_v3i64  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v3i64_v3i64, )(long3 Operand1, long3 Operand2)
{
    return (TwoOp_v3i64) { as_ulong3(Operand1 * Operand2), as_ulong3(SPIRV_OCL_BUILTIN(s_mul_hi, _v3i64_v3i64, )(Operand1, Operand2)) };
}

TwoOp_v4i64  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v4i64_v4i64, )(long4 Operand1, long4 Operand2)
{
    return (TwoOp_v4i64) { as_ulong4(Operand1 * Operand2), as_ulong4(SPIRV_OCL_BUILTIN(s_mul_hi, _v4i64_v4i64, )(Operand1, Operand2)) };
}

TwoOp_v8i64  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v8i64_v8i64, )(long8 Operand1, long8 Operand2)
{
    return (TwoOp_v8i64) { as_ulong8(Operand1 * Operand2), as_ulong8(SPIRV_OCL_BUILTIN(s_mul_hi, _v8i64_v8i64, )(Operand1, Operand2)) };
}

TwoOp_v16i64 SPIRV_OVERLOADABLE SPIRV_BUILTIN(SMulExtended, _v16i64_v16i64, )(long16 Operand1, long16 Operand2)
{
    return (TwoOp_v16i64) { as_ulong16(Operand1 * Operand2), as_ulong16(SPIRV_OCL_BUILTIN(s_mul_hi, _v16i64_v16i64, )(Operand1, Operand2)) };
}

