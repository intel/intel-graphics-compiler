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

#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/Legalizer/PeepholeTypeLegalizer.hpp"
#include "Compiler/CISACodeGen/layout.hpp"
#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/CISACodeGen/GeometryShaderLowering.hpp"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "Compiler/CISACodeGen/PixelShaderLowering.hpp"
#include "Compiler/CISACodeGen/VertexShaderLowering.hpp"
#include "Compiler/CISACodeGen/HullShaderLowering.hpp"
#include "Compiler/CISACodeGen/HullShaderClearTessFactors.hpp"
#include "Compiler/CISACodeGen/DomainShaderLowering.hpp"
#include "Compiler/CISACodeGen/AdvCodeMotion.h"
#include "Compiler/CISACodeGen/AdvMemOpt.h"
#include "Compiler/CISACodeGen/Emu64OpsPass.h"
#include "Compiler/CISACodeGen/PullConstantHeuristics.hpp"
#include "Compiler/CISACodeGen/PushAnalysis.hpp"
#include "Compiler/CISACodeGen/ScalarizerCodeGen.hpp"
#include "Compiler/CISACodeGen/CodeSinking.hpp"
#include "Compiler/CISACodeGen/CodeHoisting.hpp"
#include "Compiler/CISACodeGen/ConstantCoalescing.hpp"
#include "Compiler/CISACodeGen/CheckInstrTypes.hpp"
#include "Compiler/CISACodeGen/EstimateFunctionSize.h"
#include "Compiler/CISACodeGen/PassTimer.hpp"
#include "Compiler/CISACodeGen/FixAddrSpaceCast.h"
#include "Compiler/CISACodeGen/FixupExtractValuePair.h"
#include "Compiler/CISACodeGen/GenIRLowering.h"
#include "Compiler/CISACodeGen/GenSimplification.h"
#include "Compiler/CISACodeGen/LoopDCE.h"
#include "Compiler/CISACodeGen/LowerGSInterface.h"
#include "Compiler/CISACodeGen/LdShrink.h"
#include "Compiler/CISACodeGen/MemOpt.h"
#include "Compiler/CISACodeGen/MemOpt2.h"
#include "Compiler/CISACodeGen/PreRARematFlag.h"
#include "Compiler/CISACodeGen/PreRAScheduler.hpp"
#include "Compiler/CISACodeGen/ResolveGAS.h"
#include "Compiler/CISACodeGen/ResolvePredefinedConstant.h"
#include "Compiler/CISACodeGen/Simd32Profitability.hpp"
#include "Compiler/CISACodeGen/SimplifyConstant.h"
#include "Compiler/CISACodeGen/TimeStatsCounter.h"
#include "Compiler/CISACodeGen/TypeDemote.h"
#include "Compiler/CISACodeGen/UniformAssumptions.hpp"
#include "Compiler/Optimizer/LinkMultiRateShaders.hpp"
#include "Compiler/CISACodeGen/MergeURBWrites.hpp"
#include "Compiler/CISACodeGen/VectorProcess.hpp"
#include "Compiler/CISACodeGen/LowerGEPForPrivMem.hpp"
#include "Compiler/CISACodeGen/POSH_RemoveNonPositionOutput.h"
#include "Compiler/CISACodeGen/RegisterEstimator.hpp"
#include "Compiler/CISACodeGen/ComputeShaderLowering.hpp"
#include "Compiler/CISACodeGen/CrossPhaseConstProp.hpp"
#include "Compiler/CISACodeGen/SLMConstProp.hpp"
#include "Compiler/Optimizer/OpenCLPasses/GenericAddressResolution/GenericAddressDynamicResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryUsageAnalysis.hpp"
#include "Compiler/Optimizer/OpenCLPasses/PrivateMemory/PrivateMemoryResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ProgramScopeConstants/ProgramScopeConstantResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/BreakConstantExpr/BreakConstantExpr.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ReplaceUnsupportedIntrinsics/ReplaceUnsupportedIntrinsics.hpp"
#include "Compiler/Optimizer/PreCompiledFuncImport.hpp"
#include "Compiler/Optimizer/OpenCLPasses/AddressSpaceAliasAnalysis/AddressSpaceAliasAnalysis.h"
#include "Compiler/Optimizer/OpenCLPasses/UndefinedReferences/UndefinedReferencesPass.hpp"
#include "Compiler/Optimizer/OpenCLPasses/StatelessToStatefull/StatelessToStatefull.hpp"
#include "Compiler/Optimizer/OpenCLPasses/DisableLoopUnrollOnRetry/DisableLoopUnrollOnRetry.hpp"
#include "Compiler/Optimizer/MCSOptimization.hpp"
#include "Compiler/Optimizer/RectListOptimizationPass.hpp"
#include "Compiler/Optimizer/GatingSimilarSamples.hpp"
#include "Compiler/Optimizer/IntDivConstantReduction.hpp"
#include "Compiler/Optimizer/IntDivRemCombine.hpp"
#include "Compiler/MetaDataApi/PurgeMetaDataUtils.hpp"
#include "Compiler/HandleLoadStoreInstructions.hpp"
#include "Compiler/CustomSafeOptPass.hpp"
#include "Compiler/CustomUnsafeOptPass.hpp"
#include "Compiler/CustomLoopOpt.hpp"
#include "Compiler/GenUpdateCB.h"
#include "Compiler/PromoteResourceToDirectAS.h"
#include "Compiler/PromoteStatelessToBindless.h"
#if defined( _DEBUG ) && !defined( ANDROID )
#include "Compiler/VerificationPass.hpp"
#endif
#include "Compiler/LegalizationPass.hpp"
#include "Compiler/LowPrecisionOptPass.hpp"
#include "Compiler/WorkaroundAnalysisPass.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/IGCMetaDataHelper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/FindInterestingConstants.h"
#include "Compiler/DynamicTextureFolding.h"
#include "Compiler/SampleMultiversioning.hpp"
#include "Compiler/ThreadCombining.hpp"
#include "Compiler/InitializePasses.h"
#include "Compiler/Optimizer/Scalarizer.h"
#include "common/debug/Debug.hpp"
#include "common/igc_regkeys.hpp"
#include "common/debug/Dump.hpp"
#include "common/MemStats.h"
#include <iStdLib/utility.h>
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/PostOrderIterator.h"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Pass.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/AlwaysInliner.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/IPO/FunctionAttrs.h>
#include <llvmWrapper/Transforms/Utils.h>
#include <llvmWrapper/Transforms/Scalar/InstSimplifyPass.h>
#include <llvmWrapper/Transforms/Scalar.h>
#include <llvmWrapper/Bitcode/BitcodeWriter.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include "common/LLVMWarningsPop.hpp"
#include <sstream>
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/CISACodeGen/EmitVISAPass.hpp"
#include "Compiler/CISACodeGen/CoalescingEngine.hpp"
#include "Compiler/GenTTI.h"
#include "Compiler/Optimizer/SetMathPrecisionForPositionOutput.hpp"
#include "Compiler/DebugInfo/VISADebugEmitter.hpp"
#include "Compiler/SampleCmpToDiscard.h"
#include "Compiler/Optimizer/IGCInstCombiner/IGCInstructionCombining.hpp"
#include "DebugInfo.hpp"
#include "Compiler/CISACodeGen/HalfPromotion.h"
#include "Compiler/CISACodeGen/AnnotateUniformAllocas.h"
#include "Probe/Assertion.h"

/***********************************************************************************
This file contains the generic code generation functions for all the shaders
The class CShader is inherited for each specific type of shaders to add specific
information
************************************************************************************/

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace IGC::Debug;

