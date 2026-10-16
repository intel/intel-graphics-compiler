/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/HoistImmutableLoadsAboveBarriers.hpp"

#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SetVector.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/ResourceAddrSpace.h"
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/IR/Instructions.h"

using namespace llvm;
using namespace IGC;

char HoistImmutableLoadsAboveBarriers::ID = 0;

#define PASS_FLAG "igc-hoist-immutable-loads-above-barriers"
#define PASS_DESCRIPTION "Move read-only buffer loads above preceding barriers and fences"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HoistImmutableLoadsAboveBarriers, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(HoistImmutableLoadsAboveBarriers, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

HoistImmutableLoadsAboveBarriers::HoistImmutableLoadsAboveBarriers() : FunctionPass(ID) {
  initializeHoistImmutableLoadsAboveBarriersPass(*PassRegistry::getPassRegistry());
}

FunctionPass *IGC::createHoistImmutableLoadsAboveBarriersPass() { return new HoistImmutableLoadsAboveBarriers(); }

void HoistImmutableLoadsAboveBarriers::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
}

namespace {
/// Buffers whose contents cannot change while the shader runs. The list is the
/// one ConstantCoalescing uses when it decides a load needs no check for an
/// aliasing write.
bool isImmutableBuffer(BufferType T) {
  switch (T) {
  case STATELESS_READONLY:
  case CONSTANT_BUFFER:
  case BINDLESS_CONSTANT_BUFFER:
  case SSH_BINDLESS_CONSTANT_BUFFER:
    return true;
  default:
    return false;
  }
}

// Bounds the address computation carried along with a load.
constexpr unsigned MaxChainLength = 8;

bool isImmutableRead(const Instruction &I) {
  unsigned AS = 0;
  if (auto *LI = dyn_cast<LoadInst>(&I)) {
    if (LI->isVolatile())
      return false;
    AS = LI->getPointerAddressSpace();
  } else if (auto *Ld = dyn_cast<LdRawIntrinsic>(&I)) {
    Type *ResTy = Ld->getResourceValue()->getType();
    if (Ld->isVolatile() || !ResTy->isPointerTy())
      return false;
    AS = ResTy->getPointerAddressSpace();
  } else {
    return false;
  }
  return isImmutableBuffer(DecodeBufferType(AS));
}

bool isWait(const Instruction &I) {
  auto *GII = dyn_cast<GenIntrinsicInst>(&I);
  if (!GII)
    return false;
  // isBarrierIntrinsic returns true for GenISA_wavebarrier, but
  // it's not a wait instruction, it is just a scheduling barrier.
  if (GII->getIntrinsicID() == GenISAIntrinsic::GenISA_wavebarrier)
    return false;
  if (isBarrierIntrinsic(&I))
    return true;
  switch (GII->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_LSCFence:
  case GenISAIntrinsic::GenISA_memoryfence:
  case GenISAIntrinsic::GenISA_typedmemoryfence:
    return true;
  default:
    return false;
  }
}

/// The first wait of the nearest run of consecutive waits (e.g. a fence and
/// its barrier) before I in I's block, if I follows that run closely enough
/// for the read's latency to be on the critical path. Further down, the work
/// in between already hides the latency, and moving the read only keeps its
/// value live across that work. An earlier run is never used: the read would
/// stay live across the whole region between the runs.
Instruction *waitBefore(Instruction &I) {
  const unsigned MaxDistance = IGC_GET_FLAG_VALUE(HoistImmutableLoadsAboveBarriersMaxDistance);
  Instruction *RunStart = nullptr;
  bool InRun = false;
  unsigned Distance = 0;
  for (Instruction &J : *I.getParent()) {
    if (&J == &I)
      return RunStart && (!MaxDistance || Distance <= MaxDistance) ? RunStart : nullptr;
    if (isWait(J)) {
      if (!InRun)
        RunStart = &J;
      InRun = true;
      Distance = 0;
    } else {
      InRun = false;
      ++Distance;
    }
  }
  return nullptr;
}

/// Whether the read's value is needed within MaxUseDistance instructions of
/// the read in its block, looking through extracts and bitcasts. Only then is
/// its latency on the critical path right after the wait; a value consumed
/// later already overlaps the work in between.
bool isUsedSoon(Instruction *Ld) {
  const unsigned MaxUseDistance = IGC_GET_FLAG_VALUE(HoistImmutableLoadsAboveBarriersMaxUseDistance);
  if (!MaxUseDistance)
    return true;
  SmallPtrSet<Instruction *, 8> Values{Ld};
  unsigned Distance = 0;
  for (Instruction *I = Ld->getNextNode(); I && Distance < MaxUseDistance; I = I->getNextNode(), ++Distance) {
    if (llvm::none_of(I->operands(), [&](Value *Op) { return Values.count(dyn_cast<Instruction>(Op)); }))
      continue;
    if (!isa<ExtractElementInst, BitCastInst>(I))
      return true;
    Values.insert(I);
  }
  return false;
}

/// Collects, in definition order, the instructions that have to move together
/// with Ld so that every operand is available at IP. Fails if one of them is
/// not a pure computation.
bool collectChain(Instruction *Ld, Instruction *IP, DominatorTree &DT, SetVector<Instruction *> &Chain) {
  SmallVector<std::pair<Instruction *, unsigned>, 8> Stack{{Ld, 0}};
  SmallPtrSet<Instruction *, 8> Visiting{Ld};
  while (!Stack.empty()) {
    auto &[I, Idx] = Stack.back();
    if (Idx == I->getNumOperands()) {
      Chain.insert(I);
      Stack.pop_back();
      continue;
    }
    auto *Op = dyn_cast<Instruction>(I->getOperand(Idx++));
    if (!Op || Chain.count(Op) || DT.dominates(Op, IP))
      continue;
    if (isa<PHINode>(Op) || Op->mayReadOrWriteMemory() || Op->mayHaveSideEffects() || !Visiting.insert(Op).second ||
        Visiting.size() > MaxChainLength)
      return false;
    Stack.push_back({Op, 0});
  }
  return true;
}
} // namespace

bool HoistImmutableLoadsAboveBarriers::runOnFunction(Function &F) {
  if (IGC_IS_FLAG_ENABLED(DisableHoistImmutableLoadsAboveBarriers))
    return false;

  auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  SmallVector<Instruction *, 8> Candidates;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (isImmutableRead(I) && waitBefore(I))
        Candidates.push_back(&I);

  bool Changed = false;
  for (Instruction *Ld : Candidates) {
    // The read goes directly above the wait, not as high as it could legally
    // go. Higher up it tends to land right after the load that produces its
    // address, or among other loads that hold every scoreboard token, and
    // stalls there instead of overlapping the wait. Directly above the wait its
    // operands are long ready and the wait hides its latency.
    Instruction *Wait = waitBefore(*Ld);
    SetVector<Instruction *> Chain;
    if (!Wait || !isUsedSoon(Ld) || !collectChain(Ld, Wait, DT, Chain))
      continue;
    for (Instruction *I : Chain)
      IGCLLVM::moveBefore(I, Wait);
    Changed = true;
  }
  return Changed;
}
