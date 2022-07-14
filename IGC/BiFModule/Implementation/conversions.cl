/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.cl"
#include "../Headers/spirv.h"

#define RT_POS 0
#define RT_NEG 1
#define RT_Z   2
#define RT_NE  3

extern __constant int __UseNative64BitIntBuiltin;
extern __constant int __UseNative64BitFloatBuiltin;

static ulong OVERLOADABLE sat_ulong(half _T, ulong _R);
#if defined(cl_khr_fp64)
INLINE float __intel_convert_float_rtp_rtn(double a, uint direction);
#endif

#if defined(cl_khr_fp16)

/* Helper Functions from IBiF_Conversions.cl */
#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static ushort OVERLOADABLE sat_ushort(half _T, ushort _R)
{
  return SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )(
    _R, (ushort)0,
    SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(
        (half)((_T < (half)0) | SPIRV_BUILTIN(IsNan, _f16, )(_T))));
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static uint OVERLOADABLE sat_uint(half _T, uint _R)
{
  return SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )(
    as_int(_R), 0,
    SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(
        (half)((_T < (half)0) | SPIRV_BUILTIN(IsNan, _f16, )(_T))));
}
#endif

static ulong OVERLOADABLE sat_ulong(half _T, ulong _R)
{
  return SPIRV_OCL_BUILTIN(select, _i64_i64_i64, )(
    as_long(_R), (long)0,
    SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(
        (half)((_T < (half)0) | SPIRV_BUILTIN(IsNan, _f16, )(_T))));
}

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static uchar clamp_sat_uchar(half _T, uchar _R)
{
  _R = SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )(
    _R, (uchar)0,
    SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(
      (half)(_T < (half)0)));
  _R = SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )(
    _R, (uchar)UCHAR_MAX,
    SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(
      (half)(_T > (half)UCHAR_MAX)));
  _R = SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )(
    _R, (uchar)0,
    SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(
      (half) SPIRV_BUILTIN(IsNan, _f16, )(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static char clamp_sat_char(half _T, char _R)
{
  _R = SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )(
    _R, (char)CHAR_MIN,
    SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(
      (half)(_T < (half)CHAR_MIN)));
  _R = SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )(
    _R, (char)CHAR_MAX,
    SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(
      (half)(_T > (half)CHAR_MAX)));
  _R = SPIRV_OCL_BUILTIN(select, _i8_i8_i8, )(
    _R, (char)0,
    SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(
      (half) SPIRV_BUILTIN(IsNan, _f16, )(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static short clamp_sat_short(half _T, short _R)
{
  _R = SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )(
    _R, (short)SHRT_MIN,
    SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(
      (half)(_T < (half)SHRT_MIN)));
  _R = SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )(
    _R, (short)SHRT_MAX,
    SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(
      (half)(_T > (half)SHRT_MAX)));
  _R = SPIRV_OCL_BUILTIN(select, _i16_i16_i16, )(
    _R, (short)0,
    SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(
      (half) SPIRV_BUILTIN(IsNan, _f16, )(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static int clamp_sat_int(half _T, int _R)
{
    _R = SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )(
        _R, (int)INT_MIN,
        SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(
          (half)(_T < (half)INT_MIN)));
    _R = SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )(
        _R, (int)INT_MAX,
        SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(
          (half)(_T > (half)INT_MAX)));
    _R = SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )(
        _R, (int)0,
        SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(
          (half) SPIRV_BUILTIN(IsNan, _f16, )(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
#define UCHAR_MIN ((uchar)0)
#define USHRT_MIN ((ushort)0)
#define UINT_MIN  ((uint)0)
#define ULONG_MIN ((ulong)0)
// Helper function for conversions with saturation
#define SAT_CLAMP_HELPER_SIGN(TO, FROM, TONAME, TOA, INTTYPE, FROMA)        \
static TO clamp_sat_##TO##_##FROM(TO _R, FROM _T)                           \
{                                                                           \
  _R = SPIRV_BUILTIN(ConvertFToS, _##FROMA##_##TOA, _R##TO)((FROM)(_T < (FROM)TONAME##_MIN)) ? TONAME##_MIN : _R;  \
  _R = SPIRV_BUILTIN(ConvertFToS, _##FROMA##_##TOA, _R##TO)((FROM)(_T > (FROM)TONAME##_MAX)) ? TONAME##_MAX : _R;  \
  _R = SPIRV_BUILTIN(ConvertFToS, _##FROMA##_##TOA, _R##TO)((FROM)SPIRV_BUILTIN(IsNan, _##TOA, )(_T)) ? 0 : _R;    \
  return _R;                                                                \
}

#define SAT_CLAMP_HELPER_UNSIGNED(TO, FROM, TONAME, TOA, INTTYPE, FROMA)    \
static TO clamp_sat_##TO##_##FROM(TO _R, FROM _T)                           \
{                                                                           \
  _R = SPIRV_BUILTIN(ConvertFToU, _##FROMA##_##TOA, _R##TO)((FROM)(_T < (FROM)TONAME##_MIN)) ? TONAME##_MIN : _R;  \
  _R = SPIRV_BUILTIN(ConvertFToU, _##FROMA##_##TOA, _R##TO)((FROM)(_T > (FROM)TONAME##_MAX)) ? TONAME##_MAX : _R;  \
  _R = SPIRV_BUILTIN(ConvertFToU, _##FROMA##_##TOA, _R##TO)((FROM)SPIRV_BUILTIN(IsNan, _##TOA, )(_T)) ? 0 : _R;    \
  return _R;                                                                \
}
#if defined(cl_khr_fp64)
SAT_CLAMP_HELPER_UNSIGNED(uchar, double, UCHAR, f64, i64, i8)
SAT_CLAMP_HELPER_UNSIGNED(ushort, double, USHRT, f64, i64, i16)
SAT_CLAMP_HELPER_UNSIGNED(uint, double, UINT, f64, i64, i32)
SAT_CLAMP_HELPER_SIGN(char, double, CHAR, f64, i64, i8)
SAT_CLAMP_HELPER_SIGN(short, double, SHRT, f64, i64, i16)
SAT_CLAMP_HELPER_SIGN(int, double, INT, f64, i64, i32)

#endif //defined(cl_khr_fp64)
#endif
#endif

static float convertUItoFP32(ulong value, char roundingMode, bool s);
static float convertSItoFP32(long value, char roundingMode);


 // Conversion Instructions

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(half FloatValue)
{
    return FloatValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(half FloatValue)
{
    return FloatValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(half FloatValue)
{
    return FloatValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(half FloatValue)
{
    return FloatValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i8_f32, _Ruchar)(float FloatValue)
{
    return (uchar)FloatValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i16_f32, _Rushort)(float FloatValue)
{
    return (ushort)FloatValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(float FloatValue)
{
    return (uint)FloatValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(float FloatValue)
{
    float FC0 = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(1.0f, -32);
    float FC1 = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(-1.0f, 32);
    float HiF = SPIRV_OCL_BUILTIN(trunc, _f32, )(FloatValue * FC0);
    float LoF = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(HiF, FC1, FloatValue);
    ulong answer = 0;
    answer = (((uint)HiF | answer) << 32) | ((uint)LoF | answer);
    return answer;
}

#if defined(cl_khr_fp64)

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(double FloatValue)
{
    return FloatValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(double FloatValue)
{
    return FloatValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(double FloatValue)
{
    return FloatValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(double FloatValue)
{
    return FloatValue;
}

#endif //defined(cl_khr_fp64)

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(half FloatValue)
{
    return FloatValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(half FloatValue)
{
    return FloatValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(half FloatValue)
{
    return FloatValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(half FloatValue)
{
    return FloatValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i8_f32, _Rchar)(float FloatValue)
{
    return (char)FloatValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i16_f32, _Rshort)(float FloatValue)
{
    return (short)FloatValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(float FloatValue)
{
    return (int)FloatValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(float FloatValue)
{
    float abs_value = SPIRV_OCL_BUILTIN(fabs, _f32, )(FloatValue);
    if (abs_value < 1.0f) //for small numbers and denormals
    {
        return (long)0;
    }
    uint sign = as_uint(as_int(FloatValue) >> 31);
    float FC0 = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(1.0f, -32);
    float FC1 = SPIRV_OCL_BUILTIN(ldexp, _f32_i32, )(-1.0f, 32);
    float HiF = SPIRV_OCL_BUILTIN(trunc, _f32, )(abs_value * FC0);
    float LoF = SPIRV_OCL_BUILTIN(fma, _f32_f32_f32, )(HiF, FC1, abs_value);
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

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(double FloatValue)
{
    return FloatValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(double FloatValue)
{
    return FloatValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(double FloatValue)
{
    return FloatValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(double FloatValue)
{
    return FloatValue;
}

#endif //defined(cl_khr_fp64)

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f16_i8, _Rhalf)(char SignedValue)
{
    return SignedValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(char SignedValue)
{
    return (float)SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f16_i16, _Rhalf)(short SignedValue)
{
    return SignedValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(short SignedValue)
{
    return (float)SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f16_i32, _Rhalf)(int SignedValue)
{
    return SignedValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(int SignedValue)
{
    return (float)SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f16_i64, _Rhalf)(long SignedValue)
{
    return SignedValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(long SignedValue)
{
    return convertSItoFP32(SignedValue, 0);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i8, _Rhalf)(uchar UnsignedValue)
{
    return UnsignedValue;
}
float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i1, _Rfloat)(bool BoolValue)
{
    return BoolValue ? 1.0 : 0.0;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(uchar UnsignedValue)
{
    return (float)UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i16, _Rhalf)(ushort UnsignedValue)
{
    return UnsignedValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(ushort UnsignedValue)
{
    return (float)UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i32, _Rhalf)(uint UnsignedValue)
{
    return UnsignedValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(uint UnsignedValue)
{
    return (float)UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f16_i64, _Rhalf)(ulong UnsignedValue)
{
    return UnsignedValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(ulong UnsignedValue)
{
    return convertUItoFP32(UnsignedValue, 0, false);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i8, _Rdouble)(char SignedValue)
{
    return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i16, _Rdouble)(short SignedValue)
{
    return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i32, _Rdouble)(int SignedValue)
{
    return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _f64_i64, _Rdouble)(long SignedValue)
{
    return SignedValue;
}

double  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i1, _Rdouble)(bool BoolValue)
{
    return BoolValue ? 1.0 : 0.0;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i8, _Rdouble)(uchar UnsignedValue)
{
    return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i16, _Rdouble)(ushort UnsignedValue)
{
    return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i32, _Rdouble)(uint UnsignedValue)
{
    return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _f64_i64, _Rdouble)(ulong UnsignedValue)
{
    return UnsignedValue;
}

#endif // defined(cl_khr_fp64)


uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i8_i8, _Ruchar)(uchar UnsignedValue)
{
    return UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i8, _Rushort)(uchar UnsignedValue)
{
    return (ushort)UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i32_i8, _Ruint)(uchar UnsignedValue)
{
    return (uint)UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i8, _Rulong)(uchar UnsignedValue)
{
    return (ulong)UnsignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i8_i16, _Ruchar)(ushort UnsignedValue)
{
    return (uchar)UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i16, _Rushort)(ushort UnsignedValue)
{
    return UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i32_i16, _Ruint)(ushort UnsignedValue)
{
    return (uint)UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i16, _Rulong)(ushort UnsignedValue)
{
    return (ulong)UnsignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i8_i32, _Ruchar)(uint UnsignedValue)
{
    return (uchar)UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i32, _Rushort)(uint UnsignedValue)
{
    return (ushort)UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i32_i32, _Ruint)(uint UnsignedValue)
{
    return UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i32, _Rulong)(uint UnsignedValue)
{
    return (ulong)UnsignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i8_i64, _Ruchar)(ulong UnsignedValue)
{
    return (uchar)UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i16_i64, _Rushort)(ulong UnsignedValue)
{
    return (ushort)UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i32_i64, _Ruint)(ulong UnsignedValue)
{
    return (uint)UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _i64_i64, _Rulong)(ulong UnsignedValue)
{
    return UnsignedValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i8_i8, _Rchar)(char SignedValue)
{
    return SignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i8, _Rshort)(char SignedValue)
{
    return (short)SignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i32_i8, _Rint)(char SignedValue)
{
    return (int)SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i64_i8, _Rlong)(char SignedValue)
{
    return (long)SignedValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i8_i16, _Rchar)(short SignedValue)
{
    return (char)SignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i16, _Rshort)(short SignedValue)
{
    return SignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i32_i16, _Rint)(short SignedValue)
{
    return (int)SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i64_i16, _Rlong)(short SignedValue)
{
    return (long)SignedValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i8_i32, _Rchar)(int SignedValue)
{
    return (char)SignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i32, _Rshort)(int SignedValue)
{
    return (short)SignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i32_i32, _Rint)(int SignedValue)
{
    return SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i64_i32, _Rlong)(int SignedValue)
{
    return (long)SignedValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i8_i64, _Rchar)(long SignedValue)
{
    return (char)SignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i16_i64, _Rshort)(long SignedValue)
{
    return (short)SignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i32_i64, _Rint)(long SignedValue)
{
    return (int)SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _i64_i64, _Rlong)(long SignedValue)
{
    return SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f16_f16, _Rhalf)(half FloatValue)
{
    return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f32_f16, _Rfloat)(half FloatValue)
{
    return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f16_f32, _Rhalf)(float FloatValue)
{
    return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(float FloatValue)
{
    return FloatValue;
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f64_f16, _Rdouble)(half FloatValue)
{
    return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f64_f32, _Rdouble)(float FloatValue)
{
    return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f16_f64, _Rhalf)(double FloatValue)
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

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f32_f64, _Rfloat)(double FloatValue)
{
    return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _f64_f64, _Rdouble)(double FloatValue)
{
    return FloatValue;
}

#endif // defined(cl_khr_fp64)

// OpConvertPtrToU -> ptrtoint, no need.

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i8, _Ruchar)(char SignedValue)
{
      //return __builtin_IB_ctouc_sat((char)SignedValue);
      if (SignedValue <= 0)
      {
        return (uchar)0;
      }
      return (uchar)SignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i8, _Rushort)(char SignedValue)
{
      //return __builtin_IB_ctous_sat((char)SignedValue);
      if( SignedValue <= 0)
      {
        return 0;
      }
      return (ushort)SignedValue;
}

uint  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i8, _Ruint)(char SignedValue)
{
      //return __builtin_IB_ctoui_sat((char)SignedValue);
      return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i8, _Rulong)(char SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i16, _Ruchar)(short SignedValue)
{
  return (uchar)clamp(SignedValue, (short)0, (short)UCHAR_MAX);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i16, _Rushort)(short SignedValue)
{
      //return __builtin_IB_stous_sat((short)SignedValue);
      if( SignedValue <= 0)
      {
        return 0;
      }
      return (ushort)SignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i16, _Ruint)(short SignedValue)
{
      //return __builtin_IB_stoui_sat((short)SignedValue);
      return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i16, _Rulong)(short SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i32, _Ruchar)(int SignedValue)
{
  return (uchar)clamp(SignedValue, 0, (int)UCHAR_MAX);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i32, _Rushort)(int SignedValue)
{
      //return __builtin_IB_itous_sat((int)SignedValue);
      int res = SPIRV_OCL_BUILTIN(s_clamp, _i32_i32_i32, )(SignedValue, 0, (int)USHRT_MAX);
      return (ushort)res;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i32, _Ruint)(int SignedValue)
{
      //return __builtin_IB_itoui_sat((int)SignedValue);
      return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i32, _Rulong)(int SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i8_i64, _Ruchar)(long SignedValue)
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

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i16_i64, _Rushort)(long SignedValue)
{
    if (SignedValue <= 0) {
        return 0;
      }
    else if (SignedValue >= USHRT_MAX) {
        return USHRT_MAX;
      }
      return (ushort)SignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i32_i64, _Ruint)(long SignedValue)
{
    if (SignedValue <= 0) {
        return 0;
      }
    else if (SignedValue >= UINT_MAX) {
        return UINT_MAX;
      }
      return (uint)SignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertSToU, _i64_i64, _Rulong)(long SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}


char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i8, _Rchar)(uchar UnsignedValue)
{
      //return __builtin_IB_uctoc_sat((uchar)UnsignedValue);
      return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i16_i8, _Rshort)(uchar UnsignedValue)
{
    return (short)UnsignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i8, _Rint)(uchar UnsignedValue)
{
    return (int)UnsignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i64_i8, _Rlong)(uchar UnsignedValue)
{
    return (long)UnsignedValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i16, _Rchar)(ushort UnsignedValue)
{
      //return __builtin_IB_ustoc_sat((ushort)UnsignedValue);
      return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i16_i16, _Rshort)(ushort UnsignedValue)
{
      //return __builtin_IB_ustos_sat((ushort)UnsignedValue);
      return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (ushort)UnsignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i16, _Rint)(ushort UnsignedValue)
{
    return (int)UnsignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i64_i16, _Rlong)(ushort UnsignedValue)
{
    return (long)UnsignedValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i32, _Rchar)(uint UnsignedValue)
{
      //return __builtin_IB_uitoc_sat((uint)UnsignedValue);
      return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i16_i32, _Rshort)(uint UnsignedValue)
{
      //return __builtin_IB_uitos_sat((uint)UnsignedValue);
      return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (short)UnsignedValue;
}

int SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i32, _Rint)(uint UnsignedValue)
{
      //return __builtin_IB_uitoi_sat((uint)UnsignedValue);
      return (UnsignedValue >= INT_MAX) ? INT_MAX : (int)UnsignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i64_i32, _Rlong)(uint UnsignedValue)
{
    return (long)UnsignedValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i8_i64, _Rchar)(ulong UnsignedValue)
{
    return (UnsignedValue >= CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i16_i64, _Rshort)(ulong UnsignedValue)
{
    return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (short)UnsignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i32_i64, _Rint)(ulong UnsignedValue)
{
    return (UnsignedValue >= INT_MAX) ? INT_MAX : (int)UnsignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SatConvertUToS, _i64_i64, _Rlong)(ulong UnsignedValue)
{
    return (UnsignedValue >= LONG_MAX) ? LONG_MAX : SPIRV_BUILTIN(SConvert, _i64_i64, _Rlong)((long)UnsignedValue);
}

short  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToBF16INTEL, _f32, _Rshort)(float Value)
{
  return __builtin_IB_ftobf_1(Value);
}

short2  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToBF16INTEL, _v2f32, _Rshort2)(float2 Value)
{
  return __builtin_IB_ftobf_2(Value);
}

short3  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToBF16INTEL, _v3f32, _Rshort3)(float3 Value)
{
  return __builtin_IB_ftobf_3(Value);
}

short4  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToBF16INTEL, _v4f32, _Rshort4)(float4 Value)
{
  return __builtin_IB_ftobf_4(Value);
}

short8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToBF16INTEL, _v8f32, _Rshort8)(float8 Value)
{
  return __builtin_IB_ftobf_8(Value);
}

short16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToBF16INTEL, _v16f32, _Rshort16)(float16 Value)
{
  return __builtin_IB_ftobf_16(Value);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertBF16ToFINTEL, _i16, _Rfloat)(short Value)
{
  return __builtin_IB_bftof_1(Value);
}

float2  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v2i16, _Rfloat2)(short2 Value)
{
  return __builtin_IB_bftof_2(Value);
}

float3  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v3i16, _Rfloat3)(short3 Value)
{
  return __builtin_IB_bftof_3(Value);
}

float4  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v4i16, _Rfloat4)(short4 Value)
{
  return __builtin_IB_bftof_4(Value);
}

float8  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v8i16, _Rfloat8)(short8 Value)
{
  return __builtin_IB_bftof_8(Value);
}

float16  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertBF16ToFINTEL, _v16i16, _Rfloat16)(short16 Value)
{
  return __builtin_IB_bftof_16(Value);
}

/*
// Next is all Scalar types with Rounding modes [RTE,RTZ,RTN,RTP] and Sat
//
//
//
//
*/

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f16, _Ruchar_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f16, _Ruchar_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f16, _Ruchar_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f16, _Ruchar_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _Ruchar_sat)(half FloatValue)
{
  uchar normal = SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
  return clamp_sat_uchar(FloatValue, normal);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f16, _Ruchar_sat_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _Ruchar_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f16, _Ruchar_sat_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _Ruchar_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f16, _Ruchar_sat_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _Ruchar_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f16, _Ruchar_sat_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _Ruchar_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f16, _Rushort_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f16, _Rushort_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f16, _Rushort_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f16, _Rushort_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _Rushort_sat)(half FloatValue)
{
  ushort normal = SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
  return sat_ushort(FloatValue, normal);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f16, _Rushort_sat_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _Rushort_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f16, _Rushort_sat_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _Rushort_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f16, _Rushort_sat_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _Rushort_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f16, _Rushort_sat_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _Rushort_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f16, _Ruint_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f16, _Ruint_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f16, _Ruint_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f16, _Ruint_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _Ruint_sat)(half FloatValue)
{
  uint normal = SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
  return sat_uint(FloatValue, normal);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f16, _Ruint_sat_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _Ruint_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f16, _Ruint_sat_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _Ruint_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f16, _Ruint_sat_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _Ruint_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f16, _Ruint_sat_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _Ruint_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f16, _Rulong_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f16, _Rulong_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f16, _Rulong_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f16, _Rulong_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _Rulong_sat)(half FloatValue)
{
  ulong normal = SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
  return sat_ulong(FloatValue, normal);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f16, _Rulong_sat_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _Rulong_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f16, _Rulong_sat_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _Rulong_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f16, _Rulong_sat_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _Rulong_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f16, _Rulong_sat_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _Rulong_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f32, _Ruchar_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i8_f32, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f32, _Ruchar_rtz)(float FloatValue)
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

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f32, _Ruchar_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i8_f32, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f32, _Ruchar_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i8_f32, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _Ruchar_sat)(float FloatValue)
{
  //return __builtin_IB_ftouc_sat((float)FloatValue);
  float res = SPIRV_OCL_BUILTIN(fclamp, _f32_f32_f32, )(FloatValue, 0.0f, (float)UCHAR_MAX);
  return (uchar)res;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f32, _Ruchar_sat_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _Ruchar_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f32, _Ruchar_sat_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _Ruchar_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f32, _Ruchar_sat_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _Ruchar_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f32, _Ruchar_sat_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _Ruchar_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f32, _Rushort_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f32, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f32, _Rushort_rtz)(float FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= USHRT_MAX) {
    return USHRT_MAX;
  }
  return (ushort)FloatValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f32, _Rushort_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i16_f32, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f32, _Rushort_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i16_f32, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _Rushort_sat)(float FloatValue)
{
  //return __builtin_IB_ftous_sat((float)FloatValue);
  float res = SPIRV_OCL_BUILTIN(fclamp, _f32_f32_f32, )(FloatValue, 0.0f, (float)USHRT_MAX);
  return (ushort)res;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f32, _Rushort_sat_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _Rushort_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f32, _Rushort_sat_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _Rushort_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f32, _Rushort_sat_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _Rushort_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f32, _Rushort_sat_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _Rushort_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f32, _Ruint_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f32, _Ruint_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f32, _Ruint_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f32, _Ruint_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _Ruint_sat)(float FloatValue)
{
    return SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f32, _Ruint_sat_rtz)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f32, _Ruint_sat_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _Ruint_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f32, _Ruint_sat_rtz)(float FloatValue)
{
  uint _R = SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f32, _Ruint_rtz)(FloatValue);
  _R = SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(
      (float)(FloatValue > (float)UINT_MAX)) ?
      (uint)UINT_MAX : _R;

  return SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(
      (float)((FloatValue < (float)0) |
          SPIRV_BUILTIN(IsNan, _f32, )(FloatValue))) ?
      (uint)0 : _R;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f32, _Ruint_sat_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _Ruint_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f32, _Ruint_sat_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _Ruint_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f32, _Rulong_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f32, _Rulong_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f32, _Rulong_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f32, _Rulong_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _Rulong_sat)(float FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= ULONG_MAX) {
    return ULONG_MAX;
  }
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f32, _Rulong_sat_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _Rulong_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f32, _Rulong_sat_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _Rulong_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f32, _Rulong_sat_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _Rulong_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f32, _Rulong_sat_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _Rulong_sat)(FloatValue);
}

#if defined(cl_khr_fp64)

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f64, _Ruchar_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f64, _Ruchar_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f64, _Ruchar_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f64, _Ruchar_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _Ruchar_sat)(double FloatValue)
{
  uchar normal = SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
  return clamp_sat_uchar_double(normal, FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _Rushort_sat)(double FloatValue)
{
  ushort normal = SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
  return clamp_sat_ushort_double(normal, FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _Ruint_sat)(double FloatValue)
{
  uint normal = SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
  return clamp_sat_uint_double(normal, FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _Rchar_sat)(double FloatValue)
{
  char normal = SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
  return clamp_sat_char_double(normal, FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _Rshort_sat)(double FloatValue)
{
  short normal = SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
  return clamp_sat_short_double(normal, FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _Rint_sat)(double FloatValue)
{
  int normal = SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
  return clamp_sat_int_double(normal, FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f64, _Ruchar_sat_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _Ruchar_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f64, _Ruchar_sat_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _Ruchar_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f64, _Ruchar_sat_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _Ruchar_sat)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f64, _Ruchar_sat_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _Ruchar_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f64, _Rushort_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f64, _Rushort_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f64, _Rushort_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f64, _Rushort_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f64, _Rushort_sat_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _Rushort_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f64, _Rushort_sat_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _Rushort_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f64, _Rushort_sat_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _Rushort_sat)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f64, _Rushort_sat_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _Rushort_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f64, _Ruint_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f64, _Ruint_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f64, _Ruint_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f64, _Ruint_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f64, _Ruint_sat_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _Ruint_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f64, _Ruint_sat_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _Ruint_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f64, _Ruint_sat_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _Ruint_sat)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f64, _Ruint_sat_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _Ruint_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f64, _Rulong_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f64, _Rulong_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f64, _Rulong_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f64, _Rulong_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _Rulong_sat)(double FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= ULONG_MAX) {
    return ULONG_MAX;
  }
  return FloatValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f64, _Rulong_sat_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _Rulong_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f64, _Rulong_sat_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _Rulong_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f64, _Rulong_sat_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _Rulong_sat)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f64, _Rulong_sat_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _Rulong_sat)(FloatValue);
}

#endif // defined(cl_khr_fp64)

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f16, _Rchar_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f16, _Rchar_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f16, _Rchar_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f16, _Rchar_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _Rchar_sat)(half FloatValue)
{
  char normal = SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
  return clamp_sat_char(FloatValue, normal);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f16, _Rchar_sat_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _Rchar_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f16, _Rchar_sat_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _Rchar_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f16, _Rchar_sat_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _Rchar_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f16, _Rchar_sat_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _Rchar_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f16, _Rshort_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f16, _Rshort_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f16, _Rshort_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f16, _Rshort_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _Rshort_sat)(half FloatValue)
{
  short normal = SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
  return clamp_sat_short(FloatValue, normal);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f16, _Rshort_sat_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _Rshort_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f16, _Rshort_sat_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _Rshort_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f16, _Rshort_sat_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _Rshort_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f16, _Rshort_sat_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _Rshort_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f16, _Rint_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f16, _Rint_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f16, _Rint_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f16, _Rint_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _Rint_sat)(half FloatValue)
{
  int normal = SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
  return clamp_sat_int(FloatValue, normal);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f16, _Rint_sat_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _Rint_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f16, _Rint_sat_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _Rint_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f16, _Rint_sat_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _Rint_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f16, _Rint_sat_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _Rint_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f16, _Rlong_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f16, _Rlong_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f16, _Rlong_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f16, _Rlong_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _Rlong_sat)(half FloatValue)
{
  if (FloatValue <= LONG_MIN) {
    return LONG_MIN;
  } else if (FloatValue >= LONG_MAX) {
    return LONG_MAX;
  }
  return FloatValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f16, _Rlong_sat_rte)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _Rlong_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f16, _Rlong_sat_rtz)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _Rlong_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f16, _Rlong_sat_rtp)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _Rlong_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f16, _Rlong_sat_rtn)(half FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f16, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _Rlong_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f32, _Rchar_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f32, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f32, _Rchar_rtz)(float FloatValue)
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

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f32, _Rchar_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i8_f32, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f32, _Rchar_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i8_f32, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _Rchar_sat)(float FloatValue)
{
  float res = SPIRV_OCL_BUILTIN(fclamp, _f32_f32_f32, )(FloatValue, (float)CHAR_MIN, (float)CHAR_MAX);
  res = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )(res, 0.0f , SPIRV_BUILTIN(IsNan, _f32, )(FloatValue));
  return (char)res;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f32, _Rchar_sat_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _Rchar_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f32, _Rchar_sat_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _Rchar_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f32, _Rchar_sat_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _Rchar_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f32, _Rchar_sat_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _Rchar_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f32, _Rshort_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f32, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f32, _Rshort_rtz)(float FloatValue)
{
  if (FloatValue <= SHRT_MIN) {
    return SHRT_MIN;
  } else if (FloatValue >= SHRT_MAX) {
    return SHRT_MAX;
  }
  return (short)FloatValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f32, _Rshort_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i16_f32, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f32, _Rshort_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i16_f32, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _Rshort_sat)(float FloatValue)
{
  float res = SPIRV_OCL_BUILTIN(fclamp, _f32_f32_f32, )(FloatValue, (float)SHRT_MIN, (float)SHRT_MAX);
  res = SPIRV_OCL_BUILTIN(select, _f32_f32_i32, )(res, 0.0f , SPIRV_BUILTIN(IsNan, _f32, )(FloatValue));
  return (short)res;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f32, _Rshort_sat_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _Rshort_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f32, _Rshort_sat_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _Rshort_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f32, _Rshort_sat_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _Rshort_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f32, _Rshort_sat_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _Rshort_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f32, _Rint_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f32, _Rint_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f32, _Rint_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f32, _Rint_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _Rint_sat)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f32, _Rint_sat_rtz)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f32, _Rint_sat_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _Rint_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f32, _Rint_sat_rtz)(float FloatValue)
{
  int _R = SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f32, _Rint_rtz)(FloatValue);
  _R = SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )(
    _R, (int)INT_MIN,
    SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(
      (float)(FloatValue < (float)INT_MIN)));
  _R = SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )(
    _R, (int)INT_MAX,
    SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(
      (float)(FloatValue > (float)INT_MAX)));
  return SPIRV_OCL_BUILTIN(select, _i32_i32_i32, )(
    _R, (int)0,
    SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(
      (float) SPIRV_BUILTIN(IsNan, _f32, )(FloatValue)));
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f32, _Rint_sat_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _Rint_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f32, _Rint_sat_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _Rint_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f32, _Rlong_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f32, _Rlong_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f32, _Rlong_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f32, _Rlong_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _Rlong_sat)(float FloatValue)
{
  if (FloatValue <= LONG_MIN) {
    return LONG_MIN;
  } else if (FloatValue >= LONG_MAX) {
    return LONG_MAX;
  }
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f32, _Rlong_sat_rte)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f32, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _Rlong_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f32, _Rlong_sat_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _Rlong_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f32, _Rlong_sat_rtp)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _Rlong_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f32, _Rlong_sat_rtn)(float FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f32, )( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _Rlong_sat)(FloatValue);
}

