/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    class ResolveConstExprCalls : public llvm::ModulePass {
        // Resolves pseudo indirect constexpr cast calls
    public:
        static char ID;
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const {
            AU.setPreservesCFG();
        }

        ResolveConstExprCalls();

        ~ResolveConstExprCalls() {}

        virtual bool runOnModule(llvm::Module& M);

        virtual llvm::StringRef getPassName() const {
            return "ResolveConstExprCalls";
        }
    };

} // namespace IGC
