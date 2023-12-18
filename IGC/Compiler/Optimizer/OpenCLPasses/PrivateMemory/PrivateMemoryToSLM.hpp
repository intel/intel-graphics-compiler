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
    // It is convenient to represent the null pointer as the zero
    // bit-pattern. However, SLM address 0 is legal, and we want to be able
    // to use it.
    // To go around this, we use the fact only the low 16 bits ("low nibble")
    // of SLM addresses are significant, and set all valid pointers to have
    // a non-zero high nibble.
    static inline const unsigned int VALID_LOCAL_HIGH_BITS = 0x10000000;

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

        static const unsigned int SLM_LOCAL_VARIABLE_ALIGNMENT;
        static const unsigned int SLM_LOCAL_SIZE_ALIGNMENT;

    private:
        bool m_EnableOptReport;
        bool m_ForceAll;
        std::vector<std::string> m_ForcedBuffers;
    };
}
