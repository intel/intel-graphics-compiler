/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_BACKEND_NATIVE_FIELD_HPP
#define IGA_BACKEND_NATIVE_FIELD_HPP

#include "../../api/iga_bxml_ops.hpp"
#include "../../asserts.hpp"
#include "../../bits.hpp"
#include "../../strings.hpp"

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <ostream>
#include <string>

#if (__cplusplus >= 201402L) || (defined(_MSC_VER) && (_MSVC_LANG >= 201402L))
// We assume we are at least C++11, but here we're trying to ensure we
// are C++14 or better.  Various constexpr functions below will then be able
// to opt-in based if our compiler is C++14 or better.  C++11 allows us to
// return constexpr expressions only, but C++14 allows more statements.
//
// MSVS defines __cplusplus as 199711L, but they do use _MSVC_LANG in those
// release.  Maybe there's a better way.
#define CONSTEXPR_IF_CPP14 constexpr
#else
#define CONSTEXPR_IF_CPP14
#endif

namespace iga {
#define INVALID_FIELD Field()

// Defines a fragment of a field.  A field consists of one or
// more fragments.  See the struct below for more definition.
struct Fragment {
  static const int NO_OFFSET = -1;

  enum class Kind {
    INVALID = 0,
    ENCODED,
    ZERO_FILL,  // takes no space in encodings
    ZERO_WIRES, // takes up encoding space, but must be zero
                // ONES_FILL (only enable if needed)
                // other more complicated functions (e.g. clone)
  } kind = Kind::ENCODED;
  const char *name; // e.g. "Dst.Subreg[4:3]"
  int offset;       // offset in the instruction
  int length;       // number of bits in this fragment

  constexpr Fragment(const char *_name, int _offset, int _length,
                     Kind _kind = Kind::ENCODED)
      : kind(_kind), name(_name), offset(_offset), length(_length) {}
  constexpr Fragment() : Fragment(nullptr, NO_OFFSET, 0, Kind::INVALID) {}

  // We could lower this to C++11 if we really tried.
  //        (it's a pure boolean expression underneath)
  CONSTEXPR_IF_CPP14 bool overlaps(const Fragment &f) const {
    if (!isEncoded() || !f.isEncoded())
      return false; // pseudo-fragments (unmapped) don't collide

    // arrange intervals as: smaller or equal, and larger
    const Fragment &smaller = length <= f.length ? *this : f;
    const Fragment &larger = length <= f.length ? f : *this;
    int sLo = smaller.offset, sHi = smaller.offset + smaller.length - 1;
    int lLo = larger.offset, lHi = larger.offset + larger.length - 1;
    // determine if the ends of smaller fit in larger
    return (sLo >= lLo && sLo <= lHi) || (sHi >= lLo && sHi <= lHi);
  }

  constexpr bool isEncoded() const { return kind == Kind::ENCODED; }
  constexpr bool isZeroFill() const { return kind == Kind::ZERO_FILL; }
  constexpr bool isZeroWires() const { return kind == Kind::ZERO_WIRES; }
  constexpr bool isInvalid() const { return kind == Kind::INVALID; }

  constexpr uint64_t getMask() const {
    return getFieldMaskUnshifted<uint64_t>(length);
  }
};

// A field represents a set of bits encoded in an instruction that a
// processor uses. E.g. the opcode, a register number, ....
//
// Typically, fields are contiguous, but some are fragmented in
// non-contiguous parts in the instruction.  Moreover, some of these
// fragments aren't even encoded, but are intrinically defined
// (implicitly defined).
//
// One example of a fragmented field is ExDesc in the send instruction.
// which has fragments scattered all over the 128b encoding.
//
// Another pattern is intrinsic fragments.  That is, a field has bits
// that are not encoding in the instruction, but are implicitly some
// value (usually 0).  The .kind field will be set to KIND::ZERO_FILL,
// for example.
//
// E.g. in TGL a ternary dst register encodes Dst.Subreg[4:3].
//      Dst.Subreg[2:0] don't get encoded in the instruction and are
//      implicitly defined as zeros.  This is logically the same as
//      decoding the two bits containing Dst.Subreg[4:3] and
//      shifting it left 3 bits.  In this scheme we'd define this as
//      A field consistening of an ENCODED fragment for [4:3] and a
//
//
struct Field {
  static const int MAX_FRAGMENTS_PER_FIELD = 4;

