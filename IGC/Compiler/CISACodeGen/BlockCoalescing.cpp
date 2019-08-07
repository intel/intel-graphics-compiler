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
#include "Compiler/CISACodeGen/BlockCoalescing.hpp"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "common/igc_regkeys.hpp"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

char BlockCoalescing::ID = 0;
#define PASS_FLAG "BlockCoalescing"
#define PASS_DESCRIPTION "Mark empty blocks after deSSA"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(BlockCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DeSSA)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenPatternMatch)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(BlockCoalescing, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC
{

    BlockCoalescing::BlockCoalescing() : FunctionPass(ID)
    {
        initializeBlockCoalescingPass(*PassRegistry::getPassRegistry());
    }

    bool BlockCoalescing::runOnFunction(Function& F)
    {

        MetaDataUtils* pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
        {
            return false;
        }

        // If De-SSA is disabled we cannot remove empty blocks as they might contain move instructions
        if (IGC_IS_FLAG_ENABLED(DisableEmptyBlockRemoval) || IGC_IS_FLAG_ENABLED(DisableDeSSA))
        {
            return false;
        }
        DeSSA* deSSA = &getAnalysis<DeSSA>();
        CodeGenPatternMatch* patternMatch = &getAnalysis<CodeGenPatternMatch>();
        for (uint i = 0; i < patternMatch->m_numBlocks; i++)
        {
            SBasicBlock& block = patternMatch->m_blocks[i];
            // An empty block would have only one pattern matching the branch instruction
            if (block.m_dags.size() == 1)
            {
                if (BranchInst * br = dyn_cast<BranchInst>(block.m_dags[0].m_root))
                {
                    if (br->isUnconditional())
                    {
                        BasicBlock* succ = br->getSuccessor(0);
                        assert(succ && "Branch must have a successor!");
                        if (block.id >= patternMatch->GetBlockId(succ))
                        {
                            if (block.bb->getSinglePredecessor() == nullptr)
                            {
                                // do not remove this BB that goes backward, otherwise
                                // one back edge becomes two back edges, and the 
                                // control-flow reconverge point changes.
                                continue;
                            }
                            else if (IGC_GET_FLAG_VALUE(EnableVISAStructurizer) == FLAG_SCF_Aggressive)
                            {
                                // Do not remove the BB that goes backward, otherwise,
                                // a loop with break will end up with more than one exits,
                                // which will not be recognized as GEN while
                                continue;
                            }
                        }

                        // Make sure that if there is any loop, one of BBs in the loop will
                        // not be in the m_emptyBlocks (eventually condense it to a single BB).
                        if (!hasEmptyBlockLoop(block.bb))
                        {
                            m_emptyBlocks.insert(block.bb);
                        }
                    }
                }
            }
        }
        for (auto& ConstI : patternMatch->ConstantPlacement)
        {
            m_emptyBlocks.erase(ConstI.second);
        }
        for (auto BBI = F.begin(), BBE = F.end(); BBI != BBE; BBI++)
        {
            BasicBlock* bb = &(*BBI);
            for (auto II = BBI->begin(), IE = BBI->end(); II != IE; II++)
            {
                PHINode* phi = dyn_cast<PHINode>(II);
                if (!phi)
                {
                    break;
                }
                if (deSSA->isIsolated(phi))
                {
                    m_emptyBlocks.erase(bb);
                    for (pred_iterator PI = pred_begin(bb), PE = pred_end(bb); PI != PE; PI++)
                    {
                        m_emptyBlocks.erase(*PI);
                    }
                }
                for (unsigned int i = 0, numOperand = phi->getNumIncomingValues(); i != numOperand; i++)
                {
                    if (deSSA->getRootValue(phi->getOperand(i)) == nullptr ||
                        deSSA->getRootValue(phi->getOperand(i)) != deSSA->getRootValue(phi))
                    {
                        if (!isa<UndefValue>(phi->getOperand(i)))
                        {
                            m_emptyBlocks.erase(phi->getIncomingBlock(i));
                        }
                    }
                }
            }
        }
        return false;
    }

    // Check if EmptyBlock (not in m_emptyBlocks, and has single sucessor)
    // could form a loop with BBs in m_emptyBlocks; if so, return true.
    inline bool BlockCoalescing::hasEmptyBlockLoop(BasicBlock* EmptyBlock)
    {
        BasicBlock* succ = EmptyBlock->getTerminator()->getSuccessor(0);

        // No loop formed by BBs in m_emptyBlock, so while will stop.
        //
        // If EmptyBlock would form a loop, it shall go to BBs in
        // m_emptyBlocks and one BB in m_emptyBlocks will goto succ.
        while (m_emptyBlocks.find(succ) != m_emptyBlocks.end())
        {
            succ = succ->getTerminator()->getSuccessor(0);
        }
        return (EmptyBlock == succ);
    }

    bool BlockCoalescing::IsEmptyBlock(BasicBlock* bb)
    {
        return m_emptyBlocks.find(bb) != m_emptyBlocks.end();
    }

    BasicBlock* BlockCoalescing::FollowEmptyBlock(BasicBlock* bb)
    {
        BasicBlock* block = bb;
        while (IsEmptyBlock(block))
        {
            assert(block->getTerminator()->getNumSuccessors() == 1);
            block = block->getTerminator()->getSuccessor(0);
        }
        return block;
    }

    BasicBlock* BlockCoalescing::SkipEmptyBasicBlock(BasicBlock* bb)
    {
        BasicBlock* block = bb;
        while (IsEmptyBlock(block))
        {
            assert(block->getTerminator()->getNumSuccessors() == 1);
            block = block->getNextNode();
        }
        return block;
    }
}
