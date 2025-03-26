/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IRChecker.hpp"
#include "../../api/iga.h"

#include <sstream>

using namespace iga;

struct Checker {
  ErrorHandler *m_errHandler;
  const Model &m_model;

  Checker(const Model &model, ErrorHandler *err)
      : m_errHandler(err), m_model(model) {}

  void warning(const Loc &loc, const char *msg) {
    m_errHandler->reportWarning(loc, msg);
  }
  void error(const Loc &loc, const char *msg) {
    m_errHandler->reportError(loc, msg);
  }
};

struct LOCChecker : Checker {
  Loc m_loc;

  LOCChecker(const Model &model, Loc loc, ErrorHandler *err)
      : Checker(model, err), m_loc(loc) {}

  void warning(const char *msg) { m_errHandler->reportWarning(m_loc, msg); }
  void error(const char *msg) { m_errHandler->reportError(m_loc, msg); }
};

struct SemanticChecker : LOCChecker {
  const Instruction *m_inst;
  uint32_t m_enabled_warnings;

  SemanticChecker(const Model &model, ErrorHandler *err,
                  uint32_t enabled_warnings)
      : LOCChecker(model, Loc::INVALID, err), m_inst(nullptr),
        m_enabled_warnings(enabled_warnings) {}

  void checkKernel(const Kernel &k) {
    for (auto b : k.getBlockList()) {
      for (auto i : b->getInstList()) {
        checkInstruction(*i);
      }
    }
  }

  void checkInstruction(const Instruction &i) {
    m_inst = &i;
    m_loc = i.getLoc();
    checkInstImpl(i);
    m_inst = nullptr;
    m_loc = Loc::INVALID;
  }

  void checkInstImpl(const Instruction &i) {
    if (i.getOpSpec().supportsDestination()) {
      checkDst(i, i.getDestination());
    }
    int srcs = i.getSourceCount();
    checkExecSize(i);
    if (srcs > 0) {
      checkSrc(i, 0);
    }
    if (srcs > 1) {
      checkSrc(i, 1);
    }
    if (srcs > 2) {
      checkSrc(i, 2);
    }
  }

  void checkExecSize(const Instruction &i) {
  }

  // Scalar register on dst restrictions:
  // - Opcode must be mov
  // - Source and destination datatypes must be the same and must be integers
  //   with sizes 16, 32 or 64 bits
  // - When with immediate source, Execution size must be 1
  void checkDstScalarReg(const Instruction &i, const Operand &op) {
    if (op.getDirRegName() != RegName::ARF_S)
      return;

    if (i.getOp() != Op::MOV)
      error("Opcode must be mov for scalar dst instructions");
    if (op.getType() != i.getSource(0).getType() ||
        TypeIsFloating(op.getType()) ||
        (TypeSizeInBits(op.getType()) != 16 &&
         TypeSizeInBits(op.getType()) != 32 &&
         TypeSizeInBits(op.getType()) != 64))
      error("Invalid type for scalar dst instructions");
    if (i.getSource(0).isImm() && i.getExecSize() != ExecSize::SIMD1)
      error("Invalid execution size for scalar dst instruction");
  }

  // Scalar register on src restrictions:
  // - Opcode must be mov, send/send, sendg/sendgc. Cannot be sendgx
  // - When opcode is mov the scalar (broadcast) regioning must be used
  // - When opcode is send/sendc register-gather access must be used, and
  //   Src1 must be NULL.
  // - Scalar register operand must be on Source 0
  // - Destination must not be a scalar register
  void checkSrcScalarReg(const Instruction &i, const Operand &src) {
    if (src.isImm() || src.getDirRegName() != RegName::ARF_S)
      return;

    if (src.getKind() == Operand::Kind::DIRECT) {
      if (i.getOp() != Op::MOV)
        error("Invalid Opcode for direct scalar src instructions");
      if (src.getRegion() != Region::SRC010)
        error("Invalid region for direct scalar src instructions");
    } else if (src.getKind() == Operand::Kind::INDIRECT) {
      if (!i.getOpSpec().isAnySendFormat())
        error("Invalid Opcode for indirect scalar src instructions");
      else if (!i.isGatherSend() || !i.getSource(1).isNull())
        error("Invalid send format for indirect scalar src register");
    }
    if (i.getDestination().getDirRegName() == RegName::ARF_S)
      error("Dst and src cannot both be scalar");
  }

