/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/DenseSet.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
    class Instruction;
    class CallInst;
}

namespace AllocaTracking {

bool processAlloca(
    llvm::Instruction* I,
    bool AllowCapture,
    llvm::SmallVector<llvm::Instruction*, 4>& Insts,
    llvm::DenseSet<llvm::CallInst*>& DeferredInsts);
void rewriteTypes(
    uint32_t NewAddrSpace,
    llvm::SmallVector<llvm::Instruction*, 4>& Insts,
    llvm::DenseSet<llvm::CallInst*>& DeferredInsts);

} // namespace AllocaTracking
