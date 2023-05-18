/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_REGISTER_H
#define G4_REGISTER_H

#include "Assertions.h"
#include "G4_Declare.h"
#include "G4_Opcode.h"

#include <iostream>

namespace vISA {

// Forward declarations.
class IR_Builder;
// TODO: We should try to avoid this circular dependence where a register class
// refers to the operand class.
class G4_Operand;
// Forward declarations for each concrete register class. We need them here
// because the base class has APIs that perform downcasts to the concrete type.
class G4_RegVar;
class G4_Greg;
class G4_Areg;

class G4_VarBase {
public:
  enum G4_VarKind {
    VK_regVar,  // register allocation candidate
    VK_phyGReg, // physical general register
    VK_phyAReg  // physical architecture register
  };

protected:
  G4_VarKind Kind;
  explicit G4_VarBase(G4_VarKind K) : Kind(K) {}

public:
  G4_VarKind getKind() const { return Kind; }

  bool isRegVar() const { return getKind() == VK_regVar; }
  bool isPhyReg() const { return !isRegVar(); }
  bool isPhyGreg() const { return getKind() == VK_phyGReg; }
  bool isPhyAreg() const { return getKind() == VK_phyAReg; }

  G4_RegVar *asRegVar() const {
    vISA_ASSERT(isRegVar(), ERROR_UNKNOWN);
    return (G4_RegVar *)this;
  }
  G4_Greg *asGreg() const {
    vISA_ASSERT(isPhyGreg(), ERROR_UNKNOWN);
    return (G4_Greg *)this;
  }
  G4_Areg *asAreg() const {
    vISA_ASSERT(isPhyAreg(), ERROR_UNKNOWN);
    return (G4_Areg *)this;
  }

  bool isAreg() const;
  bool isGreg() const;
  bool isNullReg() const;
  bool isIpReg() const;
  bool isFlag() const;
  bool isNReg() const;
  bool isAccReg() const;
  bool isMaskReg() const;
  bool isMsReg() const;
  bool isSrReg() const;
  bool isCrReg() const;
  bool isDbgReg() const;
  bool isTmReg() const;
  bool isTDRReg() const;
  bool isSpReg() const;
  bool isA0() const;
  bool isAddress() const;
  bool isRegAllocPartaker() const;

  bool noScoreBoard() const;
  G4_Areg *getAreg() const;

  virtual unsigned short ExRegNum(bool &valid) {
    valid = false;
    return UNDEFINED_SHORT;
  }

  virtual unsigned short ExSubRegNum(bool &valid) {
    valid = false;
    return UNDEFINED_SHORT;
  }

  virtual void emit(std::ostream &output) = 0;
};

//
// General Register File
//
class G4_Greg final : public G4_VarBase {
  const unsigned RegNum;

public:
  explicit G4_Greg(unsigned num) : G4_VarBase(VK_phyGReg), RegNum(num) {}
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  G4_RegFileKind getRegFile() const { return G4_GRF; }

  unsigned getRegNum() const { return RegNum; }

  unsigned short ExRegNum(bool &valid) override {
    valid = true;
    return (unsigned short)getRegNum();
  }

  void emit(std::ostream &output) override;
};

//
// Architecture Register File
//
class G4_Areg final : public G4_VarBase {
  const G4_ArchRegKind ArchRegType;

public:
  explicit G4_Areg(G4_ArchRegKind k) : G4_VarBase(VK_phyAReg), ArchRegType(k) {}
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  G4_ArchRegKind getArchRegType() const { return ArchRegType; }

  void emit(std::ostream &output) override;

