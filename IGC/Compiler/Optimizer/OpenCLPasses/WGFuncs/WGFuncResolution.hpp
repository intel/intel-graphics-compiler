/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    // A pass that walks over call instructions and replaces all __builtin_IB_work_group
    // with corresponding GenISA intrinsics.
    //
    class WGFuncResolution : public llvm::ModulePass, public llvm::InstVisitor<WGFuncResolution>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        WGFuncResolution();

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "WGFuncResolution";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

        // Entry point of the pass.
        virtual bool runOnModule(llvm::Module& M) override;

        // Call instructions visitor.
        void visitCallInst(llvm::CallInst& callInst);

    private:
        llvm::Module* m_pModule = nullptr;

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed{};
    };

} // namespace IGC
