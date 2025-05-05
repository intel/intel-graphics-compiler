/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "CShaderProgram.hpp"
#include "ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

CShader* CShaderProgram::GetShader(SIMDMode simd, ShaderDispatchMode mode)
{
    return GetShaderPtr(simd, mode);
}

CShader*& CShaderProgram::GetShaderPtr(SIMDMode simd, ShaderDispatchMode mode)
{
    switch (mode)
    {
    case ShaderDispatchMode::DUAL_PATCH:
        return m_SIMDshaders[3];
    case ShaderDispatchMode::QUAD_SIMD8_DYNAMIC:
        IGC_ASSERT(simd == SIMDMode::SIMD32);
        return m_SIMDshaders[8];
    default:
        break;
    }

    switch (simd)
    {
    case SIMDMode::SIMD8:
        return m_SIMDshaders[0];
    case SIMDMode::SIMD16:
        return m_SIMDshaders[1];
    case SIMDMode::SIMD32:
        return m_SIMDshaders[2];
    default:
        IGC_ASSERT_MESSAGE(0, "wrong SIMD size");
        break;
    }
    return m_SIMDshaders[0];
}

void CShaderProgram::ClearShaderPtr(SIMDMode simd)
{
    switch (simd)
    {
    case SIMDMode::SIMD8:   m_SIMDshaders[0] = nullptr; break;
    case SIMDMode::SIMD16:  m_SIMDshaders[1] = nullptr; break;
    case SIMDMode::SIMD32:  m_SIMDshaders[2] = nullptr; break;
    default:
        IGC_ASSERT_MESSAGE(0, "wrong SIMD size");
        break;
    }
}

void CShaderProgram::clearBeforeRetry() {
    m_kernel = nullptr;
    for (auto S : m_SIMDshaders) {
        if (S != nullptr) {
            S->entry = nullptr;
        }
    }
}

bool CShaderProgram::hasShaderOutput(CShader* shader)
{
    return (shader && shader->ProgramOutput()->m_programSize > 0);
}

void CShaderProgram::freeShaderOutput(CShader* shader)
{
    if (hasShaderOutput(shader))
    {
        shader->ProgramOutput()->Destroy();
        shader->ProgramOutput()->m_programSize = 0;
    }
}

CShader* CShaderProgram::GetOrCreateShader(SIMDMode simd, ShaderDispatchMode mode)
{
    CShader*& pShader = GetShaderPtr(simd, mode);
    if (pShader == nullptr)
    {
        pShader = CreateNewShader(simd);
    }
    return pShader;
}

CShader* CShaderProgram::CreateNewShader(SIMDMode simd)
{
    CShader* pShader = nullptr;
    {
        switch (m_context->type)
        {
        case ShaderType::OPENCL_SHADER:
            pShader = new COpenCLKernel((OpenCLProgramContext*)m_context, m_kernel, this);
            break;
        default:
            IGC_ASSERT_MESSAGE(0, "wrong shader type");
            break;
        }
    }

    IGC_ASSERT(nullptr != pShader);

    pShader->m_shaderStats = m_shaderStats;
    pShader->m_DriverInfo = &m_context->m_DriverInfo;
    pShader->m_Platform = &m_context->platform;
    pShader->m_pBtiLayout = &m_context->btiLayout;
    pShader->m_ModuleMetadata = m_context->getModuleMetaData();
    pShader->m_PrivateMemoryPerWI = pShader->m_ModuleMetadata->PrivateMemoryPerFG[m_kernel];

    return pShader;
}

void CShaderProgram::DeleteShader(SIMDMode simd, ShaderDispatchMode mode)
{
    CShader*& pShader = GetShaderPtr(simd, mode);
    delete pShader;
    pShader = nullptr;
}