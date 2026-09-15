/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/PredicatedMemoryOpt.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PatternMatch.h>
#include <llvm/IR/ValueHandle.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "llvmWrapper/Analysis/InstructionSimplify.h"
#include "llvmWrapper/IR/Instructions.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace llvm::PatternMatch;
using namespace IGC;

//===----------------------------------------------------------------------===//
// Helpers shared by both passes in this file
//===----------------------------------------------------------------------===//

static cl::opt<unsigned> MaxMaskDepth("igc-predicated-load-max-mask-depth", cl::init(4), cl::Hidden,
                                      cl::desc("Max AND-mask nesting walked on the left-hand side when proving "
                                               "predicate implication"));

static cl::opt<unsigned>
    MaxUsePredicateDepth("igc-predicated-load-max-use-depth", cl::init(8), cl::Hidden,
                         cl::desc("Max forwarding-chain depth used to find a predicated load's use predicate"));

// Strip `freeze X` while X is well defined, since then freeze X == X. A freeze
// of a possibly-undef X is returned as is: it is an opaque value.
static Value *lookThroughFreeze(Value *V) {
  while (auto *FI = dyn_cast<FreezeInst>(V)) {
    Value *Op = FI->getOperand(0);
    if (!isGuaranteedNotToBeUndefOrPoison(Op))
      break;
    V = Op;
  }
  return V;
}

// Recursive helper for impliedValue(); not called directly.
static std::optional<bool> impliedValueRec(Value *LHS, Value *RHS, const DataLayout &DL, unsigned Depth,
                                           SmallPtrSetImpl<Value *> &Tried) {
  if (!Tried.insert(LHS).second)
    return std::nullopt;
  if (auto Known = isImpliedCondition(LHS, RHS, DL, /*LHSIsTrue=*/true))
    return *Known;

  Value *X, *Y;
  if (Depth >= MaxMaskDepth || !match(LHS, m_LogicalAnd(m_Value(X), m_Value(Y))))
    return std::nullopt;
  if (auto Known = impliedValueRec(lookThroughFreeze(X), RHS, DL, Depth + 1, Tried))
    return Known;
  return impliedValueRec(lookThroughFreeze(Y), RHS, DL, Depth + 1, Tried);
}

// Returns the value RHS must have on every lane where LHS has the value LHSIsTrue,
// or nullopt when neither value can be proved. Wraps llvm::isImpliedCondition()
// and additionally walks an `and` tree on the left-hand side.
//
// isImpliedCondition() looks through an `and` on its right-hand side, and on its
// left-hand side only when the right-hand side is a comparison. Example:
//
//   %inner = and i1 %q, %other1
//   %mask  = and i1 %inner, %other2
//   %x = PredicatedLoad(%p, 4, %q,    i32 0)
//   %y = PredicatedLoad(%p, 4, %mask, i32 0)
//
// Reusing %x for %y needs impliedValue(LHS = %mask, RHS = %q). RHS is a plain
// i1 and not a comparison, so isImpliedCondition() never looks through LHS's
// `and` and returns nullopt. A true LHS implies each of its components, so the
// search walks down the tree - %mask, then %inner and %other2, then %q and %other1
// - and stops at the first component that proves RHS: %q.
//
// The walk into the LHS tree requires LHS to be true; the negated direction is
// not walked.
static std::optional<bool> impliedValue(Value *LHS, Value *RHS, const DataLayout &DL, bool LHSIsTrue) {
  RHS = lookThroughFreeze(RHS);
  // LLVM's query does not fold a constant right-hand side.
  if (match(RHS, m_One()))
    return true;
  if (match(RHS, m_Zero()))
    return false;

  LHS = lookThroughFreeze(LHS);
  if (!LHSIsTrue) {
    if (auto Known = isImpliedCondition(LHS, RHS, DL, /*LHSIsTrue=*/false))
      return *Known;
    return std::nullopt;
  }

  SmallPtrSet<Value *, 8> Tried;
  return impliedValueRec(LHS, RHS, DL, 0, Tried);
}

// True when LHS being true proves RHS is true as well.
static bool implies(Value *LHS, Value *RHS, const DataLayout &DL) {
  return impliedValue(LHS, RHS, DL, /*LHSIsTrue=*/true).value_or(false);
}

// True when LHS being false proves RHS is true.
static bool impliesNot(Value *LHS, Value *RHS, const DataLayout &DL) {
  return impliedValue(LHS, RHS, DL, /*LHSIsTrue=*/false).value_or(false);
}

// Max operand depth walked when looking for the values two predicates share. A
// transform whose walk stops at a value that may be undef or poison is
// rejected.
static constexpr unsigned MaxFreezeDepth = 8;

static bool needsFreeze(const Value *V) { return !isGuaranteedNotToBeUndefOrPoison(V); }

namespace {

// Result of FreezeInserter::freezeShared().
enum class FreezeResult {
  NotNeeded, // Every shared value is well defined; the IR is unchanged.
  Frozen,    // At least one shared value was frozen; the freezes are pending.
  Failed,    // A shared value cannot be found or frozen; nothing was frozen.
};

// Optimizations rely on a relation between two i1 values, proved on the IR. It
// must also hold for the values the uses read at run time:
//
// * undef: each use of a value that may be undef can read a different value.
//   Example:
//
//     %a = PredicatedLoad(%p, 4, %q, i32 7)
//     %b = PredicatedLoad(%p, 4, %q, i32 13)
//     =>
//     %a = PredicatedLoad(%p, 4, %q, i32 7)
//     %b = select i1 %q, i32 %a, i32 13
//
//   If %q can be undef, the two uses may disagree: if the first use reads false
//   and the second true, %b becomes 7, which was impossible in the original
//   program. Freezing %q makes every use read the same value.
// * poison: every use reads the same poison, so poison cannot break a relation
//   between uses. Freezing it picks an arbitrary defined value, which is a
//   refinement and always legal.
// * poison created below the freeze: freezing %x does not make
//   `add nsw %x.fr, 1` well defined, but its poison is the same at every use, so
//   the relation proved on the frozen IR still holds. A PredicatedLoad with a
//   poison predicate is undefined behavior, as a branch on poison is, so a
//   transform only has to be correct where the original predicate of a load
//   is not poison.
//
// Freeze the values both sides are computed from, not the sides themselves.
// Freezing `icmp ult %x, 5` and `icmp ult %x, 10` separately gives two unrelated
// values; freezing `%x` keeps the implication between the two comparisons.
//
// A freeze is inserted only when everything that can reject the transform
// without freezing has been checked. Freezing can hide what the first proof
// matched on, so the proof is repeated on the frozen IR. isImpliedCondition()
// can fail, which could not be predicted before freezing. Freeze stays pending
// with optional rollback to the original IR.
class FreezeInserter {
public:
  explicit FreezeInserter(Function &F) : F(F) {}

