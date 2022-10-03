/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/DataLayout.h>
#include "common/LLVMWarningsPop.hpp"
#include <vector>
#include <map>
#include "Probe/Assertion.h"

namespace IGC
{
    /// @brief  PrivateMemoryBufferAnalysis pass used for analyzing private memory alloca instructions
    ///         and determining the size and offset of each buffer and the total amount of private memory
    ///          needed by each work item. This is done by analyzing the alloca instructions.

    class PrivateMemoryBufferAnalysis : public llvm::ModulePass, public llvm::InstVisitor<PrivateMemoryBufferAnalysis>
    {
    public:
        // Pass identification, replacement for typeid
        static char ID;

        /// @brief  Constructor
        PrivateMemoryBufferAnalysis();

        /// @brief  Destructor
        ~PrivateMemoryBufferAnalysis() { }

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "PrivateMemoryBufferAnalysis";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesAll();
        }

        /// @brief  Process module
        /// @param  M The module to process
        bool runOnModule(llvm::Module& M) override;

        /// @brief  Alloca instructions visitor.
        ///         Checks for private memory allocation, determins the size and offset of each allocation,
        ///         helps calculate the total amount of private memory needed by each wotk item.
        /// @param  AI The alloca instruction.
        void visitAllocaInst(llvm::AllocaInst& AI);

        /// @brief  get offset of alloca instruction in private memory buffer for given function.
        /// @param  pFunc The function conatins the alloca.
        /// @param  pAI The alloca instruction.
        /// @return offset in private buffer of given alloca instruction.
        unsigned int getBufferOffset(llvm::Function* pFunc, llvm::AllocaInst* pAI)
        {
            IGC_ASSERT_MESSAGE(m_privateInfoMap.count(pFunc), "Function should have private buffer info");
            IGC_ASSERT_MESSAGE(m_privateInfoMap[pFunc].m_bufferOffsets.count(pAI), "AllocalInst should have buffer offset info");
            return m_privateInfoMap[pFunc].m_bufferOffsets[pAI];
        }

        /// @brief  get offset of alloca instruction in private memory buffer for given function.
        /// @param  pFunc The function conatins the alloca.
        /// @param  pAI The alloca instruction.
        /// @return buffer size of given alloca instruction.
        unsigned int getBufferSize(llvm::Function* pFunc, llvm::AllocaInst* pAI)
        {
            IGC_ASSERT_MESSAGE(m_privateInfoMap.count(pFunc), "Function should have private buffer info");
            IGC_ASSERT_MESSAGE(m_privateInfoMap[pFunc].m_bufferSizes.count(pAI), "AllocalInst should have buffer size info");
            return m_privateInfoMap[pFunc].m_bufferSizes[pAI];
        }

        /// @brief  get alloca instructions to handle of given function.
        /// @param  pFunc The function.
        /// @return alloca instructions of given function.
        std::vector<llvm::AllocaInst*>& getAllocaInsts(llvm::Function* pFunc)
        {
            IGC_ASSERT_MESSAGE(m_privateInfoMap.count(pFunc), "Function should have private buffer info");
            return m_privateInfoMap[pFunc].m_allocaInsts;
        }

        /// @brief  get total private memory buffer size per WI of given function.
        /// @param  pFunc The function.
        /// @return total private memory buffer size of given function.
        unsigned int getTotalPrivateMemPerWI(llvm::Function* pFunc)
        {
            IGC_ASSERT_MESSAGE(m_privateInfoMap.count(pFunc), "Function should have private buffer info");
            return m_privateInfoMap[pFunc].m_bufferTotalSize;
        }

    private:
        /// @brief  Finds all alloca instructions, analyzes them, determins the size and offset of each
        ///         private buffer.
        /// @param  F The destination function.
        void runOnFunction(llvm::Function& F);

    private:

        /// @brief  The data layout
        const llvm::DataLayout* m_DL;

        /// @brief  Used for calculating the total amount of private memory size per thread
        unsigned int m_currentOffset;

        /// @brief  The max alignment needed by an alloca instruction in this function
        ///         Used for calculating the alignment of the private memory buffer size
        alignment_t m_maxAlignment;

        typedef struct {
            /// @brief - Total amount of private memory size per thread
            unsigned int m_bufferTotalSize;

            /// @brief - Collected alloca instruction for processing
            std::vector<llvm::AllocaInst*> m_allocaInsts;

            /// @brief - map between alloca instruction and offset in buffer
            llvm::MapVector<llvm::AllocaInst*, unsigned int> m_bufferOffsets;

            /// @brief - map between alloca instruction and total size
            llvm::MapVector<llvm::AllocaInst*, unsigned int> m_bufferSizes;
        } PrivateBufferPerFuncInfo;

        /// @brief - map between function and total private buffer size
        llvm::MapVector<llvm::Function*, PrivateBufferPerFuncInfo> m_privateInfoMap;
    };

} // namespace IGC
