/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/DomainShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/igc_regkeys.hpp"
#include "common/secure_mem.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include <iStdLib/utility.h>
#include "Probe/Assertion.h"

using namespace llvm;

/***********************************************************************************
This file contains the code specific to domain shader
************************************************************************************/
namespace IGC
{
    //size in dwords of m_pTessFactorURBHeader
    const uint32_t CDomainShader::m_pTessFactorsURBHeader = 8;

    CDomainShader::CDomainShader(llvm::Function* pFunc, CShaderProgram* pProgram) :
        CShader(pFunc, pProgram)
        , m_pURBWriteHandleReg(nullptr)
        , m_pMaxInputSignatureCount(0)
        , m_pMaxPatchConstantSignatureDeclarations(0)
        , m_pInputControlPointCount(0)
        , m_pURBReadHandleReg(nullptr)
        , m_hasPrimitiveIdInput(false)
    {
    }

    CDomainShader::~CDomainShader()
    {

    }

    void CDomainShader::PreCompile()
    {
        CreateImplicitArgs();

        // allocate register for urb write handles
        m_pURBWriteHandleReg = GetNewVariable(numLanes(m_SIMDSize), ISA_TYPE_D, EALIGN_GRF, "URBWriteHandle");
        m_pURBReadHandleReg = GetNewVariable(numLanes(m_SIMDSize), ISA_TYPE_D, EALIGN_GRF, "URBReadHandle");

        if (m_ShaderDispatchMode == ShaderDispatchMode::DUAL_PATCH)
        {
            if (!m_Platform->DSPrimitiveIDPayloadPhaseCanBeSkipped() || m_hasPrimitiveIdInput)
            {
                m_pPatchPrimitiveId = GetNewVariable(numLanes(m_SIMDSize), ISA_TYPE_D, EALIGN_GRF, "PatchPrimId");
            }
        }
    }

    CVariable* CDomainShader::GetPrimitiveID()
    {
        return m_pPatchPrimitiveId;
    }

    ShaderDispatchMode CDomainShader::GetShaderDispatchMode() const
    {
        return m_ShaderDispatchMode;
    }

    CVariable* CDomainShader::GetInputDelta(uint index)
    {
        CVariable* inputVar = nullptr;
        if (GetShaderDispatchMode() == ShaderDispatchMode::DUAL_PATCH)
        {
            if (setup.size() <= index / 4 || setup[index / 4] == nullptr)
            {
                inputVar = GetNewVariable(
                    numLanes(m_SIMDSize), ISA_TYPE_F, EALIGN_GRF, false,
                    CName("inputVar", index));
                AddSetup(index / 4, inputVar);
            }
            else
            {
                inputVar = setup[index / 4];
            }
        }
        else
        {
            if (setup.size() <= index || setup[index] == nullptr)
            {
                inputVar = GetNewVariable(1, ISA_TYPE_F, EALIGN_DWORD, true,
                    CName("inputVar", index));
                AddSetup(index, inputVar);
            }
            else
            {
                inputVar = setup[index];
            }
        }
        return inputVar;
    }

    void CDomainShader::ExtractGlobalVariables()
    {
        llvm::Module* module = GetContext()->getModule();

        llvm::GlobalVariable* pGlobal = module->getGlobalVariable("TessInputControlPointCount");
        m_pInputControlPointCount = int_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = module->getGlobalVariable("MaxNumOfInputSignatureEntries");
        m_pMaxInputSignatureCount = int_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());

