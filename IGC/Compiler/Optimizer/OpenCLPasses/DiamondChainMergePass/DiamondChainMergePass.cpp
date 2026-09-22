/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// DiamondChainMergePass is built against LLVM 22 only.  The CMake gate in
// ../CMakeLists.txt already keeps this file out of the build elsewhere; the
// guard below keeps the translation unit self-contained.
#if LLVM_VERSION_MAJOR == 22

#include "DiamondChainMergePass.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Uniformity.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include <algorithm>

using namespace llvm;

#define PASS_FLAG "diamond-chain-merge"
#define PASS_DESCRIPTION "Diamond chain merge optimization."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

namespace {

// Describes all repeated copies of a single if-else body that share the same
// branch condition and are candidates for inlining into the accumulator blocks
// via mergeBasicBlocks(DstTrue, DstFalse, TrueBB, FalseBB, MergeBB).
//
// DstTrue / DstFalse are shared across the whole group (first pair in the
// repeated chain).  TrueBlocks[i], FalseBlocks[i] and MergeBlocks[i] form the
// i-th source triple to inline; MergeBlocks[i] may be nullptr when the two
// paths branch directly to the continuation without a common merge block.
struct MergeCandidate {
  // Accumulator blocks - destination of every clone operation in this group.
  BasicBlock *DstTrue = nullptr;
  BasicBlock *DstFalse = nullptr;
  // Optional merge block of the accumulator (first) triple: the block where
  // DstTrue and DstFalse converge before the first source triple.  Its
  // non-PHI instructions must be cloned into DstTrue/DstFalse and its PHI
  // results must be entered into the cumulative value maps so subsequent
  // source triples can reference them.
  BasicBlock *DstMergeBB = nullptr;
  // Parallel lists of source blocks.  Index i holds the i-th repeated copy.
  SmallVector<BasicBlock *, 8> TrueBlocks;
  SmallVector<BasicBlock *, 8> FalseBlocks;
  SmallVector<BasicBlock *, 8> MergeBlocks; // entries may be nullptr
  // Per-source-triple orientation relative to accumulator pair.
  // true  => source triple orientation differs from accumulator orientation,
  //          so merge must flip destination paths to preserve semantics.
  // false => source orientation matches accumulator orientation.
  SmallVector<bool, 8> FlipDstForTriple;
};

// Returns true for branch conditions that are invalid or uninformative for
// grouping purposes (dead CFG regions often use these).
static bool isInvalidGroupingCond(Value *Cond) {
  return isa<PoisonValue>(Cond) || isa<UndefValue>(Cond) || isa<ConstantInt>(Cond);
}

// Returns true if BB is a valid triangle-sink: either it has a single
// predecessor, or it has exactly two predecessors one of which is BodyBB
// (the triangle body block that falls through to BB).
static bool isValidTriangleSink(BasicBlock *BB, BasicBlock *BodyBB) {
  if (BB->hasNPredecessors(1))
    return true;
  unsigned PredCount = 0;
  bool HasBodyPred = false;
  for (BasicBlock *Pred : predecessors(BB)) {
    ++PredCount;
    if (Pred == BodyBB)
      HasBodyPred = true;
  }
  return (PredCount == 2) && HasBodyPred;
}

// Returns the single unconditional successor of BB, or nullptr.
static BasicBlock *getUncondSucc(BasicBlock *BB) {
  if (auto *UB = dyn_cast<BranchInst>(BB->getTerminator()))
    if (UB->isUnconditional())
      return UB->getSuccessor(0);
  return nullptr;
}

// Clone all non-terminator instructions from From into Dst. PHI handling is
// delegated to HandlePHI, while non-PHI post-processing is done by
// HandleClonedInst.
template <typename PHIHandlerT, typename ClonedInstHandlerT>
static void cloneBodyWithoutTerminator(BasicBlock *From, BasicBlock *Dst, ValueToValueMapTy &VMap,
                                       SmallVectorImpl<Instruction *> &NewInsts, PHIHandlerT &&HandlePHI,
                                       ClonedInstHandlerT &&HandleClonedInst) {
  for (Instruction &Inst : *From) {
    if (Inst.isTerminator())
      continue;

    if (auto *PN = dyn_cast<PHINode>(&Inst)) {
      HandlePHI(*PN);
      continue;
    }

    Instruction *NewInst = Inst.clone();
    if (Instruction *DstTerm = Dst->getTerminator())
      NewInst->insertBefore(DstTerm->getIterator());
    else
      NewInst->insertInto(Dst, Dst->end());

    VMap[&Inst] = NewInst;
    HandleClonedInst(Inst, *NewInst);
    NewInsts.push_back(NewInst);
  }
}

class DiamondChainIfElseTracker {
  // Candidates found by findPatterns(); consumed by mergeBasicBlocks().
  SmallVector<MergeCandidate, 8> Candidates;
  DominatorTree &DT;
  OptimizationRemarkEmitter &ORE;
  Function &Func;

  static StringRef getBBDisplayName(const BasicBlock *BB);

  void emitGroupedCandidatesRemark(StringRef Stage);

  void emitRejectRemark(const MergeCandidate &C, size_t I, StringRef Reason, BasicBlock *TrueBB, BasicBlock *FalseBB,
                        BasicBlock *MergeBB);

public:
  DiamondChainIfElseTracker(DominatorTree &DT, OptimizationRemarkEmitter &ORE, Function &Func);

  SmallVector<MergeCandidate, 8> &getCandidates();

  bool hasAnySyncInst(BasicBlock *BB);

  bool findPatterns();

  void trimCandidateToPrefix(MergeCandidate &C, size_t SafeCount);

  bool basicShape(const MergeCandidate &C, size_t I, BasicBlock *TrueBB, BasicBlock *FalseBB, BasicBlock *MergeBB);

  bool mergePredCount(const MergeCandidate &C, size_t I, BasicBlock *TrueBB, BasicBlock *FalseBB, BasicBlock *MergeBB);

  bool mergeBlockBarrierWithDisjointCond(const MergeCandidate &C, size_t I, BasicBlock *TrueBB, BasicBlock *FalseBB,
                                         BasicBlock *MergeBB);

  bool hasMixedConditionInSource(const MergeCandidate &C, size_t I, BasicBlock *TrueBB, BasicBlock *FalseBB,
                                 BasicBlock *MergeBB);

  bool chainContinuity(const MergeCandidate &C, size_t I, BasicBlock *TrueBB, BasicBlock *FalseBB, BasicBlock *MergeBB);

  bool hasEscapingDef(const MergeCandidate &C, size_t I, BasicBlock *DefBB, BasicBlock *TrueBB, BasicBlock *FalseBB,
                      BasicBlock *MergeBB);

  bool noEscapingDefs(const MergeCandidate &C, size_t I, BasicBlock *TrueBB, BasicBlock *FalseBB, BasicBlock *MergeBB);

  static bool hasVolatileOrAtomic(BasicBlock *BB, bool AllowOrdinaryCalls = false);

  bool noVolatileOrAtomic(const MergeCandidate &C, size_t I, BasicBlock *TrueBB, BasicBlock *FalseBB,
                          BasicBlock *MergeBB);

  bool isLiveInAvailableAt(const MergeCandidate &C, BasicBlock *DefBB, BasicBlock *ReqDst);

  bool hasNonDominatingLiveIn(const MergeCandidate &C, size_t I, BasicBlock *BB, BasicBlock *TrueBB,
                              BasicBlock *FalseBB, BasicBlock *MergeBB, BasicBlock *ReqDst1, BasicBlock *ReqDst2);

  bool noNonDominatingLiveIns(const MergeCandidate &C, size_t I, BasicBlock *TrueBB, BasicBlock *FalseBB,
                              BasicBlock *MergeBB);

  bool shouldResolvePHI(PHINode *PN, BasicBlock *TrueBB, BasicBlock *FalseBB);

  bool verifySingleCandidate(MergeCandidate &C);

  void verifyCandidates();

  bool tryMergeWholeTriangleChain(MergeCandidate &C, SmallVector<BasicBlock *, 32> &BlocksToRemove);

  bool mergePathIntoBlock(BasicBlock *Dst, BasicBlock *Src, BasicBlock *SrcPhiIncoming, BasicBlock *TailBB,
                          BasicBlock *TailPhiIncoming, BasicBlock *TrueBB, BasicBlock *FalseBB,
                          ValueToValueMapTy &CumulativeVMap, DenseMap<Value *, Value *> &SourceToDestMap);

  bool mergeBasicBlocks(BasicBlock *DstTrue, BasicBlock *DstFalse, BasicBlock *TrueBB, BasicBlock *FalseBB,
                        BasicBlock *MergeBB, ValueToValueMapTy &CumulativeTrueVMap,
                        ValueToValueMapTy &CumulativeFalseVMap, SmallVector<BasicBlock *, 32> &BlocksToRemove);

  bool cloneDstMergeBB(BasicBlock *DstTrue, BasicBlock *DstFalse, BasicBlock *DstMergeBB,
                       ValueToValueMapTy &CumulativeTrueVMap, ValueToValueMapTy &CumulativeFalseVMap,
                       SmallVector<BasicBlock *, 32> &BlocksToRemove);

  bool cloneAccumulatorTriangleTailBB(BasicBlock *DstTrue, BasicBlock *DstFalse, ValueToValueMapTy &CumulativeTrueVMap,
                                      ValueToValueMapTy &CumulativeFalseVMap);

  bool repairEscapedRegionDefs(const MergeCandidate &C, ValueToValueMapTy &CumulativeTrueVMap,
                               ValueToValueMapTy &CumulativeFalseVMap, ArrayRef<BasicBlock *> BlocksToRemove);

  void removeCollectedBlocks(SmallVector<BasicBlock *, 32> &BlocksToRemove);

