/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/DomainShaderLowering.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/ShaderUnits.hpp"

namespace IGC
{
    class CDomainShader : public CShader
    {
    public:
        CDomainShader(llvm::Function* pFunc, CShaderProgram* pProgram);
        ~CDomainShader();

        /// Fills in the kernel program structure with data determined during compilation.
        void        FillProgram(SDomainShaderKernelProgram* pKernelProgram);
        void        PreCompile();
        void        ParseShaderSpecificOpcode(llvm::Instruction* inst);

        /// Allocates registers corresponding to input data sent in the payload.
        void        AllocatePayload();
        void        ExtractGlobalVariables();
        void        SetShaderSpecificHelper(EmitPass* emitPass);

        // Return the domain point variables
        llvm::Argument* GetDomainPointUArgu();
        llvm::Argument* GetDomainPointVArgu();
        llvm::Argument* GetDomainPointWArgu();

        void SetDomainPointUArgu(llvm::Argument* argu);
        void SetDomainPointVArgu(llvm::Argument* argu);
        void SetDomainPointWArgu(llvm::Argument* argu);

        OctEltUnit GetURBAllocationSize() const;
        OctEltUnit GetVertexURBEntryReadLength() const;
        OctEltUnit GetURBHeaderSize() const;

        virtual CVariable* GetURBInputHandle(CVariable* pVertexIndex);
        virtual CVariable* GetURBOutputHandle();
        uint32_t   GetMaxInputSignatureCount();

        virtual void AddEpilogue(llvm::ReturnInst* ret);
        CVariable* GetPrimitiveID();

        ShaderDispatchMode GetShaderDispatchMode() const;

        CVariable* GetInputDelta(uint index);
    private:
        CVariable* m_pURBWriteHandleReg;

        uint32_t               m_pMaxInputSignatureCount; // number of attributes associated with each input
        uint32_t               m_pMaxPatchConstantSignatureDeclarations; // number of patch constant declarations
        uint32_t               m_pInputControlPointCount; // number of input control points to the DS
        CVariable* m_pURBReadHandleReg;
        static const uint32_t  m_pTessFactorsURBHeader;

        std::vector<CVariable*> m_pMappedInputs;
        std::vector<CVariable*> m_pPatchConstMappedInputs;

        std::vector<CVariable*> m_pExpandMappedInputs;
        std::vector<CVariable*> m_pExpandPatchConstMappedInputs;

        CVariable* m_pPatchPrimitiveId;
        DomainShaderProperties m_properties;
        bool                   m_hasPrimitiveIdInput;
    };
}