  // Freeze the values A and B share, so that a relation proved between A and B
  // also holds for the values read at run time. On Failed the caller must not
  // apply the transform. On NotNeeded a proof made before the call still holds.
  // On Frozen the proof has to be repeated on the frozen IR, and the freezes
  // committed or rolled back.
  FreezeResult freezeShared(Value *A, Value *B);
  // Freeze V itself, for a value that is about to get one more use. Returns the
  // value every use of V reads afterwards - V itself when it needs no freeze -
  // or null, with nothing frozen, if V cannot be frozen.
  Value *freezeValue(Value *V);
  // True when freezeValue(V) would succeed.
  bool canFreeze(Value *V) const;
  // Keep the freezes created since the last commit() or rollback().
  void commit() { Pending.clear(); }
  // Remove the freezes created since the last commit() or rollback().
  void rollback();
  // Returns the value below the freezes created by this instance, committed or
  // pending, at the top of V: V itself when it is not such a freeze.
  Value *lookThroughOwnFreezes(Value *V) const;

private:
  void collectOperands(Value *V, SmallPtrSetImpl<Value *> &Out, SmallPtrSetImpl<Value *> &StoppedAt,
                       unsigned Depth) const;
  bool collectShared(Value *V, const SmallPtrSetImpl<Value *> &Other, SmallVectorImpl<Value *> &Shared,
                     SmallPtrSetImpl<Value *> &Visited, unsigned Depth) const;
  bool isStoppedAtWellDefined(Value *V, const SmallPtrSetImpl<Value *> &StoppedAt,
                              const SmallPtrSetImpl<Value *> &Shared, SmallPtrSetImpl<Value *> &Visited) const;

  Function &F;
  SmallVector<FreezeInst *, 4> Pending;
  // Every freeze created and not rolled back.
  SmallPtrSet<FreezeInst *, 8> Created;
};

// True when the value of U flows into the result of its user.
static bool isDataOperand(const Use &U) {
  Type *Ty = U->getType();
  if (Ty->isMetadataTy() || Ty->isLabelTy() || Ty->isTokenTy())
    return false;
  const auto *Call = dyn_cast<CallBase>(U.getUser());
  return !Call || !Call->isCallee(&U);
}

// Every value reachable from V through operands, bounded. The instructions whose
// operands are not walked go to StoppedAt.
void FreezeInserter::collectOperands(Value *V, SmallPtrSetImpl<Value *> &Out, SmallPtrSetImpl<Value *> &StoppedAt,
                                     unsigned Depth) const {
  if (!Out.insert(V).second)
    return;
  auto *I = dyn_cast<Instruction>(V);
  if (!I)
    return;
  // Don't walk phi.
  if (isa<PHINode>(I) || Depth >= MaxFreezeDepth) {
    StoppedAt.insert(I);
    return;
  }
  for (const Use &Op : I->operands())
    if (isDataOperand(Op))
      collectOperands(Op, Out, StoppedAt, Depth + 1);
}

// Collect the values reachable from V that are also in Other, stopping at the
// first one on each path: freezing it also fixes every value computed from it.
// At a PHI or at the depth limit the walk stops. It fails there only if that
// value may be undef or poison, because then a shared value below it could be
// missed. A well-defined value needs nothing below it frozen.
bool FreezeInserter::collectShared(Value *V, const SmallPtrSetImpl<Value *> &Other, SmallVectorImpl<Value *> &Shared,
                                   SmallPtrSetImpl<Value *> &Visited, unsigned Depth) const {
  if (!Visited.insert(V).second)
    return true;
  if (Other.count(V)) {
    Shared.push_back(V);
    return true;
  }
  auto *I = dyn_cast<Instruction>(V);
  if (!I)
    return true;
  if (isa<PHINode>(I) || Depth >= MaxFreezeDepth)
    return !needsFreeze(I);
  for (const Use &Op : I->operands())
    if (isDataOperand(Op) && !collectShared(Op, Other, Shared, Visited, Depth + 1))
      return false;
  return true;
}

// Returns true when every value in StoppedAt that V reaches without going
// through a value in Shared is well defined, so freezing Shared is enough on V's
// side. The walk follows only the operands collectOperands() walked from V. A
// value reached only through a shared value needs no check: every read of it
// goes through that one value, which is frozen or well defined.
bool FreezeInserter::isStoppedAtWellDefined(Value *V, const SmallPtrSetImpl<Value *> &StoppedAt,
                                            const SmallPtrSetImpl<Value *> &Shared,
                                            SmallPtrSetImpl<Value *> &Visited) const {
  if (!Visited.insert(V).second || Shared.count(V))
    return true;
  if (StoppedAt.count(V))
    return !needsFreeze(V);
  auto *I = dyn_cast<Instruction>(V);
  if (!I)
    return true;
  for (const Use &Op : I->operands())
    if (isDataOperand(Op) && !isStoppedAtWellDefined(Op, StoppedAt, Shared, Visited))
      return false;
  return true;
}

FreezeResult FreezeInserter::freezeShared(Value *A, Value *B) {
  SmallPtrSet<Value *, 16> OperandsOfB;
  SmallPtrSet<Value *, 4> StoppedAtInB;
  collectOperands(B, OperandsOfB, StoppedAtInB, 0);

  SmallVector<Value *, 4> Shared;
  SmallPtrSet<Value *, 16> Visited;
  if (!collectShared(A, OperandsOfB, Shared, Visited, 0))
    return FreezeResult::Failed;
  // What is below a value B's walk stopped at is unknown, and may be shared
  // with A. It is covered when the value is well defined, or when B reads it
  // only through a shared value.
  SmallPtrSet<Value *, 8> SharedSet(Shared.begin(), Shared.end());
  SmallPtrSet<Value *, 16> VisitedInB;
  if (!isStoppedAtWellDefined(B, StoppedAtInB, SharedSet, VisitedInB))
    return FreezeResult::Failed;

  if (!all_of(Shared, [&](Value *V) { return canFreeze(V); }))
    return FreezeResult::Failed;
  const size_t NumPending = Pending.size();
  for (Value *V : Shared)
    freezeValue(V);
  return Pending.size() == NumPending ? FreezeResult::NotNeeded : FreezeResult::Frozen;
}

bool FreezeInserter::canFreeze(Value *V) const {
  if (!needsFreeze(V))
    return true;
  // Only an instruction or an argument can be frozen.
  if (!isa<Instruction>(V) && !isa<Argument>(V))
    return false;
  Type *Ty = V->getType();
  if (Ty->isMetadataTy() || Ty->isLabelTy() || Ty->isTokenTy())
    return false;
  // The freeze goes right after the definition of V.
  auto *I = dyn_cast<Instruction>(V);
  return !I || IGCLLVM::getInsertionPointAfterDef(I).has_value();
}

Value *FreezeInserter::freezeValue(Value *V) {
  if (!needsFreeze(V))
    return V;
  if (!canFreeze(V))
    return nullptr;

  // Insert the freeze right after the definition of V, so it dominates every
  // use it replaces.
  IRBuilder<> Builder(F.getContext());
  if (auto *I = dyn_cast<Instruction>(V)) {
    BasicBlock::iterator AfterDef = *IGCLLVM::getInsertionPointAfterDef(I);
    Builder.SetInsertPoint(AfterDef->getParent(), AfterDef);
  } else {
    // An argument is defined at the function entry.
    Builder.SetInsertPoint(&F.getEntryBlock(), F.getEntryBlock().getFirstInsertionPt());
  }

  auto *FI = cast<FreezeInst>(Builder.CreateFreeze(V, V->getName() + ".fr"));

  // Replace all uses, not only the ones this pass changes. Otherwise the other
  // uses could still read a different value.
  V->replaceAllUsesWith(FI);
  FI->setOperand(0, V);
  Pending.push_back(FI);
  Created.insert(FI);
  return FI;
}

void FreezeInserter::rollback() {
  for (FreezeInst *FI : reverse(Pending)) {
    Created.erase(FI);
    FI->replaceAllUsesWith(FI->getOperand(0));
    FI->eraseFromParent();
  }
  Pending.clear();
}

Value *FreezeInserter::lookThroughOwnFreezes(Value *V) const {
  while (auto *FI = dyn_cast<FreezeInst>(V)) {
    if (!Created.count(FI))
      break;
    V = FI->getOperand(0);
  }
  return V;
}

// Which lanes of a value can still be observed by some user.
//
// This is the consumer-side counterpart of a predicated load's own `predicate`
// operand (PredicatedLoadIntrinsic::getPredicate). The predicate decides which
// lanes read memory; the use predicate decides which lanes actually use the
// the result.
enum class UsePredicateKind {
  Unused,   // no user observes it
  AllLanes, // some user observes it on every lane
  IfTrue,   // observed only where Cond is true
  IfFalse,  // observed only where Cond is false
};

struct UsePredicate {
  UsePredicateKind Kind = UsePredicateKind::Unused;
  // Only meaningful for IfTrue and IfFalse; null in the other two states.
  Value *Cond = nullptr;

