/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/Image3dToImage2darray/Image3dToImage2darray.hpp"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/Optimizer/OCLBIUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

#define PASS_FLAG "igc-3d-to-2darray"
#define PASS_DESCRIPTION "Converts 3d images access to 2d array image accesses where possible"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(Image3dToImage2darray, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(Image3dToImage2darray, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char Image3dToImage2darray::ID = 0;

Image3dToImage2darray::Image3dToImage2darray() :
    FunctionPass(ID), m_Changed(false), m_MetadataUtils(nullptr), m_modMD(nullptr)
{
    initializeImage3dToImage2darrayPass(*PassRegistry::getPassRegistry());
}

bool Image3dToImage2darray::createImageAnnotations(
    GenIntrinsicInst* pCall,
    unsigned imageIdx,
    const MetaDataUtils* pMdUtils,
    const ModuleMetaData* modMD,
    const Value* pCoord)
{
    uint argNum = 0;
    auto* pFunc = pCall->getParent()->getParent();
    FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(pFunc);
    auto* pImgOp = pCall->getArgOperand(imageIdx);

    // Find the arg number of the image so we can look up its ArgInfoList metadata.
    if (pImgOp->getType()->isPointerTy())
    {
        auto* pImg = ValueTracker::track(pCall, imageIdx, pMdUtils, modMD);
        if (pImg == nullptr)
            return false;

        auto* pImgArg = cast<Argument>(pImg);

        argNum = pImgArg->getArgNo();
    }
    else if (auto * Idx = dyn_cast<ConstantInt>(pImgOp))
    {
        uint64_t IdxVal = Idx->getZExtValue();
        auto* arg = CImagesBI::CImagesUtils::findImageFromBufferPtr(*pMdUtils, pFunc, RESOURCE, IdxVal, modMD);

        if (!arg)
            return false;

        argNum = arg->getArgNo();
    }
    else
    {
        return false;
    }

    ArgInfoMetaDataHandle argHandle;
    bool  found = false;
    if (funcInfoMD->isArgInfoListHasValue())
    {
        for (auto AI = funcInfoMD->begin_ArgInfoList(), AE = funcInfoMD->end_ArgInfoList(); AI != AE; ++AI)
        {
            ArgInfoMetaDataHandle arg = *AI;
            if (arg->getExplicitArgNum() == argNum)
            {
                argHandle = arg;
                found = true;
                break;
            }
        }
    }

    if (!found)
    {
        argHandle = ArgInfoMetaDataHandle(new ArgInfoMetaData());
        argHandle->setExplicitArgNum(argNum);
        funcInfoMD->addArgInfoListItem(argHandle);
    }

    if (!argHandle->isImgAccessFloatCoordsHasValue())
        argHandle->setImgAccessFloatCoords(false);

    if (!argHandle->isImgAccessIntCoordsHasValue())
        argHandle->setImgAccessIntCoords(false);

    // We check here to see if the third coord (or for 2darray, the image layer)
    // has an integer origin.  For samplers of:
    // UNNORMALIZED COORDS, NEAREST FILTERTING, CLAMP_TO_EDGE ADDRESSING
    // In OCL:
    // 3D Images 3rd component: k = clamp((int)floor(w), 0, size-1)
    // 2D Image arrays: layer = clamp((int)rint(w), 0, size-1)
    // The two will match when floor(w) == rint(w)
    bool intCoords = CImagesBI::derivedFromInt(pCoord);

    if (intCoords)
        argHandle->setImgAccessIntCoords(true);
    else
        argHandle->setImgAccessFloatCoords(true);

    return true;
}

void Image3dToImage2darray::visitCallInst(CallInst& CI)
{
    GenIntrinsicInst* pCall = nullptr;
    if ((pCall = dyn_cast<GenIntrinsicInst>(&CI)) == nullptr)
        return;

    switch (pCall->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_ldptr:
    {
        auto* pLoad = cast<SamplerLoadIntrinsic>(pCall);
        auto* pCoord = pCall->getArgOperand(3);
        m_Changed |= createImageAnnotations(pCall, pLoad->getTextureIndex(), m_MetadataUtils, m_modMD, pCoord);
        break;
    }
    case GenISAIntrinsic::GenISA_sampleLptr:
    {
        auto* pSample = cast<SampleIntrinsic>(pCall);
        auto* pCoord = pCall->getArgOperand(3);
        m_Changed |= createImageAnnotations(pCall, pSample->getTextureIndex(), m_MetadataUtils, m_modMD, pCoord);
        break;
    }
    default:
        return;
    }
}

bool Image3dToImage2darray::runOnFunction(Function& F)
{
    m_MetadataUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_modMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

    // This pass is not compatible with the SPV_INTEL_bindless_images extension.
    // The incompatibility arises because this pass requires images to be trackable
    // at compile time, a condition that bindless images from the SPV_INTEL_bindless_images
    // extension do not satisfy.
    if (m_modMD->extensions.spvINTELBindlessImages)
    {
        return false;
    }

    if (m_MetadataUtils->findFunctionsInfoItem(&F) == m_MetadataUtils->end_FunctionsInfo())
    {
        return false;
    }

    visit(F);

    if (m_Changed)
    {
        m_MetadataUtils->save(F.getContext());
    }

    return m_Changed;
}
