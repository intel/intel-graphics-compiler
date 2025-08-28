/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MergeAllocas.h"
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

// Register pass to igc-opt
IGC_INITIALIZE_PASS_BEGIN(AllocationBasedLivenessAnalysis, "igc-allocation-based-liveness-analysis", "Analyze the lifetimes of instruction allocated by a specific intrinsic", false, true)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(AllocationBasedLivenessAnalysis, "igc-allocation-based-liveness-analysis", "Analyze the lifetimes of instruction allocated by a specific intrinsic", false, true)

// Get size of bytes allocated for type including padding.
static size_t GetByteSize(Type *T, const DataLayout *DL) {
  if (T->isSized())
    return static_cast<size_t>(DL->getTypeAllocSize(T));
  return 0;
}

static AllocaInfo GetAllocaInfo(AllocaInst *alloca,
                            AllocationBasedLivenessAnalysis::LivenessData *LD,
                            const DataLayout *DL) {
    size_t allocationSize = GetByteSize(alloca->getAllocatedType(), DL);
    return {{},
            alloca,
            LD,
            alloca->getAddressSpace(),
            allocationSize,
            allocationSize,
            static_cast<size_t>(
                DL->getPrefTypeAlign(alloca->getAllocatedType()).value()),
            0,
            alloca->getMetadata("uniform") != nullptr};
}

static size_t GetStartingOffset(size_t startOffset, size_t alignment) {
    size_t remainder = startOffset % alignment;
    if (remainder == 0) {
        return startOffset;
    }
    return startOffset + (alignment - remainder);
}

static bool AddNonOverlappingAlloca(AllocaInfo *MergableAlloca,
                             AllocaInfo *NewAlloca) {
    if (MergableAlloca->isUniform != NewAlloca->isUniform) {
        return false;
    }
    if (MergableAlloca->addressSpace != NewAlloca->addressSpace) {
        return false;
    }
    if (MergableAlloca->allocationSize < NewAlloca->allocationSize) {
        return false;
    }
    if (MergableAlloca->livenessData->OverlapsWith(*NewAlloca->livenessData)) {
        return false;
    }

    if (IGC_IS_FLAG_ENABLED(DisableMergingOfAllocasWithDifferentType))
    {
        auto* AllocatedType = MergableAlloca->allocaI->getAllocatedType();
        auto* AllocatedTypeNew = NewAlloca->allocaI->getAllocatedType();
        bool IsArray = AllocatedType->isArrayTy()? true : false;
        bool IsArrayNew = AllocatedTypeNew->isArrayTy()? true : false;
        bool AreBothArrays = IsArray && IsArrayNew;
        if (AreBothArrays && AllocatedType->getArrayElementType() != AllocatedTypeNew->getArrayElementType()) {
            return false;
        }
        if (!AreBothArrays && AllocatedType != AllocatedTypeNew) {
            return false;
        }
    }

    // Check if we can merge alloca to one of existing non-overlapping allocas.
    for (auto *NonOverlappingAlloca : MergableAlloca->nonOverlapingAllocas) {
        bool added = AddNonOverlappingAlloca(NonOverlappingAlloca, NewAlloca);
        if (added) {
            return true;
        }
    }

    // Check if we have still space in existing alloca to add new alloca
    if (MergableAlloca->remainingSize >= NewAlloca->allocationSize) {
        size_t currentOffset =
            MergableAlloca->allocationSize - MergableAlloca->remainingSize;
        size_t newStartingOffset =
            GetStartingOffset(currentOffset, NewAlloca->alignment);
        size_t sizeWithPadding =
            NewAlloca->allocationSize + (newStartingOffset - currentOffset);
        // When adding alignment in consideration we can't fit new alloca.
        if (sizeWithPadding > MergableAlloca->remainingSize) {
            return false;
        }
        size_t newAllocaOffset = newStartingOffset + MergableAlloca->offset;
        if (newAllocaOffset != 0 && IGC_IS_FLAG_ENABLED(DisableMergingOfMultipleAllocasWithOffset)) {
            return false;
        }
        NewAlloca->offset = newAllocaOffset;
        MergableAlloca->nonOverlapingAllocas.push_back(NewAlloca);
        MergableAlloca->remainingSize -= sizeWithPadding;
        return true;
    }

    return false;
}

