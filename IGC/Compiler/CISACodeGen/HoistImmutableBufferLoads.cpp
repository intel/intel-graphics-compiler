/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/HoistImmutableBufferLoads.hpp"

#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/ResourceAddrSpace.h"
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/IR/Instructions.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char HoistImmutableBufferLoads::ID = 0;

#define PASS_FLAG "igc-hoist-immutable-buffer-loads"
#define PASS_DESCRIPTION "Lift read-only buffer loads out of loops"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HoistImmutableBufferLoads, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(HoistImmutableBufferLoads, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

HoistImmutableBufferLoads::HoistImmutableBufferLoads() : FunctionPass(ID) {
  initializeHoistImmutableBufferLoadsPass(*PassRegistry::getPassRegistry());
}

FunctionPass *IGC::createHoistImmutableBufferLoadsPass() { return new HoistImmutableBufferLoads(); }

namespace {
/// Buffers whose contents cannot change while the shader runs. The list is the
/// one ConstantCoalescing uses when it decides a load needs no check for an
/// aliasing write.
bool isImmutableBufferAddrSpace(unsigned AS) {
  switch (DecodeBufferType(AS)) {
  case STATELESS_READONLY:
  case CONSTANT_BUFFER:
  case BINDLESS_CONSTANT_BUFFER:
  case SSH_BINDLESS_CONSTANT_BUFFER:
    return true;
  default:
    return false;
  }
}

/// A non-volatile read-only buffer load with loop-invariant arguments.
bool isCandidate(const Instruction &I, const Loop &L) {
  const auto *Ld = dyn_cast<LdRawIntrinsic>(&I);
  if (!Ld || Ld->isVolatile())
    return false;
  Type *ResTy = Ld->getResourceValue()->getType();
  if (!ResTy->isPointerTy() || !isImmutableBufferAddrSpace(ResTy->getPointerAddressSpace()))
    return false;
  return L.hasLoopInvariantOperands(Ld);
}
} // namespace

bool HoistImmutableBufferLoads::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  const DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  const unsigned MaxPerLoop = IGC_GET_FLAG_VALUE(HoistImmutableBufferLoadsMaxPerLoop);

  bool Changed = false;
  // Innermost loops first: a load lifted into an inner preheader is then a
  // candidate of the enclosing loop.
  auto Loops = LI.getLoopsInPreorder();
  for (Loop *L : llvm::reverse(Loops)) {
    BasicBlock *Preheader = L->getLoopPreheader();
    SmallVector<BasicBlock *, 4> Exiting;
    L->getExitingBlocks(Exiting);
    if (!Preheader || Exiting.empty())
      continue;

    // Identical reads of read-only memory yield one value (an unrolled loop
    // repeats them), so the budget counts distinct loads.
    SmallVector<Instruction *, 8> Candidates;
    SmallVector<Instruction *, 8> Distinct;
    bool OverBudget = false;
    for (BasicBlock *BB : L->blocks()) {
      if (LI.getLoopFor(BB) != L)
        continue;
      // A block that dominates every exiting block runs on every trip, so
      // lifting its loads into the preheader runs them no more often.
      if (!llvm::all_of(Exiting, [&](const BasicBlock *E) { return DT.dominates(BB, E); }))
        continue;
      for (Instruction &I : *BB) {
        if (!isCandidate(I, *L))
          continue;
        Candidates.push_back(&I);
        if (llvm::none_of(Distinct, [&](const Instruction *D) { return D->isIdenticalTo(&I); }))
          Distinct.push_back(&I);
        if (MaxPerLoop && Distinct.size() > MaxPerLoop) {
          OverBudget = true;
          break;
        }
      }
      if (OverBudget)
        break;
    }
    if (Candidates.empty() || OverBudget)
      continue;

    for (Instruction *I : Candidates) {
      IGCLLVM::moveBefore(I, Preheader->getTerminator());
      I->updateLocationAfterHoist();
    }
    Changed = true;
  }
  return Changed;
}
