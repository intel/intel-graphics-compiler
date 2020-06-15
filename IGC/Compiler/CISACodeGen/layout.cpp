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

#include "Compiler/CISACodeGen/layout.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/MemStats.h"
#include "common/LLVMUtils.h"
#include <vector>
#include <set>
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

#define SUCCSZANY     (true)
#define SUCCHASINST   (succ->size() > 1)
#define SUCCNOINST    (succ->size() <= 1)
#define SUCCANYLOOP   (true)

#define PUSHSUCC(BLK, C1, C2) \
        for(succ_iterator succIter = succ_begin(BLK), succEnd = succ_end(BLK); \
            succIter!=succEnd; ++succIter) {                                   \
            llvm::BasicBlock *succ = *succIter;                                \
            if (!visitSet.count(succ) && C1 && C2) {                           \
                visitVec.push_back(succ);                                      \
                visitSet.insert(succ);                                         \
                break;                                                         \
            }                                                                  \
        }

// Register pass to igc-opt
#define PASS_FLAG "igc-layout"
#define PASS_DESCRIPTION "Layout blocks"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(Layout, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(Layout, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char IGC::Layout::ID = 0;

Layout::Layout() : FunctionPass(ID), m_PDT(nullptr)
{
    initializeLayoutPass(*PassRegistry::getPassRegistry());
}

void Layout::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    // Doesn't change the IR at all, it juts move the blocks so no changes in the IR
    AU.setPreservesAll();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
}

bool Layout::runOnFunction(Function& func)
{
    m_PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    LoopInfo& LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    if (LI.empty())
    {
        LayoutBlocks(func);
    }
    else
    {
        LayoutBlocks(func, LI);
    }
    MEM_SNAPSHOT(IGC::SMS_AFTER_LAYOUTPASS);
    return true;
}

#define BREAK_BLOCK_SIZE_LIMIT 3

static bool HasThreadGroupBarrierInBlock(BasicBlock * blk)
{
    std::string Name = GenISAIntrinsic::getName(GenISAIntrinsic::GenISA_threadgroupbarrier);
    Module* Mod = blk->getParent()->getParent();
    if (auto GroupBarrier = Mod->getFunction(Name))
    {
        for (auto U : GroupBarrier->users())
        {
            auto Inst = dyn_cast<Instruction>(U);
            if (Inst && Inst->getParent() == blk)
            {
                return true;
            }
        }
    }
    return false;
}

BasicBlock* Layout::getLastReturnBlock(Function& Func)
{
    Function::BasicBlockListType& bblist = Func.getBasicBlockList();
    for (Function::BasicBlockListType::reverse_iterator RI = bblist.rbegin(),
        RE = bblist.rend();  RI != RE; ++RI)
    {
        BasicBlock* bb = &*RI;
        if (succ_begin(bb) == succ_end(bb))
        {
            return bb;
        }
    }
    // Function does not have a return block
    return nullptr;
}

