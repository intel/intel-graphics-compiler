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
  VWidth16 = 16;
  M = static_cast<IGCLLVM::Module *>(MIn);
  // Built in types: scalar
  LLVMContext &Ctx = getContext();
  IRB = new IGCLLVM::IRBuilder<>(Ctx);
  VoidTy = Type::getVoidTy(Ctx);
  FP16Ty = Type::getHalfTy(Ctx);
  FP32Ty = Type::getFloatTy(Ctx);
  FP32PtrTy = PointerType::get(FP32Ty, 0);
  DoubleTy = Type::getDoubleTy(Ctx);
  Int1Ty = Type::getInt1Ty(Ctx);
  Int8Ty = Type::getInt8Ty(Ctx);
  Int16Ty = Type::getInt16Ty(Ctx);
  Int32Ty = Type::getInt32Ty(Ctx);
  Int8PtrTy = PointerType::get(Int8Ty, 0);
  Int16PtrTy = PointerType::get(Int16Ty, 0);
  Int32PtrTy = PointerType::get(Int32Ty, 0);
  Int64Ty = Type::getInt64Ty(Ctx);
  Simd4FP64Ty = IGCLLVM::FixedVectorType::get(DoubleTy, 4);
  // Built in types: simd16
  Simd16Int1Ty = IGCLLVM::FixedVectorType::get(Int1Ty, VWidth16);
  Simd16Int16Ty = IGCLLVM::FixedVectorType::get(Int16Ty, VWidth16);
  Simd16Int32Ty = IGCLLVM::FixedVectorType::get(Int32Ty, VWidth16);
  Simd16Int64Ty = IGCLLVM::FixedVectorType::get(Int64Ty, VWidth16);
  Simd16FP16Ty = IGCLLVM::FixedVectorType::get(FP16Ty, VWidth16);
  Simd16FP32Ty = IGCLLVM::FixedVectorType::get(FP32Ty, VWidth16);
  Simd32Int8Ty = IGCLLVM::FixedVectorType::get(Int8Ty, 32);
  if (sizeof(uint32_t *) == 4) {
    IntPtrTy = Int32Ty;
    Simd16IntPtrTy = Simd16Int32Ty;
  } else {
    IGC_ASSERT(sizeof(uint32_t *) == 8);
    IntPtrTy = Int64Ty;
    Simd16IntPtrTy = Simd16Int64Ty;
  }
  // Built in types: target simd
  setTargetWidth(Width);
}

void PacketBuilder::setTargetWidth(uint32_t Width) {
  VWidth = Width;
  SimdInt1Ty = IGCLLVM::FixedVectorType::get(Int1Ty, VWidth);
  SimdInt16Ty = IGCLLVM::FixedVectorType::get(Int16Ty, VWidth);
  SimdInt32Ty = IGCLLVM::FixedVectorType::get(Int32Ty, VWidth);
  SimdInt64Ty = IGCLLVM::FixedVectorType::get(Int64Ty, VWidth);
  SimdFP16Ty = IGCLLVM::FixedVectorType::get(FP16Ty, VWidth);
  SimdFP32Ty = IGCLLVM::FixedVectorType::get(FP32Ty, VWidth);
  if (sizeof(uint32_t *) == 4) {
    SimdIntPtrTy = SimdInt32Ty;
  } else {
    IGC_ASSERT(sizeof(uint32_t *) == 8);
    SimdIntPtrTy = SimdInt64Ty;
  }
}

/// @brief Mark this alloca as temporary to avoid hoisting later on
void PacketBuilder::setTempAlloca(Value *Inst) {
  auto *AI = dyn_cast<AllocaInst>(Inst);
  IGC_ASSERT_MESSAGE(AI, "Unexpected non-alloca instruction");
  auto *N =
      MDNode::get(getContext(), MDString::get(getContext(), "is_temp_alloca"));
  AI->setMetadata("is_temp_alloca", N);
}

bool PacketBuilder::isTempAlloca(Value *Inst) {
  auto *AI = dyn_cast<AllocaInst>(Inst);
  IGC_ASSERT_MESSAGE(AI, "Unexpected non-alloca instruction");
  return AI->getMetadata("is_temp_alloca") != nullptr;
}

// Returns true if able to find a call instruction to mark
bool PacketBuilder::setNamedMetaDataOnCallInstr(Instruction *Inst,
                                                StringRef MDName) {
  auto *CI = dyn_cast<CallInst>(Inst);
  if (CI) {
    auto *N = MDNode::get(getContext(), MDString::get(getContext(), MDName));
    CI->setMetadata(MDName, N);
    return true;
  } else {
    // Follow use def chain back up
    for (auto &U : Inst->operands()) {
      auto *SrcInst = dyn_cast<Instruction>(U.get());
      if (SrcInst)
        if (setNamedMetaDataOnCallInstr(SrcInst, MDName))
          return true;
    }
  }
  return false;
}

bool PacketBuilder::hasNamedMetaDataOnCallInstr(Instruction *Inst,
                                                StringRef MDName) {
  auto *CI = dyn_cast<CallInst>(Inst);
  if (!CI)
    return false;
  return CI->getMetadata(MDName) != nullptr;
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