  bool repairSSAAfterMerge();
};

StringRef DiamondChainIfElseTracker::getBBDisplayName(const BasicBlock *BB) {
  if (!BB)
    return "<null>";
  if (!BB->hasName())
    return "<unnamed>";
  return BB->getName();
}

void DiamondChainIfElseTracker::emitGroupedCandidatesRemark(StringRef Stage) {
  ORE.emit([&]() {
    return OptimizationRemarkAnalysis("diamond-chain-merge", "GroupedCandidates", DebugLoc(), &Func.getEntryBlock())
           << "stage=" << ore::NV("Stage", Stage)
           << ", count=" << ore::NV("Count", static_cast<uint64_t>(Candidates.size()));
  });

  for (size_t CandidateIdx = 0; CandidateIdx < Candidates.size(); ++CandidateIdx) {
    const MergeCandidate &C = Candidates[CandidateIdx];
    ORE.emit([&]() {
      return OptimizationRemarkAnalysis("diamond-chain-merge", "GroupedCandidateDetail", DebugLoc(),
                                        C.DstTrue ? C.DstTrue : &Func.getEntryBlock())
             << "candidate=" << ore::NV("Index", static_cast<uint64_t>(CandidateIdx))
             << ", dstTrue=" << ore::NV("DstTrue", getBBDisplayName(C.DstTrue))
             << ", dstFalse=" << ore::NV("DstFalse", getBBDisplayName(C.DstFalse))
             << ", dstMerge=" << ore::NV("DstMerge", getBBDisplayName(C.DstMergeBB))
             << ", groups=" << ore::NV("Groups", static_cast<uint64_t>(C.TrueBlocks.size()));
    });
  }
}

void DiamondChainIfElseTracker::emitRejectRemark(const MergeCandidate &C, size_t I, StringRef Reason,
                                                 BasicBlock *TrueBB, BasicBlock *FalseBB, BasicBlock *MergeBB) {
  BasicBlock *RemarkBB = TrueBB ? TrueBB : &Func.getEntryBlock();
  ORE.emit([&]() {
    return OptimizationRemarkMissed("diamond-chain-merge", "CandidateRejected", DebugLoc(), RemarkBB)
           << "candidate " << ore::NV("Index", static_cast<uint64_t>(I)) << " rejected: " << ore::NV("Reason", Reason)
           << ", dstTrue=" << ore::NV("DstTrue", C.DstTrue ? C.DstTrue->getName() : "<null>")
           << ", dstFalse=" << ore::NV("DstFalse", C.DstFalse ? C.DstFalse->getName() : "<null>")
           << ", true=" << ore::NV("True", TrueBB ? TrueBB->getName() : "<null>")
           << ", false=" << ore::NV("False", FalseBB ? FalseBB->getName() : "<null>")
           << ", merge=" << ore::NV("Merge", MergeBB ? MergeBB->getName() : "<null>");
  });
}

DiamondChainIfElseTracker::DiamondChainIfElseTracker(DominatorTree &DT, OptimizationRemarkEmitter &ORE, Function &Func)
    : DT(DT), ORE(ORE), Func(Func) {}

SmallVector<MergeCandidate, 8> &DiamondChainIfElseTracker::getCandidates() { return Candidates; }

// Check for synchronization instructions that prevent merging
bool DiamondChainIfElseTracker::hasAnySyncInst(BasicBlock *BB) {
  for (auto &Inst : *BB) {
    if (const IntrinsicInst *Intrinsic = dyn_cast<IntrinsicInst>(&Inst)) {
      switch (Intrinsic->getIntrinsicID()) {
      case GenISAIntrinsic::GenISA_threadgroupbarrier:
      case GenISAIntrinsic::GenISA_threadgroupbarrier_signal:
      case GenISAIntrinsic::GenISA_threadgroupbarrier_wait:
      case GenISAIntrinsic::GenISA_threadgroupnamedbarriers_signal:
      case GenISAIntrinsic::GenISA_threadgroupnamedbarriers_wait:
      case GenISAIntrinsic::GenISA_wavebarrier:
        return true;
      }
    }
  }
  return false;
}

static bool hasAnyConvergentInst(BasicBlock *BB) {
  for (Instruction &Inst : *BB) {
    if (auto *CB = dyn_cast<CallBase>(&Inst)) {
      if (CB->isConvergent())
        return true;
    }
  }
  return false;
}

// Scans Func for groups of conditional branches that share the same i1
// condition value.  For every such group the first branch provides the
// accumulator blocks (DstTrue / DstFalse) and every subsequent branch in
// the group provides the source blocks (TrueBB / FalseBB) together with
// an optional common merge block (MergeBB) when both paths converge
// unconditionally before continuing.  Validated candidates are stored in
// Candidates for later use by mergeBasicBlocks().
bool DiamondChainIfElseTracker::findPatterns() {
  // Group CondBr instructions by their i1 operand.  Iterating the function
  // in IR order preserves the natural top-down block ordering.
  DenseMap<Value *, SmallVector<BranchInst *, 8>> CondBrsByValue;
  for (BasicBlock &BB : Func) {
    // Ignore dead blocks: they frequently contain synthetic poison-based
    // branches that are not valid merge candidates.
    if (!DT.isReachableFromEntry(&BB))
      continue;

    if (auto *BI = dyn_cast<BranchInst>(BB.getTerminator())) {
      if (!BI->isConditional())
        continue;
      Value *Cond = BI->getCondition();
      if (isInvalidGroupingCond(Cond))
        continue;
      CondBrsByValue[Cond].push_back(BI);
    }
  }

  // Canonicalize branch successors so we consistently handle both triangle
  // orientations:
  //   (A) Succ0 -> Succ1  (already preferred)
  //   (B) Succ1 -> Succ0  (swap to normalize)
  // This keeps verifyCandidates()/mergeBasicBlocks() logic in one shape.
  auto CanonicalizePair = [&](BasicBlock *&Succ0, BasicBlock *&Succ1) {
    bool Swapped = false;
    BasicBlock *Succ0Next = getUncondSucc(Succ0);
    BasicBlock *Succ1Next = getUncondSucc(Succ1);
    if (Succ1Next == Succ0 && Succ0Next != Succ1) {
      std::swap(Succ0, Succ1);
      Swapped = true;
    }
    return Swapped;
  };

  for (auto &[CondVal, BrList] : CondBrsByValue) {
    // Need at least two branches with the same condition to have anything
    // to merge.
    if (BrList.size() < 2)
      continue;

    // The first branch in program order supplies the accumulator blocks.
    // All subsequent entries in BrList are source copies to inline into them.
    BranchInst *FirstBr = BrList[0];
    BasicBlock *DstTrue = FirstBr->getSuccessor(0);
    BasicBlock *DstFalse = FirstBr->getSuccessor(1);
    bool DstSwapped = CanonicalizePair(DstTrue, DstFalse);

    // Be conservative for accumulator blocks. DstTrue must have a unique
    // predecessor, while DstFalse may either have one predecessor or be the
    // triangle sink reached from DstTrue.
    if (!DstTrue->getSinglePredecessor())
      continue;
    if (!isValidTriangleSink(DstFalse, DstTrue))
      continue;

    // Accumulator blocks must not contain synchronisation instructions
    // because we will be prepending more code into them.  Likewise, any
    // convergent operation inside the region being duplicated would be forced
    // onto a narrower path-specific control-flow mask when we clone the body,
    // which is not legal for convergent instructions like shuffles.
    if (hasAnySyncInst(DstTrue) || hasAnySyncInst(DstFalse) || hasAnyConvergentInst(DstTrue) ||
        hasAnyConvergentInst(DstFalse))
      continue;

    // One MergeCandidate per unique (DstTrue, DstFalse) pair; we collect
    // all source triples (TrueBB, FalseBB, MergeBB) into its lists.
    MergeCandidate Candidate;
    Candidate.DstTrue = DstTrue;
    Candidate.DstFalse = DstFalse;

    // Detect the merge block of the accumulator triple: the block where
    // DstTrue and DstFalse both branch unconditionally to the same target.
    BasicBlock *DstTrueSucc = getUncondSucc(DstTrue);
    BasicBlock *DstFalseSucc = getUncondSucc(DstFalse);
    if (DstTrueSucc && DstFalseSucc && DstTrueSucc == DstFalseSucc)
      Candidate.DstMergeBB = DstTrueSucc;

    // Only merge directly adjacent triples. Once a same-condition branch
    // appears outside the current CFG continuation, stop growing this chain
    // instead of reaching across unrelated control flow.
    BasicBlock *ExpectedBranchBB = Candidate.DstMergeBB;
    if (!ExpectedBranchBB) {
      if (auto *Succ = getUncondSucc(DstTrue))
        ExpectedBranchBB = Succ;
      else if (auto *Succ = getUncondSucc(DstFalse))
        ExpectedBranchBB = Succ;
    }

    for (size_t I = 1; I < BrList.size(); ++I) {
      BranchInst *CurBr = BrList[I];
      // If we cannot determine an expected continuation block (e.g. because
      // both DstTrue and DstFalse end with conditional branches rather than
      // unconditional ones), we have no way to verify that the next same-
      // condition branch is directly adjacent.  Accepting it would reach
      // across arbitrary control flow - potentially skipping blocks with
      // multiple predecessors or additional merge-block exit edges - and
      // produce an incorrect transformation.  Stop the chain here.
      if (!ExpectedBranchBB)
        break;
      if (CurBr->getParent() != ExpectedBranchBB)
        break;

      BasicBlock *TrueBB = CurBr->getSuccessor(0);
      BasicBlock *FalseBB = CurBr->getSuccessor(1);
      bool SrcSwapped = CanonicalizePair(TrueBB, FalseBB);

      if (!TrueBB || !FalseBB)
        continue;

      // Keep source shape aligned with verifyCandidates(): TrueBB must be
      // single-predecessor, while FalseBB may be a triangle sink.
      if (!TrueBB->getSinglePredecessor())
        continue;
      if (!isValidTriangleSink(FalseBB, TrueBB))
        continue;

      // Source blocks must differ from the accumulator blocks.
      if (TrueBB == DstTrue || FalseBB == DstFalse)
        continue;

      // Source blocks must not contain synchronisation instructions or
      // convergent operations.  Cloning a convergent shuffle / subgroup op
      // across the true/false split changes the lane mask visible to each
      // duplicated copy and violates the semantics of a convergent call.
      if (hasAnySyncInst(TrueBB) || hasAnySyncInst(FalseBB) || hasAnyConvergentInst(TrueBB) ||
          hasAnyConvergentInst(FalseBB))
        continue;

      // Detect an optional common merge block: both TrueBB and FalseBB
      // branch unconditionally to the same successor.
      BasicBlock *MergeBB = nullptr;
      BasicBlock *TrueSucc = getUncondSucc(TrueBB);
      BasicBlock *FalseSucc = getUncondSucc(FalseBB);
      if (TrueSucc && FalseSucc && TrueSucc == FalseSucc) {
        MergeBB = TrueSucc;
        // Skip patterns where the merge block contains synchronisation
        // instructions or convergent operations.  We cannot safely clone a
        // convergent subgroup op into both path-specific copies of the
        // diamond.
        if (hasAnySyncInst(MergeBB) || hasAnyConvergentInst(MergeBB))
          continue;
      }

      Candidate.TrueBlocks.push_back(TrueBB);
      Candidate.FalseBlocks.push_back(FalseBB);
      Candidate.MergeBlocks.push_back(MergeBB);
      Candidate.FlipDstForTriple.push_back(SrcSwapped != DstSwapped);

      ExpectedBranchBB = MergeBB;
      if (!ExpectedBranchBB) {
        if (auto *Succ = getUncondSucc(TrueBB))
          ExpectedBranchBB = Succ;
        else if (auto *Succ = getUncondSucc(FalseBB))
          ExpectedBranchBB = Succ;
      }
    }

    if (!Candidate.TrueBlocks.empty())
      Candidates.push_back(std::move(Candidate));
  }
  return !Candidates.empty();
}

// Trims one candidate to the first SafeCount triples, preserving parallel
// indexing across True/False/Merge block vectors.
void DiamondChainIfElseTracker::trimCandidateToPrefix(MergeCandidate &C, size_t SafeCount) {
  C.TrueBlocks.resize(SafeCount);
  C.FalseBlocks.resize(SafeCount);
  C.MergeBlocks.resize(SafeCount);
  C.FlipDstForTriple.resize(SafeCount);
}

// Check 1: basic per-triple CFG shape.
// - True block must have a single predecessor.
// - False block must either have a single predecessor or allow the
//   canonical backedge/cross-edge from TrueBB.
bool DiamondChainIfElseTracker::basicShape(const MergeCandidate &C, size_t I, BasicBlock *TrueBB, BasicBlock *FalseBB,
                                           BasicBlock *MergeBB) {
  if (!TrueBB->getSinglePredecessor()) {
    emitRejectRemark(C, I, "check1.true-single-pred", TrueBB, FalseBB, MergeBB);
    return false;
  }

  if (!FalseBB->getSinglePredecessor() && !is_contained(predecessors(FalseBB), TrueBB)) {
    emitRejectRemark(C, I, "check1.false-pred-shape", TrueBB, FalseBB, MergeBB);
    return false;
  }

  return true;
}

// Check 2: if a merge block exists, it must be a strict 2-way join of this
// triple only (exactly TrueBB and FalseBB as predecessors).
bool DiamondChainIfElseTracker::mergePredCount(const MergeCandidate &C, size_t I, BasicBlock *TrueBB,
                                               BasicBlock *FalseBB, BasicBlock *MergeBB) {
  if (!MergeBB)
    return true;

  unsigned PredCount = 0;
  bool HasTrue = false;
  bool HasFalse = false;
  for (BasicBlock *Pred : predecessors(MergeBB)) {
    ++PredCount;
    if (Pred == TrueBB)
      HasTrue = true;
    if (Pred == FalseBB)
      HasFalse = true;
  }
  if (PredCount != 2 || !HasTrue || !HasFalse) {
    emitRejectRemark(C, I, "check2.merge-pred-count", TrueBB, FalseBB, MergeBB);
    return false;
  }

  return true;
}

// Check 2: Merge block must branch on the same condition as the diamond.
// If merge block branches on a different/external condition, we have disjoint
// control flow that would break SSA dominance when inlining merge block code.
// Example of unsafe pattern:
//   entry: %cond1 = icmp... ; br i1 %cond1
//   true1: %cond2 = icmp... ; br i1 %cond2 -> true2/false2 (diamond)
//   true2 (merge): br i1 %cond1 <- DISJOINT! %cond2 and %cond1 are different
// This causes loads in merge block to not dominate both destination blocks.
bool DiamondChainIfElseTracker::mergeBlockBarrierWithDisjointCond(const MergeCandidate &C, size_t I, BasicBlock *TrueBB,
                                                                  BasicBlock *FalseBB, BasicBlock *MergeBB) {
  // If there's no merge block, there's no issue.
  if (!MergeBB)
    return true;

  Instruction *MergeTerm = MergeBB->getTerminator();
  if (!MergeTerm || MergeTerm->getNumSuccessors() != 2)
    return true; // Not a conditional branch

  // For a conditional branch, the condition is operand 0
  Value *MergeCond = MergeTerm->getOperand(0);
  if (!MergeCond)
    return true;

  // Get the condition from the current triple (TrueBB or FalseBB should have
  // a branch that selects between them). This is the "active" condition for
  // the diamond we're considering.
  Value *TripleCond = nullptr;

  // The triple's condition comes from the predicate that selects TrueBB vs
  // FalseBB. Look at all predecessors of TrueBB to find the branch condition.
  for (BasicBlock *Pred : predecessors(TrueBB)) {
    Instruction *PredTerm = Pred->getTerminator();
    if (!PredTerm || PredTerm->getNumSuccessors() != 2)
      continue; // Not a conditional branch

    if (PredTerm->getSuccessor(0) == TrueBB || PredTerm->getSuccessor(1) == TrueBB) {
      TripleCond = PredTerm->getOperand(0);
      break;
    }
  }

  // If the merge block's condition is different from the triple's condition,
  // we have disjoint control flow. When we inline merge block code, the load
  // in the merge block would be executed before branching on a different
  // condition, violating SSA: the load wouldn't dominate both branch targets.
  if (TripleCond && MergeCond != TripleCond) {
    emitRejectRemark(C, I, "check2.disjoint-merge-cond", TrueBB, FalseBB, MergeBB);
    return false;
  }

  // Condition reuse in chain (same condition throughout) is safe:
  // all merge blocks use the same predicate, so the transformation maintains
  // correct dominance relationships.
  return true;
}

// Reject a candidate when the source blocks being inlined branch on a different
// predicate than the condition that selected this candidate.  This catches the
// real regression without rejecting valid downstream continuations, whose
// branch conditions live in merge blocks outside the current inlined source.
bool DiamondChainIfElseTracker::hasMixedConditionInSource(const MergeCandidate &C, size_t I, BasicBlock *TrueBB,
                                                          BasicBlock *FalseBB, BasicBlock *MergeBB) {
  auto FindCondFromPreds = [&](BasicBlock *BB) -> Value * {
    for (BasicBlock *Pred : predecessors(BB)) {
      auto *PredBr = dyn_cast<BranchInst>(Pred->getTerminator());
      if (!PredBr || !PredBr->isConditional())
        continue;
      if (PredBr->getSuccessor(0) == BB || PredBr->getSuccessor(1) == BB)
        return PredBr->getCondition();
    }
    return nullptr;
  };

  Value *ChainCond = FindCondFromPreds(TrueBB);
  if (!ChainCond)
    ChainCond = FindCondFromPreds(FalseBB);
  if (!ChainCond)
    return false;

  auto RejectIfMixed = [&](BasicBlock *BB, StringRef Reason) -> bool {
    // Only a source-arm body is unsafe to inline when it switches to a
    // different predicate.  Downstream merge blocks and continuation blocks are
    // valid follow-on control flow; they are not the body we are merging.
    if (!BB || !BB->getSinglePredecessor())
      return false;

    auto *Br = dyn_cast<BranchInst>(BB->getTerminator());
    if (!Br || !Br->isConditional())
      return false;
    if (Br->getCondition() == ChainCond)
      return false;
    emitRejectRemark(C, I, Reason, TrueBB, FalseBB, MergeBB);
    return true;
  };

  return RejectIfMixed(TrueBB, "check2.mixed-true-condition") || RejectIfMixed(FalseBB, "check2.mixed-false-condition");
}

// Check 3: chain continuity.
// The next triple must start where this triple ends so that prefix trimming
// remains semantically valid and leaves no gaps in the transformed chain.
bool DiamondChainIfElseTracker::chainContinuity(const MergeCandidate &C, size_t I, BasicBlock *TrueBB,
                                                BasicBlock *FalseBB, BasicBlock *MergeBB) {
  if (I + 1 >= C.TrueBlocks.size())
    return true;

  BasicBlock *NextTruePred = C.TrueBlocks[I + 1]->getSinglePredecessor();
  BasicBlock *ExpectedLink = MergeBB;
  if (!ExpectedLink) {
    if (auto *Succ = TrueBB->getSingleSuccessor())
      ExpectedLink = Succ;
    else if (auto *Succ = FalseBB->getSingleSuccessor())
      ExpectedLink = Succ;
  }

  if (!NextTruePred || NextTruePred != ExpectedLink) {
    emitRejectRemark(C, I, "check3.chain-continuity", TrueBB, FalseBB, MergeBB);
    return false;
  }

  return true;
}

// Returns true when BB belongs to the CFG region owned by C: the accumulator
// blocks plus every block of every source triple.  Everything else is "after"
// (or unrelated to) the chain.
static bool isChainBlock(const MergeCandidate &C, BasicBlock *BB) {
  if (BB == C.DstTrue || BB == C.DstFalse || BB == C.DstMergeBB)
    return true;
  return is_contained(C.TrueBlocks, BB) || is_contained(C.FalseBlocks, BB) || is_contained(C.MergeBlocks, BB);
}

// Returns true when DefBB contains a definition that is used outside the
// current triple and outside the set of explicitly allowed subsequent blocks.
bool DiamondChainIfElseTracker::hasEscapingDef(const MergeCandidate &C, size_t I, BasicBlock *DefBB, BasicBlock *TrueBB,
                                               BasicBlock *FalseBB, BasicBlock *MergeBB) {
  bool AllowSubsequentTrue = DefBB == TrueBB || DefBB == MergeBB;
  bool AllowSubsequentFalse = DefBB == FalseBB || DefBB == MergeBB;

  for (Instruction &Inst : *DefBB) {
    if (Inst.isTerminator())
      continue;

    for (Use &U : Inst.uses()) {
      auto *UserI = dyn_cast<Instruction>(U.getUser());
      if (!UserI)
        continue;
      BasicBlock *UserBB = UserI->getParent();

      if (UserBB == TrueBB || UserBB == FalseBB || UserBB == MergeBB || UserBB == C.DstTrue || UserBB == C.DstFalse)
        continue;

      bool InSubsequentTriple = false;
      for (size_t J = I + 1; J < C.TrueBlocks.size(); ++J) {
        // Preserve path semantics across the chain.
        // True-path defs are not considered safe in subsequent false blocks,
        // and false-path defs are not considered safe in subsequent true
        // blocks.
        if ((AllowSubsequentTrue && UserBB == C.TrueBlocks[J]) ||
            (AllowSubsequentFalse && UserBB == C.FalseBlocks[J]) || UserBB == C.MergeBlocks[J]) {
          InSubsequentTriple = true;
          break;
        }
      }
      if (InSubsequentTriple)
        continue;

      // A use *past* the chain is not automatically unsafe: it may belong to a
      // common continuation both paths join before.  The legality hinge is not
      // dominance alone; the opposite path must also be able to reach that
      // continuation, which is the pattern for the valid tail/merge repair.
      if (!isChainBlock(C, UserBB)) {
        auto Reaches = [&](BasicBlock *Start, BasicBlock *Target) {
          SmallPtrSet<BasicBlock *, 16> Visited;
          SmallVector<BasicBlock *, 8> Worklist{Start};
          while (!Worklist.empty()) {
            BasicBlock *BB = Worklist.pop_back_val();
            if (!BB || !Visited.insert(BB).second)
              continue;
            if (BB == Target)
              return true;
            for (BasicBlock *Succ : successors(BB))
              Worklist.push_back(Succ);
          }
          return false;
        };

        if ((DefBB == TrueBB && Reaches(FalseBB, UserBB)) || (DefBB == FalseBB && Reaches(TrueBB, UserBB)) ||
            (MergeBB &&
             ((DefBB == TrueBB && Reaches(FalseBB, UserBB)) || (DefBB == FalseBB && Reaches(TrueBB, UserBB)) ||
              (DefBB == MergeBB && (Reaches(TrueBB, UserBB) || Reaches(FalseBB, UserBB))))))
          continue;

        if (DT.dominates(TrueBB, UserBB) && DT.dominates(FalseBB, UserBB))
          continue;
        if (MergeBB && DT.dominates(TrueBB, UserBB) && DT.dominates(MergeBB, UserBB))
          continue;
        if (MergeBB && DT.dominates(FalseBB, UserBB) && DT.dominates(MergeBB, UserBB))
          continue;

        SmallVector<BasicBlock *, 4> Preds(pred_begin(UserBB), pred_end(UserBB));
        if (Preds.size() > 1) {
          bool SeenChainPred = false;
          bool SeenOtherPred = false;
          for (BasicBlock *Pred : Preds) {
            if (Pred == C.DstTrue || Pred == C.DstFalse || Pred == C.DstMergeBB)
              continue;
            if (Pred == TrueBB || Pred == FalseBB || Pred == MergeBB || is_contained(C.TrueBlocks, Pred) ||
                is_contained(C.FalseBlocks, Pred) || is_contained(C.MergeBlocks, Pred)) {
              SeenChainPred = true;
              continue;
            }
            SeenOtherPred = true;
          }
          if (SeenChainPred && SeenOtherPred)
            continue;
        }

        bool SeenTruePred = false;
        bool SeenFalsePred = false;

        for (BasicBlock *Pred : predecessors(UserBB)) {
          if (Pred == C.DstTrue || Pred == C.DstFalse || Pred == C.DstMergeBB)
            continue;

          if (Pred == MergeBB || is_contained(C.MergeBlocks, Pred)) {
            SeenTruePred = true;
            SeenFalsePred = true;
            continue;
          }

          if (Pred == TrueBB || is_contained(C.TrueBlocks, Pred))
            SeenTruePred = true;
          if (Pred == FalseBB || is_contained(C.FalseBlocks, Pred))
            SeenFalsePred = true;
        }

        if (SeenTruePred && SeenFalsePred)
          continue;

        ORE.emit([&]() {
          return OptimizationRemarkAnalysis("diamond-chain-merge", "EscapingDef", DebugLoc(), UserBB)
                 << "value from " << ore::NV("DefBB", getBBDisplayName(DefBB)) << " escapes to "
                 << ore::NV("UserBB", getBBDisplayName(UserBB));
        });
        return true;
      }

      ORE.emit([&]() {
        return OptimizationRemarkAnalysis("diamond-chain-merge", "EscapingDef", DebugLoc(), UserBB)
               << "value from " << ore::NV("DefBB", getBBDisplayName(DefBB)) << " escapes to "
               << ore::NV("UserBB", getBBDisplayName(UserBB));
      });
      return true;
    }
  }

  return false;
}

// Check 4: defs from True/False/Merge blocks must not escape into unrelated
// blocks. Subsequent triples are allowed only in path-consistent direction.
bool DiamondChainIfElseTracker::noEscapingDefs(const MergeCandidate &C, size_t I, BasicBlock *TrueBB,
                                               BasicBlock *FalseBB, BasicBlock *MergeBB) {
  if (hasEscapingDef(C, I, TrueBB, TrueBB, FalseBB, MergeBB) ||
      hasEscapingDef(C, I, FalseBB, TrueBB, FalseBB, MergeBB) ||
      (MergeBB && hasEscapingDef(C, I, MergeBB, TrueBB, FalseBB, MergeBB))) {
    emitRejectRemark(C, I, "check4.escaping-def", TrueBB, FalseBB, MergeBB);
    return false;
  }

  return true;
}

bool DiamondChainIfElseTracker::hasVolatileOrAtomic(BasicBlock *BB, bool AllowOrdinaryCalls) {
  for (Instruction &I : *BB) {
    if (I.isVolatile())
      return true;

    if (isa<CallBase>(&I)) {
      if (AllowOrdinaryCalls)
        continue;
      return true;
    }

    if (I.isAtomic()) {
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        if (isStrongerThan(LI->getOrdering(), AtomicOrdering::Monotonic))
          return true;
        continue;
      }

      if (auto *SI = dyn_cast<StoreInst>(&I)) {
        if (isStrongerThan(SI->getOrdering(), AtomicOrdering::Monotonic))
          return true;
        continue;
      }

      if (auto *RMW = dyn_cast<AtomicRMWInst>(&I)) {
        if (isStrongerThan(RMW->getOrdering(), AtomicOrdering::Monotonic))
          return true;
        continue;
      }

      if (auto *CmpXchg = dyn_cast<AtomicCmpXchgInst>(&I)) {
        if (isStrongerThan(CmpXchg->getSuccessOrdering(), AtomicOrdering::Monotonic) ||
            isStrongerThan(CmpXchg->getFailureOrdering(), AtomicOrdering::Monotonic))
          return true;
        continue;
      }

      // Unknown atomic instruction kind: stay conservative.
      return true;
    }

    if (I.isFenceLike())
      return true;
  }
  return false;
}