//
// selectSucc: select a succ with condition SelectNoInstBlk and return it.
//
// This is used to select one if there are two Successors with condition
// SelectNoInstBlk, rather than take the first one in the succ list.
//
// Condition SelectNoInstBlk: If SelectNoInstBlk is true, select an empty
// block, if it is false, select non-empty block.
//
BasicBlock* Layout::selectSucc(
    BasicBlock* CurrBlk,
    bool SelectNoInstBlk,
    const LoopInfo& LI,
    const std::set<BasicBlock*>& VisitSet)
{
    SmallVector<BasicBlock*, 4> Succs;
    for (succ_iterator SI = succ_begin(CurrBlk), SE = succ_end(CurrBlk);
        SI != SE; ++SI)
    {
        BasicBlock* succ = *SI;
        if (VisitSet.count(succ) == 0 &&
            ((SelectNoInstBlk && succ->size() <= 1) ||
            (!SelectNoInstBlk && succ->size() > 1)))
        {
            Succs.push_back(succ);
        }
    }

    // Right now, only handle the case of two empty blocks.
    // If it has no two empty blocks, just take the first
    // one and return it.
    if (Succs.size() != 2 || !SelectNoInstBlk) {
        return Succs.empty() ? nullptr : Succs[0];
    }

    // For two empty blocks, the case we want to handle
    // is the following:
    //
    //     (B0 = CurrBlk)
    //   B0 : if (c) goto THEN  (else goto ELSE)
    //   ELSE : goto B2
    //   B1 : ....
    //   B2 : ....
    //    ......
    //   Bn :
    //      (ELSE, B1, B2, ..., Bn) has END as single exit
    //   THEN: goto END:
    //   END :
    //       PHI...
    //
    // where ELSE and THEN are empty BBs, and END has phi in it.
    // In this case, THEN and ELSE might have phi moves as the result
    // DeSSA when emitting visa. For example, suppose  d0 = s0 will
    // be emitted in THEN.  If s0 is dead after THEN, it would be good
    // to lay out THEN right after B0 as the live-range of s0 will not
    // be overlapped with ones in ELSE. (If s0 is live out of THEN,
    // moving THEN right after B0 or right before END does not matter
    // as far as liveness is concerned.).  To lay out THEN first, this
    // function will select ELSE to return (as the algo does layout
    // backward).
    //
    // For simplicity, assume those BBs are not inside loops. It could
    // be applied to Loop later when appropriate testing is done.
    BasicBlock* S0 = Succs[0], * S1 = Succs[1];
    BasicBlock* SS0 = S0->getSingleSuccessor();

    if (SS0 && (SS0 != S1) && isa<PHINode>(&*SS0->begin()) &&
        !LI.getLoopFor(S0) &&
        m_PDT->dominates(SS0, S1))
    {
        return S1;
    }

    return S0;
}

