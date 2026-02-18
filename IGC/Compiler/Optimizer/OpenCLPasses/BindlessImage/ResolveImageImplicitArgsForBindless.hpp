/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of ResolveImageImplicitArgsForBindless
/// llvm pass.
///
/// This pass searches for built-in calls querying for image properties and
/// replaces them with calls to GenISA_ldraw_indexed intrinsic to fetch the
/// image properties from the ImageImplicitArgs struct in bindless mode.
///
//===----------------------------------------------------------------------===//

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include <common/LLVMWarningsPush.hpp>
#include <llvm/ADT/SetVector.h>
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <common/LLVMWarningsPop.hpp>

namespace IGC {
class CPlatform;

class ResolveImageImplicitArgsForBindless : public llvm::ModulePass,
                                            public llvm::InstVisitor<ResolveImageImplicitArgsForBindless> {
public:
  static char ID;

  ResolveImageImplicitArgsForBindless();
  ~ResolveImageImplicitArgsForBindless() override = default;

  llvm::StringRef getPassName() const override { return "ResolveImageImplicitArgsForBindless"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CodeGenContextWrapper>();
  }

  void visitCallInst(llvm::CallInst &CI);

  bool runOnModule(llvm::Module &M) override;

private:
  CDriverInfo const *mDriverInfo = nullptr;
  llvm::SmallSetVector<llvm::Instruction *, 16> mInstsToRemove;

  bool mChanged = false;
};

} // namespace IGC
