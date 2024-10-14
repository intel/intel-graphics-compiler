/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_ALIASSETTRACKER_H
#define IGCLLVM_ANALYSIS_ALIASSETTRACKER_H

#include "llvm/Analysis/AliasAnalysis.h"
#include <llvm/Analysis/AliasSetTracker.h>

namespace IGCLLVM
{
  llvm::AliasSetTracker createAliasSetTracker(llvm::AliasAnalysis *aliasAnalysis)
  {
#if LLVM_VERSION_MAJOR < 16
    return llvm::AliasSetTracker(*aliasAnalysis);
#else
    llvm::BatchAAResults batchAAResult(*aliasAnalysis);
    return llvm::AliasSetTracker(batchAAResult);
#endif
  }
}

#endif
