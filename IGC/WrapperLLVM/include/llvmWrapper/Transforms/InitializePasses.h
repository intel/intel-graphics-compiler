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
void initializeLoopIdiomRecognizeLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeLoopLoadEliminationLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeSCCPLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeStripDeadDebugInfoLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeStripDeadPrototypesLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeStripNonLineTableDebugLegacyPassWrapperPass(llvm::PassRegistry &);
void initializeLowerExpectIntrinsicLegacyPassWrapperPass(llvm::PassRegistry &);
#endif // IGCLLVM_TRANSFORMS_INITIALIZE_PASSES_H
