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

#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
    class Constant;
    class ConstantExpr;
    class ConstantVector;
    class Instruction;
    class ConstantStruct;
}

namespace IGC
{
    /// @brief  This pass breaks constant expressions appearing
    ///         in instructions into instruction sequences.
    class BreakConstantExpr : public llvm::FunctionPass
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        BreakConstantExpr();

        /// @brief  Destructor
        ~BreakConstantExpr() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "BreakConstantExprPass";
        }

        /// @brief  Main entry point.
        /// @param  F The destination function.
        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

    protected:
        /// @brief  Recursively break up a constant expression by creating instructions
        ///         for each sub-expression.
        ///         The newly created instructions are placed before the user, and
        ///         replace the constantexprs.
        /// @param  expr          The expression to break up.
        /// @param  user          The original user of the expression.
        void breakExpressions(llvm::ConstantExpr* expr, int operandIndex, llvm::Instruction* user);

        /// @brief  Break up constant expressions in a ConstantVector elements by creating instructions
        ///         for each sub-expression.
        ///         The newly created instructions are placed before the user, and replace all constant
        ///         expressions and the constant vector.
        /// @param  cvec          Constant vector with expressions to break up.
        /// @param  operandIndex  Index of the constant vector operand in the parent instruction
        /// @param  user          The original user of the expression.
        bool breakExpressionsInVector(llvm::ConstantVector* cvec, int operandIndex, llvm::Instruction* user);

        /// @brief  Break up constant structure by creating a non constant
        ///         structure and replacing all its constant operands by instructions.
        /// @param  cs            Constant structure to break up
        /// @param  operandIndex  Index of the constant vector operand in the parent instruction
        /// @param  user          The original user of the expression.
        void breakConstantStruct(llvm::ConstantStruct* cs, int operandIndex, llvm::Instruction* user);

        /// @brief  Replaces input constant expression or constant vector with a new instruction.
        /// @param  exprOrVec     Constant vector or expressions to replace.
        /// @param  newInst       Instruction to replace the constant with
        /// @param  operandIndex  Index of the constant vector operand in the parent instruction
        /// @param  user          The original user of the expression.
        void replaceConstantWith(llvm::Constant* exprOrVec, llvm::Instruction* newInst, int operandIndex, llvm::Instruction* user);
    };

} // namespace IGC
