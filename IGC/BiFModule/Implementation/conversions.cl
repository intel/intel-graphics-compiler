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

#if defined(cl_khr_fp16)

/* Helper Functions from IBiF_Conversions.cl */
#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static ushort OVERLOADABLE sat_ushort(half _T, ushort _R)
{
  return __builtin_spirv_OpenCL_select_i16_i16_i16(
    _R, (ushort)0,
    SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(
        (half)((_T < (half)0) | __builtin_spirv_OpIsNan_f16(_T))));
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static uint OVERLOADABLE sat_uint(half _T, uint _R)
{
  return __builtin_spirv_OpenCL_select_i32_i32_i32(
    _R, (uint)0,
    SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(
        (half)((_T < (half)0) | __builtin_spirv_OpIsNan_f16(_T))));
}
#endif

static ulong OVERLOADABLE sat_ulong(half _T, ulong _R)
{
  return __builtin_spirv_OpenCL_select_i64_i64_i64(
    _R, (ulong)0,
    SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(
        (half)((_T < (half)0) | __builtin_spirv_OpIsNan_f16(_T))));
}

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static uchar clamp_sat_uchar(half _T, uchar _R)
{
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(
    _R, (uchar)0,
    SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(
      (half)(_T < (half)0)));
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(
    _R, (uchar)UCHAR_MAX,
    SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(
      (half)(_T > (half)UCHAR_MAX)));
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(
    _R, (uchar)0,
    SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(
      (half)__builtin_spirv_OpIsNan_f16(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static char clamp_sat_char(half _T, char _R)
{
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(
    _R, (char)CHAR_MIN,
    SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(
      (half)(_T < (half)CHAR_MIN)));
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(
    _R, (char)CHAR_MAX,
    SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(
      (half)(_T > (half)CHAR_MAX)));
  _R = __builtin_spirv_OpenCL_select_i8_i8_i8(
    _R, (char)0,
    SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(
      (half)__builtin_spirv_OpIsNan_f16(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static short clamp_sat_short(half _T, short _R)
{
  _R = __builtin_spirv_OpenCL_select_i16_i16_i16(
    _R, (short)SHRT_MIN,
    SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(
      (half)(_T < (half)SHRT_MIN)));
  _R = __builtin_spirv_OpenCL_select_i16_i16_i16(
    _R, (short)SHRT_MAX,
    SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(
      (half)(_T > (half)SHRT_MAX)));
  _R = __builtin_spirv_OpenCL_select_i16_i16_i16(
    _R, (short)0,
    SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(
      (half)__builtin_spirv_OpIsNan_f16(_T)));
  return _R;
}
#endif

#ifdef __IGC_BUILD__
// Helper function for conversions with saturation
static int clamp_sat_int(half _T, int _R)
{
    _R = __builtin_spirv_OpenCL_select_i32_i32_i32(
        _R, (int)INT_MIN,
        SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(
          (half)(_T < (half)INT_MIN)));
    _R = __builtin_spirv_OpenCL_select_i32_i32_i32(
        _R, (int)INT_MAX,
        SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(
          (half)(_T > (half)INT_MAX)));
    _R = __builtin_spirv_OpenCL_select_i32_i32_i32(
        _R, (int)0,
        SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(
          (half)__builtin_spirv_OpIsNan_f16(_T)));
  return _R;
}
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
    float FC0 = __builtin_spirv_OpenCL_ldexp_f32_i32(1.0f, -32);
    float FC1 = __builtin_spirv_OpenCL_ldexp_f32_i32(-1.0f, 32);
    float HiF = __builtin_spirv_OpenCL_trunc_f32(FloatValue * FC0);
    float LoF = __builtin_spirv_OpenCL_fma_f32_f32_f32(HiF, FC1, FloatValue);
    ulong answer = 0;
    answer = (((uint)HiF | answer) << 32) | ((uint)LoF | answer);
    return answer;
}

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
      int res = __builtin_spirv_OpenCL_s_clamp_i32_i32_i32(SignedValue, 0, (int)USHRT_MAX);
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


/*
// Next is all Scalar types with Rounding modes [RTE,RTZ,RTN,RTP] and Sat
//
//
//
//
*/

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f16, _rte_Ruchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f16, _rtz_Ruchar)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f16, _rtp_Ruchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f16, _rtn_Ruchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _sat_Ruchar)(half FloatValue)
{
  uchar normal = SPIRV_BUILTIN(ConvertFToU, _i8_f16, _Ruchar)(FloatValue);
  return clamp_sat_uchar(FloatValue, normal);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f16, _sat_rte_Ruchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _sat_Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f16, _sat_rtz_Ruchar)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _sat_Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f16, _sat_rtp_Ruchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _sat_Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f16, _sat_rtn_Ruchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f16, _sat_Ruchar)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f16, _rte_Rushort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f16, _rtz_Rushort)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f16, _rtp_Rushort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f16, _rtn_Rushort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _sat_Rushort)(half FloatValue)
{
  ushort normal = SPIRV_BUILTIN(ConvertFToU, _i16_f16, _Rushort)(FloatValue);
  return sat_ushort(FloatValue, normal);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f16, _sat_rte_Rushort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _sat_Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f16, _sat_rtz_Rushort)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _sat_Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f16, _sat_rtp_Rushort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _sat_Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f16, _sat_rtn_Rushort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f16, _sat_Rushort)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f16, _rte_Ruint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f16, _rtz_Ruint)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f16, _rtp_Ruint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f16, _rtn_Ruint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _sat_Ruint)(half FloatValue)
{
  uint normal = SPIRV_BUILTIN(ConvertFToU, _i32_f16, _Ruint)(FloatValue);
  return sat_uint(FloatValue, normal);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f16, _sat_rte_Ruint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _sat_Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f16, _sat_rtz_Ruint)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _sat_Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f16, _sat_rtp_Ruint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _sat_Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f16, _sat_rtn_Ruint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f16, _sat_Ruint)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f16, _rte_Rulong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f16, _rtz_Rulong)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f16, _rtp_Rulong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f16, _rtn_Rulong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _sat_Rulong)(half FloatValue)
{
  ulong normal = SPIRV_BUILTIN(ConvertFToU, _i64_f16, _Rulong)(FloatValue);
  return sat_ulong(FloatValue, normal);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f16, _sat_rte_Rulong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _sat_Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f16, _sat_rtz_Rulong)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _sat_Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f16, _sat_rtp_Rulong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _sat_Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f16, _sat_rtn_Rulong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f16, _sat_Rulong)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i8_f32, _rte_Ruchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i8_f32, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i8_f32, _rtz_Ruchar)(float FloatValue)
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

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i8_f32, _rtp_Ruchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i8_f32, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i8_f32, _rtn_Ruchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i8_f32, _Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _sat_Ruchar)(float FloatValue)
{
  //return __builtin_IB_ftouc_sat((float)FloatValue);
  float res = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(FloatValue, 0.0f, (float)UCHAR_MAX);
  return (uchar)res;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i8_f32, _sat_rte_Ruchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _sat_Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i8_f32, _sat_rtz_Ruchar)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _sat_Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i8_f32, _sat_rtp_Ruchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _sat_Ruchar)(FloatValue);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i8_f32, _sat_rtn_Ruchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i8_f32, _sat_Ruchar)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i16_f32, _rte_Rushort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i16_f32, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i16_f32, _rtz_Rushort)(float FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= USHRT_MAX) {
    return USHRT_MAX;
  }
  return (ushort)FloatValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i16_f32, _rtp_Rushort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i16_f32, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i16_f32, _rtn_Rushort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i16_f32, _Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _sat_Rushort)(float FloatValue)
{
  //return __builtin_IB_ftous_sat((float)FloatValue);
  float res = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(FloatValue, 0.0f, (float)USHRT_MAX);
  return (ushort)res;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i16_f32, _sat_rte_Rushort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _sat_Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i16_f32, _sat_rtz_Rushort)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _sat_Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i16_f32, _sat_rtp_Rushort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _sat_Rushort)(FloatValue);
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i16_f32, _sat_rtn_Rushort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i16_f32, _sat_Rushort)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i32_f32, _rte_Ruint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f32, _rtz_Ruint)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i32_f32, _rtp_Ruint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i32_f32, _rtn_Ruint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _sat_Ruint)(float FloatValue)
{
    return SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f32, _sat_rtz_Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i32_f32, _sat_rte_Ruint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _sat_Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i32_f32, _sat_rtz_Ruint)(float FloatValue)
{
  uint _R = SPIRV_BUILTIN(ConvertFToU, _RTZ_i32_f32, _rtz_Ruint)(FloatValue);
  _R = __builtin_spirv_OpenCL_select_i32_i32_i32(
    _R, (uint)UINT_MAX,
    SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(
      (float)(FloatValue > (float)UINT_MAX)));
  return __builtin_spirv_OpenCL_select_i32_i32_i32(
    _R, (uint)0,
    SPIRV_BUILTIN(ConvertFToU, _i32_f32, _Ruint)(
      (float)((FloatValue < (float)0) |
      __builtin_spirv_OpIsNan_f32(FloatValue))));
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i32_f32, _sat_rtp_Ruint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _sat_Ruint)(FloatValue);
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i32_f32, _sat_rtn_Ruint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i32_f32, _sat_Ruint)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTE_i64_f32, _rte_Rulong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTZ_i64_f32, _rtz_Rulong)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTP_i64_f32, _rtp_Rulong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _RTN_i64_f32, _rtn_Rulong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _sat_Rulong)(float FloatValue)
{
  if (FloatValue <= 0) {
    return 0;
  } else if (FloatValue >= ULONG_MAX) {
    return ULONG_MAX;
  }
  return SPIRV_BUILTIN(ConvertFToU, _i64_f32, _Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTE_i64_f32, _sat_rte_Rulong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _sat_Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTZ_i64_f32, _sat_rtz_Rulong)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _sat_Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTP_i64_f32, _sat_rtp_Rulong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _sat_Rulong)(FloatValue);
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToU, _Sat_RTN_i64_f32, _sat_rtn_Rulong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToU, _Sat_i64_f32, _sat_Rulong)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f16, _rte_Rchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f16, _rtz_Rchar)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f16, _rtp_Rchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f16, _rtn_Rchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _sat_Rchar)(half FloatValue)
{
  char normal = SPIRV_BUILTIN(ConvertFToS, _i8_f16, _Rchar)(FloatValue);
  return clamp_sat_char(FloatValue, normal);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f16, _sat_rte_Rchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _sat_Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f16, _sat_rtz_Rchar)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _sat_Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f16, _sat_rtp_Rchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _sat_Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f16, _sat_rtn_Rchar)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f16, _sat_Rchar)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f16, _rte_Rshort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f16, _rtz_Rshort)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f16, _rtp_Rshort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f16, _rtn_Rshort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _sat_Rshort)(half FloatValue)
{
  short normal = SPIRV_BUILTIN(ConvertFToS, _i16_f16, _Rshort)(FloatValue);
  return clamp_sat_short(FloatValue, normal);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f16, _sat_rte_Rshort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _sat_Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f16, _sat_rtz_Rshort)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _sat_Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f16, _sat_rtp_Rshort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _sat_Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f16, _sat_rtn_Rshort)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f16, _sat_Rshort)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f16, _rte_Rint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f16, _rtz_Rint)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f16, _rtp_Rint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f16, _rtn_Rint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _sat_Rint)(half FloatValue)
{
  int normal = SPIRV_BUILTIN(ConvertFToS, _i32_f16, _Rint)(FloatValue);
  return clamp_sat_int(FloatValue, normal);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f16, _sat_rte_Rint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _sat_Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f16, _sat_rtz_Rint)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _sat_Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f16, _sat_rtp_Rint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _sat_Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f16, _sat_rtn_Rint)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f16, _sat_Rint)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f16, _rte_Rlong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f16, _rtz_Rlong)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f16, _rtp_Rlong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f16, _rtn_Rlong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f16, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _sat_Rlong)(half FloatValue)
{
  if (FloatValue <= LONG_MIN) {
    return LONG_MIN;
  } else if (FloatValue >= LONG_MAX) {
    return LONG_MAX;
  }
  return FloatValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f16, _sat_rte_Rlong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _sat_Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f16, _sat_rtz_Rlong)(half FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _sat_Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f16, _sat_rtp_Rlong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _sat_Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f16, _sat_rtn_Rlong)(half FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f16(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f16, _sat_Rlong)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i8_f32, _rte_Rchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i8_f32, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i8_f32, _rtz_Rchar)(float FloatValue)
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

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i8_f32, _rtp_Rchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i8_f32, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i8_f32, _rtn_Rchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i8_f32, _Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _sat_Rchar)(float FloatValue)
{
  float res = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(FloatValue, (float)CHAR_MIN, (float)CHAR_MAX);
  res = __builtin_spirv_OpenCL_select_f32_f32_i32(res, 0.0f , __builtin_spirv_OpIsNan_f32(FloatValue));
  return (char)res;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i8_f32, _sat_rte_Rchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _sat_Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i8_f32, _sat_rtz_Rchar)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _sat_Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i8_f32, _sat_rtp_Rchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _sat_Rchar)(FloatValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i8_f32, _sat_rtn_Rchar)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i8_f32, _sat_Rchar)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i16_f32, _rte_Rshort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i16_f32, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i16_f32, _rtz_Rshort)(float FloatValue)
{
  if (FloatValue <= SHRT_MIN) {
    return SHRT_MIN;
  } else if (FloatValue >= SHRT_MAX) {
    return SHRT_MAX;
  }
  return (short)FloatValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i16_f32, _rtp_Rshort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i16_f32, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i16_f32, _rtn_Rshort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i16_f32, _Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _sat_Rshort)(float FloatValue)
{
  float res = __builtin_spirv_OpenCL_fclamp_f32_f32_f32(FloatValue, (float)SHRT_MIN, (float)SHRT_MAX);
  res = __builtin_spirv_OpenCL_select_f32_f32_i32(res, 0.0f , __builtin_spirv_OpIsNan_f32(FloatValue));
  return (short)res;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i16_f32, _sat_rte_Rshort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _sat_Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i16_f32, _sat_rtz_Rshort)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _sat_Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i16_f32, _sat_rtp_Rshort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _sat_Rshort)(FloatValue);
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i16_f32, _sat_rtn_Rshort)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i16_f32, _sat_Rshort)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i32_f32, _rte_Rint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f32, _rtz_Rint)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i32_f32, _rtp_Rint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i32_f32, _rtn_Rint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _sat_Rint)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f32, _sat_rtz_Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i32_f32, _sat_rte_Rint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _sat_Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i32_f32, _sat_rtz_Rint)(float FloatValue)
{
  int _R = SPIRV_BUILTIN(ConvertFToS, _RTZ_i32_f32, _rtz_Rint)(FloatValue);
  _R = __builtin_spirv_OpenCL_select_i32_i32_i32(
    _R, (int)INT_MIN,
    SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(
      (float)(FloatValue < (float)INT_MIN)));
  _R = __builtin_spirv_OpenCL_select_i32_i32_i32(
    _R, (int)INT_MAX,
    SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(
      (float)(FloatValue > (float)INT_MAX)));
  return __builtin_spirv_OpenCL_select_i32_i32_i32(
    _R, (int)0,
    SPIRV_BUILTIN(ConvertFToS, _i32_f32, _Rint)(
      (float)__builtin_spirv_OpIsNan_f32(FloatValue)));
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i32_f32, _sat_rtp_Rint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _sat_Rint)(FloatValue);
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i32_f32, _sat_rtn_Rint)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i32_f32, _sat_Rint)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTE_i64_f32, _rte_Rlong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTZ_i64_f32, _rtz_Rlong)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTP_i64_f32, _rtp_Rlong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _RTN_i64_f32, _rtn_Rlong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _sat_Rlong)(float FloatValue)
{
  if (FloatValue <= LONG_MIN) {
    return LONG_MIN;
  } else if (FloatValue >= LONG_MAX) {
    return LONG_MAX;
  }
  return SPIRV_BUILTIN(ConvertFToS, _i64_f32, _Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTE_i64_f32, _sat_rte_Rlong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_rint_f32(FloatValue);
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _sat_Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTZ_i64_f32, _sat_rtz_Rlong)(float FloatValue)
{
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _sat_Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTP_i64_f32, _sat_rtp_Rlong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_ceil_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _sat_Rlong)(FloatValue);
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertFToS, _Sat_RTN_i64_f32, _sat_rtn_Rlong)(float FloatValue)
{
  FloatValue = __builtin_spirv_OpenCL_floor_f32( FloatValue );
  return SPIRV_BUILTIN(ConvertFToS, _Sat_i64_f32, _sat_Rlong)(FloatValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i8, _rte_Rhalf)(char SignedValue)
{
  return SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i8, _rtz_Rhalf)(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i8, _rtp_Rhalf)(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i8, _rtn_Rhalf)(char SignedValue)
{
  return SignedValue;    // A half can represent a char exactly.
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i8, _rte_Rfloat)(char SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i8, _rtz_Rfloat)(char SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i8, _rtp_Rfloat)(char SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i8, _rtn_Rfloat)(char SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i8, _Rfloat)(SignedValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i16, _rte_Rhalf)(short SignedValue)
{
  return SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i16, _rtz_Rhalf)(short SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _rtz_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i16, _rtp_Rhalf)(short SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _rtp_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i16, _rtn_Rhalf)(short SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _rtn_Rhalf)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i16, _rte_Rfloat)(short SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i16, _rtz_Rfloat)(short SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i16, _rtp_Rfloat)(short SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i16, _rtn_Rfloat)(short SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i16, _Rfloat)(SignedValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i32, _rte_Rhalf)(int SignedValue)
{
  return SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i32, _rtz_Rhalf)(int SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _rtz_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i32, _rtp_Rhalf)(int SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _rtp_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i32, _rtn_Rhalf)(int SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _rtn_Rhalf)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i32, _rte_Rfloat)(int SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i32, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i32, _rtz_Rfloat)(int SignedValue)
{
  return __builtin_IB_itof_rtz(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i32, _rtp_Rfloat)(int SignedValue)
{
  return __builtin_IB_itof_rtp(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i32, _rtn_Rfloat)(int SignedValue)
{
  return __builtin_IB_itof_rtn(SignedValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f16_i64, _rte_Rhalf)(long SignedValue)
{
  return SignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f16_i64, _rtz_Rhalf)(long SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _rtz_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f16_i64, _rtp_Rhalf)(long SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _rtp_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f16_i64, _rtn_Rhalf)(long SignedValue)
{
  float f = SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(SignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _rtn_Rhalf)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTE_f32_i64, _rte_Rfloat)(long SignedValue)
{
  return SPIRV_BUILTIN(ConvertSToF, _f32_i64, _Rfloat)(SignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTZ_f32_i64, _rtz_Rfloat)(long SignedValue)
{
  return convertSItoFP32(SignedValue, 3);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTP_f32_i64, _rtp_Rfloat)(long SignedValue)
{
  return convertSItoFP32(SignedValue, 1);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertSToF, _RTN_f32_i64, _rtn_Rfloat)(long SignedValue)
{
  return convertSItoFP32(SignedValue, 2);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i8, _rte_Rhalf)(uchar UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i8, _rtz_Rhalf)(uchar UnsignedValue)
{
  return UnsignedValue;    // A half can represent a uchar exactly.
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i8, _rtp_Rhalf)(uchar UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i8, _rtn_Rhalf)(uchar UnsignedValue)
{
  return UnsignedValue;    // A half can represent a uchar exactly.
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i8, _rte_Rfloat)(uchar UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i8, _rtz_Rfloat)(uchar UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i8, _rtp_Rfloat)(uchar UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i8, _rtn_Rfloat)(uchar UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i8, _Rfloat)(UnsignedValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i16, _rte_Rhalf)(ushort UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i16, _rtz_Rhalf)(ushort UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _rtz_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i16, _rtp_Rhalf)(ushort UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _rtp_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i16, _rtn_Rhalf)(ushort UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _rtn_Rhalf)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i16, _rte_Rfloat)(ushort UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i16, _rtz_Rfloat)(ushort UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i16, _rtp_Rfloat)(ushort UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i16, _rtn_Rfloat)(ushort UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i16, _Rfloat)(UnsignedValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i32, _rte_Rhalf)(uint UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i32, _rtz_Rhalf)(uint UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _rtz_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i32, _rtp_Rhalf)(uint UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _rtp_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i32, _rtn_Rhalf)(uint UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _rtn_Rhalf)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i32, _rte_Rfloat)(uint UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i32, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i32, _rtz_Rfloat)(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtz(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _rtp_Rfloat)(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtp(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i32, _rtn_Rfloat)(uint UnsignedValue)
{
  return __builtin_IB_uitof_rtn(UnsignedValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f16_i64, _rte_Rhalf)(ulong UnsignedValue)
{
  return UnsignedValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f16_i64, _rtz_Rhalf)(ulong UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _rtz_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f16_i64, _rtp_Rhalf)(ulong UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _rtp_Rhalf)(f);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f16_i64, _rtn_Rhalf)(ulong UnsignedValue)
{
  float f = SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(UnsignedValue);
  return SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _rtn_Rhalf)(f);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTE_f32_i64, _rte_Rfloat)(ulong UnsignedValue)
{
  return SPIRV_BUILTIN(ConvertUToF, _f32_i64, _Rfloat)(UnsignedValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i64, _rtz_Rfloat)(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 3, false);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i64, _rtp_Rfloat)(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 1, false);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(ConvertUToF, _RTN_f32_i64, _rtn_Rfloat)(ulong UnsignedValue)
{
  return convertUItoFP32(UnsignedValue, 2, false);
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i8, _sat_Ruchar)(uchar UnsignedValue)
{
  return UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i8, _sat_Rushort)(uchar UnsignedValue)
{
  return UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i8, _sat_Ruint)(uchar UnsignedValue)
{
  return (uint)UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i8, _sat_Rulong)(uchar UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i16, _sat_Ruchar)(ushort UnsignedValue)
{
      //return __builtin_IB_ustouc_sat((ushort)UnsignedValue);
      if (UnsignedValue > (uchar)0xff)
      {
        return (uchar)0xff;
      }
      return (uchar)UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i16, _sat_Rushort)(ushort UnsignedValue)
{
  return UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i16, _sat_Ruint)(ushort UnsignedValue)
{
  return (uint)UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i16, _sat_Rulong)(ushort UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i32, _sat_Ruchar)(uint UnsignedValue)
{
      //return __builtin_IB_uitouc_sat((uint)UnsignedValue);
      if (UnsignedValue > UCHAR_MAX)
      {
        return UCHAR_MAX;
      }
      return (uchar)UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i32, _sat_Rushort)(uint UnsignedValue)
{
      if (UnsignedValue > (ushort)0xffff) {
        return (ushort)0xffff;
      }
      return (ushort)UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i32, _sat_Ruint)(uint UnsignedValue)
{
  return UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i32, _sat_Rulong)(uint UnsignedValue)
{
  return (ulong)UnsignedValue;
}

uchar  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i8_i64, _sat_Ruchar)(ulong UnsignedValue)
{
  return (UnsignedValue >= UCHAR_MAX) ? UCHAR_MAX : (uchar)UnsignedValue;
}

ushort SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i16_i64, _sat_Rushort)(ulong UnsignedValue)
{
  return (UnsignedValue >= USHRT_MAX) ? USHRT_MAX : (ushort)UnsignedValue;
}

uint   SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i32_i64, _sat_Ruint)(ulong UnsignedValue)
{
  return (UnsignedValue >= UINT_MAX) ? UINT_MAX : (uint)UnsignedValue;
}