// Check 5: reject triples containing volatile ops, strong atomics, fences,
// or other side effects we cannot safely duplicate/reorder while merging.
bool DiamondChainIfElseTracker::noVolatileOrAtomic(const MergeCandidate &C, size_t I, BasicBlock *TrueBB,
                                                   BasicBlock *FalseBB, BasicBlock *MergeBB) {
  // In a triangle (MergeBB == null), FalseBB is the sink reached from both
  // sides of the condition (entry->FalseBB and entry->TrueBB->FalseBB).
  // Calls there are sink-style and are treated similarly to calls in MergeBB.
  // Calls in TrueBB remain path-local and therefore stay disallowed.
  bool AllowCallsInTriangleSink = !MergeBB && is_contained(predecessors(FalseBB), TrueBB);

  if (hasVolatileOrAtomic(TrueBB, false) || hasVolatileOrAtomic(FalseBB, AllowCallsInTriangleSink) ||
      (MergeBB && hasVolatileOrAtomic(MergeBB, true))) {
    emitRejectRemark(C, I, "check5.volatile-atomic", TrueBB, FalseBB, MergeBB);
    return false;
  }
  return true;
}

// Check 6: every value used by the source triple that is defined outside
// the triple must be available at the accumulator block(s) it gets hoisted
// into (see mergeBasicBlocks()); TrueBB/FalseBB need their own path's
// destination, MergeBB is cloned into both.
bool DiamondChainIfElseTracker::isLiveInAvailableAt(const MergeCandidate &C, BasicBlock *DefBB, BasicBlock *ReqDst) {
  if (DT.dominates(DefBB, ReqDst))
    return true;

  if (DefBB == C.DstMergeBB)
    return true;

  if (DefBB == C.DstFalse && ReqDst == C.DstTrue) {
    auto *Br = dyn_cast_or_null<BranchInst>(C.DstTrue->getTerminator());
    if (Br && Br->isUnconditional() && Br->getSuccessor(0) == C.DstFalse)
      return true;
  }

  return false;
}

