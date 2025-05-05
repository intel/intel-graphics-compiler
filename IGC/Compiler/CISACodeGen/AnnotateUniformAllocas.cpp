/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "AnnotateUniformAllocas.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "annotate_uniform_allocas"
#define PASS_DESCRIPTION "Annotate uniform allocas"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(AnnotateUniformAllocas, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(AnnotateUniformAllocas, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{
    char AnnotateUniformAllocas::ID = 0;

    AnnotateUniformAllocas::AnnotateUniformAllocas() : FunctionPass(ID)
    {
        initializeAnnotateUniformAllocasPass(*PassRegistry::getPassRegistry());
    }

    llvm::FunctionPass* createAnnotateUniformAllocasPass()
    {
        return new AnnotateUniformAllocas();
    }

    bool AnnotateUniformAllocas::runOnFunction(Function& F)
    {
        WI = &getAnalysis<WIAnalysis>();
        IGC_ASSERT(WI != nullptr);
        visit(F);

        while (!AssumeToErase.empty()) {
            auto Inst = AssumeToErase.pop_back_val();
            bool IsAssume = false;
            if (llvm::GenIntrinsicInst* pIntr = llvm::dyn_cast<llvm::GenIntrinsicInst>(Inst)) {
                IsAssume = (pIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_assume_uniform);
            }
            if (IsAssume || isInstructionTriviallyDead(Inst)) {
                for (Use& OpU : Inst->operands()) {
                    Value* OpV = OpU.get();
                    if (Instruction* OpI = dyn_cast<Instruction>(OpV))
                        AssumeToErase.push_back(OpI);
                }
                Inst->eraseFromParent();
            }
        }
        return m_changed;
    }

    void AnnotateUniformAllocas::visitAllocaInst(AllocaInst& I)
    {
        bool isUniformAlloca = WI->isUniform(&I);
        if (isUniformAlloca)
        {
            // add the meta-date to the alloca for promotion
            IRBuilder<> builder(&I);
            MDNode* node = MDNode::get(I.getContext(), ConstantAsMetadata::get(builder.getInt1(true)));
            I.setMetadata("uniform", node);
            m_changed = true;
        }
    }

    void AnnotateUniformAllocas::visitCallInst(CallInst& I)
    {
        if (llvm::GenIntrinsicInst* pIntr = llvm::dyn_cast<llvm::GenIntrinsicInst>(&I))
        {
            if (pIntr->getIntrinsicID() == GenISAIntrinsic::GenISA_assume_uniform)
            {
                AssumeToErase.push_back(pIntr);
                // add the meta-date to the alloca for promotion
                auto OpV = pIntr->getOperand(0);
                while (OpV)
                {
                   if (auto CI = dyn_cast<CastInst>(OpV))
                   {
                       OpV = CI->getOperand(0);
                       continue;
                   }
                   if (auto ALI = dyn_cast<AllocaInst>(OpV))
                   {
                       IRBuilder<> builder(ALI);
                       MDNode* node = MDNode::get(ALI->getContext(),
                               ConstantAsMetadata::get(builder.getInt1(true)));
                       ALI->setMetadata("UseAssumeUniform", node);
                   }
                   break;
                }
            }
        }
    }
}
