/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "FixSampleDInputs.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/BasicBlock.h>
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"

using namespace llvm;

namespace IGC
{
    class FixSampleDInputsPass : public FunctionPass
    {
    public:
        FixSampleDInputsPass() :
            FunctionPass(ID)
        { }

        virtual bool runOnFunction(Function& F) override;

        virtual void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

        virtual StringRef getPassName() const override { return "FixSampleDInputs"; }
        static char ID;
    };

    bool FixSampleDInputsPass::runOnFunction(Function& F)
    {
        bool changed = false;
        Module* M = F.getParent();
        Type* volumeTextureType = GetResourceDimensionType(*M, RESOURCE_DIMENSION_TYPE::DIM_3D_TYPE);
        Type* cubeTextureType = GetResourceDimensionType(*M, RESOURCE_DIMENSION_TYPE::DIM_CUBE_TYPE);
        Type* cubeArrayTextureType = GetResourceDimensionType(*M, RESOURCE_DIMENSION_TYPE::DIM_CUBE_ARRAY_TYPE);
        for (BasicBlock& BB : F)
        {
            for (Instruction& inst : BB)
            {
                SampleIntrinsic* sampleInst = dyn_cast<SampleIntrinsic>(&inst);
                Type* textureType = sampleInst ? sampleInst->getTextureValue()->getType()->getPointerElementType() : nullptr;
                if (sampleInst &&
                    sampleInst->getIntrinsicID() == GenISAIntrinsic::GenISA_sampleDptr &&
                    (textureType != volumeTextureType && textureType != cubeTextureType && textureType != cubeArrayTextureType))
                {
                    changed = true;
                    sampleInst->setArgOperand(7, sampleInst->getArgOperand(10));
                    sampleInst->setArgOperand(10, ConstantFP::get(sampleInst->getArgOperand(0)->getType(), 0.0));
                }
            }
        }
        return changed;
    }

    char FixSampleDInputsPass::ID = 0;

    FunctionPass* createFixSampleDInputsPass()
    {
        return new FixSampleDInputsPass();
    }
}
