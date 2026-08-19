/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// BranchToSelect linearizes small, memory-free, acyclic branch regions into
// straight-line code: cheap and known speculatable instructions are hoisted into the
// predecessor, and the merge-block PHIs are turned into SELECTs.
//
// It is built from a single minimal triangle/diamond primitive that is applied
// bottom-up to a fixpoint, so nested patterns (e.g. a short-circuit `||` chain)
// collapse one inner unit at a time. Interior PHIs need no special handling:
// they belong to inner diamonds that are resolved into selects first.
//
// Triangle:
//   P: vP = ...; br cond, T, MB
//   T: vT = ...; br MB
//   MB: phi [vT,T],[vP,P]
//
// becomes:
//   P: vP = ...; vT = ...; vC = select(cond, vT, vP); br MB
//   MB: phi [vC,P]
//
// Diamond:
//   P: vP = ...; br cond, T, F
//   T: vT = ...; br MB
//   F: vF = ...; br MB
//   MB: phi [vT,T],[vF,F]
//
// becomes:
//   P: vP = ...; vT = ...; vF = ...; vC = select(cond, vT, vF); br MB
//   MB: phi [vC,P]
//
// SimplifyCFG will clean up the single incoming value phis
//
// A foldable successor may also be a shared landing pad LP (a critical-edge-split
// block reached from P and others), in which case P is peeled off it:
//   P:  vP = ...; br cond, LP, MB
//   LP: br MB // (LP holds only PHIs, and has predecessors other than P)
//   MB: phi [0,LP],[vP,P]
//
// becomes:
//   P: vP = ...; vC = select(cond, 0, vP); br MB
//   LP: br MB (one less predecessor)
//   MB: phi [vC,P], [0,LP]
//
// LP may hold PHIs: the value it feeds MB is then peeled to that PHI's incoming for
// P's edge. Any other instruction disqualifies it -- LP's remaining predecessors
// still need that instruction, so it would have to be cloned into P rather than
// moved, and this pass does not duplicate code.
//
// Iterative approach used to potentially peel all of LP's predecessors.
//
// Only branches with a divergent condition are considered: a uniform branch is a
// scalar jump that executes exactly one arm, so linearizing it just makes the
// not-taken arm's work unconditional.
//
// Profitability
// -------------
// The usual if-conversion test weighs the linearized cost against the branched cost
// times the branch probability. That collapses for a divergent branch on a SIMD
// machine: both arms already run under a lane mask, so the branched form pays for
// both bodies plus the mask setup, the branches and the reconvergence. Linearizing
// wins on instruction count by construction, so instruction count is not what to
// budget. Register pressure is. Hence three separate gates:
//
//   - isSpeculatable: may this instruction run unconditionally at all.
//   - m_maxSpeculatedInsts: instructions in one hoisted arm. A backstop against a
//     pathologically large region, like LLVM EarlyIfConversion's BlockInstrLimit.
//   - foldPressureDelta: the profitability test. Net register pressure added,
//     bounded by m_maxPressureDelta.
//
//===----------------------------------------------------------------------===//

#include "Compiler/Optimizer/BranchToSelect.hpp"

#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Pass.h>
#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

#include <algorithm>

#define DEBUG_TYPE "igc-branch-to-select"

using namespace llvm;
using namespace IGC;

namespace {

class BranchToSelect : public FunctionPass {
public:
  static char ID;

  BranchToSelect();

