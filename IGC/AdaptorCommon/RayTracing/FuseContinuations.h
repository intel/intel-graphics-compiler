/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"
#include "MDFrameWork.h"

namespace ContinuationFusing {

struct FuncInfo
{
    uint32_t Idx = 0;
    llvm::Function* RootFn = nullptr;
    IGC::CallableShaderTypeMD ShaderTy = IGC::NumberOfCallableShaderTypes;
};

void fuseContinuations(
    llvm::Module& M,
    llvm::MapVector<llvm::Function*, FuncInfo>& ContMap);

} // namespace ContinuationFusing
