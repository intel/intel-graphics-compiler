/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===- CoroFrame.cpp - Builds and manipulates coroutine frame -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


#include "SplitAsyncUtils.h"
#include "RTBuilder.h"
#include "RTArgs.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CFG.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

static void rewritePHIs(BasicBlock& BB) {
    // For every incoming edge we will create a block holding all
    // incoming values in a single PHI nodes.
    //
    // loop:
    //    %n.val = phi i32[%n, %entry], [%inc, %loop]
    //
    // It will create:
    //
    // loop.from.entry:
    //    %n.loop.pre = phi i32 [%n, %entry]
    //    br %label loop
    // loop.from.loop:
    //    %inc.loop.pre = phi i32 [%inc, %loop]
    //    br %label loop
    //
    // After this rewrite, further analysis will ignore any phi nodes with more
    // than one incoming edge.

    // TODO: Simplify PHINodes in the basic block to remove duplicate
    // predecessors.

    SmallVector<BasicBlock*, 8> Preds(pred_begin(&BB), pred_end(&BB));
    for (BasicBlock* Pred : Preds) {
        auto* IncomingBB = SplitEdge(Pred, &BB);
        IncomingBB->setName(BB.getName() + Twine(".from.") + Pred->getName());
        auto* PN = cast<PHINode>(&BB.front());
        do {
            int Index = PN->getBasicBlockIndex(IncomingBB);
            Value* V = PN->getIncomingValue(Index);
            PHINode* InputV = PHINode::Create(
                V->getType(), 1, V->getName() + Twine(".") + BB.getName(),
                &IncomingBB->front());
            InputV->addIncoming(V, Pred);
            PN->setIncomingValue(Index, InputV);
            PN = dyn_cast<PHINode>(PN->getNextNode());
        } while (PN);
    }
}

namespace IGC {

void rewritePHIs(Function& F) {
    SmallVector<BasicBlock*, 8> WorkList;

    for (BasicBlock& BB : F)
        if (auto* PN = dyn_cast<PHINode>(&BB.front()))
            if (PN->getNumIncomingValues() > 1)
                WorkList.push_back(&BB);

    for (BasicBlock* BB : WorkList)
        ::rewritePHIs(*BB);
}

void insertSpills(
    CodeGenContext *CGCtx,
    Function& F,
    const SmallVector<Spill, 8>& Spills)
{
    if (Spills.empty())
        return;

    uint32_t Idx = 0;
    DenseMap<Value*, uint32_t> SpillSlots;

    RTBuilder RTB(F.getContext(), *CGCtx);

    Value* CurrentValue  = nullptr;
    BasicBlock* CurrentBlock = nullptr;
    Value* CurrentReload = nullptr;

    for (auto const& E : Spills)
    {
        if (CurrentValue != E.def())
        {
            CurrentValue  = E.def();
            CurrentBlock  = nullptr;
            CurrentReload = nullptr;
        }

        uint32_t CurIdx = 0;
        if (auto I = SpillSlots.find(CurrentValue); I != SpillSlots.end())
        {
            CurIdx = I->second;
        }
        else
        {
            CurIdx = Idx++;
            SpillSlots[CurrentValue] = CurIdx;

            Instruction* InsertPt = nullptr;

            if (isa<Argument>(CurrentValue))
                InsertPt = &F.getEntryBlock().front();
            else if (auto* PN = dyn_cast<PHINode>(CurrentValue))
                InsertPt = &*PN->getParent()->getFirstInsertionPt();
            else
                InsertPt = cast<Instruction>(CurrentValue)->getNextNode();

            RTB.SetInsertPoint(InsertPt);
            RTB.getSpillValue(CurrentValue, CurIdx);
        }

        if (CurrentBlock != E.userBlock())
        {
            CurrentBlock = E.userBlock();
            RTB.SetInsertPoint(&*CurrentBlock->getFirstInsertionPt());
            CurrentReload = RTB.getFillValue(
                CurrentValue->getType(),
                CurIdx,
                VALUE_NAME(CurrentValue->getName() + Twine(".fill")));
        }

        // If we have a single edge PHINode, remove it and replace it with a
        // reload. We already took care of multi edge PHINodes by rewriting them
        // in the rewritePHIs function.
        if (auto* PN = dyn_cast<PHINode>(E.user()))
        {
            IGC_ASSERT_MESSAGE(PN->getNumIncomingValues() == 1,
                "unexpected number of incoming values in the PHINode");
            PN->replaceAllUsesWith(CurrentReload);
            PN->eraseFromParent();
            SpillSlots.insert(std::make_pair(CurrentReload, CurIdx));
            continue;
        }

        // Replace all uses of CurrentValue in the current instruction with reload.
        E.user()->replaceUsesOfWith(CurrentValue, CurrentReload);
    }
}

// For every use of the value that is across suspend point, recreate that value
// after a suspend point.
void rewriteMaterializableInstructions(const SmallVector<Spill, 8>& Spills)
{
    BasicBlock* CurrentBlock = nullptr;
    Instruction* CurrentMaterialization = nullptr;
    Instruction* CurrentDef = nullptr;

    for (auto const& E : Spills)
    {
        // If it is a new definition, update CurrentXXX variables.
        if (CurrentDef != E.def())
        {
            CurrentDef = cast<Instruction>(E.def());
            CurrentBlock = nullptr;
            CurrentMaterialization = nullptr;
        }

        // If we have not seen this block, materialize the value.
        if (CurrentBlock != E.userBlock())
        {
            CurrentBlock = E.userBlock();
            CurrentMaterialization = CurrentDef->clone();
            CurrentMaterialization->setName(CurrentDef->getName());
            CurrentMaterialization->insertBefore(
                &*CurrentBlock->getFirstInsertionPt());
        }

        if (auto* PN = dyn_cast<PHINode>(E.user()))
        {
            IGC_ASSERT_MESSAGE(PN->getNumIncomingValues() == 1,
                "unexpected number of incoming values in the PHINode");
            PN->replaceAllUsesWith(CurrentMaterialization);
            PN->eraseFromParent();
            continue;
        }

        // Replace all uses of CurrentDef in the current instruction with the
        // CurrentMaterialization for the block.
        E.user()->replaceUsesOfWith(CurrentDef, CurrentMaterialization);
    }
}

RematChecker::RematChecker(CodeGenContext& Ctx, RematStage Stage) :
    Ctx(Ctx), Stage(Stage) {}

bool RematChecker::isReadOnly(const Value* Ptr) const
{
    uint32_t Addrspace = Ptr->getType()->getPointerAddressSpace();
    BufferType BufTy = GetBufferType(Addrspace);
    BufferAccessType Access = getDefaultAccessType(BufTy);
    return (Access == BufferAccessType::ACCESS_READ);
}

bool RematChecker::materializable(const Instruction& I) const
{
    if (isa<CastInst>(&I) || isa<GetElementPtrInst>(&I) ||
        isa<BinaryOperator>(&I) || isa<CmpInst>(&I) || isa<SelectInst>(&I) ||
        isa<ExtractElementInst>(&I))
    {
        return true;
    }

    if (auto* LI = dyn_cast<LoadInst>(&I))
        return (LI->getPointerAddressSpace() == ADDRESS_SPACE_CONSTANT);

    if (auto* GII = dyn_cast<GenIntrinsicInst>(&I))
    {
        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_GlobalRootSignatureValue:
        case GenISAIntrinsic::GenISA_DispatchRayIndex:
        case GenISAIntrinsic::GenISA_DispatchDimensions:
        case GenISAIntrinsic::GenISA_frc:
        case GenISAIntrinsic::GenISA_ROUNDNE:
            return true;
        case GenISAIntrinsic::GenISA_ldraw_indexed:
        case GenISAIntrinsic::GenISA_ldrawvector_indexed:
            return isReadOnly(cast<LdRawIntrinsic>(GII)->getResourceValue());
        case GenISAIntrinsic::GenISA_ldptr:
            return true;
        default:
            return false;
        }
    }

