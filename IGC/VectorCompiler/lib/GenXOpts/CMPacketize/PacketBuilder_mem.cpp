/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PacketBuilder.h"
#include "Probe/Assertion.h"
#include <cstdarg>

namespace pktz {
void PacketBuilder::assertMemoryUsageParams(Value *Ptr) {
  IGC_ASSERT_MESSAGE(Ptr->getType() != Int64Ty,
                     "Address appears to be GFX access.  Requires translation "
                     "through BuilderGfxMem.");
}

GetElementPtrInst *
PacketBuilder::GEP(Type *Ty, Value *Ptr,
                   const std::initializer_list<uint32_t> &IndexList,
                   const Twine &Name) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(C(Idx));
  return GEPA(Ty, Ptr, Indices, Name);
}

GetElementPtrInst *PacketBuilder::GEPA(Type *Ty, Value *Ptr,
                                       ArrayRef<Value *> IdxList,
                                       const Twine &Name) {
  return cast<GetElementPtrInst>(IRB->CreateGEP(Ty, Ptr, IdxList, Name));
}

LoadInst *PacketBuilder::LOAD(Type *Ty, Value *Ptr, const Twine &Name) {
  assertMemoryUsageParams(Ptr);
  return IRB->CreateLoad(Ty, Ptr, Name);
}

LoadInst *PacketBuilder::LOAD(Type *Ty, Value *BasePtr,
                              const std::initializer_list<uint32_t> &IndexList,
                              const llvm::Twine &Name) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(C(Idx));
  auto *GEPInst = GEPA(Ty, BasePtr, Indices);
  return PacketBuilder::LOAD(GEPInst->getSourceElementType(), GEPInst, Name);
}

LoadInst *PacketBuilder::ALIGNED_LOAD(Type *Ty, Value *Ptr, llvm::Align Align,
                                      const Twine &Name) {
  return IRB->CreateAlignedLoad(Ty, Ptr, Align, Name);
}

AllocaInst *PacketBuilder::ALLOCA(Type *Ty, Value *ArraySize,
                                  const Twine &Name) {
  return IRB->CreateAlloca(Ty, ArraySize, Name);
}

Value *PacketBuilder::INT_TO_PTR(Value *V, Type *DestTy, const Twine &Name) {
  return IRB->CreateIntToPtr(V, DestTy, Name);
}

CallInst *PacketBuilder::MASKED_GATHER(Type *Ty, Value *Ptrs, unsigned Align,
                                       Value *Mask, Value *PassThru,
                                       const Twine &Name) {
  return IRB->CreateMaskedGather(
      Ty, Ptrs, IGCLLVM::getAlignmentValueIfNeeded(IGCLLVM::getAlign(Align)),
      Mask, PassThru, Name);
}

CallInst *PacketBuilder::MASKED_SCATTER(Value *Val, Value *Ptrs, unsigned Align,
                                        Value *Mask) {
  return IRB->CreateMaskedScatter(
      Val, Ptrs, IGCLLVM::getAlignmentValueIfNeeded(IGCLLVM::getAlign(Align)),
      Mask);
}

CallInst *PacketBuilder::MASKED_STORE(Value *Val, Value *Ptr, unsigned Align,
                                      Value *Mask) {
  return IRB->CreateMaskedStore(
      Val, Ptr, IGCLLVM::getAlignmentValueIfNeeded(IGCLLVM::getAlign(Align)),
      Mask);
}

StoreInst *PacketBuilder::STORE(Value *Val, Value *Ptr, bool IsVolatile) {
  return IRB->CreateStore(Val, Ptr, IsVolatile);
}
} // namespace pktz
