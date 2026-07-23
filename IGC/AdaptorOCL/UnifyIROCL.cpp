/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/IPO/GlobalDCE.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/DCE.h>
#include <llvm/Transforms/Scalar/SROA.h>
#include <llvm/Transforms/Scalar/SCCP.h>
#include <llvm/Transforms/Scalar/InferAddressSpaces.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Utils/Mem2Reg.h>
#include <llvm/Transforms/Utils/LowerSwitch.h>
#include <llvm/Analysis/InlineCost.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Transforms/Utils.h>
#include "common/LLVMWarningsPop.hpp"

#include <llvmWrapper/Transforms/InstCombine/InstCombineWorklist.h>
#include <llvmWrapper/Transforms/Scalar/SCCP.h>
#include "llvmWrapper/Transforms/IPO/GlobalDCE.h"
#include "llvmWrapper/Transforms/IPO/InlineSimple.h"

#include "AdaptorCommon/AddImplicitArgs.hpp"
#include "AdaptorCommon/FreezeIntDiv.hpp"
#include "AdaptorCommon/ProcessFuncAttributes.h"
#include "AdaptorCommon/LegalizeFunctionSignatures.h"
#include "AdaptorCommon/TypesLegalizationPass.hpp"
#include "AdaptorCommon/DivergentBarrierPass.h"
#include "common/LLVMUtils.h"
#include "common/IGCNewPassManager.h"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/CISACodeGen/FixAddrSpaceCast.h"
#include "Compiler/Optimizer/OpenCLPasses/GenericCastToPtrOpt/GenericCastToPtrOpt.hpp"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GASResolving.h"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GASRetValuePropagator.h"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/StaticGASResolution.h"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/LowerGPCallArg.h"
#include "Compiler/CISACodeGen/ResolvePredefinedConstant.h"
#include "Compiler/CISACodeGen/SimplifyConstant.h"
#include "Compiler/CISACodeGen/FoldKnownWorkGroupSizes.h"
#include "Compiler/CISACodeGen/OpenCLKernelCodeGen.hpp"

#include "Compiler/HandleFRemInstructions.hpp"
#include "Compiler/Optimizer/BuiltInFuncImport.h"
#include "Compiler/Optimizer/CodeAssumption.hpp"
#include "Compiler/Optimizer/Scalarizer.h"
#include "Compiler/Optimizer/OpenCLPasses/ExtensionFuncs/ExtensionArgAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtensionFuncs/ExtensionFuncsAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtensionFuncs/ExtensionFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ImageFuncsAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ImageFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ImageFuncs/ResolveSampledImageBuiltins.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BindlessImage/ResolveImageImplicitArgsForBindless.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BindlessImage/PrepareInlineSamplerForBindless.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BindlessImage/ResolveInlineSamplerForBindless.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryUsageAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ResourceAllocator/ResourceAllocator.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakConstantExpr/BreakConstantExpr.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LocalBuffers/InlineLocalsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/Atomics/ResolveOCLAtomics.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WGFuncs/WGFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AlignmentAnalysis/AlignmentAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/InjectPrintf.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AggregateArguments/AggregateArguments.hpp"
#include "Compiler/Optimizer/OCLBIConverter.h"
#include "Compiler/Optimizer/OpenCLPasses/SetFastMathFlags/SetFastMathFlags.hpp"
#include "Compiler/Optimizer/OpenCLPasses/CorrectlyRoundedDivSqrt/CorrectlyRoundedDivSqrt.hpp"
#include "Compiler/Optimizer/OpenCLPasses/DeviceEnqueueFuncs/DeviceEnqueue.hpp"
#include "Compiler/Optimizer/OpenCLPasses/UndefinedReferences/UndefinedReferencesPass.hpp"
#include "Compiler/Optimizer/OpenCLPasses/SubGroupFuncs/SubGroupFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BIFTransforms/BIFTransforms.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakdownIntrinsic/BreakdownIntrinsic.h"
#include "Compiler/Optimizer/OpenCLPasses/TransformUnmaskedFunctionsPass/TransformUnmaskedFunctionsPass.h"
#include "Compiler/Optimizer/OpenCLPasses/DisableInlining/DisableInlining.h"
#include "Compiler/Optimizer/OpenCLPasses/DropTargetFunctions/DropTargetFunctions.h"
#include "Compiler/Optimizer/OpenCLPasses/KernelFunctionCloning/KernelFunctionCloning.h"
#include "Compiler/Optimizer/OpenCLPasses/NontemporalLoadsAndStoresInAssert/NontemporalLoadsAndStoresInAssert.hpp"
#include "Compiler/Optimizer/OpenCLPasses/HandleDevicelibAssert/HandleDevicelibAssert.hpp"
#include "Compiler/Optimizer/OpenCLPasses/StackOverflowDetection/StackOverflowDetection.hpp"
#include "Compiler/Legalizer/TypeLegalizerPass.h"
#include "Compiler/Optimizer/OpenCLPasses/Image3dToImage2darray/Image3dToImage2darray.hpp"
#include "Compiler/Optimizer/OpenCLPasses/RewriteLocalSize/RewriteLocalSize.hpp"
#include "Compiler/MetaDataApi/PurgeMetaDataUtils.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/SPIRMetaDataTranslation.h"
#include "Compiler/Optimizer/OpenCLPasses/ErrorCheckPass/ErrorCheckPass.h"
#include "Compiler/Optimizer/OpenCLPasses/PoisonFP64KernelsPass/PoisonFP64KernelsPass.h"
#include "Compiler/Optimizer/OpenCLPasses/BfloatBuiltins/BfloatBuiltinsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BfloatFuncs/BfloatFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/DpasFuncs/DpasFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LSCFuncs/LSCFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/NamedBarriers/NamedBarriersResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ManageableBarriers/ManageableBarriersResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/JointMatrixFuncsResolutionPass/JointMatrixFuncsResolutionPass.h"
#include "Compiler/Optimizer/OpenCLPasses/RayTracing/ResolveOCLRaytracingBuiltins.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AccuracyDecoratedCallsBiFResolution/AccuracyDecoratedCallsBiFResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ScalarArgAsPointer/ScalarArgAsPointer.hpp"
#include "AdaptorCommon/RayTracing/RayTracingPasses.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/FixResourcePtr.hpp"
#include "Compiler/MetaDataApi/SpirMetaDataApi.h"
#include "Compiler/Optimizer/ReduceOptPass.hpp"
#include "AdaptorCommon/MoveStaticAllocas.h"
#include "preprocess_spvir/PreprocessSPVIR.h"
#include "preprocess_spvir/ConvertUserSemanticDecoratorOnFunctions.h"
#include "preprocess_spvir/PromoteSubByte.h"
#include "preprocess_spvir/HandleSPIRVDecorations/HandleSpirvDecorationMetadata.h"
#include "LowerInvokeSIMD.hpp"
#include "ResolveConstExprCalls.h"
#include "Compiler/Optimizer/IGCInstCombiner/IGCInstructionCombining.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BufferBoundsChecking/BufferBoundsChecking.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BufferBoundsChecking/BufferBoundsCheckingPatcher.hpp"
#include "Compiler/Optimizer/OpenCLPasses/MinimumValidAddressChecking/MinimumValidAddressChecking.hpp"
#include "Compiler/Optimizer/OpenCLPasses/Subgroup2DBlockIoResolution/Subgroup2DBlockIoResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/SpvSubgroupMMAResolution/SpvSubgroupMMAResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/SpvSubgroupBitcastShuffleResolution/SpvSubgroupBitcastShuffleResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProcessBICodeAssumption/ProcessBICodeAssumption.hpp"

