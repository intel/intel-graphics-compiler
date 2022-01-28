/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/IRBuilder.h"
#include "vc/Utils/General/Types.h"

#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/Operator.h"

using namespace llvm;

Value *vc::createNopPtrToInt(Value &V, IRBuilder<> &IRB, const DataLayout &DL) {
  auto *IntPtrTy = DL.getIntPtrType(V.getType());
  return IRB.CreatePtrToInt(&V, IntPtrTy, V.getName() + ".p2i");
}

Value *vc::castToIntOrFloat(Value &V, Type &DestTy, IRBuilder<> &IRB,
                            const DataLayout &DL) {
  IGC_ASSERT_MESSAGE(DestTy.isIntOrIntVectorTy() || DestTy.isFPOrFPVectorTy(),
                     "wrong argument: dest type must be int or float or vector "
                     "of one of them");
  IGC_ASSERT_MESSAGE(DL.getTypeSizeInBits(V.getType()) ==
                         DL.getTypeSizeInBits(&DestTy),
                     "wrong argument: value V size and DestTy size must match");
  Value *BitCastSrc = &V;
  if (V.getType()->isPtrOrPtrVectorTy())
    BitCastSrc = vc::createNopPtrToInt(V, IRB, DL);
  return IRB.CreateBitCast(BitCastSrc, &DestTy, BitCastSrc->getName() + ".bc");
}

Value *vc::castFromIntOrFloat(Value &V, Type &DestTy, IRBuilder<> &IRB,
                              const DataLayout &DL) {
  IGC_ASSERT_MESSAGE(
      V.getType()->isIntOrIntVectorTy() || V.getType()->isFPOrFPVectorTy(),
      "wrong argument: V must have int or float or vector of one of them");
  IGC_ASSERT_MESSAGE(DL.getTypeSizeInBits(V.getType()) ==
                         DL.getTypeSizeInBits(&DestTy),
                     "wrong argument: value V size and DestTy size must match");
  if (!DestTy.isPtrOrPtrVectorTy())
    return IRB.CreateBitCast(&V, &DestTy, V.getName() + ".bc");
  // Going in 2 steps for pointers: bitcast + inttoptr.
  auto *IntPtrTy = DL.getIntPtrType(&DestTy);
  auto *BC = IRB.CreateBitCast(&V, IntPtrTy, V.getName() + ".bc");
  return IRB.CreateIntToPtr(BC, &DestTy, V.getName() + ".i2p");
}

Type *vc::getFloatNTy(IRBuilder<> &Builder, unsigned N) {
  switch (N) {
  default:
    IGC_ASSERT_MESSAGE(false, "Unexpected number of bits");
    return nullptr;
  case 16:
    return Builder.getHalfTy();
  case 32:
    return Builder.getFloatTy();
  case 64:
    return Builder.getDoubleTy();
  }
}

// Cast one-element result of instruction  to scalar.
Instruction *vc::fixDegenerateVector(Instruction &Inst, IRBuilder<> &Builder) {
  return cast<Instruction>(Builder.CreateBitCast(
      &Inst, &vc::fixDegenerateVectorType(*Inst.getType())));
}

bool vc::isCastToGenericAS(const Value &Op) {
  if (!isa<IGCLLVM::AddrSpaceCastOperator>(Op))
    return false;
  return Op.getType()->getPointerAddressSpace() == AddrSpace::Generic;
}

Constant &vc::castArrayToFirstElemPtr(GlobalVariable &Array) {
  IGC_ASSERT_MESSAGE(isa<ArrayType>(Array.getValueType()),
                     "Array global variable was expected");
  Constant *Zero =
      Constant::getNullValue(IntegerType::getInt32Ty(Array.getContext()));
  return *ConstantExpr::getInBoundsGetElementPtr(
      Array.getValueType(), &Array, ArrayRef<Constant *>{Zero, Zero});
}

bool vc::isBitCastAllowed(llvm::Value &Val, llvm::Type &DstType) {
  return CastInst::castIsValid(Instruction::BitCast, &Val, &DstType);
}
