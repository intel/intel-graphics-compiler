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

 /**
  * IGCConstProp was originated from llvm copy-prop code with one addition for
  * shader-constant replacement. So we need to add llvm copyright
  **/
  //
  //===- ConstantProp.cpp - Code to perform Simple Constant Propagation -----===//
  //
  //                     The LLVM Compiler Infrastructure
  //
  // This file is distributed under the University of Illinois Open Source
  // License. See LICENSE.TXT for details.
  //
  //===----------------------------------------------------------------------===//

#include "Compiler/ACLPrintfTranslation.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"

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
        if (V->getName() == "printf")
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
                        assert(I->getNumOperands() == 1);
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

