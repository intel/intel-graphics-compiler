/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_IMMVAL_HPP_
#define _IGA_IMMVAL_HPP_

#include <stdint.h>

namespace iga {
struct ImmVal {
  enum class Kind {
    UNDEF,
    F16,
    F32,
    F64,

    S8,
    S16,
    S32,
    S64,

    U8,
    U16,
    U32,
    U64
  };

  union {
    uint16_t f16;
    float f32;
    double f64;

    int8_t s8;
    int16_t s16;
    int32_t s32;
    int64_t s64;

    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64 = 0;
  };
  Kind kind = Kind::UNDEF;

  bool isS64() const { return kind == Kind::S64; }
  bool isU64() const { return kind == Kind::U64; }
  bool isI64() const { return isS64() || isU64(); }

  ImmVal &operator=(uint8_t x);
  ImmVal &operator=(int8_t x);
  ImmVal &operator=(uint16_t x);
  ImmVal &operator=(int16_t x);
  ImmVal &operator=(uint32_t x);
  ImmVal &operator=(int32_t x);
  ImmVal &operator=(uint64_t x);
  ImmVal &operator=(int64_t x);
  ImmVal &operator=(float x);
  ImmVal &operator=(double x);

  void Abs();
  void Negate();

  void reset() { u64 = 0; }
}; // class ImmVal
} // namespace iga
#endif // _IGA_IMMVAL_HPP_
