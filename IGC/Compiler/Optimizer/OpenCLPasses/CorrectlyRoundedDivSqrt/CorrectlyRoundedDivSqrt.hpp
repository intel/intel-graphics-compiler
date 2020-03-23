/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
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
        bool m_changed;

        /// @brief  Indicates that correctly rounded sqrt/div should be used even
        ///         when the option is not present in the module metadata.
        bool m_forceCR;

        bool m_hasHalfTy;

        bool m_IsCorrectlyRounded;

        llvm::Module* m_module = nullptr;

    };

} // namespace IGC
