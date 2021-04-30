/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
        /// This value is used to set the corresponding field in 3DSTATE_VS.
        OctEltUnit        GetVertexURBEntryOutputReadOffset() const;

        /// Returns the size of the vertex entry read used to load payload
        /// registers. The value is calculated with vertex element component
        /// packing applied.
        OctEltUnit        GetVertexURBEntryReadLength() const;

        /// Returns the offset that should be applied to vertex entry read used to load payload registers.
        OctEltUnit        GetVertexURBEntryReadOffset() const;

        /// Returns the overall URB Allocation Size that should be used for handling of vertex data.
        OctEltUnit        GetURBAllocationSize() const;

        /// Returns the maximum input register index (vertex element) accessed
        /// in the shader plus 1. The value returned is calculated without
        /// vertex element component packing applied. Driver uses this value to
        /// to configure VF.
        QuadEltUnit       GetMaxNumInputRegister() const;

        /// Returns the number of pushed input registers, in 4*DWORD units.
        /// The value returned is calculated with vertex element component
        /// packing applied.
        QuadEltUnit       GetNumInputRegistersPushed() const;

        void              AddEpilogue(llvm::ReturnInst* pRet);

        /// Helper to compact inputs
        void              PackVFInput(unsigned int i, unsigned int& offset);

        /// Pointer to a variable representing physical GRF register R1 containing.
        CVariable* m_R1;

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