bool DiamondChainIfElseTracker::hasNonDominatingLiveIn(const MergeCandidate &C, size_t I, BasicBlock *BB,
                                                       BasicBlock *TrueBB, BasicBlock *FalseBB, BasicBlock *MergeBB,
                                                       BasicBlock *ReqDst1, BasicBlock *ReqDst2) {
  for (Instruction &Inst : *BB) {
    if (isa<PHINode>(Inst))
      continue;

    for (Use &U : Inst.operands()) {
      auto *OpI = dyn_cast<Instruction>(U.get());
      if (!OpI)
        continue;

      BasicBlock *DefBB = OpI->getParent();
      if (DefBB == TrueBB || DefBB == FalseBB || DefBB == MergeBB)
        continue; // Defined in this triple: cloned alongside Inst.

      bool DefinedByEarlierTriple = false;
      for (size_t J = 0; J < I; ++J) {
        if (DefBB == C.TrueBlocks[J] || DefBB == C.FalseBlocks[J] || DefBB == C.MergeBlocks[J]) {
          DefinedByEarlierTriple = true;
          break;
        }
      }
      if (DefinedByEarlierTriple)
        continue; // Already cloned into Dst* via CumulativeVMap.

      if (!isLiveInAvailableAt(C, DefBB, ReqDst1) || (ReqDst2 && !isLiveInAvailableAt(C, DefBB, ReqDst2)))
        return true;
    }
  }

  return false;
}

bool DiamondChainIfElseTracker::noNonDominatingLiveIns(const MergeCandidate &C, size_t I, BasicBlock *TrueBB,
                                                       BasicBlock *FalseBB, BasicBlock *MergeBB) {
  bool Flip = C.FlipDstForTriple[I];
  BasicBlock *DstForTrue = Flip ? C.DstFalse : C.DstTrue;
  BasicBlock *DstForFalse = Flip ? C.DstTrue : C.DstFalse;

  if (hasNonDominatingLiveIn(C, I, TrueBB, TrueBB, FalseBB, MergeBB, DstForTrue, nullptr) ||
      hasNonDominatingLiveIn(C, I, FalseBB, TrueBB, FalseBB, MergeBB, DstForFalse, nullptr) ||
      (MergeBB && hasNonDominatingLiveIn(C, I, MergeBB, TrueBB, FalseBB, MergeBB, DstForTrue, DstForFalse))) {
    emitRejectRemark(C, I, "check6.non-dominating-live-in", TrueBB, FalseBB, MergeBB);
    return false;
  }

  return true;
}

// Helper: Check if PHI incoming blocks exactly match {TrueBB, FalseBB}
// Only resolve PHIs that are directly produced by diamond's branch condition
bool DiamondChainIfElseTracker::shouldResolvePHI(PHINode *PN, BasicBlock *TrueBB, BasicBlock *FalseBB) {
  if (PN->getNumIncomingValues() != 2)
    return false;

  SmallPtrSet<BasicBlock *, 2> IncomingBlocks;
  for (unsigned I = 0; I < PN->getNumIncomingValues(); ++I) {
    IncomingBlocks.insert(PN->getIncomingBlock(I));
  }

  return IncomingBlocks.count(TrueBB) && IncomingBlocks.count(FalseBB) && IncomingBlocks.size() == 2;
}

bool DiamondChainIfElseTracker::verifySingleCandidate(MergeCandidate &C) {
  assert(C.TrueBlocks.size() == C.FalseBlocks.size() && C.TrueBlocks.size() == C.MergeBlocks.size() &&
         C.TrueBlocks.size() == C.FlipDstForTriple.size() && "Parallel triple lists must have equal length");

  size_t SafeCount = 0;
  for (size_t I = 0; I < C.TrueBlocks.size(); ++I) {
    BasicBlock *TrueBB = C.TrueBlocks[I];
    BasicBlock *FalseBB = C.FalseBlocks[I];
    BasicBlock *MergeBB = C.MergeBlocks[I];

    if (!basicShape(C, I, TrueBB, FalseBB, MergeBB) || !mergePredCount(C, I, TrueBB, FalseBB, MergeBB) ||
        !mergeBlockBarrierWithDisjointCond(C, I, TrueBB, FalseBB, MergeBB) ||
        hasMixedConditionInSource(C, I, TrueBB, FalseBB, MergeBB) || !chainContinuity(C, I, TrueBB, FalseBB, MergeBB) ||
        !noEscapingDefs(C, I, TrueBB, FalseBB, MergeBB)) {
      trimCandidateToPrefix(C, SafeCount);
      return SafeCount > 0;
    }

    if (hasAnyConvergentInst(TrueBB) || hasAnyConvergentInst(FalseBB) || (MergeBB && hasAnyConvergentInst(MergeBB))) {
      emitRejectRemark(C, I, "check7.convergent", TrueBB, FalseBB, MergeBB);
      trimCandidateToPrefix(C, 0);
      return false;
    }

    if (!noVolatileOrAtomic(C, I, TrueBB, FalseBB, MergeBB)) {
      // Side-effecting instructions in any triple make prefix-only merging
      // unsafe: values from already-merged triples may still feed skipped
      // tail triples and later get repaired to poison.
      trimCandidateToPrefix(C, 0);
      return false;
    }

    if (!noNonDominatingLiveIns(C, I, TrueBB, FalseBB, MergeBB)) {
      trimCandidateToPrefix(C, SafeCount);
      return SafeCount > 0;
    }

    // Check that merging TrueBB into DstTrue (and FalseBB into DstFalse) will
    // not orphan any blocks that are exclusively owned by the destination.
    // When a source triple is merged, DstTrue's terminator is replaced by
    // TrueBB's terminator.  Any successor of DstTrue whose sole predecessor is
    // DstTrue would then become unreachable if that successor is not among
    // TrueBB's own successors.  Orphaned blocks contain code that would be
    // silently deleted by EliminateUnreachableBlocks, producing incorrect
    // output.
    //
    // This check is applied only for the FIRST source triple (I == 0) against
    // the original DstTrue/DstFalse accumulators.  For I > 0 the destination
    // blocks have already been extended by previous merges, so their
    // single-predecessor successors are merge-artifacts that are intentionally
    // replaced - not original IR that must be preserved.
    if (I == 0) {
      BasicBlock *DstForTrue = C.FlipDstForTriple[I] ? C.DstFalse : C.DstTrue;
      BasicBlock *DstForFalse = C.FlipDstForTriple[I] ? C.DstTrue : C.DstFalse;

      auto WouldOrphan = [&](BasicBlock *DstBB, BasicBlock *SrcBB) -> bool {
        // SrcBB's successors will be reachable from DstBB's new terminator.
        SmallPtrSet<BasicBlock *, 4> SrcSuccs(succ_begin(SrcBB), succ_end(SrcBB));
        // TrueBB, FalseBB and MergeBB are "consumed" by this merge: their
        // content is cloned into the destination blocks and the originals are
        // removed.  A single-predecessor successor of DstBB that equals one
        // of these blocks is therefore handled - not orphaned.
        SmallPtrSet<BasicBlock *, 4> ConsumedByMerge;
        ConsumedByMerge.insert(TrueBB);
        ConsumedByMerge.insert(FalseBB);
        if (MergeBB)
          ConsumedByMerge.insert(MergeBB);

        for (BasicBlock *S : successors(DstBB)) {
          if (!S->getSinglePredecessor())
            continue; // multiple predecessors - not exclusively owned
          if (SrcSuccs.count(S))
            continue; // reachable from SrcBB's new terminator
          if (ConsumedByMerge.count(S))
            continue;  // will be cloned/consumed by this merge
          return true; // this block would become truly unreachable
        }
        return false;
      };

      if (WouldOrphan(DstForTrue, TrueBB) || WouldOrphan(DstForFalse, FalseBB)) {
        emitRejectRemark(C, I, "check.orphan-exclusive-succ", TrueBB, FalseBB, MergeBB);
        trimCandidateToPrefix(C, SafeCount);
        return SafeCount > 0;
      }
    }

    ++SafeCount;
  }

  trimCandidateToPrefix(C, SafeCount);
  return SafeCount > 0;
}

