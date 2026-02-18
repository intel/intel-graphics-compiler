/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "AdaptorOCL/MergeAllocasOCL.h"
#include "Compiler/Legalizer/PeepholeTypeLegalizer.hpp"
#include "Compiler/CISACodeGen/DropTargetBBs.hpp"
#include "Compiler/CISACodeGen/layout.hpp"
#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "Compiler/CISACodeGen/AdvCodeMotion.h"
#include "Compiler/CISACodeGen/RematAddressArithmetic.h"
#include "Compiler/CISACodeGen/VectorShuffleAnalysis.hpp"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include "Compiler/CISACodeGen/IGCVectorizer.h"
#include "Compiler/CISACodeGen/AdvMemOpt.h"
#include "Compiler/CISACodeGen/Emu64OpsPass.h"
#include "Compiler/CISACodeGen/PushAnalysis.hpp"
#include "Compiler/CISACodeGen/ScalarizerCodeGen.hpp"
#include "Compiler/CISACodeGen/HoistCongruentPhi.hpp"
#include "Compiler/CISACodeGen/CodeScheduling.hpp"
#include "Compiler/CISACodeGen/CodeSinking.hpp"
#include "Compiler/CISACodeGen/AddressArithmeticSinking.hpp"
#include "Compiler/CISACodeGen/AtomicOptPass.hpp"
#include "Compiler/CISACodeGen/BlockMemOpAddrScalarizationPass.hpp"
#include "Compiler/CISACodeGen/SinkCommonOffsetFromGEP.h"
#include "Compiler/CISACodeGen/ConstantCoalescing.hpp"
#include "Compiler/CISACodeGen/CheckInstrTypes.hpp"
#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/CISACodeGen/GenerateBlockMemOpsPass.hpp"
#include "Compiler/CISACodeGen/GenerateFrequencyData.hpp"
#include "Compiler/CISACodeGen/FixAddrSpaceCast.h"
#include "Compiler/CISACodeGen/FixupExtractValuePair.h"
#include "Compiler/CISACodeGen/GenIRLowering.h"
#include "Compiler/CISACodeGen/GenSimplification.h"
#include "Compiler/CISACodeGen/LoopDCE.h"
#include "Compiler/CISACodeGen/LdShrink.h"
#include "Compiler/CISACodeGen/MemOpt.h"
#include "Compiler/CISACodeGen/MemOpt2.h"
#include "Compiler/CISACodeGen/SplitLoads.h"
#include "Compiler/CISACodeGen/PreRARematFlag.h"
#include "Compiler/CISACodeGen/PromoteConstantStructs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/Decompose2DBlockFuncs/Decompose2DBlockFuncs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GASResolving.h"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/LowerByValAttribute.hpp"
#include "Compiler/CISACodeGen/Simd32Profitability.hpp"
#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "Compiler/CISACodeGen/TypeDemote.h"
#include "Compiler/CISACodeGen/UniformAssumptions.hpp"
#include "Compiler/CISACodeGen/ResourceLoopUnroll.hpp"
#include "Compiler/CISACodeGen/VectorProcess.hpp"
#include "Compiler/CISACodeGen/RuntimeValueLegalizationPass.h"
#include "Compiler/CISACodeGen/LowerGEPForPrivMem.hpp"
#include "Compiler/CISACodeGen/MatchCommonKernelPatterns.hpp"
#include "Compiler/CISACodeGen/POSH_RemoveNonPositionOutput.h"
#include "Compiler/CISACodeGen/RegisterEstimator.hpp"
#include "Compiler/CISACodeGen/RegisterPressureEstimate.hpp"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"
#include "Compiler/CISACodeGen/RayTracingShaderLowering.hpp"
#include "Compiler/CISACodeGen/RayTracingStatefulPass.h"
#include "Compiler/CISACodeGen/LSCCacheOptimizationPass.h"
#include "Compiler/CISACodeGen/LSCControlsAnalysisPass.h"
#include "Compiler/ConvertMSAAPayloadTo16Bit.hpp"
#include "Compiler/MSAAInsertDiscard.hpp"
#include "Compiler/CISACodeGen/PromoteInt8Type.hpp"
#include "Compiler/CISACodeGen/PrepareLoadsStoresPass.h"
#include "Compiler/CISACodeGen/CallMergerPass.hpp"
#include "Compiler/CISACodeGen/EvaluateFreeze.hpp"
#include "Compiler/CISACodeGen/DpasScan.hpp"
#include "Compiler/CISACodeGen/FPRoundingModeCoalescing.hpp"

#include "Compiler/CISACodeGen/SLMConstProp.hpp"
#include "Compiler/Optimizer/OpenCLPasses/SplitStructurePhisPass/SplitStructurePhisPass.hpp"
#include "Compiler/Optimizer/OpenCLPasses/MergeScalarPhisPass/MergeScalarPhisPass.hpp"
#include "Compiler/Legalizer/AddRequiredMemoryFences.h"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericAddressDynamicResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericNullPtrPropagation.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryToSLM.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakConstantExpr/BreakConstantExpr.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ReplaceUnsupportedIntrinsics/ReplaceUnsupportedIntrinsics.hpp"
#include "Compiler/Optimizer/PreCompiledFuncImport.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AddressSpaceAliasAnalysis/AddressSpaceAliasAnalysis.h"
#include "Compiler/Optimizer/OpenCLPasses/StatelessToStateful/StatelessToStateful.hpp"
#include "Compiler/Optimizer/OpenCLPasses/DisableLoopUnrollOnRetry/DisableLoopUnrollOnRetry.hpp"
#include "Compiler/Optimizer/OpenCLPasses/TransformUnmaskedFunctionsPass/TransformUnmaskedFunctionsPass.h"
#include "Compiler/Optimizer/OpenCLPasses/UnreachableHandling/UnreachableHandling.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/GEPLoopStrengthReduction/GEPLoopStrengthReduction.hpp"
#include "Compiler/Optimizer/OpenCLPasses/StackOverflowDetection/StackOverflowDetection.hpp"
#include "Compiler/Optimizer/OpenCLPasses/SubGroupReductionPattern/SubGroupReductionPattern.hpp"
#include "Compiler/Optimizer/MCSOptimization.hpp"
#include "Compiler/Optimizer/GatingSimilarSamples.hpp"
#include "Compiler/Optimizer/IntDivConstantReduction.hpp"
#include "Compiler/Optimizer/IntDivRemIncrementReduction.hpp"
#include "Compiler/Optimizer/IntDivRemCombine.hpp"
#include "Compiler/Optimizer/SynchronizationObjectCoalescing.hpp"
#include "Compiler/Optimizer/BarrierControlFlowOptimization.hpp"
#include "Compiler/Optimizer/BarrierSkipOptimization.hpp"
#include "Compiler/Optimizer/RuntimeValueVectorExtractPass.h"
#include "Compiler/Optimizer/WaveShuffleIndexSinking.hpp"
#include "Compiler/Optimizer/SinkPointerConstAdd.h"
#include "Compiler/Optimizer/WaveAllJointReduction.hpp"
#include "Compiler/Optimizer/InstructionHoistingOptimization.hpp"
#include "Compiler/Optimizer/WaveBallotCSE.hpp"
#include "Compiler/MetaDataApi/PurgeMetaDataUtils.hpp"
#include "Compiler/HandleLoadStoreInstructions.hpp"
#include "Compiler/CustomSafeOptPass.hpp"
#include "Compiler/CustomUnsafeOptPass.hpp"
#include "Compiler/CustomLoopOpt.hpp"
#include "Compiler/GenUpdateCB.h"
#include "Compiler/PromoteResourceToDirectAS.h"
#include "Compiler/PromoteStatelessToBindless.h"
#include "Compiler/ShrinkArrayAlloca.h"
#if defined(_DEBUG) && !defined(ANDROID)
#include "Compiler/VerificationPass.hpp"
#endif
#include "Compiler/FixInvalidFuncNamePass.hpp"
#include "Compiler/LegalizationPass.hpp"
#include "Compiler/LowPrecisionOptPass.hpp"
#include "Compiler/WorkaroundAnalysisPass.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/DynamicTextureFolding.h"
#include "Compiler/SampleMultiversioning.hpp"
#include "Compiler/InitializePasses.h"
#include "Compiler/GenRotate.hpp"
#include "Compiler/Optimizer/Scalarizer.h"
#include "Compiler/RemoveCodeAssumptions.hpp"
#include "common/igc_regkeys.hpp"
#include "common/debug/Dump.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Pass.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/IR/Function.h>
#include <llvm/Analysis/ScopedNoAliasAA.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Transforms/IPO/FunctionAttrs.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/ADCE.h"
#include "llvmWrapper/Transforms/Scalar/CorrelatedValuePropagation.h"
#include "llvmWrapper/Transforms/Scalar/DeadStoreElimination.h"
#include "llvmWrapper/Transforms/Scalar/JumpThreading.h"
#include "llvmWrapper/Transforms/IPO/ConstantMerge.h"
#include "llvmWrapper/Transforms/IPO/SCCP.h"
#include "llvmWrapper/Transforms/IPO/GlobalDCE.h"
#include "llvmWrapper/Transforms/IPO/PostOrderFunctionAttrs.h"
#include "llvmWrapper/Transforms/Scalar/MemCpyOptimizer.h"
#include "llvmWrapper/Transforms/Scalar/LoopDeletion.h"
#include "llvmWrapper/Transforms/Scalar/LoopLoadElimination.h"
#include "llvmWrapper/Transforms/Scalar/SCCP.h"
#include "llvmWrapper/Transforms/Scalar/LoopUnrollPass.h"
#include "llvmWrapper/Transforms/Scalar/LICM.h"
#include "llvmWrapper/Transforms/Scalar/IndVarSimplify.h"

#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CISACodeGen/CoalescingEngine.hpp"
#include "Compiler/GenTTI.h"
#include "Compiler/GenRotate.hpp"
#include "Compiler/SampleCmpToDiscard.h"
#include "Compiler/Optimizer/IGCInstCombiner/IGCInstructionCombining.hpp"
#include "Compiler/Optimizer/HoistConvOpToDom.hpp"
#include "Compiler/Optimizer/PromoteToPredicatedMemoryAccess.hpp"
#include "AdaptorCommon/RayTracing/RayTracingPasses.hpp"
#include "AdaptorCommon/RayTracing/RayTracingAddressSpaceAliasAnalysis.h"
#include "AdaptorCommon/RayTracing/API/RayDispatchGlobalData.h"
#include "Compiler/SamplerPerfOptPass.hpp"
#include "Compiler/CISACodeGen/HalfPromotion.h"
#include "Compiler/CISACodeGen/CapLoopIterationsPass.h"
#include "Compiler/CISACodeGen/AnnotateUniformAllocas.h"
#include "Probe/Assertion.h"
#include "Compiler/CISACodeGen/PartialEmuI64OpsPass.h"
#include "Compiler/TranslateToProgrammableOffsetsPass.hpp"
#include "Compiler/CISACodeGen/RemoveLoopDependency.hpp"

#include <filesystem>

/***********************************************************************************
This file contains the generic code generation functions for all the shaders
The class CShader is inherited for each specific type of shaders to add specific
information
************************************************************************************/

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace IGC::Debug;

