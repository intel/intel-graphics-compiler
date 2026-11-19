/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/SplitPHIsOfAllocaPointers.hpp"
#include "Compiler/CISACodeGen/PredicatedSplitter.hpp"
#include "Compiler/CISACodeGen/SoAAcrossFNUtil.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

#include "common/igc_regkeys.hpp"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Analysis/ValueTracking.h"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"

#include <optional>

using namespace llvm;
using IGC::PredicatedSplitter;
using IGC::SoAAcrossFNUtil::LoadVia;

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

bool hasDuplicateIncomingBlocks(PHINode *Phi) {
  SmallPtrSet<BasicBlock *, 4> SeenBlocks;
  for (BasicBlock *BB : Phi->blocks()) {
    if (!SeenBlocks.insert(BB).second) {
      return true;
    }
  }
  return false;
}

bool isReplayableAddressStep(const User *U, const Value *Base) {
  if (isa<BitCastInst, AddrSpaceCastInst>(U)) {
    return true;
  }
  if (const auto *GEP = dyn_cast<GetElementPtrInst>(U)) {
    return GEP->getPointerOperand() == Base && GEP->hasAllConstantIndices();
  }
  return false;
}

bool collectLoadChainsFrom(Instruction *Node, SmallVectorImpl<Instruction *> &Chain, SmallVectorImpl<LoadVia> &Loads) {
  for (User *U : Node->users()) {
    if (auto *LI = dyn_cast<LoadInst>(U)) {
      if (!LI->isSimple()) {
        return false;
      }
      Loads.push_back({LI, SmallVector<Instruction *, 4>(Chain.begin(), Chain.end())});
      continue;
    }
    if (auto *PL = dyn_cast<PredicatedLoadIntrinsic>(U)) {
      if (PL->getPointerOperand() != Node) {
        return false;
      }
      Loads.push_back({PL, SmallVector<Instruction *, 4>(Chain.begin(), Chain.end())});
      continue;
    }
    if (!isReplayableAddressStep(U, Node)) {
      return false;
    }
    auto *Step = cast<Instruction>(U);
    Chain.push_back(Step);
    bool Ok = collectLoadChainsFrom(Step, Chain, Loads);
    Chain.pop_back();
    if (!Ok) {
      return false;
    }
  }
  return true;
}

// Collects every load reachable from PHI through bitcasts, addrspacecasts and
// constant-index GEPs. Fails if the merged pointer has any other kind of use.
bool collectLoadChains(SmallVectorImpl<LoadVia> &Loads, PHINode *Phi) {
  SmallVector<Instruction *, 4> Chain;
  return collectLoadChainsFrom(Phi, Chain, Loads);
}

bool isNonPredicatedLoadSafe(PHINode *Phi, SmallVectorImpl<LoadVia> &Loads) {
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
    Instruction *Load = LV.Load;

    // Reject predicated loads when their predicate or merge value is defined in the PHI block.
    if (auto *PL = dyn_cast<PredicatedLoadIntrinsic>(Load)) {
      for (Value *Op : {PL->getPredicate(), PL->getMergeValue()}) {
        auto *OpI = dyn_cast<Instruction>(Op);
        if (OpI && OpI->getParent() == MergeBB) {
          return false;
        }
      }
    }
    // Reject cross-block loads: the load is in a successor block of the
    // phi's block.  There may be memory-writing instructions between the
    // phi's block terminator and the load that we cannot easily check.
    if (Load->getParent() != MergeBB) {
      return false;
    }
    // Same-block case: scan instructions between phi and load for any
    // instruction that may write to memory (stores, calls, atomics, etc.).
    auto It = std::next(Phi->getIterator());
    auto End = Load->getIterator();
    for (; It != End; ++It) {
      if (It->mayWriteToMemory() || !llvm::isGuaranteedToTransferExecutionToSuccessor(&*It)) {
        return false;
      }
    }
  }
  return true;
}

