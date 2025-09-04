/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <utility>
#include "Compiler/GenTTI.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"

#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Support/InstructionCost.h"
#include "llvmWrapper/Transforms/Utils/LoopUtils.h"
#include "common/LLVMWarningsPop.hpp"

#include <algorithm>

using namespace llvm;
using namespace IGC;

#define DEBUG_TYPE "GENtti"

namespace llvm {

bool GenIntrinsicsTTIImpl::isLoweredToCall(const Function *F) {
  if (GenISAIntrinsic::isIntrinsic(F))
    return false;
  return BaseT::isLoweredToCall(F);
}

// CFG simplification may produce illegal integer types while simplifying switch
// instructions. Set this to false unless IGC legalization can fix them.
bool GenIntrinsicsTTIImpl::shouldBuildLookupTables() { return false; }

void *GenIntrinsicsTTIImpl::getAdjustedAnalysisPointer(const void *ID) {
  if (ID == &TargetTransformInfoWrapperPass::ID)
    return (TargetTransformInfo *)this;
  return this;
}

bool isSendMessage(const llvm::GenIntrinsicInst *inst) {
  if (isa<SamplerLoadIntrinsic>(inst) || isa<SampleIntrinsic>(inst) || isa<LdRawIntrinsic>(inst) ||
      isa<InfoIntrinsic>(inst) || isa<SamplerGatherIntrinsic>(inst)) {
    return true;
  }

  GenISAIntrinsic::ID ID = inst->getIntrinsicID();
  if (/*ID == llvm::GenISAIntrinsic::GenISA_typedwrite ||
  ID == llvm::GenISAIntrinsic::GenISA_typedread ||*/
      ID == llvm::GenISAIntrinsic::GenISA_URBRead || isURBWriteIntrinsic(inst) ||
      ID == llvm::GenISAIntrinsic::GenISA_ldstructured) {
    return true;
  }

  return false;
}

unsigned countTotalInstructions(const Function *F, bool CheckSendMsg = true) {
  unsigned EstimatedInstCnt = 0;
  for (const auto &BB : *F) {
    for (const auto &II : BB) {
      if (IGCLLVM::isDebugOrPseudoInst(II))
        continue;

      if (auto pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(&II)) {
        if (CheckSendMsg && isSendMessage(pIntrinsic)) {
          EstimatedInstCnt += 4;
        }
      }
      EstimatedInstCnt++;
    }
  }
  return EstimatedInstCnt;
}

unsigned GenIntrinsicsTTIImpl::getFlatAddressSpace() { return ADDRESS_SPACE_PRIVATE; }

bool GenIntrinsicsTTIImpl::isGEPLoopConstDerived(GetElementPtrInst *GEP, const Loop *L, ScalarEvolution &SE) {
  if (!GEP)
    return false;

  const SCEV *SGEP = SE.getSCEV(GEP);

  if (auto *AR = dyn_cast<SCEVAddRecExpr>(SGEP)) {
    if (AR->getLoop() == L)
      return true;
  }

  // Don't let pointer base interfere the traversal. This is due to some frontends
  // generate GEP without inbound.
  const SCEV *SGEPMinusPointerBase = SE.removePointerBase(SGEP);

  struct CheckConstDerived {
    bool TraversalDone = false;
    bool AddRecFound = false;
    bool isConstDerived = true;

    const Loop *L = nullptr;
    const SCEV *S = nullptr;

    CheckConstDerived(const Loop *L) : L(L) {}

    bool setNotConstDerived() {
      TraversalDone = true;
      isConstDerived = false;
      return false;
    }

    bool follow(const SCEV *S) {
      switch (S->getSCEVType()) {
      case scConstant:
      case scPtrToInt:
      case scTruncate:
      case scZeroExtend:
      case scSignExtend:
      case scAddExpr:
      case scMulExpr:
      case scUMaxExpr:
      case scSMaxExpr:
      case scUMinExpr:
      case scSMinExpr:
      case scSequentialUMinExpr:
      case scUDivExpr:
        return true;

      case scAddRecExpr: {
        const auto *ARLoop = cast<SCEVAddRecExpr>(S)->getLoop();
        if (L && (ARLoop == L || ARLoop->contains(L))) {
          AddRecFound = true;
          return false; // Don't traverse into it
        }

        return setNotConstDerived();
      }

      case scUnknown:
      case scCouldNotCompute:
        return setNotConstDerived();
      }
      llvm_unreachable("Unknown SCEV kind!");
    }

    bool isDone() { return TraversalDone; }
  };

  CheckConstDerived CCD(L);
  SCEVTraversal<CheckConstDerived> ST(CCD);
  ST.visitAll(SGEPMinusPointerBase);
  return (CCD.isConstDerived && CCD.AddRecFound);
}

void GenIntrinsicsTTIImpl::getUnrollingPreferences(Loop *L, ScalarEvolution &SE, TTI::UnrollingPreferences &UP,
                                                   OptimizationRemarkEmitter *ORE) {
  bool IsJointMatrixApplyLoop = false;
  for (auto BB : L->blocks()) {
    for (auto &I : *BB) {
      if (auto *MD = I.getMetadata("joint_matrix_apply")) {
        IsJointMatrixApplyLoop = true;
        break;
      }
    }
    if (IsJointMatrixApplyLoop) {
      break;
    }
  }

  unsigned LoopUnrollThreshold = ctx->m_DriverInfo.GetLoopUnrollThreshold();
  bool UnrollLoopForCodeSizeOnly =
      IGC_IS_FLAG_ENABLED(UnrollLoopForCodeSizeOnly) || (!ctx->m_retryManager.IsFirstTry());

  // override the LoopUnrollThreshold if the registry key is set
  if (IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold) != 0) {
    LoopUnrollThreshold = IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold);
  } else {
    if (ctx->type == ShaderType::COMPUTE_SHADER && ctx->getModuleMetaData()->csInfo.SetLoopUnrollThreshold > 0) {
      LoopUnrollThreshold = ctx->getModuleMetaData()->csInfo.SetLoopUnrollThreshold;
    } else if ((ctx->type == ShaderType::PIXEL_SHADER || ctx->type == ShaderType::RAYTRACING_SHADER) &&
               ctx->getModuleMetaData()->compOpt.SetLoopUnrollThreshold > 0) {
      LoopUnrollThreshold = ctx->getModuleMetaData()->compOpt.SetLoopUnrollThreshold;
    } else if (IsJointMatrixApplyLoop) {
      // For joint_matrix_apply loops, we want to unroll them as much as possible so setting high threshold.
      // From the other hand, unrolling huge loops can lead to unreasonable compile-time
      // so we can not just use UINT_MAX.
      // One case, where we hit the limit is when we print accumulator 32x64 elements
      // using joint_matrix_apply with sycl::stream.
      // If you increase this limit, please test that printing with sycl::stream still works.
      LoopUnrollThreshold = 20000;
    }
  }

  // Special case when DP emulation is needed.
  if (ctx->m_hasDPEmu && IGC_IS_FLAG_ENABLED(SelectiveLoopUnrollForDPEmu)) {
    bool hasDPInst = false;
    for (auto BB : L->blocks()) {
      for (auto &I : *BB) {
        switch (I.getOpcode()) {
        case Instruction::FMul:
        case Instruction::FAdd:
        case Instruction::FSub:
        case Instruction::FDiv:
          hasDPInst = I.getType()->isDoubleTy();
          break;
        case Instruction::FCmp:
        case Instruction::FPToUI:
        case Instruction::FPToSI:
        case Instruction::FPTrunc:
          hasDPInst = I.getOperand(0)->getType()->isDoubleTy();
          break;
        case Instruction::UIToFP:
        case Instruction::SIToFP:
        case Instruction::FPExt:
          hasDPInst = I.getType()->isDoubleTy();
          break;
        case Instruction::Call: {
          if (isa<GenIntrinsicInst>(&I) || isa<IntrinsicInst>(&I)) {
            CallInst *callI = cast<CallInst>(&I);
            hasDPInst =
                (callI->getType()->isDoubleTy() || std::any_of(callI->arg_begin(), callI->arg_end(),
                                                               [](Value *v) { return v->getType()->isDoubleTy(); }));
          }
          break;
        }
        default:
          break;
        }

        if (hasDPInst)
          break;
      }
      if (hasDPInst)
        break;
    }
    if (hasDPInst) {
      // Disable unroll
      UP.Threshold = 0;
      UP.OptSizeThreshold = 0;
      UP.Count = 1;
      UP.MaxCount = 1;
      UP.Partial = false;
      UP.Runtime = false;
      return;
    }
  }

  unsigned totalInstCountInShader = countTotalInstructions(L->getBlocks()[0]->getParent());
  uint32_t registerPressureEst =
      (uint32_t)(IGC_GET_FLAG_VALUE(SetRegisterPressureThresholdForLoopUnroll) * (ctx->getNumGRFPerThread() / 128.0));
  bool lowPressure = (this->ctx->m_tempCount < registerPressureEst) && (totalInstCountInShader < LoopUnrollThreshold);
  // For OCL shaders, do a two-step loop unrolling. The first
  // unrolling is simple and full, and the second runs after
  // LICM, which allows partial unrolling. Same for other APIs?
  if (lowPressure || (ctx->type == ShaderType::OPENCL_SHADER))
  {
    UP.Threshold = LoopUnrollThreshold;
    UP.PartialThreshold = LoopUnrollThreshold;
    UP.Partial = true;
  } else // for high registry pressure shaders, limit the unrolling to small loops and only fully unroll
  {
    UP.Threshold = IGC_GET_FLAG_VALUE(SetLoopUnrollThresholdForHighRegPressure);
    // This is similiar to LLVM OptForSize scenario in LoopUnrollPass
    UP.MaxPercentThresholdBoost = IGC_GET_FLAG_VALUE(SetLoopUnrollMaxPercentThresholdBoostForHighRegPressure);
  }

  unsigned MaxTripCount = SE.getSmallConstantMaxTripCount(L);
  const unsigned MaxTripCountToUseUpperBound = 4;
  if (MaxTripCount && MaxTripCount <= MaxTripCountToUseUpperBound) {
    UP.UpperBound = true;
    UP.Force = true;
  }

  if (UnrollLoopForCodeSizeOnly) {
    UP.Threshold = getLoopSize(L, *this) + 1;
    UP.MaxPercentThresholdBoost = 100;
    UP.Partial = false;
  }

  // For all the load/store who (having a GEP to),
  // 1. Accessing a fixed size Alloca
  // 2. Having an loop-iteration-inducted-only index
  // For exmaple,
  //
  // bb:
  //   %ALLOC = alloca [32 x float], align 4
  // Loop1:
  //   %i8 = phi i32 [ 0, %bb ], [ %i23, %Loop1 ]
  //   %i19 = getelementptr [32 x float], ptr %ALLOC, i64 0, i64 %i8
  //   %i23 = add i32 %i8, 1
  //   %i14 = fmul ...
  //   store float %i14, ptr %i19, align 4
  //   %i24 = icmp eq i32 %i23, 32
  //   br i1 %i24, label %..., label %Loop1
  // ...
  // Loop5:
  //   %i93 = phi i32 [ %i115, %Loop5 ], [ 0, %... ]
  //   %i99 = getelementptr [32 x float], ptr %ALLOC, i64 0, i64 %i93
  //   %i103 = load float, ptr %i99, align 4
  //   %i107 = fmul float %i103, 0x3F699999A0000000
  //   %i115 = add i32 %i93, 1
  //   %i116 = icmp eq i32 %i115, 32
  //   br i1 %i116, label %bb117, label %Loop5
  //
  // Fully unrolling both loops leads SROA pass eliminate the entire access chain of the alloca. This is one of most
  // impacted yet super common pattern across all application types. In many cases, especially when the only values that
  // stored into alloca are compiler-detectable constant, these loops need to be unroll regardless how high the register
  // pressure is.
  //
  // TODO: Having an analysis pass to link alloca with loops globally so that they are either unrolled together or not.
  //       It can potentially do some global cost estimations.
  const unsigned UnrollMaxCountForAlloca = IGC_GET_FLAG_VALUE(PromoteLoopUnrollwithAllocaCountThreshold);
  bool AllocaFound = false;
  if (MaxTripCount && MaxTripCount <= UnrollMaxCountForAlloca &&
      IGC_IS_FLAG_ENABLED(EnablePromoteLoopUnrollwithAlloca)) {
    unsigned int ThresholdBoost = 0;
    for (auto BB : L->blocks()) {
      for (auto &I : *BB) {
        AllocaInst *AI = nullptr;
        GetElementPtrInst *GEP = nullptr;

        if (auto *LI = dyn_cast<LoadInst>(&I))
          AI = dyn_cast<AllocaInst>(LI->getPointerOperand());
        else if ((GEP = dyn_cast<GetElementPtrInst>(&I))) {
          // Test if the GEP index is a function of the loop induction variable.
          if (!isGEPLoopConstDerived(GEP, L, SE))
            continue;

          auto *SBase = dyn_cast<SCEVUnknown>(SE.getPointerBase(SE.getSCEV(GEP)));
          AI = dyn_cast<AllocaInst>(SBase->getValue());
        } else
          continue;

        if (!AI)
          continue;
        // Not fixed size or not in entry block
        // TODO: Can a alloca with a fixed size not reside in the entry block?
        if (!AI->isStaticAlloca())
          continue;
        // Assume every iteration consumes 1 alloca element.
        if (cast<ConstantInt>(AI->getArraySize())->getZExtValue() > UnrollMaxCountForAlloca)
          continue;

        // Using alloca size in bytes as the threshold boost seems a bit tricky.
        unsigned AllocaSize = *(AI->getAllocationSizeInBits(DL)) / 8;
        ThresholdBoost += AllocaSize;
        if (GEP)
          isGEPLoopInduction[GEP] = true;
        AllocaFound = true;
      }
    }
    if (AllocaFound) {
      // LLVM default only to 10, boost to UnrollMaxCountForAlloca
      UP.MaxIterationsCountToAnalyze = UnrollMaxCountForAlloca;
      UP.UpperBound = true;
      UP.Force = UnrollLoopForCodeSizeOnly ? false : true;

      if (ctx->type != ShaderType::OPENCL_SHADER) {
        UP.Threshold += ThresholdBoost;
        LLVM_DEBUG(dbgs() << "Increasing L:" << L->getName() << " threshold to " << UP.Threshold
                          << " due to Alloca accessed by:");
        for (const auto &pair : isGEPLoopInduction)
          LLVM_DEBUG(dbgs() << " " << pair.first->getName());
        LLVM_DEBUG(dbgs() << " \n");
      }
    }
  }

  if (IGC_IS_FLAG_ENABLED(UnrollLoopForCodeSizeOnly))
    return;

  unsigned sendMessage = 0;
  unsigned TripCount = 0;
  BasicBlock *ExitingBlock = L->getLoopLatch();
  if (!ExitingBlock || !L->isLoopExiting(ExitingBlock))
    ExitingBlock = L->getExitingBlock();
  if (ExitingBlock)
    TripCount = SE.getSmallConstantTripCount(L, ExitingBlock);

  // Do not enable partial unrolling if the loop counter is float. It can cause precision issue.
  if (ExitingBlock) {
    if (UP.Partial) {
      IGCLLVM::TerminatorInst *Term = ExitingBlock->getTerminator();
      if (BranchInst *BI = dyn_cast<BranchInst>(Term)) {
        if (dyn_cast<FCmpInst>(BI->getCondition())) {
          UP.Partial = false;
          return;
        }
      }
    }
    // Add heuristic to disable loop unroll for single short BB loop with
    // barrier. Unrolling such a loop won't remove dependency due to that
    // barrier but only add register pressure potentially.
    if (L->getNumBlocks() == 1) {
      BasicBlock *BB = *L->block_begin();

      SmallPtrSet<const Value *, 32> EphValues;
      CodeMetrics Metrics;
      Metrics.analyzeBasicBlock(BB, *this, EphValues);
      if (Metrics.NumInsts < 50) {
        for (auto I = BB->begin(), E = BB->end(); I != E; ++I) {
          CallInst *Call = dyn_cast<CallInst>(I);
          if (!Call)
            continue;
          Function *F = Call->getCalledFunction();
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
    if (IGC_IS_FLAG_ENABLED(EnableAdvRuntimeUnroll) && IGCLLVM::isInnermost(L)) {
      auto countNonPHI = [](BasicBlock *BB) {
        // Count the number of instructions in the basic block without dbg instructions
        unsigned InstCountInBB = BB->sizeWithoutDebug();
        unsigned PHIs = 0;
        for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI) {
          if (!isa<PHINode>(&*BI))
            break;
          ++PHIs;
        }
        return InstCountInBB - PHIs;
      };
      auto hasLoad = [](BasicBlock *BB) {
        for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
          if (isa<LoadInst>(&*BI))
            return true;
        return false;
      };
      auto hasStore = [](BasicBlock *BB) {
        for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
          if (isa<StoreInst>(&*BI))
            return true;
        return false;
      };
      auto hasCall = [](BasicBlock *BB) {
        for (auto BI = BB->begin(), BE = BB->end(); BI != BE; ++BI)
          if (isa<CallInst>(&*BI) && !isa<IntrinsicInst>(&*BI) && !isa<GenIntrinsicInst>(&*BI))
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
        if (C == 0)
          C = 4;
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
  llvm::BasicBlock *loopBlock = L->getBlocks()[0];
  int instCount =
      std::distance(loopBlock->instructionsWithoutDebug().begin(), loopBlock->instructionsWithoutDebug().end());

  // Check if the specific basic block has block read or write.
  auto hasBlockReadWrite = [](BasicBlock *BB) {
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
      ((TripCount != 0 && TripCount <= 40 && instCount < 40) || hasBlockReadWrite(L->getHeader())) &&
      // FIXME: WA for cases where the compiler is running with a smaller stack size
      // we run into stack overflow in
      !ctx->m_DriverInfo.HasSmallStack()) {
    return;
  }

  for (I = loopBlock->begin(); I != loopBlock->end(); I++) {
    if (const auto pIntrinsic = llvm::dyn_cast<llvm::GenIntrinsicInst>(I)) {
      if (isSendMessage(pIntrinsic)) {
        sendMessage++;
      }
    }
  }

  unsigned int estimateUnrolledInstCount = (instCount + sendMessage * 4) * TripCount;
  unsigned int unrollLimitInstCount =
      LoopUnrollThreshold > totalInstCountInShader ? LoopUnrollThreshold - totalInstCountInShader : 0;
  bool limitUnrolling = (estimateUnrolledInstCount > unrollLimitInstCount) || (TripCount > unrollLimitInstCount) ||
                        (instCount + sendMessage * 4 > unrollLimitInstCount);

  // if the loop doesn't have sample, skip the unrolling parameter change
  if (!sendMessage) {
    // if the estimated unrolled instruction count is larger than the unrolling threshold, limit unrolling.
    if (limitUnrolling) {
      UP.Count = MIN(unrollLimitInstCount / (instCount + sendMessage * 4), 4);
      if (TripCount != 0)
        while (UP.Count != 0 && TripCount % UP.Count != 0)
          UP.Count--;
      UP.MaxCount = UP.Count;
    }
    return;
  }

  // if the TripCount is known, and the estimated unrolled count exceed LoopUnrollThreshold, set the unrolling count to
  // 4
  if (limitUnrolling) {
    UP.Count = MIN(TripCount, 4);
    UP.MaxCount = UP.Count;
  }

  unsigned int runtimeUnroll = IGC_GET_FLAG_VALUE(RuntimeLoopUnrolling); // 0: default, 1: on, 2: off
  if (runtimeUnroll == 2) {
    return;
  } else if (runtimeUnroll == 0) {
    // do not enable runtime unrolling if the loop is long or trip count is already known.
    // skip this check if RuntimeLoopUnrolling is set to force on.
    if (instCount > 35 || TripCount) {
      return;
    }
  }

  if (!limitUnrolling) {
    UP.Runtime = true;
    UP.Count = 4;
    UP.MaxCount = UP.Count;
    // The following is only available and required from LLVM 3.7+.
    UP.AllowExpensiveTripCount = true;
  }

  if (MDNode *LoopID = L->getLoopID()) {
    const llvm::StringRef maxIterMetadataNames = "spv.loop.iterations.max";
#if LLVM_VERSION_MAJOR < 11
    const llvm::StringRef peelCountMetadataNames = "spv.loop.peel.count";
#endif
    for (unsigned i = 0; i < LoopID->getNumOperands(); ++i) {
      if (MDNode *MD = llvm::dyn_cast<MDNode>(LoopID->getOperand(i))) {
        if (MDString *S = llvm::dyn_cast<MDString>(MD->getOperand(0))) {
          if (maxIterMetadataNames.equals(S->getString())) {
            UP.MaxCount = static_cast<unsigned>(mdconst::extract<ConstantInt>(MD->getOperand(1))->getZExtValue());
          }
#if LLVM_VERSION_MAJOR < 11
          else if (peelCountMetadataNames.equals(S->getString())) {
            UP.AllowPeeling = true;
            UP.PeelCount = static_cast<unsigned>(mdconst::extract<ConstantInt>(MD->getOperand(1))->getZExtValue());
          }
#endif
        }
      }
    }
  }
}

#if LLVM_VERSION_MAJOR >= 11
// [LLVM-UPGRADE] Peeling information was separated
// https://github.com/llvm/llvm-project/commit/e541e1b757237172c247904b670c9894d6b3759d

void GenIntrinsicsTTIImpl::getPeelingPreferences(Loop *L, ScalarEvolution &SE,
                                                 llvm::TargetTransformInfo::PeelingPreferences &PP) {
  if (MDNode *LoopID = L->getLoopID()) {
    const llvm::StringRef peelCountMetadataNames = "spv.loop.peel.count";

    for (unsigned i = 0; i < LoopID->getNumOperands(); ++i) {
      if (MDNode *MD = llvm::dyn_cast<MDNode>(LoopID->getOperand(i))) {
        if (MDString *S = llvm::dyn_cast<MDString>(MD->getOperand(0))) {
          if (peelCountMetadataNames.equals(S->getString())) {
            PP.AllowPeeling = true;
            PP.PeelCount = static_cast<unsigned>(mdconst::extract<ConstantInt>(MD->getOperand(1))->getZExtValue());
          }
        }
      }
    }
  }
}
#endif

bool GenIntrinsicsTTIImpl::isProfitableToHoist(Instruction *I) {
  if (auto *CI = dyn_cast<CallInst>(I)) {
    if (CI->isConvergent() && CI->onlyAccessesInaccessibleMemory()) {
      return false;
    }
  }
  return BaseT::isProfitableToHoist(I);
}

// TODO: Upon the complete removal of pre-LLVM 14 conditions, move to 'getInstructionCost' per LLVM 16 API
llvm::InstructionCost GenIntrinsicsTTIImpl::getUserCost(const User *U, ArrayRef<const Value *> Operands,
                                                        TTI::TargetCostKind CostKind) {
  return GenIntrinsicsTTIImpl::internalCalculateCost(U, Operands, CostKind);
}

#if LLVM_VERSION_MAJOR >= 16
llvm::InstructionCost GenIntrinsicsTTIImpl::getInstructionCost(const User *U, ArrayRef<const Value *> Operands,
                                                               TTI::TargetCostKind CostKind) {
  return GenIntrinsicsTTIImpl::internalCalculateCost(U, Operands, CostKind);
}
#endif

llvm::InstructionCost GenIntrinsicsTTIImpl::internalCalculateCost(const User *U, ArrayRef<const Value *> Operands,
                                                                  TTI::TargetCostKind CostKind) {
  // The extra cost of speculative execution for math intrinsics
  if (auto *II = dyn_cast_or_null<IntrinsicInst>(U)) {
    if (Intrinsic::ID IID = II->getIntrinsicID()) {
      switch (IID) {
      case Intrinsic::cos:
      case Intrinsic::sin:
      case Intrinsic::sqrt:
        return TTI::TCC_Expensive;
      default:
        break;
      }
    }
  }

  if (IGC_IS_FLAG_ENABLED(EnablePromoteLoopUnrollwithAlloca)) {
    const GetElementPtrInst *GEP = nullptr;
    if (Operator::getOpcode(U) == Instruction::Load)
      GEP = dyn_cast<GetElementPtrInst>(cast<LoadInst>(U)->getPointerOperand());
    if (Operator::getOpcode(U) == Instruction::Store)
      GEP = dyn_cast<GetElementPtrInst>(cast<StoreInst>(U)->getPointerOperand());

    if (GEP) {
      if (isGEPLoopInduction.find(GEP) != isGEPLoopInduction.end())
        return TTI::TCC_Free;
    }
  }

  const Function *F = dyn_cast<Function>(U);
  if (F != nullptr) {
    IGC::CodeGenContext *CGC = this->ctx;
    if (!CGC->enableFunctionCall() && !GenISAIntrinsic::isIntrinsic(F) && !F->isIntrinsic()) {
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
  }

  return BaseT::getInstructionCost(U, Operands, CostKind);
}

// Strip from LLVM::LoopUnrollPass::ApproximateLoopSize
unsigned getLoopSize(const Loop *L, const TargetTransformInfo &TTI) {
  SmallPtrSet<const Value *, 32> EphValues;

  CodeMetrics Metrics;
  for (BasicBlock *BB : L->blocks())
    Metrics.analyzeBasicBlock(BB, TTI, EphValues);

  InstructionCost LoopSize;
  LoopSize = Metrics.NumInsts;

  LoopSize = (LoopSize > 3/*BEInsns + 1*/) ? LoopSize : 3;
  return *LoopSize.getValue();
}

} // namespace llvm
