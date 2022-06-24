/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// Simple utility functions to track the usage of allocas and rewrite their
// types.  This is useful for promoting global memory allocas to either
// stateful or scratch space.
//
//===----------------------------------------------------------------------===//

#include "AllocaTracking.h"
#include "RTBuilder.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvmWrapper/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace AllocaTracking {

// recursively trace all the users of 'I' and check whether they would be safe
// to transform.  Build up the list in 'Insts'.
bool processAlloca(
    Instruction* I,
    bool AllowCapture,
    SmallVector<Instruction*, 4> &Insts,
    DenseSet<CallInst*> &DeferredInsts)
{
    Insts.push_back(I);
    for (auto* U : I->users())
    {
        auto* UI = cast<Instruction>(U);
        switch (UI->getOpcode())
        {
        case Instruction::GetElementPtr:
        case Instruction::BitCast:
            if (!processAlloca(UI, AllowCapture, Insts, DeferredInsts))
                return false;
            break;
        case Instruction::Load:
            break;
        case Instruction::Store:
            if (I == cast<StoreInst>(UI)->getValueOperand() && !AllowCapture)
                return false;
            break;
        case Instruction::Call:
            if (auto* GII = dyn_cast<GenIntrinsicInst>(UI);
                     GII && AllowCapture)
            {
                bool Legal = false;
                switch (GII->getIntrinsicID())
                {
                case GenISAIntrinsic::GenISA_TraceRayAsyncHL:
                case GenISAIntrinsic::GenISA_CallShaderHL:
                    Legal = true;
                    break;
                default:
                    return false;
                }
                if (Legal)
                {
                    DeferredInsts.insert(GII);
                    break;
                }
            }
            else if (auto *II = dyn_cast<IntrinsicInst>(UI))
            {
                bool Legal = false;
                switch (II->getIntrinsicID())
                {
                case Intrinsic::memcpy:
                case Intrinsic::lifetime_start:
                case Intrinsic::lifetime_end:
                    Legal = true;
                    break;
                default:
                    return false;
                }
                if (Legal)
                {
                    DeferredInsts.insert(II);
                    break;
                }
            }
            return false;
        default:
            return false;
        }
    }

    return true;
}

// Given a safe list, transform each of their types.
void rewriteTypes(
    uint32_t NewAddrSpace,
    SmallVector<Instruction*, 4> &Insts,
    DenseSet<CallInst*> &DeferredInsts)
{
    for (auto* I : Insts)
    {
        switch (I->getOpcode())
        {
        case Instruction::GetElementPtr:
        case Instruction::BitCast:
        case Instruction::Alloca:
        {
            auto* EltTy = I->getType()->getPointerElementType();
            auto* NewTy = PointerType::get(EltTy, NewAddrSpace);
            I->mutateType(NewTy);
            break;
        }
        default:
            break;
        }
    }

    for (auto* II : DeferredInsts)
    {
        auto* FTy = II->getFunctionType();
        IGC_ASSERT_MESSAGE(FTy->getReturnType()->isVoidTy(),
            "Only handles void right now!");

        SmallVector<Type*, 4> Tys;
        for (auto &Op : IGCLLVM::args(II))
            Tys.push_back(Op->getType());

        auto* NewFTy = FunctionType::get(
            FTy->getReturnType(), Tys, FTy->isVarArg());

        Function* CurFunc = II->getCalledFunction();
        Function* NewFunc = nullptr;

        if (isa<GenIntrinsicInst>(II))
        {
            NewFunc = Function::Create(
                NewFTy,
                CurFunc->getLinkage(),
                CurFunc->getName(),
                CurFunc->getParent());
        }
        else if (isa<IntrinsicInst>(II))
        {
            NewFunc = RTBuilder::updateIntrinsicMangle(
                NewFTy, *CurFunc);
        }
        else
        {
            IGC_ASSERT_MESSAGE(0, "not yet handled!");
        }

        II->setCalledFunction(NewFunc);
    }
}

} // namespace AllocaTracking
