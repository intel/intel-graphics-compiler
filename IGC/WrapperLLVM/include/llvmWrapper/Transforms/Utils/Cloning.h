/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_CLONING_H
#define IGCLLVM_TRANSFORMS_UTILS_CLONING_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Transforms/Utils/Cloning.h"

namespace IGCLLVM
{
    inline bool InlineFunction(llvm::CallInst* CB, llvm::InlineFunctionInfo& IFI,
        llvm::AAResults* CalleeAAR = nullptr,
        bool InsertLifetime = true,
        llvm::Function* ForwardVarArgsTo = nullptr)
    {
        return llvm::InlineFunction(
#if LLVM_VERSION_MAJOR < 11
            CB
#else
            *llvm::dyn_cast<llvm::CallBase>(CB)
#endif
            , IFI, CalleeAAR, InsertLifetime, ForwardVarArgsTo)
#if LLVM_VERSION_MAJOR >= 11
            .isSuccess()
#endif
            ;
    }

#if LLVM_VERSION_MAJOR >= 13
    inline void CloneFunctionInto(llvm::Function *NewFunc, const llvm::Function *OldFunc,
                       llvm::ValueToValueMapTy &VMap, bool ModuleLevelChanges,
                       llvm::SmallVectorImpl<llvm::ReturnInst *> &Returns,
                       const char *NameSuffix = "",
                       llvm::ClonedCodeInfo *CodeInfo = nullptr,
                       llvm::ValueMapTypeRemapper *TypeMapper = nullptr,
                       llvm::ValueMaterializer *Materializer = nullptr)
    {
        // TODO: True/false ModuleLevelChanges may not exactly map to GlobalChanges & DifferentModule
        llvm::CloneFunctionChangeType Changes = ModuleLevelChanges ?
            llvm::CloneFunctionChangeType::DifferentModule :
            llvm::CloneFunctionChangeType::GlobalChanges;

        llvm::CloneFunctionInto(NewFunc, OldFunc, VMap, Changes, Returns, NameSuffix, CodeInfo, TypeMapper, Materializer);
    }
#else
    using llvm::CloneFunctionInto;
#endif
}

#endif
