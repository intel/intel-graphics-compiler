/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_BITS_HPP
#define IGA_BITS_HPP

#include "asserts.hpp"

#include <cstdint>

namespace iga {
// macros are for initializers
//   other users should use the functions where possible
// TODO: someday constexpr will make this obsolete, but we want to avoid
//       static constructors (initializers) in large tables
#define BITFIELD_MASK32_UNSHIFTED(OFF, LEN)                                    \
  ((LEN) == 32 ? 0xFFFFFFFF : ((1 << (LEN)) - 1))
#define BITFIELD_MASK32(OFF, LEN) (BITFIELD_MASK32_UNSHIFTED(OFF, LEN) << (OFF))

template <typename T> static inline constexpr T getFieldMaskUnshifted(int len) {
  return len == 8 * sizeof(T) ? ((T)-1) : ((T)1 << len) - 1;
}
template <typename T> static inline constexpr T getFieldMask(int off, int len) {
  return getFieldMaskUnshifted<T>(len) << (off % (8 * sizeof(T)));
}
// static uint64_t getFieldMask(int off, int len)
// {
//     uint64_t mask = len == 64 ?
//         0xFFFFFFFFFFFFFFFFull : ((1ull << len) - 1);
//     return mask << off % 64;
// }
template <typename T> static inline T getBits(T bits, int off, int len) {
  IGA_ASSERT(off >= 0 && len > 0 && off + len - 1 < 8 * sizeof(T),
             "getBits: out of bounds");
  T mask = len == 8 * sizeof(T) ? ((T)-1) : (1ull << len) - 1;
  return ((bits >> off) & mask);
}
template <typename T> static inline bool testBit(T bits, int off) {
  IGA_ASSERT(off < 8 * sizeof(T), "testBit: out of bounds");
  return getBits<T>(bits, off, 1) != 0;
}
template <typename T> static inline T getSignedBits(T bits, int off, int len) {
  IGA_ASSERT(off >= 0 && len > 0 && off + len - 1 < 8 * sizeof(T),
             "getBits: out of bounds");
  T mask = len == 8 * sizeof(T) ? ((T)-1) : (1ull << len) - 1;
  T val = ((bits >> off) & mask);
  if (val & ((T)1 << (len - 1))) {
    // sign extend high bit if set
    val |= ~getFieldMaskUnshifted<T>(len);
  }
  return val;
}
template <typename T, int N>
static T getBits(const T bits[N], int off, int len) {
  IGA_ASSERT(off >= 0 && len > 0 && off + len - 1 < N * 8 * sizeof(T),
             "getBits: out of bounds");
  int wIx = off / (8 * sizeof(T)), wOff = off % (8 * sizeof(T));
  return getBits<T>(bits[wIx], wOff, len);
}
template <typename T, int N>
static inline T getSignedBits(const T bits[N], int off, int len) {
  IGA_ASSERT(off >= 0 && len > 0 && off + len - 1 < N * 8 * sizeof(T),
             "getSignedBits: out of bounds");
  int wIx = off / (8 * sizeof(T)), wOff = off % (8 * sizeof(T));
  return getSignedBits<T>(bits[wIx], wOff, len);
}

template <typename T, int N>
static inline bool setBits(T qws[N], int off, int len, T val) {
  IGA_ASSERT(off >= 0 && len > 0 && off + len - 1 < N * 8 * sizeof(T),
             "setBits: out of bounds");
  //
  const auto wIx = off / (8 * sizeof(T)), wOff = off % (8 * sizeof(T));
  //
  IGA_ASSERT(wIx == ((off + len - 1) / (8 * sizeof(T))),
             "setBits: field spans word boundary");
  //
  T shiftedVal = val << wOff; // shift into position
  T mask = getFieldMask<T>(off, len);
  if (shiftedVal & ~mask) {
    // either val has bits greater than what this field represents
    // or below the shift (e.g. smaller value than we accept)
    // e.g. Dst.Subreg[4:3] just has the high two bits of
    // the subregister and can't represent small values.
    //
    // Generally, this indicates an internal IGA problem:
    //  e.g. something we need to catch in the parser, IR checker or
    //       some other higher level (i.e. we're looking at bad IR)
    return false;
  }
  qws[wIx] |= shiftedVal;
  return true;
}

template <typename T>
static inline bool setBits(T &bits, int off, int len, T val) {
  IGA_ASSERT(off >= 0 && len > 0 && off + len - 1 < 8 * sizeof(T),
             "setBits: out of bounds");
  //
  T mask = getFieldMaskUnshifted<T>(len);
  if (val > mask)
    return false;
  bits = (bits & ~(mask << off)) | (val << off);
  return true;
}

// finds the high bit index set
// returns -1 given 0 (i.e. if no bits set)
//
// STL really needs this
// gcc has __builtin_clzll, but let's ignore the #ifdef nonsense
static inline int findLeadingOne(uint64_t v) {
  static const uint64_t MASKS[]{
      0xFFFFFFFF00000000ull, 0xFFFF0000, 0xFF00, 0xF0, 0xC, 0x2};

  // checks the top 32 (add 32 if it's there), also shift the bottom
  // check the top 16 of that result
  // ...
  // the mask could also be generated, but we expect it to unroll
  int index = 0;
  for (int i = 0, offset = 32; i < sizeof(MASKS) / sizeof(MASKS[0]);
       i++, offset >>= 1) {
    if (v & MASKS[i]) {
      v >>= offset;
      index += offset;
    }
  }

  return index;
}

} // namespace iga
#endif /* IGA_BITS_HPP */
