/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_FLOATS_HPP
#define IGA_FLOATS_HPP

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

// Provides utilities for dealing with floating point numbers including some
// minimal fp16 support.

#if !defined(_WIN32) || (_MSC_VER >= 1800)
// GCC and VS2013 and higher support these
#define IS_NAN(X) std::isnan(X)
#define IS_INF(X) std::isinf(X)
#else
#define IS_NAN(X) ((X) != (X))
#define IS_INF(X) (!IS_NAN(X) && IS_NAN((X) - (X)))
#endif

namespace iga {

// formats a floating point value in decimal if possible
// otherwise it falls back to hex
void FormatFloat(std::ostream &os, double d);
void FormatFloat(std::ostream &os, float f);
void FormatFloat(std::ostream &os, uint16_t h);
void FormatFloat(std::ostream &os, uint8_t q); // GEN's 8-bit restricted float

// These functions exist since operations on NaN values might change the NaN
// payload.  E.g. An sNan might convert to a qNan during a cast
float ConvertDoubleToFloat(double d);
uint32_t ConvertDoubleToFloatBits(double d);
uint16_t ConvertFloatToHalf(float f);
static inline uint16_t ConvertDoubleToHalf(double d) {
  return ConvertFloatToHalf(ConvertDoubleToFloat(d));
}
float ConvertHalfToFloat(uint16_t u16);
double ConvertFloatToDouble(float f32);

// This expands Intel GEN's restricted 8-bit format
float ConvertQuarterToFloatGEN(uint8_t u8);

// Various raw accessors to convert between bits and float
static inline uint16_t FloatToBits(uint16_t f) { return f; }
static inline uint64_t FloatToBits(double f) {
  union {
    double f;
    uint64_t i;
  } u;
  u.f = f;
  return u.i;
}
static inline uint32_t FloatToBits(float f) {
  union {
    float f;
    uint32_t i;
  } u;
  u.f = f;
  return u.i;
}

static inline uint16_t FloatFromBits(uint16_t f) { return f; }
static inline float FloatFromBits(uint32_t f) {
  union {
    float f;
    uint32_t i;
  } u;
  u.i = f;
  return u.f;
}
static inline double FloatFromBits(uint64_t f) {
  union {
    double f;
    uint64_t i;
  } u;
  u.i = f;
  return u.f;
}

bool IsNaN(uint16_t u16);
bool IsInf(uint16_t u16);

static const uint64_t F64_SIGN_BIT = 0x8000000000000000ull;
static const uint64_t F64_EXP_MASK = 0x7FF0000000000000ull;
static const uint64_t F64_MANT_MASK = 0x000FFFFFFFFFFFFFull;
static const uint64_t F64_QNAN_BIT = 0x0008000000000000ull;
static const uint32_t F32_SIGN_BIT = 0x80000000;
static const uint32_t F32_EXP_MASK = 0x7F800000;
static const uint32_t F32_MANT_MASK = 0x007FFFFF;
static const uint32_t F32_QNAN_BIT = 0x00400000;
static const uint16_t F16_SIGN_BIT = 0x8000;
static const uint16_t F16_EXP_MASK = 0x7C00;
static const uint16_t F16_MANT_MASK = 0x03FF;
static const uint16_t F16_QNAN_BIT = 0x0200;

// Parses the lexical FLTLIT pattern to into a double
bool ParseFLTLIT(const std::string &string, double &d);

} // namespace iga

#endif // IGA_FLOATS_HPP
