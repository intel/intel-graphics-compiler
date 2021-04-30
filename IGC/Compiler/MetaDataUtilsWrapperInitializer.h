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
    // This pass initializes the MetaDataUtilsWrapper pass with the running module.
    // Useful when not knowing the module when creating the pass (like in igc opt).
    class MetaDataUtilsWrapperInitializer : public llvm::ModulePass
    {
    public:
        static char ID;

        MetaDataUtilsWrapperInitializer();

        ~MetaDataUtilsWrapperInitializer() {}

        // Will use the module to initialize the MetaDataUtils instance from the
        // MetaDataUtilsWrapper pass
        bool runOnModule(llvm::Module& M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

    };

} // namespace IGC
