/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_MODULE_H
#define IGCLLVM_IR_MODULE_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/ADT/StringRef.h"

namespace IGCLLVM
{
    // TODO: Clean up obsolete uses at call sites
    class Module : public llvm::Module
    {
        public:
            Module(llvm::StringRef ModuleID, llvm::LLVMContext& C)
            : llvm::Module(ModuleID, C)
            {}

        // TODO: Delete getOrInsertFunction wrappers
        inline llvm::Value* getOrInsertFunction(llvm::StringRef Name, llvm::FunctionType *Ty)
        {
            return llvm::Module::getOrInsertFunction(Name, Ty).getCallee();
        }
        inline llvm::Value* getOrInsertFunction(llvm::StringRef Name, llvm::FunctionType *Ty, llvm::AttributeList attributeList)
        {
            return llvm::Module::getOrInsertFunction(Name, Ty, attributeList).getCallee();
        }

        // TODO: Refactor to use the LLVM 12+ signature at call sites
        inline llvm::StructType* getTypeByName(llvm::StringRef Name)
        {
#if LLVM_VERSION_MAJOR < 12
            return llvm::Module::getTypeByName(Name);
#else
            return llvm::StructType::getTypeByName(llvm::Module::getContext(), Name);
#endif
        }

    };

    // TODO: Refactor to use the LLVM 12+ signature at call sites
    inline llvm::StructType *getTypeByName(llvm::Module &M, llvm::StringRef Name)
    {
#if LLVM_VERSION_MAJOR < 12
        return M.getTypeByName(Name);
#else
        return llvm::StructType::getTypeByName(M.getContext(), Name);
#endif
    }
}

#endif
