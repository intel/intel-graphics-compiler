/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "llvm/ADT/StringSwitch.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/PixelShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "common/allocator.h"
#include <iStdLib/utility.h>
#include "common/secure_mem.h"
#include "Simd32Profitability.hpp"
#include "EmitVISAPass.hpp"
#include "AdaptorCommon/API/igc.h"
#include "Probe/Assertion.h"

/***********************************************************************************
This file contains the code specific to pixel shader
************************************************************************************/
using namespace llvm;
using namespace IGC::IGCMD;

namespace IGC
{
CVariable* CPixelShader::GetR1()
{
    return m_R1;
}

std::vector<CVariable*>& CPixelShader::GetR1Lo()
{
    return m_R1Lo;
}

void CPixelShader::AppendR1Lo(CVariable* var)
{
    m_R1Lo.push_back(var);
}

CVariable* CPixelShader::GetCoarseR1()
{
    IGC_ASSERT(m_phase == PSPHASE_PIXEL);
    return m_CoarseR1;
}

void CPixelShader::AllocatePayload()
{
    if (m_phase == PSPHASE_COARSE)
    {
        CreatePassThroughVar();
    }
    switch (m_phase)
    {
    case PSPHASE_LEGACY:
    case PSPHASE_COARSE:
        AllocatePSPayload();
        break;
    case PSPHASE_PIXEL:
        AllocatePixelPhasePayload();
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "unknown phase");
        break;
    }
}

void CPixelShader::AllocatePixelPhasePayload()
{
    unsigned int r1Offset = GetDispatchSignature().r1;
    AllocateInput(m_CoarseR1, r1Offset);
    for (uint i = 0; i < setup.size(); i++)
    {
        if (setup[i])
        {
            uint subRegOffset = 0;
            // PS uniform inputs are stored in the 3rd subreg
            if (setup[i]->GetSize() == SIZE_DWORD)
            {
                subRegOffset = 3 * SIZE_DWORD;
            }
            IGC_ASSERT(m_Signature != nullptr);
            uint offset = GetDispatchSignature().inputOffset[i];
            AllocateInput(setup[i], offset + subRegOffset);
        }
    }
    if (m_ZWDelta)
    {
        unsigned int offset = GetDispatchSignature().ZWDelta;
        AllocateInput(m_ZWDelta, offset);
    }
    if (m_SampleOffsetX || m_SampleOffsetY)
    {
        unsigned int offset = GetDispatchSignature().pixelOffset;
        if (m_SampleOffsetX)
        {
            AllocateInput(m_SampleOffsetX, offset);
        }
        if (m_SampleOffsetY)
        {
            AllocateInput(m_SampleOffsetY, offset + SIZE_OWORD);
        }
    }
    for (auto it = m_CoarseInput.begin(), ie = m_CoarseInput.end(); it != ie; ++it)
    {
        uint offset = GetDispatchSignature().PSOutputOffset.find(it->first)->second;
        AllocateInput(it->second, offset);
    }
    if (m_CoarseMaskInput)
    {
        uint offset = GetDispatchSignature().oMaskOffset;
        AllocateInput(m_CoarseMaskInput, offset);
    }
}

void CPixelShader::AllocatePSPayload()
{
    bool forceLiveOut = false;
    // In bytes
    uint offset = 0;

    // R0 is always allocated as a predefined variable. Increase offset for R0
    IGC_ASSERT(m_R0);
    if (encoder.IsCodePatchCandidate())
    {
        // For the payload section, we need to mark inputs to be outputs
        // so that inputs will be alive across the entire payload section
        forceLiveOut = true;
        encoder.MarkAsPayloadLiveOut(m_R0);
    }
    offset += getGRFSize();

    if (m_Signature)
    {
        GetDispatchSignature().r1 = offset;
    }

    {
        IGC_ASSERT(GetR1());
        for (uint i = 0; i < GetR1()->GetNumberInstance(); i++)
        {
            AllocateInput(GetR1(), offset, i, forceLiveOut);
            offset += getGRFSize();
        }
    }

    uint numInstances = m_numberInstance;

    for (uint i = 0; i < numInstances; i++)
    {
        // allocate size for bary
        if (m_PerspectivePixel)
        {
            AllocateInput(m_PerspectivePixel, offset, i, forceLiveOut);
            offset += m_PerspectivePixel->GetSize();
        }
        if (m_PerspectiveCentroid)
        {
            AllocateInput(m_PerspectiveCentroid, offset, i, forceLiveOut);
            offset += m_PerspectiveCentroid->GetSize();
        }
        if (m_PerspectiveSample)
        {
            AllocateInput(m_PerspectiveSample, offset, i, forceLiveOut);
            offset += m_PerspectiveSample->GetSize();
        }
        if (m_NoPerspectivePixel)
        {
            AllocateInput(m_NoPerspectivePixel, offset, i, forceLiveOut);
            offset += m_NoPerspectivePixel->GetSize();
        }
        if (m_NoPerspectiveCentroid)
        {
            AllocateInput(m_NoPerspectiveCentroid, offset, i, forceLiveOut);
            offset += m_NoPerspectiveCentroid->GetSize();
        }
        if (m_NoPerspectiveSample)
        {
            AllocateInput(m_NoPerspectiveSample, offset, i, forceLiveOut);
            offset += m_NoPerspectiveSample->GetSize();
        }

        // Add support for POSITION_Z
        if (m_pPositionZPixel)
        {
            AllocateInput(m_pPositionZPixel, offset, i, forceLiveOut);
            offset += m_pPositionZPixel->GetSize();
        }

        // Add support for POSITION_W
        if (m_pPositionWPixel)
        {
            AllocateInput(m_pPositionWPixel, offset, i, forceLiveOut);
            offset += m_pPositionWPixel->GetSize();
        }

        // Add support for POSITION_XY_OFFSET
        if (m_pPositionXYOffset)
        {
            {
                AllocateInput(m_pPositionXYOffset, offset, i, forceLiveOut);
                offset += m_pPositionXYOffset->GetSize();
            }
        }

        // Add support for input coverage mask
        if (m_pInputCoverageMask)
        {
            AllocateInput(m_pInputCoverageMask, offset, i, forceLiveOut);
            offset += m_pInputCoverageMask->GetSize();
        }

        {
            if (m_pCPSRequestedSizeX || m_pCPSRequestedSizeY)
            {
                if (m_pCPSRequestedSizeX)
                {
                    AllocateInput(m_pCPSRequestedSizeX, offset, i, forceLiveOut);
                }
                if (m_pCPSRequestedSizeY)
                {
                    AllocateInput(m_pCPSRequestedSizeY, offset + SIZE_OWORD, i, forceLiveOut);
                }
                offset += getGRFSize();
            }
            if (m_ZWDelta && i == numInstances - 1)
            {
                AllocateInput(m_ZWDelta, offset, 0, forceLiveOut);
                if (m_Signature)
                {
                    GetDispatchSignature().ZWDelta = offset;
                }
                offset += getGRFSize();
            }
            if (m_SampleOffsetX || m_SampleOffsetY)
            {
                if (m_SampleOffsetX)
                {
                    AllocateInput(m_SampleOffsetX, offset, i, forceLiveOut);
                }
                if (m_SampleOffsetY)
                {
                    AllocateInput(m_SampleOffsetY, offset + SIZE_OWORD, i, forceLiveOut);
                }
                if (m_Signature)
                {
                    GetDispatchSignature().pixelOffset = offset;
                }
                offset += getGRFSize();
            }
        }
    }

    IGC_ASSERT(offset % getGRFSize() == 0);

    // need to return the starting grf for constant to client
    ProgramOutput()->m_startReg = offset / getGRFSize();

    // allocate space for NOS constants and pushed constants
    AllocateConstants3DShader(offset);

    // RPC0 and subsequent regs: per-primitive constant data
    for (uint index = 0; index < perPrimitiveSetup.size(); index++)
    {
        CVariable* var = perPrimitiveSetup[index];
        if (var && var->GetAlias() == NULL)
        {
            AllocateInput(var, offset);
        }

        if (IsDualSIMD8())
        {
            offset += 2 * getGRFSize();
        }
        else
        {
            offset += getGRFSize();
        }
    }

    // Allocate size for attributes coming from VS
    IGC_ASSERT(offset % getGRFSize() == 0);
    unsigned int payloadEnd = offset;
    for (uint i = 0; i < setup.size(); i++)
    {
        if (setup[i] && setup[i]->GetAlias() == NULL)
        {
            uint subRegOffset = 0;
            // PS uniform (constant interpolation) inputs
            if (setup[i]->GetSize() == SIZE_DWORD)
            {
                {
                    subRegOffset = 3 * SIZE_DWORD;
                }
            }

            AllocateInput(setup[i], offset + subRegOffset, 0, forceLiveOut);

            if (m_Signature)
            {
                GetDispatchSignature().inputOffset[i] = offset;
            }
            payloadEnd = offset + setup[i]->GetSize();
        }

        {
            offset += 4 * SIZE_DWORD;
        }
    }

    offset = payloadEnd;

    // For code patching, we preallocate live-out of payload into physical registers.
    // The preallocation must be aligned across contexts to ensure it is patchable.
    if (encoder.IsCodePatchCandidate())
    {
        if (encoder.HasPrevKernel())
        {
            // Get previous context's PayloadEnd to ensure it is patchable
            offset = encoder.GetPayloadEnd();
        }
        else
        {
            encoder.SetPayloadEnd(payloadEnd);
        }
    }
    if (offset % getGRFSize() != 0)
    {
        offset += (getGRFSize() - (offset % getGRFSize()));
    }

    // This is the preallocation for payload live-outs.
    for (auto& var : payloadLiveOutSetup)
    {
        IGC_ASSERT(offset% getGRFSize() == 0);
        AllocateInput(var, offset, 0, true);
        offset += var->GetSize();
    }

    // This is the preallocation for temp variables in payload sections
    for (auto& var : payloadTempSetup)
    {
        AllocateInput(var, offset);
        offset += var->GetSize();
    }

    // When preallocation failed (exceeding the total number of physical registers), early exit and give up this compilation._
    ProgramOutput()->m_scratchSpaceUsedBySpills = offset >= encoder.GetVISAKernel()->getNumRegTotal() * getGRFSize();
    if (ProgramOutput()->m_scratchSpaceUsedBySpills)
    {
        return;
    }


    // create output registers for coarse phase
    for (const auto& it : m_CoarseOutput)
    {
        CVariable* output = it.second;
        offset = iSTD::Align(offset, (size_t) 1 << output->GetAlign());
        AllocateOutput(output, offset);
        if (m_Signature)
        {
            GetDispatchSignature().PSOutputOffset[it.first] = offset;
        }
        offset += output->GetSize();
    }
    if (m_CoarseoMask)
    {
        offset = iSTD::Align(offset, (size_t) 1 << m_CoarseoMask->GetAlign());
        AllocateOutput(m_CoarseoMask, offset);
        if (m_Signature)
        {
            GetDispatchSignature().oMaskOffset = offset;
            GetDispatchSignature().CoarseMask = true;
        }
        offset += m_CoarseoMask->GetSize();
    }
    ProgramOutput()->m_scratchSpaceUsedBySpills = (offset >= encoder.GetVISAKernel()->getNumRegTotal() * getGRFSize());
}

