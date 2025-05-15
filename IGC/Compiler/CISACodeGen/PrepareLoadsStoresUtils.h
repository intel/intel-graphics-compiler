/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

#include "common/IGCIRBuilder.h"
#include "MemOptUtils.h"

namespace IGC {
    template <typename T>
    std::pair<llvm::Value*, llvm::Instruction*> expand64BitLoad(
        llvm::IGCIRBuilder<T> &IRB,
        const llvm::DataLayout &DL,
        llvm::LoadInst* LI);
    template <typename T>
    std::pair<llvm::Value*, llvm::Instruction*> expand64BitLoad(
        llvm::IGCIRBuilder<T> &IRB,
        const llvm::DataLayout &DL,
        ALoadInst LI);
    template <typename T>
    llvm::Instruction* expand64BitStore(
        llvm::IGCIRBuilder<T> &IRB,
        const llvm::DataLayout &DL,
        llvm::StoreInst* SI);
    template <typename T>
    llvm::Instruction* expand64BitStore(
        llvm::IGCIRBuilder<T> &IRB,
        const llvm::DataLayout &DL,
        AStoreInst SI);
} // namespace IGC