  // the field name; e.g. "Dst.Reg"
  const char *name;

  // in order of low to high
  Fragment fragments[MAX_FRAGMENTS_PER_FIELD];

  // an undefined field (zero fragments)
  constexpr Field() : name(nullptr) {}
  // a simple encoded field (single contiguous)
  constexpr Field(const char *_name, int offset, int length)
      : name(_name), fragments{Fragment(_name, offset, length)} {}
  // a zero fill field or must-be-zero field
  constexpr Field(const char *_name, int length,
                  Fragment::Kind kind = Fragment::Kind::ZERO_FILL)
      : name(_name), fragments{
                         Fragment(_name, Fragment::NO_OFFSET, length, kind)} {}
  // a field with two fragments
  constexpr Field(const char *_name, const char *name0, int offset0,
                  int length0, Fragment::Kind kind0, const char *name1,
                  int offset1, int length1, Fragment::Kind kind1)
      : name(_name), fragments{Fragment(name0, offset0, length0, kind0),
                               Fragment(name1, offset1, length1, kind1)} {}
  // a field with three fragments
  constexpr Field(const char *_name, const char *name0, int offset0,
                  int length0, Fragment::Kind kind0, const char *name1,
                  int offset1, int length1, Fragment::Kind kind1,
                  const char *name2, int offset2, int length2,
                  Fragment::Kind kind2)
      : name(_name), fragments{
                         Fragment(name0, offset0, length0, kind0),
                         Fragment(name1, offset1, length1, kind1),
                         Fragment(name2, offset2, length2, kind2),
                     } {}
  // a field with two encoded fragments
  constexpr Field(const char *_name, const char *name0, int offset0,
                  int length0, const char *name1, int offset1, int length1)
      : Field(_name, name0, offset0, length0, Fragment::Kind::ENCODED, name1,
              offset1, length1, Fragment::Kind::ENCODED) {}
  // a field with three encoded fragments
  constexpr Field(const char *_name, const char *name0, int offset0,
                  int length0, const char *name1, int offset1, int length1,
                  const char *name2, int offset2, int length2)
      : Field(_name, name0, offset0, length0, Fragment::Kind::ENCODED, name1,
              offset1, length1, Fragment::Kind::ENCODED, name2, offset2,
              length2, Fragment::Kind::ENCODED) {}
  // a field a fragment of intrinsically defined 0's and top bits encoded
  constexpr Field(const char *_name, const char *name0, int shift,
                  const char *name1, int offset1, int length1)
      : Field(_name, name0, Fragment::NO_OFFSET, shift,
              Fragment::Kind::ZERO_FILL, name1, offset1, length1,
              Fragment::Kind::ENCODED) {}

  // returns the field length
  CONSTEXPR_IF_CPP14 int length() const { return decodedLength(); }

  // returns the decoded length of the entire field
  CONSTEXPR_IF_CPP14 int decodedLength() const {
    int len = 0;
    for (const auto &fr : fragments) {
      if (fr.isInvalid())
        break;
      len += fr.length;
    }
    return len;
  }

  // returns the decoded length of the entire field
  CONSTEXPR_IF_CPP14 int encodedLength() const {
    int len = 0;
    for (const auto &fr : fragments) {
      if (fr.isInvalid())
        break;
      else if (fr.isEncoded() || fr.isZeroWires())
        len += fr.length;
    }
    return len;
  }

  constexpr bool isAtomic() const {
    return fragments[1].isInvalid() &&
           // if this is MBZ field
           (fragments[0].isEncoded() || fragments[0].isZeroFill() ||
            fragments[0].isZeroWires());
  }

