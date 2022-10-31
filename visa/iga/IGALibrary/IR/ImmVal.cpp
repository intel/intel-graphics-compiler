/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ImmVal.hpp"
#include "../Frontend/Floats.hpp"

#include <cmath>

using namespace iga;

ImmVal &ImmVal::operator=(uint8_t x) {
  kind = Kind::U8;
  u64 = 0;
  u8 = x;
  return *this;
}
ImmVal &ImmVal::operator=(int8_t x) {
  kind = Kind::S8;
  u64 = 0;
  s8 = x;
  return *this;
}
ImmVal &ImmVal::operator=(uint16_t x) {
  kind = Kind::U16;
  u64 = 0;
  u16 = x;
  return *this;
}
ImmVal &ImmVal::operator=(int16_t x) {
  kind = Kind::S16;
  u64 = 0;
  s16 = x;
  return *this;
}
ImmVal &ImmVal::operator=(uint32_t x) {
  kind = Kind::U32;
  u64 = 0;
  u32 = x;
  return *this;
}
ImmVal &ImmVal::operator=(int32_t x) {
  kind = Kind::S32;
  u64 = 0;
  s32 = x;
  return *this;
}
ImmVal &ImmVal::operator=(uint64_t x) {
  kind = Kind::U64;
  u64 = x;
  return *this;
}
ImmVal &ImmVal::operator=(int64_t x) {
  kind = Kind::S64;
  s64 = x;
  return *this;
}
ImmVal &ImmVal::operator=(float x) {
  kind = Kind::F32;
  u64 = 0;
  f32 = x;
  return *this;
}
ImmVal &ImmVal::operator=(double x) {
  kind = Kind::F64;
  f64 = x;
  return *this;
}

void ImmVal::Abs() {
  switch (kind) {
  case Kind::F16:
    // the sign bit manually so that we can "negate" NaN values
    u16 &= ~F16_SIGN_BIT;
    break;
  case Kind::F32:
    // see F16
    u32 &= ~F32_SIGN_BIT;
    break;
  case Kind::F64:
    // see F64
    u64 &= ~F64_SIGN_BIT;
    break;

  case Kind::S8:
    s8 = s8 < 0 ? -s8 : s8;
    break;
  case Kind::S16:
    s32 = s16 < 0 ? -s16 : s16;
    break;
  case Kind::S32:
    s32 = s32 < 0 ? -s32 : s32;
    break;
  case Kind::S64:
    s64 = s64 < 0 ? -s64 : s64;
    break;
  default:
    break;
  }
}

void ImmVal::Negate() {
  switch (kind) {
  case Kind::F16:
    // N.B. we manually flip the sign bit
    // so that -{s,q}nan gives the right bit pattern on all HW
    u16 = F16_SIGN_BIT ^ u16;
    break;
  case Kind::F32:
    // see above
    u32 = F32_SIGN_BIT ^ u32;
    break;
  case Kind::F64:
    // see above
    u64 = F64_SIGN_BIT ^ u64;
    break;

  case Kind::S8:
    s8 = -s8;
    break;
  case Kind::S16:
    s16 = -s16;
    break;
  case Kind::S32:
    s32 = -s32;
    break;
  case Kind::S64:
    s64 = -s64;
    break;
  default:
    break;
  }
}