  static UsePredicate allLanes() { return {UsePredicateKind::AllLanes, nullptr}; }
  static UsePredicate ifTrue(Value *C) { return {UsePredicateKind::IfTrue, C}; }
  static UsePredicate ifFalse(Value *C) { return {UsePredicateKind::IfFalse, C}; }
  bool isUnused() const { return Kind == UsePredicateKind::Unused; }
  bool isAllLanes() const { return Kind == UsePredicateKind::AllLanes; }
};

} // namespace

// Two structurally identical instructions compute the same value only when the
// result is a pure function of the operands: a memory read can be separated by
// a store, and two `freeze` instructions may pick different bits for the same
// poison input.
static bool equivalentValues(const Value *A, const Value *B) {
  if (A == B)
    return true;
  const auto *AI = dyn_cast<Instruction>(A);
  const auto *BI = dyn_cast<Instruction>(B);
  if (!AI || !BI || AI->mayReadFromMemory() || AI->mayHaveSideEffects() || isa<FreezeInst>(AI) ||
      !AI->isIdenticalTo(BI))
    return false;
  // A convergent call depends on the set of lanes executing it, so two of
  // them agree only inside one block.
  const auto *Call = dyn_cast<CallBase>(AI);
  return !Call || !Call->isConvergent() || AI->getParent() == BI->getParent();
}

// Two select conditions the use predicate join treats as one.
static bool equivalentConditions(const Value *A, const Value *B) {
  if (A == B)
    return true;
  const auto *CA = dyn_cast<CmpInst>(A);
  const auto *CB = dyn_cast<CmpInst>(B);
  return CA && CB && CA->isIdenticalTo(CB);
}

// Join the use predicates contributed by independent paths. Anything the
// join cannot express as one condition becomes AllLanes, the conservative
// result that blocks all transforms.
static UsePredicate joinUsePredicates(UsePredicate A, UsePredicate B) {
  if (A.isUnused())
    return B;
  if (B.isUnused())
    return A;
  if (A.isAllLanes() || B.isAllLanes() || A.Kind != B.Kind || !equivalentConditions(A.Cond, B.Cond))
    return UsePredicate::allLanes();
  return A;
}

// Instructions that forward the value of the given operand into their result, so
// that the use predicate of the result also applies to the operand. Example:
//
//   %a      = PredicatedLoad(%p, 4, %S, i32 0)
//   %halves = bitcast i32 %a to <2 x half>
//   %lo     = extractelement <2 x half> %halves, i32 0
//   %hi     = extractelement <2 x half> %halves, i32 1
//   %rlo    = select i1 %A, half %x, half %lo
//   %rhi    = select i1 %A, half %y, half %hi
//
// `bitcast` and `extractelement` are only forwarding, load can track to `select`
// through them.
static bool forwardsValue(const Use &U) {
  const User *Usr = U.getUser();
  if (isa<CastInst>(Usr) || isa<FreezeInst>(Usr))
    return true;
  if (isa<ExtractElementInst>(Usr))
    return U.getOperandNo() == 0;
  if (isa<InsertElementInst>(Usr))
    return U.getOperandNo() <= 1;
  return false;
}

