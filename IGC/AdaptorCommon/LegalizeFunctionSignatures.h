/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

class LegalizeFunctionSignatures : public llvm::ModulePass
{
public:
    static char ID;

    LegalizeFunctionSignatures();

    ~LegalizeFunctionSignatures() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const
    {
        AU.addRequired<IGC::CodeGenContextWrapper>();
        AU.addRequired<IGC::MetaDataUtilsWrapper>();
        AU.setPreservesCFG();
    }

    virtual bool runOnModule(llvm::Module& M);

    void FixFunctionSignatures();
    void FixFunctionUsers();
    void FixCallInstruction(llvm::CallInst* callInst);

    llvm::Function* CloneFunctionSignature(llvm::Type* ReturnType,
        std::vector<llvm::Type*>& argTypes,
        llvm::Function* pOldFunc,
        bool changedRetVal);

    virtual llvm::StringRef getPassName() const
    {
        return "LegalizeFunctionSignatures";
    }

private:
    IGC::CodeGenContext* m_pContext;
    IGC::IGCMD::MetaDataUtils* m_pMdUtils;
    IGCLLVM::IRBuilder<>* m_pBuilder;
    llvm::Module* m_pModule;

    bool m_funcSignatureChanged;
    bool m_localAllocaCreated;

    std::vector<llvm::Instruction*> instsToErase;
    std::map<llvm::Function*, llvm::Function*> oldToNewFuncMap;
};

