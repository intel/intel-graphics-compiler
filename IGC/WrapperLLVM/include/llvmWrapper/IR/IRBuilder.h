/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_IRBUILDER_H
#define IGCLLVM_IR_IRBUILDER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/IRBuilder.h"

#include "llvmWrapper/Support/Alignment.h"

namespace IGCLLVM
{

#if LLVM_VERSION_MAJOR <= 10
#define InserterTyDef() Inserter
#else
#define InserterTyDef() InserterTy
#endif

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
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, getCorrectAlign(Align), Src, getCorrectAlign(Align), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, llvm::Value *Src, llvm::Value *Size, unsigned Align,
            bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, getCorrectAlign(Align), Src, getCorrectAlign(Align), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src, unsigned SrcAlign,
            llvm::Value *Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, getCorrectAlign(DstAlign), Src, getCorrectAlign(SrcAlign), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src, unsigned SrcAlign,
            uint64_t Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(
                Dst, getCorrectAlign(DstAlign), Src, getCorrectAlign(SrcAlign), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag, NoAliasTag);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateMemSet;

        inline llvm::CallInst *CreateMemSet(llvm::Value *Ptr, llvm::Value *Val, uint64_t Size,
            unsigned Alignment, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemSet(
                Ptr, Val, Size, getCorrectAlign(Alignment), isVolatile, TBAATag,
                ScopeTag, NoAliasTag);
        }

