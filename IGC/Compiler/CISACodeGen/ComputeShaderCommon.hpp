/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/


#pragma once

#include "Compiler/CISACodeGen/ComputeShaderBase.hpp"

namespace IGC
{
    class CComputeShaderCommon : public CComputeShaderBase
    {
    public:
        CComputeShaderCommon(llvm::Function* pFunc, CShaderProgram* pProgram);
        virtual ~CComputeShaderCommon();

        void        CreateThreadPayloadData(
                        void*& pThreadPayload,
                        uint& curbeTotalDataLength,
                        uint& curbeReadLength,
                        ThreadIDLayout layout) const;
        void        AllocatePerThreadConstantData(uint32_t &offset);
        uint        GetNumberOfId() const;

        /// Get the Thread ID's in Group
        CVariable* CreateThreadIDinGroup(SGVUsage channelNum);
        uint       GetThreadGroupSize() const { return m_threadGroupSize; }
    protected:
        /// Size of a thread group (X x Y x Z) provided by the front-end.
        uint                   m_threadGroupSize   = 0;
        uint                   m_threadGroupSize_X = 0;
        uint                   m_threadGroupSize_Y = 0;
        uint                   m_threadGroupSize_Z = 0;

        /// The set of X/Y/Z that form the local thread ID for each channel.
        CVariable* m_pThread_ID_in_Group_X = nullptr;
        CVariable* m_pThread_ID_in_Group_Y = nullptr;
        CVariable* m_pThread_ID_in_Group_Z = nullptr;
    };
}
