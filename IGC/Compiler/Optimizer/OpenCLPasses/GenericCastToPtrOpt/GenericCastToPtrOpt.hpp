/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGC {
/// @brief  Optimizes GenericCastToPtrExplicit calls by replacing them with
///         addrspace_cast instructions.
class GenericCastToPtrOpt : public llvm::ModulePass {
public:
  static char ID;

  GenericCastToPtrOpt();

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  StringRef getPassName() const override { return "GenericCastToPtrOpt"; }

  bool runOnModule(llvm::Module &M) override;
};

} // namespace IGC
