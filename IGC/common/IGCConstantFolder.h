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

#ifndef IGC_CONSTANT_FOLDER_H
#define IGC_CONSTANT_FOLDER_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/ConstantFolder.h"
#include "llvmWrapper/IR/Instructions.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

class IGCConstantFolder
{
private:
    llvm::ConstantFolder m_baseConstantFolder;
public:
    IGCConstantFolder();

    llvm::Constant* CreateGradientXFine(llvm::Constant* C0) const;
    llvm::Constant* CreateGradientYFine(llvm::Constant* C0) const;
    llvm::Constant* CreateGradientX(llvm::Constant* C0) const;
    llvm::Constant* CreateGradientY(llvm::Constant* C0) const;
    llvm::Constant* CreateRsq(llvm::Constant* C0) const;
    llvm::Constant* CreateRoundNE(llvm::Constant* C0) const;
    llvm::Constant* CreateFSat(llvm::Constant* C0) const;
    llvm::Constant* CreateFAdd(llvm::Constant* C0, llvm::Constant* C1, llvm::APFloatBase::roundingMode roundingMode) const;
    llvm::Constant* CreateFMul(llvm::Constant* C0, llvm::Constant* C1, llvm::APFloatBase::roundingMode roundingMode) const;
    llvm::Constant* CreateFPTrunc(llvm::Constant* C0, llvm::Type* dstType, llvm::APFloatBase::roundingMode roundingMode) const;
    llvm::Constant* CreateCanonicalize(llvm::Constant* C0, bool flushDenorms = true) const;


    //===-------------
    // FIXME:
    //   Previously IGCConstantFolder was inheriting them from llvm::ConstantFolder,
    //   now the methods below are being proxied to llvm::ConstantFolder instance.
    //   This was done because of a change in llvm::ConstantFolder class,
    //   which made it final and no longer possible to inherit from.
    //   A better implementation would require major code changes thoughout IGC
    //   where IGCConstantFolder is used.

    inline llvm::Constant* CreateAdd(llvm::Constant* LHS, llvm::Constant* RHS,
        bool HasNUW = false, bool HasNSW = false) const {
        return m_baseConstantFolder.CreateAdd(LHS, RHS, HasNUW, HasNSW);
    }

    inline llvm::Constant* CreateFAdd(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateFAdd(LHS, RHS);
    }

    inline llvm::Constant* CreateSub(llvm::Constant* LHS, llvm::Constant* RHS,
        bool HasNUW = false, bool HasNSW = false) const {
        return m_baseConstantFolder.CreateSub(LHS, RHS, HasNUW, HasNSW);
    }

    inline llvm::Constant* CreateFSub(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateFSub(LHS, RHS);
    }

    inline llvm::Constant* CreateMul(llvm::Constant* LHS, llvm::Constant* RHS,
        bool HasNUW = false, bool HasNSW = false) const {
        return m_baseConstantFolder.CreateMul(LHS, RHS, HasNUW, HasNSW);
    }

    inline llvm::Constant* CreateFMul(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateFMul(LHS, RHS);
    }

    inline llvm::Constant* CreateUDiv(llvm::Constant* LHS, llvm::Constant* RHS,
        bool isExact = false) const {
        return m_baseConstantFolder.CreateUDiv(LHS, RHS, isExact);
    }

    inline llvm::Constant* CreateSDiv(llvm::Constant* LHS, llvm::Constant* RHS,
        bool isExact = false) const {
        return m_baseConstantFolder.CreateSDiv(LHS, RHS, isExact);
    }

    inline llvm::Constant* CreateFDiv(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateFDiv(LHS, RHS);
    }

    inline llvm::Constant* CreateURem(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateURem(LHS, RHS);
    }

    inline llvm::Constant* CreateSRem(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateSRem(LHS, RHS);
    }

    inline llvm::Constant* CreateFRem(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateFRem(LHS, RHS);
    }

    inline llvm::Constant* CreateShl(llvm::Constant* LHS, llvm::Constant* RHS,
        bool HasNUW = false, bool HasNSW = false) const {
        return m_baseConstantFolder.CreateShl(LHS, RHS, HasNUW, HasNSW);
    }

