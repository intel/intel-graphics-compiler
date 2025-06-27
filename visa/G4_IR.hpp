/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _GEN4_IR_HPP_
#define _GEN4_IR_HPP_

#include <algorithm>
#include <array>
#include <bitset>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <vector>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

#include "Assertions.h"
#include "Attributes.hpp"
#include "BitSet.h"
#include "Common_GEN.h"
#include "Common_ISA.h" // for GenPrecisionTable
#include "G4_Declare.h"
#include "G4_Opcode.h"
#include "G4_Operand.h"
#include "G4_Register.h"
#include "G4_SendDescs.hpp"
#include "IGC/common/StringMacros.hpp"
#include "JitterDataStruct.h"
#include "Mem_Manager.h"
#include "Metadata.h"
#include "Option.h"
#include "visa_igc_common_header.h"

namespace vISA {
// forward declaration
class G4_INST;

class IR_Builder;

class G4_Kernel;

class G4_SpillIntrinsic;
class G4_FillIntrinsic;
class G4_PseudoAddrMovIntrinsic;
} // namespace vISA

vISA::G4_Declare *GetTopDclFromRegRegion(vISA::G4_Operand *opnd);

typedef vISA::std_arena_based_allocator<vISA::G4_INST *>
    INST_LIST_NODE_ALLOCATOR;

typedef std::list<vISA::G4_INST *, INST_LIST_NODE_ALLOCATOR> INST_LIST;
typedef std::list<vISA::G4_INST *, INST_LIST_NODE_ALLOCATOR>::iterator
    INST_LIST_ITER;
typedef std::list<vISA::G4_INST *, INST_LIST_NODE_ALLOCATOR>::const_iterator
    INST_LIST_CITER;
typedef std::list<vISA::G4_INST *, INST_LIST_NODE_ALLOCATOR>::reverse_iterator
    INST_LIST_RITER;

typedef std::pair<vISA::G4_INST *, Gen4_Operand_Number> USE_DEF_NODE;
typedef vISA::std_arena_based_allocator<USE_DEF_NODE> USE_DEF_ALLOCATOR;

typedef std::list<USE_DEF_NODE, USE_DEF_ALLOCATOR> USE_EDGE_LIST;
typedef std::list<USE_DEF_NODE, USE_DEF_ALLOCATOR>::iterator USE_EDGE_LIST_ITER;
typedef std::list<USE_DEF_NODE, USE_DEF_ALLOCATOR> DEF_EDGE_LIST;
typedef std::list<USE_DEF_NODE, USE_DEF_ALLOCATOR>::iterator DEF_EDGE_LIST_ITER;

namespace vISA {
#define SWSB_MAX_ALU_DEPENDENCE_DISTANCE 11
#define SWSB_MAX_ALU_DEPENDENCE_DISTANCE_64BIT 15

enum SB_INST_PIPE {
  PIPE_NONE = 0,
  PIPE_INT = 1,
  PIPE_FLOAT = 2,
  PIPE_LONG = 3,
  PIPE_MATH = 4,
  PIPE_S0 = 5,
  PIPE_DPAS = 6,
  PIPE_SEND = 7,
  PIPE_ALL = 8
};
// forward declaration for the binary of an instruction
class BinInst;

class G4_FCALL {
  uint16_t argSize;
  uint16_t retSize;
  bool uniform = false;

public:
  G4_FCALL() = delete;
  G4_FCALL(uint16_t argVarSz, uint16_t retVarSz, bool isUniform)
      : argSize(argVarSz), retSize(retVarSz), uniform(isUniform) {}

  uint16_t getArgSize() const { return argSize; }
  uint16_t getRetSize() const { return retSize; }
  bool isUniform() const { return uniform; }
};

// Forward references for classes used by G4_INST.
class G4_InstMath;
class G4_InstCF;
class G4_InstIntrinsic;
class G4_InstSend;
class G4_InstBfn;
class G4_InstDpas;
class GlobalOpndHashTable;

class G4_INST {
  friend class G4_SendDesc;
  friend class IR_Builder;

protected:
  G4_opcode op;
  // Reserve 4 sources should be enough for most of instructions.
  // TODO: In fact, we would reserve not just 4 but also set the size of "srcs"
  // to 4. Currently there is some legacy code that presumes srcs[0:3] are
  // always accessible for updates, and refactoring the code appears to be
  // a challenge. We probably should still pursue the opportunity to remove that
  // presumption and make the code cleaner.
  llvm::SmallVector<G4_Operand *, /*N=*/4> srcs;
  G4_DstRegRegion *dst;
  G4_Predicate *predicate;
  G4_CondMod *mod;
  unsigned int option; // inst option

  // def-use chain: list of <inst, opndPos> such that this[dst/condMod] defines
  // inst[opndPos] opndNum must be one of src0, src1, src2, pred, implAccSrc
  USE_EDGE_LIST useInstList;

  // use-def chain: list of <inst, opndPos> such that inst[dst/condMod] defines
  // this[opndPos]
  DEF_EDGE_LIST defInstList;

  // instruction's id in BB. Each optimization should re-initialize before using
  int32_t localId = 0;

  static const int UndefinedCisaOffset = -1;
  // Id of the vISA instruction this inst is generated from.
  // This is used by DebugInfo to map a Xe ISA instruction back to its vISA/llvm
  // IR instructions.
  int vISAInstId = UndefinedCisaOffset;

  Metadata *MD = nullptr;

#define UNDEFINED_GEN_OFFSET -1
  int64_t genOffset = UNDEFINED_GEN_OFFSET;

  void emit_options(std::ostream &output) const;

  // WARNING: if adding new options, please make sure that bitfield does not
  // overflow.
  bool sat : 1;
  // during optimization, an inst may become redundant and be marked dead
  bool dead : 1;
  bool evenlySplitInst : 1;
  bool doPostRA : 1; // for NoMaskWA
  bool canBeAcc : 1; // The inst can be ACC, including the inst's dst
                     // and the use operands in the DU chain.
  bool doNotDelete : 1;
  G4_ExecSize execSize;

  // make it private so only the IR_Builder can create new instructions
  void *operator new(size_t sz, Mem_Manager &m) { return m.alloc(sz); }
  uint32_t global_id = (uint32_t)-1;

  // link to builder to access the various compilation options
  const IR_Builder &builder;

public:
  enum SWSBTokenType {
    TOKEN_NONE,
    SB_SET,
    NoACCSBSet,
    AFTER_READ,
    AFTER_WRITE,
    READ_ALL,
    WRITE_ALL,
  };

  enum DistanceType {
    DIST_NONE,
    DIST,
    DISTALL,
    DISTINT,
    DISTFLOAT,
    DISTLONG,
    DISTS0,
    DISTMATH
  };
  typedef struct _SWSBInfo {
    unsigned short depDistance : 3;
    unsigned short distType : 4;
    unsigned short SBToken : 5;
    unsigned short tokenType : 4;
    _SWSBInfo() {
      depDistance = 0;
      distType = DIST_NONE;
      SBToken = 0;
      tokenType = TOKEN_NONE;
    }
  } SWSBInfo;

protected:
  // unsigned char depDistance = 0;
  bool operandTypeIndicated = false;
  bool isClosestALUType_ = false;

  SWSBInfo swsb;

public:
  void setDistance(unsigned char dep_distance) {
    vASSERT(swsb.depDistance <= 7);
    swsb.depDistance = dep_distance;
  }
  unsigned char getDistance() const { return swsb.depDistance; }

  void setDistanceTypeXe(DistanceType type) { swsb.distType = type; }
  DistanceType getDistanceTypeXe() const { return (DistanceType)swsb.distType; }

  void setToken(unsigned short token) { swsb.SBToken = token; }
  unsigned short getToken() const { return swsb.SBToken; }
  void setTokenType(SWSBTokenType type) {
    swsb.tokenType = (unsigned short)type;
  }
  SWSBTokenType getTokenType() const { return (SWSBTokenType)swsb.tokenType; }

  void setSBIDSetToken(unsigned short token) {
    swsb.SBToken = token;
    swsb.tokenType = SB_SET;
  }
  // TODO: return the swsb struct and strip out the necessary info at the
  // calling site
  unsigned short getSBIDSetToken() const {
    if (swsb.tokenType == SB_SET)
      return swsb.SBToken;
    else
      return -1;
  }

  unsigned short getSBIDToken(SWSBTokenType t) const {
    if (swsb.tokenType == t)
      return swsb.SBToken;
    return -1;
  }

  void setNoACCSBSet() { swsb.tokenType = NoACCSBSet; }
  bool hasNoACCSBSet() const { return swsb.tokenType == NoACCSBSet; }

  void setOperandTypeIndicated(bool indicated) {
    operandTypeIndicated = indicated;
  }
  void setIsClosestALUType(bool indicated) { isClosestALUType_ = indicated; }

  bool isOperandTypeIndicated() const { return operandTypeIndicated; }
  bool isClosestALUType() const { return isClosestALUType_; }

