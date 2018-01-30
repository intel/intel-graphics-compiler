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

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

class GenericAddressAnalysis : public llvm::FunctionPass, public llvm::InstVisitor<GenericAddressAnalysis>
{
public:
    // Pass identification, replacement for typeid
    static char ID;

    bool m_hasGenericAddressSpacePointers;

    GenericAddressAnalysis();

    ~GenericAddressAnalysis();

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<MetaDataUtilsWrapper>();
    }

    virtual llvm::StringRef getPassName() const override
    {
        return "GenericAddressAnalysis";
    }

    virtual bool runOnFunction(llvm::Function &F) override;

    void visitAllocaInst(llvm::AllocaInst &I);
    void visitLoadInst(llvm::LoadInst &I);
    void visitStoreInst(llvm::StoreInst &I);
    void visitGetElementPtr(llvm::GetElementPtrInst &I);
    void visitBitCastInst(llvm::BitCastInst &I);
    void visitAddrSpaceCastInst(llvm::AddrSpaceCastInst &I);
};

class GenericAddressDynamicResolution : public llvm::FunctionPass
{
public:
    // Pass identification, replacement for typeid
    static char ID;
    llvm::Module *m_module;

    GenericAddressDynamicResolution();

    ~GenericAddressDynamicResolution();

    virtual llvm::StringRef getPassName() const override
    {
        return "GenericAddressDynamicResolution";
    }

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    virtual bool runOnFunction(llvm::Function &F) override;

    bool visitLoadStoreInst(llvm::Instruction &I);

    bool visitIntrinsicCall(llvm::CallInst& I);
    llvm::Module* getModule()
    {
        return m_module;
    }


private:
    llvm::Value* addIsAddrSpaceComparison(llvm::Value* pointer, llvm::Instruction* insertPoint, const unsigned targetAS);
    
    llvm::Type*  getPointerAsIntType(llvm::LLVMContext& Ctx, const unsigned AS);

    llvm::Value* getAddrSpaceWindowEndAddress(llvm::Instruction& insertPoint, const unsigned targetAS);
    void         resolveGAS(llvm::Instruction &I, llvm::Value* pointerOperand, const unsigned targetAS);
};

}

