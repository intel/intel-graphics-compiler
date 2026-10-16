/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

namespace IGC {

// IGC's defaults for LLVM cl::opt options.
// Precedence (lowest first): these, IGC_LLVMCommandLine, VC -llvm-options.
enum class LLVMOptionScope {
  All, // every frontend and the opt tools
  OCL, // OCL scalar path only (TranslateBuildSPMD)
};

struct LLVMOptionDefault {
  enum Kind { Bool, Unsigned };
  const char *Name;
  Kind Type;      // exactly cl::opt<bool>/cl::opt<unsigned>, else UB
  unsigned Value; // Bool: 0 = false, 1 = true
  LLVMOptionScope Scope;
};

// clang-format off
inline constexpr LLVMOptionDefault LLVMOptionDefaults[] = {
  // Replaces no-instcombine-code-sinking.patch.
  {"instcombine-code-sinking",      LLVMOptionDefault::Bool,     false, LLVMOptionScope::All},
  // LLVM default 8. TODO: UnrollingPreferences::MaxUpperBound when available.
  {"unroll-max-upperbound",         LLVMOptionDefault::Unsigned, 16,    LLVMOptionScope::OCL},
  // LLVM default 250.
  {"licm-mssa-max-acc-promotion",   LLVMOptionDefault::Unsigned, 500,   LLVMOptionScope::OCL},
  // Avoids AA stack overflow; LLVM 14 patch only.
  {"basic-aa-max-query-depth",      LLVMOptionDefault::Unsigned, 192,   LLVMOptionScope::OCL},
  // Pinned: a DSE patch used to disable it (see #159153).
  {"enable-dse-partial-overwrite-tracking", LLVMOptionDefault::Bool, true, LLVMOptionScope::OCL},
  {"dse-memoryssa-walklimit",       LLVMOptionDefault::Unsigned, 150,   LLVMOptionScope::OCL},
  // Keep only -replexitval from IndVarSimplify.
  {"indvars-post-increment-ranges", LLVMOptionDefault::Bool,     false, LLVMOptionScope::OCL},
  {"disable-lftr",                  LLVMOptionDefault::Bool,     true,  LLVMOptionScope::OCL},
  {"indvars-widen-indvars",         LLVMOptionDefault::Bool,     false, LLVMOptionScope::OCL},
  // Exists only up to LLVM 16.
  {"verify-indvars",                LLVMOptionDefault::Bool,     false, LLVMOptionScope::OCL},
};
// clang-format on

// Writes only on change: other threads may be compiling and reading the option.
template <typename T> inline void setLLVMOptionIfDifferent(llvm::cl::Option *Opt, T Value) {
  auto *O = static_cast<llvm::cl::opt<T> *>(Opt);
  if (O->getValue() != Value)
    *O = Value;
}

// Call after user parsing; user-set options are skipped. Assigned, not parsed:
// LLVM 14 rejects a second occurrence. InitializeRegKeys() is compiled out in
// release, so each frontend calls this too.
inline void applyLLVMOptionDefaults(LLVMOptionScope Scope = LLVMOptionScope::All) {
  auto &Options = llvm::cl::getRegisteredOptions();
  for (const auto &D : LLVMOptionDefaults) {
    if (D.Scope != LLVMOptionScope::All && D.Scope != Scope)
      continue;
    llvm::cl::Option *Opt = Options.lookup(D.Name);
    // May be missing in some LLVM versions.
    if (!Opt || Opt->getNumOccurrences() != 0)
      continue;
    // Only parser<bool> accepts a missing value.
    IGC_ASSERT_MESSAGE((Opt->getValueExpectedFlag() == llvm::cl::ValueOptional) == (D.Type == LLVMOptionDefault::Bool),
                       "LLVM option type does not match LLVMOptionDefaults");
    if (D.Type == LLVMOptionDefault::Bool)
      setLLVMOptionIfDifferent<bool>(Opt, D.Value != 0);
    else
      setLLVMOptionIfDifferent<unsigned>(Opt, D.Value);
  }
}

} // namespace IGC
