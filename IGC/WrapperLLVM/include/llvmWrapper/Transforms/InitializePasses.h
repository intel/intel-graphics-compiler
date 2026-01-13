/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_INITIALIZE_PASSES_H
#define IGCLLVM_TRANSFORMS_INITIALIZE_PASSES_H

namespace llvm {
class PassRegistry;
}

void initializeADCELegacyPassWrapperPass(llvm::PassRegistry &);
void initializeCorrelatedValuePropagationLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeDSELegacyPassWrapperPass(llvm::PassRegistry &);
void initializeJumpThreadingPassWrapperPass(llvm::PassRegistry &);
void initializeMemCpyOptLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeGlobalDCELegacyPassWrapperPass(llvm::PassRegistry &);
void initializeIPSCCPLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeInstructionCombiningPassWrapperPass(llvm::PassRegistry &);
void initializeLoopDeletionLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeLoopIdiomRecognizeLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeLoopLoadEliminationLegacyPassWrapperPass(llvm::PassRegistry &);
void initializePostOrderFunctionAttrsLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeSCCPLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeStripDeadDebugInfoLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeStripDeadPrototypesLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeStripNonLineTableDebugLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeLowerExpectIntrinsicLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeLibCallsShrinkWrapLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeBDCELegacyPassWrapperPass(llvm::PassRegistry &);
void initializeLoopVectorizeLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeWarnMissedTransformsLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeAlignmentFromAssumptionsLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeLoopIdiomRecognizeLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeAnnotation2MetadataLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeForceFunctionAttrsLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeCallSiteSplittingLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeCalledValuePropagationLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeGlobalOptLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeEliminateAvailableExternallyLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeReversePostOrderFunctionAttrsLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeFloat2IntPassWrapperPass(llvm::PassRegistry &);
void initializeConstantMergeLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeLoopDistributeLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeDivRemPairsLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeSLPVectorizerPassWrapperPass(llvm::PassRegistry &);
void initializeVectorCombineLegacyPassWrapperPass(llvm::PassRegistry &);
#endif // IGCLLVM_TRANSFORMS_INITIALIZE_PASSES_H
