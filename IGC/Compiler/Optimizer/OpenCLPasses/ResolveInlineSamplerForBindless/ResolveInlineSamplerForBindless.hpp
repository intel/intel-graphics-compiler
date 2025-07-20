/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains declaration of ResolveInlineSamplerForBindless llvm pass.
/// This pass searches for __bindless_sampler_initializer(int32) calls and
/// replaces them with implicit arguments for inline samplers added by
/// AddImplicitArgs pass.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include <common/LLVMWarningsPush.hpp>
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <common/LLVMWarningsPop.hpp>

namespace IGC {

class ResolveInlineSamplerForBindless : public llvm::FunctionPass,
                                        public llvm::InstVisitor<ResolveInlineSamplerForBindless> {
public:
  static char ID;

  ResolveInlineSamplerForBindless();
  ~ResolveInlineSamplerForBindless() override = default;

  llvm::StringRef getPassName() const override { return "ResolveInlineSamplerForBindless"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  void visitCallInst(llvm::CallInst &CI);

  bool runOnFunction(llvm::Function &F) override;

private:
  IGC::IGCMD::MetaDataUtils *mMDUtils = nullptr;

  bool mChanged = false;
};

} // namespace IGC
