/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#include "Compiler/ACLPrintfTranslation.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-translate-acl-printf"
#define PASS_DESCRIPTION "Translate ACL printf"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ACLPrintfTranslation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(ACLPrintfTranslation, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ACLPrintfTranslation::ID = 0;

ACLPrintfTranslation::ACLPrintfTranslation()
    : ModulePass(ID)
{
    initializeACLPrintfTranslationPass(*PassRegistry::getPassRegistry());
}

void ACLPrintfTranslation::visitCallInst(llvm::CallInst& CI)
{
    Function* callee = CI.getCalledFunction();
    if (!callee) return;

    for (unsigned int i = 0; i < CI.getNumOperands(); i++)
    {
        Value* V = CI.getOperand(i);
        // we found one printf function
        if (V->getName().str() == "printf")
        {
            for (unsigned int i = 0; i < CI.getNumOperands(); i++)
            {
                Value* V = CI.getOperand(i);
                const Type* T = V->getType();
                // if the operand is a double
                if (T && T->isDoubleTy())
                {
                    // if it is a constant as in printf("%f", 10.5)
                    if (isa<ConstantFP>(V))
                    {
                        ConstantFP* C = (ConstantFP*)V;
                        const APFloat apf = C->getValueAPF();
                        Constant* newC = ConstantFP::get(Type::getFloatTy(V->getContext()), apf.convertToDouble());

                        CI.setOperand(i, newC);
                    }
                    // if it is the result of a function as in printf("%f", acospi(2.0f))
                    else if (isa<Instruction>(V))
                    {
                        Instruction* I = (Instruction*)V;
                        IGC_ASSERT(I->getNumOperands() == 1);
                        CI.setOperand(1, I->getOperand(0));
                    }
                }
            }
        }
    }
}

bool ACLPrintfTranslation::runOnModule(Module& M)
{
    bool changed = false;
    for (auto& funcIt : M.getFunctionList())
    {
        visit(funcIt);
    }

    return changed;
}

