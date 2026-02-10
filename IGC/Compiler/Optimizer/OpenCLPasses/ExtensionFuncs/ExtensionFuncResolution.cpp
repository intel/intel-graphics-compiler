/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/ExtensionFuncs/ExtensionFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtensionFuncs/ExtensionFuncsAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/IRBuilder.h>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-extension-funcs-resolution"
#define PASS_DESCRIPTION "Resolves extension function"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ExtensionFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ExtensionFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ExtensionFuncsResolution::ID = 0;

ExtensionFuncsResolution::ExtensionFuncsResolution() : FunctionPass(ID), m_implicitArgs() {
  initializeExtensionFuncsResolutionPass(*PassRegistry::getPassRegistry());
}

bool ExtensionFuncsResolution::runOnFunction(Function &F) {
  m_changed = false;
  m_implicitArgs = ImplicitArgs(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
  visit(F);
  return m_changed;
}

void ExtensionFuncsResolution::visitCallInst(CallInst &CI) {
  if (!CI.getCalledFunction()) {
    return;
  }

  StringRef funcName = CI.getCalledFunction()->getName();
  Function &F = *(CI.getParent()->getParent());

  ImplicitArg::ArgType argType;
  if (funcName.equals(ExtensionFuncsAnalysis::VME_MB_BLOCK_TYPE)) {
    argType = ImplicitArg::VME_MB_BLOCK_TYPE;
  } else if (funcName.equals(ExtensionFuncsAnalysis::VME_SUBPIXEL_MODE)) {
    argType = ImplicitArg::VME_SUBPIXEL_MODE;
  } else if (funcName.equals(ExtensionFuncsAnalysis::VME_SAD_ADJUST_MODE)) {
    argType = ImplicitArg::VME_SAD_ADJUST_MODE;
  } else if (funcName.equals(ExtensionFuncsAnalysis::VME_SEARCH_PATH_TYPE)) {
    argType = ImplicitArg::VME_SEARCH_PATH_TYPE;
  } else if (funcName.startswith(ExtensionFuncsAnalysis::VME_HELPER_GET_HANDLE)) {
    // Load from the opaque vme pointer and return the a vector with values.
    IGC_ASSERT(IGCLLVM::getNumArgOperands(&CI) == 1);
    IGCLLVM::IRBuilder<> builder(&CI);
    Type *retType = CI.getType();
    IGC_ASSERT(retType->isVectorTy() || retType->isIntegerTy());
    PointerType *ptrType = PointerType::get(retType, 0);
    auto bitcastInst = builder.CreateBitCast(CI.getArgOperand(0), ptrType);
    auto ret = builder.CreateLoad(retType, bitcastInst);
    CI.replaceAllUsesWith(ret);
    CI.eraseFromParent();
    return;
  } else if (funcName.startswith(ExtensionFuncsAnalysis::VME_HELPER_GET_AS)) {
    // Store the VME values and return an opaque vme pointer.
    IGC_ASSERT(IGCLLVM::getNumArgOperands(&CI) == 1);
    IGCLLVM::IRBuilder<> builder(&*CI.getParent()->getParent()->begin()->getFirstInsertionPt());
    Type *retType = CI.getType();
    Value *arg = CI.getArgOperand(0);
    IGC_ASSERT(arg->getType()->isVectorTy() || arg->getType()->isIntegerTy());
    AllocaInst *allocaInst = builder.CreateAlloca(arg->getType());
    builder.SetInsertPoint(&CI);
    builder.CreateStore(arg, allocaInst);
    Value *bitcastInst = builder.CreateBitCast(allocaInst, retType);
    CI.replaceAllUsesWith(bitcastInst);
    CI.eraseFromParent();
    return;
  } else {
    // Non VME function, do nothing
    return;
  }

  Value *vmeRes = m_implicitArgs.getImplicitArg(F, argType);

  // Replace original VME call instruction with the appropriate argument, example:

  // Recieves:
  // call i32 @__builtin_IB_vme_subpixel_mode()

  // Creates:
  // %vmeSubpixelMode

  CI.replaceAllUsesWith(vmeRes);
  CI.eraseFromParent();

  m_changed = true;
}
