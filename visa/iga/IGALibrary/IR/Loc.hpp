/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_LOC
#define IGA_LOC
// WARNING: the IR is subject to change without any notice.  External tools
// should use the official interfaces in the external API.  Those interfaces
// are tested between releases and maintained even with changes to the IR.

#include <cstdint>

namespace iga {
typedef int32_t PC;

// a source location (can also be binary for things like decoding)
struct Loc {
  // int      file; // an optional file index
  PC offset = 0; // file offset or binary offset (PC)
  uint32_t line = 0;
  uint32_t col = 0;
  uint32_t extent = 0; // length (in characters/bytes)

  Loc() {}
  Loc(PC pc) : offset(pc) {}

  Loc(uint32_t ln, uint32_t cl, uint32_t off, uint32_t len)
      : offset((PC)off), line(ln), col(cl), extent(len) {}

  Loc endOfToken() const {
    Loc l = *this;
    l.col += extent;
    l.offset += extent;
    l.extent = 1;
    return l;
  }

  bool isValid() const { return line != INVALID.line && col != INVALID.col; }
  // tells if the offset value is a PC or text offset
  bool isBinary() const { return line == 0 && col == 0; }
  bool isText() const { return !isBinary(); }

  // bool operator==(const Loc &rhs) const {
  //    return this->line == rhs.line &&
  //        this->col == rhs.col &&
  //        this->offset == rhs.offset &&
  //        this->extent == rhs.extent;
  // }

  const static Loc INVALID;
};

} // namespace iga

#endif
