/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AllocationLivenessAnalyzer.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/igc_regkeys.hpp"
#include "Probe/Assertion.h"
#include "debug/DebugMacros.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SetOperations.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

AllocationLivenessAnalyzer::LivenessData
AllocationLivenessAnalyzer::ProcessInstruction(Instruction *I, DominatorTree &DT, LoopInfo &LI, bool includeOrigin) {
  // static allocas are usually going to be in the entry block
  // that's a practice, but we only care about the last block that dominates all uses
  BasicBlock *commonDominator = includeOrigin ? I->getParent() : nullptr;
  SetVector<Instruction *> allUsers;
  SetVector<Instruction *> lifetimeLeakingUsers;
  SmallVector<Use *> worklist;

  for (auto &use : I->uses()) {
    auto *UasI = cast<Instruction>(use.getUser());
    if (commonDominator) {
      commonDominator = DT.findNearestCommonDominator(commonDominator, UasI->getParent());
    } else {
      commonDominator = UasI->getParent();
    }

    worklist.push_back(&use);
  }

  auto addUsesFn = [&worklist](auto uses) {
    for (auto &use : uses)
      worklist.push_back(&use);
  };

  // figure out the potential accesses to the memory via GEP and bitcasts
  while (!worklist.empty()) {
    auto *use = worklist.pop_back_val();
    auto *II = cast<Instruction>(use->getUser());

    if (!allUsers.insert(II))
      continue;

    // a possible optimization here:
    // 1. find all reachable blocks
    // 2. cull uses that are not reachable from the allocation

    commonDominator = DT.findNearestCommonDominator(commonDominator, II->getParent());

    switch (II->getOpcode()) {
    case Instruction::PHI:

      for (auto *bb : cast<PHINode>(II)->blocks())
        commonDominator = DT.findNearestCommonDominator(commonDominator, bb);
      [[fallthrough]];
    case Instruction::GetElementPtr:
    case Instruction::BitCast:
    case Instruction::Select:
      addUsesFn(II->uses());
      break;
    case Instruction::PtrToInt:
      lifetimeLeakingUsers.insert(II);
      break;
    case Instruction::Store: {
      auto *storeI = cast<StoreInst>(II);
      if (storeI->getValueOperand() == use->get()) {
        SmallVector<Instruction *> origins;
        if (Provenance::tryFindPointerOrigin(storeI->getPointerOperand(), origins)) {
          for (auto *origin : origins)
            addUsesFn(origin->uses());
        } else {
          lifetimeLeakingUsers.insert(II);
        }
      }
    } break;
    case Instruction::Call:
      implementCallSpecificBehavior(cast<CallInst>(II), use, worklist, allUsers, lifetimeLeakingUsers);
      break;
    case Instruction::Load:
      if (II->getType()->isPointerTy())
        addUsesFn(II->uses());
      break;
    default: // failsafe for handling "unapproved" instructions
      lifetimeLeakingUsers.insert(II);
      break;
    }
  }

  if (includeOrigin)
    allUsers.insert(I);

  auto *F = I->getParent()->getParent();
  if (PerFunctionBBToIndexMap.count(F) == 0) {
    initBBtoIndexMap(*F);
  }
  return {I, std::move(allUsers), LI, DT, PerFunctionBBToIndexMap[F], commonDominator, std::move(lifetimeLeakingUsers)};
}

void AllocationLivenessAnalyzer::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<LoopInfoWrapperPass>();
  getAdditionalAnalysisUsage(AU);
}

void AllocationLivenessAnalyzer::implementCallSpecificBehavior(CallInst *callI, Use *use, SmallVector<Use *> &worklist,
                                                               SetVector<Instruction *> &allUsers,
                                                               SetVector<Instruction *> &lifetimeLeakingUsers) {

  if (!callI->doesNotCapture(use->getOperandNo()))
    lifetimeLeakingUsers.insert(callI);

  if (callI->getType()->isPointerTy()) {

    for (auto &use : callI->uses())
      worklist.push_back(&use);
  }
}

