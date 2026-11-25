/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.h"
#include "../Headers/spirv.h"

/* Helper Functions from IBiF_Conversions.cl */

#ifdef __IGC_BUILD__
#define UCHAR_MIN ((uchar)0)
#define USHRT_MIN ((ushort)0)
#define UINT_MIN ((uint)0)
#define ULONG_MIN ((ulong)0)
// Helper function for conversions with saturation
#define SAT_CLAMP(TO, TONAME, FROM, FROM_MNGL)                                                                         \
  static TO __clamp_sat_##TO##_##FROM(FROM _T) {                                                                       \
    /* Produce 0 for NaN values */                                                                                     \
    FROM NaNClamp = __spirv_IsNan(_T) ? 0 : _T;                                                                        \
    FROM MinClamp = __spirv_ocl_fmax(NaNClamp, (FROM)TONAME##_MIN);                                                    \
    return (TO)__spirv_ocl_fmin(MinClamp, (FROM)TONAME##_MAX);                                                         \
  }
// We would love to use fmin/fmax clamping logic for all cases, as it yields
// better ISA. However, for some int <-> FP type pairs, IMAX value 2^n - 1
// cannot be represented exactly in the target FP type (never an issue for even
// IMIN values). When we clamp to IMAX, the backwards conversion to such int
// type will yield IMAX + 1 and result in integer overflow, which is
// technically UB for runtime values and practically - for compile-time known
// constants. Move the upper limit clamping to int type's realm for such cases.
#define SAT_CLAMP_INEXACT_MAX(TO, TONAME, FROM, FROM_MNGL)                                                             \
  static TO __clamp_sat_##TO##_##FROM(FROM _T) {                                                                       \
    FROM NaNClamp = __spirv_IsNan(_T) ? 0 : _T;                                                                        \
    FROM MinClamp = __spirv_ocl_fmax(NaNClamp, (FROM)TONAME##_MIN);                                                    \
    return MinClamp >= (FROM)TONAME##_MAX ? TONAME##_MAX : (TO)MinClamp;                                               \
  }
// If 'TO' int type limits include 'FROM' FP type normal limits, only INF
// checks are needed. We need direct comparison to INF to account for
// compile-time known constant values - with these, LLVM InstSimplifier will
// view integer min/max conversions to "lesser" FP type as invalid, resulting
// in UB.
// TODO: This is but a workaround for LLVM language limitation, because our HW
// min/max instructions handle infinity values just fine. Should any negative
// effects on performance be observed, consider re-implementing the convert_sat
// builtins as intrinsic calls (llvm.fpto*i.sat or synonymic GenISA intrinsics)
// similarly to fmin/fmax.
#define SAT_CLAMP_INF_ONLY_SIGNED(TO, TONAME, FROM, FROM_MNGL)                                                         \
  static TO __clamp_sat_##TO##_##FROM(FROM _T) {                                                                       \
    FROM NaNClamp = __spirv_IsNan(_T) ? 0 : _T;                                                                        \
    TO MinClamp = _T == (FROM) - INFINITY ? TONAME##_MIN : (TO)NaNClamp;                                               \
    return _T == (FROM)INFINITY ? TONAME##_MAX : MinClamp;                                                             \
  }
#define SAT_CLAMP_INF_ONLY_UNSIGNED(TO, TONAME, FROM, FROM_MNGL)                                                       \
  static TO __clamp_sat_##TO##_##FROM(FROM _T) {                                                                       \
    FROM NaNClamp = __spirv_IsNan(_T) ? 0 : _T;                                                                        \
    /* For unsigned, we still need a regular check of lower limit*/                                                    \
    TO MinClamp = (TO)__spirv_ocl_fmax(NaNClamp, 0);                                                                   \
    return _T == (FROM)INFINITY ? TONAME##_MAX : MinClamp;                                                             \
  }