  StringRef getPassName() const override { return "BranchToSelect"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override;

  bool runOnFunction(Function &F) override;

private:
  enum class SuccessorKind {
    None,      // not a foldable successor
    Private,   // sole successor of P; hoist its body and delete it
    SharedPad, // PHI-only pass-through with other predecessors; peel P off it
  };

  // Attempt to convert the triangle/diamond rooted at conditional block P.
  bool tryConvert(BasicBlock *P);

  // Classify a branch successor Succ of P as a private foldable block, a shared
  // landing pad, or neither. Both foldable kinds end in an unconditional branch.
  SuccessorKind classifySuccessor(BasicBlock *Succ, BasicBlock *P) const;

  // Move all non-terminator instructions of Blk to just before P's terminator.
  void hoistInto(BasicBlock *Blk, BasicBlock *P);

  // True if I is safe to run unconditionally and stays a single native operation at
  // its operand widths, i.e. is on the hoist allow-list.
  bool isSpeculatable(const Instruction *I) const;

  // Register-file weight of V in bytes (see DivergentLaneFactor).
  unsigned registerWeight(const Value *V) const;

  // Net register pressure, in bytes, that linearizing this region adds.
  int foldPressureDelta(BasicBlock *P, BasicBlock *MB, BasicBlock *TrueIn, BasicBlock *FalseIn, SuccessorKind TrueKind,
                        SuccessorKind FalseKind) const;

  // Weight of the values MB's PHIs read over Arm's edge that are defined in Arm.
  unsigned armLiveOutWeight(BasicBlock *MB, BasicBlock *Arm) const;

  // Work-item uniformity of the values in F. Drives the divergence gate in tryConvert
  // and the uniform discount in registerWeight.
  WIAnalysis *m_WI = nullptr;
  ModuleMetaData *m_modMD = nullptr;
  const DataLayout *m_DL = nullptr;

  // Whether the divergence gate is on (fold only divergent branches). Set with regkey.
  bool m_divergentOnly = true;

  // Backstop: max instructions in one hoisted arm. Set with regkey.
  unsigned m_maxSpeculatedInsts = 0;

  // Profitability budget: max net pressure one fold may add. Set with regkey.
  unsigned m_maxPressureDelta = 0;
};

// A divergent value occupies its type's width in every lane, a uniform value in one.
// SIMD16 is the reference width for modern Xe; the SIMD the shader compiles at is not
// fixed yet here, and only the uniform-to-divergent ratio matters to the budget.
constexpr unsigned DivergentLaneFactor = 16;

} // end anonymous namespace

char BranchToSelect::ID = 0;

#define PASS_FLAG "igc-branch-to-select"
#define PASS_DESCRIPTION "Flatten small memory-free branch regions into selects"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BranchToSelect, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(BranchToSelect, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

static bool isFastFPType(const llvm::Type *T) {
  const llvm::Type *S = T->getScalarType();
  return S->isFloatTy() || S->isHalfTy() || S->isBFloatTy();
}

static bool isWideScalar(const llvm::Type *T) {
  const llvm::Type *S = T->getScalarType();
  if (S->isIntegerTy())
    return S->getIntegerBitWidth() > 32;
  return S->isFloatingPointTy() && !isFastFPType(S);
}

// The value a merge PHI receives over a shared pad's edge may itself be a PHI in that
// pad. Peel it to the value the pad is handed along P's edge, which is what P has to
// feed the select.
//
// The result is always legal at P's terminator, with no dominator-tree query:
//   - a PHI's incoming for predecessor P must dominate P's terminator (SSA), and
//   - a value not defined in the pad had to dominate the pad's end, which dominates
//     P's end too since P is a predecessor of the pad.
static Value *resolvePadIncoming(Value *V, BasicBlock *Pad, BasicBlock *P) {
  auto *Phi = dyn_cast<PHINode>(V);
  if (!Phi || Phi->getParent() != Pad)
    return V;

  Value *In = Phi->getIncomingValueForBlock(P);
  // One step always suffices: In has to dominate P's terminator, which a PHI in the
  // pad cannot do (the pad does not dominate P).
  IGC_ASSERT_MESSAGE(!isa<PHINode>(In) || cast<PHINode>(In)->getParent() != Pad,
                     "pad PHI chained to another PHI in the same pad");
  return In;
}

BranchToSelect::BranchToSelect() : FunctionPass(ID) { initializeBranchToSelectPass(*PassRegistry::getPassRegistry()); }

void BranchToSelect::getAnalysisUsage(AnalysisUsage &AU) const {
  // Divergence of the branch condition decides whether a region is worth linearizing at
  // all, and the same uniformity drives registerWeight. WIAnalysis pulls in the dominator
  // trees and LoopInfo it needs itself.
  AU.addRequired<WIAnalysis>();
  AU.addRequired<MetaDataUtilsWrapper>();
}

// May I run unconditionally? True only for an instruction that cannot fault or trap, has
// no side effects, and lowers to a single native operation at the widths given. Anything
// wider legalizes into a multi-op or multi-GRF sequence; anything not listed is rejected
// outright, so an unrecognized opcode or intrinsic is never speculated.
bool BranchToSelect::isSpeculatable(const Instruction *I) const {
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
    return true;

  // Width-changing integer moves: cheap, but a >32-bit result (zext/sext to i64) manufactures a 2-GRF value we won't
  // speculate.
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
    if (isWideScalar(I->getType()))
      return false;
    return true;

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
      return false;
    return true;

  // Compares follow the operand width (the result is i1); reject a 64-bit / f64 compare.
  case Instruction::ICmp:
  case Instruction::FCmp:
    if (isWideScalar(I->getOperand(0)->getType()))
      return false;
    return true;

  // Integer multiply: only a <=16-bit multiply is a single native op. 32-bit is mul+mach and 64-bit is an emulation
  // sequence -- both expand into extra instructions, so reject rather than count as one.
  case Instruction::Mul:
    if (I->getType()->getScalarSizeInBits() > 16)
      return false;
    return true;

  // FP arithmetic on fast native types -- one ALU op. f64 is emulated: do not speculate.
  case Instruction::FAdd:
  case Instruction::FSub:
  case Instruction::FMul:
  case Instruction::FNeg:
    if (isFastFPType(I->getType()))
      return true;
    return false;

  case Instruction::FPToUI:
  case Instruction::FPToSI:
  case Instruction::UIToFP:
  case Instruction::SIToFP:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
    if (isWideScalar(I->getOperand(0)->getType()) || isWideScalar(I->getType()))
      return false;
    return true;

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
        return true;

      default:
        return false;
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
        return true;
      default:
        return false;
      }
    }
  }

  // unspecified inst -- do not speculate
  return false;
}

