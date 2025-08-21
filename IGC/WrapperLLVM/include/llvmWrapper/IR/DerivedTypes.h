/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_DERIVEDTYPES_H
#define IGCLLVM_IR_DERIVEDTYPES_H

#include "Probe/Assertion.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "common/CommonMacros.h"

#include "Type.h"

namespace IGCLLVM {
using FixedVectorType = llvm::FixedVectorType;
const uint32_t VectorTyID = llvm::Type::FixedVectorTyID;
using ShuffleVectorMaskType = int;

inline uint32_t GetVectorTypeBitWidth(llvm::Type *pType) {
  return (uint32_t)pType->getPrimitiveSizeInBits().getFixedValue();
}

inline bool isScalable(const FixedVectorType &Ty) {
  IGC_UNUSED(Ty);
  return false;
}

inline llvm::StructType *getTypeByName(llvm::Module *M, llvm::StringRef Name) {
  return llvm::StructType::getTypeByName(M->getContext(), Name);
}

inline llvm::Type *getWithNewBitWidth(const llvm::Type *Ty, unsigned NewBitWidth) {
  return Ty->getWithNewBitWidth(NewBitWidth);
}

inline llvm::PointerType *getWithSamePointeeType(llvm::PointerType *PT, unsigned AddressSpace) {
  return llvm::PointerType::getWithSamePointeeType(PT, AddressSpace);
}
} // namespace IGCLLVM

#endif
