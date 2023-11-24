/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"


namespace IGC
{
    bool isLegalOCLVersion(int major, int minor);

    // This pass Translates SPIR metadata to IGC metadata
    class SPIRMetaDataTranslation : public llvm::ModulePass
    {
    public:
        static char ID;

        SPIRMetaDataTranslation();

        ~SPIRMetaDataTranslation() {}

        void translateKernelMetadataIntoOpenCLKernelsMD(llvm::Module& M);
        bool runOnModule(llvm::Module& M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

    };

} // namespace IGC
