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
#pragma once

namespace llvm {
    class PassRegistry;
}

// The following declarations are placed according to alphabetic order for simplicity
void initializeAddImplicitArgsPass(llvm::PassRegistry&);
void initializeAddressSpaceAliasAnalysisPass(llvm::PassRegistry&);
void initializeAnnotateUniformAllocasPass(llvm::PassRegistry&);
void initializeAggregateArgumentsAnalysisPass(llvm::PassRegistry&);
void initializeAlignmentAnalysisPass(llvm::PassRegistry&);
void initializePreBIImportAnalysisPass(llvm::PassRegistry&);
void initializeBIImportPass(llvm::PassRegistry&);
void initializeBlockCoalescingPass(llvm::PassRegistry&);
void initializeBreakConstantExprPass(llvm::PassRegistry&);
void initializeBuiltinCallGraphAnalysisPass(llvm::PassRegistry&);
void initializeBuiltinsConverterPass(llvm::PassRegistry&);
void initializeCoalescingEnginePass(llvm::PassRegistry&);
void initializeCodeGenContextWrapperPass(llvm::PassRegistry&);
void initializeCodeGenPatternMatchPass(llvm::PassRegistry&);
void initializeCollectGeometryShaderPropertiesPass(llvm::PassRegistry&);
void initializeConstantCoalescingPass(llvm::PassRegistry&);
void initializeCorrectlyRoundedDivSqrtPass(llvm::PassRegistry&);
void initializeCustomSafeOptPassPass(llvm::PassRegistry&);
void initializeCustomUnsafeOptPassPass(llvm::PassRegistry&);
void initializeHoistFMulInLoopPassPass(llvm::PassRegistry&);
void initializeHandleFRemInstructionsPass(llvm::PassRegistry&);
void initializeDeSSAPass(llvm::PassRegistry&);
void initializeDynamicTextureFoldingPass(llvm::PassRegistry&);
void initializeExtensionArgAnalysisPass(llvm::PassRegistry&);
void initializeExtensionFuncsAnalysisPass(llvm::PassRegistry&);
void initializeExtensionFuncsResolutionPass(llvm::PassRegistry&);
void initializeFindInterestingConstantsPass(llvm::PassRegistry&);
void initializeGenericAddressAnalysisPass(llvm::PassRegistry&);
void initializeGenericAddressDynamicResolutionPass(llvm::PassRegistry&);
void initializeGenIRLoweringPass(llvm::PassRegistry&);
void initializeGEPLoweringPass(llvm::PassRegistry&);
void initializeGenSpecificPatternPass(llvm::PassRegistry&);
void initializeGreedyLiveRangeReductionPass(llvm::PassRegistry&);
void initializeIGCIndirectICBPropagaionPass(llvm::PassRegistry&);
void initializeGenUpdateCBPass(llvm::PassRegistry&);
void initializeGenStrengthReductionPass(llvm::PassRegistry&);
void initializeNanHandlingPass(llvm::PassRegistry&);
void initializeFlattenSmallSwitchPass(llvm::PassRegistry&);
void initializeGenXFunctionGroupAnalysisPass(llvm::PassRegistry&);
void initializeGenXCodeGenModulePass(llvm::PassRegistry&);
void initializeEstimateFunctionSizePass(llvm::PassRegistry&);
void initializeSubroutineInlinerPass(llvm::PassRegistry&);
void initializeHandleLoadStoreInstructionsPass(llvm::PassRegistry&);
void initializeIGCConstPropPass(llvm::PassRegistry&);
void initializeGatingSimilarSamplesPass(llvm::PassRegistry&);
void initializeImageFuncResolutionPass(llvm::PassRegistry&);
void initializeImageFuncsAnalysisPass(llvm::PassRegistry&);
void initializeImplicitGlobalIdPass(llvm::PassRegistry&);
void initializeInlineLocalsResolutionPass(llvm::PassRegistry&);
void initializeLegalizationPass(llvm::PassRegistry&);
void initializeLegalizeResourcePointerPass(llvm::PassRegistry&);
void initializeLegalizeFunctionSignaturesPass(llvm::PassRegistry&);
void initializeLiveVarsAnalysisPass(llvm::PassRegistry&);
void initializeLowerGEPForPrivMemPass(llvm::PassRegistry&);
void initializeLowPrecisionOptPass(llvm::PassRegistry&);
void initializeMetaDataUtilsWrapperInitializerPass(llvm::PassRegistry&);
void initializeMetaDataUtilsWrapperPass(llvm::PassRegistry&);
void initializeOpenCLPrintfAnalysisPass(llvm::PassRegistry&);
void initializeOpenCLPrintfResolutionPass(llvm::PassRegistry&);
void initializePositionDepAnalysisPass(llvm::PassRegistry&);
void initializePrivateMemoryResolutionPass(llvm::PassRegistry&);
void initializePrivateMemoryUsageAnalysisPass(llvm::PassRegistry&);
void initializeProcessFuncAttributesPass(llvm::PassRegistry&);
void initializeProcessBuiltinMetaDataPass(llvm::PassRegistry&);
void initializeProgramScopeConstantAnalysisPass(llvm::PassRegistry&);
void initializeProgramScopeConstantResolutionPass(llvm::PassRegistry&);
void initializePromoteResourceToDirectASPass(llvm::PassRegistry&);
void initializePromoteStatelessToBindless(llvm::PassRegistry&);
void initializePullConstantHeuristicsPass(llvm::PassRegistry&);
void initializeScalarizerCodeGenPass(llvm::PassRegistry&);
void initializeReduceLocalPointersPass(llvm::PassRegistry&);
void initializeReplaceUnsupportedIntrinsicsPass(llvm::PassRegistry&);
void initializePreCompiledFuncImportPass(llvm::PassRegistry&);
void initializePurgeMetaDataUtilsPass(llvm::PassRegistry&);
void initializeResolveAggregateArgumentsPass(llvm::PassRegistry&);
void initializeResolveOCLAtomicsPass(llvm::PassRegistry&);
void initializeResourceAllocatorPass(llvm::PassRegistry&);
void initializeScalarizeFunctionPass(llvm::PassRegistry&);
void initializeSimd32ProfitabilityAnalysisPass(llvm::PassRegistry&);
void initializeSetFastMathFlagsPass(llvm::PassRegistry&);
void initializeSPIRMetaDataTranslationPass(llvm::PassRegistry&);
void initializeSubGroupFuncsResolutionPass(llvm::PassRegistry&);
void initializeIndirectCallOptimizationPass(llvm::PassRegistry&);
void initializeVectorPreProcessPass(llvm::PassRegistry&);
void initializeVectorProcessPass(llvm::PassRegistry&);
void initializeVerificationPassPass(llvm::PassRegistry&);
void initializeVolatileWorkaroundPass(llvm::PassRegistry&);
void initializeWGFuncResolutionPass(llvm::PassRegistry&);
void initializeWIAnalysisPass(llvm::PassRegistry&);
void initializeWIFuncResolutionPass(llvm::PassRegistry&);
void initializeWIFuncsAnalysisPass(llvm::PassRegistry&);
void initializeWorkaroundAnalysisPass(llvm::PassRegistry&);
void initializeWAFMinFMaxPass(llvm::PassRegistry&);
void initializePingPongTexturesAnalysisPass(llvm::PassRegistry&);
void initializePingPongTexturesOptPass(llvm::PassRegistry&);
void initializeSampleMultiversioningPass(llvm::PassRegistry&);
void initializeLinkTessControlShaderPass(llvm::PassRegistry&);
void initializeLinkTessControlShaderMCFPass(llvm::PassRegistry&);
void initializeMemOptPass(llvm::PassRegistry&);
void initializePreRASchedulerPass(llvm::PassRegistry&);
void initializeBIFTransformsPass(llvm::PassRegistry&);
void initializeThreadCombiningPass(llvm::PassRegistry&);
void initializeRegisterPressureEstimatePass(llvm::PassRegistry&);
void initializeLivenessAnalysisPass(llvm::PassRegistry&);
void initializeRegisterEstimatorPass(llvm::PassRegistry&);
void initializeVariableReuseAnalysisPass(llvm::PassRegistry&);
void initializeTransformBlocksPass(llvm::PassRegistry&);
void initializeTranslationTablePass(llvm::PassRegistry&);
#if LLVM_VERSION_MAJOR >= 7
void initializeTrivialLocalMemoryOpsEliminationPass(llvm::PassRegistry&);
#endif
void initializeSLMConstPropPass(llvm::PassRegistry&);
void initializeBlendToDiscardPass(llvm::PassRegistry&);
void initializeCheckInstrTypesPass(llvm::PassRegistry&);
void initializeInstrStatisticPass(llvm::PassRegistry&);
void initializeHalfPromotionPass(llvm::PassRegistry&);
void initializeFixFastMathFlagsPass(llvm::PassRegistry&);
void initializeFCmpPaternMatchPass(llvm::PassRegistry&);
void initializeCodeAssumptionPass(llvm::PassRegistry&);
void initializeIGCInstructionCombiningPassPass(llvm::PassRegistry&);
void initializeIntDivConstantReductionPass(llvm::PassRegistry&);
void initializeIntDivRemCombinePass(llvm::PassRegistry&);