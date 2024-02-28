/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BufferBoundsCheckingPatcher.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/IR/Function.h"
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Mangler.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Regex.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-buffer-bounds-checking-patcher"
#define PASS_DESCRIPTION "Buffer bounds checking - patcher"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BufferBoundsCheckingPatcher, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(BufferBoundsCheckingPatcher, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char BufferBoundsCheckingPatcher::ID = 0;

BufferBoundsCheckingPatcher::BufferBoundsCheckingPatcher() : FunctionPass(ID)
{
    initializeBufferBoundsCheckingPatcherPass(*PassRegistry::getPassRegistry());
}

bool BufferBoundsCheckingPatcher::runOnFunction(Function& function)
{
    modified = false;

    metadataUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    implicitArgs = new ImplicitArgs(function, metadataUtils);

    if (!isEntryFunc(metadataUtils, &function))
    {
        return false;
    }

    visit(function);
    return modified;
}

void BufferBoundsCheckingPatcher::visitICmpInst(ICmpInst& icmp)
{
    modified |= patchInstruction(&icmp);
}

void BufferBoundsCheckingPatcher::visitSub(BinaryOperator& sub)
{
    modified |= patchInstruction(&sub);
}

bool BufferBoundsCheckingPatcher::patchInstruction(llvm::Instruction* instruction)
{
    if (!hasPatchInfo(instruction))
    {
        return false;
    }

    auto patchInfo = getPatchInfo(instruction);
    auto bufferSize = getBufferSizeArg(instruction->getFunction(), patchInfo.implicitArgBufferSizeIndex);
    //auto bufferSize = ConstantInt::get(Type::getInt64Ty(instruction->getContext()), 64); // Only for testing purposes
    instruction->setOperand(patchInfo.operandIndex, bufferSize);
    instruction->setMetadata("bufferboundschecking.patch", nullptr);
    return true;
}

void BufferBoundsCheckingPatcher::addPatchInfo(Instruction* instruction, const PatchInfo& patchInfo)
{
    MDNode* metadata = MDNode::get(instruction->getContext(), {
        ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(instruction->getContext()), patchInfo.operandIndex)),
        ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(instruction->getContext()), patchInfo.implicitArgBufferSizeIndex)),
    });

    instruction->setMetadata("bufferboundschecking.patch", metadata);
}

bool BufferBoundsCheckingPatcher::hasPatchInfo(Instruction* instruction)
{
    return instruction->getMetadata("bufferboundschecking.patch") != nullptr;
}

BufferBoundsCheckingPatcher::PatchInfo BufferBoundsCheckingPatcher::getPatchInfo(Instruction* instruction)
{
    MDNode* metadata = instruction->getMetadata("bufferboundschecking.patch");
    return {
        uint32_t(dyn_cast<ConstantInt>(dyn_cast<ConstantAsMetadata>(metadata->getOperand(0))->getValue())->getZExtValue()),
        uint32_t(dyn_cast<ConstantInt>(dyn_cast<ConstantAsMetadata>(metadata->getOperand(1))->getValue())->getZExtValue()),
    };
}

Argument* BufferBoundsCheckingPatcher::getBufferSizeArg(Function* function, uint32_t n)
{
    uint32_t index = implicitArgs->getNumberedArgIndex(ImplicitArg::ArgType::BUFFER_SIZE, n);
    if (index >= implicitArgs->size())
    {
        IGC_ASSERT_MESSAGE(0, "Implicit argument for BUFFER_SIZE is out of range!");
        return nullptr;
    }

    return IGCLLVM::getArg(*function, function->arg_size() - implicitArgs->size() + index);
}
