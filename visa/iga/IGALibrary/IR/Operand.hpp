/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _IGA_OPERAND_HPP_
#define _IGA_OPERAND_HPP_

#include "ImmVal.hpp"
#include "Types.hpp"

namespace iga {
class Block;

class Operand {
public:
  enum class Kind {
    INVALID,   // an invalid or uninitialized operand
    DIRECT,    // direct register reference
    MACRO,     // madm or math.invm or math.rsqrtm
    INDIRECT,  // register-indriect access
    IMMEDIATE, // immediate value
    LABEL,     // block target (can be numeric label/m_immVal)
  };

  Operand()
    : m_regOpSrcMod(SrcModifier::NONE) {}

  // direct destination constructor (for constants etc)
  Operand(DstModifier dstMod, RegName rType, const RegRef &reg,
          const Region::Horz &rgnHz, Type type) {
    setDirectDestination(dstMod, rType, reg, rgnHz, type);
  }

  // direct source constructor (for constants etc)
  Operand(SrcModifier srcMod, RegName rType, const RegRef &reg,
          const Region &rgn, Type type) {
    setDirectSource(srcMod, rType, reg, rgn, type);
  }

  // generally you want a reference to an operand, not a copy
  // this prevents:
  //    auto o = i->getSource(...); // copies all state
  // you usually want
  //    const auto &o = ...
  // but in rare cases maybe you want a copy
  //    Operand copy(i->getSource(...));
  explicit Operand(const Operand &) = default;

  // describes if the operand is direct (Operand::Kind::DIRECT),
  // indirect (Operand::Kind::INDIRECT) or immediate
  // (Operand::Kind::IMMEDIATE)
  Kind getKind() const { return m_kind; }

  // both labels and true immediates are considered immediates
  // for encoding and decoding sake
  bool isImm() const {
    switch (getKind()) {
    case Operand::Kind::IMMEDIATE:
    case Operand::Kind::LABEL:
      return true;
    default:
      return false;
    }
  }

  ///////////////////////////////////////////////////////////////////////////
  // derived accessors

  // returns true if a direct register reference to the null register
  bool isNull() const {
    return getKind() == Kind::DIRECT && getDirRegName() == RegName::ARF_NULL;
  }

  ///////////////////////////////////////////////////////////////////////////
  // other accessors
  DstModifier getDstModifier() const { return m_regOpDstMod; }
  SrcModifier getSrcModifier() const { return m_regOpSrcMod; }

  // Applies to Operand::Kind::DIRECT
  RegName getDirRegName() const { return m_regOpName; }
  const RegRef &getDirRegRef() const { return m_regOpReg; }

  // Applies to Operand::Kind::INDIRECT only
  const RegRef &getIndAddrReg() const { return m_regOpReg; }
  int16_t getIndImmAddr() const { return m_regOpIndOff; }

  // Applies to Operand::Kind::DIRECT and Operand::Kind::INDIRECT
  Region getRegion() const { return m_regOpRgn; }
  MathMacroExt getMathMacroExt() const { return m_regMathMacro; }
  // Defined if the value is immediate
  const ImmVal getImmediateValue() const { return m_immValue; }
  // if this operand corresponds to a label, this is the target block
  // nullptr if we are using numeric labels
  const Block *getTargetBlock() const { return m_lblBlock; }

  // the operand type (as in :f, :d, ...)
  Type getType() const { return m_type; }

  // re-initializes this operand as an direct destination register operand
  void setDirectDestination(DstModifier dstMod, RegName rName,
                            const RegRef &reg, const Region::Horz &rgnHz,
                            Type type);
  // re-initializes this operand as a destination register operand using
  // a math macro register access
  void setMacroDestination(DstModifier dstMod, RegName rName, const RegRef &reg,
                           MathMacroExt acc, Type type) {
    setMacroDestination(dstMod, rName, reg, acc, Region::Horz::HZ_1, type);
  }
  void setMacroDestination(DstModifier dstMod, RegName rName, const RegRef &reg,
                           MathMacroExt acc, Region::Horz rgnHz, Type type);
  // re-initializes this operand as an indirect destination register operand
  void setIndirectDestination(DstModifier dstMod, const RegRef &reg,
                              int16_t immediateOffset,
                              const Region::Horz &rgnHz, Type type);

  // re-initializes this operand as an immeidate value with a given type
  void setImmediateSource(const ImmVal &val, Type type);
  // re-initializes this operand as a direct source register
  void setDirectSource(SrcModifier srcMod, RegName rName, const RegRef &reg,
                       const Region &rgn, Type type);
  // re-initializes this operand as a source register operand using
  // an math macro register access
  void setMacroSource(SrcModifier srcMod, RegName rName, const RegRef &reg,
                      Region /* rgn */, MathMacroExt mme, Type type) {
    setMacroSource(srcMod, rName, reg, mme, Region::SRC110, type);
  }
  void setMacroSource(SrcModifier srcMod, RegName rName, const RegRef &reg,
                      MathMacroExt mme, Region rgn, Type type);

  // re-initializes this operand as an indirect register operand
  void setIndirectSource(SrcModifier srcMod, RegName regName,
                         const RegRef &reg, int16_t addrImmOff,
                         const Region &rgn, Type type);
  // re-initializes this operand as an immediate branch target
  void setLabelSource(Block *blk, Type type);
  void setLabelSource(int32_t jipOrUip, Type type);
  // set sthe operand region
  void setRegion(const Region &rgn) { m_regOpRgn = rgn; }
  // sets the operand type
  void setType(Type type) { m_type = type; }
  // set the operand reg RegRef directly
  void setRegRef(const RegRef &regRef) {
    m_regOpReg.regNum = regRef.regNum;
    m_regOpReg.subRegNum = regRef.subRegNum;
  }

private:
  Operand::Kind m_kind = Kind::INVALID;

  // direct/indirect register information
  union {
    SrcModifier m_regOpSrcMod;
    DstModifier m_regOpDstMod;
  };
  RegName m_regOpName = RegName::INVALID; // exists for ind. as well
  MathMacroExt m_regMathMacro = MathMacroExt::INVALID;
  Region m_regOpRgn = Region::INVALID;

  // direct/indirect register
  RegRef m_regOpReg;

  // indirect register offset
  int16_t m_regOpIndOff = 0;

  // Imm field information
  ImmVal m_immValue;
  // for resolved labels
  // the literal value (for immediate data)
  // also set before labels are resolved (label is .s32)
  // for numeric labels the value is normalized as bytes from
  // the pre-increment PC of the instruction.  Hence,
  //  * Pre-BDW branches which use units of QWORDS in the
  //    encoding will be converted to bytes.
  //  * JMPI uses the pos-increment PC; we normalize that here
  //    as well to a pre-increment value.
  Block *m_lblBlock = nullptr;

  // the operand type (e.g. :d, :f, etc...)
  Type m_type = Type::INVALID;

public:
  // useful constants (reusable operands to streamline codegen)
  static const Operand DST_REG_IP_D; // brd/brc use this
  static const Operand SRC_REG_IP_D;
  static const Operand DST_REG_IP_UD; // jmpi Dst and Src0
  static const Operand SRC_REG_IP_UD;
  static const Operand DST_REG_NULL_UD; // e.g. while.Dst
  static const Operand SRC_REG_NULL_UD;
  static const Operand SRC_REG_NULL_UB;
}; // class Operand
} // namespace iga
#endif // _IGA_OPERAND_HPP_
