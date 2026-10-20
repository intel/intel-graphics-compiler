/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// FoldProvableFCmp.cpp - Fold fcmp insts that FPRangeAnalysis can simplify
//
// Folds any fcmp whose result is determined by the ranges of its operands, then deletes the control flow that makes
// dead - typically a guard `fcmp %x, <threshold>` on a path %x can never reach.
//
// NaN: a NaN operand makes every ordered predicate false regardless of the bounds, so a fold needs MayBeNaN clear.
// That is not implied by the range: an unbounded value can be non-NaN, and a non-NaN value (sitofp, a clamp) folds
// with no fast-math flags at all.
//
// Margin: the analysis estimates what the hardware computes rather than giving a ULP-exact envelope - later passes
// may contract or reassociate, an arcp fdiv becomes x * rcp(y), and transcendentals are approximated. So each
// computed bound is relaxed outward by 2^-FoldProvableFCmpMarginBits of itself, floored at the smallest normal, and
// a fold only happens if the answer survives. Literals are charged nothing, except denormal ones, which flushing
// changes. Bits = 0 disables the margin.
//
// Denormals need nothing here: FPRange already snaps denormal bounds to zero.
//
//===----------------------------------------------------------------------===//

#include "Compiler/FoldProvableFCmp.hpp"
#include "Compiler/FPRangeAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/Analysis/InstructionSimplify.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "igc-fold-provable-fcmp"

namespace {

/// The three relations that can hold between two non-NaN floats. Exactly one of them holds for any given pair, so a set
/// of these says everything a predicate needs to know about a pair.
enum Relation : unsigned {
  Rel_LT = 1u << 0,
  Rel_GT = 1u << 1,
  Rel_EQ = 1u << 2,
};

/// Bound X of value V moved outward (Lower down, Upper up) by how far it may be off: |X| * 2^-Bits, floored at the
/// smallest normal. Infinite bounds stay put. A literal gets no margin unless it is denormal. A zero bound guaranteed
/// by the construct producing it (a saturate, a square) gets none either; a zero reached by subtraction keeps the
/// floor, since reassociation can take it slightly below zero.
APFloat relaxBound(const APFloat &X, Value *V, unsigned Bits, bool Downward) {
  if (Bits == 0 || X.isInfinity())
    return X;

  APFloat Margin = APFloat::getSmallestNormalized(X.getSemantics());
  if (isa<ConstantFP>(V)) {
    if (!X.isDenormal())
      return X;
  } else if (X.isZero() && (Downward ? isStructurallyNonNegative(V) : isStructurallyNonPositive(V))) {
    return X;
  } else {
    APFloat Scaled = llvm::scalbn(llvm::abs(X), -static_cast<int>(Bits), APFloat::rmTowardPositive);
    if (Scaled.compare(Margin) == APFloat::cmpGreaterThan)
      Margin = Scaled;
  }

  APFloat Relaxed = X;
  if (Downward)
    Relaxed.subtract(Margin, APFloat::rmTowardNegative);
  else
    Relaxed.add(Margin, APFloat::rmTowardPositive);
  return Relaxed;
}

/// Relations that make Pred true, given neither operand is NaN - which is what lets each unordered predicate share
/// its ordered counterpart's answer. Returns 0 for unhandled predicates.
unsigned wantedRelations(CmpInst::Predicate Pred) {
  switch (Pred) {
  case CmpInst::FCMP_OLT:
  case CmpInst::FCMP_ULT:
    return Rel_LT;
  case CmpInst::FCMP_OGT:
  case CmpInst::FCMP_UGT:
    return Rel_GT;
  case CmpInst::FCMP_OLE:
  case CmpInst::FCMP_ULE:
    return Rel_LT | Rel_EQ;
  case CmpInst::FCMP_OGE:
  case CmpInst::FCMP_UGE:
    return Rel_GT | Rel_EQ;
  case CmpInst::FCMP_OEQ:
  case CmpInst::FCMP_UEQ:
    return Rel_EQ;
  case CmpInst::FCMP_ONE:
  case CmpInst::FCMP_UNE:
    return Rel_LT | Rel_GT;
  default:
    return 0;
  }
}

} // namespace

namespace IGC {

class FoldProvableFCmp : public FunctionPass {
public:
  static char ID;
  FoldProvableFCmp() : FunctionPass(ID) {}

  bool runOnFunction(Function &F) override;