#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"

#include <iStdLib/utility.h>

#include "Compiler/CISACodeGen/DebugInfo.hpp"
#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "Compiler/DebugInfo/ScalarVISAModule.h"
#include "Compiler/Builtins/BIFFlagCtrl/BIFFlagCtrlResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/SpvPredicatedIOResolution/SpvPredicatedIOResolution.hpp"

#include <string>

using namespace llvm;
using namespace IGC::IGCMD;
using namespace IGC::Debug;

namespace IGC {
int getOCLMajorVersion(const SPIRMD::SpirMetaDataUtils &spirMDUtils) {
  int oclMajor = 0, oclMinor = 0;
  if (spirMDUtils.isOpenCLVersionsHasValue()) {
    SPIRMD::VersionMetaDataHandle oclVersion = spirMDUtils.getOpenCLVersionsItem(0);
    oclMajor = oclVersion->getMajor();
    oclMinor = oclVersion->getMinor();
  } else {
    if (!spirMDUtils.empty_CompilerOptions()) {
      // check compiler options
      for (auto i = spirMDUtils.getCompilerOptionsItem(0)->begin(), e = spirMDUtils.getCompilerOptionsItem(0)->end();
           i != e; ++i) {
        if (IGCLLVM::starts_with(StringRef(*i), "-cl-std=CL") && i->length() >= 13) {
          oclMajor = i->at(10) - '0';
          oclMinor = i->at(12) - '0';
          break;
        }
      }
    }
    // default is 1.2
    if (!isLegalOCLVersion(oclMajor, oclMinor)) {
      oclMajor = 1;
      oclMinor = 2;
    }
  }
  return oclMajor;
}

static void CommonOCLBasedPasses(OpenCLProgramContext *pContext) {
#if defined(_DEBUG)
  bool brokenDebugInfo = false;
  IGC_ASSERT(nullptr != pContext->getModule());
  IGC_ASSERT(false == llvm::verifyModule(*pContext->getModule(), &dbgs(), &brokenDebugInfo));

  // We ignore incorrect DI for now
  // We used if (false == pContext->m_hasLegacyDebugInfo), instead of passing &brokenDebugInfo earlier
  (void)brokenDebugInfo;
#endif

  COMPILER_TIME_START(pContext, TIME_UnificationPasses);

  setupTriple(*pContext);

  unify_opt_PreProcess(pContext);
  pContext->m_checkFastFlagPerInstructionInCustomUnsafeOptPass = true;
  pContext->m_mayHaveUnalignedAddressRegister = true;

  DumpLLVMIR(pContext, "beforeUnification");

  // override the data layout to match Gen HW
  int pointerSize = getPointerSize(*pContext->getModule());
  std::string layoutstr;
  if (pointerSize == 4) {
    layoutstr = "e-p:32:32:32";
  } else {
    layoutstr = "e-p:64:64:64";
  }
  layoutstr += "-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64"
               "-f32:32:32-f64:64:64-v16:16:16-v24:32:32"
               "-v32:32:32-v48:64:64-v64:64:64-v96:128:128"
               "-v128:128:128-v192:256:256-v256:256:256"
               "-v512:512:512-v1024:1024:1024-n8:16:32";

  StringRef dataLayout = layoutstr;
  pContext->getModule()->setDataLayout(dataLayout);

  MetaDataUtils *pMdUtils = pContext->getMetaDataUtils();

  // extracting OCL version major before SPIRMetadataTranslation pass deletes its metadata node
  const SPIRMD::SpirMetaDataUtils spirMDUtils(&(*pContext->getModule()));
  int OCLMajor = getOCLMajorVersion(spirMDUtils);

  CompOptions &CompilerOpts = pContext->getModuleMetaData()->compOpt;

  // check OpenCL build options
  bool shouldForceCR = pContext->m_Options.CorrectlyRoundedSqrt;

  CompilerOpts.replaceGlobalOffsetsByZero = pContext->m_InternalOptions.replaceGlobalOffsetsByZero;

  CompilerOpts.SubgroupIndependentForwardProgressRequired = (pContext->m_Options.NoSubgroupIFP == false);

  if (OCLMajor >= 2) {
    CompilerOpts.UniformWGS = pContext->m_Options.UniformWGS;
  }

  CompilerOpts.GreaterThan2GBBufferRequired = !pContext->m_InternalOptions.Use32BitPtrArith;

  CompilerOpts.GreaterThan4GBBufferRequired = (pContext->m_InternalOptions.IntelGreaterThan4GBBufferRequired ||
                                               pContext->m_Options.IntelGreaterThan4GBBufferRequired);

  CompilerOpts.DisableA64WA = pContext->m_InternalOptions.IntelDisableA64WA;

  CompilerOpts.ForceEnableA64WA = pContext->m_InternalOptions.IntelForceEnableA64WA;

  CompilerOpts.HasPositivePointerOffset = pContext->m_InternalOptions.IntelHasPositivePointerOffset;

  CompilerOpts.HasBufferOffsetArg = pContext->m_InternalOptions.IntelHasBufferOffsetArg;

  CompilerOpts.UseBindlessMode = pContext->m_InternalOptions.UseBindlessMode;

  CompilerOpts.UseLegacyBindlessMode = pContext->m_InternalOptions.UseBindlessLegacyMode;

  CompilerOpts.PreferBindlessImages =
      pContext->m_InternalOptions.PreferBindlessImages || pContext->m_InternalOptions.UseBindlessMode;

  if (CompilerOpts.PreferBindlessImages) {
    pContext->getModuleMetaData()->UseBindlessImage = true;
  }
  CompilerOpts.ForceInt32DivRemEmu = pContext->m_InternalOptions.IntelForceInt32DivRemEmu;

  CompilerOpts.ForceInt32DivRemEmuSP = pContext->m_InternalOptions.IntelForceInt32DivRemEmuSP;

  CompilerOpts.EnableTakeGlobalAddress = pContext->m_Options.EnableTakeGlobalAddress;

  CompilerOpts.IsLibraryCompilation = pContext->m_Options.IsLibraryCompilation;

  CompilerOpts.LibraryCompileSIMDSize = pContext->m_Options.LibraryCompileSIMDSize;

  CompilerOpts.ExcludeIRFromZEBinary = pContext->m_InternalOptions.ExcludeIRFromZEBinary;

  CompilerOpts.EmitZeBinVISASections = pContext->m_InternalOptions.EmitZeBinVISASections;

  CompilerOpts.FP64GenEmulationEnabled =
      pContext->m_InternalOptions.EnableFP64GenEmu || pContext->m_Options.EnableFP64GenEmu;

  CompilerOpts.FP64GenConvEmulationEnabled = pContext->m_Options.EnableFP64GenConvEmu;

  CompilerOpts.LoadCacheDefault = pContext->m_InternalOptions.LoadCacheDefault;

  CompilerOpts.StoreCacheDefault = pContext->m_InternalOptions.StoreCacheDefault;

#if LLVM_VERSION_MAJOR >= 16
  IGCNewPassManager npmSPIR(pContext, "Unify");
  npmSPIR.registerContextAnalyses(pContext, pMdUtils, pContext->getModuleMetaData());
#endif
  IGCPassManager lpmSPIR(pContext, "Unify");
  if (!IGC_IS_FLAG_ENABLED(EnableOCLNewPassManager)) {
    // These wrapper passes expose analyses (TargetLibraryInfo, MetaDataUtils, CodeGenContext) to downstream legacy
    // passes via the LPM analysis query mechanism. In the NPM path the equivalent analyses are registered by
    // registerContextAnalyses() above.
    lpmSPIR.add(new TargetLibraryInfoWrapperPass());
    lpmSPIR.add(new MetaDataUtilsWrapper(pMdUtils, pContext->getModuleMetaData()));
    lpmSPIR.add(new CodeGenContextWrapper(pContext));
  }

  IGC_ADD_PASS_AUTO(npmSPIR, lpmSPIR, PreprocessSPVIR);
  IGC_ADD_PASS_AUTO(npmSPIR, lpmSPIR, PromoteSubByte);
  IGC_ADD_PASS(npmSPIR, lpmSPIR, TypesLegalizationPassNPM(), new TypesLegalizationPassLPM());
  IGC_ADD_PASS_AUTO(npmSPIR, lpmSPIR, SPIRMetaDataTranslation);
  IGC_ADD_PASS_AUTO(npmSPIR, lpmSPIR, ConvertUserSemanticDecoratorOnFunctions);
  IGC_ADD_PASS_AUTO(npmSPIR, lpmSPIR, HandleSpirvDecorationMetadata);
  IGC_ADD_PASS(npmSPIR, lpmSPIR, DCEPass(), createDeadCodeEliminationPass());
  IGC_RUN_PM(npmSPIR, lpmSPIR, *pContext->getModule());

  bool isOptDisabled = CompilerOpts.OptDisable;

  // right now we don't support any standard function in the code gen
  // maybe we want to support some at some point to take advantage of LLVM optimizations
  TargetLibraryInfoImpl TLI(Triple(pContext->getModule()->getTargetTriple()));
  TLI.disableAllFunctions();

#if LLVM_VERSION_MAJOR >= 16
  IGCNewPassManager npm(pContext, "Unify", &TLI);
  npm.registerContextAnalyses(pContext, pMdUtils, pContext->getModuleMetaData());
#endif
  IGCPassManager lpm(pContext, "Unify");
  if (!IGC_IS_FLAG_ENABLED(EnableOCLNewPassManager)) {
    // These wrapper passes expose analyses (TargetLibraryInfo, MetaDataUtils, CodeGenContext) to
    // downstream legacy passes. In the NPM path the equivalents are registered above (the disabled
    // TargetLibraryInfo is seeded into the IGCNewPassManager constructor).
    lpm.add(new llvm::TargetLibraryInfoWrapperPass(TLI));
    lpm.add(new MetaDataUtilsWrapper(pMdUtils, pContext->getModuleMetaData()));
    lpm.add(new CodeGenContextWrapper(pContext));
  }

  if (IGC_IS_FLAG_ENABLED(EnableDropTargetFunctions)) {
    IGC_ADD_PASS_AUTO(npm, lpm, DropTargetFunctions);
  }

  if (IGC_IS_FLAG_ENABLED(DisableInlining)) {
    IGC_ADD_PASS(npm, lpm, DisableInliningNPM(), new DisableInliningLPM());
  }

  if (IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions)) {
    IGC_ADD_PASS_AUTO(npm, lpm, TransformUnmaskedFunctionsPass);
  }

