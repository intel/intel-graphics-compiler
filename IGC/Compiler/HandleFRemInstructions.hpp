/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"


namespace IGC
{
    // This pass replaces all occurences of frem instructions with proper builtin calls
    // This is needed, because new SPIRV-LLVM translator outputs frem instructions
    // which are not fully handled by IGC.
    class HandleFRemInstructions : public llvm::ModulePass, public llvm::InstVisitor<HandleFRemInstructions>
    {
    public:
        static char ID;

        HandleFRemInstructions();

        /// @brief Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "HandleFremInstructions";
        }

        /// @brief Main entry point.
        ///        Find all frem instructions and replace them with proper builtin calls
        /// @param M The destination module.
        bool runOnModule(llvm::Module& M) override;

        void visitFRem(llvm::BinaryOperator& I);

    private:
        llvm::Module* m_module = nullptr;
        bool m_changed = false;
    };
}
