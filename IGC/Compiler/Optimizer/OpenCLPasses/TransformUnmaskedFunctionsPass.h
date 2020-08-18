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

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    // Checks if function can be unmased and transforms the attributes of
    // function definition and each call site
    class TransformUnmaskedFunctionsPass final : public llvm::FunctionPass
    {
    public:
        static char ID;

        TransformUnmaskedFunctionsPass();
        ~TransformUnmaskedFunctionsPass() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "TransformUnmaskedFunctionsPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.addRequired<IGC::CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

    private:
        ModuleMetaData* MMD;
    };

    // Inlines functions marked as unmasked correclty modifying all related metadata.
    class InlineUnmaskedFunctionsPass final : public llvm::ModulePass
    {
    public:
        static char ID;

        InlineUnmaskedFunctionsPass();
        ~InlineUnmaskedFunctionsPass() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "InlineUnmaskedFunctionsPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.addRequired<IGC::CodeGenContextWrapper>();
        }

        virtual bool runOnModule(llvm::Module& M) override;

    private:
        ModuleMetaData* MMD;
    };
};