  bool isDpas() const { return (op == G4_dpas || op == G4_dpasw); }
  G4_InstDpas *asDpasInst() const {
    vISA_ASSERT(isDpas(), ERROR_UNKNOWN);
    return (G4_InstDpas *)this;
  }

private:
  void initOperands();

public:
  G4_INST(const IR_Builder &irb, G4_Predicate *prd, G4_opcode o, G4_CondMod *m,
          G4_Sat s, G4_ExecSize size, G4_DstRegRegion *d, G4_Operand *s0,
          G4_Operand *s1, G4_InstOpts opt)
      : G4_INST(irb, prd, o, m, s, size, d, s0, s1, nullptr, nullptr, opt) {}

  G4_INST(const IR_Builder &irb, G4_Predicate *prd, G4_opcode o, G4_CondMod *m,
          G4_Sat s, G4_ExecSize size, G4_DstRegRegion *d, G4_Operand *s0,
          G4_Operand *s1, G4_Operand *s2, G4_InstOpts opt)
      : G4_INST(irb, prd, o, m, s, size, d, s0, s1, s2, nullptr, opt) {}

  G4_INST(const IR_Builder &irb, G4_Predicate *prd, G4_opcode o, G4_CondMod *m,
          G4_Sat s, G4_ExecSize size, G4_DstRegRegion *d, G4_Operand *s0,
          G4_Operand *s1, G4_Operand *s2, G4_Operand *s3, G4_InstOpts opt);

  G4_INST(const IR_Builder &irb, G4_Predicate *prd, G4_opcode o, G4_CondMod *m,
          G4_Sat s, G4_ExecSize size, G4_DstRegRegion *d, G4_Operand *s0,
          G4_Operand *s1, G4_Operand *s2, G4_Operand *s3, G4_Operand *s4,
          G4_InstOpts opt);

  G4_INST(const IR_Builder &irb, G4_Predicate *prd, G4_opcode o, G4_CondMod *m,
          G4_Sat s, G4_ExecSize size, G4_DstRegRegion *d, G4_Operand *s0,
          G4_Operand *s1, G4_Operand *s2, G4_Operand *s3, G4_Operand *s4,
          G4_Operand *s5, G4_Operand *s6, G4_Operand *s7, G4_InstOpts opt);

  virtual ~G4_INST() {}

  // The method is declared virtual so subclasses of G4_INST
  // should also implement this method to populate members
  // unique to them.
  virtual G4_INST *cloneInst(const IR_Builder *b = nullptr);
  virtual bool isBaseInst() const { return true; }
  virtual bool isCFInst() const { return false; }

  uint32_t getLexicalId() const { return global_id; }
  void setLexicalId(uint32_t id) { global_id = id; }

  void setPredicate(G4_Predicate *p);
  G4_Predicate *getPredicate() const { return predicate; }
  const G4_VarBase *getPredicateBase() const {
    return predicate ? predicate->getBase() : nullptr;
  }
  G4_VarBase *getPredicateBase() {
    return const_cast<G4_VarBase *>(((const G4_INST *)this)
        ->getPredicateBase());
  }

  void setSaturate(G4_Sat s) { sat = (s == g4::SAT); }
  void setSaturate(bool z) { sat = z; }
  G4_Sat getSaturate() const { return sat ? g4::SAT : g4::NOSAT; }

  G4_opcode opcode() const { return op; }

  void setOpcode(G4_opcode opcd);

  G4_DstRegRegion *getDst() const { return dst; }
  bool supportsNullDst() const;

  bool isIEEEExceptionTrap() const;
  bool isBarrierWAIntrinsic() const;
  bool isNamedBarrierWAIntrinsic() const;
  bool isPseudoKill() const;
  bool isLifeTimeEnd() const;
  bool isSpillIntrinsic() const;
  bool isFlagSpillIntrinsic() const;
  G4_SpillIntrinsic *asSpillIntrinsic() const;
  bool isFillIntrinsic() const;
  G4_FillIntrinsic *asFillIntrinsic() const;
  bool isPseudoAddrMovIntrinsic() const;
  bool isSplitIntrinsic() const;
  bool isCallerSave() const;
  bool isCallerRestore() const;
  bool isCalleeSave() const;
  bool isCalleeRestore() const;
  bool isRelocationMov() const;
  bool isMov() const { return G4_Inst_Table[op].instType == InstTypeMov; }
  bool isLogic() const { return G4_Inst_Table[op].instType == InstTypeLogic; }
  bool isCompare() const {
    return G4_Inst_Table[op].instType == InstTypeCompare;
  }
  bool isFlowControl() const {
    return G4_Inst_Table[op].instType == InstTypeFlow;
  }
  bool isArithmetic() const {
    return G4_Inst_Table[op].instType == InstTypeArith;
  }
  bool isVector() const { return G4_Inst_Table[op].instType == InstTypeVector; }
  bool isLabel() const { return op == G4_label; }
  bool isCall() const { return op == G4_call; }
  bool isFCall() const { return op == G4_pseudo_fcall; }
  bool isReturn() const { return op == G4_return; }
  bool isFReturn() const { return (op == G4_pseudo_fret); }
  bool isMath() const { return op == G4_math; }
  bool isIntrinsic() const { return op == G4_intrinsic; }
  bool isSend() const {
    return op == G4_send || op == G4_sendc || isSplitSend();
  }
  // does the send split output payload into src0 and src1
  bool isSplitSend() const {
    return op == G4_sends || op == G4_sendsc;
  }
  bool isSendUnconditional() const {
    return op == G4_send || op == G4_sends;
  }
  bool isSendConditional() const {
    return op == G4_sendc || op == G4_sendsc;
  }
  // a send is a indirect send (gather send) if its src0 is ARF Scalar
  bool isSendi() const {
      if (!isSend())
          return false;
    return getSrc(0)->isS0();
  }
  bool isRSWADivergentInst() const {
    return op == G4_goto || op == G4_while || op == G4_if || op == G4_break;
  }
  bool isBfn() const { return op == G4_bfn; }

  bool isEOT() const { return getOption() & InstOpt_EOT; }

  bool hasVectImm() const {
    for (int i = 0, numSrc = getNumSrc(); i < numSrc; ++i) {
      if (getSrc(i)->isVectImm())
        return true;
    }
    return false;
  }

  // ToDo: get rid of this function which does't make sense for non-sends
  virtual G4_SendDesc *getMsgDesc() const { return nullptr; }

  const G4_SendDescRaw *getMsgDescRaw() const {
    const auto *msgDesc = getMsgDesc();
    if (msgDesc == nullptr || !getMsgDesc()->isRaw())
      return nullptr;
    return (const G4_SendDescRaw *)msgDesc;
  }
  G4_SendDescRaw *getMsgDescRaw() {
    auto *msgDesc = getMsgDesc();
    if (msgDesc == nullptr || !getMsgDesc()->isRaw())
      return nullptr;
    return (G4_SendDescRaw *)msgDesc;
  }

  // special instructions(e.g., send) should override
  virtual void computeRightBound(G4_Operand *opnd);

  bool isWait() const { return op == G4_wait; }
  static bool isSyncOpcode(G4_opcode opcode) {
    return opcode == G4_sync_nop || opcode == G4_sync_allrd ||
           opcode == G4_sync_allwr;
  }
  bool isSWSBSync() const { return G4_INST::isSyncOpcode(op); }

  bool isPseudoLogic() const {
    return op == G4_pseudo_and || op == G4_pseudo_or || op == G4_pseudo_xor ||
           op == G4_pseudo_not;
  }

  bool isPartialWrite() const;
  bool isPartialWriteForSpill(bool inSIMDCF, bool useNonScratchForSpill) const;
  bool isAddrAdd() const;
  bool isMovAddr() const;
  bool isAccSrcInst() const;
  bool isAccDstInst() const;

  bool nonALUInstructions() const {
    return isSend() || isLabel() || isCFInst() || isDpas() || isIntrinsic() ||
           opcode() == G4_nop || isWait();
  }

  G4_InstMath *asMathInst() const {
    vISA_ASSERT(isMath(), ERROR_UNKNOWN);
    return ((G4_InstMath *)this);
  }

  G4_InstCF *asCFInst() const {
    vISA_ASSERT(isFlowControl(), ERROR_UNKNOWN);
    return ((G4_InstCF *)this);
  }

  G4_InstIntrinsic *asIntrinsicInst() const {
    vISA_ASSERT(isIntrinsic(), ERROR_UNKNOWN);
    return (G4_InstIntrinsic *)this;
  }

  G4_PseudoAddrMovIntrinsic *asPseudoAddrMovIntrinsic() const {
    vISA_ASSERT(isPseudoAddrMovIntrinsic(), "not a fill intrinsic");
    return const_cast<G4_PseudoAddrMovIntrinsic *>(
        reinterpret_cast<const G4_PseudoAddrMovIntrinsic *>(this));
  }

