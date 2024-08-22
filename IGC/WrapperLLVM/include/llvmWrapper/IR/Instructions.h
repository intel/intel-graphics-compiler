/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INSTRUCTIONS_H
#define IGCLLVM_IR_INSTRUCTIONS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/User.h"

#if LLVM_VERSION_MAJOR < 11
#include "llvm/Analysis/OrderedBasicBlock.h"
#endif

#if LLVM_VERSION_MAJOR < 14
#include "DerivedTypes.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#endif

#include "Probe/Assertion.h"

namespace
{
#if LLVM_VERSION_MAJOR < 14
// The following is ported from LLVM 14 Instructions.cpp.
static bool isSingleSourceMaskImpl(llvm::ArrayRef<int> Mask, int NumOpElts)
{
  assert(!Mask.empty() && "Shuffle mask must contain elements");
  bool UsesLHS = false;
  bool UsesRHS = false;
  for (int I : Mask) {
    if (I == -1)
      continue;
    assert(I >= 0 && I < (NumOpElts * 2) &&
           "Out-of-bounds shuffle mask element");
    UsesLHS |= (I < NumOpElts);
    UsesRHS |= (I >= NumOpElts);
    if (UsesLHS && UsesRHS)
      return false;
  }
  // Allow for degenerate case: completely undef mask means neither source is used.
  return UsesLHS || UsesRHS;
}

static bool isIdentityMaskImpl(llvm::ArrayRef<int> Mask, int NumOpElts) {
  if (!isSingleSourceMaskImpl(Mask, NumOpElts))
    return false;
  for (int i = 0, NumMaskElts = (int) Mask.size(); i < NumMaskElts; ++i) {
    if (Mask[i] == -1)
      continue;
    if (Mask[i] != i && Mask[i] != (NumOpElts + i))
      return false;
  }
  return true;
}

static bool isInsertSubvectorMaskImpl(llvm::ShuffleVectorInst *SVI, int &NumSubElts, int &Index)
{
    if (IGCLLVM::isScalable(*SVI->getType()))
        return false;

    llvm::SmallVector<int, 32> Mask;
    SVI->getShuffleMask(Mask);

    int NumSrcElts =
        (int)llvm::cast<IGCLLVM::FixedVectorType>(SVI->getOperand(0)->getType())->getNumElements();
    int NumMaskElts = (int)Mask.size();

    // Don't try to match if we're shuffling to a smaller size.
    if (NumMaskElts < NumSrcElts)
        return false;

    // TODO: We don't recognize self-insertion/widening.
    if (isSingleSourceMaskImpl(Mask, NumSrcElts))
        return false;

    // Determine which mask elements are attributed to which source.
    llvm::APInt UndefElts(NumMaskElts, 0);
    llvm::APInt Src0Elts(NumMaskElts, 0);
    llvm::APInt Src1Elts(NumMaskElts, 0);
    bool Src0Identity = true;
    bool Src1Identity = true;

    for (int i = 0; i != NumMaskElts; ++i) {
        int M = Mask[i];
        if (M < 0) {
            UndefElts.setBit(i);
            continue;
        }
        if (M < NumSrcElts) {
            Src0Elts.setBit(i);
            Src0Identity &= (M == i);
            continue;
        }
        Src1Elts.setBit(i);
        Src1Identity &= (M == (i + NumSrcElts));
    }
    assert((Src0Elts | Src1Elts | UndefElts).isAllOnesValue() &&
            "unknown shuffle elements");
    assert(!Src0Elts.isNullValue() && !Src1Elts.isNullValue() &&
            "2-source shuffle not found");

    // Determine lo/hi span ranges.
    // TODO: How should we handle undefs at the start of subvector insertions?
    int Src0Lo = Src0Elts.countTrailingZeros();
    int Src1Lo = Src1Elts.countTrailingZeros();
    int Src0Hi = NumMaskElts - Src0Elts.countLeadingZeros();
    int Src1Hi = NumMaskElts - Src1Elts.countLeadingZeros();

    // If src0 is in place, see if the src1 elements is inplace within its own
    // span.
    if (Src0Identity) {
        int NumSub1Elts = Src1Hi - Src1Lo;
        llvm::SmallVector<int, 32> Sub1Mask;
        for (int i = 0; i < NumSub1Elts; i++)
            Sub1Mask.push_back(Mask[i + Src1Lo]);
        if (isIdentityMaskImpl(Sub1Mask, NumSrcElts)) {
            NumSubElts = NumSub1Elts;
            Index = Src1Lo;
            return true;
        }
    }

    // If src1 is in place, see if the src0 elements is inplace within its own
    // span.
    if (Src1Identity) {
        int NumSub0Elts = Src0Hi - Src0Lo;
        llvm::SmallVector<int, 32> Sub0Mask;
        for (int i = 0; i < NumSub0Elts; i++)
            Sub0Mask.push_back(Mask[i + Src0Lo]);
        if (isIdentityMaskImpl(Sub0Mask, NumSrcElts)) {
            NumSubElts = NumSub0Elts;
            Index = Src0Lo;
            return true;
        }
    }

    return false;
}
#endif // LLVM_VERSION_MAJOR < 14
} // end anonymous namespace