namespace {

// For a value V - always a predicated load, or something derived from one - finds
// the single condition under which V can still reach a use that observes it.
//
// Any unrecognized user, and any disagreement between two paths, collapses the
// answer to AllLanes. PHIs are not traversed.
//
// One instance is valid only while the use lists it walked are unchanged.
// It must be reconstructed for each query.
class UsePredicateWalk {
public:
  UsePredicate compute(Value *V) { return compute(V, 0); }
  // Returns the selects that contributed an IfTrue/IfFalse during the walk and
  // whose condition is equivalent to Cond.
  SmallPtrSet<SelectInst *, 4> selectsWithCondition(const Value *Cond) const;

private:
  // A cached answer together with the depth budget it was computed with.
  struct CachedUsePredicate {
    UsePredicate Pred;
    unsigned Depth;
  };

  UsePredicate compute(Value *V, unsigned Depth);
  UsePredicate ofUse(const Use &U, unsigned Depth);

  DenseMap<Value *, CachedUsePredicate> Cache;
  // Every select that contributed an IfTrue/IfFalse during the walk.
  SmallVector<SelectInst *, 4> Selects;
};

SmallPtrSet<SelectInst *, 4> UsePredicateWalk::selectsWithCondition(const Value *Cond) const {
  SmallPtrSet<SelectInst *, 4> Result;
  for (SelectInst *SI : Selects)
    if (equivalentConditions(SI->getCondition(), Cond))
      Result.insert(SI);
  return Result;
}

// The use predicate contributed by one use. Anything not recognized as
// value-forwarding is a use on every lane.
UsePredicate UsePredicateWalk::ofUse(const Use &U, unsigned Depth) {
  auto *SI = dyn_cast<SelectInst>(U.getUser());
  if (!SI)
    return forwardsValue(U) ? compute(U.getUser(), Depth + 1) : UsePredicate::allLanes();

  // Feeding the condition, or a vector select, is not a conditional value use.
  if (U.getOperandNo() == 0 || !SI->getCondition()->getType()->isIntegerTy(1))
    return UsePredicate::allLanes();

  UsePredicate SelectUse = compute(SI, Depth + 1);
  // With equivalent true and false values the condition selects nothing, so
  // forward the select's own use predicate.
  if (SelectUse.isUnused() || equivalentValues(SI->getTrueValue(), SI->getFalseValue()))
    return SelectUse;
  // Don't join select's condition with select's own use predicate; report only
  // the condition. This is a safe superset of the observable lanes.
  Selects.push_back(SI);
  return U.getOperandNo() == 1 ? UsePredicate::ifTrue(SI->getCondition()) : UsePredicate::ifFalse(SI->getCondition());
}

UsePredicate UsePredicateWalk::compute(Value *V, unsigned Depth) {
  if (Depth >= MaxUsePredicateDepth)
    return UsePredicate::allLanes();
  auto It = Cache.find(V);
  if (It != Cache.end() && It->second.Depth <= Depth)
    return It->second.Pred;

  UsePredicate Result;
  for (const Use &U : V->uses()) {
    Result = joinUsePredicates(Result, ofUse(U, Depth));
    if (Result.isAllLanes())
      break;
  }

  Cache[V] = {Result, Depth};
  return Result;
}

} // namespace

#define DEBUG_TYPE "reuse-predicated-load"

static cl::opt<unsigned>
    PredLoadReuseMaxCandidates("igc-predload-reuse-max-candidates", cl::init(16), cl::Hidden,
                               cl::desc("Max candidates kept per address for reuse; 0 disables the pass"));

namespace {

class ReusePredicatedLoadImpl {
public:
  explicit ReusePredicatedLoadImpl(Function &F) : F(F), DL(F.getParent()->getDataLayout()), Freezer(F) {}
  bool run();

private:
  using CandidateList = SmallVector<PredicatedLoadIntrinsic *, 4>;

  bool processBlock(BasicBlock &BB);
  PredicatedLoadIntrinsic *findReusable(PredicatedLoadIntrinsic *Load, const CandidateList &Candidates);
  void reuse(PredicatedLoadIntrinsic *Load, PredicatedLoadIntrinsic *Reused);
  bool isUsedOnlyUnder(PredicatedLoadIntrinsic *Load);
  bool impliesPredicate(const UsePredicate &Use, Value *Q) const;
  static bool sameLoadSemantics(const PredicatedLoadIntrinsic *A, const PredicatedLoadIntrinsic *B);

