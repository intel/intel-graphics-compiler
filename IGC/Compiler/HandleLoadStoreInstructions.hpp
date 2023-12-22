/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This pass converts store/load on doubles into store/loads on i32 or float types.
    class HandleLoadStoreInstructions : public llvm::FunctionPass, public llvm::InstVisitor<HandleLoadStoreInstructions, void>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        HandleLoadStoreInstructions();

        /// @brief  Destructor
        ~HandleLoadStoreInstructions() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "HandleLoadStoreInstructionsPass";
        }

        /// @brief  Main entry point.
        /// @param  F The destination function.
        virtual bool runOnFunction(llvm::Function& F) override;
        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
        }

    protected:
        void HandleLoadStore();

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed = false;
    };

} // namespace IGC
