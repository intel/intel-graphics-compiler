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

#include "Compiler/GenTTI.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

#include "common/LLVMWarningsPush.hpp"

#include "WrapperLLVM/Utils.h"

#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;


namespace llvm {

    char DummyPass::ID = 0;
    void initializeDummyPassPass(PassRegistry& Registry);
    DummyPass::DummyPass() : ImmutablePass(ID) {
        initializeDummyPassPass(*PassRegistry::getPassRegistry());
    }

    bool GenIntrinsicsTTIImpl::isLoweredToCall(const Function* F)
    {
        if (GenISAIntrinsic::isIntrinsic(F))
            return false;
        return BaseT::isLoweredToCall(F);
    }

    // CFG simplification may produce illegal integer types while simplifying switch
    // instructions. Set this to false unless IGC legalization can fix them.
    bool GenIntrinsicsTTIImpl::shouldBuildLookupTables()
    {
        return false;
    }

    void* GenIntrinsicsTTIImpl::getAdjustedAnalysisPointer(const void* ID)
    {
        if (ID == &TargetTransformInfoWrapperPass::ID)
            return (TargetTransformInfo*)this;
        return this;
    }


    bool isSendMessage(GenIntrinsicInst* inst)
    {
        if (isa<SamplerLoadIntrinsic>(inst) ||
            isa<SampleIntrinsic>(inst) ||
            isa<LdRawIntrinsic>(inst) ||
            isa<InfoIntrinsic>(inst) ||
            isa<SamplerGatherIntrinsic>(inst))
        {
            return true;
        }

        GenISAIntrinsic::ID ID = inst->getIntrinsicID();
        if (/*ID == llvm::GenISAIntrinsic::GenISA_typedwrite ||
            ID == llvm::GenISAIntrinsic::GenISA_typedread ||*/
            ID == llvm::GenISAIntrinsic::GenISA_URBRead ||
            isURBWriteIntrinsic(inst) ||
            ID == llvm::GenISAIntrinsic::GenISA_ldstructured)
        {
            return true;
        }

        return false;
    }

    unsigned countTotalInstructions(const Function* F, bool CheckSendMsg = true) {
        unsigned EstimatedInstCnt = 0;
        for (auto BBI = F->getBasicBlockList().begin(); BBI != F->getBasicBlockList().end(); BBI++)
        {
            llvm::BasicBlock* BB = const_cast<llvm::BasicBlock*>(&*BBI);
            for (auto II = BB->begin(); II != BB->end(); II++)
            {
                if (llvm::GenIntrinsicInst * pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(II))
                {
                    if (CheckSendMsg && isSendMessage(pIntrinsic))
                    {
                        EstimatedInstCnt += 4;
                    }
                }
                EstimatedInstCnt++;
            }
        }
        return EstimatedInstCnt;
    }

    void GenIntrinsicsTTIImpl::getUnrollingPreferences(Loop* L,
#if LLVM_VERSION_MAJOR >= 7
        ScalarEvolution & SE,
#endif
        TTI::UnrollingPreferences & UP)
    {
        unsigned LoopUnrollThreshold = ctx->m_DriverInfo.GetLoopUnrollThreshold();

        // override the LoopUnrollThreshold if the registry key is set
        if (IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold) != 0)
        {
            LoopUnrollThreshold = IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold);
        }
        unsigned totalInstCountInShader = countTotalInstructions(L->getBlocks()[0]->getParent());
        bool lowPressure = (this->ctx->m_tempCount < IGC_GET_FLAG_VALUE(SetRegisterPressureThresholdForLoopUnroll)) && (totalInstCountInShader < LoopUnrollThreshold);
        // For OCL shaders, do a two-step loop unrolling. The first
        // unrolling is simple and full, and the second runs after
        // LICM, which allows partial unrolling. Same for other APIs?
        if (lowPressure || (ctx->type == ShaderType::OPENCL_SHADER) ||
            (!IGC_IS_FLAG_ENABLED(DisableDynamicConstantFolding) && (!ctx->getModuleMetaData()->inlineDynConstants.empty())))
        {
            UP.Threshold = LoopUnrollThreshold;
            UP.PartialThreshold = LoopUnrollThreshold;
            UP.Partial = true;
        }
        else  // for high registry pressure shaders, limit the unrolling to small loops and only fully unroll
        {
            if(IGC_GET_FLAG_VALUE(SetLoopUnrollThresholdForHighRegPressure) != 0)
                UP.Threshold = IGC_GET_FLAG_VALUE(SetLoopUnrollThresholdForHighRegPressure);
            else
                UP.Threshold = 200;
        }