  bool isNullReg() const { return getArchRegType() == AREG_NULL; }
  bool isFlag() const {
    switch (getArchRegType()) {
    case AREG_F0:
    case AREG_F1:
    case AREG_F2:
    case AREG_F3:
      return true;
    default:
      return false;
    }
  }
  bool isIpReg() const { return getArchRegType() == AREG_IP; }
  bool isA0() const { return getArchRegType() == AREG_A0; }
  bool isNReg() const {
    return getArchRegType() == AREG_N0 || getArchRegType() == AREG_N1;
  }
  bool isAcc0Reg() const { return getArchRegType() == AREG_ACC0; }
  bool isAccReg() const {
    return getArchRegType() == AREG_ACC0 || getArchRegType() == AREG_ACC1;
  }
  bool isMaskReg() const { return getArchRegType() == AREG_MASK0; }
  bool isMsgReg() const { return getArchRegType() == AREG_MSG0; }
  bool isDbgReg() const { return getArchRegType() == AREG_DBG; }
  bool isSrReg() const { return getArchRegType() == AREG_SR0; }
  bool isCrReg() const { return getArchRegType() == AREG_CR0; }
  bool isTmReg() const { return getArchRegType() == AREG_TM0; }
  bool isTDRReg() const { return getArchRegType() == AREG_TDR0; }
  bool isSpReg() const { return getArchRegType() == AREG_SP; }

  unsigned short ExRegNum(bool &valid) override {
    unsigned short rNum = UNDEFINED_SHORT;
    valid = true;

    if (isFlag()) {
      return getFlagNum();
    }

    switch (getArchRegType()) {
    case AREG_NULL:
    case AREG_A0:
    case AREG_ACC0:
    case AREG_MASK0:
    case AREG_MSG0:
    case AREG_DBG:
    case AREG_SR0:
    case AREG_CR0:
    case AREG_TM0:
    case AREG_N0:
    case AREG_IP:
    case AREG_TDR0:
    case AREG_SP:
      rNum = 0;
      break;
    case AREG_ACC1:
    case AREG_N1:
      rNum = 1;
      break;
    default:
      valid = false;
    }
    return rNum;
  }

