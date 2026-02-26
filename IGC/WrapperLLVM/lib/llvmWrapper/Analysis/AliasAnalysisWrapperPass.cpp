/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvmWrapper/Analysis/AliasAnalysisWrapperPass.h"

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/AliasAnalysis.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {

llvm::FunctionPass *createAAResultsWrapperPass() { return new llvm::AAResultsWrapperPass(); }

} // end namespace IGCLLVM