  Function &F;
  const DataLayout &DL;
  FreezeInserter Freezer;
  // Operands left dead by a reuse. Deleting them is deferred until the whole
  // function has been walked.
  SmallVector<WeakTrackingVH, 8> MaybeDead;
};

bool ReusePredicatedLoadImpl::run() {
  bool Changed = false;
  for (BasicBlock &BB : F)
    Changed |= processBlock(BB);

  for (WeakTrackingVH &Handle : MaybeDead)
    if (auto *Dead = dyn_cast_or_null<Instruction>(Handle))
      RecursivelyDeleteTriviallyDeadInstructions(Dead);
  return Changed;
}

bool ReusePredicatedLoadImpl::processBlock(BasicBlock &BB) {
  bool Changed = false;
  // Reuse is limited to the scope of a single basic block.
  DenseMap<Value *, CandidateList> Available;
  for (Instruction &I : make_early_inc_range(BB)) {
    auto *Load = dyn_cast<PredicatedLoadIntrinsic>(&I);
    if (!Load) {
      // A write may invalidate any value read by an available load.
      if (I.mayWriteToMemory())
        Available.clear();
      continue;
    }

    CandidateList &Candidates = Available[Load->getPointerOperand()];
    if (PredicatedLoadIntrinsic *Reused = findReusable(Load, Candidates)) {
      reuse(Load, Reused);
      Changed = true;
      continue;
    }

    if (Candidates.size() >= PredLoadReuseMaxCandidates)
      Candidates.erase(Candidates.begin());
    Candidates.push_back(Load);
  }
  return Changed;
}

PredicatedLoadIntrinsic *ReusePredicatedLoadImpl::findReusable(PredicatedLoadIntrinsic *Load,
                                                               const CandidateList &Candidates) {
  // Search newest first: the nearest candidate is the likeliest match.
  for (PredicatedLoadIntrinsic *Candidate : reverse(Candidates)) {
    // A candidate nothing reads is about to be deleted by the dead-instruction
    // cleanup. Reusing it would bring it back to life.
    if (Candidate->use_empty())
      continue;
    if (Candidate->getType() != Load->getType() || Candidate->getAlignmentValue() != Load->getAlignmentValue() ||
        !sameLoadSemantics(Candidate, Load))
      continue;
    // If later load implies the candidate, it means the candidate already read
    // memory on every lane the later load would have.
    if (!implies(Load->getPredicate(), Candidate->getPredicate(), DL))
      continue;

    // The two predicates are read by two different loads, so freeze the values
    // they share and prove the implication again. Freezing replaces uses, so
    // get both predicates from the loads again.
    FreezeResult Res = Freezer.freezeShared(Load->getPredicate(), Candidate->getPredicate());
    if (Res == FreezeResult::Failed)
      continue;
    if (Res == FreezeResult::NotNeeded)
      return Candidate;
    if (implies(Load->getPredicate(), Candidate->getPredicate(), DL)) {
      Freezer.commit();
      return Candidate;
    }
    Freezer.rollback();
  }
  return nullptr;
}

void ReusePredicatedLoadImpl::reuse(PredicatedLoadIntrinsic *Load, PredicatedLoadIntrinsic *Reused) {
  // Equal predicate and fallback value make the two calls identical, so the earlier
  // value can be used directly. Otherwise the fallback value has to be restored
  // with select instruction outside this load's predicate, unless no user can observe
  // the result there.
  bool NeedsFallbackSelect =
      (Load->getPredicate() != Reused->getPredicate() || Load->getMergeValue() != Reused->getMergeValue()) &&
      !isUsedOnlyUnder(Load);
  Value *Replacement = Reused;
  if (NeedsFallbackSelect) {
    IRBuilder<> Builder(Load);
    Replacement = Builder.CreateSelect(Load->getPredicate(), Reused, Load->getMergeValue());
  }
  LLVM_DEBUG(dbgs() << "ReusePredicatedLoad: reusing " << *Reused << " for " << *Load << "\n");

  for (Value *Operand : Load->operands())
    if (isa<Instruction>(Operand))
      MaybeDead.push_back(Operand);
  Load->replaceAllUsesWith(Replacement);
  Load->eraseFromParent();
}

// True when no user of the load can observe it outside its load's predicate, so a
// value that is only valid under that predicate may replace it without
// restoring a fallback value.
bool ReusePredicatedLoadImpl::isUsedOnlyUnder(PredicatedLoadIntrinsic *Load) {
  UsePredicate Use = UsePredicateWalk().compute(Load);
  if (Use.isUnused())
    return true;
  if (Use.isAllLanes())
    return false;

  // Quick check before attempting a freeze: either side may read a freeze that
  // findReusable() created, which hides the comparison below it. Check that the
  // use condition implies the predicate when looking through the freezes this
  // pass created.
  UsePredicate UseBelowFreezes = Use;
  UseBelowFreezes.Cond = Freezer.lookThroughOwnFreezes(Use.Cond);
  if (!impliesPredicate(UseBelowFreezes, Freezer.lookThroughOwnFreezes(Load->getPredicate())))
    return false;

  // The condition is read by the select and the predicate by the load, so
  // freeze the values they share. The proof is repeated even when nothing was
  // frozen: the check above looked through the freezes this pass created, the
  // proof does not. Freezing replaces uses, so compute the use predicate again.
  FreezeResult Res = Freezer.freezeShared(Use.Cond, Load->getPredicate());
  if (Res == FreezeResult::Failed)
    return false;
  if (Res == FreezeResult::Frozen)
    Use = UsePredicateWalk().compute(Load);
  if (impliesPredicate(Use, Load->getPredicate())) {
    Freezer.commit();
    return true;
  }
  // Without the proof the fallback select is kept, and it needs no freeze.
  Freezer.rollback();
  return false;
}

// True when every lane the use predicate observes has Q set.
bool ReusePredicatedLoadImpl::impliesPredicate(const UsePredicate &Use, Value *Q) const {
  switch (Use.Kind) {
  case UsePredicateKind::Unused:
    return true;
  case UsePredicateKind::IfTrue:
    return implies(Use.Cond, Q, DL);
  case UsePredicateKind::IfFalse:
    return impliesNot(Use.Cond, Q, DL);
  case UsePredicateKind::AllLanes:
    return false;
  }
  return false;
}

// Everything that describes the memory operation, or asserts a fact about its
// result, has to agree.
bool ReusePredicatedLoadImpl::sameLoadSemantics(const PredicatedLoadIntrinsic *A, const PredicatedLoadIntrinsic *B) {
  if (A->getAttributes() != B->getAttributes())
    return false;
  // The caller has checked that the types are equal, so both or neither are
  // FPMathOperator.
  if (isa<FPMathOperator>(A) && A->getFastMathFlags() != B->getFastMathFlags())
    return false;
  SmallVector<std::pair<unsigned, MDNode *>, 8> AMetadata;
  SmallVector<std::pair<unsigned, MDNode *>, 8> BMetadata;
  A->getAllMetadataOtherThanDebugLoc(AMetadata);
  B->getAllMetadataOtherThanDebugLoc(BMetadata);
  return AMetadata == BMetadata;
}

} // namespace

#define REUSE_PASS_FLAG "igc-reuse-predicated-load"
#define REUSE_PASS_DESCRIPTION "Reuse redundant predicated loads"
#define REUSE_PASS_CFG_ONLY false
#define REUSE_PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ReusePredicatedLoad, REUSE_PASS_FLAG, REUSE_PASS_DESCRIPTION, REUSE_PASS_CFG_ONLY,
                          REUSE_PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ReusePredicatedLoad, REUSE_PASS_FLAG, REUSE_PASS_DESCRIPTION, REUSE_PASS_CFG_ONLY,
                        REUSE_PASS_ANALYSIS)

