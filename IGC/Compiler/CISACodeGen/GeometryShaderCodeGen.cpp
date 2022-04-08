/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/PostDominators.h>
#include "common/LLVMWarningsPop.hpp"
#include "ShaderTypesEnum.h"
#include "Compiler/CISACodeGen/GeometryShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/messageEncoding.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CISACodeGen/CollectGeometryShaderProperties.hpp"
#include "common/Types.hpp"
#include "common/debug/Debug.hpp"
#include "common/secure_mem.h"
#include <iStdLib/utility.h>
#include "Probe/Assertion.h"

using namespace llvm;

/***********************************************************************************
Geometry Shader CodeGen
************************************************************************************/
namespace IGC
{
bool CGeometryShader::DiscardAdjacency(IGC::GSHADER_INPUT_PRIMITIVE_TYPE inpPrimType)
{
    bool discAdj = false;
    switch (inpPrimType)
    {
    case IGC::GSHADER_INPUT_POINT:
    case IGC::GSHADER_INPUT_PATCHLIST_1:
    case IGC::GSHADER_INPUT_PATCHLIST_2:
    case IGC::GSHADER_INPUT_PATCHLIST_3:
    case IGC::GSHADER_INPUT_PATCHLIST_4:
    case IGC::GSHADER_INPUT_PATCHLIST_5:
    case IGC::GSHADER_INPUT_PATCHLIST_6:
    case IGC::GSHADER_INPUT_PATCHLIST_7:
    case IGC::GSHADER_INPUT_PATCHLIST_8:
    case IGC::GSHADER_INPUT_PATCHLIST_9:
    case IGC::GSHADER_INPUT_PATCHLIST_10:
    case IGC::GSHADER_INPUT_PATCHLIST_11:
    case IGC::GSHADER_INPUT_PATCHLIST_12:
    case IGC::GSHADER_INPUT_PATCHLIST_13:
    case IGC::GSHADER_INPUT_PATCHLIST_14:
    case IGC::GSHADER_INPUT_PATCHLIST_15:
    case IGC::GSHADER_INPUT_PATCHLIST_16:
    case IGC::GSHADER_INPUT_PATCHLIST_17:
    case IGC::GSHADER_INPUT_PATCHLIST_18:
    case IGC::GSHADER_INPUT_PATCHLIST_19:
    case IGC::GSHADER_INPUT_PATCHLIST_20:
    case IGC::GSHADER_INPUT_PATCHLIST_21:
    case IGC::GSHADER_INPUT_PATCHLIST_22:
    case IGC::GSHADER_INPUT_PATCHLIST_23:
    case IGC::GSHADER_INPUT_PATCHLIST_24:
    case IGC::GSHADER_INPUT_PATCHLIST_25:
    case IGC::GSHADER_INPUT_PATCHLIST_26:
    case IGC::GSHADER_INPUT_PATCHLIST_27:
    case IGC::GSHADER_INPUT_PATCHLIST_28:
    case IGC::GSHADER_INPUT_PATCHLIST_29:
    case IGC::GSHADER_INPUT_PATCHLIST_30:
    case IGC::GSHADER_INPUT_PATCHLIST_31:
    case IGC::GSHADER_INPUT_PATCHLIST_32:
    case IGC::GSHADER_INPUT_LINE_ADJ:
    case IGC::GSHADER_INPUT_TRIANGLE_ADJ:
        discAdj = false;
        break;
    case IGC::GSHADER_INPUT_LINE:
    case IGC::GSHADER_INPUT_TRIANGLE:
    case IGC::GSHADER_INPUT_RECTANGLE:
    case IGC::GSHADER_INPUT_QUAD:
        discAdj = true;
        break;
    default:
        // !! unsupported input primitive type
        IGC_ASSERT_MESSAGE(0, "Unimplemented Primitive type");
        break;
    }

    return discAdj;
}

/// Returns the number of vertices each primitive type consists of.
uint CGeometryShader::GetInputPrimitiveVertexCount(IGC::GSHADER_INPUT_PRIMITIVE_TYPE inpPrimType)
{
    switch (inpPrimType)
    {
    case IGC::GSHADER_INPUT_POINT: return 1;
    case IGC::GSHADER_INPUT_LINE: return 2;
    case IGC::GSHADER_INPUT_LINE_ADJ: return 4;
    case IGC::GSHADER_INPUT_TRIANGLE: return 3;
    case IGC::GSHADER_INPUT_TRIANGLE_ADJ: return 6;
    case IGC::GSHADER_INPUT_PATCHLIST_1: return 1;
    case IGC::GSHADER_INPUT_PATCHLIST_2: return 2;
    case IGC::GSHADER_INPUT_PATCHLIST_3: return 3;
    case IGC::GSHADER_INPUT_PATCHLIST_4: return 4;
    case IGC::GSHADER_INPUT_PATCHLIST_5: return 5;
    case IGC::GSHADER_INPUT_PATCHLIST_6: return 6;
    case IGC::GSHADER_INPUT_PATCHLIST_7: return 7;
    case IGC::GSHADER_INPUT_PATCHLIST_8: return 8;
    case IGC::GSHADER_INPUT_PATCHLIST_9: return 9;
    case IGC::GSHADER_INPUT_PATCHLIST_10: return 10;
    case IGC::GSHADER_INPUT_PATCHLIST_11: return 11;
    case IGC::GSHADER_INPUT_PATCHLIST_12: return 12;
    case IGC::GSHADER_INPUT_PATCHLIST_13: return 13;
    case IGC::GSHADER_INPUT_PATCHLIST_14: return 14;
    case IGC::GSHADER_INPUT_PATCHLIST_15: return 15;
    case IGC::GSHADER_INPUT_PATCHLIST_16: return 16;
    case IGC::GSHADER_INPUT_PATCHLIST_17: return 17;
    case IGC::GSHADER_INPUT_PATCHLIST_18: return 18;
    case IGC::GSHADER_INPUT_PATCHLIST_19: return 19;
    case IGC::GSHADER_INPUT_PATCHLIST_20: return 20;
    case IGC::GSHADER_INPUT_PATCHLIST_21: return 21;
    case IGC::GSHADER_INPUT_PATCHLIST_22: return 22;
    case IGC::GSHADER_INPUT_PATCHLIST_23: return 23;
    case IGC::GSHADER_INPUT_PATCHLIST_24: return 24;
    case IGC::GSHADER_INPUT_PATCHLIST_25: return 25;
    case IGC::GSHADER_INPUT_PATCHLIST_26: return 26;
    case IGC::GSHADER_INPUT_PATCHLIST_27: return 27;
    case IGC::GSHADER_INPUT_PATCHLIST_28: return 28;
    case IGC::GSHADER_INPUT_PATCHLIST_29: return 29;
    case IGC::GSHADER_INPUT_PATCHLIST_30: return 30;
    case IGC::GSHADER_INPUT_PATCHLIST_31: return 31;
    case IGC::GSHADER_INPUT_PATCHLIST_32: return 32;

    default:
        IGC_ASSERT_MESSAGE(0, "Input primitive type not implemented");
        return 0;
    }
}

CGeometryShader::CGeometryShader(llvm::Function* pFunc, CShaderProgram* pProgram)
    : CShader(pFunc, pProgram)
    , m_pURBReadHandlesReg(nullptr)
    , m_pInstanceID(nullptr)
    , m_pURBWriteHandleReg(nullptr)
    , m_pURBWriteHandle(nullptr)
    , m_pPrimitiveIDReg(nullptr)
{
}

CGeometryShader::~CGeometryShader()
{
}

CVariable* CGeometryShader::GetPrimitiveID()
{
    if (m_pPrimitiveIDReg == nullptr)
    {
        m_pPrimitiveIDReg = GetNewVariable(
            numLanes(m_SIMDSize), ISA_TYPE_D, EALIGN_GRF, "PrimitiveID");
    }

    return m_pPrimitiveIDReg;
}

CVariable* CGeometryShader::GetInstanceID()
{
    return m_pInstanceID;
}

/// Allocates a new variable that keeps input vertex URB handles.
/// The size of the variable depends on the number of vertices of the input primitive:
/// from one for point list to six for triangle with adjacency since each vertex
/// has a separate URB handle and they are placed one after another as part of the payload.
CVariable* CGeometryShader::GetURBReadHandlesReg()
{
    if (m_pURBReadHandlesReg == nullptr)
    {
        if (m_properties.Input().HasInstancing())
        {
            m_pURBReadHandlesReg = GetNewVariable(
                (uint16_t)m_properties.Input().VertexCount(),
                ISA_TYPE_UD,
                EALIGN_GRF,
                true,
                "URBReadHandle");
        }
        else
        {
            m_pURBReadHandlesReg = GetNewVariable(
                numLanes(m_SIMDSize) * m_properties.Input().VertexCount(),
                ISA_TYPE_UD,
                EALIGN_GRF,
                "URBReadHandle");
        }
    }
    return m_pURBReadHandlesReg;
}

CVariable* CGeometryShader::GetURBOutputHandle()
{
    IGC_ASSERT(m_pURBWriteHandle);
    return m_pURBWriteHandle;
}

/// Returns a variable that has the right URB read handle register.
/// Note that for GS we have as many urb read handles as vertices in input primitive
/// and we can address them indirectly.
CVariable* CGeometryShader::GetURBInputHandle(CVariable* pVertexIndex)
{
    CVariable* pSelectedHandles = nullptr;
    if (pVertexIndex->IsImmediate())
    {
        unsigned int vertexIndex = int_cast<unsigned int>(pVertexIndex->GetImmediateValue());
        pSelectedHandles = GetNewVariable(
            numLanes(m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF, CName::NONE);

        if (m_properties.Input().HasInstancing())
        {
            encoder.SetSrcSubReg(0, vertexIndex);
        }
        else
        {
            encoder.SetSrcSubVar(0, vertexIndex);
        }

        encoder.Copy(pSelectedHandles, GetURBReadHandlesReg());
        encoder.Push();
    }
    else
    {
        CVariable* pOffset = GetNewVariable(
            numLanes(m_SIMDSize), ISA_TYPE_UW, EALIGN_GRF, CName::NONE);
        CVariable* pVertexIndexWord = BitCast(pVertexIndex, ISA_TYPE_UW);
        if (!pVertexIndexWord->IsUniform())
        {
            encoder.SetSrcRegion(0, 16, 8, 2);
        }

        if (!m_properties.Input().HasInstancing())
        {
            // offset = vertexIndex * 32 or 64(based on simdsize) since offset needs to be in bytes, not grf numbers
            CVariable* pTemp = GetNewVariable(pOffset);
            encoder.Shl(pTemp, pVertexIndexWord, ImmToVariable(iSTD::Log2(getGRFSize()), ISA_TYPE_UW));
            encoder.Push();

            // offset = temp + perLaneOffsets
            // Offset is in 1 dword. So number of bytes is 4
            encoder.Add(pOffset, pTemp, GetPerLaneOffsetsReg(4));
            encoder.Push();
        }
        else
        {
            // When instancing is on each URB handle is 4 bytes, so we multiply by 4
            encoder.Shl(pOffset, pVertexIndexWord, ImmToVariable(2, ISA_TYPE_UW));
            encoder.Push();
        }

        // selectedHandles = addressof(urbhandles) + offsets2
        CVariable* URBHandles = GetURBReadHandlesReg();
        pSelectedHandles = GetNewAddressVariable(
            pOffset->IsUniform() ? 1 : numLanes(m_SIMDSize),
            ISA_TYPE_UD,
            pOffset->IsUniform(),
            URBHandles->IsUniform(),
            URBHandles->getName());
        encoder.AddrAdd(pSelectedHandles, URBHandles, pOffset);
        encoder.Push();
    }
    return pSelectedHandles;
}

void CGeometryShader::AllocatePayload()
{
    uint offset = 0;

    // Allocate 8 DWORDS for the global URB Header which stores the cut bits.
    // The global header is 16 DWORDS which is a max of 512 bits.
    // So a GS can max output 1024 DWORDS/ 3 channels = 341 vertices
    // (1024 max dwords so assuming no attributes).

    // R0 & R1 are always allocated.
    // R1 in our case is known as the URB Handle register to make it explicit
    // as to what it contains.
    //R0 is allocated as a predefined variable. Increase offset for R0
    IGC_ASSERT(m_R0);
    offset += getGRFSize();

    // allocate input for (URB Output Handle, GS Instance ID) pairs, packed in one DWORD each
    IGC_ASSERT(m_pURBWriteHandleReg);
    AllocateInput(m_pURBWriteHandleReg, offset);
    offset += getGRFSize();

    // allocate input for primitiveID register if present
    if (m_pPrimitiveIDReg != nullptr)
    {
        AllocateInput(m_pPrimitiveIDReg, offset);
        offset += getGRFSize();
    }

    // allocate input for input vertex handles for pull model if required
    if (m_pURBReadHandlesReg != nullptr)
    {
        AllocateInput(m_pURBReadHandlesReg, offset);
        // the variable has the size equal to the number of vertices in the input primitive
        offset += m_pURBReadHandlesReg->GetSize();
        offset = iSTD::Align(offset, getGRFSize());
    }
    IGC_ASSERT(offset % getGRFSize() == 0);
    ProgramOutput()->m_startReg = offset / getGRFSize();

    // allocate space for NOS constants and pushed constants
    AllocateConstants3DShader(offset);;

    IGC_ASSERT(offset % getGRFSize() == 0);

    // when instancing mode is on, there is only one set of inputs and it's
    // laid out like in constant buffers, i.e. one attribute takes four subregisters
    // of one GRF register.
    const uint varSize = m_properties.Input().HasInstancing() ? SIZE_DWORD : getGRFSize();
    for (uint i = 0; i < setup.size(); ++i)
    {
        if (setup[i])
        {
            AllocateInput(setup[i], offset);
        }
        offset += varSize;
    }
}

void CShaderProgram::FillProgram(SGeometryShaderKernelProgram* pKernelProgram)
{
    CGeometryShader* pShader = static_cast<CGeometryShader*>(GetShader(m_context->platform.getMinDispatchMode()));
    pShader->FillProgram(pKernelProgram);
}

void CGeometryShader::FillProgram(SGeometryShaderKernelProgram* pKernelProgram)
{
    {
        pKernelProgram->simd8 = *ProgramOutput();
    }

    CreateGatherMap();
    CreateConstantBufferOutput(pKernelProgram);

    pKernelProgram->ConstantBufferLoaded = m_constantBufferLoaded;
    pKernelProgram->UavLoaded = m_uavLoaded;
    for (int i = 0; i < 4; i++)
    {
        pKernelProgram->ShaderResourceLoaded[i] = m_shaderResourceLoaded[i];
    }
    pKernelProgram->RenderTargetLoaded = m_renderTargetLoaded;

    pKernelProgram->NOSBufferSize = m_NOSBufferSize / getMinPushConstantBufferAlignmentInBytes();

    pKernelProgram->MaxNumberOfThreads = m_Platform->getMaxGeometryShaderThreads() / GetShaderThreadUsageRate();
    pKernelProgram->hasControlFlow = m_numBlocks > 1 ? true : false;

    // Gen 7 specific Compiler Output
    pKernelProgram->OutputTopology = m_properties.Output().TopologyType();
    pKernelProgram->SamplerCount = m_properties.SamplerCount();

    pKernelProgram->OutputVertexSize = GetOutputVertexSize();
    pKernelProgram->VertexEntryReadLength = GetVertexEntryReadLength();

    TODO("Fix compiler output structures for uninitialized fields");
    pKernelProgram->IncludeVertexHandles = (m_pURBReadHandlesReg != nullptr); // Used for PULL model.
    pKernelProgram->VertexEntryReadOffset = OctEltUnit(0);  // Include also vertex header, so start from the beginning.

    pKernelProgram->ControlDataHeaderFormat = m_properties.Output().ControlDataFormat();
    pKernelProgram->GSEnable = true;

    // Default StreamID is also dependent on the GSEnable but since we set it to true here we are
    // not considering it right now.
    auto defStreamID = m_properties.Output().DefaultStreamID();
    pKernelProgram->DefaultStreamID = (defStreamID != -1 ? defStreamID : 0);

    pKernelProgram->ControlDataHeaderSize = m_properties.Output().ControlDataHeaderSize();

    // Since we support only channel serial, the only mode for GS is SIMD8.
    pKernelProgram->DispatchMode = USC::GFX3DSTATE_GEOMETRY_SHADER_DISPATCH_MODE_SIMD8;

    pKernelProgram->IncludePrimitiveIDEnable = (m_pPrimitiveIDReg != nullptr);
    auto instanceCount = m_properties.Input().InstanceCount();
    pKernelProgram->InstanceCount = (instanceCount == 0) ? 1 : instanceCount;
    pKernelProgram->ReorderEnable = true;
    pKernelProgram->DiscardAdjacencyEnable = CGeometryShader::DiscardAdjacency(m_properties.Input().InputPrimitiveType());
    pKernelProgram->SBEVertexURBEntryReadOffset = pKernelProgram->VertexEntryReadOffset;

    pKernelProgram->URBAllocationSize = GetURBAllocationSize();

    pKernelProgram->MaxOutputVertexCount = m_properties.Output().MaxVertexCount();

    pKernelProgram->DeclaresClipCullDistances = m_properties.Output().PerVertex().HasClipCullDistances();
    pKernelProgram->DeclaresVPAIndex = m_properties.Output().HasViewportArrayIndex();
    pKernelProgram->DeclaresRTAIndex = m_properties.Output().HasRenderTargetArrayIndex();

    pKernelProgram->SingleProgramFlow = USC::GFX3DSTATE_PROGRAM_FLOW_MULTIPLE;

    // Gen 8 specific Compiler Output
    TODO("Max threads should be calculated based on the registers spilled. -> This needs to change")
        pKernelProgram->ExpectedVertexCount = m_properties.Input().VertexCount();
    pKernelProgram->StaticOutput = !m_properties.Output().HasNonstaticVertexCount();

    pKernelProgram->StaticOutputVertexCount =
        (m_properties.Output().HasNonstaticVertexCount()) ?
        0 : m_properties.Output().ActualStaticVertexCount();

    pKernelProgram->GSVertexURBEntryOutputReadOffset = GetVertexURBEntryOutputReadOffset();
    pKernelProgram->GSVertexURBEntryOutputReadLength = GetVertexURBEntryOutputReadLength();

    pKernelProgram->bindingTableEntryCount = this->GetMaxUsedBindingTableEntryCount();
    pKernelProgram->BindingTableEntryBitmap = this->GetBindingTableEntryBitmap();
}

void CGeometryShader::PreCompile()
{
    m_pURBWriteHandleReg = GetNewVariable(
        numLanes(m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF, "URBWriteHandle");

    CreateImplicitArgs();
}

void CGeometryShader::AddPrologue()
{
    // add variable that represents instance ID
    if (m_properties.Input().HasInstanceID())
    {
        CEncoder& encoder = GetEncoder();
        m_pInstanceID = GetNewVariable(
            numLanes(m_SIMDSize), ISA_TYPE_UD, EALIGN_GRF, "InstanceID");
        encoder.Shr(m_pInstanceID, m_pURBWriteHandleReg, ImmToVariable(27, ISA_TYPE_UD));
        encoder.Push();
    }

    // set up URB Write Handle, which may be either alias of associated payload reg,
    // or a new variable properly initialized
    {
        m_pURBWriteHandle = m_pURBWriteHandleReg;
    }

    // The 'include vertex handles' field must be set if the vertex URB entry read length is 0.
    if (m_properties.Input().PerVertex().Size().Count() == 0)
    {
        GetURBReadHandlesReg();
    }
}

void CGeometryShader::SetShaderSpecificHelper(EmitPass* emitPass)
{
    m_properties = emitPass->getAnalysisIfAvailable<CollectGeometryShaderProperties>()->GetProperties();
}

Unit<Element> CGeometryShader::GetLocalOffset(SGVUsage usage)
{
    switch (usage)
    {
    case POSITION_X:
        return Unit<Element>(0);
    case POSITION_Y:
        return Unit<Element>(1);
    case POSITION_Z:
        return Unit<Element>(2);
    case POSITION_W:
        return Unit<Element>(3);
    case POINT_WIDTH:
        return Unit<Element>(3);
    case CLIP_DISTANCE_X:
        return Unit<Element>(0);
    case CLIP_DISTANCE_Y:
        return Unit<Element>(1);
    case CLIP_DISTANCE_Z:
        return Unit<Element>(2);
    case CLIP_DISTANCE_W:
        return Unit<Element>(3);

    default:
        IGC_ASSERT_MESSAGE(0, "Gs SGV Local offset not yet defined");
    }

    return Unit<Element>(0);
}

URBAllocationUnit CGeometryShader::GetURBAllocationSize() const
{
    // Calculate the size of all vertices to be emitted, rounded up to 32B.
    auto vertexCount = m_properties.Output().HasNonstaticVertexCount() ?
        m_properties.Output().MaxVertexCount() : m_properties.Output().ActualStaticVertexCount();

    const OctEltUnit vertexSpace = round_up<OctElement>(
        m_properties.Output().PerVertex().Size() * vertexCount);

    // Add the size of control data header and output vertex count structure.

    const OctEltUnit urbEntrySize =
        vertexSpace + m_properties.Output().GlobalHeaderSize();


    // URB allocation size is in units of 512 bits, so convert from 32B to 64B.
    return round_up<URBAllocation>(urbEntrySize);
}

/// Returns the size of the output vertex.
/// Unit: 16B = 4 DWORDs
/// Note: Each output vertex must be 32B-aligned when rendering is enabled (Ref. GS URB Entry).
/// Therefore, the output vertex size is also rounded up to a multiple of 2.
QuadEltUnit CGeometryShader::GetOutputVertexSize() const
{
    QuadEltUnit vertexSize = m_properties.Output().PerVertex().Size();
    return vertexSize;
}

OctEltUnit CGeometryShader::GetInputVertexHeaderSize() const
{
    return m_properties.Input().PerVertex().HeaderSize();
}

OctEltUnit CGeometryShader::GetVertexEntryReadLength() const
{
    // if we use pull model, we don't use payload data and use URB read handles
    // in that case Vertex Entry Read Length should be zero.
    // If we don't use URB read handles, we cannot have read length zero (even if we don't read
    // anything from inputs) because of hardware restriction.
    if (setup.size() == 0 && m_pURBReadHandlesReg != nullptr)
    {
        return OctEltUnit(0);
    }

    return round_up<OctElement>(m_properties.Input().PerVertex().Size());
}

OctEltUnit CGeometryShader::GetVertexURBEntryHeaderSize() const
{
    OctEltUnit headerSize = m_properties.Output().PerVertex().HeaderSize();
    return headerSize;
}

OctEltUnit  CGeometryShader::GetVertexURBEntryOutputReadLength() const
{
    // size that take all the output attributes
    const QuadEltUnit outAttribSpace(m_properties.Output().PerVertex().MaxAttributeCount());

    // OutputRead outputs attributes, possibly padded to be a multiple of 32B.
    return round_up<OctElement>(outAttribSpace);
}

OctEltUnit  CGeometryShader::GetVertexURBEntryOutputReadOffset() const
{
    // We tell SBE to skip the Vertex URB Entry header.
    return GetVertexURBEntryHeaderSize();
}

void CGeometryShader::AddEpilogue(llvm::ReturnInst* pRet)
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

} // End of Namespace