BranchToSelect::SuccessorKind BranchToSelect::classifySuccessor(BasicBlock *S, BasicBlock *P) const {
  if (S == P)
    return SuccessorKind::None;

  // Both foldable kinds fall through unconditionally to the merge block.
  auto *Br = dyn_cast<IGCLLVM::UncondBrInst>(S->getTerminator());
  if (!Br)
    return SuccessorKind::None;

  if (S->getSinglePredecessor() == P) {
    // Private successor: P is its only predecessor, so hoisting its body into P
    // and deleting it affects no other path. Accept it if every instruction is
    // legal to speculate and the arm is not pathologically large; whether the fold
    // pays off is foldPressureDelta's call.
    unsigned InstCount = 0;
    for (Instruction &I : *S) {
      if (I.isTerminator())
        break;

      // Degenerate PHI (single predecessor): one incoming value that hoistInto
      // folds away. Skip it, like a debug intrinsic.
      if (isa<PHINode>(&I))
        continue;

      // Debug intrinsics hoist along but do not count against the limit.
      if (isa<DbgInfoIntrinsic>(&I))
        continue;

      // Defensive backstop, independent of the allow-list: never speculate a
      // convergent op (wave / subgroup). Hoisting it out of the branch and dropping
      // the reconvergence can change the lane mask it executes under, silently
      // altering its result. The allow-list already excludes these, but this guards
      // against a future entry being added without re-deriving that it is safe.
      if (const auto *CB = dyn_cast<CallBase>(&I))
        if (CB->isConvergent())
          return SuccessorKind::None;

      // A non-speculatable instruction disqualifies the successor outright.
      if (!isSpeculatable(&I))
        return SuccessorKind::None;

      // Backstop: refuse an arm so large that linearizing it would reshape the
      // block wholesale, however cheap each instruction looks.
      if (++InstCount > m_maxSpeculatedInsts)
        return SuccessorKind::None;
    }

    return SuccessorKind::Private;
  }

  // Otherwise the successor has predecessors besides P -- potentially a SimplifyCFG landing pad funneling several
  // short-circuit early-out edges into one block. Its body cannot be hoisted without affecting those preds, so accept
  // it only as a pass-through: P is peeled off it (routed to the merge via a select) and the pad is left intact.
  //
  // PHIs are allowed. The value such a pad feeds the merge PHI may be one of the pad's own PHIs, which the peel
  // resolves to the incoming for P's edge -- legal at the end of P without a dominator query (see resolvePadIncoming).
  // Any other instruction is rejected: the pad's remaining predecessors still need it, so it would have to be cloned
  // into P rather than moved, and this pass does not duplicate code (JumpThreading already tail-duplicates where it
  // pays off).
  for (Instruction &I : *S) {
    if (I.isTerminator())
      break;
    if (isa<DbgInfoIntrinsic>(&I))
      continue;
    if (isa<PHINode>(&I))
      continue;
    return SuccessorKind::None;
  }

  return SuccessorKind::SharedPad;
}

