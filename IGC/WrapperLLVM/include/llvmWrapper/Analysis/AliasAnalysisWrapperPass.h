/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_ALIASANALYSISWRAPPERPASS_H
#define IGCLLVM_ANALYSIS_ALIASANALYSISWRAPPERPASS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {

/// Create and return a pass that provides AAResults.
/// This wrapper is needed because LLVM removed the createAAResultsWrapperPass()
/// function in newer versions while keeping the AAResultsWrapperPass class.
llvm::FunctionPass *createAAResultsWrapperPass();

} // end namespace IGCLLVM

#endif // IGCLLVM_ANALYSIS_ALIASANALYSISWRAPPERPASS_H
