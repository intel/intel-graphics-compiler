/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INLINEASM_H
#define IGCLLVM_IR_INLINEASM_H

#include "Probe/Assertion.h"

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/InlineAsm.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline std::string getAsmString(const llvm::InlineAsm *IA) {
  IGC_ASSERT(IA);

#if LLVM_VERSION_MAJOR >= 22
  return IA->getAsmString().str();
#else
  return IA->getAsmString();
#endif
}
} // namespace IGCLLVM

#endif