  void checkDst(const Instruction &i, const Operand &op) {
    switch (op.getKind()) {
    case Operand::Kind::DIRECT:
      // if (op.getDirRegName() == RegName::ARF_CE) {
      //    warning("register is not writable (except in SIP)");
      // }
      if ((m_enabled_warnings & IGA_WARNINGS_SCHED) &&
          !i.hasInstOpt(InstOpt::SWITCH) &&
          arfNeedsSwitch(op.getDirRegName()) && m_model.supportsHwDeps()) {
        warning("destination register ARF access requires {Switch} ThreadCtrl");
      }
      checkDstScalarReg(i, op);
      break;
    case Operand::Kind::MACRO:
      if (!i.isMacro()) {
        error("not a macro instruction");
      }
      break;
    default:
      break;
    }

    // conservatively check operands
    if (m_enabled_warnings & IGA_WARNINGS_TYPES) {
      auto ispec = i.getOpSpec();
      switch (ispec.format) {
      case OpSpec::BASIC_UNARY_REG:
      case OpSpec::BASIC_UNARY_REGIMM:
        checkOperandTypes(i);
        break;
      case OpSpec::BASIC_BINARY_REG_IMM:
      case OpSpec::BASIC_BINARY_REG_REG:
      case OpSpec::BASIC_BINARY_REG_REGIMM:
      case OpSpec::MATH_BINARY_REG_REGIMM:
        checkOperandTypes(i);
        break;
      case OpSpec::TERNARY_REGIMM_REG_REGIMM:
        checkOperandTypes(i);
        break;

      // punt on anything harder
      default:
        break;
      }
    }
  }

  void checkOperandTypes(const Instruction &i) {
    auto ispec = i.getOpSpec();
    for (size_t ti = 0;
         ti < sizeof(ispec.typeMappings) / sizeof(ispec.typeMappings[0]);
         ti++) {
      // type mappings are a zero-padded list of enum bitset pairs
      // the raw bits are stored in statically allocated structures
      // we construct them as EnumBitset<Type,uint32_t> and look
      // for a pair that matches all the source and destination
      // operand pairs
      auto tm = ispec.typeMappings[ti];
      const auto dsts = EnumBitset<Type>(tm.dsts);
      if (tm.dsts == 0) { // end of array (null dst)
        break;
      }
      if (!dsts.contains(i.getDestination().getType())) {
        // destination type mismatch, try the next destination
        continue;
      }

      const auto srcs = EnumBitset<Type>(tm.srcs);
      for (size_t s_ix = 0; s_ix < i.getSourceCount(); s_ix++) {
        if (srcs.contains(i.getSource(s_ix).getType())) {
          // we found a valid mapping
          return;
        }

        // WORKAROUNDS for busted BXML
        // TODO: fix these in the BXML
        // after they are all fixed, we can freshen BXML
        auto t = i.getSource(s_ix).getType();
        if (t == Type::UV || t == Type::V || t == Type::VF) {
          return;
        }
      }
    }
    // we ran through all the type mappings without finding a match
    // throw a fit
    warning("invalid operand type combination for instruction");
  }

