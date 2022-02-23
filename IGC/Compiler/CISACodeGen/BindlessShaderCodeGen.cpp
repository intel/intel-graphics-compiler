/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "BindlessShaderCodeGen.hpp"
#include "LLVMUtils.h"
#include "ShaderCodeGen.hpp"
#include "AdaptorCommon/ImplicitArgs.hpp"
#include "AdaptorCommon/RayTracing/RTStackFormat.h"
#include "Probe/Assertion.h"

using namespace IGC;
using namespace llvm;

RayDispatchShaderContext* CBindlessShader::getRTContext()
{
    IGC_ASSERT(GetContext()->type == ShaderType::RAYTRACING_SHADER);
    return static_cast<RayDispatchShaderContext*>(GetContext());
}

void CBindlessShader::AllocatePayload()
{
    uint offset = 0;
    //R0 is always allocated as a predefined variable. Increase offset for R0
    IGC_ASSERT(m_R0);
    offset += getGRFSize();

    if (m_StackID)
        AllocateInput(m_StackID, offset);
    StackIDsOffset = offset;
    offset += getGRFSize();

    if (isBindless())
    {
        if (m_globalBuffer)
            AllocateInput(m_globalBuffer, offset);

        GlobalPtrOffset = offset;
        offset += 8;

        if (m_localBuffer)
            AllocateInput(m_localBuffer, offset);
        LocalPtrOffset = offset;
        offset += getGRFSize();
    }
    else
    {
        if (m_InlinedDataPtr)
            AllocateInput(m_InlinedDataPtr, offset);
        GlobalPtrOffset =
            offset + offsetof(RayDispatchInlinedData, RayDispatchGlobalDataPtr);
        LocalPtrOffset =
            offset + offsetof(RayDispatchInlinedData, RayDispatchDescriptorAddress);
        offset += getGRFSize();

        uint32_t PerThreadOffset = offset;

        AllocatePerThreadConstantData(offset);

        uint perThreadInputSize = offset - PerThreadOffset;
        encoder.GetVISAKernel()->AddKernelAttribute(
            "PerThreadInputSize", sizeof(uint16_t), &perThreadInputSize);

        auto& FuncMD = m_ModuleMetadata->FuncMD;
        auto MD = FuncMD.find(entry);
        IGC_ASSERT_MESSAGE(MD != FuncMD.end(), "missing metadata?");
        RayTraceShaderInfo& Info = MD->second.rtInfo;

        m_NOSBufferSize = Info.NOSSize;

        // Since we already read the RTGlobals and root signature via the
        // the global pointer, we don't load cross-thread constant data in
        // the shader prolog.  We need to tell VISA the appropriate size
        // so it can offset the proper amount to the local ids.
        encoder.GetVISAKernel()->AddKernelAttribute(
            "CrossThreadInputSize", sizeof(uint16_t), &m_NOSBufferSize);
    }
}

CVariable* CBindlessShader::GetGlobalBufferPtr()
{
    CVariable* globalBuffer = m_globalBuffer;

    if (!isBindless())
    {
        globalBuffer = GetNewAlias(
            m_InlinedDataPtr,
            m_InlinedDataPtr->GetType(),
            sizeof(uint64_t),
            1);
    }

    IGC_ASSERT_MESSAGE(nullptr != globalBuffer, "not set!");

    return globalBuffer;
}

void CBindlessShader::PreCompile()
{
    CreateImplicitArgs();

    m_StackID = GetNewVariable(numLanes(m_SIMDSize), ISA_TYPE_W, EALIGN_GRF, false, "StackId");

    if (isBindless())
    {
        m_globalBuffer = GetNewVariable(1, ISA_TYPE_UQ, EALIGN_QWORD, true, "GlobalBuffer");
        m_localBuffer = GetNewVariable(1, ISA_TYPE_UQ, EALIGN_QWORD, true, "LocalBuffer");
    }
    else
    {
        m_InlinedDataPtr = GetNewVariable(2, ISA_TYPE_UQ, EALIGN_QWORD, true, "InlinedDataPtr");
    }

    // We already load the RTGlobals via the global pointer at the LLVM IR
    // level.  We need to tell VISA to skip emitting block reads for that data.
    encoder.GetVISABuilder()->SetOption(vISA_loadCrossThreadConstantData, false);

    // skip r0, stack ids, and inline data
    encoder.GetVISABuilder()->SetOption(vISA_loadThreadPayloadStartReg, 3U);
}

void CBindlessShader::AddEpilogue(llvm::ReturnInst* ret)
{
    CShader::AddEpilogue(ret);
}

bool CBindlessShader::CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F)
{
    if (!CompileSIMDSizeInCommon(simdMode))
        return false;

    // Always try to compile SIMD16
    // if SIMD16 compiled successfully skip SIMD8
    if (simdMode == SIMDMode::SIMD8)
    {
        CShader* simd16Program = m_parent->GetShader(SIMDMode::SIMD16);
        if (simd16Program && simd16Program->ProgramOutput()->m_programSize > 0)
        {
            return false;
        }
    }
    return true;
}

