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

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/IRBuilder.h"

namespace IGCLLVM
{

#if LLVM_VERSION_MAJOR <= 10
#define InserterTyDef() Inserter
#else
#define InserterTyDef() InserterTy
#endif


#if LLVM_VERSION_MAJOR == 4
    using llvm::IRBuilder;
#elif LLVM_VERSION_MAJOR >= 7
    template <typename T = llvm::ConstantFolder,
        typename InserterTyDef() = llvm::IRBuilderDefaultInserter>
        class IRBuilder : public llvm::IRBuilder<T, InserterTyDef()>
    {
    public:
        IRBuilder(llvm::LLVMContext &C, const T &F, InserterTyDef() I = InserterTyDef()(),
            llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, InserterTyDef()>(C, F, I, FPMathTag, OpBundles) {}

        explicit IRBuilder(llvm::LLVMContext &C, llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, InserterTyDef()>(C, FPMathTag, OpBundles) {}

        explicit IRBuilder(llvm::BasicBlock *TheBB, llvm::MDNode *FPMathTag = nullptr)
            : llvm::IRBuilder<T, InserterTyDef()>(TheBB, FPMathTag) {}

        explicit IRBuilder(llvm::Instruction *IP, llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, InserterTyDef()>(IP, FPMathTag, OpBundles) {}

        IRBuilder(llvm::BasicBlock *TheBB, llvm::BasicBlock::iterator IP, const T &F,
            llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, InserterTyDef()>(TheBB, IP, F, FPMathTag, OpBundles) {}

        IRBuilder(llvm::BasicBlock *TheBB, llvm::BasicBlock::iterator IP,
            llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::None)
            : llvm::IRBuilder<T, InserterTyDef()>(TheBB, IP, FPMathTag, OpBundles) {}

        using llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy;

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, llvm::Value *Src, uint64_t Size, unsigned Align,
            bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
#if LLVM_VERSION_MAJOR < 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, Align, Src, Align, Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, llvm::Align(Align), Src, llvm::Align(Align), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
#endif
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, llvm::Value *Src, llvm::Value *Size, unsigned Align,
            bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
#if LLVM_VERSION_MAJOR < 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, Align, Src, Align, Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, llvm::Align(Align), Src, llvm::Align(Align), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
#endif
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src, unsigned SrcAlign,
            llvm::Value *Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
#if LLVM_VERSION_MAJOR < 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, DstAlign, Src, SrcAlign, Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, llvm::Align(DstAlign), Src, llvm::Align(SrcAlign), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
#endif
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src, unsigned SrcAlign,
            uint64_t Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
#if LLVM_VERSION_MAJOR < 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(
                Dst, DstAlign, Src, SrcAlign, Size, isVolatile, TBAATag,
                TBAAStructTag, ScopeTag, NoAliasTag);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(
                Dst, llvm::Align(DstAlign), Src, llvm::Align(SrcAlign), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag, NoAliasTag);
#endif
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateMemSet;

        inline llvm::CallInst *CreateMemSet(llvm::Value *Ptr, llvm::Value *Val, uint64_t Size,
            unsigned Alignment, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr)
        {
#if LLVM_VERSION_MAJOR < 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemSet(
                Ptr, Val, Size, Alignment, isVolatile, TBAATag, ScopeTag,
                NoAliasTag);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemSet(
                Ptr, Val, Size, llvm::Align(Alignment), isVolatile, TBAATag,
                ScopeTag, NoAliasTag);
#endif
        }

        inline llvm::CallInst *CreateMemSet(llvm::Value *Ptr, llvm::Value *Val, llvm::Value *Size,
            unsigned Alignment, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr)
        {
#if LLVM_VERSION_MAJOR < 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemSet(
                Ptr, Val, Size, Alignment, isVolatile, TBAATag, ScopeTag,
                NoAliasTag);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemSet(
                Ptr, Val, Size, llvm::Align(Alignment), isVolatile, TBAATag,
                ScopeTag, NoAliasTag);
#endif
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateMemMove;

        inline llvm::CallInst *CreateMemMove(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src,
            unsigned SrcAlign, uint64_t Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr)
        {
#if LLVM_VERSION_MAJOR < 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemMove(
                Dst, DstAlign, Src, SrcAlign, Size, isVolatile, TBAATag,
                ScopeTag, NoAliasTag);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemMove(
                Dst, llvm::Align(DstAlign), Src, llvm::Align(SrcAlign), Size,
                isVolatile, TBAATag, ScopeTag, NoAliasTag);
#endif
        }

        inline llvm::CallInst *CreateMemMove(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src,
            unsigned SrcAlign, llvm::Value *Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr)
        {
#if LLVM_VERSION_MAJOR < 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemMove(
                Dst, DstAlign, Src, SrcAlign, Size, isVolatile, TBAATag,
                ScopeTag, NoAliasTag);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemMove(
                Dst, llvm::Align(DstAlign), Src, llvm::Align(SrcAlign), Size,
                isVolatile, TBAATag, ScopeTag, NoAliasTag);
#endif
        }

