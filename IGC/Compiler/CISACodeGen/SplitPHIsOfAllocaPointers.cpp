/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/SplitPHIsOfAllocaPointers.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

#include "common/igc_regkeys.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Analysis/ValueTracking.h"

#include "Compiler/IGCPassSupport.h"

#include "Probe/Assertion.h"

#include <optional>

using namespace llvm;

// Hoist loads past pointer-typed phi instructions that
// merge pointers derived from an alloca with other dereferenceable pointers.
// Rewrites:
//
// %phi = phi ptr [ alloca_gep, bb1 ], [ arg_gep, bb2 ]
// %v   = load float, ptr %phi
//
// into:
//
// bb1: %v1 = load float, ptr alloca_gep
// bb2: %v2 = load float, ptr arg_gep
// merge: %v = phi float [ %v1, bb1 ], [ %v2, bb2 ]
//
// Why: LLVM's InstCombine may introduce a [N x T]* bitcast for strided array
// accesses indexed dynamically, and the dynamic index can flow through a
// phi merge with a pointer from another source. SOALayoutChecker cannot
// walk through such merges, so the alloca would be rejected for SoA/GRF promotion.
// Hoisting converts the merge from pointer-level to value-level, leaving a clean
// GEP/load/store use chain that SOALayoutChecker can analyse.
//
// Returns true if any rewrite was applied.

