/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Constants.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"

#include "common/StringMacros.hpp"
#include "debug/DebugMacros.hpp"

namespace llvm {

template <typename Derived> class RenderSurfaceStateBuilder {
protected:
  Derived &derived() { return static_cast<Derived &>(*this); }
  const Derived &derived() const { return static_cast<const Derived &>(*this); }

protected:
#include "AutoGenRenderSurfaceStatePrivate.h"
public:
#include "AutoGenRenderSurfaceStatePublic.h"

  Value *loadSurfaceBaseAddress(Value *heapBasePtr, Value *surfaceStateOffset, const Twine &Name = "") {
    return _loadSurfaceBaseAddress_64B(heapBasePtr, surfaceStateOffset, Name);
  }
};

} // namespace llvm
