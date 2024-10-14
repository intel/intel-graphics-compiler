/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include <chrono>
#include <iostream>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/ScaledNumber.h>
#include "llvm/ADT/PostOrderIterator.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/Analysis/TargetLibraryInfo.h>

#include <llvmWrapper/Transforms/InstCombine/InstCombineWorklist.h>
#include <llvm/Transforms/Utils.h>

#include "common/LLVMWarningsPop.hpp"

#include "AdaptorCommon/AddImplicitArgs.hpp"
#include "AdaptorCommon/FreezeIntDiv.hpp"
#include "AdaptorCommon/ProcessFuncAttributes.h"
#include "AdaptorCommon/LegalizeFunctionSignatures.h"
#include "AdaptorCommon/TypesLegalizationPass.hpp"
#include "AdaptorCommon/DivergentBarrierPass.h"
#include "common/LLVMUtils.h"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/CISACodeGen/FixAddrSpaceCast.h"
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
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryUsageAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ResourceAllocator/ResourceAllocator.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakConstantExpr/BreakConstantExpr.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LocalBuffers/InlineLocalsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ReplaceUnsupportedIntrinsics/ReplaceUnsupportedIntrinsics.hpp"
#include "Compiler/Optimizer/OpenCLPasses/Atomics/ResolveOCLAtomics.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WGFuncs/WGFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AlignmentAnalysis/AlignmentAnalysis.hpp"
#include "Compiler/Optimizer/PreCompiledFuncImport.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/OpenCLPrintf/OpenCLPrintfResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AggregateArguments/AggregateArguments.hpp"
#include "Compiler/Optimizer/OCLBIConverter.h"
#include "Compiler/Optimizer/OpenCLPasses/SetFastMathFlags/SetFastMathFlags.hpp"
#include "Compiler/Optimizer/OpenCLPasses/CorrectlyRoundedDivSqrt/CorrectlyRoundedDivSqrt.hpp"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericAddressDynamicResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AddressSpaceAliasAnalysis/AddressSpaceAliasAnalysis.h"
#include "Compiler/Optimizer/OpenCLPasses/DeviceEnqueueFuncs/DeviceEnqueue.hpp"
#include "Compiler/Optimizer/OpenCLPasses/UndefinedReferences/UndefinedReferencesPass.hpp"
#include "Compiler/Optimizer/OpenCLPasses/SubGroupFuncs/SubGroupFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BIFTransforms/BIFTransforms.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakdownIntrinsic/BreakdownIntrinsic.h"
#include "Compiler/Optimizer/OpenCLPasses/TransformUnmaskedFunctionsPass/TransformUnmaskedFunctionsPass.h"
#include "Compiler/Optimizer/OpenCLPasses/StatelessToStateful/StatelessToStateful.hpp"
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
#include "Compiler/Optimizer/OpenCLPasses/BfloatFuncs/BfloatFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/DpasFuncs/DpasFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/LSCFuncs/LSCFuncsResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/NamedBarriers/NamedBarriersResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/JointMatrixFuncsResolutionPass/JointMatrixFuncsResolutionPass.h"
#include "Compiler/Optimizer/OpenCLPasses/RayTracing/ResolveOCLRaytracingBuiltins.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AccuracyDecoratedCallsBiFResolution/AccuracyDecoratedCallsBiFResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ScalarArgAsPointer/ScalarArgAsPointer.hpp"
#include "AdaptorCommon/RayTracing/RayTracingPasses.hpp"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/FixResourcePtr.hpp"
#include "Compiler/InitializePasses.h"
#include "Compiler/MetaDataApi/SpirMetaDataApi.h"
#include "Compiler/Optimizer/ReduceOptPass.hpp"
#include "Compiler/CustomUnsafeOptPass.hpp"
#include "MoveStaticAllocas.h"
#ifdef IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR
#include "preprocess_spvir/PreprocessSPVIR.h"
#include "preprocess_spvir/ConvertUserSemanticDecoratorOnFunctions.h"
#include "preprocess_spvir/PromoteBools.h"
#endif // IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR
#include "preprocess_spvir/HandleSPIRVDecorations/HandleSpirvDecorationMetadata.h"
#include "LowerInvokeSIMD.hpp"
#include "ResolveConstExprCalls.h"
#include "Compiler/Optimizer/IGCInstCombiner/IGCInstructionCombining.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BufferBoundsChecking/BufferBoundsChecking.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BufferBoundsChecking/BufferBoundsCheckingPatcher.hpp"
#include "Compiler/Optimizer/OpenCLPasses/MinimumValidAddressChecking/MinimumValidAddressChecking.hpp"

