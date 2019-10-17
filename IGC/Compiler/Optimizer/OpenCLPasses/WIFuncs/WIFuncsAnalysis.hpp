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
        static const llvm::StringRef GET_GLOBAL_SIZE;
        static const llvm::StringRef GET_LOCAL_SIZE;
        static const llvm::StringRef GET_GLOBAL_OFFSET;
        static const llvm::StringRef GET_WORK_DIM;
        static const llvm::StringRef GET_NUM_GROUPS;
        static const llvm::StringRef GET_ENQUEUED_LOCAL_SIZE;
        static const llvm::StringRef GET_STAGE_IN_GRID_ORIGIN;
        static const llvm::StringRef GET_STAGE_IN_GRID_SIZE;
        static const llvm::StringRef GET_SYNC_BUFFER;

    private:
        /// @brief  Function entry point.
        ///         Finds all OpenCL WI (Work item) function calls in this function, analyzes them and creates
        ///         metadata that represents the implicit information needed by this function
        ///         for resolving these function calls
        /// @param  F The destination function.
        bool runOnFunction(llvm::Function& F);


        /// @brief  Marks whether local id is needed by the current function
        bool m_hasLocalID;
        /// @brief  Marks whether global size is needed by the current function
        bool m_hasGlobalSize;
        /// @brief  Marks whether local size is needed by the current function
        bool m_hasLocalSize;
        /// @brief  Marks whether work dimension is needed by the current function
        bool m_hasWorkDim;
        /// @brief  Marks whether number of work groups is needed by the current function
        bool m_hasNumGroups;
        /// @brief  Marks whether enqueued local size is needed by the current function
        bool m_hasEnqueuedLocalSize;
        /// @brief  Marks whether stage_in_grid_origin is needed by the current function
        bool m_hasStageInGridOrigin;
        /// @brief  Marks whether stage_in_grid_size is needed by the current function
        bool m_hasStageInGridSize;
        /// @brief  Marks whether sync buffer is needed by the current function
        bool m_hasSyncBuffer;
    };

} // namespace IGC
