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

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
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
        llvm::Function* pOldFunc);

    virtual llvm::StringRef getPassName() const
    {
        return "LegalizeFunctionSignatures";
    }

private:
    IGC::CodeGenContext* m_pContext;
    IGC::IGCMD::MetaDataUtils* m_pMdUtils;
    llvm::IRBuilder<>* m_pBuilder;
    llvm::Module* m_pModule;

    bool m_funcSignatureChanged;
    bool m_localAllocaCreated;

    std::vector<llvm::Instruction*> instsToErase;
    std::map<llvm::Function*, llvm::Function*> oldToNewFuncMap;
};

