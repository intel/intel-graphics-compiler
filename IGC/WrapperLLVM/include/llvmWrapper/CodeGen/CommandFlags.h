/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_CODEGEN_COMMANDFLAGS_H
#define IGCLLVM_CODEGEN_COMMANDFLAGS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/CodeGen/CommandFlags.h"

namespace IGCLLVM
{
namespace codegen
{
    llvm::TargetOptions
        InitTargetOptionsFromCodeGenFlags(const llvm::Triple& TheTriple)
    {
#if LLVM_VERSION_MAJOR <= 11
        return llvm::codegen::InitTargetOptionsFromCodeGenFlags();
#else
        return llvm::codegen::InitTargetOptionsFromCodeGenFlags(TheTriple);
#endif
    }
} // namespace codegen
} // namespace IGCLLVM

#endif // IGCLLVM_CODEGEN_COMMANDFLAGS_H