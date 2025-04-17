/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvmWrapper/IR/DerivedTypes.h"

#include "PacketBuilder.h"
#include "Probe/Assertion.h"

using namespace llvm;

namespace pktz {
//////////////////////////////////////////////////////////////////////////
/// @brief Contructor for Builder.
/// @param pJitMgr - JitManager which contains modules, function passes, etc.
PacketBuilder::PacketBuilder(Module *MIn, uint32_t Width) {
  M = static_cast<IGCLLVM::Module *>(MIn);
  // Built in types: scalar
  LLVMContext &Ctx = getContext();
  IRB = new IRBuilder<>(Ctx);
  FP32Ty = Type::getFloatTy(Ctx);
  Int1Ty = Type::getInt1Ty(Ctx);
  Int8Ty = Type::getInt8Ty(Ctx);
  Int16Ty = Type::getInt16Ty(Ctx);
  Int32Ty = Type::getInt32Ty(Ctx);
  Int64Ty = Type::getInt64Ty(Ctx);
  // Built in types: target simd
  setTargetWidth(Width);
}

void PacketBuilder::setTargetWidth(uint32_t Width) {
  VWidth = Width;
  SimdInt32Ty = IGCLLVM::FixedVectorType::get(Int32Ty, VWidth);
  SimdFP32Ty = IGCLLVM::FixedVectorType::get(FP32Ty, VWidth);
}

//////////////////////////////////////////////////////////////////////////
/// @brief Packetizes the type. Assumes SOA conversion.
Type *PacketBuilder::getVectorType(Type *Ty) {
  if (Ty->isVoidTy())
    return Ty;
  if (auto *VecTy = dyn_cast<IGCLLVM::FixedVectorType>(Ty)) {
    uint32_t VecSize = VecTy->getNumElements();
    auto *ElemTy = VecTy->getElementType();
    return IGCLLVM::FixedVectorType::get(ElemTy, VecSize * VWidth);
  }
  // [N x float] should packetize to [N x <8 x float>]
  if (Ty->isArrayTy()) {
    uint32_t ArrSize = Ty->getArrayNumElements();
    auto *ArrTy = Ty->getArrayElementType();
    auto *VecArrTy = getVectorType(ArrTy);
    return ArrayType::get(VecArrTy, ArrSize);
  }
  // {float,int} should packetize to {<8 x float>, <8 x int>}
  if (Ty->isAggregateType()) {
    uint32_t NumElems = Ty->getStructNumElements();
    SmallVector<Type *, 8> VecTys;
    for (uint32_t Idx = 0; Idx < NumElems; ++Idx) {
      auto *ElemTy = Ty->getStructElementType(Idx);
      auto *VecElemTy = getVectorType(ElemTy);
      VecTys.push_back(VecElemTy);
    }
    return StructType::get(getContext(), VecTys);
  }
  // <ty> should packetize to <8 x <ty>>
  return IGCLLVM::FixedVectorType::get(Ty, VWidth);
}
} // end of namespace pktz