#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "common/debug/Dump.hpp"
#include "common/MemStats.h"

#include <iStdLib/utility.h>

#include "Compiler/CISACodeGen/DebugInfo.hpp"
#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "Compiler/DebugInfo/ScalarVISAModule.h"
#include "Compiler/DebugInfo/Utils.h"
#include "Compiler/Builtins/BIFFlagCtrl/BIFFlagCtrlResolution.hpp"
#include "DebugInfo/VISADebugEmitter.hpp"

#include <string>
#include <algorithm>

#include <Metrics/IGCMetric.h>

using namespace llvm;
using namespace IGC::IGCMD;
using namespace IGC::Debug;

namespace IGC
{
    int getOCLMajorVersion(const SPIRMD::SpirMetaDataUtils &spirMDUtils)
    {
        int oclMajor = 0, oclMinor = 0;
        if (spirMDUtils.isOpenCLVersionsHasValue())
        {
            SPIRMD::VersionMetaDataHandle oclVersion = spirMDUtils.getOpenCLVersionsItem(0);
            oclMajor = oclVersion->getMajor();
            oclMinor = oclVersion->getMinor();
        }
        else
        {
            if (!spirMDUtils.empty_CompilerOptions())
            {
                // check compiler options
                for (auto i = spirMDUtils.getCompilerOptionsItem(0)->begin(), e = spirMDUtils.getCompilerOptionsItem(0)->end(); i != e; ++i)
                {
                    if (StringRef(*i).startswith("-cl-std=CL") && i->length() >= 13)
                    {
                        oclMajor = i->at(10) - '0';
                        oclMinor = i->at(12) - '0';
                        break;
                    }
                }
            }
            // default is 1.2
            if (!isLegalOCLVersion(oclMajor, oclMinor))
            {
                oclMajor = 1;
                oclMinor = 2;
            }
        }
        return oclMajor;
    }

static void CommonOCLBasedPasses(OpenCLProgramContext* pContext)
{
#if defined( _DEBUG )
    bool brokenDebugInfo = false;
    IGC_ASSERT(nullptr != pContext->getModule());
    IGC_ASSERT(false == llvm::verifyModule(*pContext->getModule(), &dbgs(), &brokenDebugInfo));

    // We ignore incorrect DI for now
    // We used if (false == pContext->m_hasLegacyDebugInfo), instead of passing &brokenDebugInfo earlier
    (void)brokenDebugInfo;
#endif

    COMPILER_TIME_START(pContext, TIME_UnificationPasses);

    pContext->metrics.Init(&pContext->hash,
        pContext->getModule()->getNamedMetadata("llvm.dbg.cu") != nullptr);
    pContext->metrics.CollectFunctions(pContext->getModule());

    unify_opt_PreProcess(pContext);
    pContext->m_checkFastFlagPerInstructionInCustomUnsafeOptPass = true;
    pContext->m_mayHaveUnalignedAddressRegister = true;

    DumpLLVMIR(pContext, "beforeUnification");

    // override the data layout to match Gen HW
    int pointerSize = getPointerSize(*pContext->getModule());
    std::string layoutstr;
    if (pointerSize == 4)
    {
        layoutstr = "e-p:32:32:32";
    }
    else {
        layoutstr = "e-p:64:64:64";
    }
    layoutstr += "-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64"\
        "-f32:32:32-f64:64:64-v16:16:16-v24:32:32"\
        "-v32:32:32-v48:64:64-v64:64:64-v96:128:128"\
        "-v128:128:128-v192:256:256-v256:256:256"\
        "-v512:512:512-v1024:1024:1024-n8:16:32";

    StringRef dataLayout = layoutstr;
    pContext->getModule()->setDataLayout(dataLayout);

    MetaDataUtils *pMdUtils = pContext->getMetaDataUtils();

    //extracting OCL version major before SPIRMetadataTranslation pass deletes its metadata node
    const SPIRMD::SpirMetaDataUtils spirMDUtils(&(*pContext->getModule()));
    int OCLMajor = getOCLMajorVersion(spirMDUtils);

    CompOptions &CompilerOpts = pContext->getModuleMetaData()->compOpt;

    // check OpenCL build options
    bool shouldForceCR = pContext->m_Options.CorrectlyRoundedSqrt;

    CompilerOpts.replaceGlobalOffsetsByZero =
        pContext->m_InternalOptions.replaceGlobalOffsetsByZero;

    CompilerOpts.SubgroupIndependentForwardProgressRequired =
        (pContext->m_Options.NoSubgroupIFP == false);

    if (OCLMajor >= 2)
    {
        CompilerOpts.UniformWGS =
            pContext->m_Options.UniformWGS;
    }

    CompilerOpts.GreaterThan2GBBufferRequired =
        !pContext->m_InternalOptions.Use32BitPtrArith;

    CompilerOpts.GreaterThan4GBBufferRequired =
        (pContext->m_InternalOptions.IntelGreaterThan4GBBufferRequired ||
         pContext->m_Options.IntelGreaterThan4GBBufferRequired);

    CompilerOpts.DisableA64WA =
        pContext->m_InternalOptions.IntelDisableA64WA;

    CompilerOpts.ForceEnableA64WA =
        pContext->m_InternalOptions.IntelForceEnableA64WA;

    CompilerOpts.HasPositivePointerOffset =
        pContext->m_InternalOptions.IntelHasPositivePointerOffset;

    CompilerOpts.HasBufferOffsetArg =
        pContext->m_InternalOptions.IntelHasBufferOffsetArg;

    CompilerOpts.UseBindlessMode =
        pContext->m_InternalOptions.UseBindlessMode;

    CompilerOpts.UseLegacyBindlessMode =
        pContext->m_InternalOptions.UseBindlessLegacyMode;

    CompilerOpts.PreferBindlessImages =
        pContext->m_InternalOptions.PreferBindlessImages ||
        pContext->m_InternalOptions.UseBindlessMode;

    if (CompilerOpts.PreferBindlessImages) {
        pContext->getModuleMetaData()->UseBindlessImage = true;
    }
    CompilerOpts.ForceInt32DivRemEmu =
        pContext->m_InternalOptions.IntelForceInt32DivRemEmu;

    CompilerOpts.ForceInt32DivRemEmuSP =
        pContext->m_InternalOptions.IntelForceInt32DivRemEmuSP;

    CompilerOpts.EnableTakeGlobalAddress =
        pContext->m_Options.EnableTakeGlobalAddress;

    CompilerOpts.IsLibraryCompilation =
        pContext->m_Options.IsLibraryCompilation;

    CompilerOpts.LibraryCompileSIMDSize =
        pContext->m_Options.LibraryCompileSIMDSize;

    CompilerOpts.ExcludeIRFromZEBinary =
        pContext->m_InternalOptions.ExcludeIRFromZEBinary;

    CompilerOpts.EmitZeBinVISASections =
        pContext->m_InternalOptions.EmitZeBinVISASections;

    CompilerOpts.FP64GenEmulationEnabled =
        pContext->m_InternalOptions.EnableFP64GenEmu ||
        pContext->m_Options.EnableFP64GenEmu;

    CompilerOpts.FP64GenConvEmulationEnabled =
        pContext->m_Options.EnableFP64GenConvEmu;

    CompilerOpts.LoadCacheDefault =
        pContext->m_InternalOptions.LoadCacheDefault;

    CompilerOpts.StoreCacheDefault =
        pContext->m_InternalOptions.StoreCacheDefault;

    IGCPassManager mpmSPIR(pContext, "Unify");
#ifdef IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR
    mpmSPIR.add(new PreprocessSPVIR());
    mpmSPIR.add(new PromoteBools());
#endif // IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR
    mpmSPIR.add(new TypesLegalizationPass());
    mpmSPIR.add(new TargetLibraryInfoWrapperPass());
    mpmSPIR.add(new MetaDataUtilsWrapper(pMdUtils, pContext->getModuleMetaData()));
    mpmSPIR.add(new CodeGenContextWrapper(pContext));
    mpmSPIR.add(new SPIRMetaDataTranslation());
#ifdef IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR
    mpmSPIR.add(new ConvertUserSemanticDecoratorOnFunctions());
#endif // IGC_SCALAR_USE_KHRONOS_SPIRV_TRANSLATOR
    mpmSPIR.add(new HandleSpirvDecorationMetadata());
    mpmSPIR.add(createDeadCodeEliminationPass());
    mpmSPIR.run(*pContext->getModule());

    bool isOptDisabled = CompilerOpts.OptDisable;
    IGCPassManager mpm(pContext, "Unify");

    // right now we don't support any standard function in the code gen
    // maybe we want to support some at some point to take advantage of LLVM optimizations
    TargetLibraryInfoImpl TLI;
    TLI.disableAllFunctions();

    mpm.add( new llvm::TargetLibraryInfoWrapperPass(TLI));

    mpm.add(new MetaDataUtilsWrapper(pMdUtils, pContext->getModuleMetaData()));
    mpm.add(new CodeGenContextWrapper(pContext));

    if (IGC_IS_FLAG_ENABLED(EnableUnmaskedFunctions))
    {
        mpm.add(new TransformUnmaskedFunctionsPass());
    }

    mpm.add(new AccuracyDecoratedCallsBiFResolution());
    mpm.add(new MoveStaticAllocas());

    // Clone kernel function being used as user function.
    mpm.add(createKernelFunctionCloningPass());

    mpm.add(new CorrectlyRoundedDivSqrt(shouldForceCR, false));
    if(IGC_IS_FLAG_ENABLED(EnableIntelFast))
    {
        mpm.add(createBIFTransformsPass());
    }

    if (IGC_IS_FLAG_ENABLED(EnableCodeAssumption))
    {
        mpm.add(new CodeAssumption());
    }

    if (pContext->m_instrTypes.hasFRem)
    {
        mpm.add(new HandleFRemInstructions());
    }

    mpm.add(new JointMatrixFuncsResolutionPass());

    mpm.add(new ReduceOptPass());

    mpm.add(new HandleDevicelibAssert());
    if (IGC_IS_FLAG_ENABLED(StackOverflowDetection)) {
        mpm.add(new StackOverflowDetectionPass(StackOverflowDetectionPass::Mode::Initialize));
    }

    if (IGC_IS_FLAG_ENABLED(BufferBoundsChecking) || pContext->isBufferBoundsChecking())
    {
        mpm.add(new BufferBoundsChecking());
    }

    // Minimum valid address checking
    {
        uint64_t minimumValidAddress = IGC_GET_FLAG_VALUE(MinimumValidAddress);
        if (!minimumValidAddress)
        {
            minimumValidAddress = pContext->getMinimumValidAddress();
        }
        if (minimumValidAddress)
        {
            mpm.add(new MinimumValidAddressChecking(minimumValidAddress));
        }
    }

    mpm.add(new NamedBarriersResolution(pContext->platform.getPlatformInfo().eRenderCoreFamily));
    mpm.add(new PreBIImportAnalysis());
    mpm.add(createTimeStatsCounterPass(pContext, TIME_Unify_BuiltinImport, STATS_COUNTER_START));
    mpm.add(createBuiltInImportPass());
    mpm.add(createTimeStatsCounterPass(pContext, TIME_Unify_BuiltinImport, STATS_COUNTER_END));
    mpm.add(new BIFFlagCtrlResolution(pContext));

    if (IGC_GET_FLAG_VALUE(AllowMem2Reg))
    {
        mpm.add(createPromoteMemoryToRegisterPass());
    }

    // OCL has built-ins so it always need to run inlining
    {
        // We need to propagate constexpr casts to resolve pseudo indirect calls
        mpm.add(createSCCPPass());
        mpm.add(new ResolveConstExprCalls());

        // Estimate maximal function size in the module and disable subroutine
        // if not profitable.
        mpm.add(createEstimateFunctionSizePass(
            pContext->m_Options.StaticProfileGuidedTrimming));
        mpm.add(createProcessFuncAttributesPass());
        FastMathFlags Mask;
        Mask.setFast();
        Mask.setNoSignedZeros(false);
        mpm.add(new SetFastMathFlags(Mask, true));

        // mark load and stores inside assert calls as nontemporal to avoid caching.
        mpm.add(new NontemporalLoadsAndStoresInAssert());

        // Report undef references after setting func attribs for import linking
        mpm.add(new UndefinedReferencesPass());

        if (!IGC::ForceAlwaysInline(pContext))
        {
            int Threshold = IGC_GET_FLAG_VALUE(OCLInlineThreshold);
            mpm.add(createFunctionInliningPass(Threshold));
        }
        else
        {
            mpm.add(createAlwaysInlinerLegacyPass());
        }
        // The inliner sometimes fails to delete unused functions, this cleans up the remaining mess.
        mpm.add(createGlobalDCEPass());

        // Check after GlobalDCE in case of doubles in dead functions
        mpm.add(new ErrorCheck());

        mpm.add(new LowerInvokeSIMD());

        // Run BreakConstantExpr right before LegalizeFunctionSignatures, since
        // input module may contain call sites with bitcast constant expressions
        mpm.add(new BreakConstantExpr());

        // Fix illegal argument/return types in function calls not already inlined.
        // This pass changes all illegal function signatures to be passed by pointer instead.
        mpm.add(new LegalizeFunctionSignatures());

        mpm.add(createProcessBuiltinMetaDataPass());
        mpm.add(new PurgeMetaDataUtils());
    }

    // OpenCL WI + image function resolution

    // OCLTODO : do another DCE that will get rid of unused WI func calls before this?
    // We can save passing of unused implicit args from the runtime

    // Adding Mem2Reg pass in order to help ImageFuncsAnalysis to identify the image arguments
    // that the image functions operate on
    // Clang output is: alloca --> store image func arg into allocated address -->
    //                  load image arg from stored address --> call func on loaded image
    // After Mem2Reg: call func on image func arg

    mpm.add(createSROAPass());

    mpm.add(new BreakConstantExpr());

    if (IGC_IS_FLAG_ENABLED(EnableGASResolver))
    {
        // InferAddressSpaces pass requires TTI analysis, but it doesn't initialize it
        // as a dependency. Let's run it manually until IGC is switched to LLVM 14.
        mpm.add(createTargetTransformInfoWrapperPass(TargetIRAnalysis()));
        // Run InferAddressSpaces pass first - to propagate named addrspaces
        // through elementary LLVM instructions, then run custom ResolveGAS
        // pass to handle IGC specific instructions, like builtins etc.
        mpm.add(createInferAddressSpacesPass(ADDRESS_SPACE_GENERIC));

        // Add fix up of illegal `addrspacecast` in respect to OCL 2.0 spec.
        mpm.add(createFixAddrSpaceCastPass());
        mpm.add(createResolveGASPass());

        if (IGC_IS_FLAG_ENABLED(EnableLowerGPCallArg))
        {
            mpm.add(createLowerGPCallArg());
        }
        mpm.add(createStaticGASResolution());

        mpm.add(createGASRetValuePropagatorPass());

        // this pass is intended to inline the remaining always inline functions that had issues
        // with argument address spaces (byVal addrspace(4)) in the previous attempt
        mpm.add(createAlwaysInlinerLegacyPass());
        mpm.add(new PurgeMetaDataUtils());

        // Run another round of constant breaking as GAS resolving may generate constants (constant address)
        mpm.add(new BreakConstantExpr());
    }

    if (CompilerOpts.UniformWGS)
        mpm.add(new RewriteLocalSize());

    mpm.add(createSROAPass());
    mpm.add(new BreakConstantExpr());

    mpm.add(CreateFoldKnownWorkGroupSizes());

    mpm.add(new ResolveSampledImageBuiltins());

    // 64-bit atomics have to be resolved before AddImplicitArgs pass as it uses
    // local ids for spin lock initialization
    mpm.add(new ResolveOCLAtomics());

    // Run the AlignmentAnalysis pass before the passes which add implicit arguments, to ensure we do not lose load/store alignment information.
    // For example, ProgramScopeConstantResolution will relocate the buffer's base to an i8* typed pointer.
    mpm.add(new AlignmentAnalysis());

    // Analysis passes
    mpm.add(new WIFuncsAnalysis());
    mpm.add(new ImageFuncsAnalysis());
    mpm.add(new OpenCLPrintfAnalysis());
    mpm.add(createDeadCodeEliminationPass());
    mpm.add(new ProgramScopeConstantAnalysis());
    mpm.add(new PrivateMemoryUsageAnalysis());
    mpm.add(new AggregateArgumentsAnalysis());
    mpm.add(new ExtensionFuncsAnalysis());
    mpm.add(new ExtensionArgAnalysis());
    mpm.add(new DeviceEnqueueFuncsAnalysis());

    mpm.add(new BuiltinCallGraphAnalysis());

    mpm.add(new ResolveOCLRaytracingBuiltins());
    mpm.add(createRayTracingIntrinsicAnalysisPass());
    // Adding implicit args based on Analysis passes
    mpm.add(new AddImplicitArgs());

    if (IGC_IS_FLAG_ENABLED(BufferBoundsChecking) || pContext->isBufferBoundsChecking())
    {
        mpm.add(new BufferBoundsCheckingPatcher());
    }

    mpm.add(createRayTracingIntrinsicResolutionPass());

    // Resolution passes
    mpm.add(new WIFuncResolution());
    mpm.add(new OpenCLPrintfResolution());
    mpm.add(new ResolveOCLAtomics());
    mpm.add(new ResourceAllocator());
    mpm.add(new SubGroupFuncsResolution());
    mpm.add(new BfloatFuncsResolution());
    mpm.add(createDpasFuncsResolutionPass());
    mpm.add(createLSCFuncsResolutionPass());

    mpm.add(new WGFuncResolution());
    mpm.add(new ResolveAggregateArguments());
    mpm.add(new ExtensionFuncsResolution());
    mpm.add(new DeviceEnqueueFuncsResolution());

    mpm.add(createDeadCodeEliminationPass());

    mpm.add(createBuiltinsConverterPass());

    if (pContext->needsDivergentBarrierHandling())
    {
        mpm.add(createDivergentBarrierPass(nullptr));
        mpm.add(new BreakConstantExpr());
    }

    mpm.add(new InlineLocalsResolution());

    // check for unsupported intrinsics
    mpm.add(new ErrorCheck());
    if (pContext->m_Options.EnableUnsupportedFP64Poisoning) {
        mpm.add(new PoisonFP64Kernels());
        mpm.add(new PurgeMetaDataUtils());
    }

    mpm.add(new ImageFuncResolution());
    mpm.add(new Image3dToImage2darray());

    // Break down the intrinsics into smaller operations (eg. fmuladd to fmul add)
    mpm.add(new BreakdownIntrinsicPass());

    {
        if(IGC_IS_FLAG_ENABLED(EnableConstantPromotion))
        {
            mpm.add(createSimplifyConstantPass());
            mpm.add(createPromoteConstantPass());
        }
        // For LLVM 14+, make sure to freeze potential UB-causing instructions
        // before running InstructionCombining that would propagate resulting
        // poison/undef values.
        mpm.add(createFreezeIntDivPass());
        mpm.add(createIGCInstructionCombiningPass());

        // Instcombine can create constant expressions, which are not handled by the program scope constant resolution pass
        mpm.add(new BreakConstantExpr());

        // Run constant lowering conservatively for tests where constant
        // objects are over-written after casting pointers in constant address
        // space into ones in private address.
        //
        // NOTE: Per OpenCL C standard (both 1.2 and 2.0), that's illegal.
        //
        // This has to be run after instcombine to allow memcpy from GlobalVariable arrays private
        // allocs to be optimized away.
        mpm.add(new ProgramScopeConstantResolution(true));
    }

    // TODO: Run CheckInstrTypes after builtin import to determine if builtins have allocas.
    mpm.add(createSROAPass());
    mpm.add(createIGCInstructionCombiningPass());
    // See the comment above (it's copied as is).
    // Instcombine can create constant expressions, which are not handled by the program scope constant resolution pass.
    // For example, in InsertDummyKernelForSymbolTablePass addresses of indirectly called functions
    // should be processed and without BreakConstantExpr the addresses are not found.
    mpm.add(new BreakConstantExpr());

    mpm.add(new ScalarArgAsPointerAnalysis());

    if (IGC_IS_FLAG_DISABLED(DisableOCLScalarizer))
    {
        mpm.add(createScalarizerPass(SelectiveScalarizer::Auto));
    }

    // Create a dummy kernel to attach the symbol table if necessary
    // Only needed if function pointers, externally linked functions, or relocatable global variables are present
    mpm.add(createInsertDummyKernelForSymbolTablePass());

    FastMathFlags Mask;
    Mask.setNoSignedZeros(true);
    mpm.add(new SetFastMathFlags(Mask, true));
    mpm.add(new FixResourcePtr());

    if(isOptDisabled)
    {
        // Run additional predefined constant resolving when optimization is
        // disabled. It's definitely a workaround so far.
        mpm.add(createResolvePredefinedConstantPass());
    }

    mpm.add(createLowerSwitchPass());
    mpm.add(createTypeLegalizerPass());

    mpm.add(new CatchAllLineNumber());

    mpm.run(*pContext->getModule());

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

void UnifyIROCL(OpenCLProgramContext* pContext)
{
    CommonOCLBasedPasses(pContext);
}

void UnifyIRSPIR(OpenCLProgramContext* pContext)
{
    CommonOCLBasedPasses(pContext);
}

}
