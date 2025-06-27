/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGA_BACKEND_NATIVE_INSTENCODER_HPP
#define IGA_BACKEND_NATIVE_INSTENCODER_HPP

#include "../../IR/Kernel.hpp"
#include "../../asserts.hpp"
#include "../../bits.hpp"
#include "../../strings.hpp"
#include "../BitProcessor.hpp"
#include "../EncoderOpts.hpp"
#include "Field.hpp"
#include "MInst.hpp"
//
#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
//
#ifdef _DEBUG
// IGA_IGA_VALIDATE_BITS adds extra structures and code to ensure that each bit
// the instruction encoded gets written at most once.  This can catch
// accidental field overlaps quite effectively.
//
// The neceessary assumption follows that encoders must not get lazy or sloppy
// and just clobber bits.  Set it once only.
#define IGA_VALIDATE_BITS
#endif

namespace iga {
// immLo <= offset < immHi
static inline bool rangeContains(int immLo, int immHi, int offset) {
  return immLo <= offset && offset < immHi;
}

struct InstEncoderState {
  int instIndex = 0;
  const Instruction *inst = nullptr;
#ifdef IGA_VALIDATE_BITS
  // All the bit fields set by some field during this instruction encoding
  // e.g. if a field with bits [127:96] is set to 00000000....0001b
  // this bit mask will contain 1111....111b's for that field
  // This allows us to detect writing to two overlapped fields, which
  // indicates an internal logical error in the encoder.
  MInst dirty;
  // This contains a list of all the fields we set during an instruction
  // encoding so we can run through the list and determine which fields
  // overlapped.
  std::vector<const Fragment *> fragmentsSet;
#endif
  InstEncoderState(int _instIndex, const Instruction *_inst
#ifdef IGA_VALIDATE_BITS
                   ,
                   const MInst &_dirty,
                   const std::vector<const Fragment *> &_fieldsSet
#endif
                   )
      : instIndex(_instIndex), inst(_inst)
#ifdef IGA_VALIDATE_BITS
        ,
        dirty(_dirty), fragmentsSet(_fieldsSet)
#endif
  {
  }
  InstEncoderState() {}
};

struct Backpatch {
  InstEncoderState state;
  const Block *target;
  const Field &fragment;
  enum Type { REL, ABS } type;

  Backpatch(InstEncoderState &_state, const Block *_target, const Field &_field,
            Type _type = REL)
      : state(_state), target(_target), fragment(_field), type(_type) {}
  // TODO: determine where copy construction used to eliminate
  // Backpatch(const Backpatch&) = delete;
};
typedef std::vector<Backpatch> BackpatchList;

// can be passed into the encoding if we want to know the result
struct CompactionDebugInfo {
  std::vector<Op> fieldOps; // the op we were trying to compact
  std::vector<const CompactionMapping *> fieldMisses; // which indices missed
  std::vector<uint64_t> fieldMapping; // what we tried to match (parallel)
};
enum class CompactionResult {
  CR_SUCCESS,
  CR_NO_COMPACT,     // {NoCompact} (or -Xnoautocompact and {}
  CR_NO_FORMAT,      // e.g. send or jump
  CR_NO_AUTOCOMPACT, // no annotation and {Compacted} not set
  CR_MISS            // fragment misses
};

//////////////////////////////////////////////////////////
// generic encoder for all platforms
// subclasses specialize the necessary functions
class InstEncoder : public BitProcessor {
  // the encoder options (e.g. for auto-compaction)
  EncoderOpts opts;
  //
  // the platform we're encoding for
  const Model &model;
  //
  // The target bits to encode to
  MInst *bits = nullptr;
  //
  // A backpatch list that we can add to.  A parent encoder is permitted
  // to copy or shadow this list.  Hence, this class may not assume the
  // state is preserved between the calls of encodeInstruction and
  // resolveBackpatch.
  BackpatchList backpatches;
  //
  // state used by the encoder (can be saved for backpatches)
  InstEncoderState state;

public:
  InstEncoder(const EncoderOpts &_opts, BitProcessor &_parent,
              const Model &_model)
      : BitProcessor(_parent), opts(_opts), model(_model) {}
  InstEncoder(const InstEncoder &) = delete;
  InstEncoder& operator=(const InstEncoder&) = delete;

  BackpatchList &getBackpatches() { return backpatches; }

  const Instruction &getInst() const { return *state.inst; }
  const OpSpec &getOpSpec() const { return getInst().getOpSpec(); }
  const Model &getModel() const { return model; }
  Platform platform() const { return getModel().platform; }

  //        void setBits(MInst mi) {*bits = mi;}
  //        MInst getBits() const {return *bits;}

  ////////////////////////////////////////////////////////////////////////
  // external interface (called by parent encoder; e.g. SerialEncoder)  //
  ////////////////////////////////////////////////////////////////////////