namespace IGC
{
const int LOOP_ROTATION_HEADER_INST_THRESHOLD = 32;
const int LOOP_NUM_THRESHOLD = 2000;
const int LOOP_INST_THRESHOLD = 65000;
const int INST_THRESHOLD = 80000;

static void AddURBWriteRelatedPass(CodeGenContext& ctx, IGCPassManager& mpm)
{
// 3D MergeURBWrite pass
    switch (ctx.type)
{
case ShaderType::GEOMETRY_SHADER:
case ShaderType::VERTEX_SHADER:
case ShaderType::HULL_SHADER:
case ShaderType::DOMAIN_SHADER:
    if (IGC_IS_FLAG_DISABLED(DisableURBWriteMerge))
    {
        mpm.add(createMergeURBWritesPass());

        if (IGC_IS_FLAG_ENABLED(EnableTEFactorsClear) && (ctx.type == ShaderType::HULL_SHADER))
        {
            mpm.add(createClearTessFactorsPass());
        }
    }
    if (IGC_IS_FLAG_DISABLED(DisableCodeHoisting))
    {
        mpm.add(new CodeHoisting());
    }
    break;
default:
    break;
}
}

static void AddAnalysisPasses(CodeGenContext& ctx, IGCPassManager& mpm)
{
    COMPILER_TIME_START(&ctx, TIME_CG_Add_Analysis_Passes);

    bool isOptDisabled = ctx.getModuleMetaData()->compOpt.OptDisable;
    TODO("remove the following once all IGC passes are registered to PassRegistery in their constructor")
    initializeLoopInfoWrapperPassPass(*PassRegistry::getPassRegistry());

    mpm.add(createTimeStatsCounterPass(&ctx, TIME_CG_Analysis, STATS_COUNTER_START));

    // transform pull constants and inputs into push constants and inputs
    mpm.add(new PushAnalysis());
    mpm.add(CreateSampleCmpToDiscardPass());

    if (!isOptDisabled)
    {
        mpm.add(llvm::createDeadCodeEliminationPass());
    }

    // The 1st thing we do when getting into the IGC middle end is to split critical-edges:
    // PushAnalysis requires WIAnalysis
    // WIAnalysis requires dominator and post-dominator analysis
    // WIAnalysis also requires BreakCriticalEdge because it assumes that
    // potential phi-moves will be placed at those blocks
    mpm.add(llvm::createBreakCriticalEdgesPass());

    // 3D MergeURBWrite pass should be added after PushAnalysis and DeadCodeElimination
    // to avoid URBRead/URBWrite interference
    AddURBWriteRelatedPass(ctx, mpm);

    // moving the scheduling and sample clustering passes right before code-sinking.
    // Need to merge the scheduling, code-sinking and clustering passes better to avoid redundancy and better optimization
    if (IGC_IS_FLAG_DISABLED(DisablePreRAScheduler) &&
        ctx.type == ShaderType::PIXEL_SHADER &&
        ctx.m_retryManager.AllowPreRAScheduler() &&
        !ctx.m_enableSubroutine)
    {
        mpm.add(createPreRASchedulerPass());
    }

    if (IGC_IS_FLAG_DISABLED(DisableMemOpt2) &&
        (ctx.type == ShaderType::COMPUTE_SHADER || (ctx.m_DriverInfo.WAEnableMemOpt2ForOCL())) &&
        !isOptDisabled)
    {
        mpm.add(createMemOpt2Pass(16));
    }

    // only limited code-sinking to several shader-type
    // vs input has the URB-reuse issue to be resolved.
    // Also need to understand the performance benefit better.
    mpm.add(new CodeSinking(true));

    if (ctx.type == ShaderType::PIXEL_SHADER)
        mpm.add(new PixelShaderAddMask());

    // Run flag re-materialization if it's beneficial.
    if (ctx.m_DriverInfo.benefitFromPreRARematFlag() &&
        IGC_IS_FLAG_ENABLED(EnablePreRARematFlag)) {
        mpm.add(createPreRARematFlagPass());
    }
    // Peephole framework for generic type legalization
    mpm.add(new Legalizer::PeepholeTypeLegalizer());

    // need this before WIAnalysis:
    // insert phi to prevent changing of WIAnalysis result by later code-motion
    mpm.add(llvm::createLCSSAPass());
        if (!isOptDisabled)
    {
        // If you want to clean up the dead-code after push optimization
        // and IOCoalescing
        //Passes.add(llvm::createDeadCodeEliminationPass());
    }
    // Fixup extract value pairs.
    mpm.add(createExtractValuePairFixupPass());

    // This is for dumping register pressure info
    if (IGC_IS_FLAG_ENABLED(ForceRPE)) {
        mpm.add(new RegisterEstimator());
    }

    mpm.add(new Layout());

    mpm.add(createTimeStatsCounterPass(&ctx, TIME_CG_Analysis, STATS_COUNTER_END));

    COMPILER_TIME_END(&ctx, TIME_CG_Add_Analysis_Passes);
} // AddAnalysisPasses

static void UpdateInstTypeHint(CodeGenContext& ctx)
{
    // WA: save original values as preRA heuristic is based on those
    // we need to fix the preRA pass heuristic or get rid of preRA pass altogether
    unsigned int numBB = ctx.m_instrTypes.numBB;
    unsigned int numSample = ctx.m_instrTypes.numSample;
    unsigned int numInsts = ctx.m_instrTypes.numInsts;
    IGCPassManager mpm(&ctx, "UpdateOptPre");
    mpm.add(new CheckInstrTypes(&(ctx.m_instrTypes)));
    mpm.run(*ctx.getModule());
    ctx.m_instrTypes.numBB = numBB;
    ctx.m_instrTypes.numSample = numSample;
    ctx.m_instrTypes.numInsts = numInsts;
    ctx.m_instrTypes.hasLoadStore = true;
}

// forward declaration
llvm::ModulePass* createPruneUnusedArgumentsPass();

static void AddLegalizationPasses(CodeGenContext& ctx, IGCPassManager& mpm, PSSignature* pSignature=nullptr)
{
    COMPILER_TIME_START(&ctx, TIME_CG_Add_Legalization_Passes);

    mpm.add(createTimeStatsCounterPass(&ctx, TIME_CG_Legalization, STATS_COUNTER_START));

    // update type of instructions to know what passes are needed.
    UpdateInstTypeHint(ctx);

    MetaDataUtils* pMdUtils = ctx.getMetaDataUtils();
    bool isOptDisabled = ctx.getModuleMetaData()->compOpt.OptDisable;
    bool fastCompile = ctx.getModuleMetaData()->compOpt.FastCompilation;

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
    //Add alias analysis pass
    mpm.add(createAddressSpaceAAWrapperPass());
    mpm.add(createExternalAAWrapperPass(&addAddressSpaceAAResult));

    TODO("remove the following once all IGC passes are registered to PassRegistery in their constructor")
    initializeWIAnalysisPass(*PassRegistry::getPassRegistry());
    initializeSimd32ProfitabilityAnalysisPass(*PassRegistry::getPassRegistry());
    initializeGenXFunctionGroupAnalysisPass(*PassRegistry::getPassRegistry());


    if (ctx.type == ShaderType::PIXEL_SHADER)
    {
        mpm.add(new DiscardLowering());
        mpm.add(createPromoteMemoryToRegisterPass());
        if (!isOptDisabled)
        {
            if (pSignature)
            {
                mpm.add(createCrossPhaseConstPropPass(pSignature));
            }
            mpm.add(llvm::createCFGSimplificationPass());
            mpm.add(llvm::createLowerSwitchPass());
        }
    }

    if (ctx.m_threadCombiningOptDone)
    {
        mpm.add(createLoopCanonicalization());
        mpm.add(llvm::createLoopDeletionPass());
        mpm.add(llvm::createBreakCriticalEdgesPass());
        mpm.add(llvm::createLoopRotatePass(LOOP_ROTATION_HEADER_INST_THRESHOLD));
        mpm.add(llvm::createLowerSwitchPass());

        int LoopUnrollThreshold = ctx.m_DriverInfo.GetLoopUnrollThreshold();

        if (LoopUnrollThreshold > 0 && (ctx.m_tempCount < 64))
        {
            mpm.add(IGCLLVM::createLoopUnrollPass(2, LoopUnrollThreshold, -1, 1));
        }

        mpm.add(createBarrierNoopPass());

        if (ctx.m_retryManager.AllowLICM() && IGC_IS_FLAG_ENABLED(allowLICM))
        {
            mpm.add(llvm::createLICMPass());
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

    bool needDPEmu = (IGC_IS_FLAG_ENABLED(ForceDPEmulation) ||
        (ctx.m_DriverInfo.NeedFP64(ctx.platform.getPlatformInfo().eProductFamily) && ctx.platform.hasNoFP64Inst()));
    uint32_t theEmuKind = (needDPEmu ? EmuKind::EMU_DP : 0);
    theEmuKind |= (ctx.m_DriverInfo.NeedI64BitDivRem() ? EmuKind::EMU_I64DIVREM : 0);
    theEmuKind |=
        ((IGC_IS_FLAG_ENABLED(ForceSPDivEmulation) ||
            (ctx.m_DriverInfo.NeedIEEESPDiv() && !ctx.platform.hasCorrectlyRoundedMacros()))
        ? EmuKind::EMU_SP_DIV : 0);

    if (theEmuKind > 0 || IGC_IS_FLAG_ENABLED(EnableTestIGCBuiltin))
    {
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
        mpm.add(llvm::createAggressiveDCEPass());
        // TODO: we probably should be running other passes on the result

        if (IGC_GET_FLAG_VALUE(FunctionControl) != FLAG_FCALL_FORCE_INLINE)
        {
            mpm.add(new PurgeMetaDataUtils());
        }
    }

    mpm.add(new ReplaceUnsupportedIntrinsics());

    if (IGC_IS_FLAG_DISABLED(DisablePromoteToDirectAS) &&
        !ctx.getModuleMetaData()->compOpt.IsLibraryCompilation)
    {
        // Promotes indirect resource access to direct
        mpm.add(new BreakConstantExpr());
        mpm.add(new PromoteResourceToDirectAS());
    }

    if (ctx.m_instrTypes.hasReadOnlyArray)
    {
        mpm.add(createDeadCodeEliminationPass());
        mpm.add(createSROAPass());
    }

    if (ctx.m_instrTypes.hasGenericAddressSpacePointers)
    {
        if (!isOptDisabled &&
            IGC_IS_FLAG_ENABLED(EnableGASResolver))
        {
            mpm.add(createFixAddrSpaceCastPass());
            mpm.add(createResolveGASPass());
            mpm.add(createSROAPass());
        }
        mpm.add(createGenericAddressDynamicResolutionPass());
    }

    // Resolve the Private memory to register pass
    if (!isOptDisabled)
    {
        if (ctx.m_instrTypes.hasNonPrimitiveAlloca)
        {
            mpm.add(createBreakCriticalEdgesPass());
            mpm.add(createAnnotateUniformAllocasPass());

            if (IGC_IS_FLAG_DISABLED(DisablePromotePrivMem) &&
                ctx.m_retryManager.AllowPromotePrivateMemory())
            {
                mpm.add(createPromotePrivateArrayToReg());
                mpm.add(createCFGSimplificationPass());
            }
        }
        mpm.add(createPromoteMemoryToRegisterPass());
    }
    else
    {
            if (IGC_IS_FLAG_ENABLED(AllowMem2Reg))
            mpm.add(createPromoteMemoryToRegisterPass());
    }

    if (ctx.m_instrTypes.hasLoop)
    {
        // need to run loop simplify to canonicalize loop and merge latches
        mpm.add(createLoopCanonicalization());
        mpm.add(createLoopSimplifyPass());
    }

    if  (ctx.enableFunctionCall()
        )
    {
        // Sort functions if subroutine/indirect fcall is enabled.
        mpm.add(llvm::createGlobalDCEPass());
        mpm.add(new PurgeMetaDataUtils());
        mpm.add(createGenXCodeGenModulePass());
    }

    // Resolving private memory allocas
    mpm.add(CreatePrivateMemoryResolution());

    // Run MemOpt
    if (!isOptDisabled &&
        ctx.m_instrTypes.hasLoadStore && IGC_IS_FLAG_DISABLED(DisableMemOpt)) {
        if (IGC_IS_FLAG_ENABLED(EnableAdvMemOpt))
            mpm.add(createAdvMemOptPass());
        bool AllowNegativeSymPtrsForLoad =
            ctx.type == ShaderType::OPENCL_SHADER;
        mpm.add(createMemOptPass(AllowNegativeSymPtrsForLoad));
        mpm.add(createIGCInstructionCombiningPass());
    }

    if (ctx.type == ShaderType::OPENCL_SHADER &&
        static_cast<OpenCLProgramContext&>(ctx).
            m_InternalOptions.PromoteStatelessToBindless)
    {
        mpm.add(new PromoteStatelessToBindless());
    }

    if (!isOptDisabled &&
        ctx.m_instrTypes.hasLoadStore &&
        ctx.m_DriverInfo.SupportsStatelessToStatefullBufferTransformation() &&
        !ctx.getModuleMetaData()->compOpt.GreaterThan4GBBufferRequired &&
        IGC_IS_FLAG_ENABLED(EnableStatelessToStatefull) &&
        !ctx.m_instrTypes.hasInlineAsmPointerAccess)
    {
        bool hasBufOff = (IGC_IS_FLAG_ENABLED(EnableSupportBufferOffset) ||
                            ctx.getModuleMetaData()->compOpt.HasBufferOffsetArg);
        mpm.add(new StatelessToStatefull(hasBufOff));
    }

    // Light cleanup for subroutines after cloning. Note that the constant
    // propogation order is reversed, compared to the opt sequence in
    // OptimizeIR. There is a substantial gain with CFG simplification after
    // interprocedural constant propagation.
    if (ctx.m_enableSubroutine && !isOptDisabled)
    {
        mpm.add(createPruneUnusedArgumentsPass());
        mpm.add(createIPConstantPropagationPass());
        mpm.add(createConstantPropagationPass());
        mpm.add(createDeadCodeEliminationPass());
        mpm.add(createCFGSimplificationPass());
    }
    // Since we don't support switch statements, switch lowering is needed after the last CFG simplication
    mpm.add(llvm::createLowerSwitchPass());

    // Split big vector & 3-element load/store, etc.
    mpm.add(createVectorPreProcessPass());

    // Create Gen IR lowering.
    //   To replace SLM pointer if they are constants, break constants first.
    if (ctx.m_instrTypes.hasLocalLoadStore) {
        mpm.add(new BreakConstantExpr());
    }

    bool KeepGEPs = false;
    mpm.add(createGenIRLowerPass());

    if (KeepGEPs)
    {
        mpm.add(createSeparateConstOffsetFromGEPPass());
    }
    else
    {
        mpm.add(createGEPLoweringPass());
    }

    mpm.add(new WorkaroundAnalysis());
    if (!isOptDisabled)
    {
        // Optimize lower-level IR
        if (!fastCompile)
        {
            mpm.add(createIGCInstructionCombiningPass());
        }
        mpm.add(new GenSpecificPattern());
        if (!fastCompile)
        {
            mpm.add(createEarlyCSEPass());
        }
        if (!fastCompile && IGC_IS_FLAG_ENABLED(allowLICM) && ctx.m_retryManager.AllowLICM())
        {
            mpm.add(createLICMPass());
        }
        mpm.add(createDeadCodeEliminationPass());
    }

    if (IGC_IS_FLAG_ENABLED(ForceHalfPromotion) ||
        (!ctx.platform.supportFP16() && IGC_IS_FLAG_ENABLED(EnableHalfPromotion)))
    {
        mpm.add(new HalfPromotion());
        mpm.add(createGVNPass());
        mpm.add(createDeadCodeEliminationPass());
    }

    // Run type demotion if it's beneficial.
    if (ctx.m_DriverInfo.benefitFromTypeDemotion() &&
        IGC_IS_FLAG_ENABLED(EnableTypeDemotion)) {
        mpm.add(createTypeDemotePass());
    }

    // This pass can create constant expression
    if (ctx.m_DriverInfo.HasDoubleLoadStore())
    {
        mpm.add(new HandleLoadStoreInstructions());
    }

    // Do Genx strengthreduction (do things like fdiv -> inv + mul)
    if (!isOptDisabled)
    {
        mpm.add(createGenStrengthReductionPass());
    }

    if (ctx.m_instrTypes.hasUniformAssumptions) {
        mpm.add(new UniformAssumptions());
    }

    // NanHandlingPass need to be before Legalization since it might make
    // some changes and require Legalization to "legalize"
    if (IGC_IS_FLAG_DISABLED(DisableBranchSwaping) && ctx.m_DriverInfo.BranchSwapping())
    {
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
    if (IGC_IS_FLAG_DISABLED(DisableConstantCoalescing))
    {
        mpm.add(createBreakCriticalEdgesPass());
        mpm.add(new ConstantCoalescing());
    }

    // When needDPEmu is true, enable Emu64Ops as well for now until
    // DPEmu is able to get rid of all 64bit integer ops fully.
    if ((needDPEmu && IGC_IS_FLAG_ENABLED(DPEmuNeedI64Emu)) ||
        (ctx.m_DriverInfo.Enable64BitEmu() &&
            (IGC_GET_FLAG_VALUE(Enable64BitEmulation) ||
            (IGC_GET_FLAG_VALUE(Enable64BitEmulationOnSelectedPlatform) &&
            ctx.platform.need64BitEmulation()))))
    {
        mpm.add(new BreakConstantExpr());

        // Emu64OpsPass requires that we are working on legal types, specifically
        // that i128 uses are expanded to i64. This is why we need to run PeepholeTypeLegalizer
        // beforehand.
        mpm.add(new Legalizer::PeepholeTypeLegalizer());
        // Lower all GEPs now as Emu64 doesn't know how to handle them.
        if (KeepGEPs)
        {
            mpm.add(createGEPLoweringPass());
        }
        // Run dead code elimination pass right before Emu64OpsPass,
        // as legalization passes do not always clear unused (operating
        // on illegal types) instructions.
        mpm.add(llvm::createDeadCodeEliminationPass());
        mpm.add(createEmu64OpsPass());
        ctx.m_hasEmu64BitInsts = true;
    }

    mpm.add(IGCLLVM::createInstSimplifyLegacyPass());
    // This pass inserts bitcasts for vector loads/stores.
    // This pass could be moved further toward EmitPass.
    mpm.add(createVectorProcessPass());

    // handling constant expressions created by vectorProcess pass
    mpm.add(new BreakConstantExpr());

    mpm.add(new LowPrecisionOpt());


    // 3D input/output lowering
    switch (ctx.type)
    {
    case ShaderType::GEOMETRY_SHADER:
        mpm.add(createGeometryShaderLoweringPass());
        break;

    case ShaderType::PIXEL_SHADER:
        mpm.add(new PixelShaderLowering());
        break;

    case ShaderType::VERTEX_SHADER:
        mpm.add(new VertexShaderLowering());
        break;

    case ShaderType::HULL_SHADER:
        mpm.add(createHullShaderLoweringPass());
        mpm.add(new GenSpecificPattern());
        break;

    case ShaderType::DOMAIN_SHADER:
        mpm.add(createDomainShaderLoweringPass());
        break;
    case ShaderType::COMPUTE_SHADER:
        mpm.add(CreateComputeShaderLowering());
        break;
    default:
        break;
    }

    mpm.add(new WAFMinFMax());

    mpm.add(createTimeStatsCounterPass(&ctx, TIME_CG_Legalization, STATS_COUNTER_END));

    COMPILER_TIME_END(&ctx, TIME_CG_Add_Legalization_Passes);
} // AddLegalizationPasses

static void AddCodeGenPasses(
    CodeGenContext& ctx,
    CShaderProgram::KernelShaderMap& shaders,
    IGCPassManager& Passes,
    SIMDMode simdMode,
    bool canAbortOnSpill,
    ShaderDispatchMode shaderMode = ShaderDispatchMode::NOT_APPLICABLE,
    PSSignature* pSignature = nullptr)
{
    // Generate CISA
    COMPILER_TIME_START(&ctx, TIME_CG_Add_CodeGen_Passes);
    Passes.add(new EmitPass(shaders, simdMode, canAbortOnSpill, shaderMode, pSignature));
    COMPILER_TIME_END(&ctx, TIME_CG_Add_CodeGen_Passes);
}

template<typename ContextType>
void CodeGen(ContextType* ctx, CShaderProgram::KernelShaderMap& shaders);

template<>
void CodeGen(DomainShaderContext* ctx, CShaderProgram::KernelShaderMap& shaders)
{
    COMPILER_TIME_START(ctx, TIME_CodeGen);
    COMPILER_TIME_START(ctx, TIME_CG_Add_Passes);

    IGCPassManager Passes(ctx, "CG");

    AddLegalizationPasses(*ctx, Passes);

    AddAnalysisPasses(*ctx, Passes);

    AddCodeGenPasses(*ctx, shaders, Passes, ctx->platform.getMinDispatchMode(), false, ShaderDispatchMode::SINGLE_PATCH);

    if (DSDualPatchEnabled(ctx))
    {
        AddCodeGenPasses(*ctx, shaders, Passes, ctx->platform.getMinDispatchMode(), false, ShaderDispatchMode::DUAL_PATCH);
    }

    COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);

    Passes.run(*(ctx->getModule()));
    DumpLLVMIR(ctx, "codegen");

    COMPILER_TIME_END(ctx, TIME_CodeGen);
}

// check based on performance measures.
static bool SimdEarlyCheck(CodeGenContext* ctx)
{
    if (ctx->m_sampler < 11 || ctx->m_inputCount < 16 || ctx->m_tempCount < 40 || ctx->m_dxbcCount < 280 || ctx->m_ConstantBufferCount < 500)
    {
        if (ctx->m_tempCount < 90 && ctx->m_ConstantBufferCount < 200)
        {
            return true;
        }
    }
    return false;
}


static void PSCodeGen(
    PixelShaderContext* ctx,
    CShaderProgram::KernelShaderMap& shaders,
    PSSignature* pSignature = nullptr)
{
    COMPILER_TIME_START(ctx, TIME_CodeGen);

    IGCPassManager PassMgr(ctx, "CG");

    COMPILER_TIME_START(ctx, TIME_CG_Add_Passes);
    AddLegalizationPasses(*ctx, PassMgr, pSignature);
    AddAnalysisPasses(*ctx, PassMgr);

    const PixelShaderInfo& psInfo = ctx->getModuleMetaData()->psInfo;
    bool useRegKeySimd = false;
    uint32_t pixelShaderSIMDMode =
        ctx->getCompilerOption().forcePixelShaderSIMDMode;
    bool earlyExit =
        ctx->getCompilerOption().pixelShaderDoNotAbortOnSpill ? false : true;

    // for versioned loop, in general SIMD16 with spill has better perf
    bool earlyExit16 = psInfo.hasVersionedLoop ? false : earlyExit;
    bool enableSimd32 = false;

    if (psInfo.ForceEnableSimd32) // UMD forced compilation of simd32.
    {
        enableSimd32 = true;
    }
    // heuristic based on performance measures.
    else if (SimdEarlyCheck(ctx))
    {
        enableSimd32 = true;
    }

    if (IGC_IS_FLAG_ENABLED(ForceBestSIMD))
    {
        if (enableSimd32)
        {
            AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD16, true, ShaderDispatchMode::NOT_APPLICABLE, pSignature);
        }
        AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD8, !ctx->m_retryManager.IsLastTry(), ShaderDispatchMode::NOT_APPLICABLE, pSignature);
        useRegKeySimd = true;
    }
    else if (IsStage1BestPerf(ctx->m_CgFlag, ctx->m_StagingCtx))
    {
        // don't retry SIMD16 for ForcePSBestSIMD
        if (enableSimd32 || IGC_GET_FLAG_VALUE(SkipTREarlyExitCheck))
        {
            AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD16, earlyExit16, ShaderDispatchMode::NOT_APPLICABLE, pSignature);
        }
        AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD8, !ctx->m_retryManager.IsLastTry(), ShaderDispatchMode::NOT_APPLICABLE, pSignature);
        useRegKeySimd = true;
    }
    else if (IsStage1FastCompile(ctx->m_CgFlag, ctx->m_StagingCtx))
    {
        AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD8, false, ShaderDispatchMode::NOT_APPLICABLE, pSignature);
        useRegKeySimd = true;
    }
    else
    {

        if (pixelShaderSIMDMode & FLAG_PS_SIMD_MODE_FORCE_SIMD8)
        {
            AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD8, false, ShaderDispatchMode::NOT_APPLICABLE, pSignature);
            useRegKeySimd = true;
        }

        if (pixelShaderSIMDMode & FLAG_PS_SIMD_MODE_FORCE_SIMD16)
        {
            // if we forceSIMD16 or SIMD16_SIMD32 compilation modes then SIMD16 must compile and cannot abort on spill
            AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD16,
                (earlyExit &&
                (pixelShaderSIMDMode & FLAG_PS_SIMD_MODE_FORCE_SIMD8)), ShaderDispatchMode::NOT_APPLICABLE, pSignature);
            useRegKeySimd = true;
        }

        if (pixelShaderSIMDMode & FLAG_PS_SIMD_MODE_FORCE_SIMD32)
        {
            // if we forceSIMD32 compilation mode then SIMD32 must compile and cannot abort on spill
            AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD32, (earlyExit && (pixelShaderSIMDMode & FLAG_PS_SIMD_MODE_FORCE_SIMD16)), ShaderDispatchMode::NOT_APPLICABLE, pSignature);
            useRegKeySimd = true;
        }
    }

    if (!useRegKeySimd)
    {
        AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD8, !ctx->m_retryManager.IsLastTry(), ShaderDispatchMode::NOT_APPLICABLE, pSignature);

        if (enableSimd32 || IGC_GET_FLAG_VALUE(SkipTREarlyExitCheck))
        {
            AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD16, earlyExit16, ShaderDispatchMode::NOT_APPLICABLE, pSignature);
            AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD32, earlyExit, ShaderDispatchMode::NOT_APPLICABLE, pSignature);
        }
    }

    PassMgr.add(new DebugInfoPass(shaders));
    COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);

    PassMgr.run(*(ctx->getModule()));

    DumpLLVMIR(ctx, "codegen");

    COMPILER_TIME_END(ctx, TIME_CodeGen);
} // PSCodeGen