PSSignature::DispatchSignature& CPixelShader::GetDispatchSignature()
{
    switch (m_dispatchSize)
    {
    case SIMDMode::SIMD8:
        return m_Signature->dispatchSign[0];
    case SIMDMode::SIMD16:
        return m_Signature->dispatchSign[1];
    case SIMDMode::SIMD32:
        return m_Signature->dispatchSign[2];
    default:
        IGC_ASSERT_MESSAGE(0, "bad dispatch size");
        break;
    }
    return m_Signature->dispatchSign[0];
}

CVariable* CPixelShader::GetBaryReg(e_interpolation mode)
{
    uint numInstances = m_numberInstance;
    uint numElements = 2 * numLanes(m_SIMDSize);


    CVariable* baryReg = 0;
    switch (mode)
    {
    case EINTERPOLATION_LINEAR:
        if (!m_PerspectivePixel) {
            m_PerspectivePixel =
                GetNewVariable(numElements, ISA_TYPE_F, EALIGN_GRF, false, numInstances, "PerspectivePixel");
        }
        baryReg = m_PerspectivePixel;
        break;
    case EINTERPOLATION_LINEARCENTROID:
        if (!m_PerspectiveCentroid) {
            m_PerspectiveCentroid =
                GetNewVariable(
                    numElements, ISA_TYPE_F, EALIGN_GRF,
                    false, numInstances, "LinearCentroid");
        }
        baryReg = m_PerspectiveCentroid;
        break;
    case EINTERPOLATION_LINEARSAMPLE:
        if (!m_PerspectiveSample) {
            m_PerspectiveSample =
                GetNewVariable(
                    numElements, ISA_TYPE_F, EALIGN_GRF,
                    false, numInstances, "LinearSample");
        }
        baryReg = m_PerspectiveSample;
        break;
    case EINTERPOLATION_LINEARNOPERSPECTIVE:
        if (!m_NoPerspectivePixel) {
            m_NoPerspectivePixel =
                GetNewVariable(
                    numElements, ISA_TYPE_F, EALIGN_GRF,
                    false, numInstances, "LinearNoPerspective");
        }
        baryReg = m_NoPerspectivePixel;
        break;
    case EINTERPOLATION_LINEARNOPERSPECTIVECENTROID:
        if (!m_NoPerspectiveCentroid) {
            m_NoPerspectiveCentroid =
                GetNewVariable(
                    numElements, ISA_TYPE_F, EALIGN_GRF,
                    false, numInstances, "NoPerspectiveCentroid");
        }
        baryReg = m_NoPerspectiveCentroid;
        break;
    case EINTERPOLATION_LINEARNOPERSPECTIVESAMPLE:
        if (!m_NoPerspectiveSample) {
            m_NoPerspectiveSample =
                GetNewVariable(
                    numElements, ISA_TYPE_F, EALIGN_GRF,
                    false, numInstances, "NoPerspectiveSample");
        }
        baryReg = m_NoPerspectiveSample;
        break;
    default:
        IGC_ASSERT(0);
    }
    return baryReg;
}

CVariable* CPixelShader::GetBaryRegLoweredHalf(e_interpolation mode)
{
    IGC_ASSERT(IsInterpolationLinear(mode));

    const char* const name =
        mode == EINTERPOLATION_LINEAR ? "PerspectivePixelLoweredHalf" :
        mode == EINTERPOLATION_LINEARCENTROID ? "PerspectiveCentroidLoweredHalf" :
        mode == EINTERPOLATION_LINEARSAMPLE ? "PerspectiveSampleLoweredHalf" :
        mode == EINTERPOLATION_LINEARNOPERSPECTIVE ? "NoPerspectivePixelLoweredHalf" :
        mode == EINTERPOLATION_LINEARNOPERSPECTIVECENTROID ? "NoPerspectiveCentroidLoweredHalf" :
        mode == EINTERPOLATION_LINEARNOPERSPECTIVESAMPLE ? "NoPerspectiveSampleLoweredHalf" : "";

    if (!m_BaryRegLoweredHalf[mode])
    {
        m_BaryRegLoweredHalf[mode] =
            GetNewVariable(
                2 * numLanes(m_SIMDSize), ISA_TYPE_HF, EALIGN_GRF,
                false, m_numberInstance, name);
        if (encoder.IsCodePatchCandidate())
        {
            AddPatchTempSetup(m_BaryRegLoweredHalf[mode]);
        }
    }
    return m_BaryRegLoweredHalf[mode];
}

CVariable* CPixelShader::GetBaryRegLoweredFloat(e_interpolation mode)
{
    IGC_ASSERT(IsInterpolationLinear(mode));

    const char* const name =
        mode == EINTERPOLATION_LINEAR ? "PerspectivePixelLoweredFloat" :
        mode == EINTERPOLATION_LINEARCENTROID ? "PerspectiveCentroidLoweredFloat" :
        mode == EINTERPOLATION_LINEARSAMPLE ? "PerspectiveSampleLoweredFloat" :
        mode == EINTERPOLATION_LINEARNOPERSPECTIVE ? "NoPerspectivePixelLoweredFloat" :
        mode == EINTERPOLATION_LINEARNOPERSPECTIVECENTROID ? "NoPerspectiveCentroidLoweredFloat" :
        mode == EINTERPOLATION_LINEARNOPERSPECTIVESAMPLE ? "NoPerspectiveSampleLoweredFloat" : "";

    if (!m_BaryRegLoweredFloat[mode])
    {
        m_BaryRegLoweredFloat[mode] =
            GetNewVariable(
                2 * numLanes(m_SIMDSize), ISA_TYPE_F, EALIGN_GRF,
                false, m_numberInstance, name);
        if (encoder.IsCodePatchCandidate())
        {
            AddPatchTempSetup(m_BaryRegLoweredFloat[mode]);
        }
    }
    return m_BaryRegLoweredFloat[mode];
}


CVariable* CPixelShader::GetPerPrimitiveSetupVar(uint inputIndex)
{

    // A single setupVar holds up to GRF of per-primitive data.
    const uint grfSizeInDwords = getGRFSize() / sizeof(DWORD);
    const uint setupIndex = inputIndex / grfSizeInDwords;
    if (setupIndex >= perPrimitiveSetup.size())
    {
        perPrimitiveSetup.resize(setupIndex + 1, nullptr);
    }

    CVariable* inputVar = perPrimitiveSetup[setupIndex];
    if (inputVar == nullptr)
    {
        if (IsDualSIMD8())
        {
            // In dual-simd8 mode per-primitive data is interleaved.
            // Each piece of 32 bytes of data for Primitive0 is followed by
            // 32 bytes of data for Primitve1.

            // Use 2 GRFs per 32 bytes of data.
            inputVar = GetNewVariable(16, ISA_TYPE_F, EALIGN_GRF, CName::NONE);
        }
        else
        {
            inputVar = GetNewVariable(grfSizeInDwords, ISA_TYPE_F, EALIGN_GRF, CName::NONE);
        }
        perPrimitiveSetup[setupIndex] = inputVar;
    }
    return inputVar;
}

