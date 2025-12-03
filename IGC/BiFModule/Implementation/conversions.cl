/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "../Headers/spirv.h"

#define RT_POS 0
#define RT_NEG 1
#define RT_Z   2
#define RT_NE  3

#if defined(cl_khr_fp64)
INLINE float __intel_convert_float_rtp_rtn(double a, uint direction);
#endif

/* Helper Functions from IBiF_Conversions.cl */

#ifdef __IGC_BUILD__
#define UCHAR_MIN ((uchar)0)
#define USHRT_MIN ((ushort)0)
#define UINT_MIN  ((uint)0)
#define ULONG_MIN ((ulong)0)
// Helper function for conversions with saturation
#define SAT_CLAMP(TO, TONAME, FROM, FROM_MNGL)                                \
static TO __clamp_sat_##TO##_##FROM(FROM _T)                                  \
{                                                                             \
  /* Produce 0 for NaN values */                                              \
  FROM NaNClamp = __spirv_IsNan(_T) ? 0 : _T;          \
  FROM MinClamp = __spirv_ocl_fmax(      \
                                        NaNClamp, (FROM)TONAME##_MIN);        \
  return (TO)__spirv_ocl_fmin(           \
                                        MinClamp, (FROM)TONAME##_MAX);        \
}
// We would love to use fmin/fmax clamping logic for all cases, as it yields
// better ISA. However, for some int <-> FP type pairs, IMAX value 2^n - 1
// cannot be represented exactly in the target FP type (never an issue for even
// IMIN values). When we clamp to IMAX, the backwards conversion to such int
// type will yield IMAX + 1 and result in integer overflow, which is
// technically UB for runtime values and practically - for compile-time known
// constants. Move the upper limit clamping to int type's realm for such cases.
#define SAT_CLAMP_INEXACT_MAX(TO, TONAME, FROM, FROM_MNGL)                    \
static TO __clamp_sat_##TO##_##FROM(FROM _T)                                  \
{                                                                             \
  FROM NaNClamp = __spirv_IsNan(_T) ? 0 : _T;          \
  FROM MinClamp = __spirv_ocl_fmax(      \
                                        NaNClamp, (FROM)TONAME##_MIN);        \
  return MinClamp >= (FROM)TONAME##_MAX ? TONAME##_MAX : (TO)MinClamp;        \
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
#define SAT_CLAMP_INF_ONLY_SIGNED(TO, TONAME, FROM, FROM_MNGL)                \
static TO __clamp_sat_##TO##_##FROM(FROM _T)                                  \
{                                                                             \
  FROM NaNClamp = __spirv_IsNan(_T) ? 0 : _T;          \
  TO MinClamp = _T == (FROM)-INFINITY ? TONAME##_MIN : (TO)NaNClamp;          \
  return _T == (FROM)INFINITY ? TONAME##_MAX : MinClamp;                      \
}
#define SAT_CLAMP_INF_ONLY_UNSIGNED(TO, TONAME, FROM, FROM_MNGL)              \
static TO __clamp_sat_##TO##_##FROM(FROM _T)                                  \
{                                                                             \
  FROM NaNClamp = __spirv_IsNan(_T) ? 0 : _T;          \
  /* For unsigned, we still need a regular check of lower limit*/             \
  TO MinClamp = (TO)__spirv_ocl_fmax(NaNClamp, 0);         \
  return _T == (FROM)INFINITY ? TONAME##_MAX : MinClamp;                      \
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
#endif //defined(cl_khr_fp16)
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
#endif //defined(cl_khr_fp64)

#endif //__IGC_BUILD__

static float convertUItoFP32(ulong value, char roundingMode, bool s);
static float convertSItoFP32(long value, char roundingMode);


 // Conversion Instructions

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar(half FloatValue)
{
    return FloatValue;
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort(half FloatValue)
{
    return FloatValue;
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint(half FloatValue)
{
    return FloatValue;
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong(half FloatValue)
{
    return FloatValue;
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar(float FloatValue)
{
    return (uchar)FloatValue;
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort(float FloatValue)
{
    return (ushort)FloatValue;
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint(float FloatValue)
{
    return (uint)FloatValue;
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong(float FloatValue)
{
    float FC0 = __spirv_ocl_ldexp(1.0f, -32);
    float FC1 = __spirv_ocl_ldexp(-1.0f, 32);
    float HiF = __spirv_ocl_trunc(FloatValue * FC0);
    float LoF = __spirv_ocl_fma(HiF, FC1, FloatValue);
    ulong answer = 0;
    answer = (((uint)HiF | answer) << 32) | ((uint)LoF | answer);
    return answer;
}

#if defined(cl_khr_fp64)

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar(double FloatValue)
{
    return FloatValue;
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort(double FloatValue)
{
    return FloatValue;
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint(double FloatValue)
{
    return FloatValue;
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong(double FloatValue)
{
    return FloatValue;
}

#endif //defined(cl_khr_fp64)

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar(half FloatValue)
{
    return FloatValue;
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort(half FloatValue)
{
    return FloatValue;
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint(half FloatValue)
{
    return FloatValue;
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong(half FloatValue)
{
    return FloatValue;
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar(float FloatValue)
{
    return (char)FloatValue;
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort(float FloatValue)
{
    return (short)FloatValue;
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint(float FloatValue)
{
    return (int)FloatValue;
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong(float FloatValue)
{
    float abs_value = __spirv_ocl_fabs(FloatValue);
    if (abs_value < 1.0f) //for small numbers and denormals
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
    if (sign == -1 && Lo == -1)
    {
        Hi = Hi + 1;
    }
    return as_long(((ulong)Hi << 32) | (ulong)Lo_new);
}

#if defined(cl_khr_fp64)

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar(double FloatValue)
{
    return FloatValue;
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort(double FloatValue)
{
    return FloatValue;
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint(double FloatValue)
{
    return FloatValue;
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong(double FloatValue)
{
    return FloatValue;
}

#endif //defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf(char SignedValue)
{
    return SignedValue;
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(char SignedValue)
{
    return (float)SignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf(short SignedValue)
{
    return SignedValue;
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(short SignedValue)
{
    return (float)SignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf(int SignedValue)
{
    return SignedValue;
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(int SignedValue)
{
    return (float)SignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf(long SignedValue)
{
    return SignedValue;
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat(long SignedValue)
{
    return convertSItoFP32(SignedValue, 0);
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf(uchar UnsignedValue)
{
    return UnsignedValue;
}
float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(bool BoolValue)
{
    return BoolValue ? 1.0 : 0.0;
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(uchar UnsignedValue)
{
    return (float)UnsignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf(ushort UnsignedValue)
{
    return UnsignedValue;
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(ushort UnsignedValue)
{
    return (float)UnsignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf(uint UnsignedValue)
{
    return UnsignedValue;
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(uint UnsignedValue)
{
    return (float)UnsignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf(ulong UnsignedValue)
{
    return UnsignedValue;
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat(ulong UnsignedValue)
{
    return convertUItoFP32(UnsignedValue, 0, false);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(char SignedValue)
{
    return SignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(short SignedValue)
{
    return SignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(int SignedValue)
{
    return SignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble(long SignedValue)
{
    return SignedValue;
}

double  __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(bool BoolValue)
{
    return BoolValue ? 1.0 : 0.0;
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(uchar UnsignedValue)
{
    return UnsignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(ushort UnsignedValue)
{
    return UnsignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(uint UnsignedValue)
{
    return UnsignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble(ulong UnsignedValue)
{
    return UnsignedValue;
}

#endif // defined(cl_khr_fp64)


uchar  __attribute__((overloadable)) __spirv_UConvert_Ruchar(uchar UnsignedValue)
{
    return UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(uchar UnsignedValue)
{
    return (ushort)UnsignedValue;
}

uint   __attribute__((overloadable)) __spirv_UConvert_Ruint(uchar UnsignedValue)
{
    return (uint)UnsignedValue;
}

ulong  __attribute__((overloadable)) __spirv_UConvert_Rulong(uchar UnsignedValue)
{
    return (ulong)UnsignedValue;
}

uchar  __attribute__((overloadable)) __spirv_UConvert_Ruchar(ushort UnsignedValue)
{
    return (uchar)UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(ushort UnsignedValue)
{
    return UnsignedValue;
}

uint   __attribute__((overloadable)) __spirv_UConvert_Ruint(ushort UnsignedValue)
{
    return (uint)UnsignedValue;
}

ulong  __attribute__((overloadable)) __spirv_UConvert_Rulong(ushort UnsignedValue)
{
    return (ulong)UnsignedValue;
}

uchar  __attribute__((overloadable)) __spirv_UConvert_Ruchar(uint UnsignedValue)
{
    return (uchar)UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(uint UnsignedValue)
{
    return (ushort)UnsignedValue;
}

uint   __attribute__((overloadable)) __spirv_UConvert_Ruint(uint UnsignedValue)
{
    return UnsignedValue;
}

ulong  __attribute__((overloadable)) __spirv_UConvert_Rulong(uint UnsignedValue)
{
    return (ulong)UnsignedValue;
}

uchar  __attribute__((overloadable)) __spirv_UConvert_Ruchar(ulong UnsignedValue)
{
    return (uchar)UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort(ulong UnsignedValue)
{
    return (ushort)UnsignedValue;
}

uint   __attribute__((overloadable)) __spirv_UConvert_Ruint(ulong UnsignedValue)
{
    return (uint)UnsignedValue;
}

ulong  __attribute__((overloadable)) __spirv_UConvert_Rulong(ulong UnsignedValue)
{
    return UnsignedValue;
}

char  __attribute__((overloadable)) __spirv_SConvert_Rchar(char SignedValue)
{
    return SignedValue;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort(char SignedValue)
{
    return (short)SignedValue;
}

int   __attribute__((overloadable)) __spirv_SConvert_Rint(char SignedValue)
{
    return (int)SignedValue;
}

long  __attribute__((overloadable)) __spirv_SConvert_Rlong(char SignedValue)
{
    return (long)SignedValue;
}

char  __attribute__((overloadable)) __spirv_SConvert_Rchar(short SignedValue)
{
    return (char)SignedValue;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort(short SignedValue)
{
    return SignedValue;
}

int   __attribute__((overloadable)) __spirv_SConvert_Rint(short SignedValue)
{
    return (int)SignedValue;
}

long  __attribute__((overloadable)) __spirv_SConvert_Rlong(short SignedValue)
{
    return (long)SignedValue;
}

char  __attribute__((overloadable)) __spirv_SConvert_Rchar(int SignedValue)
{
    return (char)SignedValue;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort(int SignedValue)
{
    return (short)SignedValue;
}

int   __attribute__((overloadable)) __spirv_SConvert_Rint(int SignedValue)
{
    return SignedValue;
}

long  __attribute__((overloadable)) __spirv_SConvert_Rlong(int SignedValue)
{
    return (long)SignedValue;
}

char  __attribute__((overloadable)) __spirv_SConvert_Rchar(long SignedValue)
{
    return (char)SignedValue;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort(long SignedValue)
{
    return (short)SignedValue;
}

int   __attribute__((overloadable)) __spirv_SConvert_Rint(long SignedValue)
{
    return (int)SignedValue;
}

long  __attribute__((overloadable)) __spirv_SConvert_Rlong(long SignedValue)
{
    return SignedValue;
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf(half FloatValue)
{
    return FloatValue;
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat(half FloatValue)
{
    return FloatValue;
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf(float FloatValue)
{
    return FloatValue;
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat(float FloatValue)
{
    return FloatValue;
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_FConvert_Rdouble(half FloatValue)
{
    return FloatValue;
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble(float FloatValue)
{
    return FloatValue;
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf(double FloatValue)
{
  double a = FloatValue;
  ushort hp = (as_ulong(a) >> (DOUBLE_BITS - HALF_BITS)) & HALF_SIGN_MASK;
  double abs_a = as_double((as_ulong(a) & ~DOUBLE_SIGN_MASK));

  // +/- nan
  if (abs_a != abs_a)
  {
    hp = hp | (as_ulong(abs_a) >> (DOUBLE_BITS - HALF_BITS));
    hp = hp | HALF_MANTISSA_MASK; // Lose and QNANs
  }
  // Overflow
  else if (abs_a >= 0x1.ffep+15)  // max half RTE
  {
    hp = hp | 0x7c00; // exp := 0x1f
  }
  // Underflow
  else if (abs_a <= 0x1p-25)      // min half RTE
  {
    // nothing
  }
  else if (abs_a < 0x1p-24)       // smallest, non-zero exact half
  {
    // Very small denormal
    hp = hp | 0x0001;
  }
  else if (abs_a < 0x1p-14)       // larger half denormals
  {
    // Shift implicit bit into fract component of a half
    hp = hp | as_ulong(abs_a * 0x1p-1050);
  }
  else
  {
    a = a * 0x1p+42;
    a = as_double(as_ulong(a) & DOUBLE_EXPONENT_MASK);
    abs_a = abs_a + a;
    a = abs_a - a;

    a = a * 0x1p-1008;
    hp = hp | (as_ulong(a) >> (DOUBLE_MANTISSA_BITS - HALF_MANTISSA_BITS));
  }

  return as_half(hp);
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat(double FloatValue)
{
    return FloatValue;
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble(double FloatValue)
{
    return FloatValue;
}

#endif // defined(cl_khr_fp64)

// OpConvertPtrToU -> ptrtoint, no need.

uchar  __attribute__((overloadable)) __spirv_SatConvertSToU_Ruchar(char SignedValue)
{
      //return __builtin_IB_ctouc_sat((char)SignedValue);
      if (SignedValue <= 0)
      {
        return (uchar)0;
      }
      return (uchar)SignedValue;
}

ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(char SignedValue)
{
      //return __builtin_IB_ctous_sat((char)SignedValue);
      if( SignedValue <= 0)
      {
        return 0;
      }
      return (ushort)SignedValue;
}

uint  __attribute__((overloadable)) __spirv_SatConvertSToU_Ruint(char SignedValue)
{
      //return __builtin_IB_ctoui_sat((char)SignedValue);
      return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong  __attribute__((overloadable)) __spirv_SatConvertSToU_Rulong(char SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar  __attribute__((overloadable)) __spirv_SatConvertSToU_Ruchar(short SignedValue)
{
  return (uchar)clamp(SignedValue, (short)0, (short)UCHAR_MAX);
}

ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(short SignedValue)
{
      //return __builtin_IB_stous_sat((short)SignedValue);
      if( SignedValue <= 0)
      {
        return 0;
      }
      return (ushort)SignedValue;
}

uint   __attribute__((overloadable)) __spirv_SatConvertSToU_Ruint(short SignedValue)
{
      //return __builtin_IB_stoui_sat((short)SignedValue);
      return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong  __attribute__((overloadable)) __spirv_SatConvertSToU_Rulong(short SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar  __attribute__((overloadable)) __spirv_SatConvertSToU_Ruchar(int SignedValue)
{
  return (uchar)clamp(SignedValue, 0, (int)UCHAR_MAX);
}

ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(int SignedValue)
{
      //return __builtin_IB_itous_sat((int)SignedValue);
      int res = __spirv_ocl_s_clamp(SignedValue, 0, (int)USHRT_MAX);
      return (ushort)res;
}

uint   __attribute__((overloadable)) __spirv_SatConvertSToU_Ruint(int SignedValue)
{
      //return __builtin_IB_itoui_sat((int)SignedValue);
      return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong  __attribute__((overloadable)) __spirv_SatConvertSToU_Rulong(int SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar  __attribute__((overloadable)) __spirv_SatConvertSToU_Ruchar(long SignedValue)
{
    if (SignedValue <= 0)
      {
        return 0;
      }
    else if (SignedValue >= UCHAR_MAX)
      {
        return UCHAR_MAX;
      }
      return (uchar)SignedValue;
}

ushort __attribute__((overloadable)) __spirv_SatConvertSToU_Rushort(long SignedValue)
{
    if (SignedValue <= 0) {
        return 0;
      }
    else if (SignedValue >= USHRT_MAX) {
        return USHRT_MAX;
      }
      return (ushort)SignedValue;
}

uint   __attribute__((overloadable)) __spirv_SatConvertSToU_Ruint(long SignedValue)
{
    if (SignedValue <= 0) {
        return 0;
      }
    else if (SignedValue >= UINT_MAX) {
        return UINT_MAX;
      }
      return (uint)SignedValue;
}

ulong  __attribute__((overloadable)) __spirv_SatConvertSToU_Rulong(long SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}


char  __attribute__((overloadable)) __spirv_SatConvertUToS_Rchar(uchar UnsignedValue)
{
      //return __builtin_IB_uctoc_sat((uchar)UnsignedValue);
      return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __attribute__((overloadable)) __spirv_SatConvertUToS_Rshort(uchar UnsignedValue)
{
    return (short)UnsignedValue;
}

int   __attribute__((overloadable)) __spirv_SatConvertUToS_Rint(uchar UnsignedValue)
{
    return (int)UnsignedValue;
}

long  __attribute__((overloadable)) __spirv_SatConvertUToS_Rlong(uchar UnsignedValue)
{
    return (long)UnsignedValue;
}

char  __attribute__((overloadable)) __spirv_SatConvertUToS_Rchar(ushort UnsignedValue)
{
      //return __builtin_IB_ustoc_sat((ushort)UnsignedValue);
      return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __attribute__((overloadable)) __spirv_SatConvertUToS_Rshort(ushort UnsignedValue)
{
      //return __builtin_IB_ustos_sat((ushort)UnsignedValue);
      return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (ushort)UnsignedValue;
}

int   __attribute__((overloadable)) __spirv_SatConvertUToS_Rint(ushort UnsignedValue)
{
    return (int)UnsignedValue;
}

long  __attribute__((overloadable)) __spirv_SatConvertUToS_Rlong(ushort UnsignedValue)
{
    return (long)UnsignedValue;
}

char  __attribute__((overloadable)) __spirv_SatConvertUToS_Rchar(uint UnsignedValue)
{
      //return __builtin_IB_uitoc_sat((uint)UnsignedValue);
      return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __attribute__((overloadable)) __spirv_SatConvertUToS_Rshort(uint UnsignedValue)
{
      //return __builtin_IB_uitos_sat((uint)UnsignedValue);
      return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (short)UnsignedValue;
}

int __attribute__((overloadable)) __spirv_SatConvertUToS_Rint(uint UnsignedValue)
{
      //return __builtin_IB_uitoi_sat((uint)UnsignedValue);
      return (UnsignedValue >= INT_MAX) ? INT_MAX : (int)UnsignedValue;
}

long  __attribute__((overloadable)) __spirv_SatConvertUToS_Rlong(uint UnsignedValue)
{
    return (long)UnsignedValue;
}

char  __attribute__((overloadable)) __spirv_SatConvertUToS_Rchar(ulong UnsignedValue)
{
    return (UnsignedValue >= CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __attribute__((overloadable)) __spirv_SatConvertUToS_Rshort(ulong UnsignedValue)
{
    return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (short)UnsignedValue;
}

int   __attribute__((overloadable)) __spirv_SatConvertUToS_Rint(ulong UnsignedValue)
{
    return (UnsignedValue >= INT_MAX) ? INT_MAX : (int)UnsignedValue;
}

long  __attribute__((overloadable)) __spirv_SatConvertUToS_Rlong(ulong UnsignedValue)
{
    return (UnsignedValue >= LONG_MAX) ? LONG_MAX : __spirv_SConvert_Rlong((long)UnsignedValue);
}

short  __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float Value)
{
  return __builtin_IB_ftobf_1(Value);
}

short2  __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float2 Value)
{
  return __builtin_IB_ftobf_2(Value);
}

short3  __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float3 Value)
{
  return __builtin_IB_ftobf_3(Value);
}

short4  __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float4 Value)
{
  return __builtin_IB_ftobf_4(Value);
}

short8  __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float8 Value)
{
  return __builtin_IB_ftobf_8(Value);
}

short16  __attribute__((overloadable)) __spirv_ConvertFToBF16INTEL(float16 Value)
{
  return __builtin_IB_ftobf_16(Value);
}

float  __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short Value)
{
  return __builtin_IB_bftof_1(Value);
}

float2  __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short2 Value)
{
  return __builtin_IB_bftof_2(Value);
}

float3  __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short3 Value)
{
  return __builtin_IB_bftof_3(Value);
}

float4  __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short4 Value)
{
  return __builtin_IB_bftof_4(Value);
}

float8  __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short8 Value)
{
  return __builtin_IB_bftof_8(Value);
}

float16  __attribute__((overloadable)) __spirv_ConvertBF16ToFINTEL(short16 Value)
{
  return __builtin_IB_bftof_16(Value);
}

float  __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float Value)
{
  return __builtin_IB_ftotf32_1(Value);
}

float2  __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float2 Value)
{
  return __builtin_IB_ftotf32_2(Value);
}

float3  __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float3 Value)
{
  return __builtin_IB_ftotf32_3(Value);
}

float4  __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float4 Value)
{
  return __builtin_IB_ftotf32_4(Value);
}

float8  __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float8 Value)
{
  return __builtin_IB_ftotf32_8(Value);
}

float16  __attribute__((overloadable)) __spirv_RoundFToTF32INTEL(float16 Value)
{
  return __builtin_IB_ftotf32_16(Value);
}

/*
// Next is all Scalar types with Rounding modes [RTE,RTZ,RTN,RTP] and Sat
//
//
//
//
*/

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtz(half FloatValue)
{
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat(half FloatValue)
{
  return __clamp_sat_uchar_half(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtz(half FloatValue)
{
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtz(half FloatValue)
{
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat(half FloatValue)
{
  return __clamp_sat_ushort_half(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtz(half FloatValue)
{
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtz(half FloatValue)
{
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat(half FloatValue)
{
  return __clamp_sat_uint_half(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtz(half FloatValue)
{
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtz(half FloatValue)
{
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat(half FloatValue)
{
  return __clamp_sat_ulong_half(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtz(half FloatValue)
{
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint( FloatValue );
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtz(float FloatValue)
{
  if (FloatValue <= 0)
  {
    return 0;
  }
  else if (FloatValue >= UCHAR_MAX)
  {
    return UCHAR_MAX;
  }
  return (uchar)FloatValue;
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat(float FloatValue)
{
  return __clamp_sat_uchar_float(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtz(float FloatValue)
{
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtz(float FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= USHRT_MAX) {
    return USHRT_MAX;
  }
  return (ushort)FloatValue;
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat(float FloatValue)
{
  return __clamp_sat_ushort_float(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtz(float FloatValue)
{
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtz(float FloatValue)
{
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat(float FloatValue)
{
  return __clamp_sat_uint_float(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtz(float FloatValue)
{
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtz(float FloatValue)
{
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat(float FloatValue)
{
  return __clamp_sat_ulong_float(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtz(float FloatValue)
{
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

#if defined(cl_khr_fp64)

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtz(double FloatValue)
{
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat(double FloatValue)
{
  return __clamp_sat_uchar_double(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat(double FloatValue)
{
  return __clamp_sat_ushort_double(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat(double FloatValue)
{
  return __clamp_sat_uint_double(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat(double FloatValue)
{
  return __clamp_sat_char_double(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat(double FloatValue)
{
  return __clamp_sat_short_double(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat(double FloatValue)
{
  return __clamp_sat_int_double(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtz(double FloatValue)
{
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

uchar  __attribute__((overloadable)) __spirv_ConvertFToU_Ruchar_sat_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruchar_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtz(double FloatValue)
{
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtz(double FloatValue)
{
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

ushort __attribute__((overloadable)) __spirv_ConvertFToU_Rushort_sat_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rushort_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtz(double FloatValue)
{
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtz(double FloatValue)
{
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

uint   __attribute__((overloadable)) __spirv_ConvertFToU_Ruint_sat_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Ruint_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtz(double FloatValue)
{
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat(double FloatValue)
{
  return __clamp_sat_ulong_double(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtz(double FloatValue)
{
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

ulong  __attribute__((overloadable)) __spirv_ConvertFToU_Rulong_sat_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToU_Rulong_sat(FloatValue);
}

#endif // defined(cl_khr_fp64)

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtz(half FloatValue)
{
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat(half FloatValue)
{
  return __clamp_sat_char_half(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtz(half FloatValue)
{
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtz(half FloatValue)
{
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat(half FloatValue)
{
  return __clamp_sat_short_half(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtz(half FloatValue)
{
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtz(half FloatValue)
{
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat(half FloatValue)
{
  return __clamp_sat_int_half(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtz(half FloatValue)
{
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtz(half FloatValue)
{
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat(half FloatValue)
{
  return __clamp_sat_long_half(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rte(half FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtz(half FloatValue)
{
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtp(half FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtn(half FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtz(float FloatValue)
{
  if (FloatValue <= CHAR_MIN)
  {
    return CHAR_MIN;
  }
  else if (FloatValue >= CHAR_MAX)
  {
    return CHAR_MAX;
  }
  return (char)FloatValue;
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat(float FloatValue)
{
  return __clamp_sat_char_float(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtz(float FloatValue)
{
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtz(float FloatValue)
{
  if (FloatValue <= SHRT_MIN) {
    return SHRT_MIN;
  } else if (FloatValue >= SHRT_MAX) {
    return SHRT_MAX;
  }
  return (short)FloatValue;
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat(float FloatValue)
{
  return __clamp_sat_short_float(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtz(float FloatValue)
{
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtz(float FloatValue)
{
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat(float FloatValue)
{
  return __clamp_sat_int_float(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtz(float FloatValue)
{
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtz(float FloatValue)
{
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat(float FloatValue)
{
  return __clamp_sat_long_float(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rte(float FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtz(float FloatValue)
{
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtp(float FloatValue)
{
  FloatValue = __spirv_ocl_ceil( FloatValue );
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtn(float FloatValue)
{
  FloatValue = __spirv_ocl_floor( FloatValue );
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

#if defined(cl_khr_fp64)

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtz(double FloatValue)
{
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtz(double FloatValue)
{
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

char  __attribute__((overloadable)) __spirv_ConvertFToS_Rchar_sat_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rchar_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtz(double FloatValue)
{
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtz(double FloatValue)
{
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

short __attribute__((overloadable)) __spirv_ConvertFToS_Rshort_sat_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rshort_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtz(double FloatValue)
{
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtz(double FloatValue)
{
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

int   __attribute__((overloadable)) __spirv_ConvertFToS_Rint_sat_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rint_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtz(double FloatValue)
{
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat(double FloatValue)
{
  return __clamp_sat_long_double(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rte(double FloatValue)
{
  FloatValue = __spirv_ocl_rint(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtz(double FloatValue)
{
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtp(double FloatValue)
{
  FloatValue = __spirv_ocl_ceil(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

long  __attribute__((overloadable)) __spirv_ConvertFToS_Rlong_sat_rtn(double FloatValue)
{
  FloatValue = __spirv_ocl_floor(FloatValue);
  return __spirv_ConvertFToS_Rlong_sat(FloatValue);
}

#endif  // defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rte(char SignedValue)
{
  return SignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtz(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtp(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtn(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(char SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(char SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(char SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(char SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(char SignedValue)
{
  return SignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

#endif //defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rte(short SignedValue)
{
  return SignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtz(short SignedValue)
{
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtp(short SignedValue)
{
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtn(short SignedValue)
{
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(short SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(short SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(short SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(short SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(short SignedValue)
{
  return SignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

#endif  // defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rte(int SignedValue)
{
  return SignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtz(int SignedValue)
{
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtp(int SignedValue)
{
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtn(int SignedValue)
{
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(int SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(int SignedValue)
{
  return __builtin_IB_itof_rtz(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(int SignedValue)
{
  return __builtin_IB_itof_rtp(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(int SignedValue)
{
  return __builtin_IB_itof_rtn(SignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(int SignedValue)
{
  return SignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

#endif  // defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rte(long SignedValue)
{
  return SignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtz(long SignedValue)
{
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtp(long SignedValue)
{
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half   __attribute__((overloadable)) __spirv_ConvertSToF_Rhalf_rtn(long SignedValue)
{
  float f = __spirv_ConvertSToF_Rfloat(SignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rte(long SignedValue)
{
  return __spirv_ConvertSToF_Rfloat(SignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtz(long SignedValue)
{
  return convertSItoFP32(SignedValue, 3);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtp(long SignedValue)
{
  return convertSItoFP32(SignedValue, 1);
}

float  __attribute__((overloadable)) __spirv_ConvertSToF_Rfloat_rtn(long SignedValue)
{
  return convertSItoFP32(SignedValue, 2);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rte(long SignedValue)
{
  return SignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtz(long SignedValue)
{
  if(BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin))
  {
    return __builtin_IB_itofp64_rtz(SignedValue);
  }
  else
  {
    int hi = SignedValue >> 32;
    unsigned int lo = SignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double) lo,
           o_hi = (double) hi,
           o_c = (double) c;
    // o_c * o_hi + o_lo, using round-to-zero
    return __builtin_IB_fma_rtz_f64(o_c, o_hi, o_lo);
  }
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtp(long SignedValue)
{
  if(BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin))
  {
    return __builtin_IB_itofp64_rtp(SignedValue);
  }
  else
  {
    int hi = SignedValue >> 32;
    unsigned int lo = SignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double) lo,
           o_hi = (double) hi,
           o_c = (double) c;
    // o_c * o_hi + o_lo, using round-to-positive-infinity
    return __builtin_IB_fma_rtp_f64(o_c, o_hi, o_lo);
  }
}

double __attribute__((overloadable)) __spirv_ConvertSToF_Rdouble_rtn(long SignedValue)
{
  if(BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin))
  {
    return __builtin_IB_itofp64_rtn(SignedValue);
  }
  else
  {
    int hi = SignedValue >> 32;
    unsigned int lo = SignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double) lo,
           o_hi = (double) hi,
           o_c = (double) c;
    // o_c * o_hi + o_lo, using round-to-negative-infinity
    return __builtin_IB_fma_rtn_f64(o_c, o_hi, o_lo);
  }
}

#endif  // defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rte(uchar UnsignedValue)
{
  return UnsignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtz(uchar UnsignedValue)
{
  return UnsignedValue;    // A half can represent a uchar exactly.
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtp(uchar UnsignedValue)
{
  return UnsignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtn(uchar UnsignedValue)
{
  return UnsignedValue;    // A half can represent a uchar exactly.
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(uchar UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(uchar UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(uchar UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(uchar UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(uchar UnsignedValue)
{
  return UnsignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(uchar UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uchar exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(uchar UnsignedValue)
{
  return UnsignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(uchar UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uchar exactly.
}

#endif  // defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rte(ushort UnsignedValue)
{
  return UnsignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtz(ushort UnsignedValue)
{
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtp(ushort UnsignedValue)
{
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtn(ushort UnsignedValue)
{
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(ushort UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(ushort UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(ushort UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(ushort UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(ushort UnsignedValue)
{
  return UnsignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

#endif  // defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rte(uint UnsignedValue)
{
  return UnsignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtz(uint UnsignedValue)
{
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtp(uint UnsignedValue)
{
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtn(uint UnsignedValue)
{
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(uint UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtz(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtp(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtn(UnsignedValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(uint UnsignedValue)
{
  return UnsignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

#endif  // defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rte(ulong UnsignedValue)
{
  return UnsignedValue;
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtz(ulong UnsignedValue)
{
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtp(ulong UnsignedValue)
{
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half   __attribute__((overloadable)) __spirv_ConvertUToF_Rhalf_rtn(ulong UnsignedValue)
{
  float f = __spirv_ConvertUToF_Rfloat(UnsignedValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rte(ulong UnsignedValue)
{
  return __spirv_ConvertUToF_Rfloat(UnsignedValue);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtz(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 3, false);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtp(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 1, false);
}

float  __attribute__((overloadable)) __spirv_ConvertUToF_Rfloat_rtn(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 2, false);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rte(ulong UnsignedValue)
{
  return UnsignedValue;
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtz(ulong UnsignedValue)
{
  if(BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin))
  {
    return __builtin_IB_uitofp64_rtz(UnsignedValue);
  }
  else
  {
    unsigned int hi = UnsignedValue >> 32;
    unsigned int lo = UnsignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double) lo,
           o_hi = (double) hi,
           o_c = (double) c;
    // o_c * o_hi + o_lo, using round-to-zero
    return __builtin_IB_fma_rtz_f64(o_c, o_hi, o_lo);
  }
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtp(ulong UnsignedValue)
{
  if(BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin))
  {
    return __builtin_IB_uitofp64_rtp(UnsignedValue);
  }
  else
  {
    unsigned int hi = UnsignedValue >> 32;
    unsigned int lo = UnsignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double) lo,
           o_hi = (double) hi,
           o_c = (double) c;
    // o_c * o_hi + o_lo, using round-to-positive-infinity
    return __builtin_IB_fma_rtp_f64(o_c, o_hi, o_lo);
  }
}

double __attribute__((overloadable)) __spirv_ConvertUToF_Rdouble_rtn(ulong UnsignedValue)
{
  if(BIF_FLAG_CTRL_GET(UseNative64BitIntBuiltin))
  {
    return __builtin_IB_uitofp64_rtn(UnsignedValue);
  }
  else
  {
    unsigned int hi = UnsignedValue >> 32;
    unsigned int lo = UnsignedValue & 0xFFFFFFFF;
    long c = 1l << 32;

    double o_lo = (double) lo,
           o_hi = (double) hi,
           o_c = (double) c;
    // o_c * o_hi + o_lo, using round-to-negative-infinity
    return __builtin_IB_fma_rtn_f64(o_c, o_hi, o_lo);
  }
}

#endif  // defined(cl_khr_fp64)

uchar  __attribute__((overloadable)) __spirv_UConvert_Ruchar_sat(uchar UnsignedValue)
{
  return UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(uchar UnsignedValue)
{
  return UnsignedValue;
}

uint   __attribute__((overloadable)) __spirv_UConvert_Ruint_sat(uchar UnsignedValue)
{
  return (uint)UnsignedValue;
}

ulong  __attribute__((overloadable)) __spirv_UConvert_Rulong_sat(uchar UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  __attribute__((overloadable)) __spirv_UConvert_Ruchar_sat(ushort UnsignedValue)
{
      //return __builtin_IB_ustouc_sat((ushort)UnsignedValue);
      if (UnsignedValue > (uchar)0xff)
      {
        return (uchar)0xff;
      }
      return (uchar)UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(ushort UnsignedValue)
{
  return UnsignedValue;
}

uint   __attribute__((overloadable)) __spirv_UConvert_Ruint_sat(ushort UnsignedValue)
{
  return (uint)UnsignedValue;
}

ulong  __attribute__((overloadable)) __spirv_UConvert_Rulong_sat(ushort UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  __attribute__((overloadable)) __spirv_UConvert_Ruchar_sat(uint UnsignedValue)
{
      //return __builtin_IB_uitouc_sat((uint)UnsignedValue);
      if (UnsignedValue > UCHAR_MAX)
      {
        return UCHAR_MAX;
      }
      return (uchar)UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(uint UnsignedValue)
{
      if (UnsignedValue > (ushort)0xffff) {
        return (ushort)0xffff;
      }
      return (ushort)UnsignedValue;
}

uint   __attribute__((overloadable)) __spirv_UConvert_Ruint_sat(uint UnsignedValue)
{
  return UnsignedValue;
}

ulong  __attribute__((overloadable)) __spirv_UConvert_Rulong_sat(uint UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  __attribute__((overloadable)) __spirv_UConvert_Ruchar_sat(ulong UnsignedValue)
{
  return (UnsignedValue >= UCHAR_MAX) ? UCHAR_MAX : (uchar)UnsignedValue;
}

ushort __attribute__((overloadable)) __spirv_UConvert_Rushort_sat(ulong UnsignedValue)
{
  return (UnsignedValue >= USHRT_MAX) ? USHRT_MAX : (ushort)UnsignedValue;
}

uint   __attribute__((overloadable)) __spirv_UConvert_Ruint_sat(ulong UnsignedValue)
{
  return (UnsignedValue >= UINT_MAX) ? UINT_MAX : (uint)UnsignedValue;
}

ulong  __attribute__((overloadable)) __spirv_UConvert_Rulong_sat(ulong UnsignedValue)
{
  return UnsignedValue;
}

char  __attribute__((overloadable)) __spirv_SConvert_Rchar_sat(char SignedValue)
{
  return SignedValue;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort_sat(char SignedValue)
{
  return (short)SignedValue;
}

int   __attribute__((overloadable)) __spirv_SConvert_Rint_sat(char SignedValue)
{
  return (int)SignedValue;
}

long  __attribute__((overloadable)) __spirv_SConvert_Rlong_sat(char SignedValue)
{
  return __spirv_SConvert_Rlong(SignedValue);
}

char  __attribute__((overloadable)) __spirv_SConvert_Rchar_sat(short SignedValue)
{
      //return __builtin_IB_stoc_sat((short)SignedValue);
      short res = __spirv_ocl_s_clamp(SignedValue, (short)CHAR_MIN, (short)CHAR_MAX);
      return (char)res;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort_sat(short SignedValue)
{
  return SignedValue;
}

int   __attribute__((overloadable)) __spirv_SConvert_Rint_sat(short SignedValue)
{
  return (int)SignedValue;
}

long  __attribute__((overloadable)) __spirv_SConvert_Rlong_sat(short SignedValue)
{
  return __spirv_SConvert_Rlong(SignedValue);
}

char  __attribute__((overloadable)) __spirv_SConvert_Rchar_sat(int SignedValue)
{
      //return __builtin_IB_itoc_sat((int)SignedValue);
      int res = __spirv_ocl_s_clamp(SignedValue, (int)CHAR_MIN, (int)CHAR_MAX);
      return (char)res;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort_sat(int SignedValue)
{
      //return __builtin_IB_itos_sat((int)SignedValue);
      int res = __spirv_ocl_s_clamp(SignedValue, (int)SHRT_MIN, (int)SHRT_MAX);
      return (short)res;
}

int   __attribute__((overloadable)) __spirv_SConvert_Rint_sat(int SignedValue)
{
  return SignedValue;
}

long  __attribute__((overloadable)) __spirv_SConvert_Rlong_sat(int SignedValue)
{
  return __spirv_SConvert_Rlong(SignedValue);
}

char  __attribute__((overloadable)) __spirv_SConvert_Rchar_sat(long SignedValue)
{
    if (SignedValue <= CHAR_MIN)
    {
        return CHAR_MIN;
    }
    else if (SignedValue >= CHAR_MAX)
    {
        return CHAR_MAX;
    }
    return (char)SignedValue;
}

short __attribute__((overloadable)) __spirv_SConvert_Rshort_sat(long SignedValue)
{
    if (SignedValue <= SHRT_MIN)
    {
        return SHRT_MIN;
    }
    else if (SignedValue >= SHRT_MAX)
    {
        return SHRT_MAX;
    }
    return (short)SignedValue;
}

int   __attribute__((overloadable)) __spirv_SConvert_Rint_sat(long SignedValue)
{
    if (SignedValue <= INT_MIN)
    {
        return INT_MIN;
    }
    else if (SignedValue >= INT_MAX)
    {
        return INT_MAX;
    }
    return (int)SignedValue;
}

long  __attribute__((overloadable)) __spirv_SConvert_Rlong_sat(long SignedValue)
{
  return __spirv_SConvert_Rlong(SignedValue);
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rte(half FloatValue)
{
  return FloatValue;
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtz(half FloatValue)
{
  return FloatValue;
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtp(half FloatValue)
{
  return FloatValue;
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtn(half FloatValue)
{
  return FloatValue;
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rte(half FloatValue)
{
  return FloatValue;
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtz(half FloatValue)
{
  return FloatValue;
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtp(half FloatValue)
{
  return FloatValue;
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtn(half FloatValue)
{
  return FloatValue;
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rte(half FloatValue)
{
  return FloatValue;
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtz(half FloatValue)
{
  return FloatValue;
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtp(half FloatValue)
{
  return FloatValue;
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtn(half FloatValue)
{
  return FloatValue;
}

#endif // defined(cl_khr_fp64)

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rte(float FloatValue)
{
  return FloatValue;
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtz(float FloatValue)
{
  return __builtin_IB_ftoh_rtz(FloatValue);
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtp(float FloatValue)
{
  return __builtin_IB_ftoh_rtp(FloatValue);
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtn(float FloatValue)
{
  return __builtin_IB_ftoh_rtn(FloatValue);
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rte(float FloatValue)
{
  return __spirv_FConvert_Rfloat(FloatValue);
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtz(float FloatValue)
{
  return __spirv_FConvert_Rfloat(FloatValue);
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtp(float FloatValue)
{
  return __spirv_FConvert_Rfloat(FloatValue);
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtn(float FloatValue)
{
  return __spirv_FConvert_Rfloat(FloatValue);
}

#if defined(cl_khr_fp64)

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rte(float FloatValue)
{
  return FloatValue;
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtz(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtp(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtn(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rte(double FloatValue)
{
  return __spirv_FConvert_Rhalf(FloatValue);
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtz(double FloatValue)
{
  float f = __spirv_FConvert_Rfloat_rtz(FloatValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtp(double FloatValue)
{
  float f = __spirv_FConvert_Rfloat_rtp(FloatValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half   __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtn(double FloatValue)
{
  float f = __spirv_FConvert_Rfloat_rtn(FloatValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rte(double FloatValue)
{
  return FloatValue;
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtz(double FloatValue)
{
  double a = FloatValue;
  uint fp = (as_ulong(a) >> (DOUBLE_BITS - FLOAT_BITS)) & FLOAT_SIGN_MASK;  // sign bit
  double abs_a = as_double((as_ulong(a) & ~DOUBLE_SIGN_MASK));

  // +/- nan
  if (abs_a != abs_a)
  {
    fp = fp | (as_ulong(abs_a) >> (DOUBLE_BITS - FLOAT_BITS));
    fp = fp | FLOAT_MANTISSA_MASK;  // Lose any QNANs
  }
  else if (abs_a == (double)INFINITY)
  {
    fp = fp | as_uint(INFINITY);
  }
  // overflow
  else if( abs_a > 0x1.fffffep+127) // max float
  {
    fp = fp | as_uint(0x1.fffffep+127f);
  }
  // underflow
  else if (abs_a <= 0x1p-150)       // min float
  {
    // nothing
  }
  // denormal
  else if (abs_a < 0x1p-126)        // largest float, non-denormal
  {
    // Multiply/shift abs_a by 0x1p149 in order to represent the eventual
    // float denormal part as a double integer before truncating.
    uint* abs_a_ptr = (uint*)&abs_a;
    abs_a_ptr[1] = abs_a_ptr[1] + 0x09500000;         // abs_a = abs_a * 0x1p149
    int abs_fp = (int)abs_a;                          // truncate

    fp = fp | abs_fp;
  }
  else
  {
    ulong a2 = as_ulong(a);
    a2 = a2 & 0xFFFFFFFFE0000000ULL;    // Mask/truncate all usable bits of float type
    a2 = a2 - 0x3800000000000000ULL;    // Convert from double bias to float bias
    fp = fp | (uint)(a2 >> (DOUBLE_MANTISSA_BITS - FLOAT_MANTISSA_BITS));
  }

  return as_float(fp);
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtp(double FloatValue)
{
  return __intel_convert_float_rtp_rtn(FloatValue, RT_POS);
}

float  __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtn(double FloatValue)
{
  return __intel_convert_float_rtp_rtn(FloatValue, RT_NEG);
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rte(double FloatValue)
{
  return FloatValue;
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtz(double FloatValue)
{
  return FloatValue;
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtp(double FloatValue)
{
  return FloatValue;
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtn(double FloatValue)
{
  return FloatValue;
}

#endif // defined(cl_khr_fp64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( ConvertSToF, half, f16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( ConvertSToF, float, f32)
#if defined(cl_khr_fp64)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( ConvertSToF, double, f64)
#endif

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( ConvertUToF, half, f16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( ConvertUToF, float, f32)
#if defined(cl_khr_fp64)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( ConvertUToF, double, f64)
#endif

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( FConvert, half, f16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( FConvert, float, f32)
#if defined(cl_khr_fp64)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( FConvert, double, f64)
#endif

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed( SConvert, char, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed( SConvert, short, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed( SConvert, int, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed( SConvert, long, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned( UConvert, uchar, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned( UConvert, ushort, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned( UConvert, uint, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned( UConvert, ulong, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( ConvertFToS, char, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( ConvertFToS, short, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( ConvertFToS, int, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( ConvertFToS, long, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( ConvertFToU, uchar, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( ConvertFToU, ushort, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( ConvertFToU, uint, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( ConvertFToU, ulong, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned( SatConvertSToU, uchar, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned( SatConvertSToU, ushort, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned( SatConvertSToU, uint, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned( SatConvertSToU, ulong, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned( SatConvertUToS, char, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned( SatConvertUToS, short, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned( SatConvertUToS, int, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned( SatConvertUToS, long, i64)

#if defined(cl_khr_fp64)

INLINE float __intel_convert_float_rtp_rtn(double a, uint direction)
{
  uint fp = (as_ulong(a) >> (DOUBLE_BITS - FLOAT_BITS)) & FLOAT_SIGN_MASK;  // sign bit
  uint sign = fp >> (FLOAT_BITS - 1);
  double abs_a = as_double((as_ulong(a) & ~DOUBLE_SIGN_MASK));

  // +/- nan
  if (abs_a != abs_a)
  {
    fp = fp | (as_ulong(abs_a) >> (DOUBLE_BITS - FLOAT_BITS));
    fp = fp | FLOAT_MANTISSA_MASK;      // Lose any QNANs
  }
  // overflow
  else if( abs_a > 0x1.fffffep+127)     // max float
  {
    // overflow in the direction of the round?
    if (sign == direction)
    {
      fp = fp | as_uint(INFINITY);
    }
    else
    {
      if (abs_a == (double)INFINITY)
      {
        fp = fp | as_uint(INFINITY);
      }
      else
      {
        fp = fp | as_uint(0x1.fffffep+127f);
      }
    }
  }
  // underflow
  else if (abs_a <= 0x1p-150)    // min float
  {
    // non-zero
    if (as_ulong(abs_a) && (sign == direction))
    {
      // round away from zero
      fp = fp | 0x00000001;
    }
  }
  // denormal
  else if (abs_a < 0x1p-126)     // largest float, non-denormal
  {
    // Multiply/shift abs_a by 0x1p149 in order to represent the eventual
    // float denormal part as a double integer before truncating.
    uint* abs_a_ptr = (uint*)&abs_a;
    abs_a_ptr[1] = abs_a_ptr[1] + 0x09500000;         // abs_a = abs_a * 0x1p149
    int abs_fp = (int)abs_a;                          // truncate

    if (sign == direction)
    {
      // If any factional precision was lost in truncation, round away from
      // zero.
      abs_fp += (double)abs_fp != abs_a;
    }

    fp = fp | abs_fp;
  }
  else
  {
    ulong a2 = as_ulong(a);
    a2 = a2 & 0xFFFFFFFFE0000000ULL;                  // Mask/truncate all usable bits of float type
    if ((as_ulong(a) != a2) && (sign == direction))   // Losing any fractional component? And is negative?
    {
      a2 = a2 + 0x0000000020000000ULL;                //   yes, round float away from zero
    }
    a2 = a2 - 0x3800000000000000ULL;                  // Convert from double bias to float bias
    fp = fp | (uint)(a2 >> (DOUBLE_MANTISSA_BITS - FLOAT_MANTISSA_BITS));
  }

  return as_float(fp);
}

#endif // defined(cl_khr_fp64)


float convertUItoFP32(ulong value, char roundingMode, bool s)
{
    uint Hi = (uint)(value >> 32);
    uint Lo = (uint)(value & 0x00000000FFFFFFFF);
    float Res_Rounded;
    float NewValue;
    uint ShiftAmount = __spirv_ocl_clz((int)Hi);

    //For rtn and rtp we need to switch the rounding mode if
    //the sign bit is negative in order get the correct magnitude

    if (roundingMode == 1 && s )
    {
        roundingMode = 2;
    }
    else if (roundingMode == 2 && s )
    {
        roundingMode = 1;
    }

    if (Hi != 0)
    {
        uint InnerResHi = Hi;
        uint InnerResLo = Lo;
        if(ShiftAmount != 0)
        {
            uint L, H;
            L = Lo << ShiftAmount;
            H = (Hi << ShiftAmount) | (Lo >> -ShiftAmount);
            InnerResHi = H;
            InnerResLo = L;
        }

        switch(roundingMode)
        {
            case 0: //rte (same as default)
                Res_Rounded = InnerResLo ? (float)(InnerResHi | 1) : (float)InnerResHi;
                break;
            case 1: //rtp
                if (InnerResLo != 0)
                    Res_Rounded = __spirv_ConvertUToF_Rfloat_rtp(InnerResHi | 1);
                else
                    Res_Rounded = __spirv_ConvertUToF_Rfloat_rtp(InnerResHi);
                break;
            case 2: //rtn
            case 3: //rtz
                Res_Rounded = __spirv_ConvertUToF_Rfloat_rtz(InnerResHi);
                break;
            default:
                Res_Rounded = InnerResLo ? (float)(InnerResHi | 1) : (float)InnerResHi;
        }
    }
    else
    {
        if (roundingMode == 1) //rtp
            Res_Rounded = __spirv_ConvertUToF_Rfloat_rtp(Lo);
        else if (roundingMode > 1)  //rtn and rtz
            Res_Rounded = __spirv_ConvertUToF_Rfloat_rtz(Lo);
        else
            Res_Rounded = (float)Lo;
    }

    float exp = as_float((159 - ShiftAmount) << 23);
    NewValue = Res_Rounded * exp;
    return NewValue;
}


float convertSItoFP32(long value, char roundingMode)
{
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
    if (sign_bit == 0)
    {
        NewValue = convertUItoFP32(combined, roundingMode, false );
    }
    else
    {
        NewValue = convertUItoFP32(combined, roundingMode, true );
    }

    sign_bit = sign_bit & 0x80000000;

    return as_float(as_uint(NewValue) | sign_bit);
}

#if (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)
INLINE private void* __attribute__((overloadable)) __spirv_GenericCastToPtrExplicit_ToPrivate(generic char *Pointer, int Storage)
{
    return __builtin_IB_to_private(Pointer);
}

INLINE local   void* __attribute__((overloadable)) __spirv_GenericCastToPtrExplicit_ToLocal(generic char *Pointer, int Storage)
{
    return __builtin_IB_to_local(Pointer);
}

INLINE global  void* __attribute__((overloadable)) __spirv_GenericCastToPtrExplicit_ToGlobal(generic char *Pointer, int Storage)
{
    if((__spirv_GenericCastToPtrExplicit_ToLocal(Pointer, Storage) == NULL) &
       (__spirv_GenericCastToPtrExplicit_ToPrivate(Pointer, Storage) == NULL))
    {
        return (global void*)(Pointer);
    }

    return NULL;
}

INLINE uint __attribute__((overloadable)) __spirv_GenericPtrMemSemantics(generic char *Pointer)
{
    if (__spirv_GenericCastToPtrExplicit_ToLocal(Pointer, StorageWorkgroup) != NULL)
    {
        return (uint)WorkgroupMemory;
    }

    return (uint)CrossWorkgroupMemory;
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)

// lfsr scalar
char __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char seed, char polynomial) {
    return __builtin_IB_lfsr_b8v4_uchar(seed, polynomial);
}
short __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short seed, short polynomial) {
    return __builtin_IB_lfsr_b16v2_ushort(seed, polynomial);
}
int __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int seed, int polynomial) {
    return __builtin_IB_lfsr_b32(seed, polynomial);
}
long __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(long seed, long polynomial) {
    // unsigned long is needed for *zero extended* shift right
    ulong seed_r1 = as_ulong(seed) >> 1ul;
    if (seed & 1) seed_r1 ^= as_ulong(polynomial);
    return as_long(seed_r1);
}
// lfsr vec2
char2 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char2 seed, char polynomial) {
    char2 poly = {polynomial, polynomial};
    ushort result = __builtin_IB_lfsr_b8v4_ushort(as_ushort(seed), as_ushort(poly));
    return as_char2(result);
}
short2 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short2 seed, short polynomial) {
    short2 poly = {polynomial, polynomial};
    uint result = __builtin_IB_lfsr_b16v2(as_uint(seed), as_uint(poly));
    return as_short2(result);
}
int2 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int2 seed, int polynomial) {
    uint2 result;
    result.x = __builtin_IB_lfsr_b32(as_uint(seed.x), as_uint(polynomial));
    result.y = __builtin_IB_lfsr_b32(as_uint(seed.y), as_uint(polynomial));
    return as_int2(result);
}
// lfsr vec3
char3 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char3 seed, char polynomial) {
    char4 poly4 = {polynomial, polynomial, polynomial, polynomial};
    char4 seed4;
    seed4.xyz = seed;
    uint result = __builtin_IB_lfsr_b8v4(as_uint(seed4), as_uint(poly4));
    return as_char4(result).xyz;
}
short3 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short3 seed, short polynomial) {
    short2 poly2 = {polynomial, polynomial};
    uint res_xy = __builtin_IB_lfsr_b16v2(as_uint(seed.xy), as_uint(poly2));
    ushort res_z = __builtin_IB_lfsr_b16v2_ushort(as_ushort(seed.z), as_ushort(polynomial));
    short3 result;
    result.xy = as_short2(res_xy);
    result.z = as_short(res_z);
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
    char4 poly = {polynomial, polynomial, polynomial, polynomial};
    return as_char4(__builtin_IB_lfsr_b8v4(as_uint(seed), as_uint(poly)));
}
short4 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short4 seed, short polynomial) {
    short2 poly = {polynomial, polynomial};
    uint2 result;
    result.x = __builtin_IB_lfsr_b16v2(as_uint(seed.xy), as_uint(poly));
    result.y = __builtin_IB_lfsr_b16v2(as_uint(seed.zw), as_uint(poly));
    return as_short4(result);
}
int4 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int4 seed, int polynomial) {
    uint4 result;
    result.x = __builtin_IB_lfsr_b32(as_uint(seed.x), as_uint(polynomial));
    result.y = __builtin_IB_lfsr_b32(as_uint(seed.y), as_uint(polynomial));
    result.z = __builtin_IB_lfsr_b32(as_uint(seed.z), as_uint(polynomial));
    result.w = __builtin_IB_lfsr_b32(as_uint(seed.w), as_uint(polynomial));
    return as_int4(result);
}
long4 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(long4 seed, long polynomial) {
    long4 result;
    result.x = __spirv_GaloisLFSRINTEL(seed.x, polynomial);
    result.y = __spirv_GaloisLFSRINTEL(seed.y, polynomial);
    result.z = __spirv_GaloisLFSRINTEL(seed.z, polynomial);
    result.w = __spirv_GaloisLFSRINTEL(seed.w, polynomial);
    return result;
}
// lfsr vec8
char8 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char8 seed, char polynomial) {
    char4 poly = {polynomial, polynomial, polynomial, polynomial};
    uint2 result;
    result.x = __builtin_IB_lfsr_b8v4(as_uint(seed.s0123), as_uint(poly));
    result.y = __builtin_IB_lfsr_b8v4(as_uint(seed.s4567), as_uint(poly));
    return as_char8(result);
}
short8 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short8 seed, short polynomial) {
    short2 poly = {polynomial, polynomial};
    uint4 result;
    result.x = __builtin_IB_lfsr_b16v2(as_uint(seed.s01), as_uint(poly));
    result.y = __builtin_IB_lfsr_b16v2(as_uint(seed.s23), as_uint(poly));
    result.z = __builtin_IB_lfsr_b16v2(as_uint(seed.s45), as_uint(poly));
    result.w = __builtin_IB_lfsr_b16v2(as_uint(seed.s67), as_uint(poly));
    return as_short8(result);
}
int8 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int8 seed, int polynomial) {
    uint8 result;
    result.s0 = __builtin_IB_lfsr_b32(as_uint(seed.s0), as_uint(polynomial));
    result.s1 = __builtin_IB_lfsr_b32(as_uint(seed.s1), as_uint(polynomial));
    result.s2 = __builtin_IB_lfsr_b32(as_uint(seed.s2), as_uint(polynomial));
    result.s3 = __builtin_IB_lfsr_b32(as_uint(seed.s3), as_uint(polynomial));
    result.s4 = __builtin_IB_lfsr_b32(as_uint(seed.s4), as_uint(polynomial));
    result.s5 = __builtin_IB_lfsr_b32(as_uint(seed.s5), as_uint(polynomial));
    result.s6 = __builtin_IB_lfsr_b32(as_uint(seed.s6), as_uint(polynomial));
    result.s7 = __builtin_IB_lfsr_b32(as_uint(seed.s7), as_uint(polynomial));
    return as_int8(result);
}
// lfsr vec16
char16 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(char16 seed, char polynomial) {
    char4 poly = {polynomial, polynomial, polynomial, polynomial};
    uint4 result;
    result.x = __builtin_IB_lfsr_b8v4(as_uint(seed.s0123), as_uint(poly));
    result.y = __builtin_IB_lfsr_b8v4(as_uint(seed.s4567), as_uint(poly));
    result.z = __builtin_IB_lfsr_b8v4(as_uint(seed.s89ab), as_uint(poly));
    result.w = __builtin_IB_lfsr_b8v4(as_uint(seed.scdef), as_uint(poly));
    return as_char16(result);
}
short16 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(short16 seed, short polynomial) {
    short2 poly = {polynomial, polynomial};
    uint8 result;
    result.s0 = __builtin_IB_lfsr_b16v2(as_uint(seed.s01), as_uint(poly));
    result.s1 = __builtin_IB_lfsr_b16v2(as_uint(seed.s23), as_uint(poly));
    result.s2 = __builtin_IB_lfsr_b16v2(as_uint(seed.s45), as_uint(poly));
    result.s3 = __builtin_IB_lfsr_b16v2(as_uint(seed.s67), as_uint(poly));
    result.s4 = __builtin_IB_lfsr_b16v2(as_uint(seed.s89), as_uint(poly));
    result.s5 = __builtin_IB_lfsr_b16v2(as_uint(seed.sab), as_uint(poly));
    result.s6 = __builtin_IB_lfsr_b16v2(as_uint(seed.scd), as_uint(poly));
    result.s7 = __builtin_IB_lfsr_b16v2(as_uint(seed.sef), as_uint(poly));
    return as_short16(result);
}
int16 __attribute__((overloadable)) __spirv_GaloisLFSRINTEL(int16 seed, int polynomial) {
    uint16 result;
    result.s0 = __builtin_IB_lfsr_b32(as_uint(seed.s0), as_uint(polynomial));
    result.s1 = __builtin_IB_lfsr_b32(as_uint(seed.s1), as_uint(polynomial));
    result.s2 = __builtin_IB_lfsr_b32(as_uint(seed.s2), as_uint(polynomial));
    result.s3 = __builtin_IB_lfsr_b32(as_uint(seed.s3), as_uint(polynomial));
    result.s4 = __builtin_IB_lfsr_b32(as_uint(seed.s4), as_uint(polynomial));
    result.s5 = __builtin_IB_lfsr_b32(as_uint(seed.s5), as_uint(polynomial));
    result.s6 = __builtin_IB_lfsr_b32(as_uint(seed.s6), as_uint(polynomial));
    result.s7 = __builtin_IB_lfsr_b32(as_uint(seed.s7), as_uint(polynomial));
    result.s8 = __builtin_IB_lfsr_b32(as_uint(seed.s8), as_uint(polynomial));
    result.s9 = __builtin_IB_lfsr_b32(as_uint(seed.s9), as_uint(polynomial));
    result.sa = __builtin_IB_lfsr_b32(as_uint(seed.sa), as_uint(polynomial));
    result.sb = __builtin_IB_lfsr_b32(as_uint(seed.sb), as_uint(polynomial));
    result.sc = __builtin_IB_lfsr_b32(as_uint(seed.sc), as_uint(polynomial));
    result.sd = __builtin_IB_lfsr_b32(as_uint(seed.sd), as_uint(polynomial));
    result.se = __builtin_IB_lfsr_b32(as_uint(seed.se), as_uint(polynomial));
    result.sf = __builtin_IB_lfsr_b32(as_uint(seed.sf), as_uint(polynomial));
    return as_int16(result);
}