// Half - normal limits are [-65504, 65504]
#if defined(cl_khr_fp16)
SAT_CLAMP(uchar, UCHAR, half, f16)
SAT_CLAMP_INF_ONLY_UNSIGNED(ushort, USHRT, half, f16)
SAT_CLAMP_INF_ONLY_UNSIGNED(uint, UINT, half, f16)
SAT_CLAMP_INF_ONLY_UNSIGNED(ulong, ULONG, half, f16)
SAT_CLAMP(char, CHAR, half, f16)
SAT_CLAMP_INEXACT_MAX(short, SHRT, half, f16)
SAT_CLAMP_INF_ONLY_SIGNED(int, INT, half, f16)
SAT_CLAMP_INF_ONLY_SIGNED(long, LONG, half, f16)
#endif // defined(cl_khr_fp16)
// Float - normal limits are approx. [-3.4e+38, 3.4e+38] - exceeds all ints
SAT_CLAMP(uchar, UCHAR, float, f32)
SAT_CLAMP(ushort, USHRT, float, f32)
SAT_CLAMP(uint, UINT, float, f32)
SAT_CLAMP_INEXACT_MAX(ulong, ULONG, float, f32)
SAT_CLAMP(char, CHAR, float, f32)
SAT_CLAMP(short, SHRT, float, f32)
SAT_CLAMP(int, INT, float, f32)
SAT_CLAMP_INEXACT_MAX(long, LONG, float, f32)
// Double - normal limmits are approx. [-1.8e+308, 1.8e+308]
#if defined(cl_khr_fp64)
SAT_CLAMP(uchar, UCHAR, double, f64)
SAT_CLAMP(ushort, USHRT, double, f64)
SAT_CLAMP(uint, UINT, double, f64)
SAT_CLAMP_INEXACT_MAX(ulong, ULONG, double, f64)
SAT_CLAMP(char, CHAR, double, f64)
SAT_CLAMP(short, SHRT, double, f64)
SAT_CLAMP(int, INT, double, f64)
SAT_CLAMP_INEXACT_MAX(long, LONG, double, f64)
#endif // defined(cl_khr_fp64)

#endif //__IGC_BUILD__

static float convertUItoFP32(ulong value, char roundingMode, bool s);
static float convertSItoFP32(long value, char roundingMode);

// Conversion Instructions

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar(half FloatValue) { return FloatValue; }

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort(half FloatValue) { return FloatValue; }

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint(half FloatValue) { return FloatValue; }

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong(half FloatValue) { return FloatValue; }

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar(float FloatValue) { return (uchar)FloatValue; }

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort(float FloatValue) { return (ushort)FloatValue; }

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint(float FloatValue) { return (uint)FloatValue; }

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong(float FloatValue) {
  float FC0 = __spirv_ocl_ldexp(1.0f, -32);
  float FC1 = __spirv_ocl_ldexp(-1.0f, 32);
  float HiF = __spirv_ocl_trunc(FloatValue * FC0);
  float LoF = __spirv_ocl_fma(HiF, FC1, FloatValue);
  ulong answer = 0;
  answer = (((uint)HiF | answer) << 32) | ((uint)LoF | answer);
  return answer;
}

#if defined(cl_khr_fp64)

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar(double FloatValue) { return FloatValue; }

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort(double FloatValue) { return FloatValue; }

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint(double FloatValue) { return FloatValue; }

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong(double FloatValue) { return FloatValue; }

#endif // defined(cl_khr_fp64)

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar(half FloatValue) { return FloatValue; }

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort(half FloatValue) { return FloatValue; }

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint(half FloatValue) { return FloatValue; }

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong(half FloatValue) { return FloatValue; }

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar(float FloatValue) { return (char)FloatValue; }

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort(float FloatValue) { return (short)FloatValue; }

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint(float FloatValue) { return (int)FloatValue; }

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong(float FloatValue) {
  float abs_value = __spirv_ocl_fabs(FloatValue);
  if (abs_value < 1.0f) // for small numbers and denormals
  {
    return (long)0;
  }
  uint sign = as_uint(as_int(FloatValue) >> 31);
  float FC0 = __spirv_ocl_ldexp(1.0f, -32);
  float FC1 = __spirv_ocl_ldexp(-1.0f, 32);
  float HiF = __spirv_ocl_trunc(abs_value * FC0);
  float LoF = __spirv_ocl_fma(HiF, FC1, abs_value);
  uint Hi = (uint)HiF ^ sign;
  uint Lo = (uint)LoF ^ sign;
  uint Lo_new = Lo - sign;
  if (sign == -1 && Lo == -1) {
    Hi = Hi + 1;
  }
  return as_long(((ulong)Hi << 32) | (ulong)Lo_new);
}