template<>
void CodeGen(ComputeShaderContext* ctx, CShaderProgram::KernelShaderMap& shaders)
{
    COMPILER_TIME_START(ctx, TIME_CodeGen);
    COMPILER_TIME_START(ctx, TIME_CG_Add_Passes);

    bool setEarlyExit16Stat = false;

    IGCPassManager PassMgr(ctx, "CG");

    AddLegalizationPasses(*ctx, PassMgr);

    AddAnalysisPasses(*ctx, PassMgr);

    SIMDMode simdModeAllowed = ctx->GetLeastSIMDModeAllowed();
    SIMDMode maxSimdMode = ctx->GetMaxSIMDMode();
    unsigned int waveSize = ctx->getModuleMetaData()->csInfo.waveSize;


    if (IGC_IS_FLAG_ENABLED(ForceCSSIMD32) || waveSize == 32 || ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 32)
    {
        AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD32, false);
    }
    else if (((IGC_IS_FLAG_ENABLED(ForceCSSIMD16) || ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 16) && simdModeAllowed <= SIMDMode::SIMD16) ||
        waveSize == 16)
    {
        AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD16, false);
    }
    // csInfo.forcedSIMDSize == 8 means force least SIMD.
    // If the SIMD8 is not allowed, it will return higher SIMD
    else if (IGC_IS_FLAG_ENABLED(ForceCSLeastSIMD) || ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 8)
    {
        AddCodeGenPasses(*ctx, shaders, PassMgr, simdModeAllowed, false);
    }
    else
    {
        static const int SIMD16_NUM_TEMPREG_THRESHOLD = 92;
        static const int SIMD16_SLM_NUM_TEMPREG_THRESHOLD = 128;

        switch (simdModeAllowed)
        {
        case SIMDMode::SIMD8:
        {
            bool allowSpill = false;

            if (maxSimdMode >= SIMDMode::SIMD16)
            {
                if (ctx->m_slmSize)
                {
                    float occu8 = ctx->GetThreadOccupancy(SIMDMode::SIMD8);
                    float occu16 = ctx->GetThreadOccupancy(SIMDMode::SIMD16);
                    //float occu32 = ctx->getThreadOccupancy(SIMDMode::SIMD32);

                    // prefer simd16 over simd8 if same occupancy
                    if (occu16 >= occu8)
                    {
                        allowSpill = true;
                    }
                }

                if (IGC_GET_FLAG_VALUE(CSSpillThresholdNoSLM) > 0)
                {
                    allowSpill = true;
                }
            }

            // if simd16 has better thread occupancy, then allows spills
            unsigned tempThreshold16 = allowSpill
                ? SIMD16_SLM_NUM_TEMPREG_THRESHOLD
                : SIMD16_NUM_TEMPREG_THRESHOLD;

            bool cgSimd16 = maxSimdMode >= SIMDMode::SIMD16 &&
                ctx->m_tempCount <= tempThreshold16;

            bool cgSimd32 = maxSimdMode == SIMDMode::SIMD32 &&
                ctx->m_tempCount <= tempThreshold16;

            if (ctx->m_enableSubroutine || !cgSimd16)
            {
                AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD8,
                    !ctx->m_retryManager.IsLastTry());
                setEarlyExit16Stat = true;
            }
            else
            {
                bool earlyExit = (!allowSpill || ctx->instrStat[SROA_PROMOTED][EXCEED_THRESHOLD]);

                // allow simd16 spill if having SLM
                if (cgSimd16)
                    AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD16, earlyExit);

                if (cgSimd32)
                    AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD32, true);

                AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD8,
                    !ctx->m_retryManager.IsLastTry());
            }
            break;
        }

        case SIMDMode::SIMD16:
        {
            AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD16,
                !ctx->m_retryManager.IsLastTry());
            if (!ctx->m_enableSubroutine && maxSimdMode == SIMDMode::SIMD32)
            {
                AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD32, true);
            }
            break;
        }

        case SIMDMode::SIMD32:
        {
            AddCodeGenPasses(*ctx, shaders, PassMgr, SIMDMode::SIMD32,
                !ctx->m_retryManager.IsLastTry());
            break;
        }

        default:
            IGC_ASSERT_MESSAGE(0, "Unexpected SIMD mode");
        }
    }

    COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);

    PassMgr.run(*(ctx->getModule()));

    if (setEarlyExit16Stat)
        COMPILER_SHADER_STATS_SET(shaders.begin()->second->m_shaderStats, STATS_ISA_EARLYEXIT16, 1);

    DumpLLVMIR(ctx, "codegen");

    COMPILER_TIME_END(ctx, TIME_CodeGen);
} // CodeGen(ComputeShaderContext*,...)

