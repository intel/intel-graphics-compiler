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

Value *PacketBuilder::GEP(Value *Ptr, Value *Idx, Type *Ty, const Twine &Name) {
  return IRB->CreateGEP(Ptr, Idx, Name);
}

Value *PacketBuilder::GEP(Type *Ty, Value *Ptr, Value *Idx, const Twine &Name) {
  return IRB->CreateGEP(Ty, Ptr, Idx, Name);
}

Value *PacketBuilder::GEP(Value *Ptr,
                          const std::initializer_list<Value *> &IndexList,
                          Type *Ty) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(Idx);
  return GEPA(Ptr, Indices);
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

Value *PacketBuilder::GEPA(Type *Ty, Value *Ptr, ArrayRef<Value *> IdxList,
                           const Twine &Name) {
  return IRB->CreateGEP(Ty, Ptr, IdxList, Name);
}

Value *
PacketBuilder::IN_BOUNDS_GEP(Value *Ptr,
                             const std::initializer_list<Value *> &IndexList) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(Idx);
  return IN_BOUNDS_GEP(Ptr, Indices);
}

Value *
PacketBuilder::IN_BOUNDS_GEP(Value *Ptr,
                             const std::initializer_list<uint32_t> &IndexList) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(C(Idx));
  return IN_BOUNDS_GEP(Ptr, Indices);
}

LoadInst *PacketBuilder::LOAD(Value *Ptr, const char *Name, Type *Ty) {
  assertMemoryUsageParams(Ptr);
  return IRB->CreateLoad(Ptr, Name);
}

LoadInst *PacketBuilder::LOAD(Value *Ptr, const Twine &Name, Type *Ty) {
  assertMemoryUsageParams(Ptr);
  return IRB->CreateLoad(Ptr, Name);
}

LoadInst *PacketBuilder::LOAD(Type *Ty, Value *Ptr, const Twine &Name) {
  assertMemoryUsageParams(Ptr);
  return IRB->CreateLoad(Ty, Ptr, Name);
}

LoadInst *PacketBuilder::LOAD(Value *Ptr, bool IsVolatile, const Twine &Name, Type *Ty) {
  assertMemoryUsageParams(Ptr);
  return IRB->CreateLoad(Ptr, IsVolatile, Name);
}

LoadInst *PacketBuilder::LOAD(Value *BasePtr,
                              const std::initializer_list<uint32_t> &IndexList,
                              const llvm::Twine &Name, Type *Ty) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(C(Idx));
  return PacketBuilder::LOAD(GEPA(BasePtr, Indices), Name);
}

LoadInst *PacketBuilder::LOADV(Value *BasePtr,
                               const std::initializer_list<Value *> &IndexList,
                               const llvm::Twine &Name) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(Idx);
  return LOAD(GEPA(BasePtr, Indices), Name);
}

StoreInst *
PacketBuilder::STORE(Value *Val, Value *BasePtr,
                     const std::initializer_list<uint32_t> &IndexList) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(C(Idx));
  return STORE(Val, GEPA(BasePtr, Indices));
}

StoreInst *
PacketBuilder::STOREV(Value *Val, Value *BasePtr,
                      const std::initializer_list<Value *> &IndexList) {
  std::vector<Value *> Indices;
  for (auto Idx : IndexList)
    Indices.push_back(Idx);
  return STORE(Val, GEPA(BasePtr, Indices));
}

Value *PacketBuilder::OFFSET_TO_NEXT_COMPONENT(Value *Base, Constant *Offset) {
  return GEP(Base, Offset);
}

Value *PacketBuilder::MEM_ADD(Value *Increment, Value *BasePtr,
                              const std::initializer_list<uint32_t> &IndexList,
                              const llvm::Twine &Name) {
  auto *Val = LOAD(GEP(BasePtr, IndexList), Name);
  return STORE(ADD(Val, Increment), GEP(BasePtr, IndexList));
}

} // namespace pktz
