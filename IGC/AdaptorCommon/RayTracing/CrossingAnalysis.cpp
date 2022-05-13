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

//===----------------------------------------------------------------------===//
///
/// Adapted from CoroFrame.cpp, this provides a utility to determine whether
/// a given value is live across some "suspend point" (e.g., barrier, TraceRay).
///
/// As a precondition, the function should first split each of the suspend
/// points into their own basic blocks (via splitAround()) to aid the analysis.
///
//===----------------------------------------------------------------------===//

#include "CrossingAnalysis.h"
#include "debug/Dump.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Splits the block at a particular instruction unless it is the first
// instruction in the block with a single predecessor.
static BasicBlock* splitBlockIfNotFirst(Instruction* I, const Twine& Name) {
    auto* BB = I->getParent();
    if (&BB->front() == I) {
        if (BB->getSinglePredecessor()) {
            BB->setName(Name);
            return BB;
        }
    }
    return BB->splitBasicBlock(I, Name);
}

namespace IGC {

void splitAround(llvm::Instruction* I, const llvm::Twine& Name)
{
    splitBlockIfNotFirst(I, Name);
    if (I->getNextNode())
    {   //if I is terminator, we cannot split it further
        splitBlockIfNotFirst(I->getNextNode(), "After" + Name);
    }
}

} // namespace IGC

iterator_range<succ_iterator> SuspendCrossingInfo::successors(BlockData const& BD) const {
    BasicBlock* BB = Mapping.indexToBlock((unsigned)(&BD - &Block[0]));
    return llvm::successors(BB);
}

SuspendCrossingInfo::BlockData& SuspendCrossingInfo::getBlockData(BasicBlock* BB) {
    return Block[Mapping.blockToIndex(BB)];
}

LLVM_DUMP_METHOD void SuspendCrossingInfo::print(
    raw_ostream& OS, StringRef Label, BitVector const& BV) const {
    OS << Label << ":";
    for (size_t I = 0, N = BV.size(); I < N; ++I)
        if (BV[I])
            OS << " " << Mapping.indexToBlock(I)->getName();
    OS << "\n";
}

LLVM_DUMP_METHOD void SuspendCrossingInfo::print(raw_ostream& OS) const {
    for (size_t I = 0, N = Block.size(); I < N; ++I) {
        BasicBlock* const B = Mapping.indexToBlock(I);
        OS << B->getName() << ":\n";
        print(OS, "   Consumes", Block[I].Consumes);
        print(OS, "      Kills", Block[I].Kills);
    }
    OS << "\n";
}

LLVM_DUMP_METHOD void SuspendCrossingInfo::dump() const
{
    print(outs());
}

// Used for dumping into a file with a fixed name while running in debugger
LLVM_DUMP_METHOD void SuspendCrossingInfo::dumpToFile(const CodeGenContext *Ctx) const
{
    using namespace Debug;

    auto name =
        DumpName(IGC::Debug::GetShaderOutputName())
        .Hash(Ctx->hash)
        .Type(Ctx->type)
        .Pass("WIAnalysis")
        .Extension("txt");
    print(Dump(name, DumpType::DBG_MSG_TEXT).stream());
}

SuspendCrossingInfo::SuspendCrossingInfo(
    Function& F, const std::vector<Instruction*>& SuspendPoints)
    : Mapping(F) {
    const size_t N = Mapping.size();
    Block.resize(N);

    // Initialize every block so that it consumes itself
    for (size_t I = 0; I < N; ++I) {
        auto& B = Block[I];
        B.Consumes.resize(N);
        B.Kills.resize(N);
        B.Consumes.set(I);
    }

    // Mark all suspend blocks and indicate that they kill everything they
    // consume.
    auto markSuspendBlock = [&](Instruction* BarrierInst) {
        BasicBlock* SuspendBlock = BarrierInst->getParent();
        auto& B = getBlockData(SuspendBlock);
        B.Suspend = true;
        B.Kills |= B.Consumes;
    };

    for (auto *SP : SuspendPoints) {
        markSuspendBlock(SP);
    }

    // Iterate propagating consumes and kills until they stop changing.
    bool Changed;
    do {
        Changed = false;
        for (size_t I = 0; I < N; ++I) {
            auto& B = Block[I];
            for (BasicBlock* SI : successors(B)) {
                auto SuccNo = Mapping.blockToIndex(SI);

                // Saved Consumes and Kills bitsets so that it is easy to see
                // if anything changed after propagation.
                auto& S = Block[SuccNo];
                auto SavedConsumes = S.Consumes;
                auto SavedKills = S.Kills;

                // Propagate Kills and Consumes from block B into its successor S.
                S.Consumes |= B.Consumes;
                S.Kills |= B.Kills;

                // If block B is a suspend block, it should propagate kills into the
                // its successor for every block B consumes.
                if (B.Suspend) {
                    S.Kills |= B.Consumes;
                }
                if (S.Suspend) {
                    // If block S is a suspend block, it should kill all of the blocks it
                    // consumes.
                    S.Kills |= S.Consumes;
                }
                else {
                    // This is reached when S block is not Suspend and it
                    // needs to make sure that it is not in the kill set.
                    S.Kills.reset(SuccNo);
                }

                // See if anything changed.
                Changed |= (S.Kills != SavedKills) || (S.Consumes != SavedConsumes);
            }
        }
    } while (Changed);
}

bool SuspendCrossingInfo::hasPathCrossingSuspendPoint(BasicBlock* DefBB, BasicBlock* UseBB) const {
    size_t const DefIndex = Mapping.blockToIndex(DefBB);
    size_t const UseIndex = Mapping.blockToIndex(UseBB);

    IGC_ASSERT_MESSAGE(Block[UseIndex].Consumes[DefIndex], "use must consume def");
    bool const Result = Block[UseIndex].Kills[DefIndex];
    return Result;
}

bool SuspendCrossingInfo::isDefinitionAcrossSuspend(BasicBlock* DefBB, User* U) const {
    auto* I = cast<Instruction>(U);

    // We rewrote PHINodes, so that only the ones with exactly one incoming
    // value need to be analyzed.
    if (auto* PN = dyn_cast<PHINode>(I))
        if (PN->getNumIncomingValues() > 1)
            return false;

    BasicBlock* UseBB = I->getParent();
    return hasPathCrossingSuspendPoint(DefBB, UseBB);
}

bool SuspendCrossingInfo::isDefinitionAcrossSuspend(Argument& A, User* U) const {
    return isDefinitionAcrossSuspend(&A.getParent()->getEntryBlock(), U);
}

bool SuspendCrossingInfo::isDefinitionAcrossSuspend(Instruction& I, User* U) const {
    return isDefinitionAcrossSuspend(I.getParent(), U);
}