template<typename ContextType>
void CodeGen(ContextType* ctx, CShaderProgram::KernelShaderMap& shaders)
{
    COMPILER_TIME_START(ctx, TIME_CodeGen);
    COMPILER_TIME_START(ctx, TIME_CG_Add_Passes);

    IGCPassManager PassMgr(ctx, "CG");

    AddLegalizationPasses(*ctx, PassMgr);

    AddAnalysisPasses(*ctx, PassMgr);

    AddCodeGenPasses(*ctx, shaders, PassMgr, ctx->platform.getMinDispatchMode(), false);

    COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);

    PassMgr.run(*(ctx->getModule()));
    DumpLLVMIR(ctx, "codegen");

    COMPILER_TIME_END(ctx, TIME_CodeGen);
    MEM_SNAPSHOT(IGC::SMS_AFTER_CODEGEN);
}


template<>
void CodeGen(OpenCLProgramContext* ctx, CShaderProgram::KernelShaderMap& kernels)
{
    COMPILER_TIME_START(ctx, TIME_CodeGen);
    COMPILER_TIME_START(ctx, TIME_CG_Add_Passes);

    IGCPassManager Passes(ctx, "CG");

    AddLegalizationPasses(*ctx, Passes);

    AddAnalysisPasses(*ctx, Passes);

    if (ctx->m_enableFunctionPointer
        && ctx->platform.getMinDispatchMode() == SIMDMode::SIMD8
        && ctx->m_DriverInfo.sendMultipleSIMDModes()
        && ctx->getModuleMetaData()->csInfo.forcedSIMDSize == 0)
    {
        // In order to support compiling multiple SIMD modes for function pointer calls,
        // we require a separate pass manager per SIMD mode, due to interdependencies across
        // function compilations.
        // Only SIMD16 and SIMD8 are supported.

        // Run first pass for SIMD8
        AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD8, false);
        COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);
        Passes.run(*(ctx->getModule()));

        // Create and run second pass for SIMD16
        IGCPassManager Passes2(ctx, "CG2");
        // Add required immutable passes
        Passes2.add(new MetaDataUtilsWrapper(ctx->getMetaDataUtils(), ctx->getModuleMetaData()));
        Passes2.add(new CodeGenContextWrapper(ctx));
        Passes2.add(createGenXFunctionGroupAnalysisPass());
        AddCodeGenPasses(*ctx, kernels, Passes2, SIMDMode::SIMD16, false);
        Passes2.run(*(ctx->getModule()));

        COMPILER_TIME_END(ctx, TIME_CodeGen);
        DumpLLVMIR(ctx, "codegen");
        return;
    }

    if (ctx->m_DriverInfo.sendMultipleSIMDModes())
    {
        unsigned int leastSIMD = 8;
        if (ctx->getModuleMetaData()->csInfo.maxWorkGroupSize)
        {
            const SIMDMode leastSIMDMode = getLeastSIMDAllowed(ctx->getModuleMetaData()->csInfo.maxWorkGroupSize, GetHwThreadsPerWG(ctx->platform));
            leastSIMD = numLanes(leastSIMDMode);
        }
        if (ctx->getModuleMetaData()->csInfo.forcedSIMDSize)
        {
            IGC_ASSERT_MESSAGE((ctx->getModuleMetaData()->csInfo.forcedSIMDSize >= leastSIMD), "Incorrect SIMD forced");
        }
        if (leastSIMD <= 8)
        {
            AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD8, false);
            AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD16, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 16));
            AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD32, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 32));
        }
        else if (leastSIMD <= 16)
        {
            AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD16, false);
            AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD32, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 32));
        }
        else
        {
            AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD32, false);
        }
    }
    else
    {
        {
            // The order in which we call AddCodeGenPasses matters, please to not change order
            AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD32, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 32));
            AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD16, (ctx->getModuleMetaData()->csInfo.forcedSIMDSize != 16));
            AddCodeGenPasses(*ctx, kernels, Passes, SIMDMode::SIMD8, false);
        }
    }

    Passes.add(new DebugInfoPass(kernels));
    COMPILER_TIME_END(ctx, TIME_CG_Add_Passes);

    Passes.run(*(ctx->getModule()));
    COMPILER_TIME_END(ctx, TIME_CodeGen);
    DumpLLVMIR(ctx, "codegen");
} // CodeGen(OpenCLProgramContext*