namespace IGCLLVM
{

inline llvm::Value* getCalledValue(llvm::CallInst& CI)
{
#if LLVM_VERSION_MAJOR <= 10
    return CI.getCalledValue();
#else
    return CI.getCalledOperand();
#endif
}

inline llvm::Value* getCalledValue(llvm::CallInst* CI)
{
#if LLVM_VERSION_MAJOR <= 10
    return CI->getCalledValue();
#else
    return CI->getCalledOperand();
#endif
}

inline const llvm::Value* getCalledValue(const llvm::CallInst* CI)
{
#if LLVM_VERSION_MAJOR <= 10
    return CI->getCalledValue();
#else
    return CI->getCalledOperand();
#endif
}

inline unsigned getNumArgOperands(const llvm::CallInst* CI)
{
#if LLVM_VERSION_MAJOR < 14
    return CI->getNumArgOperands();
#else
    return CI->arg_size();
#endif
}

inline unsigned getArgOperandNo(llvm::CallInst &CI, const llvm::Use *U) {
#if LLVM_VERSION_MAJOR < 10
    IGC_ASSERT_MESSAGE(CI.isArgOperand(U), "Arg operand # out of range!");
    return (unsigned)(U - CI.arg_begin());
#else
    return CI.getArgOperandNo(U);
#endif
}

inline llvm::Constant* getShuffleMaskForBitcode(llvm::ShuffleVectorInst* SVI)
{
#if LLVM_VERSION_MAJOR < 11
    return SVI->getMask();
#else
    return llvm::ShuffleVectorInst::convertShuffleMaskForBitcode(SVI->getShuffleMask(), SVI->getType());
#endif
}

inline bool isInsertSubvectorMask(llvm::ShuffleVectorInst *SVI, int &NumSubElts, int &Index)
{
#if LLVM_VERSION_MAJOR < 14
    return isInsertSubvectorMaskImpl(SVI, NumSubElts, Index);
#else
    return SVI->isInsertSubvectorMask(NumSubElts, Index);
#endif
}

inline bool isFreezeInst(llvm::Instruction* I)
{
#if LLVM_VERSION_MAJOR < 10
    (void)I;
    return false;
#else
    return llvm::isa<llvm::FreezeInst>(I);
#endif
}

inline bool isDebugOrPseudoInst(llvm::Instruction& I)
{
#if LLVM_VERSION_MAJOR < 14
    return llvm::isa<llvm::DbgInfoIntrinsic>(&I);
#else
    return I.isDebugOrPseudoInst();
#endif
}

inline bool comesBefore(llvm::Instruction* A, llvm::Instruction* B)
{
#if LLVM_VERSION_MAJOR < 11
    return llvm::OrderedBasicBlock(A->getParent()).dominates(A, B);
#else
    return A->comesBefore(B);
#endif
}

inline llvm::Type *getGEPIndexedType(llvm::Type* Ty, llvm::SmallVectorImpl<unsigned>& indices) {
    llvm::SmallVector< llvm::Value*, 8> gepIndices;
    gepIndices.reserve(indices.size() + 1);
    auto* int32Ty = llvm::IntegerType::getInt32Ty(Ty->getContext());
    gepIndices.push_back(llvm::ConstantInt::get(int32Ty, 0));
    for (unsigned idx : indices) {
        gepIndices.push_back(llvm::ConstantInt::get(int32Ty, idx));
    }
    return llvm::GetElementPtrInst::getIndexedType(Ty, gepIndices);
}

inline llvm::Type* getGEPIndexedType(llvm::Type* Ty, llvm::ArrayRef<llvm::Value*> indices) {
    return llvm::GetElementPtrInst::getIndexedType(Ty, indices);
}

}

#endif
