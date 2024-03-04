/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGC {

class ForceFunctionsToNop : public FunctionPass
{
public:
    static char ID;

    ForceFunctionsToNop();

    ~ForceFunctionsToNop() {}
    bool runOnFunction(Function& F) override;
    virtual StringRef getPassName() const override { return "Force Functions to NOP"; }
};

#define PASS_FLAG     "ForceFunctionsToNop"
#define PASS_DESC     "Force Immediate Zero Return of Functions"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ForceFunctionsToNop, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ForceFunctionsToNop, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

char ForceFunctionsToNop::ID = 0;

ForceFunctionsToNop::ForceFunctionsToNop() : FunctionPass(ID)
{
    initializeForceFunctionsToNopPass(*PassRegistry::getPassRegistry());
}

// This pass will rewrite all functions to return a zero initializer of the
// function's type.  It was created to help in shader isolations.  The user
// must use Options.txt to narrow the set of hashes to apply this pass.

bool ForceFunctionsToNop::runOnFunction(Function& F)
{
    IRBuilder<> IRB(F.getContext());
    BasicBlock& oldEntryBB = F.getEntryBlock();
    BasicBlock* pNewEntryBB = BasicBlock::Create(F.getContext(), "", &F, &oldEntryBB);

    IRB.SetInsertPoint(pNewEntryBB);

    if (auto* pFnTy = F.getReturnType();
        pFnTy->isFirstClassType())
    {
        auto* pVal = Constant::getNullValue(pFnTy);
        IRB.CreateRet(pVal);
    }
    else
    {
        IGC_ASSERT_MESSAGE(pFnTy->isVoidTy(), "Expected void type");
        IRB.CreateRetVoid();
    }

    removeUnreachableBlocks(F);

    return true;
}

FunctionPass* createForceFunctionsToNop()
{
    return new ForceFunctionsToNop();
}

} // namesapace IGC
