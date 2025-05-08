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
// ex.
// %c = udiv i32 %a, %b
// %d = urem i32 %a, %b
struct DivRemPair {
  Instruction* Div = nullptr;
  Instruction* Rem = nullptr;
  Value* newDiv = nullptr;
  Value* newRem = nullptr;
  Instruction* simpleInsertPt = nullptr;
  Instruction* normalInsertPt = nullptr;
  ICmpInst* DivIsChangingTest = nullptr;

  DivRemPair(Instruction* div, Instruction* rem) : Div(div), Rem(rem), simpleInsertPt(Div) {
    IGC_ASSERT_MESSAGE(div->getOpcode() == Instruction::UDiv && rem->getOpcode() == Instruction::URem,
                       "Invalid DivRem Pair, non-matching instruction types and/or signed/unsigned types");
    IGC_ASSERT_MESSAGE(div->getOperand(0) == rem->getOperand(0), "Invalid DivRem Pair, non-matching dividend");
    IGC_ASSERT_MESSAGE(div->getOperand(1) == rem->getOperand(1), "Invalid DivRem Pair, non-matching divisor");
  }

  // Helpers for getting various values
  Value* getDividend() const { return Div->getOperand(0); }
  Value* getDivisor() const { return Div->getOperand(1); }
  Value* getQuotient() const { return newDiv ? newDiv : Div; }
  Value* getRemainder() const { return newRem ? newRem : Rem; }

  // Returns whether the optimization created a conditional branch or an unconditional branch
  bool isSimple() const {
    IGC_ASSERT(newDiv); // Should only be calling isSimple on a DivRemPair that has been simplified
    return !isa<PHINode>(newDiv);
  }

  // If div/rem pair has been optimized using the simple (no branches) method, then the original udiv/urem are dead, and can be deleted
  void deleteDeadDivRem() const {
    if (newDiv && !isa<PHINode>(newDiv)) {
      Div->eraseFromParent();
      Rem->eraseFromParent();
    }
  }

  // Specialized simplify method taking advantage of current dividend being an increment by a constant of a previous udiv/urem's dividend
  // Also can simplify a nested div/rem pair (parent div quotient used as dividend, increases by 1 or remains the same)
  void simplify(const DivRemGroup* chainPrevDivRemGroup, const DivRemGroup* divRemGroup, ConstantInt* offset, unsigned int idx);
};

// DivRemGroup, group of udiv/urem that can participate in a single trickle down optimization
// ex.
// %c = udiv i32 %a, %b
// %d = urem i32 %a, %b
// %f = udiv i32 %c, %e
// %g = urem i32 %c, %e
//
// TODO: Optimizations where the nested udiv/urem operate on the previous remainder instead of the previous quotient may also be possible
// ex.
// %c = udiv i32 %a, %b
// %d = urem i32 %a, %b
// %f = udiv i32 %d, %e
// %g = urem i32 %d, %e
//
// Real world examples: (x,y,z) dimensions, laid out in 1-D array.
// IMPLEMENTED - nested div/rem operate on quotient
// z.quo = in // (x.dim * y.dim)
// z = in % (x.dim * y.dim)
// x = z.quo // x.dim
// y = z.quo % x.dim
// VS
// NOT IMPLEMENTED YET - nested div/rem operate on remainder
// x = in // (y.dim * z.dim)
// x.rem = in % (y.dim * z.dim)
// y = x.rem // z.dim
// z = x.rem % z.dim
struct DivRemGroup {
  Value* Base = nullptr;// Base, equal to %a if this DivRemGroup is considered the base, or %x if %a = add i32 %x, C if this DivRemGroup is not the base
  ConstantInt* Offset = nullptr; // Offset equal to nullptr if this DivRemGroup is considered the base, or  C if %a = add i32 %x, C if this DivRemGroup is not the base
  SmallVector<std::unique_ptr<DivRemPair>> DivRems;

  DivRemGroup(Value* base, ConstantInt* offset, SmallVector<std::unique_ptr<DivRemPair>> divRems) : Base(base), Offset(offset), DivRems(std::move(divRems)) {}

  void simplify(const DivRemGroup* chainPrevDivRemGroup) const;

  void deleteDeadDivRems() const {
    for (unsigned i = 0; i < DivRems.size(); i++) {
      DivRems[i]->deleteDeadDivRem();
    }
  }
};