template <typename range, typename SetT>
static inline void doWorkLoop(SmallVectorImpl<BasicBlock *> &worklist, SetT &bbSet1, SetT &bbSet2,
                              std::function<range(BasicBlock *)> iterate,
                              std::function<bool(BasicBlock *)> continueCondition) {
  // perform data flow analysis
  while (!worklist.empty()) {
    auto *currbb = worklist.pop_back_val();

    if (continueCondition(currbb))
      continue;

    bool addToSet1 = false;

    for (auto *pbb : iterate(currbb)) {
      addToSet1 = true;

      bool inserted = bbSet2.insert(pbb);

      if (inserted)
        worklist.push_back(pbb);
    }

    if (addToSet1)
      bbSet1.insert(currbb);
  }
}

AllocationLivenessAnalyzer::LivenessData::LivenessData(Instruction *allocationInstruction,
                                                       SetVector<Instruction *> &&usersOfAllocation, const LoopInfo &LI,
                                                       const DominatorTree &DT, const BBToIndexMapT &bbToIndexMap,
                                                       llvm::BasicBlock *userDominatorBlock,
                                                       SetVector<Instruction *> &&lifetimeLeakingUsers) {
  llvm::SmallSetVector<llvm::BasicBlock *, 16> bbInSet;
  llvm::SmallSetVector<llvm::BasicBlock *, 16> bbOutSet;

  if (!userDominatorBlock)
    userDominatorBlock = allocationInstruction->getParent();

  if (usersOfAllocation.empty()) {

    // a pathological case of no-uses allocation instruction
    IGC_ASSERT_MESSAGE(lifetimeLeakingUsers.empty(), "What?");
    lifetimeStart = LivenessInstruction(allocationInstruction, bbToIndexMap);
    lifetimeEndInstructions.push_back(lifetimeStart);
    return;
  }

  bbOutSet.insert(userDominatorBlock);

  SmallVector<BasicBlock *> worklist;

  for (auto *I : usersOfAllocation) {
    worklist.push_back(I->getParent());
  }

  // Keep track of loop header of blocks that contain allocation instruction
  auto *allocationParent = allocationInstruction->getParent();
  llvm::SmallPtrSet<llvm::BasicBlock *, 4> containedLoopHeaders;
  if (const auto *parentLoop = LI.getLoopFor(allocationParent)) {
    containedLoopHeaders.insert(parentLoop->getHeader());
    while (parentLoop->getParentLoop()) {
      parentLoop = parentLoop->getParentLoop();
      containedLoopHeaders.insert(parentLoop->getHeader());
    }
  }

  // perform data flow analysis
  doWorkLoop<llvm::pred_range>(
      worklist, bbInSet, bbOutSet, [&](auto *currbb) { return llvm::predecessors(currbb); },
      [&](auto *currbb) {
        return bbInSet.contains(currbb) || currbb == userDominatorBlock || containedLoopHeaders.contains(currbb);
      });

  // handle infinite lifetime
  if (!lifetimeLeakingUsers.empty()) {
    // traverse all the successors until there are no left.
    decltype(bbInSet) leakingbbIn;
    decltype(bbOutSet) leakingbbOut;

    for (auto *I : lifetimeLeakingUsers)
      worklist.push_back(I->getParent());

    doWorkLoop<llvm::succ_range>(
        worklist, leakingbbOut, leakingbbIn, [&](auto *currbb) { return llvm::successors(currbb); },
        [&](auto *currbb) { return false; });

    // add terminators to users, so we can later add them to our lifetimeEnd vector
    auto leakingbbOnlyIn = leakingbbIn;
    leakingbbOnlyIn.set_subtract(leakingbbOut);

    for (auto *bb : leakingbbOnlyIn)
      usersOfAllocation.insert(bb->getTerminator());

    bbInSet.set_union(leakingbbIn);
    bbOutSet.set_union(leakingbbOut);
  }

  // if the lifetime escapes any loop, we should make sure all the loops blocks are included
  for (const auto &loop : LI) {
    SmallVector<std::pair<BasicBlock *, BasicBlock *>> exitEdges;
    loop->getExitEdges(exitEdges);

    if (llvm::any_of(exitEdges,
                     [&](auto edge) { return bbOutSet.contains(edge.first) && bbInSet.contains(edge.second); })) {
      llvm::for_each(loop->blocks(), [&](auto *block) {
        bbOutSet.insert(block);
        bbInSet.insert(block);
      });

      if (loop->getLoopPreheader()) {
        bbOutSet.insert(loop->getLoopPreheader());
      } else {
        // if the header has multiple predecessors, we need to find the common dominator of all of these
        auto *commonDominator = loop->getHeader();
        for (auto *bb : llvm::predecessors(loop->getHeader())) {
          if (loop->contains(bb))
            continue;

          commonDominator = DT.findNearestCommonDominator(commonDominator, bb);
          worklist.push_back(bb);
        }

        // acknowledge lifetime flow out of the common dominator block
        bbOutSet.insert(commonDominator);

        // add all blocks inbetween
        doWorkLoop<llvm::pred_range>(
            worklist, bbInSet, bbOutSet, [&](auto *currbb) { return llvm::predecessors(currbb); },
            [&](auto *currbb) { return bbOutSet.contains(currbb) || currbb == commonDominator; });
      }
    }
  }

  // at this point we have all the blocks we need, so fill out the start/end data

  // substract the inflow blocks from the outflow blocks to find the block which starts the lifetime - there should be
  // only one!
  auto bbOutOnly = bbOutSet;
  bbOutOnly.set_subtract(bbInSet);

  IGC_ASSERT_MESSAGE(bbOutOnly.size() == 1, "Multiple lifetime start blocks?");

  auto *lifetimeStartBB = *bbOutOnly.begin();

  // fill out the lifetime start/ends instruction
  for (auto &I : *lifetimeStartBB) {
    lifetimeStart = LivenessInstruction(&I, bbToIndexMap);
    if (usersOfAllocation.contains(&I))
      break;
  }

  // if bbIn is empty, the entire lifetime is contained within userDominatorBlock
  if (bbInSet.empty()) {
    for (auto &I : llvm::reverse(*userDominatorBlock)) {
      if (usersOfAllocation.contains(&I)) {
        lifetimeEndInstructions.push_back(LivenessInstruction(&I, bbToIndexMap));
        break;
      }
    }

    // clear the bbOut to indicate lifetime does not leave any block;
    bbOutSet.clear();
  } else {
    // find all blocks where lifetime flows in, but doesnt flow out
    auto bbOnlyIn = bbInSet;
    bbOnlyIn.set_subtract(bbOutSet);

    for (auto *bb : bbOnlyIn) {
      for (auto &I : llvm::reverse(*bb)) {
        if (usersOfAllocation.contains(&I)) {
          lifetimeEndInstructions.push_back(LivenessInstruction(&I, bbToIndexMap));
          break;
        }
      }
    }
  }

  // collect lifetime end edges (where outflow block has successors that aren't inflow blocks)
  for (auto *bb : bbOutSet) {
    // however, we can't just add successors
    // because then we can accidentally execute lifetime end instruction twice
    // which can end up causing issues similar to double-free
    // we need to make sure every successor has a single predecessor
    SmallVector<BasicBlock *> successors(llvm::successors(bb));
    for (auto *succ : successors) {
      if (bbInSet.contains(succ))
        continue;

      lifetimeEndEdges.push_back({bb, succ});
    }
  }

  auto &F = *allocationInstruction->getParent()->getParent();
  // Fill bitvectors for faster overlap checks
  bbIn.resize(F.size());
  for (auto *bb : bbInSet) {
    bbIn.set(bbToIndexMap.lookup(bb));
  }
  bbOut.resize(F.size());
  for (auto *bb : bbOutSet) {
    bbOut.set(bbToIndexMap.lookup(bb));
  }
}

