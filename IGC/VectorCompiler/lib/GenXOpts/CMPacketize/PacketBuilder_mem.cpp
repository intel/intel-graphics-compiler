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

Value *PacketBuilder::GEP(Value *Ptr,
                          const std::initializer_list<uint32_t> &IndexList,
                          Type *Ty) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(C(Idx));
  return GEPA(Ptr, Indices);
}

Value *PacketBuilder::GEPA(Value *Ptr, ArrayRef<Value *> IdxList,
                           const Twine &Name) {
  return IRB->CreateGEP(Ptr, IdxList, Name);
}

LoadInst *PacketBuilder::LOAD(Value *Ptr, const Twine &Name, Type *Ty) {
  assertMemoryUsageParams(Ptr);
  return IRB->CreateLoad(Ptr, Name);
}

LoadInst *PacketBuilder::LOAD(Value *BasePtr,
                              const std::initializer_list<uint32_t> &IndexList,
                              const llvm::Twine &Name, Type *Ty) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(C(Idx));
  return PacketBuilder::LOAD(GEPA(BasePtr, Indices), Name);
}

LoadInst *PacketBuilder::ALIGNED_LOAD(Value *Ptr, IGCLLVM::Align Align,
                                      const Twine &Name) {
  auto *Ty = IGCLLVM::getNonOpaquePtrEltTy(Ptr->getType());
  return IRB->CreateAlignedLoad(Ty, Ptr, Align, Name);
}

AllocaInst *PacketBuilder::ALLOCA(Type *Ty, Value *ArraySize,
                                  const Twine &Name) {
  return IRB->CreateAlloca(Ty, ArraySize, Name);
}

Value *PacketBuilder::INT_TO_PTR(Value *V, Type *DestTy, const Twine &Name) {
  return IRB->CreateIntToPtr(V, DestTy, Name);
}

CallInst *PacketBuilder::MASKED_GATHER(Value *Ptrs, unsigned Align, Value *Mask,
                                       Value *PassThru, const Twine &Name) {
  return IRB->CreateMaskedGather(
      Ptrs, IGCLLVM::getAlignmentValueIfNeeded(IGCLLVM::getAlign(Align)), Mask,
      PassThru, Name);
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
