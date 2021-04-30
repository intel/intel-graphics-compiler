/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This pass removes volatile markers from loads and stores
    class VolatileWorkaround : public llvm::FunctionPass, public llvm::InstVisitor<VolatileWorkaround>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        VolatileWorkaround();

        /// @brief  Destructor
        ~VolatileWorkaround() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "VolatileWorkaroundPass";
        }

        /// @brief  Main entry point.
        /// @param  F The destination function.
        virtual bool runOnFunction(llvm::Function& F) override;

        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);
    };

} // namespace IGC