namespace IGC {

char ReusePredicatedLoad::ID = 0;

ReusePredicatedLoad::ReusePredicatedLoad() : FunctionPass(ID) {
  initializeReusePredicatedLoadPass(*PassRegistry::getPassRegistry());
}

FunctionPass *createReusePredicatedLoadPass() { return new ReusePredicatedLoad(); }

bool ReusePredicatedLoad::runOnFunction(Function &F) {
  if (F.isDeclaration() || PredLoadReuseMaxCandidates == 0)
    return false;
  return ReusePredicatedLoadImpl(F).run();
}

} // namespace IGC

#undef DEBUG_TYPE
#define DEBUG_TYPE "shrink-load-predicate"

static cl::opt<unsigned>
    ShrinkPredLoadMaxHoist("igc-shrink-predload-max-hoist", cl::init(2), cl::Hidden,
                           cl::desc("Max instructions hoisted to make a predicated-load use predicate available"));

// Each mask built adds one more i1 live, increasing flag pressure. This option
// caps optimization intentionally.
static cl::opt<unsigned> ShrinkPredLoadMaxMasks("igc-shrink-predload-max-masks", cl::init(8), cl::Hidden,
                                                cl::desc("Max predicate masks created in one function"));

namespace {

// One `and` created when shrinking a predicate.
struct CachedMask {
  Value *Pred;  // the load's original predicate operand
  Value *Cond;  // the use predicate's condition
  bool Negated; // the use predicate was IfFalse, so Cond enters the mask inverted
  Value *Mask;  // the created `Pred & (Negated ? !Cond : Cond)`
};

class ShrinkLoadPredicateImpl {
public:
  ShrinkLoadPredicateImpl(Function &F, DominatorTree &DT)
      : F(F), DT(DT), DL(F.getParent()->getDataLayout()), Freezer(F) {}
  bool run();

private:
  bool process(PredicatedLoadIntrinsic *Load);
  void removeUnobservable(PredicatedLoadIntrinsic *Load);
  void removeFallbackSelects(PredicatedLoadIntrinsic *Load, const UsePredicateWalk &Walk, const UsePredicate &Use);
  void simplifyUsers(ArrayRef<WeakTrackingVH> Users);
  Value *findCachedMask(Value *Pred, const UsePredicate &Use, PredicatedLoadIntrinsic *Load) const;
  Value *createMask(Value *Pred, const UsePredicate &Use, PredicatedLoadIntrinsic *Load);
  bool collectHoist(Value *V, Instruction *Before, SmallVectorImpl<Instruction *> &ToHoist,
                    SmallPtrSetImpl<Instruction *> &Visited, unsigned Depth) const;
  bool dominatesLoad(Value *V, const Instruction *Load) const;
  Instruction *laterDefinition(Value *A, Value *B) const;
  static bool hasResultFacts(const PredicatedLoadIntrinsic *Load);

