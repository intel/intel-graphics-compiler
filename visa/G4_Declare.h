/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef G4_DECLARE_H
#define G4_DECLARE_H

#include "Assertions.h"
#include "G4_Opcode.h"

#include <iostream>
#include <vector>

namespace vISA {

// forward declarations.
class IR_Builder;
class G4_RegVar;

class G4_Declare {
  class ElemInfo {
    G4_Type type;         // element type
    unsigned numElements; // total elements
    unsigned short numRows;
    unsigned short numElemsPerRow; // number of elements per row
  public:
    ElemInfo() = delete;
    ElemInfo(G4_Type t, unsigned n, uint8_t GRFByteSize) : type(t) {
      reset(n, GRFByteSize);
    }
    G4_Type getType() const { return type; }
    unsigned short getElemSize() const { return TypeSize(type); }
    unsigned short getNumElems() const { return numElements; }
    unsigned short getNumRows() const { return numRows; }
    unsigned short getNumElemsPerRow() const { return numElemsPerRow; }
    unsigned getByteSize() const { return numElements * getElemSize(); }
    void reset(unsigned numElems, uint8_t GRFByteSize);
  };

  const char *name;
  ElemInfo elemInfo;

  // Each declare has a unique register variable (and vice versa), primarily
  // used by register allocation.
  G4_RegVar *regVar;

  // The declare this variable is aliased to. Multiple levels of aliasing is
  // supported but expected to be rare.
  G4_Declare *AliasDCL;
  // Byte offset into the alised variable.
  uint16_t AliasOffset;

  G4_RegFileKind regFile;
  // This is fixed for each platform, but we need this in G4_Declare class to
  // compute the number of GRFs for this declare.
  // TODO: It seems to make more sense for declare to only store the number of
  // elements and have the caller convert it to number of GRFs. This would
  // require large refactor though.
  const uint8_t GRFByteSize;

  // TODO: This is only used by VarSplit pass in RA and should be moved there.
  unsigned startID;

  // TODO: Many of the flags are RA/spill only and do not belong here.
  uint16_t spillFlag : 1; // Indicate this declare gets spill reg
  uint16_t addressed : 1; // whether this declare is address-taken

  uint16_t
      doNotSpill : 1; // indicates that this declare should never be spilled

  uint16_t builtin : 1;        // indicate if this variable is a builtin
  uint16_t liveIn : 1;         // indicate if this variable has "Input" or
                               // "Input_Output" attribute
  uint16_t liveOut : 1;        // indicate if this variable has "Output" or
                               // "Input_Output" attribute
  uint16_t payloadLiveOut : 1; // indicate if this variable has "Output"
                               // attribute for the payload section

  // This is an optimization *hint* to indicate if optimizer should skip
  // widening this variable or not (e.g. byte to word).
  uint16_t noWidening : 1;

  uint16_t capableOfReuse : 1;
  uint16_t isSplittedDcl : 1;
  uint16_t isPartialDcl : 1;
  uint16_t refInSend : 1;
  // indicate if this dcl is created from preDefinedVars.
  uint16_t PreDefinedVar : 1;
  uint16_t addrSpillFill : 1;
  uint16_t forceSpilled : 1;
  uint16_t exclusiveLoad : 1;
  uint16_t isCmpUseOnly : 1;
  // indicate if the declare is local referenced only
  // Especially for the variable with pseodu_kill,
  // while will be removed in removeLifetimeOps pass.
  uint16_t isBBLocal : 1;

  unsigned declId; // global decl id for this builder

  // Flag declares are repreented with Type_UW. So we need an additional field
  // to distinguish 8- v. 16-bit flags.
  uint8_t numFlagElements;

  // If set to nonzero, indicates the declare is only used by subroutine
  // "scopeID". It is used to prevent a subroutine-local declare from escaping
  // its subroutine when doing liveness analysis.
  unsigned scopeID;

  // TODO: they should be moved out of G4_Declare and stored as maps in RA/spill
  G4_Declare *spillDCL; // if an addr/flag var is spilled, SpillDCL is the
                        // location (GRF) holding spilled value

public:
  G4_Declare(const IR_Builder &builder, const char *n, G4_RegFileKind k,
             uint32_t numElems, G4_Type ty, std::vector<G4_Declare *> &dcllist);

  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  uint8_t getGRFByteSize() const { return GRFByteSize; }

