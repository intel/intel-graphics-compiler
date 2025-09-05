/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_VALUE_H
#define IGCLLVM_IR_VALUE_H

#include "Probe/Assertion.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Config/llvm-config.h"
#include "llvmWrapper/IR/User.h"

namespace IGCLLVM {
#define stripPointerCastsNoFollowAliases() stripPointerCasts()

inline void replaceUsesWithIf(llvm::Value *V, llvm::Value *New, llvm::function_ref<bool(llvm::Use &U)> ShouldReplace) {
  // similar to llvm::Value::replaceUsesWithIf implementation
  IGC_ASSERT_MESSAGE(V, "Cannot replace uses of nullptr");
  IGC_ASSERT_MESSAGE(New, "Value::replaceUsesWithIf(<null>) is invalid!");
  IGC_ASSERT_MESSAGE(New->getType() == V->getType(), "replaceUses of value with new value of different type!");
  V->replaceUsesWithIf(New, ShouldReplace);
}

inline uint64_t getPointerDereferenceableBytes(const llvm::Value *Ptr, const llvm::DataLayout &DL, bool &CanBeNull,
                                               bool &CanBeFreed) {
  return Ptr->getPointerDereferenceableBytes(DL, CanBeNull, CanBeFreed);
}

inline llvm::User *getUniqueUndroppableUser(llvm::Value *V) {
  return V->getUniqueUndroppableUser();
}
} // namespace IGCLLVM

#endif
