/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "ShaderTypesEnum.h"
#include "CollectGeometryShaderProperties.hpp"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "Compiler/InitializePasses.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;


char CollectGeometryShaderProperties::ID = 0;
#define PASS_FLAG "collectgeometryshaderproperties"
#define PASS_DESCRIPTION "Collect GS Properties"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CollectGeometryShaderProperties, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(CollectGeometryShaderProperties, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)


CollectGeometryShaderProperties::CollectGeometryShaderProperties()
    : ImmutablePass(ID)
{
    initializeCollectGeometryShaderPropertiesPass(*PassRegistry::getPassRegistry());
}


void CollectGeometryShaderProperties::gatherInformation(Function& F)
{
    ExtractGlobalVariables(F);
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_gsProps.Input().PerVertex().HasVertexHeader(ctx->getModuleMetaData()->URBInfo.hasVertexHeader);
    visit(F);
}

void CollectGeometryShaderProperties::visitCallInst(llvm::CallInst& I)
{
    if (GenIntrinsicInst * CI = dyn_cast<GenIntrinsicInst>(&I))
    {
        switch (CI->getIntrinsicID())
        {
        case llvm::GenISAIntrinsic::GenISA_GsCutControlHeader:
        case llvm::GenISAIntrinsic::GenISA_GsStreamHeader:
        {
            HandleCutOrStreamHeader(*CI);
            break;
        }

        case llvm::GenISAIntrinsic::GenISA_DCL_GSsystemValue:
        case llvm::GenISAIntrinsic::GenISA_DCL_SystemValue:
        {
            HandleSystemInput(*CI);
            break;
        }

        case llvm::GenISAIntrinsic::GenISA_OUTPUTGS:
        {
            HandleOutputWrite(*CI);
            break;
        }

        default:
            //nothing to do
            break;
        }
    }
}

