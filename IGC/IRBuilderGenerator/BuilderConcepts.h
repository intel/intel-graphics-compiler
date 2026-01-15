/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <concepts>

namespace llvm {

class IRBuilderBase;
class LLVMContext;
class Module;

// Derived must inherit from llvm::IRBuilderBase (or a subclass such as
// IGCIRBuilder<>).  This cannot be applied directly as a class-template
// requires-clause on the CRTP mixin because Derived is still incomplete
// at that point.
template <typename T>
concept DerivedFromIRBuilder = std::derived_from<T, IRBuilderBase>;

// getCtx() must return an object providing getLLVMContext() and getModule().
// The generated code calls derived().getCtx().getLLVMContext() and
// derived().getCtx().getModule().  Both IGC::CodeGenContext and
// IGC::SurfaceStateBuilderContext satisfy this.
template <typename T>
concept HasGetCtxBase = requires(const T &t) {
  { t.getCtx().getLLVMContext() } -> std::convertible_to<LLVMContext *>;
  { t.getCtx().getModule() } -> std::convertible_to<Module *>;
};

// RTStackReflectionBuilder currently only depends on the base context
// interface above.  If generated code starts depending on additional
// context operations, extend this concept to name those operations
// explicitly rather than constraining to one concrete context type.
template <typename T>
concept HasRTStackReflectionCtx = HasGetCtxBase<T>;

template <typename T>
concept HasSurfaceStateSize = requires(const T &t) {
  { t.getSurfaceStateSize() } -> std::same_as<unsigned>;
};

} // namespace llvm
