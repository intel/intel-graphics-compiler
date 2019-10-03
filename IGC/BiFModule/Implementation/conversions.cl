/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "include/BiF_Definitions.cl"
#include "../Headers/spirv.h"

#define RT_POS 0
#define RT_NEG 1
#define RT_Z   2
#define RT_NE  3

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
  return __builtin_spirv_OpenCL_select_i16_i16_i16(_R, (ushort)0, __builtin_spirv_OpConvertFToU_i16_f16((_T < (half)0) | __builtin_spirv_OpIsNan_f16(_T)));
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static uint OVERLOADABLE sat_uint(half _T, uint _R)
{
  return __builtin_spirv_OpenCL_select_i32_i32_i32(_R, (uint)0, __builtin_spirv_OpConvertFToU_i32_f16((_T < (half)0) | __builtin_spirv_OpIsNan_f16(_T)));
}
#endif

static ulong OVERLOADABLE sat_ulong(half _T, ulong _R)
{
  return __builtin_spirv_OpenCL_select_i64_i64_i64(_R, (ulong)0, __builtin_spirv_OpConvertFToU_i64_f16((_T < (half)0) | __builtin_spirv_OpIsNan_f16(_T)));
}

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static uchar clamp_sat_uchar(half _T, uchar _R)
{
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(_R, (uchar)0, __builtin_spirv_OpConvertFToU_i8_f16(_T < (half)0));
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(_R, (uchar)UCHAR_MAX, __builtin_spirv_OpConvertFToU_i8_f16(_T > (half)UCHAR_MAX));
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(_R, (uchar)0 , __builtin_spirv_OpConvertFToU_i8_f16(__builtin_spirv_OpIsNan_f16(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static char clamp_sat_char(half _T, char _R)
{
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(_R, (char)CHAR_MIN, __builtin_spirv_OpConvertFToS_i8_f16(_T < (half)CHAR_MIN));
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(_R, (char)CHAR_MAX, __builtin_spirv_OpConvertFToS_i8_f16(_T > (half)CHAR_MAX));
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(_R, (char)0 , __builtin_spirv_OpConvertFToS_i8_f16(__builtin_spirv_OpIsNan_f16(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static short clamp_sat_short(half _T, short _R)
{
  _R = __builtin_spirv_OpenCL_select_i16_i16_i16(_R, (short)SHRT_MIN, __builtin_spirv_OpConvertFToS_i16_f16(_T < (half)SHRT_MIN));
  _R = __builtin_spirv_OpenCL_select_i16_i16_i16(_R, (short)SHRT_MAX, __builtin_spirv_OpConvertFToS_i16_f16(_T > (half)SHRT_MAX));
  _R = __builtin_spirv_OpenCL_select_i16_i16_i16(_R, (short)0 , __builtin_spirv_OpConvertFToS_i16_f16(__builtin_spirv_OpIsNan_f16(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static int clamp_sat_int(half _T, int _R)
{
  _R = __builtin_spirv_OpenCL_select_i32_i32_i32(_R, (int)INT_MIN, __builtin_spirv_OpConvertFToS_i32_f16(_T < (half)INT_MIN));
  _R = __builtin_spirv_OpenCL_select_i32_i32_i32(_R, (int)INT_MAX, __builtin_spirv_OpConvertFToS_i32_f16(_T > (half)INT_MAX));
  _R = __builtin_spirv_OpenCL_select_i32_i32_i32(_R, (int)0 , __builtin_spirv_OpConvertFToS_i32_f16(__builtin_spirv_OpIsNan_f16(_T)));
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
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)TONAME##_MIN, __builtin_spirv_OpConvertFToS##_##FROMA##_##TOA(_T < (FROM)TONAME##_MIN));  \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)TONAME##_MAX, __builtin_spirv_OpConvertFToS##_##FROMA##_##TOA(_T > (FROM)TONAME##_MAX));  \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)0, __builtin_spirv_OpConvertFToS##_##FROMA##_##TOA(__builtin_spirv_OpIsNan##_##TOA(_T))); \
  return _R;                                                                \
}

#define SAT_CLAMP_HELPER_UNSIGNED(TO, FROM, TONAME, TOA, INTTYPE, FROMA)    \
static TO clamp_sat_##TO##_##FROM(TO _R, FROM _T)                           \
{                                                                           \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)TONAME##_MIN, __builtin_spirv_OpConvertFToU##_##FROMA##_##TOA(_T < (FROM)TONAME##_MIN));  \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)TONAME##_MAX, __builtin_spirv_OpConvertFToU##_##FROMA##_##TOA(_T > (FROM)TONAME##_MAX));  \
  _R = __builtin_spirv_OpenCL_select_##TOA##_##TOA##_##INTTYPE(_R, (TO)0, __builtin_spirv_OpConvertFToU##_##FROMA##_##TOA(__builtin_spirv_OpIsNan##_##TOA(_T))); \
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

uchar  __builtin_spirv_OpConvertFToU_i8_f16(half FloatValue)
{
    return FloatValue;
}

ushort __builtin_spirv_OpConvertFToU_i16_f16(half FloatValue)
{
    return FloatValue;
}

uint   __builtin_spirv_OpConvertFToU_i32_f16(half FloatValue)
{
    return FloatValue;
}

ulong  __builtin_spirv_OpConvertFToU_i64_f16(half FloatValue)
{
    return FloatValue;
}

uchar  __builtin_spirv_OpConvertFToU_i8_f32(float FloatValue)
{
    return (uchar)FloatValue;
}

ushort __builtin_spirv_OpConvertFToU_i16_f32(float FloatValue)
{
    return (ushort)FloatValue;
}

uint   __builtin_spirv_OpConvertFToU_i32_f32(float FloatValue)
{
    return (uint)FloatValue;
}

ulong  __builtin_spirv_OpConvertFToU_i64_f32(float FloatValue)
{
    float FC0 = __builtin_spirv_OpenCL_ldexp_f32_i32(1.0f, -32);
    float FC1 = __builtin_spirv_OpenCL_ldexp_f32_i32(-1.0f, 32);
    float HiF = __builtin_spirv_OpenCL_trunc_f32(FloatValue * FC0);
    float LoF = __builtin_spirv_OpenCL_fma_f32_f32_f32(HiF, FC1, FloatValue);
    ulong answer = 0;
    answer = (((uint)HiF | answer) << 32) | ((uint)LoF | answer);
    return answer;
}

#if defined(cl_khr_fp64)

uchar  __builtin_spirv_OpConvertFToU_i8_f64(double FloatValue)
{
    return FloatValue;
}

ushort __builtin_spirv_OpConvertFToU_i16_f64(double FloatValue)
{
    return FloatValue;
}

uint   __builtin_spirv_OpConvertFToU_i32_f64(double FloatValue)
{
    return FloatValue;
}

ulong  __builtin_spirv_OpConvertFToU_i64_f64(double FloatValue)
{
    return FloatValue;
}

#endif //defined(cl_khr_fp64)

char  __builtin_spirv_OpConvertFToS_i8_f16(half FloatValue)
{
    return FloatValue;
}

short __builtin_spirv_OpConvertFToS_i16_f16(half FloatValue)
{
    return FloatValue;
}

int   __builtin_spirv_OpConvertFToS_i32_f16(half FloatValue)
{
    return FloatValue;
}

long  __builtin_spirv_OpConvertFToS_i64_f16(half FloatValue)
{
    return FloatValue;
}

char  __builtin_spirv_OpConvertFToS_i8_f32(float FloatValue)
{
    return (char)FloatValue;
}

short __builtin_spirv_OpConvertFToS_i16_f32(float FloatValue)
{
    return (short)FloatValue;
}

int   __builtin_spirv_OpConvertFToS_i32_f32(float FloatValue)
{
    return (int)FloatValue;
}

long  __builtin_spirv_OpConvertFToS_i64_f32(float FloatValue)
{
    float abs_value = __builtin_spirv_OpenCL_fabs_f32(FloatValue);
    if (abs_value < 1.0f) //for small numbers and denormals
    {
        return (long)0;
    }
    uint sign = as_uint(as_int(FloatValue) >> 31);
    float FC0 = __builtin_spirv_OpenCL_ldexp_f32_i32(1.0f, -32);
    float FC1 = __builtin_spirv_OpenCL_ldexp_f32_i32(-1.0f, 32);
    float HiF = __builtin_spirv_OpenCL_trunc_f32(abs_value * FC0);
    float LoF = __builtin_spirv_OpenCL_fma_f32_f32_f32(HiF, FC1, abs_value);
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

char  __builtin_spirv_OpConvertFToS_i8_f64(double FloatValue)
{
    return FloatValue;
}

short __builtin_spirv_OpConvertFToS_i16_f64(double FloatValue)
{
    return FloatValue;
}

int   __builtin_spirv_OpConvertFToS_i32_f64(double FloatValue)
{
    return FloatValue;
}

long  __builtin_spirv_OpConvertFToS_i64_f64(double FloatValue)
{
    return FloatValue;
}

#endif //defined(cl_khr_fp64)

half   __builtin_spirv_OpConvertSToF_f16_i8(char SignedValue)
{
    return SignedValue;
}

float  __builtin_spirv_OpConvertSToF_f32_i8(char SignedValue)
{
    return (float)SignedValue;
}

half   __builtin_spirv_OpConvertSToF_f16_i16(short SignedValue)
{
    return SignedValue;
}

float  __builtin_spirv_OpConvertSToF_f32_i16(short SignedValue)
{
    return (float)SignedValue;
}

half   __builtin_spirv_OpConvertSToF_f16_i32(int SignedValue)
{
    return SignedValue;
}

float  __builtin_spirv_OpConvertSToF_f32_i32(int SignedValue)
{
    return (float)SignedValue;
}

half   __builtin_spirv_OpConvertSToF_f16_i64(long SignedValue)
{
    return SignedValue;
}

float  __builtin_spirv_OpConvertSToF_f32_i64(long SignedValue)
{
    return convertSItoFP32(SignedValue, 0);
}

half   __builtin_spirv_OpConvertUToF_f16_i8(uchar UnsignedValue)
{
    return UnsignedValue;
}
float  __builtin_spirv_OpConvertUToF_f32_i1(bool BoolValue)
{
    return BoolValue ? 1.0 : 0.0;
}

float  __builtin_spirv_OpConvertUToF_f32_i8(uchar UnsignedValue)
{
    return (float)UnsignedValue;
}

half   __builtin_spirv_OpConvertUToF_f16_i16(ushort UnsignedValue)
{
    return UnsignedValue;
}

float  __builtin_spirv_OpConvertUToF_f32_i16(ushort UnsignedValue)
{
    return (float)UnsignedValue;
}

half   __builtin_spirv_OpConvertUToF_f16_i32(uint UnsignedValue)
{
    return UnsignedValue;
}

float  __builtin_spirv_OpConvertUToF_f32_i32(uint UnsignedValue)
{
    return (float)UnsignedValue;
}

half   __builtin_spirv_OpConvertUToF_f16_i64(ulong UnsignedValue)
{
    return UnsignedValue;
}

float  __builtin_spirv_OpConvertUToF_f32_i64(ulong UnsignedValue)
{
    return convertUItoFP32(UnsignedValue, 0, false);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpConvertSToF_f64_i8(char SignedValue)
{
    return SignedValue;
}

double __builtin_spirv_OpConvertSToF_f64_i16(short SignedValue)
{
    return SignedValue;
}

double __builtin_spirv_OpConvertSToF_f64_i32(int SignedValue)
{
    return SignedValue;
}

double __builtin_spirv_OpConvertSToF_f64_i64(long SignedValue)
{
    return SignedValue;
}

double  __builtin_spirv_OpConvertUToF_f64_i1(bool BoolValue)
{
    return BoolValue ? 1.0 : 0.0;
}

double __builtin_spirv_OpConvertUToF_f64_i8(uchar UnsignedValue)
{
    return UnsignedValue;
}

double __builtin_spirv_OpConvertUToF_f64_i16(ushort UnsignedValue)
{
    return UnsignedValue;
}

double __builtin_spirv_OpConvertUToF_f64_i32(uint UnsignedValue)
{
    return UnsignedValue;
}

double __builtin_spirv_OpConvertUToF_f64_i64(ulong UnsignedValue)
{
    return UnsignedValue;
}

#endif // defined(cl_khr_fp64)


uchar  __builtin_spirv_OpUConvert_i8_i8(uchar UnsignedValue)
{
    return UnsignedValue;
}

ushort __builtin_spirv_OpUConvert_i16_i8(uchar UnsignedValue)
{
    return (ushort)UnsignedValue;
}

uint   __builtin_spirv_OpUConvert_i32_i8(uchar UnsignedValue)
{
    return (uint)UnsignedValue;
}

ulong  __builtin_spirv_OpUConvert_i64_i8(uchar UnsignedValue)
{
    return (ulong)UnsignedValue;
}

uchar  __builtin_spirv_OpUConvert_i8_i16(ushort UnsignedValue)
{
    return (uchar)UnsignedValue;
}

ushort __builtin_spirv_OpUConvert_i16_i16(ushort UnsignedValue)
{
    return UnsignedValue;
}

uint   __builtin_spirv_OpUConvert_i32_i16(ushort UnsignedValue)
{
    return (uint)UnsignedValue;
}

ulong  __builtin_spirv_OpUConvert_i64_i16(ushort UnsignedValue)
{
    return (ulong)UnsignedValue;
}

uchar  __builtin_spirv_OpUConvert_i8_i32(uint UnsignedValue)
{
    return (uchar)UnsignedValue;
}

ushort __builtin_spirv_OpUConvert_i16_i32(uint UnsignedValue)
{
    return (ushort)UnsignedValue;
}

uint   __builtin_spirv_OpUConvert_i32_i32(uint UnsignedValue)
{
    return UnsignedValue;
}

ulong  __builtin_spirv_OpUConvert_i64_i32(uint UnsignedValue)
{
    return (ulong)UnsignedValue;
}

uchar  __builtin_spirv_OpUConvert_i8_i64(ulong UnsignedValue)
{
    return (uchar)UnsignedValue;
}

ushort __builtin_spirv_OpUConvert_i16_i64(ulong UnsignedValue)
{
    return (ushort)UnsignedValue;
}

uint   __builtin_spirv_OpUConvert_i32_i64(ulong UnsignedValue)
{
    return (uint)UnsignedValue;
}

ulong  __builtin_spirv_OpUConvert_i64_i64(ulong UnsignedValue)
{
    return UnsignedValue;
}

char  __builtin_spirv_OpSConvert_i8_i8(char SignedValue)
{
    return SignedValue;
}

short __builtin_spirv_OpSConvert_i16_i8(char SignedValue)
{
    return (short)SignedValue;
}

int   __builtin_spirv_OpSConvert_i32_i8(char SignedValue)
{
    return (int)SignedValue;
}

long  __builtin_spirv_OpSConvert_i64_i8(char SignedValue)
{
    return (long)SignedValue;
}

char  __builtin_spirv_OpSConvert_i8_i16(short SignedValue)
{
    return (char)SignedValue;
}

short __builtin_spirv_OpSConvert_i16_i16(short SignedValue)
{
    return SignedValue;
}

int   __builtin_spirv_OpSConvert_i32_i16(short SignedValue)
{
    return (int)SignedValue;
}

long  __builtin_spirv_OpSConvert_i64_i16(short SignedValue)
{
    return (long)SignedValue;
}

char  __builtin_spirv_OpSConvert_i8_i32(int SignedValue)
{
    return (char)SignedValue;
}

short __builtin_spirv_OpSConvert_i16_i32(int SignedValue)
{
    return (short)SignedValue;
}

int   __builtin_spirv_OpSConvert_i32_i32(int SignedValue)
{
    return SignedValue;
}

long  __builtin_spirv_OpSConvert_i64_i32(int SignedValue)
{
    return (long)SignedValue;
}

char  __builtin_spirv_OpSConvert_i8_i64(long SignedValue)
{
    return (char)SignedValue;
}

short __builtin_spirv_OpSConvert_i16_i64(long SignedValue)
{
    return (short)SignedValue;
}

int   __builtin_spirv_OpSConvert_i32_i64(long SignedValue)
{
    return (int)SignedValue;
}

long  __builtin_spirv_OpSConvert_i64_i64(long SignedValue)
{
    return SignedValue;
}

half   __builtin_spirv_OpFConvert_f16_f16(half FloatValue)
{
    return FloatValue;
}

float  __builtin_spirv_OpFConvert_f32_f16(half FloatValue)
{
    return FloatValue;
}

half   __builtin_spirv_OpFConvert_f16_f32(float FloatValue)
{
    return FloatValue;
}

float  __builtin_spirv_OpFConvert_f32_f32(float FloatValue)
{
    return FloatValue;
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpFConvert_f64_f16(half FloatValue)
{
    return FloatValue;
}

double __builtin_spirv_OpFConvert_f64_f32(float FloatValue)
{
    return FloatValue;
}

half   __builtin_spirv_OpFConvert_f16_f64(double FloatValue)
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

float  __builtin_spirv_OpFConvert_f32_f64(double FloatValue)
{
    return FloatValue;
}

double __builtin_spirv_OpFConvert_f64_f64(double FloatValue)
{
    return FloatValue;
}

#endif // defined(cl_khr_fp64)

// OpConvertPtrToU -> ptrtoint, no need.

uchar  __builtin_spirv_OpSatConvertSToU_i8_i8(char SignedValue)
{
      //return __builtin_IB_ctouc_sat((char)SignedValue);
      if (SignedValue <= 0)
      {
        return (uchar)0;
      }
      return (uchar)SignedValue;
}

ushort __builtin_spirv_OpSatConvertSToU_i16_i8(char SignedValue)
{
      //return __builtin_IB_ctous_sat((char)SignedValue);
      if( SignedValue <= 0)
      {
        return 0;
      }
      return (ushort)SignedValue;
}

uint  __builtin_spirv_OpSatConvertSToU_i32_i8(char SignedValue)
{
      //return __builtin_IB_ctoui_sat((char)SignedValue);
      return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong  __builtin_spirv_OpSatConvertSToU_i64_i8(char SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar  __builtin_spirv_OpSatConvertSToU_i8_i16(short SignedValue)
{
  return (uchar)clamp(SignedValue, (short)0, (short)UCHAR_MAX);
}

ushort __builtin_spirv_OpSatConvertSToU_i16_i16(short SignedValue)
{
      //return __builtin_IB_stous_sat((short)SignedValue);
      if( SignedValue <= 0)
      {
        return 0;
      }
      return (ushort)SignedValue;
}

uint   __builtin_spirv_OpSatConvertSToU_i32_i16(short SignedValue)
{
      //return __builtin_IB_stoui_sat((short)SignedValue);
      return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong  __builtin_spirv_OpSatConvertSToU_i64_i16(short SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar  __builtin_spirv_OpSatConvertSToU_i8_i32(int SignedValue)
{
  return (uchar)clamp(SignedValue, 0, (int)UCHAR_MAX);
}

ushort __builtin_spirv_OpSatConvertSToU_i16_i32(int SignedValue)
{
      //return __builtin_IB_itous_sat((int)SignedValue);
      int res = __builtin_spirv_OpenCL_s_clamp_i32_i32_i32(SignedValue, 0, (int)USHRT_MAX);
      return (ushort)res;
}

uint   __builtin_spirv_OpSatConvertSToU_i32_i32(int SignedValue)
{
      //return __builtin_IB_itoui_sat((int)SignedValue);
      return (SignedValue <= 0) ? 0 : (uint)SignedValue;
}

ulong  __builtin_spirv_OpSatConvertSToU_i64_i32(int SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}

uchar  __builtin_spirv_OpSatConvertSToU_i8_i64(long SignedValue)
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

ushort __builtin_spirv_OpSatConvertSToU_i16_i64(long SignedValue)
{
    if (SignedValue <= 0) {
        return 0;
      }
    else if (SignedValue >= USHRT_MAX) {
        return USHRT_MAX;
      }
      return (ushort)SignedValue;
}

uint   __builtin_spirv_OpSatConvertSToU_i32_i64(long SignedValue)
{
    if (SignedValue <= 0) {
        return 0;
      }
    else if (SignedValue >= UINT_MAX) {
        return UINT_MAX;
      }
      return (uint)SignedValue;
}

ulong  __builtin_spirv_OpSatConvertSToU_i64_i64(long SignedValue)
{
    return (SignedValue <= 0) ? 0 : (ulong)SignedValue;
}


char  __builtin_spirv_OpSatConvertUToS_i8_i8(uchar UnsignedValue)
{
      //return __builtin_IB_uctoc_sat((uchar)UnsignedValue);
      return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __builtin_spirv_OpSatConvertUToS_i16_i8(uchar UnsignedValue)
{
    return (short)UnsignedValue;
}

int   __builtin_spirv_OpSatConvertUToS_i32_i8(uchar UnsignedValue)
{
    return (int)UnsignedValue;
}

long  __builtin_spirv_OpSatConvertUToS_i64_i8(uchar UnsignedValue)
{
    return (long)UnsignedValue;
}

char  __builtin_spirv_OpSatConvertUToS_i8_i16(ushort UnsignedValue)
{
      //return __builtin_IB_ustoc_sat((ushort)UnsignedValue);
      return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __builtin_spirv_OpSatConvertUToS_i16_i16(ushort UnsignedValue)
{
      //return __builtin_IB_ustos_sat((ushort)UnsignedValue);
      return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (ushort)UnsignedValue;
}

int   __builtin_spirv_OpSatConvertUToS_i32_i16(ushort UnsignedValue)
{
    return (int)UnsignedValue;
}

long  __builtin_spirv_OpSatConvertUToS_i64_i16(ushort UnsignedValue)
{
    return (long)UnsignedValue;
}

char  __builtin_spirv_OpSatConvertUToS_i8_i32(uint UnsignedValue)
{
      //return __builtin_IB_uitoc_sat((uint)UnsignedValue);
      return (UnsignedValue > CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __builtin_spirv_OpSatConvertUToS_i16_i32(uint UnsignedValue)
{
      //return __builtin_IB_uitos_sat((uint)UnsignedValue);
      return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (short)UnsignedValue;
}

int __builtin_spirv_OpSatConvertUToS_i32_i32(uint UnsignedValue)
{
      //return __builtin_IB_uitoi_sat((uint)UnsignedValue);
      return (UnsignedValue >= INT_MAX) ? INT_MAX : (int)UnsignedValue;
}

long  __builtin_spirv_OpSatConvertUToS_i64_i32(uint UnsignedValue)
{
    return (long)UnsignedValue;
}

char  __builtin_spirv_OpSatConvertUToS_i8_i64(ulong UnsignedValue)
{
    return (UnsignedValue >= CHAR_MAX) ? CHAR_MAX : (char)UnsignedValue;
}

short __builtin_spirv_OpSatConvertUToS_i16_i64(ulong UnsignedValue)
{
    return (UnsignedValue >= SHRT_MAX) ? SHRT_MAX : (short)UnsignedValue;
}

int   __builtin_spirv_OpSatConvertUToS_i32_i64(ulong UnsignedValue)
{
    return (UnsignedValue >= INT_MAX) ? INT_MAX : (int)UnsignedValue;
}

long  __builtin_spirv_OpSatConvertUToS_i64_i64(ulong UnsignedValue)
{
    return (UnsignedValue >= LONG_MAX) ? LONG_MAX : __builtin_spirv_OpSConvert_i64_i64(UnsignedValue);
}


/*
// Next is all Scalar types with Rounding modes [RTE,RTZ,RTN,RTP] and Sat
//
//
//
//
*/

uchar  __builtin_spirv_OpConvertFToU_RTE_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i8_f16(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_RTZ_i8_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i8_f16(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_RTP_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i8_f16(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_RTN_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i8_f16(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_i8_f16(half FloatValue)
{
  uchar normal = __builtin_spirv_OpConvertFToU_i8_f16(FloatValue);
  return clamp_sat_uchar(FloatValue, normal);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTE_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i8_f16(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTZ_i8_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i8_f16(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTP_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i8_f16(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTN_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i8_f16(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTE_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i16_f16(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTZ_i16_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i16_f16(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTP_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i16_f16(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTN_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i16_f16(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_i16_f16(half FloatValue)
{
  ushort normal = __builtin_spirv_OpConvertFToU_i16_f16(FloatValue);
  return sat_ushort(FloatValue, normal);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTE_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i16_f16(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTZ_i16_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i16_f16(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTP_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i16_f16(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTN_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i16_f16(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTE_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i32_f16(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTZ_i32_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i32_f16(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTP_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i32_f16(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTN_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i32_f16(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_i32_f16(half FloatValue)
{
  uint normal = __builtin_spirv_OpConvertFToU_i32_f16(FloatValue);
  return sat_uint(FloatValue, normal);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTE_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i32_f16(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTZ_i32_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i32_f16(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTP_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i32_f16(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTN_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i32_f16(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTE_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i64_f16(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTZ_i64_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i64_f16(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTP_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i64_f16(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTN_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_i64_f16(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_i64_f16(half FloatValue)
{
  ulong normal = __builtin_spirv_OpConvertFToU_i64_f16(FloatValue);
  return sat_ulong(FloatValue, normal);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTE_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i64_f16(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTZ_i64_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i64_f16(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTP_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i64_f16(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTN_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i64_f16(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_RTE_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_i8_f32(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_RTZ_i8_f32(float FloatValue)
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

uchar  __builtin_spirv_OpConvertFToU_RTP_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_i8_f32(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_RTN_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_i8_f32(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_i8_f32(float FloatValue)
{
  //return __builtin_IB_ftouc_sat((float)FloatValue);
  float res = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(FloatValue, 0.0f, (float)UCHAR_MAX);
  return (uchar)res;
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTE_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i8_f32(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTZ_i8_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i8_f32(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTP_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_Sat_i8_f32(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTN_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_Sat_i8_f32(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTE_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToU_i16_f32(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTZ_i16_f32(float FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= USHRT_MAX) {
    return USHRT_MAX;
  }
  return (ushort)FloatValue;
}

ushort __builtin_spirv_OpConvertFToU_RTP_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_i16_f32(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTN_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_i16_f32(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_i16_f32(float FloatValue)
{
  //return __builtin_IB_ftous_sat((float)FloatValue);
  float res = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(FloatValue, 0.0f, (float)USHRT_MAX);
  return (ushort)res;
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTE_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i16_f32(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTZ_i16_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i16_f32(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTP_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_Sat_i16_f32(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTN_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_Sat_i16_f32(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTE_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToU_i32_f32(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTZ_i32_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i32_f32(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTP_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_i32_f32(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTN_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_i32_f32(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_i32_f32(float FloatValue)
{
    return __builtin_spirv_OpConvertFToU_Sat_RTZ_i32_f32(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTE_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i32_f32(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTZ_i32_f32(float FloatValue)
{
  uint _R = __builtin_spirv_OpConvertFToU_RTZ_i32_f32(FloatValue);
  _R = __builtin_spirv_OpenCL_select_i32_i32_i32(_R, (uint)UINT_MAX, __builtin_spirv_OpConvertFToU_i32_f32(FloatValue > (float)UINT_MAX));
  return __builtin_spirv_OpenCL_select_i32_i32_i32(_R, (uint)0, __builtin_spirv_OpConvertFToU_i32_f32((FloatValue < (float)0) | __builtin_spirv_OpIsNan_f32(FloatValue)));
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTP_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_Sat_i32_f32(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTN_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_Sat_i32_f32(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTE_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToU_i64_f32(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTZ_i64_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i64_f32(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTP_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_i64_f32(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTN_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_i64_f32(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_i64_f32(float FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= ULONG_MAX) {
    return ULONG_MAX;
  }
  return __builtin_spirv_OpConvertFToU_i64_f32(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTE_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i64_f32(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTZ_i64_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i64_f32(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTP_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_Sat_i64_f32(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTN_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToU_Sat_i64_f32(FloatValue);
}

#if defined(cl_khr_fp64)

uchar  __builtin_spirv_OpConvertFToU_RTE_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i8_f64(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_RTZ_i8_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i8_f64(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_RTP_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i8_f64(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_RTN_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i8_f64(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_i8_f64(double FloatValue)
{
  uchar normal = __builtin_spirv_OpConvertFToU_i8_f64(FloatValue);
  return clamp_sat_uchar_double(normal, FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_i16_f64(double FloatValue)
{
  ushort normal = __builtin_spirv_OpConvertFToU_i16_f64(FloatValue);
  return clamp_sat_ushort_double(normal, FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_i32_f64(double FloatValue)
{
  uint normal = __builtin_spirv_OpConvertFToU_i32_f64(FloatValue);
  return clamp_sat_uint_double(normal, FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_i8_f64(double FloatValue)
{
  char normal = __builtin_spirv_OpConvertFToS_i8_f64(FloatValue);
  return clamp_sat_char_double(normal, FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_i16_f64(double FloatValue)
{
  short normal = __builtin_spirv_OpConvertFToS_i16_f64(FloatValue);
  return clamp_sat_short_double(normal, FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_i32_f64(double FloatValue)
{
  int normal = __builtin_spirv_OpConvertFToS_i32_f64(FloatValue);
  return clamp_sat_int_double(normal, FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTE_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i8_f64(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTZ_i8_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i8_f64(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTP_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i8_f64(FloatValue);
}

uchar  __builtin_spirv_OpConvertFToU_Sat_RTN_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i8_f64(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTE_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i16_f64(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTZ_i16_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i16_f64(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTP_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i16_f64(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_RTN_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i16_f64(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTE_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i16_f64(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTZ_i16_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i16_f64(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTP_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i16_f64(FloatValue);
}

ushort __builtin_spirv_OpConvertFToU_Sat_RTN_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i16_f64(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTE_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i32_f64(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTZ_i32_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i32_f64(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTP_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i32_f64(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_RTN_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i32_f64(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTE_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i32_f64(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTZ_i32_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i32_f64(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTP_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i32_f64(FloatValue);
}

uint   __builtin_spirv_OpConvertFToU_Sat_RTN_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i32_f64(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTE_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i64_f64(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTZ_i64_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToU_i64_f64(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTP_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i64_f64(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_RTN_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_i64_f64(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_i64_f64(double FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= ULONG_MAX) {
    return ULONG_MAX;
  }
  return FloatValue;
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTE_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i64_f64(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTZ_i64_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToU_Sat_i64_f64(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTP_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i64_f64(FloatValue);
}

ulong  __builtin_spirv_OpConvertFToU_Sat_RTN_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToU_Sat_i64_f64(FloatValue);
}

#endif // defined(cl_khr_fp64)

char  __builtin_spirv_OpConvertFToS_RTE_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i8_f16(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_RTZ_i8_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i8_f16(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_RTP_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i8_f16(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_RTN_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i8_f16(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_i8_f16(half FloatValue)
{
  char normal = __builtin_spirv_OpConvertFToS_i8_f16(FloatValue);
  return clamp_sat_char(FloatValue, normal);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTE_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i8_f16(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTZ_i8_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i8_f16(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTP_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i8_f16(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTN_i8_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i8_f16(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTE_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i16_f16(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTZ_i16_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i16_f16(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTP_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i16_f16(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTN_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i16_f16(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_i16_f16(half FloatValue)
{
  short normal = __builtin_spirv_OpConvertFToS_i16_f16(FloatValue);
  return clamp_sat_short(FloatValue, normal);
}

short __builtin_spirv_OpConvertFToS_Sat_RTE_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i16_f16(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTZ_i16_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i16_f16(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTP_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i16_f16(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTN_i16_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i16_f16(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTE_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i32_f16(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTZ_i32_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i32_f16(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTP_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i32_f16(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTN_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i32_f16(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_i32_f16(half FloatValue)
{
  int normal = __builtin_spirv_OpConvertFToS_i32_f16(FloatValue);
  return clamp_sat_int(FloatValue, normal);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTE_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i32_f16(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTZ_i32_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i32_f16(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTP_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i32_f16(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTN_i32_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i32_f16(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTE_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i64_f16(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTZ_i64_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i64_f16(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTP_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i64_f16(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTN_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_i64_f16(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_i64_f16(half FloatValue)
{
  if (FloatValue <= LONG_MIN) {
    return LONG_MIN;
  } else if (FloatValue >= LONG_MAX) {
    return LONG_MAX;
  }
  return FloatValue;
}

long  __builtin_spirv_OpConvertFToS_Sat_RTE_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i64_f16(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTZ_i64_f16(half FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i64_f16(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTP_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i64_f16(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTN_i64_f16(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i64_f16(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_RTE_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToS_i8_f32(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_RTZ_i8_f32(float FloatValue)
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

char  __builtin_spirv_OpConvertFToS_RTP_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_i8_f32(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_RTN_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_i8_f32(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_i8_f32(float FloatValue)
{
  float res = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(FloatValue, (float)CHAR_MIN, (float)CHAR_MAX);
  res = __builtin_spirv_OpenCL_select_f32_f32_i32(res, 0.0f , __builtin_spirv_OpIsNan_f32(FloatValue));
  return (char)res;
}

char  __builtin_spirv_OpConvertFToS_Sat_RTE_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i8_f32(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTZ_i8_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i8_f32(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTP_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_Sat_i8_f32(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTN_i8_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_Sat_i8_f32(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTE_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToS_i16_f32(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTZ_i16_f32(float FloatValue)
{
  if (FloatValue <= SHRT_MIN) {
    return SHRT_MIN;
  } else if (FloatValue >= SHRT_MAX) {
    return SHRT_MAX;
  }
  return (short)FloatValue;
}

short __builtin_spirv_OpConvertFToS_RTP_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_i16_f32(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTN_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_i16_f32(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_i16_f32(float FloatValue)
{
  float res = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(FloatValue, (float)SHRT_MIN, (float)SHRT_MAX);
  res = __builtin_spirv_OpenCL_select_f32_f32_i32(res, 0.0f , __builtin_spirv_OpIsNan_f32(FloatValue));
  return (short)res;
}

short __builtin_spirv_OpConvertFToS_Sat_RTE_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i16_f32(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTZ_i16_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i16_f32(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTP_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_Sat_i16_f32(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTN_i16_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_Sat_i16_f32(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTE_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToS_i32_f32(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTZ_i32_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i32_f32(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTP_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_i32_f32(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTN_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_i32_f32(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_i32_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_RTZ_i32_f32(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTE_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i32_f32(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTZ_i32_f32(float FloatValue)
{
    int _R = __builtin_spirv_OpConvertFToS_RTZ_i32_f32(FloatValue);
    _R = __builtin_spirv_OpenCL_select_i32_i32_i32(_R, (int)INT_MIN, __builtin_spirv_OpConvertFToS_i32_f32(FloatValue < (float)INT_MIN));
    _R = __builtin_spirv_OpenCL_select_i32_i32_i32(_R, (int)INT_MAX, __builtin_spirv_OpConvertFToS_i32_f32(FloatValue > (float)INT_MAX));
    return __builtin_spirv_OpenCL_select_i32_i32_i32(_R, (int)0, __builtin_spirv_OpConvertFToS_i32_f32(__builtin_spirv_OpIsNan_f32(FloatValue)));
}

int   __builtin_spirv_OpConvertFToS_Sat_RTP_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_Sat_i32_f32(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTN_i32_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_Sat_i32_f32(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTE_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToS_i64_f32(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTZ_i64_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i64_f32(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTP_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_i64_f32(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTN_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_i64_f32(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_i64_f32(float FloatValue)
{
  if (FloatValue <= LONG_MIN) {
    return LONG_MIN;
  } else if (FloatValue >= LONG_MAX) {
    return LONG_MAX;
  }
  return __builtin_spirv_OpConvertFToS_i64_f32(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTE_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i64_f32(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTZ_i64_f32(float FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i64_f32(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTP_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_Sat_i64_f32(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTN_i64_f32(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return __builtin_spirv_OpConvertFToS_Sat_i64_f32(FloatValue);
}

#if defined(cl_khr_fp64)

char  __builtin_spirv_OpConvertFToS_RTE_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i8_f64(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_RTZ_i8_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i8_f64(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_RTP_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i8_f64(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_RTN_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i8_f64(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTE_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i8_f64(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTZ_i8_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i8_f64(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTP_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i8_f64(FloatValue);
}

char  __builtin_spirv_OpConvertFToS_Sat_RTN_i8_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i8_f64(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTE_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i16_f64(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTZ_i16_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i16_f64(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTP_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i16_f64(FloatValue);
}

short __builtin_spirv_OpConvertFToS_RTN_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i16_f64(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTE_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i16_f64(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTZ_i16_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i16_f64(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTP_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i16_f64(FloatValue);
}

short __builtin_spirv_OpConvertFToS_Sat_RTN_i16_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i16_f64(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTE_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i32_f64(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTZ_i32_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i32_f64(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTP_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i32_f64(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_RTN_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i32_f64(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTE_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i32_f64(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTZ_i32_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i32_f64(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTP_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i32_f64(FloatValue);
}

int   __builtin_spirv_OpConvertFToS_Sat_RTN_i32_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i32_f64(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTE_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i64_f64(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTZ_i64_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToS_i64_f64(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTP_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i64_f64(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_RTN_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_i64_f64(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_i64_f64(double FloatValue)
{
  if (FloatValue <= LONG_MIN) {
    return LONG_MIN;
  } else if (FloatValue >= LONG_MAX) {
    return LONG_MAX;
  }
  return FloatValue;
}

long  __builtin_spirv_OpConvertFToS_Sat_RTE_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i64_f64(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTZ_i64_f64(double FloatValue)
{
  return __builtin_spirv_OpConvertFToS_Sat_i64_f64(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTP_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i64_f64(FloatValue);
}

long  __builtin_spirv_OpConvertFToS_Sat_RTN_i64_f64(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return __builtin_spirv_OpConvertFToS_Sat_i64_f64(FloatValue);
}

#endif  // defined(cl_khr_fp64)

half   __builtin_spirv_OpConvertSToF_RTE_f16_i8(char SignedValue)
{
  return SignedValue;
}

half   __builtin_spirv_OpConvertSToF_RTZ_f16_i8(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

half   __builtin_spirv_OpConvertSToF_RTP_f16_i8(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

half   __builtin_spirv_OpConvertSToF_RTN_f16_i8(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

float  __builtin_spirv_OpConvertSToF_RTE_f32_i8(char SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i8(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTZ_f32_i8(char SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i8(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTP_f32_i8(char SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i8(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTN_f32_i8(char SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i8(SignedValue);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpConvertSToF_RTE_f64_i8(char SignedValue)
{
  return SignedValue;
}

double __builtin_spirv_OpConvertSToF_RTZ_f64_i8(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

double __builtin_spirv_OpConvertSToF_RTP_f64_i8(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

double __builtin_spirv_OpConvertSToF_RTN_f64_i8(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

#endif //defined(cl_khr_fp64)

half   __builtin_spirv_OpConvertSToF_RTE_f16_i16(short SignedValue)
{
  return SignedValue;
}

half   __builtin_spirv_OpConvertSToF_RTZ_f16_i16(short SignedValue)
{
  float f = __builtin_spirv_OpConvertSToF_f32_i16(SignedValue);
  return __builtin_spirv_OpFConvert_RTZ_f16_f32(f);
}

half   __builtin_spirv_OpConvertSToF_RTP_f16_i16(short SignedValue)
{
  float f = __builtin_spirv_OpConvertSToF_f32_i16(SignedValue);
  return __builtin_spirv_OpFConvert_RTP_f16_f32(f);
}

half   __builtin_spirv_OpConvertSToF_RTN_f16_i16(short SignedValue)
{
  float f = __builtin_spirv_OpConvertSToF_f32_i16(SignedValue);
  return __builtin_spirv_OpFConvert_RTN_f16_f32(f);
}

float  __builtin_spirv_OpConvertSToF_RTE_f32_i16(short SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i16(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTZ_f32_i16(short SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i16(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTP_f32_i16(short SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i16(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTN_f32_i16(short SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i16(SignedValue);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpConvertSToF_RTE_f64_i16(short SignedValue)
{
  return SignedValue;
}

double __builtin_spirv_OpConvertSToF_RTZ_f64_i16(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

double __builtin_spirv_OpConvertSToF_RTP_f64_i16(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

double __builtin_spirv_OpConvertSToF_RTN_f64_i16(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

#endif  // defined(cl_khr_fp64)

half   __builtin_spirv_OpConvertSToF_RTE_f16_i32(int SignedValue)
{
  return SignedValue;
}

half   __builtin_spirv_OpConvertSToF_RTZ_f16_i32(int SignedValue)
{
  float f = __builtin_spirv_OpConvertSToF_f32_i32(SignedValue);
  return __builtin_spirv_OpFConvert_RTZ_f16_f32(f);
}

half   __builtin_spirv_OpConvertSToF_RTP_f16_i32(int SignedValue)
{
  float f = __builtin_spirv_OpConvertSToF_f32_i32(SignedValue);
  return __builtin_spirv_OpFConvert_RTP_f16_f32(f);
}

half   __builtin_spirv_OpConvertSToF_RTN_f16_i32(int SignedValue)
{
  float f = __builtin_spirv_OpConvertSToF_f32_i32(SignedValue);
  return __builtin_spirv_OpFConvert_RTN_f16_f32(f);
}

float  __builtin_spirv_OpConvertSToF_RTE_f32_i32(int SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i32(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTZ_f32_i32(int SignedValue)
{
  return __builtin_IB_itof_rtz(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTP_f32_i32(int SignedValue)
{
  return __builtin_IB_itof_rtp(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTN_f32_i32(int SignedValue)
{
  return __builtin_IB_itof_rtn(SignedValue);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpConvertSToF_RTE_f64_i32(int SignedValue)
{
  return SignedValue;
}

double __builtin_spirv_OpConvertSToF_RTZ_f64_i32(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

double __builtin_spirv_OpConvertSToF_RTP_f64_i32(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

double __builtin_spirv_OpConvertSToF_RTN_f64_i32(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

#endif  // defined(cl_khr_fp64)

half   __builtin_spirv_OpConvertSToF_RTE_f16_i64(long SignedValue)
{
  return SignedValue;
}

half   __builtin_spirv_OpConvertSToF_RTZ_f16_i64(long SignedValue)
{
  float f = __builtin_spirv_OpConvertSToF_f32_i64(SignedValue);
  return __builtin_spirv_OpFConvert_RTZ_f16_f32(f);
}

half   __builtin_spirv_OpConvertSToF_RTP_f16_i64(long SignedValue)
{
  float f = __builtin_spirv_OpConvertSToF_f32_i64(SignedValue);
  return __builtin_spirv_OpFConvert_RTP_f16_f32(f);
}

half   __builtin_spirv_OpConvertSToF_RTN_f16_i64(long SignedValue)
{
  float f = __builtin_spirv_OpConvertSToF_f32_i64(SignedValue);
  return __builtin_spirv_OpFConvert_RTN_f16_f32(f);
}

float  __builtin_spirv_OpConvertSToF_RTE_f32_i64(long SignedValue)
{
  return __builtin_spirv_OpConvertSToF_f32_i64(SignedValue);
}

float  __builtin_spirv_OpConvertSToF_RTZ_f32_i64(long SignedValue)
{
  return convertSItoFP32(SignedValue, 3);
}

float  __builtin_spirv_OpConvertSToF_RTP_f32_i64(long SignedValue)
{
  return convertSItoFP32(SignedValue, 1);
}

float  __builtin_spirv_OpConvertSToF_RTN_f32_i64(long SignedValue)
{
  return convertSItoFP32(SignedValue, 2);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpConvertSToF_RTE_f64_i64(long SignedValue)
{
  return SignedValue;
}

double __builtin_spirv_OpConvertSToF_RTZ_f64_i64(long SignedValue)
{
  return __builtin_IB_itofp64_rtz(SignedValue);
}

double __builtin_spirv_OpConvertSToF_RTP_f64_i64(long SignedValue)
{
  return __builtin_IB_itofp64_rtp(SignedValue);
}

double __builtin_spirv_OpConvertSToF_RTN_f64_i64(long SignedValue)
{
  return __builtin_IB_itofp64_rtn(SignedValue);
}

#endif  // defined(cl_khr_fp64)

half   __builtin_spirv_OpConvertUToF_RTE_f16_i8(uchar UnsignedValue)
{
  return UnsignedValue;
}

half   __builtin_spirv_OpConvertUToF_RTZ_f16_i8(uchar UnsignedValue)
{
  return UnsignedValue;    // A half can represent a uchar exactly.
}

half   __builtin_spirv_OpConvertUToF_RTP_f16_i8(uchar UnsignedValue)
{
  return UnsignedValue;
}

half   __builtin_spirv_OpConvertUToF_RTN_f16_i8(uchar UnsignedValue)
{
  return UnsignedValue;    // A half can represent a uchar exactly.
}

float  __builtin_spirv_OpConvertUToF_RTE_f32_i8(uchar UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i8(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTZ_f32_i8(uchar UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i8(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTP_f32_i8(uchar UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i8(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTN_f32_i8(uchar UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i8(UnsignedValue);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpConvertUToF_RTE_f64_i8(uchar UnsignedValue)
{
  return UnsignedValue;
}

double __builtin_spirv_OpConvertUToF_RTZ_f64_i8(uchar UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uchar exactly.
}

double __builtin_spirv_OpConvertUToF_RTP_f64_i8(uchar UnsignedValue)
{
  return UnsignedValue;
}

double __builtin_spirv_OpConvertUToF_RTN_f64_i8(uchar UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uchar exactly.
}

#endif  // defined(cl_khr_fp64)

half   __builtin_spirv_OpConvertUToF_RTE_f16_i16(ushort UnsignedValue)
{
  return UnsignedValue;
}

half   __builtin_spirv_OpConvertUToF_RTZ_f16_i16(ushort UnsignedValue)
{
  float f = __builtin_spirv_OpConvertUToF_f32_i16(UnsignedValue);
  return __builtin_spirv_OpFConvert_RTZ_f16_f32(f);
}

half   __builtin_spirv_OpConvertUToF_RTP_f16_i16(ushort UnsignedValue)
{
  float f = __builtin_spirv_OpConvertUToF_f32_i16(UnsignedValue);
  return __builtin_spirv_OpFConvert_RTP_f16_f32(f);
}

half   __builtin_spirv_OpConvertUToF_RTN_f16_i16(ushort UnsignedValue)
{
  float f = __builtin_spirv_OpConvertUToF_f32_i16(UnsignedValue);
  return __builtin_spirv_OpFConvert_RTN_f16_f32(f);
}

float  __builtin_spirv_OpConvertUToF_RTE_f32_i16(ushort UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i16(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTZ_f32_i16(ushort UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i16(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTP_f32_i16(ushort UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i16(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTN_f32_i16(ushort UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i16(UnsignedValue);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpConvertUToF_RTE_f64_i16(ushort UnsignedValue)
{
  return UnsignedValue;
}

double __builtin_spirv_OpConvertUToF_RTZ_f64_i16(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

double __builtin_spirv_OpConvertUToF_RTP_f64_i16(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

double __builtin_spirv_OpConvertUToF_RTN_f64_i16(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

#endif  // defined(cl_khr_fp64)

half   __builtin_spirv_OpConvertUToF_RTE_f16_i32(uint UnsignedValue)
{
  return UnsignedValue;
}

half   __builtin_spirv_OpConvertUToF_RTZ_f16_i32(uint UnsignedValue)
{
  float f = __builtin_spirv_OpConvertUToF_f32_i32(UnsignedValue);
  return __builtin_spirv_OpFConvert_RTZ_f16_f32(f);
}

half   __builtin_spirv_OpConvertUToF_RTP_f16_i32(uint UnsignedValue)
{
  float f = __builtin_spirv_OpConvertUToF_f32_i32(UnsignedValue);
  return __builtin_spirv_OpFConvert_RTP_f16_f32(f);
}

half   __builtin_spirv_OpConvertUToF_RTN_f16_i32(uint UnsignedValue)
{
  float f = __builtin_spirv_OpConvertUToF_f32_i32(UnsignedValue);
  return __builtin_spirv_OpFConvert_RTN_f16_f32(f);
}

float  __builtin_spirv_OpConvertUToF_RTE_f32_i32(uint UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i32(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTZ_f32_i32(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtz(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTP_f32_i32(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtp(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTN_f32_i32(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtn(UnsignedValue);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpConvertUToF_RTE_f64_i32(uint UnsignedValue)
{
  return UnsignedValue;
}

double __builtin_spirv_OpConvertUToF_RTZ_f64_i32(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

double __builtin_spirv_OpConvertUToF_RTP_f64_i32(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

double __builtin_spirv_OpConvertUToF_RTN_f64_i32(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

#endif  // defined(cl_khr_fp64)

half   __builtin_spirv_OpConvertUToF_RTE_f16_i64(ulong UnsignedValue)
{
  return UnsignedValue;
}

half   __builtin_spirv_OpConvertUToF_RTZ_f16_i64(ulong UnsignedValue)
{
  float f = __builtin_spirv_OpConvertUToF_f32_i64(UnsignedValue);
  return __builtin_spirv_OpFConvert_RTZ_f16_f32(f);
}

half   __builtin_spirv_OpConvertUToF_RTP_f16_i64(ulong UnsignedValue)
{
  float f = __builtin_spirv_OpConvertUToF_f32_i64(UnsignedValue);
  return __builtin_spirv_OpFConvert_RTP_f16_f32(f);
}

half   __builtin_spirv_OpConvertUToF_RTN_f16_i64(ulong UnsignedValue)
{
  float f = __builtin_spirv_OpConvertUToF_f32_i64(UnsignedValue);
  return __builtin_spirv_OpFConvert_RTN_f16_f32(f);
}

float  __builtin_spirv_OpConvertUToF_RTE_f32_i64(ulong UnsignedValue)
{
  return __builtin_spirv_OpConvertUToF_f32_i64(UnsignedValue);
}

float  __builtin_spirv_OpConvertUToF_RTZ_f32_i64(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 3, false);
}

float  __builtin_spirv_OpConvertUToF_RTP_f32_i64(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 1, false);
}

float  __builtin_spirv_OpConvertUToF_RTN_f32_i64(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 2, false);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpConvertUToF_RTE_f64_i64(ulong UnsignedValue)
{
  return UnsignedValue;
}

double __builtin_spirv_OpConvertUToF_RTZ_f64_i64(ulong UnsignedValue)
{
  return __builtin_IB_uitofp64_rtz(UnsignedValue);
}

double __builtin_spirv_OpConvertUToF_RTP_f64_i64(ulong UnsignedValue)
{
  return __builtin_IB_uitofp64_rtp(UnsignedValue);
}

double __builtin_spirv_OpConvertUToF_RTN_f64_i64(ulong UnsignedValue)
{
  return __builtin_IB_uitofp64_rtn(UnsignedValue);
}

#endif  // defined(cl_khr_fp64)

uchar  __builtin_spirv_OpUConvert_Sat_i8_i8(uchar UnsignedValue)
{
  return UnsignedValue;
}

ushort __builtin_spirv_OpUConvert_Sat_i16_i8(uchar UnsignedValue)
{
  return UnsignedValue;
}

uint   __builtin_spirv_OpUConvert_Sat_i32_i8(uchar UnsignedValue)
{
  return (uint)UnsignedValue;
}

ulong  __builtin_spirv_OpUConvert_Sat_i64_i8(uchar UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  __builtin_spirv_OpUConvert_Sat_i8_i16(ushort UnsignedValue)
{
      //return __builtin_IB_ustouc_sat((ushort)UnsignedValue);
      if (UnsignedValue > (uchar)0xff)
      {
        return (uchar)0xff;
      }
      return (uchar)UnsignedValue;
}

ushort __builtin_spirv_OpUConvert_Sat_i16_i16(ushort UnsignedValue)
{
  return UnsignedValue;
}

uint   __builtin_spirv_OpUConvert_Sat_i32_i16(ushort UnsignedValue)
{
  return (uint)UnsignedValue;
}

ulong  __builtin_spirv_OpUConvert_Sat_i64_i16(ushort UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  __builtin_spirv_OpUConvert_Sat_i8_i32(uint UnsignedValue)
{
      //return __builtin_IB_uitouc_sat((uint)UnsignedValue);
      if (UnsignedValue > UCHAR_MAX)
      {
        return UCHAR_MAX;
      }
      return (uchar)UnsignedValue;
}

ushort __builtin_spirv_OpUConvert_Sat_i16_i32(uint UnsignedValue)
{
      if (UnsignedValue > (ushort)0xffff) {
        return (ushort)0xffff;
      }
      return (ushort)UnsignedValue;
}

uint   __builtin_spirv_OpUConvert_Sat_i32_i32(uint UnsignedValue)
{
  return UnsignedValue;
}

ulong  __builtin_spirv_OpUConvert_Sat_i64_i32(uint UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  __builtin_spirv_OpUConvert_Sat_i8_i64(ulong UnsignedValue)
{
  return (UnsignedValue >= UCHAR_MAX) ? UCHAR_MAX : (uchar)UnsignedValue;
}

ushort __builtin_spirv_OpUConvert_Sat_i16_i64(ulong UnsignedValue)
{
  return (UnsignedValue >= USHRT_MAX) ? USHRT_MAX : (ushort)UnsignedValue;
}

uint   __builtin_spirv_OpUConvert_Sat_i32_i64(ulong UnsignedValue)
{
  return (UnsignedValue >= UINT_MAX) ? UINT_MAX : (uint)UnsignedValue;
}

ulong  __builtin_spirv_OpUConvert_Sat_i64_i64(ulong UnsignedValue)
{
  return UnsignedValue;
}

char  __builtin_spirv_OpSConvert_Sat_i8_i8(char SignedValue)
{
  return SignedValue;
}

short __builtin_spirv_OpSConvert_Sat_i16_i8(char SignedValue)
{
  return (short)SignedValue;
}

int   __builtin_spirv_OpSConvert_Sat_i32_i8(char SignedValue)
{
  return (int)SignedValue;
}

long  __builtin_spirv_OpSConvert_Sat_i64_i8(char SignedValue)
{
  return __builtin_spirv_OpSConvert_i64_i8(SignedValue);
}

char  __builtin_spirv_OpSConvert_Sat_i8_i16(short SignedValue)
{
      //return __builtin_IB_stoc_sat((short)SignedValue);
      short res = __builtin_spirv_OpenCL_s_clamp_i16_i16_i16(SignedValue, (short)CHAR_MIN, (short)CHAR_MAX);
      return (char)res;
}

short __builtin_spirv_OpSConvert_Sat_i16_i16(short SignedValue)
{
  return SignedValue;
}

int   __builtin_spirv_OpSConvert_Sat_i32_i16(short SignedValue)
{
  return (int)SignedValue;
}

long  __builtin_spirv_OpSConvert_Sat_i64_i16(short SignedValue)
{
  return __builtin_spirv_OpSConvert_i64_i16(SignedValue);
}

char  __builtin_spirv_OpSConvert_Sat_i8_i32(int SignedValue)
{
      //return __builtin_IB_itoc_sat((int)SignedValue);
      int res = __builtin_spirv_OpenCL_s_clamp_i32_i32_i32(SignedValue, (int)CHAR_MIN, (int)CHAR_MAX);
      return (char)res;
}

short __builtin_spirv_OpSConvert_Sat_i16_i32(int SignedValue)
{
      //return __builtin_IB_itos_sat((int)SignedValue);
      int res = __builtin_spirv_OpenCL_s_clamp_i32_i32_i32(SignedValue, (int)SHRT_MIN, (int)SHRT_MAX);
      return (short)res;
}

int   __builtin_spirv_OpSConvert_Sat_i32_i32(int SignedValue)
{
  return SignedValue;
}

long  __builtin_spirv_OpSConvert_Sat_i64_i32(int SignedValue)
{
  return __builtin_spirv_OpSConvert_i64_i32(SignedValue);
}

char  __builtin_spirv_OpSConvert_Sat_i8_i64(long SignedValue)
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

short __builtin_spirv_OpSConvert_Sat_i16_i64(long SignedValue)
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

int   __builtin_spirv_OpSConvert_Sat_i32_i64(long SignedValue)
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

long  __builtin_spirv_OpSConvert_Sat_i64_i64(long SignedValue)
{
  return __builtin_spirv_OpSConvert_i64_i64(SignedValue);
}

half   __builtin_spirv_OpFConvert_RTE_f16_f16(half FloatValue)
{
  return FloatValue;
}

half   __builtin_spirv_OpFConvert_RTZ_f16_f16(half FloatValue)
{
  return FloatValue;
}

half   __builtin_spirv_OpFConvert_RTP_f16_f16(half FloatValue)
{
  return FloatValue;
}

half   __builtin_spirv_OpFConvert_RTN_f16_f16(half FloatValue)
{
  return FloatValue;
}

float  __builtin_spirv_OpFConvert_RTE_f32_f16(half FloatValue)
{
  return FloatValue;
}

float  __builtin_spirv_OpFConvert_RTZ_f32_f16(half FloatValue)
{
  return FloatValue;
}

float  __builtin_spirv_OpFConvert_RTP_f32_f16(half FloatValue)
{
  return FloatValue;
}

float  __builtin_spirv_OpFConvert_RTN_f32_f16(half FloatValue)
{
  return FloatValue;
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpFConvert_RTE_f64_f16(half FloatValue)
{
  return FloatValue;
}

double __builtin_spirv_OpFConvert_RTZ_f64_f16(half FloatValue)
{
  return FloatValue;
}

double __builtin_spirv_OpFConvert_RTP_f64_f16(half FloatValue)
{
  return FloatValue;
}

double __builtin_spirv_OpFConvert_RTN_f64_f16(half FloatValue)
{
  return FloatValue;
}

#endif // defined(cl_khr_fp64)

half   __builtin_spirv_OpFConvert_RTE_f16_f32(float FloatValue)
{
  return FloatValue;
}

half   __builtin_spirv_OpFConvert_RTZ_f16_f32(float FloatValue)
{
  return __builtin_IB_ftoh_rtz(FloatValue);
}

half   __builtin_spirv_OpFConvert_RTP_f16_f32(float FloatValue)
{
  return __builtin_IB_ftoh_rtp(FloatValue);
}

half   __builtin_spirv_OpFConvert_RTN_f16_f32(float FloatValue)
{
  return __builtin_IB_ftoh_rtn(FloatValue);
}

float  __builtin_spirv_OpFConvert_RTE_f32_f32(float FloatValue)
{
  return __builtin_spirv_OpFConvert_f32_f32(FloatValue);
}

float  __builtin_spirv_OpFConvert_RTZ_f32_f32(float FloatValue)
{
  return __builtin_spirv_OpFConvert_f32_f32(FloatValue);
}

float  __builtin_spirv_OpFConvert_RTP_f32_f32(float FloatValue)
{
  return __builtin_spirv_OpFConvert_f32_f32(FloatValue);
}

float  __builtin_spirv_OpFConvert_RTN_f32_f32(float FloatValue)
{
  return __builtin_spirv_OpFConvert_f32_f32(FloatValue);
}

#if defined(cl_khr_fp64)

double __builtin_spirv_OpFConvert_RTE_f64_f32(float FloatValue)
{
  return FloatValue;
}

double __builtin_spirv_OpFConvert_RTZ_f64_f32(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

double __builtin_spirv_OpFConvert_RTP_f64_f32(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

double __builtin_spirv_OpFConvert_RTN_f64_f32(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

half   __builtin_spirv_OpFConvert_RTE_f16_f64(double FloatValue)
{
  return __builtin_spirv_OpFConvert_f16_f64(FloatValue);
}

half   __builtin_spirv_OpFConvert_RTZ_f16_f64(double FloatValue)
{
  float f = __builtin_spirv_OpFConvert_RTZ_f32_f64(FloatValue);
  return __builtin_spirv_OpFConvert_RTZ_f16_f32(f);
}

half   __builtin_spirv_OpFConvert_RTP_f16_f64(double FloatValue)
{
  float f = __builtin_spirv_OpFConvert_RTP_f32_f64(FloatValue);
  return __builtin_spirv_OpFConvert_RTP_f16_f32(f);
}

half   __builtin_spirv_OpFConvert_RTN_f16_f64(double FloatValue)
{
  float f = __builtin_spirv_OpFConvert_RTN_f32_f64(FloatValue);
  return __builtin_spirv_OpFConvert_RTN_f16_f32(f);
}

float  __builtin_spirv_OpFConvert_RTE_f32_f64(double FloatValue)
{
  return FloatValue;
}

float  __builtin_spirv_OpFConvert_RTZ_f32_f64(double FloatValue)
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

float  __builtin_spirv_OpFConvert_RTP_f32_f64(double FloatValue)
{
  return __intel_convert_float_rtp_rtn(FloatValue, RT_POS);
}

float  __builtin_spirv_OpFConvert_RTN_f32_f64(double FloatValue)
{
  return __intel_convert_float_rtp_rtn(FloatValue, RT_NEG);
}

double __builtin_spirv_OpFConvert_RTE_f64_f64(double FloatValue)
{
  return FloatValue;
}

double __builtin_spirv_OpFConvert_RTZ_f64_f64(double FloatValue)
{
  return FloatValue;
}

double __builtin_spirv_OpFConvert_RTP_f64_f64(double FloatValue)
{
  return FloatValue;
}

double __builtin_spirv_OpFConvert_RTN_f64_f64(double FloatValue)
{
  return FloatValue;
}

#endif // defined(cl_khr_fp64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( __builtin_spirv_OpConvertSToF, half, f16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( __builtin_spirv_OpConvertSToF, float, f32)
#if defined(cl_khr_fp64)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( __builtin_spirv_OpConvertSToF, double, f64)
#endif

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( __builtin_spirv_OpConvertUToF, half, f16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( __builtin_spirv_OpConvertUToF, float, f32)
#if defined(cl_khr_fp64)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( __builtin_spirv_OpConvertUToF, double, f64)
#endif

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( __builtin_spirv_OpFConvert, half, f16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( __builtin_spirv_OpFConvert, float, f32)
#if defined(cl_khr_fp64)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( __builtin_spirv_OpFConvert, double, f64)
#endif

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed( __builtin_spirv_OpSConvert, char, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed( __builtin_spirv_OpSConvert, short, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed( __builtin_spirv_OpSConvert, int, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Signed( __builtin_spirv_OpSConvert, long, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned( __builtin_spirv_OpUConvert, uchar, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned( __builtin_spirv_OpUConvert, ushort, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned( __builtin_spirv_OpUConvert, uint, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Unsigned( __builtin_spirv_OpUConvert, ulong, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __builtin_spirv_OpConvertFToS, char, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __builtin_spirv_OpConvertFToS, short, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __builtin_spirv_OpConvertFToS, int, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __builtin_spirv_OpConvertFToS, long, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __builtin_spirv_OpConvertFToU, uchar, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __builtin_spirv_OpConvertFToU, ushort, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __builtin_spirv_OpConvertFToU, uint, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_FloatToInteger( __builtin_spirv_OpConvertFToU, ulong, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned( __builtin_spirv_OpSatConvertSToU, uchar, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned( __builtin_spirv_OpSatConvertSToU, ushort, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned( __builtin_spirv_OpSatConvertSToU, uint, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_SignedToUnsigned( __builtin_spirv_OpSatConvertSToU, ulong, i64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned( __builtin_spirv_OpSatConvertUToS, char, i8)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned( __builtin_spirv_OpSatConvertUToS, short, i16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned( __builtin_spirv_OpSatConvertUToS, int, i32)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Special_Sat_UnsignedToSigned( __builtin_spirv_OpSatConvertUToS, long, i64)

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
    uint ShiftAmount = __builtin_spirv_OpenCL_clz_i32(Hi);

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
                    Res_Rounded = __builtin_spirv_OpConvertUToF_RTP_f32_i32(InnerResHi | 1);
                else
                    Res_Rounded = __builtin_spirv_OpConvertUToF_RTP_f32_i32(InnerResHi);
                break;
            case 2: //rtn
            case 3: //rtz
                Res_Rounded = __builtin_spirv_OpConvertUToF_RTZ_f32_i32(InnerResHi);
                break;
            default:
                Res_Rounded = InnerResLo ? (float)(InnerResHi | 1) : (float)InnerResHi;
        }
    }
    else
    {
        if (roundingMode == 1) //rtp
            Res_Rounded = __builtin_spirv_OpConvertUToF_RTP_f32_i32(Lo);
        else if (roundingMode > 1)  //rtn and rtz
            Res_Rounded = __builtin_spirv_OpConvertUToF_RTZ_f32_i32(Lo);
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
INLINE private void* __builtin_spirv_OpGenericCastToPtrExplicit_p0i8_p4i8_i32(const generic void *Pointer, StorageClass_t Storage)
{
    return __builtin_IB_to_private(Pointer);
}

INLINE local   void* __builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(const generic void *Pointer, StorageClass_t Storage)
{
    return __builtin_IB_to_local(Pointer);
}

INLINE global  void* __builtin_spirv_OpGenericCastToPtrExplicit_p1i8_p4i8_i32(const generic void *Pointer, StorageClass_t Storage)
{
    if((__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(Pointer, Storage) == NULL) &
       (__builtin_spirv_OpGenericCastToPtrExplicit_p0i8_p4i8_i32(Pointer, Storage) == NULL))
    {
        return (global void*)(Pointer);
    }

    return NULL;
}

INLINE uint __builtin_spirv_OpGenericPtrMemSemantics_p4i8(const generic void *Pointer)
{
    if (__builtin_spirv_OpGenericCastToPtrExplicit_p3i8_p4i8_i32(Pointer, StorageWorkgroup) != NULL)
    {
        return WorkgroupMemory;
    }

    return CrossWorkgroupMemory;
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