  const G4_InstSend *asSendInst() const {
    if (!isSend()) {
      return nullptr;
    }
    return reinterpret_cast<const G4_InstSend *>(this);
  }
  G4_InstSend *asSendInst() {
    if (!isSend()) {
      return nullptr;
    }
    return reinterpret_cast<G4_InstSend *>(this);
  }

  G4_InstBfn *asBfnInst() const {
    vISA_ASSERT(isBfn(), ERROR_UNKNOWN);
    return (G4_InstBfn *)this;
  }

  bool isPseudoUse() const;
  G4_Type getExecType() const;
  G4_Type getExecType2() const;
  bool isComprInst() const { return detectComprInst(); }
  bool isComprInvariantSrcRegion(G4_SrcRegRegion *src, int srcPos);

  G4_Operand *getOperand(Gen4_Operand_Number opnd_num);

  G4_Operand *getSrc(unsigned i) const;
  void setSrc(G4_Operand *opnd, unsigned i);
  int getNumSrc() const;
  int getNumDst() const;

  auto src_begin() const { return srcs.begin(); }
  auto src_begin() { return srcs.begin(); }
  auto src_end() const { return srcs.begin() + getNumSrc(); }
  auto src_end() { return srcs.begin() + getNumSrc(); }

  // this assume we don't have to recompute bound for the swapped source
  // Note that def-use chain is not maintained after this; call swapDefUse
  // if you want to update the du-chain.
  void swapSrc(int src1, int src2) {
    vISA_ASSERT(
        (src1 >= 0 && src1 < getNumSrc() && src2 >= 0 && src2 < getNumSrc()),
        "illegal src number");
    std::swap(srcs[src1], srcs[src2]);
  }

  G4_Label *getLabel() {
    vISA_ASSERT(op == G4_label, "inst must be a label");
    return (G4_Label *)getSrc(0);
  }

  void setDest(G4_DstRegRegion *opnd);
  void setExecSize(G4_ExecSize s);

  void computeARFRightBound();

  static bool isMaskOption(G4_InstOption opt) {
    return (opt & InstOpt_QuarterMasks) != 0;
  }

  void setOptions(unsigned int o) {
    unsigned int oldMaskOffset = getMaskOffset();
    option = o;
    unsigned int newMaskOffset = getMaskOffset();

    if (oldMaskOffset != newMaskOffset) {
      // Change in mask offset requires change in
      // bounds for pred/cond mod/impl acc src/dst
      computeARFRightBound();
    }
  }

  void setOptionOn(G4_InstOption o) {
    vISA_ASSERT(!isMaskOption(o),
                "use setMaskOption() to change emask instead");
    option |= o;
  }

  void setOptionOff(G4_InstOption o) {
    vISA_ASSERT(!isMaskOption(o),
                "use setMaskOption() to change emask instead");
    option &= (~o);
  }
  unsigned int getOption() const { return option; }
  unsigned int getMaskOption() const { return option & InstOpt_Masks; }
  void setMaskOption(G4_InstOption opt) {
    // mask options are mutually exclusive, so we have to clear any previous
    // setting note that this does not clear NoMask
    vISA_ASSERT(opt & InstOpt_QuarterMasks, "opt is not a valid mask option");
    setOptions((option & ~InstOpt_QuarterMasks) | opt);
  }

  void setNoMask(bool clearEMask) {
    if (clearEMask) {
      // Clear the M0/M4/M8 emask as well
      setOptions((getOption() & ~InstOpt_Masks) | InstOpt_WriteEnable);
    } else {
      setOptionOn(InstOpt_WriteEnable);
    }
  }

  bool is1QInst() const {
    return execSize == g4::SIMD8 && getMaskOffset() == 0;
  }
  bool isWriteEnableInst() const {
    return (option & InstOpt_WriteEnable) ? true : false;
  }
  bool isYieldInst() const { return (option & InstOpt_Switch) ? true : false; }
  bool isNoPreemptInst() const {
    return (option & InstOpt_NoPreempt) ? true : false;
  }

  void emit(std::ostream &output);
  void emitDefUse(std::ostream &output) const;
  void emitInstIds(std::ostream &output) const;
  void print(std::ostream &OS) const;
  void dump(std::ostream &OS) const;
  void dump() const;
  const char *getLabelStr() const;

  // get simd lane mask for this instruction. For example,
  //      add  (8|M8) ...
  // will have 0xFF00, which lane 8-15
  uint32_t getExecLaneMask() const;
  G4_ExecSize getExecSize() const { return execSize; }
  const G4_CondMod *getCondMod() const { return mod; }
  G4_CondMod *getCondMod() { return mod; }
  const G4_VarBase *getCondModBase() const;
  G4_VarBase *getCondModBase() {
    return const_cast<G4_VarBase *>(((const G4_INST *)this)->getCondModBase());
  }
  void setCondMod(G4_CondMod *m);

  bool isDead() const { return dead; }
  void markDead() { dead = true; }

  bool isAligned1Inst() const { return !isAligned16Inst(); }
  bool isAligned16Inst() const {
    return (option & InstOpt_Align16) ? true : false;
  }
  bool isAccWrCtrlInst() const {
    return (option & InstOpt_AccWrCtrl) ? true : false;
  }
  bool isAtomicInst() const { return (option & InstOpt_Atomic) ? true : false; }
  bool isNoDDChkInst() const {
    return (option & InstOpt_NoDDChk) ? true : false;
  }
  bool isNoDDClrInst() const {
    return (option & InstOpt_NoDDClr) ? true : false;
  }
  bool isBreakPointInst() const {
    return (option & InstOpt_BreakPoint) ? true : false;
  }
  bool isCachelineAligned() const {
    return (option & InstOpt_CachelineAligned) ? true : false;
  }
  // true if inst reads/writes acc either implicitly or explicitly
  bool useAcc() const {
    return isAccDstInst() || isAccSrcInst() || getImplAccDst() ||
           getImplAccSrc();
  }

  bool defAcc() const { return isAccDstInst() || getImplAccDst(); }

  void setCompacted() { option = option | InstOpt_Compacted; }
  void setNoCompacted() { option = option | InstOpt_NoCompact; }
  bool isCompactedInst() const {
    return (option & InstOpt_Compacted) ? true : false;
  }
  bool isNoCompactedInst() const {
    return (option & InstOpt_NoCompact) ? true : false;
  }

  void setLocalId(int32_t lid) { localId = lid; }
  int32_t getLocalId() const { return localId; }

  void setEvenlySplitInst(bool val) { evenlySplitInst = val; }
  bool getEvenlySplitInst() const { return evenlySplitInst; }

  void setVISAId(int offset) { vISAInstId = offset; }
  int getVISAId() const { return vISAInstId; }
  bool isVISAIdValid() const { return getVISAId() != UndefinedCisaOffset; }
  void invalidateVISAId() { setVISAId(UndefinedCisaOffset); }

  bool isOptBarrier() const;
  bool hasImplicitAccSrc() const {
    return op == G4_mac || op == G4_mach || op == G4_sada2;
  }

  bool hasImplicitAccDst() const { return op == G4_addc || op == G4_subb; }

  bool mayExpandToAccMacro() const;

  Gen4_Operand_Number getSrcOperandNum(int srcPos) const {
    switch (srcPos) {
    case 0:
      return Opnd_src0;
    case 1:
      return Opnd_src1;
    case 2:
      return Opnd_src2;
    case 3:
      return Opnd_src3;
    case 4:
      return Opnd_src4;
    default:
      vISA_ASSERT_UNREACHABLE("bad source id");
      return Opnd_src0;
    }
  }
  static int getSrcNum(Gen4_Operand_Number opndNum) {
    vISA_ASSERT(isSrcNum(opndNum), "not a source number");
    return opndNum - 1;
  }
  static bool isSrcNum(Gen4_Operand_Number opndNum) {
    return opndNum == Opnd_src0 || opndNum == Opnd_src1 ||
           opndNum == Opnd_src2 || opndNum == Opnd_src3 ||
           opndNum == Opnd_src4 || opndNum == Opnd_src5 ||
           opndNum == Opnd_src6 || opndNum == Opnd_src7;
  }
  const G4_Operand *getOperand(Gen4_Operand_Number opnd_num) const;