  // Called externally by our parent encoder algorithm to start the encoding
  // of an instruction.  The instruction size is determined by the compaction
  // bit in the target instruction.
  void encodeInstruction(int ix, const Instruction &i, MInst *_bits) {
    setCurrInst(&i);

    state.instIndex = ix;
    state.inst = &i;
#ifdef IGA_VALIDATE_BITS
    state.dirty.qw0 = 0;
    state.dirty.qw1 = 0;
    state.fragmentsSet.clear();
#endif
    _bits->reset();
    encodeForPlatform(i);
  }

  void resolveBackpatch(Backpatch &bp, MInst *_bits) {
    bits = _bits;
    state = bp.state;
    setCurrInst(bp.state.inst);
    if (bp.type == Backpatch::ABS) {
      encode(bp.fragment, bp.target->getPC());
    } else {
      encode(bp.fragment, bp.target->getPC() - bp.state.inst->getPC());
    }
  }

  //////////////////////////////////////////////////////
  // internal (to the encoder package, not private) instances of
  // encodeForPlatform<P> and their subtrees will call these methods.
  //////////////////////////////////////////////////////
  void registerBackpatch(const Field &f, const Block *b,
                         Backpatch::Type t = Backpatch::Type::REL) {
    backpatches.emplace_back(state, b, f, t);
  }

  void encode(const Field &f, int32_t val) {
    encodeFieldBits(f, (uint32_t)val);
  }
  void encode(const Field &f, int64_t val) {
    encodeFieldBits(f, (uint64_t)val);
  }
  void encode(const Field &f, uint32_t val) {
    encodeFieldBits(f, (uint64_t)val);
  }
  void encode(const Field &f, uint64_t val) { encodeFieldBits(f, val); }
  void encode(const Field &f, bool val) { encodeFieldBits(f, val ? 1 : 0); }
  void encode(const Field &f, const OpSpec &os);
  void encode(const Field &f, ExecSize es);
  void encode(const Field &f, MathMacroExt acc);
  void encode(const Field &f, Region::Vert vt);
  void encode(const Field &f, Region::Width wi);
  void encode(const Field &f, Region::Horz hz);
  void encode(const Field &f, SrcModifier mods);
  template <OpIx IX>
  void encodeSubreg(const Field &f, RegName reg, RegRef rr, Type ty);
  void encodeReg(const Field &fREGFILE, const Field &fREG, RegName reg,
                 int regNum);

  //////////////////////////////////////////////////////
  // Helper Functions
  //////////////////////////////////////////////////////
  void encodeFieldBits(const Field &f, uint64_t val0);

  void encodingError(const std::string &msg) { encodingError("", msg); }
  void encodingError(const Field *f, const std::string &msg) {
    encodingError(f ? f->name : "", msg);
  }
  void encodingError(const Field &f, const std::string &msg) {
    encodingError(f.name, msg);
  }
  void encodingError(OpIx ix, const std::string &msg) {
    encodingError(ToStringOpIx(ix), msg);
  }
  void encodingError(const std::string &ctx, const std::string &msg) {
    if (ctx.empty()) {
      errorT(msg);
    } else {
      errorT(ctx, ": ", msg);
    }
  }
  void internalErrorBadIR(const std::string &what) {
    errorT("INTERNAL ERROR: malformed IR: ", what);
  }
  void internalErrorBadIR(const Field &f, const char *msg = nullptr) {
    if (msg) {
      internalErrorBadIR(iga::format(f.name, ": ", msg));
    } else {
      internalErrorBadIR(f.name);
    }
  }

