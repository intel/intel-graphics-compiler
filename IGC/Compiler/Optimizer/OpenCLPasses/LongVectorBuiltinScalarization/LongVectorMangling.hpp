/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace IGC {
namespace LongVectorMangling {

struct MangledType;
using MangledTypeRef = std::shared_ptr<const MangledType>;

struct MangledType {
  enum class Kind { Builtin, Named, Vector, Pointer, Qualified };
  Kind K;
  // Builtin: type code ("f", "Dh"). Named: identifier. Pointer: "P".
  // Qualified: qualifier prefix ("U3AS1K").
  std::string Text;
  unsigned NumElements = 0;
  MangledTypeRef Inner;

  /// Mangling of the type with all substitutions expanded. Two types are the
  /// same substitution candidate iff their canonical strings are equal.
  std::string canonical() const;
};

/// A free function mangled per the Itanium C++ ABI the way clang mangles
/// OpenCL C builtins: `_Z <source-name> <parameter types>`.
struct MangledFunction {
  std::string Name;
  std::vector<MangledTypeRef> Params;
};

/// Parses the subset of the Itanium grammar used by SPIR-V friendly builtin
/// names. Returns std::nullopt for anything outside of it (nested names,
/// template arguments, std:: abbreviations).
std::optional<MangledFunction> demangle(llvm::StringRef MangledName);

/// Re-emits \p F with substitutions recomputed the way clang assigns them,
/// so the result matches the BiF symbol for the same signature.
std::string mangle(const MangledFunction &F);

/// Replaces every <N x T> parameter type (also behind pointers and
/// qualifiers) with T, and drops the width from a `_R<type><N>` return type
/// suffix in the name.
MangledFunction scalarize(const MangledFunction &F, unsigned NumElements);

/// The builtin name without its `_R<type>...` return type suffix, e.g.
/// `__spirv_ocl_vloadn` for `__spirv_ocl_vloadn_Rfloat5`.
llvm::StringRef getBaseName(llvm::StringRef Name);

} // namespace LongVectorMangling
} // namespace IGC
