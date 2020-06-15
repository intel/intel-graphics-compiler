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

#include "Compiler/CISACodeGen/VertexShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/secure_mem.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include <iStdLib/utility.h>
#include "Probe/Assertion.h"

/***********************************************************************************
This file contains the code specific to vertex shader
************************************************************************************/

#define UseRawSendForURB 0

using namespace llvm;

namespace IGC
{

    OctEltUnit CVertexShader::GetURBHeaderSize() const
    {
        if (GetContext()->getModuleMetaData()->URBInfo.hasVertexHeader)
        {
            OctEltUnit headerSize = m_properties.m_hasClipDistance ? OctEltUnit(2) : OctEltUnit(1);
            return headerSize;
        }
        else
        {
            return OctEltUnit(0);
        }
    }

    void CVertexShader::AllocatePayload()
    {
        uint offset = 0;
        //R0 and R1 are always allocated

        //R0 is always allocated as a predefined variable. Increase offset for R0
        IGC_ASSERT(m_R0);
        offset += getGRFSize();

        IGC_ASSERT(m_R1);
        AllocateInput(m_R1, offset);
        offset += getGRFSize();

        IGC_ASSERT(getGRFSize());
        IGC_ASSERT(offset % getGRFSize() == 0);
        ProgramOutput()->m_startReg = offset / getGRFSize();

        // allocate space for NOS constants and pushed constants
        AllocateConstants3DShader(offset);

        IGC_ASSERT(offset % getGRFSize() == 0);

        // TODO: handle packed vertex attribute even if we pull
        bool packedInput = m_Platform->hasPackedVertexAttr() &&
            !isInputsPulled
            // WA: Gen11+ HW has problems with doubles on vertex shader input, if the input has unused components
            // and ElementComponentEnableMask is not full == packing occurs
            // right now only OGL is affected, so there is special disableVertexComponentPacking flag set by GLSL FE
            // if there is double on input to vertex shader
            && !m_ModuleMetadata->compOpt.disableVertexComponentPacking
            ;

        m_ElementComponentPackingEnabled = packedInput;

        if (!packedInput)
        {
            for (uint i = 0; i < MAX_VSHADER_INPUT_REGISTERS_PACKAGEABLE; i++)
            {
                m_ElementComponentEnableMask[i] = 0xF;
            }
        }
        for (uint i = 0; i < setup.size(); i++)
        {
            if (setup[i])
            {
                if (packedInput)
                {
                    PackVFInput(i, offset);
                }
                AllocateInput(setup[i], offset);
            }
            if (m_ElementComponentEnableMask[i / 4] & BIT(i % 4))
            {
                offset += getGRFSize();
            }
        }
    }

    void CVertexShader::PackVFInput(unsigned int index, unsigned int& offset)
    {
        bool dontPackPartialElement = m_ModuleMetadata->compOpt.disablePartialVertexComponentPacking ||
            IGC_IS_FLAG_ENABLED(VFPackingDisablePartialElements);
        if (dontPackPartialElement)
        {
            if (m_ElementComponentEnableMask[index / 4] == 0)
            {
                // enable the full element and push the offset to consider the elements skipped
                m_ElementComponentEnableMask[index / 4] = 0xF;
                offset += (index % 4) * getGRFSize();
            }
        }
        else
        {
            m_ElementComponentEnableMask[index / 4] |= BIT(index % 4);
        }
    }

    CVertexShader::CVertexShader(llvm::Function* pFunc, CShaderProgram* pProgram) :
        CShader(pFunc, pProgram),
        m_R1(nullptr),
        m_ElementComponentPackingEnabled(false)
    {
        for (int i = 0; i < MAX_VSHADER_INPUT_REGISTERS_PACKAGEABLE; ++i)
        {
            m_ElementComponentEnableMask[i] = 0;
        }
    }

    CVertexShader::~CVertexShader()
    {
    }

    void CShaderProgram::FillProgram(SVertexShaderKernelProgram* pKernelProgram)
    {
        CVertexShader* pShader = static_cast<CVertexShader*>(GetShader(m_context->platform.getMinDispatchMode()));
        pShader->FillProgram(pKernelProgram);
    }

