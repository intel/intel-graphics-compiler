/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvmWrapper/Analysis/AliasAnalysis.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    llvm::ImmutablePass* createAddressSpaceAAWrapperPass();
    void addAddressSpaceAAResult(llvm::Pass&, llvm::Function&, llvm::AAResults&);

} // End IGC namespace

void initializeAddressSpaceAAWrapperPassPass(llvm::PassRegistry&);
