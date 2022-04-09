/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  ExtensionFuncsResolution pass used for resolving VME functions.
    ///         This pass depends on the ExtensionFuncsAnalysis and AddImplicitArgs passes running before it
    class ExtensionFuncsResolution : public llvm::FunctionPass, public llvm::InstVisitor<ExtensionFuncsResolution>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ExtensionFuncsResolution();

        /// @brief  Destructor
        ~ExtensionFuncsResolution() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ExtensionFuncsResolution";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        ///         Finds all VME function calls and resolve them into an llvm sequence
        /// @param  F The destination function.
        virtual bool runOnFunction(llvm::Function& F) override;

        /// @brief  Call instructions visitor.
        ///         Checks for VME functions and resolves them into appropriate sequence of code
        /// @param  CI The call instruction.
        void visitCallInst(llvm::CallInst& CI);

    private:

        /// @brief  The implicit arguments of the current function
        ImplicitArgs m_implicitArgs;

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed;
    };

} // namespace IGC