  void checkSrc(const Instruction &i, int srcIx) {
    const Operand &src = i.getSource(srcIx);
    const OpSpec &instSpec = i.getOpSpec();
    if ((m_enabled_warnings & IGA_WARNINGS_REGIONS) &&
        instSpec.supportsDestination() && !instSpec.isAnySendFormat()) {
      checkRegRegioningRestrictions(i.getExecSize(), i.getDestination(), src);
    }
    Type srcType = src.getType();
    bool lblArg = src.getKind() == Operand::Kind::LABEL ||
                  src.getKind() == Operand::Kind::IMMEDIATE;
    if ((m_enabled_warnings & IGA_WARNINGS_NORMFORM) &&
        srcType != Type::INVALID &&
        instSpec.hasImplicitSrcType(srcIx, lblArg) &&
        instSpec.implicitSrcType(srcIx, lblArg) != srcType) {
      warning("src type is not binary normal form");
    }
    checkSrcScalarReg(i, src);
    checkMathSource(i, srcIx);
    checkTernarySource(i, srcIx);
  }

  void checkTernarySource(const Instruction& i, int srcIx) {
    if (i.getSourceCount() != 3)
      return;
    auto& src = i.getSource(srcIx);
    if (src.isImm() || src.getDirRegName() == RegName::GRF_R)
      return;

    // valid arf kind is acc and null
    if (src.getDirRegName() == RegName::ARF_ACC ||
        src.getDirRegName() == RegName::ARF_NULL)
      return;
    warning("Invalid ARF register file for ternary instructions");
  }

  // Math can only have grf source with mme
  void checkMathSource(const Instruction &i, int srcIx) {
    if (!i.isMacro())
      return;

    auto& src = i.getSource(srcIx);
    if (src.getKind() != Operand::Kind::MACRO)
      return;

    if (src.getDirRegName() != RegName::GRF_R)
      error("Invalid register: math must have grf sources");
  }