namespace IGC {
const int LOOP_ROTATION_HEADER_INST_THRESHOLD = 32;
const int LOOP_NUM_THRESHOLD = 2000;
const int LOOP_INST_THRESHOLD = 65000;
const int INST_THRESHOLD = 80000;


void AddAnalysisPasses(CodeGenContext &ctx, IGCPassManager &mpm) {
  COMPILER_TIME_START(&ctx, TIME_CG_Add_Analysis_Passes);

  bool isOptDisabled = ctx.getModuleMetaData()->compOpt.OptDisable;
  TODO("remove the following once all IGC passes are registered to PassRegistery in their constructor")
  initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());
  initializeCastToGASInfoPass(*PassRegistry::getPassRegistry());

  mpm.add(createTimeStatsCounterPass(&ctx, TIME_CG_Analysis, STATS_COUNTER_START));

  // transform pull constants and inputs into push constants and inputs
  mpm.add(new PushAnalysis());
  mpm.add(CreateSampleCmpToDiscardPass());

  if (!isOptDisabled) {
    mpm.add(llvm::createDeadCodeEliminationPass());
  }

  // The 1st thing we do when getting into the IGC middle end is to split critical-edges:
  // PushAnalysis requires WIAnalysis
  // WIAnalysis requires dominator and post-dominator analysis
  // WIAnalysis also requires BreakCriticalEdge because it assumes that
  // potential phi-moves will be placed at those blocks
  mpm.add(llvm::createBreakCriticalEdgesPass());



  if (!isOptDisabled && IGC_IS_FLAG_DISABLED(DisableMemOpt2)) {
    if (ctx.m_DriverInfo.WAEnableMemOpt2ForOCL())
      mpm.add(createMemOpt2Pass(16));
  }

  if (!isOptDisabled) {
    mpm.add(createSplitLoadsPass());
  }

  if (IGC_IS_FLAG_ENABLED(EnableScalarPhisMerger) && ctx.type == ShaderType::OPENCL_SHADER) {
    mpm.add(new MergeScalarPhisPass());
  }

  // only limited code-sinking to several shader-type
  // vs input has the URB-reuse issue to be resolved.
  // Also need to understand the performance benefit better.
  if (!isOptDisabled) {
    mpm.add(new HoistCongruentPHI());
    mpm.add(new CodeSinking());
    if ((IGC_IS_FLAG_DISABLED(DisableLoopSink) || IGC_IS_FLAG_ENABLED(ForceLoopSink)) &&
        ctx.type == ShaderType::OPENCL_SHADER && ctx.m_instrTypes.numOfLoop > 0 &&
        ctx.m_instrTypes.numInsts >= IGC_GET_FLAG_VALUE(CodeLoopSinkingMinSize)) {
      mpm.add(new CodeLoopSinking());
    }
    if (IGC_IS_FLAG_DISABLED(DisableCodeScheduling) && (ctx.type == ShaderType::OPENCL_SHADER) &&
        (ctx.platform.isCoreChildOf(IGFX_XE_HPC_CORE) || ctx.platform.isCoreChildOf(IGFX_XE2_HPG_CORE))) {
      if (IGC_IS_FLAG_DISABLED(CodeSchedulingOnlyRecompilation) || ctx.m_retryManager->AllowCodeScheduling()) {
        mpm.add(new CodeScheduling());
      }
    }
  }


  // Run flag re-materialization if it's beneficial.
  if (ctx.m_DriverInfo.benefitFromPreRARematFlag() && IGC_IS_FLAG_ENABLED(EnablePreRARematFlag)) {
    mpm.add(createPreRARematFlagPass());
  }
  // Peephole framework for generic type legalization
  mpm.add(new Legalizer::PeepholeTypeLegalizer());
  if (IGC_IS_FLAG_ENABLED(ForcePromoteI8) ||
      (IGC_IS_FLAG_ENABLED(EnablePromoteI8) && !ctx.platform.supportByteALUOperation())) {
    mpm.add(createPromoteInt8TypePass());
  }

  // need this before WIAnalysis:
  // insert phi to prevent changing of WIAnalysis result by later code-motion
  mpm.add(llvm::createLCSSAPass());
  // Fixup extract value pairs.
  mpm.add(createExtractValuePairFixupPass());

  if (IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions) && IGC_IS_FLAG_ENABLED(LateInlineUnmaskedFunc)) {
    mpm.add(new InlineUnmaskedFunctionsPass());
    // Newly created memcpy intrinsic are lowered
    mpm.add(createReplaceUnsupportedIntrinsicsPass());
    // Split complex constant expression into 2 simple ones
    mpm.add(new BreakConstantExpr());
    // Expand newly created allocas
    mpm.add(createSROAPass());
    // Run legalization pass to expand non-supported instructions
    // like shufflevector. The code below is just copied and
    // pasted as is.
    bool preserveNan = !ctx.getCompilerOption().NoNaNs;
    mpm.add(new Legalization(preserveNan));
    // Some clean up passes.
    mpm.add(llvm::createEarlyCSEPass());
    mpm.add(new BreakConstantExpr());
    mpm.add(llvm::createCFGSimplificationPass());
    mpm.add(createDeadCodeEliminationPass());
    // Create functions groups after unmasked functions inlining
    mpm.add(createGenXCodeGenModulePass());
    // Allocate non-primitive allocas. These peace of code is copied
    if (ctx.m_instrTypes.hasNonPrimitiveAlloca) {
      mpm.add(createBreakCriticalEdgesPass());
      mpm.add(createAnnotateUniformAllocasPass());

      if (IGC_IS_FLAG_DISABLED(DisablePromotePrivMem) &&
          !isOptDisabledForModule(ctx.getModuleMetaData(), IGCOpts::LowerGEPForPrivMemPass)) {
        mpm.add(createPromotePrivateArrayToReg());
        mpm.add(createCFGSimplificationPass());
      }
    }
    mpm.add(createPromoteMemoryToRegisterPass());
    if (IGC_IS_FLAG_DISABLED(DisableMergeAllocasPrivateMemory) && ctx.type == ShaderType::OPENCL_SHADER) {
      mpm.add(createMergeAllocasOCL());
    }
    if (ctx.type == ShaderType::OPENCL_SHADER && !isOptDisabled && IGC_IS_FLAG_ENABLED(EnableExplicitCopyForByVal)) {
      mpm.add(new LowerByValAttribute());
      mpm.add(createReplaceUnsupportedIntrinsicsPass());
    }
    // Resolving private memory allocas
    mpm.add(CreatePrivateMemoryResolution());
  }

  // Reorder FP instructions to minimize number of times rounding mode is switched.
  mpm.add(createFPRoundingModeCoalescingPass());

  // Expected to be the last ModulePass before EmitPass at this point.
  // (Shall be after GenXCodeGenModulePass.)
  //
  // It uses CastToGASAnalysis and invalidates it by taking its result away.
  // This means that after this point, no CastToGASAnalysis will be used,
  // and the info should be accessed via CastToGASInfo immutable pass.
  if (ctx.type == ShaderType::OPENCL_SHADER) {
    mpm.add(new CastToGASInfoWrapper());
  }

  // Evaluates LLVM 10+ freeze instructions so EmitPass does not need to handle them.
  // The pass first occurs during optimization, however new freeze instructions could
  // have been inserted since.
  mpm.add(createEvaluateFreezePass());

  // clean up constexpressions after EarlyCSE
  mpm.add(new BreakConstantExpr());

  // This is for dumping register pressure info
  if (IGC_IS_FLAG_ENABLED(ForceRPE)) {
    mpm.add(new RegisterEstimator());
  }

  mpm.add(createFixInvalidFuncNamePass());

  // collect stats after all the optimization. This info can be dumped to the cos file
  mpm.add(new CheckInstrTypes(true, false));

  if ((IGC_GET_FLAG_VALUE(StaticProfileGuidedSpillCostAnalysis) & FrequencyDataDS::PGSS_IGC_GEN) != 0) {
    mpm.add(createGenerateFrequencyDataPass());
  }

  if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
    mpm.add(new StackOverflowDetectionPass(StackOverflowDetectionPass::Mode::RemoveDummyCalls));
  }

  //
  // Generally, passes that change IR should be prior to this place!
  //

  mpm.add(new DpasScan());

  mpm.add(new MatchCommonKernelPatterns());

  // let CleanPHINode be right before Layout
  mpm.add(createCleanPHINodePass());
  if (IGC_IS_FLAG_SET(DumpRegPressureEstimate))
    mpm.add(new IGCRegisterPressurePrinter("final"));

  // save RPE results in metadata
  if (ctx.type == ShaderType::OPENCL_SHADER) {
    mpm.add(new IGCRegisterPressurePublisher());
  }

  // Let Layout be the last pass before Emit Pass
  mpm.add(new Layout());
  if (IGC_IS_FLAG_ENABLED(EnableDropTargetBBs)) {
    mpm.add(new DropTargetBBs());
  }


  mpm.add(createTimeStatsCounterPass(&ctx, TIME_CG_Analysis, STATS_COUNTER_END));

  COMPILER_TIME_END(&ctx, TIME_CG_Add_Analysis_Passes);
} // AddAnalysisPasses

static void UpdateInstTypeHint(CodeGenContext &ctx) {
  // WA: save original values as preRA heuristic is based on those
  // we need to fix the preRA pass heuristic or get rid of preRA pass altogether
  unsigned int numBB = ctx.m_instrTypes.numBB;
  unsigned int numSample = ctx.m_instrTypes.numSample;
  unsigned int numInsts = ctx.m_instrTypes.numInsts;
  bool hasUnmaskedRegion = ctx.m_instrTypes.hasUnmaskedRegion;
  IGCPassManager mpm(&ctx, "UpdateOptPre");
  mpm.add(new CodeGenContextWrapper(&ctx));
  mpm.add(new BreakConstantExpr());
  mpm.add(new CheckInstrTypes(false, false));
  mpm.run(*ctx.getModule());
  ctx.m_instrTypes.numBB = numBB;
  ctx.m_instrTypes.numSample = numSample;
  ctx.m_instrTypes.numInsts = numInsts;
  ctx.m_instrTypes.hasLoadStore = true;
  ctx.m_instrTypes.hasUnmaskedRegion = hasUnmaskedRegion;
}

// forward declaration
llvm::ModulePass *createPruneUnusedArgumentsPass();