static void destroyShaderMap(CShaderProgram::KernelShaderMap& shaders)
{
    for (auto i : shaders)
    {
        CShaderProgram* shader = i.second;
        COMPILER_SHADER_STATS_PRINT(shader->m_shaderStats,
            shader->GetContext()->type, shader->GetContext()->hash, "");
        COMPILER_SHADER_STATS_SUM(shader->GetContext()->m_sumShaderStats,
            shader->m_shaderStats, shader->GetContext()->type);
        COMPILER_SHADER_STATS_DEL(shader->m_shaderStats);
        delete shader;
    }
}

template<typename ContextType>
void FillProgram(ContextType* ctx, CShaderProgram* shaderProgram)
{
    shaderProgram->FillProgram(&ctx->programOutput);
}


template<typename ContextType>
void CodeGenCommon(ContextType* ctx)
{
CShaderProgram::KernelShaderMap shaders;


    CodeGen(ctx, shaders);

    IGCPassManager DIPass(ctx, "DI");
    DIPass.add(new DebugInfoPass(shaders));
    DIPass.run(*(ctx->getModule()));

    // gather data to send back to the driver
    for (auto& kv : shaders)
    {
        CShaderProgram* shaderProgram = kv.second;
        FillProgram(ctx, shaderProgram);
    }


    destroyShaderMap(shaders);
}
// CodeGenCommon

