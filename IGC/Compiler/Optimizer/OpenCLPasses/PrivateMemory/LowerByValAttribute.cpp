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
#include <llvm/IR/IntrinsicInst.h>
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

// Frontends emit an explicit copy themselves when the argument can't be passed directly, e.g. when it lives
// in another address space or is a call result. Such a temporary is written right before the call and never
// read by the caller, so the callee can't change anything the caller observes and a second copy isn't
// needed. Writes have to precede the call in its block, so that a call in a loop never receives the callee's
// modifications from the previous iteration.
static bool isInitializedOnlyForCall(Value *Op, CallInst &CI, unsigned ArgNo) {
  auto *AI = dyn_cast<AllocaInst>(Op->stripPointerCasts());
  if (!AI)
    return false;

  auto isWriteBeforeCall = [&CI](Instruction *I) { return I->getParent() == CI.getParent() && I->comesBefore(&CI); };

  SmallVector<Value *, 4> Worklist{AI};
  while (!Worklist.empty()) {
    Value *V = Worklist.pop_back_val();
    for (Use &U : V->uses()) {
      auto *User = cast<Instruction>(U.getUser());
      if (isa<BitCastInst>(User) || isa<AddrSpaceCastInst>(User)) {
        Worklist.push_back(User);
        continue;
      }
      if (User->isLifetimeStartOrEnd())
        continue;
      if (auto *MI = dyn_cast<MemIntrinsic>(User)) {
        if (&U == &MI->getRawDestUse() && !MI->isVolatile() && isWriteBeforeCall(MI))
          continue;
        return false;
      }
      auto *CB = dyn_cast<CallBase>(User);
      if (!CB || !CB->isArgOperand(&U))
        return false;
      unsigned No = CB->getArgOperandNo(&U);
      if (CB == &CI && No == ArgNo)
        continue;
      if (CB != &CI && CB->paramHasAttr(No, llvm::Attribute::StructRet) && isWriteBeforeCall(CB))
        continue;
      return false;
    }
  }
  return true;
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

    if (CI.paramHasAttr(i, llvm::Attribute::ByVal) && !CI.paramHasAttr(i, llvm::Attribute::ReadOnly) &&
        !isInitializedOnlyForCall(OpI, CI, i)) {
      Type *ElTy = CI.getParamByValType(i);
      Align alignment = CI.getParamAlign(i) ? *CI.getParamAlign(i) : DL.getABITypeAlign(ElTy);
      IGCLLVM::IRBuilder<> builder(&CI);
      AllocaInst *AI = builder.CreateAlloca(ElTy);
      AI->setAlignment(alignment);
      builder.CreateMemCpy(AI, OpI, DL.getTypeAllocSize(ElTy), alignment.value());
      auto AC = builder.CreateAddrSpaceCast(AI, OpITy);
      CI.setArgOperand(i, AC);
      m_changed = true;
    }
  }
}
