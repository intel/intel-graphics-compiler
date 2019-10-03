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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  ExtensionFuncsAnalysis pass used for analyzing if VME functions are used in the
    ///         different functions in the module and creating metadata that represents
    ///         the implicit information needed by each function for resolving these function calls

    class ExtensionFuncsAnalysis : public llvm::ModulePass, public llvm::InstVisitor<ExtensionFuncsAnalysis>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ExtensionFuncsAnalysis();

        /// @brief  Destructor
        ~ExtensionFuncsAnalysis() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ExtensionFuncsAnalysis";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        /// @brief  Main entry point.
        ///         Runs on all functions defined in the given module, finds all VME function calls,
        ///         analyzes them and creates metadata that represents the implicit information needed
        ///         by each function for resolving these function calls
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

        /// @brief  Function entry point.
        ///         Finds all VME function calls in this function, analyzes them and creates
        ///         metadata that represents the implicit information needed by this function
        ///         for resolving these function calls
        /// @param  F The destination function.
        bool runOnFunction(llvm::Function& F);

        /// @brief  Call instructions visitor.
        ///         Checks for VME functions and analyzes it
        /// @param  CI The call instruction.
        void visitCallInst(llvm::CallInst& CI);

        static const llvm::StringRef VME_MB_BLOCK_TYPE;
        static const llvm::StringRef VME_SUBPIXEL_MODE;
        static const llvm::StringRef VME_SAD_ADJUST_MODE;
        static const llvm::StringRef VME_SEARCH_PATH_TYPE;
        static const llvm::StringRef VME_HELPER_GET_HANDLE;
        static const llvm::StringRef VME_HELPER_GET_AS;

    private:
        /// @brief  Marks whether VME information is needed by the current function
        bool m_hasVME;
    };

} // namespace IGC