void CodeGen(ComputeShaderContext* ctx)
{
    CodeGenCommon(ctx);
}
void CodeGen(DomainShaderContext* ctx)
{
    CodeGenCommon(ctx);
}
void CodeGen(HullShaderContext* ctx)
{
    CodeGenCommon(ctx);
}
void CodeGen(VertexShaderContext* ctx)
{
    CodeGenCommon(ctx);
}
void CodeGen(GeometryShaderContext* ctx)
{
    CodeGenCommon(ctx);
}

void CodeGen(PixelShaderContext* ctx, CShaderProgram::KernelShaderMap& shaders, PSSignature* pSignature)
{

    PSCodeGen(ctx, shaders, pSignature);

} // CodeGen(PixelShaderContext*, ...)

void CodeGen(OpenCLProgramContext* ctx, CShaderProgram::KernelShaderMap& shaders)
{
    CodeGen<OpenCLProgramContext>(ctx, shaders);
}

void unify_opt_PreProcess(CodeGenContext* pContext)
{
    TODO("hasBuiltin should be calculated based on module");
    if (IGC_IS_FLAG_ENABLED(DisableLLVMGenericOptimizations))
    {
        pContext->getModuleMetaData()->compOpt.OptDisable = true;
    }

    IGCPassManager mpm(pContext, "OPTPre");
    mpm.add(new CheckInstrTypes(&(pContext->m_instrTypes)));

    if (pContext->isPOSH())
    {
        mpm.add(createRemoveNonPositionOutputPass());
    }

    mpm.run(*pContext->getModule());

    // If the module does not contain called function declaration,
    // indirect calls are the only way to detect function pointers usage.
    if (pContext->m_instrTypes.hasIndirectCall)
        pContext->m_enableFunctionPointer = true;

    if (pContext->getMetaDataUtils()->size_FunctionsInfo() == 1 &&
        !pContext->m_instrTypes.hasSubroutines)
    {
        pContext->m_instrTypes.numBB =
            pContext->getMetaDataUtils()->begin_FunctionsInfo()->first->getBasicBlockList().size();
        pContext->m_instrTypes.hasMultipleBB = (pContext->m_instrTypes.numBB != 1);
    }
    else
    {
        pContext->m_instrTypes.hasMultipleBB = true;
    }

    pContext->m_instrTypes.hasLoadStore = true;

    pContext->m_instrTypes.CorrelatedValuePropagationEnable =
        (pContext->m_instrTypes.hasMultipleBB &&
        (pContext->m_instrTypes.hasSel ||
        pContext->m_instrTypes.hasCmp ||
        pContext->m_instrTypes.hasSwitch ||
            pContext->m_instrTypes.hasLoadStore));
}

static bool extensiveShader(CodeGenContext* pContext)
{
    return (pContext->type == ShaderType::OPENCL_SHADER &&
        pContext->m_instrTypes.numInsts > INST_THRESHOLD &&
        pContext->m_instrTypes.numLoopInsts > LOOP_INST_THRESHOLD &&
        pContext->m_instrTypes.numOfLoop > LOOP_NUM_THRESHOLD &&
        pContext->m_instrTypes.numBB == 0 &&
        pContext->m_instrTypes.numSample == 0 &&
        pContext->m_instrTypes.hasSubroutines);
}

