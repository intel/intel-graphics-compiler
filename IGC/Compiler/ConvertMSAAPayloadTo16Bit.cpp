/*========================== begin_copyright_notice ============================

Copyright (c) 2000-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "ConvertMSAAPayloadTo16Bit.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMUtils.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/Function.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-ConverMSAAPayloadTo16Bit"
#define PASS_DESCRIPTION "Comvert normal MSAA intrinsics to 16 bit payload intrinsics"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(ConvertMSAAPayloadTo16Bit, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ConvertMSAAPayloadTo16Bit, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ConvertMSAAPayloadTo16Bit::ID = 0;

ConvertMSAAPayloadTo16Bit::ConvertMSAAPayloadTo16Bit() : FunctionPass(ID)
{
    initializeWorkaroundAnalysisPass(*PassRegistry::getPassRegistry());
};

/*
Returns vector of LdmsIntrinsics using inst directly and indirectly
input: inst - instruction that ldms intrinsics we search for are using
output: ldmsUsing - vector of ldmsIntrinsics using inst
*/
void ConvertMSAAPayloadTo16Bit::findLdmsUsingInstDownInTree(Value* inst, std::vector<GenIntrinsicInst*>& ldmsUsing)
{
    if (LdmsInstrinsic* ldms = dyn_cast<LdmsInstrinsic>(inst))
    {
        ldmsUsing.push_back(ldms);
        return;
    }
    else
    {
        for (User* user : inst->users())
        {
            findLdmsUsingInstDownInTree(user, ldmsUsing);
        }
    }
}

