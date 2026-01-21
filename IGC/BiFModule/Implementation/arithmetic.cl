/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Arithmetic Instructions

half __attribute__((overloadable)) __spirv_Dot(half2 Vector1, half2 Vector2)
{
    return __spirv_ocl_fma(Vector1.x,  Vector2.x, (Vector1.y * Vector2.y));
}

half __attribute__((overloadable)) __spirv_Dot(half3 Vector1, half3 Vector2)
{
    return __spirv_ocl_fma(Vector1.x, Vector2.x,
           __spirv_ocl_fma(Vector1.y, Vector2.y, (Vector1.z * Vector2.z)));
}

half __attribute__((overloadable)) __spirv_Dot(half4 Vector1, half4 Vector2)
{
    return __spirv_ocl_fma(Vector1.x, Vector2.x,
           __spirv_ocl_fma(Vector1.y, Vector2.y,
           __spirv_ocl_fma(Vector1.z, Vector2.z,
                                           (Vector1.w * Vector2.w))));
}

// TODO: should we support beyond vec4 which is what OCL is limited to?
#if 0
half __attribute__((overloadable)) __spirv_Dot(half8 Vector1, half8 Vector2)
{
}

half __attribute__((overloadable)) __spirv_Dot(half16 Vector1, half16 Vector2)
{
}
#endif

float __attribute__((overloadable)) __spirv_Dot(float2 Vector1, float2 Vector2)
{
    return __spirv_ocl_fma(Vector1.x,  Vector2.x, (Vector1.y * Vector2.y));
}

float __attribute__((overloadable)) __spirv_Dot(float3 Vector1, float3 Vector2)
{
    return __spirv_ocl_fma(Vector1.x, Vector2.x,
           __spirv_ocl_fma(Vector1.y, Vector2.y, (Vector1.z * Vector2.z)));
}

float __attribute__((overloadable)) __spirv_Dot(float4 Vector1, float4 Vector2)
{
    return __spirv_ocl_fma(Vector1.x, Vector2.x,
           __spirv_ocl_fma(Vector1.y, Vector2.y,
           __spirv_ocl_fma(Vector1.z, Vector2.z,
                                           (Vector1.w * Vector2.w))));
}

#if 0
float __attribute__((overloadable)) __spirv_Dot(float8 Vector1, float8 Vector2)
{
}

float __attribute__((overloadable)) __spirv_Dot(float16 Vector1, float16 Vector2)
{
}
#endif

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_Dot(double2 Vector1, double2 Vector2)
{
    return __spirv_ocl_fma(Vector1.x,  Vector2.x, (Vector1.y * Vector2.y));
}

double __attribute__((overloadable)) __spirv_Dot(double3 Vector1, double3 Vector2)
{
    return __spirv_ocl_fma(Vector1.x, Vector2.x,
           __spirv_ocl_fma(Vector1.y, Vector2.y, (Vector1.z * Vector2.z)));
}

double __attribute__((overloadable)) __spirv_Dot(double4 Vector1, double4 Vector2)
{
    return __spirv_ocl_fma(Vector1.x, Vector2.x,
           __spirv_ocl_fma(Vector1.y, Vector2.y,
           __spirv_ocl_fma(Vector1.z, Vector2.z,
                                           (Vector1.w * Vector2.w))));
}

#endif

#if 0

#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_Dot(double8 Vector1, double8 Vector2)
{
}

double __attribute__((overloadable)) __spirv_Dot(double16 Vector1, double16 Vector2)
{
}
#endif

#endif

// unsigned