#if defined(cl_khr_fp64)

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f64, _Rchar_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f64, _Rchar_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f64, _Rchar_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f64, _Rchar_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f64, _Rchar_sat_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _Rchar_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f64, _Rchar_sat_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _Rchar_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f64, _Rchar_sat_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _Rchar_sat)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f64, _Rchar_sat_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _Rchar_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f64, _Rshort_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f64, _Rshort_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f64, _Rshort_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f64, _Rshort_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f64, _Rshort_sat_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _Rshort_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f64, _Rshort_sat_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _Rshort_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f64, _Rshort_sat_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _Rshort_sat)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f64, _Rshort_sat_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _Rshort_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f64, _Rint_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f64, _Rint_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f64, _Rint_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f64, _Rint_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f64, _Rint_sat_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _Rint_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f64, _Rint_sat_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _Rint_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f64, _Rint_sat_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _Rint_sat)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f64, _Rint_sat_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _Rint_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f64, _Rlong_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f64, _Rlong_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f64, _Rlong_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f64, _Rlong_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _Rlong_sat)(double FloatValue)
{
  if (FloatValue <= LONG_MIN) {
    return LONG_MIN;
  } else if (FloatValue >= LONG_MAX) {
    return LONG_MAX;
  }
  return FloatValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f64, _Rlong_sat_rte)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(rint, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _Rlong_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f64, _Rlong_sat_rtz)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _Rlong_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f64, _Rlong_sat_rtp)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(ceil, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _Rlong_sat)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f64, _Rlong_sat_rtn)(double FloatValue)
{
  FloatValue = SPIRV_OCL_BUILTIN(floor, _f64, )(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _Rlong_sat)(FloatValue);
}