bool CBindlessShader::loadThreadPayload()
{
    // only the raygen shader needs to load the payload
    return !isBindless() &&
           getRTContext()->m_DriverInfo.supportsRaytracingTiling() &&
           !getRTContext()->canEfficientTile();
}

bool CBindlessShader::isBindless()
{
    auto it = m_ctx->getModuleMetaData()->FuncMD.find(entry);
    if (it != m_ctx->getModuleMetaData()->FuncMD.end() &&
        IGC::isBindless(it->second))
    {
        return true;
    }
    return false;
}

bool CBindlessShader::isContinuation()
{
    auto it = m_ctx->getModuleMetaData()->FuncMD.find(entry);
    if (it != m_ctx->getModuleMetaData()->FuncMD.end() &&
        IGC::isContinuation(it->second))
    {
        return true;
    }
    return false;
}

bool CBindlessShader::isCallStackHandler()
{
    auto it = m_ctx->getModuleMetaData()->FuncMD.find(entry);
    if (it != m_ctx->getModuleMetaData()->FuncMD.end() &&
        IGC::isCallStackHandler(it->second))
    {
        return true;
    }
    return false;
}

CBindlessShader* CShaderProgram::FillProgram(SBindlessProgram* pKernelProgram)
{
    auto* ModuleMD = GetContext()->getModuleMetaData();
    auto FI = ModuleMD->FuncMD.find(m_kernel);
    IGC_ASSERT_MESSAGE(FI != ModuleMD->FuncMD.end(), "Missing shader info!");
    FunctionMetaData& MD = FI->second;
    const RayTraceShaderInfo& Info = MD.rtInfo;

    pKernelProgram->ShaderStackSize = Info.ShaderStackSize;
    pKernelProgram->ShaderType = MD.rtInfo.callableShaderType;
    pKernelProgram->isContinuation = MD.rtInfo.isContinuation;
    pKernelProgram->ParentName = MD.rtInfo.ParentName;
    pKernelProgram->SlotNum = MD.rtInfo.SlotNum;
    pKernelProgram->ShaderHash = MD.rtInfo.ShaderHash;
    pKernelProgram->Aliases = MD.rtInfo.Aliases;

    CBindlessShader* simd8Shader = static_cast<CBindlessShader*>(GetShader(SIMDMode::SIMD8));
    CBindlessShader* simd16Shader = static_cast<CBindlessShader*>(GetShader(SIMDMode::SIMD16));
    CBindlessShader* shader = nullptr;
    if (hasShaderOutput(simd8Shader))
    {
        pKernelProgram->simdProgram = *simd8Shader->ProgramOutput();
        pKernelProgram->SimdWidth = USC::GFXMEDIA_GPUWALKER_SIMD8;
        shader = simd8Shader;
    }
    else if (hasShaderOutput(simd16Shader))
    {
        pKernelProgram->simdProgram = *simd16Shader->ProgramOutput();
        pKernelProgram->SimdWidth = USC::GFXMEDIA_GPUWALKER_SIMD16;
        shader = simd16Shader;
    }
    pKernelProgram->name = m_kernel->getName().str();
    IGC_ASSERT_MESSAGE(nullptr != shader, "didn't set shader!");

    shader->FillProgram(pKernelProgram);

    return shader;
}

void CBindlessShader::FillProgram(SBindlessProgram* pKernelProgram)
{
    pKernelProgram->MaxNumberOfThreads =
        m_Platform->getMaxGPGPUShaderThreads() / GetShaderThreadUsageRate();

    pKernelProgram->GlobalPtrOffset = GlobalPtrOffset;
    pKernelProgram->LocalPtrOffset  = LocalPtrOffset;
    pKernelProgram->StackIDsOffset  = StackIDsOffset;

    pKernelProgram->hasEvalSampler = GetHasEval();

    auto* Ctx = getRTContext();

    if (!Ctx->m_DriverInfo.supportsRaytracingTiling() || isBindless())
        return;

    pKernelProgram->DimX1D = Ctx->opts().TileXDim1D;
    pKernelProgram->DimY1D = Ctx->opts().TileYDim1D;
    pKernelProgram->DimX2D = Ctx->opts().TileXDim2D;
    pKernelProgram->DimY2D = Ctx->opts().TileYDim2D;

    // If it is computed, no need to fill up the output buffer
    if (Ctx->canEfficientTile())
        return;

    // If we can't support both tilings, just fallback to localIDs populated
    // from 2D tile layout.
    pKernelProgram->DimX1D = pKernelProgram->DimX2D;
    pKernelProgram->DimY1D = pKernelProgram->DimY2D;

    m_threadGroupSize_X = pKernelProgram->DimX2D;
    m_threadGroupSize_Y = pKernelProgram->DimY2D;
    m_threadGroupSize_Z = 1;

    m_threadGroupSize =
        m_threadGroupSize_X * m_threadGroupSize_Y * m_threadGroupSize_Z;

    uint32_t ReadLen = 0;
    CreateThreadPayloadData(
        pKernelProgram->ThreadPayloadData,
        pKernelProgram->TotalDataLength,
        ReadLen,
        // TODO: experiment with TileY
        ThreadIDLayout::X);
}
