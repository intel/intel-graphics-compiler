/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#include "Compiler/Optimizer/OpenCLPasses/UnreachableHandling/UnreachableHandling.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/igc_regkeys.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/Instructions.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include <map>
#include "Probe/Assertion.h"

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
    IGC_ASSERT(I);
    IRBuilder<> builder(I->getContext());
    Function* F = I->getFunction();

    ReturnInst* ret = F->getReturnType()->isVoidTy() ?
        builder.CreateRetVoid() :
        builder.CreateRet(UndefValue::get(F->getReturnType()));

    // If this is the last instruction in the BB, just replace it with return instruction.
    if (&I->getParent()->back() == I) {
        I->getParent()->getInstList().push_back(ret);
        I->eraseFromParent();
        return;
    }

    // If there were some other instructions after, split the basic block and let DCE handle it.
    auto BB = I->getParent();
    BB->splitBasicBlock(I);
    auto BBWithRet = BasicBlock::Create(F->getContext(), "", F);
    BBWithRet->getInstList().push_back(ret);
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