// All functions are marked with AlwaysInline attribute. Remove them for
// non-kernels, but keep for kernels when subroutine is enabled.
//
// When we do not run optimizations, we still need to run always inline
// pass, otherwise codegen will fail.
static void purgeInlineAttribute(CodeGenContext* pContext, bool NoOpt)
{
    if (pContext->m_enableSubroutine)
    {
        MetaDataUtils* pMdUtils = pContext->getMetaDataUtils();
        for (auto& F : pContext->getModule()->getFunctionList())
        {
            if (!isEntryFunc(pMdUtils, &F))
            {
                F.removeFnAttr(llvm::Attribute::AlwaysInline);
            }
        }
    }

    // We still inline all functions if there is no optimization phase
    if (NoOpt)
    {
            MetaDataUtils* pMdUtils = pContext->getMetaDataUtils();
        IGCPassManager mpm(pContext, "OPTPost");
        mpm.add(new MetaDataUtilsWrapper(pMdUtils, pContext->getModuleMetaData()));
        mpm.add(new CodeGenContextWrapper(pContext));

        mpm.add(createAlwaysInlinerLegacyPass());
        if (IGC_GET_FLAG_VALUE(FunctionControl) != FLAG_FCALL_FORCE_INLINE)
        {
            mpm.add(new PurgeMetaDataUtils());
        }
        mpm.run(*pContext->getModule());
    }
}


