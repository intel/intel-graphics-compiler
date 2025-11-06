/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallBitVector.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SetVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
class BasicBlock;
class CallInst;
class DominatorTree;
class Instruction;
class LoopInfo;
class Use;
} // namespace llvm

namespace IGC {
class AllocationLivenessAnalyzer : public llvm::FunctionPass {
  using BBToIndexMapT = llvm::DenseMap<llvm::BasicBlock *, size_t>;

public:
  struct LivenessInstruction {
    const llvm::Instruction *inst = nullptr;
    size_t instIndexInBB = 0;
    size_t parentBBIndex = 0;

    LivenessInstruction() = default;
    LivenessInstruction(const llvm::Instruction *I)
        : inst(I), instIndexInBB(getInstructionIndex(I)), parentBBIndex(getBBIndex(I->getParent())) {}
    LivenessInstruction(const llvm::Instruction *I, const BBToIndexMapT &bbToIndexMap)
        : inst(I), instIndexInBB(getInstructionIndex(I)), parentBBIndex(bbToIndexMap.lookup(I->getParent())) {}
  };

  struct LivenessData {
    LivenessInstruction lifetimeStart = {};
    llvm::SmallVector<LivenessInstruction, 4> lifetimeEndInstructions;

    llvm::SmallBitVector bbIn;
    llvm::SmallBitVector bbOut;

    struct Edge {
      llvm::BasicBlock *from;
      llvm::BasicBlock *to;

      bool operator==(const Edge &other) const { return from == other.from && to == other.to; }
      bool operator!=(const Edge &other) const { return !(other == *this); }
    };

    llvm::SmallVector<Edge> lifetimeEndEdges;

    LivenessData(llvm::Instruction *allocationInstruction, llvm::SetVector<llvm::Instruction *> &&usersOfAllocation,
                 const llvm::LoopInfo &LI, const llvm::DominatorTree &DT, const BBToIndexMapT &bbToIndexMap,
                 llvm::BasicBlock *userDominatorBlock = nullptr,
                 llvm::SetVector<llvm::Instruction *> &&lifetimeLeakingUsers = {});

    bool OverlapsWith(const LivenessData &LD) const;
    bool ContainsInstruction(const LivenessInstruction &LI) const;
  };

  AllocationLivenessAnalyzer(char &pid) : llvm::FunctionPass(pid) {}

protected:
  LivenessData ProcessInstruction(llvm::Instruction *I, llvm::DominatorTree &DT, llvm::LoopInfo &LI,
                                  bool includeOrigin = false);

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  virtual void getAdditionalAnalysisUsage(llvm::AnalysisUsage &AU) const = 0;
  virtual void implementCallSpecificBehavior(llvm::CallInst *I, llvm::Use *use,
                                             llvm::SmallVector<llvm::Use *> &worklist,
                                             llvm::SetVector<llvm::Instruction *> &allUsers,
                                             llvm::SetVector<llvm::Instruction *> &lifetimeLeakingUsers);
  static unsigned getInstructionIndex(const llvm::Instruction *I);
  static unsigned getBBIndex(const llvm::BasicBlock *BB);
  void initBBtoIndexMap(llvm::Function &F);

private:
  llvm::DenseMap<llvm::Function *, BBToIndexMapT> PerFunctionBBToIndexMap;
};

namespace Provenance {
bool tryFindPointerOrigin(llvm::Value *ptr, llvm::SmallVectorImpl<llvm::Instruction *> &origins);
}

} // namespace IGC
