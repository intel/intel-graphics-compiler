/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_IRBUILDER_H
#define IGCLLVM_IR_IRBUILDER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/IRBuilder.h"

#include "llvmWrapper/IR/Type.h"
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

        const T& getFolder() {
#if LLVM_VERSION_MAJOR <= 10
            return llvm::IRBuilder<T, InserterTyDef()>::getFolder();
#else
            return static_cast<const T&>(llvm::IRBuilderBase::Folder);
#endif
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy;

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, llvm::Value *Src, uint64_t Size, alignment_t Align,
            bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, getCorrectAlign(Align), Src, getCorrectAlign(Align), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, llvm::Value *Src, llvm::Value *Size, alignment_t Align,
            bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, getCorrectAlign(Align), Src, getCorrectAlign(Align), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, alignment_t DstAlign, llvm::Value *Src, alignment_t SrcAlign,
            llvm::Value *Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
            llvm::MDNode *TBAAStructTag = nullptr,
            llvm::MDNode *ScopeTag = nullptr,
            llvm::MDNode *NoAliasTag = nullptr)
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateMemCpy(Dst, getCorrectAlign(DstAlign), Src, getCorrectAlign(SrcAlign), Size,
                isVolatile, TBAATag, TBAAStructTag, ScopeTag,
                NoAliasTag);
        }

        inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, alignment_t DstAlign, llvm::Value *Src, alignment_t SrcAlign,
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

#if LLVM_VERSION_MAJOR < 11
        using llvm::IRBuilder<T, InserterTyDef()>::CreateShuffleVector;

        inline llvm::Value *CreateShuffleVector(llvm::Value *V1, llvm::Value *V2, llvm::ArrayRef<int> Mask,
                                              const llvm::Twine &Name="") {
            std::vector<int> IntVec = Mask.vec();
            std::vector<uint32_t> UIntVec(IntVec.begin(), IntVec.end());
            return llvm::IRBuilder<T, InserterTyDef()>::CreateShuffleVector(V1, V2, UIntVec, Name);
        }
#endif

#if LLVM_VERSION_MAJOR >= 11

        inline llvm::CallInst* CreateCall(llvm::Value* Callee, llvm::ArrayRef<llvm::Value*> Args = llvm::None,
                                          const llvm::Twine& Name = "", llvm::MDNode* FPMathTag = nullptr) {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateCall(
                llvm::cast<llvm::Function>(Callee)->getFunctionType(),
                Callee, Args, Name, FPMathTag);
        }

        inline llvm::CallInst *
        CreateCall(llvm::Value *Callee, llvm::ArrayRef<llvm::Value *> Args,
                   llvm::ArrayRef<llvm::OperandBundleDef> OpBundles,
                   const llvm::Twine &Name = "",
                   llvm::MDNode *FPMathTag = nullptr) {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateCall(
                llvm::cast<llvm::Function>(Callee)->getFunctionType(),
                Callee, Args, OpBundles, Name, FPMathTag);
        }

        inline llvm::CallInst *
        CreateCall(llvm::FunctionType *FTy, llvm::Value *Callee,
                   llvm::ArrayRef<llvm::Value *> Args = llvm::None,
                   const llvm::Twine &Name = "",
                   llvm::MDNode *FPMathTag = nullptr) {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateCall(FTy, Callee, Args, Name, FPMathTag);
        }
#endif

