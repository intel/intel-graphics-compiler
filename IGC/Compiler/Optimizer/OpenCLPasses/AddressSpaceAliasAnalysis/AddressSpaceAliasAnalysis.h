/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/Analysis/AliasAnalysis.h>

namespace IGC {

llvm::ImmutablePass *createAddressSpaceAAWrapperPass();
llvm::ImmutablePass *createIGCExternalAAWrapper();
void addJointAddressSpaceAAResults(llvm::Pass &, llvm::Function &, llvm::AAResults &);

} // namespace IGC

void initializeAddressSpaceAAWrapperPassPass(llvm::PassRegistry &);
void initializeIGCExternalAAWrapperPass(llvm::PassRegistry &);
