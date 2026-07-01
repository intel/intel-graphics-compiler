/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/IR/Instructions.h"
#include "Compiler/Optimizer/BranchToSelect.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"

#define DEBUG_TYPE "igc-branch-to-select"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-branch-to-select"
#define PASS_DESCRIPTION "Flatten small memory-free branch regions into selects"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BranchToSelect, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(BranchToSelect, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char BranchToSelect::ID = 0;

BranchToSelect::BranchToSelect() : FunctionPass(ID) { initializeBranchToSelectPass(*PassRegistry::getPassRegistry()); }

namespace {
bool isFastFPType(const llvm::Type *T) {
  const llvm::Type *S = T->getScalarType();
  return S->isFloatTy() || S->isHalfTy() || S->isBFloatTy();
}

bool isWideScalar(const llvm::Type *T) {
  const llvm::Type *S = T->getScalarType();
  if (S->isIntegerTy())
    return S->getIntegerBitWidth() > 32;
  return S->isFloatingPointTy() && !isFastFPType(S);
}
} // namespace

std::optional<unsigned> BranchToSelect::speculationLatencyCost(const Instruction *I) const {
  switch (I->getOpcode()) {
  case Instruction::Freeze:
  case Instruction::Select:
  case Instruction::GetElementPtr:
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast:
  case Instruction::ExtractElement:
  case Instruction::InsertElement:
  case Instruction::ShuffleVector:
  case Instruction::ExtractValue:
  case Instruction::InsertValue:
    return 1;

  // Width-changing integer moves: cheap, but a >32-bit result (zext/sext to i64) manufactures a 2-GRF value we won't
  // speculate.
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
    if (isWideScalar(I->getType()))
      return std::nullopt;
    return 1;

  // Integer ALU: one native op at <=32-bit; 64-bit legalizes into a multi-op / 2-GRF sequence -- reject.
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
    if (isWideScalar(I->getType()))
      return std::nullopt;
    return 1;

  // Compares follow the operand width (the result is i1); reject a 64-bit / f64 compare.
  case Instruction::ICmp:
  case Instruction::FCmp:
    if (isWideScalar(I->getOperand(0)->getType()))
      return std::nullopt;
    return 1;

  // Integer multiply: only a <=16-bit multiply is a single native op. 32-bit is mul+mach and 64-bit is an emulation
  // sequence -- both expand into extra instructions, so reject rather than count as one.
  case Instruction::Mul:
    if (I->getType()->getScalarSizeInBits() > 16)
      return std::nullopt;
    return 1;

  // FP arithmetic on fast native types -- one ALU op. f64 is emulated: do not speculate.
  case Instruction::FAdd:
  case Instruction::FSub:
  case Instruction::FMul:
  case Instruction::FNeg:
    if (isFastFPType(I->getType()))
      return 1;
    return std::nullopt;

  case Instruction::FPToUI:
  case Instruction::FPToSI:
  case Instruction::UIToFP:
  case Instruction::SIToFP:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
    if (isWideScalar(I->getOperand(0)->getType()) || isWideScalar(I->getType()))
      return std::nullopt;
    return 1;

  default:
    break;
  }

  if (isa<CallInst>(I)) {
    // GenISA intrinsics are IntrinsicInsts too, so match them first.
    if (const auto *GII = dyn_cast<GenIntrinsicInst>(I)) {
      switch (GII->getIntrinsicID()) {
      case GenISAIntrinsic::GenISA_RuntimeValue:
      case GenISAIntrinsic::GenISA_simdLaneId:
      case GenISAIntrinsic::GenISA_simdSize:
      case GenISAIntrinsic::GenISA_bfrev:
        return 1;

      default:
        return std::nullopt;
      }
    }

    if (const auto *II = dyn_cast<IntrinsicInst>(I)) {
      switch (II->getIntrinsicID()) {
      case Intrinsic::fabs:
      case Intrinsic::abs:
      case Intrinsic::minnum:
      case Intrinsic::maxnum:
      case Intrinsic::ctpop:
      case Intrinsic::ctlz:
      case Intrinsic::floor:
      case Intrinsic::ceil:
        return 1;
      default:
        return std::nullopt;
      }
    }
  }

  // unspecified inst -- do not speculate
  return std::nullopt;
}

