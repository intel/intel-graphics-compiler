/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_CM_CL_SUPPORT_TRANSLATOR_PASS_H
#define VC_CM_CL_SUPPORT_TRANSLATOR_PASS_H

#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

// A pass that translates CM-CL builtins into legal for VC backend IR
// (e.g. into vc-intrinsics).
namespace llvm {
void initializeCMCLTranslatorPass(PassRegistry &);
ModulePass *createCMCLTranslatorPass();
} // namespace llvm

#endif // VC_CM_CL_SUPPORT_TRANSLATOR_PASS_H
