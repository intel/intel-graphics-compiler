/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

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
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC::Debug;


namespace IGC {

    // Register pass to igc-opt
#define PASS_FLAG "igc-code-sinking"
#define PASS_DESCRIPTION "code sinking"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_END(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        CodeSinking::CodeSinking(bool generalSinking) : FunctionPass(ID) {
        generalCodeSinking = generalSinking;
        initializeCodeSinkingPass(*PassRegistry::getPassRegistry());
    }

    /// AllUsesDominatedByBlock - Return true if all uses of the specified value
    /// occur in blocks dominated by the specified block.
    bool CodeSinking::AllUsesDominatedByBlock(Instruction* inst,
        BasicBlock* blk,
        SmallPtrSetImpl<Instruction*>& usesInBlk) const
    {
        usesInBlk.clear();
        // Ignoring debug uses is necessary so debug info doesn't affect the code.
        // This may leave a referencing dbg_value in the original block, before
        // the definition of the vreg.  Dwarf generator handles this although the
        // user might not get the right info at runtime.
        for (Value::user_iterator I = inst->user_begin(), E = inst->user_end(); I != E; ++I)
        {
            // Determine the block of the use.
            Instruction* useInst = cast<Instruction>(*I);
            BasicBlock* useBlock = useInst->getParent();
            if (useBlock == blk)
            {
                usesInBlk.insert(useInst);
            }
            if (PHINode * PN = dyn_cast<PHINode>(useInst))
            {
                // PHI nodes use the operand in the predecessor block,
                // not the block with the PHI.
                Use& U = I.getUse();
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
    bool CodeSinking::FindLowestSinkTarget(Instruction* inst,
        BasicBlock*& tgtBlk,
        SmallPtrSetImpl<Instruction*>& usesInBlk,
        bool& outerLoop,
        bool doLoopSink)
    {
        usesInBlk.clear();
        tgtBlk = 0x0;
        outerLoop = false;
        for (Value::user_iterator I = inst->user_begin(), E = inst->user_end(); I != E; ++I)
        {
            // Determine the block of the use.
            Instruction* useInst = cast<Instruction>(*I);
            BasicBlock* useBlock = useInst->getParent();
            if (PHINode * PN = dyn_cast<PHINode>(useInst))
            {
                // PHI nodes use the operand in the predecessor block,
                // not the block with the PHI.
                Use& U = I.getUse();
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
        BasicBlock* curBlk = inst->getParent();
        Loop* curLoop = LI->getLoopFor(inst->getParent());
        while (tgtBlk && tgtBlk != curBlk)
        {
            Loop* tgtLoop = LI->getLoopFor(tgtBlk);
            EOPCODE intrinsic_name = GetOpCode(inst);
            // sink the pln instructions in the loop to reduce pressure
            // Sink instruction outside of loop into the loop if doLoopSink is true.
            if (intrinsic_name == llvm_input ||
                (!tgtLoop || tgtLoop->contains(curLoop)) ||
                (doLoopSink && tgtLoop && (!curLoop || curLoop->contains(tgtLoop))))
            {
                for (Value::user_iterator I = inst->user_begin(), E = inst->user_end(); I != E; ++I)
                {
                    // Determine the block of the use.
                    Instruction* useInst = cast<Instruction>(*I);
                    BasicBlock* useBlock = useInst->getParent();
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

    static unsigned numInsts(const Function& F)
    {
        unsigned num = 0;
        for (auto& BB : F)
        {
            num += BB.getInstList().size();
        }
        return num;
    }

    //diagnosis code: __declspec(thread) int sinkCounter = 0;
    //diagnosis code: const int sinkLimit = 19;

    bool CodeSinking::runOnFunction(Function& F)
    {
        CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        // only limited code-sinking to several shader-type
        // vs input has the URB-reuse issue to be resolved.
        // Also need to understand the performance benefit better.
        if (CTX->type != ShaderType::PIXEL_SHADER &&
            CTX->type != ShaderType::DOMAIN_SHADER &&
            CTX->type != ShaderType::OPENCL_SHADER &&
            CTX->type != ShaderType::RAYTRACING_SHADER &&
            CTX->type != ShaderType::COMPUTE_SHADER)
        {
            return false;
        }
        if (IGC_IS_FLAG_ENABLED(DisableCodeSinking) ||
            numInsts(F) < CODE_SINKING_MIN_SIZE)
        {
            return false;
        }

        DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
        PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
        LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
        DL = &F.getParent()->getDataLayout();

        bool changed = hoistCongruentPhi(F);

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
            for (po_iterator<DomTreeNode*> domIter = po_begin(DT->getRootNode()),
                domEnd = po_end(DT->getRootNode()); domIter != domEnd; ++domIter)
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
        CTX->m_numGradientSinked = totalGradientMoved;

        uint32_t GRFThresholdDelta = IGC_GET_FLAG_VALUE(LoopSinkThresholdDelta);
        uint32_t ngrf = CTX->getNumGRFPerThread();
        for (unsigned i = 0, n = m_fatLoops.size(); i < n; ++i)
        {
            auto FatLoop = m_fatLoops[i];
            auto Pressure = m_fatLoopPressures[i];
            // Enable multiple-level loop sink if pressure is high enough
            bool sinkMultiLevel = (Pressure > (2*ngrf + 2 * GRFThresholdDelta));
            if (loopSink(FatLoop, sinkMultiLevel)) {
                changed = true;
            }
        }
        m_fatLoopPressures.clear();
        m_fatLoops.clear();

        // diagnosis code: printf("%d:%d:%x\n", sinkCounter, sinkLimit, CTX->hash.getAsmHash());
        //F.viewCFG();
        // } end of diagnosis if
        // diagnosis code: sinkCounter++;
        return everMadeChange || changed;
    }

    static uint EstimateLiveOutPressure(BasicBlock* blk, const DataLayout* DL)
    {
        // Walk the basic block bottom-up.  Remember if we saw a store.
        uint pressure = 0;
        BasicBlock::iterator I = blk->end();
        --I;
        bool processedBegin = false;
        do {
            Instruction* inst = &(*I);  // The instruction to sink.

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
                Instruction* useInst = cast<Instruction>(*useI);
                BasicBlock* useBlock = useInst->getParent();
                if (useBlock != blk)
                {
                    if (PHINode * PN = dyn_cast<PHINode>(useInst))
                    {
                        // PHI nodes use the operand in the predecessor block,
                        // not the block with the PHI.
                        Use& U = useI.getUse();
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

    Loop* CodeSinking::findLoopAsPreheader(BasicBlock& blk)
    {
        // look through the successors
        for (BasicBlock* succ : successors(&blk))
        {
            Loop* L = LI->getLoopFor(succ);
            if (L && L->getLoopPreheader() == &blk)
                return L;
        }
        return nullptr;
    }

    bool CodeSinking::ProcessBlock(BasicBlock& blk)
    {
        if (blk.empty())
            return false;

        uint32_t registerPressureThreshold = CTX->getNumGRFPerThread();

        uint pressure0 = 0;
        if (generalCodeSinking && registerPressureThreshold)
        {
            // estimate live-out register pressure for this blk
            pressure0 = EstimateLiveOutPressure(&blk, DL);
            uint32_t GRFThresholdDelta = IGC_GET_FLAG_VALUE(LoopSinkThresholdDelta);
            uint32_t ngrf = CTX->getNumGRFPerThread();
            if (pressure0 > (2*ngrf + GRFThresholdDelta) &&
                CTX->type == ShaderType::OPENCL_SHADER)
            {
                if (auto L = findLoopAsPreheader(blk))
                {
                    m_fatLoopPressures.push_back(pressure0);
                    m_fatLoops.push_back(L);
                }
            }
        }

        bool madeChange = false;
        numGradientMovedOutBB = 0;

        // Walk the basic block bottom-up.  Remember if we saw a store.
        BasicBlock::iterator I = blk.end();
        --I;
        bool processedBegin = false;
        bool metDbgValueIntrinsic = false;
        SmallPtrSet<Instruction*, 16> stores;
        undoLocas.clear();
        movedInsts.clear();
        Instruction* prevLoca = 0x0;
        do {
            Instruction* inst = &(*I);  // The instruction to sink.

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
            else if (isa<DbgInfoIntrinsic>(inst) || inst->isTerminator() ||
                isa<PHINode>(inst) || inst->use_empty())
            {
                if (isa<DbgValueInst>(inst))
                {
                    metDbgValueIntrinsic = true;
                }
                prevLoca = inst;
            }
            else {
                Instruction* undoLoca = prevLoca;
                prevLoca = inst;
                // diagnosis code: if (numChanges >= sinkLimit)
                // diagnosis code:    continue;
                if (SinkInstruction(inst, stores, false))
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
                        Instruction* undoLoca = undoLocas[i];
                        IGC_ASSERT(undoLoca);
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
        if (madeChange || metDbgValueIntrinsic) {
            ProcessDbgValueInst(blk);
        }

        return madeChange;
    }

    static bool reduceRP(Instruction* Inst)
    {
        if (auto CI = dyn_cast<CastInst>(Inst))
        {
            unsigned SrcSize = (unsigned int)CI->getSrcTy()->getPrimitiveSizeInBits();
            unsigned DstSize = (unsigned int)CI->getDestTy()->getPrimitiveSizeInBits();
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

    bool CodeSinking::isSafeToMove(Instruction* inst
        , bool& reducePressure
        , bool& hasAliasConcern
        , SmallPtrSetImpl<Instruction*>& Stores)
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
                // sink CmpInst to make the flag-register lifetime short
                reducePressure = (reduceRP(inst) || isa<CmpInst>(inst));
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
            if( IGC_IS_FLAG_ENABLED( DisableCodeSinkingInputVec ) )
            {
                hasAliasConcern = true;
                reducePressure = false;
                return false;
            }
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
                if (!inst->mayReadFromMemory())
                {
                    hasAliasConcern = false;
                    return true;
                }
            }
            if (isSubGroupIntrinsic(inst))
            {
                return false;
            }

            if (LoadInst * load = dyn_cast<LoadInst>(inst))
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
                if (SamplerLoadIntrinsic * intrin = dyn_cast<SamplerLoadIntrinsic>(inst))
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
    bool CodeSinking::SinkInstruction(
        Instruction* inst,
        SmallPtrSetImpl<Instruction*>& Stores,
        bool ForceToReducePressure)
    {
        // Check if it's safe to move the instruction.
        bool hasAliasConcern =false;
        bool reducePressure = false;
        if (!isSafeToMove(inst, reducePressure, hasAliasConcern, Stores/*, AA*/))
            return false;
        if (ForceToReducePressure) {
            reducePressure = true;
        }

        // SuccToSinkTo - This is the successor to sink this instruction to, once we
        // decide.
        BasicBlock* succToSinkTo = 0;
        SmallPtrSet<Instruction*, 16> usesInBlk;
        if (!hasAliasConcern)
        {
            // find the lowest common dominator of all uses
            BasicBlock* tgtBlk = 0x0;
            bool outerLoop = false;
            if (FindLowestSinkTarget(inst, tgtBlk, usesInBlk, outerLoop, ForceToReducePressure))
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
            BasicBlock* curBlk = inst->getParent();
            for (succ_iterator I = succ_begin(inst->getParent()),
                E = succ_end(inst->getParent()); I != E && succToSinkTo == 0; ++I)
            {
                // avoid sinking an instruction into its own block.  This can
                // happen with loops.
                if ((*I) == curBlk)
                    continue;
                // punt on it because of alias concern
                if ((*I)->getUniquePredecessor() != curBlk)
                    continue;
                // Don't move instruction across a loop.
                Loop* succLoop = LI->getLoopFor((*I));
                Loop* currLoop = LI->getLoopFor(curBlk);
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
        else if (usesInBlk.size() == 1)
        {
            Instruction* use = *(usesInBlk.begin());
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

    bool CodeSinking::LocalSink(BasicBlock* blk)
    {
        bool madeChange = false;
        for (BasicBlock::iterator I = blk->begin(), E = blk->end(); I != E; ++I)
        {
            Instruction* use = &(*I);
            for (unsigned i = 0; i < use->getNumOperands(); ++i)
            {
                Instruction* def = dyn_cast<Instruction>(use->getOperand(i));
                if (def && def->getParent() == blk && localInstSet.count(def))
                {
                    // "use" can be a phi-node for a single-block loop,
                    // which is not really a local-code-motion
                    if (def->getNextNode() != use && !isa<PHINode>(use))
                    {
                        if (!def->getMetadata("implicitGlobalID"))
                        {
                            def->moveBefore(use);
                            madeChange = true;
                        }
                    }
                    localInstSet.erase(def);
                }
            }
        }
        if (madeChange) {
            ProcessDbgValueInst(*blk);
        }
        return madeChange;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool CodeSinking::checkCongruent(std::vector<InstPair> &instMap, const InstPair& values, InstVec& leaves, unsigned depth)
    {
        Instruction* src0 = values.first;
        Instruction* src1 = values.second;

        if (depth > 32 ||
            src0->getOpcode() != src1->getOpcode() ||
            src0->getNumOperands() != src1->getNumOperands() ||
            src0->getType() != src1->getType() ||
            isa<PHINode>(src0) ||
            isa<CmpInst>(src0) ||
            !(src0->getNumOperands() == 1 ||
                src0->getNumOperands() == 2) ||
                (isa<AllocaInst>(src0) && src0 != src1))
            return false;

        if (CallInst * call0 = dyn_cast<CallInst>(src0))
        {
            CallInst* call1 = dyn_cast<CallInst>(src1);
            IGC_ASSERT(call1 != nullptr);

            if (!call0->getCalledFunction() ||
                call0->getCalledFunction() != call1->getCalledFunction() ||
                !call0->getCalledFunction()->doesNotReadMemory() ||
                call0->isConvergent())
            {
                return false;
            }
        }
        else
            if (LoadInst * ld0 = dyn_cast<LoadInst>(src0))
            {
                LoadInst* ld1 = dyn_cast<LoadInst>(src1);
                IGC_ASSERT(ld1 != nullptr);
                if (ld0->getPointerAddressSpace() != ld1->getPointerAddressSpace())
                {
                    return false;
                }
                unsigned as = ld0->getPointerAddressSpace();
                unsigned bufId = 0;
                bool directBuf = false;
                BufferType bufType = IGC::DecodeAS4GFXResource(as, directBuf, bufId);
                if (bufType != CONSTANT_BUFFER)
                {
                    return false;
                }
            }

        bool equals = true;
        InstVec tmpVec;

        unsigned nopnds = src0->getNumOperands();

        if (nopnds == 2 && src0->getOperand(0) == src0->getOperand(1))
        {
            if (src1->getOperand(0) == src1->getOperand(1))
            {
                nopnds = 1;
            }
            else
            {
                return false;
            }
        }

        for (unsigned i = 0; i < nopnds; i++)
        {
            Value* v0, * v1;
            Instruction* iv0, * iv1;
            v0 = src0->getOperand(i);
            v1 = src1->getOperand(i);
            iv0 = dyn_cast<Instruction>(v0);
            iv1 = dyn_cast<Instruction>(v1);

            if (v0 == v1)
            {
                if (iv0)
                {
                    if (DT->dominates(iv0->getParent(), src0->getParent()) &&
                        DT->dominates(iv0->getParent(), src1->getParent()))
                    {
                        appendIfNotExist(tmpVec, iv0);
                        continue;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                    if (!(isa<Argument>(v0) || isa<Constant>(v0) || isa<GlobalValue>(v0)))
                    {
                        return false;
                    }
                continue;
            }
            if (iv0 && iv0->getParent() == src0->getParent() &&
                iv1 && iv1->getParent() == src1->getParent())
            {
                if (!checkCongruent(instMap, std::make_pair(iv0, iv1), tmpVec, depth + 1))
                {
                    equals = false;
                    break;
                }
            }
            else
            {
                equals = false;
                break;
            }
        }
        if (equals)
        {
            appendIfNotExist(std::make_pair(src0, src1), instMap);
            appendIfNotExist(leaves, tmpVec);
            return equals;
        }

        if (!src0->isCommutative() ||
            (src0->isCommutative() && src0->getOperand(0) == src0->getOperand(1)))
            return equals;

        equals = true;
        tmpVec.clear();
        for (unsigned i = 0; i < src0->getNumOperands(); i++)
        {
            Value* v0, * v1;
            Instruction* iv0, * iv1;
            v0 = src0->getOperand(i);
            v1 = src1->getOperand(1 - i);
            iv0 = dyn_cast<Instruction>(v0);
            iv1 = dyn_cast<Instruction>(v1);

            if (v0 == v1)
            {
                if (iv0)
                {
                    if (DT->dominates(iv0->getParent(), src0->getParent()) &&
                        DT->dominates(iv0->getParent(), src1->getParent()))
                    {
                        appendIfNotExist(tmpVec, iv0);
                        continue;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                    if (!(isa<Argument>(v0) || isa<Constant>(v0) || isa<GlobalValue>(v0)))
                    {
                        return false;
                    }
                continue;
            }

            if (iv0 && iv0->getParent() == src0->getParent() &&
                iv1 && iv1->getParent() == src1->getParent())
            {
                if (!checkCongruent(instMap, std::make_pair(iv0, iv1), leaves, depth + 1))
                {
                    equals = false;
                    break;
                }
            }
            else
            {
                equals = false;
                break;
            }
        }
        if (equals)
        {
            appendIfNotExist(std::make_pair(src0, src1), instMap);
            appendIfNotExist(leaves, tmpVec);
        }
        return equals;
    }

    bool CodeSinking::hoistCongruentPhi(PHINode* phi)
    {
        if (phi->getNumIncomingValues() != 2)
            return false;

        bool changed = false;
        InstVec leaves;

        Instruction* src0, * src1;
        src0 = dyn_cast<Instruction>(phi->getIncomingValue(0));
        src1 = dyn_cast<Instruction>(phi->getIncomingValue(1));
        if (src0 && src1 && src0 != src1)
        {
            // this vector maps all instructions leading to source0 of phi instruction to
            // the corresponding instructions of source1
            std::vector<InstPair> instMap;

            if (checkCongruent(instMap, std::make_pair(src0, src1), leaves, 0))
            {
                BasicBlock* predBB = nullptr;
                Instruction* insertPos = nullptr;
                bool apply = true;

                if (leaves.size() == 0)
                {
                    if (DT->dominates(src0, phi->getParent()))
                    {
                        phi->replaceAllUsesWith(src0);
                        return true;
                    }
                    else
                        if (DT->dominates(src1, phi->getParent()))
                        {
                            phi->replaceAllUsesWith(src1);
                            return true;
                        }
                        else
                        {
                            predBB = DT->findNearestCommonDominator(
                                src0->getParent(), src1->getParent());
                            insertPos = predBB->getTerminator();
                        }
                }
                else
                {
                    Instruction* last = nullptr;
                    for (auto* I : leaves)
                    {
                        if (!predBB)
                        {
                            predBB = I->getParent();
                            last = I;
                        }
                        else
                            if (predBB != I->getParent() ||
                                !DT->dominates(predBB, src0->getParent()) ||
                                !DT->dominates(predBB, src1->getParent()))
                            {
                                apply = false;
                                break;
                            }
                            else
                                if (!isInstPrecede(I, last))
                                {
                                    last = I;
                                }
                    }
                    if (isa<PHINode>(last))
                    {
                        insertPos = predBB->getFirstNonPHI();
                    }
                    else
                    {
                        insertPos = last->getNextNode();
                    }
                }

                if (apply)
                {
                    auto compareFunc = [](const InstPair& a, const InstPair& b) {
                        return (a.first == b.first) ? false : isInstPrecede(a.first, b.first);
                    };
                    std::sort(instMap.begin(), instMap.end(), compareFunc);

                    for (auto& insts : instMap)
                    {
                        Instruction* I = insts.first;
                        Instruction* ni = I->clone();
                        ni->insertBefore(insertPos);
                        ni->setName(ni->getName() + ".hoist");

                        if (phi->getIncomingValue(0) == I)
                        {
                            // replace phi also
                            phi->replaceAllUsesWith(ni);
                        }
                        I->replaceAllUsesWith(ni);
                        insts.second->replaceAllUsesWith(ni);
                    }
                    changed = true;
                }
            }
        }
        return changed;
    }

    bool CodeSinking::hoistCongruentPhi(Function& F)
    {
        bool changed = false;
        for (auto& BB : F)
        {
            for (auto II = BB.begin(), IE = BB.end(); II != IE; )
            {
                PHINode* phi = dyn_cast<PHINode>(II);

                if (!phi)
                    break;

                if (hoistCongruentPhi(phi))
                {
                    changed = true;
                    II = phi->eraseFromParent();
                }
                else
                {
                    ++II;
                }
            }
        }
        return changed;
    }

    bool CodeSinking::loopSink(Loop* LoopWithPressure, bool SinkMultipleLevel)
    {
        // Sink loop invariants back into the loop body if register
        // pressure can be reduced.

        // L0 is inner loop
        Loop* const L0 = LoopWithPressure;
        IGC_ASSERT(L0);

        // L1 is parent loop
        Loop* L1 = nullptr;
        if (SinkMultipleLevel) {
            L1 = L0->getParentLoop();
        }

        // At most, do two-level loop sink
        //     x = ...
        //   ParentLoop
        //     y = ...
        //     Loop:
        //          = x
        //          = y
        // Normally, only y can be sinked. When multiLevel is true,
        // x can be sinked into Loop (inner) as well.
        bool changed = false;
        for (int i = 0; i < 2; ++i)
        {
            Loop* L = (i == 0) ? L0 : L1;
            if (!L) {
                break;
            }
            // No Preheader, stop!
            BasicBlock* Preheader = L->getLoopPreheader();
            if (!Preheader)
                break;

            // Find LIs in preheader that would definitely reduce
            // register pressure after moving those LIs inside the loop
            SmallPtrSet<Instruction*, 16> stores;
            SmallVector<Instruction*, 64> sinkCandidates;

            // Moving LI back to the loop. Here we only consider to move LIs into
            // the single BB (BBWithPressure).
            //
            // Go over instructions in reverse order and sink the noOp instructions
            // on-the-fly first, so that their dependent instructions can be added
            // into candidate lists for further sinking.
            for (auto II = Preheader->rbegin(), IE = Preheader->rend(); II != IE;)
            {
                Instruction* I = &*II++;

                if (I->mayWriteToMemory()) {
                    stores.insert(I);
                }
                if (!canLoopSink(I, L))
                    continue;

                // Sink noOp instruction.
                if (isNoOpInst(I, CTX) || reduceRP(I)) {
                    if (SinkInstruction(I, stores, true)) {
                        changed = true;
                    }
                    continue;
                }

                sinkCandidates.push_back(I);
            }

            bool t = LoopSinkInstructions(sinkCandidates, L);
            changed |= t;

            if (changed) {
                ProcessDbgValueInst(*Preheader);
            }
        }

        // Invoke LocalSink() to move def to its first use
        // (Currently, it should be no opt as LoopSink only
        // sinks singleUse instructions, which should be done
        // completely within sinkInstruction.
        if (localBlkSet.size() > 0)
        {
            for (auto BI = localBlkSet.begin(), BE = localBlkSet.end(); BI != BE; BI++)
            {
                BasicBlock* BB = *BI;
                bool t = LocalSink(BB);
                changed |= t;
            }
            localBlkSet.clear();
            localInstSet.clear();
        }

        return changed;
    }

    bool CodeSinking::canLoopSink(Instruction* I, Loop* L)
    {
        // Limit sinking for the following case for now.
        for (const User* UserInst : I->users())
        {
            if (!isa<Instruction>(UserInst))
                return false;
            if (!L->contains(cast<Instruction>(UserInst)))
                return false;
        }
        return (isNoOpInst(I, CTX) || reduceRP(I) ||
            isa<BinaryOperator>(I) /*|| isa<GetElementPtrInst>(I)*/);
    }

    bool CodeSinking::LoopSinkInstructions(
        SmallVector<Instruction*, 64> sinkCandidates,
        Loop* L)
    {
        auto IsUsedInLoop = [](Value* V, Loop* L) -> bool {
            if (isa<Constant>(V)) {
                // Ignore constant
                return false;
            }
            for (auto UI : V->users()) {
                if (Instruction * User = dyn_cast<Instruction>(UI))
                {
                    if (L->contains(User))
                        return true;
                }
            }
            return false;
        };

        auto IsSameSet = [](SmallPtrSet <Value*, 4> & S0, SmallPtrSet <Value*, 4> & S1)-> bool {
            if (S0.size() == S1.size()) {
                for (auto I : S1) {
                    Value* V = I;
                    if (!S0.count(V)) {
                        return false;
                    }
                }
                return true;
            }
            return false;
        };

        // For each candidate like the following:
        //   preheader:
        //            x = add y, z
        //   loop:
        //         ...
        //      BB:
        //           = x
        //
        // Afer sinking, x changes from global to local, and thus reduce pressure.
        // But y and z could change to global to local (if y and z are local).
        // Thus, we reduce pressure by 1 (x), but increase by the number of its
        // operands (y and z). If there are more candidates share the same operands,
        // we will reduce the pressure.  For example:
        //   preheader:
        //        x0 = add y, 10
        //        x1 = add y, 20
        //        x2 = add y, 100
        //        x3 = add y, 150
        //   loop:
        //         = x0
        //         = x1
        //         = x2
        //         = x3
        //
        // After sinking x0-x3 into loop, we make x0-x3 be local and make y be global,
        // which results in 3 (4 - 1) pressure reduction.
        //
        // Here we group all candidates based on its operands and select ones that definitely
        // reduce the pressure.
        //
        struct OperandUseGroup {
            SmallPtrSet <Value*, 4> Operands;
            SmallVector<Instruction*, 16> Users;
        };
        OperandUseGroup* allGroups = new OperandUseGroup[sinkCandidates.size()];
        SmallVector<OperandUseGroup*, 16> InstUseInfo;
        for (uint32_t i = 0, e = (uint32_t)sinkCandidates.size(); i < e; ++i)
        {
            Instruction* I = sinkCandidates[i];
            SmallPtrSet<Value*, 4> theUses;
            for (Use& U : I->operands())
            {
                Value* V = U;
                if (isa<Constant>(V) || IsUsedInLoop(V, L))
                    continue;

                theUses.insert(V);
            }
            // If this set of uses have been referenced by other instructions,
            // put this inst in the same group. Note that we don't union sets
            // that intersect each other.
            uint32_t j, je = (uint32_t)InstUseInfo.size();
            for (j = 0; j < je; ++j)
            {
                OperandUseGroup* OUG = InstUseInfo[j];
                if (IsSameSet(OUG->Operands, theUses)) {
                    OUG->Users.push_back(I);
                    break;
                }
            }


            if (j == je) {
                // No match found, create the new one.
                OperandUseGroup& OUG = allGroups[i];
                OUG.Operands = theUses;
                OUG.Users.push_back(I);
                InstUseInfo.push_back(&OUG);
            }
        }

        bool changed = false;
        // Just a placeholder, all LIs considered here are ALUs.
        SmallPtrSet<Instruction*, 16> stores;
        const int SaveThreshold = IGC_GET_FLAG_VALUE(LoopSinkMinSave);
        bool keepLooping;
        uint32_t N = (uint32_t)InstUseInfo.size();
        do {
            keepLooping = false;
            for (uint32_t i = 0; i < N; ++i)
            {
                OperandUseGroup* OUG = InstUseInfo[i];
                if (!OUG)
                    continue;

                int sz1 = (int)OUG->Users.size();
                int save = sz1 - (int)(OUG->Operands.size());
                if (save >= SaveThreshold)
                {
                    // Sink
                    bool t = false;
                    for (int j = 0; j < sz1; ++j)
                    {
                        Instruction* I = OUG->Users[j];
                        bool t1 = SinkInstruction(I, stores, true);
                        t |= t1;
                    }
                    if (t) {
                        changed = true;
                        keepLooping = true;

                        // Since those operands become global already, remove
                        // them from the sets in the vector.
                        for (uint32_t k = 0; k < N; ++k)
                        {
                            OperandUseGroup* OUG1 = InstUseInfo[k];
                            if (k == i || !OUG1)
                                continue;

                            for (auto I : OUG->Operands) {
                                Value* V = I;
                                OUG1->Operands.erase(V);
                            }
                        }
                    }

                    // Just set it to nullptr (erasing it would be more expensive).
                    InstUseInfo[i] = nullptr;
                }
            }
        } while (keepLooping);

        delete[] allGroups;

        return changed;
    }

    // Move referenced DbgValueInst intrinsics calls after defining instructions
    // it is requared for correct work of LiveVariables analysis and other
    void CodeSinking::ProcessDbgValueInst(BasicBlock& blk)
    {
        if (!CTX->m_instrTypes.hasDebugInfo)
        {
            return;
        }

        BasicBlock::iterator I = blk.end();
        --I;
        bool processedBegin = false;
        do {
            Instruction* inst = cast<Instruction>(I);
            processedBegin = (I == blk.begin());
            if (!processedBegin)
                --I;

            if (auto* DVI = dyn_cast<DbgValueInst>(inst))
            {
                // As debug intrinsics are not specified as users of an llvm instructions,
                // it may happen during transformation/optimization the first argument is
                // malformed (actually is dead). Not to chase each possible optimzation
                // let's do a general check here.
                if (DVI->getValue() != nullptr) {
                    if (auto* def = dyn_cast<Instruction>(DVI->getValue()))
                    {
                        if (!DT->dominates(def, inst))
                        {
                            auto* instClone = inst->clone();
                            instClone->insertAfter(def);
                            Value* undef = UndefValue::get(def->getType());
                            MetadataAsValue* MAV = MetadataAsValue::get(inst->getContext(), ValueAsMetadata::get(undef));
                            cast<CallInst>(inst)->setArgOperand(0, MAV);
                        }
                    }
                }
                else {
                    // The intrinsic is actually unneeded and will be removed later. Thus the type of the
                    // first argument is not important now.
                    Value* undef = UndefValue::get(llvm::Type::getInt32Ty(inst->getContext()));
                    MetadataAsValue* MAV = MetadataAsValue::get(inst->getContext(), ValueAsMetadata::get(undef));
                    cast<CallInst>(inst)->setArgOperand(0, MAV);
                }
            }
        } while (!processedBegin);
    }

    char CodeSinking::ID = 0;
}
