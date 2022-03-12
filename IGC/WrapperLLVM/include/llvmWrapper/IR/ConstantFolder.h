/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CONSTANTFOLDER_H
#define IGCLLVM_IR_CONSTANTFOLDER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/ConstantFolder.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR <= 10
    using ConstantFolderBase = llvm::ConstantFolder;
#else
    // The main methods of the class now get proxied to an llvm::ConstantFolder instance,
    // while IGCConstantFolder itself is switched to inheriting from
    // llvm::IRBuilderFolder to make it an acceptable template argument for the
    // llvm::IRBuilder hierarchy.
    // This was done because of a change in llvm::ConstantFolder class,
    // which made it final, i.e. impossible to inherit from.
    class ConstantFolderBase : public llvm::IRBuilderFolder
    {
    private:
        llvm::ConstantFolder m_baseConstantFolder;
    public:
        ConstantFolderBase() :
            m_baseConstantFolder(llvm::ConstantFolder()) {}

        inline llvm::Constant* CreateAdd(llvm::Constant* LHS, llvm::Constant* RHS,
            bool HasNUW = false, bool HasNSW = false) const override {
            return m_baseConstantFolder.CreateAdd(LHS, RHS, HasNUW, HasNSW);
        }

        inline llvm::Constant* CreateFAdd(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateFAdd(LHS, RHS);
        }

        inline llvm::Constant* CreateSub(llvm::Constant* LHS, llvm::Constant* RHS,
            bool HasNUW = false, bool HasNSW = false) const override {
            return m_baseConstantFolder.CreateSub(LHS, RHS, HasNUW, HasNSW);
        }

        inline llvm::Constant* CreateFSub(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateFSub(LHS, RHS);
        }

        inline llvm::Constant* CreateMul(llvm::Constant* LHS, llvm::Constant* RHS,
            bool HasNUW = false, bool HasNSW = false) const override {
            return m_baseConstantFolder.CreateMul(LHS, RHS, HasNUW, HasNSW);
        }

        inline llvm::Constant* CreateFMul(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateFMul(LHS, RHS);
        }

        inline llvm::Constant* CreateUDiv(llvm::Constant* LHS, llvm::Constant* RHS,
            bool isExact = false) const override {
            return m_baseConstantFolder.CreateUDiv(LHS, RHS, isExact);
        }

        inline llvm::Constant* CreateSDiv(llvm::Constant* LHS, llvm::Constant* RHS,
            bool isExact = false) const override {
            return m_baseConstantFolder.CreateSDiv(LHS, RHS, isExact);
        }

        inline llvm::Constant* CreateFDiv(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateFDiv(LHS, RHS);
        }

        inline llvm::Constant* CreateURem(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateURem(LHS, RHS);
        }

        inline llvm::Constant* CreateSRem(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateSRem(LHS, RHS);
        }

        inline llvm::Constant* CreateFRem(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateFRem(LHS, RHS);
        }

        inline llvm::Constant* CreateShl(llvm::Constant* LHS, llvm::Constant* RHS,
            bool HasNUW = false, bool HasNSW = false) const override {
            return m_baseConstantFolder.CreateShl(LHS, RHS, HasNUW, HasNSW);
        }

        inline llvm::Constant* CreateLShr(llvm::Constant* LHS, llvm::Constant* RHS,
            bool isExact = false) const override {
            return m_baseConstantFolder.CreateLShr(LHS, RHS, isExact);
        }

        inline llvm::Constant* CreateAShr(llvm::Constant* LHS, llvm::Constant* RHS,
            bool isExact = false) const override {
            return m_baseConstantFolder.CreateAShr(LHS, RHS, isExact);
        }

        inline llvm::Constant* CreateAnd(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateAnd(LHS, RHS);
        }

        inline llvm::Constant* CreateOr(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateOr(LHS, RHS);
        }

        inline llvm::Constant* CreateXor(llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateXor(LHS, RHS);
        }

        inline llvm::Constant* CreateBinOp(llvm::Instruction::BinaryOps Opc,
            llvm::Constant* LHS, llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateBinOp(Opc, LHS, RHS);
        }

        inline llvm::Constant* CreateNeg(llvm::Constant* C,
            bool HasNUW = false, bool HasNSW = false) const override {
            return m_baseConstantFolder.CreateNeg(C, HasNUW, HasNSW);
        }

        inline llvm::Constant* CreateFNeg(llvm::Constant* C) const override {
            return m_baseConstantFolder.CreateFNeg(C);
        }

        inline llvm::Constant* CreateNot(llvm::Constant* C) const override {
            return m_baseConstantFolder.CreateNot(C);
        }

        inline llvm::Constant* CreateUnOp(llvm::Instruction::UnaryOps Opc, llvm::Constant* C) const override {
            return m_baseConstantFolder.CreateUnOp(Opc, C);
        }

        inline llvm::Constant* CreateGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
            llvm::ArrayRef<llvm::Constant*> IdxList) const override {
            return m_baseConstantFolder.CreateGetElementPtr(Ty, C, IdxList);
        }

        inline llvm::Constant* CreateGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
            llvm::Constant* Idx) const override {
            return m_baseConstantFolder.CreateGetElementPtr(Ty, C, Idx);
        }

        inline llvm::Constant* CreateGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
            llvm::ArrayRef<llvm::Value*> IdxList) const override {
            return m_baseConstantFolder.CreateGetElementPtr(Ty, C, IdxList);
        }

        inline llvm::Constant* CreateInBoundsGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
            llvm::ArrayRef<llvm::Constant*> IdxList) const override {
            return m_baseConstantFolder.CreateInBoundsGetElementPtr(Ty, C, IdxList);
        }

        inline llvm::Constant* CreateInBoundsGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
            llvm::Constant* Idx) const override {
            return m_baseConstantFolder.CreateInBoundsGetElementPtr(Ty, C, Idx);
        }

        inline llvm::Constant* CreateInBoundsGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
            llvm::ArrayRef<llvm::Value*> IdxList) const override {
            return m_baseConstantFolder.CreateInBoundsGetElementPtr(Ty, C, IdxList);
        }

        inline llvm::Constant* CreateCast(llvm::Instruction::CastOps Op, llvm::Constant* C,
            llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreateCast(Op, C, DestTy);
        }

        inline llvm::Constant* CreatePointerCast(llvm::Constant* C, llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreatePointerCast(C, DestTy);
        }

        inline llvm::Constant* CreatePointerBitCastOrAddrSpaceCast(llvm::Constant* C,
            llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreatePointerBitCastOrAddrSpaceCast(C, DestTy);
        }

        inline llvm::Constant* CreateIntCast(llvm::Constant* C, llvm::Type* DestTy,
            bool isSigned) const override {
            return m_baseConstantFolder.CreateIntCast(C, DestTy, isSigned);
        }

        inline llvm::Constant* CreateFPCast(llvm::Constant* C, llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreateFPCast(C, DestTy);
        }

        inline llvm::Constant* CreateBitCast(llvm::Constant* C, llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreateCast(llvm::Instruction::BitCast, C, DestTy);
        }

        inline llvm::Constant* CreateIntToPtr(llvm::Constant* C, llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreateCast(llvm::Instruction::IntToPtr, C, DestTy);
        }

        inline llvm::Constant* CreatePtrToInt(llvm::Constant* C, llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreateCast(llvm::Instruction::PtrToInt, C, DestTy);
        }

        inline llvm::Constant* CreateZExtOrBitCast(llvm::Constant* C, llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreateZExtOrBitCast(C, DestTy);
        }

        inline llvm::Constant* CreateSExtOrBitCast(llvm::Constant* C, llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreateSExtOrBitCast(C, DestTy);
        }

        inline llvm::Constant* CreateTruncOrBitCast(llvm::Constant* C, llvm::Type* DestTy) const override {
            return m_baseConstantFolder.CreateTruncOrBitCast(C, DestTy);
        }

        inline llvm::Constant* CreateICmp(llvm::CmpInst::Predicate P, llvm::Constant* LHS,
            llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateICmp(P, LHS, RHS);
        }

        inline llvm::Constant* CreateFCmp(llvm::CmpInst::Predicate P, llvm::Constant* LHS,
            llvm::Constant* RHS) const override {
            return m_baseConstantFolder.CreateFCmp(P, LHS, RHS);
        }

        inline llvm::Constant* CreateSelect(llvm::Constant* C, llvm::Constant* True,
            llvm::Constant* False) const override {
            return m_baseConstantFolder.CreateSelect(C, True, False);
        }

        inline llvm::Constant* CreateExtractElement(llvm::Constant* Vec, llvm::Constant* Idx) const override {
            return m_baseConstantFolder.CreateExtractElement(Vec, Idx);
        }

        inline llvm::Constant* CreateInsertElement(llvm::Constant* Vec, llvm::Constant* NewElt,
            llvm::Constant* Idx) const override {
            return m_baseConstantFolder.CreateInsertElement(Vec, NewElt, Idx);
        }

        inline llvm::Constant* CreateShuffleVector(llvm::Constant* V1, llvm::Constant* V2,
            llvm::ArrayRef<int> Mask) const override {
            return m_baseConstantFolder.CreateShuffleVector(V1, V2, Mask);
        }

        inline llvm::Constant* CreateExtractValue(llvm::Constant* Agg,
            llvm::ArrayRef<unsigned> IdxList) const override {
            return m_baseConstantFolder.CreateExtractValue(Agg, IdxList);
        }

        inline llvm::Constant* CreateInsertValue(llvm::Constant* Agg, llvm::Constant* Val,
            llvm::ArrayRef<unsigned> IdxList) const override {
            return m_baseConstantFolder.CreateInsertValue(Agg, Val, IdxList);
        }
    };
#endif
} // namespace IGCLLVM

#endif // IGCLLVM_IR_CONSTANTFOLDER_H
