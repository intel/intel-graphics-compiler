/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/LowerByValAttribute.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/Instructions.h>
#include <llvmWrapper/IR/IRBuilder.h>

using namespace llvm;
using namespace IGC;

char LowerByValAttribute::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-lower-byval-attribute"
#define PASS_DESCRIPTION "LowerByValAttribute"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowerByValAttribute, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(LowerByValAttribute, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// This pass lowers usage of `byval` attribute into an explicit copy (alloca + memcpy).
//
// According to the LLVM documentation:
// "The attribute implies that a hidden copy of the pointee is made between the caller
//  and the callee, so the callee is unable to modify the value in the caller."
//
// The "hidden copy" means that frontend compilers don't generate an explicit copy in SPIRV,
// but just decorates a pointer with ByVal. It moves the responsibility, for generating a copy,
// to the backend compiler (IGC).

LowerByValAttribute::LowerByValAttribute(void) : FunctionPass(ID) {
  initializeLowerByValAttributePass(*PassRegistry::getPassRegistry());
}

bool LowerByValAttribute::runOnFunction(Function &F) {
  visit(F);

  return m_changed;
}

void LowerByValAttribute::visitCallInst(CallInst &CI) {
  auto &DL = CI.getModule()->getDataLayout();

  // Skip intrinsics
  auto F = CI.getCalledFunction();
  if (F && F->isDeclaration())
    return;

  for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(&CI); ++i) {
    Value *OpI = CI.getArgOperand(i);
    if (isa<UndefValue>(OpI))
      continue;

    Type *OpITy = OpI->getType();
    if (!OpITy->isPointerTy())
      continue;

    if (CI.paramHasAttr(i, llvm::Attribute::ByVal) && !CI.paramHasAttr(i, llvm::Attribute::ReadOnly)) {
      Type *ElTy = CI.getParamByValType(i);
      IGCLLVM::IRBuilder<> builder(&CI);
      Value *AI = builder.CreateAlloca(ElTy);
      builder.CreateMemCpy(AI, OpI, DL.getTypeAllocSize(ElTy), DL.getABITypeAlign(ElTy).value());
      auto AC = builder.CreateAddrSpaceCast(AI, OpITy);
      CI.replaceUsesOfWith(OpI, AC);
      m_changed = true;
    }
  }
}