  Function &F;
  DominatorTree &DT;
  const DataLayout &DL;
  FreezeInserter Freezer;
  // Created masks are cached so that loads agreeing on predicate and use predicate
  // share a single `and` - MemOpt merges adjacent predicated loads only when their
  // predicate operands are the same Value.
  SmallVector<CachedMask, 8> MaskCache;
  // Values left dead by a removed load. Deleting them is deferred until the whole
  // function has been walked.
  SmallVector<WeakTrackingVH, 8> MaybeDead;
  unsigned NumMasks = 0;
};

bool ShrinkLoadPredicateImpl::run() {
  // Snapshot the loads because hoisting and mask creation mutate instruction
  // order while the pass is running.
  SmallVector<PredicatedLoadIntrinsic *, 16> Loads;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      if (auto *Load = dyn_cast<PredicatedLoadIntrinsic>(&I))
        Loads.push_back(Load);

  bool Changed = false;
  for (PredicatedLoadIntrinsic *Load : Loads)
    Changed |= process(Load);

  for (WeakTrackingVH &Handle : MaybeDead)
    if (auto *Dead = dyn_cast_or_null<Instruction>(Handle))
      RecursivelyDeleteTriviallyDeadInstructions(Dead);
  return Changed;
}

// Restrict one load to the lanes where its result is observable by shrinking
// its predicate to `%pred & %cond`. If the predicate proves the condition against
// the use, the load is completely removed. Returns true if the predicate was
// shrunk or the load removed, false if it was left alone.
bool ShrinkLoadPredicateImpl::process(PredicatedLoadIntrinsic *Load) {
  if (Load->use_empty() || hasResultFacts(Load))
    return false;

  // The walk starts over for every load: shrinking adds uses as it goes, so a
  // cache shared across loads would need invalidation. All observable paths must
  // agree on one condition before shrinking is safe.
  UsePredicateWalk Walk;
  UsePredicate Use = Walk.compute(Load);
  if (Use.isUnused() || Use.isAllLanes())
    return false;

  bool Negated = Use.Kind == UsePredicateKind::IfFalse;
  Value *Pred = Load->getPredicate();
  // If mask is `%pred & %cond` and `%pred` implies `%cond`, the mask folds back to `%pred`.
  // If mask is `%pred & !%cond` and `%pred` implies `%cond`, the mask folds back false.
  std::optional<bool> KnownCond = impliedValue(Pred, Use.Cond, DL, /*LHSIsTrue=*/true);

  // The mask folds back to the predicate, so shrinking cannot mask off a single
  // lane. Nothing is changed.
  if (KnownCond == !Negated)
    return false;

  // The mask folds to false: no lane that reads memory is ever observed, so the
  // load can be removed. The predicate is read by the load and the condition by
  // the select, so freeze the values they share and prove the implication
  // again. Freezing replaces uses, so compute the use predicate again. When
  // nothing needed a freeze, the IR is unchanged and the proof above holds.
  if (KnownCond == Negated) {
    FreezeResult Res = Freezer.freezeShared(Pred, Use.Cond);
    if (Res == FreezeResult::NotNeeded) {
      removeUnobservable(Load);
      return true;
    }
    if (Res == FreezeResult::Frozen) {
      UsePredicate FrozenUse = UsePredicateWalk().compute(Load);
      if (!FrozenUse.isUnused() && !FrozenUse.isAllLanes() &&
          impliedValue(Load->getPredicate(), FrozenUse.Cond, DL, /*LHSIsTrue=*/true) ==
              (FrozenUse.Kind == UsePredicateKind::IfFalse)) {
        Freezer.commit();
        removeUnobservable(Load);
        return true;
      }
      // Fall through and shrink instead. Rolling back erases the freezes and
      // can reorder use lists, so compute the use predicate again.
      Freezer.rollback();
      Walk = UsePredicateWalk();
      Use = Walk.compute(Load);
      if (Use.isUnused() || Use.isAllLanes())
        return false;
      Negated = Use.Kind == UsePredicateKind::IfFalse;
    }
  }

  // An already built mask adds no i1, so reuse is free. It also needs no hoist -
  // the cached mask dominates the load.
  if (Value *Cached = findCachedMask(Pred, Use, Load)) {
    Load->setPredicate(Cached);
    removeFallbackSelects(Load, Walk, Use);
    LLVM_DEBUG(dbgs() << "ShrinkLoadPredicate: reused mask for " << *Load << "\n");
    return true;
  }

  // Building a new mask costs a flag. If an unconditional predicate shrinks to
  // the condition itself, `createMask` returns condition directly, which costs
  // nothing.
  const bool BuildsMask = Negated || !match(Pred, m_One());
  if (BuildsMask && NumMasks >= ShrinkPredLoadMaxMasks) {
    LLVM_DEBUG(dbgs() << "ShrinkLoadPredicate: mask budget exhausted\n");
    return false;
  }

  // Everything that can reject the shrink is checked before the IR changes:
  // the hoist, the freeze of the condition, and the mask placement.
  SmallVector<Instruction *, 4> ToHoist;
  SmallPtrSet<Instruction *, 8> Visited;
  if (!collectHoist(Use.Cond, Load, ToHoist, Visited, 0) || ToHoist.size() > ShrinkPredLoadMaxHoist)
    return false;
  if (!Freezer.canFreeze(Use.Cond))
    return false;
  // The mask goes after the later of the predicate and the condition. A hoisted
  // condition ends up right before the load, after the predicate.
  if (Instruction *Def = laterDefinition(Pred, Use.Cond))
    if (!IGCLLVM::getInsertionPointAfterDef(Def))
      return false;

  for (Instruction *I : ToHoist)
    IGCLLVM::moveBefore(I, Load);

  // The condition gets one more use, in the mask, so freeze it. freezeValue()
  // makes every use of Use.Cond read the freeze. The use predicate join may also
  // have accepted identical copies of the condition (equivalentConditions()). Their
  // selects still read the copy, which could differ from the freeze if undef, so
  // set their condition to the freeze too. The condition dominates the load after
  // the hoist, and the load dominates every select it reaches, so the freeze
  // dominates them too.
  Value *Cond = Freezer.freezeValue(Use.Cond);
  Freezer.commit();
  if (Cond != Use.Cond) {
    // After the freeze, only the selects reading a copy still match Use.Cond.
    for (SelectInst *SI : Walk.selectsWithCondition(Use.Cond)) {
      MaybeDead.push_back(SI->getCondition());
      SI->setCondition(Cond);
    }
    Use.Cond = Cond;
  }
  Pred = Load->getPredicate();
  Value *Mask = createMask(Pred, Use, Load);
  Load->setPredicate(Mask);
  if (BuildsMask)
    ++NumMasks;
  removeFallbackSelects(Load, Walk, Use);
  LLVM_DEBUG(dbgs() << "ShrinkLoadPredicate: shrunk predicate of " << *Load << "\n");
  return true;
}

// After shrinking, the load returns its fallback value on every lane where the
// use condition rejects it. A select on that condition whose other incoming value
// is the same fallback value then equals the load, so it is replaced by the load.
// Only selects that read the load directly are handled.
void ShrinkLoadPredicateImpl::removeFallbackSelects(PredicatedLoadIntrinsic *Load, const UsePredicateWalk &Walk,
                                                    const UsePredicate &Use) {
  const bool Negated = Use.Kind == UsePredicateKind::IfFalse;
  Value *Fallback = Load->getMergeValue();
  // Select are unordered, collect first.
  SmallVector<SelectInst *, 4> Identities;
  for (SelectInst *SI : Walk.selectsWithCondition(Use.Cond)) {
    Value *Observed = Negated ? SI->getFalseValue() : SI->getTrueValue();
    Value *Other = Negated ? SI->getTrueValue() : SI->getFalseValue();
    if (Observed == Load && Other == Fallback)
      Identities.push_back(SI);
  }
  for (SelectInst *SI : Identities) {
    LLVM_DEBUG(dbgs() << "ShrinkLoadPredicate: select equals the shrunk load " << *SI << "\n");
    SI->replaceAllUsesWith(Load);
    MaybeDead.push_back(SI);
  }
}

// Remove a load whose result no consumer can observe, and let those consumers
// take its fallback value instead.
void ShrinkLoadPredicateImpl::removeUnobservable(PredicatedLoadIntrinsic *Load) {
  LLVM_DEBUG(dbgs() << "ShrinkLoadPredicate: removing unobservable " << *Load << "\n");

  SmallVector<WeakTrackingVH, 4> Users(Load->users());
  for (Value *Operand : Load->operands())
    if (isa<Instruction>(Operand))
      MaybeDead.push_back(Operand);

  Load->replaceAllUsesWith(Load->getMergeValue());
  Load->eraseFromParent();

  simplifyUsers(Users);
}

// Fold what the fallback value makes trivial in the consumers of a removed load -
// a `select %c, %x, %a` whose fallback value is also %x becomes `select %c, %x, %x`.
void ShrinkLoadPredicateImpl::simplifyUsers(ArrayRef<WeakTrackingVH> Users) {
  SmallVector<WeakTrackingVH, 8> Worklist(Users.begin(), Users.end());
  SmallPtrSet<Value *, 8> Visited;
  const SimplifyQuery Query(DL);
  for (unsigned Idx = 0; Idx != Worklist.size(); ++Idx) {
    auto *User = dyn_cast_or_null<Instruction>(Worklist[Idx]);
    if (!User || User->use_empty() || !Visited.insert(User).second)
      continue;
    Value *Folded = IGCLLVM::simplifyInstruction(User, Query);
    if (!Folded)
      continue;
    Worklist.append(User->user_begin(), User->user_end());
    User->replaceAllUsesWith(Folded);
    MaybeDead.push_back(User);
  }
}

// An already built mask usable for this load, or null. Reusing one allows MemOpt
// to merge adjacent loads.
Value *ShrinkLoadPredicateImpl::findCachedMask(Value *Pred, const UsePredicate &Use,
                                               PredicatedLoadIntrinsic *Load) const {
  bool Negated = Use.Kind == UsePredicateKind::IfFalse;
  for (const CachedMask &Entry : MaskCache)
    if (Entry.Pred == Pred && Entry.Negated == Negated && equivalentValues(Entry.Cond, Use.Cond) &&
        dominatesLoad(Entry.Mask, Load))
      return Entry.Mask;
  return nullptr;
}

// Build the mask for this load and cache it. The caller has checked that the
// mask can be placed after the later of Pred and the condition.
Value *ShrinkLoadPredicateImpl::createMask(Value *Pred, const UsePredicate &Use, PredicatedLoadIntrinsic *Load) {
  bool Negated = Use.Kind == UsePredicateKind::IfFalse;

  // Define the mask right after the later of its operands.
  IRBuilder<> Builder(Load);
  if (Instruction *Def = laterDefinition(Pred, Use.Cond)) {
    std::optional<BasicBlock::iterator> AfterDef = IGCLLVM::getInsertionPointAfterDef(Def);
    IGC_ASSERT_MESSAGE(AfterDef.has_value(), "the mask insertion point is checked before the shrink starts");
    Builder.SetInsertPoint((*AfterDef)->getParent(), *AfterDef);
  }
  Value *Cond = Negated ? Builder.CreateNot(Use.Cond, "predload.usepred.not") : Use.Cond;
  // An unconditional predicate shrinks to the condition itself, with nothing to AND.
  Value *Mask = match(Pred, m_One()) ? Cond : Builder.CreateAnd(Pred, Cond, "predload.shrunk");
  MaskCache.push_back({Pred, Use.Cond, Negated, Mask});
  return Mask;
}

// Collect the same-block instructions that must move before the load to make V
// available there, operands first. Memory reads and side effects are rejected.
// Visited marks the nodes already collected; PHIs are rejected.
bool ShrinkLoadPredicateImpl::collectHoist(Value *V, Instruction *Before, SmallVectorImpl<Instruction *> &ToHoist,
                                           SmallPtrSetImpl<Instruction *> &Visited, unsigned Depth) const {
  auto *I = dyn_cast<Instruction>(V);
  if (!I || DT.dominates(I, Before) || Visited.count(I))
    return true;
  if (Depth >= ShrinkPredLoadMaxHoist || I->getParent() != Before->getParent() || isa<PHINode>(I) ||
      I->isTerminator() || I->mayReadFromMemory() || I->mayHaveSideEffects() || !isSafeToSpeculativelyExecute(I))
    return false;

  for (Value *Op : I->operands())
    if (!collectHoist(Op, Before, ToHoist, Visited, Depth + 1))
      return false;
  Visited.insert(I);
  ToHoist.push_back(I);
  return true;
}

// Returns true when V dominates Load. A V that is not an instruction has no
// definition point and is available everywhere.
bool ShrinkLoadPredicateImpl::dominatesLoad(Value *V, const Instruction *Load) const {
  auto *I = dyn_cast<Instruction>(V);
  return !I || DT.dominates(I, Load);
}

// Of two values that both dominate the same instruction, return the definition
// that comes last, or nullptr when neither is an instruction.
Instruction *ShrinkLoadPredicateImpl::laterDefinition(Value *A, Value *B) const {
  auto *AI = dyn_cast<Instruction>(A);
  auto *BI = dyn_cast<Instruction>(B);
  if (!AI || !BI)
    return AI ? AI : BI;
  return DT.dominates(AI, BI) ? BI : AI;
}

// Shrinking makes the fallback value observable on lanes that used to read memory,
// so every fact asserted about the result would have to hold for the fallback
// value too.
bool ShrinkLoadPredicateImpl::hasResultFacts(const PredicatedLoadIntrinsic *Load) {
  if (isa<FPMathOperator>(Load) && (Load->hasNoNaNs() || Load->hasNoInfs()))
    return true;
  return Load->getAttributes().getRetAttrs().hasAttributes() || Load->hasMetadata(LLVMContext::MD_range) ||
         Load->hasMetadata(LLVMContext::MD_noundef);
}

} // namespace

