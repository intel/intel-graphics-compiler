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
#include "Compiler/CISACodeGen/HullShaderLowering.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/ShaderUnits.hpp"

namespace IGC
{
class CHullShader : public CShader
{
public:
    CHullShader(llvm::Function *pFunc, CShaderProgram* pProgram);
    ~CHullShader();

    /// Fills in the kernel program structure with data determined during compilation.
    void        FillProgram(SHullShaderKernelProgram* pKernelProgram);
    void        PreCompile();
    void        ParseShaderSpecificOpcode(llvm::Instruction* inst);
    void        AddPrologue();

    /// Allocates registers corresponding to input data sent in the payload.
    void        AllocatePayload();
    void        AllocateSinglePatchPayload();
    void        AllocateEightPatchPayload();

    void        SetShaderSpecificHelper(EmitPass* emitPass);

    CVariable* GetURBReadHandlesReg();
    CVariable* GetR1();
    CVariable* GetR2();
    virtual CVariable* GetURBInputHandle(CVariable* pVertexIndex);
    virtual uint32_t GetMaxNumOfPushedInputs() const;

    void EmitPatchConstantHeader(
        CVariable* var[],
        bool EOT );

    OctEltUnit GetURBAllocationSize() const;
    OctEltUnit GetPatchConstantURBSize() const;

    /// Returns the size of the vertex entry read used to load payload registers.
    // Unit: 32B = 8DWORDs.
    OctEltUnit GetVertexURBEntryReadLength() const;

    /// Returns a variable that stores URB write handle register
    virtual CVariable* GetURBOutputHandle();

    CVariable* GetPrimitiveID();

    HullShaderDispatchModes GetShaderDispatchMode();
    HullShaderDispatchModes DetermineDispatchMode() const;

    virtual void AddEpilogue(llvm::ReturnInst* ret);

    void SetBarrierEncountered();

    uint32_t DetermineInstanceCount();

    OctEltUnit GetURBHeaderSize();

    HullShaderProperties GetHullShaderProperties()
    {
        return m_properties;
    }

protected:
    bool NeedVertexHandle();
    CVariable*             m_R1;
    CVariable*             m_R2;

private:
    /// Stores an array of vertex URB Read handles that are used for pull model data reads.
    CVariable*             m_pURBWriteHandleReg;
    CVariable*             m_pURBReadHandlesReg; // used for vertex data pulled from URB

    static const uint32_t  m_pMaxNumOfPushedInputs; // holds max number of inputs that can be pushed for this shader unit
    CVariable*             m_IncludeVertexHandles;
    bool                   m_HasPrimitiveIDInstruction;
    uint32_t               m_pNumURBReadHandleGRF;
    HullShaderProperties   m_properties;
    bool                   m_pBarrierEncountered;
};
}
