/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// This file defines a routine for folding a GenX intrinsic call into a
// constant.
//
//===----------------------------------------------------------------------===//

#include "vc/GenXOpts/GenXAnalysis.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"
#include "vc/Utils/GenX/Region.h"

#include "Probe/Assertion.h"

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Debug.h"

#include "llvmWrapper/Analysis/CallGraph.h"
#include "llvmWrapper/IR/CallSite.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Instructions.h"
#include "llvmWrapper/Support/TypeSize.h"

#define DEBUG_TYPE "genx-constantfolding"

using namespace llvm;

/***********************************************************************
 * canConstantFoldGenXIntrinsic : Return true if it is even possible to fold
 *     a call to the specified GenX intrinsic
 */
bool llvm::canConstantFoldGenXIntrinsic(unsigned IID) {
  switch (IID) {
  case GenXIntrinsic::genx_rdregioni:
  case GenXIntrinsic::genx_rdregionf:
  // The wrregion case specifically excludes genx_wrconstregion
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_wrregionf:
  case GenXIntrinsic::genx_all:
  case GenXIntrinsic::genx_any:
    return true;
  }
  return false;
}

/***********************************************************************
 * constantFoldRdRegion : attempt to constant fold rdregion
 */
static Constant *constantFoldRdRegion(Type *RetTy,
                                      ArrayRef<Constant *> Operands,
                                      const vc::CMRegion &R,
                                      const DataLayout &DL) {
  Constant *Input = Operands[GenXIntrinsic::GenXRegion::OldValueOperandNum];
  // The input can be a ConstantExpr if we are being called from
  // CallAnalyzer.
  if (isa<ConstantExpr>(Input))
    return nullptr;
  // If the input value is undef, just return undef.
  if (isa<UndefValue>(Input))
    return UndefValue::get(RetTy);
  // Parse the region parameters.
  unsigned WholeNumElements =
    cast<IGCLLVM::FixedVectorType>(Input->getType())->getNumElements();
  auto OffsetC = dyn_cast<Constant>(
      Operands[GenXIntrinsic::GenXRegion::RdIndexOperandNum]);
  if (!OffsetC)
    return nullptr;

  const int RetElemSize = DL.getTypeSizeInBits(RetTy->getScalarType()) / 8;
  unsigned Offset = 0;
  if (!isa<VectorType>(OffsetC->getType()))
    Offset = cast<ConstantInt>(OffsetC)->getZExtValue() / RetElemSize;
  else
    IGC_ASSERT(dyn_cast<IGCLLVM::FixedVectorType>(OffsetC->getType())
                   ->getNumElements() == R.NumElements);
  if (Offset >= WholeNumElements)
    return UndefValue::get(RetTy); // out of range index
  if (!isa<VectorType>(RetTy))
    return Input->getAggregateElement(Offset);
  // Gather the elements of the region being read.
  SmallVector<Constant *, 8> Values;
  unsigned RowIdx = Offset;
  unsigned Idx = RowIdx;
  unsigned NextRow = R.Width;
  for (unsigned i = 0; i != R.NumElements; ++i) {
    if (i == NextRow) {
      NextRow += R.Width;
      RowIdx += R.VStride;
      Idx = RowIdx;
    }
    if (isa<VectorType>(OffsetC->getType()) &&
        isa<ConstantInt>(OffsetC->getAggregateElement(i))) {
      auto EltOffset =
          cast<ConstantInt>(OffsetC->getAggregateElement(i))->getZExtValue();
      EltOffset =
          EltOffset / (DL.getTypeSizeInBits(RetTy->getScalarType()) / 8);
      Idx += EltOffset;
    }
    if (Idx >= WholeNumElements)
      // push undef value if idx is out of bounds
      Values.push_back(UndefValue::get(RetTy->getScalarType()));
    else
      // Get the element value and push it into Values.
      Values.push_back(Input->getAggregateElement(Idx));
    Idx += R.Stride;
  }
  return ConstantVector::get(Values);
}

/***********************************************************************
 * constantFoldWrRegion : attempt to constant fold Wrregion
 */
