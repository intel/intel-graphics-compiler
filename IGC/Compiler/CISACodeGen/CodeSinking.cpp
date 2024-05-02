/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#include <fstream>
#include "common/debug/Debug.hpp"
#include "common/debug/Dump.hpp"
#include "common/Stats.hpp"
#include "common/LLVMUtils.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Verifier.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvmWrapper/IR/Function.h"
#include "llvmWrapper/IR/Value.h"
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

    /// ================================= ///
    /// Common functions for code sinking ///
    /// ================================= ///

    // Move referenced DbgValueInst intrinsics calls after defining instructions
    // it is required for correct work of LiveVariables analysis and other
    static void ProcessDbgValueInst(BasicBlock& blk, DominatorTree *DT)
    {
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

    // Sink to the use within basic block
    static bool LocalSink(
        BasicBlock *BB,
        SmallPtrSet<Instruction*, 8> &LocalInstSet,
        DominatorTree *DT,
        CodeGenContext *CTX
        )
    {
        auto getInsertPointBeforeUse = [&](Instruction *InstToMove, Instruction *StartInsertPoint)
        {
            // Try scheduling the instruction earlier than the use.
            // Useful for loads to cover some latency.

            // Restrict to OCL shaders for now
            if (CTX->type != ShaderType::OPENCL_SHADER)
                return StartInsertPoint;

            int Cnt = IGC_GET_FLAG_VALUE(CodeSinkingLoadSchedulingInstr);
            Instruction *InsertPoint = StartInsertPoint;
            Instruction *I = StartInsertPoint->getPrevNode();
            for (;;) {
                if (I == nullptr)
                    break;
                if (isa<PHINode>(I))
                    break;
                if (std::any_of(I->use_begin(), I->use_end(),
                        [InstToMove](auto &U) {return llvm::cast<Instruction>(&U) == InstToMove;}))
                    break;
                if (I->mayWriteToMemory())
                {
                    // At this point of the program we might have lost some information
                    // About aliasing so don't schedule anything before possible stores
                    // But it's OK to alias with prefetch
                    GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I);
                    if (!(Intr && Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSCPrefetch))
                    {
                        break;
                    }
                }
                if (--Cnt <= 0)
                    break;

                InsertPoint = I;
                I = I->getPrevNode();
            }
            return InsertPoint;
        };

        bool Changed = false;
        for (auto &I : *BB)
        {
            Instruction *Use = &I;
            for (unsigned i = 0; i < Use->getNumOperands(); ++i)
            {
                Instruction *Def = dyn_cast<Instruction>(Use->getOperand(i));
                if (Def && Def->getParent() == BB && LocalInstSet.count(Def))
                {
                    // "Use" can be a phi-node for a single-block loop,
                    // which is not really a local-code-motion
                    if (Def->getNextNode() != Use && !isa<PHINode>(Use))
                    {
                        // If it's a load we'll try scheduling earlier than the use
                        // to cover latency
                        Instruction *InsertPoint =
                            isa<LoadInst>(Def) ? getInsertPointBeforeUse(Def, Use) : Use;
                        Def->moveBefore(InsertPoint);
                        Changed = true;
                    }
                    LocalInstSet.erase(Def);
                }
            }
        }
        if (Changed && CTX->m_instrTypes.hasDebugInfo) {
            ProcessDbgValueInst(*BB, DT);
        }
        return Changed;
    }

    static void rollbackSinking(
        bool ReverseOrder,
        BasicBlock* BB,
        std::vector<llvm::Instruction*> &UndoLocas,
        std::vector<llvm::Instruction*> &MovedInsts)
    {
        // undo code motion
        int NumChanges = MovedInsts.size();
        for (int i = 0; i < NumChanges; ++i)
        {
            int Index = ReverseOrder ? NumChanges - i - 1 : i;
            Instruction* UndoLoca = UndoLocas[Index];
            if (BB)
                IGC_ASSERT(UndoLoca->getParent() == BB);
            MovedInsts[Index]->moveBefore(UndoLoca);
        }
    }

    // Find the BasicBlock to sink
    // return nullptr if instruction cannot be moved to another block
    static BasicBlock* findLowestSinkTarget(Instruction* inst,
        SmallPtrSetImpl<Instruction*>& usesInBlk,
        bool& outerLoop,
        bool doLoopSink,
        llvm::DominatorTree* DT,
        llvm::LoopInfo* LI
        )
    {
        usesInBlk.clear();
        BasicBlock *tgtBlk = nullptr;
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
                    return nullptr;
                }
            }
            if (tgtBlk == nullptr)
            {
                tgtBlk = useBlock;
            }
            else
            {
                tgtBlk = DT->findNearestCommonDominator(tgtBlk, useBlock);
                if (tgtBlk == nullptr)
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
                return tgtBlk;
            }
            else
            {
                tgtBlk = DT->getNode(tgtBlk)->getIDom()->getBlock();
            }
        }

        return nullptr;
    }

    static bool isCastInstrReducingPressure(Instruction* Inst, bool FlagPressureAware)
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
            if (FlagPressureAware)
            {
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
                if (SrcSize < DstSize)
                {
                    // sext i32 to i64.
                    return true;
                }
            }
            else
            {
                return SrcSize < DstSize;
            }
        }

        return false;
    }

    // Number of instructions in the function
    static unsigned numInsts(const Function& F)
    {
        return std::count_if(llvm::inst_begin(F), llvm::inst_end(F), [](const auto& I){ return !isDbgIntrinsic(&I); });
    }

    /// ===================== ///
    /// Non-loop code sinking ///
    /// ===================== ///

    // Register pass to igc-opt