#define SHRINK_PASS_FLAG "igc-shrink-load-predicate"
#define SHRINK_PASS_DESCRIPTION "Shrink predicated load predicates to their use predicates"
#define SHRINK_PASS_CFG_ONLY false
#define SHRINK_PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ShrinkLoadPredicate, SHRINK_PASS_FLAG, SHRINK_PASS_DESCRIPTION, SHRINK_PASS_CFG_ONLY,
                          SHRINK_PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(ShrinkLoadPredicate, SHRINK_PASS_FLAG, SHRINK_PASS_DESCRIPTION, SHRINK_PASS_CFG_ONLY,
                        SHRINK_PASS_ANALYSIS)

namespace IGC {

char ShrinkLoadPredicate::ID = 0;

ShrinkLoadPredicate::ShrinkLoadPredicate() : FunctionPass(ID) {
  initializeShrinkLoadPredicatePass(*PassRegistry::getPassRegistry());
}

FunctionPass *createShrinkLoadPredicatePass() { return new ShrinkLoadPredicate(); }

bool ShrinkLoadPredicate::runOnFunction(Function &F) {
  if (F.isDeclaration())
    return false;
  return ShrinkLoadPredicateImpl(F, getAnalysis<DominatorTreeWrapperPass>().getDomTree()).run();
}

} // namespace IGC