    if (auto* II = dyn_cast<IntrinsicInst>(&I))
    {
        switch (II->getIntrinsicID())
        {
        case Intrinsic::floor:
        case Intrinsic::minnum:
        case Intrinsic::maxnum:
        case Intrinsic::fabs:
        case Intrinsic::sqrt:
            return true;
        default:
            return false;
        }
    }

    return false;
}

bool RematChecker::canFullyRemat(
    Instruction* I,
    std::vector<Instruction*>& Insts,
    std::unordered_set<Instruction*>& Visited,
    unsigned StartDepth,
    unsigned Depth,
    ValueToValueMapTy* VM) const
{
    if (!Visited.insert(I).second)
        return true;

    if (StartDepth != Depth && VM && VM->count(I) != 0)
        return true;

    if (Depth == 0 || !materializable(*I))
        return false;

    for (auto& Op : I->operands())
    {
        if (isa<Constant>(Op))
            continue;
        if (auto* A = dyn_cast<Argument>(Op);
            A && Stage == RematStage::MID)
        {
            auto& F = *I->getFunction();
            if (ArgQuery{ F, Ctx }.getPayloadArg(&F) == A)
                continue;
        }

        auto* OpI = dyn_cast<Instruction>(Op);
        if (!OpI)
            return false;

        if (!canFullyRemat(OpI, Insts, Visited, StartDepth, Depth - 1, VM))
            return false;
    }

    Insts.push_back(I);
    return true;
}

Optional<std::vector<Instruction*>>
RematChecker::canFullyRemat(
    Instruction* I,
    uint32_t Threshold,
    ValueToValueMapTy* VM) const
{
    std::vector<Instruction*> Insts;
    std::unordered_set<Instruction*> Visited;
    if (!canFullyRemat(I, Insts, Visited, Threshold, Threshold, VM))
        return None;

    return Insts;
}

} // namespace IGC
