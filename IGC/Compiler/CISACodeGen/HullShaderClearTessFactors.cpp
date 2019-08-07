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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"

#include "HullShaderClearTessFactors.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

namespace
{
    using namespace llvm;
    using namespace IGC;

    class ClearTessFactors : public FunctionPass
    {
    public:
        static char ID;

        ClearTessFactors();

        StringRef getPassName() const override { return "ClearTessFactors"; }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(Function& F) override;

    private:
        std::vector<CallInst*> mIntrinsicInstToRemove;

        void processBlock(BasicBlock* BB, unsigned int tessShaderDomain);
        bool m_changed;
    };

    char ClearTessFactors::ID = 0;

#define PASS_FLAG     "igc-cleartessfactors"
#define PASS_DESC     "Clear tessellation factors"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(ClearTessFactors, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_END(ClearTessFactors, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

        bool isReturnBlock(BasicBlock* BB)
    {
        // Check whether current BB has only 'ret' instruction.
        bool isRet = true;
        for (auto II = BB->begin(), IE = BB->end(); II != IE; ++II)
        {
            if (!isa<ReturnInst>(&(*II)))
            {
                isRet = false;
                break;
            }
        }
        return isRet;
    }

    bool hasReturnInst(BasicBlock* BB)
    {
        // Check the last instruction in basic block
        auto II = BB->rbegin();
        auto IE = BB->rend();
        bool retCondition = false;

        if (II != IE)
        {
            if (isa<ReturnInst>(&(*II)))
            {
                // Last instruction is return instruction
                retCondition = true;
            }
            else if (llvm::BranchInst * pBrInst = dyn_cast<BranchInst>(&(*II)))
            {
                // Last instruction is branch instruction. Check whether target BB
                // has only 'ret' instruction.
                if (pBrInst->getSuccessor(0))
                {
                    if (isReturnBlock(pBrInst->getSuccessor(0)))
                    {
                        retCondition = true;
                    }
                }
            }

        }
        return retCondition;
    }
} // end of unnamed namespace to contain class definition and auxiliary functions

ClearTessFactors::ClearTessFactors() : FunctionPass(ID), m_changed(false)
{
    initializeClearTessFactorsPass(*PassRegistry::getPassRegistry());
}

void ClearTessFactors::processBlock(BasicBlock* BB, unsigned int tessShaderDomain)
{
    if (hasReturnInst(BB))
    {
        std::vector<CallInst*> intrinsicInstToRemove;
        bool isAnyOfOuterTFZero = false;

        for (auto II = BB->rbegin(), IE = BB->rend(); II != IE; ++II)
        {
            if (GenIntrinsicInst * genIntr = dyn_cast<GenIntrinsicInst>(&(*II)))
            {
                GenISAIntrinsic::ID ID = genIntr->getIntrinsicID();

                // We are interested only in URBWrite intrinsics
                if (ID != GenISAIntrinsic::GenISA_URBWrite)
                {
                    continue;
                }

                // Intrinsic has the format: URBWrite (%offset, %mask, %data0, ... , %data7)
                ConstantInt* pOffset = dyn_cast<ConstantInt>(genIntr->getOperand(0));
                ConstantInt* pImmediateMask = dyn_cast<ConstantInt>(genIntr->getOperand(1));
                if (pOffset == nullptr || pImmediateMask == nullptr)
                {
                    intrinsicInstToRemove.push_back(genIntr);
                    continue;
                }
                const unsigned int offset = int_cast<unsigned int>(pOffset->getZExtValue());
                const unsigned int mask = int_cast<unsigned int>(pImmediateMask->getZExtValue());

                if (offset != 0 && offset != 1)
                {
                    // Only URBWrite(0, ...) or URBWrite(1, ...) instructions can write outer tessellation factors
                    intrinsicInstToRemove.push_back(genIntr);
                    continue;
                }
                if (offset == 0 && mask <= 0x0F)
                {
                    // Only URBWrite(0, ...) instructions of size >4 can write outer tessellation factors
                    intrinsicInstToRemove.push_back(genIntr);
                    continue;
                }

                unsigned int urbWriteOuterTessFactorStartIndex =
                    // Triangle URBWrite(0, mask, %data, %data, %data, %data, inner0, outer2, outer1, outer0)
                    (tessShaderDomain == USC::TESSELLATOR_DOMAIN_TRI) ? 7 :
                    // Quad     URBWrite(0, mask, %data, %data, inner1, inner0, outer3, outer2, outer1, outer0)
                    (tessShaderDomain == USC::TESSELLATOR_DOMAIN_QUAD) ? 6 :
                    // Isoline  URBWrite(0, mask, %data, %data, %data, %data, %data, %data, outer1, outer0)
                    (tessShaderDomain == USC::TESSELLATOR_DOMAIN_ISOLINE) ? 8 : 8;

                unsigned int urbWriteOuterTessFactorStopIndex = 10;

                if (offset == 1)
                {
                    // Adjust indexes for URBWrite(1, ...) instructions
                    urbWriteOuterTessFactorStartIndex -= 4;
                    urbWriteOuterTessFactorStopIndex -= 4;
                }

                // Check if any of written outer tessellation factors is 0
                bool isZero = false;
                for (unsigned int i = urbWriteOuterTessFactorStartIndex; i < urbWriteOuterTessFactorStopIndex; i++)
                {
                    unsigned int outerTessFactorPositionInURBWriteMask = 1 << (i - 2);
                    bool isOuterTessFactorWritten = (mask & outerTessFactorPositionInURBWriteMask);

                    llvm::ConstantFP* zero = llvm::dyn_cast<llvm::ConstantFP>(genIntr->getOperand(i));
                    if (zero && zero->isExactlyValue(0.f) && isOuterTessFactorWritten)
                    {
                        // Clear tessellation factors. Only one outer tessellation factor needs to be set to zero
                        // to ensure that patch will be culled at the TE stage.

                        // Offset 0
                        genIntr->setOperand(0, llvm::ConstantInt::get(Type::getInt32Ty(BB->getContext()), 0x0));
                        // Full mask
                        genIntr->setOperand(1, llvm::ConstantInt::get(Type::getInt32Ty(BB->getContext()), 0xFF));
                        // Set tessellation factors and padding
                        Value* undef = llvm::UndefValue::get(Type::getFloatTy(BB->getContext()));
                        Value* data[8] = { undef,undef,undef,undef,undef,undef,undef,zero };
                        for (int ii = 0; ii < 8; ii++)
                        {
                            genIntr->setOperand(2 + ii, data[ii]);
                        }

                        isZero = true;
                        break;
                    }
                }
                if (isZero)
                {
                    m_changed = true;
                    isAnyOfOuterTFZero = true;
                }
                else
                {
                    intrinsicInstToRemove.push_back(genIntr);
                }
            }
        }

        if (isAnyOfOuterTFZero)
        {
            for (auto pIntrinsicInst : intrinsicInstToRemove)
            {
                mIntrinsicInstToRemove.push_back(pIntrinsicInst);
            }
        }
    }
}