  IGC_ADD_PASS_AUTO(npm, lpm, AccuracyDecoratedCallsBiFResolution);
  IGC_ADD_PASS(npm, lpm, MoveStaticAllocasNPM(), new MoveStaticAllocasLPM());

  // Clone kernel function being used as user function.
  IGC_ADD_PASS(npm, lpm, KernelFunctionCloningNPM(), createKernelFunctionCloningPass());

  IGC_ADD_PASS(npm, lpm, CorrectlyRoundedDivSqrtNPM(shouldForceCR, false),
               new CorrectlyRoundedDivSqrtLPM(shouldForceCR, false));
  if (IGC_IS_FLAG_ENABLED(EnableIntelFast)) {
    IGC_ADD_PASS(npm, lpm, BIFTransformsNPM(), createBIFTransformsPass());
  }

  if (IGC_IS_FLAG_ENABLED(EnableCodeAssumption)) {
    IGC_ADD_PASS_AUTO(npm, lpm, CodeAssumption);
  }

  if (pContext->m_instrTypes.hasFRem) {
    IGC_ADD_PASS_AUTO(npm, lpm, HandleFRemInstructions);
  }

  IGC_ADD_PASS_AUTO(npm, lpm, JointMatrixFuncsResolutionPass);
  IGC_ADD_PASS(npm, lpm, BfloatBuiltinsResolutionNPM(), new BfloatBuiltinsResolutionLPM());