// Replaces LV.Load, which loads through PHI via LV.Chain, with a value PHI of
// per-predecessor loads. Each predecessor replays LV.Chain on its incoming
// pointer and loads before its terminator.
void splitLoadThroughPhi(PHINode *Phi, const LoadVia &LV, const DataLayout &DL,
                         const DenseSet<const Value *> &ExtraRoots) {
  Instruction *Load = LV.Load;
  PHINode *NewPhi = PHINode::Create(Load->getType(), Phi->getNumIncomingValues(), Phi->getName() + ".lowered",
                                    IGCLLVM::insertPosition(Phi));
  for (unsigned I = 0, E = Phi->getNumIncomingValues(); I < E; ++I) {
    BasicBlock *BBIn = Phi->getIncomingBlock(I);
    IRBuilder<> IRB(BBIn->getTerminator());
    Value *Ptr = IGC::SoAAcrossFNUtil::replayAddressChain(IRB, Phi->getIncomingValue(I), LV.Chain, DL, ExtraRoots);
    Instruction *NewLoad = nullptr;
    if (isa<PredicatedLoadIntrinsic>(Load)) {
      NewLoad = IGC::SoAAcrossFNUtil::createPredicatedLoad(IRB, Load, Ptr, IRB.getTrue());
    } else {
      NewLoad = Load->clone();
      NewLoad->setOperand(LoadInst::getPointerOperandIndex(), Ptr);
      IRB.Insert(NewLoad);
    }
    NewPhi->addIncoming(NewLoad, BBIn);
  }
  Load->replaceAllUsesWith(NewPhi);
}