  /// Remove all definitons that contribute to this[opndNum] and remove all
  /// uses from their corresponding definitions. To maintain def-use's, this
  /// is required while resetting a source operand.
  void removeDefUse(Gen4_Operand_Number opndNum);
  /// Remove a use from this instruction and update its correponding def.
  /// Returns the next use iterator of this instruction.
  USE_EDGE_LIST_ITER eraseUse(USE_EDGE_LIST_ITER iter);
  /// Remove all uses defined by this. To maintain def-use's, this is
  /// required to clear useInstList.
  void removeAllUses();
  /// Remove all defs that used by this. To maintain def-use's, this is
  /// required to clear defInstList.
  void removeAllDefs();
  void transferDef(G4_INST *inst2, Gen4_Operand_Number opndNum1,
                   Gen4_Operand_Number opndNum2);
  void transferUse(G4_INST *inst2, bool keepExisting = false);
  /// Copy this[opndNum1]'s definition to inst2[opndNum2]'s definition.
  void copyDef(G4_INST *inst2, Gen4_Operand_Number opndNum1,
               Gen4_Operand_Number opndNum2, bool checked = false);
  /// Copy this instructions's defs to inst2. If checked is true, then only
  /// copy those effective defs.
  void copyDefsTo(G4_INST *inst2, bool checked);
  /// Copy this instruction's uses to inst2. If checked is true, then only
  /// copy those effective uses.
  void copyUsesTo(G4_INST *inst2, bool checked);
  void removeUseOfInst();
  void trimDefInstList();
  bool isDFInstruction() const;
  bool isMathPipeInst() const;
  bool distanceHonourInstruction() const;
  bool tokenHonourInstruction() const;
  bool hasNoPipe() const;
  bool isS0Opnd(G4_Operand *opnd) const;
  bool isLongPipeType(G4_Type type) const;
  bool isIntegerPipeType(G4_Type type) const;
  bool isJEUPipeInstructionXe() const;
  bool isS0PipeInstructionXe() const;
  bool isLongPipeInstructionXe() const;
  bool isIntegerPipeInstructionXe() const;
  bool isFloatPipeInstructionXe() const;

  int getMaxDepDistance() const;
  SB_INST_PIPE getInstructionPipeXe() const;
  SB_INST_PIPE getDistDepPipeXe() const;

  void swapDefUse(Gen4_Operand_Number srcIxA = Opnd_src0,
                  Gen4_Operand_Number srcIxB = Opnd_src1);
  void addDefUse(G4_INST *use, Gen4_Operand_Number usePos);
  void uniqueDefUse() {
    useInstList.unique();
    defInstList.unique();
  }
  void clearUse() { useInstList.clear(); }
  void clearDef() { defInstList.clear(); }
  bool useEmpty() const { return useInstList.empty(); }
  bool hasOneUse() const { return useInstList.size() == 1; }
  /// Returns its definition if this's operand has a single definition. Returns
  /// 0 otherwise.
  G4_INST *getSingleDef(Gen4_Operand_Number opndNum, bool MakeUnique = false);
  USE_EDGE_LIST::const_iterator use_begin() const {
    return useInstList.begin();
  }
  USE_EDGE_LIST::iterator use_begin() { return useInstList.begin(); }
  USE_EDGE_LIST::const_iterator use_end() const { return useInstList.end(); }
  USE_EDGE_LIST::iterator use_end() { return useInstList.end(); }
  USE_EDGE_LIST::reference use_front() { return useInstList.front(); }
  USE_EDGE_LIST::reference use_back() { return useInstList.back(); }
  DEF_EDGE_LIST::const_iterator def_begin() const {
    return defInstList.begin();
  }
  DEF_EDGE_LIST::iterator def_begin() { return defInstList.begin(); }
  DEF_EDGE_LIST::const_iterator def_end() const { return defInstList.end(); }
  DEF_EDGE_LIST::iterator def_end() { return defInstList.end(); }
  DEF_EDGE_LIST::reference def_front() { return defInstList.front(); }
  DEF_EDGE_LIST::reference def_back() { return defInstList.back(); }
  size_t use_size() const { return useInstList.size(); }
  size_t def_size() const { return defInstList.size(); }
  void dumpDefUse(std::ostream &os = std::cerr);
  template <typename Compare> void sortUses(Compare Cmp) {
    useInstList.sort(Cmp);
  }

  void fixMACSrc2DefUse();
  void setImplAccSrc(G4_SrcRegRegion *opnd);
  void setImplAccDst(G4_DstRegRegion *opnd);

  bool isWAWdep(G4_INST *inst); /* not const: may compute bound */
  bool isWARdep(G4_INST *inst); /* not const: may compute bound */
  bool isRAWdep(G4_INST *inst); /* not const: may compute bound */
  G4_SrcRegRegion *getImplAccSrc() const;
  G4_DstRegRegion *getImplAccDst() const;
  uint16_t getMaskOffset() const;
  static G4_InstOption offsetToMask(int execSize, int offset, bool nibOk);
  bool isRawMov() const;
  bool hasACCSrc() const;
  bool hasACCOpnd() const;
  G4_Type getOpExecType(int &extypesize) const;
  bool isCopyMov() const;
  bool canHoistTo(const G4_INST *defInst, bool simdBB) const;
  enum MovType {
    Copy = 0,           // MOV is a copy.
    ZExt = 1,           // MOV is a zero extension.
    SExt = 2,           // MOV is a sign extension.
    Trunc = 3,          // MOV is a truncation.
    IntToFP = 4,        // MOV is a conversion from Int to Float.
    FPToInt = 5,        // MOV is a conversion from Float to Int.
    FPUpConv = 6,       // MOV is a conversion from low precision to
                        // high precision.
    FPDownConv = 7,     // MOV is a conversion from high precision to
                        // low precision.
    FPDownConvSafe = 8, // Float down conversion for DX shaders.
    SuperMov = 9,       // MOV is a mov with other effects.
  };
  MovType canPropagate() const;
  //
  // check if this is a simple integer add that can be propagated to a
  // ternary instruction potentially
  //
  // has to to be:
  //   {add,mul} (E)  dst.0<1>:d  src0:{d,w} {src1|imm32}:{d,w}
  //   {add,mul} (1)  dst.X<1>:d  src0.X:{d,w} {src1|imm32}:{d,w}
  // And there are other various constraints....
  bool canPropagateBinaryToTernary() const;

  G4_Type getPropType(Gen4_Operand_Number opndNum, MovType MT,
                      const G4_INST *mov) const;
  bool isSignSensitive(Gen4_Operand_Number opndNum) const;
  bool canPropagateTo(G4_INST *useInst, Gen4_Operand_Number opndNum, MovType MT,
                      bool inSimdFlow, bool statelessAddrss = false);
  bool canHoist(bool simdBB, const Options *opt) const;
  bool isCommutative() const;

  bool hasNULLDst() const;
  bool goodTwoGRFDst(bool &evenSplitDst) const;
  void setGenOffset(int64_t off) { genOffset = off; }
  int64_t getGenOffset() const { return genOffset; }

  void computeLeftBoundForImplAcc(G4_Operand *opnd);

  void setNoSrcDepSet(bool val) {
    if (val) {
      option |= InstOpt_NoSrcDepSet;
    } else {
      option &= ~InstOpt_NoSrcDepSet;
    }
  }

  bool isNoSrcDepSet() const { return (option & InstOpt_NoSrcDepSet) != 0; }
  bool isMixedMode() const;
  bool isIllegalMixedMode() const;
  bool isAllSrcsAlignedToDst() const;
  bool canSupportCondMod() const;
  bool canSwapSource() const;
  bool canSupportSaturate() const;
  bool canSupportSrcModifier() const;
  bool isSamePrePostConds() const;

  bool writesFlag() const;

  bool usesFlag() const {
    return predicate != nullptr || (op != G4_sel && mod != nullptr);
  }

  bool is2SrcAlign16() const {
    return op == G4_dp2 || op == G4_dp3 || op == G4_dp4 || op == G4_dph;
  }
  bool isFastHFInstruction(void) const;

  bool isAlign1Ternary() const;

  // if instruction requries operansd to have DW (D/UD) type
  bool needsDWType() const { return op == G4_mulh || op == G4_madw; }

  bool canOpndBeAccCommon(Gen4_Operand_Number opndNum) const;
  bool isSupportedAccDstType() const;
  bool canDstBeAcc() const;
  bool canSrcBeAcc(Gen4_Operand_Number opndNum) const;

  bool canInstBeAcc(GlobalOpndHashTable *ght);

  bool canInstBeAcc() const { return canBeAcc; }

  virtual bool isDoNotDelete() const { return doNotDelete; }
  void markDoNotDelete() { doNotDelete = true; }

  bool canSrcBeFlagForPropagation(Gen4_Operand_Number opndNum) const;

  bool canSrcBeAccBeforeHWConform(Gen4_Operand_Number opndNum) const;

  bool canSrcBeAccAfterHWConform(Gen4_Operand_Number opndNum) const;

  TARGET_PLATFORM getPlatform() const;

  void setMetadata(const std::string &key, MDNode *value);

  MDNode *getMetadata(const std::string &key) const {
    return MD ? MD->getMetadata(key) : nullptr;
  }

  unsigned getTokenLocationNum() const {
    auto tokenLoc = getMetadata(Metadata::TokenLoc);
    if (!tokenLoc) {
      return 0;
    }
    MDTokenLocation *token = tokenLoc->asMDTokenLocation();
    if (token != nullptr) {
      return token->getTokenLocationNum();
    } else {
      return 0;
    }
  }

