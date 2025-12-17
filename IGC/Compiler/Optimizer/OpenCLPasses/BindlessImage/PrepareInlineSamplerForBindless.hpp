/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains declaration of PrepareInlineSamplerForBindless llvm pass.
/// This pass searches for __bindless_sampler_initializer(int32) calls, maps
/// them to inline sampler implicit args that will be added in AddImplicitArgs
/// pass and creates metadata for the inline samplers.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include <common/LLVMWarningsPush.hpp>
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <common/LLVMWarningsPop.hpp>

namespace IGC {

class PrepareInlineSamplerForBindless : public llvm::FunctionPass,
                                        public llvm::InstVisitor<PrepareInlineSamplerForBindless> {
public:
  static char ID;

  PrepareInlineSamplerForBindless();
  ~PrepareInlineSamplerForBindless() override = default;

  llvm::StringRef getPassName() const override { return "PrepareInlineSamplerForBindless"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  void visitCallInst(llvm::CallInst &CI);

  bool runOnFunction(llvm::Function &F) override;

private:
  IGC::IGCMD::MetaDataUtils *mMDUtils = nullptr;

  ImplicitArg::ArgMap mArgMap{};

  int mInlineSamplerIndex = 0;

  bool mChanged = false;
};

} // namespace IGC
