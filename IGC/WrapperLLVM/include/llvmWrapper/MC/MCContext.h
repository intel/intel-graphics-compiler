/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_MC_MCCONTEXT_H
#define IGCLLVM_MC_MCCONTEXT_H

#include "llvm/Config/llvm-config.h"
#include "llvm/MC/MCContext.h"
#include "llvmWrapper/Support/TargetRegistry.h"

namespace IGCLLVM {

inline llvm::MCContext *CreateMCContext(const llvm::Triple &TheTriple, const llvm::MCAsmInfo *MAI,
                                        const llvm::MCRegisterInfo *MRI, const llvm::MCObjectFileInfo *MOFI,
                                        const llvm::SourceMgr *Mgr = nullptr,
                                        llvm::MCTargetOptions const *TargetOpts = nullptr, bool DoAutoReset = true) {
  // Refactor MCObjectFileInfo initialization and allow targets to create MCObjectFileInfo
  //
  //      Differential Revision: https://reviews.llvm.org/D101921

  auto *Context = new llvm::MCContext(TheTriple, MAI, MRI, nullptr, Mgr, TargetOpts, DoAutoReset);
  Context->setObjectFileInfo(MOFI);
  return Context;
}
} // namespace IGCLLVM

#endif