void AddLegalizationPasses(CodeGenContext &ctx, IGCPassManager &mpm, PSSignature *pSignature) {
  COMPILER_TIME_START(&ctx, TIME_CG_Add_Legalization_Passes);

  mpm.add(createTimeStatsCounterPass(&ctx, TIME_CG_Legalization, STATS_COUNTER_START));

  // update type of instructions to know what passes are needed.
  UpdateInstTypeHint(ctx);
  // check again after full inlining if subroutines are still present
  ctx.CheckEnableSubroutine(*ctx.getModule());

  MetaDataUtils *pMdUtils = ctx.getMetaDataUtils();
  bool isOptDisabled = ctx.getModuleMetaData()->compOpt.OptDisable;
  bool fastCompile = ctx.getModuleMetaData()->compOpt.FastCompilation;
  bool highAllocaPressure = ctx.m_instrTypes.numAllocaInsts > IGC_GET_FLAG_VALUE(AllocaRAPressureThreshold);
  bool isPotentialHPCKernel = (ctx.m_instrTypes.numInsts > IGC_GET_FLAG_VALUE(HPCInstNumThreshold)) ||
                              (ctx.m_instrTypes.numGlobalInsts > IGC_GET_FLAG_VALUE(HPCGlobalInstNumThreshold)) ||
                              IGC_GET_FLAG_VALUE(HPCFastCompilation);
  highAllocaPressure = IGC_GET_FLAG_VALUE(DisableFastRAWA) ? false : highAllocaPressure;
  isPotentialHPCKernel = IGC_GET_FLAG_VALUE(DisableFastRAWA) ? false : isPotentialHPCKernel;

  if (highAllocaPressure || isPotentialHPCKernel) {
    IGC_SET_FLAG_VALUE(FastCompileRA, 1);
    IGC_SET_FLAG_VALUE(HybridRAWithSpill, 1);
  }
  // In case of presence of Unmasked regions disable loop invariant motion after
  // Unmasked functions are inlined at the end of optimization phase
  if (IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions) && IGC_IS_FLAG_DISABLED(LateInlineUnmaskedFunc) &&
      ctx.m_instrTypes.hasUnmaskedRegion) {
    IGC_SET_FLAG_VALUE(allowLICM, false);
  }
  bool disableConvergentInstructionsHoisting =
      ctx.m_DriverInfo.DisableConvergentInstructionsHoisting() && ctx.m_instrTypes.numWaveIntrinsics > 0;
  if (disableConvergentInstructionsHoisting || IGC_IS_FLAG_ENABLED(ForceAllPrivateMemoryToSLM) ||
      IGC_IS_FLAG_ENABLED(ForcePrivateMemoryToSLMOnBuffers)) {
    TargetIRAnalysis GenTTgetIIRAnalysis([&](const Function &F) {
      GenIntrinsicsTTIImpl GTTI(&ctx);
      return TargetTransformInfo(GTTI);
    });
    mpm.add(new TargetTransformInfoWrapperPass(std::move(GenTTgetIIRAnalysis)));
  }

  // Disable all target library functions.
  // right now we don't support any standard function in the code gen
  // maybe we want to support some at some point to take advantage of LLVM optimizations
  TargetLibraryInfoImpl TLI;
  TLI.disableAllFunctions();
  mpm.add(new llvm::TargetLibraryInfoWrapperPass(TLI));

  // Add Metadata API immutable pass
  mpm.add(new MetaDataUtilsWrapper(pMdUtils, ctx.getModuleMetaData()));
  // Add CodeGen Context Wrapper immutable pass
  mpm.add(new CodeGenContextWrapper(&ctx));
  // Add alias analysis pass
  mpm.add(createAddressSpaceAAWrapperPass());

  if (ctx.type == ShaderType::RAYTRACING_SHADER || ctx.hasSyncRTCalls()) {
    if (IGC_IS_FLAG_DISABLED(DisableRTAliasAnalysis))
      mpm.add(createRayTracingAddressSpaceAAWrapperPass());
  }

  mpm.add(createIGCExternalAAWrapper());
  mpm.add(createScopedNoAliasAAWrapperPass());

  TODO("remove the following once all IGC passes are registered to PassRegistery in their constructor")
  initializeWIAnalysisPass(*PassRegistry::getPassRegistry());
  initializeSimd32ProfitabilityAnalysisPass(*PassRegistry::getPassRegistry());
  initializeGenXFunctionGroupAnalysisPass(*PassRegistry::getPassRegistry());


  if (ctx.m_threadCombiningOptDone) {
    mpm.add(createLoopCanonicalization());
    mpm.add(IGCLLVM::createLegacyWrappedLoopDeletionPass());
    mpm.add(llvm::createBreakCriticalEdgesPass());
    mpm.add(llvm::createLoopRotatePass(LOOP_ROTATION_HEADER_INST_THRESHOLD));
    mpm.add(llvm::createLowerSwitchPass());

    int LoopUnrollThreshold = ctx.m_DriverInfo.GetLoopUnrollThreshold();

    if (LoopUnrollThreshold > 0 && (ctx.m_tempCount < 64)) {
      mpm.add(IGCLLVM::createLegacyWrappedLoopUnrollPass(2, false, false, LoopUnrollThreshold, -1, 1, -1, -1, -1));
    }

    mpm.add(createBarrierNoopPass());

    bool AllowLICM =
        IGC_IS_FLAG_SET(allowLICM) ? IGC_IS_FLAG_ENABLED(allowLICM) : ctx.getModuleMetaData()->compOpt.AllowLICM;

    if (ctx.m_retryManager->AllowLICM() && AllowLICM) {
      mpm.add(createSpecialCasesDisableLICM());
      mpm.add(IGCLLVM::createLegacyWrappedLICMPass(100, 500, true));
    }
    mpm.add(llvm::createLoopSimplifyPass());
  }

  // Lower/Resolve OCL inlined constants.
  if (ctx.m_DriverInfo.NeedLoweringInlinedConstants()) {
    // Run additional constant breaking which is assumed by the constant
    // resolver.
    mpm.add(new BreakConstantExpr());
    mpm.add(new ProgramScopeConstantResolution());
  }

  // This is the condition that double emulation is used.
  ctx.checkDPEmulationEnabled();

  bool hasDPDivSqrtEmu =
      !ctx.platform.hasNoFP64Inst() && !ctx.platform.hasCorrectlyRoundedMacros() && ctx.m_DriverInfo.NeedFP64DivSqrt();
  uint32_t theEmuKind = (ctx.m_hasDPEmu ? EmuKind::EMU_DP : 0);
  theEmuKind |= (hasDPDivSqrtEmu ? EmuKind::EMU_DP_DIV_SQRT : 0);
  theEmuKind |= (ctx.m_hasDPConvEmu ? EmuKind::EMU_DP_CONV : 0);
  theEmuKind |= (ctx.m_DriverInfo.NeedI64BitDivRem() ? EmuKind::EMU_I64DIVREM : 0);
  theEmuKind |= (ctx.m_DriverInfo.NeedFP64toFP16Conv() && IGC_IS_FLAG_DISABLED(ForceDisableDPToHFConvEmu)
                     ? EmuKind::EMU_FP64_FP16_CONV
                     : 0);
  theEmuKind |= ((IGC_IS_FLAG_ENABLED(ForceSPDivEmulation) ||
                  (ctx.m_DriverInfo.NeedIEEESPDiv() && !ctx.platform.hasCorrectlyRoundedMacros()))
                     ? EmuKind::EMU_SP_DIV
                     : 0);
  if (ctx.platform.preferFP32IntDivRemEmu() && IGC_IS_FLAG_DISABLED(Force32BitIntDivRemEmu)) {
    // Prefer using FP32 emulation even though DP support is available
    theEmuKind |= EmuKind::EMU_I32DIVREM_SP;
  } else if (!ctx.platform.hasNoFP64Inst() &&
             (IGC_IS_FLAG_ENABLED(Force32BitIntDivRemEmu) || ctx.getCompilerOption().ForceInt32DivRemEmu ||
              (ctx.platform.Enable32BitIntDivRemEmu() && !ctx.getCompilerOption().ForceInt32DivRemEmuSP &&
               IGC_IS_FLAG_DISABLED(Force32BitIntDivRemEmuSP)))) {
    // Use DP (and float) opeations to emulate int32 div/rem
    theEmuKind |= EmuKind::EMU_I32DIVREM;
  } else if (ctx.platform.Enable32BitIntDivRemEmu() || ctx.getCompilerOption().ForceInt32DivRemEmuSP ||
             IGC_IS_FLAG_ENABLED(Force32BitIntDivRemEmuSP)) {
    // Use SP floating operations to emulate int32 div/rem
    theEmuKind |= EmuKind::EMU_I32DIVREM_SP;
  }

  if (IGC_IS_FLAG_ENABLED(RayTracingKeepUDivRemWA)) {
    theEmuKind &= ~EmuKind::EMU_I32DIVREM;
    theEmuKind &= ~EmuKind::EMU_I32DIVREM_SP;
  }

  if (theEmuKind > 0 || IGC_IS_FLAG_ENABLED(EnableTestIGCBuiltin)) {
    // Need to break constant expr as PreCompiledFuncImport does not handle it.
    mpm.add(new BreakConstantExpr());
    mpm.add(new PreCompiledFuncImport(&ctx, theEmuKind));
    mpm.add(createAlwaysInlinerLegacyPass());

    // Using DCE here as AlwaysInliner does not completely remove dead functions.
    // Once AlwaysInliner can delete all of them, this DCE is no longer needed.
    // mpm.add(createDeadCodeEliminationPass());
    //
    // DCE doesn't remove dead control flow; ADCE does (currently)
    // otherwise you'd have to call createCFGSimplificationPass and DCE
    // iteratively e.g..
    mpm.add(IGCLLVM::createLegacyWrappedADCEPass());
    // TODO: we probably should be running other passes on the result

    if (!IGC::ForceAlwaysInline(&ctx)) {
      mpm.add(new PurgeMetaDataUtils());
    }
  }

  // Find rotate pattern.
  //   Invoked after DP emulation so that it'd handle emulation functions.
  if (ctx.platform.supportRotateInstruction()) {
    mpm.add(createGenRotatePass());
  }

  mpm.add(createReplaceUnsupportedIntrinsicsPass());

  if (IGC_IS_FLAG_DISABLED(DisablePromoteToDirectAS) && !ctx.getModuleMetaData()->compOpt.IsLibraryCompilation) {
    // Promotes indirect resource access to direct
    mpm.add(new BreakConstantExpr());
    mpm.add(new PromoteResourceToDirectAS());
  }

  if (!isOptDisabled) {
    mpm.add(createPruneUnusedArgumentsPass());
  }

  if (ctx.m_instrTypes.hasReadOnlyArray) {
    mpm.add(createDeadCodeEliminationPass());
    mpm.add(createSROAPass());
  }

  if (ctx.m_instrTypes.hasGenericAddressSpacePointers) {
    if (IGC_IS_FLAG_ENABLED(EnableGASResolver)) {
      mpm.add(createSROAPass());
      mpm.add(createFixAddrSpaceCastPass());
      mpm.add(createResolveGASPass());
    }
    mpm.add(createGenericAddressDynamicResolutionPass());
    mpm.add(createDeadCodeEliminationPass());
    mpm.add(createGenericNullPtrPropagationPass());
  }

  // Resolve the Private memory to register pass
  if (!isOptDisabled) {
    // In case of late inlining of Unmasked function allocate non
    // primitive Allocas after inlining is done. Otherwise there
    // is possibility RegAlloc cannot allocate registers for all
    // virtual registers. This piece of code is copied at the place
    // where inlining is done.
    if (ctx.m_instrTypes.hasNonPrimitiveAlloca &&
        !(IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions) && IGC_IS_FLAG_ENABLED(LateInlineUnmaskedFunc))) {
      mpm.add(createBreakCriticalEdgesPass());
      mpm.add(createAnnotateUniformAllocasPass());

      if (IGC_IS_FLAG_DISABLED(DisablePromotePrivMem) &&
          !isOptDisabledForModule(ctx.getModuleMetaData(), IGCOpts::LowerGEPForPrivMemPass)) {
        mpm.add(createPromotePrivateArrayToReg());
        mpm.add(createCFGSimplificationPass());
      }
    }
    mpm.add(createPromoteMemoryToRegisterPass());
  } else {
    if (IGC_IS_FLAG_ENABLED(AllowMem2Reg))
      mpm.add(createPromoteMemoryToRegisterPass());
  }

  if (ctx.type == ShaderType::OPENCL_SHADER || ctx.type == ShaderType::COMPUTE_SHADER) {
    if (IGC_IS_FLAG_ENABLED(ForceAllPrivateMemoryToSLM)) {
      mpm.add(new PrivateMemoryToSLM(IGC_IS_FLAG_ENABLED(EnableOptReportPrivateMemoryToSLM)));
      mpm.add(createInferAddressSpacesPass());
    } else if (IGC_IS_FLAG_ENABLED(ForcePrivateMemoryToSLMOnBuffers)) {
      std::string forcedBuffers(IGC_GET_REGKEYSTRING(ForcePrivateMemoryToSLMOnBuffers));

      mpm.add(new PrivateMemoryToSLM(std::move(forcedBuffers), IGC_IS_FLAG_ENABLED(EnableOptReportPrivateMemoryToSLM)));
      mpm.add(createInferAddressSpacesPass());
    }
  }

  if (ctx.m_instrTypes.numOfLoop) {
    // need to run loop simplify to canonicalize loop and merge latches
    mpm.add(createLoopCanonicalization());
    mpm.add(createLoopSimplifyPass());
    if (!IGC_IS_FLAG_ENABLED(DisableLoopSplitWidePHIs))
      mpm.add(createLoopSplitWidePHIs());
  }

  if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
    // Cleanup stack overflow detection calls if necessary.
    mpm.add(new StackOverflowDetectionPass(StackOverflowDetectionPass::Mode::AnalyzeAndCleanup));
  }

  if (ctx.enableFunctionCall() || ctx.type == ShaderType::RAYTRACING_SHADER) {
    // Sort functions if subroutine/indirect fcall is enabled.
    mpm.add(IGCLLVM::createLegacyWrappedGlobalDCEPass());
    mpm.add(new PurgeMetaDataUtils());
    mpm.add(createGenXCodeGenModulePass());
  }

  // Remove all uses of implicit arg instrinsics after inlining by lowering them to kernel args
  mpm.add(new LowerImplicitArgIntrinsics());

  // Resolving private memory allocas
  // In case of late inlining of Unmasked function postpone memory
  // resolution till inlining is done as during inlining new Allocas
  // are created.
  if (!(IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions) && IGC_IS_FLAG_ENABLED(LateInlineUnmaskedFunc))) {
    if (IGC_IS_FLAG_DISABLED(DisableMergeAllocasPrivateMemory) && ctx.type == ShaderType::OPENCL_SHADER) {
      mpm.add(createMergeAllocasOCL());
    }
    if (ctx.type == ShaderType::OPENCL_SHADER && !isOptDisabled && IGC_IS_FLAG_ENABLED(EnableExplicitCopyForByVal)) {
      mpm.add(new LowerByValAttribute());
      mpm.add(createReplaceUnsupportedIntrinsicsPass());
    }
    mpm.add(CreatePrivateMemoryResolution());
  }
  // Should help MemOpt pass to merge more loads
  mpm.add(createSinkCommonOffsetFromGEPPass());

  // Run MemOpt
  if (!isOptDisabled && ctx.m_instrTypes.hasLoadStore && IGC_IS_FLAG_DISABLED(DisableMemOpt) &&
      !ctx.getModuleMetaData()->disableMemOptforNegativeOffsetLoads) {

    if ((ctx.type == ShaderType::RAYTRACING_SHADER || ctx.hasSyncRTCalls()) &&
        IGC_IS_FLAG_DISABLED(DisablePrepareLoadsStores)) {
      mpm.add(createPrepareLoadsStoresPass());
    }

    // run AdvMemOpt and MemOPt back-to-back so that we only
    // need to run WIAnalysis once
    if (IGC_IS_FLAG_ENABLED(EnableAdvMemOpt))
      mpm.add(createAdvMemOptPass());

    if (doLdStCombine(&ctx)) {
      // Once it is stable, no split 64bit store/load anymore.
      mpm.add(createLdStCombinePass());
    }

    bool AllowNegativeSymPtrsForLoad =
        ctx.type == ShaderType::OPENCL_SHADER;

    bool AllowVector8LoadStore =
        IGC_IS_FLAG_ENABLED(EnableVector8LoadStore) ||
        ((ctx.type == ShaderType::RAYTRACING_SHADER || ctx.hasSyncRTCalls()) && ctx.platform.supports8DWLSCMessage());

    mpm.add(createMemOptPass(AllowNegativeSymPtrsForLoad, AllowVector8LoadStore));

    if ((ctx.type == ShaderType::RAYTRACING_SHADER || ctx.hasSyncRTCalls()) &&
        IGC_IS_FLAG_ENABLED(EnableLSCCacheOptimization)) {
      // Optimize store instructions for utilizing the LSC-L1 cache.
      // This only runs for shaders with raytracing functionality.
      mpm.add(createLSCCacheOptimizationPass());
    }

    mpm.add(createIGCInstructionCombiningPass());
  }



    if (ctx.hasSyncRTCalls()) {
      mpm.add(createRaytracingStatefulPass());
    }

  if (ctx.type == ShaderType::OPENCL_SHADER &&
      static_cast<OpenCLProgramContext &>(ctx).m_InternalOptions.PromoteStatelessToBindless) {
    if (static_cast<OpenCLProgramContext &>(ctx).m_InternalOptions.UseBindlessLegacyMode) {
      mpm.add(new PromoteStatelessToBindless());
    } else if (!ctx.platform.hasEfficient64bEnabled() &&
               !ctx.getModuleMetaData()->compOpt.GreaterThan4GBBufferRequired && !isOptDisabled) {
      // Advanced bindless mode used by the regular OpenCL compilation path
      mpm.add(new StatelessToStateful(TargetAddressing::BINDLESS));
    }
  }

  if (!isOptDisabled && ctx.useStatelessToStateful()) {
    mpm.add(new StatelessToStateful(TargetAddressing::BINDFUL));
  }

  // Light cleanup for subroutines after cloning. Note that the constant
  // propogation order is reversed, compared to the opt sequence in
  // OptimizeIR. There is a substantial gain with CFG simplification after
  // interprocedural constant propagation.
  if (ctx.m_enableSubroutine && !isOptDisabled) {
    mpm.add(createPruneUnusedArgumentsPass());

    const bool allowIPConstProp = !ctx.m_hasStackCalls && IGC_IS_FLAG_DISABLED(DisableIPConstantPropagation);

    if (allowIPConstProp) {
      mpm.add(IGCLLVM::createLegacyWrappedIPSCCPPass());
    }
    mpm.add(createDeadCodeEliminationPass());
    mpm.add(createCFGSimplificationPass());
  }
  // Since we don't support switch statements, switch lowering is needed after the last CFG simplication
  mpm.add(llvm::createLowerSwitchPass());

  // This pass can create constant expression
  if (ctx.m_DriverInfo.HasDoubleLoadStore()) {
    mpm.add(new HandleLoadStoreInstructions());
  }

  // Split big vector & 3-element load/store, etc.
  mpm.add(createVectorPreProcessPass());

  // Create Gen IR lowering.
  //   To replace SLM pointer if they are constants, break constants first.
  if (ctx.m_instrTypes.hasLocalLoadStore) {
    mpm.add(new BreakConstantExpr());
  }

  bool KeepGEPs;
  // In case of late inlining of Unmasked function postpone memory
  // resolution till inlining is done as during inlining new Allocas
  // are created.
  if (IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions) && IGC_IS_FLAG_ENABLED(LateInlineUnmaskedFunc)) {
    KeepGEPs = true;
  } else {
    KeepGEPs = false;
  }
  mpm.add(createGenIRLowerPass());

  if (KeepGEPs) {
    mpm.add(createSeparateConstOffsetFromGEPPass());
  } else {
    // Also break and lower GEP constexpr.
    mpm.add(new BreakConstantExpr());
    mpm.add(createGEPLoweringPass());
  }

  mpm.add(new WorkaroundAnalysis());

  if (!isOptDisabled) {
    // Removing code assumptions can enable some InstructionCombining optimizations.
    // Last instruction combining pass needs to be before Legalization pass, as it can produce illegal instructions.
    mpm.add(new RemoveCodeAssumptions());
    mpm.add(createIGCInstructionCombiningPass());
    if (ctx.platform.doIntegerMad() && ctx.m_DriverInfo.EnableIntegerMad()) {
      mpm.add(createCanonicalizeMulAddPass());
    }
    mpm.add(new GenSpecificPattern());
    // Cases with DPDivSqrtEmu grow significantly.
    // We can disable EarlyCSE when m_hasDPDivSqrtEmu is true,
    // what causes the values will have shorter lifetime and we can avoid spills.
    if (!fastCompile && !highAllocaPressure && !isPotentialHPCKernel && !ctx.m_hasDPDivSqrtEmu) {
      mpm.add(createEarlyCSEPass());
    } else if (highAllocaPressure || isPotentialHPCKernel) {
      mpm.add(createSinkingPass());
    }

    bool AllowLICM =
        IGC_IS_FLAG_SET(allowLICM) ? IGC_IS_FLAG_ENABLED(allowLICM) : ctx.getModuleMetaData()->compOpt.AllowLICM;

    if (!fastCompile && !highAllocaPressure && !isPotentialHPCKernel && AllowLICM && ctx.m_retryManager->AllowLICM()) {
      mpm.add(createSpecialCasesDisableLICM());
      mpm.add(IGCLLVM::createLegacyWrappedLICMPass(100, 500, true));
      mpm.add(llvm::createEarlyCSEPass());
    }
    mpm.add(IGCLLVM::createLegacyWrappedADCEPass());
    // As DPC++ FE apply LICM we cannot reduce register pressure just
    // by turning off LICM at IGC in some cases so apply sinking address arithmetic
    if ((IGC_IS_FLAG_ENABLED(ForceAddressArithSinking) ||
         !isOptDisabledForModule(ctx.getModuleMetaData(), IGCOpts::AddressArithmeticSinkingPass)) &&
        ctx.type == ShaderType::OPENCL_SHADER) {
      mpm.add(new AddressArithmeticSinking());
    }
  }

  // Enabling half promotion AIL for compute shaders only at this point.
  // If needed ctx.type check can be removed to apply for all shader types
  if (IGC_IS_FLAG_ENABLED(ForceHalfPromotion) ||
      (ctx.getModuleMetaData()->compOpt.WaForceHalfPromotionComputeShader && ctx.type == ShaderType::COMPUTE_SHADER) ||
      (ctx.getModuleMetaData()->compOpt.WaForceHalfPromotionPixelVertexShader &&
       (ctx.type == ShaderType::PIXEL_SHADER || ctx.type == ShaderType::VERTEX_SHADER)) ||
      (!ctx.platform.supportFP16() && IGC_IS_FLAG_ENABLED(EnableHalfPromotion))) {
    mpm.add(new HalfPromotion());
    if (IGC_IS_FLAG_ENABLED(EnableGVN)) {
      mpm.add(createGVNPass());
    }
    mpm.add(createDeadCodeEliminationPass());
  }

  if (IGC_IS_FLAG_ENABLED(ForceNoInfiniteLoops)) {
    mpm.add(createLoopSimplifyPass());
    mpm.add(new CapLoopIterations(UINT_MAX));
  }

  // Run address remat after GVN as it may hoist address calculations and
  // create PHI nodes with addresses.
  if (IGC_IS_FLAG_ENABLED(RematEnable) ||
      (ctx.m_retryManager->AllowCloneAddressArithmetic() && ctx.type == ShaderType::OPENCL_SHADER)) {

    if (IGC_GET_FLAG_VALUE(RematInstCombineBefore))
      mpm.add(createIGCInstructionCombiningPass());
    // TODO: This is a workaround that helps to reduce amount of instructions for clone address arithmetic
    // it helps with chain of instructions like this
    // %remat12 = add i64 %baseArith, 100780848
    // %remat13 = add i64 %remat12, %basePtr
    // %remat14 = add i64 %remat13, %offsetI
    // %remat15 = add i64 %remat14, %offsetJ
    // load ...
    // ....
    // %remat21 = add i64 %baseArith, 201561696
    // %remat22 = add i64 %remat21, %basePtr
    // %remat23 = add i64 %remat22, %offsetI
    // %remat24 = add i64 %remat23, %offsetJ
    // load ...
    // we can compress this chain of instruction into one "add" for each "load"
    // this is achieved by combining reassoc + cse 3 times (each pair hoists one add)
    // it should be substituted for general pass when it's implemented
    //
    // Now it's accessible through flag, for testing purposes
    if (IGC_GET_FLAG_VALUE(RematReassocBefore)) {
      mpm.add(llvm::createReassociatePass());
      mpm.add(llvm::createEarlyCSEPass());
      mpm.add(llvm::createReassociatePass());
      mpm.add(llvm::createEarlyCSEPass());
      mpm.add(llvm::createReassociatePass());
      mpm.add(llvm::createEarlyCSEPass());
    }

    mpm.add(createCloneAddressArithmeticPass());
    // cloneAddressArithmetic leaves old instructions unnecessary
    // dce pass helps to clean that up
    mpm.add(createDeadCodeEliminationPass());
    if (IGC_IS_FLAG_SET(DumpRegPressureEstimate))
      mpm.add(new IGCRegisterPressurePrinter("after_remat"));
  } else if (ctx.m_retryManager->AllowCloneAddressArithmetic() && IGC_GET_FLAG_VALUE(RematOptionsForRetry) ||
             ctx.platform.supportsVRT() && IGC_GET_FLAG_VALUE(RematOptionsForVRT)) {

    if (IGC_GET_FLAG_VALUE(RematInstCombineBefore))
      mpm.add(createIGCInstructionCombiningPass());

    // see comment above
    if (IGC_GET_FLAG_VALUE(RematReassocBefore)) {
      mpm.add(llvm::createReassociatePass());
      mpm.add(llvm::createEarlyCSEPass());
      mpm.add(llvm::createReassociatePass());
      mpm.add(llvm::createEarlyCSEPass());
      mpm.add(llvm::createReassociatePass());
      mpm.add(llvm::createEarlyCSEPass());
    }

    // if both retry and VRT checks go through, retry is more important
    auto rematOptions = ctx.m_retryManager->AllowCloneAddressArithmetic() && IGC_GET_FLAG_VALUE(RematOptionsForRetry)
                            ? static_cast<IGC::REMAT_OPTIONS>(IGC_GET_FLAG_VALUE(RematOptionsForRetry))
                            : static_cast<IGC::REMAT_OPTIONS>(IGC_GET_FLAG_VALUE(RematOptionsForVRT));

    mpm.add(createCloneAddressArithmeticPassWithFlags(rematOptions));

    // cloneAddressArithmetic leaves old instructions unnecessary
    // dce pass helps to clean that up
    mpm.add(createDeadCodeEliminationPass());
    if (IGC_IS_FLAG_SET(DumpRegPressureEstimate))
      mpm.add(new IGCRegisterPressurePrinter("after_remat"));
  }

  mpm.add(createRematAddressArithmeticPass());


  // Run type demotion if it's beneficial.
  if (ctx.m_DriverInfo.benefitFromTypeDemotion() && IGC_IS_FLAG_ENABLED(EnableTypeDemotion)) {
    mpm.add(createTypeDemotePass());
  }

  // Do Genx strengthreduction (do things like fdiv -> inv + mul)
  if (!isOptDisabled) {
    mpm.add(createGenStrengthReductionPass());
    mpm.add(createVectorBitCastOptPass());
  }

  bool forceUniformSurfaceSampler = ctx.getModuleMetaData()->compOpt.ForceUniformSurfaceSampler;
  bool forceUniformBuffer = ctx.getModuleMetaData()->compOpt.ForceUniformBuffer;

  if (ctx.m_instrTypes.hasUniformAssumptions || IGC_IS_FLAG_ENABLED(ForceUniformSurfaceSampler) ||
      forceUniformSurfaceSampler || IGC_IS_FLAG_ENABLED(ForceUniformBuffer) || forceUniformBuffer) {
    mpm.add(new UniformAssumptions(IGC_IS_FLAG_ENABLED(ForceUniformSurfaceSampler) || forceUniformSurfaceSampler,
                                   IGC_IS_FLAG_ENABLED(ForceUniformBuffer) || forceUniformBuffer));
  }

  // NanHandlingPass need to be before Legalization since it might make
  // some changes and require Legalization to "legalize"
  if (IGC_IS_FLAG_DISABLED(DisableBranchSwaping) && ctx.m_DriverInfo.BranchSwapping()) {
    mpm.add(createNanHandlingPass());
  }

  // TODO: move to use instruction flags
  // to figure out if we need to preserve Nan
  bool preserveNan = !ctx.getCompilerOption().NoNaNs;

  // Legalizer does not handle constant expressions
  mpm.add(new BreakConstantExpr());
  mpm.add(new Legalization(preserveNan));

  // Scalarizer in codegen to handle the vector instructions
  mpm.add(new ScalarizerCodeGen());

  // coalesce scalar loads into loads of larger quantity.
  // This require and preserves uniform analysis we should keep
  // other passes using uniformness together to avoid re-running it several times
  if (IGC_IS_FLAG_DISABLED(DisableConstantCoalescing) && !ctx.getModuleMetaData()->compOpt.DisableConstantCoalescing &&
      !isOptDisabledForModule(ctx.getModuleMetaData(), IGCOpts::ConstantCoalescingPass)) {
    mpm.add(createBreakCriticalEdgesPass());
    mpm.add(new ConstantCoalescing());
  }

  if (ctx.type == ShaderType::RAYTRACING_SHADER || ctx.hasSyncRTCalls()) {
    if (IGC_IS_FLAG_DISABLED(DisableLSCControlsForRayTracing))
      mpm.add(CreateLSCControlsAnalysisPass());

    // We do raytracing lowering a little earlier than the others here
    // to take advantage of the instruction simplification below.
    mpm.add(CreateRayTracingShaderLowering());
  }


  // Instruction combining may merge instruction back into unsupported intrinsics.
  // Therefore last Replace Unsupported Intrinsics Pass must be after last
  // Instruction combining pass.
  // Replace Unsupported Intrinsics Pass may generate new 64 bit operations.
  // Therefore last 64bit emulation pass must be after the last Replace Unsupported Intrinsics Pass.
  mpm.add(createReplaceUnsupportedIntrinsicsPass());

  if (!ctx.platform.hasFP32GlobalAtomicAdd()) {
    mpm.add(new AtomicOptPass());
  }

  // When m_hasDPEmu is true, enable Emu64Ops as well for now until
  // DPEmu is able to get rid of all 64bit integer ops fully.
  if ((ctx.m_hasDPEmu && IGC_IS_FLAG_ENABLED(DPEmuNeedI64Emu)) ||
      (ctx.m_DriverInfo.Enable64BitEmu() &&
       (IGC_GET_FLAG_VALUE(Enable64BitEmulation) ||
        (IGC_GET_FLAG_VALUE(Enable64BitEmulationOnSelectedPlatform) && ctx.platform.need64BitEmulation()))) ||
      ctx.platform.hasPartialInt64Support()) {
    mpm.add(new BreakConstantExpr());

    // Emu64OpsPass requires that we are working on legal types, specifically
    // that i128 uses are expanded to i64. This is why we need to run PeepholeTypeLegalizer
    // beforehand.
    mpm.add(new Legalizer::PeepholeTypeLegalizer());
    // Lower all GEPs now as Emu64 doesn't know how to handle them.
    if (KeepGEPs) {
      mpm.add(createGEPLoweringPass());
      mpm.add(llvm::createEarlyCSEPass());
    }
    // Run dead code elimination pass right before Emu64OpsPass,
    // as legalization passes do not always clear unused (operating
    // on illegal types) instructions.
    mpm.add(llvm::createDeadCodeEliminationPass());

    if (ctx.type == ShaderType::OPENCL_SHADER && IGC_IS_FLAG_ENABLED(EnableKernelCostInfo)) {
      mpm.add(createLoopCountAnalysisPass());
    }

    if (ctx.platform.hasPartialEmuI64Enabled()) {
      mpm.add(createPartialEmuI64OpsPass());
    } else {
      mpm.add(createEmu64OpsPass());
    }

    ctx.m_hasEmu64BitInsts = true;
    if (!isOptDisabled) {
      mpm.add(new GenSpecificPattern());
    }
  }

  if (ctx.m_instrTypes.hasRuntimeValueVector) {
    // Legalize RuntimeValue calls for push analysis
    mpm.add(new RuntimeValueLegalizationPass());
  }

  if ((ctx.m_instrTypes.hasLocalLoadStore || ctx.m_instrTypes.hasLocalAtomics) && ctx.platform.hasLSC() &&
      !ctx.platform.NeedsLSCFenceUGMBeforeEOT() && // VISA will add the fence
      IGC_IS_FLAG_DISABLED(DisableAddRequiredMemoryFencesPass)) {
    mpm.add(createAddRequiredMemoryFencesPass());
  }

  mpm.add(createInstSimplifyLegacyPass());
  // This pass inserts bitcasts for vector loads/stores.
  // This pass could be moved further toward EmitPass.
  mpm.add(createVectorProcessPass());

  // handling constant expressions created by vectorProcess pass
  mpm.add(new BreakConstantExpr());

  mpm.add(new LowPrecisionOpt());


  mpm.add(new WAFMinFMax());

  mpm.add(createTimeStatsCounterPass(&ctx, TIME_CG_Legalization, STATS_COUNTER_END));

  COMPILER_TIME_END(&ctx, TIME_CG_Add_Legalization_Passes);
} // AddLegalizationPasses

