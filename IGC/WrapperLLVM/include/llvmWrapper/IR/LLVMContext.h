/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_LLVMCONTEXT_H
#define IGCLLVM_IR_LLVMCONTEXT_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/LLVMContext.h"

#include "Probe/Assertion.h"

namespace IGCLLVM {
inline void setOpaquePointers(llvm::LLVMContext *Ctx, const bool Enable) {
  IGC_ASSERT_MESSAGE(Ctx, "Null LLVMContext pointer!");
#if LLVM_VERSION_MAJOR == 14
  if (Enable)
    Ctx->enableOpaquePointers();
#elif LLVM_VERSION_MAJOR >= 15
  Ctx->setOpaquePointers(Enable);
#endif // LLVM_VERSION_MAJOR
};
} // end namespace IGCLLVM

namespace IGC {
inline bool canOverwriteLLVMCtxPtrMode(llvm::LLVMContext *Ctx, bool IGC_IsPointerModeAlreadySet) {
  IGC_ASSERT_MESSAGE(Ctx, "Null LLVMContext pointer!");
#if LLVM_VERSION_MAJOR == 14
  // With LLVM 14, we invoke a proper check for the -opaque-pointers CL
  // option. Regardless of whether it's false by LLVM 14's default, or
  // through an explicit setting, we deem it acceptable for IGC to
  // override this when opaque pointers are force-enabled in experimental
  // mode.
  return Ctx->supportsTypedPointers();
#elif LLVM_VERSION_MAJOR == 15
  // With LLVM 15-16, we should not trigger CL option evaluation, as the
  // OPs mode will then get set as a permanent default. The only
  // alternative is to use an API below, non-native for LLVM 16.
  return !Ctx->hasSetOpaquePointersValue();
#elif LLVM_VERSION_MAJOR >= 16
  // LLVM 16: we start removing switching between typed/opaque ptrs mode
  // In order to prepare for full move to the opaque pointers.
  // The first step is to get rid of .patches related to opaque pointers mode.
  return !IGC_IsPointerModeAlreadySet;
#endif // LLVM_VERSION_MAJOR
}
} // end namespace IGC

#endif // IGCLLVM_IR_LLVMCONTEXT_H