// Weight of V in bytes of register file: its size times the lanes it occupies. A divergent
// i32 weighs 64, a uniform i32 weighs 4, a divergent i64 weighs 128, a divergent i1 16.
unsigned BranchToSelect::registerWeight(const Value *V) const {
  Type *Ty = V->getType();
  auto *VecTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
  Type *EltTy = VecTy ? VecTy->getElementType() : Ty;
  unsigned Elts = VecTy ? (unsigned)VecTy->getNumElements() : 1;
  unsigned Bytes = Elts * (((uint32_t)m_DL->getTypeSizeInBits(EltTy) + 7) / 8);
  return Bytes * (m_WI->isUniform(V) ? 1 : DivergentLaneFactor);
}

// The values an arm keeps live until the select consumes them: the ones the merge PHIs
// read over its edge. Those are the only candidates, since an arm dominates nothing past
// its own end -- its single successor, the merge, has other predecessors. Values defined
// outside the arm are already live in P and are not its cost. A value feeding several
// PHIs occupies one register, hence the set.
unsigned BranchToSelect::armLiveOutWeight(BasicBlock *MB, BasicBlock *Arm) const {
  SmallPtrSet<const Value *, 8> Counted;
  unsigned W = 0;
  for (PHINode &Phi : MB->phis()) {
    auto *In = dyn_cast<Instruction>(Phi.getIncomingValueForBlock(Arm));
    if (In && In->getParent() == Arm && Counted.insert(In).second)
      W += registerWeight(In);
  }
  return W;
}

