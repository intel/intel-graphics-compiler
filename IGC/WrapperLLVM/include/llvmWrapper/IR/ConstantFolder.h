/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CONSTANTFOLDER_H
#define IGCLLVM_IR_CONSTANTFOLDER_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/ConstantFolder.h"
#include <llvm/Support/Casting.h>
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
// The main methods of the class now get proxied to an llvm::ConstantFolder
// instance so as to avoid letting `ConstantFolderBase` class become a
// pure-virtual class. Meanwhile, IGCConstantFolder itself is switched to
// inheriting from llvm::IRBuilderFolder to make it an acceptable template
// argument for the llvm::IRBuilder hierarchy.
// This was done because of a change in llvm::ConstantFolder class,
// which made it final, i.e. impossible to inherit from.
class ConstantFolderBase : public llvm::IRBuilderFolder {
private:
  llvm::ConstantFolder m_baseConstantFolder;

public:
  ConstantFolderBase() : m_baseConstantFolder(llvm::ConstantFolder()) {}

  /// -------------------------------------------------------------------
  /// This block defines virtual methods that are present in all versions
  /// of the base llvm::IRBuilderFolder class prior to LLVM 14, and
  /// wrapper methods that are needed for build compability accross all
  /// versions prior to LLVM 15.
  /// -------------------------------------------------------------------

