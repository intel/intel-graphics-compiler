/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  PrivateMemoryUsageAnalysis pass used for analyzing if functions use private memory.
    ///         This is done by analyzing the alloca instructions.

    class PrivateMemoryUsageAnalysis : public llvm::ModulePass, public llvm::InstVisitor<PrivateMemoryUsageAnalysis>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        PrivateMemoryUsageAnalysis();

        /// @brief  Destructor
        ~PrivateMemoryUsageAnalysis() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "PrivateMemoryUsageAnalysis";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        ///         Runs on all functions defined in the given module, finds all alloca instructions,
        ///         analyzes them and checks if the functions use private memory.
        ///         If so, private base implicit argument is added to the implicit arguments of the function
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

        /// @brief  Alloca instructions visitor.
        ///         Analyzes if there are private memory allocation.
        /// @param  AI The alloca instruction.
        void visitAllocaInst(llvm::AllocaInst& AI);

        /// @brief  BinaryOperator instructions visitor.
        ///         Analyzes if there are private memory allocation.
        /// @param  I The binary op
        void visitBinaryOperator(llvm::BinaryOperator& I);

        /// @brief  CallInst instructions visitor.
        ///         Analyzes if there are private memory allocation.
        /// @param  CI The binary op
        void visitCallInst(llvm::CallInst& CI);

    private:
        /// @brief  Function entry point.
        ///         Finds all alloca instructions in this function, analyzes them and adds
        ///         private base implicit argument if needed.
        /// @param  F The destination function.
        bool runOnFunction(llvm::Function& F);

        /// @brief  A flag signaling if the current function uses private memory
        bool m_hasPrivateMem;

        /// @brief A flag signaling if the platform has partial fp64 emulation
        bool m_hasDPDivSqrtEmu = false;

        /// @brief  MetaData utils used to generate LLVM metadata
        IGCMD::MetaDataUtils* m_pMDUtils;
    };

} // namespace IGC
