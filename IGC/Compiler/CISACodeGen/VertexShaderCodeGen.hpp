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
#include "Compiler/CISACodeGen/VertexShaderLowering.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/ShaderUnits.hpp"

namespace IGC
{
    class CVertexShader : public CShader
    {
    public:
        CVertexShader(llvm::Function* pFunc, CShaderProgram* pProgram);
        ~CVertexShader();

        virtual void PreCompile();
        virtual void AddPrologue();

        virtual CVariable* GetURBOutputHandle();
        virtual CVariable* GetURBInputHandle(CVariable* pVertexIndex);

        /// Set helper pass for this shader type
        virtual  void SetShaderSpecificHelper(EmitPass* emitPass);

        /// Allocates registers corresponding to input data sent in the payload.
        virtual void        AllocatePayload();

        /// Fills in the kernel program structure with data determined during compilation.
        void        FillProgram(SVertexShaderKernelProgram* pKernelProgram);

    private:

        /// Returns the size of output Vertex URB Entry.
        OctEltUnit        GetURBHeaderSize() const;

        /// Returns the length of the URB read that should be performed by GS or SBE
        /// when reading data written by Vertex Shader.
        /// This value is used to set the corresponding field in 3DSTATE_VS.
        OctEltUnit        GetVertexURBEntryOutputReadLength() const;

        /// Returns the offset that SBE or GS should apply when reading the URB entries
        /// output by Vertex Shader.
        /// This value is used to set the corresponding field in 3DSTATE_GS.
        OctEltUnit        GetVertexURBEntryOutputReadOffset() const;

        /// Returns the size of the vertex entry read used to load payload registers.
        OctEltUnit        GetVertexURBEntryReadLength() const;

        /// Returns the offset that should be applied to vertex entry read used to load payload registers.
        OctEltUnit        GetVertexURBEntryReadOffset() const;

        /// Returns the overall URB Allocation Size that should be used for handling of vertex data.
        OctEltUnit        GetURBAllocationSize() const;

        /// Returns
        QuadEltUnit       GetMaxNumInputRegister() const;

        void              AddEpilogue(llvm::ReturnInst* pRet);

        /// Helper to compact inputs
        void              PackVFInput(unsigned int i, unsigned int& offset);

        /// Pointer to a variable representing physical GRF register R1 containing.
        CVariable* m_R1;

        /// Pointer to a variable representing physical GRF register with output URB handle
        CVariable* m_pURBWriteHandleReg;

        // Indicates if Vertex Elements Components packing was applied.
        bool m_ElementComponentPackingEnabled;

        /// Bitmask of input registers that are used
        unsigned char     m_ElementComponentEnableMask[MAX_VSHADER_INPUT_REGISTERS_PACKAGEABLE];

        /// holds max number of inputs that can be pushed for this shader unit
        static const uint32_t m_pMaxNumOfPushedInputs;

        /// Keeps information about all the properties of the VS program, its inputs and outputs.
        VertexShaderProperties m_properties;
    };

}//namespace IGC
