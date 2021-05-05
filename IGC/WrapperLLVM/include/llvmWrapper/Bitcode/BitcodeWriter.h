/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_BITCODE_BITCODEWRITER_H
#define IGCLLVM_BITCODE_BITCODEWRITER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvmWrapper/IR/Module.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR == 4
    using llvm::WriteBitcodeToFile;
#elif LLVM_VERSION_MAJOR >= 7
    inline void WriteBitcodeToFile(const llvm::Module *M, llvm::raw_ostream &Out,
        bool ShouldPreserveUseListOrder = false,
        const llvm::ModuleSummaryIndex *Index = nullptr,
        bool GenerateHash = false,
        llvm::ModuleHash *ModHash = nullptr)
    {
        llvm::WriteBitcodeToFile(*M, Out, ShouldPreserveUseListOrder, Index, GenerateHash);
    }
#if LLVM_VERSION_MAJOR > 8
    inline void WriteBitcodeToFile(const IGCLLVM::Module *M, llvm::raw_ostream &Out,
        bool ShouldPreserveUseListOrder = false,
        const llvm::ModuleSummaryIndex *Index = nullptr,
        bool GenerateHash = false,
        llvm::ModuleHash *ModHash = nullptr)
    {
        IGCLLVM::WriteBitcodeToFile((llvm::Module*)M, Out, ShouldPreserveUseListOrder, Index, GenerateHash);
    }
#endif
#endif
}

#endif