namespace {
constexpr int MAX_HOISTING_ITER = 16;

using MergeHoistRecord = SmallVector<LoadInst *, 4>;

struct LoadVia {
  LoadInst *LI;
  SmallVector<Instruction *, 4> Chain; // merge → Chain[0] → ... → LI (casts and const-GEPs)
};

void commitHoistRecord(MergeHoistRecord &Record) {
  // Seeding the recursive deleter with the (now trivially dead) original loads
  // cascades the cleanup through the post-merge cast chains, the pointer merge,
  // and the dead pre-merge cast chains automatically.
  SmallVector<WeakTrackingVH, 8> DeadInsts;
  for (LoadInst *LI : Record) {
    IGC_ASSERT(LI->use_empty());
    DeadInsts.push_back(LI);
  }
  RecursivelyDeleteTriviallyDeadInstructionsPermissive(DeadInsts);
}

bool hasDuplicateIncomingBlocks(PHINode *Phi) {
  SmallPtrSet<BasicBlock *, 4> SeenBlocks;
  for (BasicBlock *BB : Phi->blocks()) {
    if (!SeenBlocks.insert(BB).second) {
      return true;
    }
  }
  return false;
}

bool collectLoadChains(SmallVectorImpl<LoadVia> &Loads, PHINode *Phi) {
  struct Frame {
    Instruction *Node;
    SmallVector<Instruction *, 4> Chain;
  };
  SmallVector<Frame, 8> Stack;
  Stack.push_back({Phi, {}});

  auto pushNext = [&](Instruction *I, const SmallVector<Instruction *, 4> &Chain) {
    Frame Next;
    Next.Node = I;
    Next.Chain = Chain;
    Next.Chain.push_back(I);
    Stack.push_back(std::move(Next));
  };

  while (!Stack.empty()) {
    Frame F = Stack.pop_back_val();
    for (User *U : F.Node->users()) {
      if (auto *LI = dyn_cast<LoadInst>(U)) {
        if (!LI->isSimple()) {
          return false;
        }
        Loads.push_back({LI, F.Chain});
      } else if (isa<BitCastInst, AddrSpaceCastInst>(U)) {
        auto *CI = cast<Instruction>(U);
        pushNext(CI, F.Chain);
      } else if (auto *GEP = dyn_cast<GetElementPtrInst>(U)) {
        // Only follow GEPs rooted at the current node with all-constant
        // indices: constants dominate every predecessor, so the GEP can be
        // replayed per predecessor. Anything else (e.g. a dynamic index derived from
        // the merge) is not safely reproducible and disqualifies the merge.
        if (GEP->getPointerOperand() != F.Node || !GEP->hasAllConstantIndices()) {
          return false;
        }
        pushNext(GEP, F.Chain);
      } else {
        return false;
      }
    }
  }
  return true;
}

bool checkPHILoadSafety(PHINode *Phi, SmallVectorImpl<LoadVia> &Loads) {
  BasicBlock *MergeBB = Phi->getParent();

  // Each hoisted load is inserted before the terminator of its incoming block.
  // For that to be non-speculative the incoming block's sole successor must be
  // mergeBB: otherwise the block ends in a conditional branch and the hoisted
  // load would also execute on the edge(s) that bypass the merge, dereferencing
  // an address the original load-through-PHI never touched on that path (and
  // whose index may be out of range).
  for (BasicBlock *BBIn : Phi->blocks()) {
    if (BBIn->getSingleSuccessor() != MergeBB) {
      return false;
    }
  }

  for (const LoadVia &LV : Loads) {
    LoadInst *LI = LV.LI;
    // Reject cross-block loads: the load is in a successor block of the
    // phi's block.  There may be memory-writing instructions between the
    // phi's block terminator and the load that we cannot easily check.
    if (LI->getParent() != MergeBB) {
      return false;
    }
    // Same-block case: scan instructions between phi and load for any
    // instruction that may write to memory (stores, calls, atomics, etc.).
    auto It = std::next(Phi->getIterator());
    auto End = LI->getIterator();
    for (; It != End; ++It) {
      if (It->mayWriteToMemory() || !llvm::isGuaranteedToTransferExecutionToSuccessor(&*It)) {
        return false;
      }
    }
  }
  return true;
}

// Replay a cast chain from startPtr, stripping alloca-derived pre-merge
// casts and clamping addrspacecasts to the alloca's address space.
Value *cloneCastChainPHI(IRBuilder<> &B, Value *Root, ArrayRef<Instruction *> Chain, std::optional<unsigned> AllocaAS) {
  Value *Cur = Root;

  for (Instruction *I : Chain) {
    if (auto *GEP = dyn_cast<GetElementPtrInst>(I)) {
      Type *SrcElemTy = GEP->getSourceElementType();
      // The alloca incoming value had its pre-merge casts stripped, so under
      // typed pointers Cur may no longer carry the pointee type the original
      // GEP indexed. Restore it, staying in Cur's address space, before replaying.
      auto *CurPtrTy = cast<PointerType>(Cur->getType());
      if (!IGCLLVM::isOpaqueOrPointeeTypeMatches(CurPtrTy, SrcElemTy)) {
        Cur = B.CreateBitCast(Cur, IGCLLVM::PointerType::get(SrcElemTy, CurPtrTy->getAddressSpace()));
      }
      SmallVector<Value *, 4> Idxs(GEP->idx_begin(), GEP->idx_end());
      if (GEP->isInBounds()) {
        Cur = B.CreateInBoundsGEP(SrcElemTy, Cur, Idxs);
      } else {
        Cur = B.CreateGEP(SrcElemTy, Cur, Idxs);
      }
      continue;
    }
    IGC_ASSERT(isa<BitCastInst>(I) || isa<AddrSpaceCastInst>(I));
    auto *Cast = cast<CastInst>(I);
    Type *DestTy = Cast->getType();
    // Clamp addrspacecast destinations to AllocaAS to prevent
    // private→generic→global casts that cause GPU hangs when SoA/GRF
    // promotion is rejected and the surviving load reads from wrong memory
    if (AllocaAS.has_value()) {
      if (auto *PT = dyn_cast<PointerType>(DestTy)) {
        if (PT->getAddressSpace() != AllocaAS.value()) {
          DestTy = IGCLLVM::PointerType::get(PT, AllocaAS.value());
        }
      }
    }
    if (Cur->getType() == DestTy) {
      continue;
    }
    // Recompute the opcode rather than reusing Cast's: stripping the alloca
    // incoming value pre-merge casts and clamping DestTy to AllocaAS can
    // leave source and destination in the same address space, where the
    // original addrspacecast opcode is not a valid cast.
    Cur = B.CreateCast(CastInst::getCastOpcode(Cur, false, DestTy, false), Cur, DestTy);
  }
  return Cur;
}

// If Incoming is derived from an alloca, strip its leading bitcast /
// addrspacecast casts (but keep GEPs) back to the alloca-space base and report
// that base's address space, so the replayed post-merge cast chain can be
// clamped to it. Stripping the pre-merge addrspacecasts is what keeps the
// incoming value in private space and prevents a private→generic→global chain.
// For a non-alloca incoming value it is returned unchanged with no clamp.
Value *stripAllocaIncomingCasts(Value *Incoming, const DataLayout &DL, std::optional<unsigned> &AllocaAS) {
  AllocaAS = std::nullopt;
  if (!isa<AllocaInst>(IGCLLVM::getUnderlyingObject(Incoming, DL))) {
    return Incoming;
  }
  // Stripping bitcast/addrspacecast preserves the underlying object, so the
  // base stays rooted at the same alloca throughout.
  Value *Cur = Incoming;
  while (isa<BitCastInst, AddrSpaceCastInst>(Cur)) {
    Cur = cast<Instruction>(Cur)->getOperand(0);
  }
  AllocaAS = Cur->getType()->getPointerAddressSpace();
  return Cur;
}

// Only target large scalar array allocas that stand to benefit from SoA scratch promotion (PMR's domain).
// This avoids the IR explosion that would result from splitting every pointer merge.
bool isPHISplitWorthwhileForAlloca(AllocaInst *AI, const DataLayout &DL) {
  IGC_ASSERT(AI != nullptr);
  Type *AllocTy = AI->getAllocatedType();
  // Only flat [N x T] of arithmetic T qualifies for the target pattern.
  auto *ArrTy = dyn_cast<ArrayType>(AllocTy);
  if (!ArrTy) {
    return false;
  }
  Type *ETy = ArrTy->getElementType();
  if (!ETy->isFloatingPointTy() && !ETy->isIntegerTy()) {
    return false;
  }
  uint64_t Bytes = DL.getTypeAllocSize(AllocTy).getFixedValue();
  uint64_t MinBytes = (uint64_t)IGC_GET_FLAG_VALUE(PHIOfAllocaPtrSplitMinSize);
  return Bytes >= MinBytes;
}

bool runHoistPass(Function &F, bool VerifyAlloca, const DataLayout &DL, SmallVectorImpl<MergeHoistRecord> &OutRecords) {
  auto IsCandidate = [&](PHINode &Phi) {
    if (!Phi.getType()->isPointerTy()) {
      return false;
    }
    return llvm::any_of(Phi.incoming_values(), [&](Value *Inc) {
      auto *Root = dyn_cast<AllocaInst>(IGCLLVM::getUnderlyingObject(Inc, DL));
      return Root && (!VerifyAlloca || isPHISplitWorthwhileForAlloca(Root, DL));
    });
  };

  SmallVector<PHINode *, 16> Candidates;
  for (BasicBlock &BB : F) {
    for (PHINode &Phi : BB.phis()) {
      if (IsCandidate(Phi)) {
        Candidates.push_back(&Phi);
      }
    }
  }

  bool Changed = false;
  for (PHINode *Phi : Candidates) {
    if (hasDuplicateIncomingBlocks(Phi)) {
      continue;
    }

    SmallVector<LoadVia, 4> Loads;
    if (!collectLoadChains(Loads, Phi) || Loads.empty()) {
      continue;
    }

    if (!checkPHILoadSafety(Phi, Loads)) {
      continue;
    }

    auto ReplayChain = [&](IRBuilder<> &B, Value *StartPtr, ArrayRef<Instruction *> Chain) -> Value * {
      std::optional<unsigned> AllocaAS;
      Value *Base = stripAllocaIncomingCasts(StartPtr, DL, AllocaAS);
      return cloneCastChainPHI(B, Base, Chain, AllocaAS);
    };

    MergeHoistRecord Rec;

    for (const LoadVia &LV : Loads) {
      LoadInst *LI = LV.LI;
      PHINode *NewPhi = PHINode::Create(LI->getType(), Phi->getNumIncomingValues(), Phi->getName() + ".lowered",
                                        IGCLLVM::insertPosition(Phi));

      for (unsigned I = 0; I < Phi->getNumIncomingValues(); ++I) {
        BasicBlock *BBIn = Phi->getIncomingBlock(I);
        IRBuilder<> IRB(BBIn->getTerminator());
        Value *Ptr = ReplayChain(IRB, Phi->getIncomingValue(I), LV.Chain);
        LoadInst *NewLI = cast<LoadInst>(LI->clone());
        NewLI->setOperand(0, Ptr);
        IRB.Insert(NewLI);
        NewPhi->addIncoming(NewLI, BBIn);
      }
      LI->replaceAllUsesWith(NewPhi);
      Rec.push_back(LI);
      IGC_ASSERT(LI->use_empty());
    }

    OutRecords.push_back(std::move(Rec));
    Changed = true;
  }

  return Changed;
}

} // anonymous namespace

