/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Constants.h"
#if defined(_DEBUG) || defined(_RELEASE_INTERNAL)
#include "llvm/IR/Module.h"
#endif // defined(_DEBUG) || defined(_RELEASE_INTERNAL)
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"

#include "common/StringMacros.hpp"
#include "debug/DebugMacros.hpp"

#include "BuilderConcepts.h"

namespace llvm {

#if defined(_DEBUG) || defined(_RELEASE_INTERNAL)
bool useRTStackReflectionTestCalls();
#endif // defined(_DEBUG) || defined(_RELEASE_INTERNAL)

// CRTP mixin providing RTStack reflection IRBuilder functionality.
//
// Derived classes must:
// 1. Inherit from an IRBuilder (e.g., IGCIRBuilder<>)
// 2. Implement getCtx() returning an object with getLLVMContext() and
//    getModule()
//
// The mixin validates these requirements in its constructor.  That is
// early enough to diagnose a malformed derived builder as soon as the
// builder type is instantiated, while still avoiding incomplete-type
// issues during CRTP base-class formation.
//
// Example usage:
//   class MyContext {
//   public:
//     llvm::LLVMContext *getLLVMContext() const;
//     llvm::Module *getModule() const;
//   };
//
//   class MyBuilder : public IGCIRBuilder<>,
//                     public RTStackReflectionBuilder<MyBuilder> {
//     const MyContext &Ctx;
//   public:
//     const MyContext &getCtx() const { return Ctx; }
//   };
template <typename Derived> class RTStackReflectionBuilder {
protected:
  RTStackReflectionBuilder() { checkDerivedRequirements(); }

  static void checkDerivedRequirements() {
    static_assert(DerivedFromIRBuilder<Derived>, "Derived must inherit from llvm::IRBuilderBase");
    static_assert(HasRTStackReflectionCtx<Derived>,
                  "Derived must implement getCtx() returning an object with getLLVMContext() and getModule()");
  }

  Derived &derived() { return static_cast<Derived &>(*this); }
  const Derived &derived() const { return static_cast<const Derived &>(*this); }

#if defined(_DEBUG) || defined(_RELEASE_INTERNAL)
  static bool shouldEmitTestCalls() { return useRTStackReflectionTestCalls(); }

  Value *createTestCall(StringRef Name, Type *ReturnTy, ArrayRef<Value *> Args, const Twine &ReturnName = "") {
    SmallVector<Type *, 16> ArgTypes;
    for (Value *Arg : Args)
      ArgTypes.push_back(Arg->getType());
    auto *FTy = FunctionType::get(ReturnTy, ArgTypes, false);
    auto *M = derived().getCtx().getModule();

    // A builder can be used with different address spaces in the same module.
    std::string FunctionName = Name.str();
    unsigned Suffix = 0;
    while (auto *Existing = M->getNamedValue(FunctionName)) {
      auto *F = dyn_cast<Function>(Existing);
      if (F && F->isDeclaration() && F->getFunctionType() == FTy)
        break;
      FunctionName = (Name + "." + Twine(++Suffix)).str();
    }
    return derived().CreateCall(M->getOrInsertFunction(FunctionName, FTy), Args, ReturnName);
  }
#endif // defined(_DEBUG) || defined(_RELEASE_INTERNAL)

protected:
#include "AutoGenRTStackReflectionPrivate.h"
public:
#include "AutoGenRTStackReflectionPublic.h"
};

} // namespace llvm