#define PASS_FLAG "igc-code-sinking"
#define PASS_DESCRIPTION "code sinking"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_END(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        char CodeSinking::ID = 0;
        CodeSinking::CodeSinking() : FunctionPass(ID) {
            initializeCodeSinkingPass(*PassRegistry::getPassRegistry());
    }


    // Sink the code down the tree, but not in the loop
    bool CodeSinking::treeSink(Function &F)
    {
        bool IterChanged, EverChanged = false;
        totalGradientMoved = 0;
        // even if we limit code-sinking to ps-input instructions, we still need to iterate through
        // all the blocks because llvm-InstCombine may have sinked some ps-input instructions out of entry-block
        do
        {
            IterChanged = false;
            // Process all basic blocks in dominator-tree post-order
            for (po_iterator<DomTreeNode*> domIter = po_begin(DT->getRootNode()),
                domEnd = po_end(DT->getRootNode()); domIter != domEnd; ++domIter)
            {
                IterChanged |= processBlock(*(domIter->getBlock()));
            }
        } while (IterChanged);

        EverChanged = IterChanged;
        for (auto BI = LocalBlkSet.begin(), BE = LocalBlkSet.end(); BI != BE; BI++)
        {
            IterChanged = LocalSink(*BI, LocalInstSet, DT, CTX);
            EverChanged |= IterChanged;
        }
        LocalBlkSet.clear();
        LocalInstSet.clear();
        CTX->m_numGradientSinked = totalGradientMoved;

        return EverChanged;
    }

    bool CodeSinking::runOnFunction(Function& F)
    {
        if (skipFunction(F))
            return false;

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
            numInsts(F) < IGC_GET_FLAG_VALUE(CodeSinkingMinSize))
        {
            return false;
        }

        DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
        PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
        LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
        DL = &F.getParent()->getDataLayout();

        bool Changed = treeSink(F);

        if (Changed)
        {
            // the verifier currently rejects allocas with non-default
            // address space (which is legal). Raytracing does this, so we skip
            // verification here.
            if (CTX->type != ShaderType::RAYTRACING_SHADER)
            {
                IGC_ASSERT(false == verifyFunction(F, &dbgs()));
            }
        }

        return Changed;
    }

    bool CodeSinking::processBlock(BasicBlock& blk)
    {
        if (blk.empty())
            return false;

        uint32_t registerPressureThreshold = CTX->getNumGRFPerThread();

        uint pressure0 = 0;
        if (registerPressureThreshold)
        {
            // estimate live-out register pressure for this blk
            pressure0 = estimateLiveOutPressure(&blk, DL);
        }

        bool madeChange = false;
        numGradientMovedOutBB = 0;

        // Walk the basic block bottom-up.  Remember if we saw a store.
        BasicBlock::iterator I = blk.end();
        --I;
        bool processedBegin = false;
        bool metDbgValueIntrinsic = false;
        SmallPtrSet<Instruction*, 16> stores;
        UndoLocas.clear();
        MovedInsts.clear();
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

                if (sinkInstruction(inst, stores))
                {
                    if (ComputesGradient(inst))
                        numGradientMovedOutBB++;
                    madeChange = true;
                    MovedInsts.push_back(inst);
                    UndoLocas.push_back(undoLoca);
                }
            }
            // If we just processed the first instruction in the block, we're done.
        } while (!processedBegin);

        if (registerPressureThreshold)
        {
            if (madeChange)
            {
                // measure the live-out register pressure again
                uint pressure1 = estimateLiveOutPressure(&blk, DL);
                if (pressure1 > pressure0 + registerPressureThreshold)
                {
                    rollbackSinking(false, &blk, UndoLocas, MovedInsts);
                    madeChange = false;
                }
                else
                {
                    totalGradientMoved += numGradientMovedOutBB;
                }
            }
        }
        if ((madeChange || metDbgValueIntrinsic) && CTX->m_instrTypes.hasDebugInfo) {
            ProcessDbgValueInst(blk, DT);
        }

        return madeChange;
    }

    bool CodeSinking::sinkInstruction(
        Instruction *InstToSink,
        SmallPtrSetImpl<Instruction *> &Stores
        )
    {
        // Check if it's safe to move the instruction.
        bool HasAliasConcern = false;
        bool ReducePressure = false;

        if (!isSafeToMove(InstToSink, ReducePressure, HasAliasConcern, Stores))
            return false;

        // SuccToSinkTo - This is the successor to sink this instruction to, once we
        // decide.
        BasicBlock *SuccToSinkTo = nullptr;
        SmallPtrSet<Instruction *, 16> UsesInBB;

        if (!HasAliasConcern)
        {
            // find the lowest common dominator of all uses
            bool IsOuterLoop = false;
            if (BasicBlock *TgtBB = findLowestSinkTarget(InstToSink, UsesInBB, IsOuterLoop, false, DT, LI))
            {
                // heuristic, avoid code-motion that does not reduce execution frequency
                // but may increase register usage
                if (ReducePressure ||
                    (TgtBB && (IsOuterLoop || !PDT->dominates(TgtBB, InstToSink->getParent()))))
                {
                    SuccToSinkTo = TgtBB;
                }
            }
            else
            {
                // local code motion for cases like cmp and pln
                if (ReducePressure)
                {
                    LocalBlkSet.insert(InstToSink->getParent());
                    LocalInstSet.insert(InstToSink);
                }
                return false;
            }
        }
        else
        {
            // when aliasing is a concern, only look at all the immed successors and
            // decide which one we should sink to, if any.
            BasicBlock *CurBB = InstToSink->getParent();
            for (succ_iterator I = succ_begin(InstToSink->getParent()),
                E = succ_end(InstToSink->getParent()); I != E && SuccToSinkTo == 0; ++I)
            {
                // avoid sinking an instruction into its own block.  This can
                // happen with loops.
                if ((*I) == CurBB)
                    continue;
                // punt on it because of alias concern
                if ((*I)->getUniquePredecessor() != CurBB)
                    continue;
                // Don't move instruction across a loop.
                Loop *succLoop = LI->getLoopFor((*I));
                Loop *currLoop = LI->getLoopFor(CurBB);
                if (succLoop != currLoop)
                    continue;
                if (allUsesDominatedByBlock(InstToSink, (*I), UsesInBB))
                    SuccToSinkTo = *I;
            }
        }

        // If we couldn't find a block to sink to, ignore this instruction.
        if (!SuccToSinkTo)
        {
            return false;
        }

        if (!ReducePressure || HasAliasConcern)
        {
            InstToSink->moveBefore(&(*SuccToSinkTo->getFirstInsertionPt()));
        }
        // when alasing is not an issue and reg-pressure is not an issue
        // move it as close to the uses as possible
        else if (UsesInBB.empty())
        {
            InstToSink->moveBefore(SuccToSinkTo->getTerminator());
        }
        else if (UsesInBB.size() == 1)
        {
            InstToSink->moveBefore(*(UsesInBB.begin()));
        }
        else
        {
            // first move to the beginning of the target block
            InstToSink->moveBefore(&(*SuccToSinkTo->getFirstInsertionPt()));
            // later on, move it close to the use
            LocalBlkSet.insert(SuccToSinkTo);
            LocalInstSet.insert(InstToSink);
        }
        return true;
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
        if (isa<GetElementPtrInst>(inst) ||
            isa<ExtractElementInst>(inst) ||
            isa<InsertElementInst>(inst) ||
            isa<InsertValueInst>(inst) ||
            (isa<UnaryInstruction>(inst) && !isa<LoadInst>(inst)) ||
            isa<BinaryOperator>(inst))
        {
            hasAliasConcern = false;
            // sink CmpInst to make the flag-register lifetime short
            reducePressure = (isCastInstrReducingPressure(inst, true) || isa<CmpInst>(inst));
            return true;
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

    /// AllUsesDominatedByBlock - Return true if all uses of the specified value
    /// occur in blocks dominated by the specified block.
    bool CodeSinking::allUsesDominatedByBlock(Instruction* inst,
        BasicBlock* blk,
        SmallPtrSetImpl<Instruction*>& usesInBlk
        ) const
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

    uint CodeSinking::estimateLiveOutPressure(BasicBlock* blk, const DataLayout* DL)
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

    /// ==================///
    /// Loop code sinking ///
    /// ==================///

    // Sink in the loop if loop preheader's potential to sink covers at least 20% of registers delta
// between grf number and max estimated pressure in the loop
#define LOOPSINK_PREHEADER_IMPACT_THRESHOLD 0.2

    // Helper functions for loop sink debug dumps
#define PrintDump(Contents) if (IGC_IS_FLAG_ENABLED(DumpLoopSink)) {LogStream << Contents;}
#define PrintInstructionDump(Inst) if (IGC_IS_FLAG_ENABLED(DumpLoopSink)) {Inst->print(LogStream, false); LogStream << "\n";}
#define PrintOUGDump(OUG) if (IGC_IS_FLAG_ENABLED(DumpLoopSink)) {OUG.print(LogStream); LogStream << "\n";}


    // Register pass to igc-opt
#define PASS_FLAG1 "igc-code-loop-sinking"
#define PASS_DESCRIPTION1 "code loop sinking"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false
    IGC_INITIALIZE_PASS_BEGIN(CodeLoopSinking, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(TranslationTable)
        IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
        IGC_INITIALIZE_PASS_DEPENDENCY(IGCFunctionExternalRegPressureAnalysis)
        IGC_INITIALIZE_PASS_END(CodeLoopSinking, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)

        char CodeLoopSinking::ID = 0;
        CodeLoopSinking::CodeLoopSinking() : FunctionPass(ID), LogStream(Log) {
            initializeCodeLoopSinkingPass(*PassRegistry::getPassRegistry());
    }


    bool CodeLoopSinking::runOnFunction(Function& F)
    {
        if (skipFunction(F))
            return false;

        CTX = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
        if (CTX->type != ShaderType::OPENCL_SHADER)
            return false;

        if (IGC_IS_FLAG_ENABLED(DisableCodeSinking) ||
            numInsts(F) < IGC_GET_FLAG_VALUE(CodeLoopSinkingMinSize))
        {
            return false;
        }

        if (IGC_IS_FLAG_ENABLED(DumpLoopSink))
        {
            Log.clear();
            PrintDump("=====================================\n");
            PrintDump("Function " << F.getName() << "\n");
        }

        DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
        PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
        LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
        AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
        MDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
        ModMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();

        TranslationTable TT;
        TT.run(F);

        WI = WIAnalysisRunner(&F, LI, DT, PDT, MDUtils, CTX, ModMD, &TT);
        WI.run();

        // Note: FRPE is a Module analysis and currently it runs only once.
        // If function A calls function B then
        // it's possible that transformation of function A reduces the regpressure good enough
        // and we could not apply sinking in function B, but we don't recompute FPRE
        // to save compile time, so in this case LoopSinking might apply for loops in function B

        RPE = &getAnalysis<IGCLivenessAnalysis>();
        FRPE = &getAnalysis<IGCFunctionExternalRegPressureAnalysis>();

        // clear caching structures before handling the new function
        MemoizedStoresInLoops.clear();
        BlacklistedLoops.clear();
        BBPressures.clear();

        bool Changed = loopSink(F);

        if (Changed)
        {
            IGC_ASSERT(false == verifyFunction(F, &dbgs()));
        }

        if (IGC_IS_FLAG_ENABLED(DumpLoopSink))
        {
            if (IGC_IS_FLAG_ENABLED(PrintToConsole))
                IGC::Debug::ods() << Log;
            else
                dumpToFile(Log);
        }

        return Changed;
    }

    void CodeLoopSinking::dumpToFile(const std::string& Log)
    {
        auto Name = Debug::DumpName(IGC::Debug::GetShaderOutputName())
                                    .Hash(CTX->hash)
                                    .Type(CTX->type)
                                    .Retry(CTX->m_retryManager.GetRetryId())
                                    .Pass("loopsink")
                                    .Extension("txt");
        IGC::Debug::DumpLock();
        std::ofstream OutputFile(Name.str(), std::ios_base::app);
        if (OutputFile.is_open())
            OutputFile << Log;
        OutputFile.close();
        IGC::Debug::DumpUnlock();
    }

    // Implementation of RPE->getMaxRegCountForLoop(*L, SIMD);
    // with per-BB pressure caching to improve compile-time
    uint CodeLoopSinking::getMaxRegCountForLoop(Loop *L)
    {
        IGC_ASSERT(RPE);
        Function *F = L->getLoopPreheader()->getParent();
        uint SIMD = numLanes(RPE->bestGuessSIMDSize(F));
        unsigned int Max = 0;
        for (BasicBlock *BB : L->getBlocks())
        {
            auto BBPressureEntry = BBPressures.try_emplace(BB);
            unsigned int &BBPressure = BBPressureEntry.first->second;
            if (BBPressureEntry.second)  // BB was not in the set, need to recompute
            {
                BBPressure = RPE->getMaxRegCountForBB(*BB, SIMD, &WI);
            }
            Max = std::max(BBPressure, Max);
        }
        return Max;
    }

    // Find the loops with too high regpressure and sink the instructions from
    // preheaders into them
    bool CodeLoopSinking::loopSink(Function &F)
    {
        bool Changed = false;
        for (auto &L : LI->getLoopsInPreorder())
        {
            LoopSinkMode SinkMode = IGC_IS_FLAG_ENABLED(ForceLoopSink) ? LoopSinkMode::FullSink : LoopSinkMode::NoSink;
            if (SinkMode == LoopSinkMode::NoSink)
                SinkMode = needLoopSink(L);
            if (SinkMode != LoopSinkMode::NoSink)
                Changed |= loopSink(L, SinkMode);
        }
        return Changed;
    }

    LoopSinkMode CodeLoopSinking::needLoopSink(Loop *L)
    {
        BasicBlock *Preheader = L->getLoopPreheader();
        if (!Preheader)
            return LoopSinkMode::NoSink;
        if (!RPE)
            return LoopSinkMode::NoSink;

        Function *F = Preheader->getParent();
        uint GRFThresholdDelta = IGC_GET_FLAG_VALUE(LoopSinkThresholdDelta);
        uint NGRF = CTX->getNumGRFPerThread();
        uint SIMD = numLanes(RPE->bestGuessSIMDSize(F));

        PrintDump("\n");
        if (!Preheader->getName().empty())
        {
            PrintDump("Checking loop with preheader " << Preheader->getName() << ": \n");
        }
        else if (!Preheader->empty())
        {
            PrintDump("Checking loop with unnamed preheader. First preheader instruction:\n");
            Instruction* First = &Preheader->front();
            PrintInstructionDump(First);
        }
        else
        {
            PrintDump("Checking loop with unnamed empty preheader.");
        }

        // Estimate preheader's potential to sink
        ValueSet PreheaderDefs = RPE->getDefs(*Preheader);
        // Filter out preheader defined values that are used not in the loop or not supported
        ValueSet PreheaderDefsCandidates;
        for (Value *V : PreheaderDefs)
        {
            Instruction *I = dyn_cast<Instruction>(V);
            if (I && isLoopSinkCandidate(I, L,
                    (IGC_IS_FLAG_ENABLED(EnableLoadsLoopSink) || IGC_IS_FLAG_ENABLED(ForceLoadsLoopSink))))
            {
                PreheaderDefsCandidates.insert(V);
            }
        }

        if (PreheaderDefsCandidates.empty())
        {
            PrintDump(">> No sinking candidates in the preheader.\n");
            return LoopSinkMode::NoSink;
        }

        uint PreheaderDefsSizeInBytes = RPE->estimateSizeInBytes(PreheaderDefsCandidates, *F, SIMD, &WI);
        uint PreheaderDefsSizeInRegs = RPE->bytesToRegisters(PreheaderDefsSizeInBytes);

        // Estimate max pressure in the loop and the external pressure
        uint MaxLoopPressure = getMaxRegCountForLoop(L);
        uint FunctionExternalPressure = FRPE ? FRPE->getExternalPressureForFunction(F) : 0;

        auto isSinkCriteriaMet = [&](uint MaxLoopPressure)
        {
            // loop sinking is needed if the loop's pressure is higher than number of GRFs by threshold
            // and preheader's potential to reduce the delta is good enough
            return ((MaxLoopPressure > NGRF + GRFThresholdDelta) &&
                    (PreheaderDefsSizeInRegs > (MaxLoopPressure - NGRF) * LOOPSINK_PREHEADER_IMPACT_THRESHOLD));
        };

        PrintDump("Threshold to sink = " << NGRF + GRFThresholdDelta << "\n");
        PrintDump("MaxLoopPressure = " << MaxLoopPressure << "\n");
        PrintDump("MaxLoopPressure + FunctionExternalPressure = " << MaxLoopPressure + FunctionExternalPressure << "\n");

        // Sink if the regpressure in the loop is high enough (including function external regpressure)
        if (isSinkCriteriaMet(MaxLoopPressure + FunctionExternalPressure))
            return LoopSinkMode::SinkWhileRegpressureIsHigh;

        PrintDump(">> No sinking.\n");
        return LoopSinkMode::NoSink;
    }

    bool CodeLoopSinking::loopSink(Loop *L, LoopSinkMode Mode)
    {
        // Sink loop invariants back into the loop body if register
        // pressure can be reduced.

        IGC_ASSERT(L);

        // No Preheader, stop!
        BasicBlock *Preheader = L->getLoopPreheader();
        if (!Preheader)
            return false;

        PrintDump(">> Sinking in the loop with preheader " << Preheader->getName() << "\n");

        Function *F = Preheader->getParent();
        uint NGRF = CTX->getNumGRFPerThread();

        uint FunctionExternalPressure = FRPE ? FRPE->getExternalPressureForFunction(F) : 0;
        uint NeededRegpressure = NGRF - IGC_GET_FLAG_VALUE(LoopSinkRegpressureMargin);
        if ((NeededRegpressure >= FunctionExternalPressure) &&
                (Mode == LoopSinkMode::SinkWhileRegpressureIsHigh))
        {
            NeededRegpressure -= FunctionExternalPressure;
            PrintDump("Targeting new own regpressure in the loop = " << NeededRegpressure << "\n");
        }
        else
        {
            Mode = LoopSinkMode::FullSink;
            PrintDump("Doing full sink.\n");
        }

        // We can only affect Preheader and the loop.
        // Collect affected BBs to invalidate cached regpressure
        // and request recomputation of liveness analysis preserving not affected BBs
        BBSet AffectedBBs;
        AffectedBBs.insert(Preheader);
        for (BasicBlock *BB: L->blocks())
            AffectedBBs.insert(BB);

        auto rerunLiveness = [&]()
        {
            for (BasicBlock *BB: AffectedBBs)
                BBPressures.erase(BB);
            RPE->rerunLivenessAnalysis(*F, &AffectedBBs);
        };

        bool EverChanged = false;

        // Find LIs in preheader that would definitely reduce
        // register pressure after moving those LIs inside the loop
        SmallPtrSet<Instruction *, 16> Stores;
        SmallVector<Instruction *, 64> SinkCandidates;
        SmallPtrSet<Instruction *, 32> LoadChains;

        MovedInsts.clear();
        UndoLocas.clear();

        if (IGC_IS_FLAG_ENABLED(PrepopulateLoadChainLoopSink))
            prepopulateLoadChains(L, LoadChains);

        bool AllowLoadSinking = IGC_IS_FLAG_ENABLED(ForceLoadsLoopSink);
        bool AllowOnlySingleUseLoadChainSinking = false;
        bool IterChanged = false;

        uint InitialLoopPressure = getMaxRegCountForLoop(L);
        uint MaxLoopPressure = InitialLoopPressure;

        bool AchievedNeededRegpressure = false;

        do
        {
            Stores.clear();
            SinkCandidates.clear();

            // Moving LI back to the loop
            // If we sinked something we could allow sinking of the previous instructions as well
            // on the next iteration of do-loop
            //
            // For example, here we sink 2 EE first and need one more iteration to sink load:
            // preheader:
            //   %l = load <2 x double>
            //   extractelement 1, %l
            //   extractelement 2, %l
            // loop:
            //   ...
            IterChanged = false;

            PrintDump("Starting sinking iteration...\n");

            for (auto II = Preheader->rbegin(), IE = Preheader->rend(); II != IE;)
            {
                Instruction *I = &*II++;

                if (I->mayWriteToMemory())
                    Stores.insert(I);

                if (isLoopSinkCandidate(I, L, AllowLoadSinking))
                {
                    if (!AllowOnlySingleUseLoadChainSinking || isLoadChain(I, LoadChains, true))
                    {
                        SinkCandidates.push_back(I);
                    }
                }
            }

            IterChanged |= loopSinkInstructions(SinkCandidates, LoadChains, L);
            if (IterChanged)
            {
                EverChanged = true;

                // Invoke LocalSink() to move def to its first use
                if (LocalBlkSet.size() > 0)
                {
                    for (auto BI = LocalBlkSet.begin(), BE = LocalBlkSet.end(); BI != BE; BI++)
                    {
                        BasicBlock *BB = *BI;
                        LocalSink(BB, LocalInstSet, DT, CTX);
                    }
                    LocalBlkSet.clear();
                    LocalInstSet.clear();
                }

                rerunLiveness();
                MaxLoopPressure = getMaxRegCountForLoop(L);
                PrintDump("New max loop pressure = " << MaxLoopPressure << "\n");
                if ((MaxLoopPressure < NeededRegpressure)
                        && (Mode == LoopSinkMode::SinkWhileRegpressureIsHigh))
                {
                    AchievedNeededRegpressure = true;
                    if (IGC_IS_FLAG_ENABLED(EnableLoadChainLoopSink) && !LoadChains.empty())
                    {
                        PrintDump("Allowing only chain sinking...\n");
                        AllowOnlySingleUseLoadChainSinking = true;
                    }
                    else
                    {
                        PrintDump("Achieved needed regpressure, finished.\n");
                        break;
                    }
                }
            }
            else
            {
                if (!AllowLoadSinking && IGC_IS_FLAG_ENABLED(EnableLoadsLoopSink))
                {
                    PrintDump("Allowing loads...\n");
                    AllowLoadSinking = true;
                }
                else
                {
                    PrintDump("Nothing to sink, finished.\n");
                    break;
                }
            }
        } while (true);

        if (!EverChanged)
        {
            PrintDump("No changes were made in this loop.\n");
            return false;
        }

        PrintDump("New max loop pressure = " << MaxLoopPressure << "\n");

        bool NeedToRollback = false;

        // We always estimate if the sinking of a candidate is beneficial.
        // So it's unlikely we increase the regpressure in the loop.
        //
        // But due to iterative approach we have some heuristics to sink
        // instruction that don't reduce the regpressure immediately in order to
        // enable the optimization for some potential candidates on the next iteration.
        // Rollback the transformation if the result regpressure becomes higher
        // as a result of such speculative sinking.
        if (MaxLoopPressure > InitialLoopPressure)
        {
            PrintDump("Loop pressure increased after sinking.\n");
            NeedToRollback = true;
        }

        // If we haven't achieved the needed regpressure, it's possible that even if the sinking
        // would be beneficial for small GRF, there still will be spills.
        // In this case there is a chance that just choosing
        // more GRF will be enough to eliminate spills and we would degrade performance
        // if we sinked. So we rollback the changes if autoGRF is provided
        if (Mode == LoopSinkMode::SinkWhileRegpressureIsHigh &&
            !AchievedNeededRegpressure &&
            (NGRF <= 128 && CTX->isAutoGRFSelectionEnabled()) &&
            MaxLoopPressure >= (NGRF + IGC_GET_FLAG_VALUE(LoopSinkRollbackThreshold))
            )
        {
            PrintDump("AutoGRF is enabled and the needed regpressure is not achieved:\n");
            PrintDump("New max loop pressure = " << MaxLoopPressure << "\n");
            PrintDump("Threshold to rollback = " <<
                NGRF + IGC_GET_FLAG_VALUE(LoopSinkRollbackThreshold) << "\n");

            NeedToRollback = true;
        }

        if (NeedToRollback)
        {
            PrintDump(">> Reverting the changes.\n");
            rollbackSinking(true, Preheader, UndoLocas, MovedInsts);
            rerunLiveness();
            return false;
        }

        if (CTX->m_instrTypes.hasDebugInfo)
            ProcessDbgValueInst(*Preheader, DT);

        // We decided we don't rollback, change the names of the instructions in IR
        for (Instruction *I : MovedInsts)
        {
            I->setName("sink_" + I->getName());
        }

        return true;
    }

    bool CodeLoopSinking::isAlwaysSinkInstruction(Instruction *I)
    {
        return (isa<IntToPtrInst>(I) ||
                isa<PtrToIntInst>(I) ||
                isa<ExtractElementInst>(I) ||
                isa<InsertValueInst>(I));
    }

    // Check that this instruction is a part of address calc
    // chain of an already sinked load
    bool CodeLoopSinking::isLoadChain(Instruction *I, SmallPtrSet<Instruction *, 32> &LoadChains, bool EnsureSingleUser)
    {
        if (!isa<BinaryOperator>(I) && !isa<CastInst>(I))
            return false;
        User *InstrUser = IGCLLVM::getUniqueUndroppableUser(I);
        if (EnsureSingleUser && !InstrUser)
            return false;

        return std::all_of(I->user_begin(), I->user_end(),
            [&](User *U)
            {
                Instruction *UI = dyn_cast<Instruction>(U);
                return UI && LoadChains.count(UI);
            });
    }

    // Prepopulate load chain with the loads that are already in the loop
    void CodeLoopSinking::prepopulateLoadChains(Loop *L, SmallPtrSet<Instruction *, 32> &LoadChains)
    {
        std::function<void(Value *)> addInstructionIfLoadChain = [&](Value *V)-> void
        {
            Instruction *I = dyn_cast<Instruction>(V);
            if (!I)
                return;

            if (!L->contains(I))
                return;

            if (!isLoadChain(I, LoadChains))
                return;

            LoadChains.insert(I);
            for (auto &U : I->operands()) {
                addInstructionIfLoadChain(U);
            }
        };

        for (BasicBlock *BB: L->blocks())
        {
            for (Instruction &I : *BB)
            {
                if (LoadInst *LI = dyn_cast<LoadInst>(&I))
                {
                    LoadChains.insert(&I);
                    addInstructionIfLoadChain(LI->getPointerOperand());
                }
            }
        }
    }

    CodeLoopSinking::StoresVec CodeLoopSinking::getAllStoresInLoop(Loop *L)
    {
        IGC_ASSERT(!BlacklistedLoops.count(L));

        // if all the stores for this loop are not memoized yet, do it first
        if (!MemoizedStoresInLoops.count(L))
        {
            llvm::SmallVector<Instruction *, 32>& StoresInLoop = MemoizedStoresInLoops[L];
            for (BasicBlock *BB: L->blocks())
            {
                for (Instruction &I : *BB)
                {
                    if (I.mayWriteToMemory())
                    {
                        StoresInLoop.push_back(&I);
                    }
                }
            }
        }
        return MemoizedStoresInLoops[L];
    }

    /// isSafeToLoopSinkLoad - Determine whether it is safe to sink the load
    /// instruction in the loop using alias information
    bool CodeLoopSinking::isSafeToLoopSinkLoad(Instruction *InstToSink, Loop *L)
    {
        if (!L || !AA)
            return false;

        if (BlacklistedLoops.count(L))
            return false;

        // Only load instructions are supported for now
        if (!isa<LoadInst>(InstToSink))
            return false;

        IGC_ASSERT(InstToSink->getParent() == L->getLoopPreheader());

        auto getRemainingStoresInBB = [](Instruction *I)
        {
            StoresVec Stores;
            BasicBlock *BB = I->getParent();
            Instruction *Last = BB->getTerminator();
            for ( ; I != Last ; I = I->getNextNode())
            {
                if (I->mayWriteToMemory())
                {
                    Stores.push_back(I);
                }
            }
            return Stores;
        };

        StoresVec RemainingStores = getRemainingStoresInBB(InstToSink);
        StoresVec LoopStores = getAllStoresInLoop(L);
        MemoryLocation A = MemoryLocation::get(InstToSink);
        for (auto Stores : { &RemainingStores, &LoopStores })
        {
            for (Instruction *I: *Stores)
            {
                if (StoreInst *SI = dyn_cast<StoreInst>(I))
                {
                    MemoryLocation B = MemoryLocation::get(SI);
                    if (!A.Ptr || !B.Ptr || AA->alias(A, B))
                    {
                        return false;
                    }
                    continue;
                }
                if (GenIntrinsicInst *Intr = dyn_cast<GenIntrinsicInst>(I))
                {
                    if (Intr->getIntrinsicID() == GenISAIntrinsic::GenISA_LSCPrefetch)
                    {
                        continue;
                    }
                }

                // unsupported store
                if (L->contains(I->getParent()))
                    BlacklistedLoops.insert(L);
                return false;
            }
        }

        return true;
    }

    bool CodeLoopSinking::isLoopSinkCandidate(Instruction *I, Loop *L, bool AllowLoadSinking)
    {
        // Limit sinking for the following case for now.
        for (const User *UserInst : I->users())
        {
            if (!isa<Instruction>(UserInst))
                return false;
            if (!L->contains(cast<Instruction>(UserInst)))
                return false;
        }

        if (isAlwaysSinkInstruction(I) || isa<BinaryOperator>(I) || isa<CastInst>(I))
            return true;
        if (isa<LoadInst>(I) && AllowLoadSinking)
            return true;

        return false;
    }

    // Iteration of loop sinking: take the candidates,
    // determine which of them are beneficial to sink
    // and move the instructions
    bool CodeLoopSinking::loopSinkInstructions(
        SmallVector<Instruction *, 64> &SinkCandidates,
        SmallPtrSet<Instruction *, 32> &LoadChains,
        Loop *L)
    {
        struct OperandUseGroup {
            SmallPtrSet <Value *, 4> Operands;
            SmallVector<Instruction *, 16> Users;

            void print(raw_ostream& OS)
            {
                OS << "OUG " << Operands.size() << " -> " << Users.size() << "\n";
                OS << "    Operands:\n";
                for (Value* V : Operands)
                {
                    OS << "  ";
                    V->print(OS);
                    OS << "\n";
                }
                OS << "    Users:\n";
                for (Instruction* I : Users)
                {
                    OS << "  ";
                    I->print(OS);
                    OS << "\n";
                }
            }
        };

        auto isUsedInLoop = [](Value *V, Loop *L) {
            if (isa<Constant>(V))
            {
                // Ignore constant
                return false;
            }
            for (auto UI : V->users())
            {
                if (Instruction *User = dyn_cast<Instruction>(UI))
                {
                    if (L->contains(User))
                        return true;
                }
            }
            return false;
        };

        auto isSameSet = [](SmallPtrSet <Value *, 4> &S0, SmallPtrSet <Value *, 4> &S1) {
            if (S0.size() == S1.size())
            {
                for (auto I : S1)
                {
                    Value* V = I;
                    if (!S0.count(V)) {
                        return false;
                    }
                }
                return true;
            }
            return false;
        };

        // This bitcast is unlikely to increase the regpressure and
        // might enable load sinking on the next iteration.
        auto isBeneficialToSinkBitcast = [&](Instruction *I, OperandUseGroup &OUG) {
            BitCastInst *BC = dyn_cast<BitCastInst>(I);
            if (!BC)
                return false;

            IGC_ASSERT(OUG.Operands.size() <= 1);

            if (OUG.Operands.size() == 0)
                return true;

            Value *V = *(OUG.Operands.begin());
            LoadInst *LI = dyn_cast<LoadInst>(V);
            if (!LI)
                return true;

            // Either load will be sinked before bitcast or the loaded value would anyway be alive
            // in the whole loop body. So it's safe to sink the bitcast
            if (BC->hasOneUse())
                return true;

            // Now it makes sense to sink bitcast only if it would enable load sinking
            // Otherwise it can lead to the increase of register pressure
            if (!IGC_IS_FLAG_ENABLED(EnableLoadsLoopSink) && !IGC_IS_FLAG_ENABLED(ForceLoadsLoopSink))
                return false;

            // Check the load would be a candidate if not for this bitcast
            for (const User *UserInst : LI->users())
            {
                if (!isa<Instruction>(UserInst))
                    return false;
                if (dyn_cast<BitCastInst>(UserInst) == BC)
                    continue;
                if (!L->contains(cast<Instruction>(UserInst)))
                    return false;
            }

            return isSafeToLoopSinkLoad(LI, L);
        };

        // Check if it's beneficial to sink it in the loop
        auto isBeneficialToSink = [&](OperandUseGroup &OUG)-> bool
        {
            auto getDstSize = [this](Value *V)
            {
                int DstSize = 0;
                Type* Ty = V->getType();
                if (Ty->isPointerTy())
                {
                    uint32_t addrSpace = cast<PointerType>(Ty)->getAddressSpace();
                    int PtrSize = (int) CTX->getRegisterPointerSizeInBits(addrSpace);
                    DstSize = PtrSize;
                }
                else
                {
                    DstSize = (int) Ty->getPrimitiveSizeInBits();
                }
                return DstSize;
            };

            // All instructions are safe to sink always or consume larger type than produce
            if (std::all_of(OUG.Users.begin(), OUG.Users.end(),
                [&](Instruction *I)
                {
                    return isAlwaysSinkInstruction(I) ||
                        isCastInstrReducingPressure(I, false) ||
                        isBeneficialToSinkBitcast(I, OUG);
                }))
            {
                return true;
            }

            // Estimate how much regpressure we save (in bytes).
            // Don't count uniform values. This way if every operand that is used only in the loop
            // is uniform, but the User (instruction to sink) is uniform, we'll decide it's beneficial to sink
            int AccSave = 0;

            for (Value *V : OUG.Operands)
            {
                int DstSize = getDstSize(V);
                if (!DstSize)
                    return false;
                if (WI.isUniform(V))
                    continue;
                AccSave -= DstSize / 8;
            }

            bool AllUsersAreUniform = true;
            for (Value *V : OUG.Users)
            {
                int DstSize = getDstSize(V);
                if (!DstSize)
                    return false;
                if (WI.isUniform(V))
                    continue;
                AllUsersAreUniform = false;
                AccSave += DstSize / 8;
            }

            // If all uses are uniform, and we save enough SSA-values it's still beneficial
            if (AccSave >= 0 && AllUsersAreUniform &&
                ((int)OUG.Users.size() - (int)OUG.Operands.size() >= (int)(IGC_GET_FLAG_VALUE(LoopSinkMinSaveUniform))))
            {
                return true;
            }

            // All instructions are part of a chain to already sinked load and don't
            // increase pressure too much. It simplifies the code a little and without
            // adding remat pass for simple cases
            if (AccSave >= 0 && std::all_of(OUG.Users.begin(), OUG.Users.end(),
                [&](Instruction *I) {return isLoadChain(I, LoadChains);}))
            {
                return true;
            }

            // Compare estimated saved regpressure with the specified threshold
            // Number 4 here is just a constant multiplicator of the option to make the numbers more human-friendly,
            // as the typical minimum data size is usually 32-bit. 1 (=4b) means roughly 1 register of saved regpressure
            return AccSave >= (int)(IGC_GET_FLAG_VALUE(LoopSinkMinSave) * 4);
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

        SmallVector<OperandUseGroup, 16> InstUseInfo;
        InstUseInfo.reserve(SinkCandidates.size());

        for (Instruction *I : SinkCandidates)
        {
            SmallPtrSet<Value *, 4> CandidateOperands;
            for (Use &U : I->operands())
            {
                Value *V = U;
                if (isa<Constant>(V) || isUsedInLoop(V, L))
                    continue;

                CandidateOperands.insert(V);
            }

            // If this set of uses have been referenced by other instructions,
            // put this inst in the same group. Note that we don't union sets
            // that intersect each other.
            auto it = std::find_if(InstUseInfo.begin(), InstUseInfo.end(), [&](OperandUseGroup &OUG)
            {
                return isSameSet(OUG.Operands, CandidateOperands);
            });

            if (it != InstUseInfo.end())
                it->Users.push_back(I);
            else
                InstUseInfo.push_back(OperandUseGroup{CandidateOperands, {I}});
        }

        // Sink the instructions from every group if they are beneficial
        bool Changed = false;
        for (OperandUseGroup &OUG : InstUseInfo)
        {

            PrintDump("Checking if sinking the group is beneficial:\n");
            PrintOUGDump(OUG);

            if (!isBeneficialToSink(OUG))
                continue;
            PrintDump(">> Beneficial to sink.\n\n");

            bool GroupChanged = false;
            for (Instruction *I : OUG.Users)
            {
                Instruction *PrevLoc = I->getNextNode();
                bool UserChanged = sinkInstruction(I);
                if (UserChanged)
                {
                    PrintDump("Sinking instruction:\n");
                    PrintInstructionDump(I);

                    UndoLocas.push_back(PrevLoc);
                    MovedInsts.push_back(I);

                    GroupChanged = true;
                    if (isa<LoadInst>(I) || isLoadChain(I, LoadChains))
                        LoadChains.insert(I);
                }
                else
                {
                    PrintDump("Couldn't sink the instruction:\n");
                    PrintInstructionDump(I);
                }
            }
            PrintDump("\n");

            if (GroupChanged)
            {
                Changed = true;

                // If the group is sinked, remove its operands from other groups
                // So that the same operands were not considered in the next's group
                // estimation of whether it's beneficial to sink the users.
                //
                // It's still useful if we don't sink all the users from the group, but sink at least one.
                // Because if we sink, the operands of the sinked group become alive in the loop's body,
                // so they should not be considered for the next group
                for (OperandUseGroup &OUG1 : InstUseInfo)
                {
                    // Just don't remove the operands from the same group
                    // so that we don't lose the operands set
                    if (&OUG1 == &OUG)
                        continue;

                    for (Value *V : OUG.Operands)
                        OUG1.Operands.erase(V);
                }
            }
        }

        return Changed;
    }

    // Find the target BB and move the instruction
    bool CodeLoopSinking::sinkInstruction(Instruction *InstToSink)
    {
        // SuccToSinkTo - This is the successor to sink this instruction to, once we
        // decide.
        BasicBlock *SuccToSinkTo = nullptr;
        SmallPtrSet<Instruction *, 16> UsesInBB;

        // find the lowest common dominator of all uses
        bool IsOuterLoop = false;
        if (BasicBlock *TgtBB = findLowestSinkTarget(InstToSink, UsesInBB, IsOuterLoop, true, DT, LI))
        {
            // heuristic, avoid code-motion that does not reduce execution frequency
            // but may increase register usage
            if (!isa<LoadInst>(InstToSink) ||
                isSafeToLoopSinkLoad(InstToSink, LI->getLoopFor(TgtBB)))
            {
                SuccToSinkTo = TgtBB;
            }
        }

        // If we couldn't find a block to sink to, ignore this instruction.
        if (!SuccToSinkTo)
        {
            return false;
        }

        // when alasing is not an issue and reg-pressure is not an issue
        // move it as close to the uses as possible
        if (UsesInBB.empty())
        {
            InstToSink->moveBefore(SuccToSinkTo->getTerminator());
        }
        else if (UsesInBB.size() == 1)
        {
            InstToSink->moveBefore(*(UsesInBB.begin()));
        }
        else
        {
            // first move to the beginning of the target block
            InstToSink->moveBefore(&(*SuccToSinkTo->getFirstInsertionPt()));
            // later on, move it close to the use
            LocalBlkSet.insert(SuccToSinkTo);
            LocalInstSet.insert(InstToSink);
        }
        return true;
    }

}
