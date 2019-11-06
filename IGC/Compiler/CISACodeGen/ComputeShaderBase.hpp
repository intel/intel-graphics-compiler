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

#pragma once
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC
{
    class CComputeShaderBase : public CShader
    {
    public:
        CComputeShaderBase(llvm::Function* pFunc, CShaderProgram* pProgram);
        virtual ~CComputeShaderBase();

        void        CreateThreadPayloadData(
                        void*& pThreadPayload,
                        uint& curbeTotalDataLength,
                        uint& curbeReadLength,
                        bool tileY);
        void        AllocatePerThreadConstantData(uint32_t &offset);
        uint        GetNumberOfId();

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