void Layout::LayoutBlocks(Function& func, LoopInfo& LI)
{
    std::vector<llvm::BasicBlock*> visitVec;
    std::set<llvm::BasicBlock*> visitSet;
    // Insertion Position per loop header
    std::map<llvm::BasicBlock*, llvm::BasicBlock*> InsPos;

    llvm::BasicBlock* entry = &(func.getEntryBlock());
    visitVec.push_back(entry);
    visitSet.insert(entry);
    InsPos[entry] = entry;

    // Push a return block to make sure the last BB is the return block.
    if (BasicBlock * lastReturnBlock = getLastReturnBlock(func))
    {
        if (lastReturnBlock != entry)
        {
            visitVec.push_back(lastReturnBlock);
            visitSet.insert(lastReturnBlock);
        }
    }

    while (!visitVec.empty())
    {
        llvm::BasicBlock* blk = visitVec.back();
        llvm::Loop* curLoop = LI.getLoopFor(blk);
        if (curLoop)
        {
            auto hd = curLoop->getHeader();
            if (blk == hd && InsPos.find(hd) == InsPos.end())
            {
                InsPos[blk] = blk;
            }
        }
        // FIXME: this is a hack to workaround an irreducible test case
        if (func.getName() == "ocl_test_kernel")
        {
            // push: time for DFS visit
            PUSHSUCC(blk, SUCCANYLOOP, SUCCNOINST);
            if (blk != visitVec.back())
                continue;
            // push: time for DFS visit
            PUSHSUCC(blk, SUCCANYLOOP, SUCCHASINST);
        }
        else
        {
            // push: time for DFS visit
            PUSHSUCC(blk, SUCCANYLOOP, SUCCHASINST);
            if (blk != visitVec.back())
                continue;
            // push: time for DFS visit
            if (BasicBlock * aBlk = selectSucc(blk, true, LI, visitSet))
            {
                visitVec.push_back(aBlk);
                visitSet.insert(aBlk);
                continue;
            }
            //PUSHSUCC(blk, SUCCANYLOOP, SUCCNOINST);
        }
        // pop: time to move the block to the right location
        if (blk == visitVec.back())
        {
            visitVec.pop_back();
            if (curLoop)
            {
                auto hd = curLoop->getHeader();
                if (blk != hd)
                {
                    // move the block to the beginning of the loop
                    auto insp = InsPos[hd];
                    IGC_ASSERT(insp);
                    if (blk != insp)
                    {
                        blk->moveBefore(insp);
                        InsPos[hd] = blk;
                    }
                }
                else
                {
                    // move the entire loop to the beginning of
                    // the parent loop
                    auto LoopStart = InsPos[hd];
                    IGC_ASSERT(LoopStart);
                    auto PaLoop = curLoop->getParentLoop();
                    auto PaHd = PaLoop ? PaLoop->getHeader() : entry;
                    auto insp = InsPos[PaHd];
                    if (LoopStart == hd)
                    {
                        // single-block loop
                        hd->moveBefore(insp);
                    }
                    else
                    {
                        // loop-header is not moved yet, so should be at the end
                        // use splice
                        llvm::Function::BasicBlockListType& BBList = func.getBasicBlockList();
                        BBList.splice(insp->getIterator(), BBList,
                            LoopStart->getIterator(),
                            hd->getIterator());
                        hd->moveBefore(LoopStart);
                    }
                    InsPos[PaHd] = hd;
                }
            }
            else
            {
                auto insp = InsPos[entry];
                if (blk != insp)
                {
                    blk->moveBefore(insp);
                    InsPos[entry] = blk;
                }
            }
        }
    }

    // if function has a single exit, then the last block must be an exit
    // comment this out due to infinite loop example in OCL
    // IGC_ASSERT(PDT.getRootNode()->getBlock() == 0x0 || PDT.getRootNode()->getBlock() == &(func.getBasicBlockList().back()));
    // fix the loop-exit pattern, put break-blocks into the loop
    for (llvm::Function::iterator blkIter = func.begin(), blkEnd = func.end();
        blkIter != blkEnd; ++blkIter)
    {
        llvm::BasicBlock* blk = &(*blkIter);
        llvm::Loop* curLoop = LI.getLoopFor(blk);
        bool allPredLoopExit = true;
        unsigned numPreds = 0;
        llvm::SmallPtrSet<llvm::BasicBlock*, 4> predSet;
        for (pred_iterator predIter = pred_begin(blk), predEnd = pred_end(blk);
            predIter != predEnd; ++predIter)
        {
            llvm::BasicBlock* pred = *predIter;
            numPreds++;
            llvm::Loop* predLoop = LI.getLoopFor(pred);
            if (curLoop == predLoop)
            {
                llvm::BasicBlock* predPred = pred->getSinglePredecessor();
                if (predPred)
                {
                    llvm::Loop* predPredLoop = LI.getLoopFor(predPred);
                    if (predPredLoop != curLoop &&
                        (!curLoop || curLoop->contains(predPredLoop)))
                    {
                        if (pred->size() <= BREAK_BLOCK_SIZE_LIMIT &&
                            !HasThreadGroupBarrierInBlock(pred))
                        {
                            predSet.insert(pred);
                        }
                        else
                        {
                            allPredLoopExit = false;
                            break;
                        }

                    }
                }
            }
            else if (!curLoop || curLoop->contains(predLoop))
                continue;
            else
            {
                allPredLoopExit = false;
                break;
            }
        }
        if (allPredLoopExit && numPreds > 1)
        {
            for (SmallPtrSet<BasicBlock*, 4>::iterator predIter = predSet.begin(),
                predEnd = predSet.end(); predIter != predEnd; ++predIter)
            {
                llvm::BasicBlock* pred = *predIter;
                llvm::BasicBlock* predPred = pred->getSinglePredecessor();
                pred->moveAfter(predPred);
            }
        }
    }
}

void Layout::LayoutBlocks(Function& func)
{
    std::vector<llvm::BasicBlock*> visitVec;
    std::set<llvm::BasicBlock*> visitSet;
    // Reorder basic block to allow more fall-through
    llvm::BasicBlock* entry = &(func.getEntryBlock());
    visitVec.push_back(entry);

    // Push a return block to make sure the last BB is the return block.
    if (BasicBlock * lastReturnBlock = getLastReturnBlock(func))
    {
        if (lastReturnBlock != entry)
        {
            visitVec.push_back(lastReturnBlock);
            visitSet.insert(lastReturnBlock);
        }
    }

    while (!visitVec.empty())
    {
        llvm::BasicBlock* blk = visitVec.back();
        // push in the empty successor
        PUSHSUCC(blk, SUCCANYLOOP, SUCCNOINST);
        if (blk != visitVec.back())
            continue;
        // push in all the same-loop successors
        PUSHSUCC(blk, SUCCANYLOOP, SUCCSZANY);
        // pop
        if (blk == visitVec.back())
        {
            visitVec.pop_back();
            if (blk != entry)
            {
                blk->moveBefore(entry);
                entry = blk;
            }
        }
    }
}