bool runHoistPass(Function &F, bool VerifyAlloca, const DataLayout &DL, const DenseSet<const Value *> &ExtraRoots,
                  bool UsePredicatedLoads) {
  std::optional<DominatorTree> DT;
  auto GetDT = [&]() -> DominatorTree & {
    if (!DT) {
      DT.emplace(F);
    }
    return *DT;
  };
  auto IsCandidate = [&](PHINode &Phi) {
    if (!Phi.getType()->isPointerTy()) {
      return false;
    }
    if (Phi.getNumIncomingValues() < 2)
      return false;
    bool MergesRoot = llvm::any_of(Phi.incoming_values(), [&](Value *Inc) {
      Value *Obj = IGCLLVM::getUnderlyingObject(Inc, DL);
      if (ExtraRoots.contains(Obj)) {
        return true;
      }
      auto *Root = dyn_cast<AllocaInst>(Obj);
      return Root && (!VerifyAlloca || IGC::SoAAcrossFNUtil::isWorthwhileForAlloca(Root, DL));
    });
    if (!MergesRoot)
      return false;
    // Unreachable code may define a value in terms of itself; don't try to split through it.
    return llvm::none_of(Phi.incoming_values(), [&](Value *Inc) {
      auto *IncI = dyn_cast<Instruction>(Inc);
      return IncI && !GetDT().isReachableFromEntry(IncI->getParent());
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

  SmallVector<WeakTrackingVH, 8> DeadLoads;
  for (PHINode *Phi : Candidates) {
    if (hasDuplicateIncomingBlocks(Phi)) {
      continue;
    }

    SmallVector<LoadVia, 4> Loads;
    if (!collectLoadChains(Loads, Phi) || Loads.empty()) {
      continue;
    }

    if (!isNonPredicatedLoadSafe(Phi, Loads)) {
      if (!UsePredicatedLoads) {
        continue;
      }
      PredicatedSplitter Splitter(Phi, GetDT(), DL, ExtraRoots);
      if (!Splitter.canSplit()) {
        continue;
      }
      Splitter.split(Loads);
      for (const LoadVia &LV : Loads) {
        DeadLoads.push_back(LV.Load);
      }
      continue;
    }
    // PHI and LOAD in single block
    llvm::sort(Loads, [](const LoadVia &A, const LoadVia &B) { return A.Load->comesBefore(B.Load); });

    for (const LoadVia &LV : Loads) {
      splitLoadThroughPhi(Phi, LV, DL, ExtraRoots);
      DeadLoads.push_back(LV.Load);
    }
  }

  bool Changed = !DeadLoads.empty();
  RecursivelyDeleteTriviallyDeadInstructionsPermissive(DeadLoads);
  return Changed;
}

} // anonymous namespace

namespace IGC {
class SplitPHIsOfAllocaPointers : public ModulePass {
public:
  static char ID;

  SplitPHIsOfAllocaPointers();
  SplitPHIsOfAllocaPointers(bool VerifyAlloca, bool CheckCrossFnArgs);

  StringRef getPassName() const override { return "SplitPHIsOfAllocaPointers"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    if (m_CheckCrossFnArgs && IGC_IS_FLAG_ENABLED(EnableSOAAcrossCalls)) {
      AU.addRequired<CodeGenContextWrapper>();
    }
  }

  bool runOnModule(Module &M) override;

private:
  bool m_VerifyAlloca;
  bool m_CheckCrossFnArgs;
};

ModulePass *createSplitPHIsOfAllocaPointers(bool VerifyAlloca, bool CheckCrossFnArgs) {
  return new SplitPHIsOfAllocaPointers(VerifyAlloca, CheckCrossFnArgs);
}
} // namespace IGC

using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-split-phis-of-alloca-pointers"
#define PASS_DESCRIPTION "Split PHIs of alloca pointers into per-predecessor loads joined by a value PHI"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SplitPHIsOfAllocaPointers, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SplitPHIsOfAllocaPointers, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char IGC::SplitPHIsOfAllocaPointers::ID = 0;

IGC::SplitPHIsOfAllocaPointers::SplitPHIsOfAllocaPointers()
    : SplitPHIsOfAllocaPointers(/*VerifyAlloca*/ false, /*CheckCrossFnArgs*/ true) {}

IGC::SplitPHIsOfAllocaPointers::SplitPHIsOfAllocaPointers(bool VerifyAlloca, bool CheckCrossFnArgs)
    : ModulePass(ID), m_VerifyAlloca(VerifyAlloca), m_CheckCrossFnArgs(CheckCrossFnArgs) {
  initializeSplitPHIsOfAllocaPointersPass(*PassRegistry::getPassRegistry());
}

bool IGC::SplitPHIsOfAllocaPointers::runOnModule(Module &M) {
  if (!IGC_IS_FLAG_ENABLED(EnablePHIOfAllocaPtrSplit)) {
    return false;
  }

  IGCMD::MetaDataUtils *MdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  DenseSet<const Value *> RootArgs;
  bool UsePredicatedLoads = false;
  if (m_CheckCrossFnArgs && IGC_IS_FLAG_ENABLED(EnableSOAAcrossCalls) && IGC_IS_FLAG_DISABLED(DisableSOAPromotion)) {
    SoAAcrossFNUtil::collectRootArgs(M, getAnalysisIfAvailable<GenXFunctionGroupAnalysis>(), MdUtils, m_VerifyAlloca,
                                     RootArgs);
    CodeGenContext *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    UsePredicatedLoads = Ctx->platform.hasLSC() && Ctx->platform.LSCEnabled();
  }

  const DataLayout &DL = M.getDataLayout();

  bool Changed = false;
  for (Function &F : M) {
    if (F.isDeclaration() || MdUtils->findFunctionsInfoItem(&F) == MdUtils->end_FunctionsInfo()) {
      continue;
    }
    // Loop: lowering one pointer merge may expose a new inner candidate (e.g. a
    // phi-of-phi whose outer incoming value only roots at the alloca once the
    // inner merge is lowered). Re-scan until a pass finds nothing new.
    for (int HoistIter = 0; HoistIter < MAX_HOISTING_ITER; ++HoistIter) {
      if (!runHoistPass(F, m_VerifyAlloca, DL, RootArgs, UsePredicatedLoads)) {
        break;
      }
      Changed = true;
    }
  }
  return Changed;
}