CVariable* CPixelShader::GetInputDelta(uint index, bool loweredInput)
{
    CVariable* inputVar = setup[index];
    if (inputVar == nullptr)
    {
        if (loweredInput)
        {
            if (index % 2 == 0)
            {
                inputVar = GetNewVariable(8, ISA_TYPE_F, EALIGN_GRF, true, "InputDelta" + std::to_string(index));
                setup[index + 1] = GetNewAlias(inputVar, ISA_TYPE_F, 16, 4);
            }
            else
            {
                inputVar = GetNewAlias(GetInputDelta(index - 1), ISA_TYPE_F, 16, 4);
            }
        }
        else
        {
            inputVar = GetNewVariable(4, ISA_TYPE_F, EALIGN_OWORD, true, "InputDelta" + std::to_string(index));
        }
        setup[index] = inputVar;
    }
    return inputVar;
}

CVariable* CPixelShader::GetInputDeltaLowered(uint index)
{
    CVariable* inputVar = setupLowered[index];
    if (inputVar == nullptr)
    {
        IGC_ASSERT(LowerPSInput());
        if (index % 2 == 0)
        {
            inputVar = GetNewVariable(8, ISA_TYPE_HF, EALIGN_OWORD, true, CName::NONE);
        }
        else
        {
            if (setupLowered[index - 1])
            {
                inputVar = GetNewAlias(setupLowered[index - 1], ISA_TYPE_HF, 8, 4);
            }
            else
            {
                inputVar = GetNewVariable(4, ISA_TYPE_HF, EALIGN_OWORD, true, CName::NONE);
            }
        }

        setupLowered[index] = inputVar;
    }
    return inputVar;
}

CVariable* CPixelShader::GetZWDelta()
{
    if (!m_ZWDelta)
    {
        uint numLanes = getGRFSize() / SIZE_DWORD ; // single GRF

        m_ZWDelta =
            GetNewVariable(numLanes, ISA_TYPE_F, EALIGN_GRF, false, 1, "ZWDelta");
    }
    return m_ZWDelta;
}


CVariable* CPixelShader::GetPositionZ()
{
    uint16_t numberInstance, numberLanes;
    {
        numberLanes = numLanes(m_SIMDSize);
        numberInstance = m_numberInstance;
    }
    if (!m_pPositionZPixel)
    {
        m_pPositionZPixel =
            GetNewVariable(numberLanes, ISA_TYPE_F, EALIGN_GRF, false, numberInstance, "PosZPixel");
    }
    return m_pPositionZPixel;
}

CVariable* CPixelShader::GetPositionW()
{
    uint16_t numberInstance, numberLanes;
    {
        numberLanes = numLanes(m_SIMDSize);
        numberInstance = m_numberInstance;
    }
    if (!m_pPositionWPixel)
    {
        m_pPositionWPixel =
            GetNewVariable(numberLanes, ISA_TYPE_F, EALIGN_GRF, false, numberInstance, "PosWPixel");
    }
    return m_pPositionWPixel;
}

CVariable* CPixelShader::GetPositionXYOffset()
{
    if (!m_pPositionXYOffset)
    {
        m_pPositionXYOffset =
            GetNewVariable(32, ISA_TYPE_B, EALIGN_GRF, false, m_numberInstance, "PosXYOff");
    }
    return m_pPositionXYOffset;
}

CVariable* CPixelShader::GetInputCoverageMask()
{
    uint16_t numberInstance, numberLanes;
    {
        numberLanes = numLanes(m_SIMDSize);
        numberInstance = m_numberInstance;
    }
    if (!m_pInputCoverageMask)
    {
        m_pInputCoverageMask = GetNewVariable(
                numberLanes, ISA_TYPE_F, EALIGN_GRF, false, numberInstance, "InputCoverageMask");
    }
    return m_pInputCoverageMask;
}

CVariable* CPixelShader::GetSampleOffsetX()
{
    if (!m_SampleOffsetX)
    {
        m_SampleOffsetX = GetNewVariable(16, ISA_TYPE_UB, EALIGN_OWORD, true, "SmplOffX");
    }
    return m_SampleOffsetX;
}

CVariable* CPixelShader::GetSampleOffsetY()
{
    if (!m_SampleOffsetY)
    {
        m_SampleOffsetY = GetNewVariable(16, ISA_TYPE_UB, EALIGN_OWORD, true, "SmplOffY");
    }
    return m_SampleOffsetY;
}

CVariable* CPixelShader::GetCPSRequestedSizeX()
{
    if (!m_pCPSRequestedSizeX)
    {
        m_pCPSRequestedSizeX =
            GetNewVariable(
                numLanes(m_SIMDSize) / 4, ISA_TYPE_F,
                EALIGN_OWORD, false, m_numberInstance, "CPSReqSizeX");
    }
    return m_pCPSRequestedSizeX;
}

CVariable* CPixelShader::GetCPSRequestedSizeY()
{
    if (!m_pCPSRequestedSizeY)
    {
        m_pCPSRequestedSizeY =
            GetNewVariable(
                numLanes(m_SIMDSize) / 4, ISA_TYPE_F,
                EALIGN_OWORD, false, m_numberInstance, "CPSReqSizeY");
    }
    return m_pCPSRequestedSizeY;
}

CPixelShader::CPixelShader(llvm::Function* pFunc, CShaderProgram* pProgram)
    : CShader(pFunc, pProgram)
{
    m_RenderTargetMask = 0;
    m_HasoDepth = false;
    m_HasoStencil = false;
    m_HasoMask = false;
    m_isPerSample = false;
    m_HasInputCoverageMask = false;
    m_HasPullBary = false;
    m_HasCoarseSize = false;
    m_HasDouble = false;
    m_hasDualBlendSource = false;
    m_HasDiscard = false;
    m_IsLastPhase = false;
    m_phase = PSPHASE_LEGACY;
    m_Signature = nullptr;
    m_samplerCount = 0;
    m_ModeUsedHalf.reset();
    m_ModeUsedFloat.reset();
    setupLowered.clear();
    loweredSetupIndexes.clear();

    m_BaryRegLoweredHalf.fill(nullptr);
    m_BaryRegLoweredFloat.fill(nullptr);

    Function* coarsePhase = nullptr;
    Function* pixelPhase = nullptr;
    NamedMDNode* coarseNode = pFunc->getParent()->getNamedMetadata(NAMED_METADATA_COARSE_PHASE);
    NamedMDNode* pixelNode = pFunc->getParent()->getNamedMetadata(NAMED_METADATA_PIXEL_PHASE);
    if (coarseNode)
    {
        coarsePhase = llvm::mdconst::dyn_extract<Function>(coarseNode->getOperand(0)->getOperand(0));
    }
    if (pixelNode)
    {
        pixelPhase = llvm::mdconst::dyn_extract<Function>(pixelNode->getOperand(0)->getOperand(0));
    }
    if (pFunc == coarsePhase)
    {
        m_phase = PSPHASE_COARSE;
    }
    if (coarsePhase && pixelPhase)
    {
        if (pFunc == pixelPhase)
        {
            m_phase = PSPHASE_PIXEL;
            m_IsLastPhase = true;
        }
    }
    else
    {
        m_IsLastPhase = true;
    }
}

CPixelShader::~CPixelShader()
{
}

void CPixelShader::InitEncoder(SIMDMode simdMode, bool canAbortOnSpill, ShaderDispatchMode shaderMode)
{
    m_R1 = NULL;
    m_PerspectiveBaryPlanes = nullptr;
    m_NonPerspectiveBaryPlanes = nullptr;
    m_PerspectivePixel = NULL;
    m_PerspectiveCentroid = NULL;
    m_PerspectiveSample = NULL;
    m_NoPerspectivePixel = NULL;
    m_NoPerspectiveCentroid = NULL;
    m_NoPerspectiveSample = NULL;
    m_BaryRegLoweredHalf.fill(nullptr);
    m_BaryRegLoweredFloat.fill(nullptr);
    m_KillPixelMask = NULL;
    m_HasDiscard = false;
    m_pPositionZPixel = NULL;
    m_pPositionWPixel = NULL;
    m_pPositionXYOffset = NULL;
    m_pInputCoverageMask = NULL;
    m_pCPSRequestedSizeX = NULL;
    m_pCPSRequestedSizeY = NULL;
    m_PixelPhasePayload = nullptr;
    m_PixelPhaseCounter = nullptr;
    m_SampleOffsetX = nullptr;
    m_SampleOffsetY = nullptr;
    m_ZWDelta = nullptr;
    m_hasEOT = false;
    m_NeedPSSync = false;
    m_CoarseoMask = nullptr;
    m_CoarseMaskInput = nullptr;
    m_CoarseR1 = nullptr;

    m_CoarseOutput.clear();
    m_CoarseInput.clear();
    rtWriteList.clear();
    setupLowered.clear();
    loweredSetupIndexes.clear();
    m_ModeUsedHalf.reset();
    m_ModeUsedFloat.reset();
    CShader::InitEncoder(simdMode, canAbortOnSpill, shaderMode);
}


