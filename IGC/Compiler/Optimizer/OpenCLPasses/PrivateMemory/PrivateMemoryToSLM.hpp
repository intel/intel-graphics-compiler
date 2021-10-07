/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/ModuleAllocaAnalysis.hpp"

#include <llvm/Pass.h>

using namespace llvm;

namespace IGC
{
    // Experimental pass to move private memory allocations to SLM where it's
    // profitable. The pass is able to handle Compute and OpenCL shader types.
    class PrivateMemoryToSLM : public ModulePass
    {

    public:
        static char ID;

        PrivateMemoryToSLM(bool enableOptReport = false);
        PrivateMemoryToSLM(
            std::string forcedBuffers,
            bool enableOptReport);
        ~PrivateMemoryToSLM() {}

        virtual StringRef getPassName() const override
        {
            return "PrivateMemoryToSLMPass";
        }

        virtual void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<ModuleAllocaAnalysis>();
        }

        virtual bool runOnModule(Module& M) override;

        static const unsigned int VALID_LOCAL_HIGH_BITS;
        static const unsigned int SLM_LOCAL_VARIABLE_ALIGNMENT;
        static const unsigned int SLM_LOCAL_SIZE_ALIGNMENT;

    private:
        bool m_EnableOptReport;
        bool m_ForceAll;
        std::vector<std::string> m_ForcedBuffers;
    };
}
