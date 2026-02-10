/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DIBuilder.h"

namespace IGC {
namespace Utils {

#define __OCL_DBG_VARIABLES 9

/// @brief return true if given module contain debug info
/// @param M The LLVM module.
/// @return true if given module contain debug info
bool HasDebugInfo(llvm::Module &M);

/// @brief creates a new call instruction to llvm.dbg.value intrinsic with
///        same information as in debug info of given global variable and
///        with value set to new given value.
/// @param pGlobalVar global variable to handle its debug info
/// @param pNewVal new value to map to the source variable (in the debug info)
/// @param pEntryPoint entry point instruction to add new instructions before.
/// @isIndirect true iff pNewValue type is a pointer to source variable type.
/// @return new call instruction to llvm.dbg.value intrinsic
llvm::Instruction *UpdateGlobalVarDebugInfo(llvm::GlobalVariable *pGlobalVar, llvm::Value *pNewVal,
                                            llvm::Instruction *pEntryPoint, bool isIndirect);

} // namespace Utils
} // namespace IGC
