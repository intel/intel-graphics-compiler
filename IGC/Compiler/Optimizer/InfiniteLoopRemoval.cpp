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