  void checkRegRegioningRestrictions(ExecSize es, const Operand &dst,
                                     const Operand &src) {
    // only for the form:
    //  op (...)  dst.#<H> ... src.#<V;W,H>
    if (src.getKind() != Operand::Kind::DIRECT)
      return; // TODO: check indirect someday too
    int dstTypeSz = TypeSizeInBitsWithDefault(dst.getType(), 0) / 8, // to bytes
        srcTypeSz = TypeSizeInBitsWithDefault(src.getType(), 0) / 8; // to bytes
    if (dstTypeSz == 0 || srcTypeSz == 0)
      return; // e.g. Type::INVALID or a sub-byte type
    if (dst.getRegion().getHz() == Region::Horz::HZ_INVALID)
      return; // e.g. if the dst has no region
    if (!src.getRegion().isVWH())
      return; // e.g. if the src has no region
    int srcRgnV = static_cast<int>(src.getRegion().getVt()),
        srcRgnW = static_cast<int>(src.getRegion().getWi()),
        srcRgnH = static_cast<int>(src.getRegion().getHz());
    int execSize = ExecSizeToInt(es);
    // int dstReg = dst.getDirRegRef().regNum;
    // int srcReg = src.getDirRegRef().regNum;
    int srcSubReg = src.getDirRegRef().subRegNum;
    // int dstSubReg = dst.getDirRegRef().subRegNum;

    // Restriction 1.1: Where n is the largest element size in bytes for
    // any source or destination operand type, ExecSize * n must be <= 64.
    int restrictSize = 64;
    if (m_model.platform >= Platform::XE_HPC)
      restrictSize = 128;
    if (execSize * srcTypeSz > restrictSize) {
      warning("register regioning restriction warning: "
              "ExecSize * sizeof(Type) exceeds maximum GRFs\n"
              "see Programmer's Reference Manual (Restriction 1.1)");
    }
    // Restriction 1.2:
    // When the Execution Data Type is wider than the destination data type,
    // the destination must be aligned as required by the wider execution
    // data type and specify a HorzStride equal to the ratio in sizes of
    // the two data types. For example, a mov with a D source and B destination
    // must use a 4-byte aligned destination and a Dst.HorzStride of 4.
#if 0
    // TODO: this fails on:
    // mov      (8|M0)         r1.0<1>:b     r2.0<8;8,1>:b
    // The "Execution Data Type" ends up being 2, but the dst is 0
    int execTypeSz = srcTypeSz == 1 ? 2 : srcTypeSz; // "Execution Data Type" is max(2,..)
    int dstRgnH = static_cast<int>(dst.getRegion().getHz());
    bool dstIsDirect = dst.getKind() == Operand::Kind::DIRECT;
    if (dstIsDirect && execTypeSz > dstTypeSz) {
      if (dstTypeSz * dstRgnH != execTypeSz) {
        // e.g. the following violate this
        //  mov (8) r1.0<1>:b   r2...:d
        //  mov (8) r1.0<2>:b   r2...:d
        // but this is okay (correct)
        //  mov (8) r1.0<4>:b   r2...:d
        warning(
            "register regioning restriction warning: "
            "Dst.Hz * sizeof(Dst.Type) != Execution Data Type Size (destination misaligned for type)\n"
            "see Programmer's Reference Manual (Restriction 1.2)");
      }
    }
#endif

    // Restriction 2.1: ExecSize must be greater than or equal to Width.
    if (execSize < srcRgnW) {
      //  mov (1) r1.0<1>:d   r2.0<8;8,1>:d
      warning("register regioning restriction warning: "
              "ExecSize <= Src.W (partial row)\n"
              "see Programmer's Reference Manual (Restriction 2.1)");
    }

    // Restriction 2.2: If ExecSize = Width and HorzStride != 0,
    // VertStride must be set to Width * HorzStride.
    if (execSize == srcRgnW && srcRgnH != 0 && srcRgnV != srcRgnW * srcRgnH) {
      warning("register regioning restriction warning: "
              "ExecSize == Src.W && Src.H != 0 && Src.V != Src.W * Src.H "
              "(vertical misalignment)\n"
              "see Programmer's Reference Manual (Restriction 2.2)");
    }

    // Restriction 2.3: If ExecSize = Width and HorzStride = 0,
    //   there is no restriction on VertStride.
    // The above checks this.

    // Restriction 2.4: If Width = 1, HorzStride must be 0 regardless of
    // the values of ExecSize and VertStride.
    if (execSize == 1 && srcRgnH != 0) {
      warning("register regioning restriction warning: "
              "SIMD1 requires horizontal stride of 0 (scalar region access)\n"
              "see Programmer's Reference Manual (Restriction 2.4)");
    }

    // Restriction 2.5: If ExecSize = Width = 1, both VertStride and
    // HorzStride must be 0.
    if (execSize == 1 && srcRgnW == 1 && (srcRgnV != 0 || srcRgnH != 0)) {
      warning("register regioning restriction warning: "
              "SIMD1 requires vertical and horiztonal to be 0 (scalar region "
              "access)\n"
              "see Programmer's Reference Manual (Restriction 2.5)");
    }

    // Restriction 2.6: If VertStride = HorzStride = 0, Width must be 1
    // regardless of the value of ExecSize.
    if (srcRgnV == 0 && srcRgnH == 0 && srcRgnW != 1) {
      warning(
          "register regioning restriction warning: "
          "If vertical stride and horizontal stride are 0, width must be 1.\n"
          "see Programmer's Reference Manual (Restriction 2.6)");
    }

    // Restriction 2.7: Dst.HorzStride must not be 0.
    // This will be checked higher up since this method gets called on
    // all source operands (and would duplicate error reports)

    // Restriction 2.8: VertStride must be used to cross GRF register
    // boundaries. This rule implies that elements within a 'Width' cannot cross
    // GRF boundaries.
    //
    // Examples:
    //    LEGAL:     mov (16) r3:ub    r11.1<32;16,2>:ub
    //                e.g. the bytes accessed are
    //                  |.0.1.2.3.4.5.6.7.8.9.A.B.C.D.E.F|
    //    LEGAL:     mov  (4) r3:d     r11.1<8;4,2>:d
    //                e.g. the bytes accessed are
    //                  |....0000....1111....2222....3333|
    //
    //    ILLEGAL:   mov (8)  r3:d     r2.1<8;8,1>:d
    //                  |....0000111122223333444455556666|7777
    //                                                   ^ bad
    if (src.getDirRegName() == RegName::GRF_R) {
      // int slotsInReg = 32/srcTypeSz/srcRgnH;
      if (srcSubReg + (srcRgnW - 1) * srcRgnH >= restrictSize / 2 / srcTypeSz) {
        // the last element is >= the maximum number of elements of
        // this type size
        warning("register regioning restriction warning: "
                "Vertical stride must be used to cross GRF boundaries.\n"
                "see Programmer's Reference Manual (Restriction 2.8)");
      }
    }
  }
};

