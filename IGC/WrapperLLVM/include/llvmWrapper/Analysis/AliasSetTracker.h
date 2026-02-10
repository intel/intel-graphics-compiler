/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_ALIASSETTRACKER_H
#define IGCLLVM_ANALYSIS_ALIASSETTRACKER_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/AliasAnalysis.h"
#include <llvm/Analysis/AliasSetTracker.h>
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
namespace AliasAnalysis {
#if LLVM_VERSION_MAJOR < 16
using BatchAAResults = llvm::AliasAnalysis *;
#else
using BatchAAResults = llvm::BatchAAResults;
#endif

BatchAAResults createAAresults(llvm::AliasAnalysis *AA) {
#if LLVM_VERSION_MAJOR < 16
  return AA;
#else
  return llvm::BatchAAResults(*AA);
#endif
}
} // end namespace AliasAnalysis

llvm::AliasSetTracker createAliasSetTracker(AliasAnalysis::BatchAAResults AARes) {
#if LLVM_VERSION_MAJOR < 16
  return llvm::AliasSetTracker(*AARes);
#else
  return llvm::AliasSetTracker(AARes);
#endif
}
} // namespace IGCLLVM

#endif
