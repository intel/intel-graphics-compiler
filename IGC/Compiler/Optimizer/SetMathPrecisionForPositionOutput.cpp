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

#include "Compiler/Optimizer/SetMathPrecisionForPositionOutput.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"

using namespace llvm;
namespace IGC
{
    char SetMathPrecisionForPositionOutput::ID = 0;

    bool SetMathPrecisionForPositionOutput::runOnFunction(llvm::Function& F)
    {
        m_visitedInst.clear();
        visit(F);
        return true;
    }

    void SetMathPrecisionForPositionOutput::visitCallInst(CallInst& I)
    {
        if (GenIntrinsicInst * intr = dyn_cast<GenIntrinsicInst>(&I))
        {
            if (intr->getIntrinsicID() == GenISAIntrinsic::GenISA_OUTPUT)
            {
                const ShaderOutputType usage = static_cast<ShaderOutputType>(
                    llvm::cast<llvm::ConstantInt>(intr->getOperand(4))->getZExtValue());
                if (usage == SHADER_OUTPUT_TYPE_POSITION)
                {
                    UpdateDependency(intr);
                }
            }
        }
    }

    void SetMathPrecisionForPositionOutput::UpdateDependency(Instruction* inst)
    {
        auto inserted = m_visitedInst.insert(inst);
        if (!inserted.second)
        {
            return;
        }
        unsigned int op = inst->getOpcode();
        if (op == Instruction::FAdd ||
            op == Instruction::FSub ||
            op == Instruction::FMul ||
            op == Instruction::FDiv ||
            op == Instruction::FRem)
        {
            // TODO: We should revisit this and understand, if total disabling of fast
            // flags is needed in order to achieve position invariance.
            inst->setFast(false);
        }
        for (unsigned int i = 0; i < inst->getNumOperands(); ++i)
        {
            if (Instruction * srcInst = dyn_cast<Instruction>(inst->getOperand(i)))
            {
                UpdateDependency(srcInst);
            }
        }
    }
}