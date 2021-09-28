/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
