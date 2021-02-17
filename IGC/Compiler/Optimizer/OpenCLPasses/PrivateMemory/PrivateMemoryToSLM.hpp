/*========================== begin_copyright_notice ============================

Copyright (c) 2010-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

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