/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "include/BiF_Definitions.h"
#include "../Headers/spirv.h"

#define RT_POS 0
#define RT_NEG 1
#define RT_Z 2
#define RT_NE 3

#if defined(cl_khr_fp64)

INLINE float __intel_convert_float_rtp_rtn(double a, uint direction) {
  uint fp = (as_ulong(a) >> (DOUBLE_BITS - FLOAT_BITS)) & FLOAT_SIGN_MASK; // sign bit
  uint sign = fp >> (FLOAT_BITS - 1);
  double abs_a = as_double((as_ulong(a) & ~DOUBLE_SIGN_MASK));

  // +/- nan
  if (abs_a != abs_a) {
    fp = fp | (as_ulong(abs_a) >> (DOUBLE_BITS - FLOAT_BITS));
    fp = fp | FLOAT_MANTISSA_MASK; // Lose any QNANs
  }
  // overflow
  else if (abs_a > 0x1.fffffep+127) // max float
  {
    // overflow in the direction of the round?
    if (sign == direction) {
      fp = fp | as_uint(INFINITY);
    } else {
      if (abs_a == (double)INFINITY) {
        fp = fp | as_uint(INFINITY);
      } else {
        fp = fp | as_uint(0x1.fffffep+127f);
      }
    }
  }
  // underflow
  else if (abs_a <= 0x1p-150) // min float
  {
    // non-zero
    if (as_ulong(abs_a) && (sign == direction)) {
      // round away from zero
      fp = fp | 0x00000001;
    }
  }
  // denormal
  else if (abs_a < 0x1p-126) // largest float, non-denormal
  {
    // Multiply/shift abs_a by 0x1p149 in order to represent the eventual
    // float denormal part as a double integer before truncating.
    uint *abs_a_ptr = (uint *)&abs_a;
    abs_a_ptr[1] = abs_a_ptr[1] + 0x09500000; // abs_a = abs_a * 0x1p149
    int abs_fp = (int)abs_a;                  // truncate

    if (sign == direction) {
      // If any factional precision was lost in truncation, round away from
      // zero.
      abs_fp += (double)abs_fp != abs_a;
    }

    fp = fp | abs_fp;
  } else {
    ulong a2 = as_ulong(a);
    a2 = a2 & 0xFFFFFFFFE0000000ULL;                // Mask/truncate all usable bits of float type
    if ((as_ulong(a) != a2) && (sign == direction)) // Losing any fractional component? And is negative?
    {
      a2 = a2 + 0x0000000020000000ULL; //   yes, round float away from zero
    }
    a2 = a2 - 0x3800000000000000ULL; // Convert from double bias to float bias
    fp = fp | (uint)(a2 >> (DOUBLE_MANTISSA_BITS - FLOAT_MANTISSA_BITS));
  }

  return as_float(fp);
}

#endif // defined(cl_khr_fp64)

half __attribute__((overloadable)) __spirv_FConvert_Rhalf(half FloatValue) { return FloatValue; }

float __attribute__((overloadable)) __spirv_FConvert_Rfloat(half FloatValue) { return FloatValue; }

half __attribute__((overloadable)) __spirv_FConvert_Rhalf(float FloatValue) { return FloatValue; }

float __attribute__((overloadable)) __spirv_FConvert_Rfloat(float FloatValue) { return FloatValue; }

#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_FConvert_Rdouble(half FloatValue) { return FloatValue; }

double __attribute__((overloadable)) __spirv_FConvert_Rdouble(float FloatValue) { return FloatValue; }

half __attribute__((overloadable)) __spirv_FConvert_Rhalf(double FloatValue) {
  double a = FloatValue;
  ushort hp = (as_ulong(a) >> (DOUBLE_BITS - HALF_BITS)) & HALF_SIGN_MASK;
  double abs_a = as_double((as_ulong(a) & ~DOUBLE_SIGN_MASK));

  // +/- nan
  if (abs_a != abs_a) {
    hp = hp | (as_ulong(abs_a) >> (DOUBLE_BITS - HALF_BITS));
    hp = hp | HALF_MANTISSA_MASK; // Lose and QNANs
  }
  // Overflow
  else if (abs_a >= 0x1.ffep+15) // max half RTE
  {
    hp = hp | 0x7c00; // exp := 0x1f
  }
  // Underflow
  else if (abs_a <= 0x1p-25) // min half RTE
  {
    // nothing
  } else if (abs_a < 0x1p-24) // smallest, non-zero exact half
  {
    // Very small denormal
    hp = hp | 0x0001;
  } else if (abs_a < 0x1p-14) // larger half denormals
  {
    // Shift implicit bit into fract component of a half
    hp = hp | as_ulong(abs_a * 0x1p-1050);
  } else {
    a = a * 0x1p+42;
    a = as_double(as_ulong(a) & DOUBLE_EXPONENT_MASK);
    abs_a = abs_a + a;
    a = abs_a - a;

    a = a * 0x1p-1008;
    hp = hp | (as_ulong(a) >> (DOUBLE_MANTISSA_BITS - HALF_MANTISSA_BITS));
  }

  return as_half(hp);
}

float __attribute__((overloadable)) __spirv_FConvert_Rfloat(double FloatValue) { return FloatValue; }