ulong  SPIRV_OVERLOADABLE SPIRV_BUILTIN(UConvert, _Sat_i64_i64, _sat_Rulong)(ulong UnsignedValue)
{
  return UnsignedValue;
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i8, _sat_Rchar)(char SignedValue)
{
  return SignedValue;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i8, _sat_Rshort)(char SignedValue)
{
  return (short)SignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i8, _sat_Rint)(char SignedValue)
{
  return (int)SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i8, _sat_Rlong)(char SignedValue)
{
  return SPIRV_BUILTIN(SConvert, _i64_i8, _Rlong)(SignedValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i16, _sat_Rchar)(short SignedValue)
{
      //return __builtin_IB_stoc_sat((short)SignedValue);
      short res = __builtin_spirv_OpenCL_s_clamp_i16_i16_i16(SignedValue, (short)CHAR_MIN, (short)CHAR_MAX);
      return (char)res;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i16, _sat_Rshort)(short SignedValue)
{
  return SignedValue;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i16, _sat_Rint)(short SignedValue)
{
  return (int)SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i16, _sat_Rlong)(short SignedValue)
{
  return SPIRV_BUILTIN(SConvert, _i64_i16, _Rlong)(SignedValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i32, _sat_Rchar)(int SignedValue)
{
      //return __builtin_IB_itoc_sat((int)SignedValue);
      int res = __builtin_spirv_OpenCL_s_clamp_i32_i32_i32(SignedValue, (int)CHAR_MIN, (int)CHAR_MAX);
      return (char)res;
}

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i32, _sat_Rshort)(int SignedValue)
{
      //return __builtin_IB_itos_sat((int)SignedValue);
      int res = __builtin_spirv_OpenCL_s_clamp_i32_i32_i32(SignedValue, (int)SHRT_MIN, (int)SHRT_MAX);
      return (short)res;
}

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i32, _sat_Rint)(int SignedValue)
{
  return SignedValue;
}

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i32, _sat_Rlong)(int SignedValue)
{
  return SPIRV_BUILTIN(SConvert, _i64_i32, _Rlong)(SignedValue);
}