#if defined(cl_khr_fp64)

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar(double FloatValue) { return FloatValue; }

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort(double FloatValue) { return FloatValue; }

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint(double FloatValue) { return FloatValue; }

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong(double FloatValue) { return FloatValue; }

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf(char SignedValue) { return SignedValue; }

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(char SignedValue) { return (float)SignedValue; }

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf(short SignedValue) { return SignedValue; }

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(short SignedValue) { return (float)SignedValue; }

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf(int SignedValue) { return SignedValue; }

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(int SignedValue) { return (float)SignedValue; }

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf(long SignedValue) { return SignedValue; }

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(long SignedValue) {
  return convertSItoFP32(SignedValue, 0);
}

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf(uchar UnsignedValue) { return UnsignedValue; }
float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(bool BoolValue) { return BoolValue ? 1.0 : 0.0; }

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(uchar UnsignedValue) { return (float)UnsignedValue; }

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf(ushort UnsignedValue) { return UnsignedValue; }

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(ushort UnsignedValue) { return (float)UnsignedValue; }

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf(uint UnsignedValue) { return UnsignedValue; }

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(uint UnsignedValue) { return (float)UnsignedValue; }

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf(ulong UnsignedValue) { return UnsignedValue; }

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(ulong UnsignedValue) {
  return convertUItoFP32(UnsignedValue, 0, false);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(char SignedValue) { return SignedValue; }

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(short SignedValue) { return SignedValue; }

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(int SignedValue) { return SignedValue; }

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(long SignedValue) { return SignedValue; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(bool BoolValue) { return BoolValue ? 1.0 : 0.0; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(uchar UnsignedValue) { return UnsignedValue; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(ushort UnsignedValue) { return UnsignedValue; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(uint UnsignedValue) { return UnsignedValue; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(ulong UnsignedValue) { return UnsignedValue; }

#endif // defined(cl_khr_fp64)

// OpConvertPtrToU -> ptrtoint, no need.

short __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_ftobf_1(Value);
  // Round to nearest even, then truncate to the high 16 bits.
  if (__spirv_IsNan(Value))
    return 0xFFC1; // quiet NaN
  uint bits = as_uint(Value);
  uint bias = ((bits >> 16) & 0x1u) + 0x00007FFFu;
  return (short)((bits + bias) >> 16);
}

float __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_bftof_1(Value);
  return as_float((uint)as_ushort(Value) << 16);
}

short2 __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float2 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_ftobf_2(Value);
  return (short2)(__spirv_ConvertFToBF16INTEL(Value.s0), __spirv_ConvertFToBF16INTEL(Value.s1));
}

short3 __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float3 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_ftobf_3(Value);
  return (short3)(__spirv_ConvertFToBF16INTEL(Value.s0), __spirv_ConvertFToBF16INTEL(Value.s1),
                  __spirv_ConvertFToBF16INTEL(Value.s2));
}

short4 __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float4 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_ftobf_4(Value);
  return (short4)(__spirv_ConvertFToBF16INTEL(Value.s0), __spirv_ConvertFToBF16INTEL(Value.s1),
                  __spirv_ConvertFToBF16INTEL(Value.s2), __spirv_ConvertFToBF16INTEL(Value.s3));
}

short8 __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float8 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_ftobf_8(Value);
  return (short8)(__spirv_ConvertFToBF16INTEL(Value.s0), __spirv_ConvertFToBF16INTEL(Value.s1),
                  __spirv_ConvertFToBF16INTEL(Value.s2), __spirv_ConvertFToBF16INTEL(Value.s3),
                  __spirv_ConvertFToBF16INTEL(Value.s4), __spirv_ConvertFToBF16INTEL(Value.s5),
                  __spirv_ConvertFToBF16INTEL(Value.s6), __spirv_ConvertFToBF16INTEL(Value.s7));
}

short16 __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float16 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_ftobf_16(Value);
  return (short16)(__spirv_ConvertFToBF16INTEL(Value.s0), __spirv_ConvertFToBF16INTEL(Value.s1),
                   __spirv_ConvertFToBF16INTEL(Value.s2), __spirv_ConvertFToBF16INTEL(Value.s3),
                   __spirv_ConvertFToBF16INTEL(Value.s4), __spirv_ConvertFToBF16INTEL(Value.s5),
                   __spirv_ConvertFToBF16INTEL(Value.s6), __spirv_ConvertFToBF16INTEL(Value.s7),
                   __spirv_ConvertFToBF16INTEL(Value.s8), __spirv_ConvertFToBF16INTEL(Value.s9),
                   __spirv_ConvertFToBF16INTEL(Value.sa), __spirv_ConvertFToBF16INTEL(Value.sb),
                   __spirv_ConvertFToBF16INTEL(Value.sc), __spirv_ConvertFToBF16INTEL(Value.sd),
                   __spirv_ConvertFToBF16INTEL(Value.se), __spirv_ConvertFToBF16INTEL(Value.sf));
}

float2 __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short2 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_bftof_2(Value);
  return (float2)(__spirv_ConvertBF16ToFINTEL(Value.s0), __spirv_ConvertBF16ToFINTEL(Value.s1));
}

float3 __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short3 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_bftof_3(Value);
  return (float3)(__spirv_ConvertBF16ToFINTEL(Value.s0), __spirv_ConvertBF16ToFINTEL(Value.s1),
                  __spirv_ConvertBF16ToFINTEL(Value.s2));
}