void AddCodeGenPasses(CodeGenContext &ctx, CShaderProgram::KernelShaderMap &shaders, IGCPassManager &Passes,
                      SIMDMode simdMode, bool canAbortOnSpill, ShaderDispatchMode shaderMode, PSSignature *pSignature) {
  // Generate CISA
  COMPILER_TIME_START(&ctx, TIME_CG_Add_CodeGen_Passes);
  Passes.add(new EmitPass(shaders, simdMode, canAbortOnSpill, shaderMode, pSignature));
  COMPILER_TIME_END(&ctx, TIME_CG_Add_CodeGen_Passes);
}


// check based on performance measures.
bool SimdEarlyCheck(CodeGenContext *ctx) {
  if (ctx->m_sampler < 11 || ctx->m_inputCount < 16 || ctx->m_tempCount < 40 || ctx->m_dxbcCount < 280 ||
      ctx->m_ConstantBufferCount < 500) {
    if (ctx->m_tempCount < 90 && ctx->m_ConstantBufferCount < 210) {
      return true;
    }
  }
  return false;
}


void destroyShaderMap(CShaderProgram::KernelShaderMap &shaders) {
  for (const auto &i : shaders) {
    CShaderProgram *shader = i.second;
    COMPILER_SHADER_STATS_PRINT(shader->m_shaderStats, shader->GetContext()->type, shader->GetContext()->hash, "");
    COMPILER_SHADER_STATS_SUM(shader->GetContext()->m_sumShaderStats, shader->m_shaderStats,
                              shader->GetContext()->type);
    COMPILER_SHADER_STATS_DEL(shader->m_shaderStats);
    delete shader;
  }
}

