/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/Types.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class CodeGenContext;
    class CBTILayout;
    class CPlatform;
    class CDriverInfo;
    // This pass provides access to  CodeGenContext.
    //
    // To use this from within another pass:
    //  1. Add CodeGenContextWrapper to the pass manager.
    //  2. Use getAnalysisIfAvailable on CodeGenContextWrapper:
    //       CodeGenContextWrapper* pCtxWrapper = getAnalysis<CodeGenContextWrapper>();
    //  3. Get the CodeGenContext:
    //      CodeGenContext* ctx = pCtxWrapper->getCodeGenContext();

    class CodeGenContextWrapper : public llvm::ImmutablePass
    {
    public:
        static char ID;

        // Constructs a wrapper to the given CodeGenContext instance.
        CodeGenContextWrapper(CodeGenContext* pCtx);
        CodeGenContextWrapper();

        // return the Context
        CodeGenContext* getCodeGenContext();

        virtual llvm::StringRef getPassName() const override
        {
            return "CodeGen Context Wrapper";
        }

    private:
        CodeGenContext* m_ctx;
    };

} // namespace IGC