BranchToSelect::SuccessorKind BranchToSelect::classifySuccessor(BasicBlock *S, BasicBlock *P,
                                                                unsigned &SpeculationCost) const {
  if (S == P)
    return SuccessorKind::None;

  // Both foldable kinds fall through unconditionally to the merge block.
  auto *Br = dyn_cast<BranchInst>(S->getTerminator());
  if (!Br || Br->isConditional())
    return SuccessorKind::None;

  if (S->getSinglePredecessor() == P) {
    // Private successor: P is its only predecessor, so hoisting its body into P
    // and deleting it affects no other path. Accept it if its summed speculation
    // cost stays within budget (see speculationLatencyCost).
    for (Instruction &I : *S) {
      if (I.isTerminator())
        break;

      // Degenerate PHI (single predecessor): one incoming value that hoistInto
      // folds away. Skip it, like a debug intrinsic.
      if (isa<PHINode>(&I))
        continue;

      // Debug intrinsics hoist along but do not count against the budget.
      if (isa<DbgInfoIntrinsic>(&I))
        continue;

      // Defensive backstop, independent of the cost allow-list: never speculate a
      // convergent op (wave / subgroup). Hoisting it out of the branch and dropping
      // the reconvergence can change the lane mask it executes under, silently
      // altering its result. The allow-list already excludes these, but this guards
      // against a future entry being added without re-deriving that it is safe.
      if (const auto *CB = dyn_cast<CallBase>(&I))
        if (CB->isConvergent())
          return SuccessorKind::None;

      // A non-speculatable instruction disqualifies the successor outright.
      std::optional<unsigned> C = speculationLatencyCost(&I);
      if (!C)
        return SuccessorKind::None;

      // Sum the cost, if the instructions in the region get too expensive to hoist, bail out
      SpeculationCost += *C;
      if (SpeculationCost > m_maxSpeculatedCost)
        return SuccessorKind::None;
    }

    return SuccessorKind::Private;
  }

  // Otherwise the successor has predecessors besides P -- potentially a SimplifyCFG landing pad funneling several
  // short-circuit early-out edges (all carrying the same value) into one block. It cannot be hoisted without affecting
  // those preds, so accept it only as an empty pass-through: P is peeled off it (routed to the merge via a select) and
  // the pad is left intact. Emptiness also makes the peel dominance-safe -- the value it feeds the merge PHI is defined
  // in a block dominating the pad (hence P), legal to use in a select at the end of P.
  for (Instruction &I : *S) {
    if (I.isTerminator())
      break;
    if (isa<DbgInfoIntrinsic>(&I))
      continue;
    // Any real instruction (including a PHI) means it is not a pass-through.
    return SuccessorKind::None;
  }

  return SuccessorKind::SharedPad;
}

void BranchToSelect::hoistInto(BasicBlock *Blk, BasicBlock *P) {
  Instruction *InsertPt = P->getTerminator();
  for (auto II = Blk->begin(), IE = Blk->end(); II != IE;) {
    Instruction *I = &*II++;
    if (I->isTerminator())
      break;
    if (auto *Phi = dyn_cast<PHINode>(I)) {
      // Degenerate PHI (Blk's only pred is P): one incoming value that dominates
      // P. Fold it to that value rather than moving it (a PHI cannot move into
      // the middle of P). Erase now to avoid a transient malformed PHI in P.
      Phi->replaceAllUsesWith(Phi->getIncomingValue(0));
      Phi->eraseFromParent();
      continue;
    }
    IGCLLVM::moveBefore(I, InsertPt);
  }
}