  // returns a reference the first fragment
  // asserts that the fragment is atomic
  const Fragment &atomFragment() const {
    IGA_ASSERT(isAtomic(), "fragment is not atomic");
    return fragments[0];
  }

  // implicit conversion to fragment
  operator const Fragment &() const { return atomFragment(); }

  constexpr Field fragmentToField(int ix) const {
    return Field(fragments[ix].name, fragments[ix].offset,
                 fragments[ix].length);
  }
}; // Field

static inline bool operator==(const Fragment &f1, const Fragment &f2) {
  return f1.kind == f2.kind && f1.offset == f2.offset &&
         f1.length == f2.length && strcmp(f1.name, f2.name) == 0;
}
static inline bool operator<(const Fragment &f1, const Fragment &f2) {
  if (f1.kind != f2.kind) {
    return f1.kind < f2.kind;
  } else if (f1.offset != f2.offset) {
    return f1.offset < f2.offset;
  } else if (f1.length != f2.length) {
    return f1.length < f2.length;
  } else {
    return strcmp(f1.name, f2.name) < 0;
  }
}

//////////////////////////////////////////////////////////////////////////
// enables us to group various operands together
struct OperandFields {
  const char *name;
  const Field &fTYPE;
  const Field *pfSRCMODS;
  const Field &fREGFILE;

  const Field &fREG;
  const Field &fSUBREG;
  const Field &fSPCACC;

  const Field *pfRGNVT;
  const Field *pfRGNWI;
  const Field &fRGNHZ;

  const Field *pfADDRMODE;
  const Field *pfADDRREG;
  const Field *pfADDROFF;

  const Field *pfSRCISIMM;
  const Field *pfSRCIMM32L; // also IMM16
  const Field *pfSRCIMM32H;
};

enum OpIx {
  IX_DST = 0x40, // must not overlap with other bits
  IX_SRC0 = 0x10,
  IX_SRC1 = 0x20,
  IX_SRC2 = 0x30,
  OP_IX_TYPE_MASK = 0xF0,

  OP_IX_MASK = 0x0F, // masks table index (unique across bas. and trn.)

  BAS = 0x100,
  BAS_DST = BAS | IX_DST | 0,
  BAS_SRC0 = BAS | IX_SRC0 | 1,
  BAS_SRC1 = BAS | IX_SRC1 | 2,

  TER = 0x200,
  TER_DST = TER | IX_DST | 3,
  TER_SRC0 = TER | IX_SRC0 | 4,
  TER_SRC1 = TER | IX_SRC1 | 5,
  TER_SRC2 = TER | IX_SRC2 | 6
};
static inline bool IsTernary(OpIx IX) { return (IX & OpIx::TER) == OpIx::TER; }
static inline bool IsDst(OpIx IX) {
  return (IX & OpIx::IX_DST) == OpIx::IX_DST;
}
static inline int ToSrcIndex(OpIx IX) {
  IGA_ASSERT(!IsDst(IX), "ToSrcIndex(OpIx) on dst index");
  return ((IX & 0x30) >> 4) - 1;
}
static inline int ToFieldOperandArrayIndex(OpIx IX) {
  return (IX & OpIx::OP_IX_MASK);
}
static inline std::string ToStringOpIx(OpIx ix) {
  switch (ix & OpIx::OP_IX_TYPE_MASK) {
  case OpIx::IX_DST:
    return "dst";
  case OpIx::IX_SRC0:
    return "src0";
  case OpIx::IX_SRC1:
    return "src1";
  case OpIx::IX_SRC2:
    return "src2";
  default:
    return "?";
  }
}

// a grouping of compaction information
struct CompactionMapping {
  const Field &index;
  const uint64_t *values;
  size_t numValues;
  const Field **mappings;
  size_t numMappings;
  const char **meanings; // length is numValues
  // this is used for "what would this compaction word mean"
  std::string (*format)(Op, uint64_t);