float4 __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short4 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_bftof_4(Value);
  return (float4)(__spirv_ConvertBF16ToFINTEL(Value.s0), __spirv_ConvertBF16ToFINTEL(Value.s1),
                  __spirv_ConvertBF16ToFINTEL(Value.s2), __spirv_ConvertBF16ToFINTEL(Value.s3));
}

float8 __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short8 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_bftof_8(Value);
  return (float8)(__spirv_ConvertBF16ToFINTEL(Value.s0), __spirv_ConvertBF16ToFINTEL(Value.s1),
                  __spirv_ConvertBF16ToFINTEL(Value.s2), __spirv_ConvertBF16ToFINTEL(Value.s3),
                  __spirv_ConvertBF16ToFINTEL(Value.s4), __spirv_ConvertBF16ToFINTEL(Value.s5),
                  __spirv_ConvertBF16ToFINTEL(Value.s6), __spirv_ConvertBF16ToFINTEL(Value.s7));
}

float16 __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short16 Value) {
  if (BIF_FLAG_CTRL_GET(UseNativeBF16Conversion))
    return __builtin_IB_bftof_16(Value);
  return (float16)(__spirv_ConvertBF16ToFINTEL(Value.s0), __spirv_ConvertBF16ToFINTEL(Value.s1),
                   __spirv_ConvertBF16ToFINTEL(Value.s2), __spirv_ConvertBF16ToFINTEL(Value.s3),
                   __spirv_ConvertBF16ToFINTEL(Value.s4), __spirv_ConvertBF16ToFINTEL(Value.s5),
                   __spirv_ConvertBF16ToFINTEL(Value.s6), __spirv_ConvertBF16ToFINTEL(Value.s7),
                   __spirv_ConvertBF16ToFINTEL(Value.s8), __spirv_ConvertBF16ToFINTEL(Value.s9),
                   __spirv_ConvertBF16ToFINTEL(Value.sa), __spirv_ConvertBF16ToFINTEL(Value.sb),
                   __spirv_ConvertBF16ToFINTEL(Value.sc), __spirv_ConvertBF16ToFINTEL(Value.sd),
                   __spirv_ConvertBF16ToFINTEL(Value.se), __spirv_ConvertBF16ToFINTEL(Value.sf));
}

float __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float Value) { return __builtin_IB_ftotf32_1(Value); }

float2 __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float2 Value) { return __builtin_IB_ftotf32_2(Value); }

float3 __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float3 Value) { return __builtin_IB_ftotf32_3(Value); }

float4 __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float4 Value) { return __builtin_IB_ftotf32_4(Value); }

float8 __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float8 Value) { return __builtin_IB_ftotf32_8(Value); }

float16 __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float16 Value) {
  return __builtin_IB_ftotf32_16(Value);
}

