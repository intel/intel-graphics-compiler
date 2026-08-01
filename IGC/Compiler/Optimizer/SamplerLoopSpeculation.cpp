/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SamplerLoopSpeculation.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"
#include "common/EmUtils.h"
#include "llvmWrapper/IR/Instructions.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Metadata.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/Utils.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-sampler-loop-speculation"
#define PASS_DESCRIPTION "Speculatively issue sampler iterations of an unrolled loop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SamplerLoopSpeculation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(SamplerLoopSpeculation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC {

char SamplerLoopSpeculation::ID = 0;

SamplerLoopSpeculation::SamplerLoopSpeculation() : llvm::LoopPass(ID) {
  initializeSamplerLoopSpeculationPass(*PassRegistry::getPassRegistry());
}

void SamplerLoopSpeculation::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  // The transform does not modify the CFG, so the dominator tree and loop info
  // remain valid. Instructions move earlier along a dominance chain while
  // preserving SSA dominance and LCSSA form.
  AU.setPreservesCFG();
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<LoopInfoWrapperPass>();
  AU.addPreservedID(LCSSAID);
}

StringRef SamplerLoopSpeculation::getPassName() const { return "IGC Sampler Loop Speculation"; }

bool SamplerLoopSpeculation::CollectSpeculativeAddressSlice(Value *V, Instruction *InsertBefore, Loop *L,
                                                            SmallPtrSetImpl<Instruction *> &Visited,
                                                            SmallVectorImpl<Instruction *> &AddressSlice) {
  Instruction *I = dyn_cast<Instruction>(V);
  if (!I)
    return true;

  // A later address must not depend on any sampler result, including the
  // first sampler at the issue point.
  if (isa<SampleIntrinsic>(I))
    return false;

  if (m_pDT->dominates(I, InsertBefore))
    return true;

  bool SafeToSpeculate = isSafeToSpeculativelyExecute(I);
  if (!SafeToSpeculate) {
    if (auto *CB = dyn_cast<CallBase>(I)) {
      Function *Callee = CB->getCalledFunction();

      // LLVM requires the speculatable attribute even for calls that have no
      // observable side effects. For this experimental transform, locally
      // accept a direct, readnone, nounwind, willreturn, non-convergent call.
      SafeToSpeculate = Callee && Callee->doesNotAccessMemory() && Callee->doesNotThrow() && Callee->willReturn() &&
                        !Callee->isConvergent();
    }
  }

  if (!L->contains(I) || isa<PHINode>(I) || I->isTerminator() || I->mayReadOrWriteMemory() || !SafeToSpeculate ||
      !m_pDT->dominates(InsertBefore->getParent(), I->getParent()))
    return false;

  if (!Visited.insert(I).second)
    return true;

  for (Value *Operand : I->operand_values()) {
    if (!CollectSpeculativeAddressSlice(Operand, InsertBefore, L, Visited, AddressSlice))
      return false;
  }

  AddressSlice.push_back(I);
  return true;
}

bool SamplerLoopSpeculation::ProcessSamplerLoop(Loop *L) {
  SmallVector<SampleIntrinsic *, 8> Samples;

  for (BasicBlock *BB : L->blocks()) {
    for (Instruction &I : *BB) {
      if (auto *Sample = dyn_cast<SampleIntrinsic>(&I)) {
        Samples.push_back(Sample);
        continue;
      }

      // Clustering speculates later iterations. Keep the initial form limited
      // to loops without observable writes or other side effects.
      if (I.mayHaveSideEffects())
        return false;
    }
  }

  if (Samples.size() < 2)
    return false;

  Function *SampleDecl = Samples.front()->getCalledFunction();
  if (!SampleDecl)
    return false;
  for (SampleIntrinsic *Sample : Samples) {
    if (Sample->getCalledFunction() != SampleDecl)
      return false;
  }

  // The cloned iterations must form one dominance chain. Reconstruct it
  // without sorting on a partial order: at each position there must be one
  // sampler that dominates every remaining sampler.
  SmallVector<SampleIntrinsic *, 8> OrderedSamples;
  SmallPtrSet<SampleIntrinsic *, 8> Ordered;
  while (OrderedSamples.size() != Samples.size()) {
    SampleIntrinsic *Next = nullptr;
    for (SampleIntrinsic *Candidate : Samples) {
      if (Ordered.contains(Candidate))
        continue;

      bool DominatesRemaining = true;
      for (SampleIntrinsic *Other : Samples) {
        if (Candidate == Other || Ordered.contains(Other))
          continue;
        DominatesRemaining &= m_pDT->dominates(Candidate, Other);
      }

      if (DominatesRemaining) {
        if (Next)
          return false;
        Next = Candidate;
      }
    }

    if (!Next)
      return false;
    Ordered.insert(Next);
    OrderedSamples.push_back(Next);
  }

  Instruction *FirstSample = OrderedSamples.front();
  SmallPtrSet<Instruction *, 32> Visited;
  SmallVector<Instruction *, 32> AddressSlice;
  for (SampleIntrinsic *Sample : llvm::drop_begin(OrderedSamples)) {
    for (Value *Operand : Sample->operand_values()) {
      if (!CollectSpeculativeAddressSlice(Operand, FirstSample, L, Visited, AddressSlice))
        return false;
    }
  }

  // Materialize every independent address before issuing the sampler cluster.
  for (Instruction *I : AddressSlice)
    IGCLLVM::moveBefore(I, FirstSample);

  Instruction *IssueCursor = FirstSample;
  for (SampleIntrinsic *Sample : llvm::drop_begin(OrderedSamples)) {
    IGCLLVM::moveBefore(Sample, IssueCursor->getNextNode());
    IssueCursor = Sample;
  }

  // Preserve the latency cluster through the later CodeSinking pass. Mark the
  // anchor as well as the relocated samples because sinking any member would
  // reintroduce a result-dependent wait between sampler sends.
  for (SampleIntrinsic *Sample : OrderedSamples)
    Sample->setMetadata(MD_LATENCY_HOISTED_SAMPLE, MDNode::get(Sample->getContext(), {}));

  return true;
}

bool SamplerLoopSpeculation::runOnLoop(Loop *L, LPPassManager & /*LPM*/) {
  // Operate on innermost loops only; the transform works within one loop body.
  if (!L->isInnermost())
    return false;

  m_pDT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  return ProcessSamplerLoop(L);
}

llvm::LoopPass *createSamplerLoopSpeculation() { return new SamplerLoopSpeculation(); }

} // namespace IGC
