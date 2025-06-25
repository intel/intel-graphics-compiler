/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MergeAllocas.h"
#include "Compiler/IGCPassSupport.h"
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

// Get size of bytes allocated for type including padding.
static size_t GetByteSize(Type *T, const DataLayout *DL) {
  if (T->isSized())
    return static_cast<size_t>(DL->getTypeAllocSize(T));
  return 0;
}

static MergeAllocas::AllocaInfo GetAllocaInfo(AllocaInst *allocaI,
                            AllocationLivenessAnalyzer::LivenessData *LD,
                            const DataLayout *DL) {
    size_t allocationSize = GetByteSize(allocaI->getAllocatedType(), DL);
    return {{},
            allocaI,
            LD,
            allocaI->getAddressSpace(),
            allocationSize,
            allocationSize,
            static_cast<size_t>(
                DL->getPrefTypeAlign(allocaI->getAllocatedType()).value()),
            0,
            allocaI->getMetadata("uniform") != nullptr};
}

static size_t GetStartingOffset(size_t startOffset, size_t alignment) {
    size_t remainder = startOffset % alignment;
    if (remainder == 0) {
        return startOffset;
    }
    return startOffset + (alignment - remainder);
}

static bool AddNonOverlappingAlloca(MergeAllocas::AllocaInfo* MergableAlloca,
                                    MergeAllocas::AllocaInfo* NewAlloca) {
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

static void ReplaceAllocas(const MergeAllocas::AllocaInfo &MergableAlloca, Function &F) {
    Instruction *topAlloca = MergableAlloca.allocaI;
    topAlloca->moveBefore(F.getEntryBlock().getFirstNonPHI());
    topAlloca->setName(VALUE_NAME("MergedAlloca"));

    IRBuilder<> Builder(topAlloca->getParent());
    Instruction *topAllocaBitcast = nullptr;

    SmallVector<MergeAllocas::AllocaInfo *> allocasToReplace;
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

bool MergeAllocas::runOnFunction(Function &F) {
    if (skipFunction(F)) {
        return false;
    }

    auto& DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    auto& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    // in the past, the analysis pass used to be responsible for the liveness data objects
    // the pass got deleted as a part of refactor (it was leaking memory anyway),
    // so to avoid changing the logic, we create a backing storage for the liveness data objects
    // we should revisit this at some point to clean it up, but for now
    // we use std::list so it doesn't invalidate the references when inserting
    std::list<LivenessData> storage;

    llvm::SmallVector<std::pair<llvm::Instruction*, LivenessData*>> ABLA;

    for (auto& I : make_filter_range(instructions(F), [](auto& I) { return isa<AllocaInst>(&I); }))
    {
        storage.push_back(ProcessInstruction(&I, DT, LI));
        ABLA.push_back(std::make_pair(&I, &storage.back()));
    }

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
