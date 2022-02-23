/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CPSMSAAOMaskWA.h"
#include "LLVM3DBuilder/MetadataBuilder.h"
#include "LLVM3DBuilder/BuiltinsFrontend.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/Function.h>
#include <llvm/ADT/SmallVector.h>
#include <llvmWrapper/IR/Intrinsics.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CodeGenPublicEnums.h"
#include "Compiler/CodeGenPublic.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;


// Register pass to igc-opt
#define PASS_FLAG "igc-cpsmsaaomaskwa"
#define PASS_DESCRIPTION "Workaround pass used to fix functionality of special cases"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CPSMSAAOMaskWA, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(CPSMSAAOMaskWA, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#undef PASS_ANALYSIS
#undef PASS_CFG_ONLY
#undef PASS_DESCRIPTION
#undef PASS_FLAG

char CPSMSAAOMaskWA::ID = 0;

CPSMSAAOMaskWA::CPSMSAAOMaskWA()
    : ModulePass(ID), m_oMaskInst(nullptr), m_pModule(nullptr)
{
    initializeCPSMSAAOMaskWAPass(*PassRegistry::getPassRegistry());
}

void CPSMSAAOMaskWA::visitCallInst(llvm::CallInst& I)
{
    if (auto output = dyn_cast<GenIntrinsicInst>(&I))
    {
        if (output->getIntrinsicID() == GenISAIntrinsic::GenISA_OUTPUT)
        {
            auto constOutput = dyn_cast<ConstantInt>(output->getArgOperand(4));
            ShaderOutputType outputType = (ShaderOutputType)int_cast<signed int>(constOutput->getZExtValue());
            if (outputType == SHADER_OUTPUT_TYPE_OMASK)
            {
                m_oMaskInst = output;
            }
            else if (outputType == SHADER_OUTPUT_TYPE_DEFAULT)
            {
                m_outputs.push_back(output);
            }
            else
            {
                IGC_ASSERT_MESSAGE(false, "Unsupported output type found");
            }
        }
    }
}

