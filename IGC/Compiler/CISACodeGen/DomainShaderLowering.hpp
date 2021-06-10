/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Argument.h>
#include "common/LLVMWarningsPop.hpp"


namespace IGC
{
    class DomainShaderProperties
    {
    public:
        bool m_hasClipDistance = false;
        QuadEltUnit  m_URBOutputLength = QuadEltUnit(0);
        llvm::Value* m_UArg;
        llvm::Value* m_VArg;
        llvm::Value* m_WArg;
        bool m_isVPAIndexDeclared = false;
        bool m_isRTAIndexDeclared = false;
    };

    class CollectDomainShaderProperties : public llvm::ImmutablePass
    {
    public:
        CollectDomainShaderProperties();
        static char ID;
        void DeclareClipDistance();
        void DeclareOutput(QuadEltUnit offset);
        void SetDomainPointUArgu(llvm::Value* Arg);
        void SetDomainPointVArgu(llvm::Value* Arg);
        void SetDomainPointWArgu(llvm::Value* Arg);
        llvm::Value* GetDomainPointUArgu();
        llvm::Value* GetDomainPointVArgu();
        llvm::Value* GetDomainPointWArgu();
        void SetRTAIndexDeclared(bool isRTAIndexDeclared);
        void SetVPAIndexDeclared(bool isVPAIndexDeclared);

        const DomainShaderProperties& GetProperties() { return m_dsProps; }
    protected:
        DomainShaderProperties m_dsProps;
    };
    /// Creates a function pass that lowers input/output intrinsics to URB read/write
    llvm::FunctionPass* createDomainShaderLoweringPass();
    void initializeDomainShaderLoweringPass(llvm::PassRegistry&);
} // namespace IGC
