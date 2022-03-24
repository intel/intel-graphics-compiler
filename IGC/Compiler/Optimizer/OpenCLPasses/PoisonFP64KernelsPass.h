/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class PoisonFP64Kernels : public llvm::CallGraphSCCPass
    {
    public:
        static char ID;

        PoisonFP64Kernels();

        virtual llvm::StringRef getPassName() const override
        {
            return "Poison FP64 Kernels";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        virtual bool doInitialization(llvm::CallGraph &CG) override;
        virtual bool runOnSCC(llvm::CallGraphSCC &SCC) override;
        virtual bool doFinalization(llvm::CallGraph &CG) override;

        void markForRemoval(llvm::Function *F);

    private:
        /* Use set as an index of found FP64 functions. Store the order in
         * which they were encountered in sequential container. Since this is a
         * CallGraphSCC pass the reverse of this order should give us the safe
         * removal order (from callers to callees) */
        llvm::SmallPtrSet<llvm::Function *, 8> fp64Functions;
        llvm::SmallVector<llvm::Function *, 8> fp64FunctionsOrder;
    };

} // namespace IGC
