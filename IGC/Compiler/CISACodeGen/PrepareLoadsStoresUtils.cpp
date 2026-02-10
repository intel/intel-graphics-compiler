/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// These functions convert load and store instructions that have 64-bit
/// elements (whether scalar or vector) to vectors of dwords.
///
/// This is used by PrepareLoadsStoresPass to run prior to MemOpt to aid
/// vectorization of values with different types.
///
/// For example:
///
/// %x = load i64, i64 addrspace(1)* %"&x"
/// ===>
/// %7 = bitcast i64 addrspace(1)* %"&x" to <2 x i32> addrspace(1)*
/// %8 = load <2 x i32>, <2 x i32> addrspace(1)* %7
/// %9 = bitcast <2 x i32> %8 to i64
///
/// Example with PredicatedLoad:
///
/// %x = call i64 @llvm.genx.GenISA.PredicatedLoad.i64.p1i64.i64(i64 addrspace(1)* %"&x", i64 8, i1 %p, i64 %mv)
/// ===>
/// %7 = bitcast i64 addrspace(1)* %"&x" to <2 x i32> addrspace(1)*
/// %8 = bitcast i64 %mv to <2 x i32>
/// %9 = call <2 x i32> @llvm.genx.GenISA.PredicatedLoad.v2i32.p1v2i32.v2i32(<2 x i32> addrspace(1)* %7, i64 8, i1 %p,
/// <2 x i32> %8) %10 = bitcast <2 x i32> %9 to i64
//===----------------------------------------------------------------------===//

#include "PrepareLoadsStoresUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/NoFolder.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;
using namespace IGC;
using IGCLLVM::getAlign;

template <typename T> static Value *getIntToPtr(IRBuilder<T> &IRB, Value *NewVal, Type *Ty) {
  // We must emit scalar inttoptr this late in compilation otherwise it won't
  // be handled correctly (by e.g., Emu64Ops).

  if (NewVal->getType()->isIntegerTy())
    return IRB.CreateIntToPtr(NewVal, Ty);

  // otherwise, 'NewVal' and 'Ty' are both vectors and will have the same
  // length.
  IGCLLVM::FixedVectorType *newVecType = dyn_cast<IGCLLVM::FixedVectorType>(NewVal->getType());
  IGCLLVM::FixedVectorType *vecType = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
  uint64_t NumElts = newVecType->getNumElements();
  IGC_ASSERT(NumElts == vecType->getNumElements());
  Value *ResultVec = UndefValue::get(vecType);
  auto *ResEltTy = vecType->getElementType();
  for (uint32_t i = 0; i < NumElts; i++) {
    auto *Elt = IRB.CreateExtractElement(NewVal, i);
    auto *Cast = IRB.CreateIntToPtr(Elt, ResEltTy);
    ResultVec = IRB.CreateInsertElement(ResultVec, Cast, i);
  }

  return ResultVec;
}

template <typename T> static Value *getPtrToInt(IRBuilder<T> &IRB, Value *NewVal, Type *Ty) {
  // We must emit scalar ptrtoint this late in compilation otherwise it won't
  // be handled correctly (by e.g., Emu64Ops).

  if (NewVal->getType()->isPointerTy())
    return IRB.CreatePtrToInt(NewVal, Ty);

  // otherwise, 'NewVal' and 'Ty' are both vectors and will have the same
  // length.
  IGCLLVM::FixedVectorType *newVecType = dyn_cast<IGCLLVM::FixedVectorType>(NewVal->getType());
  IGCLLVM::FixedVectorType *vecType = dyn_cast<IGCLLVM::FixedVectorType>(Ty);
  uint64_t NumElts = newVecType->getNumElements();
  IGC_ASSERT(NumElts == vecType->getNumElements());
  Value *ResultVec = UndefValue::get(Ty);
  auto *ResEltTy = vecType->getElementType();
  for (uint32_t i = 0; i < NumElts; i++) {
    auto *Elt = IRB.CreateExtractElement(NewVal, i);
    auto *Cast = IRB.CreatePtrToInt(Elt, ResEltTy);
    ResultVec = IRB.CreateInsertElement(ResultVec, Cast, i);
  }

  return ResultVec;
}