        pGlobal = module->getGlobalVariable("MaxNumOfPatchConstantSignatureEntries");
        m_pMaxPatchConstantSignatureDeclarations = int_cast<unsigned int>(llvm::cast<llvm::ConstantInt>(pGlobal->getInitializer())->getZExtValue());
    }

    OctEltUnit CDomainShader::GetURBAllocationSize() const
    {
        return std::max(round_up<OctElement>(m_properties.m_URBOutputLength), OctEltUnit(1));
    }

    OctEltUnit CDomainShader::GetVertexURBEntryReadLength() const
    {
        return
            GetShaderDispatchMode() == ShaderDispatchMode::DUAL_PATCH ?
            OctEltUnit(setup.size()) : round_up<OctElement>(EltUnit(setup.size()));
    }


    OctEltUnit CDomainShader::GetURBHeaderSize() const
    {
        if (GetContext()->getModuleMetaData()->URBInfo.hasVertexHeader)
        {
            return (m_properties.m_hasClipDistance ? OctEltUnit(2) : OctEltUnit(1));
        }
        else
        {
            return OctEltUnit(0);
        }
    }

    CVariable* CDomainShader::GetURBInputHandle(CVariable* pVertexIndex)
    {
        if (m_ShaderDispatchMode == ShaderDispatchMode::SINGLE_PATCH)
        {
            // First fetch the URBHandle from R0.0
            encoder.SetSrcRegion(0, 0, 1, 0);
            encoder.SetSrcSubReg(0, 0);
            encoder.Copy(m_pURBReadHandleReg, GetR0());
            encoder.Push();
        }
        else if (m_ShaderDispatchMode == ShaderDispatchMode::DUAL_PATCH)
        {
            encoder.SetSimdSize(m_Platform->getMinDispatchMode());
            encoder.SetSrcRegion(0, 1, 4, 0);
            {
                encoder.SetSrcSubReg(0, 0);
            }

            encoder.Copy(m_pURBReadHandleReg, GetR0());
            encoder.Push();
        }
        return m_pURBReadHandleReg;
    }

    uint32_t CDomainShader::GetMaxInputSignatureCount()
    {
        return m_pMaxInputSignatureCount;
    }

    void CDomainShader::AllocatePayload()
    {
        uint offset = 0;

        //R0 is always allocated as a predefined variable. Increase offset for R0
        IGC_ASSERT(m_R0);
        offset += getGRFSize();

        if (m_ShaderDispatchMode == ShaderDispatchMode::DUAL_PATCH)
        {
            if (!m_Platform->DSPrimitiveIDPayloadPhaseCanBeSkipped() || m_hasPrimitiveIdInput)
            {
                AllocateInput(m_pPatchPrimitiveId, offset);
                offset += getGRFSize();
            }
        }

        AllocateInput(GetSymbol(m_properties.m_UArg), offset);
        offset += getGRFSize();

        AllocateInput(GetSymbol(m_properties.m_VArg), offset);
        offset += getGRFSize();

        AllocateInput(GetSymbol(m_properties.m_WArg), offset);
        offset += getGRFSize();

        // allocate input for URB return handles
        IGC_ASSERT(m_pURBWriteHandleReg);
        AllocateInput(m_pURBWriteHandleReg, offset);
        offset += getGRFSize();

        IGC_ASSERT(0 < getGRFSize());
        IGC_ASSERT((offset % getGRFSize()) == 0);
        ProgramOutput()->m_startReg = offset / getGRFSize();

        // allocate space for NOS constants and pushed constants
        AllocateConstants3DShader(offset);

        IGC_ASSERT((offset % getGRFSize()) == 0);
        // Allocate space for vertex element data
        for (uint i = 0; i < setup.size(); ++i)
        {
            if (setup[i])
            {
                AllocateInput(setup[i], offset);
            }

            offset +=
                (m_ShaderDispatchMode == ShaderDispatchMode::DUAL_PATCH) ?
                getGRFSize() : SIZE_DWORD;
        }
        offset = iSTD::Align(offset, getGRFSize());
    }

    CVariable* CDomainShader::GetURBOutputHandle()
    {
        return m_pURBWriteHandleReg;
    }

    void CShaderProgram::FillProgram(SDomainShaderKernelProgram* pKernelProgram)
    {
        auto simdMode = m_context->platform.getMinDispatchMode();
        CDomainShader* pShader = static_cast<CDomainShader*>(GetShader(simdMode));
        pShader->FillProgram(pKernelProgram);

        SProgramOutput* output  = &pKernelProgram->simd8;

        *output = *pShader->ProgramOutput();

        CDomainShader* dualPatchShader = static_cast<CDomainShader*>(GetShader(simdMode, ShaderDispatchMode::DUAL_PATCH));
        if (dualPatchShader)
        {
            pKernelProgram->simd8DualPatch = *dualPatchShader->ProgramOutput();
            pKernelProgram->DispatchMode = DomainShaderDispatchModes::DS_DUAL_PATCH_DISPATCH_MODE;
        }
        else
        {
            pKernelProgram->DispatchMode = DomainShaderDispatchModes::DS_SINGLE_PATCH_DISPATCH_MODE;
        }
    }

    void CDomainShader::FillProgram(SDomainShaderKernelProgram* pKernelProgram)
    {
        CreateGatherMap();
        CreateConstantBufferOutput(pKernelProgram);

        pKernelProgram->NOSBufferSize = m_NOSBufferSize / getMinPushConstantBufferAlignmentInBytes();
        pKernelProgram->hasControlFlow = m_numBlocks > 1 ? true : false;

        pKernelProgram->MaxNumberOfThreads = m_Platform->getMaxDomainShaderThreads() / GetShaderThreadUsageRate();
        pKernelProgram->ComputeWAttribute = !m_properties.m_WArg->use_empty();
        pKernelProgram->URBAllocationSize = GetURBAllocationSize();
        pKernelProgram->VertexURBEntryOutputLength = GetURBAllocationSize() - GetURBHeaderSize();
        pKernelProgram->VertexURBEntryReadLength = GetVertexURBEntryReadLength();
        pKernelProgram->VertexURBEntryReadOffset = OctEltUnit(0);
        pKernelProgram->VertexURBEntryOutputReadOffset = GetURBHeaderSize();
        pKernelProgram->ConstantBufferLoaded = m_constantBufferLoaded;
        pKernelProgram->UavLoaded = m_uavLoaded;
        for (int i = 0; i < 4; i++)
        {
            pKernelProgram->ShaderResourceLoaded[i] = m_shaderResourceLoaded[i];
        }
        pKernelProgram->RenderTargetLoaded = m_renderTargetLoaded;

        pKernelProgram->DeclaresRTAIndex = m_properties.m_isRTAIndexDeclared;
        pKernelProgram->DeclaresVPAIndex = m_properties.m_isVPAIndexDeclared;
        pKernelProgram->HasClipCullAsOutput = m_properties.m_hasClipDistance;
        pKernelProgram->isMessageTargetDataCacheDataPort = isMessageTargetDataCacheDataPort;
        pKernelProgram->bindingTableEntryCount = this->GetMaxUsedBindingTableEntryCount();
        pKernelProgram->BindingTableEntryBitmap = this->GetBindingTableEntryBitmap();
        pKernelProgram->HasPrimitiveIDInput = this->m_hasPrimitiveIdInput;
    }

    void CDomainShader::AddEpilogue(llvm::ReturnInst* pRet)
    {
        if (this->GetContext()->platform.WaForceDSToWriteURB() &&
            m_ShaderDispatchMode != ShaderDispatchMode::DUAL_PATCH)
        {
            CVariable* channelMask = ImmToVariable(0xFF, ISA_TYPE_D);
            CVariable* URBHandle = GetNewVariable(numLanes(m_SIMDSize), ISA_TYPE_D, EALIGN_GRF, "URBWriteHandle");
            encoder.SetNoMask();
            encoder.SetSrcRegion(0, 0, 1, 0);
            encoder.Or(URBHandle, GetURBOutputHandle(), ImmToVariable(0x0000F000, ISA_TYPE_D));
            encoder.Push();
            CVariable* offset = ImmToVariable(0, ISA_TYPE_D);
            CVariable* payload = GetNewVariable(8 * numLanes(m_SIMDSize), ISA_TYPE_D, EALIGN_GRF, "URBPayload");
            encoder.SetNoMask();
            encoder.URBWrite(payload, 0, offset, URBHandle, channelMask);
            encoder.Push();
        }
        else
        {
            bool addDummyURB = true;
            if (pRet != &(*pRet->getParent()->begin()))
            {
                auto intinst = dyn_cast<GenIntrinsicInst>(pRet->getPrevNode());

                // if a URBWrite intrinsic is present no need to insert dummy urb write
                if (intinst && intinst->getIntrinsicID() == GenISAIntrinsic::GenISA_URBWrite)
                {
                    addDummyURB = false;
                }
            }

            if (addDummyURB)
            {
                EOTURBWrite();
            }
        }
        CShader::AddEpilogue(pRet);
    }

    void CDomainShader::SetShaderSpecificHelper(EmitPass* emitPass)
    {
        m_properties = emitPass->getAnalysisIfAvailable<CollectDomainShaderProperties>()->GetProperties();
    }

    void CDomainShader::ParseShaderSpecificOpcode(Instruction* inst)
    {
        switch (GetOpCode(inst))
        {
        case llvm_sgv:
        {
            SGVUsage usage = static_cast<SGVUsage>(
                dyn_cast<ConstantInt>(inst->getOperand(0))->getZExtValue());
            if (usage == PRIMITIVEID)
            {
                m_hasPrimitiveIdInput = true;
            }
            break;
        }
        default:
            break;
        }
    }

} // namespace IGC
