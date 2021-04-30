/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/InfiniteLoopRemoval.hpp"
#include "Probe/Assertion.h"

using namespace llvm;

namespace IGC
{

    char InfiniteLoopRemoval::ID = 0;

    IGC_INITIALIZE_PASS_BEGIN(InfiniteLoopRemoval, "InfiniteLoopRemoval",
        "Remove infinite loop from shader", false, false)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_END(InfiniteLoopRemoval, "InfiniteLoopRemoval",
            "Remove infinite loop from shader", false, false)

        InfiniteLoopRemoval::InfiniteLoopRemoval()
        : FunctionPass(ID)
    {
        initializeInfiniteLoopRemovalPass(*PassRegistry::getPassRegistry());
    }

    bool InfiniteLoopRemoval::runOnFunction(Function& F)
    {
        LLVMContext& ctx = F.getParent()->getContext();
        PostDominatorTree& PDT = getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
        LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

        bool cfgChanged = false;

        if (PDT.getRootNode() == nullptr)
        {
            // has infinite loop
            BasicBlock* retBB;

            // create a new return block
            retBB = BasicBlock::Create(ctx, "ret", &F);
            ReturnInst::Create(ctx, retBB);

            for (LoopInfo::iterator li = LI.begin(), le = LI.end();
                li != le; ++li)
            {
                llvm::Loop* loop = *li;
                SmallVector<BasicBlock*, 4> exitBB;
                loop->getExitBlocks(exitBB);

                if (exitBB.size() == 0)
                {
                    llvm::BasicBlock* predBB;
                    predBB = loop->getLoopPredecessor();
                    IGC_ASSERT(predBB != nullptr);

                    BranchInst* branch;
                    branch = llvm::cast<BranchInst>(predBB->getTerminator());
                    IGC_ASSERT(branch != nullptr);

                    for (unsigned i = 0; i < branch->getNumSuccessors(); i++)
                    {
                        if (branch->getSuccessor(i) == loop->getHeader())
                        {
                            branch->setSuccessor(i, retBB);
                        }
                    }
                    cfgChanged = true;
                }
            }
        }

        return cfgChanged;
    }

} // namespace
