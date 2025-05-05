/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/LowPrecisionOptPass.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/IGCIRBuilder.h"
#include "llvmWrapper/IR/Instructions.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace GenISAIntrinsic;

char LowPrecisionOpt::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-low-precision-opt"
#define PASS_DESCRIPTION "Low Precision Opt"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowPrecisionOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LowPrecisionOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

LowPrecisionOpt::LowPrecisionOpt() : FunctionPass(ID)
{
    initializeLowPrecisionOptPass(*PassRegistry::getPassRegistry());
    m_func_llvm_GenISA_DCL_inputVec_f16 = nullptr;
    m_func_llvm_GenISA_DCL_inputVec_f32 = nullptr;
    m_currFunction = nullptr;
    func_llvm_floor_f32 = nullptr;
}

bool LowPrecisionOpt::runOnFunction(Function& F)
{
    m_changed = false;
    CodeGenContextWrapper* pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
    CodeGenContext* ctx = pCtxWrapper->getCodeGenContext();

    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();

    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
    {
        return m_changed;
    }
    llvm::IGCIRBuilder<> builder(F.getContext());
    m_builder = &builder;
    m_currFunction = &F;
    shdrType = ctx->type;
    bundles.clear();
    m_simplifyAlu = true;
    m_changeSample = false;
    visit(F);
    // change sampler only after we simplified fext + ftrunc
    m_simplifyAlu = false;
    m_changeSample = true;
    visit(F);
    std::sort(bundles.begin(), bundles.end(), cmpOperator);
    auto bundleEnd = bundles.end();
    for (auto bundle = bundles.begin(); bundle != bundleEnd; ++bundle)
    {
        (*bundle).fpTrunc->moveBefore(&(*(m_currFunction->getEntryBlock().begin())));
        (*bundle).cInst->moveBefore(&(*(m_currFunction->getEntryBlock().begin())));
    }
    return m_changed;
}

void LowPrecisionOpt::visitFPExtInst(llvm::FPExtInst& I)
{
    if (!m_simplifyAlu)
    {
        return;
    }
    if (I.getOperand(0)->getType()->isHalfTy())
    {
        Instruction* I0 = dyn_cast<Instruction>(I.getOperand(0));
        llvm::GenIntrinsicInst* callInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(I.getOperand(0));

        if (I0 && I0->getOpcode() == Instruction::FPTrunc && I.getDestTy() == I0->getOperand(0)->getType())
        {
            I.replaceAllUsesWith(I0->getOperand(0));
            I.eraseFromParent();
            m_changed = true;
        }
        else if (callInst && callInst->hasOneUse())
        {
            GenISAIntrinsic::ID ID = callInst->getIntrinsicID();
            if (ID == GenISAIntrinsic::GenISA_DCL_ShaderInputVec || ID == GenISAIntrinsic::GenISA_DCL_inputVec)
            {
                /*
                Catches a pattern where we have a lowp input, then extend it back up. This
                generates mixed mode instructions and so it's better to keep it as PLN.
                Example if it's used directly in the sample instruction before CNL.
                */

                if (m_func_llvm_GenISA_DCL_inputVec_f32 == nullptr)
                {
                    m_func_llvm_GenISA_DCL_inputVec_f32 = llvm::GenISAIntrinsic::getDeclaration(
                        m_currFunction->getParent(),
                        ID,
                        Type::getFloatTy(m_builder->getContext()));
                }

                m_builder->SetInsertPoint(callInst);
                Value* v = m_builder->CreateCall2(m_func_llvm_GenISA_DCL_inputVec_f32, callInst->getOperand(0), callInst->getOperand(1));
#if VALUE_NAME_ENABLE
                v->setName(callInst->getName());
#endif
                I.replaceAllUsesWith(v);
                I.eraseFromParent();
                callInst->eraseFromParent();
                m_changed = true;
            }
        }
    }
}

void LowPrecisionOpt::visitFPTruncInst(llvm::FPTruncInst& I)
{
    if (!m_simplifyAlu)
    {
        return;
    }
    llvm::GenIntrinsicInst* cInst = llvm::dyn_cast<llvm::GenIntrinsicInst>(I.getOperand(0));

    if (cInst &&
        cInst->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue)
    {
        if (!IGC_IS_FLAG_ENABLED(HoistPSConstBufferValues) ||
            shdrType != ShaderType::PIXEL_SHADER)
            return;
        moveBundle bundle;
        bundle.index = (uint)llvm::cast<llvm::ConstantInt>(cInst->getOperand(0))->getZExtValue();
        bundle.cInst = cInst;
        bundle.fpTrunc = &I;
        bundles.push_back(bundle);
    }
}