// Net pressure the fold adds, in bytes. The arms are mutually exclusive beforehand,
// so the region's peak live set carries the larger arm's live-outs; afterwards it
// carries both, until the selects consume them:
//
//   peak_before  = LiveThrough + max(T, F)
//   peak_after  <= LiveThrough + T + F
//   delta       <= min(T, F)
//
// An upper bound, exact for the order this pass emits (all of T, all of F, then the
// selects at P's terminator, so every arm value is live at once before the first
// select). Interleaving by select group instead holds one group at a time and peaks
// near LiveThrough + max_k(T_k + F_k) -- smaller by roughly a factor of N for a wide
// diamond of N groups. The gate thus errs toward refusing folds rather than spilling,
// at the price of over-charging a diamond with many independent merge PHIs.
//
// A triangle scores 0 either way: one side is P's own fallthrough, whose values are
// already live in P. Cascades do not accumulate either -- the PHIs a fold retires
// become selects of the same type holding the same live range, a wash that is not
// charged -- and a PHI whose incomings resolve to one value needs no select at all,
// which is credited.
int BranchToSelect::foldPressureDelta(BasicBlock *P, BasicBlock *MB, BasicBlock *TrueIn, BasicBlock *FalseIn,
                                      SuccessorKind TrueKind, SuccessorKind FalseKind) const {
  // Only a hoisted (Private) arm contributes. A shared pad stays put and hands the merge
  // a value already live in P, so it costs 0 -- as does a triangle's direct edge from P.
  unsigned TrueWeight = TrueKind == SuccessorKind::Private ? armLiveOutWeight(MB, TrueIn) : 0;
  unsigned FalseWeight = FalseKind == SuccessorKind::Private ? armLiveOutWeight(MB, FalseIn) : 0;
  int Delta = (int)std::min(TrueWeight, FalseWeight);

  // Credit the PHIs that collapse without a select. Mirrors the select-building loop in
  // tryConvert, including the pad peel, so the two agree on which PHIs need one.
  for (PHINode &Phi : MB->phis()) {
    Value *VTrue = Phi.getIncomingValueForBlock(TrueIn);
    Value *VFalse = Phi.getIncomingValueForBlock(FalseIn);
    if (TrueKind == SuccessorKind::SharedPad)
      VTrue = resolvePadIncoming(VTrue, TrueIn, P);
    if (FalseKind == SuccessorKind::SharedPad)
      VFalse = resolvePadIncoming(VFalse, FalseIn, P);
    if (VTrue == VFalse)
      Delta -= (int)registerWeight(&Phi);
  }

  return Delta;
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
  auto *Br = dyn_cast<IGCLLVM::CondBrInst>(P->getTerminator());
  if (!Br)
    return false;

  Value *Cond = Br->getCondition();

  // Divergence gate. A divergent branch enters both arms under a lane mask, so it already
  // pays for the mask setup, the branches and the reconvergence -- selects replace that at
  // no added work. A uniform branch is a scalar jump running exactly one arm, so
  // linearizing it makes the other arm's work unconditional for the price of one jump.
  //
  // WIAnalysis is computed once and stays valid across folds: its dep-map is keyed on
  // values, and folding only reshapes control flow around them. The selects this pass
  // creates are absent from that map, and a fold can leave one as the condition of a
  // branch a later iteration re-examines; isUniform() reports an unknown value as
  // non-uniform, which is right here, since a select on a divergent condition (the only
  // kind this gate admits) is itself divergent.
  if (m_divergentOnly && m_WI->isUniform(Cond)) {
    LLVM_DEBUG(dbgs() << "BranchToSelect: skipping uniform branch in " << P->getName() << "\n");
    return false;
  }

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
  SuccessorKind TrueKind = classifySuccessor(TrueS, P);
  SuccessorKind FalseKind = classifySuccessor(FalseS, P);
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

  // Profitability gate. Everything below this point mutates the IR, so decide here.
  int PressureDelta = foldPressureDelta(P, MB, TrueIn, FalseIn, TrueKind, FalseKind);
  if (PressureDelta > (int)m_maxPressureDelta) {
    LLVM_DEBUG(dbgs() << "BranchToSelect: region in " << P->getName() << " would add " << PressureDelta
                      << " units of register pressure (budget " << m_maxPressureDelta << ") -- skipping\n");
    return false;
  }

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

    // A peeled pad may feed the merge one of its own PHIs; take the value that PHI
    // carries along P's edge. This can also make the two sides equal -- the pad PHI
    // and P's direct edge resolving to the same value -- which drops the select.
    if (TrueKind == SuccessorKind::SharedPad)
      VTrue = resolvePadIncoming(VTrue, TrueIn, P);
    if (FalseKind == SuccessorKind::SharedPad)
      VFalse = resolvePadIncoming(VFalse, FalseIn, P);

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
  IGCLLVM::UncondBrInst::Create(MB, P)->setDebugLoc(BrDL);

  // Erasing the terminator drops P's edge to a peeled pad but does not fix up the
  // pad's PHIs, so drop P's incoming from each of them by hand. A shared pad keeps
  // predecessors besides P, so none of these PHIs is left empty.
  auto DropPadIncoming = [P](BasicBlock *Pad) {
    for (PHINode &Phi : Pad->phis()) {
      Phi.removeIncomingValue(P, /*DeletePHIIfEmpty*/ false);
      IGC_ASSERT_MESSAGE(Phi.getNumIncomingValues() > 0, "peeled pad PHI left with no incoming value");
    }
  };
  if (TrueKind == SuccessorKind::SharedPad)
    DropPadIncoming(TrueS);
  if (FalseKind == SuccessorKind::SharedPad)
    DropPadIncoming(FalseS);

  // Private successors are now empty and unreferenced -- erase them. Shared pads still
  // have other predecessors, so leave them for a later SimplifyCFG sweep.
  if (TrueKind == SuccessorKind::Private)
    TrueS->eraseFromParent();
  if (FalseKind == SuccessorKind::Private)
    FalseS->eraseFromParent();

  LLVM_DEBUG(dbgs() << "BranchToSelect: linearized region into " << P->getName() << " merging to " << MB->getName()
                    << "\n");

  // P now branches unconditionally to MB. When P is MB's only predecessor they are
  // a straight-line pair, so merge MB into P. Besides collapsing the degenerate
  // merge PHIs immediately, this hands P the merge's terminator, which exposes any
  // triangle/diamond rooted at P's own predecessors that spans the old P->MB seam
  // -- regions the single-block classifier cannot see. Because BranchToSelect runs
  // only once, before the later SimplifyCFGs, those folds would otherwise be lost.
  // No pressure bookkeeping is needed across the seam: a later fold rooted at P
  // re-derives its own delta from the IR as it then stands.
  if (MB->getSinglePredecessor() == P)
    MergeBlockIntoPredecessor(MB);

  return true;
}

bool BranchToSelect::runOnFunction(Function &F) {
  m_maxSpeculatedInsts = IGC_GET_FLAG_VALUE(BranchToSelectMaxSpeculatedInsts);
  m_maxPressureDelta = IGC_GET_FLAG_VALUE(BranchToSelectMaxPressureDelta);
  m_divergentOnly = IGC_IS_FLAG_ENABLED(BranchToSelectDivergentOnly);
  m_WI = &getAnalysis<WIAnalysis>();
  m_modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  m_DL = &F.getParent()->getDataLayout();

  if (m_modMD->csInfo.neededThreadIdLayout.value_or(ThreadIDLayout::X) == ThreadIDLayout::QuadTile)
    return false;

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

FunctionPass *IGC::createBranchToSelectPass() { return new BranchToSelect(); }
