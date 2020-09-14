/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

#ifndef IGCLLVM_IR_MODULE_H
#define IGCLLVM_IR_MODULE_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Module.h"
#include "Attributes.h"

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
        inline llvm::Value* getOrInsertFunction(llvm::StringRef Name, llvm::FunctionType *Ty, IGCLLVM::AttributeSet AttributeList)
        {
            return llvm::Module::getOrInsertFunction(Name, Ty, AttributeList).getCallee();
        }
    };
#endif
}

#endif