#endif  // defined(cl_khr_fp64)

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i8, _Rhalf_rte)(char SignedValue)
{
  return SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i8, _Rhalf_rtz)(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i8, _Rhalf_rtp)(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i8, _Rhalf_rtn)(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i8, _Rfloat_rte)(char SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i8, _Rfloat_rtz)(char SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i8, _Rfloat_rtp)(char SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i8, _Rfloat_rtn)(char SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(SignedValue);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i8, _Rdouble_rte)(char SignedValue)
{
  return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i8, _Rdouble_rtz)(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i8, _Rdouble_rtp)(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i8, _Rdouble_rtn)(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

#endif //defined(cl_khr_fp64)

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i16, _Rhalf_rte)(short SignedValue)
{
  return SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i16, _Rhalf_rtz)(short SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i16, _Rhalf_rtp)(short SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i16, _Rhalf_rtn)(short SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i16, _Rfloat_rte)(short SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i16, _Rfloat_rtz)(short SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i16, _Rfloat_rtp)(short SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i16, _Rfloat_rtn)(short SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i16, _Rdouble_rte)(short SignedValue)
{
  return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i16, _Rdouble_rtz)(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i16, _Rdouble_rtp)(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i16, _Rdouble_rtn)(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

#endif  // defined(cl_khr_fp64)

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i32, _Rhalf_rte)(int SignedValue)
{
  return SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i32, _Rhalf_rtz)(int SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i32, _Rhalf_rtp)(int SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i32, _Rhalf_rtn)(int SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i32, _Rfloat_rte)(int SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i32, _Rfloat_rtz)(int SignedValue)
{
  return __builtin_IB_itof_rtz(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i32, _Rfloat_rtp)(int SignedValue)
{
  return __builtin_IB_itof_rtp(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i32, _Rfloat_rtn)(int SignedValue)
{
  return __builtin_IB_itof_rtn(SignedValue);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i32, _Rdouble_rte)(int SignedValue)
{
  return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i32, _Rdouble_rtz)(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i32, _Rdouble_rtp)(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i32, _Rdouble_rtn)(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

#endif  // defined(cl_khr_fp64)

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i64, _Rhalf_rte)(long SignedValue)
{
  return SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i64, _Rhalf_rtz)(long SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i64, _Rhalf_rtp)(long SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i64, _Rhalf_rtn)(long SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i64, _Rfloat_rte)(long SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i64, _Rfloat_rtz)(long SignedValue)
{
  return convertSItoFP32(SignedValue, 3);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i64, _Rfloat_rtp)(long SignedValue)
{
  return convertSItoFP32(SignedValue, 1);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i64, _Rfloat_rtn)(long SignedValue)
{
  return convertSItoFP32(SignedValue, 2);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i64, _Rdouble_rte)(long SignedValue)
{
  return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i64, _Rdouble_rtz)(long SignedValue)
{
  if(__UseNative64BitIntBuiltin)
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

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i64, _Rdouble_rtp)(long SignedValue)
{
  if(__UseNative64BitIntBuiltin)
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

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i64, _Rdouble_rtn)(long SignedValue)
{
  if(__UseNative64BitIntBuiltin)
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

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i8, _Rhalf_rte)(uchar UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i8, _Rhalf_rtz)(uchar UnsignedValue)
{
  return UnsignedValue;    // A half can represent a uchar exactly.
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i8, _Rhalf_rtp)(uchar UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i8, _Rhalf_rtn)(uchar UnsignedValue)
{
  return UnsignedValue;    // A half can represent a uchar exactly.
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i8, _Rfloat_rte)(uchar UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i8, _Rfloat_rtz)(uchar UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i8, _Rfloat_rtp)(uchar UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i8, _Rfloat_rtn)(uchar UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(UnsignedValue);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i8, _Rdouble_rte)(uchar UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i8, _Rdouble_rtz)(uchar UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uchar exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i8, _Rdouble_rtp)(uchar UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i8, _Rdouble_rtn)(uchar UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uchar exactly.
}

#endif  // defined(cl_khr_fp64)

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i16, _Rhalf_rte)(ushort UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i16, _Rhalf_rtz)(ushort UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i16, _Rhalf_rtp)(ushort UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i16, _Rhalf_rtn)(ushort UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i16, _Rfloat_rte)(ushort UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i16, _Rfloat_rtz)(ushort UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i16, _Rfloat_rtp)(ushort UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i16, _Rfloat_rtn)(ushort UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i16, _Rdouble_rte)(ushort UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i16, _Rdouble_rtz)(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i16, _Rdouble_rtp)(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i16, _Rdouble_rtn)(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

#endif  // defined(cl_khr_fp64)

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i32, _Rhalf_rte)(uint UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i32, _Rhalf_rtz)(uint UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i32, _Rhalf_rtp)(uint UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i32, _Rhalf_rtn)(uint UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i32, _Rfloat_rte)(uint UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i32, _Rfloat_rtz)(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtz(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _Rfloat_rtp)(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtp(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i32, _Rfloat_rtn)(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtn(UnsignedValue);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i32, _Rdouble_rte)(uint UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i32, _Rdouble_rtz)(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i32, _Rdouble_rtp)(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i32, _Rdouble_rtn)(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

#endif  // defined(cl_khr_fp64)

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i64, _Rhalf_rte)(ulong UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i64, _Rhalf_rtz)(ulong UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i64, _Rhalf_rtp)(ulong UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i64, _Rhalf_rtn)(ulong UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i64, _Rfloat_rte)(ulong UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i64, _Rfloat_rtz)(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 3, false);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i64, _Rfloat_rtp)(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 1, false);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i64, _Rfloat_rtn)(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 2, false);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i64, _Rdouble_rte)(ulong UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i64, _Rdouble_rtz)(ulong UnsignedValue)
{
  if(__UseNative64BitIntBuiltin)
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

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i64, _Rdouble_rtp)(ulong UnsignedValue)
{
  if(__UseNative64BitIntBuiltin)
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

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i64, _Rdouble_rtn)(ulong UnsignedValue)
{
  if(__UseNative64BitIntBuiltin)
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

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i8, _Ruchar_sat)(uchar UnsignedValue)
{
  return UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i8, _Rushort_sat)(uchar UnsignedValue)
{
  return UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i8, _Ruint_sat)(uchar UnsignedValue)
{
  return (uint)UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i8, _Rulong_sat)(uchar UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i16, _Ruchar_sat)(ushort UnsignedValue)
{
      //return __builtin_IB_ustouc_sat((ushort)UnsignedValue);
      if (UnsignedValue > (uchar)0xff)
      {
        return (uchar)0xff;
      }
      return (uchar)UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i16, _Rushort_sat)(ushort UnsignedValue)
{
  return UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i16, _Ruint_sat)(ushort UnsignedValue)
{
  return (uint)UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i16, _Rulong_sat)(ushort UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i32, _Ruchar_sat)(uint UnsignedValue)
{
      //return __builtin_IB_uitouc_sat((uint)UnsignedValue);
      if (UnsignedValue > UCHAR_MAX)
      {
        return UCHAR_MAX;
      }
      return (uchar)UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i32, _Rushort_sat)(uint UnsignedValue)
{
      if (UnsignedValue > (ushort)0xffff) {
        return (ushort)0xffff;
      }
      return (ushort)UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i32, _Ruint_sat)(uint UnsignedValue)
{
  return UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i32, _Rulong_sat)(uint UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i64, _Ruchar_sat)(ulong UnsignedValue)
{
  return (UnsignedValue >= UCHAR_MAX) ? UCHAR_MAX : (uchar)UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i64, _Rushort_sat)(ulong UnsignedValue)
{
  return (UnsignedValue >= USHRT_MAX) ? USHRT_MAX : (ushort)UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i64, _Ruint_sat)(ulong UnsignedValue)
{
  return (UnsignedValue >= UINT_MAX) ? UINT_MAX : (uint)UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i64, _Rulong_sat)(ulong UnsignedValue)
{
  return UnsignedValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i8, _Rchar_sat)(char SignedValue)
{
  return SignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i8, _Rshort_sat)(char SignedValue)
{
  return (short)SignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i8, _Rint_sat)(char SignedValue)
{
  return (int)SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i8, _Rlong_sat)(char SignedValue)
{
  return SPIRV_BUILTIN(SConvert, _i64_i8, _Rlong)(SignedValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i16, _Rchar_sat)(short SignedValue)
{
      //return __builtin_IB_stoc_sat((short)SignedValue);
      short res = SPIRV_OCL_BUILTIN(s_clamp, _i16_i16_i16, )(SignedValue, (short)CHAR_MIN, (short)CHAR_MAX);
      return (char)res;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i16, _Rshort_sat)(short SignedValue)
{
  return SignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i16, _Rint_sat)(short SignedValue)
{
  return (int)SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i16, _Rlong_sat)(short SignedValue)
{
  return SPIRV_BUILTIN(SConvert, _i64_i16, _Rlong)(SignedValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i32, _Rchar_sat)(int SignedValue)
{
      //return __builtin_IB_itoc_sat((int)SignedValue);
      int res = SPIRV_OCL_BUILTIN(s_clamp, _i32_i32_i32, )(SignedValue, (int)CHAR_MIN, (int)CHAR_MAX);
      return (char)res;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i32, _Rshort_sat)(int SignedValue)
{
      //return __builtin_IB_itos_sat((int)SignedValue);
      int res = SPIRV_OCL_BUILTIN(s_clamp, _i32_i32_i32, )(SignedValue, (int)SHRT_MIN, (int)SHRT_MAX);
      return (short)res;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i32, _Rint_sat)(int SignedValue)
{
  return SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i32, _Rlong_sat)(int SignedValue)
{
  return SPIRV_BUILTIN(SConvert, _i64_i32, _Rlong)(SignedValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i64, _Rchar_sat)(long SignedValue)
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

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i64, _Rshort_sat)(long SignedValue)
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

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i64, _Rint_sat)(long SignedValue)
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

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i64, _Rlong_sat)(long SignedValue)
{
  return SPIRV_BUILTIN(SConvert, _i64_i64, _Rlong)(SignedValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f16, _Rhalf_rte)(half FloatValue)
{
  return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f16, _Rhalf_rtz)(half FloatValue)
{
  return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f16, _Rhalf_rtp)(half FloatValue)
{
  return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f16, _Rhalf_rtn)(half FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f32_f16, _Rfloat_rte)(half FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f32_f16, _Rfloat_rtz)(half FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f32_f16, _Rfloat_rtp)(half FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f32_f16, _Rfloat_rtn)(half FloatValue)
{
  return FloatValue;
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f64_f16, _Rdouble_rte)(half FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f64_f16, _Rdouble_rtz)(half FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f64_f16, _Rdouble_rtp)(half FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f64_f16, _Rdouble_rtn)(half FloatValue)
{
  return FloatValue;
}

#endif // defined(cl_khr_fp64)

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f32, _Rhalf_rte)(float FloatValue)
{
  return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(float FloatValue)
{
  return __builtin_IB_ftoh_rtz(FloatValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(float FloatValue)
{
  return __builtin_IB_ftoh_rtp(FloatValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(float FloatValue)
{
  return __builtin_IB_ftoh_rtn(FloatValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f32_f32, _Rfloat_rte)(float FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(FloatValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f32_f32, _Rfloat_rtz)(float FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(FloatValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f32_f32, _Rfloat_rtp)(float FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(FloatValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f32_f32, _Rfloat_rtn)(float FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(FloatValue);
}

#if defined(cl_khr_fp64)

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f64_f32, _Rdouble_rte)(float FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f64_f32, _Rdouble_rtz)(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f64_f32, _Rdouble_rtp)(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f64_f32, _Rdouble_rtn)(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f64, _Rhalf_rte)(double FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f16_f64, _Rhalf)(FloatValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f64, _Rhalf_rtz)(double FloatValue)
{
  float f = SPIRV_BUILTIN(FConvert, _RTZ_f32_f64, _Rfloat_rtz)(FloatValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _Rhalf_rtz)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f64, _Rhalf_rtp)(double FloatValue)
{
  float f = SPIRV_BUILTIN(FConvert, _RTP_f32_f64, _Rfloat_rtp)(FloatValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _Rhalf_rtp)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f64, _Rhalf_rtn)(double FloatValue)
{
  float f = SPIRV_BUILTIN(FConvert, _RTN_f32_f64, _Rfloat_rtn)(FloatValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _Rhalf_rtn)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f32_f64, _Rfloat_rte)(double FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f32_f64, _Rfloat_rtz)(double FloatValue)
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

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f32_f64, _Rfloat_rtp)(double FloatValue)
{
  return __intel_convert_float_rtp_rtn(FloatValue, RT_POS);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f32_f64, _Rfloat_rtn)(double FloatValue)
{
  return __intel_convert_float_rtp_rtn(FloatValue, RT_NEG);
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f64_f64, _Rdouble_rte)(double FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f64_f64, _Rdouble_rtz)(double FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f64_f64, _Rdouble_rtp)(double FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f64_f64, _Rdouble_rtn)(double FloatValue)
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
    uint ShiftAmount = SPIRV_OCL_BUILTIN(clz, _i32, )((int)Hi);

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
                    Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _Rfloat_rtp)(InnerResHi | 1);
                else
                    Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _Rfloat_rtp)(InnerResHi);
                break;
            case 2: //rtn
            case 3: //rtz
                Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i32, _Rfloat_rtz)(InnerResHi);
                break;
            default:
                Res_Rounded = InnerResLo ? (float)(InnerResHi | 1) : (float)InnerResHi;
        }
    }
    else
    {
        if (roundingMode == 1) //rtp
            Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _Rfloat_rtp)(Lo);
        else if (roundingMode > 1)  //rtn and rtz
            Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i32, _Rfloat_rtz)(Lo);
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
INLINE private void* SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p0i8_p4i8_i32, _ToPrivate)(generic char *Pointer, int Storage)
{
    return __builtin_IB_to_private(Pointer);
}

INLINE local   void* SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(generic char *Pointer, int Storage)
{
    return __builtin_IB_to_local(Pointer);
}

INLINE global  void* SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p1i8_p4i8_i32, _ToGlobal)(generic char *Pointer, int Storage)
{
    if((SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(Pointer, Storage) == NULL) &
       (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p0i8_p4i8_i32, _ToPrivate)(Pointer, Storage) == NULL))
    {
        return (global void*)(Pointer);
    }

    return NULL;
}

INLINE uint SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericPtrMemSemantics, _p4i8, )(generic char *Pointer)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(Pointer, StorageWorkgroup) != NULL)
    {
        return (uint)WorkgroupMemory;
    }

    return (uint)CrossWorkgroupMemory;
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