  unsigned int getGRFOffsetFromR0() const;

  void setBuiltin() { builtin = true; }
  bool isBuiltin() const { return builtin; }
  void setLiveIn() { liveIn = true; }
  bool isLiveIn() const { return liveIn; }
  void setLiveOut() { liveOut = true; }
  void resetLiveOut() { liveOut = false; }
  void setPayloadLiveOut() { payloadLiveOut = true; }

  void setExclusiveLoad() { exclusiveLoad = true; }
  bool isExclusiveLoad() const { return exclusiveLoad; }

  void setDoNotWiden() { noWidening = true; }
  bool doNotWiden() const { return noWidening; }

  unsigned getScopeID() const { return scopeID; }
  void updateScopeID(unsigned id) {
    if (!isInput() && (scopeID < id))
      scopeID = id;
  }

  void setRegVar(G4_RegVar *rv) {
    vISA_ASSERT(regVar == NULL, ERROR_UNKNOWN);
    regVar = rv;
  }

  // caller manages the name str
  void setName(const char *newName) { name = newName; }

  unsigned int getByteSize() const { return elemInfo.getByteSize(); }

  unsigned int getWordSize() const { return (getByteSize() + 1) / 2; }

  void resizeNumRows(unsigned int numrows);

  // declare this to be aliased to dcl+offset
  // This is an error if dcl+offset is not aligned to the type of this dcl
  // note that offset is in terms of bytes
  void setAliasDeclare(G4_Declare *dcl, unsigned int offset) {
    AliasDCL = dcl;
    AliasOffset = offset;
  }

  void resetSpillFlag() {
    // This function is invoked from rematerialization pass.
    if (getAliasDeclare())
      getAliasDeclare()->resetSpillFlag();
    spillFlag = false;
  }
  void setSpillFlag() {
    if (getAliasDeclare()) {
      // Iterate to top level dcl to set spill flag
      getAliasDeclare()->setSpillFlag();
    }
    spillFlag = true;
  }
  bool isSpilled() const {
    if (getAliasDeclare()) {
      return getAliasDeclare()->isSpilled();
    }

    // Following executed only if G4_Declare doesnt have an alias
    return spillFlag;
  }

  void setForceSpilled() {
    if (auto *dcl = getAliasDeclare()) {
      // Iterate to top level dcl to set spill flag
      dcl->setForceSpilled();
    }
    forceSpilled = true;
  }

  bool isForceSpilled() const {
    if (auto *dcl = getAliasDeclare()) {
      return dcl->isForceSpilled();
    }

    // Following executed only if G4_Declare doesnt have an alias
    return forceSpilled;
  }

  bool isEvenAlign() const;
  G4_SubReg_Align getSubRegAlign() const;
  void setEvenAlign();
  void setSubRegAlign(G4_SubReg_Align subAl);

  void copyAlign(G4_Declare *dcl);

  unsigned getByteAlignment() const {
    // we only consider subalign here
    unsigned byteAlign = getSubRegAlign() * TypeSize(Type_UW);
    return byteAlign < getElemSize() ? getElemSize() : byteAlign;
  }

  void setRegFile(G4_RegFileKind rfile) { regFile = rfile; }

  bool useGRF() const { return (regFile & (G4_GRF | G4_INPUT)) != 0; }
  bool isInput() const { return liveIn || ((regFile & G4_INPUT) != 0); }
  bool isOutput() const { return liveOut; }
  bool isPayloadLiveOut() const { return payloadLiveOut; }

  unsigned getAliasOffset() const { return AliasOffset; }
  const G4_Declare *getAliasDeclare() const { return AliasDCL; }
  G4_Declare *getAliasDeclare() { return AliasDCL; }
  const G4_Declare *getRootDeclare() const {
    const G4_Declare *rootDcl = this;
    while (rootDcl->getAliasDeclare() != NULL) {
      rootDcl = rootDcl->getAliasDeclare();
    }
    return rootDcl;
  }
  G4_Declare *getRootDeclare() {
    return const_cast<G4_Declare *>(
        ((const G4_Declare *)this)->getRootDeclare());
  }