// Validates every collected MergeCandidate and trims each one to the longest
// contiguous prefix of triples that can be safely merged. Candidates that
// are completely unsafe are removed.
//
// We must keep a CONTIGUOUS prefix (not an arbitrary subset) because each
// triple in the chain assumes that all earlier triples have already been
// merged into DstTrue/DstFalse. Skipping triple i while merging triple i+1
// would leave an unprocessed block in the middle of the merged sequence,
// corrupting the CFG.
void DiamondChainIfElseTracker::verifyCandidates() {
  // Candidate order is irrelevant for subsequent processing, so remove
  // rejected entries by replacing them with the last element. This avoids
  // shifting the tail of the vector for every rejected candidate.
  size_t I = 0;
  while (I < Candidates.size()) {
    if (verifySingleCandidate(Candidates[I])) {
      ++I;
      continue;
    }

    if (I + 1 != Candidates.size())
      Candidates[I] = std::move(Candidates.back());
    Candidates.pop_back();
  }

  // Emit the final grouping used by mergeBasicBlocks().
  emitGroupedCandidatesRemark("post-verify");
}

bool DiamondChainIfElseTracker::tryMergeWholeTriangleChain(MergeCandidate &C,
                                                           SmallVector<BasicBlock *, 32> &BlocksToRemove) {
  // Whole-chain path applies only to pure triangle chains without
  // accumulator merge block. Any violation falls back to regular merging.
  auto EmitWholeChainSkip = [&](StringRef Reason, BasicBlock *TrueBB = nullptr, BasicBlock *FalseBB = nullptr) {
    ORE.emit([&]() {
      return OptimizationRemarkMissed("diamond-chain-merge", "WholeChainSkipped", DebugLoc(),
                                      TrueBB ? TrueBB : &Func.getEntryBlock())
             << "reason=" << ore::NV("Reason", Reason)
             << ", dstTrue=" << ore::NV("DstTrue", getBBDisplayName(C.DstTrue))
             << ", dstFalse=" << ore::NV("DstFalse", getBBDisplayName(C.DstFalse))
             << ", true=" << ore::NV("True", getBBDisplayName(TrueBB))
             << ", false=" << ore::NV("False", getBBDisplayName(FalseBB));
    });
    return false;
  };

  // Require at least one extra triple and no pre-existing Dst merge.
  if (C.DstMergeBB || C.TrueBlocks.empty())
    return false;

  // Build aligned body/merge sequences:
  //   BodyChain[i] executes then unconditionally flows to MergeChain[i].
  SmallVector<BasicBlock *, 16> BodyChain;
  SmallVector<BasicBlock *, 16> MergeChain;
  BodyChain.push_back(C.DstTrue);
  MergeChain.push_back(C.DstFalse);
  BodyChain.append(C.TrueBlocks.begin(), C.TrueBlocks.end());
  MergeChain.append(C.FalseBlocks.begin(), C.FalseBlocks.end());

  if (BodyChain.size() != MergeChain.size() || BodyChain.size() < 2)
    return EmitWholeChainSkip("size-mismatch");

  // Find the external entry edge to the first merge block (DstFalse).
  // The other predecessor is the first body block (DstTrue).
  BasicBlock *EntryBB = nullptr;
  for (BasicBlock *Pred : predecessors(C.DstFalse)) {
    if (Pred == C.DstTrue)
      continue;
    if (EntryBB)
      return EmitWholeChainSkip("multiple-entry-preds");
    EntryBB = Pred;
  }
  if (!EntryBB)
    return EmitWholeChainSkip("missing-entry-pred");

  auto *EntryBr = dyn_cast<BranchInst>(EntryBB->getTerminator());
  if (!EntryBr || !EntryBr->isConditional())
    return EmitWholeChainSkip("entry-not-condbr");

  // Validate chain shape: every non-final merge is PHI-only + condbr using
  // the same condition as EntryBr and must point to the next pair.
  Value *ChainCond = EntryBr->getCondition();
  for (size_t I = 0; I < BodyChain.size(); ++I) {
    BasicBlock *BodyBB = BodyChain[I];
    BasicBlock *MergeBB = MergeChain[I];
    auto *BodyBr = dyn_cast<BranchInst>(BodyBB->getTerminator());
    if (!BodyBr || !BodyBr->isUnconditional() || BodyBr->getSuccessor(0) != MergeBB)
      return EmitWholeChainSkip("body-not-triangle", BodyBB, MergeBB);

    if (!BodyBB->empty() && isa<PHINode>(BodyBB->front()))
      return EmitWholeChainSkip("body-has-phi", BodyBB, MergeBB);

    bool IsLast = I + 1 == BodyChain.size();
    if (IsLast)
      continue;

    for (Instruction &Inst : *MergeBB) {
      if (!isa<PHINode>(Inst) && !Inst.isTerminator())
        return EmitWholeChainSkip("merge-has-body", BodyBB, MergeBB);
    }

    auto *MergeBr = dyn_cast<BranchInst>(MergeBB->getTerminator());
    if (!MergeBr || !MergeBr->isConditional() || MergeBr->getCondition() != ChainCond)
      return EmitWholeChainSkip("merge-cond-mismatch", BodyBB, MergeBB);

    BasicBlock *Succ0 = MergeBr->getSuccessor(0);
    BasicBlock *Succ1 = MergeBr->getSuccessor(1);
    if (!((Succ0 == MergeChain[I + 1] && Succ1 == BodyChain[I + 1]) ||
          (Succ1 == MergeChain[I + 1] && Succ0 == BodyChain[I + 1])))
      return EmitWholeChainSkip("merge-next-mismatch", BodyBB, MergeBB);
  }

  // Map each intermediate merge PHI to the value visible on the entry path.
  // This is used later when recreating equivalent PHIs in the final merge.
  DenseMap<Value *, Value *> EntryValueMap;
  auto ResolveEntryValue = [&](Value *V) {
    SmallPtrSet<Value *, 8> Seen;
    while (EntryValueMap.count(V) && Seen.insert(V).second)
      V = EntryValueMap[V];
    return V;
  };

  for (size_t I = 0; I < MergeChain.size(); ++I) {
    BasicBlock *PrevMerge = (I == 0) ? EntryBB : MergeChain[I - 1];
    for (Instruction &Inst : *MergeChain[I]) {
      auto *PN = dyn_cast<PHINode>(&Inst);
      if (!PN)
        break;

      int PrevIdx = PN->getBasicBlockIndex(PrevMerge);
      int BodyIdx = PN->getBasicBlockIndex(BodyChain[I]);
      if (PrevIdx < 0 || BodyIdx < 0)
        return EmitWholeChainSkip("phi-incoming-mismatch", BodyChain[I], MergeChain[I]);

      EntryValueMap[PN] = ResolveEntryValue(PN->getIncomingValue(PrevIdx));
    }
  }

  // Flatten by moving payload instructions from all previous body blocks
  // into the final body block, preserving terminators.
  BasicBlock *FinalBody = BodyChain.back();
  BasicBlock *FinalMerge = MergeChain.back();

  for (size_t I = 0; I + 1 < BodyChain.size(); ++I) {
    BasicBlock *BodyBB = BodyChain[I];
    auto InsertPt = FinalBody->getFirstNonPHIIt();
    for (auto It = BodyBB->begin(); It != BodyBB->end();) {
      Instruction &Inst = *It++;
      if (Inst.isTerminator())
        continue;
      Inst.moveBefore(InsertPt);
    }
  }

  // Rewire the original entry branch to jump directly to the final pair.
  for (unsigned SuccIdx = 0; SuccIdx < EntryBr->getNumSuccessors(); ++SuccIdx) {
    if (EntryBr->getSuccessor(SuccIdx) == C.DstTrue)
      EntryBr->setSuccessor(SuccIdx, FinalBody);
    else if (EntryBr->getSuccessor(SuccIdx) == C.DstFalse)
      EntryBr->setSuccessor(SuccIdx, FinalMerge);
  }

  // Rebuild PHI semantics in FinalMerge.
  // 1) Patch existing final PHIs to use EntryBB as the non-body predecessor.
  // 2) Materialize synthetic .dcm PHIs for intermediate merge PHIs.
  DenseMap<Value *, Value *> PhiReplacement;
  auto ReplaceUsesInBlock = [](Value *OldV, Value *NewV, BasicBlock *BB) {
    SmallVector<Use *, 8> UsesToRewrite;
    for (Use &U : OldV->uses()) {
      auto *UserI = dyn_cast<Instruction>(U.getUser());
      if (UserI && UserI->getParent() == BB)
        UsesToRewrite.push_back(&U);
    }
    for (Use *U : UsesToRewrite)
      U->set(NewV);
  };

  BasicBlock *PrevFinalMerge = MergeChain[MergeChain.size() - 2];
  for (Instruction &Inst : *FinalMerge) {
    auto *PN = dyn_cast<PHINode>(&Inst);
    if (!PN)
      break;

    int PrevIdx = PN->getBasicBlockIndex(PrevFinalMerge);
    int BodyIdx = PN->getBasicBlockIndex(FinalBody);
    if (PrevIdx < 0 || BodyIdx < 0)
      return EmitWholeChainSkip("final-phi-incoming-mismatch", FinalBody, FinalMerge);

    PN->setIncomingBlock(PrevIdx, EntryBB);
    PN->setIncomingValue(PrevIdx, ResolveEntryValue(PN->getIncomingValue(PrevIdx)));
    PhiReplacement[PN] = PN;
  }

  for (size_t I = 0; I + 1 < MergeChain.size(); ++I) {
    BasicBlock *BodyBB = BodyChain[I];
    BasicBlock *MergeBB = MergeChain[I];
    for (Instruction &Inst : *MergeBB) {
      auto *PN = dyn_cast<PHINode>(&Inst);
      if (!PN)
        break;

      // Value visible when execution reaches FinalBody through the body path.
      Value *BodyVal = PN->getIncomingValueForBlock(BodyBB);
      if (auto It = PhiReplacement.find(BodyVal); It != PhiReplacement.end())
        BodyVal = It->second;

      // Uses in FinalBody must stay on body-path values. Replacing them with
      // a PHI in FinalMerge would violate dominance.
      ReplaceUsesInBlock(PN, BodyVal, FinalBody);

      Value *EntryVal = ResolveEntryValue(PN);
      auto InsertPt = FinalMerge->getFirstNonPHIIt();
      PHINode *NewPN = PHINode::Create(PN->getType(), 2, PN->getName() + ".dcm", InsertPt);
      NewPN->addIncoming(BodyVal, FinalBody);
      NewPN->addIncoming(EntryVal, EntryBB);
      PhiReplacement[PN] = NewPN;
    }
  }

  // Swap old PHI defs to reconstructed values.
  for (const auto &It : PhiReplacement) {
    Value *OldV = It.first;
    Value *NewV = It.second;
    if (OldV != NewV)
      OldV->replaceAllUsesWith(NewV);
  }

  // Keep only final body/final merge; all earlier chain nodes become dead.
  for (size_t I = 0; I + 1 < BodyChain.size(); ++I)
    BlocksToRemove.push_back(BodyChain[I]);
  for (size_t I = 0; I + 1 < MergeChain.size(); ++I)
    BlocksToRemove.push_back(MergeChain[I]);

  ORE.emit([&]() {
    return OptimizationRemarkAnalysis("diamond-chain-merge", "WholeChainApplied", DebugLoc(), FinalBody)
           << "body=" << ore::NV("Body", getBBDisplayName(FinalBody))
           << ", merge=" << ore::NV("Merge", getBBDisplayName(FinalMerge));
  });

  C.TrueBlocks.clear();
  C.FalseBlocks.clear();
  C.MergeBlocks.clear();
  return true;
}