void CShaderProgram::FillProgram(SPixelShaderKernelProgram* pKernelProgram)
{

    const unsigned int InstCacheSize = 0xC000;
    CPixelShader* simd8Shader = static_cast<CPixelShader*>(GetShader(SIMDMode::SIMD8));
    CPixelShader* simd16Shader = static_cast<CPixelShader*>(GetShader(SIMDMode::SIMD16));
    CPixelShader* simd32Shader = static_cast<CPixelShader*>(GetShader(SIMDMode::SIMD32));
    CPixelShader* pShader = nullptr;
    if (simd32Shader)
    {
        const unsigned kernelSize = simd32Shader->m_simdProgram.m_programSize;
        const bool forceSIMD32 =
            (this->GetContext()->getCompilerOption().forcePixelShaderSIMDMode &
                FLAG_PS_SIMD_MODE_FORCE_SIMD32) != 0;

        if ((!simd8Shader && !simd16Shader) ||
            (kernelSize > 0 && (kernelSize < InstCacheSize || forceSIMD32)))
        {
            pKernelProgram->simd32 = *simd32Shader->ProgramOutput();
            pShader = simd32Shader;
            GetContext()->SetSIMDInfo(SIMD_SELECTED, SIMDMode::SIMD32, ShaderDispatchMode::NOT_APPLICABLE);
        }
        else if (kernelSize > 0 && (kernelSize < InstCacheSize))
        {
            GetContext()->SetSIMDInfo(SIMD_SKIP_PERF, SIMDMode::SIMD32, ShaderDispatchMode::NOT_APPLICABLE);
        }
    }
    if (simd16Shader)
    {
        if (!simd8Shader ||
            (simd16Shader->m_simdProgram.m_programSize > 0))
        {
            pKernelProgram->simd16 = *simd16Shader->ProgramOutput();
            pShader = simd16Shader;
            GetContext()->SetSIMDInfo(SIMD_SELECTED, SIMDMode::SIMD16, ShaderDispatchMode::NOT_APPLICABLE);
        }
    }
    {
        if (simd8Shader && simd8Shader->m_simdProgram.m_programSize > 0)
        {
            pKernelProgram->simd8 = *simd8Shader->ProgramOutput();
            pShader = simd8Shader;
            GetContext()->SetSIMDInfo(SIMD_SELECTED, SIMDMode::SIMD8, ShaderDispatchMode::NOT_APPLICABLE);
        }
    }

    if (pShader)
    {
        pShader->FillProgram(pKernelProgram);
    }
    pKernelProgram->SIMDInfo = GetContext()->GetSIMDInfo();
}

void CPixelShader::FillProgram(SPixelShaderKernelProgram* pKernelProgram)
{
    const PixelShaderInfo& psInfo = GetContext()->getModuleMetaData()->psInfo;

    pKernelProgram->m_StagingCtx = GetContext()->m_StagingCtx;
    pKernelProgram->m_RequestStage2 = RequestStage2(GetContext()->m_CgFlag, GetContext()->m_StagingCtx);
    pKernelProgram->blendToFillEnabled = psInfo.blendToFillEnabled;
    pKernelProgram->forceEarlyZ = psInfo.forceEarlyZ;

    pKernelProgram->isCoarsePS = m_phase == PSPHASE_COARSE;
    pKernelProgram->hasCoarsePixelSize = m_HasCoarseSize;
    pKernelProgram->hasSampleOffset = m_SampleOffsetX || m_SampleOffsetY;
    pKernelProgram->hasZWDelta = m_ZWDelta;
    pKernelProgram->needPerspectiveBaryPlane = m_PerspectiveBaryPlanes ? true : false;;
    pKernelProgram->needNonPerspectiveBaryPlane = m_NonPerspectiveBaryPlanes ? true : false;;
    pKernelProgram->ConstantBufferLoaded = m_constantBufferLoaded;
    pKernelProgram->UavLoaded = m_uavLoaded;
    for (int i = 0; i < 4; i++)
    {
        pKernelProgram->ShaderResourceLoaded[i] = m_shaderResourceLoaded[i];
    }
    pKernelProgram->RenderTargetLoaded = m_renderTargetLoaded;

    pKernelProgram->hasControlFlow = m_numBlocks > 1 ? true : false;
    pKernelProgram->MaxNumberOfThreads = m_Platform->getMaxPixelShaderThreads() / GetShaderThreadUsageRate();
    pKernelProgram->needPerspectiveBary = m_PerspectivePixel ? true : false;
    pKernelProgram->needPerspectiveCentroidBary = m_PerspectiveCentroid ? true : false;
    pKernelProgram->needPerspectiveSampleBary = m_PerspectiveSample ? true : false;
    pKernelProgram->needNonPerspectiveBary = m_NoPerspectivePixel ? true : false;
    pKernelProgram->needNonPerspectiveCentroidBary = m_NoPerspectiveCentroid ? true : false;
    pKernelProgram->needNonPerspectiveSampleBary = m_NoPerspectiveSample ? true : false;
    pKernelProgram->killPixel = m_HasDiscard;
    pKernelProgram->needSourceDepth = m_pPositionZPixel != nullptr;
    pKernelProgram->needSourceW = m_pPositionWPixel != nullptr;
    pKernelProgram->outputDepth = m_HasoDepth;
    pKernelProgram->oMask = m_HasoMask;
    pKernelProgram->outputStencil = m_HasoStencil;
    pKernelProgram->sampleCmpToDiscardOptimizationPossible = GetContext()->m_instrTypes.sampleCmpToDiscardOptimizationPossible;
    pKernelProgram->sampleCmpToDiscardOptimizationSlot = GetContext()->m_instrTypes.sampleCmpToDiscardOptimizationSlot;
    pKernelProgram->needPSSync = m_NeedPSSync;
    pKernelProgram->hasInputCoverageMask = m_HasInputCoverageMask;
    pKernelProgram->hasPullBary = m_HasPullBary;
    pKernelProgram->isPerSample = IsPerSample();
    if (NamedMDNode * primIdNod = entry->getParent()->getNamedMetadata("PrimIdLocation"))
    {
        pKernelProgram->primIdLocation = int_cast<uint>(
            mdconst::dyn_extract<ConstantInt>(primIdNod->getOperand(0)->getOperand(0))->getZExtValue());
        pKernelProgram->hasPrimID = true;
    }
    if (NamedMDNode* PointCoordNod = entry->getParent()->getNamedMetadata("PointCoordLocation"))
    {
        pKernelProgram->pointCoordLocation = int_cast<uint>(
            mdconst::dyn_extract<ConstantInt>(PointCoordNod->getOperand(0)->getOperand(0))->getZExtValue());
        pKernelProgram->hasPointCoord = true;
    }

    pKernelProgram->posXYOffsetEnable = m_pPositionXYOffset ? true : false;
    pKernelProgram->VectorMask = m_VectorMask;
    pKernelProgram->samplerCount = GetSamplerCount(m_samplerCount);
    pKernelProgram->renderTargetMask = m_RenderTargetMask;
    pKernelProgram->constantInterpolationEnableMask = m_ConstantInterpolationMask;
    pKernelProgram->NOSBufferSize = m_NOSBufferSize / getMinPushConstantBufferAlignmentInBytes();
    pKernelProgram->isMessageTargetDataCacheDataPort = isMessageTargetDataCacheDataPort;


    CreateGatherMap();
    CreateConstantBufferOutput(pKernelProgram);

    pKernelProgram->bindingTableEntryCount = this->GetMaxUsedBindingTableEntryCount();
    pKernelProgram->BindingTableEntryBitmap = this->GetBindingTableEntryBitmap();

    // PS packed attributes
    for (uint attribute = 0; attribute <= m_MaxSetupIndex / 4; ++attribute)
    {
        pKernelProgram->attributeActiveComponent[attribute] = GetActiveComponents(attribute);

        const bool useComponent = pKernelProgram->attributeActiveComponent[attribute] !=
            USC::GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT_DISABLED;
        if (useComponent)
        {
            pKernelProgram->nbOfSFOutput = attribute + 1;
        }
    }

    for (unsigned i = 0; i < USC::NUM_PSHADER_OUTPUT_REGISTERS; i++)
    {
        pKernelProgram->OutputUseMask[i] = (unsigned char)psInfo.colorOutputMask[i];
    }
}

void CPixelShader::PreCompile()
{
    CreateImplicitArgs();
    CodeGenContext* ctx = GetContext();

    const uint8_t numberInstance = m_numberInstance;
    const bool isR1Available = true;

    if (isR1Available)
    {
        m_R1 = GetNewVariable(getGRFSize() / SIZE_DWORD, ISA_TYPE_D, EALIGN_GRF, false, numberInstance, "R1");
    }

    // make sure the return block is properly set
    if (ctx->getModule()->getNamedMetadata("KillPixel"))
    {
        m_HasDiscard = true;
    }

    setup.resize(4 * g_c_Max_PS_attributes, nullptr);
    if (LowerPSInput())
    {
        setupLowered.resize(4 * g_c_Max_PS_attributes, nullptr);
    }
}

