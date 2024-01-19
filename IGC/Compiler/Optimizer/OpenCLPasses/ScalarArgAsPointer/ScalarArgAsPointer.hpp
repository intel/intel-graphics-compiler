/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

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

    /// IMPORTANT!!!
    /// Please bump up INDIRECT_ACCESS_DETECTION_VERSION when introducing any functional fixes in this pass.

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
        void visitCallInst(llvm::CallInst& I);

    private:

        using ArgSet = llvm::SmallPtrSet<llvm::Argument*, 2>;

        /// @brief    Analyzes kernel function and saves result in function metadata.
        /// @param    F function to analyze.
        /// @returns  True if function metadata was updated.
        bool analyzeFunction(llvm::Function& F);

        /// @brief  Common entrypoint for load/store visitors - traces back destination pointer to kernel
        ///         arguments and saves them to common list.
        /// @param  V destination pointer.
        void analyzePointer(llvm::Value* V);

        void analyzeValue(llvm::Value* V);

        /// @brief    Returns a set of matching kernel arguments. Returns null if load/store:
        ///             (1) is indirect, or
        ///             (2) uses pointer kernel argument
        /// @param    I instruction to trace back.
        /// @returns  Set of matching kernel arguments.
        const std::shared_ptr<ArgSet> findArgs(llvm::Instruction* I);

        /// @brief    Called by findArgs to analyzes instruction's operands. Returns null if load/store:
        ///             (1) is indirect, or
        ///             (2) uses pointer kernel argument
        /// @param    op operand to trace back.
        /// @returns  Set of matching kernel arguments.
        const std::shared_ptr<ArgSet> analyzeOperand(llvm::Value* op);

        /// @brief  Checks if instruction stores kernel argument, and if true, traces back to
        ///         alloca instruction (with offset). This is required for decomposed structs
        ///         that are copied into private memory.
        /// @param  SI visited store instruction.
        void analyzeStoredArg(llvm::StoreInst& I);

        /// @brief    Searches for kernel argument stored in allocated memory. Required for
        ///           decomposed structs that are copied into private memory. Can return more
        ///           than one argument if load instruction uses variable offset.
        /// @param    LI load instruction to check.
        /// @param    args output list of kernel arguments.
        /// @returns  true if kernel argument found or false if instruction doesn't trace back to alloca.
        bool findStoredArgs(llvm::LoadInst& LI, ArgSet& args);

        /// @brief    Traces pointer of load/store instruction back to alloca instruction.
        /// @param    V input load/store pointer.
        /// @param    AI output alloca instruction.
        /// @param    GEPI output GEP instruction (offset from alloca to value V).
        /// @returns  true if traced back to alloca.
        bool findAllocaWithOffset(llvm::Value* V, llvm::AllocaInst*& AI, llvm::GetElementPtrInst*& GEPI);

        /// @brief Combined set of all matching arguments found in currently analyzed function.
        ArgSet m_matchingArgs;

        /// @brief Cached results for visited instructions.
        // TODO: Should memory footprint be deemed acceptable,
        //       switch to holding the argument sets by value.
        llvm::DenseMap<llvm::Instruction*, std::shared_ptr<ArgSet>> m_visitedInst;

        /// @brief Mapping of basePtr + offset to kernel argument.
        llvm::DenseMap<std::pair<llvm::AllocaInst*, uint64_t>, llvm::Argument*> m_allocas;

        /// @brief Data layout of currently analyzed module.
        const llvm::DataLayout* DL = nullptr;
    };

} // namespace IGC
