/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/RemoveCodeAssumptions.hpp"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-remove-code-assumptions"
#define PASS_DESCRIPTION "Remove code assumptions from the module"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RemoveCodeAssumptions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(RemoveCodeAssumptions, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char RemoveCodeAssumptions::ID = 0;

RemoveCodeAssumptions::RemoveCodeAssumptions()
    : FunctionPass(ID)
{
    initializeRemoveCodeAssumptionsPass(*PassRegistry::getPassRegistry());
}

bool RemoveCodeAssumptions::runOnFunction(Function& F)
{
    visit(F);

    bool changed = m_instructionsToRemove.size() > 0;
    for (auto I : m_instructionsToRemove)
    {
        I->eraseFromParent();
    }
    m_instructionsToRemove.clear();

    return changed;
}


void RemoveCodeAssumptions::visitIntrinsicInst(llvm::IntrinsicInst& I)
{
    auto intrinsicID = I.getIntrinsicID();

    switch (intrinsicID)
    {
    case Intrinsic::assume:
        m_instructionsToRemove.push_back(&I);
        break;
    default:
        break;
    }
}
