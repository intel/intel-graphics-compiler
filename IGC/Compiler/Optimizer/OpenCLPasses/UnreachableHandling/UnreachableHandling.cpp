/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/UnreachableHandling/UnreachableHandling.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvmWrapper/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"


using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-unreachable-handling"
#define PASS_DESCRIPTION "Make sure kernel has EOT instruction even if it hits unreachable code."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(UnreachableHandling, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(UnreachableHandling, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char UnreachableHandling::ID = 0;

UnreachableHandling::UnreachableHandling() : FunctionPass(ID)
{
    initializeUnreachableHandlingPass(*PassRegistry::getPassRegistry());
}


void UnreachableHandling::visitUnreachableInst(UnreachableInst& I) {

    m_instsToReplace.push_back(&I);
}

void IGC::UnreachableHandling::replaceUnreachable(llvm::UnreachableInst* I)
{
    assert(I);
    IRBuilder<> builder(I->getContext());
    Function* F = I->getFunction();

    ReturnInst* ret = F->getReturnType()->isVoidTy() ?
        builder.CreateRetVoid() :
        builder.CreateRet(UndefValue::get(F->getReturnType()));

    // If this is the last instruction in the BB, just replace it with return instruction.
    if (&I->getParent()->back() == I) {
        IGCLLVM::pushBackInstruction(I->getParent(), ret);
        I->eraseFromParent();
        return;
    }

    // If there were some other instructions after, split the basic block and let DCE handle it.
    auto BB = I->getParent();
    BB->splitBasicBlock(I);
    auto BBWithRet = BasicBlock::Create(F->getContext(), "", F);
    IGCLLVM::pushBackInstruction(BBWithRet, ret);
    cast<BranchInst>(BB->getTerminator())->setSuccessor(0, BBWithRet);
    I->eraseFromParent();
}

bool UnreachableHandling::runOnFunction(Function& F)
{
    m_instsToReplace.clear();
    visit(F);

    for (auto I : m_instsToReplace) {
        replaceUnreachable(I);
    }

    return m_instsToReplace.size() > 0;
}
