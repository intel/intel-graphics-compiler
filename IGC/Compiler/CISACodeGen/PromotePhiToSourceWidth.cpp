/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/PromotePhiToSourceWidth.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/IR/Instructions.h"
#include "common/igc_regkeys.hpp"

using namespace llvm;
using namespace IGC;

// Sinks a width-narrowing cast (fptrunc/trunc, optionally preceded by a constant
// lshr that selects a packed lane and/or followed by a same-width bitcast that
// reinterprets the result, e.g. an fp<->int repack) out of a constant-guarded
// merge PHI, re-typing the PHI to the cast's source width so it matches the loop
// accumulators:
//
//   %p  = phi iN     [ bitcast(fptrunc(%a)), L ], [ 0, G ]
//     => %pw = phi float [ %a, L ], [ 0.0, G ]    ; PHI now has the accumulator width
//        %p  = bitcast(fptrunc(%pw))              ; cast sunk below the PHI
//
// Together with MergeScalarPhis this transformation results in the PHI reusing the
// accumulator's registers in this (and similar) pattern:
//                       cond-add-join
//             /                            |
//        .loop                         bypass edge
//            │ loop accumulating in float    │
//        .loopexit                │
//   %1443 = extractelement <8 x float> …     │           ; f32 accumulator result
//   %1571 = fptrunc  float %1443 to half      │          ; <-- f32→f16 conversion
//   %1699 = bitcast  half  %1571 to i16        \         │
//            \                                  \        │
//             ────────────────► ._crit_edge ◄───────────┘
//   %1827 = phi i16 [ %1699, loopexit ], [ 0, bypass ]
//   call @…PredicatedStore.p1i16.i16(ptr, i16 %1827, i64 2, i1 %2419)

namespace {

// One incoming edge that carries a narrowing cast.
struct NarrowingEdge {
  unsigned Idx = 0;                 // incoming index in the PHI
  Value *WideSrc = nullptr;         // wide value carried by the promoted PHI (pre-lshr)
  Instruction *Narrowing = nullptr; // the fptrunc / trunc
  BitCastInst *Bitcast = nullptr;   // optional fp->int repack, nullptr if absent
};

class PromotePhiToSourceWidth : public FunctionPass {
public:
  static char ID;
  PromotePhiToSourceWidth();

  StringRef getPassName() const override { return "PromotePhiToSourceWidth"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

  bool runOnFunction(Function &F) override;

private:
  // Widen the PHI to the cast's source width.
  bool tryRewrite(PHINode *P);
};

} // namespace

#define PASS_FLAG "igc-promote-phi-to-source-width"
#define PASS_DESCRIPTION "Promote a zero-guarded merge PHI to its narrowing cast's source width"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PromotePhiToSourceWidth, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PromotePhiToSourceWidth, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PromotePhiToSourceWidth::ID = 0;

FunctionPass *IGC::createPromotePhiToSourceWidthPass() { return new PromotePhiToSourceWidth(); }

PromotePhiToSourceWidth::PromotePhiToSourceWidth() : FunctionPass(ID) {
  initializePromotePhiToSourceWidthPass(*PassRegistry::getPassRegistry());
}

bool PromotePhiToSourceWidth::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;
  bool Changed = false;
  for (BasicBlock &BB : F) {
    SmallVector<PHINode *, 16> Phis;
    for (PHINode &P : BB.phis())
      Phis.push_back(&P);
    for (PHINode *P : Phis)
      Changed |= tryRewrite(P);
  }
  return Changed;
}

