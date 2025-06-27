/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_BACKEND_NATIVE_MINST_HPP
#define IGA_BACKEND_NATIVE_MINST_HPP

#include "../../bits.hpp"
#include "Field.hpp"

#include <cstdint>

namespace iga {
// machine instruction
struct MInst {
  union {
    struct {
      uint32_t dw0, dw1, dw2, dw3;
    };
    struct {
      uint32_t dws[4];
    };
    struct {
      uint64_t qw0, qw1;
    };
    struct {
      uint64_t qws[2];
    };
  };

  MInst() { qw0 = qw1 = 0; }
  void reset() { qw0 = qw1 = 0; }

  bool operator==(const MInst &rhs) const {
    return qw0 == rhs.qw0 && qw1 == rhs.qw1;
  }
  bool operator!=(const MInst &rhs) const { return !(*this == rhs); }
  bool operator<(const MInst &rhs) const {
    return (qw1 < rhs.qw1) || (qw1 == rhs.qw1 && qw0 < rhs.qw0);
  }

  bool testBit(int off) const { return getBits(off, 1) != 0; }

  uint64_t getBits(int off, int len) const {
    return iga::getBits<uint64_t, 2>(qws, off, len);
  }

  uint64_t getFragment(const Fragment &f) const {
    switch (f.kind) {
    case Fragment::Kind::ZERO_FILL:
    case Fragment::Kind::ZERO_WIRES:
      return 0;
    case Fragment::Kind::ENCODED:
      return getBits(f.offset, f.length);
    default:
      IGA_ASSERT_FALSE("unreachable");
      return (uint64_t)-1;
    }
  }

  // gets a fragmented field
  uint64_t getBits(const Fragment &fLo, const Fragment &fHi) const {
    uint64_t lo = getBits(fLo.offset, fLo.length);
    uint64_t hi = getBits(fHi.offset, fHi.length);
    return ((hi << fLo.length) | lo);
  }

  uint64_t getField(const Field &f) const {
    uint64_t bits = 0;
    //
    int off = 0;
    for (const Fragment &fr : f.fragments) {
      if (fr.kind == Fragment::Kind::INVALID)
        break;
      auto frag = fr.isZeroFill() ? 0 : getBits(fr.offset, fr.length);
      bits |= frag << off;
      off += fr.length;
    }
    //
    return bits;
  }
  int64_t getSignedField(const Field &f) const {
    auto bits = getField(f);
    return getSignedBits(bits, 0, f.length());
  }

  // gets a fragmented field from an array of fields
  // the fields are ordered low bit to high bit
  // we stop when we find a field with length 0
  template <int N> uint64_t getBits(const Fragment ff[N]) const {
    uint64_t bits = 0;

    int off = 0;
    for (const Fragment &fr : ff) {
      if (fr.isInvalid()) {
        break;
      }
      auto frag = fr.isZeroFill()
                      ? 0
                      : iga::getBits<uint64_t, N>(qws, fr.offset, fr.length);
      bits |= frag << off;
      off += fr.length;
    }

    return bits;
  }

  // returns false if field is too small to hold the value
  bool setField(const Field &f, uint64_t val) {
    for (const auto &fr : f.fragments) {
      if (fr.kind == Fragment::Kind::INVALID)
        break;
      const auto fragValue = val & fr.getMask();
      switch (fr.kind) {
      case Fragment::Kind::ENCODED:
        if (!setBits(fr.offset, fr.length, fragValue)) {
          // this overflows the fragment
          return false;
        }
        break;
      case Fragment::Kind::ZERO_WIRES:
      case Fragment::Kind::ZERO_FILL:
        if (fragValue != 0) {
          // an intrinsic fragment has non-zero bits
          // e.g. ternary Dst.Subreg[3:0] is non-zero
          return false;
        }
        break;
      default:
        IGA_ASSERT_FALSE("unreachable");
      }
      val >>= fr.length;
    }
    return val == 0; // ensure no high bits left over
  }

  bool setFragment(const Fragment &fr, uint64_t val) {
    if (fr.isEncoded())
      return iga::setBits<uint64_t, 2>(qws, fr.offset, fr.length, val);
    else
      return (fr.isZeroFill() && val == 0);
  }

  bool setBits(int off, int len, uint64_t val) {
    return iga::setBits<uint64_t, 2>(qws, off, len, val);
  }

  // checks if a machine instruction is compacted
  // if compacted, the high 64b are undefined
  bool isCompact() const { return testBit(29); }
};
static_assert(sizeof(MInst) == 16, "MInst must be 16 bytes");
} // namespace iga

#endif // IGA_BACKEND_NATIVE_MINST_HPP
