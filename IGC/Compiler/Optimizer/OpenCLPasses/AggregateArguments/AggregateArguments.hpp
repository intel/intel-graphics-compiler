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

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class AggregateArgumentsAnalysis : public llvm::ModulePass
    {
    public:

        // Pass identification, replacement for typeid
        static char ID;

        AggregateArgumentsAnalysis();

        ~AggregateArgumentsAnalysis() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "AggregateArgumentsAnalysis";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        virtual bool runOnModule(llvm::Module& M) override;

    private:
        void addImplictArgs(llvm::Type* type, uint64_t baseAllocaOffset);

    private:
        const llvm::DataLayout* m_pDL;
        ImplicitArg::StructArgList m_argList;
        IGCMD::MetaDataUtils* m_pMdUtils;

    };

    class ResolveAggregateArguments : public llvm::FunctionPass
    {
    public:

        // Pass identification, replacement for typeid
        static char ID;

        ResolveAggregateArguments();

        ~ResolveAggregateArguments() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "ResolveAggregateArguments";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

    private:
        void storeArgument(const llvm::Argument*, llvm::AllocaInst* base, llvm::IRBuilder<>& irBuilder);

        void getImplicitArg(unsigned int explicitArgNo, unsigned int& startArgNo, unsigned int& endArgNo);

    protected:
        llvm::Function* m_pFunction;

        ImplicitArgs m_implicitArgs;
    };

} // namespace IGC
