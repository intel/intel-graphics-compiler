/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

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
    /// @brief  Pass to analyze if scalar (byval) kernel argument is used for access to global memory.
    ///         This includes explicit scalar arguments casted to pointers:
    ///
    ///             kernel void f(long addr) {
    ///                 *((global int*) addr) += 1;
    ///             }
    ///
    ///         Or pointers in structs (implicit arguments):
    ///
    ///             struct S { int* ptr };
    ///             kernel void f(struct S s) {
    ///                 *(s.ptr) += 1;
    ///             }
    ///
    ///         To parse implicit arguments this pass must be run after ResolveAggregateArguments.

    class ScalarArgAsPointerAnalysis : public llvm::ModulePass, public llvm::InstVisitor<ScalarArgAsPointerAnalysis>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        ScalarArgAsPointerAnalysis();

        /// @brief  Destructor
        ~ScalarArgAsPointerAnalysis() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "ScalarArgAsPointerAnalysis";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        /// @brief   Main entry point. Runs on all kernel functions in the given module, analyzes them and
        ///          creates metadata with scalar kernel arguments (both explicit and implicit) used to
        ///          access global memory.
        /// @param   M module to analyze.
        /// @returns True if metadata was modified.
        virtual bool runOnModule(llvm::Module& M) override;

        void visitStoreInst(llvm::StoreInst& I);
        void visitLoadInst(llvm::LoadInst& I);

    private:

        typedef llvm::SmallPtrSet<llvm::Argument*, 2> ArgSet;

        /// @brief    Analyzes kernel function and saves result in function metadata.
        /// @param    F function to analyze.
        /// @returns  True if function metadata was updated.
        bool analyzeFunction(llvm::Function& F);

        /// @brief  Common entrypoint for load/store visitors - traces back destination pointer to kernel
        ///         arguments and saves them to common list.
        /// @param  V destination pointer.
        void analyzePointer(llvm::Value* V);

        /// @brief    Returns a set of matching kernel arguments. Returns null if load/store:
        ///             (1) is indirect, or
        ///             (2) uses pointer kernel argument
        /// @param    I instruction to trace back.
        /// @returns  Set of matching kernel arguments.
        const ArgSet* searchForArgs(llvm::Instruction* I);

        /// @brief Combined set of all matching arguments found in currently analyzed function.
        ArgSet m_matchingArgs;

        /// @brief Cached results for visited instructions.
        llvm::DenseMap<llvm::Instruction*, std::unique_ptr<ArgSet>> m_visitedInst;
    };

} // namespace IGC