bool DiamondChainIfElseTracker::mergePathIntoBlock(BasicBlock *Dst, BasicBlock *Src, BasicBlock *SrcPhiIncoming,
                                                   BasicBlock *TailBB, BasicBlock *TailPhiIncoming, BasicBlock *TrueBB,
                                                   BasicBlock *FalseBB, ValueToValueMapTy &CumulativeVMap,
                                                   DenseMap<Value *, Value *> &SourceToDestMap) {
  assert(Dst && Src && "mergePathIntoBlock expects non-null destination/source");

  bool Changed = false;
  ValueToValueMapTy VMap;
  SmallVector<Instruction *, 32> NewInsts;

  // Note: llvm::CloneBasicBlock cannot replace this lambda because it
  // (a) creates a new block rather than inlining into an existing one,
  // (b) clones the terminator which we intentionally skip, and
  // (c) clones PHI nodes as-is, whereas we implement "fold-by-path":
  //     PHIs whose incoming blocks are exactly {TrueBB, FalseBB} are
  //     resolved to the value for the current execution path instead of
  //     being preserved as PHIs.  This fold is the core transformation.
  auto CloneBodyFrom = [&](BasicBlock *From, BasicBlock *PhiIncoming) {
    cloneBodyWithoutTerminator(
        From, Dst, VMap, NewInsts,
        [&](PHINode &PN) {
          // FoldByPath: Resolve PHI that directly corresponds to diamond
          // branches These PHIs don't need to be cloned; instead resolve to
          // the value for the current execution path.
          if (shouldResolvePHI(&PN, TrueBB, FalseBB)) {
            Value *Selected = PoisonValue::get(PN.getType());
            if (PhiIncoming) {
              int Idx = PN.getBasicBlockIndex(PhiIncoming);
              if (Idx >= 0) {
                Value *IncomingVal = PN.getIncomingValue(Idx);
                // Resolve incoming value, checking CumulativeVMap for
                // values from previous triples. This is the safe "fold-by-
                // path" case: the PHI is exactly the current diamond's branch
                // outcome and therefore has a single valid path-specific value.
                if (CumulativeVMap.count(IncomingVal)) {
                  Selected = CumulativeVMap[IncomingVal];
                } else if (VMap.count(IncomingVal)) {
                  Selected = VMap[IncomingVal];
                } else {
                  Selected = IncomingVal;
                }
              }
            }
            VMap[&PN] = Selected;
            SourceToDestMap[&PN] = Selected;
            return;
          }

          // Other PHI patterns must not silently become poison.  When the PHI
          // depends on the previous accumulator merge or on a value already
          // cloned into the current path, resolve it via the cumulative/path
          // value map instead of dropping to poison.
          Value *Selected = nullptr;
          if (PhiIncoming) {
            int Idx = PN.getBasicBlockIndex(PhiIncoming);
            if (Idx >= 0) {
              Value *IncomingVal = PN.getIncomingValue(Idx);
              if (CumulativeVMap.count(IncomingVal)) {
                Selected = CumulativeVMap[IncomingVal];
              } else if (VMap.count(IncomingVal)) {
                Selected = VMap[IncomingVal];
              } else {
                Selected = IncomingVal;
              }
            }
          }

          if (!Selected) {
            for (unsigned I = 0; I < PN.getNumIncomingValues(); ++I) {
              Value *IncomingVal = PN.getIncomingValue(I);
              if (CumulativeVMap.count(IncomingVal)) {
                Selected = CumulativeVMap[IncomingVal];
                break;
              }
              if (VMap.count(IncomingVal)) {
                Selected = VMap[IncomingVal];
                break;
              }
            }
          }

          if (!Selected)
            Selected = PoisonValue::get(PN.getType());

          VMap[&PN] = Selected;
          SourceToDestMap[&PN] = Selected;
        },
        [&](Instruction &OldInst, Instruction &NewInst) {
          // Record mapping: original instruction -> cloned instruction
          SourceToDestMap[&OldInst] = &NewInst;
          Changed = true;
        });
  };

  if (Src != Dst)
    CloneBodyFrom(Src, SrcPhiIncoming);

  if (TailBB && TailBB != Src)
    CloneBodyFrom(TailBB, TailPhiIncoming);

  for (Instruction *I : NewInsts)
    RemapInstruction(I, VMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);

  // After remapping with current VMap, check for operands from previous
  // triples that are in CumulativeVMap but not in current VMap
  for (Instruction *I : NewInsts) {
    for (Use &U : I->operands()) {
      Value *Op = U.get();
      if (Op && CumulativeVMap.count(Op) && !VMap.count(Op)) {
        Value *RemappedOp = CumulativeVMap[Op];
        I->setOperand(U.getOperandNo(), RemappedOp);
      }
    }
  }

  // Also update SourceToDestMap with any operands we just remapped, so
  // they'll be available for future triples
  for (Instruction *I : NewInsts) {
    for (Use &U : I->operands()) {
      Value *Op = U.get();
      if (Op && CumulativeVMap.count(Op) && !SourceToDestMap.count(Op)) {
        SourceToDestMap[Op] = CumulativeVMap[Op];
      }
    }
  }

  // Select the terminator from MergeBB when available: it is the last
  // block in the merged sequence and its successors are the correct
  // continuation for both the true and false paths.  Fall back to Src's
  // terminator when there is no separate merge block (Src itself is the
  // last block and already points to the right continuation).
  BasicBlock *TermSrc = (TailBB && TailBB->getTerminator()) ? TailBB : Src;
  Instruction *NewTerm = nullptr;
  if (Instruction *T = TermSrc->getTerminator())
    NewTerm = T->clone();

  if (Instruction *DstTerm = Dst->getTerminator()) {
    DstTerm->eraseFromParent();
    Changed = true;
  }

  if (NewTerm) {
    RemapInstruction(NewTerm, VMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);
    // Same fixup as for NewInsts above: the terminator can also reference a
    // value that only exists in CumulativeVMap.
    for (Use &U : NewTerm->operands()) {
      Value *Op = U.get();
      if (Op && CumulativeVMap.count(Op) && !VMap.count(Op))
        U.set(CumulativeVMap[Op]);
    }
    NewTerm->insertInto(Dst, Dst->end());
    Changed = true;
  } else if (!Dst->getTerminator()) {
    new UnreachableInst(Dst->getContext(), Dst);
    Changed = true;
  }

  return Changed;
}

bool DiamondChainIfElseTracker::mergeBasicBlocks(BasicBlock *DstTrue, BasicBlock *DstFalse, BasicBlock *TrueBB,
                                                 BasicBlock *FalseBB, BasicBlock *MergeBB,
                                                 ValueToValueMapTy &CumulativeTrueVMap,
                                                 ValueToValueMapTy &CumulativeFalseVMap,
                                                 SmallVector<BasicBlock *, 32> &BlocksToRemove) {
  assert(DstTrue && DstFalse && TrueBB && FalseBB && "mergeBasicBlocks expects validated non-null blocks");

  // Maps each instruction from source blocks (TrueBB/FalseBB/MergeBB) to its
  // cloned counterpart in the destination blocks (DstTrue/DstFalse).
  // Separate maps for each path to avoid overwriting mappings.
  DenseMap<Value *, Value *> SourceToDestMapTrue;
  DenseMap<Value *, Value *> SourceToDestMapFalse;

  BasicBlock *TrueTailBB = MergeBB;
  BasicBlock *TrueTailPhiIncoming = TrueBB;
  BasicBlock *FalseSrcPhiIncoming = FalseBB;

  if (!MergeBB) {
    BasicBlock *TrueSucc = nullptr;
    if (auto *UB = dyn_cast<BranchInst>(TrueBB->getTerminator()))
      if (UB->isUnconditional())
        TrueSucc = UB->getSuccessor(0);

    if (TrueSucc == FalseBB) {
      TrueTailBB = FalseBB;
      TrueTailPhiIncoming = TrueBB;
      FalseSrcPhiIncoming = DstFalse;
    }
  }

  bool Changed = false;

  Changed |= mergePathIntoBlock(DstTrue, TrueBB, TrueBB, TrueTailBB, TrueTailPhiIncoming, TrueBB, FalseBB,
                                CumulativeTrueVMap, SourceToDestMapTrue);
  // Update CumulativeTrueVMap with new mappings from True path before
  // processing False path
  for (const auto &Mapping : SourceToDestMapTrue) {
    CumulativeTrueVMap[Mapping.first] = Mapping.second;
  }

  Changed |= mergePathIntoBlock(DstFalse, FalseBB, FalseSrcPhiIncoming, MergeBB, FalseBB, TrueBB, FalseBB,
                                CumulativeFalseVMap, SourceToDestMapFalse);
  // Update CumulativeFalseVMap with new mappings from False path
  for (const auto &Mapping : SourceToDestMapFalse) {
    CumulativeFalseVMap[Mapping.first] = Mapping.second;
  }

  // Update all uses of source block instructions to refer to destination
  // equivalents.  This allows TrueBB, FalseBB, and MergeBB to be safely
  // removed from the function after this transformation.
  //
  // IMPORTANT: only update an operand if its source instruction appears in
  // exactly one of the two maps.  MergeBB instructions are cloned into BOTH
  // paths (with path-specific destinations), so they appear in both maps
  // with *different* target values.  Blindly picking one (True path) would
  // corrupt any future source block that still references the original
  // MergeBB instruction: its operand would be fixed to the True clone, and
  // when the False path later clones that block it would see the True value
  // and produce a non-dominating reference.  Such cross-triple values are
  // handled correctly by the CumulativeVMap post-remap step inside
  // MergePath, so we deliberately skip them here; uses that outlive the whole
  // chain are given a PHI by repairEscapedRegionDefs() once it is fully
  // merged.
  for (BasicBlock &BB : *DstTrue->getParent()) {
    for (Instruction &I : BB) {
      for (Use &U : I.operands()) {
        Value *Op = U.get();
        bool InTrue = SourceToDestMapTrue.count(Op) > 0;
        bool InFalse = SourceToDestMapFalse.count(Op) > 0;
        // Only update if unambiguous (value belongs to exactly one path).
        if (InTrue && !InFalse) {
          I.setOperand(U.getOperandNo(), SourceToDestMapTrue[Op]);
          Changed = true;
        } else if (!InTrue && InFalse) {
          I.setOperand(U.getOperandNo(), SourceToDestMapFalse[Op]);
          Changed = true;
        }
        // If in both maps: path-specific, skip - CumulativeVMap handles it.
      }
    }
  }

  // The block whose terminator is copied into BOTH destinations is the point
  // where the merged triple hands control back: MergeBB in a diamond, the
  // sink FalseBB in a triangle (see the TrueTailBB selection above), and none
  // when the two paths leave through separate terminators.  Its successors
  // gain DstTrue *and* DstFalse as predecessors, so their PHIs need one entry
  // per path rather than a renamed single entry.
  BasicBlock *ExitBB = TrueTailBB;

  // Update PHI node incoming blocks: an incoming from ExitBB is split into one
  // entry per destination, while an incoming from TrueBB or FalseBB - blocks
  // that feed a single path - is simply renamed to that path's destination.
  for (BasicBlock &BB : *DstTrue->getParent()) {
    for (Instruction &I : BB) {
      if (auto *PN = dyn_cast<PHINode>(&I)) {
        for (unsigned Idx = 0; Idx < PN->getNumIncomingValues(); ++Idx) {
          BasicBlock *IncomingBB = PN->getIncomingBlock(Idx);

          if (IncomingBB == ExitBB) {
            // ExitBB was cloned into both destinations, so a single
            // [value, ExitBB] incoming must be split into path-specific
            // incoming entries for DstTrue and DstFalse.
            Value *OrigVal = PN->getIncomingValue(Idx);
            Value *TrueVal = OrigVal;
            Value *FalseVal = OrigVal;
            if (auto It = SourceToDestMapTrue.find(OrigVal); It != SourceToDestMapTrue.end())
              TrueVal = It->second;
            if (auto It = SourceToDestMapFalse.find(OrigVal); It != SourceToDestMapFalse.end())
              FalseVal = It->second;

            PN->setIncomingBlock(Idx, DstTrue);
            PN->setIncomingValue(Idx, TrueVal);

            int FalseIdx = PN->getBasicBlockIndex(DstFalse);
            if (FalseIdx >= 0) {
              PN->setIncomingValue(FalseIdx, FalseVal);
            } else {
              PN->addIncoming(FalseVal, DstFalse);
            }
            Changed = true;
          } else if (IncomingBB == TrueBB) {
            PN->setIncomingBlock(Idx, DstTrue);
            Changed = true;
          } else if (IncomingBB == FalseBB) {
            PN->setIncomingBlock(Idx, DstFalse);
            Changed = true;
          }
        }
      }
    }
  }

  // Collect source blocks for removal.  We defer actual removal until after
  // all triples have been processed, since later triples may reference
  // cloned instructions from earlier triples.  Removing blocks immediately
  // would erase those instructions, causing uses to become dangling (mapped
  // to PoisonValue).
  for (BasicBlock *SrcBB : {TrueBB, FalseBB, MergeBB}) {
    if (SrcBB)
      BlocksToRemove.push_back(SrcBB);
  }

  return Changed;
}

