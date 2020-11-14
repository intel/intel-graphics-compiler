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

#include "AnnotateUniformAllocas.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/IGCPassSupport.h"
#include "IGCIRBuilder.h"
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
            if (isInstructionTriviallyDead(Inst)) {
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
        bool isUniformAlloca = WI->whichDepend(&I) == WIAnalysis::UNIFORM;
        if (isUniformAlloca)
        {
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
            }
        }
    }
}