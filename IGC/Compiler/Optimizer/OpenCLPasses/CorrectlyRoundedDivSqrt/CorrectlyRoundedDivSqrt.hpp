/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  if the proper compiler option is present, makes sure usages of single precision floating point
    ///         sqrt and divide are IEEE compliant, otherwise does nothing.
    ///         Achieved by replacing div and sqrt with calls to compliant versions.
    class CorrectlyRoundedDivSqrt : public llvm::ModulePass, public llvm::InstVisitor<CorrectlyRoundedDivSqrt>
    {
    public:
        /// @brief  Pass identification.
        static char ID;

        CorrectlyRoundedDivSqrt();

        CorrectlyRoundedDivSqrt(bool forceCR, bool HasHalf);

        ~CorrectlyRoundedDivSqrt() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "CorrectlyRoundedDivSqrt";
        }

        virtual bool runOnModule(llvm::Module& M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        /// @brief  replace given divide instruction with a call to a correctly rounded version.
        /// @param  I - the fdiv instruction.
        void visitFDiv(llvm::BinaryOperator& I);

    private:
        /// @brief  if the given function is a sqrt function, replace the declaration with a correctly rounded version.
        /// @param  F - the function.
        /// @return true if the function was changed.
        static bool processDeclaration(llvm::Function& F);

        llvm::Value* emitIEEEDivide(llvm::BinaryOperator* I, llvm::Value* Op0, llvm::Value* Op1);

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed = false;

        /// @brief  Indicates that correctly rounded sqrt/div should be used even
        ///         when the option is not present in the module metadata.
        bool m_forceCR;

        bool m_hasHalfTy;

        bool m_IsCorrectlyRounded;

        llvm::Module* m_module = nullptr;

    };

} // namespace IGC
