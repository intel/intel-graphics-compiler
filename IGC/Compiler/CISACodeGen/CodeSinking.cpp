/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

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

    // Register pass to igc-opt
#define PASS_FLAG "igc-code-sinking"
#define PASS_DESCRIPTION "code sinking"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
        IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
        IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
        IGC_INITIALIZE_PASS_DEPENDENCY(TranslationTable)
        IGC_INITIALIZE_PASS_DEPENDENCY(IGCLivenessAnalysis)
        IGC_INITIALIZE_PASS_DEPENDENCY(IGCFunctionExternalRegPressureAnalysis)
        IGC_INITIALIZE_PASS_END(CodeSinking, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

        CodeSinking::CodeSinking(bool generalSinking) : FunctionPass(ID), LogStream(Log) {
        generalCodeSinking = generalSinking || IGC_IS_FLAG_ENABLED(ForceLoopSink);
        initializeCodeSinkingPass(*PassRegistry::getPassRegistry());
    }

// Sink in the loop if loop preheader's potential to sink covers at least 20% of registers delta
// between grf number and max estimated pressure in the loop
#define LOOPSINK_PREHEADER_IMPACT_THRESHOLD 0.2

// Helper functions for loop sink debug dumps
#define PrintDump(Contents) if (IGC_IS_FLAG_ENABLED(DumpLoopSink)) {LogStream << Contents;}
#define PrintInstructionDump(Inst) if (IGC_IS_FLAG_ENABLED(DumpLoopSink)) {Inst->print(LogStream, false); LogStream << "\n";}
#define PrintOUGDump(OUG) if (IGC_IS_FLAG_ENABLED(DumpLoopSink)) {OUG->print(LogStream); LogStream << "\n";}


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
        return std::count_if(llvm::inst_begin(F), llvm::inst_end(F), [](const auto& I){ return !isDbgIntrinsic(&I); });
    }

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
                IterChanged |= ProcessBlock(*(domIter->getBlock()));
            }
        } while (IterChanged /*diagnosis: && numChanges < sinkLimit*/);

        EverChanged = IterChanged;
        for (auto BI = LocalBlkSet.begin(), BE = LocalBlkSet.end(); BI != BE; BI++)
        {
            IterChanged = LocalSink(*BI);
            EverChanged |= IterChanged;
        }
        LocalBlkSet.clear();
        LocalInstSet.clear();
        CTX->m_numGradientSinked = totalGradientMoved;

        return EverChanged;
    }

    bool CodeSinking::loopSink(Function &F)
    {
        MemoizedStoresInLoops.clear();
        BlacklistedLoops.clear();
        BBPressures.clear();

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

        DL = &F.getParent()->getDataLayout();

        RPE = &getAnalysis<IGCLivenessAnalysis>();
        FRPE = &getAnalysis<IGCFunctionExternalRegPressureAnalysis>();
        // Note: FRPE is a Module analysis and currently it runs only once.
        // If function A calls function B then
        // it's possible that transformation of function A reduces the regpressure good enough
        // and we could not apply sinking in function B, but we don't recompute FPRE
        // to save compile time, so in this case LoopSinking might apply for loops in function B

        bool Changed = false;

        Changed |= hoistCongruentPhi(F);
        Changed |= treeSink(F);

        if (IGC_IS_FLAG_DISABLED(DisableLoopSink) && CTX->type == ShaderType::OPENCL_SHADER)
        {
            if (Changed && RPE)
                RPE->rerunLivenessAnalysis(F);
            Changed |= loopSink(F);
        }

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

        if (IGC_IS_FLAG_ENABLED(DumpLoopSink))
        {
            if (IGC_IS_FLAG_ENABLED(PrintToConsole))
            {
                IGC::Debug::ods() << Log;
            }
            else
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
                {
                    OutputFile << Log;
                }
                OutputFile.close();
                IGC::Debug::DumpUnlock();
            }
        }

        return Changed;
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
                // diagnosis code: if (numChanges >= sinkLimit)
                // diagnosis code:    continue;
                if (SinkInstruction(inst, stores, false))
                {
                    madeChange = true;
                    MovedInsts.push_back(inst);
                    UndoLocas.push_back(undoLoca);
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
                    rollbackSinking(false, &blk);
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

    void CodeSinking::rollbackSinking(bool ReverseOrder, BasicBlock* BB)
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
                return SrcSize <= DstSize;
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
                reducePressure = (isCastInstrReducingPressure(inst, true) || isa<CmpInst>(inst));
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

    const CodeSinking::StoresVec CodeSinking::getAllStoresInLoop(Loop *L)
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
    /// instruction in the loop, using alias information
    bool CodeSinking::isSafeToLoopSinkLoad(Instruction *InstToSink, Loop *L, AliasAnalysis *AA)
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

    /// SinkInstruction - Determine whether it is safe to sink the specified machine
    /// instruction out of its current block into a successor.
    bool CodeSinking::SinkInstruction(
        Instruction *InstToSink,
        SmallPtrSetImpl<Instruction *> &Stores,
        bool IsLoopSink)
    {
        // Check if it's safe to move the instruction.
        bool HasAliasConcern = false;
        bool ReducePressure = false;

        if (!isSafeToMove(InstToSink, ReducePressure, HasAliasConcern, Stores/*, AA*/))
            return false;

        if (IsLoopSink)
        {
            // forcing that we reduce pressure
            // as we already checked it is beneficial to sink in the loop
            ReducePressure = true;
        }

        // SuccToSinkTo - This is the successor to sink this instruction to, once we
        // decide.
        BasicBlock *SuccToSinkTo = nullptr;
        SmallPtrSet<Instruction *, 16> UsesInBB;

        if (!HasAliasConcern || IsLoopSink)
        {
            // find the lowest common dominator of all uses
            BasicBlock *TgtBB = nullptr;
            bool IsOuterLoop = false;
            if (FindLowestSinkTarget(InstToSink, TgtBB, UsesInBB, IsOuterLoop, IsLoopSink))
            {
                // heuristic, avoid code-motion that does not reduce execution frequency
                // but may increase register usage
                if (ReducePressure ||
                    (TgtBB && (IsOuterLoop || !PDT->dominates(TgtBB, InstToSink->getParent()))))
                {
                    if (!HasAliasConcern ||
                        (IsLoopSink && isSafeToLoopSinkLoad(InstToSink, LI->getLoopFor(TgtBB), AA)))
                    {
                        SuccToSinkTo = TgtBB;
                    }
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
                if (AllUsesDominatedByBlock(InstToSink, (*I), UsesInBB))
                    SuccToSinkTo = *I;
            }
        }

        // If we couldn't find a block to sink to, ignore this instruction.
        if (!SuccToSinkTo)
        {
            return false;
        }

        if (ComputesGradient(InstToSink))
        {
            numGradientMovedOutBB++;
        }

        if (!ReducePressure || (!IsLoopSink && HasAliasConcern))
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

    bool CodeSinking::LocalSink(BasicBlock *BB)
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
        if (Changed) {
            ProcessDbgValueInst(*BB);
        }
        return Changed;
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
                !IGCLLVM::onlyWritesMemory(call0->getCalledFunction()) ||
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

                        // It is possible that the `insertPos` is in the same
                        // block as a "replaced" instruction (the second
                        // instruction in an InstPair) and that the "replaced"
                        // instruction has users that are before the
                        // `insertPos`. In such cases the`insertPos` must be
                        // moved before all such users.
                        IGC_ASSERT(std::all_of(insts.first->user_begin(), insts.first->user_end(),
                            [this, insertPos](User* user)
                            {
                                Instruction* userInst = dyn_cast<Instruction>(user);
                                return userInst == nullptr ||
                                    DT->dominates(insertPos, userInst);
                            }));
                        IGC_ASSERT(std::all_of(insts.second->user_begin(), insts.second->user_end(),
                            [this, insertPos](User* user)
                            {
                                Instruction* userInst = dyn_cast<Instruction>(user);
                                return userInst == nullptr ||
                                    DT->dominates(insertPos->getParent(), userInst->getParent());
                            }));
                        Instruction* insertBefore = insertPos;
                        if (insts.second->getParent() == insertBefore->getParent() &&
                            isInstPrecede(insts.second, insertBefore))
                        {
                            for (User* user : insts.second->users())
                            {
                                Instruction* userInst = dyn_cast<Instruction>(user);
                                if (!userInst ||
                                    userInst->getParent() != insertBefore->getParent())
                                {
                                    continue;
                                }
                                if (isInstPrecede(userInst, insertBefore))
                                {
                                    insertBefore = userInst;
                                }
                            }
                        }
                        ni->insertBefore(insertBefore);
                        ni->setName(VALUE_NAME(ni->getName() + ".hoist"));

                        if (phi->getIncomingValue(0) == I)
                        {
                            // replace phi also
                            phi->replaceAllUsesWith(ni);
                        }
                        I->replaceAllUsesWith(ni);
                        insts.second->replaceAllUsesWith(ni);
                        // Note that instructions are removed in the second loop
                        // below to not invalidate the `insertPos` that may also
                        // be in the `instMap`
                    }
                    for (auto& insts : instMap)
                    {
                        insts.first->eraseFromParent();
                        insts.second->eraseFromParent();
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

    // Implementation of RPE->getMaxRegCountForLoop(*L, SIMD, &WI);
    // with per-BB pressure caching to improve compile-time
    uint CodeSinking::getMaxRegCountForLoop(Loop *L)
    {
        IGC_ASSERT(RPE);
        uint SIMD = numLanes(RPE->bestGuessSIMDSize());
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

    LoopSinkMode CodeSinking::needLoopSink(Loop *L)
    {
        BasicBlock *Preheader = L->getLoopPreheader();
        if (!Preheader)
            return LoopSinkMode::NoSink;
        if (!RPE)
            return LoopSinkMode::NoSink;

        Function *F = Preheader->getParent();
        uint GRFThresholdDelta = IGC_GET_FLAG_VALUE(LoopSinkThresholdDelta);
        uint NGRF = CTX->getNumGRFPerThread();
        uint SIMD = numLanes(RPE->bestGuessSIMDSize());

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

        PrintDump("Threshold to sink = " << NGRF + GRFThresholdDelta << "\n");
        PrintDump("MaxLoopPressure = " << MaxLoopPressure << "\n");
        PrintDump("MaxLoopPressure + FunctionExternalPressure = " << MaxLoopPressure + FunctionExternalPressure << "\n");

        // Sink if the regpressure in the loop is high enough (including function external regpressure)
        if (isSinkCriteriaMet(MaxLoopPressure + FunctionExternalPressure))
            return LoopSinkMode::SinkWhileRegpressureIsHigh;

        PrintDump(">> No sinking.\n");
        return LoopSinkMode::NoSink;
    }

    bool CodeSinking::loopSink(Loop *L, LoopSinkMode Mode)
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

        uint MaxLoopPressure = 0;
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
                        LocalSink(BB);
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
            PrintDump(">> Reverting the changes.\n");

            rollbackSinking(true, Preheader);
            rerunLiveness();

            return false;
        }

        if (EverChanged)
        {
            ProcessDbgValueInst(*Preheader);

            // We decided we don't rollback, change the names of the instructions in IR
            for (Instruction *I : MovedInsts)
            {
                I->setName("sink_" + I->getName());
            }
        }

        return EverChanged;
    }

    bool CodeSinking::isAlwaysSinkInstruction(Instruction *I)
    {
        return (isa<IntToPtrInst>(I) ||
                isa<PtrToIntInst>(I) ||
                isa<ExtractElementInst>(I) ||
                isa<InsertValueInst>(I));
    }

    // Check that this instruction is a part of address calc
    // chain of an already sinked load
    bool CodeSinking::isLoadChain(Instruction *I, SmallPtrSet<Instruction *, 32> &LoadChains, bool EnsureSingleUser)
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
    void CodeSinking::prepopulateLoadChains(Loop *L, SmallPtrSet<Instruction *, 32> &LoadChains)
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

    bool CodeSinking::isLoopSinkCandidate(Instruction *I, Loop *L, bool AllowLoadSinking)
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

    bool CodeSinking::loopSinkInstructions(
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

        // Check if it's beneficial to sink it in the loop
        auto isBeneficialToSink = [&](OperandUseGroup *OUG)-> bool
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

            IGC_ASSERT(OUG);

            // All instructions are safe to sink always or consume larger type than produce
            if (std::all_of(OUG->Users.begin(), OUG->Users.end(),
                [this](Instruction *I)
                {
                    return isAlwaysSinkInstruction(I) || isCastInstrReducingPressure(I, false);
                }))
            {
                return true;
            }

            // Estimate how much regpressure we save (in bytes).
            // Don't count uniform values. This way if every operand that is used only in the loop
            // is uniform, but the User (instruction to sink) is uniform, we'll decide it's beneficial to sink
            int AccSave = 0;

            for (Value *V : OUG->Operands)
            {
                int DstSize = getDstSize(V);
                if (!DstSize)
                    return false;
                if (WI.isUniform(V))
                    continue;
                AccSave -= DstSize / 8;
            }

            bool AllUsersAreUniform = true;
            for (Value *V : OUG->Users)
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
                ((int)OUG->Users.size() - (int)OUG->Operands.size() >= (int)(IGC_GET_FLAG_VALUE(LoopSinkMinSaveUniform))))
            {
                return true;
            }

            // All instructions are part of a chain to already sinked load and don't
            // increase pressure too much. It simplifies the code a little and without
            // adding remat pass for simple cases
            if (AccSave >= 0 && std::all_of(OUG->Users.begin(), OUG->Users.end(),
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
        OperandUseGroup *AllGroups = new OperandUseGroup[SinkCandidates.size()];
        SmallVector<OperandUseGroup *, 16> InstUseInfo;
        for (uint32_t i = 0, e = (uint32_t)SinkCandidates.size(); i < e; ++i)
        {
            Instruction *I = SinkCandidates[i];
            SmallPtrSet<Value *, 4> theUses;
            for (Use &U : I->operands())
            {
                Value *V = U;
                if (isa<Constant>(V) || isUsedInLoop(V, L))
                    continue;

                theUses.insert(V);
            }

            // If this set of uses have been referenced by other instructions,
            // put this inst in the same group. Note that we don't union sets
            // that intersect each other.
            uint32_t j, je = (uint32_t)InstUseInfo.size();
            for (j = 0; j < je; ++j)
            {
                OperandUseGroup *OUG = InstUseInfo[j];
                if (isSameSet(OUG->Operands, theUses)) {
                    OUG->Users.push_back(I);
                    break;
                }
            }

            if (j == je) {
                // No match found, create the new one.
                OperandUseGroup &OUG = AllGroups[i];
                OUG.Operands = std::move(theUses);
                OUG.Users.push_back(I);
                InstUseInfo.push_back(&OUG);
            }
        }

        bool EverChanged = false;
        // Just a placeholder, all LIs considered here are ALUs.
        SmallPtrSet<Instruction *, 16> Stores;
        bool IterChanged;
        uint32_t N = (uint32_t) InstUseInfo.size();
        do {
            IterChanged = false;
            for (uint32_t i = 0; i < N; ++i)
            {
                OperandUseGroup *OUG = InstUseInfo[i];
                if (!OUG)
                    continue;

                PrintDump("Checking if sinking the group is beneficial:\n");
                PrintOUGDump(OUG);

                if (!isBeneficialToSink(OUG))
                    continue;
                PrintDump(">> Beneficial to sink.\n\n");

                bool GroupChanged = false;
                for (int j = 0; j < (int)(OUG->Users.size()); ++j)
                {
                    Instruction *I = OUG->Users[j];
                    Instruction *PrevLoc = I->getNextNode();
                    bool UserChanged = SinkInstruction(I, Stores, true);
                    if (UserChanged)
                    {
                        PrintDump("Sinking instruction:\n");
                        PrintInstructionDump(I);

                        UndoLocas.push_back(PrevLoc);
                        MovedInsts.push_back(I);

                        GroupChanged = true;
                        if (isa<LoadInst>(I) || isLoadChain(I, LoadChains))
                        {
                            LoadChains.insert(I);
                        }
                    }
                }
                if (GroupChanged) {
                    IterChanged = true;
                    EverChanged = true;

                    // Since those operands become global already, remove
                    // them from the sets in the vector.
                    for (uint32_t k = 0; k < N; ++k)
                    {
                        OperandUseGroup *OUG1 = InstUseInfo[k];
                        if (k == i || !OUG1)
                            continue;

                        for (auto I : OUG->Operands) {
                            Value *V = I;
                            OUG1->Operands.erase(V);
                        }
                    }
                }

                // Just set it to nullptr (erasing it would be more expensive).
                InstUseInfo[i] = nullptr;
            }
        } while (IterChanged);

        delete[] AllGroups;

        return EverChanged;
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
