/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/DenseSet.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This pass unset fast math flag on all instructions which influence position output
    class PositionDepAnalysis : public llvm::FunctionPass, public llvm::InstVisitor<PositionDepAnalysis>
    {
    public:
        /// @brief  Pass identification.
        static char ID;

        PositionDepAnalysis();

        ~PositionDepAnalysis() {}

        void visitCallInst(llvm::CallInst& I);

        virtual llvm::StringRef getPassName() const override
        {
            return "SetMathPrecisionForPositionOutput";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesAll();
            AU.addRequired<CodeGenContextWrapper>();
        }
        bool PositionDependsOnInst(llvm::Instruction* inst);
    private:
        void UpdateDependency(llvm::Instruction* inst);
        llvm::DenseSet<llvm::Instruction*> m_PositionDep;
        bool m_newURBEncoding = false;
    };

} // namespace IGC
