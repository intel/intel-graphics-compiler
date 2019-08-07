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
