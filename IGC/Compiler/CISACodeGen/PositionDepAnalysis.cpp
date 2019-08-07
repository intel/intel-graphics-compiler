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

#include "Compiler/CISACodeGen/PositionDepAnalysis.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
using namespace IGC;
using namespace llvm;

char PositionDepAnalysis::ID = 0;
#define PASS_FLAG "PositionDepAnalysis"
#define PASS_DESCRIPTION "Pos"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(PositionDepAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(PositionDepAnalysis, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{


    PositionDepAnalysis::PositionDepAnalysis() : FunctionPass(ID)
    {
        initializePositionDepAnalysisPass(*PassRegistry::getPassRegistry());
    }

    bool PositionDepAnalysis::runOnFunction(llvm::Function& F)
    {
        CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        if (ctx->type == ShaderType::VERTEX_SHADER && ctx->m_DriverInfo.PreventZFighting())
        {
            visit(F);
        }
        return false;
    }

    void PositionDepAnalysis::visitCallInst(CallInst& I)
    {
        if (GenIntrinsicInst * intr = dyn_cast<GenIntrinsicInst>(&I))
        {
            if (intr->getIntrinsicID() == GenISAIntrinsic::GenISA_URBWrite)
            {
                if (ConstantInt * index = dyn_cast<ConstantInt>(intr->getOperand(0)))
                {
                    // position is written at offset 0 or 1
                    if (index->isZero() || index->isOne())
                    {
                        unsigned int baseSourceIndex = index->isZero() ? 6 : 2;
                        for (unsigned int i = 0; i < 4; i++)
                        {
                            if (Instruction * instDep = dyn_cast<Instruction>(intr->getOperand(baseSourceIndex + i)))
                            {
                                UpdateDependency(instDep);
                            }
                        }
                    }
                }
            }
        }
    }

    void PositionDepAnalysis::UpdateDependency(Instruction* inst)
    {
        auto it = m_PositionDep.insert(inst);
        if (it.second)
        {
            for (unsigned int i = 0; i < inst->getNumOperands(); ++i)
            {
                if (Instruction * srcInst = dyn_cast<Instruction>(inst->getOperand(i)))
                {
                    UpdateDependency(srcInst);
                }
            }
        }
    }
    bool PositionDepAnalysis::PositionDependsOnInst(Instruction* inst)
    {
        return m_PositionDep.find(inst) != m_PositionDep.end();
    }




}