TwoOp_i8    __attribute__((overloadable)) __spirv_UMulExtended(uchar Operand1, uchar Operand2)
{
    return (TwoOp_i8) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v2i8  __attribute__((overloadable)) __spirv_UMulExtended(uchar2 Operand1, uchar2 Operand2)
{
    return (TwoOp_v2i8) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v3i8  __attribute__((overloadable)) __spirv_UMulExtended(uchar3 Operand1, uchar3 Operand2)
{
    return (TwoOp_v3i8) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v4i8  __attribute__((overloadable)) __spirv_UMulExtended(uchar4 Operand1, uchar4 Operand2)
{
    return (TwoOp_v4i8) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v8i8  __attribute__((overloadable)) __spirv_UMulExtended(uchar8 Operand1, uchar8 Operand2)
{
    return (TwoOp_v8i8) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v16i8 __attribute__((overloadable)) __spirv_UMulExtended(uchar16 Operand1, uchar16 Operand2)
{
    return (TwoOp_v16i8) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_i16    __attribute__((overloadable)) __spirv_UMulExtended(ushort Operand1, ushort Operand2)
{
    return (TwoOp_i16) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v2i16  __attribute__((overloadable)) __spirv_UMulExtended(ushort2 Operand1, ushort2 Operand2)
{
    return (TwoOp_v2i16) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v3i16  __attribute__((overloadable)) __spirv_UMulExtended(ushort3 Operand1, ushort3 Operand2)
{
    return (TwoOp_v3i16) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v4i16  __attribute__((overloadable)) __spirv_UMulExtended(ushort4 Operand1, ushort4 Operand2)
{
    return (TwoOp_v4i16) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v8i16  __attribute__((overloadable)) __spirv_UMulExtended(ushort8 Operand1, ushort8 Operand2)
{
    return (TwoOp_v8i16) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v16i16 __attribute__((overloadable)) __spirv_UMulExtended(ushort16 Operand1, ushort16 Operand2)
{
    return (TwoOp_v16i16) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_i32    __attribute__((overloadable)) __spirv_UMulExtended(uint Operand1, uint Operand2)
{
    return (TwoOp_i32) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v2i32  __attribute__((overloadable)) __spirv_UMulExtended(uint2 Operand1, uint2 Operand2)
{
    return (TwoOp_v2i32) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v3i32  __attribute__((overloadable)) __spirv_UMulExtended(uint3 Operand1, uint3 Operand2)
{
    return (TwoOp_v3i32) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v4i32  __attribute__((overloadable)) __spirv_UMulExtended(uint4 Operand1, uint4 Operand2)
{
    return (TwoOp_v4i32) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v8i32  __attribute__((overloadable)) __spirv_UMulExtended(uint8 Operand1, uint8 Operand2)
{
    return (TwoOp_v8i32) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v16i32 __attribute__((overloadable)) __spirv_UMulExtended(uint16 Operand1, uint16 Operand2)
{
    return (TwoOp_v16i32) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_i64    __attribute__((overloadable)) __spirv_UMulExtended(ulong Operand1, ulong Operand2)
{
    return (TwoOp_i64) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v2i64  __attribute__((overloadable)) __spirv_UMulExtended(ulong2 Operand1, ulong2 Operand2)
{
    return (TwoOp_v2i64) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v3i64  __attribute__((overloadable)) __spirv_UMulExtended(ulong3 Operand1, ulong3 Operand2)
{
    return (TwoOp_v3i64) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v4i64  __attribute__((overloadable)) __spirv_UMulExtended(ulong4 Operand1, ulong4 Operand2)
{
    return (TwoOp_v4i64) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v8i64  __attribute__((overloadable)) __spirv_UMulExtended(ulong8 Operand1, ulong8 Operand2)
{
    return (TwoOp_v8i64) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

TwoOp_v16i64 __attribute__((overloadable)) __spirv_UMulExtended(ulong16 Operand1, ulong16 Operand2)
{
    return (TwoOp_v16i64) { Operand1 * Operand2, __spirv_ocl_u_mul_hi(Operand1, Operand2) };
}

// signed

TwoOp_i8    __attribute__((overloadable)) __spirv_SMulExtended(char Operand1, char Operand2)
{
    return (TwoOp_i8) { as_uchar((char)(Operand1 * Operand2)), as_uchar(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v2i8  __attribute__((overloadable)) __spirv_SMulExtended(char2 Operand1, char2 Operand2)
{
    return (TwoOp_v2i8) { as_uchar2(Operand1 * Operand2), as_uchar2(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v3i8  __attribute__((overloadable)) __spirv_SMulExtended(char3 Operand1, char3 Operand2)
{
    return (TwoOp_v3i8) { as_uchar3(Operand1 * Operand2), as_uchar3(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v4i8  __attribute__((overloadable)) __spirv_SMulExtended(char4 Operand1, char4 Operand2)
{
    return (TwoOp_v4i8) { as_uchar4(Operand1 * Operand2), as_uchar4(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v8i8  __attribute__((overloadable)) __spirv_SMulExtended(char8 Operand1, char8 Operand2)
{
    return (TwoOp_v8i8) { as_uchar8(Operand1 * Operand2), as_uchar8(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v16i8 __attribute__((overloadable)) __spirv_SMulExtended(char16 Operand1, char16 Operand2)
{
    return (TwoOp_v16i8) { as_uchar16(Operand1 * Operand2), as_uchar16(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_i16    __attribute__((overloadable)) __spirv_SMulExtended(short Operand1, short Operand2)
{
    return (TwoOp_i16) { as_ushort((short)(Operand1 * Operand2)), as_ushort(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v2i16  __attribute__((overloadable)) __spirv_SMulExtended(short2 Operand1, short2 Operand2)
{
    return (TwoOp_v2i16) { as_ushort2(Operand1 * Operand2), as_ushort2(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v3i16  __attribute__((overloadable)) __spirv_SMulExtended(short3 Operand1, short3 Operand2)
{
    return (TwoOp_v3i16) { as_ushort3(Operand1 * Operand2), as_ushort3(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v4i16  __attribute__((overloadable)) __spirv_SMulExtended(short4 Operand1, short4 Operand2)
{
    return (TwoOp_v4i16) { as_ushort4(Operand1 * Operand2), as_ushort4(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v8i16  __attribute__((overloadable)) __spirv_SMulExtended(short8 Operand1, short8 Operand2)
{
    return (TwoOp_v8i16) { as_ushort8(Operand1 * Operand2), as_ushort8(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v16i16 __attribute__((overloadable)) __spirv_SMulExtended(short16 Operand1, short16 Operand2)
{
    return (TwoOp_v16i16) { as_ushort16(Operand1 * Operand2), as_ushort16(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_i32    __attribute__((overloadable)) __spirv_SMulExtended(int Operand1, int Operand2)
{
    return (TwoOp_i32) { as_uint(Operand1 * Operand2), as_uint(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v2i32  __attribute__((overloadable)) __spirv_SMulExtended(int2 Operand1, int2 Operand2)
{
    return (TwoOp_v2i32) { as_uint2(Operand1 * Operand2), as_uint2(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v3i32  __attribute__((overloadable)) __spirv_SMulExtended(int3 Operand1, int3 Operand2)
{
    return (TwoOp_v3i32) { as_uint3(Operand1 * Operand2), as_uint3(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v4i32  __attribute__((overloadable)) __spirv_SMulExtended(int4 Operand1, int4 Operand2)
{
    return (TwoOp_v4i32) { as_uint4(Operand1 * Operand2), as_uint4(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v8i32  __attribute__((overloadable)) __spirv_SMulExtended(int8 Operand1, int8 Operand2)
{
    return (TwoOp_v8i32) { as_uint8(Operand1 * Operand2), as_uint8(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v16i32 __attribute__((overloadable)) __spirv_SMulExtended(int16 Operand1, int16 Operand2)
{
    return (TwoOp_v16i32) { as_uint16(Operand1 * Operand2), as_uint16(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_i64    __attribute__((overloadable)) __spirv_SMulExtended(long Operand1, long Operand2)
{
    return (TwoOp_i64) { as_ulong(Operand1 * Operand2), as_ulong(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v2i64  __attribute__((overloadable)) __spirv_SMulExtended(long2 Operand1, long2 Operand2)
{
    return (TwoOp_v2i64) { as_ulong2(Operand1 * Operand2), as_ulong2(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v3i64  __attribute__((overloadable)) __spirv_SMulExtended(long3 Operand1, long3 Operand2)
{
    return (TwoOp_v3i64) { as_ulong3(Operand1 * Operand2), as_ulong3(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v4i64  __attribute__((overloadable)) __spirv_SMulExtended(long4 Operand1, long4 Operand2)
{
    return (TwoOp_v4i64) { as_ulong4(Operand1 * Operand2), as_ulong4(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v8i64  __attribute__((overloadable)) __spirv_SMulExtended(long8 Operand1, long8 Operand2)
{
    return (TwoOp_v8i64) { as_ulong8(Operand1 * Operand2), as_ulong8(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

TwoOp_v16i64 __attribute__((overloadable)) __spirv_SMulExtended(long16 Operand1, long16 Operand2)
{
    return (TwoOp_v16i64) { as_ulong16(Operand1 * Operand2), as_ulong16(__spirv_ocl_s_mul_hi(Operand1, Operand2)) };
}