  int getFlagNum() const {
    switch (getArchRegType()) {
    case AREG_F0:
      return 0;
    case AREG_F1:
      return 1;
    case AREG_F2:
      return 2;
    case AREG_F3:
      return 3;
    default:
      vISA_ASSERT_UNREACHABLE("should only be called on flag ARF");
      return -1;
    }
  }
};

// Since the sub regs of address reg a0 can be allocated individually,
// we use subRegOff to indicate the sub registers
// Address operands with the same base variable(i.e. A0(0), A0(1), A0(2)...)
// have the same subRegOff as base variable "A0".
struct AssignedReg {
  vISA::G4_VarBase *phyReg = nullptr;
  unsigned subRegOff = 0;
};

class G4_RegVar : public G4_VarBase {
  friend class G4_Declare;

public:
  enum RegVarType {
    Default = 0,
    GRFSpillTmp = 1,
    AddrSpillLoc = 2,
    Transient = 3,
    Coalesced = 4,
  };

private:
  // G4_RegVar now has an enum that holds its type. Each subclass of G4_RegVar
  // will initialize the type according to its specific class. For eg,
  // Spill/Fill transient ranges will set this type to RegVarType::Transient.
  unsigned id; // id for register allocation
  const RegVarType type;
  G4_Declare *const decl;   // corresponding declare
  AssignedReg reg;          // assigned physical register; set after reg alloc
  unsigned disp;            // displacement offset in spill memory
  G4_SubReg_Align subAlign; // To support sub register alignment
  bool evenAlignment =
      false; // Align this regVar to even GRFs regardless of its size

public:
  // To support sub register alignment
  G4_RegVar(G4_Declare *d, RegVarType t)
      : G4_VarBase(VK_regVar), id(UNDEFINED_VAL), type(t), decl(d),
        disp(UINT_MAX), subAlign(Any) {}
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  unsigned getId() const { return id; }
  void setId(unsigned i) { id = i; }
  const char *getName() const { return decl->getName(); }
  const G4_Declare *getDeclare() const { return decl; }
  G4_Declare *getDeclare() { return decl; }
  bool isPhyRegAssigned() const { return reg.phyReg != NULL; }
  bool isFlag() const { return decl->getRegFile() == G4_FLAG; }
  bool isAreg() const { return (reg.phyReg != NULL) && (reg.phyReg->isAreg()); }
  bool isA0() const { return (reg.phyReg != NULL) && (reg.phyReg->isA0()); }
  bool isCrReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isCrReg());
  }
  bool isDbgReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isDbgReg());
  }
  bool isGreg() const { return (reg.phyReg != NULL) && (reg.phyReg->isGreg()); }
  bool isNReg() const { return (reg.phyReg != NULL) && (reg.phyReg->isNReg()); }
  bool isNullReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isNullReg());
  }
  bool isSrReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isSrReg());
  }
  bool isTDRReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isTDRReg());
  }
  bool isTmReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isTmReg());
  }
  bool isAccReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isAccReg());
  }
  bool isIpReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isIpReg());
  }
  bool isMaskReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isMaskReg());
  }
  bool isMsReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isMsReg());
  }
  bool isSpReg() const {
    return (reg.phyReg != NULL) && (reg.phyReg->isSpReg());
  }

  bool isRegAllocPartaker() const { return id != UNDEFINED_VAL; }
  unsigned getRegAllocPartaker() const { return id; }
  bool isAddress() const { return decl->getRegFile() == G4_ADDRESS; }
  const G4_VarBase *getPhyReg() const { return reg.phyReg; }
  G4_VarBase *getPhyReg() { return reg.phyReg; }
  unsigned getByteAddr(const IR_Builder &builder) const;
  unsigned getPhyRegOff() const { return reg.subRegOff; }
  void setPhyReg(G4_VarBase *pr, unsigned off) {
    vISA_ASSERT(pr == NULL || pr->isPhyReg(), ERROR_UNKNOWN);
    reg.phyReg = pr;
    reg.subRegOff = off;
  }
  void resetPhyReg() {
    reg.phyReg = NULL;
    reg.subRegOff = 0;
  }
  bool isSpilled() const { return decl->isSpilled(); }
  void setDisp(unsigned offset) { disp = offset; }
  unsigned getDisp() const { return disp; }
  bool isAliased() const { return decl->getAliasDeclare() != NULL; }
  unsigned getLocId() const;

  bool isRegVarTransient() const { return type == RegVarType::Transient; }
  bool isRegVarSpill() const;
  bool isRegVarFill() const;

  bool isRegVarTmp() const { return type == RegVarType::GRFSpillTmp; }
  bool isRegVarAddrSpillLoc() const { return type == RegVarType::AddrSpillLoc; }

  bool isRegVarCoalesced() const { return type == RegVarType::Coalesced; }

  G4_RegVar *getBaseRegVar();

  G4_RegVar *getNonTransientBaseRegVar();

  void emit(std::ostream &output) override;

  unsigned short ExRegNum(bool &valid) override {
    return reg.phyReg->ExRegNum(valid);
  }

  unsigned short ExSubRegNum(bool &valid) override {
    valid = true;
    return (unsigned short)reg.subRegOff;
  }

protected:
  bool isEvenAlign() const { return evenAlignment; }
  void setEvenAlign() { evenAlignment = true; }
  G4_SubReg_Align getSubRegAlignment() const { return subAlign; }

  void setSubRegAlignment(G4_SubReg_Align subAlg);
};

class G4_RegVarTransient : public G4_RegVar {
public:
  enum TransientType {
    Spill = 0,
    Fill = 1,
  };

private:
  G4_RegVar *baseRegVar;
  G4_Operand *repRegion;
  G4_ExecSize execSize;
  TransientType type;

public:
  G4_RegVarTransient(G4_Declare *d, G4_RegVar *base, G4_Operand *reprRegion,
                     G4_ExecSize eSize, TransientType t)
      : G4_RegVar(d, Transient), baseRegVar(base), repRegion(reprRegion),
        execSize(eSize), type(t) {}

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  G4_RegVar *getBaseRegVar() { return baseRegVar; }