// If all the uses of a sampler instruction are converted to a different floating point type
// try to propagate the type in the sampler
bool LowPrecisionOpt::propagateSamplerType(llvm::GenIntrinsicInst& I)
{
    if (IGC_IS_FLAG_DISABLED(UpConvertF16Sampler) && cast<VectorType>(I.getType())->getElementType()->isHalfTy())
    {
        return false;
    }

    IGC::CodeGenContext& CGContext = *getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (!CGContext.platform.supportFP16())
    {
        return false;
    }

    Type* eltTy = NULL;
    bool isFloatType = false;

    if (I.getType()->isVectorTy())
    {
        eltTy = cast<VectorType>(I.getType())->getElementType();
        isFloatType = cast<VectorType>(I.getType())->getElementType()->isFloatTy();
    }
    else
    {
        eltTy = I.getType();
        isFloatType = I.getType()->isFloatTy();
    }

    Type* newDstType = nullptr;
    if (eltTy->isFloatingPointTy())
    {
        // check that all uses are extractelement followed by fpext
        newDstType = isFloatType ?
            m_builder->getHalfTy() : m_builder->getFloatTy();
        for (auto use = I.user_begin(); use != I.user_end(); ++use)
        {
            auto extractElt = dyn_cast<ExtractElementInst>(*use);
            if (!(extractElt && extractElt->hasOneUse()))
            {
                return false;
            }
            auto fpExtOrTrunc = dyn_cast<CastInst>(*extractElt->user_begin());

            if (!(fpExtOrTrunc && fpExtOrTrunc->getType() == newDstType))
            {
                return false;
            }
        }
    }
    else if (eltTy == m_builder->getInt32Ty() &&
        IGC_IS_FLAG_ENABLED(DownConvertI32Sampler))
    {
        // This optimization is disabled by default and can only be enabled for
        // resources with 16bit integer format or if it is known that the upper
        // 16bits of data is always 0.
        // i32 to i16 conversion in sampler is a clamp operation and not
        // a truncation operation, e.g. for 0x10000u input data the 16bit
        // result returned by sampler is 0xFFFFu (R32_UINT format).
        newDstType = m_builder->getInt16Ty();
        for (auto use = I.user_begin(); use != I.user_end(); ++use)
        {
            auto extractElt = dyn_cast<ExtractElementInst>(*use);
            if (!(extractElt && extractElt->hasOneUse()))
            {
                return false;
            }
            auto isUpperBitClear = [this](User* U)
            {
                // match the pattern
                // %scalar59 = extractelement <4 x i32> % 83, i32 3
                // % 84 = and i32 %scalar59, 65535
                if (U->getType() != m_builder->getInt32Ty())
                {
                    return false;
                }
                auto andInst = dyn_cast<BinaryOperator>(U);
                if (!andInst || andInst->getOpcode() != BinaryOperator::And)
                {
                    return false;
                }
                auto andSrc1 = dyn_cast<ConstantInt>(andInst->getOperand(1));
                if (!andSrc1 || andSrc1->getZExtValue() != 0xFFFF)
                {
                    return false;
                }
                return true;
            };

            auto Use = *extractElt->user_begin();
            bool isInt32to16Trunc = dyn_cast<TruncInst>(Use) && Use->getType() == m_builder->getInt16Ty();
            if (!isInt32to16Trunc && !isUpperBitClear(Use))
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }

    unsigned int numberOfElements = 1;

    if (I.getType()->isVectorTy())
    {
        numberOfElements = int_cast<unsigned int>(cast<IGCLLVM::FixedVectorType>(I.getType())->getNumElements());
    }

    llvm::SmallVector<llvm::Type*, 4> overloadTys;
    auto retTy = IGCLLVM::FixedVectorType::get(newDstType, numberOfElements);
    overloadTys.push_back(retTy);
    auto ID = I.getIntrinsicID();
    switch (ID)
    {
    case GenISAIntrinsic::GenISA_sampleptr:
    case GenISAIntrinsic::GenISA_sampleBptr:
    case GenISAIntrinsic::GenISA_sampleCptr:
    case GenISAIntrinsic::GenISA_sampleDptr:
    case GenISAIntrinsic::GenISA_sampleDCptr:
    case GenISAIntrinsic::GenISA_sampleLptr:
    case GenISAIntrinsic::GenISA_sampleLCptr:
    case GenISAIntrinsic::GenISA_sampleBCptr:
        // 4 overloaded tys: ret, arg0, resource, sampler
        overloadTys.push_back(I.getArgOperand(0)->getType());
        overloadTys.push_back(cast<SampleIntrinsic>(&I)->getPairedTextureValue()->getType());
        overloadTys.push_back(cast<SampleIntrinsic>(&I)->getTextureValue()->getType());
        overloadTys.push_back(cast<SampleIntrinsic>(&I)->getSamplerValue()->getType());
        break;
    case GenISAIntrinsic::GenISA_ldptr:
    case GenISAIntrinsic::GenISA_ldlptr:
        overloadTys.push_back(cast<SamplerLoadIntrinsic>(&I)->getPairedTextureValue()->getType());
        overloadTys.push_back(cast<SamplerLoadIntrinsic>(&I)->getTextureValue()->getType());
        break;
    case GenISAIntrinsic::GenISA_ldmsptr:
        overloadTys.push_back(cast<SamplerLoadIntrinsic>(&I)->getTextureValue()->getType());
        break;
    case GenISAIntrinsic::GenISA_gather4ptr:
    case GenISAIntrinsic::GenISA_gather4Cptr:
    case GenISAIntrinsic::GenISA_gather4POptr:
    case GenISAIntrinsic::GenISA_gather4POCptr:
    case GenISAIntrinsic::GenISA_gather4Iptr:
    case GenISAIntrinsic::GenISA_gather4IPOptr:
    case GenISAIntrinsic::GenISA_gather4Bptr:
    case GenISAIntrinsic::GenISA_gather4BPOptr:
    case GenISAIntrinsic::GenISA_gather4Lptr:
    case GenISAIntrinsic::GenISA_gather4LPOptr:
    case GenISAIntrinsic::GenISA_gather4ICptr:
    case GenISAIntrinsic::GenISA_gather4ICPOptr:
    case GenISAIntrinsic::GenISA_gather4LCptr:
    case GenISAIntrinsic::GenISA_gather4LCPOptr:
        // 4 overloaded tys: ret, arg0, resource, sampler
        overloadTys.push_back(I.getArgOperand(0)->getType());
        overloadTys.push_back(cast<SamplerGatherIntrinsic>(&I)->getPairedTextureValue()->getType());
        overloadTys.push_back(cast<SamplerGatherIntrinsic>(&I)->getTextureValue()->getType());
        overloadTys.push_back(cast<SamplerGatherIntrinsic>(&I)->getSamplerValue()->getType());
        break;
    default:
        return false;
    }

    Function* newSample = GenISAIntrinsic::getDeclaration(
        m_currFunction->getParent(), I.getIntrinsicID(), overloadTys);
    llvm::SmallVector<llvm::Value*, 8> newArgs;
    for (unsigned int i = 0, argSize = IGCLLVM::getNumArgOperands(&I); i < argSize; i++)
    {
        newArgs.push_back(I.getArgOperand(i));
    }
    m_builder->SetInsertPoint(&I);
    auto newCall = m_builder->CreateCall(newSample, newArgs);

    for (auto use = I.user_begin(); use != I.user_end(); ++use)
    {
        ExtractElementInst* extractElt = cast<ExtractElementInst>(*use);
        m_builder->SetInsertPoint(extractElt);

        Value* extractUse = *extractElt->user_begin();
        Value* newExtract = m_builder->CreateExtractElement(newCall, extractElt->getIndexOperand());
        if (extractUse->getType()->isFloatingPointTy())
        {
            extractUse->replaceAllUsesWith(newExtract);
        }
        else
        {
            if (dyn_cast<TruncInst>(extractUse))
            {
                // replace trunc with new extractElt
                extractUse->replaceAllUsesWith(newExtract);
            }
            else
            {
                // replace and with zext
                Value* zextInst = m_builder->CreateZExt(newExtract, m_builder->getInt32Ty());
                extractUse->replaceAllUsesWith(zextInst);
            }
        }
    }
    return true;
}

void LowPrecisionOpt::visitIntrinsicInst(llvm::IntrinsicInst& I)
{
    if (!m_simplifyAlu)
    {
        return;
    }
    if (I.getIntrinsicID() != llvm::Intrinsic::floor ||
        I.getType() != Type::getHalfTy(m_builder->getContext()))
        return;

    auto src = I.getOperand(0);
    m_builder->SetInsertPoint(&I);

    auto fpTrunc = llvm::dyn_cast <llvm::FPTruncInst>(src);
    if (fpTrunc)
    {
        src = fpTrunc->getOperand(0);
    }
    else
    {
        src = m_builder->CreateFPExt(src, m_builder->getFloatTy());
    }

    if (!func_llvm_floor_f32)
        func_llvm_floor_f32 = llvm::Intrinsic::getDeclaration(m_currFunction->getParent(), Intrinsic::floor, m_builder->getFloatTy());

    auto floor32 = m_builder->CreateCall(func_llvm_floor_f32, src);
#if VALUE_NAME_ENABLE
    floor32->setName(I.getName());
#endif

    if (I.hasOneUse())
    {
        auto hfSub = llvm::dyn_cast<llvm::BinaryOperator>(*I.user_begin());

        if (hfSub && hfSub->getOpcode() == llvm::Instruction::BinaryOps::FSub)
        {
            if (hfSub->getOperand(0) == I.getOperand(0))
            {
                auto fSub = m_builder->CreateFSub(src, floor32, hfSub->getName());
                auto fpdst = m_builder->CreateFPTrunc(fSub, Type::getHalfTy(m_builder->getContext()));
                hfSub->replaceAllUsesWith(fpdst);
            }
        }
    }
    else
    {
        auto fpdst = m_builder->CreateFPTrunc(floor32, Type::getHalfTy(m_builder->getContext()));
        I.replaceAllUsesWith(fpdst);
        I.eraseFromParent();
    }

}

/*FP16SamplerOptimization*/
void LowPrecisionOpt::visitCallInst(CallInst& I)
{
    if (!m_changeSample)
    {
        return;
    }
    if (isSampleLoadGather4InfoInstruction(&I))
    {
        bool changed = propagateSamplerType(*cast<GenIntrinsicInst>(&I));
        if (changed)
        {
            return;
        }
    }
}
