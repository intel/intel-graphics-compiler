/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PacketBuilder.h"
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/TypeSize.h"
#include <llvm/IR/DerivedTypes.h>

namespace pktz {
Constant *PacketBuilder::C(int Val) {
  return ConstantInt::get(IRB->getInt32Ty(), Val);
}

Constant *PacketBuilder::C(uint32_t Val) {
  return ConstantInt::get(IRB->getInt32Ty(), Val);
}

Constant *PacketBuilder::C(float Val) {
  return ConstantFP::get(IRB->getFloatTy(), Val);
}

Value *PacketBuilder::VIMMED1(int Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth),
                                  cast<ConstantInt>(C(Val)));
}

Value *PacketBuilder::VIMMED1(uint32_t Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth),
                                  cast<ConstantInt>(C(Val)));
}

Value *PacketBuilder::VIMMED1(float Val) {
  return ConstantVector::getSplat(IGCLLVM::getElementCount(VWidth),
                                  cast<ConstantFP>(C(Val)));
}

Value *PacketBuilder::VBROADCAST(Value *Src, const llvm::Twine &Name) {
  // check if Src is already a vector
  if (Src->getType()->isVectorTy()) {
    if (auto *CV = dyn_cast<ConstantVector>(Src)) {
      if (CV->getSplatValue()) {
        return VECTOR_SPLAT(VWidth *
                                cast<IGCLLVM::FixedVectorType>(Src->getType())
                                    ->getNumElements(),
                            CV->getSplatValue(), Name);
      }
    }
    return Src;
  }
  return VECTOR_SPLAT(VWidth, Src, Name);
}

CallInst *PacketBuilder::CALL(FunctionCallee *Callee,
                              const std::initializer_list<Value *> &ArgsList,
                              const llvm::Twine &Name) {
  std::vector<Value *> Args;
  for (auto *Arg : ArgsList)
    Args.push_back(Arg);
  return CALLA(Callee, Args, Name);
}

//////////////////////////////////////////////////////////////////////////
/// @brief C functions called by LLVM IR
//////////////////////////////////////////////////////////////////////////

uint32_t PacketBuilder::getTypeSize(Type *Ty) {
  if (Ty->isStructTy()) {
    uint32_t NumElems = Ty->getStructNumElements();
    auto *ElemTy = Ty->getStructElementType(0);
    return NumElems * getTypeSize(ElemTy);
  }
  if (Ty->isArrayTy()) {
    uint32_t NumElems = Ty->getArrayNumElements();
    auto *ElemTy = Ty->getArrayElementType();
    return NumElems * getTypeSize(ElemTy);
  }
  if (Ty->isIntegerTy()) {
    uint32_t BitSize = Ty->getIntegerBitWidth();
    return BitSize / 8;
  }
  if (Ty->isFloatTy())
    return 4;
  if (Ty->isHalfTy())
    return 2;
  if (Ty->isDoubleTy())
    return 8;
  IGC_ASSERT_MESSAGE(0, "Unimplemented type.");
  return 0;
}

Value *PacketBuilder::BITCAST(Value *V, Type *DestTy, const Twine &Name) {
  return IRB->CreateBitCast(V, DestTy, Name);
}

CallInst *PacketBuilder::CALLA(FunctionCallee *Callee, ArrayRef<Value *> Args,
                               const Twine &Name, MDNode *FPMathTag) {
  return IRB->CreateCall(*Callee, Args, Name, FPMathTag);
}

Value *PacketBuilder::CAST(Instruction::CastOps Op, Value *V, Type *DestTy,
                           const Twine &Name) {
  return IRB->CreateCast(Op, V, DestTy, Name);
}

Value *PacketBuilder::SELECT(Value *C, Value *True, Value *False,
                             const Twine &Name, Instruction *MDFrom) {
  return IRB->CreateSelect(C, True, False, Name, MDFrom);
}

Value *PacketBuilder::VECTOR_SPLAT(unsigned NumElts, Value *V,
                                   const Twine &Name) {
  return IRB->CreateVectorSplat(NumElts, V, Name);
}

Value *PacketBuilder::VEXTRACT(Value *Vec, uint64_t Idx, const Twine &Name) {
  return IRB->CreateExtractElement(Vec, Idx, Name);
}
} // namespace pktz