  unsigned getTokenLoc(int i, unsigned short &tokenID) const {
    auto tokenLoc = getMetadata(Metadata::TokenLoc);
    if (!tokenLoc) {
      return 0;
    }
    MDTokenLocation *token = tokenLoc->asMDTokenLocation();
    if (token != nullptr) {
      tokenID = token->getToken(i);
      return token->getTokenLocation(i);
    } else {
      return 0;
    }
  }

  void setTokenLoc(unsigned short token, unsigned globalID);

  // adds a comment to this instruction
  // this appends the comment to any existing comment separating it with
  // some separator e.g. "foo; bar"
  void addComment(const std::string &comment);

  // replaces any old comments with this
  // prefer addComment if don't wish to stomp earlier comments
  void setComments(const std::string &comments);

  // For NoMaskWA. If set, it needs WA.
  bool getNeedPostRA() const { return doPostRA; }
  void setNeedPostRA(bool V) { doPostRA = V; }

  std::string getComments() const {
    auto comments = getMetadata(Metadata::InstComment);
    return comments && comments->isMDString()
               ? comments->asMDString()->getData()
               : "";
  }

  MDLocation *getLocation() const {
    auto location = getMetadata(Metadata::InstLoc);
    return (location && location->isMDLocation()) ? location->asMDLocation()
                                                  : nullptr;
  }

  int getLineNo() const {
    auto location = getLocation();
    return location ? location->getLineNo() : 0;
  }

  const char *getSrcFilename() const {
    auto location = getLocation();
    return location ? location->getSrcFilename() : nullptr;
  }

  void inheritDIFrom(const G4_INST *inst);

  void inheritSWSBFrom(const G4_INST *inst);

  const IR_Builder &getBuilder() const { return builder; }

  bool isPureBFInst() const {
    G4_Operand *dst = getDst();
    if (!dst || dst->getType() != Type_BF)
      return false;
    for (int i = 0, numSrcs = getNumSrc(); i < numSrcs; i++) {
      auto src = getSrc(i);
      if (src && src->getType() != Type_BF)
        return false;
    }
    return true;
  }

  bool canSupportPureBF() const {
    return (op == G4_mov || op == G4_add || op == G4_sel || op == G4_cmp ||
           op == G4_csel || op == G4_cmpn || op == G4_mul || op == G4_mad ||
           op == G4_math);
  }

  virtual bool requireNopAfter() const { return false; }
private:
  // use inheritDIFrom() instead
  void setLocation(MDLocation *loc) { setMetadata(Metadata::InstLoc, loc); }
  bool detectComprInst() const;
  bool isLegalType(G4_Type type, Gen4_Operand_Number opndNum) const;
  bool isFloatOnly() const;
};
} // namespace vISA

std::ostream &operator<<(std::ostream &os, vISA::G4_INST &inst);
std::ostream &operator<<(std::ostream &os, vISA::G4_Operand &opnd);

namespace vISA {

class G4_InstBfn : public G4_INST {
  uint8_t funcCtrl;

public:
  G4_InstBfn(const IR_Builder &builder, G4_Predicate *prd, G4_CondMod *m,
             G4_Sat sat, G4_ExecSize size, G4_DstRegRegion *d, G4_Operand *s0,
             G4_Operand *s1, G4_Operand *s2, G4_InstOpts opt,
             uint8_t mBooleanFuncCtrl)
      : G4_INST(builder, prd, G4_bfn, m, sat, size, d, s0, s1, s2, opt),
        funcCtrl(mBooleanFuncCtrl) {}

  G4_INST *cloneInst(const IR_Builder *b = nullptr) override;

  uint8_t getBooleanFuncCtrl() const { return funcCtrl; }
};

class G4_InstDpas : public G4_INST {
  GenPrecision Src1Precision; // Weights
  GenPrecision Src2Precision; // Activation
  uint8_t SystolicDepth;      // 1|2|4|8
  uint8_t RepeatCount;        // 1-8
  bool mayNeedRSWA = false;

  enum {
    OPS_PER_CHAN_1 = 1,
    OPS_PER_CHAN_2 = 2,
    OPS_PER_CHAN_4 = 4,
    OPS_PER_CHAN_8 = 8
  };

public:
  static uint32_t GetPrecisionSizeInBits(GenPrecision P) {
    return GenPrecisionTable[(int)P].BitSize;
  }

  static bool hasSamePrecision(GenPrecision p1, GenPrecision p2) {
    if (p1 == p2) {
      return true;
    }

    return (((p1 == GenPrecision::U8 || p1 == GenPrecision::S8) &&
             (p2 == GenPrecision::U8 || p2 == GenPrecision::S8)) ||
            ((p1 == GenPrecision::U4 || p1 == GenPrecision::S4 ||
              p1 == GenPrecision::U2 || p1 == GenPrecision::S2) &&
             (p2 == GenPrecision::U4 || p2 == GenPrecision::S4 ||
              p2 == GenPrecision::U2 || p2 == GenPrecision::S2)));
  }

  G4_InstDpas(const IR_Builder &builder, G4_opcode o, G4_ExecSize size,
              G4_DstRegRegion *d, G4_Operand *s0, G4_Operand *s1,
              G4_Operand *s2, G4_Operand *s3, G4_Operand *s4, G4_InstOpts opt,
              GenPrecision a, GenPrecision w, uint8_t sd, uint8_t rc)
      : G4_INST(builder, nullptr, o, nullptr, g4::NOSAT, size, d, s0, s1, s2,
                s3, s4, opt),
        Src1Precision(w), Src2Precision(a), SystolicDepth(sd), RepeatCount(rc) {
  }

  G4_INST *cloneInst(const IR_Builder *b = nullptr) override;

  // Check if this is int dpas or half-float dpas
  bool isBF16() const { return Src1Precision == GenPrecision::BF16; }
  bool isFP16() const { return Src1Precision == GenPrecision::FP16; }
  bool isTF32() const { return Src1Precision == GenPrecision::TF32; }
  bool isInt() const;
  bool isInt8() const;
  bool is2xInt8() const; // true if it is 2xint8 dpas

  uint8_t getOpsPerChan() const;
  uint8_t getSystolicDepth() const { return SystolicDepth; }
  uint8_t getRepeatCount() const { return RepeatCount; }
  GenPrecision getSrc1Precision() const { return Src1Precision; }
  GenPrecision getSrc2Precision() const { return Src2Precision; }

  bool hasSameSrc1Precision(GenPrecision p) const {
    return hasSamePrecision(Src1Precision, p);
  }

  bool hasSameSrc2Precision(GenPrecision p) const {
    return hasSamePrecision(Src2Precision, p);
  }

  void setRepeatCount(uint8_t rc) { RepeatCount = rc; }
  // data size per lane (data size per each systolic depth)
  uint32_t getPrecisionSizePerLaneInByte(GenPrecision P) const {
    uint32_t PBits = G4_InstDpas::GetPrecisionSizeInBits(P);
    return (PBits * getOpsPerChan() / 8);
  }
  uint32_t getSrc1SizePerLaneInByte() const {
    return getPrecisionSizePerLaneInByte(Src1Precision);
  }
  uint32_t getSrc2SizePerLaneInByte() const {
    return getPrecisionSizePerLaneInByte(Src2Precision);
  }

  void computeRightBound(G4_Operand *opnd) override;
  void setMayNeedWA(bool b) { mayNeedRSWA = b; }
  bool mayNeedWA() const { return mayNeedRSWA; }
  // The function checks if both dpas instructions satisfy macro rules for
  // operand types.
  bool checksMacroTypes(const G4_InstDpas &next) const;
};

// TODO: move this into G4_InstMath.
typedef enum {
  MATH_RESERVED = 0,
  MATH_INV = 1,
  MATH_LOG = 2,
  MATH_EXP = 3,
  MATH_SQRT = 4,
  MATH_RSQ = 5,
  MATH_SIN = 6,
  MATH_COS = 7,
  // 8 is skipped
  MATH_FDIV = 9,
  MATH_POW = 0xA,
  MATH_INT_DIV = 0xB,
  MATH_INT_DIV_QUOT = 0xC,
  MATH_INT_DIV_REM = 0xD,
  MATH_INVM = 0xE,
  MATH_RSQRTM = 0xF,
} G4_MathOp;

class G4_InstMath : public G4_INST {
  G4_MathOp mathOp;

public:
  G4_InstMath(const IR_Builder &builder, G4_Predicate *prd, G4_opcode o,
              G4_CondMod *m, G4_Sat sat, G4_ExecSize execSize,
              G4_DstRegRegion *d, G4_Operand *s0, G4_Operand *s1,
              G4_InstOpts opt, G4_MathOp mOp = MATH_RESERVED)
      : G4_INST(builder, prd, o, m, sat, execSize, d, s0, s1, opt),
        mathOp(mOp) {}

  G4_INST *cloneInst(const IR_Builder *b = nullptr) override;

