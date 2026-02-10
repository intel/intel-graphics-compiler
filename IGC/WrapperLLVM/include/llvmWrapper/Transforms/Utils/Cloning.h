/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_CLONING_H
#define IGCLLVM_TRANSFORMS_UTILS_CLONING_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Support/Casting.h"
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

namespace IGCLLVM {
inline bool InlineFunction(llvm::CallBase &CB, llvm::InlineFunctionInfo &IFI, llvm::AAResults *CalleeAAR = nullptr,
                           bool InsertLifetime = true, llvm::Function *ForwardVarArgsTo = nullptr) {
#if LLVM_VERSION_MAJOR <= 15
  return llvm::InlineFunction(CB, IFI, CalleeAAR, InsertLifetime, ForwardVarArgsTo).isSuccess();
#else // LLVM_VERSION_MAJOR >= 16
  return llvm::InlineFunction(CB, IFI, true, CalleeAAR, InsertLifetime, ForwardVarArgsTo).isSuccess();
#endif
}

using llvm::CloneFunctionChangeType;
using llvm::CloneFunctionInto;
} // namespace IGCLLVM

#endif
