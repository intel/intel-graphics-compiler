/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/BasicBlock.h"
#include "common/LLVMWarningsPop.hpp"
#include "Types.hpp"

namespace llvm
{
class GenIntrinsicInst;
}

namespace IGC
{
class CodeGenContext;

/// @brief The pass is used to remove duplicates of memory fences
/// and thread group barriers.
class SynchronizationObjectCoalescing : public llvm::FunctionPass
{
public:
    static char ID; ///< ID used by the llvm PassManager (the value is not important)

    SynchronizationObjectCoalescing();
    ~SynchronizationObjectCoalescing();

    ////////////////////////////////////////////////////////////////////////
    /// @brief Removes duplicates which were found during visiting basic blocks.
    virtual bool runOnFunction(llvm::Function& F);


    ////////////////////////////////////////////////////////////////////////
    /// @brief Iterates over the basic block to find fence instructions
    /// @param  beg
    /// @param  end
    void Visit(llvm::BasicBlock::iterator beg, llvm::BasicBlock::iterator end);

    ////////////////////////////////////////////////////////////////////////
    /// @brief Iterates over the basic block to find redundant of fence instructions.
    /// @param  it
    /// @param  end
    void VisitFence(llvm::BasicBlock::iterator& it, llvm::BasicBlock::iterator end);

    ////////////////////////////////////////////////////////////////////////
    /// @brief Iterates over the basic block to find duplicates of thread group barriers.
    /// @param  it
    /// @param  end
    void VisitThreadGroupBarrier(llvm::BasicBlock::iterator& it, llvm::BasicBlock::iterator end);

    ////////////////////////////////////////////////////////////////////////
    /// @brief Checks if the container holds a similar memory fence calls.
    /// Changes instruction in the container if the input instruction is
    /// more extended.
    /// @param  inst
    /// @param  container
    bool CompareAndMergeMemoryFenceWithContainerContent(const llvm::GenIntrinsicInst* inst, const std::vector<llvm::GenIntrinsicInst*>& container);

    ////////////////////////////////////////////////////////////////////////
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const;

private:
    std::vector<llvm::Instruction*> m_RepeatedBarrierCalls;
};

} // namespace IGC

void initializeSynchronizationObjectCoalescingPass(llvm::PassRegistry&);