double __attribute__((overloadable)) __spirv_FConvert_Rdouble(double FloatValue) { return FloatValue; }
#endif

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rte(half FloatValue) { return FloatValue; }

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtz(half FloatValue) { return FloatValue; }

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtp(half FloatValue) { return FloatValue; }

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtn(half FloatValue) { return FloatValue; }

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rte(half FloatValue) { return FloatValue; }

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtz(half FloatValue) { return FloatValue; }

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtp(half FloatValue) { return FloatValue; }

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtn(half FloatValue) { return FloatValue; }

#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rte(half FloatValue) { return FloatValue; }

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtz(half FloatValue) { return FloatValue; }

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtp(half FloatValue) { return FloatValue; }

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtn(half FloatValue) { return FloatValue; }
#endif

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rte(float FloatValue) { return FloatValue; }

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtz(float FloatValue) {
  return __builtin_IB_ftoh_rtz(FloatValue);
}

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtp(float FloatValue) {
  return __builtin_IB_ftoh_rtp(FloatValue);
}

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtn(float FloatValue) {
  return __builtin_IB_ftoh_rtn(FloatValue);
}

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rte(float FloatValue) {
  return __spirv_FConvert_Rfloat(FloatValue);
}

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtz(float FloatValue) {
  return __spirv_FConvert_Rfloat(FloatValue);
}

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtp(float FloatValue) {
  return __spirv_FConvert_Rfloat(FloatValue);
}

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtn(float FloatValue) {
  return __spirv_FConvert_Rfloat(FloatValue);
}

#if defined(cl_khr_fp64)
double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rte(float FloatValue) { return FloatValue; }

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtz(float FloatValue) {
  return FloatValue; // A double can represent a float exactly.
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtp(float FloatValue) {
  return FloatValue; // A double can represent a float exactly.
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtn(float FloatValue) {
  return FloatValue; // A double can represent a float exactly.
}

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rte(double FloatValue) {
  return __spirv_FConvert_Rhalf(FloatValue);
}

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtz(double FloatValue) {
  float f = __spirv_FConvert_Rfloat_rtz(FloatValue);
  return __spirv_FConvert_Rhalf_rtz(f);
}

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtp(double FloatValue) {
  float f = __spirv_FConvert_Rfloat_rtp(FloatValue);
  return __spirv_FConvert_Rhalf_rtp(f);
}

half __attribute__((overloadable)) __spirv_FConvert_Rhalf_rtn(double FloatValue) {
  float f = __spirv_FConvert_Rfloat_rtn(FloatValue);
  return __spirv_FConvert_Rhalf_rtn(f);
}

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rte(double FloatValue) { return FloatValue; }

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtz(double FloatValue) {
  double a = FloatValue;
  uint fp = (as_ulong(a) >> (DOUBLE_BITS - FLOAT_BITS)) & FLOAT_SIGN_MASK; // sign bit
  double abs_a = as_double((as_ulong(a) & ~DOUBLE_SIGN_MASK));

  // +/- nan
  if (abs_a != abs_a) {
    fp = fp | (as_ulong(abs_a) >> (DOUBLE_BITS - FLOAT_BITS));
    fp = fp | FLOAT_MANTISSA_MASK; // Lose any QNANs
  } else if (abs_a == (double)INFINITY) {
    fp = fp | as_uint(INFINITY);
  }
  // overflow
  else if (abs_a > 0x1.fffffep+127) // max float
  {
    fp = fp | as_uint(0x1.fffffep+127f);
  }
  // underflow
  else if (abs_a <= 0x1p-150) // min float
  {
    // nothing
  }
  // denormal
  else if (abs_a < 0x1p-126) // largest float, non-denormal
  {
    // Multiply/shift abs_a by 0x1p149 in order to represent the eventual
    // float denormal part as a double integer before truncating.
    uint *abs_a_ptr = (uint *)&abs_a;
    abs_a_ptr[1] = abs_a_ptr[1] + 0x09500000; // abs_a = abs_a * 0x1p149
    int abs_fp = (int)abs_a;                  // truncate

    fp = fp | abs_fp;
  } else {
    ulong a2 = as_ulong(a);
    a2 = a2 & 0xFFFFFFFFE0000000ULL; // Mask/truncate all usable bits of float type
    a2 = a2 - 0x3800000000000000ULL; // Convert from double bias to float bias
    fp = fp | (uint)(a2 >> (DOUBLE_MANTISSA_BITS - FLOAT_MANTISSA_BITS));
  }

  return as_float(fp);
}

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtp(double FloatValue) {
  return __intel_convert_float_rtp_rtn(FloatValue, RT_POS);
}

float __attribute__((overloadable)) __spirv_FConvert_Rfloat_rtn(double FloatValue) {
  return __intel_convert_float_rtp_rtn(FloatValue, RT_NEG);
}

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rte(double FloatValue) { return FloatValue; }

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtz(double FloatValue) { return FloatValue; }

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtp(double FloatValue) { return FloatValue; }

double __attribute__((overloadable)) __spirv_FConvert_Rdouble_rtn(double FloatValue) { return FloatValue; }
#endif

GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float(FConvert, half, f16)
    GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float(FConvert, float, f32)

#if defined(cl_khr_fp64)
        GENERATE_CONVERSIONS_FUNCTIONS_ALL_TYPES_Float(FConvert, double, f64)
#endif
