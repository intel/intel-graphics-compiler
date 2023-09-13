/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_LOCAL_H
#define IGCLLVM_TRANSFORMS_UTILS_LOCAL_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/ValueHandle.h"

namespace IGCLLVM
{
    /// In LLVM 11 first argument changed type from SmallVectorImpl<Instruction*> to SmallVectorImpl<WeakTrackingVH>
    /// For LLVM >= 11: Proxy llvm:: call.
    /// For LLVM < 11: Unpack WeakTrackingVH and call normally.
    inline void RecursivelyDeleteTriviallyDeadInstructions(
        llvm::SmallVectorImpl<llvm::WeakTrackingVH>& DeadInsts,
        const llvm::TargetLibraryInfo*               TLI = nullptr,
        llvm::MemorySSAUpdater*                      MSSAU = nullptr)
    {
        using namespace llvm;

#if LLVM_VERSION_MAJOR < 11
        SmallVector<Instruction*, 8> instPtrsVector = SmallVector<Instruction*, 8>();

        // Unpack items of type 'WeakTrackingVH' to 'Instruction*'
        for (unsigned i = 0; i < DeadInsts.size(); i++)
        {
            Value* tmpVecItem = DeadInsts[i]; // Using 'WeakTrackingVH::operator Value*()'
            instPtrsVector.push_back(cast<Instruction>(tmpVecItem));
        }

        llvm::RecursivelyDeleteTriviallyDeadInstructions(instPtrsVector, TLI, MSSAU);
#else
        llvm::RecursivelyDeleteTriviallyDeadInstructions(DeadInsts, TLI, MSSAU);
#endif
    }
}
#endif
