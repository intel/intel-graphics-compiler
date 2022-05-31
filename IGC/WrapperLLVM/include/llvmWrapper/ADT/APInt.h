/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_APINT_H
#define IGCLLVM_ADT_APINT_H

#include <llvm/ADT/APInt.h>

#if LLVM_VERSION_MAJOR >= 14
#include "llvm/Support/DivisionByConstantInfo.h"
#endif

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR >= 14
using SignedDivisionByConstantInfo = llvm::SignedDivisionByConstantInfo;
using UnsignedDivisonByConstantInfo = llvm::UnsignedDivisonByConstantInfo;
#else
using SignedDivisionByConstantInfo = llvm::APInt::ms;
using UnsignedDivisonByConstantInfo = llvm::APInt::mu;
#endif

inline SignedDivisionByConstantInfo getAPIntMagic(const llvm::APInt &value) {
#if LLVM_VERSION_MAJOR >= 14
    return llvm::SignedDivisionByConstantInfo::get(value);
#else
    return value.magic();
#endif
}

inline UnsignedDivisonByConstantInfo getAPIntMagicUnsigned(const llvm::APInt &value, const unsigned LeadingZeros = 0) {
#if LLVM_VERSION_MAJOR >= 14
    return llvm::UnsignedDivisonByConstantInfo::get(value, LeadingZeros);
#else
    return value.magicu(LeadingZeros);
#endif
}

inline bool IsAddition(const UnsignedDivisonByConstantInfo &mu) {
#if LLVM_VERSION_MAJOR >= 14
    return mu.IsAdd;
#else
    return mu.a;
#endif
}

inline unsigned ShiftAmount(const UnsignedDivisonByConstantInfo &mu) {
#if LLVM_VERSION_MAJOR >= 14
    return mu.ShiftAmount;
#else
    return mu.s;
#endif
}

inline unsigned ShiftAmount(const SignedDivisionByConstantInfo &ms) {
#if LLVM_VERSION_MAJOR >= 14
    return ms.ShiftAmount;
#else
    return ms.s;
#endif
}

inline llvm::APInt MagicNumber(const UnsignedDivisonByConstantInfo &mu) {
#if LLVM_VERSION_MAJOR >= 14
    return mu.Magic;
#else
    return mu.m;
#endif
}

inline llvm::APInt MagicNumber(const SignedDivisionByConstantInfo &ms) {
#if LLVM_VERSION_MAJOR >= 14
    return ms.Magic;
#else
    return ms.m;
#endif
}
}

#endif