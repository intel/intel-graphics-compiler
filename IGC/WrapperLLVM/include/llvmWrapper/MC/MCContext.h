/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_MC_MCCONTEXT_H
#define IGCLLVM_MC_MCCONTEXT_H

#include "llvm/Config/llvm-config.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/TargetRegistry.h"

namespace IGCLLVM
{

    inline llvm::MCContext* CreateMCContext(
                       const llvm::Triple &TheTriple, const llvm::MCAsmInfo *MAI, const llvm::MCRegisterInfo *MRI,
                       const llvm::MCObjectFileInfo *MOFI,
                       const llvm::SourceMgr *Mgr = nullptr,
                       llvm::MCTargetOptions const *TargetOpts = nullptr,
                       bool DoAutoReset = true)
   {
#if LLVM_VERSION_MAJOR >= 13
        std::string Err;
        const llvm::Target *T = llvm::TargetRegistry::lookupTarget(TheTriple.str(), Err);
        std::unique_ptr<llvm::MCSubtargetInfo> STI(T->createMCSubtargetInfo(TheTriple.str(), "", ""));
        return new llvm::MCContext(TheTriple, MAI, MRI, STI.get(), Mgr, TargetOpts, DoAutoReset);
#elif LLVM_VERSION_MAJOR >= 10
        return new llvm::MCContext(MAI, MRI, MOFI, Mgr, TargetOpts, DoAutoReset);
#else
        return new llvm::MCContext(MAI, MRI, MOFI, Mgr, DoAutoReset);
#endif
   }
}

#endif