struct DivRemChain {
  SmallVector<std::unique_ptr<DivRemGroup>> Chain;

  DivRemChain(std::unique_ptr<DivRemGroup> group) {
    Chain.push_back(std::move(group));
  }

  // Helpers for accessing values used for map keys
  Value* getBaseDividend() const { return Chain.front()->Base; }
  Value* getBaseDivisor() const { return Chain.front()->DivRems.front()->getDivisor(); }

  void addDivRemGroup(std::unique_ptr<DivRemGroup> divRemGroup) {
    Chain.push_back(std::move(divRemGroup));
  }

  // Trim uneven depths of all DivRemGroups in the chain so that all of the groups have the same depth
  void trim() const;
  // Sort the DivRemGroups in the chain by increasing constant Offset from the Base group (with nullptr Offset)
  void sort();
  // Simplify the DivRemPairs in a chain
  void simplify() const;
  // Delete dead udiv/urem instructions that were replaced with optimized instructions
  void deleteDeadDivRems() const;
};

class IntDivRemIncrementReductionImpl {
public:
  IntDivRemIncrementReductionImpl(DominatorTree *DT) : DT(DT) {}
  bool run(Function& F);

private:
  DominatorTree* DT;
  std::pair<Value*, ConstantInt*> getBaseAndOffset(Value* dividend);
};

