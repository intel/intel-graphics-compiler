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

AllocationLivenessAnalyzer::LivenessData AllocationLivenessAnalyzer::ProcessInstruction(
    Instruction* I,
    DominatorTree& DT,
    LoopInfo& LI
)
{
    // static allocas are usually going to be in the entry block
    // that's a practice, but we only care about the last block that dominates all uses
    BasicBlock* commonDominator = nullptr;
    SetVector<Instruction*> allUsers;
    SetVector<Instruction*> lifetimeLeakingUsers;
    SmallVector<Use*> worklist;

    for (auto& use : I->uses())
    {
        auto* UasI = cast<Instruction>(use.getUser());
        if (commonDominator)
        {
            commonDominator = DT.findNearestCommonDominator(commonDominator, UasI->getParent());
        }
        else
        {
            commonDominator = UasI->getParent();
        }

        worklist.push_back(&use);
    }

    auto addUsesFn = [&worklist](auto uses) {
        for (auto& use : uses)
            worklist.push_back(&use);
    };

    // figure out the potential accesses to the memory via GEP and bitcasts
    while (!worklist.empty())
    {
        auto* use = worklist.pop_back_val();
        auto* II = cast<Instruction>(use->getUser());

        if (!allUsers.insert(II))
            continue;

        // a possible optimization here:
        // 1. find all reachable blocks
        // 2. cull uses that are not reachable from the allocation

        commonDominator = DT.findNearestCommonDominator(commonDominator, II->getParent());

        switch (II->getOpcode())
        {
        case Instruction::PHI:
        case Instruction::GetElementPtr:
        case Instruction::BitCast:
        case Instruction::Select:
            addUsesFn(II->uses());
            break;
        case Instruction::PtrToInt:
            lifetimeLeakingUsers.insert(II);
            break;
        case Instruction::Store:
        {
            auto* storeI = cast<StoreInst>(II);
            if (storeI->getValueOperand() == use->get())
            {
                SmallVector<Instruction*> origins;
                if (Provenance::tryFindPointerOrigin(storeI->getPointerOperand(), origins))
                {
                    for (auto* origin : origins)
                        addUsesFn(origin->uses());
                }
                else
                {
                    lifetimeLeakingUsers.insert(II);
                }
            }
        }
        break;
        case Instruction::Call:
        {
            auto* callI = cast<CallInst>(II);
            if (!callI->doesNotCapture(use->getOperandNo()))
                lifetimeLeakingUsers.insert(II);

            if (II->getType()->isPointerTy())
                addUsesFn(II->uses());
        }
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

    return LivenessData(I, std::move(allUsers), LI, DT, commonDominator, std::move(lifetimeLeakingUsers));
}

void AllocationLivenessAnalyzer::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    getAdditionalAnalysisUsage(AU);
}

template<typename range>
static inline void doWorkLoop(
    SmallVector<BasicBlock*>& worklist,
    DenseSet<BasicBlock*>& bbSet1,
    DenseSet<BasicBlock*>& bbSet2,
    std::function<range(BasicBlock*)> iterate,
    std::function<bool(BasicBlock*)> continueCondition
) {
    // perform data flow analysis
    while (!worklist.empty())
    {
        auto* currbb = worklist.pop_back_val();

        if (continueCondition(currbb))
            continue;

        bool addToSet1 = false;

        for (auto* pbb : iterate(currbb))
        {
            addToSet1 = true;

            bool inserted = bbSet2.insert(pbb).second;

            if (inserted)
                worklist.push_back(pbb);
        }

        if (addToSet1)
            bbSet1.insert(currbb);
    }

}

AllocationLivenessAnalyzer::LivenessData::LivenessData(
    Instruction* allocationInstruction,
    SetVector<Instruction*>&& usersOfAllocation,
    const LoopInfo& LI,
    const DominatorTree& DT,
    BasicBlock* userDominatorBlock,
    SetVector<Instruction*>&& lifetimeLeakingUsers
) {
    if (!userDominatorBlock)
        userDominatorBlock = allocationInstruction->getParent();

    bbOut.insert(userDominatorBlock);

    SmallVector<BasicBlock*> worklist;

    for (auto* I : usersOfAllocation)
    {
        worklist.push_back(I->getParent());
    }

    // Keep track of loop header of blocks that contain allocation instruction
    auto* allocationParent = allocationInstruction->getParent();
    llvm::SmallPtrSet<llvm::BasicBlock*, 4> containedLoopHeaders;
    if (const auto* parentLoop = LI.getLoopFor(allocationParent))
    {
        containedLoopHeaders.insert(parentLoop->getHeader());
        while (parentLoop->getParentLoop()) {
            parentLoop = parentLoop->getParentLoop();
            containedLoopHeaders.insert(parentLoop->getHeader());
        }
    }

    // perform data flow analysis
    doWorkLoop<llvm::pred_range>(
        worklist,
        bbIn,
        bbOut,
        [&](auto* currbb) { return llvm::predecessors(currbb); },
        [&](auto* currbb) { return bbIn.contains(currbb) || currbb == userDominatorBlock || containedLoopHeaders.contains(currbb); }
    );

    // handle infinite lifetime
    if (!lifetimeLeakingUsers.empty())
    {
        // traverse all the successors until there are no left.
        decltype(bbIn) leakingbbIn;
        decltype(bbOut) leakingbbOut;

        for (auto* I : lifetimeLeakingUsers)
            worklist.push_back(I->getParent());

        doWorkLoop<llvm::succ_range>(
            worklist,
            leakingbbOut,
            leakingbbIn,
            [&](auto* currbb) { return llvm::successors(currbb); },
            [&](auto* currbb) { return false; }
        );

        // add terminators to users, so we can later add them to our lifetimeEnd vector
        auto leakingbbOnlyIn = leakingbbIn;
        set_subtract(leakingbbOnlyIn, leakingbbOut);

        for (auto* bb : leakingbbOnlyIn)
            usersOfAllocation.insert(bb->getTerminator());

        set_union(bbIn, leakingbbIn);
        set_union(bbOut, leakingbbOut);
    }

    // if the lifetime escapes any loop, we should make sure all the loops blocks are included
    for (const auto& loop : LI)
    {
        SmallVector<std::pair<BasicBlock*, BasicBlock*>> exitEdges;
        loop->getExitEdges(exitEdges);

        if (llvm::any_of(exitEdges, [&](auto edge) { return bbOut.contains(edge.first) && bbIn.contains(edge.second); }))
        {
            llvm::for_each(
                loop->blocks(),
                [&](auto* block) { bbOut.insert(block); bbIn.insert(block); }
            );

            if (loop->getLoopPreheader())
            {
                bbOut.insert(loop->getLoopPreheader());
            }
            else
            {
                // if the header has multiple predecessors, we need to find the common dominator of all of these
                auto* commonDominator = loop->getHeader();
                for (auto* bb : llvm::predecessors(loop->getHeader()))
                {
                    if (loop->contains(bb))
                        continue;

                    commonDominator = DT.findNearestCommonDominator(commonDominator, bb);
                    worklist.push_back(bb);
                }

                // acknowledge lifetime flow out of the common dominator block
                bbOut.insert(commonDominator);

                // add all blocks inbetween
                doWorkLoop<llvm::pred_range>(
                    worklist,
                    bbIn,
                    bbOut,
                    [&](auto* currbb) { return llvm::predecessors(currbb); },
                    [&](auto* currbb) { return bbOut.contains(currbb) || currbb == commonDominator; }
                );
            }
        }
    }

    // at this point we have all the blocks we need, so fill out the start/end data

    // substract the inflow blocks from the outflow blocks to find the block which starts the lifetime - there should be only one!
    auto bbOutOnly = bbOut;
    set_subtract(bbOutOnly, bbIn);

    IGC_ASSERT_MESSAGE(bbOutOnly.size() == 1, "Multiple lifetime start blocks?");

    auto* lifetimeStartBB = *bbOutOnly.begin();

    // fill out the lifetime start/ends instruction
    for (auto& I : *lifetimeStartBB)
    {
        lifetimeStart = &I;
        if (usersOfAllocation.contains(&I))
            break;
    }

    // if bbIn is empty, the entire lifetime is contained within userDominatorBlock
    if (bbIn.empty())
    {
        for (auto& I : llvm::reverse(*userDominatorBlock))
        {
            if (usersOfAllocation.contains(&I))
            {
                lifetimeEndInstructions.push_back(&I);
                break;
            }
        }

        // clear the bbOut to indicate lifetime does not leave any block;
        bbOut.clear();
    }
    else
    {
        // find all blocks where lifetime flows in, but doesnt flow out
        auto bbOnlyIn = bbIn;
        set_subtract(bbOnlyIn, bbOut);

        for (auto* bb : bbOnlyIn)
        {
            for (auto& I : llvm::reverse(*bb))
            {
                if (usersOfAllocation.contains(&I))
                {
                    lifetimeEndInstructions.push_back(&I);
                    break;
                }
            }
        }
    }

    // collect lifetime end edges (where outflow block has successors that aren't inflow blocks)
    for (auto* bb : bbOut)
    {
        // however, we can't just add successors
        // because then we can accidentally execute lifetime end instruction twice
        // which can end up causing issues similar to double-free
        // we need to make sure every successor has a single predecessor
        SmallVector<BasicBlock*> successors(llvm::successors(bb));
        for (auto* succ : successors)
        {
            if (bbIn.contains(succ))
                continue;

            lifetimeEndEdges.push_back({ bb, succ });
        }
    }
}

bool AllocationLivenessAnalyzer::LivenessData::OverlapsWith(const LivenessData& LD) const
{
    auto overlapIn = bbIn;
    set_intersect(overlapIn, LD.bbIn);

    auto overlapOut = bbOut;
    set_intersect(overlapOut, LD.bbOut);

    // check if both lifetimes flow out or in the same block, this means overlap
    if (!overlapIn.empty() || !overlapOut.empty())
        return true;

    // check lifetime boundaries
    for (auto& [LD1, LD2] : { std::make_pair(this, &LD), std::make_pair(&LD, this) })
    {
        // TODO: replace the whole logic with ContainsInstruction checks
        for (auto* I : LD1->lifetimeEndInstructions)
        {
            // what if LD1 is contained in a single block
            if (I->getParent() == LD1->lifetimeStart->getParent())
            {
                auto* bb = I->getParent();
                bool inflow = LD2->bbIn.contains(bb);
                bool outflow = LD2->bbOut.contains(bb);
                bool lifetimeStart = LD2->lifetimeStart->getParent() == bb && LD2->lifetimeStart->comesBefore(I);

                auto* LD1_lifetimeStart = LD1->lifetimeStart; // we have to copy LD1.lifetimeStart to avoid clang complaining about LD1 being captured by the lambda
                bool lifetimeEnd = any_of(LD2->lifetimeEndInstructions, [&](auto* lifetimeEnd) {
                    return lifetimeEnd->getParent() == bb && LD1_lifetimeStart->comesBefore(lifetimeEnd);
                    });

                if (inflow && outflow)
                    return true;

                if (inflow && lifetimeEnd)
                    return true;

                if (outflow && lifetimeStart)
                    return true;

                if (lifetimeEnd && lifetimeStart)
                    return true;
            }
            else if (I->getParent() == LD2->lifetimeStart->getParent())
            {
                if (LD2->lifetimeStart->comesBefore(I))
                    return true;
            }
        }
    }
    return false;
}

bool AllocationLivenessAnalyzer::LivenessData::ContainsInstruction(const llvm::Instruction& I) const
{
    auto* bb = I.getParent();

    // if the LD is contained in a single block, bbIn and bbOut are going to be empty.
    // TODO: maybe LivenessData deserves a flag to mark livenesses contained in a single block?
    if (bbIn.empty() && bbOut.empty())
    {
        if (bb != lifetimeStart->getParent())
            return false;

        if (I.comesBefore(lifetimeStart))
            return false;

        if (lifetimeEndInstructions[0]->comesBefore(&I))
            return false;

        return true;
    }

    if (!bbIn.contains(bb) && !bbOut.contains(bb))
        return false;

    if (bbIn.contains(bb) && bbOut.contains(bb))
        return true;

    if (lifetimeStart->getParent() == bb && !I.comesBefore(lifetimeStart))
        return true;

    bool overlapsWithEnd = any_of(lifetimeEndInstructions, [&](auto* lifetimeEnd) {
        return lifetimeEnd->getParent() == bb && !lifetimeEnd->comesBefore(&I);
        });

    return overlapsWithEnd;
}

namespace IGC
{
    namespace Provenance
    {
        static bool tryFindPointerOriginImpl(Value* ptr, SmallVectorImpl<Instruction*>& origins, DenseSet<Value*>& cache);

        bool tryFindPointerOrigin(Value* ptr, SmallVectorImpl<Instruction*>& origins)
        {
            origins.clear();

            DenseSet<Value*> cache;
            bool found = tryFindPointerOriginImpl(ptr, origins, cache);

            IGC_ASSERT_MESSAGE(found && !origins.empty(), "Origin reported as found but no origins were added!");

            return found;
        }

        static bool tryFindPointerOrigin(GetElementPtrInst* Ptr, SmallVectorImpl<Instruction*>& origins, DenseSet<Value*>& cache)
        {
            return tryFindPointerOriginImpl(Ptr->getPointerOperand(), origins, cache);
        }

        static bool tryFindPointerOriginImpl(Value* ptr, SmallVectorImpl<Instruction*>& origins, DenseSet<Value*>& cache)
        {
            if (!cache.insert(ptr).second)
                return true;

            if (auto* GEP = dyn_cast<GetElementPtrInst>(ptr))
            {
                return tryFindPointerOrigin(GEP, origins, cache);
            }

            if (auto* allocaI = dyn_cast<AllocaInst>(ptr))
            {
                origins.push_back(allocaI);
                return true;
            }

            return false;
        }

    }
}
