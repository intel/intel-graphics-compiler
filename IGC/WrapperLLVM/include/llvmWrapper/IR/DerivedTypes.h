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

namespace IGCLLVM
{

 #if LLVM_VERSION_MAJOR <= 10
     using FixedVectorType = llvm::VectorType;
     const uint32_t VectorTyID = llvm::Type::VectorTyID;
     using ShuffleVectorMaskType = uint32_t;
 #else
     using FixedVectorType = llvm::FixedVectorType;
     const uint32_t VectorTyID = llvm::Type::FixedVectorTyID;
     using ShuffleVectorMaskType = int;
 #endif

    inline uint32_t GetVectorTypeBitWidth(llvm::Type* pType)
    {
#if LLVM_VERSION_MAJOR <= 10
        return llvm::cast<llvm::VectorType>(pType)->getBitWidth();
#else
      return (uint32_t)pType->getPrimitiveSizeInBits().getFixedValue();
#endif
    }

    inline bool isScalable(const FixedVectorType &Ty)
    {
#if LLVM_VERSION_MAJOR < 11
        return Ty.isScalable();
#else
        // Scalable vectors became a separate type since LLVM-11
        IGC_UNUSED(Ty);
        return false;
#endif
    }

    inline llvm::StructType *getTypeByName(llvm::Module *M, llvm::StringRef Name) {
#if LLVM_VERSION_MAJOR >= 12
        return llvm::StructType::getTypeByName(M->getContext(), Name);
#else
        return M->getTypeByName(Name);
#endif
    }

    inline llvm::Type *getWithNewBitWidth(const llvm::Type *Ty,
                                          unsigned NewBitWidth) {
#if LLVM_VERSION_MAJOR < 10
      IGC_ASSERT(Ty && Ty->isIntOrIntVectorTy());
      auto EltTy = llvm::Type::getIntNTy(Ty->getContext(), NewBitWidth);
      if (auto *VTy = llvm::dyn_cast<llvm::VectorType>(Ty))
        return llvm::VectorType::get(EltTy, VTy->getElementCount());
      return EltTy;
#else
      return Ty->getWithNewBitWidth(NewBitWidth);
#endif
    }

    inline llvm::PointerType* getWithSamePointeeType(llvm::PointerType* PT, unsigned AddressSpace) {
#if LLVM_VERSION_MAJOR < 14
        return llvm::PointerType::get(PT->getElementType(), AddressSpace);
#else
        return llvm::PointerType::getWithSamePointeeType(PT, AddressSpace);
#endif
    }
}

#endif
