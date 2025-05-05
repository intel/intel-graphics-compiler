/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "slm-blocking"
#include "Compiler/CISACodeGen/LoopDCE.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"
#include <functional>   // for std::function

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {

    class LoopDeadCodeElimination : public FunctionPass {
        CodeGenContext* CGC;
        const DataLayout* DL;
        LoopInfo* LI;
        PostDominatorTree* PDT;

    public:
        static char ID;

        LoopDeadCodeElimination() : FunctionPass(ID),
            CGC(nullptr), DL(nullptr), LI(nullptr), PDT(nullptr) {
            initializeLoopDeadCodeEliminationPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<PostDominatorTreeWrapperPass>();
            AU.addPreserved<PostDominatorTreeWrapperPass>();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addPreserved<LoopInfoWrapperPass>();
        }

    private:
        bool processLoop(Loop* L);
    };

    /// This is to remove any recursive PHINode. For example,
    ///   Bx:
    ///     x = phi [] ...[y, By]
    ///   By:
    ///     y = phi [] ... [x, Bx]
    /// Both x and y are only used by PHINode, thus they can be
    /// removed.  This recursive PHINodes happens only if there
    /// are loops, and could be introduced in SROA.
    ///
    /// This pass detects the cases above and remove those PHINodes
    class DeadPHINodeElimination : public FunctionPass {
    public:
        static char ID;

        DeadPHINodeElimination() : FunctionPass(ID)
        {
            initializeDeadPHINodeEliminationPass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function&) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<LoopInfoWrapperPass>();
        }
    };

} // End anonymous namespace

char LoopDeadCodeElimination::ID = 0;

#define PASS_FLAG     "igc-loop-dce"
#define PASS_DESC     "Advanced DCE on loop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(LoopDeadCodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_END(LoopDeadCodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

FunctionPass* IGC::createLoopDeadCodeEliminationPass() {
    return new LoopDeadCodeElimination();
}

bool LoopDeadCodeElimination::runOnFunction(Function& F) {
    // Skip non-kernel function.
    MetaDataUtils* MDU = nullptr;
    MDU = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    auto FII = MDU->findFunctionsInfoItem(&F);
    if (FII == MDU->end_FunctionsInfo())
        return false;

    CGC = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    DL = &F.getParent()->getDataLayout();
    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

    bool Changed = false;
    // DFT all loops.
    for (auto I = LI->begin(), E = LI->end(); I != E; ++I)
        for (auto L = df_begin(*I), F = df_end(*I); L != F; ++L)
            Changed |= processLoop(*L);

    return Changed;
}

static Instruction* getSingleUserInLoop(Value* V, Loop* L) {
    Instruction* UserInLoop = nullptr;
    for (auto U : V->users()) {
        auto I = dyn_cast<Instruction>(U);
        if (!I)
            return nullptr;
        if (!L->contains(I->getParent()))
            continue;
        if (UserInLoop)
            return nullptr;
        UserInLoop = I;
    }
    return UserInLoop;
}

bool LoopDeadCodeElimination::processLoop(Loop* L) {
    SmallVector<BasicBlock*, 8> ExitingBlocks;
    L->getExitingBlocks(ExitingBlocks);
    if (ExitingBlocks.empty())
        return false;

    bool Changed = false;
    for (auto BB : ExitingBlocks) {
        auto BI = dyn_cast<BranchInst>(BB->getTerminator());
        // Skip exiting block with non-conditional branch.
        if (!BI || !BI->isConditional())
            continue;
        bool ExitingOnTrue = !L->contains(BI->getSuccessor(0));
        auto Cond = BI->getCondition();
        for (auto U : Cond->users()) {

            auto SI = dyn_cast<SelectInst>(U);

            //Check that 'select' instruction is within the loop and also that the
            //Branch condition is used as the select's condition and not as the select's
            //true or false value
            if (!SI || !L->contains(SI->getParent()) || SI->getCondition() != Cond)
                continue;
            // TODO: Handle the trivial case where 'select' is used as a loop-carried
            // value.
            auto I = getSingleUserInLoop(SI, L);
            if (!I)
                continue;
            auto PN = dyn_cast<PHINode>(I);
            if (!PN || L->getHeader() != PN->getParent())
                continue;
            // v2 := phi(v0/bb0, v1/bb1)
            // ...
            // bb1:
            // v1 := select(cond, va, vb);
            // br cond, out_of_loop
            //
            // replace all uses of v1 in loop with vb
            // replace all uses of v1 out of loop with va
            Value* NewValInLoop = SI->getFalseValue();
            Value* NewValOutLoop = SI->getTrueValue();
            if (!ExitingOnTrue)
                std::swap(NewValInLoop, NewValOutLoop);
            for (auto UI = SI->use_begin(), UE = SI->use_end(); UI != UE; /*EMPTY*/) {
                auto& Use = *UI++;
                auto I = cast<Instruction>(Use.getUser());
                auto NewVal =
                    L->contains(I->getParent()) ? NewValInLoop : NewValOutLoop;
                Use.set(NewVal);
            }
        }
    }
    return Changed;
}