bool PromotePhiToSourceWidth::tryRewrite(PHINode *P) {
  Type *NarrowTy = P->getType();
  // Scalars only, vectors should be already handled by
  // IGCVectorizer, MergeScalarPhis and VariableReuseAnalysis
  if (NarrowTy->isVectorTy())
    return false;
  if (!NarrowTy->isIntegerTy() && !NarrowTy->isFloatingPointTy())
    return false;

  const unsigned NumInc = P->getNumIncomingValues();
  if (NumInc < 2)
    return false;

  SmallVector<NarrowingEdge, 4> NarrowingEdges;
  // Constant guard edges (incoming index + value), widened alongside the PHI.
  SmallVector<std::pair<unsigned, Constant *>, 4> ConstEdges;
  bool AnyNonNullConst = false;

  // Shape shared by every narrowing edge (must be consistent so the widened PHI
  // is well-typed): the wide source type, the narrowing result type, fp/int, an
  // optional same-width bitcast, and an optional constant lshr offset (a packed
  // lane selected by trunc(lshr(x, k))).
  Type *WideTy = nullptr;
  Type *InterTy = nullptr;
  bool IsFP = false;
  bool UseBitcast = false;
  ConstantInt *ShiftAmt = nullptr;

  for (unsigned i = 0; i < NumInc; ++i) {
    Value *V = P->getIncomingValue(i);

    if (auto *C = dyn_cast<Constant>(V)) {
      // A null guard widens to getNullValue (any type); a non-null guard is only
      // widenable as zext of an integer constant. Bail on undef/poison/fp/expr.
      if (!C->isNullValue() && !isa<ConstantInt>(C))
        return false;
      ConstEdges.push_back({i, C});
      AnyNonNullConst |= !C->isNullValue();
      continue;
    }

    // Narrowing edge: optional bitcast wrapping a narrowing cast.
    BitCastInst *BC = dyn_cast<BitCastInst>(V);
    Value *Inner = BC ? BC->getOperand(0) : V;
    auto *Narrowing = dyn_cast<Instruction>(Inner);
    if (!Narrowing)
      return false;

    Value *Src = nullptr;
    bool ThisFP = false;
    ConstantInt *ThisShift = nullptr;
    if (auto *FT = dyn_cast<FPTruncInst>(Narrowing)) {
      Src = FT->getOperand(0);
      ThisFP = true;
    } else if (auto *TR = dyn_cast<TruncInst>(Narrowing)) {
      Src = TR->getOperand(0);
      ThisFP = false;
      // Peel a constant lshr: trunc(lshr(x, k)) selects a packed lane. Carry the
      // full wide value x in the PHI and re-emit the lshr below it.
      if (auto *Shr = dyn_cast<BinaryOperator>(Src)) {
        if (Shr->getOpcode() == Instruction::LShr) {
          if (auto *K = dyn_cast<ConstantInt>(Shr->getOperand(1))) {
            ThisShift = K;
            Src = Shr->getOperand(0);
          }
        }
      }
    } else {
      return false; // not a recognized narrowing
    }

    // We re-emit the narrowing (and bitcast) in the merge block, so it must not
    // be used elsewhere.
    if (!Narrowing->hasOneUse())
      return false;
    if (BC && !BC->hasOneUse())
      return false;

    Type *ThisInter = Narrowing->getType();
    Type *ThisWide = Src->getType();

    if (NarrowingEdges.empty()) {
      WideTy = ThisWide;
      InterTy = ThisInter;
      IsFP = ThisFP;
      UseBitcast = (BC != nullptr);
      ShiftAmt = ThisShift;
    } else if (WideTy != ThisWide || InterTy != ThisInter || IsFP != ThisFP || UseBitcast != (BC != nullptr) ||
               ShiftAmt != ThisShift) {
      return false; // inconsistent narrowing edges
    }

    NarrowingEdges.push_back({i, Src, Narrowing, BC});
  }

  if (NarrowingEdges.empty())
    return false; // nothing to widen
  if (NarrowingEdges.size() + ConstEdges.size() != NumInc)
    return false; // every edge must be classified

  // The narrowing produces InterTy; an optional same-width bitcast then
  // reinterprets it as NarrowTy (e.g. fp->int repack for an f16 store
  if (UseBitcast) {
    if (NarrowTy->getScalarSizeInBits() != InterTy->getScalarSizeInBits())
      return false;
  } else if (NarrowTy != InterTy) {
    return false;
  }
  if (WideTy->getScalarSizeInBits() <= InterTy->getScalarSizeInBits())
    return false; // not actually a narrowing

  // A non-zero constant guard is widened with zext, which reproduces it only for a
  // plain integer trunc (trunc(zext C) == C). With an fp narrowing, a bitcast, or
  // an lshr offset the wide preimage is not a simple zext, so require a zero guard.
  if (AnyNonNullConst && (IsFP || UseBitcast || ShiftAmt))
    return false;

  // ---- rewrite ----
  IGC_ASSERT(WideTy && InterTy);

  PHINode *WideP = IGCLLVM::createPHINode(WideTy, NumInc, "width_promoted_phi", P);
  WideP->setDebugLoc(P->getDebugLoc());

  for (unsigned i = 0; i < NumInc; ++i) {
    BasicBlock *Pred = P->getIncomingBlock(i);
    const NarrowingEdge *NE = nullptr;
    for (const NarrowingEdge &N : NarrowingEdges) {
      if (N.Idx == i) {
        NE = &N;
        break;
      }
    }
    if (NE) {
      WideP->addIncoming(NE->WideSrc, Pred);
      continue;
    }
    // Constant guard edge: widen so the sunk narrowing reproduces the constant.
    // A non-zero integer guard becomes zext(C) (gated above to the plain-trunc
    // shape); any zero guard maps to the all-zero pattern getNullValue(WideTy).
    Constant *GC = nullptr;
    for (auto &PE : ConstEdges)
      if (PE.first == i) {
        GC = PE.second;
        break;
      }
    IGC_ASSERT(GC);
    Constant *Wide = GC->isNullValue()
                         ? Constant::getNullValue(WideTy)
                         : ConstantInt::get(WideTy->getContext(),
                                            cast<ConstantInt>(GC)->getValue().zext(WideTy->getIntegerBitWidth()));
    WideP->addIncoming(Wide, Pred);
  }

  // Sink the narrowing into the merge block, right after the PHI region.
  Instruction *InsertPt = IGCLLVM::getFirstNonPHI(P->getParent());
  IRBuilder<> B(InsertPt);
  B.SetCurrentDebugLocation(NarrowingEdges[0].Narrowing->getDebugLoc());

  // Re-emit on the widened PHI: an optional lshr to reselect the packed lane, the
  // narrowing, then the optional bitcast. (ShiftAmt is only set for integer trunc.)
  Value *Base = ShiftAmt ? B.CreateLShr(WideP, ShiftAmt) : static_cast<Value *>(WideP);
  Value *Inter = IsFP ? B.CreateFPTrunc(Base, InterTy) : B.CreateTrunc(Base, InterTy);
  Value *Narrowed = UseBitcast ? B.CreateBitCast(Inter, NarrowTy) : Inter;

  P->replaceAllUsesWith(Narrowed);
  P->eraseFromParent();

  // The original per-edge narrowing/bitcast chains are now dead (each was
  // single-use, feeding only the erased PHI). Their wide source survives because
  // the new PHI references it.
  for (const NarrowingEdge &N : NarrowingEdges) {
    Instruction *Root = N.Bitcast ? static_cast<Instruction *>(N.Bitcast) : N.Narrowing;
    RecursivelyDeleteTriviallyDeadInstructions(Root);
  }

  return true;
}