static void ReplaceAllocas(const AllocaInfo &MergableAlloca, Function &F) {
    Instruction *topAlloca = MergableAlloca.allocaI;
    topAlloca->moveBefore(F.getEntryBlock().getFirstNonPHI());
    topAlloca->setName(VALUE_NAME("MergedAlloca"));

    IRBuilder<> Builder(topAlloca->getParent());
    Instruction *topAllocaBitcast = nullptr;

    SmallVector<AllocaInfo *> allocasToReplace;
    allocasToReplace.insert(allocasToReplace.end(),
                            MergableAlloca.nonOverlapingAllocas.begin(),
                            MergableAlloca.nonOverlapingAllocas.end());

    while (!allocasToReplace.empty()) {
        auto *subAlloca = allocasToReplace.pop_back_val();

        auto *subInst = subAlloca->allocaI;
        auto *ReplacementValue = topAlloca;

        if (topAlloca->getType() != subInst->getType()) {
            auto *InsertionPoint =
                (topAllocaBitcast != nullptr) ? topAllocaBitcast : topAlloca;
            Builder.SetInsertPoint(InsertionPoint->getNextNode());

            Value *ValueToCast = nullptr;
            // If we have offset from original alloca we need to create GEP
            if (subAlloca->offset != 0) {
                // We can re-use same bitcast
                if (topAllocaBitcast == nullptr) {
                    topAllocaBitcast = cast<Instruction>(
                        Builder.CreateBitCast(topAlloca,  Builder.getInt8PtrTy(topAlloca->getType()->getPointerAddressSpace())));
                }
                auto *Offset = Builder.getInt32(subAlloca->offset);
                auto *GEP = Builder.CreateGEP(Builder.getInt8Ty(),
                                            topAllocaBitcast, Offset);
                ValueToCast = GEP;
            } else {
                // If no offset is needed we can directly cast to target type
                ValueToCast = Builder.CreateBitCast(topAlloca, subInst->getType());
            }
            auto *CastedValue = llvm::cast<Instruction>(
                Builder.CreateBitCast(ValueToCast, subInst->getType()));
            ReplacementValue = CastedValue;
        }
        subInst->replaceAllUsesWith(ReplacementValue);
        subInst->eraseFromParent();

        allocasToReplace.insert(allocasToReplace.end(),
                                subAlloca->nonOverlapingAllocas.begin(),
                                subAlloca->nonOverlapingAllocas.end());
    }
}

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

    SetVector<Instruction*> allUsers;
    SetVector<Instruction*> lifetimeLeakingUsers;
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
    while (!worklist.empty())
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
        case Instruction::Select:
            for (auto& use : II->uses())
                worklist.push_back(&use);

            break;
        case Instruction::PtrToInt:
            lifetimeLeakingUsers.insert(II);
            break;
        case Instruction::Store:
            {
                auto* storeI = cast<StoreInst>(II);
                if (storeI->getValueOperand() == cast<Value>(use))
                    lifetimeLeakingUsers.insert(II);
            }
            break;
        case Instruction::Call:
            {
                auto* callI = cast<CallInst>(II);
                if (!callI->doesNotCapture(use->getOperandNo()))
                    lifetimeLeakingUsers.insert(II);
            }
            break;
        case Instruction::Load:
            break;
        default: // failsafe for handling "unapproved" instructions
            lifetimeLeakingUsers.insert(II);
            break;
        }
    }

    return new LivenessData(I, std::move(allUsers), *LI, *DT, commonDominator, std::move(lifetimeLeakingUsers));
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