void CPixelShader::ParseShaderSpecificOpcode(llvm::Instruction* inst)
{
    // temporary workaround to disable SIMD16 when double are present
    if (inst->getType()->isDoubleTy())
    {
        m_HasDouble = true;
    }
    if (GenIntrinsicInst * genIntr = dyn_cast<GenIntrinsicInst>(inst))
    {
        uint setupIndex;
        switch (genIntr->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_RenderTargetRead:
            m_NeedPSSync = true;
            if (GetPhase() == PSPHASE_LEGACY)
            {
                m_isPerSample = true;
            }
            break;
        case GenISAIntrinsic::GenISA_uavSerializeAll:
        case GenISAIntrinsic::GenISA_uavSerializeOnResID:
            m_NeedPSSync = true;
            break;
        case GenISAIntrinsic::GenISA_RTDualBlendSource:
            m_hasDualBlendSource = true;
            AddRenderTarget(
                cast<RTDualBlendSourceIntrinsic>(genIntr)->getRTIndexImm());
            break;
        case GenISAIntrinsic::GenISA_RTWrite:
        {
            RTWritIntrinsic* rt = cast<RTWritIntrinsic>(genIntr);
            if (rt->getRTIndexImm() != -1)
            {
                AddRenderTarget(rt->getRTIndexImm());
            }
            if (rt->hasStencil())
            {
                OutputStencil();
            }
            break;
        }
        case GenISAIntrinsic::GenISA_DCL_ShaderInputVec:
        case GenISAIntrinsic::GenISA_DCL_inputVec:
        {
            IGC_ASSERT(llvm::isa<llvm::ConstantInt>(inst->getOperand(0)));
            IGC_ASSERT(llvm::isa<llvm::ConstantInt>(inst->getOperand(1)));
            setupIndex = int_cast<uint>(llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue());
            m_MaxSetupIndex = std::max(setupIndex, m_MaxSetupIndex);
            // attribute packing
            m_SetupIndicesUsed.insert(setupIndex);

            e_interpolation mode = static_cast<e_interpolation>(llvm::cast<llvm::ConstantInt>(inst->getOperand(1))->getZExtValue());
            if (mode != EINTERPOLATION_CONSTANT)
            {
                {
                    if (inst->getType()->isHalfTy())
                    {
                        loweredSetupIndexes.insert(setupIndex);
                        m_ModeUsedHalf.set(mode);
                    }
                    else
                    {
                        m_ModeUsedFloat.set(mode);
                    }
                }
            }
            break;
        }
        case GenISAIntrinsic::GenISA_PullSampleIndexBarys:
        case GenISAIntrinsic::GenISA_PullSnappedBarys:
        case GenISAIntrinsic::GenISA_PullCentroidBarys:
            m_HasPullBary = true;
            break;
        case GenISAIntrinsic::GenISA_Interpolate:
            IGC_ASSERT(llvm::isa<llvm::ConstantInt>(inst->getOperand(0)));
            setupIndex = int_cast<uint>(llvm::cast<llvm::ConstantInt>(inst->getOperand(0))->getZExtValue());
            m_MaxSetupIndex = std::max(setupIndex, m_MaxSetupIndex);
            // attribute packing
            m_SetupIndicesUsed.insert(setupIndex);
            break;
        default:
            break;
        }
    }
}

void CPixelShader::AddRenderTarget(uint RTIndex)
{
    m_RenderTargetMask |= 1 << RTIndex;
}

void CPixelShader::DeclareSGV(uint usage)
{
    switch (usage)
    {
    case POSITION_X:
    case POSITION_Y:
        break;
    case POSITION_Z:
        break;
    case POSITION_W:
        break;
    case VFACE:
        break;
    case INPUT_COVERAGE_MASK:
        m_HasInputCoverageMask = true;
        break;
    case SAMPLEINDEX:
        m_isPerSample = true;
        break;
    case REQUESTED_COARSE_SIZE_X:
    case REQUESTED_COARSE_SIZE_Y:
        m_HasCoarseSize = true;
        break;
    default:
        break;
        // nothing to do
    }
}

void CPixelShader::PullPixelPhasePayload()
{
    CVariable* payload = nullptr;
    bool oMask = false;
    if (GetDispatchSignature().CoarseMask)
    {
        payload = GetCoarseMask();
        oMask = true;
    }
    else
    {
        payload = GetNewVariable(8, ISA_TYPE_D, EALIGN_GRF, CName::NONE);
    }
    uint messageDescriptor = PIPullPixelPayload(
        m_SIMDSize == SIMDMode::SIMD8 ? EU_PI_MESSAGE_SIMD8 : EU_PI_MESSAGE_SIMD16,
        m_PixelPhasePayload->GetSize() / getGRFSize(),
        payload->GetSize() / getGRFSize(),
        false,
        false,
        false,
        false,
        false,
        oMask);

    CVariable* desc = ImmToVariable(messageDescriptor, ISA_TYPE_UD);
    // save the current phase counter as it is needed by the RT write
    m_CurrentPhaseCounter = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, "CurrPhaseCounter");
    encoder.SetSrcRegion(0, 0, 1, 0);
    encoder.Shl(m_CurrentPhaseCounter, m_PixelPhaseCounter, ImmToVariable(0x10, ISA_TYPE_UW));
    encoder.Push();
    CVariable* nextPhase = GetNewVariable(1, ISA_TYPE_UW, EALIGN_DWORD, true, "NextPhase");
    encoder.SetSrcRegion(0, 0, 1, 0);
    encoder.Shl(nextPhase, m_PixelPhaseCounter, ImmToVariable(8, ISA_TYPE_D));
    encoder.Push();
    CVariable* a0 = GetNewVariable(1, ISA_TYPE_UD, EALIGN_DWORD, true, CName::NONE);
    encoder.Or(a0, nextPhase, desc);
    encoder.Push();
    encoder.SetNoMask();
    encoder.Send(m_PixelPhasePayload, payload, EU_GEN7_MESSAGE_TARGET_PIXEL_INTERPOLATOR, a0);
    encoder.Push();
    CVariable* mask = BitCast(m_PixelPhasePayload, ISA_TYPE_UW);
    CVariable* f0 = GetNewVariable(numLanes(m_SIMDSize), ISA_TYPE_BOOL, EALIGN_DWORD, true, CName::NONE);
    encoder.SetSrcSubReg(0, 14);
    encoder.SetSrcRegion(0, 0, 1, 0);
    encoder.Cmp(EPREDICATE_EQ, f0, mask, ImmToVariable(0, ISA_TYPE_UW));
    encoder.Push();
    m_epilogueLabel = encoder.GetNewLabelID("epilogue");
    encoder.Jump(f0, m_epilogueLabel);
    encoder.Push();
    // override the execution mask in sr0.2
    encoder.SetSrcSubReg(0, 14);
    encoder.SetDstSubReg(2);
    encoder.SetSrcRegion(0, 0, 1, 0);
    encoder.Cast(GetSR0(), mask);
    encoder.Push();
}

void CPixelShader::AddPrologue()
{
    if (m_phase == PSPHASE_PIXEL)
    {
        uint responseLength = 2;
        m_CoarseR1 = GetR1();
        m_PixelPhasePayload =
            GetNewVariable(responseLength * (getGRFSize() >> 2),
                ISA_TYPE_D, EALIGN_GRF, "PixelPhasePayload");
        m_PixelPhaseCounter = GetNewAlias(m_PixelPhasePayload, ISA_TYPE_UW, 0, 1);
        m_CoarseParentIndex = GetNewAlias(m_PixelPhasePayload, ISA_TYPE_UW, getGRFSize(), numLanes(m_SIMDSize));
        m_R1 = GetNewAlias(m_PixelPhasePayload, ISA_TYPE_D, 0, getGRFSize() / SIZE_DWORD);
        encoder.SetNoMask();
        encoder.SetSimdSize(SIMDMode::SIMD1);
        encoder.Copy(m_PixelPhaseCounter, ImmToVariable(0, ISA_TYPE_UW));
        encoder.Push();
        m_pixelPhaseLabel = encoder.GetNewLabelID("pixel_phase");
        encoder.Label(m_pixelPhaseLabel);
        encoder.Push();
        PullPixelPhasePayload();
    }
    {
        emitPSInputLowering();
    }
}

void CPixelShader::PreAnalysisPass()
{
    m_VectorMask = m_CG->NeedVMask();
    CShader::PreAnalysisPass();
}