    inline llvm::Constant* CreateLShr(llvm::Constant* LHS, llvm::Constant* RHS,
        bool isExact = false) const {
        return m_baseConstantFolder.CreateLShr(LHS, RHS, isExact);
    }

    inline llvm::Constant* CreateAShr(llvm::Constant* LHS, llvm::Constant* RHS,
        bool isExact = false) const {
        return m_baseConstantFolder.CreateAShr(LHS, RHS, isExact);
    }

    inline llvm::Constant* CreateAnd(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateAnd(LHS, RHS);
    }

    inline llvm::Constant* CreateOr(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateOr(LHS, RHS);
    }

    inline llvm::Constant* CreateXor(llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateXor(LHS, RHS);
    }

    inline llvm::Constant* CreateBinOp(llvm::Instruction::BinaryOps Opc,
        llvm::Constant* LHS, llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateBinOp(Opc, LHS, RHS);
    }

    inline llvm::Constant* CreateNeg(llvm::Constant* C,
        bool HasNUW = false, bool HasNSW = false) const {
        return m_baseConstantFolder.CreateNeg(C, HasNUW, HasNSW);
    }

    inline llvm::Constant* CreateFNeg(llvm::Constant* C) const {
        return m_baseConstantFolder.CreateFNeg(C);
    }

    inline llvm::Constant* CreateNot(llvm::Constant* C) const {
        return m_baseConstantFolder.CreateNot(C);
    }

#if LLVM_VERSION_MAJOR > 7
    inline llvm::Constant* CreateUnOp(llvm::Instruction::UnaryOps Opc, llvm::Constant* C) const {
        return m_baseConstantFolder.CreateUnOp(Opc, C);
    }

#endif
    inline llvm::Constant* CreateGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
        llvm::ArrayRef<llvm::Constant*> IdxList) const {
        return m_baseConstantFolder.CreateGetElementPtr(Ty, C, IdxList);
    }

    inline llvm::Constant* CreateGetElementPtr(llvm::Type* Ty, llvm::Constant* C, llvm::Constant* Idx) const {
        return m_baseConstantFolder.CreateGetElementPtr(Ty, C, Idx);
    }

    inline llvm::Constant* CreateGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
        llvm::ArrayRef<llvm::Value*> IdxList) const {
        return m_baseConstantFolder.CreateGetElementPtr(Ty, C, IdxList);
    }

    inline llvm::Constant* CreateInBoundsGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
        llvm::ArrayRef<llvm::Constant*> IdxList) const {
        return m_baseConstantFolder.CreateInBoundsGetElementPtr(Ty, C, IdxList);
    }

    inline llvm::Constant* CreateInBoundsGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
        llvm::Constant* Idx) const {
        return m_baseConstantFolder.CreateInBoundsGetElementPtr(Ty, C, Idx);
    }

    inline llvm::Constant* CreateInBoundsGetElementPtr(llvm::Type* Ty, llvm::Constant* C,
        llvm::ArrayRef<llvm::Value*> IdxList) const {
        return m_baseConstantFolder.CreateInBoundsGetElementPtr(Ty, C, IdxList);
    }

    inline llvm::Constant* CreateCast(llvm::Instruction::CastOps Op, llvm::Constant* C,
        llvm::Type* DestTy) const {
        return m_baseConstantFolder.CreateCast(Op, C, DestTy);
    }

    inline llvm::Constant* CreatePointerCast(llvm::Constant* C, llvm::Type* DestTy) const {
        return m_baseConstantFolder.CreatePointerCast(C, DestTy);
    }

    inline llvm::Constant* CreatePointerBitCastOrAddrSpaceCast(llvm::Constant* C,
        llvm::Type* DestTy) const {
        return m_baseConstantFolder.CreatePointerBitCastOrAddrSpaceCast(C, DestTy);
    }

    inline llvm::Constant* CreateIntCast(llvm::Constant* C, llvm::Type* DestTy,
        bool isSigned) const {
        return m_baseConstantFolder.CreateIntCast(C, DestTy, isSigned);
    }

    inline llvm::Constant* CreateFPCast(llvm::Constant* C, llvm::Type* DestTy) const {
        return m_baseConstantFolder.CreateFPCast(C, DestTy);
    }

    inline llvm::Constant* CreateBitCast(llvm::Constant* C, llvm::Type* DestTy) const {
        return CreateCast(llvm::Instruction::BitCast, C, DestTy);
    }

    inline llvm::Constant* CreateIntToPtr(llvm::Constant* C, llvm::Type* DestTy) const {
        return CreateCast(llvm::Instruction::IntToPtr, C, DestTy);
    }

    inline llvm::Constant* CreatePtrToInt(llvm::Constant* C, llvm::Type* DestTy) const {
        return CreateCast(llvm::Instruction::PtrToInt, C, DestTy);
    }

    inline llvm::Constant* CreateZExtOrBitCast(llvm::Constant* C, llvm::Type* DestTy) const {
        return m_baseConstantFolder.CreateZExtOrBitCast(C, DestTy);
    }

    inline llvm::Constant* CreateSExtOrBitCast(llvm::Constant* C, llvm::Type* DestTy) const {
        return m_baseConstantFolder.CreateSExtOrBitCast(C, DestTy);
    }

    inline llvm::Constant* CreateTruncOrBitCast(llvm::Constant* C, llvm::Type* DestTy) const {
        return m_baseConstantFolder.CreateTruncOrBitCast(C, DestTy);
    }

    inline llvm::Constant* CreateICmp(llvm::CmpInst::Predicate P, llvm::Constant* LHS,
        llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateICmp(P, LHS, RHS);
    }

    inline llvm::Constant* CreateFCmp(llvm::CmpInst::Predicate P, llvm::Constant* LHS,
        llvm::Constant* RHS) const {
        return m_baseConstantFolder.CreateFCmp(P, LHS, RHS);
    }

    inline llvm::Constant* CreateSelect(llvm::Constant* C, llvm::Constant* True, llvm::Constant* False) const {
        return m_baseConstantFolder.CreateSelect(C, True, False);
    }

    inline llvm::Constant* CreateExtractElement(llvm::Constant* Vec, llvm::Constant* Idx) const {
        return m_baseConstantFolder.CreateExtractElement(Vec, Idx);
    }

    inline llvm::Constant* CreateInsertElement(llvm::Constant* Vec, llvm::Constant* NewElt,
        llvm::Constant* Idx) const {
        return m_baseConstantFolder.CreateInsertElement(Vec, NewElt, Idx);
    }

