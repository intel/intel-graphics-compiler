/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_STRINGEXTRAS_H
#define IGCLLVM_ADT_STRINGEXTRAS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline std::string toString(const llvm::APInt &I, unsigned Radix, bool Signed, bool formatAsCLiteral = false) {
  llvm::SmallString<40> S;
  I.toString(S, Radix, Signed, formatAsCLiteral);
  return std::string(S.str());
}

inline std::string toString(const llvm::APSInt &I, unsigned Radix) { return IGCLLVM::toString(I, Radix, I.isSigned()); }
} // namespace IGCLLVM

#endif