        inline llvm::AllocaInst *CreateAlloca(llvm::Type *Ty, llvm::Value *ArraySize = nullptr, const llvm::Twine &Name = "", unsigned AddrSpace = 0)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateAlloca(Ty, AddrSpace, ArraySize, Name);
        }

        inline llvm::CallInst *CreateBinaryIntrinsic(llvm::Intrinsic::ID ID, llvm::Value *LHS,
           llvm::Value *RHS,
           llvm::Instruction *FMFSource = nullptr,
           const llvm::Twine &Name = "")
        {
#if LLVM_VERSION_MAJOR > 7
          return llvm::IRBuilder<T, InserterTyDef()>::CreateBinaryIntrinsic(
              ID, LHS, RHS, FMFSource, Name);
#else
          return llvm::IRBuilder<T, InserterTyDef()>::CreateBinaryIntrinsic(ID, LHS,
                                                                     RHS, Name);
#endif
        }

#if LLVM_VERSION_MAJOR >= 11
      inline llvm::CallInst* CreateCall(llvm::Value* Callee, llvm::ArrayRef<llvm::Value*> Args = llvm::None,
                                           const llvm::Twine& Name = "", llvm::MDNode* FPMathTag = nullptr) {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateCall(
                                                           llvm::cast<llvm::FunctionType>(Callee->getType()->getPointerElementType()), Callee,
            Args, Name, FPMathTag);
        }

        inline llvm::CallInst *
        CreateCall(llvm::Value *Callee, llvm::ArrayRef<llvm::Value *> Args,
                   llvm::ArrayRef<llvm::OperandBundleDef> OpBundles,
                   const llvm::Twine &Name = "",
                   llvm::MDNode *FPMathTag = nullptr) {
          return llvm::IRBuilder<T, InserterTyDef()>::CreateCall(
              llvm::cast<llvm::FunctionType>(
                  Callee->getType()->getPointerElementType()),
              Callee, Args, OpBundles, Name, FPMathTag);
        }

        inline llvm::CallInst *
        CreateCall(llvm::FunctionType *FTy, llvm::Value *Callee,
                   llvm::ArrayRef<llvm::Value *> Args = llvm::None,
                   const llvm::Twine &Name = "",
                   llvm::MDNode *FPMathTag = nullptr) {
          return llvm::IRBuilder<T, InserterTyDef()>::CreateCall(FTy, Callee, Args,
                                                          Name, FPMathTag);
        }
#endif

        inline llvm::Value* CreateConstInBoundsGEP2_64(
            llvm::Value* Ptr,
            uint64_t Idx0,
            uint64_t Idx1,
            const llvm::Twine& Name = "")
        {
#if LLVM_VERSION_MAJOR <= 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateConstInBoundsGEP2_64(Ptr, Idx0, Idx1, Name);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateConstInBoundsGEP2_64(Ptr->getType()->getPointerElementType(), Ptr, Idx0, Idx1, Name);
#endif
        }

        inline static llvm::CallInst* Create(llvm::Value* Func, llvm::ArrayRef<llvm::Value*> Args,
            llvm::ArrayRef<llvm::OperandBundleDef> Bundles = llvm::None,
            const llvm::Twine& NameStr = "",
            llvm::Instruction* InsertBefore = nullptr)
        {
#if LLVM_VERSION_MAJOR <= 7
            return llvm::IRBuilder<T, InserterTyDef()>::Create(
                Func, Args, Bundles, NameStr, InsertBefore);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::Create(llvm::cast<llvm::FunctionType>(
                llvm::cast<llvm::PointerType>(Func->getType())->getElementType()),
                Func, Args, Bundles, NameStr, InsertBefore);
#endif
        }

    };
#endif
}

#endif
