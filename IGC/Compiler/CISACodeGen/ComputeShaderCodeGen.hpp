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
class CComputeShader : public CShader
{
public:
    CComputeShader(llvm::Function *pFunc, CShaderProgram* pProgram);
    ~CComputeShader();

    virtual void        AllocatePayload() override;
    virtual void        AddPrologue() override;
    virtual bool        CompileSIMDSize(SIMDMode simdMode, EmitPass &EP, llvm::Function &F) override;
    virtual void        InitEncoder(SIMDMode simdMode, bool canAbortOnSpill, ShaderDispatchMode shaderMode = ShaderDispatchMode::NOT_APPLICABLE) override;

    void        FillProgram(SComputeShaderKernelProgram* pKernelProgram);
    void        PreCompile() override;
    void        ExtractGlobalVariables() override;
    void        CreateThreadPayloadData(void* & pThreadPayload, uint& threadPayloadSize);
    uint        GetNumberOfId();
    void        ParseShaderSpecificOpcode(llvm::Instruction* inst) override;

    /// Get the Thread ID's in Group
    CVariable*  CreateThreadIDinGroup(uint channelNum);
    uint        GetThreadGroupSize() const { return m_threadGroupSize; }
    bool        GetDispatchAlongY() const { return m_dispatchAlongY; }
    void        SetDisableMidthreadPreemption()
    {
        m_disableMidThreadPreemption = true;
    }

    bool        HasSLM() const { return m_hasSLM; }
    bool        HasFullDispatchMask() override;

protected:
    /// Size of a thread group (X x Y x Z) provided by the front-end.
    uint                   m_threadGroupSize;
    uint                   m_threadGroupSize_X;
    uint                   m_threadGroupSize_Y;
    uint                   m_threadGroupSize_Z;

    /// The set of X/Y/Z that form the local thread ID for each channel.
    CVariable*             m_pThread_ID_in_Group_X;
    CVariable*             m_pThread_ID_in_Group_Y;
    CVariable*             m_pThread_ID_in_Group_Z;

    uint                   m_numberOfUntypedAccess;
    uint                   m_numberOfTypedAccess;
	uint				   m_num1DAccesses;
	uint				   m_num2DAccesses;

    bool                   m_dispatchAlongY;
    bool                   m_disableMidThreadPreemption;
    bool                   m_hasSLM;

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
