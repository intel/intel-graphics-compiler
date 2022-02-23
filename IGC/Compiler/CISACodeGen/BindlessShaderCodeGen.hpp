/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/ComputeShaderCommon.hpp"
#include "Probe/Assertion.h"

namespace IGC
{
    class CBindlessShader : public CComputeShaderCommon
    {
    public:
        CBindlessShader(llvm::Function* pFunc, CShaderProgram* pProgram) :
            CComputeShaderCommon(pFunc, pProgram) {}

        void PreCompile() override;
        void AllocatePayload() override;
        bool CompileSIMDSize(SIMDMode simdMode, EmitPass& EP, llvm::Function& F) override;
        void AddEpilogue(llvm::ReturnInst* ret) override;
        bool loadThreadPayload() override;
        void FillProgram(SBindlessProgram* pKernelProgram);
        CVariable* GetGlobalBufferPtr() override;
        CVariable* GetLocalBufferPtr() override {
            IGC_ASSERT_MESSAGE(nullptr != m_localBuffer, "not set!");
            return m_localBuffer;
        }
        CVariable* GetStackID() override {
            IGC_ASSERT_MESSAGE(nullptr != m_StackID, "not set!");
            return m_StackID;
        }
        CVariable* GetInlinedDataPtr() override {
            IGC_ASSERT_MESSAGE(nullptr != m_InlinedDataPtr, "not set!");
            return m_InlinedDataPtr;
        }
        bool isBindless();
        bool isContinuation();
        bool isCallStackHandler();
    private:
        RayDispatchShaderContext* getRTContext();
        CVariable* m_StackID = nullptr;
        CVariable* m_InlinedDataPtr = nullptr;
        CVariable* m_globalBuffer = nullptr;
        CVariable* m_localBuffer = nullptr;

        // For GTPin
        uint32_t GlobalPtrOffset = 0;
        uint32_t LocalPtrOffset  = 0;
        uint32_t StackIDsOffset  = 0;
    };
}