bool CPSMSAAOMaskWA::runOnModule(Module& M)
{
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    //ModuleMetaData* modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
    NamedMDNode* coarseNode = M.getNamedMetadata(NAMED_METADATA_COARSE_PHASE);
    if (!(coarseNode != nullptr && IGC_IS_FLAG_ENABLED(EnableCPSMSAAOMaskWA) &&
          ctx->platform.getWATable().Wa_22012766191))
    {
        return false;
    }

    LLVM3DBuilder<false> builder(M.getContext(), ctx->platform.getPlatformInfo());
    m_pModule = &M;
    m_oMaskInst = nullptr;
    m_outputs.clear();
    visit(M);
    if (m_oMaskInst == nullptr)
    {
        return false;
    }

    unsigned int maxOutputIndex = 0;
    for (auto output : m_outputs)
    {
        Type* outputType = output->getArgOperand(0)->getType();
        ConstantInt* outputIndexValue = cast<ConstantInt>(output->getArgOperand(5));
        unsigned int outputIndex = int_cast<unsigned int>(outputIndexValue->getZExtValue());
        maxOutputIndex = std::max(outputIndex, maxOutputIndex);
        Function* phaseOutput = GenISAIntrinsic::getDeclaration(
            m_pModule,
            GenISAIntrinsic::GenISA_PHASE_OUTPUT,
            outputType);
        builder.SetInsertPoint(output);
        for (int i = 0; i < 4; ++i)
        {
            builder.CreateCall2(phaseOutput, output->getArgOperand(i), builder.getInt32(outputIndex * 4 + i));
        }
    }

    Type* omaskType = m_oMaskInst->getArgOperand(0)->getType();
    Function* omaskPhaseOutput = GenISAIntrinsic::getDeclaration(
        m_pModule,
        GenISAIntrinsic::GenISA_PHASE_OUTPUT,
        omaskType);
    unsigned int omaskOffset = m_outputs.size() == 0 ? 0 : (maxOutputIndex + 1) * 4;
    Value* omaskIndex = builder.getInt32(omaskOffset);
    builder.SetInsertPoint(m_oMaskInst);
    builder.CreateCall2(omaskPhaseOutput, m_oMaskInst->getArgOperand(0), omaskIndex);

    ConstantInt* positionXIndex = builder.getInt32(omaskOffset + 1);
    ConstantInt* positionYIndex = builder.getInt32(omaskOffset + 2);
    Function* svFunc = GenISAIntrinsic::getDeclaration(
        &M, GenISAIntrinsic::GenISA_DCL_SystemValue, builder.getFloatTy());
    Function* intSVFunc = GenISAIntrinsic::getDeclaration(
        &M, GenISAIntrinsic::GenISA_DCL_SystemValue, builder.getInt32Ty());
    Value* positionX = builder.CreateCall(svFunc, builder.getInt32(POSITION_X));
    positionX = builder.CreateFPToUI(positionX, builder.getInt32Ty());
    positionX = builder.CreateBitCast(positionX, builder.getFloatTy());
    Value* positionY = builder.CreateCall(svFunc, builder.getInt32(POSITION_Y));
    positionY = builder.CreateFPToUI(positionY, builder.getInt32Ty());
    positionY = builder.CreateBitCast(positionY, builder.getFloatTy());

    Function* positionPhaseOutput = GenISAIntrinsic::getDeclaration(
        m_pModule, GenISAIntrinsic::GenISA_PHASE_OUTPUT, builder.getFloatTy());
    builder.CreateCall2(positionPhaseOutput, positionX, positionXIndex);
    builder.CreateCall2(positionPhaseOutput, positionY, positionYIndex);

    MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    for (auto& F : M.functions())
    {
        if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
        {
            continue;
        }

        // Create the pixel phase.
        std::string perPixelName = "_per_pixel_" + std::string(F.getName());
        FunctionType* ft = FunctionType::get(builder.getVoidTy(), false);
        Function* perPixel = Function::Create(
            ft,
            GlobalValue::LinkageTypes::ExternalLinkage,
            perPixelName,
            &M);
        MetadataBuilder metadata(&M);
        metadata.SetShadingRate(perPixel, PSPHASE_PIXEL);
        IGC::IGCMD::IGCMetaDataHelper::addFunction(*pMdUtils, perPixel);
        BasicBlock::Create(M.getContext(), "", perPixel);

        builder.SetInsertPoint(&perPixel->getEntryBlock());
        Function* omaskPhaseInput = GenISAIntrinsic::getDeclaration(
            &M,
            GenISAIntrinsic::GenISA_PHASE_INPUT,
            omaskType);
        Value* omaskPixelInput = builder.CreateCall(omaskPhaseInput, omaskIndex);
        omaskPixelInput = builder.CreateBitCast(omaskPixelInput, builder.getInt32Ty());

        Function* floatPhaseInput = GenISAIntrinsic::getDeclaration(
            &M,
            GenISAIntrinsic::GenISA_PHASE_INPUT,
            builder.getFloatTy());
        Value* resourceIndex = builder.getInt32(0);
        unsigned int addrSpaceRsc = IGC::EncodeAS4GFXResource(*resourceIndex, RENDER_TARGET);
        PointerType* ptrTy = llvm::PointerType::get(builder.getInt32Ty(), addrSpaceRsc);
        Value* rastResource = ConstantPointerNull::get(ptrTy);
        Value* numSamples = builder.CreateExtractElement(builder.Create_SampleInfo(rastResource), builder.getInt32(0));
        Value* coarseSizeX = builder.CreateCall(intSVFunc, builder.getInt32(ACTUAL_COARSE_SIZE_X));
        Value* coarseSizeY = builder.CreateCall(intSVFunc, builder.getInt32(ACTUAL_COARSE_SIZE_Y));
        Value* coarsePositionX = builder.CreateCall(floatPhaseInput, positionXIndex);
        coarsePositionX = builder.CreateBitCast(coarsePositionX, builder.getInt32Ty());
        Value* coarsePositionY = builder.CreateCall(floatPhaseInput, positionYIndex);
        coarsePositionY = builder.CreateBitCast(coarsePositionY, builder.getInt32Ty());
        Value* pixelPositionX = builder.CreateCall(svFunc, builder.getInt32(POSITION_X));
        pixelPositionX = builder.CreateFPToUI(pixelPositionX, builder.getInt32Ty());
        Value* pixelPositionY = builder.CreateCall(svFunc, builder.getInt32(POSITION_Y));
        pixelPositionY = builder.CreateFPToUI(pixelPositionY, builder.getInt32Ty());
        Value* xInCoarsePixel = builder.CreateSub(coarsePositionX, pixelPositionX);
        Value* yInCoarsePixel = builder.CreateSub(coarsePositionY, pixelPositionY);
        Value* pixelsInCoarsePixel = builder.CreateMul(coarseSizeX, coarseSizeY);
        Value* indexInCoarsePixel = builder.CreateMul(coarseSizeX, yInCoarsePixel);
        indexInCoarsePixel = builder.CreateAdd(indexInCoarsePixel, xInCoarsePixel);
        indexInCoarsePixel = builder.CreateSub(builder.CreateSub(pixelsInCoarsePixel, builder.getInt32(1)), indexInCoarsePixel);
        indexInCoarsePixel = builder.CreateMul(indexInCoarsePixel, numSamples);
        Value* perPixelOMask = builder.CreateAShr(omaskPixelInput, indexInCoarsePixel);
        perPixelOMask = builder.CreateAnd(
            perPixelOMask,
            builder.CreateSub(builder.CreateShl(builder.getInt32(1), numSamples), builder.getInt32(1)));
        Value* floatUndef = UndefValue::get(builder.getFloatTy());
        Value* omaskOutputArgs[6] =
        {
            floatUndef,
            floatUndef,
            floatUndef,
            floatUndef,
            builder.getInt32(SHADER_OUTPUT_TYPE_OMASK),
            builder.getInt32(0)
        };

        omaskOutputArgs[0] = builder.CreateBitCast(perPixelOMask, builder.getFloatTy());
        Function* omaskOutput = GenISAIntrinsic::getDeclaration(
            &M,
            GenISAIntrinsic::GenISA_OUTPUT,
            builder.getFloatTy());
        builder.CreateCall(omaskOutput, omaskOutputArgs);

        for (auto output : m_outputs)
        {
            Type* outputType = output->getArgOperand(0)->getType();
            ConstantInt* outputIndexValue = cast<ConstantInt>(output->getArgOperand(5));
            unsigned int outputIndex = int_cast<unsigned int>(outputIndexValue->getZExtValue());
            Function* phaseInput = GenISAIntrinsic::getDeclaration(
                &M,
                GenISAIntrinsic::GenISA_PHASE_INPUT,
                outputType);
            Function* outputFn = GenISAIntrinsic::getDeclaration(
                &M,
                GenISAIntrinsic::GenISA_OUTPUT,
                outputType);
            Value* pixelOutputArgs[6] =
            {
                floatUndef,
                floatUndef,
                floatUndef,
                floatUndef,
                builder.getInt32(SHADER_OUTPUT_TYPE_DEFAULT),
                builder.getInt32(outputIndex)
            };
            for (int i = 0; i < 4; ++i)
            {
                Value* inIndexValue = builder.getInt32(outputIndex * 4 + i);
                pixelOutputArgs[i] = builder.CreateCall(phaseInput, inIndexValue);
            }
            builder.CreateCall(outputFn, pixelOutputArgs);
        }
        builder.CreateRetVoid();
        break;
    }

    for (auto inst : m_outputs)
    {
        inst->eraseFromParent();
    }

    m_oMaskInst->eraseFromParent();

    return true;
}