/*
// Next is all Scalar types with Rounding modes [RTE,RTZ,RTN,RTP] and Sat
//
//
//
//
*/

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtz(half FloatValue) {
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat(half FloatValue) {
  return __clamp_sat_uchar_half(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtz(half FloatValue) {
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtz(half FloatValue) {
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat(half FloatValue) {
  return __clamp_sat_ushort_half(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtz(half FloatValue) {
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtz(half FloatValue) {
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat(half FloatValue) {
  return __clamp_sat_uint_half(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtz(half FloatValue) {
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtz(half FloatValue) {
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat(half FloatValue) {
  return __clamp_sat_ulong_half(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtz(half FloatValue) {
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtz(float FloatValue) {
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= UCHAR_MAX) {
    return UCHAR_MAX;
  }
  return (uchar)FloatValue;
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat(float FloatValue) {
  return __clamp_sat_uchar_float(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtz(float FloatValue) {
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtz(float FloatValue) {
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= USHRT_MAX) {
    return USHRT_MAX;
  }
  return (ushort)FloatValue;
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat(float FloatValue) {
  return __clamp_sat_ushort_float(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtz(float FloatValue) {
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtz(float FloatValue) {
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat(float FloatValue) {
  return __clamp_sat_uint_float(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtz(float FloatValue) {
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtz(float FloatValue) {
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat(float FloatValue) {
  return __clamp_sat_ulong_float(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtz(float FloatValue) {
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

#if defined(cl_khr_fp64)

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtz(double FloatValue) {
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat(double FloatValue) {
  return __clamp_sat_uchar_double(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat(double FloatValue) {
  return __clamp_sat_ushort_double(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat(double FloatValue) {
  return __clamp_sat_uint_double(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat(double FloatValue) {
  return __clamp_sat_char_double(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat(double FloatValue) {
  return __clamp_sat_short_double(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat(double FloatValue) {
  return __clamp_sat_int_double(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtz(double FloatValue) {
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtz(double FloatValue) {
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtz(double FloatValue) {
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtz(double FloatValue) {
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtz(double FloatValue) {
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtz(double FloatValue) {
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat(double FloatValue) {
  return __clamp_sat_ulong_double(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtz(double FloatValue) {
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

#endif // defined(cl_khr_fp64)

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtz(half FloatValue) {
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat(half FloatValue) {
  return __clamp_sat_char_half(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtz(half FloatValue) {
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtz(half FloatValue) {
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat(half FloatValue) {
  return __clamp_sat_short_half(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtz(half FloatValue) {
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtz(half FloatValue) {
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat(half FloatValue) {
  return __clamp_sat_int_half(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtz(half FloatValue) {
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtz(half FloatValue) {
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat(half FloatValue) {
  return __clamp_sat_long_half(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rte(half FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtz(half FloatValue) {
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtp(half FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtn(half FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtz(float FloatValue) {
  if (FloatValue <= CHAR_MIN) {
    return CHAR_MIN;
  } else if (FloatValue >= CHAR_MAX) {
    return CHAR_MAX;
  }
  return (char)FloatValue;
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat(float FloatValue) {
  return __clamp_sat_char_float(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtz(float FloatValue) {
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtz(float FloatValue) {
  if (FloatValue <= SHRT_MIN) {
    return SHRT_MIN;
  } else if (FloatValue >= SHRT_MAX) {
    return SHRT_MAX;
  }
  return (short)FloatValue;
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat(float FloatValue) {
  return __clamp_sat_short_float(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtz(float FloatValue) {
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtz(float FloatValue) {
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat(float FloatValue) {
  return __clamp_sat_int_float(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtz(float FloatValue) {
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtz(float FloatValue) {
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat(float FloatValue) {
  return __clamp_sat_long_float(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rte(float FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtz(float FloatValue) {
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtp(float FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtn(float FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

#if defined(cl_khr_fp64)

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtz(double FloatValue) {
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtz(double FloatValue) {
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtz(double FloatValue) {
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtz(double FloatValue) {
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtz(double FloatValue) {
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtz(double FloatValue) {
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtz(double FloatValue) {
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat(double FloatValue) {
  return __clamp_sat_long_double(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rte(double FloatValue) {
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtz(double FloatValue) {
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtp(double FloatValue) {
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtn(double FloatValue) {
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rte(char SignedValue) { return SignedValue; }

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtz(char SignedValue) {
  return SignedValue; // A half can represent a char exactly.
}

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtp(char SignedValue) {
  return SignedValue; // A half can represent a char exactly.
}

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtn(char SignedValue) {
  return SignedValue; // A half can represent a char exactly.
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(char SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(char SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(char SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(char SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(char SignedValue) { return SignedValue; }

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(char SignedValue) {
  return SignedValue; // A double can represent a char exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(char SignedValue) {
  return SignedValue; // A double can represent a char exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(char SignedValue) {
  return SignedValue; // A double can represent a char exactly.
}

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rte(short SignedValue) { return SignedValue; }

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtz(short SignedValue) {
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtp(short SignedValue) {
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtn(short SignedValue) {
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(short SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(short SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(short SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(short SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(short SignedValue) { return SignedValue; }

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(short SignedValue) {
  return SignedValue; // A double can represent a short exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(short SignedValue) {
  return SignedValue; // A double can represent a short exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(short SignedValue) {
  return SignedValue; // A double can represent a short exactly.
}

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rte(int SignedValue) { return SignedValue; }

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtz(int SignedValue) {
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtp(int SignedValue) {
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtn(int SignedValue) {
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(int SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(int SignedValue) {
  return __builtin_IB_itof_rtz(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(int SignedValue) {
  return __builtin_IB_itof_rtp(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(int SignedValue) {
  return __builtin_IB_itof_rtn(SignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(int SignedValue) { return SignedValue; }

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(int SignedValue) {
  return SignedValue; // A double can represent an int exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(int SignedValue) {
  return SignedValue; // A double can represent an int exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(int SignedValue) {
  return SignedValue; // A double can represent an int exactly.
}

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rte(long SignedValue) { return SignedValue; }

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtz(long SignedValue) {
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtp(long SignedValue) {
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtn(long SignedValue) {
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(long SignedValue) {
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(long SignedValue) {
  return convertSItoFP32(SignedValue, 3);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(long SignedValue) {
  return convertSItoFP32(SignedValue, 1);
}

float __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(long SignedValue) {
  return convertSItoFP32(SignedValue, 2);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(long SignedValue) { return SignedValue; }

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(long SignedValue) {
  if (BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin)) {
    return __builtin_IB_itofp64_rtz(SignedValue);
  } else {
    int hi = SignedValue >> 32;
    unsigned int lo = SignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double)lo, o_hi = (double)hi, o_c = (double)c;
    // o_c * o_hi + o_lo, using round-to-zero
    return __builtin_IB_fma_rtz_f64(o_c, o_hi, o_lo);
  }
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(long SignedValue) {
  if (BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin)) {
    return __builtin_IB_itofp64_rtp(SignedValue);
  } else {
    int hi = SignedValue >> 32;
    unsigned int lo = SignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double)lo, o_hi = (double)hi, o_c = (double)c;
    // o_c * o_hi + o_lo, using round-to-positive-infinity
    return __builtin_IB_fma_rtp_f64(o_c, o_hi, o_lo);
  }
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(long SignedValue) {
  if (BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin)) {
    return __builtin_IB_itofp64_rtn(SignedValue);
  } else {
    int hi = SignedValue >> 32;
    unsigned int lo = SignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double)lo, o_hi = (double)hi, o_c = (double)c;
    // o_c * o_hi + o_lo, using round-to-negative-infinity
    return __builtin_IB_fma_rtn_f64(o_c, o_hi, o_lo);
  }
}

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rte(uchar UnsignedValue) { return UnsignedValue; }

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtz(uchar UnsignedValue) {
  return UnsignedValue; // A half can represent a uchar exactly.
}

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtp(uchar UnsignedValue) { return UnsignedValue; }

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtn(uchar UnsignedValue) {
  return UnsignedValue; // A half can represent a uchar exactly.
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(uchar UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(uchar UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(uchar UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(uchar UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(uchar UnsignedValue) { return UnsignedValue; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(uchar UnsignedValue) {
  return UnsignedValue; // A double can represent a uchar exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(uchar UnsignedValue) { return UnsignedValue; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(uchar UnsignedValue) {
  return UnsignedValue; // A double can represent a uchar exactly.
}

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rte(ushort UnsignedValue) { return UnsignedValue; }

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtz(ushort UnsignedValue) {
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtp(ushort UnsignedValue) {
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtn(ushort UnsignedValue) {
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(ushort UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(ushort UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(ushort UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(ushort UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(ushort UnsignedValue) { return UnsignedValue; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(ushort UnsignedValue) {
  return UnsignedValue; // A double can represent a ushort exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(ushort UnsignedValue) {
  return UnsignedValue; // A double can represent a ushort exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(ushort UnsignedValue) {
  return UnsignedValue; // A double can represent a ushort exactly.
}

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rte(uint UnsignedValue) { return UnsignedValue; }

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtz(uint UnsignedValue) {
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtp(uint UnsignedValue) {
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtn(uint UnsignedValue) {
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(uint UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(uint UnsignedValue) {
  return __builtin_IB_uitof_rtz(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(uint UnsignedValue) {
  return __builtin_IB_uitof_rtp(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(uint UnsignedValue) {
  return __builtin_IB_uitof_rtn(UnsignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(uint UnsignedValue) { return UnsignedValue; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(uint UnsignedValue) {
  return UnsignedValue; // A double can represent a uint exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(uint UnsignedValue) {
  return UnsignedValue; // A double can represent a uint exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(uint UnsignedValue) {
  return UnsignedValue; // A double can represent a uint exactly.
}

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rte(ulong UnsignedValue) { return UnsignedValue; }

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtz(ulong UnsignedValue) {
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtp(ulong UnsignedValue) {
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtn(ulong UnsignedValue) {
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(ulong UnsignedValue) {
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(ulong UnsignedValue) {
  return convertUItoFP32(UnsignedValue, 3, false);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(ulong UnsignedValue) {
  return convertUItoFP32(UnsignedValue, 1, false);
}

float __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(ulong UnsignedValue) {
  return convertUItoFP32(UnsignedValue, 2, false);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(ulong UnsignedValue) { return UnsignedValue; }

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(ulong UnsignedValue) {
  if (BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin)) {
    return __builtin_IB_uitofp64_rtz(UnsignedValue);
  } else {
    unsigned int hi = UnsignedValue >> 32;
    unsigned int lo = UnsignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double)lo, o_hi = (double)hi, o_c = (double)c;
    // o_c * o_hi + o_lo, using round-to-zero
    return __builtin_IB_fma_rtz_f64(o_c, o_hi, o_lo);
  }
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(ulong UnsignedValue) {
  if (BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin)) {
    return __builtin_IB_uitofp64_rtp(UnsignedValue);
  } else {
    unsigned int hi = UnsignedValue >> 32;
    unsigned int lo = UnsignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double)lo, o_hi = (double)hi, o_c = (double)c;
    // o_c * o_hi + o_lo, using round-to-positive-infinity
    return __builtin_IB_fma_rtp_f64(o_c, o_hi, o_lo);
  }
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(ulong UnsignedValue) {
  if (BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin)) {
    return __builtin_IB_uitofp64_rtn(UnsignedValue);
  } else {
    unsigned int hi = UnsignedValue >> 32;
    unsigned int lo = UnsignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double)lo, o_hi = (double)hi, o_c = (double)c;
    // o_c * o_hi + o_lo, using round-to-negative-infinity
    return __builtin_IB_fma_rtn_f64(o_c, o_hi, o_lo);
  }
}

#endif // defined(cl_khr_fp64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat(ConvertSToF, half, f16)
    GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat(ConvertSToF, float, f32)
#if defined(cl_khr_fp64)
        GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat(ConvertSToF, double, f64)
#endif

            GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat(ConvertUToF, half, f16)
                GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat(ConvertUToF, float, f32)
#if defined(cl_khr_fp64)
                    GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat(ConvertUToF, double, f64)
#endif

                        GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger(ConvertFToS, char, i8)
                            GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger(ConvertFToS, short, i16)
                                GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger(ConvertFToS, int, i32)
                                    GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger(ConvertFToS, long, i64)

                                        GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger(ConvertFToU, uchar, i8)
                                            GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger(ConvertFToU, ushort,
                                                                                                    i16)
                                                GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger(ConvertFToU,
                                                                                                        uint, i32)
                                                    GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger(ConvertFToU,
                                                                                                            ulong, i64)

                                                        float convertUItoFP32(ulong value, char roundingMode, bool s) {
  uint Hi = (uint)(value >> 32);
  uint Lo = (uint)(value & 0x00000000FFFFFFFF);
  float Res_Rounded;
  float NewValue;
  uint ShiftAmount = __spirv_ocl_clz((int)Hi);

  // For rtn and rtp we need to switch the rounding mode if
  // the sign bit is negative in order get the correct magnitude

  if (roundingMode == 1 && s) {
    roundingMode = 2;
  } else if (roundingMode == 2 && s) {
    roundingMode = 1;
  }

  if (Hi != 0) {
    uint InnerResHi = Hi;
    uint InnerResLo = Lo;
    if (ShiftAmount != 0) {
      uint L, H;
      L = Lo << ShiftAmount;
      H = (Hi << ShiftAmount) | (Lo >> -ShiftAmount);
      InnerResHi = H;
      InnerResLo = L;
    }

    switch (roundingMode) {
    case 0: // rte (same as default)
      Res_Rounded = InnerResLo ? (float)(InnerResHi | 1) : (float)InnerResHi;
      break;
    case 1: // rtp
      if (InnerResLo != 0)
        Res_Rounded = __spirv_ConvertUToF_Rfloat_rtp(InnerResHi | 1);
      else
        Res_Rounded = __spirv_ConvertUToF_Rfloat_rtp(InnerResHi);
      break;
    case 2: // rtn
    case 3: // rtz
      Res_Rounded = __spirv_ConvertUToF_Rfloat_rtz(InnerResHi);
      break;
    default:
      Res_Rounded = InnerResLo ? (float)(InnerResHi | 1) : (float)InnerResHi;
    }
  } else {
    if (roundingMode == 1) // rtp
      Res_Rounded = __spirv_ConvertUToF_Rfloat_rtp(Lo);
    else if (roundingMode > 1) // rtn and rtz
      Res_Rounded = __spirv_ConvertUToF_Rfloat_rtz(Lo);
    else
      Res_Rounded = (float)Lo;
  }

  float exp = as_float((159 - ShiftAmount) << 23);
  NewValue = Res_Rounded * exp;
  return NewValue;
}

float convertSItoFP32(long value, char roundingMode) {
  uint Hi = (uint)(value >> 32);
  uint Lo = (uint)(value & 0x00000000FFFFFFFF);
  uint sign_bit = as_uint(as_int(Hi) >> 31);
  float NewValue;
  Lo = Lo ^ sign_bit;
  uint Hi_add = Hi ^ sign_bit;

  uint Lo_new = Lo - sign_bit;
  uint acc = Lo < sign_bit ? 1 : 0;
  Hi = Hi_add - sign_bit - acc;

  ulong combined = ((ulong)Hi << 32) | (ulong)Lo_new;
  if (sign_bit == 0) {
    NewValue = convertUItoFP32(combined, roundingMode, false);
  } else {
    NewValue = convertUItoFP32(combined, roundingMode, true);
  }

  sign_bit = sign_bit & 0x80000000;

  return as_float(as_uint(NewValue) | sign_bit);
}

// lfsr scalar
char __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char seed, char polynomial) {
  char4 seed4, poly4;
  seed4.x = seed;
  poly4.x = polynomial;
  uint result = __builtin_IB_lfsr_b8v4(as_uint(seed4), as_uint(poly4));
  return as_char4(result).x;
}
short __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short seed, short polynomial) {
  short2 seed2, poly2;
  seed2.x = seed;
  poly2.x = polynomial;
  uint result = __builtin_IB_lfsr_b16v2(as_uint(seed2), as_uint(poly2));
  return as_short2(result).x;
}
int __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int seed, int polynomial) {
  return __builtin_IB_lfsr_b32(seed, polynomial);
}
// lfsr vec2
char2 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char2 seed, char polynomial) {
  char4 seed4, poly4;
  seed4.xy = seed;
  poly4.xy = polynomial;
  uint result = __builtin_IB_lfsr_b8v4(as_uint(seed4), as_uint(poly4));
  return as_char4(result).xy;
}
short2 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short2 seed, short polynomial) {
  short2 poly = (short2)polynomial;
  uint result = __builtin_IB_lfsr_b16v2(as_uint(seed), as_uint(poly));
  return as_short2(result);
}
int2 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int2 seed, int polynomial) {
  uint2 poly = as_uint2((int2)polynomial);
  return as_int2(__builtin_IB_lfsr_b32_v2(as_uint2(seed), poly));
}
// lfsr vec3
char3 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char3 seed, char polynomial) {
  char4 seed4, poly4;
  seed4.xyz = seed;
  poly4.xyz = polynomial;
  uint result = __builtin_IB_lfsr_b8v4(as_uint(seed4), as_uint(poly4));
  return as_char4(result).xyz;
}
short3 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short3 seed, short polynomial) {
  short2 poly2 = (short2)polynomial;
  uint res_xy = __builtin_IB_lfsr_b16v2(as_uint(seed.xy), as_uint(poly2));
  short2 seed_z;
  seed_z.x = seed.z;
  uint res_z = __builtin_IB_lfsr_b16v2(as_uint(seed_z), as_uint(poly2));
  short3 result;
  result.xy = as_short2(res_xy);
  result.z = as_short2(res_z).x;
  return result;
}
int3 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int3 seed, int polynomial) {
  uint3 result;
  result.x = __builtin_IB_lfsr_b32(as_uint(seed.x), as_uint(polynomial));
  result.y = __builtin_IB_lfsr_b32(as_uint(seed.y), as_uint(polynomial));
  result.z = __builtin_IB_lfsr_b32(as_uint(seed.z), as_uint(polynomial));
  return as_int3(result);
}
// lfsr vec4
char4 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char4 seed, char polynomial) {
  char4 poly = (char4)polynomial;
  return as_char4(__builtin_IB_lfsr_b8v4(as_uint(seed), as_uint(poly)));
}
short4 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short4 seed, short polynomial) {
  short4 poly4 = (short4)polynomial;
  uint2 poly = as_uint2(poly4);
  return as_short4(__builtin_IB_lfsr_b16v2_v2(as_uint2(seed), poly));
}
int4 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int4 seed, int polynomial) {
  uint4 poly = as_uint4((int4)polynomial);
  return as_int4(__builtin_IB_lfsr_b32_v4(as_uint4(seed), poly));
}
// lfsr vec8
char8 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char8 seed, char polynomial) {
  char8 poly8 = (char8)polynomial;
  uint2 poly = as_uint2(poly8);
  return as_char8(__builtin_IB_lfsr_b8v4_v2(as_uint2(seed), poly));
}
short8 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short8 seed, short polynomial) {
  short8 poly8 = (short8)polynomial;
  uint4 poly = as_uint4(poly8);
  return as_short8(__builtin_IB_lfsr_b16v2_v4(as_uint4(seed), poly));
}
int8 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int8 seed, int polynomial) {
  uint8 poly = as_uint8((int8)polynomial);
  return as_int8(__builtin_IB_lfsr_b32_v8(as_uint8(seed), poly));
}
// lfsr vec16
char16 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char16 seed, char polynomial) {
  char16 poly16 = (char16)polynomial;
  uint4 poly = as_uint4(poly16);
  return as_char16(__builtin_IB_lfsr_b8v4_v4(as_uint4(seed), poly));
}
short16 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short16 seed, short polynomial) {
  short16 poly16 = (short16)polynomial;
  uint8 poly = as_uint8(poly16);
  return as_short16(__builtin_IB_lfsr_b16v2_v8(as_uint8(seed), poly));
}
int16 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int16 seed, int polynomial) {
  uint16 poly = as_uint16((int16)polynomial);
  return as_int16(__builtin_IB_lfsr_b32_v16(as_uint16(seed), poly));
}
