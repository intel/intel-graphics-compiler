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
#include <llvm/ADT/StringRef.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  WIFuncsAnalysis pass used for analyzing which OpenCL WI (work item) functions
    ///         are used in the different functions in the module and creating metadata that represents
    ///         the implicit information needed by each function for resolving these function calls

    class WIFuncsAnalysis : public llvm::ModulePass, public llvm::InstVisitor<WIFuncsAnalysis>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        WIFuncsAnalysis();

        /// @brief  Destructor
        ~WIFuncsAnalysis() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "WIFuncsAnalysis";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        ///         Runs on all functions defined in the given module, finds all OpenCL WI function calls,
        ///         analyzes them and creates metadata that represents the implicit information needed
        ///         by each function for resolving these function calls
        /// @param  M The destination module.
        virtual bool runOnModule(llvm::Module& M) override;

        /// @brief  Call instructions visitor.
        ///         Checks for OpenCL WI functions and analyzes it
        /// @param  CI The call instruction.
        void visitCallInst(llvm::CallInst& CI);

        static const llvm::StringRef GET_LOCAL_ID_X;
        static const llvm::StringRef GET_LOCAL_ID_Y;
        static const llvm::StringRef GET_LOCAL_ID_Z;
        static const llvm::StringRef GET_GROUP_ID;
        static const llvm::StringRef GET_LOCAL_THREAD_ID;
        static const llvm::StringRef GET_GLOBAL_SIZE;
        static const llvm::StringRef GET_LOCAL_SIZE;
        static const llvm::StringRef GET_GLOBAL_OFFSET;
        static const llvm::StringRef GET_WORK_DIM;
        static const llvm::StringRef GET_NUM_GROUPS;
        static const llvm::StringRef GET_ENQUEUED_LOCAL_SIZE;
        static const llvm::StringRef GET_STAGE_IN_GRID_ORIGIN;
        static const llvm::StringRef GET_STAGE_IN_GRID_SIZE;
        static const llvm::StringRef GET_SYNC_BUFFER;
        static const llvm::StringRef GET_ASSERT_BUFFER;

    private:
        /// @brief  Function entry point.
        ///         Finds all OpenCL WI (Work item) function calls in this function, analyzes them and creates
        ///         metadata that represents the implicit information needed by this function
        ///         for resolving these function calls
        /// @param  F The destination function.
        bool runOnFunction(llvm::Function& F);

        /// @brief  Marks whether group id is needed by the current function
        bool m_hasGroupID = false;
        /// @brief  Marks whether local thread id is needed by the current function
        bool m_hasLocalThreadID = false;
        /// @brief  Marks whether global offset is needed by the current function
        bool m_hasGlobalOffset = false;
        /// @brief  Marks whether local id is needed by the current function
        bool m_hasLocalID = false;
        /// @brief  Marks whether global size is needed by the current function
        bool m_hasGlobalSize = false;
        /// @brief  Marks whether local size is needed by the current function
        bool m_hasLocalSize = false;
        /// @brief  Marks whether work dimension is needed by the current function
        bool m_hasWorkDim = false;
        /// @brief  Marks whether number of work groups is needed by the current function
        bool m_hasNumGroups = false;
        /// @brief  Marks whether enqueued local size is needed by the current function
        bool m_hasEnqueuedLocalSize = false;
        /// @brief  Marks whether stage_in_grid_origin is needed by the current function
        bool m_hasStageInGridOrigin = false;
        /// @brief  Marks whether stage_in_grid_size is needed by the current function
        bool m_hasStageInGridSize = false;
        /// @brief  Marks whether sync buffer is needed by the current function
        bool m_hasSyncBuffer = false;
        /// @brief  Marks whether assert buffer is needed by the current function
        bool m_hasAssertBuffer = false;
        /// @brief Marks whether kernel has stackcalls
        bool m_hasStackCalls = false;
        /// @brief MetaData utils used to generate LLVM metadata
        IGCMD::MetaDataUtils* m_pMDUtils = nullptr;
        /// @brief context for compilation
        IGC::CodeGenContext* m_ctx = nullptr;
    };

} // namespace IGC