        inline llvm::CallInst *CreateMemSet(llvm::Value *Ptr, llvm::Value *Val, llvm::Value *Size,
            unsigned Alignment, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemSet(
                Ptr, Val, Size, getCorrectAlign(Alignment), isVolatile, TBAATag,
                ScopeTag, NoAliasTag);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateMemMove;

        inline llvm::CallInst *CreateMemMove(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src,
            unsigned SrcAlign, uint64_t Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemMove(
                Dst, getCorrectAlign(DstAlign), Src, getCorrectAlign(SrcAlign), Size,
                isVolatile, TBAATag, ScopeTag, NoAliasTag);
        }

        inline llvm::CallInst *CreateMemMove(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src,
            unsigned SrcAlign, llvm::Value *Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemMove(
                Dst, getCorrectAlign(DstAlign), Src, getCorrectAlign(SrcAlign), Size,
                isVolatile, TBAATag, ScopeTag, NoAliasTag);
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

#if LLVM_VERSION_MAJOR >= 13
        inline llvm::LoadInst* CreateLoad(llvm::Value* Ptr, const char *Name)
        {
            Type* ptrType = Ptr->getType()->getPointerElementType();
            return llvm::IRBuilder<T, InserterTyDef()>::CreateLoad(ptrType, Ptr, Name);
        }

        inline llvm::LoadInst* CreateLoad(llvm::Value* Ptr, const Twine &Name = "")
        {
            Type* ptrType = Ptr->getType()->getPointerElementType();
            return llvm::IRBuilder<T, InserterTyDef()>::CreateLoad(ptrType, Ptr, Name);
        }

        inline llvm::LoadInst* CreateLoad(llvm::Value* Ptr, bool isVolatile, const Twine &Name = "")
        {
            Type* ptrType = Ptr->getType()->getPointerElementType();
            return llvm::IRBuilder<T, InserterTyDef()>::CreateLoad(ptrType, Ptr, isVolatile, Name);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateLoad;

        inline llvm::LoadInst* CreateAlignedLoad(llvm::Value* Ptr, IGCLLVM::Align Align, const llvm::Twine& Name = "")
        {
            Type* ptrType = Ptr->getType()->getPointerElementType();
            return llvm::IRBuilder<T, InserterTyDef()>::CreateAlignedLoad(ptrType, Ptr, Align, Name);
        }

        inline llvm::LoadInst* CreateAlignedLoad(llvm::Value* Ptr, IGCLLVM::Align Align, bool isVolatile, const llvm::Twine& Name = "")
        {
            Type* ptrType = Ptr->getType()->getPointerElementType();
            return llvm::IRBuilder<T, InserterTyDef()>::CreateAlignedLoad(ptrType, Ptr, Align, isVolatile, Name);
        }

        inline llvm::Value* CreateConstGEP1_32(
            llvm::Value* Ptr,
            unsigned Idx0,
            const llvm::Twine& Name = "")
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateConstGEP1_32(Ptr->getType()->getPointerElementType(), Ptr, Idx0, Name);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateConstGEP1_32;

        inline llvm::Value* CreateInBoundsGEP(llvm::Value *Ptr, llvm::ArrayRef<llvm::Value*> IdxList,
                           const llvm::Twine &Name = "") {
            Type *Ty = cast<PointerType>(Ptr->getType()->getScalarType())->getElementType();
            return llvm::IRBuilder<T, InserterTyDef()>::CreateInBoundsGEP(Ty, Ptr, IdxList, Name);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateInBoundsGEP;

        inline llvm::Value* CreateGEP(llvm::Value* Ptr, llvm::ArrayRef<llvm::Value*> IdxList,
            const llvm::Twine& Name = "") {
            Type* Ty = cast<PointerType>(Ptr->getType()->getScalarType())->getElementType();
            return llvm::IRBuilder<T, InserterTyDef()>::CreateGEP(Ty, Ptr, IdxList, Name);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateGEP;

        CallInst *CreateMaskedGather(Value *Ptrs, Align Alignment, Value *Mask,
                                     Value *PassThru, const Twine &Name) {
          auto *PtrsTy = cast<FixedVectorType>(Ptrs->getType());
          auto *PtrTy = cast<PointerType>(PtrsTy->getElementType());
          unsigned NumElts = PtrsTy->getNumElements();
          auto *Ty = FixedVectorType::get(PtrTy->getElementType(), NumElts);
          return llvm::IRBuilder<T, InserterTyDef()>::CreateMaskedGather(
              Ty, Ptrs, Alignment, Mask, PassThru, Name);
        }

        AtomicCmpXchgInst *
        CreateAtomicCmpXchg(Value *Ptr, Value *Cmp, Value *New,
                            AtomicOrdering SuccessOrdering,
                            AtomicOrdering FailureOrdering,
                            SyncScope::ID SSID = SyncScope::System) {
          return createAtomicCmpXchg(*this, Ptr, Cmp, New, SuccessOrdering,
                                     FailureOrdering, SSID);
        }

        AtomicRMWInst *CreateAtomicRMW(AtomicRMWInst::BinOp Op, Value *Ptr,
                                       Value *Val, AtomicOrdering Ordering,
                                       SyncScope::ID SSID = SyncScope::System) {
          return createAtomicRMW(*this, Op, Ptr, Val, Ordering, SSID);
        }

        CallInst *CreateMaskedLoad(Value *Ptr, Align Alignment, Value *Mask,
                                   Value *PassThru, const Twine &Name) {
          auto *PtrTy = cast<PointerType>(Ptr->getType());
          Type *Ty = PtrTy->getElementType();
          return llvm::IRBuilder<T, InserterTyDef()>::CreateMaskedLoad(
              Ty, Ptr, Alignment, Mask, PassThru, Name);
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

        using llvm::IRBuilder<T, InserterTyDef()>::CreateConstInBoundsGEP2_64;

        inline static llvm::CallInst* Create(llvm::Value* Func, llvm::ArrayRef<llvm::Value*> Args,
            llvm::ArrayRef<llvm::OperandBundleDef> Bundles = llvm::None,
            const llvm::Twine& NameStr = "",
            llvm::Instruction* InsertBefore = nullptr)
        {
#if LLVM_VERSION_MAJOR == 7
            return llvm::IRBuilder<T, InserterTyDef()>::Create(
                Func, Args, Bundles, NameStr, InsertBefore);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::Create(llvm::cast<llvm::FunctionType>(
                llvm::cast<llvm::PointerType>(Func->getType())->getElementType()),
                Func, Args, Bundles, NameStr, InsertBefore);
#endif
        }
    };

    template <typename T = llvm::ConstantFolder,
              typename InserterT = llvm::IRBuilderDefaultInserter>
    inline llvm::AtomicRMWInst *
    createAtomicRMW(llvm::IRBuilder<T, InserterT> &IRB,
                    llvm::AtomicRMWInst::BinOp Op, llvm::Value *Ptr,
                    llvm::Value *Val, llvm::AtomicOrdering Ordering,
                    llvm::SyncScope::ID SSID = llvm::SyncScope::System) {
#if LLVM_VERSION_MAJOR >= 13
      return IRB.CreateAtomicRMW(Op, Ptr, Val, MaybeAlign{}, Ordering, SSID);
#else
      return IRB.CreateAtomicRMW(Op, Ptr, Val, Ordering, SSID);
#endif
    }

    template <typename T = llvm::ConstantFolder,
              typename InserterT = llvm::IRBuilderDefaultInserter>
    inline llvm::AtomicCmpXchgInst *
    createAtomicCmpXchg(llvm::IRBuilder<T, InserterT> &IRB, llvm::Value *Ptr,
                        llvm::Value *Cmp, llvm::Value *New,
                        llvm::AtomicOrdering SuccessOrdering,
                        llvm::AtomicOrdering FailureOrdering,
                        llvm::SyncScope::ID SSID = llvm::SyncScope::System) {
#if LLVM_VERSION_MAJOR >= 13
      return IRB.CreateAtomicCmpXchg(Ptr, Cmp, New, MaybeAlign{},
                                     SuccessOrdering, FailureOrdering, SSID);
#else
      return IRB.CreateAtomicCmpXchg(Ptr, Cmp, New, SuccessOrdering,
                                     FailureOrdering, SSID);
#endif
    }
}

#endif
