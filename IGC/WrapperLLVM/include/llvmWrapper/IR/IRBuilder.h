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

#ifndef IGCLLVM_IR_IRBUILDER_H
#define IGCLLVM_IR_IRBUILDER_H

#include <llvm/IR/IRBuilder.h>

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR == 4
    using llvm::IRBuilder;
#elif LLVM_VERSION_MAJOR >= 7
    template <typename T = llvm::ConstantFolder,
        typename Inserter = llvm::IRBuilderDefaultInserter>
        class IRBuilder : public llvm::IRBuilder<T, Inserter>
    {
    public:
        IRBuilder(llvm::LLVMContext &C, const T &F, Inserter I = Inserter(),
            llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, Inserter>(C, F, I, FPMathTag, OpBundles) {}

        explicit IRBuilder(llvm::LLVMContext &C, llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, Inserter>(C, FPMathTag, OpBundles) {}

        explicit IRBuilder(llvm::BasicBlock *TheBB, llvm::MDNode *FPMathTag = nullptr)
            : llvm::IRBuilder<T, Inserter>(TheBB, FPMathTag) {}

        explicit IRBuilder(llvm::Instruction *IP, llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, Inserter>(IP, FPMathTag, OpBundles) {}

        IRBuilder(llvm::BasicBlock *TheBB, llvm::BasicBlock::iterator IP, const T &F,
            llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, Inserter>(TheBB, IP, F, FPMathTag, OpBundles) {}

        IRBuilder(llvm::BasicBlock *TheBB, llvm::BasicBlock::iterator IP,
            llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, Inserter>(TheBB, IP, FPMathTag, OpBundles) {}

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, llvm::Value *Src, uint64_t Size, unsigned Align,
            bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, Inserter>::CreateMemCpy(Dst, Align, Src, Align, Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, llvm::Value *Src, llvm::Value *Size, unsigned Align,
            bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, Inserter>::CreateMemCpy(Dst, Align, Src, Align, Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
        }

        inline llvm::CallInst* CreateMemCpy(llvm::Value* Dst, unsigned DstAlign, llvm::Value* Src, unsigned SrcAlign,
            llvm::Value* Size, bool isVolatile = false, llvm::MDNode* TBAATag = nullptr,
            llvm::MDNode* TBAAStructTag = nullptr,
            llvm::MDNode* ScopeTag = nullptr,
            llvm::MDNode* NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, Inserter>::CreateMemCpy(Dst, DstAlign, Src, SrcAlign, Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
        }

        inline llvm::AllocaInst *CreateAlloca(llvm::Type *Ty, llvm::Value *ArraySize = nullptr, const llvm::Twine &Name = "", unsigned AddrSpace = 0)
        {
            return llvm::IRBuilder<T, Inserter>::CreateAlloca(Ty, AddrSpace, ArraySize, Name);
        }
    };
#endif
}

#endif
