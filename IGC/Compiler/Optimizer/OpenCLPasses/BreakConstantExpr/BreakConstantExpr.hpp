/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
        bool breakConstantStruct(llvm::ConstantStruct* cs, int operandIndex, llvm::Instruction* user);

        /// @brief  Replaces input constant expression or constant vector with a new instruction.
        /// @param  exprOrVec     Constant vector or expressions to replace.
        /// @param  newInst       Instruction to replace the constant with
        /// @param  operandIndex  Index of the constant vector operand in the parent instruction
        /// @param  user          The original user of the expression.
        void replaceConstantWith(llvm::Constant* exprOrVec, llvm::Instruction* newInst, int operandIndex, llvm::Instruction* user);
    private:
        bool hasConstantExpr(llvm::ConstantVector* cvec) const;
        bool hasConstantExpr(llvm::ConstantStruct* cstruct) const;
    };

} // namespace IGC