    void CVertexShader::FillProgram(SVertexShaderKernelProgram* pKernelProgram)
    {
        IGC_ASSERT(nullptr != entry);
        IGC_ASSERT(entry->getParent());
        const bool isPositionOnlyShader = (entry->getParent()->getModuleFlag("IGC::PositionOnlyVertexShader") != nullptr);

        pKernelProgram->simd8 = *ProgramOutput();
        pKernelProgram->MaxNumInputRegister = GetMaxNumInputRegister();
        pKernelProgram->VertexURBEntryReadLength = GetVertexURBEntryReadLength();
        pKernelProgram->VertexURBEntryReadOffset = GetVertexURBEntryReadOffset();
        pKernelProgram->VertexURBEntryOutputReadLength = GetVertexURBEntryOutputReadLength();
        pKernelProgram->VertexURBEntryOutputReadOffset = GetVertexURBEntryOutputReadOffset();
        pKernelProgram->SBEURBReadOffset = GetVertexURBEntryOutputReadOffset();
        pKernelProgram->URBAllocationSize = GetURBAllocationSize();
        pKernelProgram->hasControlFlow = m_numBlocks > 1 ? true : false;
        pKernelProgram->MaxNumberOfThreads = m_Platform->getMaxVertexShaderThreads(isPositionOnlyShader);
        pKernelProgram->ConstantBufferLoaded = m_constantBufferLoaded;
        pKernelProgram->UavLoaded = m_uavLoaded;
        for (unsigned int i = 0; i < 4; i++)
        {
            pKernelProgram->ShaderResourceLoaded[i] = m_shaderResourceLoaded[i];
        }
        pKernelProgram->RenderTargetLoaded = m_renderTargetLoaded;

        pKernelProgram->hasVertexID = m_properties.m_HasVertexID;
        pKernelProgram->vertexIdLocation = m_properties.m_VID;
        pKernelProgram->hasInstanceID = m_properties.m_HasInstanceID;
        pKernelProgram->instanceIdLocation = m_properties.m_IID;
        pKernelProgram->vertexFetchSGVExtendedParameters = m_properties.m_VertexFetchSGVExtendedParameters;
        pKernelProgram->NOSBufferSize = m_NOSBufferSize / getGRFSize(); // in 256 bits
        pKernelProgram->DeclaresVPAIndex = m_properties.m_hasVPAI;
        pKernelProgram->DeclaresRTAIndex = m_properties.m_hasRTAI;
        pKernelProgram->HasClipCullAsOutput = m_properties.m_hasClipDistance;
        pKernelProgram->isMessageTargetDataCacheDataPort = isMessageTargetDataCacheDataPort;
        pKernelProgram->singleInstanceVertexShader =
            ((entry->getParent())->getNamedMetadata("ConstantBufferIndexedWithInstanceId") != nullptr) ? true : false;

        CreateGatherMap();
        CreateConstantBufferOutput(pKernelProgram);

        pKernelProgram->bindingTableEntryCount = this->GetMaxUsedBindingTableEntryCount();
        pKernelProgram->BindingTableEntryBitmap = this->GetBindingTableEntryBitmap();

        pKernelProgram->enableElementComponentPacking = m_ElementComponentPackingEnabled;

        for (int i = 0; i < MAX_VSHADER_INPUT_REGISTERS_PACKAGEABLE; ++i)
        {
            pKernelProgram->ElementComponentDeliverMask[i] = m_ElementComponentEnableMask[i];
        }

        // Implement workaround code. We cannot have all component enable masks equal to zero
        // so we need to enable one dummy component.
        //WaVFComponentPackingRequiresEnabledComponent is made default behavior
        //3DSTATE_VF_COMPONENT_PACKING: At least one component of a "valid"
        //Vertex Element must be enabled.
        bool anyComponentEnabled = false;
        for (int i = 0; i < MAX_VSHADER_INPUT_REGISTERS_PACKAGEABLE; ++i)
        {
            anyComponentEnabled = anyComponentEnabled || (m_ElementComponentEnableMask[i] != 0);
        }
        if (!anyComponentEnabled)
        {
            pKernelProgram->ElementComponentDeliverMask[0] = 1;
        }
    }

    void CVertexShader::PreCompile()
    {
        CreateImplicitArgs();
        m_R1 = GetNewVariable(
            numLanes(m_Platform->getMinDispatchMode()), ISA_TYPE_D, EALIGN_GRF, "R1");
        m_pURBWriteHandleReg = m_R1;

    }

    void CVertexShader::AddPrologue()
    {
    }

    CVariable* CVertexShader::GetURBOutputHandle()
    {
        return m_pURBWriteHandleReg;
    }

    CVariable* CVertexShader::GetURBInputHandle(CVariable* pVertexIndex)
    {
        return m_R1;
    }


    /// Returns VS URB allocation size.
    /// This is the size of VS URB entry consisting of the header data and attribute data.
    OctEltUnit CVertexShader::GetURBAllocationSize() const
    {
        // max index of the variables in the payload
        const EltUnit maxSetupVarNum(isInputsPulled ? 132 : setup.size());
        const OctEltUnit maxSetupOct = round_up<OctElement>(maxSetupVarNum);
        // URB allocation size is the maximum of the input and ouput entry size.
        return std::max(round_up<OctElement>(m_properties.m_URBOutputLength), maxSetupOct);
    }

    OctEltUnit CVertexShader::GetVertexURBEntryReadLength() const
    {
        // max index of the variables in the payload
        const EltUnit maxSetupVarNum(setup.size());

        // rounded up to 8-element size
        return round_up<OctElement>(maxSetupVarNum);
    }

    OctEltUnit CVertexShader::GetVertexURBEntryReadOffset() const
    {
        return OctEltUnit(0);  // since we always read in vertex header
    }

    OctEltUnit CVertexShader::GetVertexURBEntryOutputReadLength() const
    {
        // Since we skip vertex header, the output write length is the
        // total size of VUE minus the size of VUE header.
        // Note: for shaders outputing only position, the output write length calculated may be
        // less than the VUE header size because the header size can be fixed to always
        // include clip&cull distance.
        if (round_up<OctElement>(m_properties.m_URBOutputLength) > GetURBHeaderSize())
        {
            return round_up<OctElement>(m_properties.m_URBOutputLength) - GetURBHeaderSize();
        }
        // The minimum valid value for vertex URB entry output length is 1.
        return OctEltUnit(1);
    }

    OctEltUnit CVertexShader::GetVertexURBEntryOutputReadOffset() const
    {
        return GetURBHeaderSize(); // we skip the header
    }

    QuadEltUnit CVertexShader::GetMaxNumInputRegister() const
    {
        // max index of the variables in the payload
        // if there are any pulled inputs set max num input register to max possible inputs 33 * 4
        const EltUnit maxSetupVarNum(isInputsPulled ? 132 : setup.size());
        return round_up<QuadElement>(maxSetupVarNum);
    }

    void CVertexShader::SetShaderSpecificHelper(EmitPass* emitPass)
    {
        m_properties =
            emitPass->getAnalysisIfAvailable<CollectVertexShaderProperties>()->GetProperties();
    }

    void CVertexShader::AddEpilogue(llvm::ReturnInst* pRet)
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

        CShader::AddEpilogue(pRet);
    }

} // namespace IGC