unsigned AllocationLivenessAnalyzer::getInstructionIndex(const llvm::Instruction *I) {
  unsigned index = 0;
  for (const auto &CurI : *I->getParent()) {
    if (&CurI == I) {
      return index;
    }
    ++index;
  }
  llvm_unreachable("Instruction not found in parent BasicBlock!");
}

unsigned AllocationLivenessAnalyzer::getBBIndex(const llvm::BasicBlock *BB) {
  unsigned index = 0;
  for (const auto &CurBB : *BB->getParent()) {
    if (&CurBB == BB) {
      return index;
    }
    ++index;
  }
  llvm_unreachable("BasicBlock not found in parent Function!");
}

void AllocationLivenessAnalyzer::initBBtoIndexMap(llvm::Function &F) {
  size_t index = 0;
  for (auto &BB : F) {
    PerFunctionBBToIndexMap[&F][&BB] = index++;
  }
}

bool AllocationLivenessAnalyzer::LivenessData::OverlapsWith(const LivenessData &LD) const {

  // check if both lifetimes flow out or in the same block, this means overlap
  if (this->bbIn.anyCommon(LD.bbIn) || this->bbOut.anyCommon(LD.bbOut))
    return true;

  // check lifetime boundaries
  auto pairs = {std::make_pair(this, &LD), std::make_pair(&LD, this)};
  for (auto &[LD1, LD2] : pairs) {
    // If either the start or any end of LD1 lies within LD2, lifetimes overlap
    if (LD2->ContainsInstruction(LD1->lifetimeStart))
      return true;

    for (auto &LI : LD1->lifetimeEndInstructions) {
      if (LD2->ContainsInstruction(LI))
        return true;
    }
  }
  return false;
}

