/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _REGALLOC_H_
#define _REGALLOC_H_
#include "BitSet.h"
#include "LinearScanRA.h"
#include "LocalRA.h"
#include "PhyRegUsage.h"
#include "llvm/ADT/SparseBitVector.h"

namespace vISA {

#if defined(_DEBUG) || !defined(DLL_MODE)
// Flag to control whether RA trace is on. This should be set to
// builder.getOption(vISA_RATrace) at RA entry.
extern bool RATraceFlag;
// Debug macro to dump register allocator traces
#define RA_TRACE(X)                                                            \
  do {                                                                         \
    if (::RATraceFlag) {                                                       \
      X;                                                                       \
    }                                                                          \
  }                                                                            \
  while (false)
#else
#define RA_TRACE(X)
#endif

class PointsToAnalysis;

struct VarRange {
  unsigned int leftBound;
  unsigned int rightBound;
};

typedef std::vector<VarRange *> VAR_RANGE_LIST;
typedef std::vector<VarRange *>::iterator VAR_RANGE_LIST_ITER;
typedef std::vector<VarRange *>::reverse_iterator VAR_RANGE_LIST_REV_ITER;

struct VarRangeListPackage {
  bool usedInSend;
  unsigned char rangeUnit;
  VAR_RANGE_LIST list;
};

class LivenessAnalysis {
  unsigned numVarId = 0;           // the var count
  unsigned numSplitVar = 0;        // the split var count
  unsigned numSplitStartID = 0;    // the split var count
  unsigned numUnassignedVarId = 0; // the unassigned var count
  unsigned numAddrId = 0;          // the addr count
  unsigned numBBId = 0;            // the block count
  unsigned numFnId = 0;            // the function count
  const unsigned char selectedRF =
      0; // the selected reg file kind for performing liveness
  const PointsToAnalysis &pointsToAnalysis;
  std::unordered_map<G4_Declare *, BitSet> neverDefinedRows;
  std::unordered_set<const G4_Declare *> defWriteEnable;

  void computeGenKillandPseudoKill(G4_BB *bb, llvm_SBitVector &def_out,
                                   llvm_SBitVector &use_in, llvm_SBitVector &use_gen,
                                   llvm_SBitVector &use_kill) const;

  bool contextFreeUseAnalyze(G4_BB *bb, bool isChanged);
  bool contextFreeDefAnalyze(G4_BB *bb, bool isChanged);

  bool livenessCandidate(const G4_Declare *decl, bool verifyRA) const;

  void dump_bb_vector(char *vname, std::vector<BitSet> &vec);
  void dump_fn_vector(char *vname, std::vector<FuncInfo *> &fns,
                      std::vector<BitSet> &vec);

  void updateKillSetForDcl(G4_Declare *dcl, llvm_SBitVector *curBBGen,
                           llvm_SBitVector *curBBKill, G4_BB *curBB,
                           llvm_SBitVector *entryBBGen, llvm_SBitVector *entryBBKill,
                           G4_BB *entryBB, unsigned scopeID);
  void footprintDst(const G4_BB *bb, const G4_INST *i, G4_Operand *opnd,
                    BitSet *dstfootprint) const;
  static void footprintSrc(const G4_INST *i, G4_Operand *opnd,
                           BitSet *srcfootprint);
  void detectNeverDefinedVarRows();
  void collectNoMaskVars();
  bool doesVarNeedNoMaskForKill(const G4_Declare* dcl) const;

public:
  GlobalRA &gra;
  std::vector<G4_RegVar *> vars;
  BitSet addr_taken;
  FlowGraph &fg;

  //
  // Bitsets used for data flow.
  //
  std::vector<llvm_SBitVector> def_in;
  std::vector<llvm_SBitVector> def_out;
  std::vector<llvm_SBitVector> use_in;
  std::vector<llvm_SBitVector> use_out;
  std::vector<llvm_SBitVector> use_gen;
  std::vector<llvm_SBitVector> use_kill;
  std::vector<llvm_SBitVector> indr_use;
  std::unordered_map<FuncInfo *, llvm_SBitVector> subroutineMaydef;

  // Hold variables known to be globals
  llvm::SparseBitVector<> globalVars;

  bool isLocalVar(G4_Declare *decl) const;
  bool setVarIDs(bool verifyRA, bool areAllPhyRegAssigned);
  LivenessAnalysis(GlobalRA &gra, unsigned char kind, bool verifyRA = false,
                   bool forceRun = false);
  ~LivenessAnalysis();
  void computeLiveness();
  bool isLiveAtEntry(const G4_BB *bb, unsigned var_id) const;
  bool isUseThrough(const G4_BB *bb, unsigned var_id) const;
  bool isDefThrough(const G4_BB *bb, unsigned var_id) const;
  bool isLiveAtExit(const G4_BB *bb, unsigned var_id) const;
  bool isUseOut(const G4_BB *bb, unsigned var_id) const;
  bool isUseIn(const G4_BB *bb, unsigned var_id) const;
  bool isAddressSensitive(
      unsigned num) const // returns true if the variable is address taken and
                          // also has indirect access
  {
    return addr_taken.isSet(num);
  }
  unsigned int getSelectedRF() const { return selectedRF; }
  static bool livenessClass(G4_RegFileKind kind1, G4_RegFileKind kind2) {
    return (kind1 & kind2) != 0;
  }
  bool livenessClass(G4_RegFileKind regKind) const {
    return livenessClass((G4_RegFileKind)selectedRF, regKind);
  }
  unsigned getNumSelectedVar() const { return numVarId; }
  unsigned getNumSelectedGlobalVar() const { return globalVars.count(); }
  unsigned getNumSplitVar() const { return numSplitVar; }
  unsigned getNumSplitStartID() const { return numSplitStartID; }
  unsigned getNumUnassignedVar() const { return numUnassignedVarId; }
  void dump() const;
  void dumpBB(G4_BB *bb) const;
  void dumpLive(BitSet &live) const;
  void dumpGlobalVarNum() const;
  void reportUndefinedUses() const;
  bool isEmptyLiveness() const;
  bool writeWholeRegion(const G4_BB *bb, const G4_INST *prd,
                        G4_DstRegRegion *dst, const Options *opt) const;

  bool writeWholeRegion(const G4_BB *bb, const G4_INST *prd,
                        const G4_VarBase *flagReg) const;

  void performScoping(llvm_SBitVector *curBBGen, llvm_SBitVector *curBBKill,
                      G4_BB *curBB, llvm_SBitVector *entryBBGen,
                      llvm_SBitVector *entryBBKill, G4_BB *entryBB);

  void hierarchicalIPA(const llvm_SBitVector &kernelInput,
                       const llvm_SBitVector &kernelOutput);
  void useAnalysis(FuncInfo *subroutine);
  void useAnalysisWithArgRetVal(
      FuncInfo *subroutine,
      const std::unordered_map<FuncInfo *, llvm_SBitVector> &args,
      const std::unordered_map<FuncInfo *, llvm_SBitVector> &retVal);
  void defAnalysis(FuncInfo *subroutine);
  void maydefAnalysis();

  const PointsToAnalysis &getPointsToAnalysis() const {
    return pointsToAnalysis;
  }

  bool performIPA() const {
    return fg.builder->getOption(vISA_IPA) && livenessClass(G4_GRF) &&
           fg.getNumCalls() > 0;
  }
};

//
// entry point of register allocation
//
class IR_Builder; // forward declaration

} // namespace vISA

int regAlloc(vISA::IR_Builder &builder, vISA::PhyRegPool &regPool,
             vISA::G4_Kernel &kernel);

#endif