void unify_opt_PreProcess(CodeGenContext *pContext) {
  TODO("hasBuiltin should be calculated based on module");
  if (IGC_IS_FLAG_ENABLED(DisableLLVMGenericOptimizations)) {
    pContext->getModuleMetaData()->compOpt.OptDisable = true;
  }

  if (IGC_GET_FLAG_VALUE(StripDebugInfo) == FLAG_DEBUG_INFO_STRIP_ALL) {
    StripDebugInfo(*pContext->getModule());
  } else if (IGC_GET_FLAG_VALUE(StripDebugInfo) == FLAG_DEBUG_INFO_STRIP_NONLINE) {
    stripNonLineTableDebugInfo(*pContext->getModule());
  }

  IGCPassManager mpm(pContext, "OPTPre");
  mpm.add(new CodeGenContextWrapper(pContext));
  mpm.add(new CheckInstrTypes(false, true));

  if (pContext->isPOSH()) {
    mpm.add(createRemoveNonPositionOutputPass());
  }

  mpm.run(*pContext->getModule());

  // If the module does not contain called function declaration,
  // indirect calls are the only way to detect function pointers usage.
  if (pContext->m_instrTypes.hasIndirectCall)
    pContext->m_enableFunctionPointer = true;

  if (pContext->getMetaDataUtils()->size_FunctionsInfo() == 1 && !pContext->m_instrTypes.hasSubroutines) {
    pContext->m_instrTypes.numBB = pContext->getMetaDataUtils()->begin_FunctionsInfo()->first->size();
    pContext->m_instrTypes.hasMultipleBB = (pContext->m_instrTypes.numBB != 1);
  } else {
    pContext->m_instrTypes.hasMultipleBB = true;
  }

  pContext->m_instrTypes.hasLoadStore = true;

  pContext->m_instrTypes.CorrelatedValuePropagationEnable =
      (pContext->m_instrTypes.hasMultipleBB &&
       (pContext->m_instrTypes.hasSel || pContext->m_instrTypes.hasCmp || pContext->m_instrTypes.hasSwitch ||
        pContext->m_instrTypes.hasLoadStore));
}