  bool isIEEEMath() const {
    return mathOp == MATH_INVM || mathOp == MATH_RSQRTM;
  }
  bool isMathIntDiv() const {
    return mathOp >= MATH_INT_DIV && mathOp < MATH_INVM;
  }
  bool isOneSrcMath() const {
    return mathOp == MATH_INV || mathOp == MATH_LOG || mathOp == MATH_EXP ||
      mathOp == MATH_SQRT || mathOp == MATH_RSQ || mathOp == MATH_SIN ||
      mathOp == MATH_COS || mathOp == MATH_RSQRTM
      ;
  }
  G4_MathOp getMathCtrl() const { return mathOp; }
};

class G4_InstCF : public G4_INST {
  // operands for CF instructions
  // -- if, else, endif, while, break, cont, goto: JIP and UIP are used for the
  // branch target.
  // -- jmpi: src0 stores the branch target, and it can be either a label
  // (direct) or a SrcRegRegion (indirect)
  // -- call: src0 stores the callee address, and it must be a label
  // -- fcall: src0 stores the callee address, and it can be either a label
  // (direct) or a SrcRegRegion (indirect).
  //           dst contains the ret IP and call mask.
  // -- ret, fret: src0 contains the ret IP and call mask
  // Note that for call/ret the retIP variable is not created till RA
  G4_Label *jip; // GT JIP.
  G4_Label *uip; // GT UIP.
  // list of labels that this instruction could jump to.  Only used for switch
  // jmps
  std::list<G4_Label *> indirectJmpTarget;

  // True if this is a backward branch (including while)
  bool isBackwardBr;

  // True if this branch is a uniform. By uniform, it means that all active
  // lanes at the branch goes to the same target (Valid for
  // if/while/break/goto/jmpi only. This info could be encoded in instOpt.).
  // Note that all active lanes at the branch could be subset of all active
  // lanes on entry to shader/kernel.
  bool isUniformBr;

  // Nomrally, SWSB has to consider the control-flow edge created by a join.
  // If this field is true, SWSB can safely skip the edge from this join.
  bool SWSBCanSkip;

public:
  static const uint32_t unknownCallee = 0xFFFF;

  // used by non jmp/call/ret instructions
  G4_InstCF(const IR_Builder &builder, G4_Predicate *prd, G4_opcode op,
            G4_ExecSize size, G4_Label *jipLabel, G4_Label *uipLabel,
            G4_InstOpts instOpt)
      : G4_INST(builder, prd, op, nullptr, g4::NOSAT, size, nullptr, nullptr,
                nullptr, instOpt),
        jip(jipLabel), uip(uipLabel), isBackwardBr(op == G4_while),
        isUniformBr(false), SWSBCanSkip(false) {
    isUniformBr = (op == G4_jmpi ||
                   (op == G4_goto && (size == g4::SIMD1 || prd == nullptr)));
  }

  // used by jump/call/ret
  G4_InstCF(const IR_Builder &builder, G4_Predicate *prd, G4_opcode o,
            G4_CondMod *m, G4_ExecSize size, G4_DstRegRegion *d, G4_Operand *s0,
            G4_InstOpts opt)
      : G4_INST(builder, prd, o, m, g4::NOSAT, size, d, s0, nullptr, opt),
        jip(NULL), uip(NULL), isBackwardBr(o == G4_while), isUniformBr(false),
        SWSBCanSkip(false) {
    isUniformBr = (op == G4_jmpi ||
                   (op == G4_goto && (size == g4::SIMD1 || prd == nullptr)));
  }

  bool isCFInst() const override { return true; }

  void setJip(G4_Label *opnd) { jip = opnd; }
  const G4_Label *getJip() const { return jip; }
  G4_Label *getJip() { return jip; }
  const char *getJipLabelStr() const;

  void setUip(G4_Label *opnd) { uip = opnd; }
  const G4_Label *getUip() const { return uip; }
  G4_Label *getUip() { return uip; }
  const char *getUipLabelStr() const;

  void addIndirectJmpLabel(G4_Label *label) {
    vISA_ASSERT(isIndirectJmp(), "may only be called for indirect jmp");
    indirectJmpTarget.push_back(label);
  }
  const std::list<G4_Label *> &getIndirectJmpLabels() {
    vISA_ASSERT(isIndirectJmp(), "may only be called for indirect jmp");
    return indirectJmpTarget;
  }

  void setBackward(bool val) { isBackwardBr = val; }

  bool isBackward() const { return isBackwardBr; }

  void setUniform(bool val) { isUniformBr = val; }
  bool isUniform() const { return isUniformBr; }

  void setSWSBSkip(bool val) { SWSBCanSkip = val; }
  bool canSWSBSkip() const { return SWSBCanSkip; }

  bool isIndirectJmp() const;

  bool isUniformGoto(unsigned KernelSimdSize) const;

  bool isIndirectCall() const;

  // for direct call, this is null till after the compilation units are stitched
  // together for indirect call, this is src0
  G4_Operand *getCalleeAddress() const {
    if (op == G4_pseudo_fcall) {
      return getSrc(0);
    } else {
      return nullptr;
    }
  }

  void pseudoCallToCall() {
    vASSERT(isFCall() || op == G4_pseudo_fc_call);
    setOpcode(G4_call);
  }

  void pseudoRetToRet() {
    vASSERT(isFReturn() || op == G4_pseudo_fc_ret);
    setOpcode(G4_return);
  }

  void callToFCall() {
    vASSERT(isCall());
    setOpcode(G4_pseudo_fcall);
  }

  void retToFRet() {
    if (isFReturn())
      return;
    vASSERT(isReturn());
    setOpcode(G4_pseudo_fret);
  }

  bool requireNopAfter() const override;
}; // G4_InstCF

class G4_InstSend : public G4_INST {
  // Once initialized, remain unchanged as it could be shared among several
  // sends.
  G4_SendDesc *msgDesc;

public:
  // send (one source)
  // desc is either imm or a0.0 and in src1
  // extDesc is always immediate and encoded in md
  G4_InstSend(const IR_Builder &builder, G4_Predicate *prd, G4_opcode o,
              G4_ExecSize execSize, G4_DstRegRegion *dst,
              G4_SrcRegRegion *payload, G4_Operand *desc, G4_InstOpts opt,
              G4_SendDescRaw *md);

  // split send (two source)
  // desc is either imm or a0.0 and in src2
  // extDesc is either imm or a0.N and in src3
  G4_InstSend(const IR_Builder &builder, G4_Predicate *prd, G4_opcode o,
              G4_ExecSize execSize, G4_DstRegRegion *dst, G4_SrcRegRegion *src0,
              G4_SrcRegRegion *src1, G4_Operand *src2desc,
              G4_Operand *src3extDesc, G4_InstOpts opt, G4_SendDescRaw *md);

  G4_INST *cloneInst(const IR_Builder *b = nullptr) override;

  void setSendc() {
    // no effect if op is already G4_sendc/G4_sendsc
    if (op == G4_send) {
      op = G4_sendc;
    } else if (op == G4_sends) {
      op = G4_sendsc;
    }
  }

  G4_Operand *getMsgDescOperand() const {
    return isSplitSend() ? srcs[2] : srcs[1];
  }

  G4_Operand *getMsgExtDescOperand() const {
    vISA_ASSERT(isSplitSend(), "must be a split send instruction");
    return srcs[3];
  }

  // returns the number of effective srcs that may come from GRFS
  // for ancient unary send this is only 1; sends allows 2
  int getNumSrcPayloads() const { return isSplitSend() ? 2 : 1; }

  G4_SendDesc *getMsgDesc() const override { return msgDesc; }

  void setMsgDesc(G4_SendDesc *in);

  // restrictions on whether a send may be EOT:
  // -- The posted destination operand must be null
  // -- A thread must terminate with a send instruction with message to a shared
  // function on the output message bus;
  //    therefore, it cannot terminate with a send instruction with message to
  //    the following shared functions: Sampler unit, NULL function For example,
  //    a thread may terminate with a URB write message or a render cache write
  //    message.
  // -- A root thread originated from the media (generic) pipeline must
  // terminate
  //    with a send instruction with message to the Thread Spawner unit. A child
  //    thread should also terminate with a send to TS.
  bool canBeEOT() const {
    bool canEOT = getMsgDesc()->getDstLenRegs() == 0 &&
                  (getMsgDesc()->getSFID() != SFID::NULL_SFID &&
                   getMsgDesc()->getSFID() != SFID::SAMPLER &&
                  !getMsgDesc()->isFence());

    return canEOT;
  }
  void setEOT() {
    vISA_ASSERT(canBeEOT(), "canBeEOT() must hold");
    setOptionOn(InstOpt_EOT);
  }

  bool isFence() const { return getMsgDesc()->isFence(); }

  bool isSVMScatterRW() const;

  bool isDirectSplittableSend() const;

  void computeRightBound(G4_Operand *opnd) override;

  void emit_send(std::ostream &output);
  void emit_send_desc(std::ostream &output);

  void setSerialize() { option = option | InstOpt_Serialize; }
  bool isSerializedInst() const { return (option & InstOpt_Serialize) != 0; }
};     // G4_InstSend

} // namespace vISA

