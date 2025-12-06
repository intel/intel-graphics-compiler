/*========================== begin_copyright_notice ============================

Copyright (C) 2025-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "IntDivRemIncrementReduction.hpp"

#include "common/igc_regkeys.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Support/Debug.h"
#include "common/LLVMWarningsPop.hpp"

#define DEBUG_TYPE "igc-divrem-increment-reduction"

using namespace IGC;
using namespace llvm;

namespace IGC {

struct DivRemPair;
struct DivRemGroup;
struct DivRemChain;

// DivRemPair, stores info related to a udiv/urem pair using the same dividend and same divisor
// urem may be first in inst order, and may not be back-to-back insts.
// ex.
// %c = udiv i32 %a, %b
// %d = urem i32 %a, %b
struct DivRemPair {
  Instruction *Div = nullptr;            // original quotient
  Instruction *Rem = nullptr;            // original remainder
  Value *newDiv = nullptr;               // optimized quotient
  Value *newRem = nullptr;               // optimized remainder
  Instruction *simpleInsertPt = nullptr; // insert point for simple (no branch) optimization
  Instruction *normalInsertPt = nullptr; // insert point for normal unoptimized branch
  ICmpInst *DivIsChangingTest = nullptr; // test for quotient changing (used by next nested group div/rem)
  ICmpInst *DivisorIsZeroTest = nullptr; // test for divisor being zero

  DivRemPair(Instruction *div, Instruction *rem) : Div(div), Rem(rem) {
    IGC_ASSERT_MESSAGE(div->getOpcode() == Instruction::UDiv && rem->getOpcode() == Instruction::URem,
                       "Invalid DivRem Pair, non-matching instruction types and/or signed/unsigned types");
    IGC_ASSERT_MESSAGE(div->getOperand(0) == rem->getOperand(0), "Invalid DivRem Pair, non-matching dividend");
    IGC_ASSERT_MESSAGE(div->getOperand(1) == rem->getOperand(1), "Invalid DivRem Pair, non-matching divisor");
    simpleInsertPt = div->comesBefore(rem) ? div : rem; // insert before the first of the two
  }

  // Helpers for getting various values
  Value *getDividend() const { return Div->getOperand(0); }
  Value *getDivisor() const { return Div->getOperand(1); }
  // return optimized value if exists, used for downstream chain + group optimizations
  Value *getQuotient() const { return newDiv ? newDiv : Div; }
  Value *getRemainder() const { return newRem ? newRem : Rem; }

  // Returns whether the optimization created a conditional branch or an unconditional branch
  bool isSimple() const {
    IGC_ASSERT(newDiv); // Should only be calling isSimple on a DivRemPair that has been simplified
    return !isa<PHINode>(newDiv);
  }

  // If div/rem pair has been optimized using the simple (no branches) method, then the original udiv/urem are dead,
  // and can be deleted
  void deleteDeadDivRem() const {
    if (newDiv && !isa<PHINode>(newDiv)) {
      Div->eraseFromParent();
      Rem->eraseFromParent();
    }
  }

  // Specialized simplify method taking advantage of current dividend being an increment by a constant of a previous
  // udiv/urem's dividend
  // Also can simplify a nested div/rem pair (parent div quotient used as dividend, increases by 1 or remains the same)
  // This function references DivRemGroups, but is placed here to associate it as simplifying a single DivRemPair
  void simplify(const DivRemGroup *chainPrevDivRemGroup, const DivRemGroup *divRemGroup, APInt offset,
                unsigned int idx);

  // Perform simple optimiztion (increment by 1) on current DivRemPair, using previous DivRemPair's result
  void simplifySimple(APInt offset, DivRemPair *prevDivRemPair);
};

// DivRemGroup, group of udiv/urem that can participate in a single trickle down optimization
// ex.
// %c = udiv i32 %a, %b
// %d = urem i32 %a, %b
// %f = udiv i32 %c, %e
// %g = urem i32 %c, %e
//
// TODO: Optimizations where the nested udiv/urem operate on the previous remainder instead of the previous quotient
// may also be possible ex.
// %c = udiv i32 %a, %b
// %d = urem i32 %a, %b
// %f = udiv i32 %d, %e
// %g = urem i32 %d, %e
struct DivRemGroup {
  Value *Base = nullptr;
  APInt Offset;
  SmallVector<std::unique_ptr<DivRemPair>> DivRems;

  DivRemGroup(Value *base, APInt offset, SmallVector<std::unique_ptr<DivRemPair>> divRems)
      : Base(base), Offset(offset), DivRems(std::move(divRems)) {}

  // Simplify all DivRemPairs in the group given the true offset from the referenced chainPrevDivRemGroup
  void simplify(APInt trueOffset, const DivRemGroup *chainPrevDivRemGroup) const;

  void deleteDeadDivRems() const {
    for (unsigned i = 0; i < DivRems.size(); i++) {
      DivRems[i]->deleteDeadDivRem();
    }
  }
};

struct DivRemChain {
  SmallVector<std::unique_ptr<DivRemGroup>> Chain;

  DivRemChain(std::unique_ptr<DivRemGroup> group) { Chain.push_back(std::move(group)); }

  Value *getBaseDividend() const { return Chain.front()->Base; }

  void addDivRemGroup(std::unique_ptr<DivRemGroup> divRemGroup) { Chain.push_back(std::move(divRemGroup)); }

  // Trim uneven depths of all DivRemGroups in the chain so that all of the groups have the same depth
  void trim() const;
  // Simplify the DivRemPairs in a chain
  void simplify() const;
  // Delete dead udiv/urem instructions that were replaced with optimized instructions
  void deleteDeadDivRems() const;
};

class IntDivRemIncrementReductionImpl {
public:
  IntDivRemIncrementReductionImpl(DominatorTree *DT) : DT(DT) {}
  bool run(Function &F);

private:
  DominatorTree *DT;

  // Get Base and Offset for a value if it matches V = Base + ConstantOffset or V = Base | ConstantOffset
  std::pair<Value *, APInt> getBaseAndOffset(Value *V);

  // Find the urem instruction for a given udiv instruction. Returns nullptr if not found
  Instruction *getRemForDiv(Instruction *div);

  // Compare and decide whether a new DivRemGroup is compatible with an existing DivRemGroup in a chain
  bool compareDivRemGroups(std::unique_ptr<DivRemGroup> &group1, SmallVector<std::unique_ptr<DivRemPair>> &group2);

  // Perform CSE on the DivisorIsZeroTest instructions for the whole chain
  void divisorIsZeroCSE(DivRemChain *chain, Function &F, DominatorTree *DT) const;
};

void DivRemPair::simplifySimple(APInt offset, DivRemPair *chainPrevDivRemPair) {
  auto *prevQuo = chainPrevDivRemPair->getQuotient();
  auto *prevRem = chainPrevDivRemPair->getRemainder();
  IGC_ASSERT(!offset.isZero());
  IRBuilder<> builder(simpleInsertPt);
  unsigned bitWidth = Div->getType()->getIntegerBitWidth();
  // %prevDividend = ...
  // %divisor = ...
  // %prevQuo = [udiv|simplified form] i32 %prevDividend, %divisor
  // %prevRem = [urem|simplified form] i32 %prevDividend, %divisor
  // %dividend = add i32 %prevDividend, OFFSET (constant int)
  // %quo = udiv i32 %dividend, %divisor
  // %rem = urem i32 %dividend, %divisor
  if (offset.isStrictlyPositive()) {
    // Positive offset, non-branching optimization (OFFSET <= divisor)
    // %quo is either:
    //  1. %prevQuo if (%prevRem + OFFSET)  %sameDivisor
    //  2. %prevQuo + 1 if %prevRem + OFFSET does add up to %sameDivisor
    // %rem is either:
    //  1. %prevRem + 1 if %prevRem + 1 does not add up to %sameDivisor
    //  2. 0 if %prevRem + 1 does add up to %sameDivisor (since %quo would increment by 1)

    // pre-increment remainder by offset:
    auto *preIncRem = builder.CreateAdd(prevRem, builder.getInt(offset), "pre.inc." + prevRem->getName());
    // check if previous remainder + offset results in 1 more time that %dividend goes into %sameDivisor
    DivIsChangingTest = cast<ICmpInst>(builder.CreateICmp((offset.isOne() ? ICmpInst::ICMP_EQ : ICmpInst::ICMP_UGE),
                                                          preIncRem, getDivisor(), "cmp." + preIncRem->getName()));
    // pre-increment quotient by 1
    auto *preIncDiv = builder.CreateAdd(prevQuo, builder.getInt(APInt(bitWidth, 1)), "pre.inc." + prevQuo->getName());
    // pre-calculate new remainder if preIncRemTest is true, since that indicates that new dividend divides divisor 1
    // more time
    // do this only if offset is not 1, because if offset is 1, then new remainder is always 0
    Value *preDecRem = nullptr;
    if (!offset.isOne()) {
      preDecRem = builder.CreateSub(preIncRem, getDivisor(), "pre.dec." + prevRem->getName());
    } else {
      preDecRem = builder.getInt(APInt::getZero(bitWidth));
    }
    // form new quotient
    newDiv = builder.CreateSelect(DivIsChangingTest, preIncDiv, prevQuo, "new.div." + getQuotient()->getName());
    // form new remainder
    newRem = builder.CreateSelect(DivIsChangingTest, preDecRem, preIncRem, "new.rem." + getRemainder()->getName());
  } else {
    // TODO: Handle negative offsets
  }
}

void DivRemPair::simplify(const DivRemGroup *chainPrevDivRemGroup, const DivRemGroup *divRemGroup, APInt offset,
                          unsigned int idx) {
  //         base         offset
  // chainPrevDivRemGroup | +x |   divRemGroup
  // ---------------------|    |------------------
  //                      |    | groupPrevDivRem (idx-1)
  //                      |    |         |
  //                      |    |         v
  // chainPrevDivRem (idx)| -> |     udiv/urem (idx)
  // Since we do not simplify the first DivRemGroup in a DivRemChain, chainPrevDivRemGroup is guaranteed to not be
  // nullptr
  auto *chainPrevDivRem = chainPrevDivRemGroup->DivRems[idx].get();
  auto *groupPrevDivRem = idx == 0 ? nullptr : divRemGroup->DivRems[idx - 1].get();
  IGC_ASSERT(!offset.isZero());
  IGC_ASSERT(chainPrevDivRem && chainPrevDivRem->getDivisor() == getDivisor()); // Required

  // Insert point for simple optimization described below
  if (groupPrevDivRem && !groupPrevDivRem->isSimple()) {
    // Previous div/rem in group used normal (branched) optimization, so insert point is terminator of that branch
    // This is because the simple optimization cannot be performed on a nested quotient which was not simplified
    // Thus, if one div/rem pair in a group uses the normal optimization, all subsequent div/rem pairs in that group
    // will also be branched
    simpleInsertPt = groupPrevDivRem->simpleInsertPt;
  }

  IRBuilder<> builder(simpleInsertPt);
  unsigned bitWidth = Div->getType()->getIntegerBitWidth();
  if (offset.isOne()) {
    simplifySimple(offset, chainPrevDivRem);
    if (groupPrevDivRem) {
      // not first udiv/urem in group
      // %prevDividend = ...
      // %divisor = ...
      // %divisor2 = ...
      // %prevQuo = [udiv|simplified form] i32 %prevDividend, %divisor
      // %prevRem = [urem|simplified form] i32 %prevDividend, %divisor
      // %prevNestedQuo = [udiv|simplified form] i32 %prevQuo, %divisor2
      // %prevNestedRem = [urem|simplified form] i32 %prevQuo, %divisor2
      // %dividend = add i32 %prevDividend, OFFSET
      // %quo = udiv i32 %dividend, %divisor
      // %rem = urem i32 %dividend, %divisor
      // %nestedQuo = udiv i32 %quo, %divisor2
      // %nestedRem = urem i32 %quo, %divisor2
      // For a nested DivRemPair, the offset for continuing the branch containing the simple optimization of the prior
      // DivRemPair in the DivRemGroup is 1 (guaranteed because of initial branching ICmp)
      // %nestedQuo is either:
      //  1. %prevNestedQuo if %quo is equal to %prevQuo
      //  2. %prevNestedQuo if %quo is equal to %prevQuo + 1 but %prevNestedRem + OFFSET < %divisor2
      //  3. %prevNestedQuo + 1 if %quo is equal to %prevQuo + 1 and %prevNestedRem + OFFSET >= %divisor2
      // %nestedRem is either:
      //  1. %prevNestedRem if %quo is equal to %prevQuo
      //  2. %prevNestedRem + OFFSET if %quo is equal to %prevQuo + 1 but %prevNestedRem + OFFSET < %divisor2
      //  3. %prevNestedRem + OFFSET - %divisor2 if %quo is equal to %prevQuo + 1 and %prevNestedRem + OFFSET >=
      //     %divisor2
      //
      // create the merging select statements for a three-way merge for a nested DivRemPair
      // can AND prevGroupDivRem DivIsChanging test with current DivIsChanging test to avoid additional select inst for
      // div unavoidable for rem since 3 different possible values, compared to 2 for div
      builder.SetInsertPoint(cast<Instruction>(newDiv));
      auto *andDivIsChangingTest =
          builder.CreateAnd(DivIsChangingTest, groupPrevDivRem->DivIsChangingTest, "use.new.div.");
      cast<SelectInst>(newDiv)->setCondition(andDivIsChangingTest);
      builder.SetInsertPoint(simpleInsertPt);
      newRem = builder.CreateSelect(groupPrevDivRem->DivIsChangingTest, newRem, chainPrevDivRem->getRemainder(),
                                    "merge." + getRemainder()->getName());
    }

    if (IGC_IS_FLAG_ENABLED(SanitizeDivRemIncrementDivisorIsZero)) {
      // add ICmp + Select for case when divisor is 0
      DivisorIsZeroTest =
          cast<ICmpInst>(builder.CreateICmp(ICmpInst::ICMP_EQ, getDivisor(), builder.getInt(APInt::getZero(bitWidth)),
                                            "divisor.is.zero" + getDivisor()->getName()));
      // sanitize and return -1 or 0xFFFF... for div and rem respectively
      newDiv =
          builder.CreateSelect(DivisorIsZeroTest, builder.getInt(APInt::getAllOnes(bitWidth)), newDiv, "sanitized.div");
      newRem =
          builder.CreateSelect(DivisorIsZeroTest, builder.getInt(APInt::getAllOnes(bitWidth)), newRem, "sanitized.rem");
    }

    if (!groupPrevDivRem || groupPrevDivRem->isSimple()) {
      // No previous udiv/urem or previous udiv/urem had uncond optimization done, no extra logic needed
      Div->replaceAllUsesWith(newDiv);
      Rem->replaceAllUsesWith(newRem);
      return;
    } else {
      // Previous udiv/urem had cond optimization done, set normalInsertPt and continue to common PHINode creation
      // below
      normalInsertPt = groupPrevDivRem->normalInsertPt;
    }
  } else if (offset.isAllOnes()) {
    // TODO: Handle -1 offset case with uncond optimization
  } else if (offset.isNegative()) {
    // TODO: Handle negative offset case > 1 with cond optimization
  } else {
    // Constant > 1, need to add ICmp to test whether offset is lesser than or equal to divisor (will be a runtime
    // check)
    // If so, can still do a simplified calculation, otherwise fallback to normal div/rem (will be replaced with
    // precompiled func later)
    // This will create a conditional branch that replaces the udiv/urem to allow for the simple path to be taken at
    // runtime
    // This optimization can be restricted by the DivRemIncrementCondBranchSimplify flag, which will make all groups
    // with offset > 1 go unoptimized if disabled, therefore not creating any extra conditional branches
    Value *divisorTest = builder.CreateICmp(ICmpInst::ICMP_ULE, builder.getInt(offset), getDivisor());
    Instruction *simpleDivRem = nullptr; // then block, unconditional branch inst to join block
    Instruction *normalDivRem = nullptr; // else block, unconditional branch inst to join block

    // Create if and else block. If block will contain simplified udiv/urem based on previous result, else block will
    // contain udiv/urem unoptimized insts
    SplitBlockAndInsertIfThenElse(divisorTest, Div, &simpleDivRem, &normalDivRem);
    simpleDivRem->getParent()->setName("simple.div.rem");
    simpleInsertPt = simpleDivRem;
    normalDivRem->getParent()->setName("normal.div.rem");
    normalInsertPt = normalDivRem;

    simplifySimple(offset, chainPrevDivRem);

    if (IGC_IS_FLAG_ENABLED(SanitizeDivRemIncrementDivisorIsZero)) {
      builder.SetInsertPoint(simpleInsertPt);
      // add ICmp + Select for case when divisor is 0
      DivisorIsZeroTest =
          cast<ICmpInst>(builder.CreateICmp(ICmpInst::ICMP_EQ, getDivisor(), builder.getInt(APInt::getZero(bitWidth)),
                                            "divisor.is.zero" + getDivisor()->getName()));
      // sanitize and return -1 or 0xFFFF... for div and rem respectively
      newDiv =
          builder.CreateSelect(DivisorIsZeroTest, builder.getInt(APInt::getAllOnes(bitWidth)), newDiv, "sanitized.div");
      newRem =
          builder.CreateSelect(DivisorIsZeroTest, builder.getInt(APInt::getAllOnes(bitWidth)), newRem, "sanitized.rem");
    }
  }

  auto noOverrideDivRemInGroup = [&](Use &U) {
    for (unsigned i = 0; i < divRemGroup->DivRems.size(); i++) {
      if (U.getUser() == divRemGroup->DivRems[i]->Div || U.getUser() == divRemGroup->DivRems[i]->Rem)
        return false;
    }
    return true;
  };

  auto *joinInsertPt = Div->comesBefore(Rem) ? Rem->getNextNode() : Div->getNextNode();

  // Either predecessor DivRemPair got transformed to conditional and need to retain udiv and urem in the normal branch,
  // or current DivRemPair is undergoing conditional transformation, and need to create join PHINodes and retain udiv
  // and urem in the normal branch
  // create phis for this udiv/urem pair at joinInsertPt, join the simple and normal results
  auto *joinDiv = PHINode::Create(Div->getType(), 2, "join.div.", joinInsertPt);
  Div->replaceUsesWithIf(joinDiv, noOverrideDivRemInGroup); // do not replace uses in the normal branch
  auto *joinRem = PHINode::Create(Rem->getType(), 2, "join.rem.", joinInsertPt);
  Rem->replaceUsesWithIf(joinRem, noOverrideDivRemInGroup); // do not replace uses in the normal branch
  Div->moveBefore(normalInsertPt);
  Rem->moveBefore(normalInsertPt);

  // connect phis
  joinDiv->addIncoming(Div, normalInsertPt->getParent());
  joinDiv->addIncoming(newDiv, simpleInsertPt->getParent());

  joinRem->addIncoming(Rem, normalInsertPt->getParent());
  joinRem->addIncoming(newRem, simpleInsertPt->getParent());

  newDiv = joinDiv;
  newRem = joinRem;
}

void DivRemGroup::simplify(APInt trueOffset, const DivRemGroup *chainPrevDivRemGroup) const {
  if (trueOffset.isZero()) {
    LLVM_DEBUG(dbgs() << "Offset is zero, doing CSE\n");
    // No offset, nothing to optimize
    for (unsigned i = 0; i < DivRems.size(); i++) {
      DivRems[i]->newDiv = chainPrevDivRemGroup->DivRems[i]->getQuotient();
      DivRems[i]->Div->replaceAllUsesWith(DivRems[i]->newDiv);
      DivRems[i]->newRem = chainPrevDivRemGroup->DivRems[i]->getRemainder();
      DivRems[i]->Rem->replaceAllUsesWith(DivRems[i]->newRem);
    }
    return;
  }
  // First DivRemGroup, call simplify with trueOffset
  DivRems.front()->simplify(chainPrevDivRemGroup, this, trueOffset, 0);
  LLVM_DEBUG(dbgs() << "Simplified first DivRemPair in DivRemGroup\n");
  // TODO: Based on sign of trueOffset, call simplify accordingly with +1 or -1 for nested DivRemPairs
  for (unsigned i = 1; i < DivRems.size(); i++) {
    // Nested div/rems in a group, if simplified, will only ever have to deal with the dividend
    // that is passed down from the previous div/rem in the group potentially increasing by 1
    DivRems[i]->simplify(chainPrevDivRemGroup, this,
                         (trueOffset.isZero()       ? trueOffset
                          : trueOffset.isNegative() ? APInt(trueOffset.getBitWidth(), -1, true)
                                                    : APInt(trueOffset.getBitWidth(), 1)),
                         i);
  }
}

// Trim the number of DivRemPairs in each DivRemGroup in this DivRemChain in case they are uneven (in terms of number of
// DivRemPairs in each DivRemGroup)
void DivRemChain::trim() const {
  unsigned minDepth = Chain.front()->DivRems.size();
  for (auto &DRG : Chain) {
    if (DRG->DivRems.size() < minDepth) {
      minDepth = DRG->DivRems.size();
    }
  }

  for (auto &DRG : Chain) {
    while (DRG->DivRems.size() > minDepth) {
      DRG->DivRems.pop_back();
    }
  }
}

// Call simplify on each DivRemGroup in a DivRemChain
void DivRemChain::simplify() const {
  // Start from index 1, since the 0-index udiv/urem are kept as baseline full calculation
  for (unsigned i = 1; i < Chain.size(); i++) {
    // Find the minimum abs true offset between this group and a previous computed group in the chain
    APInt minAbsTrueOffset = Chain[i]->Offset - Chain[i - 1]->Offset;
    auto *chainPrevDivRemGroup = Chain[i - 1].get();

    // Find best chainPrevDivRemGroup to use for optimization
    for (unsigned j = 0; j < (i - 1); j++) {
      // Calculate true offset between Chain[i] and Chain[j]
      auto trueOffset = Chain[i]->Offset - Chain[j]->Offset;
      // Prefer 1 over -1 because 1 less instruction to generate (divisor - 1), and prefer 0 over both since no
      // optimization needed, can just replace all uses with prior result
      if (trueOffset.abs().slt(minAbsTrueOffset.abs()) && !minAbsTrueOffset.isOne() && !minAbsTrueOffset.isZero()) {
        minAbsTrueOffset = trueOffset;
        chainPrevDivRemGroup = Chain[j].get();
      }
    }

    // TODO: Can handle negative 1 as well using non-branching logic
    if (minAbsTrueOffset.isNegative() || (!minAbsTrueOffset.isOne() && !minAbsTrueOffset.isZero() &&
                                          IGC_IS_FLAG_DISABLED(DivRemIncrementCondBranchSimplify))) {
      // Flag to guard conditional branch creation for optimization disabled, do not optimize this DivRemGroup
      // The subsequent DivRemGroup (if it exists and the true offset is 1 from this group) will use the non-optimized
      // udiv/urem results of the current udiv/urem group in its optimizations
      continue;
    }

    Chain[i]->simplify(minAbsTrueOffset, chainPrevDivRemGroup);
  }
}

// Delete dead udiv/urem in each DivRemGroup in a DivRemChain
void DivRemChain::deleteDeadDivRems() const {
  // Start from index 1, since the 0-index udiv/urem are never optimized away
  for (unsigned i = 1; i < Chain.size(); i++) {
    Chain[i]->deleteDeadDivRems();
  }
}

std::pair<Value *, APInt> IntDivRemIncrementReductionImpl::getBaseAndOffset(Value *V) {
  IGC_ASSERT(V->getType()->isIntegerTy());
  unsigned bitWidth = V->getType()->getIntegerBitWidth();
  if (auto *I = dyn_cast<BinaryOperator>(V)) {
    if (IGC_IS_FLAG_ENABLED(GuardDivRemIncrementDividendOverflow) && !I->hasNoUnsignedWrap()) {
      // Optimization technically unsafe to apply for offset without nuw flag, but in practice wrapped dividends from
      // add/sub rarely used and/or expected to be used and produce a quotient that is of meaningful value.
      // If this happens in practice, optimization will produce the numerically correct quotient and remainder for
      // overflow and UB for underflow, as opposed to an numerically incorrect quotient and remainder for overflow (from
      // wrapping back to 0 and doing udiv/urem) and a very large quotient and/or remainder from underflow (from
      // wrapping to UINT_MAX),
      // If flag is enabled, do not optimize if user expected to perform udiv on wrapped value
      return {V, APInt::getZero(bitWidth)};
    }
    LLVM_DEBUG(dbgs() << "Checking base and offset for inst: " << *I << "\n");
    auto *c0 = dyn_cast<ConstantInt>(I->getOperand(0));
    auto *c1 = dyn_cast<ConstantInt>(I->getOperand(1));
    // Return {%base, %offset} when V is of the form:
    // %V = add i32 %base, %offset
    // %V = or i32 %base, %offset (with no common bits set between both operands; guarantees equivalency to add)
    // %V = sub i32 %base, %offset (returns {%base, -%offset})
    // Otherwise return {%V, nullptr}
    if (((c0 && !c1) || (!c0 && c1))) {
      // only one operand is a constant
      if (I->getOpcode() == Instruction::Add || // ADD inst
          (I->getOpcode() == Instruction::Or && // OR inst with no common bits set between both operands
           haveNoCommonBitsSet(I->getOperand(0), I->getOperand(1), I->getFunction()->getParent()->getDataLayout(),
                               nullptr, I, DT))) {
        if (c0)
          return {I->getOperand(1), c0->getValue()};
        else
          return {I->getOperand(0), c1->getValue()};
      } else if (I->getOpcode() == Instruction::Sub) {
        // SUB inst, only consider when subtracting a constant from a base
        if (c1)
          return {I->getOperand(0), -c1->getValue()};
      }
    }
  }
  return {V, APInt::getZero(bitWidth)};
}

Instruction *IntDivRemIncrementReductionImpl::getRemForDiv(Instruction *div) {
  // Find corresponding urem for a udiv instruction
  // Just return the first one, multiple urem with same dividend/divisor should have been CSE'ed away
  for (auto *user : div->getOperand(0)->users()) {
    if (cast<Instruction>(user)->getParent() != div->getParent())
      continue; // only consider urem in the same basic block for simplicity

    if (auto *inst = dyn_cast<Instruction>(user)) {
      if (inst->getOpcode() == Instruction::URem && inst->getOperand(0) == div->getOperand(0) &&
          inst->getOperand(1) == div->getOperand(1)) {
        return inst;
      }
    }
  }
  return nullptr;
}

bool IntDivRemIncrementReductionImpl::compareDivRemGroups(std::unique_ptr<DivRemGroup> &group1,
                                                          SmallVector<std::unique_ptr<DivRemPair>> &group2) {
  if (group1->DivRems.size() != group2.size()) {
    LLVM_DEBUG(dbgs() << "Group size mismatch, one group may have deeper nested DivRemPairs\n");
  }
  // Care more about matching divisors to existing chain than matching depth, depth will get trimmed later
  for (unsigned i = 0; i < std::min(group1->DivRems.size(), group2.size()); i++) {
    // Divisors must match
    if (group1->DivRems[i]->getDivisor() != group2[i]->getDivisor())
      return false;
    // chainPrevDivRem must dominate new DivRemPair's insert point, to ensure availability of chainPrevDivRem's results
    if (!DT->dominates(group1->DivRems[i]->getQuotient(), group2[i]->simpleInsertPt) ||
        !DT->dominates(group1->DivRems[i]->getRemainder(), group2[i]->simpleInsertPt))
      return false;
  }
  return true;
}

void IntDivRemIncrementReductionImpl::divisorIsZeroCSE(DivRemChain *divRemChain, Function &F, DominatorTree *DT) const {
  if (divRemChain->Chain.size() < 2)
    return;
  DT->recalculate(F); // Update dominator tree after potential CFG changes
  for (unsigned i = 0; i < divRemChain->Chain.front()->DivRems.size(); i++) {
    // First DivRemGroup in chain is unoptimized so DivisorIsZeroTest is nullptr,use the second DivRemGroup's
    // DivisorIsZeroTest as the canonical one
    auto *divisorIsZeroTest = divRemChain->Chain[1]->DivRems[i]->DivisorIsZeroTest;
    if (divisorIsZeroTest) {
      LLVM_DEBUG(dbgs() << "Using " << *divisorIsZeroTest << " as canonical DivisorIsZeroTest for DivRemPair index "
                        << i << " in DivRemChain\n");
      for (unsigned j = 2; j < divRemChain->Chain.size(); j++) {
        if (divRemChain->Chain[j]->DivRems[i]->DivisorIsZeroTest) {
          if (!DT->dominates(divisorIsZeroTest, divRemChain->Chain[j]->DivRems[i]->DivisorIsZeroTest)) {
            // May happen from branching optimization, previous divisorIsZeroTest is on simple branch, need to hoist to
            // common parent block
            auto *insertBlock = DT->findNearestCommonDominator(
                divisorIsZeroTest->getParent(), divRemChain->Chain[j]->DivRems[i]->DivisorIsZeroTest->getParent());
            divisorIsZeroTest->moveBefore(insertBlock->getTerminator());
          }
          // Replace with prior ICmpInst
          divRemChain->Chain[j]->DivRems[i]->DivisorIsZeroTest->replaceAllUsesWith(divisorIsZeroTest);
          // Erase redundant ICmpInst
          divRemChain->Chain[j]->DivRems[i]->DivisorIsZeroTest->eraseFromParent();
        }
      }
    }
  }
}

bool IntDivRemIncrementReductionImpl::run(Function &F) {
  LLVM_DEBUG(dbgs() << "IntDivRemIncrementReduction on " << F.getName() << "\n");
  bool Changed = false;

  // Use worklist to gather initial udiv/urem instructions
  // Do not want to keep iterating over any udiv/urem instructions moved around by this optimization, which would happen
  // if InstVisitor was used
  SmallVector<Instruction *> InstWorklist;
  for (inst_iterator it = inst_begin(&F), eit = inst_end(&F); it != eit; it++) {
    // TODO: Handle sdiv and srem, but it may not be easy to reason the simplified form compared to udiv/urem
    //       since negative divisors and negative dividends need to be handled
    if (it->getOpcode() == Instruction::UDiv) {
      // Use UDiv as start of group, find matching URem later
      InstWorklist.push_back(&*it);
    }
  }

  SmallVector<std::unique_ptr<DivRemChain>> Worklist;
  SmallPtrSet<Instruction *, 16> Visited;

  for (auto *udiv : InstWorklist) {
    // skip if already visited, as it is now a part of a prior created group
    if (Visited.count(udiv))
      continue;

    auto *divIt = udiv;
    // Gather DivRemPairs participating in new group
    SmallVector<std::unique_ptr<DivRemPair>> group;
    while (divIt) {
      LLVM_DEBUG(dbgs() << "Candidate: " << *divIt << "\n");
      auto *remIt = getRemForDiv(divIt);
      if (!remIt) {
        // Did not find corresponding urem, break out of group gathering
        break;
      }

      LLVM_DEBUG(dbgs() << "Adding DivRemPair:\n" << *divIt << "\n" << *remIt << "\n");
      group.push_back(std::move(std::make_unique<DivRemPair>(divIt, remIt)));
      Visited.insert(divIt);

      // find next candidate
      Instruction *candidate = nullptr;
      for (auto *user : divIt->users()) {
        if (auto *inst = dyn_cast<Instruction>(user)) {
          if (inst->getOpcode() == Instruction::UDiv && inst->getOperand(0) == divIt) {
            // Note: Currently does not factor in tree structures in DivRemGroups
            // DivRemPairs in DivRemGroups grouped to a DivRemChain may not be currently maximally optimizable if tree
            // structures exist
            // Fix by implementing tree structure or iterative algorithm
            // ex.
            // %a = udiv i32 %w, %x <-DivRemGroup 1, mapDividend: %w, mapDivisor: %x
            // %b = urem i32 %w, %x <-1
            // %c = udiv i32 %a, %y <-1
            // %d = urem i32 %a, %y <-1
            // %w1 = add i32 %w, 1
            // %e = udiv i32 %w1, %x <-2a possible/correct DivRemGroup 2a, mapDividend: %w, mapDivisor: %x
            // %f = urem i32 %w1, %x   2b current/incorrect DivRemGroup 2b, mapDividend: %w, mapDivisor: %x
            // currently DivRemGroup2 is group 2b because last candidate overwrites previous candidates
            // second DivRemPair across chain will be trimmed later because of mismatched divisor, giving up some perf
            // this pattern has not occurred in practice, but if seen, can be fixed by implementing tree
            // structure/search
            // %g = udiv i32 %e, %y <-2a
            // %h = urem i32 %e, %y <-2a
            // %i = udiv i32 %e, %z <-2b
            // %j = urem i32 %e, %z <-2b
            if (candidate != nullptr) {
              LLVM_DEBUG(dbgs() << "Multiple candidates found, overwriting last candidate, may want to implement tree "
                                   "structure for DivRemPair");
            }
            candidate = inst;
          }
        }
      }

      divIt = candidate;
    }

    if (group.empty())
      continue;

    // Break out baseDividend and offset to find whether there is an existing DivRemChain to insert the new DivRemGroup
    // into, or create a new DivRemChain to insert the new DivRemGroup
    auto baseAndOffset = getBaseAndOffset(group.front()->getDividend());
    Value *baseDividend = baseAndOffset.first;
    APInt offset = baseAndOffset.second;
    while (baseAndOffset.second != 0) {
      baseAndOffset = getBaseAndOffset(baseAndOffset.first);
      baseDividend = baseAndOffset.first;
      offset += baseAndOffset.second;
    }
    LLVM_DEBUG(dbgs() << "Base Dividend: " << *baseDividend << "\nOffset: " << offset << "\n");
    // Have the DivRemGroup own the memory for each DivRemPair
    auto divRemGroup = std::make_unique<DivRemGroup>(baseDividend, offset, std::move(group));

    bool matchingDivRemChain = false;
    for (auto &chain : Worklist) {
      if (chain->getBaseDividend() == baseDividend && compareDivRemGroups(chain->Chain.back(), divRemGroup->DivRems)) {
        matchingDivRemChain = true;
        LLVM_DEBUG(dbgs() << "Adding new group to chain\n");
        chain->addDivRemGroup(std::move(divRemGroup));
        break;
      }
    }

    if (!matchingDivRemChain) {
      // Start a new chain if no match was found
      LLVM_DEBUG(dbgs() << "Starting new chain with group\n");
      // Have the DivRemChain own the memory for each DivRemGroup
      auto divRemChain = std::make_unique<DivRemChain>(std::move(divRemGroup));
      Worklist.push_back(std::move(divRemChain));
    }
  }

  for (unsigned i = 0; i < Worklist.size(); i++) {
    auto chain = Worklist[i].get();

    LLVM_DEBUG(dbgs() << "Working on chain with base: " << *chain->getBaseDividend() << "\n");
    // Process each chain by trimming, simplifying, and deleting dead div/rem and doing CSE on divisor is zero tests
    chain->trim();
    if (chain->Chain.size() < 2) {
      // If chain only has 1 DivRemGroup, that is the base and there are no subsequent DivRemGroups to optimize using
      // the result of the base calculation
      continue;
    }

    chain->simplify();
    Changed = true;

    // Depending on where this pass is run, CSE and DCE may not be run after, so perform relevant cleanup here
    chain->deleteDeadDivRems();
    if (IGC_IS_FLAG_ENABLED(SanitizeDivRemIncrementDivisorIsZero)) {
      divisorIsZeroCSE(chain, F, DT);
    }
  }

  Worklist.clear();
  return Changed;
}

class IntDivRemIncrementReduction : public FunctionPass {
public:
  static char ID;

  IntDivRemIncrementReduction() : FunctionPass(ID) {}
  virtual bool runOnFunction(Function &F) override {
    auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    IntDivRemIncrementReductionImpl IDRIR(&DT);
    return IDRIR.run(F);
  }

  virtual StringRef getPassName() const override { return "IntDivRemIncrementReductionPass"; }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override { AU.addRequired<DominatorTreeWrapperPass>(); }
};

FunctionPass *createIntDivRemIncrementReductionPass() { return new IntDivRemIncrementReduction(); }
} // end namespace IGC

char IntDivRemIncrementReduction::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(IntDivRemIncrementReduction, DEBUG_TYPE,
                          "Optimize consecutive div/rem instructions that increment dividend by constant lesser than "
                          "divisor with the same divisor",
                          false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(IntDivRemIncrementReduction, DEBUG_TYPE,
                        "Optimize consecutive div/rem instructions that increment dividend by constant lesser than "
                        "divisor with the same divisor",
                        false, false)