static bool extensiveShader(CodeGenContext *pContext) {
  return (pContext->type == ShaderType::OPENCL_SHADER && pContext->m_instrTypes.numInsts > INST_THRESHOLD &&
          pContext->m_instrTypes.numLoopInsts > LOOP_INST_THRESHOLD &&
          pContext->m_instrTypes.numOfLoop > LOOP_NUM_THRESHOLD && pContext->m_instrTypes.numBB == 0 &&
          pContext->m_instrTypes.numSample == 0 && pContext->m_instrTypes.hasSubroutines);
}

// When we do not run optimizations, we still need to run always inline
// pass, otherwise codegen will fail.
static void alwaysInlineForNoOpt(CodeGenContext *pContext, bool NoOpt) {
  if (NoOpt) {
    MetaDataUtils *pMdUtils = pContext->getMetaDataUtils();
    IGCPassManager mpm(pContext, "OPTPost");
    mpm.add(new MetaDataUtilsWrapper(pMdUtils, pContext->getModuleMetaData()));
    mpm.add(new CodeGenContextWrapper(pContext));
    mpm.add(createAlwaysInlinerLegacyPass());
    mpm.add(new PurgeMetaDataUtils());
    mpm.run(*pContext->getModule());
  }
}


#define GFX_ONLY_PASS if (pContext->type != ShaderType::OPENCL_SHADER)