bool BranchToSelect::tryConvert(BasicBlock *P) {
  auto *Br = dyn_cast<BranchInst>(P->getTerminator());
  if (!Br || !Br->isConditional())
    return false;

  Value *Cond = Br->getCondition();
  BasicBlock *TrueS = Br->getSuccessor(0);  // taken when Cond == true
  BasicBlock *FalseS = Br->getSuccessor(1); // taken when Cond == false

  if (TrueS == FalseS)
    return false;

  // Classify the region. A successor is "foldable" when its kind is not None;
  // that block (TrueS/FalseS) is what gets folded. Results:
  //   MB                 - merge block holding the PHIs to convert
  //   TrueKind/FalseKind - foldable kind of each successor (None if not foldable)
  //   TrueIn/FalseIn     - PHI pred carrying the true/false value (successor, or P)
  BasicBlock *MB = nullptr;
  BasicBlock *TrueIn = nullptr;
  BasicBlock *FalseIn = nullptr;
  unsigned TrueSpeculationCost = 0, FalseSpeculationCost = 0;
  SuccessorKind TrueKind = classifySuccessor(TrueS, P, TrueSpeculationCost);
  SuccessorKind FalseKind = classifySuccessor(FalseS, P, FalseSpeculationCost);
  bool TrueFoldable = TrueKind != SuccessorKind::None;
  bool FalseFoldable = FalseKind != SuccessorKind::None;

  if (TrueFoldable && FalseFoldable && TrueS->getSingleSuccessor() == FalseS->getSingleSuccessor()) {
    // Diamond: both successors are foldable and meet at a common merge.
    MB = TrueS->getSingleSuccessor();
    TrueIn = TrueS;
    FalseIn = FalseS;
  } else if (TrueFoldable && TrueS->getSingleSuccessor() == FalseS) {
    // Triangle: true side is folded, false side goes straight to the merge.
    MB = FalseS;
    TrueIn = TrueS;
    FalseIn = P;
    FalseKind = SuccessorKind::None;
  } else if (FalseFoldable && FalseS->getSingleSuccessor() == TrueS) {
    // Triangle: false side is folded, true side goes straight to the merge.
    MB = TrueS;
    TrueIn = P;
    FalseIn = FalseS;
    TrueKind = SuccessorKind::None;
  } else {
    return false;
  }

  // The merge cannot be the converting block itself (that would be a loop), nor a block we are about to fold.
  if (MB == P || (TrueKind != SuccessorKind::None && MB == TrueS) || (FalseKind != SuccessorKind::None && MB == FalseS))
    return false;

  // This fold's cost is simply the number of speculatable instructions hoisted from the private successors; a shared
  // pad is empty and adds nothing. The emitted selects are not charged.
  unsigned FoldCost = (TrueKind == SuccessorKind::Private ? TrueSpeculationCost : 0) +
                      (FalseKind == SuccessorKind::Private ? FalseSpeculationCost : 0);

  // If this fold will let us merge MB into P below, MB's already-accumulated body joins P's block, so it must count
  // against the budget now. The merge fires when MB's only predecessor becomes P -- i.e. its only current predecessors
  // are P (a triangle's direct edge) and the private arms we are about to delete. Declining the fold is the only way to
  // keep MB's cost out: SimplifyCFG would fuse the straight-line P->MB anyway, so refusing just the merge is futile.
  bool WillMerge = true;
  for (BasicBlock *Pred : predecessors(MB)) {
    bool FoldedArm = (Pred == TrueS && TrueKind == SuccessorKind::Private) ||
                     (Pred == FalseS && FalseKind == SuccessorKind::Private);
    if (Pred != P && !FoldedArm) {
      WillMerge = false;
      break;
    }
  }

  // Cumulative region gate: bail if this fold (plus the body it would absorb via the merge) would push P's linearized
  // region past the budget.
  unsigned PriorCost = m_regionCost.lookup(P);
  unsigned MergeCost = WillMerge ? m_regionCost.lookup(MB) : 0;
  if (PriorCost + FoldCost + MergeCost > m_maxRegionCost)
    return false;

  // Hoist private-successor instructions into P before materializing the
  // selects, so the select operands are defined in P and dominate the merge.
  // Shared pads are empty and stay in place, so there is nothing to hoist.
  if (TrueKind == SuccessorKind::Private)
    hoistInto(TrueS, P);
  if (FalseKind == SuccessorKind::Private)
    hoistInto(FalseS, P);

  // Build one select per merge PHI. Read every incoming value first; only then
  // mutate the PHIs, since removing incomings invalidates indices.
  IRBuilder<> Builder(P->getTerminator());
  SmallVector<std::pair<PHINode *, Value *>, 8> NewValues;
  for (PHINode &Phi : MB->phis()) {
    Value *VTrue = Phi.getIncomingValueForBlock(TrueIn);
    Value *VFalse = Phi.getIncomingValueForBlock(FalseIn);
    Value *NewVal = VTrue;
    if (VTrue != VFalse) {
      Builder.SetCurrentDebugLocation(Phi.getDebugLoc());
      NewVal = Builder.CreateSelect(Cond, VTrue, VFalse);
    }
    NewValues.emplace_back(&Phi, NewVal);
  }

  // A diamond is where both successors are folded; a triangle folds exactly one
  // (the other is P's direct edge to the merge). Derive this from the final
  // kinds, since a triangle zeroes the merge side's kind above.
  bool IsDiamond = TrueKind != SuccessorKind::None && FalseKind != SuccessorKind::None;
  for (auto &PV : NewValues) {
    PHINode *Phi = PV.first;
    Value *NewVal = PV.second;

    // Drop the incoming edge of each *private* successor: block is being deleted.
    // Keep a *shared pad*'s incoming -- it still carries the pad's other
    // predecessors, so the pad and that PHI entry survive the peel.
    if (TrueKind == SuccessorKind::Private)
      Phi->removeIncomingValue(TrueS, /*DeletePHIIfEmpty*/ false);
    if (FalseKind == SuccessorKind::Private)
      Phi->removeIncomingValue(FalseS, /*DeletePHIIfEmpty*/ false);

    if (IsDiamond) {
      // P had no direct edge into the merge; it now carries the selected value.
      Phi->addIncoming(NewVal, P);
    } else {
      // Triangle: P's existing direct edge carries the selected value.
      Phi->setIncomingValueForBlock(P, NewVal);
    }
  }

  // Replace P's conditional branch with an unconditional branch to the merge.
  // This also removes P's edge to any shared pad it was peeled off of.
  DebugLoc BrDL = Br->getDebugLoc();
  Br->eraseFromParent();
  BranchInst::Create(MB, P)->setDebugLoc(BrDL);

  // Private successors are now empty and unreferenced -- erase them, dropping any
  // region cost recorded against them so a recycled BasicBlock address cannot alias
  // a stale entry. Shared pads still have other predecessors, so leave them for a
  // later SimplifyCFG sweep.
  if (TrueKind == SuccessorKind::Private) {
    m_regionCost.erase(TrueS);
    TrueS->eraseFromParent();
  }
  if (FalseKind == SuccessorKind::Private) {
    m_regionCost.erase(FalseS);
    FalseS->eraseFromParent();
  }

  LLVM_DEBUG(dbgs() << "BranchToSelect: linearized region into " << P->getName() << " merging to " << MB->getName()
                    << "\n");

  // P now branches unconditionally to MB. When P is MB's only predecessor they are
  // a straight-line pair, so merge MB into P. Besides collapsing the degenerate
  // merge PHIs immediately, this hands P the merge's terminator, which exposes any
  // triangle/diamond rooted at P's own predecessors that spans the old P->MB seam
  // -- regions the single-block classifier cannot see. Because BranchToSelect runs
  // only once, before the later SimplifyCFGs, those folds would otherwise be lost.
  // MB's own accumulated body (if any) now lives in P, so it joins P's region cost.
  bool Merged = false;
  unsigned MBRegionCost = m_regionCost.lookup(MB);
  if (MB->getSinglePredecessor() == P) {
    m_regionCost.erase(MB); // Delete first in case MB pointer is freed
    Merged = MergeBlockIntoPredecessor(MB);
  }
  if (!Merged) {
    m_regionCost[MB] = MBRegionCost; // Restore MB if not merged
    m_regionCost[P] = PriorCost + FoldCost;
  } else {
    m_regionCost[P] = PriorCost + FoldCost + MBRegionCost;
  }

  return true;
}

bool BranchToSelect::runOnFunction(Function &F) {
  m_maxSpeculatedCost = IGC_GET_FLAG_VALUE(BranchToSelectMaxSpeculatedCost);
  m_maxRegionCost = IGC_GET_FLAG_VALUE(BranchToSelectMaxRegionCost);
  m_regionCost.clear();

  bool Changed = false;
  bool LocalChanged = true;
  while (LocalChanged) {
    LocalChanged = false;
    for (auto BI = F.begin(), BE = F.end(); BI != BE;) {
      BasicBlock *BB = &*BI++;
      if (tryConvert(BB)) {
        Changed = true;
        LocalChanged = true;
        break;
      }
    }
  }

  return Changed;
}

namespace IGC {
FunctionPass *createBranchToSelectPass() { return new BranchToSelect(); }
} // namespace IGC