// Clone the accumulator triple's merge block (DstMergeBB) into DstTrue and
// DstFalse, and populate the cumulative value maps with the PHI results and
// cloned non-PHI instructions.  This must be called before processing source
// triples so that any reference to a value defined in DstMergeBB (e.g. a
// PHI that selects between the two accumulator paths) is resolved to the
// correct cloned version rather than left as a dangling reference.
bool DiamondChainIfElseTracker::cloneDstMergeBB(BasicBlock *DstTrue, BasicBlock *DstFalse, BasicBlock *DstMergeBB,
                                                ValueToValueMapTy &CumulativeTrueVMap,
                                                ValueToValueMapTy &CumulativeFalseVMap,
                                                SmallVector<BasicBlock *, 32> &BlocksToRemove) {
  if (!DstMergeBB)
    return false;

  // For each path (true / false) clone DstMergeBB's non-PHI instructions
  // into the corresponding accumulator block, and record the PHI -> incoming
  // value mapping in the cumulative map so subsequent triples can look it up.
  auto CloneIntoPath = [](BasicBlock *Dst, BasicBlock *MergeBB, ValueToValueMapTy &CumulativeVMap) {
    ValueToValueMapTy VMap;
    SmallVector<Instruction *, 8> NewInsts;

    cloneBodyWithoutTerminator(
        MergeBB, Dst, VMap, NewInsts,
        [&](PHINode &PN) {
          // Select the incoming value that comes from Dst (this path).
          Value *Selected = PoisonValue::get(PN.getType());
          int Idx = PN.getBasicBlockIndex(Dst);
          if (Idx >= 0) {
            Value *IncomingVal = PN.getIncomingValue(Idx);
            Selected =
                CumulativeVMap.count(IncomingVal) ? static_cast<Value *>(CumulativeVMap[IncomingVal]) : IncomingVal;
          }
          VMap[&PN] = Selected;
          CumulativeVMap[&PN] = Selected;
        },
        [&](Instruction &OldInst, Instruction &NewInst) { CumulativeVMap[&OldInst] = &NewInst; });

    for (Instruction *I : NewInsts)
      RemapInstruction(I, VMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);

    for (Instruction *I : NewInsts)
      for (Use &U : I->operands()) {
        Value *Op = U.get();
        if (Op && CumulativeVMap.count(Op) && !VMap.count(Op))
          I->setOperand(U.getOperandNo(), CumulativeVMap[Op]);
      }
  };

  CloneIntoPath(DstTrue, DstMergeBB, CumulativeTrueVMap);
  CloneIntoPath(DstFalse, DstMergeBB, CumulativeFalseVMap);
  BlocksToRemove.push_back(DstMergeBB);
  return true;
}

bool DiamondChainIfElseTracker::cloneAccumulatorTriangleTailBB(BasicBlock *DstTrue, BasicBlock *DstFalse,
                                                               ValueToValueMapTy &CumulativeTrueVMap,
                                                               ValueToValueMapTy &CumulativeFalseVMap) {
  auto *DstTrueBr = dyn_cast_or_null<BranchInst>(DstTrue->getTerminator());
  if (!DstTrueBr || !DstTrueBr->isUnconditional() || DstTrueBr->getSuccessor(0) != DstFalse)
    return false;

  BasicBlock *FalseIncoming = nullptr;
  for (BasicBlock *Pred : predecessors(DstFalse)) {
    if (Pred == DstTrue)
      continue;
    if (FalseIncoming)
      return false;
    FalseIncoming = Pred;
  }
  if (!FalseIncoming)
    return false;

  ValueToValueMapTy TrueVMap;
  SmallVector<Instruction *, 8> NewInsts;

  for (Instruction &Inst : *DstFalse) {
    if (Inst.isTerminator())
      continue;

    if (auto *PN = dyn_cast<PHINode>(&Inst)) {
      auto SelectIncoming = [&](BasicBlock *IncomingBB, ValueToValueMapTy &CumulativeVMap) {
        Value *Selected = PoisonValue::get(PN->getType());
        int Idx = PN->getBasicBlockIndex(IncomingBB);
        if (Idx >= 0) {
          Value *IncomingVal = PN->getIncomingValue(Idx);
          Selected =
              CumulativeVMap.count(IncomingVal) ? static_cast<Value *>(CumulativeVMap[IncomingVal]) : IncomingVal;
        }
        return Selected;
      };

      Value *TrueSelected = SelectIncoming(DstTrue, CumulativeTrueVMap);
      TrueVMap[&Inst] = TrueSelected;
      CumulativeTrueVMap[&Inst] = TrueSelected;

      Value *FalseSelected = SelectIncoming(FalseIncoming, CumulativeFalseVMap);
      CumulativeFalseVMap[&Inst] = FalseSelected;
      continue;
    }

    Instruction *NewInst = Inst.clone();
    NewInst->insertBefore(DstTrueBr->getIterator());
    TrueVMap[&Inst] = NewInst;
    CumulativeTrueVMap[&Inst] = NewInst;
    CumulativeFalseVMap[&Inst] = &Inst;
    NewInsts.push_back(NewInst);
  }

  for (Instruction *I : NewInsts)
    RemapInstruction(I, TrueVMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);

  for (Instruction *I : NewInsts)
    for (Use &U : I->operands()) {
      Value *Op = U.get();
      if (Op && CumulativeTrueVMap.count(Op) && !TrueVMap.count(Op))
        I->setOperand(U.getOperandNo(), CumulativeTrueVMap[Op]);
    }

  Instruction *NewTerm = DstFalse->getTerminator()->clone();
  RemapInstruction(NewTerm, TrueVMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);
  DstTrueBr->eraseFromParent();
  NewTerm->insertInto(DstTrue, DstTrue->end());
  return true;
}

// Once a chain has been merged, every value defined inside the merged region
// exists as two independent clones: one live out of DstTrue and one live out
// of DstFalse.  A use that survives *outside* the region therefore cannot
// refer to either clone - it is reached through both paths and needs a PHI at
// the point where they converge:
//
//   body0:  %e.t = add i32 %m, 1        ; true-path clone
//           br label %tail
//   merge0: %e.f = add i32 %m, 1        ; false-path clone
//           br label %tail
//   tail:   %e.dcm = phi [ %e.t, %body0 ], [ %e.f, %merge0 ]
//
// SSAUpdater places the PHI (or a cascade of PHIs, when the two paths converge
// through several blocks) and rewrites the uses.  Without this step the stale
// original definition is left dangling and is later degraded to poison - by
// removeCollectedBlocks() when its block is consumed, or by
// repairSSAAfterMerge()'s dominance fallback when it survives in DstFalse -
// which silently drops the value on both paths.
bool DiamondChainIfElseTracker::repairEscapedRegionDefs(const MergeCandidate &C, ValueToValueMapTy &CumulativeTrueVMap,
                                                        ValueToValueMapTy &CumulativeFalseVMap,
                                                        ArrayRef<BasicBlock *> BlocksToRemove) {
  SmallPtrSet<BasicBlock *, 32> Removed(BlocksToRemove.begin(), BlocksToRemove.end());

  // Walk the region in IR order instead of iterating the cumulative value
  // maps: their iteration order is pointer-hash dependent, which would make
  // the names and the relative order of the inserted PHIs unstable.
  // DstTrue is skipped - it only ever holds clones, never map keys.
  SmallVector<BasicBlock *, 16> RegionBlocks;
  RegionBlocks.push_back(C.DstFalse);
  if (C.DstMergeBB)
    RegionBlocks.push_back(C.DstMergeBB);
  for (size_t I = 0; I < C.TrueBlocks.size(); ++I) {
    RegionBlocks.push_back(C.TrueBlocks[I]);
    RegionBlocks.push_back(C.FalseBlocks[I]);
    if (C.MergeBlocks[I])
      RegionBlocks.push_back(C.MergeBlocks[I]);
  }

  bool Changed = false;
  for (BasicBlock *BB : RegionBlocks) {
    for (Instruction &Orig : *BB) {
      if (Orig.isTerminator())
        continue;

      // Only values cloned into *both* paths need a PHI.  A value that lives
      // on a single path cannot be used outside the region at all (its block
      // dominates nothing beyond itself), and mergeBasicBlocks() has already
      // rewritten such uses to the one clone that exists.
      Value *TrueV = CumulativeTrueVMap.lookup(&Orig);
      Value *FalseV = CumulativeFalseVMap.lookup(&Orig);
      if (!TrueV || !FalseV || TrueV == FalseV)
        continue;

      // Collect before rewriting: both paths below mutate Orig's use list.
      SmallVector<Use *, 8> UsesToRewrite;
      SmallVector<std::pair<Use *, Value *>, 8> UsesToRepoint;
      for (Use &U : Orig.uses()) {
        auto *UserI = dyn_cast<Instruction>(U.getUser());
        if (!UserI)
          continue;

        // A PHI operand is live out of its incoming edge, not out of the
        // block holding the PHI.
        auto *PN = dyn_cast<PHINode>(UserI);
        BasicBlock *UseBB = PN ? PN->getIncomingBlock(U) : UserI->getParent();

        if (PN && (UseBB == C.DstTrue || UseBB == C.DstFalse)) {
          // A PHI entry that reaches its block over exactly one of the two
          // paths.  mergeBasicBlocks() repointed the incoming block from the
          // consumed source block to this destination, but left the value
          // naming the original definition - which is about to disappear.
          // The right clone is live out of the incoming block, so name it
          // directly; no new PHI is needed.
          UsesToRepoint.emplace_back(&U, UseBB == C.DstTrue ? TrueV : FalseV);
          continue;
        }
        if (Removed.count(UseBB))
          continue; // Consumed by the merge: dies with the region.
        if (UseBB == C.DstTrue || UseBB == C.DstFalse)
          continue; // Path-local: already points at the correct clone.

        UsesToRewrite.push_back(&U);
      }

      if (UsesToRewrite.empty() && UsesToRepoint.empty())
        continue;

      for (auto &[U, PathVal] : UsesToRepoint)
        U->set(PathVal);

      if (!UsesToRewrite.empty()) {
        std::string PHIName = Orig.hasName() ? (Orig.getName() + ".dcm").str() : std::string("dcm.phi");
        SSAUpdater SSAU;
        SSAU.Initialize(Orig.getType(), PHIName);
        SSAU.AddAvailableValue(C.DstTrue, TrueV);
        SSAU.AddAvailableValue(C.DstFalse, FalseV);
        for (Use *U : UsesToRewrite)
          SSAU.RewriteUseAfterInsertions(*U);
      }

      ORE.emit([&]() {
        return OptimizationRemarkAnalysis("diamond-chain-merge", "EscapedDefRepaired", DebugLoc(), BB)
               << "value from " << ore::NV("DefBB", getBBDisplayName(BB)) << " escapes the merged chain; rewired "
               << ore::NV("PhiUses", static_cast<uint64_t>(UsesToRewrite.size()))
               << " use(s) through a new PHI and repointed "
               << ore::NV("PathUses", static_cast<uint64_t>(UsesToRepoint.size())) << " path-specific PHI entrie(s)";
      });
      Changed = true;
    }
  }

  return Changed;
}

