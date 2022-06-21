/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ResolveSampledImageBuiltins.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvmWrapper/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-image-sampler-resolution"
#define PASS_DESCRIPTION "Resolves getter builtins operating on VMEImageIntel/SampledImage objects"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResolveSampledImageBuiltins, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ResolveSampledImageBuiltins, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ResolveSampledImageBuiltins::ID = 0;

const llvm::StringRef ResolveSampledImageBuiltins::GET_IMAGE = "__builtin_IB_get_image";
const llvm::StringRef ResolveSampledImageBuiltins::GET_SAMPLER = "__builtin_IB_get_sampler";

ResolveSampledImageBuiltins::ResolveSampledImageBuiltins() : ModulePass(ID)
{
    initializeResolveSampledImageBuiltinsPass(*PassRegistry::getPassRegistry());
}

bool ResolveSampledImageBuiltins::runOnModule(Module& M) {
    m_changed = false;
    visit(M);

    for (auto builtin : m_builtinsToRemove)
    {
        if (!builtin->use_empty())
        {
            SmallVector<Instruction*, 4> usersToErase;
            for (auto* user : builtin->users())
                usersToErase.push_back(cast<Instruction>(user));

            for (auto* user : usersToErase)
                user->eraseFromParent();
        }
        builtin->eraseFromParent();
    }

    return m_changed;
}

void ResolveSampledImageBuiltins::visitCallInst(CallInst& CI)
{
    if (!CI.getCalledFunction())
    {
        return;
    }

    Value* res = nullptr;
    StringRef funcName = CI.getCalledFunction()->getName();

    if (funcName.equals(ResolveSampledImageBuiltins::GET_IMAGE))
    {
        res = lowerGetImage(CI);
    }
    else if (funcName.equals(ResolveSampledImageBuiltins::GET_SAMPLER))
    {
        res = lowerGetSampler(CI);
    }
    else
    {
        return;
    }

    CI.replaceAllUsesWith(res);
    CI.eraseFromParent();
    m_changed = true;
}

// Transform the following sequence:
//
// %opaque = call spir_func %spirv.{VmeImageINTEL|SampledImage} addrspace(1)* @__builtin_spirv_{OpVmeImageINTEL|OpSampledImage}(
//     %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %image, %spirv.Sampler* %sampler)
// %queried_image = call spir_func i64 @__builtin_IB_get_image(%spirv.{VmeImageINTEL|SampledImage} addrspace(1)* %opaque)
//
// Into:
//
// %queried_image = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %image to i64

Value* ResolveSampledImageBuiltins::lowerGetImage(CallInst& CI)
{
    IGC_ASSERT(IGCLLVM::getNumArgOperands(&CI) == 1);

    CallInst* callReturningOpaque = dyn_cast<CallInst>(CI.getArgOperand(0)->stripPointerCasts());
    IGC_ASSERT(callReturningOpaque);

    m_builtinsToRemove.insert(callReturningOpaque);

    Value* image = callReturningOpaque->getArgOperand(0);
    IGC_ASSERT(isa<PointerType>(image->getType()));

    return PtrToIntInst::Create(
        Instruction::PtrToInt,
        image,
        Type::getInt64Ty(CI.getContext()),
        "",
        &CI);
}

// Transforms the following sequence:
//
// %sampler = call %spirv.Sampler* @__translate_sampler_initializer(i32 16)
// %opaque = call spir_func %spirv.{VmeImageINTEL|SampledImage} addrspace(1)* @__builtin_spirv_{OpVmeImageINTEL|OpSampledImage}(
//     %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %image, %spirv.Sampler* %sampler)
// %queried_sampler = call spir_func i64 @__builtin_IB_get_sampler(%spirv.{VmeImageINTEL|SampledImage} addrspace(1)* %opaque)
//
// Into:
//
// %queried_sampler = zext i32 16 to i64

Value* ResolveSampledImageBuiltins::lowerGetSampler(CallInst& CI)
{
    IGC_ASSERT(IGCLLVM::getNumArgOperands(&CI) == 1);
    CallInst* callReturningOpaque = dyn_cast<CallInst>(CI.getArgOperand(0)->stripPointerCasts());
    IGC_ASSERT(callReturningOpaque);

    m_builtinsToRemove.insert(callReturningOpaque);

    Value* samplerArg = callReturningOpaque->getArgOperand(1);
    if (CallInst* samplerInitializer = dyn_cast<CallInst>(samplerArg))
    {
        IGC_ASSERT(samplerInitializer->getCalledFunction()->getName() == "__translate_sampler_initializer");
        return ZExtInst::Create(
            Instruction::ZExt,
            samplerInitializer->getArgOperand(0),
            Type::getInt64Ty(CI.getContext()),
            "",
            &CI);
    }
    else
    {
        IGC_ASSERT(samplerArg->getType()->isPointerTy());
        return PtrToIntInst::Create(
            Instruction::PtrToInt,
            samplerArg,
            Type::getInt64Ty(CI.getContext()),
            "",
            &CI);
    }
}
