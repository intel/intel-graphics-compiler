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

INLINE float __intel_convert_float_rtp_rtn(double a, uint direction);

#if defined(cl_khr_fp16)
#ifdef __IGC_BUILD__
#define UCHAR_MIN ((uchar)0)
#define USHRT_MIN ((ushort)0)
#define UINT_MIN  ((uint)0)
#define ULONG_MIN ((ulong)0)
SAT_CLAMP_HELPER_UNSIGNED(uchar, double, UCHAR, f64, i64, i8)
SAT_CLAMP_HELPER_UNSIGNED(ushort, double, USHRT, f64, i64, i16)
SAT_CLAMP_HELPER_UNSIGNED(uint, double, UINT, f64, i64, i32)
SAT_CLAMP_HELPER_SIGN(char, double, CHAR, f64, i64, i8)
SAT_CLAMP_HELPER_SIGN(short, double, SHRT, f64, i64, i16)
SAT_CLAMP_HELPER_SIGN(int, double, INT, f64, i64, i32)
#endif
#endif

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

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f64, _rte_Ruchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f64, _rtz_Ruchar)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f64, _rtp_Ruchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f64, _rtn_Ruchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _sat_Ruchar)(double FloatValue)
{
  uchar normal = SPIRV_BUILTIN(ConvertFToU, _i8_f64, _Ruchar)(FloatValue);
  return clamp_sat_uchar_double(normal, FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _sat_Rushort)(double FloatValue)
{
  ushort normal = SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
  return clamp_sat_ushort_double(normal, FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _sat_Ruint)(double FloatValue)
{
  uint normal = SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
  return clamp_sat_uint_double(normal, FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _sat_Rchar)(double FloatValue)
{
  char normal = SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
  return clamp_sat_char_double(normal, FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _sat_Rshort)(double FloatValue)
{
  short normal = SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
  return clamp_sat_short_double(normal, FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _sat_Rint)(double FloatValue)
{
  int normal = SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
  return clamp_sat_int_double(normal, FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f64, _sat_rte_Ruchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _sat_Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f64, _sat_rtz_Ruchar)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _sat_Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f64, _sat_rtp_Ruchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _sat_Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f64, _sat_rtn_Ruchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f64, _sat_Ruchar)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f64, _rte_Rushort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f64, _rtz_Rushort)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f64, _rtp_Rushort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f64, _rtn_Rushort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f64, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f64, _sat_rte_Rushort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _sat_Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f64, _sat_rtz_Rushort)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _sat_Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f64, _sat_rtp_Rushort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _sat_Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f64, _sat_rtn_Rushort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f64, _sat_Rushort)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f64, _rte_Ruint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f64, _rtz_Ruint)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f64, _rtp_Ruint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f64, _rtn_Ruint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f64, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f64, _sat_rte_Ruint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _sat_Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f64, _sat_rtz_Ruint)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _sat_Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f64, _sat_rtp_Ruint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _sat_Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f64, _sat_rtn_Ruint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f64, _sat_Ruint)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f64, _rte_Rulong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f64, _rtz_Rulong)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f64, _rtp_Rulong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f64, _rtn_Rulong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f64, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _sat_Rulong)(double FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= ULONG_MAX) {
    return ULONG_MAX;
  }
  return FloatValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f64, _sat_rte_Rulong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _sat_Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f64, _sat_rtz_Rulong)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _sat_Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f64, _sat_rtp_Rulong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _sat_Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f64, _sat_rtn_Rulong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f64, _sat_Rulong)(FloatValue);

}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f64, _rte_Rchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f64, _rtz_Rchar)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f64, _rtp_Rchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f64, _rtn_Rchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f64, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f64, _sat_rte_Rchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _sat_Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f64, _sat_rtz_Rchar)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _sat_Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f64, _sat_rtp_Rchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _sat_Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f64, _sat_rtn_Rchar)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f64, _sat_Rchar)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f64, _rte_Rshort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f64, _rtz_Rshort)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f64, _rtp_Rshort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f64, _rtn_Rshort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f64, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f64, _sat_rte_Rshort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _sat_Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f64, _sat_rtz_Rshort)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _sat_Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f64, _sat_rtp_Rshort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _sat_Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f64, _sat_rtn_Rshort)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f64, _sat_Rshort)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f64, _rte_Rint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f64, _rtz_Rint)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f64, _rtp_Rint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f64, _rtn_Rint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f64, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f64, _sat_rte_Rint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _sat_Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f64, _sat_rtz_Rint)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _sat_Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f64, _sat_rtp_Rint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _sat_Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f64, _sat_rtn_Rint)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f64, _sat_Rint)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f64, _rte_Rlong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f64, _rtz_Rlong)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f64, _rtp_Rlong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f64, _rtn_Rlong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f64, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _sat_Rlong)(double FloatValue)
{
  if (FloatValue <= LONG_MIN) {
    return LONG_MIN;
  } else if (FloatValue >= LONG_MAX) {
    return LONG_MAX;
  }
  return FloatValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f64, _sat_rte_Rlong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _sat_Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f64, _sat_rtz_Rlong)(double FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _sat_Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f64, _sat_rtp_Rlong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _sat_Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f64, _sat_rtn_Rlong)(double FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f64(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f64, _sat_Rlong)(FloatValue);
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i8, _rte_Rdouble)(char SignedValue)
{
  return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i8, _rtz_Rdouble)(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i8, _rtp_Rdouble)(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i8, _rtn_Rdouble)(char SignedValue)
{
  return SignedValue;    // A double can represent a char exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i16, _rte_Rdouble)(short SignedValue)
{
  return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i16, _rtz_Rdouble)(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i16, _rtp_Rdouble)(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i16, _rtn_Rdouble)(short SignedValue)
{
  return SignedValue;    // A double can represent a short exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i32, _rte_Rdouble)(int SignedValue)
{
  return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i32, _rtz_Rdouble)(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i32, _rtp_Rdouble)(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i32, _rtn_Rdouble)(int SignedValue)
{
  return SignedValue;    // A double can represent an int exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f64_i64, _rte_Rdouble)(long SignedValue)
{
  return SignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f64_i64, _rtz_Rdouble)(long SignedValue)
{
  return __builtin_IB_itofp64_rtz(SignedValue);
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f64_i64, _rtp_Rdouble)(long SignedValue)
{
  return __builtin_IB_itofp64_rtp(SignedValue);
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f64_i64, _rtn_Rdouble)(long SignedValue)
{
  return __builtin_IB_itofp64_rtn(SignedValue);
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i8, _rte_Rdouble)(uchar UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i8, _rtz_Rdouble)(uchar UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uchar exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i8, _rtp_Rdouble)(uchar UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i8, _rtn_Rdouble)(uchar UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uchar exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i16, _rte_Rdouble)(ushort UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i16, _rtz_Rdouble)(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i16, _rtp_Rdouble)(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i16, _rtn_Rdouble)(ushort UnsignedValue)
{
  return UnsignedValue;    // A double can represent a ushort exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i32, _rte_Rdouble)(uint UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i32, _rtz_Rdouble)(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i32, _rtp_Rdouble)(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i32, _rtn_Rdouble)(uint UnsignedValue)
{
  return UnsignedValue;    // A double can represent a uint exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f64_i64, _rte_Rdouble)(ulong UnsignedValue)
{
  return UnsignedValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f64_i64, _rtz_Rdouble)(ulong UnsignedValue)
{
  return __builtin_IB_uitofp64_rtz(UnsignedValue);
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f64_i64, _rtp_Rdouble)(ulong UnsignedValue)
{
  return __builtin_IB_uitofp64_rtp(UnsignedValue);
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f64_i64, _rtn_Rdouble)(ulong UnsignedValue)
{
  return __builtin_IB_uitofp64_rtn(UnsignedValue);
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f64_f16, _rte_Rdouble)(half FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f64_f16, _rtz_Rdouble)(half FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f64_f16, _rtp_Rdouble)(half FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f64_f16, _rtn_Rdouble)(half FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f64_f32, _rte_Rdouble)(float FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f64_f32, _rtz_Rdouble)(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f64_f32, _rtp_Rdouble)(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f64_f32, _rtn_Rdouble)(float FloatValue)
{
  return FloatValue;    // A double can represent a float exactly.
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f64, _rte_Rhalf)(double FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f16_f64, _Rhalf)(FloatValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f64, _rtz_Rhalf)(double FloatValue)
{
  float f = SPIRV_BUILTIN(FConvert, _RTZ_f32_f64, _rtz_Rfloat)(FloatValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _rtz_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f64, _rtp_Rhalf)(double FloatValue)
{
  float f = SPIRV_BUILTIN(FConvert, _RTP_f32_f64, _rtp_Rfloat)(FloatValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _rtp_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f64, _rtn_Rhalf)(double FloatValue)
{
  float f = SPIRV_BUILTIN(FConvert, _RTN_f32_f64, _rtn_Rfloat)(FloatValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _rtn_Rhalf)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f32_f64, _rte_Rfloat)(double FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f32_f64, _rtz_Rfloat)(double FloatValue)
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

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f32_f64, _rtp_Rfloat)(double FloatValue)
{
  return __intel_convert_float_rtp_rtn(FloatValue, RT_POS);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f32_f64, _rtn_Rfloat)(double FloatValue)
{
  return __intel_convert_float_rtp_rtn(FloatValue, RT_NEG);
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f64_f64, _rte_Rdouble)(double FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f64_f64, _rtz_Rdouble)(double FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f64_f64, _rtp_Rdouble)(double FloatValue)
{
  return FloatValue;
}

double SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f64_f64, _rtn_Rdouble)(double FloatValue)
{
  return FloatValue;
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( ConvertSToF, double, f64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( ConvertUToF, double, f64)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( FConvert, double, f64)

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