/// DeadPHINodeElimination
char DeadPHINodeElimination::ID = 0;

#undef PASS_FLAG
#undef PASS_DESC
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS

#define PASS_FLAG     "igc-phielimination"
#define PASS_DESC     "Remove Dead Recurisive PHINode"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
    IGC_INITIALIZE_PASS_BEGIN(DeadPHINodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_END(DeadPHINodeElimination, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
}

FunctionPass* IGC::createDeadPHINodeEliminationPass() {
    return new DeadPHINodeElimination();
}

static bool eliminateDeadPHINodes(Function& F)
{
    // This is to eliminate potential recursive phi like the following:
    //  Bx:   x = phi [y0, B0], ...[z, Bz]
    //  Bz:   z = phi [x, Bx], ...[]
    //  x and z are recursive phi's that are not used anywhere but in those phi's.
    //  Thus, they can be eliminated.
    auto phiUsedOnlyByPhi = [](PHINode* P) {
        bool ret = true;
        for (auto U : P->users())
        {
            Instruction* Inst = dyn_cast<Instruction>(U);
            if (!Inst || !isa<PHINode>(Inst)) {
                ret = false;
                break;
            }
        }
        return ret;
    };

    DenseMap<PHINode*, int> candidates;
    for (auto& BI : F)
    {
        BasicBlock* BB = &BI;
        for (auto& II : *BB)
        {
            PHINode* Phi = dyn_cast<PHINode>(&II);
            if (Phi && phiUsedOnlyByPhi(Phi)) {
                candidates[Phi] = 1;
            }
            else if (!Phi) {
                // No more phi, stop looping.
                break;
            }
        }
    }

    /// Iteratively removing non-candidate PHINode by
    /// setting its map value to zero.
    bool changed;
    do
    {
        changed = false;
        for (auto MI = candidates.begin(), ME = candidates.end(); MI != ME; ++MI)
        {
            PHINode* P = MI->first;
            if (MI->second == 0)
                continue;

            for (auto U : P->users()) {
                PHINode* phiUser = dyn_cast<PHINode>(U);
                IGC_ASSERT_MESSAGE(nullptr != phiUser, "ICE: all candidates should have phi as its users!");
                auto iter = candidates.find(phiUser);
                if (iter == candidates.end())
                {
                    // not candidate as its user is not in the map.
                    MI->second = 0;
                    changed = true;
                    break;
                }

                // If it is the user of itself, skip.
                if (iter->first == MI->first) {
                    continue;
                }

                if (iter->second == 0)
                {
                    // not candidate as being used by a non-candidate phi
                    MI->second = 0;
                    changed = true;
                    break;
                }
            }
        }
    } while (changed);

    // Prepare phi for deletion by setting its operands to null
    SmallVector<PHINode*, 8> toBeDeleted;
    for (auto& MI : candidates)
    {
        PHINode* P = MI.first;
        if (MI.second == 0)
            continue;
        // reset its operands to zero, which will eventually
        // make all dead Phi's uses to be empty!
        Value* nilVal = Constant::getNullValue(P->getType());
        for (int i = 0, e = (int)P->getNumIncomingValues(); i < e; ++i)
        {
            P->setIncomingValue(i, nilVal);
        }
        toBeDeleted.push_back(P);
    }

    // Actually delete them.
    for (int i = 0, e = (int)toBeDeleted.size(); i < e; ++i)
    {
        PHINode* P = toBeDeleted[i];
        P->eraseFromParent();
    }

    return (toBeDeleted.size() > 0);
}

using BlockValueMap = DenseMap<const BasicBlock*, PHINode*>;
using BlockValueMapVector = SmallVector<BlockValueMap, 16>;
using BlockValueMapDuplicateIDs = SmallVector<SmallVector<size_t, 2>, 16>;

static BlockValueMapVector getNodeLoops(const Loop* L) {
    // Traverse the loop L and collect all the phi nodes
    // and their phi node predecessors.

    std::function<bool(BlockValueMap&, PHINode*)> CollectConnected =
        [&CollectConnected](BlockValueMap& BlockMap, PHINode* P) {
        // Check if we have never met phi node parent (basic block) before
        auto Inserted = BlockMap.insert(make_pair(P->getParent(), P));

        if (Inserted.second) {
            // New pair of phi node and basic block inserted
            // Traverse node's incoming values
            for (auto& V : P->incoming_values()) {
                if (auto N = dyn_cast<PHINode>(V))
                    if (!CollectConnected(BlockMap, N))
                        return false;
            }

            // Return true if phi node P and all its incoming phi nodes N
            // have been successfully inserted into a block map
            return true;
        }

        // Return true if phi node P have already been inserted into a block map before
        return Inserted.first->second == P;
    };

    BlockValueMapVector BlockMaps;
    // Start with phi node in a header
    for (auto& I : *L->getHeader())
        if (auto P = dyn_cast<PHINode>(&I)) {
            BlockValueMap BlockMap;
            if (CollectConnected(BlockMap, P)) {
                BlockMaps.push_back(BlockMap);
            }
        }

    return BlockMaps;
}

static bool isSameTopology(const BlockValueMap& BlockMapX, const BlockValueMap& BlockMapY) {
    // Check if phi node recursive loops x and y
    // are of the same topology, e.g. each node in x
    // loop has unique node in y loop corresponding
    // to the same block.

    if (BlockMapX.size() != BlockMapY.size())
        return false;

    for (auto& PairX : BlockMapX) {
        auto BlockX = PairX.first;
        auto ValueX = PairX.second;
        auto PairY = BlockMapY.find(BlockX);

        // Return false if loop of y blocks are different
        if (PairY == BlockMapY.end())
            return false;

        auto ValueY = PairY->second;

        // Compare two phi nodes x and y
        // Check if every non-phi node incomming value of x
        // has matching incoming value of y

        // Loop over all incoming values of phi node x
        for (unsigned int i = 0; i < ValueX->getNumIncomingValues(); ++i) {
            auto IncomingValueX = ValueX->getIncomingValue(i);
            auto IncomingBlockX = ValueX->getIncomingBlock(i);

            // Nodes with undef incoming value considered being unique,
            // because we can't tell for sure if it is a duplitcate or not.
            if (isa<UndefValue>(IncomingValueX))
                return false;

            // For all phi node incoming values of x being part of a loop
            // check that there are no matching incoming value in y
            // having mismatching incoming block
            if (auto IncomingPHINodeX = dyn_cast<PHINode>(IncomingValueX)) {
                if (BlockMapX.find(IncomingPHINodeX->getParent()) != BlockMapX.end()) {
                    for (unsigned int j = 0; j < ValueY->getNumIncomingValues(); ++j) {
                        auto IncomingValueY = ValueY->getIncomingValue(j);

                        if (IncomingValueX == IncomingValueY)
                            return false;

                        // If one value is recurred value, then the other should be recurred value too.
                        if ((IncomingValueX == ValueX && IncomingValueY != ValueY) ||
                            (IncomingValueX != ValueX && IncomingValueY == ValueY))
                            return false;
                    }
                    continue;
                }
            }
            bool Found = false;

            // For all other incoming values of x check that there is
            // a matching incoming value in y having the same incoming block
            for (unsigned int j = 0; j < ValueY->getNumIncomingValues(); ++j) {
                auto IncomingValueY = ValueY->getIncomingValue(j);
                auto IncomingBlockY = ValueY->getIncomingBlock(j);

                if (IncomingValueX == IncomingValueY && IncomingBlockX == IncomingBlockY) {
                    // Found a match
                    Found = true;
                    break;
                }
            }

            // Return false if there is a non-phi node incoming value of phi node x
            // having no match among the incoming values of y
            if (!Found)
                return false;
        }
    }

    return true;
}

static BlockValueMapDuplicateIDs splitLoopsIntoDuplicateGroups(const BlockValueMapVector& BlockMaps) {
    // Split all found recursive phi node loops into
    // groups of duplicates by comparing each loop
    // to all others.

    BlockValueMapDuplicateIDs DuplicatesIDs;
    if (BlockMaps.size()) {
        SmallVector<bool, 16> Found(BlockMaps.size(), false);

        for (size_t i = 0; i < BlockMaps.size() - 1; ++i) {
            SmallVector<size_t, 2> Duplicates;

            for (size_t j = i + 1; j < BlockMaps.size(); ++j)
                if (!Found[j] && isSameTopology(BlockMaps[i], BlockMaps[j])) {
                    Duplicates.push_back(j);
                    Found[j] = true;
                }

            if (Duplicates.size()) {
                Duplicates.push_back(i);
                DuplicatesIDs.push_back(Duplicates);
                Found[i] = true;
            }
        }
    }

    return DuplicatesIDs;
}

static bool replaceNonPHINodeUses(const BlockValueMapVector& BlockMaps, const BlockValueMapDuplicateIDs& DuplicatesIDs) {
    // Replace all non-phi uses of nodes x with nodes y
    // within a group according to the correspondence maps.

    bool Changed = false;
    for (size_t i = 0; i < DuplicatesIDs.size(); ++i) {
        auto& Duplicates = DuplicatesIDs[i];
        IGC_ASSERT_MESSAGE(Duplicates.size() >= 2, "Number of duplicates is expected to be at least 2");

        // Let y be the first loop
        auto& BlockMapY = BlockMaps[Duplicates[0]];

        for (auto& PairY : BlockMapY) {
            auto BlockY = PairY.first;
            auto ValueY = PairY.second;

            // All other loops are considered x
            for (size_t j = 1; j < Duplicates.size(); ++j) {
                auto& BlockMapX = BlockMaps[Duplicates[j]];

                auto PairX = BlockMapX.find(BlockY);
                IGC_ASSERT_MESSAGE(PairX != BlockMapX.end(), "Inconsistent duplicate maps");
                auto ValueX = PairX->second;

                if (ValueX != ValueY) {
                    ValueX->replaceAllUsesWith(ValueY);
                    Changed = true;
                }
            }
        }
    }

    return Changed;
}

static bool resolveDuplicateLoops(Function& F, const Loop* L) {
    // This is to eliminate duplicate recursive phis like the following:
    //  B0:   x0 = phi [v, ...], [xN, BN]
    //  B0:   y0 = phi [v, ...], [yN, BN]
    //  ...
    //  B1:   x1 = phi [x0, B0], ...
    //  B1:   y1 = phi [y0, B0], ...
    //  ...
    //  t = add %x1, 1.0
    //  ...
    //  B2:   x2 = phi [t, ...], [x1, B1]
    //  B2:   y2 = phi [t, ...], [y1, B1]
    //  ...
    //        store yN ...
    // xi are recursive duplicate of yi, and are not used anywhere but in those phis.
    // Thus, they can be eliminated.

    // Find all recursive phis within current loop L:
    // loops of xi, yi, etc.
    auto BlockMaps = getNodeLoops(L);

    // Group all recursive phis within current loop L
    // into duplicate groups:
    // xi duplicates yi, so put then into the same group
    auto DuplicatesIDs = splitLoopsIntoDuplicateGroups(BlockMaps);

    // Replace x1 with y1 in t = add %x1, 1.0,
    // so all xi become unused anywhere except in xi
    // Unused phi nodes will be eliminated by upcomming
    // call to eliminateDeadPHINodes(F)
    return replaceNonPHINodeUses(BlockMaps, DuplicatesIDs);
}

bool DeadPHINodeElimination::runOnFunction(Function& F) {
    bool Changed = false;

    if (eliminateDeadPHINodes(F))
        Changed = true;

    auto LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    for (auto& L : LI->getLoopsInPreorder()) {
        if (resolveDuplicateLoops(F, L))
            Changed = true;

        if (eliminateDeadPHINodes(F))
            Changed = true;
    }

    return Changed;
}