  G4_Operand *getRepRegion() const { return repRegion; }
  G4_RegVar *getNonTransientBaseRegVar();
  G4_ExecSize getExecSize() const { return execSize; }

  bool isRegVarSpill() const { return type == TransientType::Spill; }
  bool isRegVarFill() const { return type == TransientType::Fill; }
};

class G4_RegVarTmp : public G4_RegVar {
  G4_RegVar *const baseRegVar;

public:
  G4_RegVarTmp(G4_Declare *d, G4_RegVar *base)
      : G4_RegVar(d, RegVarType::GRFSpillTmp), baseRegVar(base) {
    vASSERT(base->isRegVarTransient() == false);
    vASSERT(base == base->getBaseRegVar());
  }
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }

  G4_RegVar *getBaseRegVar() { return baseRegVar; }
  G4_RegVar *getNonTransientBaseRegVar() { return baseRegVar; }
};

class G4_RegVarAddrSpillLoc : public G4_RegVar {
  unsigned loc_id;

public:
  G4_RegVarAddrSpillLoc(G4_Declare *d, int &loc_count)
      : G4_RegVar(d, RegVarType::AddrSpillLoc) {
    if (d->getAliasDeclare() != NULL) {
      unsigned elemSize = d->getRegVar()->getDeclare()->getElemSize();
      loc_id = d->getRegVar()->getLocId() + d->getAliasOffset() / elemSize;
    } else {
      loc_id = (++loc_count) * getNumAddrRegisters();
    }
  }
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  unsigned getLocId() const { return loc_id; }
};

// Inlined members of G4_VarBase.
// TODO: Move them into the class.
inline bool G4_VarBase::isAreg() const {
  if (isRegVar())
    return asRegVar()->isAreg();
  return isPhyAreg();
}
inline bool G4_VarBase::isGreg() const {
  if (isRegVar())
    return asRegVar()->isGreg();
  return isPhyGreg();
}
inline bool G4_VarBase::isNullReg() const {
  if (isRegVar())
    return asRegVar()->isNullReg();
  return isPhyAreg() && asAreg()->isNullReg();
}
inline bool G4_VarBase::isIpReg() const {
  if (isRegVar())
    return asRegVar()->isIpReg();
  return isPhyAreg() && asAreg()->isIpReg();
}
inline bool G4_VarBase::isFlag() const {
  if (isRegVar())
    return asRegVar()->isFlag();
  return isPhyAreg() && asAreg()->isFlag();
}
inline bool G4_VarBase::isNReg() const {
  if (isRegVar())
    return asRegVar()->isNReg();
  return isPhyAreg() && asAreg()->isNReg();
}
inline bool G4_VarBase::isAccReg() const {
  if (isRegVar())
    return asRegVar()->isAccReg();
  return isPhyAreg() && asAreg()->isAccReg();
}
inline bool G4_VarBase::isMaskReg() const {
  if (isRegVar())
    return asRegVar()->isMaskReg();
  return isPhyAreg() && asAreg()->isMaskReg();
}
inline bool G4_VarBase::isMsReg() const {
  if (isRegVar())
    return asRegVar()->isMsReg();
  return isPhyAreg() && asAreg()->isMsReg();
}
inline bool G4_VarBase::isSrReg() const {
  if (isRegVar())
    return asRegVar()->isSrReg();
  return isPhyAreg() && asAreg()->isSrReg();
}
inline bool G4_VarBase::isCrReg() const {
  if (isRegVar())
    return asRegVar()->isCrReg();
  return isPhyAreg() && asAreg()->isCrReg();
}
inline bool G4_VarBase::isDbgReg() const {
  if (isRegVar())
    return asRegVar()->isDbgReg();
  return isPhyAreg() && asAreg()->isDbgReg();
}
inline bool G4_VarBase::isTmReg() const {
  if (isRegVar())
    return asRegVar()->isTmReg();
  return isPhyAreg() && asAreg()->isTmReg();
}
inline bool G4_VarBase::isTDRReg() const {
  if (isRegVar())
    return asRegVar()->isTDRReg();
  return isPhyAreg() && asAreg()->isTDRReg();
}
inline bool G4_VarBase::isA0() const {
  if (isRegVar())
    return asRegVar()->isA0();
  return isPhyAreg() && asAreg()->isA0();
}
inline bool G4_VarBase::isAddress() const {
  if (isRegVar())
    return asRegVar()->isAddress();
  return isPhyAreg() && asAreg()->isA0();
}

