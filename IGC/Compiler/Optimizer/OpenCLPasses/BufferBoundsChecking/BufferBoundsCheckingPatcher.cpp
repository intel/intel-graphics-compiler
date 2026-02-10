/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BufferBoundsCheckingPatcher.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Regex.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Function.h"

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

BufferBoundsCheckingPatcher::BufferBoundsCheckingPatcher() : ModulePass(ID) {
  initializeBufferBoundsCheckingPatcherPass(*PassRegistry::getPassRegistry());
}

bool BufferBoundsCheckingPatcher::runOnModule(Module &M) {
  metadataUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  for (auto &function : M) {
    if (function.isDeclaration()) {
      continue;
    }

    implicitArgs = new ImplicitArgs(function, metadataUtils);
    if (!isEntryFunc(metadataUtils, &function)) {
      return false;
    }

    for (auto &instruction : instructions(&function)) {
      if (auto call = dyn_cast<CallInst>(&instruction)) {
        auto function = call->getCalledFunction();
        if (!function || function->getName() != BUFFER_SIZE_PLACEHOLDER_FUNCTION_NAME) {
          continue;
        }

        auto bufferSize =
            getBufferSizeArg(call->getFunction(), (uint32_t)cast<ConstantInt>(call->getArgOperand(0))->getSExtValue());
        // auto bufferSize = ConstantInt::get(Type::getInt64Ty(call->getContext()), 64); // Only for testing purposes
        call->replaceAllUsesWith(bufferSize);
        toRemove.push_back(call);
      }
    }
  }

  for (const auto &it : toRemove) {
    it->eraseFromParent();
  }

  auto bufferSizePlaceholderFunction = M.getFunction(BUFFER_SIZE_PLACEHOLDER_FUNCTION_NAME);
  if (bufferSizePlaceholderFunction) {
    bufferSizePlaceholderFunction->eraseFromParent();
    return true;
  }

  return !toRemove.empty();
}

Argument *BufferBoundsCheckingPatcher::getBufferSizeArg(Function *function, uint32_t n) {
  uint32_t index = implicitArgs->getNumberedArgIndex(ImplicitArg::ArgType::BUFFER_SIZE, n);
  if (index >= implicitArgs->size()) {
    IGC_ASSERT_MESSAGE(0, "Implicit argument for BUFFER_SIZE is out of range!");
    return nullptr;
  }

  return IGCLLVM::getArg(*function, function->arg_size() - implicitArgs->size() + index);
}
