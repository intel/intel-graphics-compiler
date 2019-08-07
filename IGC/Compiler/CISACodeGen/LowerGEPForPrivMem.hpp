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
#include <vector>
namespace llvm
{
    class FunctionPass;
    class AllocaInst;
    class Instruction;
    class Value;
    class GetElementPtrInst;
    class StoreInst;
    class IntrinsicInst;
    class Type;
}

namespace IGC
{
    /// Tries to promote array in private memory to indexable vector
    /// Uses register pressure to make sure it won't cause spilling
    llvm::FunctionPass* createPromotePrivateArrayToReg();

    /// Array can be stored in SOA form
    /// Helper allowing function to detect if an array can stored in SOA form
    /// It is true
    bool CanUseSOALayout(llvm::AllocaInst* inst, llvm::Type*& baseType);

    class TransposeHelper
    {
    public:
        TransposeHelper(bool vectorIndex) : m_vectorIndex(vectorIndex) {}
        void HandleAllocaSources(
            llvm::Instruction* v,
            llvm::Value* idx);
        void handleGEPInst(
            llvm::GetElementPtrInst* pGEP,
            llvm::Value* idx);
        virtual void handleLoadInst(
            llvm::LoadInst* pLoad,
            llvm::Value* pScalarizedIdx) = 0;
        virtual void handleStoreInst(
            llvm::StoreInst* pStore,
            llvm::Value* pScalarizedIdx) = 0;
        void handleLifetimeMark(llvm::IntrinsicInst* inst);
        void EraseDeadCode();
    private:
        bool m_vectorIndex;
        std::vector<llvm::Instruction*> m_toBeRemovedGEP;
    };
}