AllocationBasedLivenessAnalysis::LivenessData::LivenessData(
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
    for (auto& [LD1, LD2] : { std::make_pair(this, &LD), std::make_pair(&LD, this) })
    {
        // TODO: replace the whole logic with ContainsInstruction checks
        for (auto* I : LD1->lifetimeEnds)
        {
            // what if LD1 is contained in a single block
            if (I->getParent() == LD1->lifetimeStart->getParent())
            {
                auto* bb = I->getParent();
                bool inflow = LD2->bbIn.contains(bb);
                bool outflow = LD2->bbOut.contains(bb);
                bool lifetimeStart = LD2->lifetimeStart->getParent() == bb && LD2->lifetimeStart->comesBefore(I);

                auto* LD1_lifetimeStart = LD1->lifetimeStart; // we have to copy LD1.lifetimeStart to avoid clang complaining about LD1 being captured by the lambda
                bool lifetimeEnd = any_of(LD2->lifetimeEnds, [&](auto* lifetimeEnd) {
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

bool IGC::AllocationBasedLivenessAnalysis::LivenessData::ContainsInstruction(const llvm::Instruction& I) const
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

        if (lifetimeEnds[0]->comesBefore(&I))
            return false;

        return true;
    }

    if (!bbIn.contains(bb) && !bbOut.contains(bb))
        return false;

    if (bbIn.contains(bb) && bbOut.contains(bb))
        return true;

    if (lifetimeStart->getParent() == bb && !I.comesBefore(lifetimeStart))
        return true;

    bool overlapsWithEnd = any_of(lifetimeEnds, [&](auto* lifetimeEnd) {
        return lifetimeEnd->getParent() == bb && !lifetimeEnd->comesBefore(&I);
    });

    return overlapsWithEnd;
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

bool MergeAllocas::runOnFunction(Function &F) {
    if (skipFunction(F)) {
        return false;
    }

    auto ABLA = getAnalysis<AllocationBasedLivenessAnalysis>().getLivenessInfo();
    const auto *DataLayout = &F.getParent()->getDataLayout();

    // We group non-overlapping allocas for replacements.
    SmallVector<AllocaInfo *> MergableAllocas;

    // First we sort analysis results based on allocation size, from larger to
    // smaller.
    llvm::sort(ABLA, [&](const auto &a, const auto &b) {
        return GetByteSize(cast<AllocaInst>(a.first)->getAllocatedType(),
                        DataLayout) >
            GetByteSize(cast<AllocaInst>(b.first)->getAllocatedType(),
                        DataLayout);
    });

    // Reserve space for all alloca infos so we can use pointers to them.
    AllAllocasInfos.resize(ABLA.size());
    size_t currentIndex = 0;

    // We iterate over analysis results collecting non-overlapping allocas.
    for (const auto &A : ABLA) {
        const auto &[currI, currLD] = A;

        if (skipInstruction(F, *currLD))
            continue;

        AllAllocasInfos[currentIndex] =
            GetAllocaInfo(cast<AllocaInst>(currI), currLD, DataLayout);
        AllocaInfo &AllocaInfo = AllAllocasInfos[currentIndex];
        currentIndex++;

        // We check if the current alloca overlaps with any of the previously added.
        bool added = false;
        for (auto *MergableAlloca : MergableAllocas) {
            if (AllocaInfo.livenessData->OverlapsWith(*MergableAlloca->livenessData)) {
                continue;
            }
            added = AddNonOverlappingAlloca(MergableAlloca, &AllocaInfo);
            if (added) {
                break;
            }
        }
        // Alloca overlaps with all of the current ones so it will be added as new
        // element.
        if (!added && AllocaInfo.allocationSize != 0) {
            MergableAllocas.push_back(&AllocaInfo);
        }
    }

    bool changed = false;

    // Replace alloca usages
    for (auto *MergableAlloca : MergableAllocas) {
        if (MergableAlloca->nonOverlapingAllocas.empty()) {
            continue;
        }
        changed = true;
        ReplaceAllocas(*MergableAlloca, F);
    }

    return changed;
}

// Register pass to igc-opt
IGC_INITIALIZE_PASS_BEGIN(RaytracingMergeAllocas, "igc-rtx-merge-allocas", "Try to reuse allocas with nonoverlapping lifetimes - raytracing version", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(AllocationBasedLivenessAnalysis)
IGC_INITIALIZE_PASS_END(RaytracingMergeAllocas, "igc-rtx-merge-allocas", "Try to reuse allocas with nonoverlapping lifetimes - raytracing version", false, false)

char RaytracingMergeAllocas::ID = 0;

RaytracingMergeAllocas::RaytracingMergeAllocas()
{
    initializeRaytracingMergeAllocasPass(*llvm::PassRegistry::getPassRegistry());
}

bool RaytracingMergeAllocas::skipInstruction(llvm::Function& F, AllocationBasedLivenessAnalysis::LivenessData& LD)
{
    for (auto& I : llvm::make_filter_range(instructions(F), [](auto& I) { return isa<ContinuationHLIntrinsic>(&I); }))
    {
        if (LD.ContainsInstruction(I))
            return false;
    }

    return true;
}

namespace IGC
{
    Pass* createRaytracingMergeAllocas()
    {
        return new RaytracingMergeAllocas();
    }
}