  /// -------------------------------------------------------------------
  /// This block defines virtual methods that are present in all versions
  /// of the base llvm::IRBuilderFolder class up to LLVM 14, and wrapper
  /// methods that are needed for build compability accross all versions
  /// up to LLVM 14.
  /// -------------------------------------------------------------------

#if (LLVM_VERSION_MAJOR < 15)

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Value *FoldAdd(llvm::Value *LHS, llvm::Value *RHS, bool HasNUW = false,
                              bool HasNSW = false) const override {
    return m_baseConstantFolder.FoldAdd(LHS, RHS, HasNUW, HasNSW);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Value *FoldAnd(llvm::Value *LHS, llvm::Value *RHS) const override {
    return m_baseConstantFolder.FoldAnd(LHS, RHS);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Value *FoldOr(llvm::Value *LHS, llvm::Value *RHS) const override {
    return m_baseConstantFolder.FoldOr(LHS, RHS);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateFAdd(llvm::Constant *LHS, llvm::Constant *RHS) const override {
    return m_baseConstantFolder.CreateFAdd(LHS, RHS);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateSub(llvm::Constant *LHS, llvm::Constant *RHS, bool HasNUW = false,
                                   bool HasNSW = false) const override {
    return m_baseConstantFolder.CreateSub(LHS, RHS, HasNUW, HasNSW);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateFSub(llvm::Constant *LHS, llvm::Constant *RHS) const override {
    return m_baseConstantFolder.CreateFSub(LHS, RHS);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateMul(llvm::Constant *LHS, llvm::Constant *RHS, bool HasNUW = false,
                                   bool HasNSW = false) const override {
    return m_baseConstantFolder.CreateMul(LHS, RHS, HasNUW, HasNSW);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateFMul(llvm::Constant *LHS, llvm::Constant *RHS) const override {
    return m_baseConstantFolder.CreateFMul(LHS, RHS);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateUDiv(llvm::Constant *LHS, llvm::Constant *RHS, bool isExact = false) const override {
    return m_baseConstantFolder.CreateUDiv(LHS, RHS, isExact);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateSDiv(llvm::Constant *LHS, llvm::Constant *RHS, bool isExact = false) const override {
    return m_baseConstantFolder.CreateSDiv(LHS, RHS, isExact);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateFDiv(llvm::Constant *LHS, llvm::Constant *RHS) const override {
    return m_baseConstantFolder.CreateFDiv(LHS, RHS);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateURem(llvm::Constant *LHS, llvm::Constant *RHS) const override {
    return m_baseConstantFolder.CreateURem(LHS, RHS);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateSRem(llvm::Constant *LHS, llvm::Constant *RHS) const override {
    return m_baseConstantFolder.CreateSRem(LHS, RHS);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateFRem(llvm::Constant *LHS, llvm::Constant *RHS) const override {
    return m_baseConstantFolder.CreateFRem(LHS, RHS);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateShl(llvm::Constant *LHS, llvm::Constant *RHS, bool HasNUW = false,
                                   bool HasNSW = false) const override {
    return m_baseConstantFolder.CreateShl(LHS, RHS, HasNUW, HasNSW);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateLShr(llvm::Constant *LHS, llvm::Constant *RHS, bool isExact = false) const override {
    return m_baseConstantFolder.CreateLShr(LHS, RHS, isExact);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateAShr(llvm::Constant *LHS, llvm::Constant *RHS, bool isExact = false) const override {
    return m_baseConstantFolder.CreateAShr(LHS, RHS, isExact);
  }

  // Note: for direct usage in code, prefer `CreateBinOp` wrapper for all
  // LLVM versions up to 15.
  inline llvm::Constant *CreateXor(llvm::Constant *LHS, llvm::Constant *RHS) const override {
    return m_baseConstantFolder.CreateXor(LHS, RHS);
  }

  inline llvm::Constant *CreateNeg(llvm::Constant *C, bool HasNUW = false, bool HasNSW = false) const override {
    return m_baseConstantFolder.CreateNeg(C, HasNUW, HasNSW);
  }

  inline llvm::Constant *CreateFNeg(llvm::Constant *C) const override { return m_baseConstantFolder.CreateFNeg(C); }

  inline llvm::Constant *CreateNot(llvm::Constant *C) const override { return m_baseConstantFolder.CreateNot(C); }

  inline llvm::Constant *CreateUnOp(llvm::Instruction::UnaryOps Opc, llvm::Constant *C) const override {
    return m_baseConstantFolder.CreateUnOp(Opc, C);
  }

  inline llvm::Constant *CreateExtractElement(llvm::Constant *Vec, llvm::Constant *Idx) const override {
    return m_baseConstantFolder.CreateExtractElement(Vec, Idx);
  }

  inline llvm::Constant *CreateInsertElement(llvm::Constant *Vec, llvm::Constant *NewElt,
                                             llvm::Constant *Idx) const override {
    return m_baseConstantFolder.CreateInsertElement(Vec, NewElt, Idx);
  }

  inline llvm::Constant *CreateShuffleVector(llvm::Constant *V1, llvm::Constant *V2,
                                             llvm::ArrayRef<int> Mask) const override {
    return m_baseConstantFolder.CreateShuffleVector(V1, V2, Mask);
  }

  inline llvm::Constant *CreateExtractValue(llvm::Constant *Agg, llvm::ArrayRef<unsigned> IdxList) const override {
    return m_baseConstantFolder.CreateExtractValue(Agg, IdxList);
  }

  inline llvm::Constant *CreateInsertValue(llvm::Constant *Agg, llvm::Constant *Val,
                                           llvm::ArrayRef<unsigned> IdxList) const override {
    return m_baseConstantFolder.CreateInsertValue(Agg, Val, IdxList);
  }
#endif // (LLVM_VERSION_MAJOR < 15)

  /// -------------------------------------------------------------------
  /// This block defines virtual methods that are present in the base
  /// llvm::IRBuilderFolder class starting from LLVM 15.
  /// -------------------------------------------------------------------

#if (LLVM_VERSION_MAJOR >= 15)
  inline llvm::Value *FoldBinOp(llvm::Instruction::BinaryOps Opc, llvm::Value *LHS, llvm::Value *RHS) const override {
    return m_baseConstantFolder.FoldBinOp(Opc, LHS, RHS);
  }

  inline llvm::Value *FoldExactBinOp(llvm::Instruction::BinaryOps Opc, llvm::Value *LHS, llvm::Value *RHS,
                                     bool IsExact) const override {
    return m_baseConstantFolder.FoldExactBinOp(Opc, LHS, RHS, IsExact);
  }

  inline llvm::Value *FoldNoWrapBinOp(llvm::Instruction::BinaryOps Opc, llvm::Value *LHS, llvm::Value *RHS, bool HasNUW,
                                      bool HasNSW) const override {
    return m_baseConstantFolder.FoldNoWrapBinOp(Opc, LHS, RHS, HasNUW, HasNSW);
  }

  inline llvm::Value *FoldBinOpFMF(llvm::Instruction::BinaryOps Opc, llvm::Value *LHS, llvm::Value *RHS,
                                   llvm::FastMathFlags FMF) const override {
    return m_baseConstantFolder.FoldBinOpFMF(Opc, LHS, RHS, FMF);
  }

  inline llvm::Value *FoldUnOpFMF(llvm::Instruction::UnaryOps Opc, llvm::Value *V,
                                  llvm::FastMathFlags FMF) const override {
    return m_baseConstantFolder.FoldUnOpFMF(Opc, V, FMF);
  }

  inline llvm::Value *FoldExtractValue(llvm::Value *Agg, llvm::ArrayRef<unsigned> IdxList) const override {
    return m_baseConstantFolder.FoldExtractValue(Agg, IdxList);
  }

  inline llvm::Value *FoldInsertValue(llvm::Value *Agg, llvm::Value *Val,
                                      llvm::ArrayRef<unsigned> IdxList) const override {
    return m_baseConstantFolder.FoldInsertValue(Agg, Val, IdxList);
  }

  inline llvm::Value *FoldExtractElement(llvm::Value *Vec, llvm::Value *Idx) const override {
    return m_baseConstantFolder.FoldExtractElement(Vec, Idx);
  }

  inline llvm::Value *FoldInsertElement(llvm::Value *Vec, llvm::Value *NewElt, llvm::Value *Idx) const override {
    return m_baseConstantFolder.FoldInsertElement(Vec, NewElt, Idx);
  }

  inline llvm::Value *FoldShuffleVector(llvm::Value *V1, llvm::Value *V2, llvm::ArrayRef<int> Mask) const override {
    return m_baseConstantFolder.FoldShuffleVector(V1, V2, Mask);
  }
#endif // LLVM_VERSION_MAJOR >= 15

  /// -------------------------------------------------------------------
  /// This block defines virtual methods that are present in all versions
  /// of the base llvm::IRBuilderFolder class up to LLVM 15, and wrapper
  /// methods that are needed for build compability accross all versions
  /// up to LLVM 15.
  /// -------------------------------------------------------------------

  inline llvm::Constant *CreateBinOp(llvm::Instruction::BinaryOps Opc, llvm::Constant *LHS, llvm::Constant *RHS) const
#if (LLVM_VERSION_MAJOR < 15)
      override {
    return m_baseConstantFolder.CreateBinOp(Opc, LHS, RHS);
  }
#else
  {
    return llvm::cast_or_null<llvm::Constant>(m_baseConstantFolder.FoldBinOp(Opc, llvm::cast_or_null<llvm::Value>(LHS),
                                                                             llvm::cast_or_null<llvm::Value>(RHS)));
  }
#endif

#if LLVM_VERSION_MAJOR < 22
  inline llvm::Value *FoldICmp(llvm::CmpInst::Predicate P, llvm::Value *LHS, llvm::Value *RHS) const override {
    return m_baseConstantFolder.FoldICmp(P, LHS, RHS);
  }
#else
  inline llvm::Value *FoldCmp(llvm::CmpInst::Predicate P, llvm::Value *LHS, llvm::Value *RHS) const override {
    return m_baseConstantFolder.FoldCmp(P, LHS, RHS);
  }
#endif

#if LLVM_VERSION_MAJOR >= 23
  inline llvm::Value *FoldSelect(llvm::Value *C, llvm::Value *True, llvm::Value *False,
                                 llvm::FastMathFlags FMF = llvm::FastMathFlags()) const override {
    return m_baseConstantFolder.FoldSelect(C, True, False, FMF);
  }
#else
  inline llvm::Value *FoldSelect(llvm::Value *C, llvm::Value *True, llvm::Value *False) const override {
    return m_baseConstantFolder.FoldSelect(C, True, False);
  }
#endif

#if LLVM_VERSION_MAJOR < 22
  inline llvm::Value *FoldGEP(llvm::Type *Ty, llvm::Value *Ptr, llvm::ArrayRef<llvm::Value *> IdxList,
                              bool IsInBounds = false) const override {
    return m_baseConstantFolder.FoldGEP(Ty, Ptr, IdxList, IsInBounds);
  }
  inline llvm::Constant *CreateCast(llvm::Instruction::CastOps Op, llvm::Constant *C,
                                    llvm::Type *DestTy) const override {
    return m_baseConstantFolder.CreateCast(Op, C, DestTy);
  }
#else
  inline llvm::Value *FoldGEP(llvm::Type *Ty, llvm::Value *Ptr, llvm::ArrayRef<llvm::Value *> IdxList,
                              llvm::GEPNoWrapFlags NW) const override {
    return m_baseConstantFolder.FoldGEP(Ty, Ptr, IdxList, NW);
  }
  inline llvm::Value *FoldCast(llvm::Instruction::CastOps Op, llvm::Value *V, llvm::Type *DestTy) const override {
    return m_baseConstantFolder.FoldCast(Op, V, DestTy);
  }
#endif

#if LLVM_VERSION_MAJOR >= 23
  inline llvm::Value *FoldIntrinsic(llvm::Intrinsic::ID ID, llvm::ArrayRef<llvm::Value *> Ops, llvm::Type *Ty,
                                    llvm::FastMathFlags FMF = {}, llvm::Function *CtxF = nullptr) const override {
    return m_baseConstantFolder.FoldIntrinsic(ID, Ops, Ty, FMF, CtxF);
  }
#elif LLVM_VERSION_MAJOR >= 22
  inline llvm::Value *FoldBinaryIntrinsic(llvm::Intrinsic::ID ID, llvm::Value *LHS, llvm::Value *RHS, llvm::Type *Ty,
                                          llvm::Instruction *FMFSource = nullptr) const override {
    return m_baseConstantFolder.FoldBinaryIntrinsic(ID, LHS, RHS, Ty, FMFSource);
  }
#endif

  inline llvm::Constant *CreatePointerCast(llvm::Constant *C, llvm::Type *DestTy) const override {
    return m_baseConstantFolder.CreatePointerCast(C, DestTy);
  }

  inline llvm::Constant *CreatePointerBitCastOrAddrSpaceCast(llvm::Constant *C, llvm::Type *DestTy) const override {
    return m_baseConstantFolder.CreatePointerBitCastOrAddrSpaceCast(C, DestTy);
  }

#if LLVM_VERSION_MAJOR < 22
  inline llvm::Constant *CreateIntCast(llvm::Constant *C, llvm::Type *DestTy, bool isSigned) const override {
    return m_baseConstantFolder.CreateIntCast(C, DestTy, isSigned);
  }

  inline llvm::Constant *CreateIntToPtr(llvm::Constant *C, llvm::Type *DestTy) const override {
    return m_baseConstantFolder.CreateCast(llvm::Instruction::IntToPtr, C, DestTy);
  }

  inline llvm::Constant *CreatePtrToInt(llvm::Constant *C, llvm::Type *DestTy) const override {
    return m_baseConstantFolder.CreateCast(llvm::Instruction::PtrToInt, C, DestTy);
  }

  inline llvm::Constant *CreateSExtOrBitCast(llvm::Constant *C, llvm::Type *DestTy) const override {
    return m_baseConstantFolder.CreateSExtOrBitCast(C, DestTy);
  }

  inline llvm::Constant *CreateTruncOrBitCast(llvm::Constant *C, llvm::Type *DestTy) const override {
    return m_baseConstantFolder.CreateTruncOrBitCast(C, DestTy);
  }

  inline llvm::Constant *CreateFCmp(llvm::CmpInst::Predicate P, llvm::Constant *LHS,
                                    llvm::Constant *RHS) const override {
    return m_baseConstantFolder.CreateFCmp(P, LHS, RHS);
  }
#endif

  inline llvm::Constant *CreateBitCast(llvm::Constant *C, llvm::Type *DestTy) const
#if LLVM_VERSION_MAJOR < 22
      override {
    return m_baseConstantFolder.CreateCast(llvm::Instruction::BitCast, C, DestTy);
  }
#else
  {
    return llvm::cast_or_null<llvm::Constant>(m_baseConstantFolder.FoldCast(llvm::Instruction::BitCast, C, DestTy));
  }
#endif

  inline llvm::Constant *CreateZExtOrBitCast(llvm::Constant *C, llvm::Type *DestTy) const
#if LLVM_VERSION_MAJOR < 22
      override {
    return m_baseConstantFolder.CreateZExtOrBitCast(C, DestTy);
  }
#else
  {
    llvm::Instruction::CastOps Op = C->getType()->getScalarSizeInBits() == DestTy->getScalarSizeInBits()
                                        ? llvm::Instruction::BitCast
                                        : llvm::Instruction::ZExt;
    return llvm::cast_or_null<llvm::Constant>(m_baseConstantFolder.FoldCast(Op, C, DestTy));
  }
#endif

  inline llvm::Constant *CreateFPCast(llvm::Constant *C, llvm::Type *DestTy) const
#if LLVM_VERSION_MAJOR < 22
      override {
    return m_baseConstantFolder.CreateFPCast(C, DestTy);
  }
#else
  {
    unsigned SrcBits = C->getType()->getScalarSizeInBits();
    unsigned DstBits = DestTy->getScalarSizeInBits();
    llvm::Instruction::CastOps Op = SrcBits == DstBits  ? llvm::Instruction::BitCast
                                    : SrcBits > DstBits ? llvm::Instruction::FPTrunc
                                                        : llvm::Instruction::FPExt;
    return llvm::cast_or_null<llvm::Constant>(m_baseConstantFolder.FoldCast(Op, C, DestTy));
  }
#endif
};
} // namespace IGCLLVM

#endif // IGCLLVM_IR_CONSTANTFOLDER_H
