/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/DenseSet.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This pass unset fast math flag on all instructions which influence position output
    class SetMathPrecisionForPositionOutput : public llvm::FunctionPass, public llvm::InstVisitor<SetMathPrecisionForPositionOutput>
    {
    public:
        /// @brief  Pass identification.
        static char ID;


        SetMathPrecisionForPositionOutput() : FunctionPass(ID) {}

        ~SetMathPrecisionForPositionOutput() {}

        void visitCallInst(llvm::CallInst& I);

        virtual llvm::StringRef getPassName() const override
        {
            return "SetMathPrecisionForPositionOutput";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }
    private:
        void UpdateDependency(llvm::Instruction* inst);
        llvm::DenseSet<llvm::Instruction*> m_visitedInst;
    };

} // namespace IGC