inline bool G4_VarBase::isSpReg() const {
  if (isRegVar()) {
    return asRegVar()->isSpReg();
  }
  return isPhyAreg() && asAreg()->isSpReg();
}

/// return the physical AReg associated with this VarBase objkect.
/// This is either the VarBase itself, or if this is a RegVar the phyAReg it is
/// allocated to. return null if VarBase is not a AReg or it's a RegVar that has
/// not been assigned a AReg yet
inline G4_Areg *G4_VarBase::getAreg() const {
  G4_Areg *areg = nullptr;
  if (isRegVar()) {
    G4_VarBase *phyReg = asRegVar()->getPhyReg();
    if (phyReg && phyReg->isAreg()) {
      areg = phyReg->asAreg();
    }
  } else if (isPhyAreg()) {
    areg = asAreg();
  }
  return areg;
}

// CR/SR/SP/TM0/IP do not have scoreboard
inline bool G4_VarBase::noScoreBoard() const {
  G4_Areg *areg = getAreg();

  if (areg != nullptr) {
    return areg->isCrReg() || areg->isSrReg() || areg->isSpReg() ||
           areg->isTmReg() || areg->isIpReg() || areg->isDbgReg();
  } else {
    return false;
  }
}

inline bool G4_VarBase::isRegAllocPartaker() const {
  return isRegVar() && asRegVar()->isRegAllocPartaker();
}

// Inlined G4_RegVar methods.
inline unsigned G4_RegVar::getLocId() const {
  vISA_ASSERT(type == RegVarType::AddrSpillLoc,
              "Unexpected type in getLocId()");

  G4_RegVarAddrSpillLoc *addrSpillLoc =
      static_cast<G4_RegVarAddrSpillLoc *>(const_cast<G4_RegVar *>(this));
  return addrSpillLoc->getLocId();
}

inline bool G4_RegVar::isRegVarSpill() const {
  if (isRegVarTransient()) {
    G4_RegVarTransient *transientVar =
        static_cast<G4_RegVarTransient *>(const_cast<G4_RegVar *>(this));
    return transientVar->isRegVarSpill();
  }
  return false;
}

inline bool G4_RegVar::isRegVarFill() const {
  if (isRegVarTransient()) {
    G4_RegVarTransient *transientVar =
        static_cast<G4_RegVarTransient *>(const_cast<G4_RegVar *>(this));
    return transientVar->isRegVarFill();
  }
  return false;
}

inline G4_RegVar *G4_RegVar::getBaseRegVar() {
  if (type == RegVarType::Transient) {
    G4_RegVarTransient *transient = static_cast<G4_RegVarTransient *>(this);
    return transient->getBaseRegVar();
  } else if (type == RegVarType::GRFSpillTmp) {
    G4_RegVarTmp *tmp = static_cast<G4_RegVarTmp *>(this);
    return tmp->getBaseRegVar();
  }

  // For Default, AddrSpillLoc
  return this;
}

inline G4_RegVar *G4_RegVar::getNonTransientBaseRegVar() {
  if (type == RegVarType::Transient) {
    G4_RegVarTransient *transient = static_cast<G4_RegVarTransient *>(this);
    return transient->getNonTransientBaseRegVar();
  } else if (type == RegVarType::GRFSpillTmp) {
    G4_RegVarTmp *tmp = static_cast<G4_RegVarTmp *>(this);
    return tmp->getNonTransientBaseRegVar();
  }

  return this;
}
} // namespace vISA

#endif // G4_REGISTER_H