void DivRemPair::simplify(const DivRemGroup* chainPrevDivRemGroup, const DivRemGroup* divRemGroup, ConstantInt* offset, unsigned int idx) {
  auto* chainPrevDivRem = chainPrevDivRemGroup->DivRems[idx].get();
  auto* groupPrevDivRem = idx == 0 ? nullptr : divRemGroup->DivRems[idx-1].get();
  IGC_ASSERT(chainPrevDivRem->getDivisor() == getDivisor()); // Required
  IGC_ASSERT(!offset->isNegative()); // TODO: Handle negative offsets (probably similar to checking if

  // %prevDividend = ...
  // %sameDivisor = ...
  // %sameDivisor2 = ...
  // %prevQuo = [udiv|simplified form] i32 %prevDividend, %sameDivisor < chainPrevDivRemGroup, idx-1
  // %prevRem = [urem|simplified form] i32 %prevDividend, %sameDivisor < chainPrevDivRemGroup, idx-1
  // %prevNestedQuo = [udiv|simplified form] i32 %prevQuo, %sameDivisor2 < chainPrevDivRemGroup, idx
  // %prevNestedRem = [urem|simplified form] i32 %prevQuo, %sameDivisor2 < chainPrevDivRemGroup, idx
  // %dividend = add i32 %prevDividend, offset (constant int)
  // %quo = udiv i32 %dividend, %sameDivisor < divRemGroup, idx-1
  // %rem = urem i32 %dividend, %sameDivisor < divRemGroup. idx-1
  // %nestedQuo = udiv i32 %quo, %sameDivisor2 < divRemGroup, idx
  // %nestedRem = urem i32 %quo, %sameDivisor2 < divRemGroup, idx

  // Insert point for simple optimization described below
  simpleInsertPt = groupPrevDivRem ? (!groupPrevDivRem->isSimple() ? groupPrevDivRem->simpleInsertPt : simpleInsertPt) : simpleInsertPt;
  IRBuilder<> builder(simpleInsertPt);

  auto noOverrideDivRemInGroup = [&](Use& U) {
    for (unsigned i = 0; i < divRemGroup->DivRems.size(); i++) {
      if (U.getUser() == divRemGroup->DivRems[i]->Div ||
          U.getUser() == divRemGroup->DivRems[i]->Rem)
        return false;
    }
    return true;
  };

  if (offset->isOne()) {
    // For a top-level DivRemPair (first DivRemPair in a DivRemGroup in a chain), simple opt if the offset from the previous DivRemGroup is 1
    // %quo is either:
    //  1. %prevQuo if %prevRem + 1 does not add up to %sameDivisor
    //  2. %prevQuo + 1 if %prevRem + 1 does add up to %sameDivisor
    // %rem is either:
    //  1. %prevRem + 1 if %prevRem + 1 does not add up to %sameDivisor
    //  2. 0 if %prevRem + 1 does add up to %sameDivisor (since %quo would increment by 1)
    //
    // For a nested DivRemPair (non-first DivRemPair in a DivRemGroup in a chain), the offset for continuing the simple opt portion of the prior DivRemPair in the DivRemGroup is 1 (guaranteed because of later checks)
    // %nestedQuo is either:
    //  1. %prevNestedQuo if %quo is equal to %prevQuo (option 1 of prior DivRemGroup
    //  2. %prevNestedQuo if %quo is equal to %prevQuo + 1 but %prevNestedRem + 1 does not add up to %sameDivisor2
    //  3. %prevNestedQuo + 1 if %quo is equal to %prevQuo + 1 and %prevNestedRem + 1 does add up to %sameDivisor2
    // %nestedRem is either:
    //  1. %prevNestedRem if %quo is equal to %prevQuo (option 1 of prior DivRemGroup)
    //  2. %prevNestedRem + 1 if %quo is equal to %prevQuo + 1 but %prevNestedRem + 1 does not add up to %sameDivisor2
    //  3. 0 if %quo is equal to %prevQuo + 1 and %prevNestedRem + 1 does add up to %sameDivisor2

    // pre-increment remainder (by 1):
    auto* preIncRem = builder.CreateAdd(chainPrevDivRem->getRemainder(), offset, "pre.inc." + chainPrevDivRem->getRemainder()->getName());
    // check if previous remainder + 1 results in 1 more time that %dividend goes into %sameDivisor
    DivIsChangingTest = cast<ICmpInst>(builder.CreateICmp(ICmpInst::ICMP_EQ, preIncRem, getDivisor(), "cmp." + preIncRem->getName()));
    // pre-increment quotient by 1
    auto* preIncDiv = builder.CreateAdd(chainPrevDivRem->getQuotient(), offset, "pre.inc." + chainPrevDivRem->getQuotient()->getName());

    // form new quotient
    newDiv = builder.CreateSelect(DivIsChangingTest, preIncDiv, chainPrevDivRem->getQuotient(), "new.div." + getQuotient()->getName());
    // form new remainder
    newRem = builder.CreateSelect(DivIsChangingTest, builder.getInt32(0), preIncRem, "new.rem." + getRemainder()->getName());

    if (!groupPrevDivRem) {
      // first udiv/urem in group, uncond optimization, no extra logic needed
      Div->replaceAllUsesWith(newDiv);
      Rem->replaceAllUsesWith(newRem);
      return;
    } else {
      // create the merging select statements for a three-way merge for a nested DivRemPair
      // can AND prevGroupDivRem DivIsChanging test with current DivIsChanging test to avoid additional select inst for div
      // unavoidable for rem since 3 different possible values, compared to 2 for div (see above numbering for nested DivRemPair)
      builder.SetInsertPoint(cast<Instruction>(newDiv));
      auto* andDivIsChangingTest = builder.CreateAnd(DivIsChangingTest, groupPrevDivRem->DivIsChangingTest, "use.new.div.");
      builder.SetInsertPoint(simpleInsertPt);
      cast<SelectInst>(newDiv)->setCondition(andDivIsChangingTest);
      newRem = builder.CreateSelect(groupPrevDivRem->DivIsChangingTest, newRem, chainPrevDivRem->getRemainder(), "merge." + getRemainder()->getName());

      if (groupPrevDivRem->isSimple()) {
        // Previous udiv/urem had uncond optimization done, no extra logic needed
        Div->replaceAllUsesWith(newDiv);
        Rem->replaceAllUsesWith(newRem);
        return;
      } else {
        // Previous udiv/urem had cond optimization done, set normalInsertPt and continue to common PHINode creation below
        normalInsertPt = groupPrevDivRem->normalInsertPt;
      }
    }
  } else {
    // Constant > 1, need to add ICmp to test whether offset is lesser than or equal to divisor (will be a runtime check)
    // If so, can still do a simplified calculation, otherwise fallback to normal div/rem (will be replaced with precompiled func later)
    // This will create a conditional branch that replaces the udiv/urem to allow for the simple path to be taken at runtime
    // Since the normal path has conditional branches anyways, it is assumed that this is still preferred
    // However, this handling can be restricted by the DivRemIncrementCondBranchSimplify flag, which will make all groups with offset > 1 go unoptimized
    Value* divisorTest = builder.CreateICmp(ICmpInst::ICMP_ULE, offset, getDivisor());
    Instruction* simpleDivRem = nullptr; // then block, unconditional branch inst to join block
    Instruction* normalDivRem = nullptr; // else block, unconditional branch inst to join block

    // Create if and else block. If block will contain simplified udiv/urem based on previous result, else block will contain udiv/urem unoptimized insts
    SplitBlockAndInsertIfThenElse(divisorTest, Div, &simpleDivRem, &normalDivRem);
    simpleDivRem->getParent()->setName("simple.div.rem");
    simpleInsertPt = simpleDivRem;
    normalDivRem->getParent()->setName("normal.div.rem");
    normalInsertPt = normalDivRem;

    // Simple Div/Rem for constant > 1
    builder.SetInsertPoint(simpleInsertPt);

    // pre-increment remainder
    auto* preIncRem = builder.CreateAdd(chainPrevDivRem->getRemainder(), offset, "pre.inc." + chainPrevDivRem->getRemainder()->getName());
    // check if previous remainder + offset results in 1 more than that %dividend goes into %sameDivisor
    DivIsChangingTest = cast<ICmpInst>(builder.CreateICmp(ICmpInst::ICMP_UGE, preIncRem, getDivisor(), "cmp." + preIncRem->getName()));
    // pre-increment quotient by 1
    auto* preIncDiv = builder.CreateAdd(chainPrevDivRem->getQuotient(), builder.getInt32(1), "pre.inc." + chainPrevDivRem->getQuotient()->getName());
    // pre-calculate new remainder if preIncRemTest is true, since that indicates that new dividend divides divisor 1 more time
    auto* preDecRem = builder.CreateSub(preIncRem, getDivisor(), "pre.dec." + chainPrevDivRem->getRemainder()->getName());
    // form new quotient
    newDiv = builder.CreateSelect(DivIsChangingTest, preIncDiv, chainPrevDivRem->getQuotient(), "new.div." + getQuotient()->getName());
    // form new remainder
    newRem = builder.CreateSelect(DivIsChangingTest, preDecRem, preIncRem, "new.rem." + getRemainder()->getName());
  }

  auto* joinInsertPt = Rem->getNextNode();

  // Either predecessor DivRemPair got transformed to conditional and need to retain udiv and urem in the normal branch,
  // Or current DivRemPair is undergoing conditional transformation, and need to create join PHINodes and retain udiv and urem in the normal branch
  // create phis for this udiv/urem pair at joinInsertPt, join the simple and normal results
  auto* joinDiv = PHINode::Create(Div->getType(), 2, "join.div.", joinInsertPt);
  Div->replaceUsesWithIf(joinDiv, noOverrideDivRemInGroup);
  auto* joinRem = PHINode::Create(Rem->getType(), 2, "join.rem.", joinInsertPt);
  Rem->replaceUsesWithIf(joinRem, noOverrideDivRemInGroup);
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

void DivRemGroup::simplify(const DivRemGroup* chainPrevDivRemGroup) const {
  IGC_ASSERT(chainPrevDivRemGroup->Base == Base);
  ConstantInt* TrueOffset = nullptr;

  // TrueOffset calculates the offset between this group and the previous group in the chain,
  // since Offset is the offset of each group from the first group in the chain (that has Offset == nullptr)
  if (chainPrevDivRemGroup->Offset) {
    TrueOffset = cast<ConstantInt>(ConstantInt::get(Base->getType(), Offset->getZExtValue() - chainPrevDivRemGroup->Offset->getZExtValue()));
  } else {
    TrueOffset = Offset;
  }
  LLVM_DEBUG(dbgs() << "True offset: " << *TrueOffset << "\n");

  if (!TrueOffset->isOne() && IGC_IS_FLAG_DISABLED(DivRemIncrementCondBranchSimplify)) {
    // Flag to guard conditional branch creation for optimization disabled, do not optimize this DivRemGroup
    // The subsequent DivRemGroup (if it exists and the true offset is 1 from this group) will use the non-optimized udiv/urem results of the current udiv/urem group in its optimizations
    return;
  }

  // First DivRemGroup, call simplify with TrueOffset
  DivRems.front()->simplify(chainPrevDivRemGroup, this, TrueOffset, 0);
  for (unsigned i = 1; i < DivRems.size(); i++) {
    // Nested div/rems in a group, if simplified, will only ever have to deal with the dividend
    // that is passed down from the previous div/rem in the group potentially increasing by 1
    DivRems[i]->simplify(chainPrevDivRemGroup, this, cast<ConstantInt>(ConstantInt::get(TrueOffset->getType(), 1)), i);
  }
}

// Trim the number of DivRemPairs in each DivRemGroup in this DivRemChain in case they are uneven (in terms of number of DivRemPairs in each DivRemGroup)
void DivRemChain::trim() const {
  unsigned minDepth = Chain.front()->DivRems.size();
  for (auto& DRG : Chain) {
    if (DRG->DivRems.size() < minDepth) {
      minDepth = DRG->DivRems.size();
    }
  }

  for (auto& DRG : Chain) {
    while (DRG->DivRems.size() > minDepth) {
      DRG->DivRems.pop_back();
    }
  }
}

// Sort the DivRemGroups in a DivRemChain in order by increasing offset
void DivRemChain::sort() {
  std::sort(Chain.begin(), Chain.end(), [](const std::unique_ptr<DivRemGroup>& divRemGroup1, const std::unique_ptr<DivRemGroup>& divRemGroup2) {
    if (!divRemGroup1->Offset) {
      return true;
    } else if (!divRemGroup2->Offset) {
      return false;
    } else {
      return divRemGroup1->Offset->getSExtValue() < divRemGroup2->Offset->getSExtValue();
    }
  });
}

// Call simplify on each DivRemGroup in a DivRemChain
void DivRemChain::simplify() const {
  auto* prevChainDivRemGroup = Chain.front().get();
  // Start from index 1, since the 0-index udiv/urem is kept as baseline full calculation
  for (unsigned i = 1; i < Chain.size(); i++) {
    Chain[i]->simplify(prevChainDivRemGroup);
    prevChainDivRemGroup = Chain[i].get();
  }
}

// Delete dead udiv/urem in each DivRemGroup in a DivRemChain
void DivRemChain::deleteDeadDivRems() const {
  for (unsigned i = 0; i < Chain.size(); i++) {
    Chain[i]->deleteDeadDivRems();
  }
}

// Simple check: Check if dividend is add inst with a constant ,or OR with a constant
//               InstCombine will change some "add i32 x, imm" to "or i32 x, imm" if some prior shl pattern is present
//               Can add more cases later (like sub with a constant), particularly if running after inst combine.
// This also does not check for indirect adds, and will return the incorrect base and offset
// This is intentional, to avoid polluting this pass with checking/performing optimizations that are usually handled by other passes such as InstCombine, EarlyCSE, etc.
// ex.
//    %a = udiv i32 %w, %x
//    %b = urem i32 %w, %x
//    %w1 = add i32 %w, 1 <- direct add of %w
//    %c = udiv i32 %w1, %x
//    %d = urem i32 %w1, %x
//    %w2 = add i32 %w, 2 <- direct add of %w
//    %e = udiv i32 %w2, %x
//    %f = urem i32 %w2, %x
//    %w3 = add i32 %w2, 1 <- indirect add of %w, could be merged with DivRemChain, but will not be
//    %g = udiv i32 %w3, %x
//    %h = urem i32 %w3, %x
std::pair<Value*, ConstantInt*> IntDivRemIncrementReductionImpl::getBaseAndOffset(Value* dividend) {
  if (auto* dividendInst = dyn_cast<BinaryOperator>(dividend)) {
    LLVM_DEBUG(dbgs() << "Checking base and offset for inst: " << *dividendInst << "\n");
    auto* c0 = dyn_cast<ConstantInt>(dividendInst->getOperand(0));
    auto* c1 = dyn_cast<ConstantInt>(dividendInst->getOperand(1));
    // Treat following patterns of: %dividend = OP %base, %offset -> Base = %base, Offset = %offset
    // Treat anything else as Base = %dividend, Offset = nullptr
    if (((c0 && !c0->isNegative() && !c1) || (!c0 && c1 && !c1->isNegative())) && // only one operand is a non negative constant
        (dividendInst->getOpcode() == Instruction::Add || // ADD inst
        (dividendInst->getOpcode() == Instruction::Or && // OR inst with no common bits set between both operands
         haveNoCommonBitsSet(dividendInst->getOperand(0), dividendInst->getOperand(1), dividendInst->getFunction()->getParent()->getDataLayout(), nullptr, dividendInst, DT)))) {
      if (c0)
        return { dividendInst->getOperand(1), c0 };
      else
        return { dividendInst->getOperand(0), c1 };
    } else {
      return { dividend, nullptr };
    }
  } else {
    return { dividend, nullptr };
  }
}

bool IntDivRemIncrementReductionImpl::run(Function& F) {
  LLVM_DEBUG(dbgs() << "IntDivRemIncrementReduction on " << F.getName() << "\n");
  bool Changed = false;

  DenseMap<Value*, DenseMap<Value*, std::unique_ptr<DivRemChain>>> DividendToDivisorToDivRemChainMap;

  // Use worklist to gather initial udiv/urem instructions
  // Do not want to keep iterating over any udiv/urem instructions moved around by this optimization, which would happen if InstVisitor was used
  SmallVector<Instruction*> InstWorklist;
  for (inst_iterator it = inst_begin(&F), eit = inst_end(&F); it != eit; it++) {
    // TODO: Handle sdiv and srem, but it may not be easy to reason the simplified form compared to udiv/urem
    //       since negative constant increments, negative divisors, negative dividends need to be handled
    if (it->getOpcode() == Instruction::UDiv) {
      // Use UDiv as start of group, check for consecutive URem
      // TODO: Handle non-consecutive UDiv/URem pairs only if needed
      if (it->getNextNode()->getOpcode() == Instruction::URem) {
        LLVM_DEBUG(dbgs() << "Found a udiv/urem pair: " << *it << "\n");
        InstWorklist.push_back(&*it);
      }
    }
  }

  SmallVector<DivRemChain*> Worklist;
  SmallPtrSet<Instruction*, 16> Visited;

  for (auto* udiv : InstWorklist) {
    // skip if already visited, as it is now a part of a prior created group
    if (Visited.count(udiv))
      continue;

    auto* divIt = udiv;
    // Gather DivRemPairs participating in new group
    SmallVector<std::unique_ptr<DivRemPair>> group;
    while (divIt) {
      LLVM_DEBUG(dbgs() << "Candidate: " << *divIt << "\n");
      auto* remIt = divIt->getNextNode();
      if (divIt->getOpcode() != Instruction::UDiv || remIt->getOpcode() != Instruction::URem ||
          divIt->getOperand(0) != remIt->getOperand(0) ||
          divIt->getOperand(1) != remIt->getOperand(1)) {
        // udiv and urem need to have the same dividend and divisor
        break;
      }

      LLVM_DEBUG(dbgs() << "Adding DivRemPair:\n" << *divIt << "\n" << *remIt << "\n");
      group.push_back(std::move(std::make_unique<DivRemPair>(divIt, remIt)));
      Visited.insert(divIt);

      // find next candidate
      Instruction* candidate = nullptr;
      for (auto* user : divIt->users()) {
        if (auto* inst = dyn_cast<Instruction>(user)) {
          if (inst->getOpcode() == Instruction::UDiv && inst->getOperand(0) == divIt) {
            if (candidate != nullptr) {
              LLVM_DEBUG(dbgs() << "Multiple candidates found, overwriting last candidate, may want to implement tree structure for DivRemPair");
            }
            candidate = inst;
          }
        }
      }

      divIt = candidate;
    }

    if (group.empty())
      continue;

    // Break out baseDividend and offset to find whether there is an existing DivRemChain to insert the new DivRemGroup into,
    // or create a new DivRemChain to insert the new DivRemGroup
    auto baseAndOffset = getBaseAndOffset(group.front()->getDividend());
    auto* baseDividend = baseAndOffset.first;
    auto* offset = baseAndOffset.second;

    LLVM_DEBUG(dbgs() << "Base Dividend: " << *baseDividend << "\n");
    if (offset)
      LLVM_DEBUG(dbgs() << "Offset: " << *offset << "\n");
    if (!offset || // Complex dividend or constant + constant
        !DividendToDivisorToDivRemChainMap.count(baseDividend) || // No previous chain started from baseDividend
        !DividendToDivisorToDivRemChainMap[baseDividend].count(group.front()->getDivisor())) { // No previous chain started from baseDividend with same divisor
      // Note: If statement does not factor in tree structures in udiv/urem groups
      //       DivRems in DivRemGroups grouped to a DivRemChain may not be currently maximally optimizable if tree structures exist
      //       Fix by implementing tree structure or iterative algorithm
      //       ex.
      //          %a = udiv i32 %w, %x < DivRemGroup 1, mapDividend: %w, mapDivisor: %x
      //          %b = urem i32 %w, %x <
      //          %c = udiv i32 %a, %y <
      //          %d = urem i32 %a, %y <
      //          %w1 = add i32 %w, 1
      //          %e = udiv i32 %w1, %x <ab DivRemGroup 2a, mapDividend: %w, mapDivisor: %x
      //          %f = urem i32 %w1, %x <ab DivRemGroup 2b, mapDividend: %w, mapDivisor: %x, second DivRemPair across chain will be trimmed later because of mismatched divisor
      //          %g = udiv i32 %e, %y <a
      //          %h = urem i32 %e, %y <a
      //          %i = udiv i32 %e, %z <b
      //          %j = urem i32 %e, %z <b
      // Start new chain with correct base dividend
      LLVM_DEBUG(dbgs() << "Starting new chain with group\n");
      // Have the DivRemGroup own the memory for each DivRemPair
      auto divRemGroup = std::make_unique<DivRemGroup>(group.front()->getDividend(), nullptr, std::move(group));
      // Have the DivRemChain own the memory for each DivRemGroup
      auto divRemChain = std::make_unique<DivRemChain>(std::move(divRemGroup));
      Worklist.push_back(divRemChain.get());
      // Have the map own the memory for each DivRemChain
      DividendToDivisorToDivRemChainMap[divRemChain->getBaseDividend()][divRemChain->getBaseDivisor()] = std::move(divRemChain);
    } else {
      LLVM_DEBUG(dbgs() << "Adding new group to chain\n");
      // Have the DivRemGroup own the memory for each pair
      auto divRemGroup = std::make_unique<DivRemGroup>(baseDividend, offset, std::move(group));
      // Move ownership of DivRemGroup memory to the DivRemChain that is in the map
      DividendToDivisorToDivRemChainMap[baseDividend][divRemGroup->DivRems.front()->getDivisor()]->addDivRemGroup(std::move(divRemGroup));
    }
  }

  while (!Worklist.empty()) {
    auto chain = Worklist.back();
    Worklist.pop_back();

    LLVM_DEBUG(dbgs() << "Working on chain with base: " << *chain->getBaseDividend() << "\n");
    // Process each chain by trimming, sorting, simplifying, and deleting dead div/rem
    chain->trim();
    if (chain->Chain.size() < 2) {
      // If chain only has 1 DivRemGroup, that is the base and there are no subsequent DivRemGroups to optimize using the result of the base calculation
      continue;
    }

    chain->sort();
    chain->simplify();
    Changed = true;
    chain->deleteDeadDivRems();
  }

  DividendToDivisorToDivRemChainMap.clear(); // destroy all unique_ptrs of DivRemChain, DivRemGroup, DivRemPair

  return Changed;
}

class IntDivRemIncrementReduction: public FunctionPass {
public:
  static char ID;

  IntDivRemIncrementReduction() : FunctionPass(ID) {}
  virtual bool runOnFunction(Function& F) override {
    auto& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

    IntDivRemIncrementReductionImpl IDRIR(&DT);
    return IDRIR.run(F);
  }

  virtual StringRef getPassName() const override {
    return "IntDivRemIncrementReductionPass";
  }

  virtual void getAnalysisUsage(AnalysisUsage& AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
  }
};

FunctionPass* createIntDivRemIncrementReductionPass() {
  return new IntDivRemIncrementReduction();
}
} // end namespace IGC

char IntDivRemIncrementReduction::ID = 0;

IGC_INITIALIZE_PASS_BEGIN(IntDivRemIncrementReduction, DEBUG_TYPE,
                          "Optimize consecutive div/rem instructions that increment dividend by constant lesser than divisor with the same divisor", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(IntDivRemIncrementReduction, DEBUG_TYPE,
                        "Optimize consecutive div/rem instructions that increment dividend by constant lesser than divisor with the same divisor", false, false)