  IGC_ADD_PASS(npm, lpm, ReduceOptPassNPM(), new ReduceOptPassLPM());

  IGC_ADD_PASS_AUTO(npm, lpm, HandleDevicelibAssert);
  if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
    IGC_ADD_PASS(npm, lpm, StackOverflowDetectionPassNPM(StackOverflowDetectionPass::Mode::Initialize),
                 new StackOverflowDetectionPassLPM(StackOverflowDetectionPass::Mode::Initialize));
  }

  if (IGC_IS_FLAG_ENABLED(BufferBoundsChecking) || pContext->isBufferBoundsChecking()) {
    IGC_ADD_PASS_AUTO(npm, lpm, BufferBoundsChecking);
  }

  // Minimum valid address checking
  {
    uint64_t minimumValidAddress = IGC_GET_FLAG_VALUE(MinimumValidAddress);
    if (!minimumValidAddress) {
      minimumValidAddress = pContext->getMinimumValidAddress();
    }
    if (minimumValidAddress) {
      IGC_ADD_PASS(npm, lpm, MinimumValidAddressCheckingNPM(minimumValidAddress),
                   new MinimumValidAddressCheckingLPM(minimumValidAddress));
    }
  }

  IGC_ADD_PASS(npm, lpm, NamedBarriersResolutionNPM(pContext->platform.getPlatformInfo().eRenderCoreFamily),
               new NamedBarriersResolutionLPM(pContext->platform.getPlatformInfo().eRenderCoreFamily));

  // This pass should be run before BuiltInImport and before Inlining to allow cache control to be resolved
  IGC_ADD_PASS_AUTO(npm, lpm, Subgroup2DBlockIoResolution);

  IGC_ADD_PASS_AUTO(npm, lpm, SpvSubgroupMMAResolution);
  IGC_ADD_PASS_AUTO(npm, lpm, SpvPredicatedIOResolution);
  IGC_ADD_PASS_AUTO(npm, lpm, SpvSubgroupBitcastShuffleResolution);

  IGC_ADD_PASS(npm, lpm, ProcessBICodeAssumptionNPM(), createProcessBICodeAssumptionPass());
  IGC_ADD_PASS_AUTO(npm, lpm, PreBIImportAnalysis);
  IGC_ADD_PASS(npm, lpm, TimeStatsCounterNPM(pContext, TIME_Unify_BuiltinImport, STATS_COUNTER_START),
               createTimeStatsCounterPass(pContext, TIME_Unify_BuiltinImport, STATS_COUNTER_START));
  IGC_ADD_PASS(npm, lpm, BIImportNPM(), createBuiltInImportPass());
  IGC_ADD_PASS(npm, lpm, TimeStatsCounterNPM(pContext, TIME_Unify_BuiltinImport, STATS_COUNTER_END),
               createTimeStatsCounterPass(pContext, TIME_Unify_BuiltinImport, STATS_COUNTER_END));
  IGC_ADD_PASS(npm, lpm, BIFFlagCtrlResolutionNPM(), new BIFFlagCtrlResolutionLPM(pContext));

  if (IGC_GET_FLAG_VALUE(AllowMem2Reg)) {
    IGC_ADD_PASS(npm, lpm, PromotePass(), createPromoteMemoryToRegisterPass());
  }

  // OCL has built-ins so it always need to run inlining
  {
    // We need to propagate constexpr casts to resolve pseudo indirect calls
    IGC_ADD_PASS(npm, lpm, SCCPPass(), IGCLLVM::createLegacyWrappedSCCPPass());
    IGC_ADD_PASS_AUTO(npm, lpm, ResolveConstExprCalls);

    // Estimate maximal function size in the module and disable subroutine if not profitable. This is
    // a pure analysis whose result is queried by ProcessFuncAttributes; the NPM ProcessFuncAttributes
    // computes it inline, so it is only scheduled on the legacy path.
    lpm.add(createEstimateFunctionSizePass(pContext->m_Options.StaticProfileGuidedTrimming));
    IGC_ADD_PASS(npm, lpm, ProcessFuncAttributesNPM(), createProcessFuncAttributesPass());
    FastMathFlags Mask;
    Mask.setFast();
    Mask.setNoSignedZeros(false);
    IGC_ADD_PASS(npm, lpm, SetFastMathFlagsNPM(Mask, true), new SetFastMathFlagsLPM(Mask, true));

    // mark load and stores inside assert calls as nontemporal to avoid caching.
    IGC_ADD_PASS_AUTO(npm, lpm, NontemporalLoadsAndStoresInAssert);

    if (IGC_IS_FLAG_ENABLED(EnableGenericCastToPtrOpt)) {
      IGC_ADD_PASS_AUTO(npm, lpm, GenericCastToPtrOpt);
    }

    // Report undef references after setting func attribs for import linking
    IGC_ADD_PASS_AUTO(npm, lpm, UndefinedReferencesPass);

    if (!IGC::ForceAlwaysInline(pContext)) {
      int Threshold = IGC_GET_FLAG_VALUE(OCLInlineThreshold);
      IGC_ADD_PASS(npm, lpm, IGCLLVM::SimpleInlinerNPMWrapper(llvm::getInlineParams(Threshold), &TLI),
                   IGCLLVM::createLegacyWrappedSimpleInlinerPass(Threshold));
    } else {
      IGC_ADD_PASS(npm, lpm, AlwaysInlinerPass(), createAlwaysInlinerLegacyPass());
    }
    // The inliner sometimes fails to delete unused functions, this cleans up the remaining mess.
    IGC_ADD_PASS(npm, lpm, GlobalDCEPass(), IGCLLVM::createLegacyWrappedGlobalDCEPass());

    // After inlining, some functions that were originally called indirectly
    // (address-taken) may now only have direct callers or be completely dead.
    // Strip the stale "referenced-indirectly"/"visaStackCall" attributes and
    // restore internal linkage so DCE / PurgeMetaDataUtils can remove them.
    IGC_ADD_PASS(npm, lpm, CleanupIndirectlyReferencedFunctionsNPM(), createCleanupIndirectlyReferencedFunctionsPass());

    IGC_ADD_PASS(npm, lpm, GlobalDCEPass(), IGCLLVM::createLegacyWrappedGlobalDCEPass());

    // Check after GlobalDCE in case of doubles in dead functions
    IGC_ADD_PASS_AUTO(npm, lpm, ErrorCheck);

    IGC_ADD_PASS_AUTO(npm, lpm, LowerInvokeSIMD);

    // Run BreakConstantExpr right before LegalizeFunctionSignatures, since
    // input module may contain call sites with bitcast constant expressions
    IGC_ADD_PASS(npm, lpm, BreakConstantExprNPM(), new BreakConstantExprLPM());

    // Fix illegal argument/return types in function calls not already inlined.
    // This pass changes all illegal function signatures to be passed by pointer instead.
    IGC_ADD_PASS_AUTO(npm, lpm, LegalizeFunctionSignatures);

    IGC_ADD_PASS(npm, lpm, ProcessBuiltinMetaDataNPM(), createProcessBuiltinMetaDataPass());
    IGC_ADD_PASS_AUTO(npm, lpm, PurgeMetaDataUtils);
  }

  if (ManageableBarriersResolution::HasHWSupport(pContext->platform.getPlatformInfo().eRenderCoreFamily)) {
    IGC_ADD_PASS_AUTO(npm, lpm, ManageableBarriersResolution);
  }

  // OpenCL WI + image function resolution

  // OCLTODO : do another DCE that will get rid of unused WI func calls before this?
  // We can save passing of unused implicit args from the runtime

  // Adding Mem2Reg pass in order to help ImageFuncsAnalysis to identify the image arguments
  // that the image functions operate on
  // Clang output is: alloca --> store image func arg into allocated address -->
  //                  load image arg from stored address --> call func on loaded image
  // After Mem2Reg: call func on image func arg

  IGC_ADD_PASS(npm, lpm, SROAPass(llvm::SROAOptions::PreserveCFG), createSROAPass());

  IGC_ADD_PASS(npm, lpm, BreakConstantExprNPM(), new BreakConstantExprLPM());

  if (IGC_IS_FLAG_ENABLED(EnableGASResolver)) {
    // InferAddressSpaces pass requires TTI analysis, but it doesn't initialize it
    // as a dependency. Let's run it manually until IGC is switched to LLVM 14.
    // (The NPM path gets TargetIRAnalysis from the function analysis manager, so this is
    // legacy-only.)
    lpm.add(createTargetTransformInfoWrapperPass(TargetIRAnalysis()));
    // Run InferAddressSpaces pass first - to propagate named addrspaces
    // through elementary LLVM instructions, then run custom ResolveGAS
    // pass to handle IGC specific instructions, like builtins etc.
    IGC_ADD_PASS(npm, lpm, InferAddressSpacesPass(ADDRESS_SPACE_GENERIC),
                 createInferAddressSpacesPass(ADDRESS_SPACE_GENERIC));

    // Add fix up of illegal `addrspacecast` in respect to OCL 2.0 spec.
    IGC_ADD_PASS(npm, lpm, AddrSpaceCastFixingNPM(), createFixAddrSpaceCastPass());
    IGC_ADD_PASS(npm, lpm, GASResolvingNPM(), createResolveGASPass());

    if (IGC_IS_FLAG_ENABLED(EnableLowerGPCallArg)) {
      IGC_ADD_PASS(npm, lpm, LowerGPCallArgNPM(), createLowerGPCallArg());
    }
    IGC_ADD_PASS(npm, lpm, StaticGASResolutionNPM(), createStaticGASResolution());

    IGC_ADD_PASS(npm, lpm, GASRetValuePropagatorNPM(), createGASRetValuePropagatorPass());

    // this pass is intended to inline the remaining always inline functions that had issues
    // with argument address spaces (byVal addrspace(4)) in the previous attempt
    IGC_ADD_PASS(npm, lpm, AlwaysInlinerPass(), createAlwaysInlinerLegacyPass());
    IGC_ADD_PASS_AUTO(npm, lpm, PurgeMetaDataUtils);

    // Run another round of constant breaking as GAS resolving may generate constants (constant address)
    IGC_ADD_PASS(npm, lpm, BreakConstantExprNPM(), new BreakConstantExprLPM());
  }

  if (CompilerOpts.UniformWGS)
    IGC_ADD_PASS_AUTO(npm, lpm, RewriteLocalSize);

  IGC_ADD_PASS(npm, lpm, SROAPass(llvm::SROAOptions::PreserveCFG), createSROAPass());
  IGC_ADD_PASS(npm, lpm, BreakConstantExprNPM(), new BreakConstantExprLPM());

  IGC_ADD_PASS(npm, lpm, FoldKnownWorkGroupSizesNPM(), CreateFoldKnownWorkGroupSizes());

  IGC_ADD_PASS_AUTO(npm, lpm, ResolveSampledImageBuiltins);
  IGC_ADD_PASS_AUTO(npm, lpm, ResolveImageImplicitArgsForBindless);

  // 64-bit atomics have to be resolved before AddImplicitArgs pass as it uses
  // local ids for spin lock initialization
  IGC_ADD_PASS_AUTO(npm, lpm, ResolveOCLAtomics);

  // Run the AlignmentAnalysis pass before the passes which add implicit arguments, to ensure we do not lose load/store
  // alignment information. For example, ProgramScopeConstantResolution will relocate the buffer's base to an i8* typed
  // pointer.
  IGC_ADD_PASS_AUTO(npm, lpm, AlignmentAnalysis);

  // Analysis passes
  IGC_ADD_PASS_AUTO(npm, lpm, WIFuncsAnalysis);
  IGC_ADD_PASS_AUTO(npm, lpm, ImageFuncsAnalysis);
  IGC_ADD_PASS(npm, lpm, InjectPrintfNPM(), new InjectPrintfLPM());
  IGC_ADD_PASS_AUTO(npm, lpm, OpenCLPrintfAnalysis);
  IGC_ADD_PASS(npm, lpm, DCEPass(), createDeadCodeEliminationPass());
  IGC_ADD_PASS_AUTO(npm, lpm, ProgramScopeConstantAnalysis);
  IGC_ADD_PASS_AUTO(npm, lpm, PrivateMemoryUsageAnalysis);
  IGC_ADD_PASS_AUTO(npm, lpm, AggregateArgumentsAnalysis);
  IGC_ADD_PASS_AUTO(npm, lpm, ExtensionFuncsAnalysis);
  // ExtensionArgAnalysis result is queried by ResourceAllocator, whose NPM wrapper computes it inline,
  // so it is only scheduled on the legacy path.
  lpm.add(new ExtensionArgAnalysisLPM());
  IGC_ADD_PASS_AUTO(npm, lpm, DeviceEnqueueFuncsAnalysis);

  IGC_ADD_PASS_AUTO(npm, lpm, BuiltinCallGraphAnalysis);

  IGC_ADD_PASS_AUTO(npm, lpm, ResolveOCLRaytracingBuiltins);
  IGC_ADD_PASS(npm, lpm, RayTracingIntrinsicAnalysisNPM(), createRayTracingIntrinsicAnalysisPass());
  IGC_ADD_PASS_AUTO(npm, lpm, PrepareInlineSamplerForBindless);

  // Adding implicit args based on Analysis passes
  IGC_ADD_PASS_AUTO(npm, lpm, AddImplicitArgs);

  IGC_ADD_PASS_AUTO(npm, lpm, ResolveInlineSamplerForBindless);
  if (IGC_IS_FLAG_ENABLED(BufferBoundsChecking) || pContext->isBufferBoundsChecking()) {
    IGC_ADD_PASS_AUTO(npm, lpm, BufferBoundsCheckingPatcher);
  }

  IGC_ADD_PASS(npm, lpm, RayTracingIntrinsicResolutionNPM(), createRayTracingIntrinsicResolutionPass());

  // Resolution passes
  IGC_ADD_PASS_AUTO(npm, lpm, WIFuncResolution);
  IGC_ADD_PASS_AUTO(npm, lpm, OpenCLPrintfResolution);
  IGC_ADD_PASS_AUTO(npm, lpm, ResolveOCLAtomics);
  IGC_ADD_PASS_AUTO(npm, lpm, ResourceAllocator);
  IGC_ADD_PASS_AUTO(npm, lpm, SubGroupFuncsResolution);
  IGC_ADD_PASS_AUTO(npm, lpm, BfloatFuncsResolution);
  IGC_ADD_PASS(npm, lpm, DpasFuncsResolutionNPM(), createDpasFuncsResolutionPass());
  IGC_ADD_PASS(npm, lpm, LSCFuncsResolutionNPM(), createLSCFuncsResolutionPass());

  IGC_ADD_PASS_AUTO(npm, lpm, WGFuncResolution);
  IGC_ADD_PASS_AUTO(npm, lpm, ResolveAggregateArguments);
  IGC_ADD_PASS_AUTO(npm, lpm, ExtensionFuncsResolution);
  IGC_ADD_PASS_AUTO(npm, lpm, DeviceEnqueueFuncsResolution);

  IGC_ADD_PASS(npm, lpm, DCEPass(), createDeadCodeEliminationPass());

  IGC_ADD_PASS(npm, lpm, BuiltinsConverterNPM(), createBuiltinsConverterPass());

  if (pContext->needsDivergentBarrierHandling()) {
    IGC_ADD_PASS(npm, lpm, DivergentBarrierPassNPM(nullptr), createDivergentBarrierPass(nullptr));
    IGC_ADD_PASS(npm, lpm, BreakConstantExprNPM(), new BreakConstantExprLPM());
  }

  IGC_ADD_PASS_AUTO(npm, lpm, InlineLocalsResolution);

  // check for unsupported intrinsics
  IGC_ADD_PASS_AUTO(npm, lpm, ErrorCheck);
  if (pContext->m_Options.EnableUnsupportedFP64Poisoning) {
    IGC_ADD_PASS_AUTO(npm, lpm, PoisonFP64Kernels);
    IGC_ADD_PASS_AUTO(npm, lpm, PurgeMetaDataUtils);
  }

  IGC_ADD_PASS_AUTO(npm, lpm, ImageFuncResolution);
  IGC_ADD_PASS_AUTO(npm, lpm, Image3dToImage2darray);

  // Break down the intrinsics into smaller operations (eg. fmuladd to fmul add)
  IGC_ADD_PASS_AUTO(npm, lpm, BreakdownIntrinsicPass);

  {
    if (IGC_IS_FLAG_ENABLED(EnableConstantPromotion)) {
      IGC_ADD_PASS(npm, lpm, SimplifyConstantNPM(), createSimplifyConstantPass());
      IGC_ADD_PASS(npm, lpm, PromoteConstantNPM(), createPromoteConstantPass());
    }
    // For LLVM 14+, make sure to freeze potential UB-causing instructions
    // before running InstructionCombining that would propagate resulting
    // poison/undef values.
    IGC_ADD_PASS(npm, lpm, FreezeIntDivNPM(), createFreezeIntDivPass());
    IGC_ADD_PASS(npm, lpm, InstCombinePass(), createIGCInstructionCombiningPass());

    // Instcombine can create constant expressions, which are not handled by the program scope constant resolution pass
    IGC_ADD_PASS(npm, lpm, BreakConstantExprNPM(), new BreakConstantExprLPM());

    // Run constant lowering conservatively for tests where constant
    // objects are over-written after casting pointers in constant address
    // space into ones in private address.
    //
    // NOTE: Per OpenCL C standard (both 1.2 and 2.0), that's illegal.
    //
    // This has to be run after instcombine to allow memcpy from GlobalVariable arrays private
    // allocs to be optimized away.
    IGC_ADD_PASS(npm, lpm, ProgramScopeConstantResolutionNPM(true), new ProgramScopeConstantResolutionLPM(true));
  }

  // TODO: Run CheckInstrTypes after builtin import to determine if builtins have allocas.
  IGC_ADD_PASS(npm, lpm, SROAPass(llvm::SROAOptions::PreserveCFG), createSROAPass());
  IGC_ADD_PASS(npm, lpm, InstCombinePass(), createIGCInstructionCombiningPass());
  // See the comment above (it's copied as is).
  // Instcombine can create constant expressions, which are not handled by the program scope constant resolution pass.
  // For example, in InsertDummyKernelForSymbolTablePass addresses of indirectly called functions
  // should be processed and without BreakConstantExpr the addresses are not found.
  IGC_ADD_PASS(npm, lpm, BreakConstantExprNPM(), new BreakConstantExprLPM());

  IGC_ADD_PASS_AUTO(npm, lpm, ScalarArgAsPointerAnalysis);

  if (IGC_IS_FLAG_DISABLED(DisableOCLScalarizer)) {
    IGC_ADD_PASS(npm, lpm, ScalarizeFunctionNPM(SelectiveScalarizer::Auto),
                 createScalarizerPass(SelectiveScalarizer::Auto));
  }

  // Create a dummy kernel to attach the symbol table if necessary
  // Only needed if function pointers, externally linked functions, or relocatable global variables are present
  IGC_ADD_PASS(npm, lpm, InsertDummyKernelForSymbolTableNPM(), createInsertDummyKernelForSymbolTablePass());

  FastMathFlags Mask;
  Mask.setNoSignedZeros(true);
  IGC_ADD_PASS(npm, lpm, SetFastMathFlagsNPM(Mask, true), new SetFastMathFlagsLPM(Mask, true));
  IGC_ADD_PASS(npm, lpm, FixResourcePtrNPM(), new FixResourcePtrLPM());

  if (isOptDisabled) {
    // Run additional predefined constant resolving when optimization is
    // disabled. It's definitely a workaround so far.
    IGC_ADD_PASS(npm, lpm, PredefinedConstantResolvingNPM(), createResolvePredefinedConstantPass());
  }

  IGC_ADD_PASS(npm, lpm, LowerSwitchPass(), createLowerSwitchPass());
  IGC_ADD_PASS(npm, lpm, TypeLegalizerNPM(), createTypeLegalizerPass());

  IGC_ADD_PASS(npm, lpm, CatchAllLineNumberNPM(), new CatchAllLineNumberLPM());

  IGC_RUN_PM(npm, lpm, *pContext->getModule());

  // Following functions checks whether -g option is specified.
  // The flag is set only in SPIRMetadataTranslationPass which
  // is run in above mpm.run statement. The downside of calling
  // this function here, as opposed to beginning of this function,
  // is that unreferenced constants will be eliminated. So
  // debugger will not be able to query those variables.
  insertOCLMissingDebugConstMetadata(pContext);

  COMPILER_TIME_END(pContext, TIME_UnificationPasses);

  DumpLLVMIR(pContext, "afterUnification");

  MEM_SNAPSHOT(IGC::SMS_AFTER_UNIFICATION);
}

void UnifyIROCL(OpenCLProgramContext *pContext) { CommonOCLBasedPasses(pContext); }

void UnifyIRSPIR(OpenCLProgramContext *pContext) { CommonOCLBasedPasses(pContext); }

} // namespace IGC
