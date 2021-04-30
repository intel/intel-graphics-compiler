/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This pass resolves references to inline constants
    class ProgramScopeConstantResolution : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ProgramScopeConstantResolution(bool RunCautiously = false);

        /// @brief  Destructor
        ~ProgramScopeConstantResolution() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ProgramScopeConstantResolutionPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

    private:
        bool RunCautiously;

    };

} // namespace IGC
