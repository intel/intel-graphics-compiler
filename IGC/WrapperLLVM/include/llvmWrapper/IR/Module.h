/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_MODULE_H
#define IGCLLVM_IR_MODULE_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Attributes.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR <= 8
	using llvm::Module;
#else
	class Module : public llvm::Module
	{
    public:
        Module(llvm::StringRef ModuleID, llvm::LLVMContext& C)
            : llvm::Module(ModuleID, C)
        { }

		inline llvm::Value* getOrInsertFunction(llvm::StringRef Name, llvm::FunctionType *Ty)
		{
            return llvm::Module::getOrInsertFunction(Name, Ty).getCallee();
		}
        inline llvm::Value* getOrInsertFunction(llvm::StringRef Name, llvm::FunctionType *Ty, llvm::AttributeList attributeList)
        {
            return llvm::Module::getOrInsertFunction(Name, Ty, attributeList).getCallee();
        }
    };
#endif
}

#endif