char  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i8_i64, _sat_Rchar)(long SignedValue)
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

short SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i16_i64, _sat_Rshort)(long SignedValue)
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

int   SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i32_i64, _sat_Rint)(long SignedValue)
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

long  SPIRV_OVERLOADABLE SPIRV_BUILTIN(SConvert, _Sat_i64_i64, _sat_Rlong)(long SignedValue)
{
  return SPIRV_BUILTIN(SConvert, _i64_i64, _Rlong)(SignedValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f16, _rte_Rhalf)(half FloatValue)
{
  return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f16, _rtz_Rhalf)(half FloatValue)
{
  return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f16, _rtp_Rhalf)(half FloatValue)
{
  return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f16, _rtn_Rhalf)(half FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f32_f16, _rte_Rfloat)(half FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f32_f16, _rtz_Rfloat)(half FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f32_f16, _rtp_Rfloat)(half FloatValue)
{
  return FloatValue;
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f32_f16, _rtn_Rfloat)(half FloatValue)
{
  return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f16_f32, _rte_Rhalf)(float FloatValue)
{
  return FloatValue;
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f16_f32, _rtz_Rhalf)(float FloatValue)
{
  return __builtin_IB_ftoh_rtz(FloatValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f16_f32, _rtp_Rhalf)(float FloatValue)
{
  return __builtin_IB_ftoh_rtp(FloatValue);
}

half   SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f16_f32, _rtn_Rhalf)(float FloatValue)
{
  return __builtin_IB_ftoh_rtn(FloatValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTE_f32_f32, _rte_Rfloat)(float FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(FloatValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTZ_f32_f32, _rtz_Rfloat)(float FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(FloatValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTP_f32_f32, _rtp_Rfloat)(float FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(FloatValue);
}

float  SPIRV_OVERLOADABLE SPIRV_BUILTIN(FConvert, _RTN_f32_f32, _rtn_Rfloat)(float FloatValue)
{
  return SPIRV_BUILTIN(FConvert, _f32_f32, _Rfloat)(FloatValue);
}

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( ConvertSToF, half, f16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_SignedToFloat( ConvertSToF, float, f32)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( ConvertUToF, half, f16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_UnsignedToFloat( ConvertUToF, float, f32)

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( FConvert, half, f16)
GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float( FConvert, float, f32)

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
                    Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _rtp_Rfloat)(InnerResHi | 1);
                else
                    Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _rtp_Rfloat)(InnerResHi);
                break;
            case 2: //rtn
            case 3: //rtz
                Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i32, _rtz_Rfloat)(InnerResHi);
                break;
            default:
                Res_Rounded = InnerResLo ? (float)(InnerResHi | 1) : (float)InnerResHi;
        }
    }
    else
    {
        if (roundingMode == 1) //rtp
            Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTP_f32_i32, _rtp_Rfloat)(Lo);
        else if (roundingMode > 1)  //rtn and rtz
            Res_Rounded = SPIRV_BUILTIN(ConvertUToF, _RTZ_f32_i32, _rtz_Rfloat)(Lo);
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
INLINE private void* SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p0i8_p4i8_i32, _ToPrivate)(const generic void *Pointer, StorageClass_t Storage)
{
    return __builtin_IB_to_private(Pointer);
}

INLINE local   void* SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(const generic void *Pointer, StorageClass_t Storage)
{
    return __builtin_IB_to_local(Pointer);
}

INLINE global  void* SPIRV_OVERLOADABLE SPIRV_BUILTIN(GenericCastToPtrExplicit, _p1i8_p4i8_i32, _ToGlobal)(const generic void *Pointer, StorageClass_t Storage)
{
    if((SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(Pointer, Storage) == NULL) &
       (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p0i8_p4i8_i32, _ToPrivate)(Pointer, Storage) == NULL))
    {
        return (global void*)(Pointer);
    }

    return NULL;
}

INLINE uint __builtin_spirv_OpGenericPtrMemSemantics_p4i8(const generic void *Pointer)
{
    if (SPIRV_BUILTIN(GenericCastToPtrExplicit, _p3i8_p4i8_i32, _ToLocal)(Pointer, StorageWorkgroup) != NULL)
    {
        return WorkgroupMemory;
    }

    return CrossWorkgroupMemory;
}

#endif // (__OPENCL_C_VERSION__ >= CL_VERSION_2_0)