bool ClearTessFactors::runOnFunction(Function& F)
{
    /*
    This is an example of what this optimization does:

    ## Before optimization ##

    if.then76.i.2:                                    ; preds = %entry
    call void @llvm.genx.GenISA.URBWrite(i32 2, i32 255, float %541, float %542, float %543, float %544, float %555, float %548, float %554, float %551)
    call void @llvm.genx.GenISA.URBWrite(i32 0, i32 255, float undef, float undef, float undef, float undef, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00, float 0.000000e+00)
    br label %tessControlShaderEntry.exit.2

    tessControlShaderEntry.exit.2:                    ; preds = %if.then76.i.2, %if.then181.i.2, %if.then257.i.2, %if.end258.i.2
    ret void

    ## After optimization ##

    if.then76.i.2:                                    ; preds = %entry
    call void @llvm.genx.GenISA.URBWrite(i32 0, i32 255, float undef, float undef, float undef, float undef, float undef, float undef, float undef, float 0.000000e+00)
    br label %tessControlShaderEntry.exit.2

    tessControlShaderEntry.exit.2:                    ; preds = %if.then76.i.2, %if.then181.i.2, %if.then257.i.2, %if.end258.i.2
    ret void

    */
    unsigned int tessShaderDomain = USC::TESSELLATOR_DOMAIN_ISOLINE;
    llvm::NamedMDNode* pMetaData = F.getParent()->getOrInsertNamedMetadata("TessellationShaderDomain");
    if (pMetaData && (pMetaData->getNumOperands() == 1))
    {
        llvm::MDNode* pTessShaderDomain = pMetaData->getOperand(0);
        if (pTessShaderDomain)
        {
            tessShaderDomain = int_cast<uint32_t>(
                mdconst::dyn_extract<ConstantInt>(pTessShaderDomain->getOperand(0))->getZExtValue());
        }
    }

    for (auto BI = F.begin(), BE = F.end(); BI != BE; BI++)
    {
        BasicBlock* bb = dyn_cast<BasicBlock>(BI);
        processBlock(bb, tessShaderDomain);
    }

    // Remove GenISAIntrinsic instructions.
    for (auto pIntrinsicInst : mIntrinsicInstToRemove)
    {
        pIntrinsicInst->eraseFromParent();
    }
    return m_changed;
}

llvm::FunctionPass* IGC::createClearTessFactorsPass()
{
    return new ClearTessFactors();
}
