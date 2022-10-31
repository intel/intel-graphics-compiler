/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "InstEncoder.hpp"
#include "../BitProcessor.hpp"

#include <sstream>

// dummy symbol to avoid LNK4221 warning

using namespace iga;

#ifdef IGA_VALIDATE_BITS
void InstEncoder::reportFieldOverlap(const Fragment &fr) {
  std::stringstream ss;
  ss << "INTERNAL ERROR: ";
  ss << "instruction #" << state.instIndex << " [PC 0x" << std::hex
     << state.inst->getPC() << "]: " << fr.name << " overlaps with ";
  // iterate the old fragments and find one that overlapped
  for (const Fragment *of : state.fragmentsSet) {
    if (of->overlaps(fr)) {
      ss << of->name;
      auto str = ss.str();
      // std::cerr << "checkFieldOverlaps: " << str << "\n";
      // IGA_ASSERT_FALSE(str.c_str());
      errorT(str);
      return;
    }
  }
  ss << "another field (?)";
  auto str = ss.str();
  errorT(str);
  // IGA_ASSERT_FALSE(str.c_str());
}
#endif

void InstEncoder::encodeFieldBits(const Field &f, uint64_t val0) {
#ifndef IGA_VALIDATE_BITS
  // bit-validation can't cut this corner
  if (val0 == 0) {
    // short circuit since we start with zeros
    return;
  }
#endif
  uint64_t val = val0;
  for (const Fragment &fr : f.fragments) {
    if (fr.kind == Fragment::Kind::INVALID)
      break;
    const auto MASK = fr.getMask();
    const auto fragValue = val & MASK;
    switch (fr.kind) {
    case Fragment::Kind::ENCODED:
      bits->setBits(fr.offset, fr.length, fragValue);
#ifdef IGA_VALIDATE_BITS
      if (state.dirty.getBits(fr.offset, fr.length)) {
        // some of these bits have already been set by some
        // other field; this demonstrates an internal error
        // in our encoding logic
        reportFieldOverlap(fr);
      }
      state.fragmentsSet.push_back(&fr);
      state.dirty.setBits(fr.offset, fr.length, MASK);
#endif
      break;
    case Fragment::Kind::ZERO_WIRES:
    case Fragment::Kind::ZERO_FILL:
      if (fragValue != 0) {
        errorAtT(state.inst->getLoc(), fr.name, ": ", fmtHex(val0),
                 ": field fragment must be zero");
      }
      break;
    default:
      IGA_ASSERT_FALSE("unreachable");
    }
    val >>= fr.length;
  }

  if (val != 0) {
    // value overflows the virtual encoding
    errorAtT(state.inst->getLoc(), f.name, ": ", fmtHex(val0),
             ": value is too large for field");
  }
}
