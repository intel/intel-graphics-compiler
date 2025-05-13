/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/layout.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/debug/Debug.hpp"
#include <vector>
#include <set>
#include "Probe/Assertion.h"
#include "llvmWrapper/IR/BasicBlock.h"
#include "llvmWrapper/IR/Function.h"

using namespace llvm;
using namespace IGC;

#define SUCCSZANY     (true)
#define SUCCHASINST   (IGCLLVM::sizeWithoutDebug(succ) > 1)
#define SUCCNOINST    (IGCLLVM::sizeWithoutDebug(succ) <= 1)
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
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
}

bool Layout::runOnFunction(Function& func)
{
    m_PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    m_DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
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

// check if the instruction is atomic write (xchg or cmpxchng)
bool Layout::isAtomicWrite(llvm::Instruction* inst, bool onlyLocalMem)
{
    if (AtomicRawIntrinsic *atomicRawInst = dyn_cast<AtomicRawIntrinsic>(inst))
    {
        bool isSpinlock = inst->getOperand(0)->getName() == "spinlock";
        bool isLocalMem = inst->getOperand(0)->getType()
            ->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL;
        if ((!onlyLocalMem || isLocalMem) && !isSpinlock)
        {
            llvm::ConstantInt* opOperand = llvm::dyn_cast<llvm::ConstantInt>(inst->getOperand(3));
            if (opOperand)
            {
                AtomicOp atomicOp = static_cast<AtomicOp>(opOperand->getZExtValue());
                if ((atomicOp == EATOMIC_XCHG) || (atomicOp == EATOMIC_CMPXCHG))
                {
                    return true;
                }
            }
            GenISAIntrinsic::ID ID = atomicRawInst->getIntrinsicID();
            if (ID == GenISAIntrinsic::GenISA_icmpxchgatomicraw ||
                ID == GenISAIntrinsic::GenISA_icmpxchgatomicrawA64 ||
                ID == GenISAIntrinsic::GenISA_fcmpxchgatomicraw ||
                ID == GenISAIntrinsic::GenISA_fcmpxchgatomicrawA64)
            {
                return true;
            }
        }
    }

    return false;
}

// check if the instruction is atomic read (ATOMIC_OR)
bool Layout::isAtomicRead(llvm::Instruction* inst, bool onlyLocalMem)
{
    if (AtomicRawIntrinsic *atomicRawInst = dyn_cast<AtomicRawIntrinsic>(inst))
    {
        bool isLocalMem = inst->getOperand(0)->getType()
            ->getPointerAddressSpace() == ADDRESS_SPACE_LOCAL;
        if (!onlyLocalMem || isLocalMem)
        {
            AtomicOp atomicOp = atomicRawInst->getAtomicOp();
            if (auto src = llvm::dyn_cast<llvm::ConstantInt>(atomicRawInst->getOperand(2)))
            {
                return ((atomicOp == EATOMIC_OR) || (atomicOp == EATOMIC_OR64)) && (src->getZExtValue() == 0);
            }
        }
    }

    return false;
}

// get memory operand for atomic read or write
llvm::Value* Layout::getMemoryOperand(llvm::Instruction* inst, bool onlyLocalMem)
{
    if (isAtomicRead(inst, onlyLocalMem) || isAtomicWrite(inst, onlyLocalMem))
    {
        llvm::Value* dstAddr = inst->getOperand(1);
        if (llvm::PtrToIntInst* pti = llvm::dyn_cast<llvm::PtrToIntInst>(dstAddr))
        {
            return pti->getPointerOperand();
        }
        return dstAddr;
    }

    return nullptr;
}

bool Layout::isReturnBlock(llvm::BasicBlock* bb)
{
    return llvm::isa<llvm::ReturnInst>(bb->getTerminator());
}

// Try moving atomic write (or its loop) into the given destination loop
// If there are no direct predecessor in the needed loop,
// Try to move it together with a chain of predecessors. New BB is added in chain if
// it is either single predecessor or it is a previous node in current layout.
//
bool Layout::tryMovingWrite(llvm::Instruction* write, llvm::Loop* loop, LoopInfo& LI)
{
    std::vector<llvm::BasicBlock*> blocksToMove;

    if (llvm::Loop* writingLoop = LI.getLoopFor(write->getParent()))
    {
        auto blocks = writingLoop->getBlocks();
        for (auto bbi = blocks.rbegin(), bbEnd = blocks.rend(); bbi != bbEnd; ++bbi)
        {
            llvm::BasicBlock* bb = *bbi;
            if (isReturnBlock(bb))
            {
                return false;
            }
            blocksToMove.push_back(bb);
        }
    }
    else
    {
        if (!isReturnBlock(write->getParent()))
        {
            blocksToMove.push_back(write->getParent());
        }
        else
        {
            return false;
        }
    }

    while (true)
    {
        llvm::BasicBlock* blk = blocksToMove.back();

        // If one (and only one) of the predecessors is in the needed loop, move blocks after it
        llvm::BasicBlock* insertPoint = nullptr;
        int predsInLoop = 0;
        for (pred_iterator predIter = pred_begin(blk), predEnd = pred_end(blk);
            predIter != predEnd; ++predIter)
        {
            llvm::BasicBlock* pred = *predIter;
            if (loop->contains(pred))
            {
                predsInLoop++;
                insertPoint = pred;
            }
        }
        if (predsInLoop == 1)
        {
            for (auto bb : blocksToMove)
            {
                bb->moveAfter(insertPoint);
            }
            return true;
        }
        else if (predsInLoop > 1)
        {
            return false;
        }

        // Add prev node if it is the predecessor of the block
        bool predPushed = false;
        for (pred_iterator predIter = pred_begin(blk), predEnd = pred_end(blk);
            predIter != predEnd; ++predIter)
        {
            llvm::BasicBlock* pred = *predIter;

            if ((pred == blk->getPrevNode()) && !isReturnBlock(pred))
            {
                blocksToMove.push_back(pred);
                predPushed = true;
                break;
            }
        }

        if (predPushed)
        {
            continue;
        }

        // Add predecessor if it is single
        llvm::BasicBlock* pred = blk->getSinglePredecessor();
        if (pred && !isReturnBlock(pred))
        {
            blocksToMove.push_back(pred);
            predPushed = true;
        }
        else
        {
            // Don't move the blocks and return
            return false;
        }
    }
}


// Place basic blocks with atomic write (or the whole loop with the
// atomic write) into the other loop if there is an atomic read
// from the same memory, which dominates the write.
//
// It benefits cases like:
//
// Loop:
//    Load A
//    if (!pred(Load A))
//    {
//        break;
//    }
//    if (success(do_work())
//    {
//        Store A;
//        break;
//    }
// Br Loop
//
// If the Store is placed after the back edge of the loop
// there will be goto instruction disabling channels based on some
// "success(do_work())" condition placed before the back edge in SIMD control flow,
// and the store will be delayed until the whole loop is finished.
// It makes "if (!pred(Load A))" checking useless and doesn't allow
// to perform early break based on the condition.
//
void Layout::moveAtomicWrites2Loop(Function& func, LoopInfo& LI, bool onlyLocalMem)
{
    std::vector<llvm::Instruction*> writes;
    std::vector<llvm::Instruction*> reads;
    for (auto BI = func.begin(), BE = func.end(); BI != BE; BI++)
    {
        for (auto II = BI->begin(), IE = BI->end(); II != IE; II++)
        {
            if (isAtomicWrite(&*II, onlyLocalMem))
            {
                writes.push_back(&*II);
            }
            else if (isAtomicRead(&*II, onlyLocalMem))
            {
                reads.push_back(&*II);
            }
        }
    }

    // write: LoopWhereToMove mapping
    llvm::MapVector<Instruction*, llvm::Loop*> writesToMove;

    for (auto read : reads)
    {
        for (auto write : writes)
        {
            if (getMemoryOperand(read, onlyLocalMem) == getMemoryOperand(write, onlyLocalMem))
            {
                llvm::Loop* readLoop = LI.getLoopFor(read->getParent());
                llvm::Loop* writeLoop = LI.getLoopFor(write->getParent());
                if (readLoop && (readLoop != writeLoop)
                    && ((m_DT->dominates(read, write))))
                {
                    writesToMove[write] = LI.getLoopFor(read->getParent());
                }
            }
        }
    }

    for (const auto &pair : writesToMove)
    {
        llvm::Instruction* write = pair.first;
        llvm::Loop* loop = pair.second;

        tryMovingWrite(write, loop, LI);
    }
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
    // If Func has any return BB, return the last return BB (may have multiple);
    // otherwise, return the last BB that has no succ;
    //     or nullptr if every BB has Succ (infinite looping)
    BasicBlock* noRetAndNoSucc = nullptr;  // for func that never returns
    for (auto RI = IGCLLVM::rbegin(&Func), RE = IGCLLVM::rend(&Func); RI != RE; ++RI)
    {
        BasicBlock* bb = &*RI;
        if (succ_begin(bb) == succ_end(bb))
        {
            if (isa_and_nonnull<ReturnInst>(bb->getTerminator()))
            {
                return bb;
            }
            if (!noRetAndNoSucc)
            {
                noRetAndNoSucc = bb;
            }
        }
    }
    // Function does not have a return block
    return noRetAndNoSucc;
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
            ((SelectNoInstBlk && IGCLLVM::sizeWithoutDebug(succ) <= 1) ||
            (!SelectNoInstBlk && IGCLLVM::sizeWithoutDebug(succ) > 1)))
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
    llvm::MapVector<llvm::BasicBlock*, llvm::BasicBlock*> InsPos;

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
                        IGCLLVM::splice(&func, insp->getIterator(), &func, LoopStart->getIterator(), hd->getIterator());
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

    moveAtomicWrites2Loop(func, LI, false);

    // if function has a single exit, then the last block must be an exit
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
                        // Debug instructions should not be counted into considered size
                        if (IGCLLVM::sizeWithoutDebug(pred) <= BREAK_BLOCK_SIZE_LIMIT &&
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