void CollectGeometryShaderProperties::ExtractGlobalVariables(llvm::Function& F)
{
    llvm::Module* module = F.getParent();
    // GsMaxOutputVertices/GsOutputPrimitiveTopology are all global variables
    // set in the front-end with the same names. These have to be retrieved and typecast correctly.
    llvm::GlobalVariable* pGlobal = module->getGlobalVariable("GsMaxOutputVertices");
    unsigned int maxOutputVertices = int_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
    m_gsProps.Output().MaxVertexCount(maxOutputVertices);

    pGlobal = module->getGlobalVariable("GsOutputPrimitiveTopology");
    auto outputTopologyType = static_cast<USC::GFX3DPRIMITIVE_TOPOLOGY_TYPE>
        (llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
    m_gsProps.Output().TopologyType(outputTopologyType);

    pGlobal = module->getGlobalVariable("SamplerCount");
    auto samplerCount = llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue();
    m_gsProps.SamplerCount(int_cast<unsigned int>(samplerCount));

    pGlobal = module->getGlobalVariable("GsInputPrimitiveType");
    auto inputPrimitiveType = static_cast<IGC::GSHADER_INPUT_PRIMITIVE_TYPE>
        (llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
    m_gsProps.Input().InputPrimitiveType(inputPrimitiveType);

    pGlobal = module->getGlobalVariable("GsMaxInputAttributeCount");
    auto maxInputAttrCount = (llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
    m_gsProps.Input().PerVertex().MaxAttributeCount(int_cast<unsigned int>(maxInputAttrCount));

    pGlobal = module->getGlobalVariable("CutEncountered");
    auto cutEncountered = llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue() > 0 ? true : false;
    m_gsProps.Output().HasNontrivialCuts(cutEncountered);

    pGlobal = module->getGlobalVariable("DefaultStreamID");
    auto defaultStreamID = static_cast<int>(
        (llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue()));
    m_gsProps.Output().DefaultStreamID(defaultStreamID);

    pGlobal = module->getGlobalVariable("GsInstanceCount");
    auto instanceCount = (pGlobal == nullptr) ? 0 :
        static_cast<unsigned int>(
        (llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue()));
    m_gsProps.Input().InstanceCount(instanceCount);

    //see if clip and cull were sent as input
    CodeGenContext* context = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    if (context->getModuleMetaData()->URBInfo.has64BVertexHeaderInput)
    {
        m_gsProps.Input().PerVertex().HasClipCullDistances(true);
    }
    if (context->getModuleMetaData()->URBInfo.has64BVertexHeaderOutput)
    {
        m_gsProps.Output().PerVertex().HasClipCullDistances(true);
    }

}

void CollectGeometryShaderProperties::HandleSystemInput(llvm::GenIntrinsicInst& I)
{
    unsigned int sgvIndex = I.getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_DCL_GSsystemValue ? 1 : 0;
    SGVUsage usage = static_cast<SGVUsage>
        (llvm::cast<llvm::ConstantInt>(I.getOperand(sgvIndex))->getZExtValue());
    // If input usage contains clip distances, we need a bigger
    // vertex header to accommodate for these values.
    switch (usage)
    {
    case CLIP_DISTANCE_X:
    case CLIP_DISTANCE_Y:
    case CLIP_DISTANCE_Z:
    case CLIP_DISTANCE_W:
        m_gsProps.Input().PerVertex().HasVertexHeader(true);
        m_gsProps.Input().PerVertex().HasClipCullDistances(true);
        break;

    case POSITION_X:
    case POSITION_Y:
    case POSITION_Z:
    case POSITION_W:
        m_gsProps.Input().PerVertex().HasVertexHeader(true);

    case POINT_WIDTH:
        m_gsProps.Input().PerVertex().HasVertexHeader(true);

    case GS_INSTANCEID:
        m_gsProps.Input().HasInstanceID(true);
        break;

    case PRIMITIVEID:
        m_gsProps.Input().HasPrimitiveID(true);
        break;
    default:
        break;
    }
}

void CollectGeometryShaderProperties::HandleOutputWrite(llvm::GenIntrinsicInst& I)
{
    const unsigned int shaderTypeArgIdx = 4;
    const unsigned int shaderAttrArgIdx = 5;
    ShaderOutputType outType = static_cast<ShaderOutputType>(
        llvm::cast<llvm::ConstantInt>(I.getOperand(shaderTypeArgIdx))->getZExtValue());
    switch (outType)
    {
    case SHADER_OUTPUT_TYPE_CLIPDISTANCE_LO:
    case SHADER_OUTPUT_TYPE_CLIPDISTANCE_HI:
        // shader has output clip or cull distances
        // this should have been recognized already by global variable extraction
        // that deals with clip or cull distance masks
        m_gsProps.Output().PerVertex().HasClipCullDistances(true);
        break;
    case SHADER_OUTPUT_TYPE_VIEWPORT_ARRAY_INDEX:
        m_gsProps.Output().HasViewportArrayIndex(true);
        break;
    case SHADER_OUTPUT_TYPE_RENDER_TARGET_ARRAY_INDEX:
        m_gsProps.Output().HasRenderTargetArrayIndex(true);
        break;
    case SHADER_OUTPUT_TYPE_DEFAULT:
    {
        unsigned int attribIdx = int_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(
            I.getOperand(shaderAttrArgIdx))->getZExtValue());
        unsigned int maxattribcount = std::max(
            attribIdx + 1,
            m_gsProps.Output().PerVertex().MaxAttributeCount());
        m_gsProps.Output().PerVertex().MaxAttributeCount(maxattribcount);
        break;
    }
    default:
        // we don't need to do anything for them
        break;
    }
}

void CollectGeometryShaderProperties::HandleCutOrStreamHeader(llvm::GenIntrinsicInst& I)
{
    // set control data format based on the intrinsic type
    auto format = (I.getIntrinsicID() == llvm::GenISAIntrinsic::GenISA_GsCutControlHeader) ?
        USC::GFX3DSTATE_CONTROL_DATA_FORMAT_CUT : USC::GFX3DSTATE_CONTROL_DATA_FORMAT_SID;
    m_gsProps.Output().ControlDataFormat(format);

    auto pVertexCount = llvm::dyn_cast<llvm::ConstantInt>(I.getOperand(I.getNumOperands() - 2));
    // if the emitCount is not a constant int, we have runtime value of vertex count
    auto ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    m_gsProps.Output().HasNonstaticVertexCount(pVertexCount == nullptr || ctx->platform.disableStaticVertexCount());

    // for static number of vertices, we get the actual value,
    // for dynamic, declaration-derived upper bound (for safety)
    const unsigned int numVertices = (pVertexCount != nullptr) ?
        int_cast<unsigned int>(pVertexCount->getZExtValue()) : m_gsProps.Output().MaxVertexCount();

    m_gsProps.Output().ActualStaticVertexCount(numVertices);

    if (IGC_IS_FLAG_ENABLED(EnableGSURBEntryPadding))
    {
        if (!m_gsProps.Output().ControlDataHeaderRequired() &&
            m_gsProps.Output().HasNonstaticVertexCount())
        {
            m_gsProps.Output().SetControlDataHeaderPaddingRequired(true);
            m_gsProps.Output().ControlDataFormat(USC::GFX3DSTATE_CONTROL_DATA_FORMAT_CUT);
        }
    }
    if (IGC_IS_FLAG_ENABLED(EnableGSVtxCountMsgHalfCLSize))
    {
        if (m_gsProps.Output().HasNonstaticVertexCount())
        {
            m_gsProps.Output().HasVtxCountMsgHalfCLSize(true);
        }
    }
}