enum class PseudoKillType { FromLiveness = 1, Src = 2, Other = 3 };

// a special intrinsic instruction for any pseudo operations. An intrinsic inst
// has the following characteristics
// -- it is modeled as a call to some unknown function
// -- 1 dst and up to 3 srcs are allowed for the intrinsic
// -- conditonal modifier and saturation are currently not allowed (can add
// later)
// -- an intrinsic may reserve additional GRF/addr/flag for its code gen, which
// RA needs to honor
// -- it must be lowered/deleted before certain phases in the finalizer (no
// later than binary encoding)

// all intrinsic opcode go here
// order must match that of the G4_Intrinsics table
enum class Intrinsic {
  Wait,
  Use,
  MemFence,
  PseudoKill,
  PseudoUse, // ToDo: can we merge Use and PseudoUse? former is from input while
             // latter is internally generated.
  Spill,
  Fill,
  Split,
  CallerSave,
  CallerRestore,
  CalleeSave,
  CalleeRestore,
  FlagSpill,
  PseudoAddrMov,
  NamedBarrierWA,
  BarrierWA,
  IEEEExceptionTrap,
  Breakpoint,
  NumIntrinsics
};

enum class Phase {
  CFG,
  Optimizer,
  HWConformity,
  RA,
  Scheduler,
  SWSB,
  BinaryEncoding
};

// bit-field enum for various intrinsic properties, i.e., 1 << Property gives
// its bit offset.
// TODO: Would it make sense to unify it with G4_INST's attributes?
enum IntrinsicFlags { HasSideEffects = 0 };

struct IntrinsicInfo {
  Intrinsic id;
  const char *name;
  int numDst;
  int numSrc;
  Phase loweredBy; // intrinsic must be lowered before entering this phase
  uint64_t flags;
};

namespace vISA {
class G4_InstIntrinsic : public G4_INST {
  const Intrinsic intrinsicId;

  static constexpr IntrinsicInfo G4_Intrinsics[(int)Intrinsic::NumIntrinsics] =
      {
          //  id  name   numDst   numSrc  loweredBy
          {Intrinsic::Wait, "wait", 0, 0, Phase::Optimizer, 0},
          {Intrinsic::Use, "use", 0, 1, Phase::Scheduler, 0},
          {Intrinsic::MemFence, "mem_fence", 0, 0, Phase::BinaryEncoding,
           1ull << HasSideEffects},
          {Intrinsic::PseudoKill, "pseudo_kill", 1, 1, Phase::RA, 0},
          {Intrinsic::PseudoUse, "pseudo_use", 0, 1, Phase::RA, 0},
          {Intrinsic::Spill, "spill", 1, 2, Phase::RA, 0},
          {Intrinsic::Fill, "fill", 1, 1, Phase::RA, 0},
          {Intrinsic::Split, "split", 1, 1, Phase::RA, 0},
          {Intrinsic::CallerSave, "caller_save", 1, 0, Phase::RA, 0},
          {Intrinsic::CallerRestore, "caller_restore", 0, 1, Phase::RA, 0},
          {Intrinsic::CalleeSave, "callee_save", 1, 0, Phase::RA, 0},
          {Intrinsic::CalleeRestore, "callee_restore", 0, 1, Phase::RA, 0},
          {Intrinsic::FlagSpill, "flagSpill", 0, 1, Phase::RA, 0},
          {Intrinsic::PseudoAddrMov, "pseudo_addr_mov", 1, 8,
           Phase::BinaryEncoding, 0},
          {Intrinsic::NamedBarrierWA, "namedBarrierWA", 1, 1, Phase::SWSB, 0},
          {Intrinsic::BarrierWA, "barrierWA", 1, 0, Phase::SWSB, 0},
          {Intrinsic::IEEEExceptionTrap, "ieee_exception_trap", 1, 0,
           Phase::SWSB, 0},
          {Intrinsic::Breakpoint, "breakpoint", 0, 0, Phase::SWSB, 1ull << HasSideEffects},
      };

public:
  G4_InstIntrinsic(const IR_Builder &builder, G4_Predicate *prd,
                   Intrinsic intrinId, G4_ExecSize execSize, G4_DstRegRegion *d,
                   G4_Operand *s0, G4_Operand *s1, G4_Operand *s2,
                   G4_InstOpts opt)
      : G4_INST(builder, prd, G4_intrinsic, nullptr, g4::NOSAT, execSize, d, s0,
                s1, s2, opt),
        intrinsicId(intrinId) {}

  G4_InstIntrinsic(const IR_Builder &builder, G4_Predicate *prd,
                   Intrinsic intrinId, G4_ExecSize execSize, G4_DstRegRegion *d,
                   G4_Operand *s0, G4_Operand *s1, G4_Operand *s2,
                   G4_Operand *s3, G4_Operand *s4, G4_Operand *s5,
                   G4_Operand *s6, G4_Operand *s7, G4_InstOpts opt)
      : G4_INST(builder, prd, G4_intrinsic, nullptr, g4::NOSAT, execSize, d, s0,
                s1, s2, s3, s4, s5, s6, s7, opt),
        intrinsicId(intrinId) {}

  G4_INST *cloneInst(const IR_Builder *b = nullptr) override;

  int getNumDst() const { return G4_Intrinsics[(int)intrinsicId].numDst; }
  int getNumSrc() const { return G4_Intrinsics[(int)intrinsicId].numSrc; }
  unsigned getDstByteSize() const;

  Intrinsic getIntrinsicId() const { return intrinsicId; }
  const char *getName() const { return G4_Intrinsics[(int)intrinsicId].name; }
  Phase getLoweredByPhase() const {
    return G4_Intrinsics[(int)intrinsicId].loweredBy;
  }

  void computeRightBound(G4_Operand *opnd) override;
  bool hasSideEffects() const {
    return G4_Intrinsics[(int)intrinsicId].flags &
           (1ull << IntrinsicFlags::HasSideEffects);
  }

  bool isDoNotDelete() const override {
    // Return true if hasSideEffects is true. Otherwise, check if doNotDelete
    // is marked or not.
    return hasSideEffects() ? true : doNotDelete;
  }
};
// place for holding all physical register operands
//
class PhyRegPool {
  unsigned maxGRFNum;
  G4_Greg **GRF_Table;
  G4_Areg *ARF_Table[AREG_LAST];

public:
  PhyRegPool(
      Mem_Manager &m,
      unsigned int maxRegisterNumber); // create all physical register operands
  void rebuildRegPool(Mem_Manager &m, unsigned int numRegisters);
  G4_Greg *getGreg(unsigned i) {
    vISA_ASSERT(i < maxGRFNum, "invalid GRF");
    return GRF_Table[i];
  }

  G4_Areg *getNullReg() { return ARF_Table[AREG_NULL]; }
  G4_Areg *getMask0Reg() { return ARF_Table[AREG_MASK0]; }
  G4_Areg *getAcc0Reg() { return ARF_Table[AREG_ACC0]; }
  G4_Areg *getAcc1Reg() { return ARF_Table[AREG_ACC1]; }
  G4_Areg *getDbgReg() { return ARF_Table[AREG_DBG]; }
  G4_Areg *getMsg0Reg() { return ARF_Table[AREG_MSG0]; }
  G4_Areg *getSr0Reg() { return ARF_Table[AREG_SR0]; }
  G4_Areg *getCr0Reg() { return ARF_Table[AREG_CR0]; }
  G4_Areg *getTm0Reg() { return ARF_Table[AREG_TM0]; }
  G4_Areg *getAddrReg() { return ARF_Table[AREG_A0]; }
  G4_Areg *getN0Reg() { return ARF_Table[AREG_N0]; }
  G4_Areg *getN1Reg() { return ARF_Table[AREG_N1]; }
  G4_Areg *getIpReg() { return ARF_Table[AREG_IP]; }
  G4_Areg *getF0Reg() { return ARF_Table[AREG_F0]; }
  G4_Areg *getF1Reg() { return ARF_Table[AREG_F1]; }
  G4_Areg *getTDRReg() { return ARF_Table[AREG_TDR0]; }
  G4_Areg *getSPReg() { return ARF_Table[AREG_SP]; }
  G4_Areg *getF2Reg() { return ARF_Table[AREG_F2]; }
  G4_Areg *getF3Reg() { return ARF_Table[AREG_F3]; }
  G4_Areg *getScalarReg() { return ARF_Table[AREG_S0]; }