void OptimizeIR(CodeGenContext *const pContext) {
  IGC_ASSERT(nullptr != pContext);
  MetaDataUtils *pMdUtils = pContext->getMetaDataUtils();
  IGC_ASSERT(nullptr != pContext->getModuleMetaData());
  bool NoOpt = pContext->getModuleMetaData()->compOpt.OptDisable;

  DumpHashToOptions(pContext->hash, pContext->type);

  alwaysInlineForNoOpt(pContext, NoOpt);

  // Insert per-func optimization metadata
  for (auto &F : *pContext->getModule()) {
    if (!F.empty()) {
      IGC::InsertOptsMetadata(pContext, &F);
    }
  }

  if (NoOpt) {
    return;
  }

  IGCPassManager mpm(pContext, "OPT");
#if !defined(_DEBUG)
  if (IGC_IS_FLAG_ENABLED(EnableDebugging))
#endif
    // do verifyModule for debug/release_internal only.
    if (false == pContext->m_hasLegacyDebugInfo) {
      IGC_ASSERT(nullptr != pContext->getModule());
      IGC_ASSERT(false == llvm::verifyModule(*pContext->getModule(), &dbgs()));
    }

  COMPILER_TIME_START(pContext, TIME_OptimizationPasses);
  // scope to force destructors before mem usage sampling
  {
    unify_opt_PreProcess(pContext);
    /// Keeps track of the Dump objects so that we can free them after the pass manager has been run

    // right now we don't support any standard function in the code gen
    // maybe we want to support some at some point to take advantage of LLVM optimizations
    TargetLibraryInfoImpl TLI;
    TLI.disableAllFunctions();

    mpm.add(new MetaDataUtilsWrapper(pMdUtils, pContext->getModuleMetaData()));

    mpm.add(new CodeGenContextWrapper(pContext));
    TargetIRAnalysis GenTTgetIIRAnalysis([&](const Function &F) {
      GenIntrinsicsTTIImpl GTTI(pContext);
      return TargetTransformInfo(GTTI);
    });

    mpm.add(new TargetTransformInfoWrapperPass(GenTTgetIIRAnalysis));
#if defined(_DEBUG) && !defined(__ANDROID__)
    // IGC IR Verification pass checks that we get a correct IR after the Unification.
    mpm.add(new VerificationPass());
#endif
    mpm.add(new llvm::TargetLibraryInfoWrapperPass(TLI));
    initializeWIAnalysisPass(*PassRegistry::getPassRegistry());

    if (IGC_IS_FLAG_ENABLED(EnableSinkPointerConstAdd)) {
      mpm.add(createSinkPointerConstAddPass());
    }

       // Do inter-procedural constant propagation early.
    if (pContext->m_enableSubroutine) {
      // Here, we propagate function attributes across calls.  Remaining
      // function calls that were conservatively marked as 'convergent'
      // in ProcessBuiltinMetaData can have that attribute stripped if
      // possible which potentially allows late stage code sinking of
      // those calls by the instruction combiner.
      mpm.add(IGCLLVM::createLegacyWrappedPostOrderFunctionAttrsPass());

      // Don't run IPConstantProp if there are stackcalls
      const bool allowIPConstProp = !pContext->m_hasStackCalls && IGC_IS_FLAG_DISABLED(DisableIPConstantPropagation);

      if (allowIPConstProp) {
        mpm.add(IGCLLVM::createLegacyWrappedIPSCCPPass());
      }
      // Note / todo: LLVM < 12 also runs simple constant propagation pass
      // regardless of IPSCCP in this case. This pass is not available on
      // >= 12 version, but maybe SCCP pass would be suitable here.
    }
    if (IGC_IS_FLAG_ENABLED(MSAA16BitPayloadEnable) && pContext->platform.support16bitMSAAPayload()) {
      mpm.add(new ConvertMSAAPayloadTo16Bit());
    }

    if (IGC_GET_FLAG_VALUE(MSAAClearedKernel) > 0) {
      mpm.add(new MSAAInsertDiscard());
    }
    mpm.add(createSamplerPerfOptPass());


    if ((!IGC_IS_FLAG_ENABLED(DisableDynamicTextureFolding) &&
         pContext->getModuleMetaData()->inlineDynTextures.size() > 0) ||
        (!IGC_IS_FLAG_ENABLED(DisableDynamicResInfoFolding))) {
      mpm.add(new DynamicTextureFolding());
    }


    if (pContext->m_DriverInfo.CodeSinkingBeforeCFGSimplification()) {
      mpm.add(new HoistCongruentPHI());
      mpm.add(new CodeSinking());
    }
    mpm.add(llvm::createCFGSimplificationPass(SimplifyCFGOptions().hoistCommonInsts(true)));

    mpm.add(llvm::createBasicAAWrapperPass());
    mpm.add(createAddressSpaceAAWrapperPass());

    if (pContext->type == ShaderType::RAYTRACING_SHADER || pContext->hasSyncRTCalls()) {
      if (IGC_IS_FLAG_DISABLED(DisableRTAliasAnalysis))
        mpm.add(createRayTracingAddressSpaceAAWrapperPass());
    }

    mpm.add(createIGCExternalAAWrapper());
    mpm.add(createScopedNoAliasAAWrapperPass());

    if (pContext->m_instrTypes.hasLoadStore) {
      mpm.add(IGCLLVM::createLegacyWrappedDeadStoreEliminationPass());
      mpm.add(createMarkReadOnlyLoadPass());
    }

    mpm.add(createLogicalAndToBranchPass());


    mpm.add(llvm::createEarlyCSEPass());


    if (pContext->m_instrTypes.CorrelatedValuePropagationEnable) {
      mpm.add(IGCLLVM::createLegacyWrappedCorrelatedValuePropagationPass());
    }

    mpm.add(new BreakConstantExpr());
    mpm.add(new IGCConstProp());
    GFX_ONLY_PASS { mpm.add(createTranslateToProgrammableOffsetsPass()); }

    mpm.add(new CustomSafeOptPass());
    if (!pContext->m_DriverInfo.WADisableCustomPass()) {
      mpm.add(new CustomUnsafeOptPass());
    }
    mpm.add(createSubGroupReductionPatternPass());

    if (IGC_IS_FLAG_ENABLED(EmulateFDIV)) {
      mpm.add(createGenFDIVEmulation());
    }

    mpm.add(createIGCInstructionCombiningPass());
    if (IGC_IS_FLAG_ENABLED(EnableWaveShuffleIndexSinking)) {
      mpm.add(createWaveShuffleIndexSinking());
    }
    mpm.add(new FCmpPaternMatch());
    mpm.add(llvm::createDeadCodeEliminationPass()); // this should be done both before/after constant propagation

    if (pContext->m_instrTypes.hasGenericAddressSpacePointers && IGC_IS_FLAG_ENABLED(EnableGASResolver)) {
      mpm.add(createSROAPass());
      mpm.add(createFixAddrSpaceCastPass());
      mpm.add(createResolveGASPass());
    }

    if (pContext->m_instrTypes.hasNonPrimitiveAlloca && IGC_IS_FLAG_DISABLED(DisableShrinkArrayAllocaPass)) {
      mpm.add(new ShrinkArrayAllocaPass());
    }

    if (IGC_IS_FLAG_ENABLED(SampleMultiversioning) || pContext->m_enableSampleMultiversioning) {
      if (pContext->m_instrTypes.numOfLoop == 0)
        mpm.add(new SampleMultiversioning(pContext));
    }

    bool disableGOPT =
        ((IsStage1FastestCompile(pContext->m_CgFlag, pContext->m_StagingCtx) || IGC_GET_FLAG_VALUE(ForceFastestSIMD)) &&
         ((FastestS1Options(pContext) & FCEXP_DISABLE_GOPT) || FastestS1Options(pContext) == FCEXP_NO_EXPRIMENT ||
          pContext->getModuleMetaData()->compOpt.DisableFastestGopt));


    // EnableBarrierControlFlowOptimizationPass: enable BCF optimization
    // UseBarrierControlFlowOptimization: UMD AIL option to use BCF optimization
    // supportBarrierControlFlowOptimization: API control (D3D12, Vulkan, OCL)
    if ((IGC_IS_FLAG_ENABLED(EnableBarrierControlFlowOptimizationPass) ||
         pContext->getModuleMetaData()->compOpt.UseBarrierControlFlowOptimization) &&
        pContext->m_DriverInfo.supportBarrierControlFlowOptimization() &&
        pContext->platform.hasBarrierControlFlowOpt() && !pContext->hasSyncRTCalls() &&
        (pContext->type != ShaderType::PIXEL_SHADER)) {
      mpm.add(createBarrierControlFlowOptimization());
    }

    if (pContext->m_instrTypes.hasMultipleBB && !disableGOPT) {
      // disable loop unroll for excessive large shaders
      if (pContext->m_instrTypes.numOfLoop) {
        mpm.add(createLoopDeadCodeEliminationPass());
        mpm.add(createLoopCanonicalization());
        mpm.add(IGCLLVM::createLegacyWrappedLoopDeletionPass());
        mpm.add(llvm::createBreakCriticalEdgesPass());
        mpm.add(llvm::createLoopRotatePass(LOOP_ROTATION_HEADER_INST_THRESHOLD));
        mpm.add(llvm::createLCSSAPass());
        mpm.add(llvm::createLoopSimplifyPass());
      }
    }

    // This pass needs to be extended for other devices
    if (pContext->platform.getPlatformInfo().eProductFamily == IGFX_PVC) {
      mpm.add(new GenerateBlockMemOpsPass());
    }
    mpm.add(new BlockMemOpAddrScalarizationPass());

    if (pContext->m_instrTypes.hasMultipleBB && !disableGOPT) {
      if (pContext->m_instrTypes.numOfLoop) {
        bool AllowLICM = pContext->m_retryManager->AllowLICM() &&
                         (IGC_IS_FLAG_SET(allowLICM) ? IGC_IS_FLAG_ENABLED(allowLICM)
                                                     : pContext->getModuleMetaData()->compOpt.AllowLICM);
        bool runGEPLSR = IGC_IS_FLAG_ENABLED(EnableGEPLSR) && pContext->type == ShaderType::OPENCL_SHADER &&
                         (pContext->platform.getPlatformInfo().eProductFamily == IGFX_PVC ||
                          pContext->platform.getPlatformInfo().eProductFamily == IGFX_CRI) &&
                         !pContext->useStatelessToStateful() && !pContext->platform.hasEfficient64bEnabled() &&
                         pContext->m_retryManager->IsFirstTry();

        if (runGEPLSR && IGC_IS_FLAG_DISABLED(RunGEPLSRAfterLICM)) {
          mpm.add(createGEPLoopStrengthReductionPass(AllowLICM));
        }

        if (AllowLICM) {
          mpm.add(createSpecialCasesDisableLICM());
          int licmTh = IGC_GET_FLAG_VALUE(LICMStatThreshold);
          mpm.add(new InstrStatistic(pContext, LICM_STAT, InstrStatStage::BEGIN, licmTh));
          mpm.add(IGCLLVM::createLegacyWrappedLICMPass(100, 500, true));
          mpm.add(new InstrStatistic(pContext, LICM_STAT, InstrStatStage::END, licmTh));
        }

        if (runGEPLSR && IGC_IS_FLAG_ENABLED(RunGEPLSRAfterLICM)) {
          mpm.add(createGEPLoopStrengthReductionPass(AllowLICM));
        }


        if (!pContext->m_retryManager->IsFirstTry() && pContext->type == ShaderType::OPENCL_SHADER) {
          mpm.add(new DisableLoopUnrollOnRetry());
        }


        mpm.add(createIGCInstructionCombiningPass());

        if (IGC_IS_FLAG_ENABLED(EnableIndVarSimplification) && pContext->type == ShaderType::OPENCL_SHADER) {
          mpm.add(IGCLLVM::createLegacyWrappedIndVarSimplifyPass());
        }

        if (IGC_IS_FLAG_ENABLED(EnableLoopHoistConstant)) {
          mpm.add(createLoopHoistConstant());
        }
        if (IGC_IS_FLAG_ENABLED(EnableAdvCodeMotion) && pContext->type == ShaderType::OPENCL_SHADER &&
            !pContext->m_instrTypes.hasSwitch) {
          mpm.add(createAdvCodeMotionPass(IGC_GET_FLAG_VALUE(AdvCodeMotionControl)));
        }

        mpm.add(createLoopAllocaUpperbound());

        int LoopUnrollThreshold = pContext->m_DriverInfo.GetLoopUnrollThreshold();

        // override the LoopUnrollThreshold if the registry key is set
        if (IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold) != 0) {
          LoopUnrollThreshold = IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold);
        } else if (pContext->getModuleMetaData()->compOpt.SetLoopUnrollThreshold > 0) {
          LoopUnrollThreshold = pContext->getModuleMetaData()->compOpt.SetLoopUnrollThreshold;
        }

        // if the shader contains indexable_temp, we'll keep unroll
        bool unroll =
            !pContext->getModuleMetaData()->compOpt.DisableLoopUnroll && IGC_IS_FLAG_DISABLED(DisableLoopUnroll);
        bool hasIndexTemp = (pContext->m_indexableTempSize[0] > 0);
        bool disableLoopUnrollStage1 =
            IsStage1FastestCompile(pContext->m_CgFlag, pContext->m_StagingCtx) &&
            (FastestS1Options(pContext) == FCEXP_NO_EXPRIMENT || (FastestS1Options(pContext) & FCEXP_DISABLE_UNROLL));
        if ((LoopUnrollThreshold > 0 && unroll && !disableLoopUnrollStage1) || hasIndexTemp) {
          mpm.add(IGCLLVM::createLegacyWrappedLoopUnrollPass(2, false, false, -1, -1, -1, -1, -1, -1));
        }

        // Due to what looks like a bug in LICM, we need to break the LoopPassManager between
        // LoopUnroll and LICM.
        mpm.add(createBarrierNoopPass());

        if (AllowLICM) {
          mpm.add(createSpecialCasesDisableLICM());
          mpm.add(IGCLLVM::createLegacyWrappedLICMPass(100, 500, true));
        }

        // Second unrolling with the same threshold.
        unroll = !pContext->getModuleMetaData()->compOpt.DisableLoopUnroll && IGC_IS_FLAG_DISABLED(DisableLoopUnroll);
        if (LoopUnrollThreshold > 0 && unroll) {
          mpm.add(IGCLLVM::createLegacyWrappedLoopUnrollPass(2, false, false, -1, -1, -1, -1, -1, -1));
        }
        // Should be after LICM to accurately reason about which
        // instructions are loop-dependent or not. Needs to be before
        // another LICM call which will hoist relevant intrinsics
        if (IGC_GET_FLAG_VALUE(allowDecompose2DBlockFuncs)) {
          mpm.add(createDecompose2DBlockFuncsPass());
        }

        mpm.add(IGCLLVM::createLegacyWrappedLoopLoadEliminationPass());

        if (!extensiveShader(pContext) && pContext->m_instrTypes.hasNonPrimitiveAlloca) {
          if (pContext->m_DriverInfo.NeedCountSROA()) {
            mpm.add(new InstrStatistic(pContext, SROA_PROMOTED, InstrStatStage::BEGIN, 300));
            mpm.add(createSROAPass());
            mpm.add(new InstrStatistic(pContext, SROA_PROMOTED, InstrStatStage::END, 300));
          } else {
            mpm.add(createSROAPass());
          }
        }
      }

      mpm.add(new SplitStructurePhisPass());

      if (IGC_IS_FLAG_ENABLED(EnableRemoveLoopDependency)) {
        mpm.add(new RemoveLoopDependency());
      }

      // Note:
      // call reassociation pass before IGCConstProp(EnableSimplifyGEP)
      // to preserve the the expr evaluation order that IGCConstProp
      // creates.
      // Limit this optimization to GPGPU-only because it tends to have
      // more address computation.
      // Do not apply reordering on vertex-shader as CustomUnsafeOptPass
      // does.
      if (IGC_IS_FLAG_ENABLED(OCLEnableReassociate) && pContext->type == ShaderType::OPENCL_SHADER) {
        mpm.add(createReassociatePass());
      }

      mpm.add(createPromoteConstantStructsPass());

      if (IGC_IS_FLAG_ENABLED(EnableGVN)) {
        mpm.add(llvm::createGVNPass());
      }
      mpm.add(createGenOptLegalizer());

      mpm.add(IGCLLVM::createLegacyWrappedSCCPPass());

      mpm.add(llvm::createDeadCodeEliminationPass());
      if (!extensiveShader(pContext))
        mpm.add(IGCLLVM::createLegacyWrappedADCEPass());

      mpm.add(new BreakConstantExpr());
      mpm.add(new IGCConstProp(IGC_IS_FLAG_ENABLED(EnableSimplifyGEP)));
      // Now that constant propagation is largely complete, perform
      // initial evaluation of freeze instructions. We need this to make
      // life easier for subsequent LLVM passes, as passes like
      // InstCombine/SimplifyCFG can sometimes be lazy in checking freeze
      // operand's validity over the more complex instruction chains,
      // simply assuming that it's safer to refrain from optimizations.
      // TODO: Check if LLVM 15+ provides improvements in that regard,
      // alleviating the need for early freeze evaluation.
      mpm.add(createEvaluateFreezePass());

      if (IGC_IS_FLAG_DISABLED(DisableImmConstantOpt)) {
        // If we have ICBs, need to emit clamp code so OOB access doesn't occur
        if (pContext->getModuleMetaData()->immConstant.zeroIdxs.size()) {
          mpm.add(createClampICBOOBAccess());
        }
        GFX_ONLY_PASS { mpm.add(createIGCIndirectICBPropagaionPass()); }
      }
      GFX_ONLY_PASS { mpm.add(new GenUpdateCB()); }

      // Inserting PromoteToPredicatedMemoryAccess after GVN and several
      // other passes, to not block optimizations changing LLVM
      // load/stores, but before multiple SimplifyCFGs to allow more
      // aggressive CFG simplification.
      if (IGC_IS_FLAG_ENABLED(EnablePromoteToPredicatedMemoryAccess)) {
        mpm.add(new HoistConvOpToDom());
        mpm.add(llvm::createCFGSimplificationPass());
        mpm.add(new PromoteToPredicatedMemoryAccess());
      }

      if (IGC_IS_FLAG_ENABLED(EnableJumpThreading) && !pContext->m_instrTypes.hasAtomics &&
          !extensiveShader(pContext)) {
        if (pContext->type == ShaderType::OPENCL_SHADER) {
          // Add CFGSimplification for clean-up before JumpThreading.
          mpm.add(llvm::createCFGSimplificationPass());
        }

        // jump threading currently causes the atomic_flag test from c11 conformance to fail.  Right now,
        // only do jump threading if we don't have atomics as using atomics as locks seems to be the most common
        // case of violating the no independent forward progress clause from the spec.

        // We need to increase default duplication threshold since JumpThreading pass cost estimation does
        // not consider that not all instructions need to be duplicated.
        int BBDuplicateThreshold = (pContext->type == ShaderType::OPENCL_SHADER) ? 9 : -1;
#if LLVM_VERSION_MAJOR >= 15
        // In LLVM-12.x an extra parameter InsertFreezeWhenUnfoldingSelect = false was added
        // to JumpThreading pass, but since LLVM-15.x it was removed again.
        mpm.add(IGCLLVM::createLegacyWrappedJumpThreadingPass(BBDuplicateThreshold));
#else  // LLVM_VERSION_MAJOR
        mpm.add(llvm::createJumpThreadingPass(false, BBDuplicateThreshold));
#endif // LLVM_VERSION_MAJOR
      }
      mpm.add(llvm::createCFGSimplificationPass());
      mpm.add(llvm::createEarlyCSEPass());
      if (pContext->m_instrTypes.hasNonPrimitiveAlloca) {
        // run custom safe opts to potentially get rid of indirect
        // addressing of private arrays, see visitLoadInst
        mpm.add(new CustomSafeOptPass());
        mpm.add(createSROAPass());
      }

      // Use CFGSimplification to do clean-up. Needs to be invoked before lowerSwitch.
      mpm.add(llvm::createCFGSimplificationPass());

      if (IGC_IS_FLAG_DISABLED(DisableFlattenSmallSwitch)) {
        mpm.add(createFlattenSmallSwitchPass());
      }
      // some optimization can create switch statement we don't support
      mpm.add(llvm::createLowerSwitchPass());

      // preferred to be added after all LowerSwitch pass runs, as switch lowering is able
      // to benefit from unreachable instruction when it's in default switch case
      mpm.add(new UnreachableHandling());

      // Conditions apply just as above due to problems with atomics
      // (see comment above for details).
      if (IGC_IS_FLAG_ENABLED(EnableJumpThreading) && !pContext->m_instrTypes.hasAtomics &&
          !extensiveShader(pContext)) {
        // After lowering 'switch', run jump threading to remove redundant jumps.
        mpm.add(IGCLLVM::createLegacyWrappedJumpThreadingPass());
      }

      // run instruction combining to clean up the code after CFG optimizations
      mpm.add(createIGCInstructionCombiningPass());

      mpm.add(llvm::createDeadCodeEliminationPass());
      mpm.add(llvm::createEarlyCSEPass());
       // need to be before code sinking
      GFX_ONLY_PASS { mpm.add(createInsertBranchOptPass()); }

      mpm.add(new CustomSafeOptPass());
      if (!pContext->m_DriverInfo.WADisableCustomPass()) {
        mpm.add(new CustomUnsafeOptPass());
      }
    } else {
      if (pContext->m_instrTypes.hasMultipleBB) {
        assert(disableGOPT);
        // disable loop unroll for excessive large shaders
        if (pContext->m_instrTypes.numOfLoop) {
          mpm.add(llvm::createLoopRotatePass(LOOP_ROTATION_HEADER_INST_THRESHOLD));


          int LoopUnrollThreshold = pContext->m_DriverInfo.GetLoopUnrollThreshold();

          // override the LoopUnrollThreshold if the registry key is set
          if (IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold) != 0) {
            LoopUnrollThreshold = IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold);
          } else if (pContext->getModuleMetaData()->compOpt.SetLoopUnrollThreshold > 0) {
            LoopUnrollThreshold = pContext->getModuleMetaData()->compOpt.SetLoopUnrollThreshold;
          }

          // if the shader contains indexable_temp, we'll keep unroll
          bool unroll =
              !pContext->getModuleMetaData()->compOpt.DisableLoopUnroll && IGC_IS_FLAG_DISABLED(DisableLoopUnroll);
          bool hasIndexTemp = (pContext->m_indexableTempSize[0] > 0);
          // Enable loop unrolling for stage 1 for now due to persisting regressions
          bool disableLoopUnrollStage1 = IsStage1FastestCompile(pContext->m_CgFlag, pContext->m_StagingCtx) &&
                                         ( // FastestS1Options(pContext) == FCEXP_NO_EXPRIMENT ||
                                             (FastestS1Options(pContext) & FCEXP_DISABLE_UNROLL));
          if ((LoopUnrollThreshold > 0 && unroll && !disableLoopUnrollStage1) || hasIndexTemp) {
            mpm.add(IGCLLVM::createLegacyWrappedLoopUnrollPass(2, false, false, -1, -1, -1, -1, -1, -1));
          }
        }

        if (IGC_IS_FLAG_ENABLED(EnableGVN)) {
          mpm.add(llvm::createGVNPass());
        }
      }
      if (IGC_IS_FLAG_DISABLED(DisableImmConstantOpt)) {
        // If we have ICBs, need to emit clamp code so OOB access
        // doesn't occur
        if (pContext->getModuleMetaData()->immConstant.zeroIdxs.size()) {
          mpm.add(createClampICBOOBAccess());
        }

        GFX_ONLY_PASS { mpm.add(createIGCIndirectICBPropagaionPass()); }
      }
       // single basic block
      if (!pContext->m_DriverInfo.WADisableCustomPass()) {
        mpm.add(llvm::createEarlyCSEPass());
        mpm.add(new CustomSafeOptPass());
        mpm.add(new CustomUnsafeOptPass());
      }
      mpm.add(createGenOptLegalizer());
      GFX_ONLY_PASS { mpm.add(createInsertBranchOptPass()); }
    }
    // If we have ICBs, need to emit clamp code so OOB access doesn't occur
    if (pContext->getModuleMetaData()->immConstant.zeroIdxs.size() && IGC_IS_FLAG_ENABLED(DisableImmConstantOpt)) {
      mpm.add(createClampICBOOBAccess());
    }

    if (pContext->m_instrTypes.hasRuntimeValueVector) {
      // Optimize extracts from RuntimeValue vectors. It should be executed
      // after constants propagation and loop unrolling
      mpm.add(createVectorBitCastOptPass());
      mpm.add(new RuntimeValueVectorExtractPass());
    }

    if (pContext->m_enableSubroutine && getFunctionControl(pContext) == FLAG_FCALL_DEFAULT) {
      mpm.add(createEstimateFunctionSizePass(EstimateFunctionSize::AL_Kernel));
      if (IGC_IS_FLAG_ENABLED(EnableLargeFunctionCallMerging)) {
        mpm.add(new CallMerger());
      }
      mpm.add(createEstimateFunctionSizePass(EstimateFunctionSize::AL_Kernel));
      mpm.add(createSubroutineInlinerPass());
    } else {
      // Inline all remaining functions with always inline attribute.
      mpm.add(createAlwaysInlinerLegacyPass());
    }
    if ((pContext->m_DriverInfo.NeedExtraPassesAfterAlwaysInlinerPass() || pContext->m_enableSubroutine) &&
        pContext->m_instrTypes.hasNonPrimitiveAlloca) {
      mpm.add(createSROAPass());
    }

    if (pContext->type == ShaderType::COMPUTE_SHADER &&
        (IGC_IS_FLAG_ENABLED(RemoveUnusedTGMFence) || pContext->getModuleMetaData()->enableRemoveUnusedTGMFence)) {
      mpm.add(new TrivialUnnecessaryTGMFenceElimination());
    }

    mpm.add(createGenSimplificationPass());

    if (pContext->m_instrTypes.hasLoadStore) {
      mpm.add(IGCLLVM::createLegacyWrappedDeadStoreEliminationPass());
      mpm.add(IGCLLVM::createLegacyWrappedMemCpyOptPass());
      mpm.add(createLdShrinkPass());
    }

    mpm.add(llvm::createDeadCodeEliminationPass());

    if (IGC_IS_FLAG_ENABLED(EnableWaveAllJointReduction)) {
      mpm.add(createWaveAllJointReduction());
    }

    if (IGC_IS_FLAG_ENABLED(EnableIntDivRemCombine)) {
      // simplify rem if the quotient is availble
      //
      // run GVN first so that stuff like the following can be
      // reduced as well:
      //  = foo / (2*x + 1)
      //  = foo % (2*x + 1)
      // can be reduced as well
      if (IGC_IS_FLAG_ENABLED(EnableGVN)) {
        mpm.add(llvm::createGVNPass());
      }
      //
      mpm.add(createIntDivRemCombinePass());
    }
    if (IGC_IS_FLAG_ENABLED(EnableConstIntDivReduction)) {
      // reduce division/remainder with a constant divisors/moduli to
      // more efficient sequences of multiplies, shifts, and adds
      mpm.add(createIntDivConstantReductionPass());
    }

    if (IGC_IS_FLAG_ENABLED(EnableIntDivRemIncrementReduction) &&
        !pContext->getModuleMetaData()->compOpt.DisableIntDivRemIncrementReduction) {
      mpm.add(createIntDivRemIncrementReductionPass());
    }
    GFX_ONLY_PASS { mpm.add(createMergeMemFromBranchOptPass()); }

    if (IGC_IS_FLAG_DISABLED(DisableLoadSinking) &&
        !isOptDisabledForModule(pContext->getModuleMetaData(), IGCOpts::SinkLoadOptPass)) {
      mpm.add(createSinkLoadOptPass());
    }

    mpm.add(IGCLLVM::createLegacyWrappedConstantMergePass());
    GFX_ONLY_PASS { mpm.add(CreateMCSOptimization()); }
    GFX_ONLY_PASS { mpm.add(CreateGatingSimilarSamples()); }

    if (!IGC::ForceAlwaysInline(pContext)) {
      mpm.add(new PurgeMetaDataUtils());
    }
       // mpm.add(llvm::createDeadCodeEliminationPass()); // this should be done both before/after constant propagation

    if (IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions) && IGC_IS_FLAG_DISABLED(LateInlineUnmaskedFunc)) {
      mpm.add(new InlineUnmaskedFunctionsPass());
    }

    if (pContext->m_instrTypes.numOfLoop) {
      mpm.add(createDeadPHINodeEliminationPass());
    }

    if (IGC_IS_FLAG_ENABLED(EnableMadLoopSlice)) {
      mpm.add(createMadLoopSlicePass());
    }
    if (IGC_IS_FLAG_ENABLED(EnableVectorizer)) {
      mpm.add(new IGCVectorizer());
      mpm.add(IGCLLVM::createLegacyWrappedADCEPass());
      if (IGC_IS_FLAG_ENABLED(VectorizerCheckScalarizer))
        mpm.add(createScalarizerPass(SelectiveScalarizer::Auto));
      mpm.add(new IGCVectorCoalescer());
      mpm.add(IGCLLVM::createLegacyWrappedADCEPass());
    }


    mpm.run(*pContext->getModule());
  } // end scope
  COMPILER_TIME_END(pContext, TIME_OptimizationPasses);

  // pContext->shaderEntry->viewCFG();
  DumpLLVMIR(pContext, "optimized");
  MEM_SNAPSHOT(IGC::SMS_AFTER_OPTIMIZER);
} // OptimizeIR

} // namespace IGC