#if LLVM_VERSION_MAJOR == 4
        ScalarEvolution * SE = &dummyPass->getAnalysisIfAvailable<ScalarEvolutionWrapperPass>()->getSE();
        if (!SE)
            return;
#endif

        unsigned sendMessage = 0;
        unsigned TripCount = 0;
        BasicBlock* ExitingBlock = L->getLoopLatch();
        if (!ExitingBlock || !L->isLoopExiting(ExitingBlock))
            ExitingBlock = L->getExitingBlock();
        if (ExitingBlock)
            TripCount = IGCLLVM::getref(SE).getSmallConstantTripCount(L, ExitingBlock);

        // Do not enable partial unrolling if the loop counter is float. It can cause precision issue.
        if (ExitingBlock) {
            if (UP.Partial) {
                IGCLLVM::TerminatorInst* Term = ExitingBlock->getTerminator();
                if (BranchInst * BI = dyn_cast<BranchInst>(Term))
                {
                    if (dyn_cast<FCmpInst>(BI->getCondition()))
                    {
                        UP.Partial = false;
                        return;
                    }
                }
            }
            // Add heuristic to disable loop unroll for single short BB loop with
            // barrier. Unrolling such a loop won't remove dependency due to that
            // barrier but only add register pressure potentially.
            if (L->getNumBlocks() == 1) {
                BasicBlock* BB = *L->block_begin();

                SmallPtrSet<const Value*, 32> EphValues;
                CodeMetrics Metrics;
                Metrics.analyzeBasicBlock(BB, *this, EphValues);
                if (Metrics.NumInsts < 50) {
                    for (auto I = BB->begin(), E = BB->end(); I != E; ++I) {
                        CallInst* Call = dyn_cast<CallInst>(I);
                        if (!Call)
                            continue;
                        Function* F = Call->getCalledFunction();
                        if (!F)
                            continue;
                        // FIXME: Shall we already inline barrier even in two-phase
                        // inlining?
                        if (F->getName() == "_Z7barrierj") {
                            // Disable loop unrolling for short loop with
                            // barrier, where we prefer wider SIMD to mitigate
                            // the barrier overhead.
                            UP.Count = 1;
                            UP.MaxCount = UP.Count;
                            UP.Partial = false;
                            UP.Runtime = false;
                            return;
                        }
                    }
                }
            }
        }

        // Skip non-simple loop.
        if (L->getNumBlocks() != 1) {
            if (IGC_IS_FLAG_ENABLED(EnableAdvRuntimeUnroll) && L->empty()) {
                auto countNonPHI = [](BasicBlock* BB) {
                    unsigned Total = BB->size();
                    unsigned PHIs = 0;
                    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
                        if (!isa<PHINode>(&*BI))
                            break;
                        ++PHIs;
                    }
                    return Total - PHIs;
                };
                auto hasLoad = [](BasicBlock* BB) {
                    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
                        if (isa<LoadInst>(&*BI))
                            return true;
                    return false;
                };
                auto hasStore = [](BasicBlock* BB) {
                    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
                        if (isa<StoreInst>(&*BI))
                            return true;
                    return false;
                };
                auto hasCall = [](BasicBlock* BB) {
                    for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
                        if (isa<CallInst>(&*BI) &&
                            !isa<IntrinsicInst>(&*BI) && !isa<GenIntrinsicInst>(&*BI))
                            return true;
                    return false;
                };
                // For innermost loop, allow certain patterns.
                unsigned Count = 0;
                bool HasCall = false;
                bool HasStore = false;
                bool MayHasLoadInHeaderOnly = true;
                for (auto BI = L->block_begin(), BE = L->block_end(); BI != BE; ++BI) {
                    Count += countNonPHI(*BI);
                    HasCall |= hasCall(*BI);
                    HasStore |= hasStore(*BI);
                    if (L->getHeader() != *BI)
                        MayHasLoadInHeaderOnly &= !hasLoad(*BI);
                }
                // Runtime unroll it.
                if (!HasCall && !HasStore && MayHasLoadInHeaderOnly && Count < 100) {
                    unsigned C = IGC_GET_FLAG_VALUE(AdvRuntimeUnrollCount);
                    if (C == 0) C = 4;
                    UP.Runtime = true;
                    UP.Count = C;
                    UP.MaxCount = UP.Count;
                    // The following is only available and required from LLVM 3.7+.
                    UP.AllowExpensiveTripCount = true;
                }

            }
            return;
        }

        llvm::BasicBlock::InstListType::iterator I;
        llvm::BasicBlock::InstListType& instructionList = L->getBlocks()[0]->getInstList();
        int instCount = instructionList.size();

        // Check if the specific basic block has block read or write.
        auto hasBlockReadWrite = [](BasicBlock* BB) -> bool {
            for (auto I = BB->begin(), E = BB->end(); I != E; ++I)
                if (auto GII = dyn_cast<GenIntrinsicInst>(I))
                    switch (GII->getIntrinsicID()) {
                    case GenISAIntrinsic::GenISA_simdBlockRead:
                    case GenISAIntrinsic::GenISA_simdBlockWrite:
                        return true;
                    default:
                        break;
                    }
            return false;
        };
        // Skip the following logic for OCL. Apparently, it's designed to prevent
        // loops in 3D shaders being aggressively unrolled to increase shader
        // binary size dramatically. So far, OCL doesn't have such concern and, if
        // we need to consider that, more factors need consideration. Just skip
        // that for OCL.
        if (ctx->type == ShaderType::OPENCL_SHADER &&
            // Only try to fully unroll small loop with known but small trip count.
            // This's PURELY heuristics.
            ((TripCount != 0 && TripCount <= 40 && instCount < 40) ||
                hasBlockReadWrite(L->getHeader())) &&
            // FIXME: WA for cases where the compiler is running with a smaller stack size
            // we run into stack overflow in
            !ctx->m_DriverInfo.HasSmallStack())
        {
            return;
        }

        for (I = instructionList.begin(); I != instructionList.end(); I++)
        {
            if (llvm::GenIntrinsicInst * pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(I))
            {
                if (isSendMessage(pIntrinsic))
                {
                    sendMessage++;
                }
            }
        }

        unsigned int estimateUnrolledInstCount = (instCount + sendMessage * 4) * TripCount;
        unsigned int unrollLimitInstCount = LoopUnrollThreshold > totalInstCountInShader ? LoopUnrollThreshold - totalInstCountInShader : 0;
        bool limitUnrolling = (estimateUnrolledInstCount > unrollLimitInstCount) ||
                              (TripCount > unrollLimitInstCount) ||
                              (instCount + sendMessage * 4 > unrollLimitInstCount);

        // if the loop doesn't have sample, skip the unrolling parameter change
        if (!sendMessage)
        {
            // if the estimated unrolled instruction count is larger than the unrolling threshold, limit unrolling.
            if (limitUnrolling)
            {
                UP.Count = MIN(unrollLimitInstCount / (instCount + sendMessage * 4), 4);
                if (TripCount != 0)
                    while (UP.Count != 0 && TripCount % UP.Count != 0)
                        UP.Count--;
                UP.MaxCount = UP.Count;
            }
            return;
        }

        // if the TripCount is known, and the estimated unrolled count exceed LoopUnrollThreshold, set the unrolling count to 4
        if (limitUnrolling)
        {
            UP.Count = MIN(TripCount, 4);
            UP.MaxCount = UP.Count;
        }

        // do not enable runtime unrolling if the loop is long or trip count is already known.
        if (instCount > 35 || TripCount)
        {
            return;
        }

        if (IGC_IS_FLAG_ENABLED(DisableRuntimeLoopUnrolling))
        {
            return;
        }

        UP.Runtime = true;
        UP.Count = 4;
        UP.MaxCount = UP.Count;
        // The following is only available and required from LLVM 3.7+.
        UP.AllowExpensiveTripCount = true;

        if (MDNode * LoopID = L->getLoopID())
        {
            const llvm::StringRef maxIterMetadataNames = "spv.loop.iterations.max";
            const llvm::StringRef peelCountMetadataNames = "spv.loop.peel.count";

            for (unsigned i = 0; i < LoopID->getNumOperands(); ++i)
            {
                if (MDNode * MD = llvm::dyn_cast<MDNode>(LoopID->getOperand(i)))
                {
                    if (MDString * S = llvm::dyn_cast<MDString>(MD->getOperand(0)))
                    {
                        if (maxIterMetadataNames.equals(S->getString()))
                        {
                            UP.MaxCount = static_cast<unsigned>(
                                mdconst::extract<ConstantInt>(MD->getOperand(1))->getZExtValue());
                        }
                        else if (peelCountMetadataNames.equals(S->getString()))
                        {
                            UP.AllowPeeling = true;
                            UP.PeelCount = static_cast<unsigned>(
                                mdconst::extract<ConstantInt>(MD->getOperand(1))->getZExtValue());
                        }
                    }
                }
            }
        }
    }

    bool GenIntrinsicsTTIImpl::isProfitableToHoist(Instruction* I)
    {
        if (auto * CI = dyn_cast<CallInst>(I))
        {
            if (CI->isConvergent() &&
#if LLVM_VERSION_MAJOR >= 7
                CI->onlyAccessesInaccessibleMemory()
#else
                CI->hasFnAttr(Attribute::InaccessibleMemOnly)
#endif
                )
            {
                return false;
            }
        }
        return BaseT::isProfitableToHoist(I);
    }

    unsigned GenIntrinsicsTTIImpl::getCallCost(const Function* F, ArrayRef<const Value*> Arguments
#if LLVM_VERSION_MAJOR >= 9
        , const User * U
#endif
    ) {
        IGC::CodeGenContext* CGC = this->ctx;
        if (!CGC->enableFunctionCall() && !GenISAIntrinsic::isIntrinsic(F) &&
            !F->isIntrinsic()) {
            // If subroutine call is not enabled but we have function call. They
            // are not inlined. e.g. due to two-phase inlining. Return function
            // size instead of to avoid under-estimating the cost of function call.
            //
            // FIXME: We need to collect the cost following calling graph. However,
            // as LLVM's ininer only support bottom-up inlining currently. That's
            // not a big issue so far.
            //
            // FIXME: We also need to consider the case where sub-routine call is
            // enabled.
            unsigned FuncSize = countTotalInstructions(F, false);
            return TargetTransformInfo::TCC_Basic * FuncSize;
        }
        return BaseT::getCallCost(F, Arguments
#if LLVM_VERSION_MAJOR >= 9
            , U
#endif
        );
    }

} // namespace llvm
// Register the basic pass.
INITIALIZE_PASS(DummyPass, "gen-tti-dummy-pass",
    "Dummy Pass for GenTTIImpl", false, true)