  template <typename T> void encodeEnum(const Field &f, T lo, T hi, T x) {
    if (x < lo || x > hi) {
      encodingError(f, "invalid value");
    }
    encodeFieldBits(f, static_cast<uint64_t>(x));
  }

private:
#ifdef IGA_VALIDATE_BITS
  void reportFieldOverlap(const Fragment &fr);
#endif
private:
  void encodeForPlatform(const Instruction &i);
  void encodeForPlatformFamilyXE(const Instruction &i);
}; // end class InstEncoder

///////////////////////////////////////////////////////////////////////////
// longer method implementations (declared above)
///////////////////////////////////////////////////////////////////////////

inline void InstEncoder::encode(const Field &f, const OpSpec &os) {
  if (!os.isValid()) {
    encodingError(f, "invalid opcode");
  }
  encodeFieldBits(f, os.opcode);
}

#define ENCODING_CASE(X, V)                                                    \
  case X:                                                                      \
    val = (V);                                                                 \
    break
inline void InstEncoder::encode(const Field &f, ExecSize es) {
  uint64_t val = 0;
  switch (es) {
    ENCODING_CASE(ExecSize::SIMD1, 0);
    ENCODING_CASE(ExecSize::SIMD2, 1);
    ENCODING_CASE(ExecSize::SIMD4, 2);
    ENCODING_CASE(ExecSize::SIMD8, 3);
    ENCODING_CASE(ExecSize::SIMD16, 4);
    ENCODING_CASE(ExecSize::SIMD32, 5);
  default:
    internalErrorBadIR(f);
  }
  encodeFieldBits(f, val);
}

// encodes this as an math macro operand reference (e.g. r12.mme2)
// *not* as an explicit operand (for context save and restore)
inline void InstEncoder::encode(const Field &f, MathMacroExt mme) {
  uint64_t val = 0;
  switch (mme) {
    ENCODING_CASE(MathMacroExt::MME0, 0x0);
    ENCODING_CASE(MathMacroExt::MME1, 0x1);
    ENCODING_CASE(MathMacroExt::MME2, 0x2);
    ENCODING_CASE(MathMacroExt::MME3, 0x3);
    ENCODING_CASE(MathMacroExt::MME4, 0x4);
    ENCODING_CASE(MathMacroExt::MME5, 0x5);
    ENCODING_CASE(MathMacroExt::MME6, 0x6);
    ENCODING_CASE(MathMacroExt::MME7, 0x7);
    ENCODING_CASE(MathMacroExt::NOMME, 0x8);
  default:
    internalErrorBadIR(f);
  }
  encodeFieldBits(f, val);
}

inline void InstEncoder::encode(const Field &f, Region::Vert vt) {
  uint64_t val = 0;
  switch (vt) {
    ENCODING_CASE(Region::Vert::VT_0, 0x0);
    ENCODING_CASE(Region::Vert::VT_1, 0x1);
    ENCODING_CASE(Region::Vert::VT_2, 0x2);
    ENCODING_CASE(Region::Vert::VT_4, 0x3);
    ENCODING_CASE(Region::Vert::VT_8, 0x4);
    ENCODING_CASE(Region::Vert::VT_16, 0x5);
    ENCODING_CASE(Region::Vert::VT_32, 0x6);
  case Region::Vert::VT_VxH:
    val = 0xF;
    if (f.encodedLength() == 3) {
      val = 0x7; // XeHPC+
    }
    break;
  default:
    internalErrorBadIR(f);
  }
  encodeFieldBits(f, val);
}

inline void InstEncoder::encode(const Field &f, Region::Width wi) {
  uint64_t val = 0;
  switch (wi) {
    ENCODING_CASE(Region::Width::WI_1, 0x0);
    ENCODING_CASE(Region::Width::WI_2, 0x1);
    ENCODING_CASE(Region::Width::WI_4, 0x2);
    ENCODING_CASE(Region::Width::WI_8, 0x3);
    ENCODING_CASE(Region::Width::WI_16, 0x4);
  default:
    internalErrorBadIR(f);
  }
  encodeFieldBits(f, val);
}

inline void InstEncoder::encode(const Field &f, Region::Horz hz) {
  uint64_t val = 0;
  switch (hz) {
    ENCODING_CASE(Region::Horz::HZ_0, 0);
    ENCODING_CASE(Region::Horz::HZ_1, 1);
    ENCODING_CASE(Region::Horz::HZ_2, 2);
    ENCODING_CASE(Region::Horz::HZ_4, 3);
  default:
    internalErrorBadIR(f);
  }
  encodeFieldBits(f, val);
}

inline void InstEncoder::encode(const Field &f, SrcModifier mods) {
  uint64_t val = 0;
  switch (mods) {
    ENCODING_CASE(SrcModifier::NONE, 0x0);
    ENCODING_CASE(SrcModifier::ABS, 0x1);
    ENCODING_CASE(SrcModifier::NEG, 0x2);
    ENCODING_CASE(SrcModifier::NEG_ABS, 0x3);
  default:
    internalErrorBadIR(f);
  }
  encodeFieldBits(f, val);
}
#undef ENCODING_CASE

inline void InstEncoder::encodeReg(const Field &fREGFILE, const Field &fREG,
                                   RegName reg, int regNum) {
  const RegInfo *ri = model.lookupRegInfoByRegName(reg);
  if (ri == nullptr) {
    internalErrorBadIR(fREG, "unsupported register for platform");
    return;
  }
  uint16_t regNumBits = 0;
  if (!ri->encode(regNum, regNumBits)) {
    internalErrorBadIR(fREG, "invalid register");
    return;
  }
  encodeFieldBits(fREGFILE, reg == RegName::GRF_R ? 1 : 0);
  encodeFieldBits(fREG, regNumBits);
}

template <OpIx IX>
inline void InstEncoder::encodeSubreg(const Field &f, RegName reg, RegRef rr,
                                      Type ty) {
  uint64_t val = (uint64_t)rr.subRegNum;
  // branches have implicit :d
  val = ty == Type::INVALID
            ? 4 * val
            : SubRegToBinaryOffset((int)val, reg, ty, model.platform);
  encodeFieldBits(f, val);
}

inline void InstEncoder::encodeForPlatform(const Instruction &i) {
  switch (platform()) {
  case Platform::XE:
  case Platform::XE_HP:
  case Platform::XE_HPG:
  case Platform::XE_HPC:
  case Platform::XE2:
  case Platform::XE3:
    break;
  case Platform::FUTURE:
  default:
    // caller checks this and gives a soft error
    IGA_ASSERT_FALSE("unsupported platform for native encoder");
  }
}
} // namespace iga
#endif /* IGA_BACKEND_NATIVE_INSTENCODER_HPP */
