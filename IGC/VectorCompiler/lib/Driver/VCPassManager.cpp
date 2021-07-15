/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "VCPassManager.h"

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/CommandLine.h>

#include "Probe/Assertion.h"

#include <string>

using namespace llvm;

static cl::opt<std::string> ExtraVerificationAfterPass(
    "vc-run-verifier-after-pass",
    cl::desc("Debug only. Run verifier after the specified pass."));

void VCPassManager::add(Pass *P) {
  IGC_ASSERT(P);

  legacy::PassManager::add(P);

  if (ExtraVerificationAfterPass.empty())
    return;

  const auto *PassInfo = Pass::lookupPassInfo(P->getPassID());
  if (!PassInfo) {
#ifndef NDEBUG
    llvm::errs() << "WARNING: LLVM could not find PassInfo for the <"
                 << P->getPassName() << "> pass! Extra verification pass " <<
                 "won't be injected.\n";
#endif // NDEBUG
    return;
  }

  auto PassArg = PassInfo->getPassArgument();
  if (ExtraVerificationAfterPass == "*" ||
      ExtraVerificationAfterPass == PassArg) {
    llvm::errs() << "extra verifier shall be run after <" << PassArg << ">\n";
    legacy::PassManager::add(createVerifierPass());
  }
}
