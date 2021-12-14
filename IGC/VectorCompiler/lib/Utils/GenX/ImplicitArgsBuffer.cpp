/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/ImplicitArgsBuffer.h"
#include "vc/Utils/General/Types.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/Module.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <algorithm>
#include <array>

using namespace llvm;

StructType &vc::ImplicitArgs::Buffer::getType(Module &M) {
  using namespace vc::ImplicitArgs::Buffer;
  auto *MaybeResult = IGCLLVM::getTypeByName(M, TypeName);
  if (MaybeResult)
    return *MaybeResult;

  // Have to construct the type.
  auto &C = M.getContext();
  auto *Int8Ty = IntegerType::getInt8Ty(C);
  auto *Int32Ty = IntegerType::getInt32Ty(C);
  auto *Int64Ty = IntegerType::getInt64Ty(C);
  std::array<Type *, Indices::Size> ElementTys = {nullptr};
  ElementTys[Indices::StructSize] = Int8Ty;
  ElementTys[Indices::StructVersion] = Int8Ty;
  ElementTys[Indices::NumWorkDim] = Int8Ty;
  ElementTys[Indices::SIMDWidth] = Int8Ty;
  ElementTys[Indices::LocalSizeX] = Int32Ty;
  ElementTys[Indices::LocalSizeY] = Int32Ty;
  ElementTys[Indices::LocalSizeZ] = Int32Ty;
  ElementTys[Indices::GlobalSizeX] = Int64Ty;
  ElementTys[Indices::GlobalSizeY] = Int64Ty;
  ElementTys[Indices::GlobalSizeZ] = Int64Ty;
  ElementTys[Indices::PrintfBufferPtr] = Int64Ty;
  ElementTys[Indices::GlobalOffsetX] = Int64Ty;
  ElementTys[Indices::GlobalOffsetY] = Int64Ty;
  ElementTys[Indices::GlobalOffsetZ] = Int64Ty;
  ElementTys[Indices::LocalIDTablePtr] = Int64Ty;
  ElementTys[Indices::GroupCountX] = Int32Ty;
  ElementTys[Indices::GroupCountY] = Int32Ty;
  ElementTys[Indices::GroupCountZ] = Int32Ty;
  IGC_ASSERT_MESSAGE(std::all_of(ElementTys.begin(), ElementTys.end(),
                                 [](Type *Ty) -> bool { return Ty; }),
                     "all structure element types must be set by the function");
  return *StructType::create(C, ElementTys, TypeName);
}

PointerType &vc::ImplicitArgs::Buffer::getPtrType(Module &M) {
  return *PointerType::get(&vc::ImplicitArgs::Buffer::getType(M),
                           vc::AddrSpace::GlobalA32);
}

Value &vc::ImplicitArgs::Buffer::getPointer(IRBuilder<> &IRB) {
  auto *R0Decl = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertBlock()->getModule(), GenXIntrinsic::genx_r0,
      IRB.getInt32Ty());
  auto *R0 = IRB.CreateCall(R0Decl, None, "r0.0");
  auto *IntPtr = IRB.CreateAnd(
      R0, IRB.getInt32(maskTrailingZeros<uint32_t>(vc::PtrOffsetInR00)),
      "indir.data.heap.ptr.int");
  auto *PtrTy =
      &vc::ImplicitArgs::Buffer::getPtrType(*IRB.GetInsertBlock()->getModule());
  auto *Ptr = IRB.CreateIntToPtr(IntPtr, PtrTy, "indir.data.heap.ptr");
  return *Ptr;
}

Value &vc::ImplicitArgs::Buffer::loadField(Value &BufferPtr,
                                           Indices::Enum FieldIdx,
                                           IRBuilder<> &IRB, StringRef Name) {
  IGC_ASSERT_MESSAGE(BufferPtr.getType() ==
                         &vc::ImplicitArgs::Buffer::getPtrType(
                             *IRB.GetInsertBlock()->getModule()),
                     "wrong argument: a wrong type for buffer pointer value");
  auto *FieldPtr = IRB.CreateInBoundsGEP(
      BufferPtr.getType()->getPointerElementType(), &BufferPtr,
      {IRB.getInt32(0), IRB.getInt32(FieldIdx)}, Name + ".ptr");
  auto *FieldVal = IRB.CreateLoad(FieldPtr->getType()->getPointerElementType(),
                                  FieldPtr, Name);
  return *FieldVal;
}