void ConvertMSAAPayloadTo16Bit::visitCallInst(CallInst& I)
{
    if (GenIntrinsicInst* intr = dyn_cast<GenIntrinsicInst>(&I))
    {
        GenISAIntrinsic::ID intrID = intr->getIntrinsicID();
        switch (intrID)
        {
        case GenISAIntrinsic::GenISA_ldmcsptr:
        {
            GenIntrinsicInst* ldmcs = intr;

            Type* coordType = m_builder->getInt32Ty();
            Type* types_ldmcs[] = {
                IGCLLVM::FixedVectorType::get(m_builder->getInt16Ty(), 4),
                coordType,
                ldmcs->getOperand(4)->getType() };

            Function* func_ldmcs =
                GenISAIntrinsic::getDeclaration(
                    I.getParent()->getParent()->getParent(),
                    GenISAIntrinsic::GenISA_ldmcsptr,
                    types_ldmcs);

            m_builder->SetInsertPoint(ldmcs);
            Value* packed_tex_params_ldmcs[] = {
                 m_builder->CreateTrunc(ldmcs->getOperand(0), coordType),
                 m_builder->CreateTrunc(ldmcs->getOperand(1), coordType),
                 m_builder->CreateTrunc(ldmcs->getOperand(2), coordType),
                 m_builder->CreateTrunc(ldmcs->getOperand(3), coordType),
                 ldmcs->getOperand(4),
                 ldmcs->getOperand(5),
                 ldmcs->getOperand(6),
                 ldmcs->getOperand(7)
            };

            llvm::CallInst* new_mcs_call = m_builder->CreateCall(func_ldmcs, packed_tex_params_ldmcs);

            llvm::Value* mcs0 = m_builder->CreateExtractElement(new_mcs_call, m_builder->getInt32(0));
            llvm::Value* mcs1 = m_builder->CreateExtractElement(new_mcs_call, m_builder->getInt32(1));
            llvm::Value* mcs2 = m_builder->CreateExtractElement(new_mcs_call, m_builder->getInt32(2));
            llvm::Value* mcs3 = m_builder->CreateExtractElement(new_mcs_call, m_builder->getInt32(3));

            // find ldms using this ldmcs call and convert them to 16 bit
            std::vector<GenIntrinsicInst*> ldmsUsingldmcs;
            findLdmsUsingInstDownInTree(ldmcs, ldmsUsingldmcs);

            for (GenIntrinsicInst* ldms : ldmsUsingldmcs)
            {
                Type* types_ldms[] = { ldms->getType(), ldms->getOperand(7)->getType() };
                Function* func_ldms = GenISAIntrinsic::getDeclaration(
                    ldms->getParent()->getParent()->getParent(),
                    GenISAIntrinsic::GenISA_ldmsptr16bit,
                    types_ldms);

                m_builder->SetInsertPoint(ldms);
                llvm::Value* packed_tex_params_ldms[] = {
                     m_builder->CreateTrunc(ldms->getOperand(0), m_builder->getInt16Ty(), ""),
                     mcs0,
                     mcs1,
                     mcs2,
                     mcs3,
                     m_builder->CreateTrunc(ldms->getOperand(3), m_builder->getInt16Ty()),
                     m_builder->CreateTrunc(ldms->getOperand(4), m_builder->getInt16Ty()),
                     m_builder->CreateTrunc(ldms->getOperand(5), m_builder->getInt16Ty()),
                     m_builder->CreateTrunc(ldms->getOperand(6), m_builder->getInt16Ty()),
                     ldms->getOperand(7),
                     ldms->getOperand(8),
                     ldms->getOperand(9),
                     ldms->getOperand(10)
                };

                llvm::CallInst* new_ldms = m_builder->CreateCall(func_ldms, packed_tex_params_ldms);
                ldms->replaceAllUsesWith(new_ldms);
            }

            // In OGL there are uses of ldmcs other then ldms, using vec4float type.
            // Fix them to use newly created 16bit ldmcs.
            if (ldmcs->getType()->isVectorTy() &&
                ldmcs->getType()->getVectorElementType()->isFloatTy())
            {
                m_builder->SetInsertPoint(ldmcs);

                uint ldmcsNumOfElements = ldmcs->getType()->getVectorNumElements();
                uint new_mcs_callNumOfElements = new_mcs_call->getType()->getVectorNumElements();

                // vec of 16bit ints to vec of 32bit ints
                Type* new_mcs_callVecType = VectorType::get(m_builder->getInt32Ty(), new_mcs_callNumOfElements);
                Value* ldmcsExtendedToInt32 = m_builder->CreateSExt(new_mcs_call, new_mcs_callVecType);

                // if new ldmcs has fewer elements than ldmcs, extend vector
                Value* newLdmcsSizedVector;
                if (new_mcs_callNumOfElements < ldmcsNumOfElements)
                {
                    SmallVector<uint32_t, 4> maskVals;
                    for (uint i = 0; i < ldmcsNumOfElements; i++)
                    {
                        maskVals.push_back(i);
                    }
                    auto* pMask = ConstantDataVector::get(I.getContext(), maskVals);

                    newLdmcsSizedVector = m_builder->CreateShuffleVector(ldmcsExtendedToInt32, UndefValue::get(VectorType::get(m_builder->getInt32Ty(), ldmcsNumOfElements)), pMask);
                }
                else
                {
                    newLdmcsSizedVector = ldmcsExtendedToInt32;
                }
                IGC_ASSERT(newLdmcsSizedVector);

                Type* ldmcsFloatVecType = VectorType::get(m_builder->getFloatTy(), ldmcsNumOfElements);
                Value* ldmcsBitcastedToFloat = m_builder->CreateBitCast(ldmcsExtendedToInt32, ldmcsFloatVecType);
                ldmcs->replaceAllUsesWith(ldmcsBitcastedToFloat);
            }

            break;
        }

        default:
            break;
        }
    }
}

bool ConvertMSAAPayloadTo16Bit::runOnFunction(Function& F)
{
    m_pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
    CodeGenContext* cgCtx = m_pCtxWrapper->getCodeGenContext();
    IRBuilder<> builder(F.getContext());
    m_builder = &builder;
    visit(F);
    DumpLLVMIR(cgCtx, "AfterMSAA16bitPayload");
    return true;
}