void CPixelShader::AddEpilogue(llvm::ReturnInst* ret)
{
    if (!IsLastPhase() && m_KillPixelMask)
    {
        if (!m_CoarseoMask)
        {
            m_CoarseoMask = GetNewVariable(
                numLanes(m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF, "CoarseOMask");
            encoder.Copy(m_CoarseoMask, ImmToVariable(0xFFFFFFFF, ISA_TYPE_UD));
            encoder.Push();
        }
        encoder.SetPredicate(m_KillPixelMask);
        encoder.Copy(m_CoarseoMask, ImmToVariable(0x0, ISA_TYPE_UD));
        encoder.Push();
    }
    if (m_phase == PSPHASE_PIXEL)
    {
        encoder.Label(m_epilogueLabel);
        encoder.Push();
        // next phase index is in the first dword of the payload
        CVariable* flag = GetNewVariable(
            numLanes(m_SIMDSize), ISA_TYPE_BOOL, EALIGN_BYTE, true, CName::NONE);
        encoder.SetSrcRegion(0, 0, 1, 0);
        encoder.Cmp(EPREDICATE_NE, flag, m_PixelPhaseCounter, ImmToVariable(0, ISA_TYPE_UW));
        encoder.Push();
        encoder.Jump(flag, m_pixelPhaseLabel);
        encoder.Push();
        const bool isPerCoarse = true;
        EOTRenderTarget(GetR1(), isPerCoarse);
        m_hasEOT = true;
    }
    if (IsLastPhase())
    {
        CShader::AddEpilogue(ret);
    }
}

void CPixelShader::AddCoarseOutput(CVariable* output, unsigned int index)
{
    IGC_ASSERT(m_CoarseOutput.find(index) == m_CoarseOutput.end());
    m_CoarseOutput[index] = output;
}

CVariable* CPixelShader::GetCoarseInput(unsigned int index, uint16_t vectorSize, VISA_Type type)
{
    auto it = m_CoarseInput.find(index);
    CVariable* coarseInput = nullptr;
    if (it == m_CoarseInput.end())
    {
        coarseInput = GetNewVariable(
            numLanes(m_SIMDSize) * vectorSize, type, EALIGN_GRF, "CoarseInput");
        m_CoarseInput[index] = coarseInput;
    }
    else
    {
        coarseInput = it->second;
    }
    return coarseInput;
}

void CPixelShader::SetCoarseoMask(CVariable* oMask)
{
    m_CoarseoMask = oMask;
}

CVariable* CPixelShader::GetCoarseMask()
{
    if (m_CoarseMaskInput == nullptr)
    {
        m_CoarseMaskInput = GetNewVariable(
            numLanes(m_SIMDSize), ISA_TYPE_F, EALIGN_GRF, "CoarseMaskInput");
    }
    return m_CoarseMaskInput;
}

CVariable* CPixelShader::GetCoarseParentIndex()
{
    return m_CoarseParentIndex;
}

CVariable* CPixelShader::GetCurrentPhaseCounter()
{
    return m_CurrentPhaseCounter;
}


bool CPixelShader::CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F)
{
    if (!CompileSIMDSizeInCommon(simdMode))
        return false;


    CodeGenContext* ctx = GetContext();
    if (!ctx->m_retryManager.IsFirstTry())
    {
        ctx->ClearSIMDInfo(simdMode, EP.m_ShaderDispatchMode);
        ctx->SetSIMDInfo(SIMD_RETRY, simdMode, EP.m_ShaderDispatchMode);
    }

    bool forceSIMD32 =
        (ctx->getCompilerOption().forcePixelShaderSIMDMode &
            FLAG_PS_SIMD_MODE_FORCE_SIMD32) != 0;
    bool forceSIMD16 =
        (ctx->getCompilerOption().forcePixelShaderSIMDMode &
            FLAG_PS_SIMD_MODE_FORCE_SIMD16) != 0;

    // For staged compilation, we try to avoid duplicated compilation for the same SIMD mode
    if ((simdMode == SIMDMode::SIMD8  && AvoidDupStage2(8 , ctx->m_CgFlag, ctx->m_StagingCtx)) ||
        (simdMode == SIMDMode::SIMD16 && AvoidDupStage2(16, ctx->m_CgFlag, ctx->m_StagingCtx)))
    {
        return false;
    }

    if (ctx->PsHighSimdDisable)
    {
        if (simdMode == SIMDMode::SIMD32)
            return false;
        if (EP.m_ShaderDispatchMode == ShaderDispatchMode::DUAL_SIMD8)
            return false;
    }

    if (m_HasoStencil && !ctx->platform.supportsStencil(simdMode))
    {
        ctx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, EP.m_ShaderDispatchMode);
        return false;
    }
    if (m_HasDouble && simdMode != SIMDMode::SIMD8)
    {
        ctx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, EP.m_ShaderDispatchMode);
        return false;
    }
    if (m_hasDualBlendSource && simdMode != SIMDMode::SIMD8 &&
        (m_phase == PSPHASE_PIXEL || ((m_phase != PSPHASE_LEGACY) && (ctx->platform.getWATable().Wa_1409392000 || ctx->platform.getPlatformInfo().eProductFamily == IGFX_ICELAKE))))
    {
        // Spec restriction CPS multi-phase cannot use SIMD16 with dual source blending
        ctx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, EP.m_ShaderDispatchMode);
        return false;
    }
    if (m_phase != PSPHASE_LEGACY &&
        simdMode == SIMDMode::SIMD32)
    {
        ctx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, EP.m_ShaderDispatchMode);
        return false;
    }

    if (GetContext()->platform.hasFusedEU() &&
        simdMode == SIMDMode::SIMD32 &&
        IsPerSample() && !IsStage1(ctx))
    {
        //Fused SIMD32 not enabled when dispatch rate is per sample
        ctx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, EP.m_ShaderDispatchMode);
        return false;
    }

    if (simdMode == SIMDMode::SIMD16 && EP.m_ShaderDispatchMode == ShaderDispatchMode::NOT_APPLICABLE)
    {
        if (IsStage1BestPerf(ctx->m_CgFlag, ctx->m_StagingCtx))
        {
            return true;
        }
        if (DoSimd16Stage2(ctx->m_StagingCtx))
        {
            return true;
        }

        if (IGC_IS_FLAG_ENABLED(ForceBestSIMD))
        {
            return true;
        }

        if (forceSIMD16)
        {
            return true;
        }
        CShader* simd8Program = m_parent->GetShader(SIMDMode::SIMD8);
        if (simd8Program != nullptr && simd8Program->ProgramOutput()->m_scratchSpaceUsedBySpills > 0)
        {
            ctx->SetSIMDInfo(SIMD_SKIP_REGPRES, simdMode, EP.m_ShaderDispatchMode);
            return false;
        }
    }
    if (simdMode == SIMDMode::SIMD32)
    {
        if (DoSimd32Stage2(ctx->m_StagingCtx))
        {
            return true;
        }

        if (forceSIMD32)
        {
            return true;
        }

        CShader* simd16Program = m_parent->GetShader(SIMDMode::SIMD16);
        if ((simd16Program == nullptr ||
            simd16Program->ProgramOutput()->m_programBin == 0 ||
            simd16Program->ProgramOutput()->m_scratchSpaceUsedBySpills > 0))
        {
            ctx->SetSIMDInfo(SIMD_SKIP_REGPRES, simdMode, EP.m_ShaderDispatchMode);
            return false;
        }

        const PixelShaderInfo& psInfo = ctx->getModuleMetaData()->psInfo;

        // Disable simd32 compilation on platforms that do not support per-pixel
        // dispatch with num samples == 16.
        if (psInfo.NumSamples == 16 &&
            !ctx->platform.supportSimd32PerPixelPSWithNumSamples16() &&
            !IsPerSample())
        {
            return false;
        }

        if (psInfo.ForceEnableSimd32) // UMD forced compilation of simd32.
        {
            return true;
        }

        if (!ctx->platform.enablePSsimd32())
        {
            ctx->SetSIMDInfo(SIMD_SKIP_HW, simdMode, EP.m_ShaderDispatchMode);
            return false;
        }

        if (iSTD::BitCount(m_RenderTargetMask) > 1)
        {
            // don't compile SIMD32 for MRT as we may trash the render cache
            ctx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, EP.m_ShaderDispatchMode);
            return false;
        }

        Simd32ProfitabilityAnalysis& PA = EP.getAnalysis<Simd32ProfitabilityAnalysis>();
        if (PA.isSimd32Profitable())
        {
            return true;
        }
        else
        {
            ctx->SetSIMDInfo(SIMD_SKIP_PERF, simdMode, EP.m_ShaderDispatchMode);
        }

        if (simd16Program && static_cast<CPixelShader*>(simd16Program)->m_sendStallCycle == 0)
        {
            // simd16 doesn't have any latency issue, no need to try simd32
            ctx->SetSIMDInfo(SIMD_SKIP_STALL, simdMode, EP.m_ShaderDispatchMode);
            return false;
        }

        if (ctx->platform.psSimd32SkipStallHeuristic() && ctx->m_DriverInfo.AlwaysEnableSimd32())
        {
            return true;
        }

        if (simd16Program)
        {
            uint sendStallCycle = static_cast<CPixelShader*>(simd16Program)->m_sendStallCycle;
            uint staticCycle = static_cast<CPixelShader*>(simd16Program)->m_staticCycle;
            if (sendStallCycle / (float)staticCycle > 0.4)
            {
                return true;
            }
            else
            {
                ctx->SetSIMDInfo(SIMD_SKIP_STALL, simdMode, EP.m_ShaderDispatchMode);
            }
        }
        return false;
    }
    return true;
}

void linkProgram(const SProgramOutput& cps, const SProgramOutput& ps, SProgramOutput& linked)
{
    linked.m_unpaddedProgramSize =
        cps.m_unpaddedProgramSize + ps.m_unpaddedProgramSize;
    linked.m_scratchSpaceUsedByShader =
        cps.m_scratchSpaceUsedByShader + ps.m_scratchSpaceUsedByShader;
    linked.m_scratchSpaceUsedBySpills =
        cps.m_scratchSpaceUsedBySpills + ps.m_scratchSpaceUsedBySpills;
    linked.m_scratchSpaceUsedByGtpin =
        cps.m_scratchSpaceUsedByGtpin + ps.m_scratchSpaceUsedByGtpin;
    linked.m_programSize = iSTD::Align(linked.m_unpaddedProgramSize, 64);
    linked.m_programBin = IGC::aligned_malloc(linked.m_programSize, 16);
    // Copy coarse phase
    memcpy_s(linked.m_programBin,
        cps.m_unpaddedProgramSize,
        cps.m_programBin,
        cps.m_unpaddedProgramSize);
    // Copy pixel phase
    memcpy_s((char*)linked.m_programBin + cps.m_unpaddedProgramSize,
        ps.m_unpaddedProgramSize,
        ps.m_programBin,
        ps.m_unpaddedProgramSize);
    memset((char*)linked.m_programBin + linked.m_unpaddedProgramSize,
        0,
        linked.m_programSize - linked.m_unpaddedProgramSize);
}

