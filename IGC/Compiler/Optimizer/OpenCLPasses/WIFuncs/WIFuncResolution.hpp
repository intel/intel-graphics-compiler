/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"

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

        /// @brief  Call getlocalID function in module
        /// @param  pInsertBefore - insert getlocalID after the given instruction.
        static llvm::CallInst* CallGetLocalID(llvm::Instruction* pInsertBefore);

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
        /// @brief  Resolves get_local_thread_id(dim).
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the local thread id
        llvm::Value* getLocalThreadId(llvm::CallInst &CI);

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

        /// @brief  Resolves get_assert_buffer().
        ///         Adds the appropriate sequence of code before the given call instruction
        /// @param  CI The call instruction.
        /// @return A value representing the assert buffer
        llvm::Value* getAssertBufferPtr(llvm::CallInst& CI);

        /// @brief  The implicit arguments of the current function
        ImplicitArgs m_implicitArgs;

        /// @brief  Emit intrinsics to store implicit buffer
        ///         pointer and local id pointer.
        /// @param  Function to add intrinsics to. This has to be
        ///         an entry level, ie kernel function.
        void storeImplicitBufferPtrs(llvm::Function& F);

    private:
        /// @brief  Indicates if the pass changed the processed function
        bool m_changed = false;
        IGCMD::MetaDataUtils* m_pMdUtils = nullptr;
    };

} // namespace IGC

namespace IGC
{
    class LowerImplicitArgIntrinsics : public llvm::FunctionPass, public llvm::InstVisitor<LowerImplicitArgIntrinsics>
    {
    public:
        static char ID;

        LowerImplicitArgIntrinsics();

        ~LowerImplicitArgIntrinsics() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "LowerImplicitArgIntrinsics";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        void visitCallInst(llvm::CallInst& CI);

    private:
        GenXFunctionGroupAnalysis* m_FGA = nullptr;
        IGC::CodeGenContext* m_ctx = nullptr;
        std::map<llvm::GenISAIntrinsic::ID, llvm::Instruction*> usedIntrinsicsMap;
        llvm::Value* BuildLoadInst(llvm::CallInst& CI, unsigned int Offset, llvm::Type* DataType);
        llvm::Value* getIntrinsicCall(llvm::Function* F, llvm::GenISAIntrinsic::ID IntrinsicID, llvm::Type* DataType);
    };
} // namespace IGC