static Constant *constantFoldWrRegion(Type *RetTy,
                                      ArrayRef<Constant *> Operands,
                                      const vc::CMRegion &R,
                                      const DataLayout &DL) {
  Constant *OldValue = Operands[GenXIntrinsic::GenXRegion::OldValueOperandNum];
  Constant *NewValue = Operands[GenXIntrinsic::GenXRegion::NewValueOperandNum];
  Constant *Mask = Operands[GenXIntrinsic::GenXRegion::PredicateOperandNum];
  // The inputs can be ConstantExpr if we are being called from
  // CallAnalyzer.
  if (isa<ConstantExpr>(OldValue) || isa<ConstantExpr>(NewValue))
    return nullptr;
  IGC_ASSERT(RetTy == OldValue->getType());
  auto OffsetC = dyn_cast<ConstantInt>(
      Operands[GenXIntrinsic::GenXRegion::WrIndexOperandNum]);
  if (!OffsetC)
    return nullptr; // allow for but do not const fold when index is vector

  const int RetElemSize = DL.getTypeSizeInBits(RetTy->getScalarType()) / 8;
  unsigned Offset = OffsetC->getSExtValue() / RetElemSize;
  if (isa<UndefValue>(OldValue) && R.isContiguous() && Offset == 0 &&
      Mask->isAllOnesValue()) {
    // If old value is undef and new value is splat, and the result vector
    // is no bigger than 2 GRFs, then just return a splat of the right type.
    Constant *Splat = NewValue;
    if (isa<VectorType>(NewValue->getType()))
      Splat = NewValue->getSplatValue();
    if (Splat)
      if (DL.getTypeSizeInBits(RetTy) <= 2 * 32 * 8)
        return ConstantVector::getSplat(
            IGCLLVM::getElementCount(
                cast<IGCLLVM::FixedVectorType>(RetTy)->getNumElements()),
            Splat);
    // If new value fills the whole vector, just return the new value.
    if (NewValue->getType() == RetTy)
      return NewValue;
  }
  unsigned WholeNumElements =
      cast<IGCLLVM::FixedVectorType>(RetTy)->getNumElements();
  // Gather the elements of the old value.
  SmallVector<Constant *, 8> Values;
  for (unsigned i = 0; i != WholeNumElements; ++i)
    Values.push_back(OldValue->getAggregateElement(i));
  // Insert the elements of the new value.
  if (Offset >= Values.size())
    return UndefValue::get(RetTy); // out of range index
  if (!isa<VectorType>(NewValue->getType()))
    Values[Offset] = NewValue;
  else if (!Mask->isZeroValue()) {
    unsigned RowIdx = Offset;
    unsigned Idx = RowIdx;
    unsigned NextRow = R.Width;
    for (unsigned i = 0; i != R.NumElements; ++i) {
      if (i == NextRow) {
        NextRow += R.Width;
        RowIdx += R.VStride;
        Idx = RowIdx;
      }
      if (Idx >= WholeNumElements)
        // return collected values even if idx is out of bounds
        return ConstantVector::get(Values);
      if (Mask->isAllOnesValue() ||
          (Mask->getType()->isVectorTy() &&
           !cast<ConstantVector>(Mask)->getAggregateElement(i)->isZeroValue()))
        Values[Idx] = NewValue->getAggregateElement(i);
      Idx += R.Stride;
    }
  }
  return ConstantVector::get(Values);
}

/***********************************************************************
 * constantFoldAll : constant fold llvm.genx.all
 * constantFoldAny : constant fold llvm.genx.any
 */
static Constant *constantFoldAll(Type *RetTy, Constant *In) {
  if (In->isAllOnesValue())
    return Constant::getAllOnesValue(RetTy);
  return Constant::getNullValue(RetTy);
}
static Constant *constantFoldAny(Type *RetTy, Constant *In) {
  if (!In->isNullValue())
    return Constant::getAllOnesValue(RetTy);
  return Constant::getNullValue(RetTy);
}

/***********************************************************************
 * ConstantFoldGenXIntrinsic : attempt to constant fold a call to the
 *    specified GenX intrinsic with the specified arguments, returning null if
 *    unsuccessful
 */
Constant *llvm::ConstantFoldGenXIntrinsic(unsigned IID, Type *RetTy,
                                          ArrayRef<Constant *> Operands,
                                          Instruction *CSInst,
                                          const DataLayout &DL) {
  switch (IID) {
  case GenXIntrinsic::genx_rdregioni:
  case GenXIntrinsic::genx_rdregionf: {
    vc::CMRegion R(CSInst);
    return constantFoldRdRegion(RetTy, Operands, R, DL);
  }
  // The wrregion case specifically excludes genx_wrconstregion
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_wrregionf: {
    vc::CMRegion R(CSInst);
    return constantFoldWrRegion(RetTy, Operands, R, DL);
  }
  case GenXIntrinsic::genx_all:
    return constantFoldAll(RetTy, Operands[0]);
  case GenXIntrinsic::genx_any:
    return constantFoldAny(RetTy, Operands[0]);
  }
  return nullptr;
}

/***********************************************************************
 * ConstantFoldGenX : attempt to constant fold genx intrinsics including
 * its arguments, returning null if unsuccessful.
 */
Constant *llvm::ConstantFoldGenX(Instruction *I, const DataLayout &DL) {
  LLVM_DEBUG(dbgs() << "Trying to fold " << *I << "\n");
  auto IID = GenXIntrinsic::getGenXIntrinsicID(I);
  if (!canConstantFoldGenXIntrinsic(IID)) {
    LLVM_DEBUG(dbgs() << "Fail: not a genx intrinsic\n");
    return nullptr;
  }

  auto &CS = *cast<CallInst>(I);

  auto CheckConst = [](const Use &A) {
    Value *V = A.get();
    bool IsConst = isa<Constant>(V);
    if (!IsConst)
      LLVM_DEBUG(dbgs() << "Fail: operand " << *V << " is not a constant\n");
    return IsConst;
  };
  if (!std::all_of(CS.arg_begin(), CS.arg_end(), CheckConst))
    return nullptr;

  SmallVector<Constant *, 4> ConstantArgs;
  ConstantArgs.reserve(IGCLLVM::getNumArgOperands(&CS));
  auto FoldOperand = [&DL](const Use &A) {
    auto *C = cast<Constant>(A.get());
    Constant *Folded = ConstantFoldConstant(C, DL);
    if (Folded)
      LLVM_DEBUG(dbgs() << "Folded operand " << *C << " to " << *Folded
                        << "\n");
    return Folded ? Folded : C;
  };
  std::transform(CS.arg_begin(), CS.arg_end(), std::back_inserter(ConstantArgs),
                 FoldOperand);

  Constant *Folded = ConstantFoldGenXIntrinsic(
      IID, CS.getFunctionType()->getReturnType(), ConstantArgs, I, DL);
  if (Folded)
    LLVM_DEBUG(dbgs() << "Successfully constant folded instruction to "
                      << *Folded << "\n");
  else
    LLVM_DEBUG(dbgs() << "Failed to constant fold instruction\n");
  return Folded;
}
