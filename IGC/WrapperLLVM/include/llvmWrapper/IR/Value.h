/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_VALUE_H
#define IGCLLVM_IR_VALUE_H

#include "Probe/Assertion.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Value.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR >= 10
#define stripPointerCastsNoFollowAliases() stripPointerCasts()
#endif

    inline void
    replaceUsesWithIf(llvm::Value *V, llvm::Value *New,
                      llvm::function_ref<bool(llvm::Use &U)> ShouldReplace) {
      // similar to llvm::Value::replaceUsesWithIf implementation
      IGC_ASSERT_MESSAGE(V, "Cannot replace uses of nullptr");
      IGC_ASSERT_MESSAGE(New, "Value::replaceUsesWithIf(<null>) is invalid!");
      IGC_ASSERT_MESSAGE(New->getType() == V->getType(),
                         "replaceUses of value with new value of different type!");
#if LLVM_VERSION_MAJOR < 10
      for (auto UI = V->use_begin(), E = V->use_end(); UI != E;) {
        llvm::Use &U = *UI;
        ++UI;
        if (!ShouldReplace(U))
          continue;
        U.set(New);
      }
#else
      V->replaceUsesWithIf(New, ShouldReplace);
#endif
    }

    inline uint64_t getPointerDereferenceableBytes(const llvm::Value* Ptr, const llvm::DataLayout& DL, bool& CanBeNull, bool& CanBeFreed)
    {
#if LLVM_VERSION_MAJOR >= 13
        // LLVM 13 introduced CanBeFreed argument.
        //
        // > IF CanBeFreed is true, the pointer is known to be dereferenceable at
        // > point of definition only.  Caller must prove that allocation is not
        // > deallocated between point of definition and use.
        //
        //     Differential Revision: https://reviews.llvm.org/D98908

        return Ptr->getPointerDereferenceableBytes(DL, CanBeNull, CanBeFreed);
#else
        return Ptr->getPointerDereferenceableBytes(DL, CanBeNull);
#endif
    }
}

#endif
