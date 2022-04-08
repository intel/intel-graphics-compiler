/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/ComputeShaderCommon.hpp"

namespace IGC
{
    class CComputeShader : public CComputeShaderCommon
    {
    public:
        CComputeShader(llvm::Function* pFunc, CShaderProgram* pProgram);
        ~CComputeShader();

        void        AllocatePayload() override;
        void        AddPrologue() override;
        bool        CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F) override;
        void        InitEncoder(SIMDMode simdMode, bool canAbortOnSpill, ShaderDispatchMode shaderMode = ShaderDispatchMode::NOT_APPLICABLE) override;

        void        FillProgram(SComputeShaderKernelProgram* pKernelProgram);
        void        PreCompile() override;
        void        ExtractGlobalVariables() override;
        void        CreateThreadPayloadData(void*& pThreadPayload, uint& curbeTotalDataLength, uint& curbeReadLength);
        void        ParseShaderSpecificOpcode(llvm::Instruction* inst) override;

        /// Get the Thread ID's in Group
        CVariable* CreateThreadIDsinGroup(SGVUsage channelNum);
        bool        GetDispatchAlongY() const { return m_dispatchAlongY; }
        void        SetDisableMidthreadPreemption()
        {
            m_disableMidThreadPreemption = true;
        }

        bool        GetDisableMidthreadPreemption()
        {
            return m_disableMidThreadPreemption;
        }

        bool        HasSLM() const { return m_hasSLM; }
        bool        HasFullDispatchMask() override;

        bool        passNOSInlineData() override;
        bool        loadThreadPayload() override;

    protected:
        uint                   m_numberOfUntypedAccess;
        uint                   m_numberOfTypedAccess;
        uint                   m_num1DAccesses;
        uint                   m_num2DAccesses;
        uint                   m_numSLMAccesses;

        bool                   m_dispatchAlongY;
        bool                   m_disableMidThreadPreemption;
        bool                   m_hasSLM;

        uint                   m_threadGroupModifier_X;
        uint                   m_threadGroupModifier_Y;
    private:
        CShader* getSIMDEntry(CodeGenContext* ctx, SIMDMode simdMode)
        {
            CShader* prog = ctx->m_retryManager.GetSIMDEntry(simdMode);
            if (prog)
            {
                return prog;
            }
            else
            {
                return m_parent->GetShader(simdMode);
            }
        }
    };

}