bool AllocationLivenessAnalyzer::LivenessData::ContainsInstruction(const LivenessInstruction &LI) const {

  // if the LD is contained in a single block, bbIn and bbOut are going to be empty.
  // TODO: maybe LivenessData deserves a flag to mark livenesses contained in a single block?
  if (bbIn.none() && bbOut.none()) {
    if (LI.parentBBIndex != lifetimeStart.parentBBIndex)
      return false;

    if (LI.instIndexInBB < lifetimeStart.instIndexInBB)
      return false;

    if (lifetimeEndInstructions[0].instIndexInBB < LI.instIndexInBB)
      return false;

    return true;
  }

  if (!bbIn.test(LI.parentBBIndex) && !bbOut.test(LI.parentBBIndex))
    return false;

  if (bbIn.test(LI.parentBBIndex) && bbOut.test(LI.parentBBIndex))
    return true;

  if (lifetimeStart.parentBBIndex == LI.parentBBIndex && !(LI.instIndexInBB < lifetimeStart.instIndexInBB))
    return true;

  bool overlapsWithEnd = any_of(lifetimeEndInstructions, [&](auto &lifetimeEnd) {
    return lifetimeEnd.parentBBIndex == LI.parentBBIndex && !(lifetimeEnd.instIndexInBB < LI.instIndexInBB);
  });

  return overlapsWithEnd;
}

namespace IGC {
namespace Provenance {
static bool tryFindPointerOriginImpl(Value *ptr, SmallVectorImpl<Instruction *> &origins, DenseSet<Value *> &cache);

bool tryFindPointerOrigin(Value *ptr, SmallVectorImpl<Instruction *> &origins) {
  origins.clear();

  DenseSet<Value *> cache;
  bool found = tryFindPointerOriginImpl(ptr, origins, cache);

  IGC_ASSERT_MESSAGE(!(found && origins.empty()), "Origin reported as found but no origins were added!");

  return found;
}

static bool tryFindPointerOrigin(GetElementPtrInst *Ptr, SmallVectorImpl<Instruction *> &origins,
                                 DenseSet<Value *> &cache) {
  return tryFindPointerOriginImpl(Ptr->getPointerOperand(), origins, cache);
}

static bool tryFindPointerOrigin(SelectInst *Ptr, SmallVectorImpl<Instruction *> &origins, DenseSet<Value *> &cache) {
  return tryFindPointerOriginImpl(Ptr->getTrueValue(), origins, cache) &&
         tryFindPointerOriginImpl(Ptr->getFalseValue(), origins, cache);
}

static bool tryFindPointerOriginImpl(Value *ptr, SmallVectorImpl<Instruction *> &origins, DenseSet<Value *> &cache) {
  if (!cache.insert(ptr).second)
    return true;

  if (auto *GEP = dyn_cast<GetElementPtrInst>(ptr)) {
    return tryFindPointerOrigin(GEP, origins, cache);
  }

  if (auto *select = dyn_cast<SelectInst>(ptr)) {
    return tryFindPointerOrigin(select, origins, cache);
  }

  if (auto *allocaI = dyn_cast<AllocaInst>(ptr)) {
    origins.push_back(allocaI);
    return true;
  }

  return false;
}

} // namespace Provenance
} // namespace IGC
