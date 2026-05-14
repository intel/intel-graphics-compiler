/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvmWrapper/Transforms/InitializePasses.h"

void initializeAllLLVMWrapperPasses(llvm::PassRegistry &Registry) {
#if LLVM_VERSION_MAJOR >= 16
  initializeADCELegacyPassWrapperPass(Registry);
#endif
  initializeCorrelatedValuePropagationLegacyPassWrapperPass(Registry);
  initializeDSELegacyPassWrapperPass(Registry);
  initializeJumpThreadingPassWrapperPass(Registry);
  initializeMemCpyOptLegacyPassWrapperPass(Registry);
  initializeGlobalDCELegacyPassWrapperPass(Registry);
  initializeIPSCCPLegacyPassWrapperPass(Registry);
  initializeInstructionCombiningPassWrapperPass(Registry);
  initializeLoopDeletionLegacyPassWrapperPass(Registry);
  initializeLoopIdiomRecognizeLegacyPassWrapperPass(Registry);
  initializeLoopLoadEliminationLegacyPassWrapperPass(Registry);
  initializePostOrderFunctionAttrsLegacyPassWrapperPass(Registry);
  initializeSCCPLegacyPassWrapperPass(Registry);
  initializeStripDeadDebugInfoLegacyPassWrapperPass(Registry);
  initializeStripDeadPrototypesLegacyPassWrapperPass(Registry);
  initializeStripNonLineTableDebugLegacyPassWrapperPass(Registry);
  initializeLowerExpectIntrinsicLegacyPassWrapperPass(Registry);
  initializeLibCallsShrinkWrapLegacyPassWrapperPass(Registry);
  initializeBDCELegacyPassWrapperPass(Registry);
  initializeLoopVectorizeLegacyPassWrapperPass(Registry);
  initializeWarnMissedTransformsLegacyPassWrapperPass(Registry);
  initializeAlignmentFromAssumptionsLegacyPassWrapperPass(Registry);
  initializeAnnotation2MetadataLegacyPassWrapperPass(Registry);
  initializeForceFunctionAttrsLegacyPassWrapperPass(Registry);
  initializeCallSiteSplittingLegacyPassWrapperPass(Registry);
  initializeCalledValuePropagationLegacyPassWrapperPass(Registry);
  initializeGlobalOptLegacyPassWrapperPass(Registry);
  initializeEliminateAvailableExternallyLegacyPassWrapperPass(Registry);
  initializeReversePostOrderFunctionAttrsLegacyPassWrapperPass(Registry);
  initializeFloat2IntPassWrapperPass(Registry);
  initializeConstantMergeLegacyPassWrapperPass(Registry);
  initializeLoopDistributeLegacyPassWrapperPass(Registry);
  initializeDivRemPairsLegacyPassWrapperPass(Registry);
  initializeSLPVectorizerPassWrapperPass(Registry);
  initializeVectorCombineLegacyPassWrapperPass(Registry);
  initializeLoopUnrollLegacyPassWrapperPass(Registry);
  initializeLICMLegacyPassWrapperPass(Registry);
  initializeIndVarSimplifyLegacyPassWrapperPass(Registry);
  initializeDemandedBitsLegacyPassWrapperPass(Registry);
  initializeLoopAccessAnalysisLegacyPassWrapperPass(Registry);
  initializeInjectTLIMappingsLegacyPassWrapperPass(Registry);
  initializeSimpleInlinerLegacyPassWrapperPass(Registry);
  initializeLoopRotateLegacyPassWrapperPass(Registry);
  initializeLoopSinkLegacyPassWrapperPass(Registry);
}