  // Like the above, but also return the alias offset in bytes.
  const G4_Declare *getRootDeclare(uint32_t &offset) const {
    const G4_Declare *rootDcl = this;
    offset = 0;
    while (rootDcl->getAliasDeclare() != NULL) {
      offset += rootDcl->getAliasOffset();
      rootDcl = rootDcl->getAliasDeclare();
    }
    return rootDcl;
  }
  G4_Declare *getRootDeclare(uint32_t &offset) {
    return const_cast<G4_Declare *>(
        ((const G4_Declare *)this)->getRootDeclare(offset));
  }

  const char *getName() const { return name; }
  std::string getNonAliasedName() const;
  G4_RegFileKind getRegFile() const { return regFile; }

  // returns number of elements per row
  unsigned short getNumElems() const { return elemInfo.getNumElemsPerRow(); }
  unsigned short getNumRows() const { return elemInfo.getNumRows(); }
  unsigned short getTotalElems() const { return elemInfo.getNumElems(); }

  void setTotalElems(uint32_t numElems) {
    elemInfo.reset(numElems, GRFByteSize);
  }
  unsigned short getNumberFlagElements() const {
    vISA_ASSERT(regFile == G4_FLAG, "should only be called for flag vars");
    return numFlagElements;
  }
  void setNumberFlagElements(uint8_t numEl) {
    vISA_ASSERT(regFile == G4_FLAG, "may only be called on a flag");
    numFlagElements = numEl;
  }

  G4_Type getElemType() const { return elemInfo.getType(); }
  uint16_t getElemSize() const { return elemInfo.getElemSize(); }
  const G4_RegVar *getRegVar() const { return regVar; }
  G4_RegVar *getRegVar() { return regVar; }

  int getOffsetFromBase() {
    int offsetFromBase = 0;
    for (const G4_Declare *dcl = this; dcl->getAliasDeclare();
         dcl = dcl->getAliasDeclare())
      offsetFromBase += dcl->getAliasOffset();
    return offsetFromBase;
  }

  void setSpilledDeclare(G4_Declare *sd) { spillDCL = sd; }
  const G4_Declare *getSpilledDeclare() const { return spillDCL; }
  G4_Declare *getSpilledDeclare() { return spillDCL; }

  void setDeclId(unsigned id) { declId = id; }
  unsigned getDeclId() const { return declId; }

  void setIsSplittedDcl(bool b) { isSplittedDcl = b; }
  bool getIsSplittedDcl() const { return isSplittedDcl; }

  void setIsPartialDcl(bool b) { isPartialDcl = b; }
  bool getIsPartialDcl() const { return isPartialDcl; }

  void setIsRefInSendDcl(bool b) { refInSend |= b; }
  bool getIsRefInSendDcl() const { return refInSend; }

  void setSplitVarStartID(unsigned id) { startID = id; };
  unsigned getSplitVarStartID() const { return startID; };

  void setDoNotSpill() { doNotSpill = true; }
  bool isDoNotSpill() const { return doNotSpill; }

  void setAddrSpillFill() { addrSpillFill = true; }
  bool isAddrSpillFill() const { return addrSpillFill; }

  bool isMsgDesc() const {
    return regFile == G4_ADDRESS && elemInfo.getType() == Type_UD;
  }

  void setCapableOfReuse() { capableOfReuse = true; }
  bool getCapableOfReuse() const { return capableOfReuse; }

  void setAddressed() { addressed = true; }
  bool getAddressed() const {
    if (addressed) {
      return true;
    }
    if (AliasDCL) {
      return AliasDCL->getAddressed();
    } else {
      return false;
    }
  }

  void setPreDefinedVar(bool b) { PreDefinedVar = b; }
  bool isPreDefinedVar() const { return PreDefinedVar; }

  void setIsCmpUseOnly(bool b) { isCmpUseOnly = b; }
  bool getIsCmpUseOnly() const { return isCmpUseOnly; }

  void setIsBBLocal(bool b) { isBBLocal = b; }
  bool getIsBBLocal() const { return isBBLocal; }

  unsigned getNumRegNeeded() const;

  void emit(std::ostream &output) const;

  void dump() const { emit(std::cerr); }
};

typedef std::vector<vISA::G4_Declare *> DECLARE_LIST;
typedef std::vector<vISA::G4_Declare *>::iterator DECLARE_LIST_ITER;
} // namespace vISA

#endif // G4_DECLARE_H
