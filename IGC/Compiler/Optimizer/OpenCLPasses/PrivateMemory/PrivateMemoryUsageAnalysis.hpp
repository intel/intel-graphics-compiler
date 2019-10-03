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

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  PrivateMemoryUsageAnalysis pass used for analyzing if functions use private memory.
    ///         This is done by analyzing the alloca instructions.

    class PrivateMemoryUsageAnalysis : public llvm::ModulePass, public llvm::InstVisitor<PrivateMemoryUsageAnalysis>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        PrivateMemoryUsageAnalysis();

        /// @brief  Destructor
        ~PrivateMemoryUsageAnalysis() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "PrivateMemoryUsageAnalysis";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        ///         Runs on all functions defined in the given module, finds all alloca instructions,
        ///         analyzes them and checks if the functions use private memory.
        ///         If so, private base implicit argument is added to the implicit arguments of the function
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

        /// @brief  Alloca instructions visitor.
        ///         Analyzes if there are private memory allocation.
        /// @param  AI The alloca instruction.
        void visitAllocaInst(llvm::AllocaInst& AI);

        /// @brief  BinaryOperator instructions visitor.
        ///         Analyzes if there are private memory allocation.
        /// @param  I The binary op
        void visitBinaryOperator(llvm::BinaryOperator& I);

        void visitCallInst(llvm::CallInst& CI);

    private:
        /// @brief  Function entry point.
        ///         Finds all alloca instructions in this function, analyzes them and adds
        ///         private base implicit argument if needed.
        /// @param  F The destination function.
        bool runOnFunction(llvm::Function& F);


        /// @brief  A flag signaling if the current function uses private memory
        bool m_hasPrivateMem;
    };

} // namespace IGC
