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

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class WIFuncsAnalysis;

    /// @brief  WIFuncResolution pass used for resolving OpenCL WI (work item) functions.
    ///         This pass depends on the WIFuncAnalysis and AddImplicitArgs passes running before it

    class WIFuncResolution : public llvm::FunctionPass, public llvm::InstVisitor<WIFuncResolution>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        WIFuncResolution();

        /// @brief  Destructor
        ~WIFuncResolution() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "WIFuncResolution";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        /// @brief  Main entry point.
        ///         Finds all OpenCL WI (Work item) function calls and resolve them into an llvm sequence
        /// @param  F The destination function.
        virtual bool runOnFunction(llvm::Function& F) override;

        /// @brief  Call instructions visitor.
        ///         Checks for OpenCL WI functions and resolves them into appropriate sequence of code
        /// @param  CI The call instruction.
        void visitCallInst(llvm::CallInst& CI);

    private:

        /// @brief  Resolves get_local_id(dim).
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @param  argType The type of the appropriate implicit arg.
        /// @return A value representing the local id
        llvm::Value* getLocalId(llvm::CallInst& CI, ImplicitArg::ArgType argType);

        /// @brief  Resolves get_group_id(dim).
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the group id
        llvm::Value* getGroupId(llvm::CallInst& CI);

        /// @brief  Resolves get_global_size(dim).
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the global size
        llvm::Value* getGlobalSize(llvm::CallInst& CI);

        /// @brief  Resolves get_local_size(dim).
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the local size
        llvm::Value* getLocalSize(llvm::CallInst& CI);

        /// @brief  Resolves get_enqueued_local_size(dim).
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the enqueued local size
        llvm::Value* getEnqueuedLocalSize(llvm::CallInst& CI);

        /// @brief  Resolves get_global_offset(dim).
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the global offset
        llvm::Value* getGlobalOffset(llvm::CallInst& CI);

        /// @brief  Resolves get_work_dim().
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the work dimension
        llvm::Value* getWorkDim(llvm::CallInst& CI);

        /// @brief  Resolves get_num_groups(dim).
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the number of work groups
        llvm::Value* getNumGroups(llvm::CallInst& CI);

        /// @brief  Returns the appropriate implicit argument of the function
        ///         containing the given call instruction, based on the given argument type
        /// @param  CI       The call instruction.
        /// @param  argType  The argument type.
        /// @return The function argument associated with the given implicit arg type
        llvm::Argument* getImplicitArg(llvm::CallInst& CI, ImplicitArg::ArgType argType);

        /// @brief  Resolves get_stage_in_grid_origin().
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the stage in grid origin
        llvm::Value* getStageInGridOrigin(llvm::CallInst& CI);

        /// @brief  Resolves get_stage_in_grid_size().
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the stage in grid size
        llvm::Value* getStageInGridSize(llvm::CallInst& CI);

        /// @brief  Resolves get_sync_buffer().
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the sync buffer
        llvm::Value* getSyncBufferPtr(llvm::CallInst& CI);

        /// @brief  get vector of work group size if reqd_work_group_size is set.
        /// @param  F the function to check
        /// @return A vector with work group size (e.g., <i32 16, i32 1, i32 1>)
        ///         or nullptr if not known.
        llvm::Constant* getKnownWorkGroupSize(
            IGCMD::MetaDataUtils* MDUtils,
            llvm::Function& F) const;

        /// @brief  The implicit arguments of the current function
        ImplicitArgs m_implicitArgs;

    private:
        /// @brief  Indicates if the pass changed the processed function
        bool m_changed;
    };

} // namespace IGC

