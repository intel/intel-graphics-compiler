/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_LEGACY_PASS_MANAGERS_H
#define IGCLLVM_LEGACY_PASS_MANAGERS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/LegacyPassManagers.h"

namespace IGCLLVM
{
    class PMDataManager : public llvm::PMDataManager
    {
    public:
      inline unsigned initSizeRemarkInfo(llvm::Module &M,
                                  llvm::StringMap<std::pair<unsigned, unsigned>> &FunctionToInstrCount)
        {
#if LLVM_VERSION_MAJOR == 7
            return llvm::PMDataManager::initSizeRemarkInfo(M);
#else
            return llvm::PMDataManager::initSizeRemarkInfo(M, FunctionToInstrCount);
#endif
        }

      inline void emitInstrCountChangedRemark(llvm::Pass *P, llvm::Module &M, int64_t Delta, unsigned CountBefore,
                                       llvm::StringMap<std::pair<unsigned, unsigned>> &FunctionToInstrCount)
        {
#if LLVM_VERSION_MAJOR == 7
            llvm::PMDataManager::emitInstrCountChangedRemark(P, M, CountBefore);
#else
            llvm::PMDataManager::emitInstrCountChangedRemark(P, M, Delta, CountBefore, FunctionToInstrCount);
#endif
        }
    };
}

#endif
