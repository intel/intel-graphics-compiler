/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ResolveSampledImageBuiltins.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/MDFrameWork.h"
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
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
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
    modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
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

// Resolve __builtin_IB_get_image
//
// Case 1: both image and sampler are passed as kernel arguments:
//   Transform the following sequence:
//
//     %opaque = call spir_func %spirv.{VmeImageINTEL|SampledImage} addrspace(1)* @__builtin_spirv_{OpVmeImageINTEL|OpSampledImage}(
//       %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %image, %spirv.Sampler* %sampler)
//     %queried_image = call spir_func i64 @__builtin_IB_get_image(%spirv.{VmeImageINTEL|SampledImage} addrspace(1)* %opaque)
//
//   Into:
//
//     %queried_image = ptrtoint %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %image to i64
//
// Case 2: only sampled image value is present.
//   Transform the following sequence:
//
//     %image = bitcast %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %sampled_image to i8 addrspace(1)*
//     %queried_image = call spir_func i64 @__builtin_IB_get_image(i8 addrspace(1)* %image)
//
//   Into:
//
//     %queried_image = ptrtoint %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %image to i64
Value* ResolveSampledImageBuiltins::lowerGetImage(CallInst& CI)
{
    IGC_ASSERT(IGCLLVM::getNumArgOperands(&CI) == 1);

    Value* image = CI.getArgOperand(0)->stripPointerCasts();
    if (auto *callReturningOpaque = dyn_cast<CallInst>(image))
    {
        m_builtinsToRemove.insert(callReturningOpaque);
        image = callReturningOpaque->getArgOperand(0);
    }

    IGC_ASSERT(isa<PointerType>(image->getType()));

    return PtrToIntInst::Create(
        Instruction::PtrToInt,
        image,
        Type::getInt64Ty(CI.getContext()),
        "",
        &CI);
}

// Resolve __builtin_IB_get_sampler
Value* ResolveSampledImageBuiltins::lowerGetSampler(CallInst& CI)
{
    IGC_ASSERT(IGCLLVM::getNumArgOperands(&CI) == 1);
    Value *image = CI.getArgOperand(0)->stripPointerCasts();
    Value *samplerArg = nullptr;
    if (auto *callReturningOpaque = dyn_cast<CallInst>(image))
    {
        m_builtinsToRemove.insert(callReturningOpaque);
        samplerArg = callReturningOpaque->getArgOperand(1);
    }

    auto *Int64Ty = Type::getInt64Ty(CI.getContext());

    // In the case of SYCL bindless sampler, transform the following sequence:
    //
    //   %image = bitcast %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %sampled_image to i8 addrspace(1)*
    //   %queried_sampler = call spir_func i64 @__builtin_IB_get_image(i8 addrspace(1)* %image)
    //
    // Into:
    //
    //   %image_offset = ptrtoint %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %image to i64
    //   %sampler_offset = add i64 %image_offset, 128
    //   %queried_sampler = or i64 %sampler_offset, 1
    if (!samplerArg)
    {
        IGC_ASSERT(modMD->UseBindlessImage);
        IGC_ASSERT(image->getType()->isPointerTy());
        Value *imageOffset = PtrToIntInst::Create(
            Instruction::PtrToInt,
            image,
            Int64Ty,
            "",
            &CI);
        // When sampled image is created in a single API call, e.g. SYCL bindless image,
        // bindless surface state heap layout is
        // | image state | image implicit args state | sampler state | redescribed image state | ...
        // Sampler state offset is addition of image state offset, size of
        // image state and size of image implicit args state.
        // Both size of image state and image implicit args state are 64 bytes.
        constexpr uint64_t surfaceStateSize = 64;
        auto *stateSizeValue = ConstantInt::get(Int64Ty, surfaceStateSize * 2);
        auto *samplerOffset = BinaryOperator::CreateAdd(imageOffset, stateSizeValue, "sampler_offset", &CI);
        // Set bit-field 0 to 1 to select Bindless Sampler State Base Address.
        return BinaryOperator::CreateOr(samplerOffset, ConstantInt::get(Int64Ty, 1), "", &CI);
    }

    // Transforms the following sequence:
    //
    //   %sampler = call %spirv.Sampler* @__translate_sampler_initializer(i32 16)
    //   %opaque = call spir_func %spirv.{VmeImageINTEL|SampledImage} addrspace(1)* @__builtin_spirv_{OpVmeImageINTEL|OpSampledImage}(
    //     %spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %image, %spirv.Sampler* %sampler)
    //   %queried_sampler = call spir_func i64 @__builtin_IB_get_sampler(%spirv.{VmeImageINTEL|SampledImage} addrspace(1)* %opaque)
    //
    // Into:
    //
    //   %queried_sampler = zext i32 16 to i64
    if (CallInst* samplerInitializer = dyn_cast<CallInst>(samplerArg))
    {
        // The __bindless_sampler_initializer calls are handled by PrepareInlineSamplerForBindless
        // and ResolveInlineSamplerForBindless. They are meant to be replaced with inline sampler implicit arg.
        if (!(samplerInitializer->getCalledFunction()->getName() == "__bindless_sampler_initializer"))
        {
            IGC_ASSERT(samplerInitializer->getCalledFunction()->getName() == "__translate_sampler_initializer");
            return ZExtInst::Create(
                Instruction::ZExt,
                samplerInitializer->getArgOperand(0),
                Int64Ty,
                "",
                &CI);
        }
    }

    // In the case of sampler as kernel argument, transform the following sequence:
    //
    //   %sampled_image = call spir_func %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* @__spirv_SampledImage(%spirv.Image._void_1_0_0_0_0_0_0 addrspace(1)* %img, %spirv.Sampler addrspace(2)* %sampler)
    //   %0 = bitcast %spirv.SampledImage._void_1_0_0_0_0_0_0 addrspace(1)* %sampled_image to i8 addrspace(1)*
    //   %queried_sampler = call spir_func i64 @__builtin_IB_get_sampler(i8 addrspace(1)* %0)
    //
    // Into:
    //
    //   %sampler_offset = ptrtoint %spirv.Sampler addrspace(2)* %sampler to i64
    //   %queried_sampler = or i64 %sampler_offset, 1    // if UseBindlessImage is true
    IGC_ASSERT(samplerArg->getType()->isPointerTy());
    Value *samplerOffset = PtrToIntInst::Create(
        Instruction::PtrToInt,
        samplerArg,
        Int64Ty,
        "",
        &CI);
    if (modMD->UseBindlessImage) {
        // Set bit-field 0 to 1 to select Bindless Sampler State Base Address.
        samplerOffset = BinaryOperator::CreateOr(samplerOffset, ConstantInt::get(Int64Ty, 1), "", &CI);
    }
    return samplerOffset;
}