  StringRef getPassName() const override { return "Fold Provable FCmp"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {}

private:
  /// Replace FCmp with a constant if its ranges decide it. Returns true if folded.
  /// On success the former users of FCmp are appended to Users.
  bool tryFoldCompare(FCmpInst *FCmp, FPRangeCache &Cache, SmallVectorImpl<WeakTrackingVH> &Users);

  /// Propagate folded constants through their users, so a branch on `or` of two folded compares ends up on a
  /// Constant. Instructions left dead are appended to Dead.
  void simplifyUsers(SmallVectorImpl<WeakTrackingVH> &Worklist, const SimplifyQuery &Query,
                     SmallVectorImpl<WeakTrackingVH> &Dead);
};

bool FoldProvableFCmp::tryFoldCompare(FCmpInst *FCmp, FPRangeCache &Cache, SmallVectorImpl<WeakTrackingVH> &Users) {
  // Vector compares would need per-lane ranges, which FPRange does not model.
  if (FCmp->getType()->isVectorTy())
    return false;

  const unsigned Wanted = wantedRelations(FCmp->getPredicate());
  if (Wanted == 0)
    return false;

  Value *LV = FCmp->getOperand(0);
  Value *RV = FCmp->getOperand(1);
  const FPRange L = computeFPRange(LV, &Cache);
  const FPRange R = computeFPRange(RV, &Cache);

  // A NaN operand makes every ordered predicate false and every unordered one true, independent of the bounds. Only
  // the bounds are consulted below, so NaN has to be off the table first.
  if (L.MayBeNaN || R.MayBeNaN)
    return false;

  // Empty means "no values", an unverified dead path, so it decides nothing. Full carries no bound information, and
  // can be non-NaN, so the check above does not exclude it.
  if (L.isEmptySet() || R.isEmptySet() || L.isFullSet() || R.isFullSet())
    return false;

  // Relax each bound by its margin.
  const unsigned Bits = IGC_GET_FLAG_VALUE(FoldProvableFCmpMarginBits);
  const APFloat LLo = relaxBound(L.Lower, LV, Bits, /*Downward=*/true);
  const APFloat LHi = relaxBound(L.Upper, LV, Bits, /*Downward=*/false);
  const APFloat RLo = relaxBound(R.Lower, RV, Bits, /*Downward=*/true);
  const APFloat RHi = relaxBound(R.Upper, RV, Bits, /*Downward=*/false);

  // Relations realizable by some pair in the relaxed intervals. Any overlap yields all three and decides nothing.
  unsigned Possible = 0;
  if (LLo.compare(RHi) == APFloat::cmpLessThan)
    Possible |= Rel_LT; // some x < y
  if (LHi.compare(RLo) == APFloat::cmpGreaterThan)
    Possible |= Rel_GT; // some x > y
  if (LLo.compare(RHi) != APFloat::cmpGreaterThan && RLo.compare(LHi) != APFloat::cmpGreaterThan)
    Possible |= Rel_EQ; // the intervals overlap

  // Decided only if the realizable relations all agree on Pred.
  bool Known;
  if ((Possible & ~Wanted) == 0)
    Known = true;
  else if ((Possible & Wanted) == 0)
    Known = false;
  else
    return false;

  LLVM_DEBUG(dbgs() << "igc-fold-provable-fcmp: " << (Known ? "true " : "false") << "  " << *FCmp << "\n    lhs " << L
                    << "\n    rhs " << R << "\n");

  Users.append(FCmp->user_begin(), FCmp->user_end());
  FCmp->replaceAllUsesWith(ConstantInt::get(FCmp->getType(), Known));
  return true;
}

void FoldProvableFCmp::simplifyUsers(SmallVectorImpl<WeakTrackingVH> &Worklist, const SimplifyQuery &Query,
                                     SmallVectorImpl<WeakTrackingVH> &Dead) {
  // Worklist grows as simplifications expose further ones, so index rather than
  // iterate. WeakTrackingVH because an entry can be erased before we reach it.
  SmallPtrSet<Value *, 8> Visited;
  for (unsigned Idx = 0; Idx != Worklist.size(); ++Idx) {
    auto *User = dyn_cast_or_null<Instruction>(Worklist[Idx]);
    if (!User || User->use_empty() || !Visited.insert(User).second)
      continue;
    Value *Simplified = IGCLLVM::simplifyInstruction(User, Query);
    if (!Simplified || Simplified == User)
      continue;
    // Queue this instruction's users before the replacement, so the constant
    // keeps propagating towards the terminator.
    Worklist.append(User->user_begin(), User->user_end());
    User->replaceAllUsesWith(Simplified);
    Dead.push_back(User);
  }
}

bool FoldProvableFCmp::runOnFunction(Function &F) {
  // One cache per function. Folding an fcmp does not change any FP value, so
  // the ranges stay valid for the whole walk.
  FPRangeCache Cache;

  SmallVector<FCmpInst *, 8> Candidates;
  for (auto &I : instructions(F)) {
    if (auto *FCmp = dyn_cast<FCmpInst>(&I))
      Candidates.push_back(FCmp);
  }

  SmallVector<FCmpInst *, 8> Folded;
  SmallVector<WeakTrackingVH, 8> Users;
  for (FCmpInst *FCmp : Candidates) {
    if (tryFoldCompare(FCmp, Cache, Users))
      Folded.push_back(FCmp);
  }

  if (Folded.empty())
    return false;

  // Drop the dead compares before touching the CFG so that the guard condition does not keep its operand chain alive.
  for (FCmpInst *FCmp : Folded)
    FCmp->eraseFromParent();

  // Folding both compares of `or i1 (fcmp ...), (fcmp ...)` leaves `or i1 false, false`, which ConstantFoldTerminator
  // would not look through, so push the constants through the condition first. This also catches the short
  // circuits, where `or %a, true` / `and %a, false` decides the branch with %a unknown.
  const SimplifyQuery Query(F.getParent()->getDataLayout());
  SmallVector<WeakTrackingVH, 8> Dead;
  simplifyUsers(Users, Query, Dead);

  for (WeakTrackingVH &V : Dead) {
    if (auto *I = dyn_cast_or_null<Instruction>(V))
      RecursivelyDeleteTriviallyDeadInstructions(I);
  }

  // Turn `br i1 <const>` into an unconditional branch, then drop the blocks that became unreachable.
  for (BasicBlock &BB : F)
    ConstantFoldTerminator(&BB, /*DeleteDeadConditions=*/true);

  removeUnreachableBlocks(F);

  return true;
}

FunctionPass *createFoldProvableFCmpPass() { return new FoldProvableFCmp(); }

} // namespace IGC

char FoldProvableFCmp::ID = 0;

#define PASS_FLAG "igc-fold-provable-fcmp"
#define PASS_DESC "Fold fcmp guards whose result FPRangeAnalysis can prove, and delete the dead paths"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(FoldProvableFCmp, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(FoldProvableFCmp, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