#if LLVM_VERSION_MAJOR < 11
    inline llvm::Constant* CreateShuffleVector(llvm::Constant* V1, llvm::Constant* V2,
        llvm::Constant* Mask) const {

        //===-------------
        // FIXME:
        //   To adhere to LLVM 11 practices the cast from Constant* to ArrayRef<int>
        //   should happen before this function's calls and ArrayRef<int> should be
        //   passed as the argument.

#if LLVM_VERSION_MAJOR < 11
        return m_baseConstantFolder.CreateShuffleVector(V1, V2, Mask);
#else
        using namespace llvm;

        ShuffleVectorInst* shuffleVector = cast<ShuffleVectorInst>(m_baseConstantFolder.CreateShuffleVector(V1, V2, /* Dummy mask */ { 0 } ));

        SmallVector<int, 16> maskArr;
        shuffleVector->getShuffleMask(cast<Constant>(Mask), maskArr);
        shuffleVector->setShuffleMask(maskArr);

        return cast<Constant>(shuffleVector);
#endif
    }
#else
    inline llvm::Constant* CreateShuffleVector(llvm::Constant* V1, llvm::Constant* V2,
        llvm::ArrayRef<int> Mask) const {
        return m_baseConstantFolder.CreateShuffleVector(V1, V2, Mask);
    }
#endif

    inline llvm::Constant* CreateExtractValue(llvm::Constant* Agg,
        llvm::ArrayRef<unsigned> IdxList) const {
        return m_baseConstantFolder.CreateExtractValue(Agg, IdxList);
    }

    inline llvm::Constant* CreateInsertValue(llvm::Constant* Agg, llvm::Constant* Val,
        llvm::ArrayRef<unsigned> IdxList) const {
        return m_baseConstantFolder.CreateInsertValue(Agg, Val, IdxList);
    }


private:

    llvm::Constant* CreateGradient(llvm::Constant* C0) const;

};

}

#endif // IGC_CONSTANT_FOLDER_H
