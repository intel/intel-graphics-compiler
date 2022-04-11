/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===- CoroFrame.cpp - Builds and manipulates coroutine frame -------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//


#pragma once
#include "Probe/Assertion.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/ADT/BitVector.h>
#include <llvm/IR/CFG.h>
#include "common/LLVMWarningsPop.hpp"

#include <vector>

namespace IGC {

void splitAround(llvm::Instruction* I, const llvm::Twine& Name);

class BlockToIndexMapping {
  llvm::SmallVector<llvm::BasicBlock *, 32> V;

public:
  size_t size() const { return V.size(); }

  BlockToIndexMapping(llvm::Function &F) {
    for (llvm::BasicBlock &BB : F)
      V.push_back(&BB);
    llvm::sort(V);
  }

  size_t blockToIndex(llvm::BasicBlock *BB) const {
    auto *I = llvm::lower_bound(V, BB);
    IGC_ASSERT_MESSAGE(I != V.end(), "BasicBlockNumberng: Unknown block");
    IGC_ASSERT_MESSAGE(*I == BB, "BasicBlockNumberng: Unknown block");
    return I - V.begin();
  }

  llvm::BasicBlock *indexToBlock(unsigned Index) const { return V[Index]; }
};

// The SuspendCrossingInfo maintains data that allows to answer a question
// whether given two BasicBlocks A and B there is a path from A to B that
// passes through a suspend point.
//
// For every basic block 'i' it maintains a BlockData that consists of:
//   Consumes:  a bit vector which contains a set of indices of blocks that can
//              reach block 'i'
//   Kills: a bit vector which contains a set of indices of blocks that can
//          reach block 'i', but one of the path will cross a suspend point
//   Suspend: a boolean indicating whether block 'i' contains a suspend point.
//

class SuspendCrossingInfo {
    BlockToIndexMapping Mapping;

    struct BlockData {
        llvm::BitVector Consumes;
        llvm::BitVector Kills;
        bool Suspend = false;
    };
    llvm::SmallVector<BlockData, 32> Block;

    llvm::iterator_range<llvm::succ_iterator> successors(BlockData const& BD) const;

    BlockData& getBlockData(llvm::BasicBlock* BB);

public:
    void print(llvm::raw_ostream& OS) const;
    void print(llvm::raw_ostream& OS, llvm::StringRef Label, llvm::BitVector const& BV) const;

    void dump() const;
    void dumpToFile(const CodeGenContext *Ctx) const;

    SuspendCrossingInfo(llvm::Function& F, const std::vector<llvm::Instruction*>& SuspendPoints);

    bool hasPathCrossingSuspendPoint(llvm::BasicBlock* DefBB, llvm::BasicBlock* UseBB) const;

    bool isDefinitionAcrossSuspend(llvm::BasicBlock* DefBB, llvm::User* U) const;

    bool isDefinitionAcrossSuspend(llvm::Argument& A, llvm::User* U) const;

    bool isDefinitionAcrossSuspend(llvm::Instruction& I, llvm::User* U) const;
};

} // namespace IGC