  // sums up with width of all the fields mapped
  size_t countNumBitsMapped() const {
    size_t entrySizeBits = 0;
    for (size_t k = 0; k < numMappings; k++)
      entrySizeBits += mappings[k]->length();
    return entrySizeBits;
  }
  bool isSrcImmField() const { return mappings == nullptr; }

  // emits output such as  "0`001`1`0`001"
  // for SrcImm compacted fields it just emits the value
  void emitBinary(std::ostream &os, uint64_t val) const {
    if (mappings == nullptr) {
      // srcimm field
      os << std::setw(16) << "(" << fmtHex(val) << ")";
    } else {
      int bitOff = (int)countNumBitsMapped();
      for (int mIx = 0; mIx < (int)numMappings; ++mIx) {
        if (mIx != 0) {
          os << "`";
        }
        const Field *mf = mappings[mIx];
        bitOff -= mf->length();
        auto bs = iga::getBits(val, bitOff, mf->length());
        fmtBinaryDigits(os, bs, mf->length());
      }
    }
  }
};

// groups all CompactionMappings for a given platform
struct CompactionScheme {
  const CompactionMapping &CTRLIX_2SRC;
  const CompactionMapping &DTIX_2SRC;
  const CompactionMapping &SRIX_2SRC;
  const CompactionMapping &SRC0IX_2SRC;
  const CompactionMapping &SRC1IX_2SRC;
  //
  const CompactionMapping &CTRLIX_3SRC;
  const CompactionMapping &SRCIX_3SRC;
  const CompactionMapping &SRIX_3SRC;
};

// Assumes existence of:
//   constexpr static Field CMP_ ## SYM ## _ ## PLATFORM = ...
//   std::string [namespace::]Format_ ## SYM ## _ ## PLATFORM (Op, uint64_t)
//   const const Field         SYM ## _MAPPINGS_ ## PLATFORM  [M] {...}
//   const const uint64_t      SYM ## _VALUES_   ## PLATFORM  [N] {...}
//   const const const char *  SYM ## _MEANINGS_ ## PLATFORM  [N] {...}
//
// For example:
//     MAKE_COMPACTION_MAPPING(CTRLIX_2SRC, TGL);
//
// utilizes all of the following:
//
//   constexpr static Field    iga::xe::CMP_CTRLIX_2SRC_TGL {...}
//   std::string               iga::xe::Format_CTRLIX_2SRC_TGL (Op, uint64_t)
//   const const Field         iga::xe::CTRLIX_2SRC_MAPPINGS_TGL  [M] {...}
//   const const uint64_t      iga::xe::CTRLIX_2SRC_VALUES_TGL    [N] {...}
//   const const const char *  iga::xe::CTRLIX_2SRC_MEANINGS_TGL  [N] {...}
#define MAKE_COMPACTION_MAPPING(SYM, PLATFORM)                                 \
  extern std::string Format_##SYM##_##PLATFORM(Op, uint64_t);                  \
                                                                               \
  static_assert(sizeof(SYM##_VALUES_##PLATFORM) /                              \
                        sizeof(SYM##_VALUES_##PLATFORM[0]) ==                  \
                    sizeof(SYM##_MEANINGS_##PLATFORM) /                        \
                        sizeof(SYM##_MEANINGS_##PLATFORM[0]),                  \
                "mismatch in table sizes");                                    \
  static const CompactionMapping CM_##SYM##_##PLATFORM {                       \
    CMP_##SYM##_##PLATFORM, SYM##_VALUES_##PLATFORM,                           \
        sizeof(SYM##_VALUES_##PLATFORM) / sizeof(SYM##_VALUES_##PLATFORM[0]),  \
        SYM##_MAPPINGS_##PLATFORM,                                             \
        sizeof(SYM##_MAPPINGS_##PLATFORM) /                                    \
            sizeof(SYM##_MAPPINGS_##PLATFORM[0]),                              \
        SYM##_MEANINGS_##PLATFORM, &(Format_##SYM##_##PLATFORM),               \
  }
} // namespace iga

#endif /* IGA_BACKEND_NATIVE_FIELD_HPP */