void OptimizeIR(CodeGenContext* const pContext)
{
    IGC_ASSERT(nullptr != pContext);
    MetaDataUtils* pMdUtils = pContext->getMetaDataUtils();
    IGC_ASSERT(nullptr != pContext->getModuleMetaData());
    bool NoOpt = pContext->getModuleMetaData()->compOpt.OptDisable;

    // Remove inline attribute if subroutine is enabled.
    purgeInlineAttribute(pContext, NoOpt);
    if (NoOpt)
    {
        return;
    }

    IGCPassManager mpm(pContext, "OPT");

    if (false == pContext->m_hasLegacyDebugInfo)
    {
        IGC_ASSERT(nullptr != pContext->getModule());
        // IGC_ASSERT(false == llvm::verifyModule(*pContext->getModule()));
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
        DummyPass* dummypass = new DummyPass();
        mpm.add(dummypass);
            TargetIRAnalysis GenTTgetIIRAnalysis([&](const Function& F) {
            GenIntrinsicsTTIImpl GTTI(pContext, dummypass);
            return TargetTransformInfo(GTTI);
        });

        mpm.add(new TargetTransformInfoWrapperPass(GenTTgetIIRAnalysis));
#if defined( _DEBUG ) && !defined( ANDROID )
        // IGC IR Verification pass checks that we get a correct IR after the Unification.
        mpm.add(new VerificationPass());
#endif
        mpm.add(new llvm::TargetLibraryInfoWrapperPass(TLI));
        initializeWIAnalysisPass(*PassRegistry::getPassRegistry());

        // Do inter-procedural constant propagation early.
        if (pContext->m_enableSubroutine)
        {
            // Here, we propagate function attributes across calls.  Remaining
            // function calls that were conservatively marked as 'convergent'
            // in ProcessBuiltinMetaData can have that attribute stripped if
            // possible which potentially allows late stage code sinking of
            // those calls by the instruction combiner.
            mpm.add(createPostOrderFunctionAttrsLegacyPass());
            mpm.add(createConstantPropagationPass());
            mpm.add(createIPConstantPropagationPass());
        }

        // enable this only when Pooled EU is not supported
        if (IGC_IS_FLAG_ENABLED(EnableThreadCombiningOpt) &&
            (pContext->type == ShaderType::COMPUTE_SHADER) &&
            !pContext->platform.supportPooledEU() &&
            pContext->platform.supportsThreadCombining())
        {
            initializePostDominatorTreeWrapperPassPass(*PassRegistry::getPassRegistry());
            mpm.add(new ThreadCombining());
            mpm.add(createAlwaysInlinerLegacyPass());
            mpm.add(createPromoteMemoryToRegisterPass());
        }

        if (!IGC_IS_FLAG_ENABLED(DisableDynamicTextureFolding) &&
            pContext->getModuleMetaData()->inlineDynTextures.size() > 0)
        {
            mpm.add(new DynamicTextureFolding());
        }

        if (IGC_IS_FLAG_ENABLED(EnableSLMConstProp) &&
            pContext->type == ShaderType::COMPUTE_SHADER)
        {
            mpm.add(createSLMConstPropPass());
        }

        if (pContext->m_DriverInfo.CodeSinkingBeforeCFGSimplification())
        {
            mpm.add(new CodeSinking(true));
        }
        mpm.add(llvm::createCFGSimplificationPass());

        mpm.add(llvm::createBasicAAWrapperPass());
        mpm.add(createAddressSpaceAAWrapperPass());
        mpm.add(createExternalAAWrapperPass(&addAddressSpaceAAResult));

        if (pContext->m_instrTypes.hasLoadStore)
        {
            mpm.add(llvm::createDeadStoreEliminationPass());
            mpm.add(createMarkReadOnlyLoadPass());
        }

        mpm.add(createLogicalAndToBranchPass());
        mpm.add(llvm::createEarlyCSEPass());

        if (pContext->m_instrTypes.CorrelatedValuePropagationEnable)
        {
            mpm.add(llvm::createCorrelatedValuePropagationPass());
        }

        mpm.add(new BreakConstantExpr());
        mpm.add(new IGCConstProp(!pContext->m_DriverInfo.SupportsPreciseMath()));

        mpm.add(new CustomSafeOptPass());
        if (!pContext->m_DriverInfo.WADisableCustomPass())
        {
            mpm.add(new CustomUnsafeOptPass());
        }

        if (IGC_IS_FLAG_ENABLED(EmulateFDIV))
        {
            mpm.add(createGenFDIVEmulation());
        }

        mpm.add(createIGCInstructionCombiningPass());
        mpm.add(new FCmpPaternMatch());
        mpm.add(llvm::createDeadCodeEliminationPass()); // this should be done both before/after constant propagation

        if (pContext->m_instrTypes.hasGenericAddressSpacePointers &&
            IGC_IS_FLAG_ENABLED(EnableGASResolver))
        {
            mpm.add(createFixAddrSpaceCastPass());
            mpm.add(createResolveGASPass());
            mpm.add(createSROAPass());
        }

        if (IGC_IS_FLAG_ENABLED(SampleMultiversioning) || pContext->m_enableSampleMultiversioning)
        {
            mpm.add(new SampleMultiversioning(pContext));
        }

        if (pContext->m_instrTypes.hasMultipleBB)
        {
            // disable loop unroll for excessive large shaders
            if (pContext->m_instrTypes.hasLoop)
            {
                mpm.add(createLoopDeadCodeEliminationPass());
                mpm.add(createLoopCanonicalization());
                mpm.add(llvm::createLoopDeletionPass());
                mpm.add(llvm::createBreakCriticalEdgesPass());
                mpm.add(llvm::createLoopRotatePass(LOOP_ROTATION_HEADER_INST_THRESHOLD));
                mpm.add(llvm::createLCSSAPass());
                mpm.add(llvm::createLoopSimplifyPass());

                if (pContext->m_retryManager.AllowLICM() && IGC_IS_FLAG_ENABLED(allowLICM))
                {
                    mpm.add(llvm::createLICMPass());
                    mpm.add(llvm::createLICMPass());
                }

                mpm.add(CreateHoistFMulInLoopPass());

                if (!pContext->m_retryManager.IsFirstTry())
                {
                    mpm.add(new DisableLoopUnrollOnRetry());
                }

                if (IGC_IS_FLAG_ENABLED(EnableCustomLoopVersioning) &&
                    pContext->type == ShaderType::PIXEL_SHADER)
                {
                    // custom loop versioning relies on LCSSA form
                    mpm.add(new CustomLoopVersioning());
                }

                mpm.add(createIGCInstructionCombiningPass());
                if (IGC_IS_FLAG_ENABLED(EnableAdvCodeMotion) &&
                    pContext->type == ShaderType::OPENCL_SHADER &&
                    !pContext->m_instrTypes.hasSwitch)
                {
                    mpm.add(createAdvCodeMotionPass(IGC_GET_FLAG_VALUE(AdvCodeMotionControl)));
                }

                int LoopUnrollThreshold = pContext->m_DriverInfo.GetLoopUnrollThreshold();

                // override the LoopUnrollThreshold if the registry key is set
                if (IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold) != 0)
                {
                    LoopUnrollThreshold = IGC_GET_FLAG_VALUE(SetLoopUnrollThreshold);
                }

                if (LoopUnrollThreshold > 0 && !IGC_IS_FLAG_ENABLED(DisableLoopUnroll))
                {
                    mpm.add(IGCLLVM::createLoopUnrollPass());
                }

                // Due to what looks like a bug in LICM, we need to break the LoopPassManager between
                // LoopUnroll and LICM.
                mpm.add(createBarrierNoopPass());

                if (pContext->m_retryManager.AllowLICM() && IGC_IS_FLAG_ENABLED(allowLICM))
                {
                    mpm.add(llvm::createLICMPass());
                }

                // Second unrolling with the same threshold.
                if (LoopUnrollThreshold > 0 && !IGC_IS_FLAG_ENABLED(DisableLoopUnroll))
                {
                    mpm.add(IGCLLVM::createLoopUnrollPass());
                }

                if (!extensiveShader(pContext) && pContext->m_instrTypes.hasNonPrimitiveAlloca)
                {
                        if (pContext->m_DriverInfo.NeedCountSROA())
                    {
                        mpm.add(new InstrStatitic(pContext, SROA_PROMOTED, InstrStatStage::BEGIN, 300));
                        mpm.add(createSROAPass());
                        mpm.add(new InstrStatitic(pContext, SROA_PROMOTED, InstrStatStage::END, 300));
                    }
                    else
                    {
                        mpm.add(createSROAPass());
                    }
                }
            }

            // Note: call reassociation pass before IGCConstProp(EnableSimplifyGEP) to preserve the
            // the expr evaluation order that IGCConstProp creates.
            //
            // Do not apply reordering on VS as CustomUnsafeOptPass does.
            //
            if (IGC_IS_FLAG_ENABLED(EnableReasso) &&
                (pContext->type != ShaderType::VERTEX_SHADER))
            {
                mpm.add(createReassociatePass());
            }

            if (IGC_IS_FLAG_ENABLED(EnableGVN))
            {
                mpm.add(llvm::createGVNPass());
            }
            mpm.add(createGenOptLegalizer());

            mpm.add(llvm::createSCCPPass());

            mpm.add(llvm::createDeadCodeEliminationPass());
            if (!extensiveShader(pContext))
                mpm.add(llvm::createAggressiveDCEPass());

            mpm.add(new BreakConstantExpr());
            mpm.add(new IGCConstProp(!pContext->m_DriverInfo.SupportsPreciseMath(),
                IGC_IS_FLAG_ENABLED(EnableSimplifyGEP)));

            if (IGC_IS_FLAG_DISABLED(DisableImmConstantOpt))
            {
                mpm.add(createIGCIndirectICBPropagaionPass());
            }

            mpm.add(new GenUpdateCB());

            if (!pContext->m_instrTypes.hasAtomics && !extensiveShader(pContext))
            {
                // jump threading currently causes the atomic_flag test from c11 conformance to fail.  Right now,
                // only do jump threading if we don't have atomics as using atomics as locks seems to be the most common
                // case of violating the no independent forward progress clause from the spec.
                mpm.add(llvm::createJumpThreadingPass());
            }
            mpm.add(llvm::createCFGSimplificationPass());
            mpm.add(llvm::createEarlyCSEPass());
            if (pContext->m_instrTypes.hasNonPrimitiveAlloca)
            {
                // run custom safe opts to potentially get rid of indirect
                // addressing of private arrays, see visitLoadInst
                mpm.add(new CustomSafeOptPass());
                mpm.add(createSROAPass());
            }

            // Use CFGSimplification to do clean-up. Needs to be invoked before lowerSwitch.
            mpm.add(llvm::createCFGSimplificationPass());

            if (IGC_IS_FLAG_DISABLED(DisableFlattenSmallSwitch))
            {
                mpm.add(createFlattenSmallSwitchPass());
            }
            //some optimization can create switch statement we don't support
            mpm.add(llvm::createLowerSwitchPass());
            // After lowering 'switch', run jump threading to remove redundant jumps.
            mpm.add(llvm::createJumpThreadingPass());

            // run instruction combining to clean up the code after CFG optimizations
            mpm.add(createIGCInstructionCombiningPass());

            mpm.add(llvm::createDeadCodeEliminationPass());
            mpm.add(llvm::createEarlyCSEPass());

            if (pContext->type == ShaderType::COMPUTE_SHADER)
            {
                mpm.add(CreateEarlyOutPatternsPass());
            }
            if (pContext->type == ShaderType::PIXEL_SHADER)
            {
                // insert early output in case sampleC returns 0
                mpm.add(new CodeSinking(true));
                mpm.add(CreateEarlyOutPatternsPass());
                mpm.add(createBlendToDiscardPass());
            }
            mpm.add(new CustomSafeOptPass());
            if (!pContext->m_DriverInfo.WADisableCustomPass())
            {
                mpm.add(new CustomUnsafeOptPass());
            }
        }
        else
        {
            if (IGC_IS_FLAG_DISABLED(DisableImmConstantOpt))
            {
                mpm.add(createIGCIndirectICBPropagaionPass());
            }

            if (pContext->type == ShaderType::PIXEL_SHADER ||
                pContext->type == ShaderType::COMPUTE_SHADER)
            {
                mpm.add(CreateEarlyOutPatternsPass());
            }
            if (pContext->type == ShaderType::PIXEL_SHADER)
            {
                mpm.add(createBlendToDiscardPass());
            }

            //single basic block
            if (!pContext->m_DriverInfo.WADisableCustomPass())
            {
                mpm.add(llvm::createEarlyCSEPass());
                mpm.add(new CustomSafeOptPass());
                mpm.add(new CustomUnsafeOptPass());
            }
            mpm.add(createGenOptLegalizer());
        }

        if (IGC_GET_FLAG_VALUE(FunctionControl) == FLAG_FCALL_DEFAULT)
        {
            if (pContext->m_enableSubroutine)
            {
                mpm.add(createEstimateFunctionSizePass(EstimateFunctionSize::AL_Kernel));
                mpm.add(createSubroutineInlinerPass());
            }
            else
            {
                // Inline all remaining functions with always inline attribute.
                mpm.add(createAlwaysInlinerLegacyPass());
            }

            if ((pContext->m_DriverInfo.NeedExtraPassesAfterAlwaysInlinerPass() || pContext->m_enableSubroutine)
                && pContext->m_instrTypes.hasNonPrimitiveAlloca)
            {
                mpm.add(createSROAPass());
            }
        }
#if LLVM_VERSION_MAJOR >= 7
        mpm.add(new TrivialLocalMemoryOpsElimination());
#endif
        mpm.add(createGenSimplificationPass());

        if (pContext->m_instrTypes.hasLoadStore)
        {
            mpm.add(llvm::createDeadStoreEliminationPass());
            mpm.add(llvm::createMemCpyOptPass());
            mpm.add(createLdShrinkPass());
        }

        mpm.add(llvm::createDeadCodeEliminationPass());

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

        mpm.add(CreateMCSOptimization());

        if (pContext->type == ShaderType::GEOMETRY_SHADER)
            mpm.add(createRectListOptimizationPass());

        mpm.add(CreateGatingSimilarSamples());

        if (IGC_GET_FLAG_VALUE(FunctionControl) != FLAG_FCALL_FORCE_INLINE)
        {
            mpm.add(new PurgeMetaDataUtils());
        }

        if (!IGC_IS_FLAG_ENABLED(DisableDynamicConstantFolding) &&
            pContext->getModuleMetaData()->inlineDynConstants.empty())
        {
            mpm.add(new FindInterestingConstants());
        }
        // mpm.add(llvm::createDeadCodeEliminationPass()); // this should be done both before/after constant propagation

        if (pContext->m_instrTypes.hasLoop)
        {
            mpm.add(createDeadPHINodeEliminationPass());
        }

        mpm.run(*pContext->getModule());
    } // end scope
    COMPILER_TIME_END(pContext, TIME_OptimizationPasses);

    //pContext->shaderEntry->viewCFG();
    DumpLLVMIR(pContext, "optimized");
    MEM_SNAPSHOT(IGC::SMS_AFTER_OPTIMIZER);
} // OptimizeIR

}  // namespace IGC