// Remove a list of blocks after all triples have been processed.
// This allows later triples to reference cloned instructions from earlier
// triples before those instructions are erased.
void DiamondChainIfElseTracker::removeCollectedBlocks(SmallVector<BasicBlock *, 32> &BlocksToRemove) {
  for (BasicBlock *SrcBB : BlocksToRemove) {
    if (!SrcBB || SrcBB->empty())
      continue;

    // Clear all instructions: remove uses and erase them.
    while (!SrcBB->empty() && !SrcBB->back().isTerminator()) {
      Instruction &I = SrcBB->back();
      if (!I.use_empty())
        I.replaceAllUsesWith(PoisonValue::get(I.getType()));
      I.eraseFromParent();
    }

    // Replace terminator with unreachable so the block becomes obviously
    // dead and can be removed by standard dead-code elimination passes.
    if (Instruction *Term = SrcBB->getTerminator())
      Term->eraseFromParent();
    new UnreachableInst(SrcBB->getContext(), SrcBB);
  }
}

bool DiamondChainIfElseTracker::repairSSAAfterMerge() {
  // Keep predecessor topology repair always enabled. For now we also keep a
  // dominance fallback to avoid producing verifier-invalid modules on known
  // corner cases; debug verifier diagnostics in run() still surface them.
  DominatorTree FreshDT(Func);
  bool Repaired = false;

  // Fix PHI nodes: remove invalid incoming blocks and add missing ones.
  for (BasicBlock &BB : Func) {
    // A predecessor may reach BB over several edges - a switch with more than
    // one case sharing a destination, or a conditional branch whose two
    // successors are the same block.  A PHI must carry one incoming entry per
    // *edge*, not per distinct predecessor block, so the repair below works on
    // edge multiplicity.  Collapsing such entries to one leaves
    // getNumIncomingValues() < pred_size(), which the verifier rejects and
    // which makes a later BasicBlock::removePredecessor() index a PHI with -1.
    SmallDenseMap<BasicBlock *, unsigned, 16> PredEdgeCount;
    for (BasicBlock *Pred : predecessors(&BB))
      ++PredEdgeCount[Pred];

    auto It = BB.begin();
    while (It != BB.end() && isa<PHINode>(*It)) {
      PHINode *PN = cast<PHINode>(&*It);

      // Canonicalize the incoming values per predecessor.  A PHI may have
      // several edges from the same predecessor, but all of them must carry the
      // same value; otherwise the verifier rejects it as a broken PHI.  Keep
      // only the valid edge count and fold any stale mixed entries to a single
      // representative value for that predecessor.
      SmallDenseMap<BasicBlock *, Value *, 16> CanonicalValue;
      SmallDenseMap<BasicBlock *, unsigned, 16> SeenCount;
      for (unsigned I = PN->getNumIncomingValues(); I-- > 0;) {
        BasicBlock *Pred = PN->getIncomingBlock(I);
        if (!PredEdgeCount.count(Pred)) {
          PN->removeIncomingValue(I, false);
          Repaired = true;
          continue;
        }

        Value *V = PN->getIncomingValue(I);
        auto &Count = SeenCount[Pred];
        if (Count == 0)
          CanonicalValue[Pred] = V;
        else if (CanonicalValue[Pred] != V) {
          // Mixed values for the same predecessor can come from stale path
          // duplication; keep the first valid value and drop the conflicting
          // duplicates to satisfy the verifier.
          PN->removeIncomingValue(I, false);
          Repaired = true;
          continue;
        }
        ++Count;

        if (Count > PredEdgeCount.lookup(Pred)) {
          PN->removeIncomingValue(I, false);
          Repaired = true;
          --Count;
        }
      }

      // Add the missing edges back, all with the same value per predecessor.
      SmallDenseMap<BasicBlock *, unsigned, 16> Kept;
      for (const auto &KV : SeenCount)
        Kept[KV.first] = std::min(KV.second, PredEdgeCount.lookup(KV.first));
      for (BasicBlock *Pred : predecessors(&BB)) {
        unsigned &Remaining = Kept[Pred];
        if (Remaining) {
          --Remaining;
          continue;
        }
        Value *V = CanonicalValue.lookup(Pred);
        if (!V)
          V = PoisonValue::get(PN->getType());
        PN->addIncoming(V, Pred);
        Repaired = true;
      }

      // Fallback: if incoming value does not dominate incoming edge, replace
      // it with poison to keep IR verifier-clean.
      for (unsigned I = 0; I < PN->getNumIncomingValues(); ++I) {
        if (Instruction *OpI = dyn_cast<Instruction>(PN->getIncomingValue(I))) {
          BasicBlock *IncomingBB = PN->getIncomingBlock(I);
          Instruction *IncomingTerm = IncomingBB ? IncomingBB->getTerminator() : nullptr;
          bool DominatesIncoming = IncomingTerm && FreshDT.dominates(OpI, IncomingTerm);
          if (!DominatesIncoming) {
            PN->setIncomingValue(I, PoisonValue::get(PN->getType()));
            Repaired = true;
          }
        }
      }

      if (PN->getNumIncomingValues() == 1) {
        Value *Incoming = PN->getIncomingValue(0);
        PN->replaceAllUsesWith(Incoming);
        It = PN->eraseFromParent();
        Repaired = true;
        continue;
      }

      ++It;
    }
  }

  // Fallback for non-PHI instructions: replace non-dominating operands with
  // poison to maintain verifier-valid IR.
  for (BasicBlock &BB : Func) {
    for (Instruction &I : BB) {
      if (isa<PHINode>(I))
        continue;

      for (Use &U : I.operands()) {
        if (Instruction *OpI = dyn_cast<Instruction>(U.get())) {
          if (!FreshDT.dominates(OpI, &I)) {
            I.setOperand(U.getOperandNo(), PoisonValue::get(U->getType()));
            Repaired = true;
          }
        }
      }
    }
  }

  return Repaired;
}

} // namespace

namespace IGC {

IGC_INITIALIZE_PASS_BEGIN(DiamondChainMergePass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(DiamondChainMergePass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char DiamondChainMergePass::ID = 0;

DiamondChainMergePass::DiamondChainMergePass() : FunctionPass(ID) {
  initializeDiamondChainMergePassPass(*PassRegistry::getPassRegistry());
}

bool DiamondChainMergePass::runOnFunction(Function &F) {
  DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  OptimizationRemarkEmitter ORE(&F);
  DiamondChainIfElseTracker Tracker(DT, ORE, F);

  if (!Tracker.findPatterns())
    return false;

  bool Changed = false;
  SmallVector<BasicBlock *, 32> BlocksToRemove;
  Tracker.verifyCandidates();

  auto &Candidates = Tracker.getCandidates();
  size_t I = 0;
  while (I < Candidates.size()) {
    if (Tracker.tryMergeWholeTriangleChain(Candidates[I], BlocksToRemove)) {
      Changed = true;
      if (I + 1 != Candidates.size())
        Candidates[I] = std::move(Candidates.back());
      Candidates.pop_back();
      continue;
    }
    ++I;
  }

  ValueToValueMapTy CumulativeTrueVMap;
  ValueToValueMapTy CumulativeFalseVMap;
  for (const auto &Candidate : Tracker.getCandidates()) {
    Changed |= Tracker.cloneAccumulatorTriangleTailBB(Candidate.DstTrue, Candidate.DstFalse, CumulativeTrueVMap,
                                                      CumulativeFalseVMap);

    Changed |= Tracker.cloneDstMergeBB(Candidate.DstTrue, Candidate.DstFalse, Candidate.DstMergeBB, CumulativeTrueVMap,
                                       CumulativeFalseVMap, BlocksToRemove);

    for (size_t I = 0; I < Candidate.TrueBlocks.size(); ++I) {
      bool FlipDst = Candidate.FlipDstForTriple[I];

      BasicBlock *DstForTruePath = FlipDst ? Candidate.DstFalse : Candidate.DstTrue;
      BasicBlock *DstForFalsePath = FlipDst ? Candidate.DstTrue : Candidate.DstFalse;

      ValueToValueMapTy &TruePathMap = FlipDst ? CumulativeFalseVMap : CumulativeTrueVMap;
      ValueToValueMapTy &FalsePathMap = FlipDst ? CumulativeTrueVMap : CumulativeFalseVMap;

      Changed |=
          Tracker.mergeBasicBlocks(DstForTruePath, DstForFalsePath, Candidate.TrueBlocks[I], Candidate.FalseBlocks[I],
                                   Candidate.MergeBlocks[I], TruePathMap, FalsePathMap, BlocksToRemove);
    }

    Changed |= Tracker.repairEscapedRegionDefs(Candidate, CumulativeTrueVMap, CumulativeFalseVMap, BlocksToRemove);
  }

  Tracker.removeCollectedBlocks(BlocksToRemove);

  if (Changed) {
    llvm::EliminateUnreachableBlocks(F);
    Tracker.repairSSAAfterMerge();
  }

  return Changed;
}

} // namespace IGC

void initializeDiamondChainMergePassPass(llvm::PassRegistry &Registry) {
  IGC::initializeDiamondChainMergePassPass(Registry);
}

#endif // LLVM_VERSION_MAJOR == 22
