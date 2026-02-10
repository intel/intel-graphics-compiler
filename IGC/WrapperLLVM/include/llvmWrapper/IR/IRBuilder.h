/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_IRBUILDER_H
#define IGCLLVM_IR_IRBUILDER_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/IRBuilder.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/Support/Alignment.h"

namespace IGCLLVM {

template <typename T = llvm::ConstantFolder, typename InserterTy = llvm::IRBuilderDefaultInserter>
class IRBuilder : public llvm::IRBuilder<T, InserterTy> {
public:
  IRBuilder(llvm::LLVMContext &C, const T &F, InserterTy I = InserterTy(), llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::ArrayRef<llvm::OperandBundleDef>())
      : llvm::IRBuilder<T, InserterTy>(C, F, I, FPMathTag, OpBundles) {}

  explicit IRBuilder(llvm::LLVMContext &C, llvm::MDNode *FPMathTag = nullptr,
                     llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::ArrayRef<llvm::OperandBundleDef>())
      : llvm::IRBuilder<T, InserterTy>(C, FPMathTag, OpBundles) {}

  explicit IRBuilder(llvm::BasicBlock *TheBB, llvm::MDNode *FPMathTag = nullptr)
      : llvm::IRBuilder<T, InserterTy>(TheBB, FPMathTag) {}

  explicit IRBuilder(llvm::Instruction *IP, llvm::MDNode *FPMathTag = nullptr,
                     llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::ArrayRef<llvm::OperandBundleDef>())
      : llvm::IRBuilder<T, InserterTy>(IP, FPMathTag, OpBundles) {}

  IRBuilder(llvm::BasicBlock *TheBB, llvm::BasicBlock::iterator IP, const T &F, llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::ArrayRef<llvm::OperandBundleDef>())
      : llvm::IRBuilder<T, InserterTy>(TheBB, IP, F, FPMathTag, OpBundles) {}

  IRBuilder(llvm::BasicBlock *TheBB, llvm::BasicBlock::iterator IP, llvm::MDNode *FPMathTag = nullptr,
            llvm::ArrayRef<llvm::OperandBundleDef> OpBundles = llvm::ArrayRef<llvm::OperandBundleDef>())
      : llvm::IRBuilder<T, InserterTy>(TheBB, IP, FPMathTag, OpBundles) {}

  const T &getFolder() { return static_cast<const T &>(llvm::IRBuilderBase::Folder); }

  /// See https://github.com/llvm/llvm-project/commit/caa22582
  inline llvm::Value *CreateNegNoNUW(llvm::Value *V, const llvm::Twine &Name = "", bool HasNSW = false) {
#if LLVM_VERSION_MAJOR < 19
    return llvm::IRBuilder<T, InserterTy>::CreateNeg(V, Name, /*bool NUW=*/false, HasNSW);
#else // LLVM_VERSION_MAJOR >= 19
    return llvm::IRBuilder<T, InserterTy>::CreateNeg(V, Name, HasNSW);
#endif
  }

  /// See https://github.com/llvm/llvm-project/commit/caa22582
  inline llvm::Value *CreateNSWNegNoNUW(llvm::Value *V, const llvm::Twine &Name = "") {
#if LLVM_VERSION_MAJOR < 19
    return llvm::IRBuilder<T, InserterTy>::CreateNeg(V, Name, /*bool NUW=*/false, /*HasNSW=*/true);
#else // LLVM_VERSION_MAJOR >= 19
    return llvm::IRBuilder<T, InserterTy>::CreateNeg(V, Name, /*HasNSW=*/true);
#endif
  }

  // FIXME: Clean up pre-LLVM 14 wrappers

  using llvm::IRBuilder<T, InserterTy>::CreateMemCpy;

  inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, llvm::Value *Src, uint64_t Size, alignment_t Align,
                                      bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
                                      llvm::MDNode *TBAAStructTag = nullptr, llvm::MDNode *ScopeTag = nullptr,
                                      llvm::MDNode *NoAliasTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateMemCpy(Dst, getCorrectAlign(Align), Src, getCorrectAlign(Align), Size,
                                                        isVolatile, TBAATag, TBAAStructTag, ScopeTag, NoAliasTag);
  }

  inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, llvm::Value *Src, llvm::Value *Size, alignment_t Align,
                                      bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
                                      llvm::MDNode *TBAAStructTag = nullptr, llvm::MDNode *ScopeTag = nullptr,
                                      llvm::MDNode *NoAliasTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateMemCpy(Dst, getCorrectAlign(Align), Src, getCorrectAlign(Align), Size,
                                                        isVolatile, TBAATag, TBAAStructTag, ScopeTag, NoAliasTag);
  }

  inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, alignment_t DstAlign, llvm::Value *Src, alignment_t SrcAlign,
                                      llvm::Value *Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
                                      llvm::MDNode *TBAAStructTag = nullptr, llvm::MDNode *ScopeTag = nullptr,
                                      llvm::MDNode *NoAliasTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateMemCpy(Dst, getCorrectAlign(DstAlign), Src, getCorrectAlign(SrcAlign),
                                                        Size, isVolatile, TBAATag, TBAAStructTag, ScopeTag, NoAliasTag);
  }

  inline llvm::CallInst *CreateMemCpy(llvm::Value *Dst, alignment_t DstAlign, llvm::Value *Src, alignment_t SrcAlign,
                                      uint64_t Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
                                      llvm::MDNode *TBAAStructTag = nullptr, llvm::MDNode *ScopeTag = nullptr,
                                      llvm::MDNode *NoAliasTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateMemCpy(Dst, getCorrectAlign(DstAlign), Src, getCorrectAlign(SrcAlign),
                                                        Size, isVolatile, TBAATag, TBAAStructTag, ScopeTag, NoAliasTag);
  }

  using llvm::IRBuilder<T, InserterTy>::CreateMemSet;

  inline llvm::CallInst *CreateMemSet(llvm::Value *Ptr, llvm::Value *Val, uint64_t Size, unsigned Alignment,
                                      bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
                                      llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateMemSet(Ptr, Val, Size, getCorrectAlign(Alignment), isVolatile, TBAATag,
                                                        ScopeTag, NoAliasTag);
  }

  inline llvm::CallInst *CreateMemSet(llvm::Value *Ptr, llvm::Value *Val, llvm::Value *Size, unsigned Alignment,
                                      bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
                                      llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateMemSet(Ptr, Val, Size, getCorrectAlign(Alignment), isVolatile, TBAATag,
                                                        ScopeTag, NoAliasTag);
  }

  using llvm::IRBuilder<T, InserterTy>::CreateMemMove;

  inline llvm::CallInst *CreateMemMove(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src, unsigned SrcAlign,
                                       uint64_t Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
                                       llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateMemMove(Dst, getCorrectAlign(DstAlign), Src, getCorrectAlign(SrcAlign),
                                                         Size, isVolatile, TBAATag, ScopeTag, NoAliasTag);
  }

  inline llvm::CallInst *CreateMemMove(llvm::Value *Dst, unsigned DstAlign, llvm::Value *Src, unsigned SrcAlign,
                                       llvm::Value *Size, bool isVolatile = false, llvm::MDNode *TBAATag = nullptr,
                                       llvm::MDNode *ScopeTag = nullptr, llvm::MDNode *NoAliasTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateMemMove(Dst, getCorrectAlign(DstAlign), Src, getCorrectAlign(SrcAlign),
                                                         Size, isVolatile, TBAATag, ScopeTag, NoAliasTag);
  }

  inline llvm::AllocaInst *CreateAlloca(llvm::Type *Ty, llvm::Value *ArraySize = nullptr, const llvm::Twine &Name = "",
                                        unsigned AddrSpace = 0) {
    return llvm::IRBuilder<T, InserterTy>::CreateAlloca(Ty, AddrSpace, ArraySize, Name);
  }

  inline llvm::CallInst *CreateCall(llvm::Value *Callee,
                                    llvm::ArrayRef<llvm::Value *> Args = llvm::ArrayRef<llvm::Value *>(),
                                    const llvm::Twine &Name = "", llvm::MDNode *FPMathTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateCall(llvm::cast<llvm::Function>(Callee)->getFunctionType(), Callee,
                                                      Args, Name, FPMathTag);
  }

  inline llvm::CallInst *CreateCall(llvm::Value *Callee, llvm::ArrayRef<llvm::Value *> Args,
                                    llvm::ArrayRef<llvm::OperandBundleDef> OpBundles, const llvm::Twine &Name = "",
                                    llvm::MDNode *FPMathTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateCall(llvm::cast<llvm::Function>(Callee)->getFunctionType(), Callee,
                                                      Args, OpBundles, Name, FPMathTag);
  }

  inline llvm::CallInst *CreateCall(llvm::FunctionType *FTy, llvm::Value *Callee,
                                    llvm::ArrayRef<llvm::Value *> Args = llvm::ArrayRef<llvm::Value *>(),
                                    const llvm::Twine &Name = "", llvm::MDNode *FPMathTag = nullptr) {
    return llvm::IRBuilder<T, InserterTy>::CreateCall(FTy, Callee, Args, Name, FPMathTag);
  }

  // This wrapper function is deprecated because it uses typed pointer and
  // should no longer be used in LLVM 14+ compatible code.
  inline llvm::LoadInst *CreateLoad(llvm::Value *Ptr, const char *Name) {
    llvm::Type *ptrType = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
    return llvm::IRBuilder<T, InserterTy>::CreateLoad(ptrType, Ptr, Name);
  }

  // This wrapper function is deprecated because it uses typed pointer and
  // should no longer be used in LLVM 14+ compatible code.
  inline llvm::LoadInst *CreateLoad(llvm::Value *Ptr, const llvm::Twine &Name = "") {
    llvm::Type *ptrType = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
    return llvm::IRBuilder<T, InserterTy>::CreateLoad(ptrType, Ptr, Name);
  }

  // This wrapper function is deprecated because it uses typed pointer and
  // should no longer be used in LLVM 14+ compatible code.
  inline llvm::LoadInst *CreateLoad(llvm::Value *Ptr, bool isVolatile, const llvm::Twine &Name = "") {
    llvm::Type *ptrType = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
    return llvm::IRBuilder<T, InserterTy>::CreateLoad(ptrType, Ptr, isVolatile, Name);
  }

  using llvm::IRBuilder<T, InserterTy>::CreateLoad;

  // This wrapper function is deprecated because it uses typed pointer and
  // should no longer be used in LLVM 14+ compatible code.
  inline llvm::Value *CreateInBoundsGEP(llvm::Value *Ptr, llvm::ArrayRef<llvm::Value *> IdxList,
                                        const llvm::Twine &Name = "") {
    llvm::Type *Ty = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType()->getScalarType());
    return llvm::IRBuilder<T, InserterTy>::CreateInBoundsGEP(Ty, Ptr, IdxList, Name);
  }

  using llvm::IRBuilder<T, InserterTy>::CreateInBoundsGEP;

  // This wrapper function is deprecated because it uses typed pointer and
  // should no longer be used in LLVM 14+ compatible code.
  inline llvm::Value *CreateGEP(llvm::Value *Ptr, llvm::ArrayRef<llvm::Value *> IdxList, const llvm::Twine &Name = "") {
    llvm::Type *Ty = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType()->getScalarType());
    return llvm::IRBuilder<T, InserterTy>::CreateGEP(Ty, Ptr, IdxList, Name);
  }

  using llvm::IRBuilder<T, InserterTy>::CreateGEP;

  // This wrapper function is deprecated because it uses getNonOpaquePointerElementType() function
  // and should no longer be used in LLVM 14+ compatible code.
  llvm::Value *CreatePtrDiff(llvm::Value *LHS, llvm::Value *RHS, const llvm::Twine &Name = "") {
    auto *PtrTy = llvm::cast<llvm::PointerType>(LHS->getType());
    llvm::Type *Ty = PtrTy->getNonOpaquePointerElementType();
    return llvm::IRBuilder<T, InserterTy>::CreatePtrDiff(Ty, LHS, RHS, Name);
  }
};

} // namespace IGCLLVM

#endif