void linkCPS(SPixelShaderKernelProgram* output, SPixelShaderKernelProgram& linked, unsigned int numberPhases)
{
    SPixelShaderKernelProgram CoarsePhaseOutput = output[0];
    SPixelShaderKernelProgram PixelPhaseOutput = output[1];
    linked = output[0];

    if (CoarsePhaseOutput.simd16.m_scratchSpaceUsedBySpills == 0 &&
        CoarsePhaseOutput.simd16.m_programBin != nullptr &&
        PixelPhaseOutput.simd16.m_scratchSpaceUsedBySpills == 0 &&
        PixelPhaseOutput.simd16.m_programBin != nullptr)
    {
        linkProgram(CoarsePhaseOutput.simd16, PixelPhaseOutput.simd16, linked.simd16);
    }
    else
    {
        linked.simd16.m_programBin = nullptr;
        linked.simd16.m_programSize = 0;
    }
    linkProgram(CoarsePhaseOutput.simd8, PixelPhaseOutput.simd8, linked.simd8);
    linked.hasPullBary = true;
    linked.renderTargetMask = (CoarsePhaseOutput.renderTargetMask || PixelPhaseOutput.renderTargetMask);
    IGC_ASSERT_MESSAGE(numberPhases == 2, "maximum number of phases is 2");
}

void CodeGen(PixelShaderContext* ctx)
{
    Function* coarsePhase = nullptr;
    Function* pixelPhase = nullptr;
    NamedMDNode* coarseNode = nullptr;
    NamedMDNode* pixelNode = nullptr;
    MetaDataUtils* pMdUtils = nullptr;
    if (!HasSavedIR(ctx))
    {
        coarseNode = ctx->getModule()->getNamedMetadata(NAMED_METADATA_COARSE_PHASE);
        pixelNode = ctx->getModule()->getNamedMetadata(NAMED_METADATA_PIXEL_PHASE);
        if (coarseNode)
        {
            coarsePhase = mdconst::dyn_extract<Function>(coarseNode->getOperand(0)->getOperand(0));
        }
        if (pixelNode)
        {
            pixelPhase = mdconst::dyn_extract<Function>(pixelNode->getOperand(0)->getOperand(0));
        }
        pMdUtils = ctx->getMetaDataUtils();
    }

    bool codegenDone = false;

    CShaderProgram::KernelShaderMap coarseShaders;
    CShaderProgram::KernelShaderMap pixelShaders;

    if (coarsePhase && pixelPhase)
    {
        // Cancelling staged compilation for multi stage PS.
        ctx->m_CgFlag = FLAG_CG_ALL_SIMDS;

        //Multi stage PS, need to do separate compiler and link them
        unsigned int numStage = 2;
        PSSignature signature;
        FunctionInfoMetaDataHandle coarseFI, pixelFI;
        coarseFI = pMdUtils->getFunctionsInfoItem(coarsePhase);
        pixelFI = pMdUtils->getFunctionsInfoItem(pixelPhase);

        for (unsigned int i = 0; i < numStage; i++)
        {
            Function* phaseFunc = (i == 0) ? coarsePhase : pixelPhase;
            FunctionInfoMetaDataHandle phaseFI = (i == 0) ? coarseFI : pixelFI;
            CShaderProgram::KernelShaderMap& shaders = (i == 0) ? coarseShaders : pixelShaders;

            pMdUtils->clearFunctionsInfo();
            pMdUtils->setFunctionsInfoItem(phaseFunc, phaseFI);
            pMdUtils->save(phaseFunc->getContext());
            CodeGen(ctx, shaders, &signature);

            // Read the phase function from metadata again as it could be changed in the PushAnalysis pass
            if (i == 0)
            {
                coarseNode = ctx->getModule()->getNamedMetadata(NAMED_METADATA_COARSE_PHASE);
            }
            else
            {
                pixelNode = ctx->getModule()->getNamedMetadata(NAMED_METADATA_PIXEL_PHASE);
            }
        }

        codegenDone = true;


        for (unsigned int i = 0; i < numStage; i++)
        {
            Function* phaseFunc = (i == 0) ?
                mdconst::dyn_extract<Function>(coarseNode->getOperand(0)->getOperand(0)) :
                mdconst::dyn_extract<Function>(pixelNode->getOperand(0)->getOperand(0));

            CShaderProgram::KernelShaderMap& shaders = (i == 0) ? coarseShaders : pixelShaders;
            CPixelShader* simd8Shader = static_cast<CPixelShader*>(shaders[phaseFunc]->GetShader(SIMDMode::SIMD8));
            CPixelShader* simd16Shader = static_cast<CPixelShader*>(shaders[phaseFunc]->GetShader(SIMDMode::SIMD16));
            CPixelShader* simd32Shader = static_cast<CPixelShader*>(shaders[phaseFunc]->GetShader(SIMDMode::SIMD32));
            if (!((simd8Shader && simd8Shader->ProgramOutput()->m_programBin) ||
                (simd16Shader && simd16Shader->ProgramOutput()->m_programBin) ||
                (simd32Shader && simd32Shader->ProgramOutput()->m_programBin)
                ))
            {
                shaders[phaseFunc]->DeleteShader(SIMDMode::SIMD8);
                shaders[phaseFunc]->DeleteShader(SIMDMode::SIMD16);
                shaders[phaseFunc]->DeleteShader(SIMDMode::SIMD32);
                if (i == 0)
                {
                    delete shaders[coarsePhase];
                    coarsePhase = nullptr;
                }
                else
                {
                    delete shaders[pixelPhase];
                    pixelPhase = nullptr;
                }
            }
        }

        if (coarsePhase && pixelPhase)
        {
            SPixelShaderKernelProgram outputs[2] = { SPixelShaderKernelProgram(), SPixelShaderKernelProgram() };
            //memset(&outputs, 0, 2 * sizeof(SPixelShaderKernelProgram));

            for (unsigned int i = 0; i < numStage; i++)
            {
                Function* phaseFunc = (i == 0) ?
                    mdconst::dyn_extract<Function>(coarseNode->getOperand(0)->getOperand(0)) :
                    mdconst::dyn_extract<Function>(pixelNode->getOperand(0)->getOperand(0));

                CShaderProgram::KernelShaderMap& shaders = (i == 0) ? coarseShaders : pixelShaders;

                shaders[phaseFunc]->FillProgram(&outputs[i]);
                COMPILER_SHADER_STATS_PRINT(shaders[phaseFunc]->m_shaderStats, ShaderType::PIXEL_SHADER, ctx->hash, "");
                COMPILER_SHADER_STATS_SUM(ctx->m_sumShaderStats, shaders[phaseFunc]->m_shaderStats, ShaderType::PIXEL_SHADER);
                COMPILER_SHADER_STATS_DEL(shaders[phaseFunc]->m_shaderStats);
                delete shaders[phaseFunc];
            }

            linkCPS(outputs, ctx->programOutput, numStage);
            // Kernels allocated in CISABuilder.cpp (Compile())
            // are freed in CompilerOutputOGL.hpp (DeleteShaderCompilerOutputOGL())
            // in case of CPS multistage PS they are separated.
            // Need to free original kernels here as DeleteShaderCompilerOutputOGL()
            // will clear new allocations for separated phases in this case.
            for (unsigned int i = 0; i < numStage; i++)
            {
                outputs[i].simd8.Destroy();
                outputs[i].simd16.Destroy();
                outputs[i].simd32.Destroy();
            }
        }
    }

    if (!(coarsePhase && pixelPhase))
    {
        CShaderProgram::KernelShaderMap shaders;
        Function* pFunc = nullptr;

        if (!codegenDone)
        {
            // Single PS
            CodeGen(ctx, shaders);
            pFunc = getUniqueEntryFunc(ctx->getMetaDataUtils(), ctx->getModuleMetaData());
        }
        else
        {
            shaders = coarsePhase ? coarseShaders : pixelShaders;
            pFunc = coarsePhase ? coarsePhase : pixelPhase;
        }

        // gather data to send back to the driver
        shaders[pFunc]->FillProgram(&ctx->programOutput);
        COMPILER_SHADER_STATS_PRINT(shaders[pFunc]->m_shaderStats, ShaderType::PIXEL_SHADER, ctx->hash, "");
        COMPILER_SHADER_STATS_SUM(ctx->m_sumShaderStats, shaders[pFunc]->m_shaderStats, ShaderType::PIXEL_SHADER);
        COMPILER_SHADER_STATS_DEL(shaders[pFunc]->m_shaderStats);
        delete shaders[pFunc];
    }
}

