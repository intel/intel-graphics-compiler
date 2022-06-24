/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_MEMORYLOCATION_H
#define IGCLLVM_ANALYSIS_MEMORYLOCATION_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/MemoryLocation.h"

#if LLVM_VERSION_MAJOR <= 10
#include "llvm/IR/CallSite.h"
#else
#include "llvm/IR/Instructions.h"
#endif

namespace IGCLLVM
{

    class MemoryLocation : public llvm::MemoryLocation
    {
    public:
        static inline llvm::MemoryLocation getForArgument(
            llvm::Instruction* I, unsigned ArgIdx,
            const llvm::TargetLibraryInfo* TLI)
        {
            return llvm::MemoryLocation::getForArgument(
#if LLVM_VERSION_MAJOR <= 7
                llvm::ImmutableCallSite(I), ArgIdx, *TLI
#elif LLVM_VERSION_MAJOR >= 8
                llvm::cast<llvm::CallInst>(I), ArgIdx, TLI
#endif
            );
        }
    };

    class LocationSize : public llvm::LocationSize
    {
    public:
#if LLVM_VERSION_MAJOR < 12
        // LLVM 12 introduced changes in LocationSize available parameters.
        // "Unknown" memory location changed name to "BeforeOrAfterPointer"
        // having the same value ~uint64_t(0).
        //
        //     Differential revision: https://reviews.llvm.org/D91649
        //
        constexpr static llvm::LocationSize beforeOrAfterPointer() {
            return llvm::LocationSize::unknown();
        }
#endif
    };
}

#endif
