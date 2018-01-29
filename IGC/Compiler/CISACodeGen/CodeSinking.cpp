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
/**
 * Originated from llvm code-sinking, need add their copyright
 **/
//===-- Sink.cpp - Code Sinking -------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/***
 * The 1st application of code-sinking is to reduce register pressure:
 * - move input instructions close to its uses
 * - move flag-register definition close to its uses (yet to be done)
 *
 * The 2nd application of code-sinking is to move high-cost operation, 
 * like texture sample, into the control-flow branch that is really used. 
 * However, I think this 2nd goal should only be allowed only if it does 
 * not increase register pressure.
 *
 * step 1. efficient code-sinking limited to inputs and cmps
 * step 2. efficient code-sinking into branch, limited to read-only memory op
 *         and alu operations
 *
 * Two tuning paramters for code-sinking:
 * - General code-sinking, enable code-sinking of step 2
 * - Register-pressure threshold, undo code-sinking when live-out pressure is high 
 */
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/Stats.hpp"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/CFG.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/CodeSinking.hpp"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
using namespace IGC::Debug;


namespace IGC {

// Register pass to igc-opt
#define PASS_FLAG "code sinking"
#define PASS_DESCRIPTION "code sinking"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

CodeSinking::CodeSinking(bool generalSinking, unsigned pressureThreshold) : FunctionPass(ID) {
    generalCodeSinking = generalSinking;
    registerPressureThreshold = pressureThreshold;
    initializeCodeSinkingPass(*PassRegistry::getPassRegistry());
}

/// AllUsesDominatedByBlock - Return true if all uses of the specified value
/// occur in blocks dominated by the specified block.
bool CodeSinking::AllUsesDominatedByBlock(Instruction *inst,
                                          BasicBlock *blk,
                                          SmallPtrSetImpl<Instruction*> &usesInBlk) const 
{
    usesInBlk.clear();
    // Ignoring debug uses is necessary so debug info doesn't affect the code.
    // This may leave a referencing dbg_value in the original block, before
    // the definition of the vreg.  Dwarf generator handles this although the
    // user might not get the right info at runtime.
    for (Value::user_iterator I = inst->user_begin(), E = inst->user_end(); I != E; ++I) 
    {
        // Determine the block of the use.
        Instruction *useInst = cast<Instruction>(*I);
        BasicBlock *useBlock = useInst->getParent();
        if (useBlock == blk) 
        {
            usesInBlk.insert(useInst);
        }
        if (PHINode *PN = dyn_cast<PHINode>(useInst))
        {
            // PHI nodes use the operand in the predecessor block, 
            // not the block with the PHI.
            Use &U = I.getUse();
            unsigned num = PHINode::getIncomingValueNumForOperand(U.getOperandNo());
            useBlock = PN->getIncomingBlock(num);
        }
        // Check that it dominates.
        if (!DT->dominates(blk, useBlock))
            return false;
    }
    return true;
}

/// return false if instruction cannot be moved to another block
bool CodeSinking::FindLowestSinkTarget( Instruction *inst,
                                        BasicBlock* &tgtBlk,
                                        SmallPtrSetImpl<Instruction*> &usesInBlk,
                                        bool& outerLoop)
{
    usesInBlk.clear();
    tgtBlk = 0x0;
    outerLoop = false;
    for (Value::user_iterator I = inst->user_begin(), E = inst->user_end(); I != E; ++I) 
    {
        // Determine the block of the use.
        Instruction *useInst = cast<Instruction>(*I);
        BasicBlock *useBlock = useInst->getParent();
        if (PHINode *PN = dyn_cast<PHINode>(useInst))
        {
            // PHI nodes use the operand in the predecessor block, 
            // not the block with the PHI.
            Use &U = I.getUse();
            unsigned num = PHINode::getIncomingValueNumForOperand(U.getOperandNo());
            useBlock = PN->getIncomingBlock(num);
        }
        else
        {
            if (useBlock == inst->getParent())
            {
                return false;
            }
        }
        if (tgtBlk == 0x0)
        {
            tgtBlk = useBlock;
        }
        else
        {
            tgtBlk = DT->findNearestCommonDominator(tgtBlk, useBlock);
            if (tgtBlk == 0x0)
                break;
        }
    }
    BasicBlock *curBlk = inst->getParent();
    Loop *curLoop = LI->getLoopFor(inst->getParent());
    while (tgtBlk && tgtBlk != curBlk)
    {
        Loop *tgtLoop = LI->getLoopFor(tgtBlk);
        EOPCODE intrinsic_name = GetOpCode(inst);
        // sink the pln instructions in the loop to reduce pressure
        if(intrinsic_name == llvm_input ||
           !tgtLoop || tgtLoop->contains(curLoop))
        {
            for (Value::user_iterator I = inst->user_begin(), E = inst->user_end(); I != E; ++I) 
            {
                // Determine the block of the use.
                Instruction *useInst = cast<Instruction>(*I);
                BasicBlock *useBlock = useInst->getParent();
                if (useBlock == tgtBlk) 
                {
                    usesInBlk.insert(useInst);
                }
            }
            outerLoop = (tgtLoop != curLoop);
            return true;
        }
        else
        {
            tgtBlk = DT->getNode(tgtBlk)->getIDom()->getBlock();
        }
    }

    return false;
}

static unsigned numInsts(const Function &F)
{
    unsigned num = 0;
    for (auto &BB : F)
    {
        num += BB.getInstList().size();
    }
    return num;
}

//diagnosis code: __declspec(thread) int sinkCounter = 0;
//diagnosis code: const int sinkLimit = 19;

bool CodeSinking::runOnFunction(Function &F)
{
    CodeGenContext *ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    // only limited code-sinking to several shader-type
    // vs input has the URB-reuse issue to be resolved. 
    // Also need to understand the performance benefit better.
    if(ctx->type != ShaderType::PIXEL_SHADER &&
        ctx->type != ShaderType::DOMAIN_SHADER &&
        ctx->type != ShaderType::OPENCL_SHADER &&
        ctx->type != ShaderType::COMPUTE_SHADER)
    {
        return false;
    }
    if(IGC_IS_FLAG_ENABLED(DisableCodeSinking) ||
        numInsts(F) < CODE_SINKING_MIN_SIZE)
    {
        return false;
    }

    DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    DL = &F.getParent()->getDataLayout();

    bool madeChange, everMadeChange = false;
    totalGradientMoved = 0;
    // diagnosis code: numChanges = 0;
    // diagnosis code: if (sinkCounter >= 94 && sinkCounter < 95) {
    // even if we limit code-sinking to ps-input instructions, we still need to iterate through
    // all the blocks because llvm-InstCombine may have sinked some ps-input instructions out of entry-block
    do 
    {
        madeChange = false;
        // Process all basic blocks in dominator-tree post-order
        for(po_iterator<DomTreeNode*> domIter = po_begin( DT->getRootNode() ),
            domEnd = po_end( DT->getRootNode() ); domIter != domEnd; ++domIter )
        {
            madeChange |= ProcessBlock(*(domIter->getBlock()));
        }
    } while (madeChange /*diagnosis: && numChanges < sinkLimit*/);

    everMadeChange = madeChange;
    for (SmallPtrSet<BasicBlock*, 8>::iterator BI = localBlkSet.begin(), BE = localBlkSet.end(); BI != BE; BI++)
    {
        madeChange = LocalSink(*BI);
        everMadeChange |= madeChange;
    }
    localBlkSet.clear();
    localInstSet.clear();
    ctx->m_numGradientSinked = totalGradientMoved;

    // diagnosis code: printf("%d:%d:%x\n", sinkCounter, sinkLimit, ctx->hash.getAsmHash());
    //F.viewCFG();
    // } end of diagnosis if 
    // diagnosis code: sinkCounter++;
    return everMadeChange;
}

static uint EstimateLiveOutPressure(BasicBlock *blk, const DataLayout *DL)
{
    // Walk the basic block bottom-up.  Remember if we saw a store.
    uint pressure = 0;
    BasicBlock::iterator I = blk->end();
    --I;
    bool processedBegin = false;
    do {
        Instruction *inst = &(*I);  // The instruction to sink.

        // Predecrement I (if it's not begin) so that it isn't invalidated by sinking.
        processedBegin = (I == blk->begin());
        if (!processedBegin)
            --I;

        if (isa<DbgInfoIntrinsic>(inst))
            continue;
        // intrinsic like discard has no explicit use, get skipped here
        if (inst->use_empty())
            continue;

        bool useOutside = false;
        for (Value::user_iterator useI = inst->user_begin(), useE = inst->user_end(); 
             !useOutside && useI != useE; ++useI) 
        {
            // Determine the block of the use.
            Instruction *useInst = cast<Instruction>(*useI);
            BasicBlock *useBlock = useInst->getParent();
            if (useBlock != blk) 
            {
                if (PHINode *PN = dyn_cast<PHINode>(useInst))
                {
                    // PHI nodes use the operand in the predecessor block, 
                    // not the block with the PHI.
                    Use &U = useI.getUse();
                    unsigned num = PHINode::getIncomingValueNumForOperand(U.getOperandNo());
                    if (PN->getIncomingBlock(num) != blk)
                    {
                        useOutside = true;
                    }
                }
                else
                {
                    useOutside = true;
                }
            }
        }
        
        // estimate register usage by value
        if (useOutside)
        {
            pressure += (uint)(DL->getTypeAllocSize(inst->getType()));
        }
        // If we just processed the first instruction in the block, we're done.
    } while (!processedBegin);
    return pressure;
}

bool CodeSinking::ProcessBlock(BasicBlock &blk) 
{
    if (blk.empty())
        return false;

    uint pressure0 = 0;
    if (generalCodeSinking && registerPressureThreshold)
    {
        // estimate live-out register pressure for this blk
        pressure0 = EstimateLiveOutPressure(&blk, DL);
    }

    bool madeChange = false;
    numGradientMovedOutBB = 0;

    // Walk the basic block bottom-up.  Remember if we saw a store.
    BasicBlock::iterator I = blk.end();
    --I;
    bool processedBegin = false;
    SmallPtrSet<Instruction *, 16> stores;
    undoLocas.clear();
    movedInsts.clear();
    Instruction *prevLoca = 0x0;
    do {
        Instruction *inst = &(*I);  // The instruction to sink.

        // Predecrement I (if it's not begin) so that it isn't invalidated by sinking.
        processedBegin = (I == blk.begin());
        if (!processedBegin)
            --I;

        if (inst->mayWriteToMemory())
        {
            stores.insert(inst);
            prevLoca = inst;
        }
        // intrinsic like discard has no explict use, gets skipped here
        else if (isa<DbgInfoIntrinsic>(inst) || isa<TerminatorInst>(inst) || 
                 isa<PHINode>(inst) || inst->use_empty() )
        {
            prevLoca = inst;
        }
        else {
            Instruction *undoLoca = prevLoca;
            prevLoca = inst;
            // diagnosis code: if (numChanges >= sinkLimit)
            // diagnosis code:    continue;
            if (SinkInstruction(inst, stores))
            {
                madeChange = true;
                movedInsts.push_back(inst);
                undoLocas.push_back(undoLoca);
                // diagnosis code: numChanges++;
            }
        }
        // If we just processed the first instruction in the block, we're done.
    } while (!processedBegin);

    if (generalCodeSinking && registerPressureThreshold)
    {
        if (madeChange)
        {
            // measure the live-out register pressure again
            uint pressure1 = EstimateLiveOutPressure(&blk, DL);
            if (pressure1 > pressure0 + registerPressureThreshold)
            {
                // undo code motion
                int numChanges = movedInsts.size();
                for (int i = 0; i < numChanges; ++i)
                {
                    Instruction *undoLoca = undoLocas[i];
                    assert(undoLoca);
                    movedInsts[i]->moveBefore(undoLoca);
                }
                madeChange = false;
            }
            else
            {
                totalGradientMoved += numGradientMovedOutBB;
            }
        }
    }

    return madeChange;
}

static bool reduceRP(Instruction *Inst)
{
    // igc specific: to make the flag-register lifetime short
    if (isa<CmpInst>(Inst))
    {
        return true;
    }

    if (auto CI = dyn_cast<CastInst>(Inst))
    {
        unsigned SrcSize = CI->getSrcTy()->getPrimitiveSizeInBits();
        unsigned DstSize = CI->getDestTy()->getPrimitiveSizeInBits();
        if (SrcSize == 0 || DstSize == 0)
        {
            // Non-primitive types.
            return false;
        }
        if (SrcSize == 1)
        {
            // i1 -> i32, reduces GRF pressure but increases flag pressure.
            // Do not consider it as reduce.
            return false;
        }
        else if (DstSize == 1)
        {
            // i32 -> i1, reduces flag pressure but increases grf pressure.
            // Consider it as reduce.
            return true;
        }
        else if (SrcSize < DstSize)
        {
            // sext i32 to i64.
            return true;
        }
    }

    return false;
}

bool CodeSinking::isSafeToMove(Instruction *inst
                         , bool &reducePressure
                         , bool &hasAliasConcern
                         , SmallPtrSetImpl<Instruction *> &Stores)
{
    if (isa<AllocaInst>(inst) || isa<ExtractValueInst>(inst))
    {
        return false;
    }
    if (isa<CallInst>(inst) && cast<CallInst>(inst)->isConvergent())
    {
        return false;
    }
    hasAliasConcern = true;
    reducePressure = false;
    if (generalCodeSinking)
    {
        if (isa<GetElementPtrInst>(inst) ||
            isa<ExtractElementInst>(inst) ||
            isa<InsertElementInst>(inst) ||
            isa<InsertValueInst>(inst) ||
            (isa<UnaryInstruction>(inst) && !isa<LoadInst>(inst)) ||
            isa<BinaryOperator>(inst))
        {
            hasAliasConcern = false;
            reducePressure = reduceRP(inst);
            return true;
        }
    }
    if (isa<CmpInst>(inst))
    {
        hasAliasConcern = false;
        reducePressure = true;
        return true;
    }
    EOPCODE intrinsic_name = GetOpCode(inst);
    if (intrinsic_name == llvm_input ||
        intrinsic_name == llvm_shaderinputvec)
    {
        hasAliasConcern = false;
        reducePressure = true;
        return true;
    }
    if (generalCodeSinking)
    {
        if (IsMathIntrinsic(intrinsic_name) || IsGradientIntrinsic(intrinsic_name))
        {
            hasAliasConcern = false;
            reducePressure = false;
            return true;
        }
        if (isSampleInstruction(inst) || isGather4Instruction(inst) ||
            isInfoInstruction(inst) || isLdInstruction(inst))
        {
            if(!inst->mayReadFromMemory())
            {
                hasAliasConcern = false;
                return true;
            }
        }
        if (isSubGroupIntrinsic(inst))
        {
            return false;
        }

        if (LoadInst *load = dyn_cast<LoadInst>(inst))
        {
            if (load->isVolatile())
                return false;

            BufferType bufType = GetBufferType(load->getPointerAddressSpace());
            if (bufType == CONSTANT_BUFFER || bufType == RESOURCE)
            {
                hasAliasConcern = false;
                return true;
            }
            if (!Stores.empty())
            {
                return false;
            }
        }
        else
        if (SamplerLoadIntrinsic* intrin = dyn_cast<SamplerLoadIntrinsic>(inst))
        {
            Value* texture = intrin->getTextureValue();
            if (texture->getType()->isPointerTy())
            {
                unsigned as = texture->getType()->getPointerAddressSpace();
                BufferType bufType = GetBufferType(as);
                if (bufType == CONSTANT_BUFFER || bufType == RESOURCE)
                {
                    hasAliasConcern = false;
                    return true;
                }
                else
                {
                    return (Stores.empty());
                }
            }
            else
            {
                hasAliasConcern = false;
                return true;
            }
        }
        else if (inst->mayReadFromMemory())
        {
            return (Stores.empty());
        }

        return true;
    }
    return false;
}

/// SinkInstruction - Determine whether it is safe to sink the specified machine
/// instruction out of its current block into a successor.
bool CodeSinking::SinkInstruction(Instruction *inst
                                  , SmallPtrSetImpl<Instruction *> &Stores)
{
    // Check if it's safe to move the instruction.
    bool hasAliasConcern;
    bool reducePressure;
    if (!isSafeToMove(inst, reducePressure, hasAliasConcern, Stores/*, AA*/))
        return false;

    // SuccToSinkTo - This is the successor to sink this instruction to, once we
    // decide.
    BasicBlock *succToSinkTo = 0;
    SmallPtrSet<Instruction*, 16> usesInBlk;
    if (!hasAliasConcern)
    {
        // find the lowest common dominator of all uses
        BasicBlock *tgtBlk = 0x0;
        bool outerLoop;
        if (FindLowestSinkTarget(inst, tgtBlk, usesInBlk, outerLoop))
        {
            // heuristic, avoid code-motion that does not reduce execution frequency but may increase register usage
            if (reducePressure || 
                (tgtBlk && (outerLoop || !PDT->dominates(tgtBlk, inst->getParent()))))
            {
                succToSinkTo = tgtBlk;
            }
        }
        else
        {
            // local code motion for cases like cmp and pln
            if (reducePressure)
            {
                localBlkSet.insert(inst->getParent());
                localInstSet.insert(inst);
            }
            return false;
        }
    }
    else 
    {
        // when aliasing is a concern, only look at all the immed successors and
        // decide which one we should sink to, if any.
        BasicBlock *curBlk = inst->getParent();
        for (succ_iterator I = succ_begin(inst->getParent()),
             E = succ_end(inst->getParent()); I != E && succToSinkTo == 0; ++I)
        {
            // avoid sinking an instruction into its own block.  This can
            // happen with loops.
            if ( (*I) == curBlk )
                continue;
            // punt on it because of alias concern
            if ( (*I)->getUniquePredecessor() != curBlk )
                continue;
            // Don't move instruction across a loop.
            Loop *succLoop = LI->getLoopFor((*I));
            Loop *currLoop = LI->getLoopFor(curBlk);
            if (succLoop != currLoop)
                continue;
            if (AllUsesDominatedByBlock(inst, (*I), usesInBlk))
                succToSinkTo = *I;
        }
    }

    // If we couldn't find a block to sink to, ignore this instruction.
    if (succToSinkTo == 0)
    {
        return false;
    }

    if (ComputesGradient(inst))
    {
        numGradientMovedOutBB++;
    }

    if (!reducePressure || hasAliasConcern)
    {
        inst->moveBefore(&(*succToSinkTo->getFirstInsertionPt()));
    }
    // when alasing is not an issue and reg-pressure is not an issue
    // move it as close to the uses as possible
    else if (usesInBlk.empty())
    {
        inst->moveBefore(succToSinkTo->getTerminator());
    }
    else if (usesInBlk.size()==1)
    {
        Instruction *use = *(usesInBlk.begin());
        inst->moveBefore(use);
    }
    else
    {
        // first move to the beginning of the target block
        inst->moveBefore(&(*succToSinkTo->getFirstInsertionPt()));
        // later on, move it close to the use
        localBlkSet.insert(succToSinkTo);
        localInstSet.insert(inst);
    }
    return true;
}

bool CodeSinking::LocalSink(BasicBlock *blk)
{
    bool madeChange = false;
    for (BasicBlock::iterator I = blk->begin(), E = blk->end(); I != E; ++I)
    {
        Instruction *use = &(*I);
        for (unsigned i = 0; i < use->getNumOperands(); ++i)
        {
            Instruction *def = dyn_cast<Instruction>(use->getOperand(i));
            if (def && def->getParent() == blk && localInstSet.count(def))
            {
                // "use" can be a phi-node for a single-block loop, 
                // which is not really a local-code-motion
                if (def->getNextNode() != use && !isa<PHINode>(use))
                {
                    def->moveBefore(use);
                    madeChange = true;
                }
                localInstSet.erase(def);
            }
        }
    }
    return madeChange;
}

char CodeSinking::ID = 0;
}
