/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  This pass fixes the usage of GetBufferPtr, remove the combination of GetBufferPtr and GetElementPtr
    class FixResourcePtr : public llvm::FunctionPass
    {
    public:
        /// Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        FixResourcePtr();

        /// @brief  Destructor
        ~FixResourcePtr() {}

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "FixResourcePtrPass";
        }

        /// @brief  Main entry point.
        /// @param  F The destination function.
        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            IGC_UNUSED(AU);
        }

    private:

        llvm::Value* ResolveBufferIndex(llvm::Value* bufferIndex, llvm::Value* vectorIndex = nullptr);

        void RemoveGetBufferPtr(llvm::GenIntrinsicInst* bufPtr, llvm::Value* bufIdx);

        void FindGetElementPtr(llvm::Instruction* bufPtr, llvm::Instruction* searchPtr);

        void FindLoadStore(llvm::Instruction* bufPtr, llvm::Instruction* eltPtr, llvm::Instruction* searchPtr);

        llvm::Value* GetByteOffset(llvm::Instruction* eltPtr);

        llvm::Value* CreateLoadIntrinsic(llvm::LoadInst* inst, llvm::Instruction* bufPtr, llvm::Value* offsetVal);

        llvm::Value* CreateStoreIntrinsic(llvm::StoreInst* inst, llvm::Instruction* bufPtr, llvm::Value* offsetVal);

        /// @brief  Indicates if the pass changed the processed function
        bool m_changed;
        /// Function we are processing
        llvm::Function* curFunc;
        /// Need data-layout for fixing pointer
        const llvm::DataLayout* DL;
        /// agent to modify the llvm-ir
        llvm::IRBuilder<>* builder;
        /// list of clean up after change
        std::vector<llvm::Instruction*> eraseList;
    };

} // namespace IGC
