/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "MergeAllocas.h"
#include "Probe/Assertion.h"
#include "debug/DebugMacros.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SetOperations.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
IGC_INITIALIZE_PASS_BEGIN(AllocationBasedLivenessAnalysis, "igc-allocation-based-liveness-analysis", "Analyze the lifetimes of instruction allocated by a specific intrinsic", false, true)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(AllocationBasedLivenessAnalysis, "igc-allocation-based-liveness-analysis", "Analyze the lifetimes of instruction allocated by a specific intrinsic", false, true)

char AllocationBasedLivenessAnalysis::ID = 0;

void AllocationBasedLivenessAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.setPreservesAll();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
}

AllocationBasedLivenessAnalysis::AllocationBasedLivenessAnalysis() : FunctionPass(ID)
{
    initializeAllocationBasedLivenessAnalysisPass(*llvm::PassRegistry::getPassRegistry());
}

bool AllocationBasedLivenessAnalysis::runOnFunction(llvm::Function& F)
{
    // collect all allocation instructions
    SmallVector<Instruction*> allocationInstructions;

    for (auto& I : instructions(F))
    {
        if (isa<AllocaInst>(&I))
            allocationInstructions.push_back(&I);
    }

    clearLivenessInfo();

    for (auto* I : allocationInstructions)
    {
        m_LivenessInfo.push_back(std::make_pair(I, ProcessInstruction(I)));
    }

    return false;
}

AllocationBasedLivenessAnalysis::LivenessData* AllocationBasedLivenessAnalysis::ProcessInstruction(Instruction* I)
{
    // static allocas are usually going to be in the entry block
    // that's a practice, but we only care about the last block that dominates all uses
    BasicBlock* commonDominator = nullptr;
    auto* DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    auto* LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    bool hasNoLifetimeEnd = false;

    SetVector<Instruction*> allUsers;
    SmallVector<Use*> worklist;

    for (auto& use : I->uses())
    {
        auto* UasI = cast<Instruction>(use.getUser());
        if (commonDominator)
        {
            commonDominator = DT->findNearestCommonDominator(commonDominator, UasI->getParent());
        }
        else
        {
            commonDominator = UasI->getParent();
        }

        worklist.push_back(&use);
    }

    // figure out the potential accesses to the memory via GEP and bitcasts
    while (!worklist.empty() && !hasNoLifetimeEnd)
    {
        auto* use = worklist.pop_back_val();
        auto* II = cast<Instruction>(use->getUser());

        if (allUsers.contains(II))
            continue;

        allUsers.insert(II);

        switch (II->getOpcode())
        {
        case Instruction::PHI:
        case Instruction::GetElementPtr:
        case Instruction::BitCast:
            for (auto& use : II->uses())
                worklist.push_back(&use);

            break;
        case Instruction::PtrToInt:
            hasNoLifetimeEnd = true;
            break;
        case Instruction::Store:
            {
                auto* storeI = cast<StoreInst>(II);
                if (storeI->getValueOperand() == cast<Value>(use))
                    hasNoLifetimeEnd = true;
            }
            break;
        case Instruction::Call:
            {
                auto* callI = cast<CallInst>(II);
                if (!callI->doesNotCapture(use->getOperandNo()))
                    hasNoLifetimeEnd = true;
            }
            break;
        case Instruction::Load:
            break;
        default: // failsafe for handling "unapproved" instructions
            hasNoLifetimeEnd = true;
            break;
        }
    }

    // we add the return instructions to the list of users to express the infinite lifetime
    if (hasNoLifetimeEnd)
    {
        for_each(instructions(*I->getFunction()),
            [&](auto& II)
            {
                if (isa<ReturnInst>(&II))
                    allUsers.insert(&II);
            }
        );
    }

    return new LivenessData(I, allUsers, *LI, commonDominator);
}

AllocationBasedLivenessAnalysis::LivenessData::LivenessData(Instruction* allocationInstruction, const SetVector<Instruction*>& usersOfAllocation, const LoopInfo& LI, BasicBlock* userDominatorBlock)
{
    if (!userDominatorBlock)
        userDominatorBlock = allocationInstruction->getParent();

    bbOut.insert(userDominatorBlock);

    SmallVector<BasicBlock*> worklist;

    for (auto* I : usersOfAllocation)
    {
        worklist.push_back(I->getParent());
    }

    // perform data flow analysis
    while (!worklist.empty())
    {
        auto* currbb = worklist.pop_back_val();

        if (bbIn.contains(currbb) || currbb == userDominatorBlock)
            continue;

        bbIn.insert(currbb);

        for (auto* pbb : llvm::predecessors(currbb))
        {
            bbOut.insert(pbb);
            worklist.push_back(pbb);
        }
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
                [&](auto* block) {
                    bbOut.insert(block);
                    if (block != loop->getHeader())
                        bbIn.insert(block);
                }
            );
        }
    }

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
                lifetimeEnds.push_back(&I);
                break;
            }
        }

        // clear the bbOut to indicate lifetime does not leave any block;
        bbOut.clear();
    }
    else
    {
        auto bbOnlyIn = bbIn;
        set_subtract(bbOnlyIn, bbOut);

        for (auto* bb : bbOnlyIn)
        {
            for (auto& I : llvm::reverse(*bb))
            {
                if (usersOfAllocation.contains(&I))
                {
                    lifetimeEnds.push_back(&I);
                    break;
                }
            }
        }
    }
}

