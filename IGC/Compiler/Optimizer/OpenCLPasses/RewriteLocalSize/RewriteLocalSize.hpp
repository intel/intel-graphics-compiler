/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class RewriteLocalSize : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        RewriteLocalSize();

        /// @brief  Destructor
        ~RewriteLocalSize() {}

        /// @brief  Provides name of pass
        llvm::StringRef getPassName() const override
        {
            return "RewriteLocalSize";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

        bool runOnModule(llvm::Module& M) override;
    };

} // namespace IGC