namespace IGC {

template <typename T>
std::pair<Value *, Instruction *> expand64BitLoad(IGCIRBuilder<T> &IRB, const DataLayout &DL, ALoadInst LI) {
  auto *Ty = LI.getType();
  if (Ty->isAggregateType())
    return {};

  auto *EltTy = Ty->getScalarType();
  auto *OldPtrTy = LI.getPointerOperandType();
  uint64_t NumEltBytes = DL.getTypeStoreSize(EltTy);

  if (NumEltBytes != 8)
    return {};

  uint32_t NumElts =
      isa<IGCLLVM::FixedVectorType>(Ty) ? (uint32_t)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() : 1;

  uint32_t NewNumElts = NumElts * 2;

  auto *NewTy = IGCLLVM::FixedVectorType::get(IRB.getInt32Ty(), NewNumElts);
  auto *PtrTy = PointerType::get(NewTy, OldPtrTy->getPointerAddressSpace());

  auto *NewPtr = IRB.CreateBitCast(LI.getPointerOperand(), PtrTy);

  Value *MVal = nullptr;
  if (auto *PLI = LI.getPredicatedLoadIntrinsic())
    MVal = IRB.CreateBitCast(PLI->getMergeValue(), NewTy);

  Instruction *NewLI = LI.CreateAlignedLoad(IRB, NewTy, NewPtr, MVal);
  NewLI->copyMetadata(*LI.inst());
  Value *NewVal = NewLI;

  if (Ty->isPtrOrPtrVectorTy()) {
    Type *NewTy =
        (NumElts == 1) ? (Type *)IRB.getInt64Ty() : (Type *)IGCLLVM::FixedVectorType::get(IRB.getInt64Ty(), NumElts);
    NewVal = IRB.CreateBitCast(NewVal, NewTy);
    NewVal = getIntToPtr(IRB, NewVal, Ty);
  } else {
    NewVal = IRB.CreateBitCast(NewVal, Ty);
  }

  return {NewVal, NewLI};
}

template <typename T>
std::pair<Value *, Instruction *> expand64BitLoad(IGCIRBuilder<T> &IRB, const DataLayout &DL, LoadInst *LI) {
  return expand64BitLoad(IRB, DL, ALoadInst::get(LI).value());
}

template <typename T> Instruction *expand64BitStore(IGCIRBuilder<T> &IRB, const DataLayout &DL, AStoreInst SI) {
  auto *Ty = SI.getValueOperand()->getType();
  if (Ty->isAggregateType())
    return nullptr;

  auto *EltTy = Ty->getScalarType();
  auto *OldPtrTy = SI.getPointerOperandType();
  uint64_t NumEltBytes = DL.getTypeStoreSize(EltTy);

  if (NumEltBytes != 8)
    return nullptr;

  uint32_t NumElts =
      isa<IGCLLVM::FixedVectorType>(Ty) ? (uint32_t)cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements() : 1;

  uint32_t NewNumElts = NumElts * 2;

  auto *NewTy = IGCLLVM::FixedVectorType::get(IRB.getInt32Ty(), NewNumElts);

  Value *NewVal = SI.getValueOperand();
  if (Ty->isPtrOrPtrVectorTy()) {
    Type *NewTy =
        (NumElts == 1) ? (Type *)IRB.getInt64Ty() : (Type *)IGCLLVM::FixedVectorType::get(IRB.getInt64Ty(), NumElts);
    NewVal = getPtrToInt(IRB, NewVal, NewTy);
  }
  NewVal = IRB.CreateBitCast(NewVal, NewTy);
  auto *NewPtr =
      IRB.CreateBitCast(SI.getPointerOperand(), NewVal->getType()->getPointerTo(OldPtrTy->getPointerAddressSpace()));
  auto *NewST = SI.CreateAlignedStore(IRB, NewVal, NewPtr);
  NewST->copyMetadata(*SI.inst());

  return NewST;
}

template <typename T> Instruction *expand64BitStore(IGCIRBuilder<T> &IRB, const DataLayout &DL, StoreInst *SI) {
  return expand64BitStore(IRB, DL, AStoreInst::get(SI).value());
}

template std::pair<Value *, Instruction *> expand64BitLoad(IGCIRBuilder<> &IRB, const DataLayout &DL, ALoadInst LI);

template std::pair<Value *, Instruction *> expand64BitLoad(IGCIRBuilder<NoFolder> &IRB, const DataLayout &DL,
                                                           ALoadInst LI);

template std::pair<Value *, Instruction *> expand64BitLoad(IGCIRBuilder<> &IRB, const DataLayout &DL, LoadInst *LI);

template std::pair<Value *, Instruction *> expand64BitLoad(IGCIRBuilder<NoFolder> &IRB, const DataLayout &DL,
                                                           LoadInst *LI);

template Instruction *expand64BitStore(IGCIRBuilder<> &IRB, const DataLayout &DL, AStoreInst SI);

template Instruction *expand64BitStore(IGCIRBuilder<NoFolder> &IRB, const DataLayout &DL, AStoreInst SI);

template Instruction *expand64BitStore(IGCIRBuilder<> &IRB, const DataLayout &DL, StoreInst *SI);

template Instruction *expand64BitStore(IGCIRBuilder<NoFolder> &IRB, const DataLayout &DL, StoreInst *SI);
} // namespace IGC
