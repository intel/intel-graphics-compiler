/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_PREDEFINEDVARIABLE_H
#define VC_UTILS_GENX_PREDEFINEDVARIABLE_H

//===----------------------------------------------------------------------===//
//
/// Predefined Variables
///
/// vISA predefined variables (aka registers) are represented in LLVM IR as
/// global variables. This header provides utilities for working with such
/// variables.
///
/// Note that there are another approaches for the same idea in VC backend. The
/// main one is based on adding specific read/write intrinsics for every
/// predefined variable. This approach is considered obsolete. New predefined
/// variables should be represented via predefined variables and accessed via
/// common read/write instrinsics. The legacy approaches are to be replaced with
/// the new one.
//===----------------------------------------------------------------------===//

#include "Probe/Assertion.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>

namespace vc {
namespace PredefVar {

// The attribute that defines that a global variable is a predefined variable.
// Don't use this attribute directly, use interfaces below to create and
// identify predefined variables.
constexpr const char Attribute[] = "VCPredefinedVariable";

// The name of a global variable that represents predefined VISA variable BSS.
constexpr const char BSSName[] = "llvm.vc.predef.var.bss";

// The name of a global variable that represents predefined VISA variable
// IMPL_ARG_BUF_PTR.
constexpr const char ImplicitArgsBufferName[] =
    "llvm.vc.predef.var.impl.args.buf";

// The name of a global variable that represents predefined VISA variable
// LOCAL_ID_BUF_PTR.
constexpr const char LocalIDBufferName[] = "llvm.vc.predef.var.loc.id.buf";

enum class ID {
  BSS,
  ImplicitArgsBuffer,
  LocalIDBuffer,
};

// Get the name of a predefined variable defined by the provided ID.
template <enum ID PVID> const char *getName();

template <> inline const char *getName<ID::BSS>() { return BSSName; }

template <> inline const char *getName<ID::ImplicitArgsBuffer>() {
  return ImplicitArgsBufferName;
}

template <> inline const char *getName<ID::LocalIDBuffer>() {
  return LocalIDBufferName;
}

// Get the type of a predefined variable defined by the provided ID.
template <enum ID PVID> llvm::Type *getType(llvm::LLVMContext &C);

template <> inline llvm::Type *getType<ID::BSS>(llvm::LLVMContext &C) {
  return llvm::Type::getInt32Ty(C);
}

template <>
inline llvm::Type *getType<ID::ImplicitArgsBuffer>(llvm::LLVMContext &C) {
  return llvm::Type::getInt64Ty(C);
}

template <>
inline llvm::Type *getType<ID::LocalIDBuffer>(llvm::LLVMContext &C) {
  return llvm::Type::getInt64Ty(C);
}

// Checks whether a global variable \p GV is a predefined variable.
inline bool isPV(const llvm::GlobalVariable &GV) {
  return GV.hasAttribute(Attribute);
}

// Checks whether a value \p V is a predefined variable.
inline bool isPV(const llvm::Value &V) {
  if (!llvm::isa<llvm::GlobalVariable>(V))
    return false;
  return isPV(llvm::cast<llvm::GlobalVariable>(V));
}

// Creates a predefined varaible defined by the provided ID.
// The predefined variable must not have been created before.
// The reference to the created global variable is returned.
template <enum ID PVID> llvm::GlobalVariable &createPV(llvm::Module &M) {
  auto *Name = getName<PVID>();
  IGC_ASSERT_MESSAGE(M.getNamedGlobal(Name) == nullptr,
                     "Unexpected BSS global already created");
  auto *PVTy = getType<PVID>(M.getContext());
  auto *PV = new llvm::GlobalVariable(M, PVTy, /*isConstant=*/false,
                                      llvm::GlobalValue::ExternalLinkage,
                                      /*Initializer=*/nullptr, Name);
  PV->addAttribute(Attribute);
  return *PV;
}

// Creates BSS predefined variable. Matches \p createPV restrictions.
inline llvm::GlobalVariable &createBSS(llvm::Module &M) {
  return createPV<ID::BSS>(M);
}

// Creates ImplicitArgsBuffer predefined variable. Matches \p createPV
// restrictions.
inline llvm::GlobalVariable &createImplicitArgsBuffer(llvm::Module &M) {
  return createPV<ID::ImplicitArgsBuffer>(M);
}

// Creates LocalIDBuffer predefined variable. Matches \p createPV restrictions.
inline llvm::GlobalVariable &createLocalIDBuffer(llvm::Module &M) {
  return createPV<ID::LocalIDBuffer>(M);
}

} // namespace PredefVar
} // namespace vc

#endif // VC_UTILS_GENX_PREDEFINEDVARIABLE_H
