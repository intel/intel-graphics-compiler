/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/RuntimeValueLegalizationPass.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/DerivedTypes.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-runtimevalue-legalization-pass"
#define PASS_DESCRIPTION "Shader runtime value legalization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RuntimeValueLegalizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(RuntimeValueLegalizationPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{

char RuntimeValueLegalizationPass::ID = 0;

////////////////////////////////////////////////////////////////////////////
RuntimeValueLegalizationPass::RuntimeValueLegalizationPass() : llvm::ModulePass(ID)
{
    initializeRuntimeValueLegalizationPassPass(*llvm::PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////////
void RuntimeValueLegalizationPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.setPreservesCFG();
}

////////////////////////////////////////////////////////////////////////////
// @brief Get all RuntimeValue calls. Calls are ordered according to the place
// in the basic block and instruction lists. Split calls into two separate
// sets: one for RuntimeValue calls representing vectors of scalars and
// other one for the rest of RuntimeValue calls
static void GetAllRuntimeValueCalls(
    llvm::Module& module,
    std::vector<llvm::GenIntrinsicInst*>& runtimeValueCalls,
    std::vector<llvm::GenIntrinsicInst*>& runtimeValueVectorCalls)
{
    for (llvm::Function& F : module)
    {
        for (llvm::BasicBlock& B : F)
        {
            for (llvm::Instruction& I : B)
            {
                llvm::GenIntrinsicInst* intr = llvm::dyn_cast<llvm::GenIntrinsicInst>(&I);
                if (intr &&
                    intr->getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_RuntimeValue)
                {
                    intr->getType()->isVectorTy() ?
                        runtimeValueVectorCalls.push_back(intr) :
                        runtimeValueCalls.push_back(intr);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////
// @brief Legalizes RuntimeValue calls for push analysis. RuntimeValue calls
// returning single scalars are converted to extracts of elements
// from corresponding RuntimeValue vector, but only in case such a vector exists.
// Only 32-bit and 64-bit RuntimeValues are supported at the moment.
//
// Replace:
//   %1 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//   %3 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
//  %14 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 5)
// with:
//   %4 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//   %1 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//   %2 = extractelement <3 x i32> %1, i32 0
//  %15 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//  %16 = extractelement <3 x i32> %15, i32 1
bool RuntimeValueLegalizationPass::runOnModule(llvm::Module& module)
{
    bool shaderModified = false;

    std::vector<llvm::GenIntrinsicInst*> runtimeValueCalls;
    std::vector<llvm::GenIntrinsicInst*> runtimeValueVectorCalls;
    GetAllRuntimeValueCalls(module, runtimeValueCalls, runtimeValueVectorCalls);
    if (!runtimeValueVectorCalls.empty() && !runtimeValueCalls.empty())
    {
        // Loop through all RuntimeValue calls
        for (auto rvCall : runtimeValueCalls)
        {
            llvm::CallInst* const callToResolve = llvm::cast<llvm::CallInst>(rvCall);
            IGC_ASSERT(nullptr != callToResolve);

            llvm::Value* const callToResolveOffsetInDwords = callToResolve->getArgOperand(0);
            if (llvm::isa<llvm::ConstantInt>(callToResolveOffsetInDwords))
            {
                const uint32_t resolvedOffset = int_cast<uint32_t>(
                    cast<ConstantInt>(callToResolveOffsetInDwords)->getZExtValue());

                // Loop through all RuntimeValue vector calls
                for (auto rvVectorCall : runtimeValueVectorCalls)
                {
                    llvm::CallInst* const vecCall = llvm::cast<llvm::CallInst>(rvVectorCall);
                    IGC_ASSERT(nullptr != vecCall);

                    llvm::Value* const arrayOffsetInDwords = vecCall->getArgOperand(0);
                    if (llvm::isa<llvm::ConstantInt>(arrayOffsetInDwords) &&
                        llvm::isa<IGCLLVM::FixedVectorType>(vecCall->getType()))
                    {
                        IGCLLVM::FixedVectorType* const fixedVectorTy =
                            cast<IGCLLVM::FixedVectorType>(vecCall->getType());
                        // Types of calls should match
                        if (callToResolve->getType() == fixedVectorTy->getElementType())
                        {
                            // Only 32-bit and 64-bit values are supported at the moment
                            if (callToResolve->getType()->getPrimitiveSizeInBits() == 32 ||
                                callToResolve->getType()->getPrimitiveSizeInBits() == 64)
                            {
                                bool is64bit = callToResolve->getType()->getPrimitiveSizeInBits() == 64;

                                const uint32_t arrayOffset = int_cast<uint32_t>(
                                    cast<ConstantInt>(arrayOffsetInDwords)->getZExtValue());
                                uint32_t numElements = int_cast<uint32_t>(fixedVectorTy->getNumElements());
                                if (is64bit)
                                {
                                    numElements *= 2;
                                }

                                if (resolvedOffset >= arrayOffset && resolvedOffset < (arrayOffset + numElements))
                                {
                                    llvm::IRBuilder<> builder(callToResolve);
                                    builder.SetInsertPoint(callToResolve);

                                    Function* runtimeValueFunc = GenISAIntrinsic::getDeclaration(&module,
                                        GenISAIntrinsic::GenISA_RuntimeValue,
                                        vecCall->getType());

                                    // Create new RuntimeValue call
                                    Value* ciValue = builder.CreateCall(runtimeValueFunc, builder.getInt32(arrayOffset));
                                    // Extract element
                                    uint32_t offset = resolvedOffset - arrayOffset;
                                    if (is64bit)
                                    {
                                        offset /= 2;
                                    }
                                    ciValue = builder.CreateExtractElement(ciValue, builder.getInt32(offset));

                                    callToResolve->replaceAllUsesWith(ciValue);
                                    callToResolve->eraseFromParent();

                                    shaderModified = true;
                                    break;
                                }
                            }
                            else
                            {
                                IGC_ASSERT_MESSAGE(0, "Only 32-bit and 64-bit values are supported at the moment");
                            }
                        }
                    }
                }
            }
        }
    }

    return shaderModified;
}

}