#if LLVM_VERSION_MAJOR >= 13
        inline llvm::LoadInst* CreateLoad(llvm::Value* Ptr, const char *Name)
        {
            llvm::Type* ptrType = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
            return llvm::IRBuilder<T, InserterTyDef()>::CreateLoad(ptrType, Ptr, Name);
        }

        inline llvm::LoadInst* CreateLoad(llvm::Value* Ptr, const llvm::Twine &Name = "")
        {
            llvm::Type* ptrType = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
            return llvm::IRBuilder<T, InserterTyDef()>::CreateLoad(ptrType, Ptr, Name);
        }

        inline llvm::LoadInst* CreateLoad(llvm::Value* Ptr, bool isVolatile, const llvm::Twine &Name = "")
        {
            llvm::Type* ptrType = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
            return llvm::IRBuilder<T, InserterTyDef()>::CreateLoad(ptrType, Ptr, isVolatile, Name);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateLoad;

        /// Provided to resolve 'CreateAlignedLoad(Ptr, Align, "...")'
        /// correctly, instead of converting the string to 'bool' for the isVolatile
        /// parameter.
        inline llvm::LoadInst* CreateAlignedLoad(llvm::Value* Ptr, IGCLLVM::Align Align, const char* Name)
        {
            llvm::Type* ptrType = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
            return llvm::IRBuilder<T, InserterTyDef()>::CreateAlignedLoad(ptrType, Ptr, Align, Name);
        }

        inline llvm::LoadInst* CreateAlignedLoad(llvm::Value* Ptr, IGCLLVM::Align Align, const llvm::Twine& Name = "")
        {
            llvm::Type* ptrType = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
            return llvm::IRBuilder<T, InserterTyDef()>::CreateAlignedLoad(ptrType, Ptr, Align, Name);
        }

        inline llvm::LoadInst* CreateAlignedLoad(llvm::Value* Ptr, IGCLLVM::Align Align, bool isVolatile, const llvm::Twine& Name = "")
        {
            llvm::Type* ptrType = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
            return llvm::IRBuilder<T, InserterTyDef()>::CreateAlignedLoad(ptrType, Ptr, Align, isVolatile, Name);
        }

        inline llvm::Value* CreateConstGEP1_32(
            llvm::Value* Ptr,
            unsigned Idx0,
            const llvm::Twine& Name = "")
        {
            return llvm::IRBuilder<T, InserterTyDef()>::CreateConstGEP1_32(IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType()), Ptr, Idx0, Name);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateConstGEP1_32;

        inline llvm::Value* CreateInBoundsGEP(llvm::Value *Ptr, llvm::ArrayRef<llvm::Value*> IdxList,
                           const llvm::Twine &Name = "") {
            llvm::Type *Ty = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType()->getScalarType());
            return llvm::IRBuilder<T, InserterTyDef()>::CreateInBoundsGEP(Ty, Ptr, IdxList, Name);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateInBoundsGEP;

        inline llvm::Value* CreateGEP(llvm::Value* Ptr, llvm::ArrayRef<llvm::Value*> IdxList,
            const llvm::Twine& Name = "") {
            llvm::Type* Ty = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType()->getScalarType());
            return llvm::IRBuilder<T, InserterTyDef()>::CreateGEP(Ty, Ptr, IdxList, Name);
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateGEP;

        llvm::CallInst *CreateMaskedGather(llvm::Value *Ptrs, Align Alignment, llvm::Value *Mask,
                                     llvm::Value *PassThru, const llvm::Twine &Name) {
          auto *PtrsTy = llvm::cast<llvm::FixedVectorType>(Ptrs->getType());
          auto *PtrTy = llvm::cast<llvm::PointerType>(PtrsTy->getElementType());
          unsigned NumElts = PtrsTy->getNumElements();
          auto *Ty = llvm::FixedVectorType::get(IGCLLVM::getNonOpaquePtrEltTy(PtrTy), NumElts);
          return llvm::IRBuilder<T, InserterTyDef()>::CreateMaskedGather(
              Ty, Ptrs, Alignment, Mask, PassThru, Name);
        }

        llvm::AtomicCmpXchgInst *
        CreateAtomicCmpXchg(llvm::Value *Ptr, llvm::Value *Cmp, llvm::Value *New,
                            llvm::AtomicOrdering SuccessOrdering,
                            llvm::AtomicOrdering FailureOrdering,
                            llvm::SyncScope::ID SSID = llvm::SyncScope::System) {
          return createAtomicCmpXchg(*this, Ptr, Cmp, New, SuccessOrdering,
                                     FailureOrdering, SSID);
        }

        llvm::AtomicRMWInst *CreateAtomicRMW(llvm::AtomicRMWInst::BinOp Op, llvm::Value *Ptr,
                                       llvm::Value *Val, llvm::AtomicOrdering Ordering,
                                       llvm::SyncScope::ID SSID = llvm::SyncScope::System) {
          return createAtomicRMW(*this, Op, Ptr, Val, Ordering, SSID);
        }

        llvm::CallInst *CreateMaskedLoad(llvm::Value *Ptr, Align Alignment, llvm::Value *Mask,
                                   llvm::Value *PassThru, const llvm::Twine &Name) {
          auto *PtrTy = llvm::cast<llvm::PointerType>(Ptr->getType());
          llvm::Type *Ty = IGCLLVM::getNonOpaquePtrEltTy(PtrTy);
          return llvm::IRBuilder<T, InserterTyDef()>::CreateMaskedLoad(
              Ty, Ptr, Alignment, Mask, PassThru, Name);
        }

#endif

#if LLVM_VERSION_MAJOR >= 14
        llvm::Value* CreatePtrDiff(llvm::Value *LHS, llvm::Value *RHS, const llvm::Twine &Name = "") {
          auto *PtrTy = llvm::cast<llvm::PointerType>(LHS->getType());
          llvm::Type *Ty = PtrTy->getNonOpaquePointerElementType();
          return llvm::IRBuilder<T, InserterTyDef()>::CreatePtrDiff(Ty, LHS, RHS, Name);
        }
#endif

        inline llvm::Value* CreateFreezeIfSupported(llvm::Value* V, const llvm::Twine& Name = "")
        {
#if LLVM_VERSION_MAJOR < 10
            return V;
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateFreeze(V, Name);
#endif
        }

        inline llvm::Value* CreateConstInBoundsGEP2_64(
            llvm::Value* Ptr,
            uint64_t Idx0,
            uint64_t Idx1,
            const llvm::Twine& Name = "")
        {
#if LLVM_VERSION_MAJOR <= 10
            return llvm::IRBuilder<T, InserterTyDef()>::CreateConstInBoundsGEP2_64(Ptr, Idx0, Idx1, Name);
#else
            return llvm::IRBuilder<T, InserterTyDef()>::CreateConstInBoundsGEP2_64(IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType()), Ptr, Idx0, Idx1, Name);
#endif
        }

        using llvm::IRBuilder<T, InserterTyDef()>::CreateConstInBoundsGEP2_64;

    };

    template <typename T = llvm::ConstantFolder,
              typename InserterT = llvm::IRBuilderDefaultInserter>
    inline llvm::AtomicRMWInst *
    createAtomicRMW(llvm::IRBuilder<T, InserterT> &IRB,
                    llvm::AtomicRMWInst::BinOp Op, llvm::Value *Ptr,
                    llvm::Value *Val, llvm::AtomicOrdering Ordering,
                    llvm::SyncScope::ID SSID = llvm::SyncScope::System) {
#if LLVM_VERSION_MAJOR >= 13
      return IRB.CreateAtomicRMW(Op, Ptr, Val, llvm::MaybeAlign{}, Ordering, SSID);
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
      return IRB.CreateAtomicCmpXchg(Ptr, Cmp, New, llvm::MaybeAlign{},
                                     SuccessOrdering, FailureOrdering, SSID);
#else
      return IRB.CreateAtomicCmpXchg(Ptr, Cmp, New, SuccessOrdering,
                                     FailureOrdering, SSID);
#endif
    }
}

#endif