void iga::CheckSemantics(const Kernel &k, ErrorHandler &err,
                         uint32_t enabled_warnings) {
  SemanticChecker checker(k.getModel(), &err, enabled_warnings);
  checker.checkKernel(k);
}

// checks basic IR structure (very incomplete)
struct SanityChecker {
  ErrorHandler *m_errHandler;
  const Instruction *m_inst;

  SanityChecker() : m_errHandler(nullptr), m_inst(nullptr) {}

  void checkKernel(const Kernel &k) {
    for (auto b : k.getBlockList()) {
      for (auto i : b->getInstList()) {
        checkInstruction(*i);
      }
    }
  }

  void checkInstruction(const Instruction &i) {
    m_inst = &i;
    if (i.getOpSpec().supportsDestination()) {
      checkDst(i.getDestination());
    } else if (i.getDestination().getKind() != Operand::Kind::INVALID) {
      IGA_ASSERT_FALSE("unsupported destination should be .kind=INVALID");
    }

    checkExecSize(i);
    int srcs = i.getSourceCount();
    if (srcs > 0) {
      checkSrc(i, 0);
    }
    if (srcs > 1) {
      checkSrc(i, 1);
    }
    if (srcs > 2) {
      checkSrc(i, 2);
    }
    m_inst = nullptr;
  }

  void checkDst(const Operand &op) {
    switch (op.getKind()) {
    case Operand::Kind::DIRECT:
      IGA_ASSERT(op.getDirRegName() != RegName::INVALID, "invalid register");
      break;
    case Operand::Kind::INDIRECT:
      break;
    case Operand::Kind::MACRO:
      IGA_ASSERT(m_inst->isMacro(), "instruction is not macro");
      IGA_ASSERT(op.getMathMacroExt() != MathMacroExt::INVALID,
                 "invalid accumulator for macro");
      break;
    default:
      IGA_ASSERT_FALSE("wrong kind for destination");
    }
  }

  void checkExecSize(const Instruction &i) {
  }

  void checkSrc(const Instruction &inst, int srcIx) {
    const Operand &op = inst.getSource(srcIx);
    switch (op.getKind()) {
    case Operand::Kind::DIRECT:
      IGA_ASSERT(op.getDirRegName() != RegName::INVALID, "invalid register");
      break;
    case Operand::Kind::INDIRECT:
      break;
    case Operand::Kind::MACRO:
      IGA_ASSERT(m_inst->isMacro(), "instruction is not macro");
      IGA_ASSERT(op.getMathMacroExt() != MathMacroExt::INVALID,
                 "invalid accumulator for macro");
      break;
    case Operand::Kind::IMMEDIATE:
    case Operand::Kind::LABEL:
      break;
    default: {
      std::stringstream ss;
      auto loc = inst.getLoc();
      ss << loc.line << ":" << loc.col << ":[" << loc.offset
         << "]: wrong kind for src" << srcIx;
      IGA_ASSERT_FALSE(ss.str().c_str());
    }
    }
  }
};

void iga::SanityCheckIR(const Kernel &k) {
  SanityChecker checker;
  checker.checkKernel(k);
}

void iga::SanityCheckIR(const Instruction &i) {
  SanityChecker checker;
  checker.checkInstruction(i);
}
