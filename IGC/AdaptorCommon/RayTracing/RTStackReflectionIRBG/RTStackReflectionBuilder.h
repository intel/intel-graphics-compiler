/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

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

/// CRTP mixin providing RTStack reflection IRBuilder functionality.
///
/// Derived classes must:
/// 1. Inherit from an IRBuilder (e.g., IGCIRBuilder<>)
/// 2. Implement: const IGC::CodeGenContext& getCtx() const
///
/// Example usage:
///   class MyBuilder : public IGCIRBuilder<>,
///                     public RTStackReflectionBuilder<MyBuilder> {
///     const IGC::CodeGenContext& Ctx;
///   public:
///     const IGC::CodeGenContext& getCtx() const { return Ctx; }
///   };
template <typename Derived> class RTStackReflectionBuilder {
protected:
  Derived &derived() { return static_cast<Derived &>(*this); }
  const Derived &derived() const { return static_cast<const Derived &>(*this); }

protected:
#include "AutoGenRTStackReflectionPrivate.h"
public:
#include "AutoGenRTStackReflectionPublic.h"
};

} // namespace llvm
