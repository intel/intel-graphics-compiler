/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CONSTANTS_H
#define IGCLLVM_IR_CONSTANTS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Constants.h"

#if LLVM_VERSION_MAJOR > 12
#include "llvm/Support/TypeSize.h"
#endif

namespace IGCLLVM
{
  inline
#if LLVM_VERSION_MAJOR <= 12
  unsigned
#else
  llvm::ElementCount
#endif
  getElementCount(const llvm::ConstantAggregateZero &C) {
#if LLVM_VERSION_MAJOR <= 12
    return C.getNumElements();
#else
    return C.getElementCount();
#endif
  }

  namespace ConstantExpr {
#if LLVM_VERSION_MAJOR < 11
        inline llvm::Constant *getShuffleVector(llvm::Constant *V1,
            llvm::Constant *V2, llvm::ArrayRef<uint64_t> Mask,
            llvm::Type *OnlyIfReducedTy = nullptr) {
            return llvm::ConstantExpr::getShuffleVector(V1, V2,
                       llvm::ConstantDataVector::get(V1->getContext(), Mask),
                       OnlyIfReducedTy);
        }
#else
        inline llvm::Constant *getShuffleVector(llvm::Constant *V1,
            llvm::Constant *V2, llvm::ArrayRef<int> Mask,
            llvm::Type *OnlyIfReducedTy = nullptr) {
            return llvm::ConstantExpr::getShuffleVector(V1, V2, Mask,
                                                        OnlyIfReducedTy);
        }
#endif
    }

    namespace ConstantFixedVector
    {
    inline llvm::Constant *getSplatValue(llvm::ConstantVector *CV,
                                         bool AllowUndefs = false) {
#if LLVM_VERSION_MAJOR < 10
      if (!AllowUndefs)
        CV->getSplatValue();
      llvm::Constant *Elt = CV->getOperand(0);
      for (unsigned I = 1, E = CV->getNumOperands(); I < E; ++I) {
        llvm::Constant *OpC = CV->getOperand(I);
        if (llvm::isa<llvm::UndefValue>(OpC))
          continue;
        if (llvm::isa<llvm::UndefValue>(Elt))
          Elt = OpC;
        if (OpC != Elt)
          return nullptr;
      }
      return Elt;
#else
      return CV->getSplatValue(AllowUndefs);
#endif
    }
    inline llvm::Constant *getSplat(unsigned NumElements, llvm::Constant *V) {
#if LLVM_VERSION_MAJOR < 11
            return llvm::ConstantVector::getSplat(NumElements, V);
#elif LLVM_VERSION_MAJOR == 11
            return llvm::ConstantVector::getSplat(
                llvm::ElementCount{NumElements, /* IsScalable=*/false}, V);
#else
            return llvm::ConstantVector::getSplat(
                llvm::ElementCount::getFixed(NumElements), V);
#endif
        }
    }

    namespace Constant {
    inline llvm::Constant *getSplatValue(llvm::Constant *C,
                                         bool AllowUndefs = false) {
#if LLVM_VERSION_MAJOR < 10
      if (auto CV = llvm::dyn_cast<llvm::ConstantVector>(C))
        return IGCLLVM::ConstantFixedVector::getSplatValue(CV, AllowUndefs);
      return C->getSplatValue();
#else
      return C->getSplatValue(AllowUndefs);
#endif
    }
    } // namespace Constant
}

#endif
