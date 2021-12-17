/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
    std::pair<llvm::Value*, llvm::LoadInst*> expand64BitLoad(
        llvm::IRBuilder<> &IRB,
        const llvm::DataLayout &DL,
        llvm::LoadInst* LI);
    llvm::StoreInst* expand64BitStore(
        llvm::IRBuilder<> &IRB,
        const llvm::DataLayout &DL,
        llvm::StoreInst* SI);
} // namespace IGC