void CPixelShader::CreatePassThroughVar()
{
    CodeGenContext* ctx = GetContext();
    NamedMDNode* pixelNode = ctx->getModule()->getNamedMetadata("pixel_phase");
    if (!pixelNode)
    {
        // if there is no pixel phase we have nothing to do
        return;
    }
    IGC_ASSERT(nullptr != GetR1());
    encoder.MarkAsOutput(GetR1());
    Function* pixelPhase = mdconst::dyn_extract<Function>(pixelNode->getOperand(0)->getOperand(0));
    for (auto BB = pixelPhase->begin(), BE = pixelPhase->end(); BB != BE; ++BB)
    {
        llvm::BasicBlock* pLLVMBB = &(*BB);
        llvm::BasicBlock::InstListType& instructionList = pLLVMBB->getInstList();
        for (auto I = instructionList.begin(), E = instructionList.end(); I != E; ++I)
        {
            if (GenIntrinsicInst * intr = dyn_cast<GenIntrinsicInst>(I))
            {
                GenISAIntrinsic::ID IID = intr->getIntrinsicID();
                if (IID == GenISAIntrinsic::GenISA_DCL_inputVec)
                {
                    unsigned int setupIndex =
                        (uint)llvm::cast<llvm::ConstantInt>(intr->getOperand(0))->getZExtValue();
                    CVariable* input = GetInputDelta(setupIndex);
                    encoder.MarkAsOutput(input);
                }
                else if (IID == GenISAIntrinsic::GenISA_SampleOffsetX)
                {
                    CVariable* offset = GetSampleOffsetX();
                    encoder.MarkAsOutput(offset);
                }
                else if (IID == GenISAIntrinsic::GenISA_SampleOffsetY)
                {
                    CVariable* offset = GetSampleOffsetY();
                    encoder.MarkAsOutput(offset);
                }
                else if (IID == GenISAIntrinsic::GenISA_DCL_SystemValue)
                {
                    SGVUsage usage = (SGVUsage)llvm::cast<llvm::ConstantInt>(intr->getOperand(0))->getZExtValue();
                    if (usage == POSITION_Z || usage == POSITION_W)
                    {
                        CVariable* deltas = GetZWDelta();
                        encoder.MarkAsOutput(deltas);
                    }
                }
            }
        }
    }
    GetDispatchSignature().inputOffset.resize(setup.size());
}

void CPixelShader::ExtractGlobalVariables()
{
    llvm::Module* module = GetContext()->getModule();
    llvm::GlobalVariable* pGlobal = module->getGlobalVariable("SamplerCount");
    if (pGlobal)
    {
        auto samplerCount = int_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
        m_samplerCount = samplerCount;
    }
}

bool CPixelShader::IsReturnBlock(llvm::BasicBlock* bb)
{
    return llvm::isa<llvm::ReturnInst>(bb->getTerminator());
}

bool CPixelShader::IsLastRTWrite(llvm::GenIntrinsicInst* inst)
{
    bool isLastRT;
    isLastRT = llvm::isa<llvm::ReturnInst>(inst->getNextNode());

    return isLastRT && IsLastPhase() && GetPhase() != PSPHASE_PIXEL;
}

bool CPixelShader::LowerPSInput()
{
    return (m_SIMDSize == SIMDMode::SIMD16 || !m_Platform->supportMixMode());
}

bool CPixelShader::IsInterpolationLinear(e_interpolation mode)
{
    return mode == EINTERPOLATION_LINEAR ||
        mode == EINTERPOLATION_LINEARCENTROID ||
        mode == EINTERPOLATION_LINEARSAMPLE ||
        mode == EINTERPOLATION_LINEARNOPERSPECTIVE ||
        mode == EINTERPOLATION_LINEARNOPERSPECTIVECENTROID ||
        mode == EINTERPOLATION_LINEARNOPERSPECTIVESAMPLE;
}

void CPixelShader::emitPSInputLowering()
{
    auto iterSetupIndex = loweredSetupIndexes.begin();
    auto iterSetupIndexEnd = loweredSetupIndexes.end();


    if (LowerPSInput())
    {
        for (; iterSetupIndex != iterSetupIndexEnd; ++iterSetupIndex)
        {
            bool combineTwoDelta = false;
            auto nextElemt = iterSetupIndex;
            nextElemt++;
            if (nextElemt != iterSetupIndexEnd && *iterSetupIndex % 2 == 0 && *iterSetupIndex + 1 == *nextElemt)
            {
                combineTwoDelta = true;
            }
            unsigned int index = *iterSetupIndex;
            CVariable* inputVar = GetInputDelta(index, combineTwoDelta);
            CVariable* inputVarLowered = GetInputDeltaLowered(index);
            if (encoder.IsCodePatchCandidate())
            {
                encoder.SetPayloadSectionAsPrimary();
                AddPatchTempSetup(inputVarLowered);
            }

            encoder.SetSrcRegion(0, 1, 1, 0);
            encoder.SetUniformSIMDSize(combineTwoDelta ? SIMDMode::SIMD8 : SIMDMode::SIMD4);
            encoder.SetNoMask();
            encoder.Cast(inputVarLowered, inputVar);
            encoder.Push();
            if (encoder.IsCodePatchCandidate())
            {
                encoder.SetPayloadSectionAsSecondary();
            }
            if (combineTwoDelta)
            {
                ++iterSetupIndex;
            }
        }

        for (uint i = EINTERPOLATION_LINEAR; i < NUMBER_EINTERPOLATION; ++i)
        {
            if (m_ModeUsedHalf.test(i))
            {
                CVariable* baryVar = GetBaryReg((e_interpolation)i);
                CVariable* baryVarLowered = GetBaryRegLoweredHalf((e_interpolation)i);

                if (encoder.IsCodePatchCandidate())
                {
                    encoder.SetPayloadSectionAsPrimary();
                }
                for (uint8_t i = 0; i < m_numberInstance; ++i)
                {
                    encoder.SetSecondHalf(i == 1);

                    // mov SIMD8 U1/barry(0, 0) in to tmpU(0, 0)
                    // mov (8) r1.0<1>:hf r2.0<8;8,1>:f {Align1, Q1, NoMask} // #??:$27:%30
                    encoder.SetSimdSize(SIMDMode::SIMD8);
                    encoder.SetNoMask();
                    encoder.Cast(baryVarLowered, baryVar);
                    encoder.Push();

                    if (m_SIMDSize == SIMDMode::SIMD16)
                    {
                        // mov SIMD8 U2/barry(2, 0) in to tmpU(0, 8)
                        // mov (8) r1.8<1>:hf r4.0<8;8,1>:f {Align1, Q1, NoMask} // #??:$28:%31
                        encoder.SetSrcSubVar(0, 2);
                        encoder.SetDstSubReg(8);
                        encoder.SetSimdSize(SIMDMode::SIMD8);
                        encoder.SetNoMask();
                        encoder.Cast(baryVarLowered, baryVar);
                        encoder.Push();
                    }

                    // mov SIMD8 V1/barry(1, 0) in to tmpV(0, 0)
                    // mov (8) r12.0<1>:hf r3.0<8;8,1>:f {Align1, Q1, NoMask} // #??:$29:%32
                    encoder.SetSrcSubVar(0, 1);
                    encoder.SetSimdSize(SIMDMode::SIMD8);
                    encoder.SetNoMask();
                    encoder.SetDstSubReg(numLanes(m_SIMDSize));
                    encoder.Cast(baryVarLowered, baryVar);
                    encoder.Push();

                    if (m_SIMDSize == SIMDMode::SIMD16)
                    {
                        // mov SIMD8 V1/barry(3, 0) in to tmpV(0, 0)
                        // mov (8) r12.8<1>:hf r5.0<8;8,1>:f {Align1, Q1, NoMask} // #??:$30:%33
                        encoder.SetSrcSubVar(0, 3);
                        encoder.SetDstSubReg(8);
                        encoder.SetSimdSize(SIMDMode::SIMD8);
                        encoder.SetNoMask();
                        encoder.SetDstSubVar(1);
                        encoder.Cast(baryVarLowered, baryVar);
                        encoder.Push();
                    }
                    encoder.SetSecondHalf(false);
                }
                if (encoder.IsCodePatchCandidate())
                {
                    encoder.SetPayloadSectionAsSecondary();
                }
            }
        }
    }
}

void CPixelShader::MarkConstantInterpolation(unsigned int index)
{
    m_ConstantInterpolationMask |= BIT(index / 4);
}

// Take PS attribute and return active components within, encoded as HW expects.
USC::GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT CPixelShader::GetActiveComponents(uint attribute) const
{
    USC::GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT result =
        USC::GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT_DISABLED;
    for (auto it = m_SetupIndicesUsed.lower_bound(attribute * 4);
        it != m_SetupIndicesUsed.end(); ++it)
    {
        if (attribute != (*it / 4)) break;
        switch (*it % 4)
        {
        case 0:
        case 1:
            result = USC::GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT_XY;
            break;
        case 2:
            result = USC::GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT_XYZ;
            break;
        case 3:
            result = USC::GFX3DSTATE_SF_ATTRIBUTE_ACTIVE_COMPONENT_XYZW;
            break;
        }
    }
    return result;
}

// this method must be run only after CShader::PreAnalysisPass() was run
void CPixelShader::MapPushedInputs()
{
    // first gather setup index info
    for (auto I = pushInfo.inputs.begin(), E = pushInfo.inputs.end(); I != E; I++)
    {
        m_SetupIndicesUsed.insert(I->second.index);
        m_MaxSetupIndex = std::max(I->second.index, m_MaxSetupIndex);
    }
    // then map using proper indexing
    for (auto I = pushInfo.inputs.begin(), E = pushInfo.inputs.end(); I != E; I++)
    {
        // We need to map the value associated with the value pushed to a physical register
        if (I->second.interpolationMode == EINTERPOLATION_CONSTANT)
        {
            this->MarkConstantInterpolation(I->second.index);
        }
        CVariable* var = GetSymbol(m_argListCache[I->second.argIndex]);
        AddSetup(getSetupIndex(I->second.index), var);
    }
}

int CPixelShader::getSetupIndex(uint inputIndex)
{
    {
        return inputIndex;
    }
}

} // namespace IGC