  // map int to flag areg
  G4_Areg *getFlagAreg(int flagNum) {
    switch (flagNum) {
    case 0:
      return getF0Reg();
    case 1:
      return getF1Reg();
    case 2:
      return getF2Reg();
    case 3:
      return getF3Reg();
    default:
      vISA_ASSERT_UNREACHABLE("unexpected flag register value");
      return nullptr;
    }
  }
};

inline G4_Operand *G4_INST::getOperand(Gen4_Operand_Number opnd_num) {
  return const_cast<G4_Operand *>(
      ((const G4_INST *)this)->getOperand(opnd_num));
}

inline G4_Operand *G4_INST::getSrc(unsigned i) const {
  vISA_ASSERT(i < srcs.size(), ERROR_INTERNAL_ARGUMENT);
  return srcs[i];
}

inline int G4_INST::getNumSrc() const {
  return isIntrinsic() ? asIntrinsicInst()->getNumSrc()
                       : G4_Inst_Table[op].n_srcs;
}

inline int G4_INST::getNumDst() const {
  return isIntrinsic() ? asIntrinsicInst()->getNumDst()
                       : G4_Inst_Table[op].n_dst;
}

inline bool G4_INST::isPseudoUse() const {
  return isIntrinsic() && asIntrinsicInst()->getIntrinsicId() == Intrinsic::Use;
}

inline bool G4_INST::isPseudoKill() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::PseudoKill;
}

inline bool G4_INST::isLifeTimeEnd() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::PseudoUse;
}

inline bool G4_INST::isSpillIntrinsic() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::Spill;
}

inline G4_SpillIntrinsic *G4_INST::asSpillIntrinsic() const {
  vISA_ASSERT(isSpillIntrinsic(), "not a spill intrinsic");
  return const_cast<G4_SpillIntrinsic *>(
      reinterpret_cast<const G4_SpillIntrinsic *>(this));
}

inline bool G4_INST::isFillIntrinsic() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::Fill;
}

inline G4_FillIntrinsic *G4_INST::asFillIntrinsic() const {
  vISA_ASSERT(isFillIntrinsic(), "not a fill intrinsic");
  return const_cast<G4_FillIntrinsic *>(
      reinterpret_cast<const G4_FillIntrinsic *>(this));
}

inline bool G4_INST::isPseudoAddrMovIntrinsic() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::PseudoAddrMov;
}

inline bool G4_INST::isSplitIntrinsic() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::Split;
}

inline bool G4_INST::isCallerSave() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::CallerSave;
}

inline bool G4_INST::isCallerRestore() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::CallerRestore;
}

inline bool G4_INST::isCalleeSave() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::CalleeSave;
}

inline bool G4_INST::isCalleeRestore() const {
  return isIntrinsic() &&
         asIntrinsicInst()->getIntrinsicId() == Intrinsic::CalleeRestore;
}

inline bool G4_INST::isRelocationMov() const {
  return isMov() && srcs[0]->isRelocImm();
}

inline const char *G4_INST::getLabelStr() const {
  vISA_ASSERT(srcs[0] && srcs[0]->isLabel(), ERROR_UNKNOWN);
  return srcs[0]->asLabel()->getLabelName();
}

inline bool G4_InstCF::isUniformGoto(unsigned KernelSimdSize) const {
  vASSERT(op == G4_goto);
  const G4_Predicate *pred = getPredicate();
  if (getExecSize() == g4::SIMD1 || pred == nullptr)
    return true;

  // This is uniform if group size equals to the kernel simd size.
  return pred->getPredCtrlGroupSize() == KernelSimdSize;
}

inline bool G4_InstCF::isIndirectJmp() const {
  return op == G4_jmpi && !srcs[0]->isLabel();
}

inline const char *G4_InstCF::getJipLabelStr() const {
  vISA_ASSERT(jip != NULL && jip->isLabel(), ERROR_UNKNOWN);
  return jip->asLabel()->getLabelName();
}

inline const char *G4_InstCF::getUipLabelStr() const {
  vISA_ASSERT(uip != NULL && uip->isLabel(), ERROR_UNKNOWN);
  return uip->asLabel()->getLabelName();
}

inline bool G4_InstCF::isIndirectCall() const {
  return op == G4_pseudo_fcall && !getSrc(0)->isLabel();
}

class G4_SpillIntrinsic : public G4_InstIntrinsic {
public:
  G4_SpillIntrinsic(const IR_Builder &builder, G4_Predicate *prd,
                    Intrinsic intrinId, G4_ExecSize execSize,
                    G4_DstRegRegion *d, G4_Operand *header, G4_Operand *payload,
                    G4_Operand *s2, G4_InstOpts opt)
      : G4_InstIntrinsic(builder, prd, intrinId, execSize, d, header, payload,
                         s2, opt) {}

  const static unsigned int InvalidOffset = 0xfffffffe;

  bool isOffBP() const { return getFP() != nullptr; }

  uint32_t getNumRows() const { return numRows; }
  uint32_t getOffset() const { return offset; }
  uint32_t getOffsetInBytes() const;
  G4_Declare *getFP() const { return fp; }
  G4_SrcRegRegion *getHeader() const { return getSrc(0)->asSrcRegRegion(); }
  G4_SrcRegRegion *getPayload() const { return getSrc(1)->asSrcRegRegion(); }

  void setNumRows(uint32_t r) { numRows = r; }
  void setOffset(uint32_t o) { offset = o; }
  void setFP(G4_Declare *f) { fp = f; }
  void setScatterSpill(bool b) { scatterSpill = b; }

  bool isOffsetValid() const { return offset != InvalidOffset; }
  bool isScatterSpill() const { return scatterSpill; }

  void computeRightBound(G4_Operand *opnd) override;

private:
  G4_Declare *fp = nullptr;
  uint32_t numRows = 0;
  uint32_t offset = InvalidOffset;
  bool scatterSpill = false;
};

class G4_PseudoAddrMovIntrinsic : public G4_InstIntrinsic {
public:
  G4_PseudoAddrMovIntrinsic(const IR_Builder &builder, Intrinsic intrinId,
                            G4_DstRegRegion *d, G4_Operand *s0, G4_Operand *s1,
                            G4_Operand *s2, G4_Operand *s3, G4_Operand *s4,
                            G4_Operand *s5, G4_Operand *s6, G4_Operand *s7)
      : G4_InstIntrinsic(builder, nullptr, intrinId, G4_ExecSize(1), d, s0, s1,
                         s2, s3, s4, s5, s6, s7, InstOpt_NoOpt) {}
};

class G4_FillIntrinsic : public G4_InstIntrinsic {
public:
  G4_FillIntrinsic(const IR_Builder &builder, G4_Predicate *prd,
                   Intrinsic intrinId, G4_ExecSize execSize, G4_DstRegRegion *d,
                   G4_Operand *header, G4_Operand *s1, G4_Operand *s2,
                   G4_InstOpts opt)
      : G4_InstIntrinsic(builder, prd, intrinId, execSize, d, header, s1, s2,
                         opt) {}

  const static unsigned int InvalidOffset = 0xfffffffe;

  bool isOffBP() const { return getFP() != nullptr; }

  uint32_t getNumRows() const { return numRows; }
  uint32_t getOffset() const { return offset; }
  uint32_t getOffsetInBytes() const;
  G4_Declare *getFP() const { return fp; }
  G4_SrcRegRegion *getHeader() const { return getSrc(0)->asSrcRegRegion(); }

  void setNumRows(uint32_t r) { numRows = r; }
  void setOffset(uint32_t o) { offset = o; }
  void setFP(G4_Declare *f) { fp = f; }

  bool isOffsetValid() const { return offset != InvalidOffset; }

  void computeRightBound(G4_Operand *opnd) override;

private:
  G4_Declare *fp = nullptr;
  uint32_t numRows = 0;
  uint32_t offset = InvalidOffset;
};

// Inlined functions for G4_Operand. We have to put them here rather than
// G4_Opernad header because they use G4_INST.
// FIXME: revisit the decision of lazy computing operand bound.
inline unsigned G4_Operand::getLeftBound() {
  // The default left bound does not take emask into account for flags.
  // Compute the right bound in which updates the left bound accordingly.
  if (isRightBoundSet() == false && !isNullReg()) {
    inst->computeRightBound(this);
  }
  return left_bound;
}

inline unsigned G4_Operand::getRightBound() {
  if (isRightBoundSet() == false && !isNullReg()) {
    inst->computeRightBound(this);
  }
  return right_bound;
}

inline uint64_t G4_Operand::getBitVecL() {
  if (isRightBoundSet() == false && !isNullReg()) {
    // computeRightBound also computes bitVec
    inst->computeRightBound(this);
  }
  return bitVec[0];
}

// TODO: This probably doesn't need to be inlined.
inline void G4_DstRegRegion::setType(const IR_Builder &builder, G4_Type ty) {
  bool recomputeLeftBound = false;

  if (TypeSize(type) != TypeSize(ty)) {
    unsetRightBound();
    recomputeLeftBound = true;
  }

  type = ty;

  if (recomputeLeftBound) {
    computeLeftBound(builder);

    if (getInst()) {
      getInst()->computeLeftBoundForImplAcc(getInst()->getImplAccDst());
      getInst()->computeLeftBoundForImplAcc(getInst()->getImplAccSrc());
    }
  }
}

inline bool G4_INST::writesFlag() const {
  return (mod && op != G4_sel) || (dst && dst->isFlag());
}

} // namespace vISA

#endif