bool AllocationBasedLivenessAnalysis::LivenessData::OverlapsWith(const LivenessData& LD) const
{
    auto overlapIn = bbIn;
    set_intersect(overlapIn, LD.bbIn);

    auto overlapOut = bbOut;
    set_intersect(overlapOut, LD.bbOut);

    // check if both lifetimes flow out or in the same block, this means overlap
    if (!overlapIn.empty() || !overlapOut.empty())
        return true;

    // check lifetime boundaries
    for (auto& [LD1, LD2] : { std::make_pair(*this, LD), std::make_pair(LD, *this) })
    {
        if (LD1.lifetimeEnds.size() == 1 && *LD1.lifetimeEnds.begin() == LD1.lifetimeStart)
            continue;

        for (auto* I : LD1.lifetimeEnds)
        {
            if (I->getParent() == LD2.lifetimeStart->getParent())
            {
                if (LD2.lifetimeStart->comesBefore(I))
                    return true;
            }
        }
    }

    return false;
}

// Register pass to igc-opt
IGC_INITIALIZE_PASS_BEGIN(MergeAllocas, "igc-merge-allocas", "Try to reuse allocas with nonoverlapping lifetimes", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(AllocationBasedLivenessAnalysis)
IGC_INITIALIZE_PASS_END(MergeAllocas, "igc-merge-allocas", "Try to reuse allocas with nonoverlapping lifetimes", false, false)

char MergeAllocas::ID = 0;

namespace IGC
{
    Pass* createMergeAllocas()
    {
        return new MergeAllocas();
    }
}

MergeAllocas::MergeAllocas() : FunctionPass(ID)
{
    initializeMergeAllocasPass(*llvm::PassRegistry::getPassRegistry());
}

void MergeAllocas::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<AllocationBasedLivenessAnalysis>();
}

bool MergeAllocas::runOnFunction(Function& F)
{
    auto ABLA = getAnalysis<AllocationBasedLivenessAnalysis>().getLivenessInfo();

    // we group the allocations by type, then sort them into buckets with nonoverlapping liveranges
    // can this be generalized into allocas for types of the same size, not only types?
    using BucketT = SmallVector<std::pair<Instruction*, AllocationBasedLivenessAnalysis::LivenessData*>>;
    DenseMap<std::tuple<llvm::Type*, uint64_t, uint32_t>, SmallVector<BucketT>> buckets;

    for (const auto& A : ABLA)
    {
        const auto& [currI, currLD] = A;
        // at this point we assume all I's are alloca instructions
        // later AllocationBasedLivenessAnalysis will be generalized to any instruction that can allocate something (like allocaterayquery)
        auto* AI = cast<AllocaInst>(currI);

        if (!isa<ConstantInt>(AI->getArraySize()))
            continue;

        auto& perTypeBuckets = buckets[std::make_tuple(
            AI->getAllocatedType(),
            cast<ConstantInt>(AI->getArraySize())->getZExtValue(),
            AI->getAddressSpace()
        )];

        bool found = false;

        for (auto& bucket : perTypeBuckets)
        {
            if (llvm::none_of(bucket, [&](std::pair<Instruction*, AllocationBasedLivenessAnalysis::LivenessData*> b) { return b.second->OverlapsWith(*A.second); }))
            {
                bucket.push_back(std::make_pair(currI, currLD));
                found = true;
                break;
            }
        }

        if (!found)
        {
            perTypeBuckets.push_back({ std::make_pair(currI, currLD) });
        }
    }

    bool changed = false;

    for (const auto& [_, perTypeBuckets] : buckets)
    {
        for (const auto& bucket : perTypeBuckets)
        {
            if (bucket.size() == 1)
            {
                continue;
            }

            Instruction* firstAlloca = nullptr;
            for (const auto& [I, _] : bucket)
            {
                if (!firstAlloca)
                {
                    firstAlloca = I;
                    firstAlloca->moveBefore(F.getEntryBlock().getFirstNonPHI());
                    firstAlloca->setName(VALUE_NAME("MergedAlloca"));
                }
                else
                {
                    I->replaceAllUsesWith(firstAlloca);
                    I->eraseFromParent();
                }

            }

            changed = true;
        }
    }

    return changed;
}
