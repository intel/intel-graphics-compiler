/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
        CHullShader(llvm::Function* pFunc, CShaderProgram* pProgram);
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
            bool EOT);

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
        CVariable* m_R1;
        CVariable* m_R2;

    private:
        /// Stores an array of vertex URB Read handles that are used for pull model data reads.
        CVariable* m_pURBWriteHandleReg;
        CVariable* m_pURBReadHandlesReg; // used for vertex data pulled from URB

        static const uint32_t  m_pMaxNumOfPushedInputs; // holds max number of inputs that can be pushed for this shader unit
        CVariable* m_IncludeVertexHandles;
        bool                   m_HasPrimitiveIDInstruction;
        uint32_t               m_pNumURBReadHandleGRF;
        HullShaderProperties   m_properties;
        bool                   m_BarrierEncountered;
    };
}
