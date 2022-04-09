/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtensionFuncs/ExtensionArgAnalysis.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief This pass allocates UAV and SRV numbers to kernel arguments.
    class ResourceAllocator : public llvm::ModulePass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ResourceAllocator();

        /// @brief  Destructor
        ~ResourceAllocator() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ResourceAllocatorPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<ExtensionArgAnalysis>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

    protected:

        bool runOnFunction(llvm::Function& F);
    };

} // namespace IGC