namespace IGC {
class SplitPHIsOfAllocaPointers : public FunctionPass {
public:
  static char ID;

  SplitPHIsOfAllocaPointers(bool VerifyAlloca = false);

  StringRef getPassName() const override { return "SplitPHIsOfAllocaPointers"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  bool runOnFunction(Function &F) override;

private:
  bool m_VerifyAlloca;
};

FunctionPass *createSplitPHIsOfAllocaPointers(bool VerifyAlloca) { return new SplitPHIsOfAllocaPointers(VerifyAlloca); }
} // namespace IGC

using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-split-phis-of-alloca-pointers"
#define PASS_DESCRIPTION "Split PHIs of alloca pointers into per-predecessor loads joined by a value PHI"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SplitPHIsOfAllocaPointers, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(SplitPHIsOfAllocaPointers, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char IGC::SplitPHIsOfAllocaPointers::ID = 0;

IGC::SplitPHIsOfAllocaPointers::SplitPHIsOfAllocaPointers(bool VerifyAlloca)
    : FunctionPass(ID), m_VerifyAlloca(VerifyAlloca) {
  initializeSplitPHIsOfAllocaPointersPass(*PassRegistry::getPassRegistry());
}

bool IGC::SplitPHIsOfAllocaPointers::runOnFunction(Function &F) {
  if (!IGC_IS_FLAG_ENABLED(EnablePHIOfAllocaPtrSplit)) {
    return false;
  }

  const DataLayout &DL = F.getParent()->getDataLayout();

  bool Changed = false;
  // Loop: committing one pointer merge may expose a new inner candidate (e.g. a
  // phi-of-phi whose outer incoming value only roots at the alloca once the
  // inner merge is lowered). Re-scan until a pass finds nothing new.
  SmallVector<MergeHoistRecord, 8> HoistRecords;
  for (int HoistIter = 0; HoistIter < MAX_HOISTING_ITER; ++HoistIter) {
    const size_t PrevCount = HoistRecords.size();
    runHoistPass(F, m_VerifyAlloca, DL, HoistRecords);
    if (HoistRecords.size() == PrevCount) {
      break;
    }
    // Commit only the newly added records.
    for (size_t J = PrevCount; J < HoistRecords.size(); ++J) {
      commitHoistRecord(HoistRecords[J]);
      Changed = true;
    }
  }
  return Changed;
}
