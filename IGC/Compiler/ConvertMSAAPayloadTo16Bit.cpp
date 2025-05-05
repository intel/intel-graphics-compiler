/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "ConvertMSAAPayloadTo16Bit.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMUtils.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvm/IR/Function.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-ConvertMSAAPayloadTo16Bit"
#define PASS_DESCRIPTION "Convert normal MSAA intrinsics to 16 bit payload intrinsics"
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
            if (m_pCtxWrapper->getCodeGenContext()->platform.supports16BitLdMcs())
            {
                coordType = m_builder->getInt16Ty();
            }

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


            // There are uses of ldmcs other then ldms, using vector of int32 type.
            // Fix them to use newly created 16bit ldmcs.
            if (ldmcs->getType()->isVectorTy() &&
                cast<IGCLLVM::FixedVectorType>(ldmcs->getType())->getElementType() == m_builder->getInt32Ty())
            {
                m_builder->SetInsertPoint(ldmcs);

                uint32_t ldmcsNumOfElements =    (uint32_t)cast<IGCLLVM::FixedVectorType>(ldmcs->getType())->getNumElements();
                uint32_t newLdmcsNumOfElements = (uint32_t)cast<IGCLLVM::FixedVectorType>(new_mcs_call->getType())->getNumElements();

                // vec of 16bit ints to vec of 32bit ints
                Type* newLdmcsVecType = IGCLLVM::FixedVectorType::get(m_builder->getInt32Ty(), newLdmcsNumOfElements / 2);
                Value* ldmcsExtendedToInt32 = m_builder->CreateBitCast(new_mcs_call, newLdmcsVecType);
                uint32_t ldmcsExtendedToInt32NumOfElements = (uint32_t)cast<IGCLLVM::FixedVectorType>(ldmcsExtendedToInt32->getType())->getNumElements();

                // if ldmcs has fewer elements than new ldmcs, extend vector
                Value* ldmcsInt32CorrectlySized;
                if (ldmcsExtendedToInt32NumOfElements != ldmcsNumOfElements)
                {
                    IGC_ASSERT(ldmcsExtendedToInt32NumOfElements * 2 >= ldmcsNumOfElements);
                    SmallVector<uint32_t, 4> maskVals;
                    for (uint i = 0; i < ldmcsNumOfElements; i++)
                    {
                        maskVals.push_back(i);
                    }
                    auto* pMask = ConstantDataVector::get(I.getContext(), maskVals);

                    ldmcsInt32CorrectlySized = m_builder->CreateShuffleVector(ldmcsExtendedToInt32, UndefValue::get(ldmcsExtendedToInt32->getType()), pMask);
                }
                else
                {
                    ldmcsInt32CorrectlySized = ldmcsExtendedToInt32;
                }
                IGC_ASSERT(ldmcsInt32CorrectlySized);

                ldmcs->replaceAllUsesWith(ldmcsInt32CorrectlySized);
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
    IRBuilder<> builder(F.getContext());
    m_builder = &builder;
    visit(F);
    return true;
}
